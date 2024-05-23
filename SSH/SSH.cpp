#include "SSH.h"

#include <fcntl.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

#include <libssh/libssh.h>
#include <libssh/sftp.h>


// When compiling on Windows
#ifdef WIN32
    // Define the read, write and execute permission bits
    #define S_IRWXU 0000700
#endif

// Indicates that no Bytes were read after trying to read a SSH channel
#define SSH_NO_BYTES_READ   0

// Indicates that Bytes were read after trying to read a SSH channel
#define SSH_BYTES_READ      1

// Constructor
SSH::SSH() {
    // Initially not connected
    connected = false;

    // Create a new ssh session.
    session = ssh_new();

    // When there was an error creating the session
    if (session == nullptr)
        // Throw an exception
        throw std::runtime_error("Couldn't create the SSH session.");
}

// Destructor
SSH::~SSH() {
    // Disconnect from the SSH client
    disconnect();

    // Deallocate the SSH session handle
    ssh_free(session);
}

int SSH::connect(const char* addr, const char* user, const char* passwd, long timeout) {
    // Call the overloaded function with the default port number 22
    return connect(addr, 22, user, passwd, timeout);
}

int SSH::connect(const char* addr, unsigned int port, const char* user, const char* passwd, long timeout) {
    // Set address, port, and timeout
    ssh_options_set(session, SSH_OPTIONS_HOST, addr);
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    //ssh_options_set(*session, SSH_OPTIONS_USER, "pi");
    ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &timeout);

    // Connect to the client ...
    // If there was a connection error
    if (ssh_connect(session) != SSH_OK) {
        // Print the error message
        std::cerr << ssh_get_error(session) << std::endl;

        return SSH_ERROR;
    }

    // Authentificate with the given username and password ...
    // If the authentification was unsuccessfull (e.g., wrong password)
    if (ssh_userauth_password(session, user, passwd) != SSH_OK) {
        // Print the error message
        std::cerr << ssh_get_error(session) << std::endl;

        // Disconnect from the client
        ssh_disconnect(session);

        return SSH_ERROR;
    }

    // Set an infinite timeout after being connected for blocking commands/calls (timeout = 0)
    const long infinite = 0;
    ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &infinite);

    connected = true;

    return SSH_OK;
}

void SSH::disconnect() {
    // Only disconnect if there is a connection
    if (connected) {
        // Disconnect from the client
        ssh_disconnect(session);

        // Not connected anymore
        connected = false;
    }
}

size_t SSH::write_to_stdout(const char* buffer, size_t count) {
    // Write the buffer to stdout, save the number of written bytes (should be equal to count)
    size_t nbytes = fwrite(buffer, 1, count, stdout);

    // Flush stdout
    fflush(stdout);

    // Return the number of bytes written
    return nbytes;
}

size_t SSH::write_to_stderr(const char* buffer, size_t count) {
    // Write the buffer to stdout, return the number of bytes written
    return fwrite(buffer, 1, count, stderr);
}

int SSH::channel_redirect_output(ssh_channel* channel, enum File from, size_t (*to)(const char*, size_t), unsigned int buffer_size) {
    // Allocate a new array for the bytes to read
    char* buffer = new char[buffer_size];

    // Save if Bytes were read
    bool bytesRead = false;

    // Read data (i.e., answer) of the SSH client from stdout or stderr, save the number of bytes read
    int nbytes = ssh_channel_read(*channel, buffer, buffer_size, from);

    // If Bytes were read
    if (nbytes > 0) {
        // Save that Bytes were read
        bytesRead = true;

        // While Bytes are read
        do {
            // Redirect the data
            to(buffer, nbytes);

            // Try to read more data
            nbytes = ssh_channel_read(*channel, buffer, buffer_size, from);
        } while (nbytes > 0);
    }

    // Free the allocated array
    delete[] buffer;

    // If there was an error while reading
    if (nbytes < 0)
        return SSH_ERROR;

    // Return if bytes were read
    return bytesRead ? SSH_BYTES_READ : SSH_NO_BYTES_READ;
}

