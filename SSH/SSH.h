#pragma once

#include <list>
#include <string>

#define SSH_OK				0
#define SSH_ERROR		   -1
#define SSH_CMD_ERROR	   -2

#define SSH_APP_RUNNING		1
#define SSH_APP_NOT_RUNNING 2

typedef struct ssh_session_struct*  ssh_session;
typedef struct ssh_channel_struct*	ssh_channel;
typedef struct sftp_session_struct* sftp_session;

/// <summary>
/// Class for a SSH client
/// </summary>
class SSH {

public:
	/// <summary>
	/// Constructor for initialization
	/// </summary>
	SSH();

	/// <summary>
	/// Destructor automatically diconnects and frees allocated space
	/// </summary>
	~SSH();

	/// <summary>
	/// Connect a SSH client with the default port number 22
	/// </summary>
	/// <param name="addr">Address of the SSH client</param>
	/// <param name="user">Username of the SSH client</param>
	/// <param name="passwd">Password of the SSH client</param>
	/// <param name="timeout">Optionnal, timeout for the connection in seconds (default: 5)</param>
	/// <returns>SSH_OK on success, SSH_ERROR on error</returns>
	int connect(const char* addr, const char* user, const char* passwd, long timeout = 5);

	/// <summary>
	/// Connect a SSH client
	/// </summary>
	/// <param name="addr">Address of the SSH client</param>
	/// <param name="user">Username of the SSH client</param>
	/// <param name="port">Port of the SSH client</param>
	/// <param name="passwd">Password of the SSH client</param>
	/// <param name="timeout">Optionnal, timeout for the connection in seconds (default: 5)</param>
	/// <returns>SSH_OK on success, SSH_ERROR on error</returns>
	int connect(const char* addr, unsigned int port, const char* user, const char* passwd, long timeout = 5);

	/// <summary>
	/// Disconnect from the SSH client
	/// </summary>
	void disconnect();

	/// <summary>
	/// Executes a command on the SSH client
	/// </summary>
	/// <param name="cmd">Command to be executed</param>
	/// <param name="buffer_size">Optional, if the number bytes of the return ouput is known they can be specified here (default: 256)</param>
	/// <returns>SSH_OK on success, SSH_CMD_ERROR on command error, SSH_ERROR on connection error</returns>
	int execute(const char* cmd, unsigned int buffer_size = 256);

	/// <summary>
	/// Saves the project in the folder 'sw_workspace' on the SSH client
	/// </summary>
	/// <param name="project">Name of the project (i.e., folder)</param>
	/// <param name="path2project">Path to the project on the host disk</param>
	/// <returns>SSH_OK on success, SSH_ERROR on error</returns>
	int save(const char* project, const char* path2project = nullptr);

	/// <summary>
	/// Calls 'make' inside the project folder on the SSH client i.e., builds the project
	/// </summary>
	/// <param name="project">Name of the project</param>
	/// <returns>SSH_OK on build success, SSH_CMD_ERROR on build error, SSH_ERROR on connection or build error</returns>
	int compile(const char* project);

	/// <summary>
	/// Starts the previously compiled binary (application) of the project on the SSH client
	/// </summary>
	/// <param name="project">Name of the project</param>
	/// <param name="args">Additional arguments when calling the binary, can be NULL for no arguments</param>
	/// <returns>SSH_OK on success, SSH_CMD_ERROR on start error, SSH_ERROR on connection error</returns>
	int start(const char* project, const char* args = nullptr);

	/// <summary>
	/// Stops the previously started application of the project on the SSH client
	/// </summary>
	/// <param name="project">Name of the project</param>
	/// <returns>SSH_OK on success, SSH_CMD_ERROR on stop error, SSH_ERROR on connection error</returns>
	int stop(const char* project);

	/// <summary>
	/// Checks if the application of the project is running on the SSH client
	/// </summary>
	/// <param name="project">Name of the project</param>
	/// <returns>SSH_APP_RUNNING if the application is running, SSH_APP_NOT_RUNNING if the applications is not running, 0 on connection error</returns>
	int isAppRunning(const char* project);

	/// <summary>
	/// Reads the output of the project's application (through tail command) once
	/// </summary>
	/// <param name="project">Name of the project</param>
	/// <param name="nbytes">Optional, specifies how much of the last bytes should be read (default: 2048)</param>
	/// <returns>SSH_OK on success, SSH_CMD_ERROR on tail command error, SSH_ERROR on connection error</returns>
	int readOutput_once(const char* project, unsigned int nbytes = 2048);

