#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"

#include <QFileSystemModel>

enum class piStatus;
class RaspberryPi;

// Used to save the configuration of a Raspberry Pi
struct piConfig {
    // Save the name, address, and project name
    QString name, addr, project;

    // Map the meta parameter names to their values
    QMap<QString, QString> args;

    // Save if the project name is valid (found in sw_workspace)
    bool projectValid;
};

/// <summary>
/// MainWindow Class
/// </summary>
class MainWindow : public QMainWindow {

    Q_OBJECT

public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="parent">Parent QWidget</param>
    MainWindow(QWidget *parent = Q_NULLPTR);

protected:
    /// <summary>
    /// Called when the window is closed
    /// </summary>
    void closeEvent(QCloseEvent*);

private:
    // Contains the pointers to all QWidgets of the GUI
    Ui::MainWindowClass ui;

    // Stores the path of the projects
    QString projectsPath = "";

    // Maps the project names to the arguments of the project
    QMap<QString, QString> projects_argv;

    // Store every Raspberry Pi inside a list
    QList<RaspberryPi*> pis;

    // Maps the items of the QTreeWidget to the corresponding Raspberry Pis
    QMap<QTreeWidgetItem*, RaspberryPi*> item2pi;

    // Maps the output TextEdits to the corresponding Raspberry Pis
    QMap<QTextEdit*, RaspberryPi*> output2pi;

    // Pointer to "(unassigned)" inside the client's QTreeWidget
    QTreeWidgetItem* unassigned;

    // Pointer to a data model for the local filesystem
    QFileSystemModel* fileSystemModel;
    
    // Saves the number of busy Raspberry Pis where a operation is executed (e.g., currenly saving, building, starting, or stopping)
    unsigned int busyPiCount = 0;

    // Save if the last change to the output scrollbar was made by the user
    //bool scrollbarChangedByUser = true;

    /// <summary>
    /// Refreshs the Raspberry Pi and file tree by reading the config file and the local filesystem
    /// </summary>
    void refreshTrees();

    /// <summary>
    /// Reads and checks the configuration file "testbedkonfiguration.txt"
    /// </summary>
    /// <returns>A list containing the configuration of every Raspberry Pi from the file</returns>
    QList<piConfig> readConfig();

    /// <summary>
    /// Reads the arguments file (argv.txt) of the given project
    /// </summary>
    /// <param name="projectName">Name of the project</param>
    /// <param name="projectPath">Path to the project's directory</param>
    /// <returns>true if the file was read successfully, false if an error occured (e.g., file not found)</returns>
    bool read_argv(QString projectName, QString projectPath);

    /// <summary>
    /// Reads the local filesystem tree resp. the projects of projectsPath and adds them to the QTreeWidget
    /// </summary>
    /// <returns>A map mapping the project names to their item inside the project's QTreeWidget</returns>
    QMap<QString, QTreeWidgetItem*> getProjects();

    /// <summary>
    /// Checks the name, address, and project of the given config for invalid entries or duplicates
    /// </summary>
    /// <param name="pi_config">Pointer to the Raspberry Pi configuration</param>
    /// <param name="projects">List of all project names</param>
    /// <param name="projects">If there is a warning append it to the string at the pointer location</param>
    /// <returns>true if the configuration is ok, else false</returns>
    bool check_piConfig(piConfig* pi_config, QList<QString> projects, QString* warnings);

    /// <summary>
    /// Creates a Raspberry Pi item inside the QTreeWidget
    /// </summary>
    /// <param name="pi_configs">List of Raspberry Pi configurations</param>
    /// <param name="project2item">Project name to item map</param>
    void create_piNodes(QList<piConfig> pi_configs, QMap<QString, QTreeWidgetItem*> project2item);

    /// <summary>
    /// Writes the current Raspberry Pi configuration back to the configuration file "testbedkonfiguration.txt"
    /// </summary>
    void writeConfig();

    /// <summary>
    /// Enables or disables all buttons of the GUI
    /// </summary>
    /// <param name="enable">If true enables all buttons, else disables all buttons</param>
    void btns_setEnabled(bool enable);

public slots:
    /// <summary>
    /// Called if the button "btn_save" was clicked
    /// </summary>
    void btn_save_clicked();