int SSH::execute(const char* cmd, unsigned int buffer_size) {
    // Allocate a new channel
    ssh_channel channel = ssh_channel_new(session);

    // If there was an allocation error
    if (channel == nullptr) {
        // Print an error message
        std::cerr << "Couldn't create Channel." << std::endl;

        return SSH_ERROR;
    }

    // Open a session channel (shell) ...
    // If an error occured
    if (ssh_channel_open_session(channel) != SSH_OK) {
        // Print the error message
        std::cerr << ssh_get_error(session) << std::endl;

        // Free the channel
        ssh_channel_free(channel);

        return SSH_ERROR;
    }

    // Run the given shell command cmd (sh -c cmd) ...
    // If there was an error
    if (ssh_channel_request_exec(channel, cmd) != SSH_OK) {
        // Print the error message
        std::cerr << ssh_get_error(session) << std::endl;

        // Close and free the channel
        ssh_channel_close(channel);
        ssh_channel_free(channel);

        return SSH_ERROR;
    }

    // Read from the client's stdout and redirect to host's stdout, check if there was an error
    if (channel_redirect_output(&channel, File::STDOUT, write_to_stdout, buffer_size) == SSH_ERROR) {
        // Print the error message
        std::cerr << ssh_get_error(session) << std::endl;

        // Close and free the channel
        ssh_channel_close(channel);
        ssh_channel_free(channel);

        return SSH_ERROR;
    }

    // Read from the client's stderr and redirect to host's stderr, save the return code
    int rc = channel_redirect_output(&channel, File::STDERR, write_to_stderr, buffer_size);

    // If there was an error reading and redirecting stderr
    if (rc == SSH_ERROR) {
        // Print the error message
        std::cerr << ssh_get_error(session) << std::endl;

        // Close and free the channel
        ssh_channel_close(channel);
        ssh_channel_free(channel);

        return SSH_ERROR;
    }

    // Send an end of file, close and free the channel
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);

    // If bytes were read from stderr (i.e., an error occured executing the command) return SSH_CMD_ERROR, SSH_OK else
    return rc == SSH_BYTES_READ ? SSH_CMD_ERROR : SSH_OK;
}

int SSH::get_project_dirs_and_files(const char* project, const char* path2project, std::list<std::string>* dirs, std::list<std::string>* files) {
    try {
        // Recursively iterate through all folders and files (entries) of the given project
        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(std::format("{}/{}", path2project, project))) {
            // Get the path of the current entry
            std::filesystem::path path = entry.path();

            // Convert the absolute path to a path relative to path2project, get the path as string and append it to the respective directory or file list
            (is_directory(path) ? dirs : files)->push_back(relative(path, path2project).generic_string());
        }
    }
    // If an error, exception occured (e.g., The project is missing and/or path2project is invalid)
    catch (const std::filesystem::filesystem_error& err) {
        // Print the exception message to stderr
        std::cerr << err.what() << std::endl;

        return SSH_ERROR;
    }

    return SSH_OK;
}

int SSH::project_mkdirs(const char* project, std::list<std::string>* dirs) {
    // Create the command for removing and recreating the current sw_workspace folder and creating the project folder
    //std::string cmd = format("rm -rf sw_workspace && mkdir sw_workspace sw_workspace/{}", project);
    std::string cmd = std::format("mkdir -p sw_workspace && rm -rf sw_workspace/{}/* && mkdir -p sw_workspace/{}", project, project);

    // Iterate through all directories of the project
    for (std::string dirPath : *dirs)
        // Append the current directory to the mkdir command
        cmd.append(std::format(" sw_workspace/{}", dirPath));

    // Execute the command, return the return code
    return execute(cmd.c_str());
}

