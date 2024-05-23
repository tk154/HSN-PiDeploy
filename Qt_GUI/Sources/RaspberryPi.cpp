#include "RaspberryPi.h"
#include "MainWindow.h"

#include <QTimer>

RaspberryPi::RaspberryPi(MainWindow* w, piConfig config, QString project_argv, QTreeWidgetItem* item, QTextEdit* te_output) {
    // Save the values of the config struct
    this->name = config.name;
    this->addr = config.addr;
    this->project = config.project;
    this->args = config.args;
    this->projectValid = config.projectValid;

    // Save the pointers to the item and text widget of the Raspberry Pi from the GUI/MainWindow
    this->item = item;
    this->te_output = te_output;

    // Calculate the arguments from the (meta) parameters
    calc_argv(project_argv);

    // Create a new SSHaction object, move it to a QThread, and delete it when the QThread finishes/exits
    sshAction = new SSHaction(this);
    sshAction->moveToThread(&sshActionThread);
    connect(&sshActionThread, &QThread::finished, sshAction, &QObject::deleteLater);

    // Connect the save, compile, start, and stop signals of this Raspberry Pi to the corresponding slots of the SSHaction object
    connect(this, &RaspberryPi::SSHsave, sshAction, &SSHaction::SSHsave);
    connect(this, &RaspberryPi::SSHcompile, sshAction, &SSHaction::SSHcompile);
    connect(this, &RaspberryPi::SSHstart, sshAction, &SSHaction::SSHstart);
    connect(this, &RaspberryPi::SSHstop, sshAction, &SSHaction::SSHstop);

    // Connect the finished signals of save, compile, start, and stop from sshAction to the corresponding slots of the GUI/MainWindow
    connect(sshAction, &SSHaction::SSHsave_finished, w, &MainWindow::SSHsave_finished);
    connect(sshAction, &SSHaction::SSHcompile_finished, w, &MainWindow::SSHcompile_finished);
    connect(sshAction, &SSHaction::SSHstart_finished, w, &MainWindow::SSHstart_finished);
    connect(sshAction, &SSHaction::SSHstop_finished, w, &MainWindow::SSHstop_finished);

    // Connect the current status signal from sshAction to the SSHgetStatus_finished slots of this Raspberry Pi
    connect(sshAction, &SSHaction::SSHcurrStatus, this, &RaspberryPi::SSHgetStatus_finished);

    // Create a new SSHstatus object, move it to a QThread, and delete it when the QThread finishes/exits
    sshStatus = new SSHstatus(this);
    sshStatus->moveToThread(&sshStatusThread);
    connect(&sshStatusThread, &QThread::finished, sshStatus, &QObject::deleteLater);

    // Connect the getStatus signal of this Raspberry Pi to the corresponding slot of sshStatus
    connect(this, &RaspberryPi::SSHgetStatus, sshStatus, &SSHstatus::SSHgetStatus);

    // Connect the current status signal from sshStatus to the corresponding slot of this Raspberry Pi
    connect(sshStatus, &SSHstatus::SSHcurrStatus, this, &RaspberryPi::SSHgetStatus_finished);

    // Connect the currStatus signal of this Raspberry Pi to the corresponding slot of the GUI/MainWindow
    connect(this, &RaspberryPi::SSHcurrStatus, w, &MainWindow::SSHcurrStatus);

    // Create a new SSHoutput object, move it to a QThread, and delete it when the QThread finishes/exits
    sshOutput = new SSHoutput(this);
    sshOutput->moveToThread(&sshOutputThread);
    connect(&sshOutputThread, &QThread::finished, sshOutput, &QObject::deleteLater);

    // Connect the readOutput once, continuously, and stop signals of this Raspberry to the corresponding slots of SSHoutput
    connect(this, &RaspberryPi::SSHreadOutput_once, sshOutput, &SSHoutput::SSHreadOutput_once);
    connect(this, &RaspberryPi::SSHreadOutput_cont, sshOutput, &SSHoutput::SSHreadOutput_cont);
    connect(this, &RaspberryPi::SSHreadOutput_stop, sshOutput, &SSHoutput::SSHreadOutput_stop);

    // Connect the current status signal from sshOutput to the corresponding slot of this Raspberry Pi
    connect(sshOutput, &SSHaction::SSHcurrStatus, this, &RaspberryPi::SSHgetStatus_finished);

    // Connect the finished signals of readOutput from sshOutput to the corresponding slot of the GUI/MainWindow
    connect(sshOutput, &SSHoutput::SSHreadOutput_finished, w, &MainWindow::SSHnewOutput);

    // Connect the clear Raspberry Pis signal from the GUI/MainWindow to the clear slot of this Raspberry Pi
    connect(w, &MainWindow::clearRaspberryPis, this, &RaspberryPi::clear);

    // Start all three QThreads
    sshActionThread.start();
    sshStatusThread.start();
    sshOutputThread.start();

    // Create a new QTimer object, get the status of the Raspberry Pi on a five seconds timeout, and set that it must be re-started manually
    statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &RaspberryPi::SSHgetStatus);
    statusTimer->setInterval(5000);
    statusTimer->setSingleShot(true);

    // Get the status of the Raspberry Pi
    SSHgetStatus();
}