    /// <summary>
    /// Called if the button "btn_compile" was clicked
    /// </summary>
    void btn_compile_clicked();

    /// <summary>
    /// Called if the button "btn_start" was clicked
    /// </summary>
    void btn_start_clicked();

    /// <summary>
    /// Called if the button "btn_stop" was clicked
    /// </summary>
    void btn_stop_clicked();

    /// <summary>
    /// Called if the button "btn_refresh" was clicked
    /// </summary>
    void btn_refresh_clicked();

    /// <summary>
    /// Called if the checkbox "chbx_selectAll" has been toggled
    /// </summary>
    /// <param name="state">Current state of the checkbox i.e., checked or unchecked</param>
    void chbx_selectAll_stateChanged(int state);

    /// <summary>
    /// Called if the checkbox of a Raspberry Pi item was toggled inside the QTreeWidget
    /// </summary>
    void tw_pis_itemChanged(QTreeWidgetItem*, int);

    /// <summary>
    /// Called if an item inside the QTreeWidget has been clicked/pressed
    /// </summary>
    /// <param name="item">The clicked/pressed QTreeWidgetItem</param>
    void tw_pis_itemPressed(QTreeWidgetItem* item, int);

    /// <summary>
    /// Called if a Raspberry Pi item was dropped under a project item inside the QTreeWidget
    /// </summary>
    void tw_pis_itemDropped(QTreeWidgetItem*);

    /// <summary>
    /// Called if a folder or file inside the QTreeView was double-clicked
    /// </summary>
    /// <param name="index">Index of the clicked element inside the FileSystemModel</param>
    void tv_files_doubleClicked(QModelIndex index);

    /// <summary>
    /// Received after the project has been saved, response from RaspberryPi
    /// </summary>
    /// <param name="pi">Pointer to a RaspberryPi object</param>
    /// <param name="success">true, if the project was saved successfully, else false</param>
    /// <param name="output">Output from the SSH executable</param>
    void SSHsave_finished(RaspberryPi* pi, bool success, QByteArray output);

    /// <summary>
    /// Received after the project has been compiled, response from RaspberryPi
    /// </summary>
    /// <param name="pi">Pointer to a RaspberryPi object</param>
    /// <param name="success">true, if the project was compiled successfully, else false</param>
    /// <param name="output">Output from the SSH executable</param>
    void SSHcompile_finished(RaspberryPi* pi, bool success, QByteArray output);

    /// <summary>
    /// Received after the project has been started, response from RaspberryPi
    /// </summary>
    /// <param name="pi">Pointer to a RaspberryPi object</param>
    /// <param name="success">true, if the project was started successfully, else false</param>
    /// <param name="output">Output from the SSH executable</param>
    void SSHstart_finished(RaspberryPi* pi, bool success, QByteArray output);

    /// <summary>
    /// Received after the project has been stopped, response from RaspberryPi
    /// </summary>
    /// <param name="pi">Pointer to a RaspberryPi object</param>
    /// <param name="success">true, if the project was stopped successfully, else false</param>
    /// <param name="output">Output from the SSH executable</param>
    void SSHstop_finished(RaspberryPi* pi, bool success, QByteArray output);

    /// <summary>
    /// Received after checking the status of the Raspberry Pi, signal from RaspberryPi
    /// </summary>
    /// <param name="pi">Pointer to a Raspberry Pi object</param>
    /// <param name="currStatus">Current status of the Raspberry Pi</param>
    /// <param name="error">The error message, if the Raspberry Pi is offline, else empty</param>
    void SSHcurrStatus(RaspberryPi* pi, piStatus currStatus, QByteArray error);

    /// <summary>
    /// Received after output has been read from the project, signal from SSH
    /// </summary>
    /// <param name="pi">Pointer to a RaspberryPi object</param>
    /// <param name="output">Output from the project</param>
    void SSHnewOutput(RaspberryPi* pi, QByteArray output);

signals:
    /// <summary>
    /// Signal to all RaspberryPi objects to stop their threads and delete themselves
    /// </summary>
    void clearRaspberryPis();

};