int SSH::project_save_file(sftp_session* sftp, std::string filePath, const char* path2project, unsigned int buffer_size) {
    // Create the local path of the file
    std::string localPath = std::format("{}/{}", path2project, filePath);

    // Open the local file in binary mode
    std::ifstream localFile(localPath, std::ifstream::binary);

    // If the local file couldn't be opened
    if (!localFile.is_open()) {
        // Print an error message to stderr
        std::cerr << "Can't open local file " << localPath << std::endl;

        return SSH_ERROR;
    }

    // Create the remote path of the file
    std::string remotePath = format("sw_workspace/{}", filePath);

    // Open the remote file in write mode, if it doesn't exist yet create it, else truncate it
    // give user read, write and execute permission
    sftp_file remoteFile = sftp_open(*sftp, remotePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

    // If the remote file couldn't be opened
    if (remoteFile == nullptr) {
        // Print the error message to stderr
        std::cerr << "Can't open remote file " << remotePath << ": " << ssh_get_error(session) << std::endl;

        // Close the local file
        localFile.close();

        return SSH_ERROR;
    }

    // Allocate a new array for the Bytes to read
    char* text = new char[buffer_size];

    // While the end of the local file isn't reached
    do {
        // Read the first buffer_size Bytes from the local file
        localFile.read(text, buffer_size);

        // If there was an error reading the local file
        if (localFile.bad()) {
            // Print an error message to stderr
            std::cerr << "Error reading local file " << localPath << std::endl;

            // Close the local and remote file
            localFile.close();
            sftp_close(remoteFile);

            // Free the allocated array
            delete[] text;

            return SSH_ERROR;
        }

        // Get the number of Bytes read
        size_t nbytes = localFile.gcount();

        // Write the Bytes to the remote file, verify that all Bytes have been written
        if (sftp_write(remoteFile, text, nbytes) != nbytes) {
            // Print the error message to stderr
            std::cerr << ssh_get_error(session) << std::endl;

            // Close the local and remote file
            localFile.close();
            sftp_close(remoteFile);

            // Free the allocated array
            delete[] text;

            return SSH_ERROR;
        }
    } while (!localFile.eof());

    // Print the success to stdout
    std::cout << filePath << " was saved successfully." << std::endl;

    // Close the local and remote file
    localFile.close();
    sftp_close(remoteFile);

    // Free the allocated array
    delete[] text;

    return SSH_OK;
}

int SSH::project_save_files(std::list<std::string>* files, const char* path2project) {
    // Create a new SFTP session and allocate a new SFTP channel
    sftp_session sftp = sftp_new(session);

    // If there was an allocation error
    if (sftp == nullptr) {
        // Print the error message to stderr
        std::cerr << "Error allocating SFTP session: " << ssh_get_error(session) << std::endl;

        return SSH_ERROR;
    }

    // Initialize the SFTP protocol with the server
    if (sftp_init(sftp) != SSH_OK) {
        // Print an error message together with the error code
        std::cerr << "Error initializing SFTP session: code " << sftp_get_error(sftp) << std::endl;

        // Close and deallocate the SFTP session
        sftp_free(sftp);

        return SSH_ERROR;
    }

    // Save the return code from the upcoming function calls
    // Initialize with SSH_OK in case there are no files to be saved
    int rc = SSH_OK;

    // Iterate through all file paths
    for (std::string filePath : *files) {
        // Save the file on the SSH client, and save the return code
        rc = project_save_file(&sftp, filePath, path2project);

        // If there was an error saving the last file
        if (rc != SSH_OK)
            break;
    }

    // Close and deallocate the SFTP session
    sftp_free(sftp);

    return rc;
}

int SSH::save(const char* project, const char* path2project) {
    // If path2project isn't specified
    if (path2project == nullptr)
        // Take the current directory
        path2project = ".";

    // Create two lists which save the paths of the directories and files from the project
    std::list<std::string> dirs, files;

    // Get the paths of all directories and paths from the project and store them in the respective lists
    // Save the return code
    int rc = get_project_dirs_and_files(project, path2project, &dirs, &files);

    // If there was an error retrieving the paths
    if (rc != SSH_OK)
        return rc;

    // Create all the directories on the SSH client, save the return code
    rc = project_mkdirs(project, &dirs);

    // If there was an error creating the directories
    if (rc != SSH_OK)
        return rc;

    // Save all files on the SSH client, return the return code
    return project_save_files(&files, path2project);
}

int SSH::compile(const char* project) {
    // Create the command string/script which builds the project
    std::string cmd = std::format(
        // Change to the project directory, create the Debug folder -------------------------------------------------------|
        "cd sw_workspace/{} && (mkdir -p Debug && ("                                                                    // |
                                                                                                                        // |
            // Check if the makefile is there -------------------------------------------------------|                  // |
            "test -f makefile && ("                                                               // |                  // |
                                                                                                  // |                  // |
                // Execute make and create the application output file output.txt ---|            // |                  // |
                "make -s && > Debug/output.txt || "                              // |            // |                  // |
                                                                                  // |            // |                  // |
                // If there was a build error <--------------------------------------|            // |                  // |
                ">&2 echo Build error)) || "                                                       // |                  // |
                                                                                                  // |                  // |
            // If there is no makefile <-------------------------------------------------------------|                  // |
            ">&2 echo Error: The project is missing a makefile) || "                                                    // |
                                                                                                                        // |
        // If the change to the project directory was unsuccessful, the project was probably not saved before <------------|              
        ">&2 echo Have you saved the project before?",
    project);
    
    // Execute the command, save the return code
    int rc = execute(cmd.c_str());

    // If there was an error executing the command
    if (rc != SSH_OK)
        // Return the return code
        return rc;

    // Print the build success to stdout
    std::cout << "Build succeeded." << std::endl;

    return SSH_OK;
}

int SSH::start(const char* project, const char* args) {
    // Create the first part of the command string/script which starts the project
    std::string cmd = std::format(
        // Change to the project directory
        "cd sw_workspace/{} && ("

            // Change to the Debug directory
            "cd Debug && ("
                
                // Check if the previously built project binary is there, invoke a new bash, and start the project binary
                "test -f {} && {{ bash -c './{} ",
    project, project, project);

    // If arguments are given
    if (args != nullptr)
        // Append the arguments to the command string
        cmd.append(args);

    // Append the second and last part of the command string
    cmd.append(std::format(
                // Redirect stdout and stderr from the project binary to the output file output.txt, wait for the termination and append the exit code to output.txt
                // Redirect stdout and stderr from the bash to /dev/null (so that later read calls don't block) and run the bash commands in background
                " &> output.txt; echo \"\nThe Process exited with Code $?\" >> output.txt' &>/dev/null & }} || "

                // If there is no binary, it may has a wrong name or the previous compilation was unsuccessful
                ">&2 echo 'The binary {} of the project is missing\nIs the final target of the makefile called {}?\nWas the compilation of the project successful?') || "

            // If the change to the Debug directory was unsuccessful, it was probably not compiled before
            ">&2 echo 'Have you compiled the project before?') || "

        // If the change to the project directory was unsuccessful, the project was probably not saved before
        ">&2 echo 'Have you saved the project before?'",
    project, project));
    
    // Execute the command, save the return code
    int rc = execute(cmd.c_str());

    // If there was an error executing the command
    if (rc != SSH_OK)
        // Return the return code
        return rc;

    // Print the successful start to stdout
    std::cout << "Application " << project << " started." << std::endl;

    return SSH_OK;
}

int SSH::stop(const char* project) {
    // Create the command which sends a termination signal to the project application
    // If an error occured, the project application is probably not running
    std::string cmd = std::format("killall {} || >&2 echo Are you sure that the Application is running?", project);

    // Execute the command, save the return code
    int rc = execute(cmd.c_str());

    // If there was an error executing the command
    if (rc != SSH_OK)
        // Return the return code
        return rc;

    // Print the successful stop to stdout
    std::cout << "Application " << project << " stopped." << std::endl;

    return SSH_OK;
}

int SSH::isAppRunning(const char* project) {
    // Create the command which gets the PID of the project application and redirect it to /dev/null (so that it isn't printed to stdout because it isn't important)
    // If the command is successful the application of the project is currently running, else not
    std::string com = std::format("pidof {} > /dev/null && echo Application is running. || >&2 echo Application is not running.", project);

    // Execute the command, save the return code
    int rc = execute(com.c_str());

    switch (rc) {
        // If the command was executed successfully the project's application is running
        case SSH_OK:        return SSH_APP_RUNNING;

        // If the command resulted in an error the project's application is not running
        case SSH_CMD_ERROR: return SSH_APP_NOT_RUNNING;

        // Otherwise return the return code
        default:            return rc;
    }
}

int SSH::readOutput_once(const char* project, unsigned int nbytes) {
    // Create the tail command for reading the last nbytes Bytes of the project's output file output.txt once
    std::string tail = std::format("tail -c{} sw_workspace/{}/Debug/output.txt", nbytes, project);

    // Execute the tail command with a buffer size of nbytes, return the return code
    return execute(tail.c_str(), nbytes);
}

int SSH::readOutput_cont(const char* project, unsigned int nbytes) {
    // Create the tail command for reading the last nbytes Bytes of the project's output file output.txt continuously
    std::string tail = std::format("tail -f -c{} sw_workspace/{}/Debug/output.txt", nbytes, project);

    // Execute the tail command with a buffer size of nbytes, return the return code
    return execute(tail.c_str(), nbytes);
}