RaspberryPi::~RaspberryPi() {
    // Quit from all three QThreads
    sshActionThread.quit();
    sshStatusThread.quit();
    sshOutputThread.quit();

    // Wait for the termination of the three QThreads
    sshActionThread.wait();
    sshStatusThread.wait();
    sshOutputThread.wait();
}

void RaspberryPi::clear() {
    // Block the signals of all three SSH objects so that pending signals from a currently executed operation are ignored
    sshAction->blockSignals(true);
    sshStatus->blockSignals(true);
    sshOutput->blockSignals(true);

    // If the application is currently running on the Raspberry Pi
    if (status == piStatus::appRunning)
        // Stop the continuous reading of the output
        SSHreadOutput_stop();

    // Delete this Raspberry Pi object
    deleteLater();
}

void RaspberryPi::calc_argv(QString project_argv) {
    // Save the argument string of the project
    QString argv = project_argv;

    // Set the regular expression pattern and match it against the argument string of the project
    QRegularExpression re("%\\w+%");
    QRegularExpressionMatchIterator it = re.globalMatch(argv);

    // While there is a match
    while (it.hasNext()) {
        // Save the next match
        QString match = it.next().captured();

        // Replace the meta parameter of the argument string with the value from the args map
        // If the meta parameter wasn't found inside the map remove it from the argument string ("")
        argv.replace(match, args.value(match.mid(1, match.length() - 2), ""));
    }

    // Save the argument string inside this Raspberry Pi object
    this->argv = argv;
}

void RaspberryPi::SSHgetStatus_finished(piStatus currStatus, QByteArray error) {
    // Save the previous status of this Raspberry Pi
    piStatus prevStatus = this->status;

    switch (prevStatus) {
        // If the status was previously checked or the Raspberry Pi was offline
        case piStatus::checking:
        case piStatus::offline:
            // If the Raspberry Pi is now online
            if (currStatus == piStatus::online)
                // Read the output of the Raspberry Pi once
                SSHreadOutput_once();
            // If the Raspberry Pi is now online and the application is running
            else if (currStatus == piStatus::appRunning)
                // Read the output of the Raspberry Pi continuously
                SSHreadOutput_cont();
            break;

        // If the Raspberry Pi was previously online
        case piStatus::online:
            // If the application is now running
            if (currStatus == piStatus::appRunning)
                // Read the output of the Raspberry Pi continuously
                SSHreadOutput_cont();
            break;

        // If the application was previously running on the Raspberry Pi
        case piStatus::appRunning:
            // If the application isn't running anymore
            if (currStatus == piStatus::online)
                // Stop the continuous reading of the output
                SSHreadOutput_stop();
            break;
    }

    // Signal the GUI/MainWindow the current status of this Raspberry Pi
    SSHcurrStatus(this, currStatus, error);

    // Restart the status timer
    statusTimer->start();
}
