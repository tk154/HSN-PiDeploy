#pragma once

#include <QObject>
#include <QProcess>

class RaspberryPi;
enum class piStatus;

/// <summary>
/// Base class used to execute certain operations of a Raspberry Pi project through the SSH executable
/// </summary>
class SSH : public QObject {

	Q_OBJECT

public:
	/// <summary>
	/// Constructor
	/// </summary>
	/// <param name="pi">Pointer to a RaspberryPi object</param>
	SSH(RaspberryPi* pi);

protected:
	// Pointer to a RaspberryPi object
	RaspberryPi* pi;

	/// <summary>
	/// Execute an operation the Raspberry Pi project through the SSH executable
	/// </summary>
	/// <param name="process">Pointer to a newly created QProcess object. Can be used to retreive the exit code or output</param>
	/// <param name="operation">Operation be executed (e.g. "save", "compile", ...)</param>
	/// <param name="args">Optional, arguments to be passed (default: "")</param>
	/// <returns>true if the SSH executable was started successfully, false otherwise</returns>
	bool execSSHexe(QProcess* process, QString operation, QString args = "");

signals:
	/// <summary>
	/// Sent to RaspberryPi to signal the current status of the Raspberry Pi
	/// </summary>
	/// <param name="pi">Current status of the Raspberry Pi</param>
	/// <param name="success">The error message, if the Raspberry Pi is offline, else empty</param>
	void SSHcurrStatus(piStatus status, QByteArray error = "");

};

/// <summary>
/// Used to execute the save, compile, start, and stop operation through the SSH executable
/// </summary>
class SSHaction : public SSH {

	Q_OBJECT

	// Use the base constructor
	using SSH::SSH;

public slots:
	/// <summary>
	/// Save the project of the Raspberry Pi, request from MainWindow
	/// </summary>
	/// <param name="path2project">Path to the project folder on the disk</param>
	void SSHsave(QString path2project);

	/// <summary>
	/// Compile the project of the Raspberry Pi, request from MainWindow
	/// </summary>
	void SSHcompile();

	/// <summary>
	/// Start the project of the Raspberry Pi, request from MainWindow
	/// </summary>
	void SSHstart();

	/// <summary>
	/// Stop the project of the Raspberry Pi, request from MainWindow
	/// </summary>
	void SSHstop();

signals:
	/// <summary>
	/// Sent after the project has been saved, response to MainWindow
	/// </summary>
	/// <param name="pi">Pointer to the RaspberryPi object</param>
	/// <param name="success">true, if the project was saved successfully, else false</param>
	/// <param name="output">Output from the SSH executable</param>
	void SSHsave_finished(RaspberryPi* pi, bool success, QByteArray output);

	/// <summary>
	/// Sent after the project has been compiled, response to MainWindow
	/// </summary>
	/// <param name="pi">Pointer to the RaspberryPi object</param>
	/// <param name="success">true, if the project was compiled successfully, else false</param>
	/// <param name="output">Output from the SSH executable</param>
	void SSHcompile_finished(RaspberryPi* pi, bool success, QByteArray output);

	/// <summary>
	/// Sent after the project has been started, response to MainWindow
	/// </summary>
	/// <param name="pi">Pointer to the RaspberryPi object</param>
	/// <param name="success">true, if the project was started successfully, else false</param>
	/// <param name="output">Output from the SSH executable</param>
	void SSHstart_finished(RaspberryPi* pi, bool success, QByteArray output);

	/// <summary>
	/// Sent after the project has been stopped, response to MainWindow
	/// </summary>
	/// <param name="pi">Pointer to the RaspberryPi object</param>
	/// <param name="success">true, if the project was stopped successfully, else false</param>
	/// <param name="output">Output from the SSH executable</param>
	void SSHstop_finished(RaspberryPi* pi, bool success, QByteArray output);

};

/// <summary>
/// Used to check the status of the Raspberry Pi through the SSH executable
/// </summary>
class SSHstatus : public SSH {

	Q_OBJECT

	// Use the base constructor
	using SSH::SSH;

public slots:
	/// <summary>
	/// Check the status of the Raspberry Pi, request from RaspberryPi
	/// </summary>
	void SSHgetStatus();

};

/// <summary>
/// Used to read the output of the Raspberry Pi project through the SSH executable
/// </summary>
class SSHoutput : public SSH {

	Q_OBJECT

	// Use the base constructor
	using SSH::SSH;

private:
	// Pointer to a QProcess object
	QProcess* process;

public slots:
	/// <summary>
	/// Read the output of the Raspberry Pi project once, request from RaspberryPi
	/// </summary>
	void SSHreadOutput_once();

	/// <summary>
	/// Read the output of the Raspberry Pi project continuously, request from RaspberryPi
	/// </summary>
	void SSHreadOutput_cont();

	/// <summary>
	/// Stop the continuous reading of the output by exiting the SSH executable, request from RaspberryPi
	/// </summary>
	void SSHreadOutput_stop();

private slots:
	/// <summary>
	/// Called when new output from the project is ready to read
	/// </summary>
	void process_readyReadStandardOutput();

	/// <summary>
	/// Called when the process finishes/exits
	/// <param name="exitCode">The exit code of the process</param>
	/// </summary>
	void process_finished(int exitCode);

signals:
	/// <summary>
	/// Sent after output has been read from the project, response to MainWindow
	/// </summary>
	/// <param name="pi">Pointer to the RaspberryPi object</param>
	/// <param name="output">Output from the project</param>
	void SSHreadOutput_finished(RaspberryPi* pi, QByteArray output);

};
