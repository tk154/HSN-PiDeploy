#include "SSH.h"
#include "RaspberryPi.h"

// When compiling on Windows
#ifdef WIN32
    #define SSH_BIN "SSH.exe"
// When compiling on Linux
#elif __linux__
    #define SSH_BIN "./SSH"
#endif

SSH::SSH(RaspberryPi* pi) {
    // Save the pointer to the RaspberryPi object
    this->pi = pi;
}

bool SSH::execSSHexe(QProcess* process, QString operation, QString args) {
    // Save the address, operation, project name, and additional arguments as parameters
    QStringList params;
    params << pi->addr << operation << pi->project << args;

    // Start the SSH executable with the saved parameters
    process->start(SSH_BIN, params);

    // Wait for the process to be started
    if (!process->waitForStarted())
        // If the process couldn't start (e.g., SSH executable missing)
        return false;

    // Wait forever (-1) until the process exits
    process->waitForFinished(-1);

    return true;
}

void SSHstatus::SSHgetStatus() {
    // Create a new QProcess object
    QProcess process;

    // Execute the status operation to get the current status of the Raspberry Pi
    if (execSSHexe(&process, "status")) {
        switch (process.exitCode()) {
            case 1:
                // The application is running on the Raspberry Pi
                SSHcurrStatus(piStatus::appRunning);
                break;

            case 2:
                // The Raspberry Pi is online but the application is not running
                SSHcurrStatus(piStatus::online);
                break;

            default:
                // The Raspberry Pi is offline (code = 0) or there was an error (code = -1)
                SSHcurrStatus(piStatus::offline, process.readAllStandardError());
        }
    }
    else
        // If the SSH executable couldn't be started
        SSHcurrStatus(piStatus::offline, "SSH executable couldn't be started.");
}

void SSHaction::SSHsave(QString path2project) {
    // Create a new QProcess object
    QProcess process;

    // Execute the save operation to save the project of the Raspberry Pi from the given projects path
    if (execSSHexe(&process, "save", path2project)) {
        if (process.exitCode() == 0)
            // If the operation was successful read (and return) from stdout
            SSHsave_finished(pi, true, process.readAllStandardOutput());
        else
            // If the operation was unsuccessful read (and return) from stderr
            SSHsave_finished(pi, false, process.readAllStandardError());
    }
    else
        // If the SSH executable couldn't be started
        SSHsave_finished(pi, false, "SSH executable couldn't be started.");
}

void SSHaction::SSHcompile() {
    // Create a new QProcess object
    QProcess process;

    // Execute the compile operation to build the project of the Raspberry Pi
    if (execSSHexe(&process, "compile")) {
        if (process.exitCode() == 0)
            // If the operation was successful read (and return) from stdout
            SSHcompile_finished(pi, true, process.readAllStandardOutput());
        else
            // If the operation was unsuccessful read (and return) from stderr
            SSHcompile_finished(pi, false, process.readAllStandardError());
    }
    else
        // If the SSH executable couldn't be started
        SSHcompile_finished(pi, false, "SSH executable couldn't be started.");
}

void SSHaction::SSHstart() {
    // Create a new QProcess object
    QProcess process;

    // Execute the start operation to start the project of the Raspberry Pi with the given arguments
    if (execSSHexe(&process, "start", pi->argv)) {
        if (process.exitCode() == 0) {
            // If the operation was successful read (and return) from stdout
            SSHstart_finished(pi, true, process.readAllStandardOutput());

            // Signal that the application is running on the Raspberry Pi
            SSHcurrStatus(piStatus::appRunning);
        }
        else
            // If the operation was unsuccessful read (and return) from stderr
            SSHstart_finished(pi, false, process.readAllStandardError());
    }
    else
        // If the SSH executable couldn't be started
        SSHstart_finished(pi, false, "SSH executable couldn't be started.");
}

void SSHaction::SSHstop() {
    // Create a new QProcess object
    QProcess process;

    // Execute the stop operation to stop the project of the Raspberry Pi
    if (execSSHexe(&process, "stop")) {
        if (process.exitCode() == 0) {
            // If the operation was successful read (and return) from stdout
            SSHstop_finished(pi, true, process.readAllStandardOutput());

            // Signal that the application is not running anymore on the Raspberry Pi
            SSHcurrStatus(piStatus::online);
        }
        else
            // If the operation was unsuccessful read (and return) from stderr
            SSHstop_finished(pi, false, process.readAllStandardError());
    }
    else
        // If the SSH executable couldn't be started
        SSHstop_finished(pi, false, "SSH executable couldn't be started.");
}

void SSHoutput::SSHreadOutput_once() {
    // Create a new QProcess object
    QProcess process;

    // Execute the readOutput_once operation to read the output of the application once
    if (execSSHexe(&process, "readOutput_once")) {
        if (process.exitCode() == 0)
            // If the operation was successful read the output of the application from stdout
            SSHreadOutput_finished(pi, process.readAllStandardOutput());
    }
    else
        // If the SSH executable couldn't be started
        SSHreadOutput_finished(pi, "SSH executable couldn't be started.");
}

void SSHoutput::SSHreadOutput_cont() {
    // Save the address, operation, and project name as parameters
    QStringList params;
    params << pi->addr << "readOutput_cont" << pi->project;

    // Create a new QProcess object, save the pointer
    process = new QProcess();

    // If new data is available to read from stdout execute readyReadStandardOutput
    connect(process, &QProcess::readyReadStandardOutput, this, &SSHoutput::process_readyReadStandardOutput);

    // If the process has finished/exited
    connect(process, &QProcess::finished, this, &SSHoutput::process_finished);

    // Start the SSH executable with the saved parameters
    process->start(SSH_BIN, params);
}

void SSHoutput::SSHreadOutput_stop() {
    // Write some character to the process' stdout so that it exits
    process->write("q");

    // Wait two seconds long for the termination of the process
    if (!process->waitForFinished(2000))
        // If the process hasn't terminated yet kill it
        process->kill();
}

void SSHoutput::process_readyReadStandardOutput() {
    // If new data is available from stdout read and signal it to MainWindow
    SSHreadOutput_finished(pi, process->readAllStandardOutput());
}

void SSHoutput::process_finished(int exitCode) {
    // If there was a connection error
    if (exitCode == -1)
        // Signal that the Raspberry Pi is offline along the exit code
        SSHcurrStatus(piStatus::offline, process->readAllStandardError());

    // Delete the QProcess object
    process->deleteLater();
}
