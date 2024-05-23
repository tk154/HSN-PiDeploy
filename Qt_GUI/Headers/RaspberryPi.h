#pragma once

#include "SSH.h"

#include <QObject>
#include <QTextEdit>
#include <QThread>
#include <QTreeWidgetItem>

class MainWindow;
struct piConfig;

// Used to distinguish between the different connection and application states of the Raspberry Pi
enum class piStatus { checking, offline, online, appRunning };

/// <summary>
/// Used for each Raspberry Pi client
/// </summary>
class RaspberryPi : public QObject {

    Q_OBJECT

public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="w">Pointer to a MainWindow object</param>
    /// <param name="config">The configuration (struct) for the Raspberry Pi</param>
    /// <param name="project_argv">The arguments of the project which may include meta parameters</param>
    /// <param name="item">A pointer to the item of the Raspberry Pi inside the QTreeWidget of the clients</param>
    /// <param name="te_output">A pointer to the QTextEdit tab of the Raspberry Pi from the output tab widget</param>
    RaspberryPi(MainWindow* w, piConfig config, QString project_argv, QTreeWidgetItem* item, QTextEdit* te_output);

    // Destructor
    ~RaspberryPi();

    // The name of the Raspberry Pi
    QString name;

    // The address of the Raspberry Pi
    QString addr;

    // The project of the Raspberry Pi
    QString project;

    // A map for the meta parameters and their values
    QMap<QString, QString> args;

    // Indicate if the project name is valid
    bool projectValid;

    // Saves the arguments of the Raspberry Pi
    QString argv;

    // Save the last error message
    QString error = "";

    // Save the current status, initialize with checking
    piStatus status = piStatus::checking;

    // Save the pointer to a timer for checking the status periodically
    QTimer* statusTimer;

    // Save the pointer to the item of the Raspberry Pi inside the QTreeWidget of the clients
    QTreeWidgetItem* item;

    // Save the pointer to the QTextEdit tab of the Raspberry Pi from the output tab widget to add output to it
    QTextEdit* te_output;

    /// <summary>
    /// Calculate the arguments from the (meta) parameters of the Raspberry Pi
    /// </summary>
    /// <param name="project_argv">The arguments of the project which may include meta parameters</param>
    void calc_argv(QString project_argv);

private:
    // Pointers to the different SSH sub-classes
    SSHaction* sshAction;
    SSHstatus* sshStatus;
    SSHoutput* sshOutput;

    // QThreads for the SSH sub-classes
    QThread sshActionThread;
    QThread sshStatusThread;
    QThread sshOutputThread;

signals:
    /// <summary>
    /// Check the status of the Raspberry Pi, signal to SSHstatus
    /// </summary>
    void SSHgetStatus();

    /// <summary>
    /// Save the project of the Raspberry Pi, signal to SSHaction
    /// </summary>
    /// <param name="path2project">Path to the project folder on the disk</param>
    void SSHsave(QString path2project);

    /// <summary>
    /// Compile the project of the Raspberry Pi, signal to SSHaction
    /// </summary>
    void SSHcompile();

    /// <summary>
    /// Start the project of the Raspberry Pi, signal to SSHaction
    /// </summary>
    void SSHstart();

    /// <summary>
    /// Stop the project of the Raspberry Pi, signal to SSHaction
    /// </summary>
    void SSHstop();

    /// <summary>
    /// Read the output of the Raspberry Pi project once, signal to SSHoutput
    /// </summary>
    void SSHreadOutput_once();

    /// <summary>
    /// Read the output of the Raspberry Pi project continuously, signal to SSHoutput
    /// </summary>
    void SSHreadOutput_cont();

    /// <summary>
    /// Stop the continuous reading of the output by exiting the SSH executable, signal to SSHoutput
    /// </summary>
    void SSHreadOutput_stop();

    /// <summary>
    /// Sent after checking the status of the Raspberry Pi, signal to MainWindow
    /// </summary>
    /// <param name="pi">Pointer to this Raspberry Pi object</param>
    /// <param name="currStatus">Current status of the Raspberry Pi</param>
    /// <param name="error">The error message, if the Raspberry Pi is offline, else empty</param>
    void SSHcurrStatus(RaspberryPi* pi, piStatus currStatus, QByteArray error);

public slots:
    /// <summary>
    /// Stops the threads and deletes the Raspberry Pi object
    /// </summary>
    void clear();

    /// <summary>
    /// Sent after the status of the Raspberry Pi has been checked, response from RaspberryPi
    /// </summary>
    /// <param name="status">Current status of the Raspberry Pi</param>
    /// <param name="error">The error message, if the Raspberry Pi is offline, else empty</param>
    void SSHgetStatus_finished(piStatus status, QByteArray error);

};
