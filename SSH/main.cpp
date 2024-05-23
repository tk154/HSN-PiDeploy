#include "SSH.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include <functional>
#include <iostream>
#include <unordered_map>

#define USERNAME "pi"
#define PASSWORD "raspberry"

SSH ssh;

// Disconnect from the SSH client and exit the application
void disconnectAndExit() {
    // Disconnect from the SSH client
    ssh.disconnect();

    // Exit the application
    exit(EXIT_SUCCESS);
}

// Function to be executed by a seperate thread
void* t_func(void* args) {
    // Block here until something is written to stdin
    fgetc(stdin);

    // Disconnect from the SSH client and exit the application
    disconnectAndExit();

    // Suppress the error that the function must return a value
    return nullptr;
}

// Signal Handler
void signalHandler(int sig) {
    // If CTRL+C was pressed
    if (sig == SIGINT)
        // Disconnect from the SSH client and exit the application
        disconnectAndExit();
}

int main(int argc, char* argv[]) {
    // Save the pointer from the arguments which points to the host name
    char* hostName = argv[1];

    // If no host name is given
    if (hostName == nullptr) {
        // Print an error message to stderr
        std::cerr << "Error: No host name given" << std::endl;

        // Exit with an error code
        return SSH_ERROR;
    }

    // Save the pointer from the arguments which points to the command
    char* command = argv[2];

    // If no command is given
    if (command == nullptr) {
        // Print an error message to stderr
        std::cerr << "Error: No command given" << std::endl;

        // Exit with an error code
        return SSH_ERROR;
    }

    // Save the pointer from the arguments which points to the project (name)
    char* project = argv[3];

    // If no project is given
    if (project == nullptr) {
        // Print an error message to stderr
        std::cerr << "Error: No project given" << std::endl;

        // Exit with an error code
        return SSH_ERROR;
    }

    // Save the pointer where the optional arguments start
    char** optArgs = &argv[4];

    // Create a map to assign the command string to the function which should be executed later
    // The functions are lambda expressions to deal with different parameter signatures and default values
    std::unordered_map<std::string, std::function<int()>> str2func = {
        { "status",          [project]()          { return ssh.isAppRunning   (project);             } },
        { "execute",         [project]()          { return ssh.execute        (project);             } },
        { "save",            [project, optArgs]() { return ssh.save           (project, optArgs[0]); } },
        { "compile",         [project]()          { return ssh.compile        (project);             } },
        { "start",           [project, optArgs]() { return ssh.start          (project, optArgs[0]); } },
        { "stop",            [project]()          { return ssh.stop           (project);             } },
        { "readOutput_once", [project]()          { return ssh.readOutput_once(project);             } },
        { "readOutput_cont", [project]()          { return ssh.readOutput_cont(project);             } }
    };

    // Save the function which should be executed as a function object to call it later
    std::function<int()> execFunc = str2func[command];

    // If the command wasn't found inside the HashMap
    if (!execFunc) {
        // Print an error message to stderr
        std::cerr << "Unknown command: " << command << std::endl;

        // Exit with an error code
        return SSH_ERROR;
    }

    // Connect to the SSH client with the given address, username and password
    // Save the return code
    int rc = ssh.connect(hostName, USERNAME, PASSWORD);

    // If there was an error while connecting to the SSH client
    if (rc != SSH_OK)
        // Exit with the error code
        return rc;

    // Create a thread which waits for input from stdin
    pthread_t thread;
    pthread_create(&thread, nullptr, t_func, nullptr);

    // If CTRL+C was pressed call the signal handler
    signal(SIGINT, signalHandler);
    
    // Execute the previously saved function, save the return code
    rc = execFunc();

    // Terminate the thread
    pthread_cancel(thread);

    // Disconnect from the SSH client
    ssh.disconnect();

    // Exit with the saved return code
    return rc;
}