	/// <summary>
	/// Reads the output of the project's application (through tail command) continuously (indefinitely)
	/// </summary>
	/// <param name="project">Name of the project</param>
	/// <param name="nbytes">Optional, specifies how much of the last bytes should be read (default: 2048)</param>
	/// <returns>SSH_OK on success, SSH_CMD_ERROR on tail command error, SSH_ERROR on connection error</returns>
	int readOutput_cont(const char* project, unsigned int nbytes = 2048);

private:
	/// <summary>
	/// SSH session handle
	/// </summary>
	ssh_session session;

	/// <summary>
	/// Set true when connect is called, false when disconnect is called
	/// </summary>
	bool connected;

	/// <summary>
	/// Used to specify the file to read from, easier to understand when using SSH read commands
	/// </summary>
	enum File { STDOUT = 0, STDERR = 1 };

	/// <summary>
	/// Writes data/characters to stdout (fwrite) and flushes afterwards
	/// </summary>
	/// <param name="buffer">Pointer to the array of characters to be written</param>
	/// <param name="count">Number of characters to write</param>
	/// <returns>count on success, less on error</returns>
	static size_t write_to_stdout(const char* buffer, size_t count);

	/// <summary>
	/// Writes data/characters to stderr (fwrite)
	/// </summary>
	/// <param name="buffer">Pointer to the array of characters to be written</param>
	/// <param name="count">Number of characters to write</param>
	/// <returns>count on success, less on error</returns>
	static size_t write_to_stderr(const char* buffer, size_t count);

	/// <summary>
	/// Reads and redirects the stdout or stderr from the channel to a given function
	/// </summary>
	/// <param name="channel">SSH channel to read from</param>
	/// <param name="from">Specifies wether to read from stdout or stderr</param>
	/// <param name="to">Pointer to the function to which the data is passed</param>
	/// <param name="buffer_size">Optional, if the number bytes of the return ouput is known they can be specified here (default: 256)</param>
	/// <returns>SSH_BYTES_READ when Bytes where read, SSH_NO_BYTES_READ when no Bytes where read, SSH_ERROR on error</returns>
	int channel_redirect_output(ssh_channel* channel, enum File from, size_t(*to)(const char*, size_t), unsigned int buffer_size = 256);

	/// <summary>
	/// Gets all folders and files from the project directory
	/// </summary>
	/// <param name="project">Name of the project</param>
	/// <param name="path2project">Path to the project on the host disk</param>
	/// <param name="dirs">A pointer to a list of strings in which the paths of the folders are pushed</param>
	/// <param name="files">A pointer to a list of strings in which the paths of the files are pushed</param>
	/// <returns>SSH_OK on success, SSH_ERROR on error</returns>
	int get_project_dirs_and_files(const char* project, const char* path2project, std::list<std::string>* dirs, std::list<std::string>* files);

	/// <summary>
	/// Removes the current sw_workspace folder from the SSH client and recreates it including the given folders
	/// </summary>
	/// <param name="project">Name of the project</param>
	/// <param name="dirs">A pointer to a list of strings containing the paths of the folders to be created</param>
	/// <returns>SSH_OK on success, SSH_CMD_ERROR when the folder creation failed, SSH_ERROR on connection error</returns>
	int project_mkdirs(const char* project, std::list<std::string>* dirs);

	/// <summary>
	/// Saves a single file from a project on the SSH client
	/// </summary>
	/// <param name="sftp">An already open SFTP session handle</param>
	/// <param name="filePath">Path to the file relative the project directory</param>
	/// <param name="path2project">Path to the project on the host disk</param>
	/// <param name="buffer_size">Optional, the size of the read buffer can be specified here (default: 16384)</param>
	/// <returns>SSH_OK on success, SSH_ERROR on error</returns>
	int project_save_file(sftp_session* sftp, std::string filePath, const char* path2project, unsigned int buffer_size = 16384);

	/// <summary>
	/// Saves/Transfers multiple files from a project on the SSH client
	/// </summary>
	/// <param name="files">A pointer to a list of strings containing the paths of the files to be transferred</param>
	/// <param name="path2project">Path to the project on the host disk</param>
	/// <returns>SSH_OK on success, SSH_ERROR on error</returns>
	int project_save_files(std::list<std::string>* files, const char* path2project);

};
