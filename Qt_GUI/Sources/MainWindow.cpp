#include "MainWindow.h"
#include "RaspberryPi.h"

#include <QDesktopServices>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui.setupUi(this);

    // Don't allow resize and disable the maximize button
    setFixedSize(width(), height());

    // Is needed to use piStatus as signal/slot parameters
    qRegisterMetaType<piStatus>("piStatus");

    // Connect the button and checkbox clicked signals to the corresponding slots
    connect(ui.btn_save, &QPushButton::clicked, this, &MainWindow::btn_save_clicked);
    connect(ui.btn_compile, &QPushButton::clicked, this, &MainWindow::btn_compile_clicked);
    connect(ui.btn_start, &QPushButton::clicked, this, &MainWindow::btn_start_clicked);
    connect(ui.btn_stop, &QPushButton::clicked, this, &MainWindow::btn_stop_clicked);
    connect(ui.btn_refresh, &QPushButton::clicked, this, &MainWindow::btn_refresh_clicked);
    connect(ui.btn_collapse, &QPushButton::clicked, ui.tv_files, &QTreeView::collapseAll);
    connect(ui.chbx_selectAll, &QCheckBox::clicked, this, &MainWindow::chbx_selectAll_stateChanged);

    // Connect the clicked/pressed, changed, and dropped signals of the QTreeWidget to the corresponding slots
    connect(ui.tw_pis, &QTreeWidgetPis::itemPressed, this, &MainWindow::tw_pis_itemPressed);
    connect(ui.tw_pis, &QTreeWidgetPis::itemChanged, this, &MainWindow::tw_pis_itemChanged);
    connect(ui.tw_pis, &QTreeWidgetPis::itemDropped, this, &MainWindow::tw_pis_itemDropped);

    // Connect the double-clicked signals of the QTreeView to the corresponding slots
    connect(ui.tv_files, &QTreeView::doubleClicked, this, &MainWindow::tv_files_doubleClicked);

    // If new output is available inside the info ListWidget scroll to bottom
    connect(ui.lw_infoOutput->model(), &QAbstractItemModel::rowsInserted, ui.lw_infoOutput, &QListWidget::scrollToBottom);

    // Clear the error label and output TabWidget
    ui.lbl_error->clear();
    ui.tabW_appOutput->clear();

    // Create a new FileSystemModel object, don't refresh if there are changes on the local filesystem, show it inside the TreeView
    fileSystemModel = new QFileSystemModel;
    fileSystemModel->setOption(QFileSystemModel::DontWatchForChanges);
    ui.tv_files->setModel(fileSystemModel);

    // Hide all columns of the TreeView except the first showing the filename
    for (int i = 1; i < fileSystemModel->columnCount(); i++)
        ui.tv_files->hideColumn(i);

    // Fill the TreeWidget and TreeView
    refreshTrees();
}

void MainWindow::closeEvent(QCloseEvent*) {
    // Delete all RaspberryPi objects when the window is closed
    clearRaspberryPis();
}

QList<piConfig> MainWindow::readConfig() {
    // Try to open the configuration textfile for reading
    QFile file("testbed_workspace/testbedkonfiguration.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // If the file couldn't be opened show an error message and exit the application afterwards
        QMessageBox::critical(this, "Error", "Cannot open file testbed_workspace/testbedkonfiguration.txt");
        exit(EXIT_FAILURE);
    }

    // Create a new list for the read Raspberry Pi configurations
    QList<piConfig> pis;

    // Create a TextStream for reading the file
    QTextStream in(&file);

    // While the end of the file isn't reached
    while (!in.atEnd()) {
        // Read the next line
        QString line = in.readLine();

        // Ignore comments
        if (line.startsWith('#'))
            continue;

        // If the configuration for a new Raspberry Pi starts
        if (line.startsWith('[') && line.endsWith(']')) {
            // Create a new piConfig and save the name of the Raspberry Pi inside it
            piConfig pi;
            pi.name = line.mid(1, line.length() - 2);

            // Read the next line
            line = in.readLine();

            // While the current line isn't empty
            while (!line.isEmpty()) {
                // Ignore comments
                if (line.startsWith('#'))
                    continue;

                // Split the read line at the equal sign to get property and value
                QStringList split = line.split('=');

                // If there is only a property and a value
                if (split.length() == 2) {
                    // Save the property
                    QString prop = split.at(0);

                    // Distinguish between properties and save the value
                    if (prop == "address")
                        pi.addr = split.at(1);
                    else if (prop == "project")
                        pi.project = split.at(1);
                    else if (prop == "args") {
                        // If arguments are given iterate through all of them
                        for (QString arg : split.at(1).split(',')) {
                            // Split to get the name and value
                            QStringList argSplit = arg.split(':');

                            // If there is only a name and a value
                            if (argSplit.length() == 2)
                                // Save the name and value pair inside Raspberry Pi arguments map
                                pi.args[argSplit.at(0)] = argSplit.at(1);
                        }
                    }
                }

                // Read the next line
                line = in.readLine();
            }

            // Add the configuration to the list
            pis.push_back(pi);
        }
        else {
            // Split the read line at the equal sign to get property and value
            QStringList split = line.split('=');

            // If there is only a property and a value
            if (split.length() == 2)
                // If the property is the project path
                if (split.at(0) == "projectsPath")
                    // Save the project path
                    projectsPath = split.at(1);
        }
    }

    // Close the file
    file.close();

    // If no project path was given inside the configuration file
    if (projectsPath == "") {
        QMessageBox::critical(this, "Error", "No project directory was given.");
        exit(EXIT_FAILURE);
    }

    // Return the configuration list
    return pis;
}

bool MainWindow::read_argv(QString projectName, QString projectPath) {
    // Try to open the arguments textfile for reading
    QFile file(QString("%1/config.txt").arg(projectPath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        // If the file couldn't be opened
        return false;

    // Create a TextStream for reading the file
    QTextStream in(&file);

    // While the end of the file isn't reached
    while (!in.atEnd()) {
        // Read the next line
        QString line = in.readLine();

        // Ignore comments
        if (line.startsWith('#'))
            continue;

        // Read the next line and split it at the equal sign to get property and value
        QStringList split = line.split("=");

        // If there is only a property and a value
        if (split.length() == 2) {
            // Save the property
            QString prop = split.at(0);

            // If the property is the argument line
            if (prop == "argv")
                // Save the argument line of the project
                projects_argv[projectName] = split.at(1);
        }
    }

    // Close the file
    file.close();

    return true;
}

QMap<QString, QTreeWidgetItem*> MainWindow::getProjects() {
    // Check if the project path exists
    QDir projectsDir(projectsPath);
    if (!projectsDir.exists()) {
        // If it doesn't exist show an error message and exit the application afterwards
        QMessageBox::critical(this, "Error", QString("Directory %1 doesn't exist.").arg(projectsPath));
        exit(EXIT_FAILURE);
    }

    // Show the project directory inside the TreeView
    fileSystemModel->setRootPath(projectsPath);
    ui.tv_files->setRootIndex(fileSystemModel->index(projectsPath));

    // Make the root of the TreeWidget invisible
    QTreeWidgetItem* rootItem = ui.tw_pis->invisibleRootItem();
    rootItem->setFlags(Qt::NoItemFlags);

    // Create the "(unassigned)" item and hide it for now
    unassigned = new QTreeWidgetItem();
    unassigned->setText(0, "(unassigned)");
    unassigned->setIcon(0, QIcon(":/icons/ApplicationGroup.png"));
    unassigned->setFlags(unassigned->flags() & ~(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled));
    rootItem->addChild(unassigned);
    unassigned->setHidden(true);

    // Create a map for the projects mapping them to their QTreeWidgetItem
    QMap<QString, QTreeWidgetItem*> projects;

    // Iterate through all directories inside the project path
    for (QFileInfo entry : projectsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        // Get the full path of the current directory on the filesystem
        QString projectPath = entry.absoluteFilePath();

        // If no makefile exists inside the directory then it isn't a project directory
        if (!QFileInfo::exists(QString("%1/makefile").arg(projectPath)))
            continue;

        // Save the name of the project (directory name)
        QString projectName = entry.fileName();

        // Create a new QTreeWidgetItem for the project
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, projectName);
        item->setIcon(0, QIcon(":/icons/MakefileProject.png"));
        item->setFlags(item->flags() ^ Qt::ItemIsDragEnabled);
        rootItem->addChild(item);

        // Save the QTreeWidgetItem inside the map
        projects[projectName] = item;

        // Get the argument line of the project
        read_argv(projectName, projectPath);
    }

    // Return the project to QTreeWidgetItem map
    return projects;
}

bool MainWindow::check_piConfig(piConfig* pi_config, QList<QString> projects, QString* warnings) {
    // If no name was set
    if (pi_config->name == "")
        return false;

    // If no address was set
    if (pi_config->addr == "") {
        warnings->append(QString("No address was set for Client %1.\n").arg(pi_config->name));
        return false;
    }

    // Iterate through all created RaspberryPi objects
    for (RaspberryPi* pi : pis) {
        // If the name already exists
        if (pi_config->name == pi->name) {
            warnings->append(QString("Duplicate name: %1.\n").arg(pi_config->name));
            return false;
        }

        // If the address already exists
        if (pi_config->addr == pi->addr) {
            warnings->append(QString("Client %1 has the same address as Client %2 (%3).\n").arg(pi_config->name, pi->name, pi_config->addr));
            return false;
        }
    }

    // If no project was set
    if (pi_config->project == "")
        pi_config->projectValid = false;
    // If the project doesn't exist inside the project path
    else if (!projects.contains(pi_config->project))
        pi_config->projectValid = false;
    else
        pi_config->projectValid = true;

    return true;
}

void MainWindow::create_piNodes(QList<piConfig> pi_configs, QMap<QString, QTreeWidgetItem*> project2item) {
    // Create an empty string for possible warnings
    QString warnings = "";

    // Get the list containing all project names
    QList<QString> projects = project2item.keys();

    // Iterate through all Raspberry Pi configurations
    for (piConfig pi_config : pi_configs) {
        // Check the configuration, skip if there is something not right
        if (!check_piConfig(&pi_config, projects, &warnings))
            continue;

        // Create a new QTreeWidgetItem for the Raspberry Pi
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, QString("%1 | %2").arg(pi_config.addr, pi_config.name));
        item->setIcon(0, QIcon(":/icons/StatusNotStarted.png"));
        item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);

        // Check if the project of the Raspberry Pi is valid/exists
        if (pi_config.projectValid) {
            // Show the checkbox of the item
            item->setCheckState(0, Qt::Unchecked);

            // Get the QTreeWidgetItem of the project
            QTreeWidgetItem* projectItem = project2item[pi_config.project];

            // Add the Raspberry Pi item to the project item and expand the project item
            projectItem->addChild(item);
            projectItem->setExpanded(true);
        }
        else {
            // Show the "unassigned" item and add the Raspberry Pi item to it
            unassigned->setHidden(false);
            unassigned->addChild(item);
            unassigned->setExpanded(true);
        }

        // Iterate through all arguments of the Raspberry Pi
        for (QString key : pi_config.args.keys()) {
            // Create an QTreeWidgetItem for the argument and add it to the Raspberry Pi item
            QTreeWidgetItem* argItem = new QTreeWidgetItem();
            argItem->setText(0, QString("%1=%2").arg(key, pi_config.args[key]));
            argItem->setIcon(0, QIcon(":/icons/Parameter.png"));
            argItem->setFlags(argItem->flags() & ~(Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled));
            item->addChild(argItem);
        }

        // Create a new QTextEdit for the output and change its size to fit the TabWidget
        QTextEdit* te_appOutput = new QTextEdit(this);
        te_appOutput->setFixedSize(ui.tabW_appOutput->width() - 6, ui.tabW_appOutput->height() - 24);

        // Set the font of the QTextEdit and make it readonly
        QFont font = QFont("Cascadia Mono", 10);
        font.setStyleStrategy(QFont::PreferAntialias);
        te_appOutput->setFont(font);
        te_appOutput->setReadOnly(true);

        // Add the QTextEdit to the output TabWidget
        ui.tabW_appOutput->addTab(te_appOutput, pi_config.name);

        // Create a new RaspberryPi object and add it to the list of Raspberry Pis
        RaspberryPi* pi = new RaspberryPi(this, pi_config, projects_argv[pi_config.project], item, te_appOutput);
        pis.push_back(pi);

        // Map the QTreeWidgetItem and QTextEdit to the RaspberryPi object
        item2pi[item] = pi;
        output2pi[te_appOutput] = pi;
    }

    // If there are warnings
    if (!warnings.isEmpty())
        // Show the warnings inside a QMessageBox
        QMessageBox::warning(this, "Warning(s)", warnings);
}

void MainWindow::refreshTrees() {
    // Read the configuration file and save the Raspberry Pi configurations
    QList<piConfig> pi_configs = readConfig();

    // Get the projects from the project path and save the QTreeWidgetItem map
    QMap<QString, QTreeWidgetItem*> project2item = getProjects();

    // Create the Raspberry Pi items inside the QTreeWidget
    create_piNodes(pi_configs, project2item);
}

void MainWindow::writeConfig() {
    // Try to open the configuration textfile for writing
    QFile file("testbed_workspace/testbedkonfiguration.txt");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // If the file couldn't be opened show an error message and return
        QMessageBox::critical(this, "Error", "The current configuration wasn't saved because the file testbed_workspace/testbedkonfiguration.txt couldn't be opened.");
        return;
    }

    // Create a TextStream for writing the file
    QTextStream out(&file);

    // Write the projectsPath to the file
    out << "projectsPath=" << projectsPath << '\n';

    // Iterate through all Raspberry Pis
    for (RaspberryPi* pi : pis) {
        // Write the name, address, and project to the file
        out << "\n["      << pi->name    << "]\n"
            << "address=" << pi->addr    <<  '\n'
            << "project=" << pi->project <<  '\n'
            << "args=";

        // If there are arguments for the Raspberry Pi
        if (!pi->args.isEmpty()) {
            // Save the arguments inside a list of strings
            QStringList args = pi->args.keys();

            // Write the first argument and its value to the file
            out << args[0] << ':' << pi->args[args[0]];

            // Write the remaining arguments to the file
            for (int i = 1; i < args.length(); i++)
                out << ',' << args[i] << ':' << pi->args[args[i]];
        }

        out << '\n';
    }

    // Close the file
    file.close();
}

void MainWindow::btns_setEnabled(bool enable) {
    // Enable or disable all (action) buttons of the application
    ui.btn_save->setEnabled(enable);
    ui.btn_compile->setEnabled(enable);
    ui.btn_start->setEnabled(enable);
    ui.btn_stop->setEnabled(enable);
    ui.btn_refresh->setEnabled(enable);
}

void MainWindow::btn_save_clicked() {
    // Indicates that no Raspberry Pi has been selected from the QTreeWidget
    bool noPiSelected = true;

    // Create an empty string for possible warnings
    QString warnings = "";

    // Iterate through all Raspberry Pis
    for (RaspberryPi* pi : pis) {
        // If the checkbox of the Raspberry Pi QTreeWidgetItem is checked
        if (pi->item->checkState(0) == Qt::CheckState::Checked) {
            // At least one Raspberry Pi has been selected
            noPiSelected = false;

            switch (pi->status) {
                // If the Raspberry Pi is online and the application isn't running on it
                case piStatus::online:
                    // Signal to save the project of the Raspberry Pi
                    pi->SSHsave(projectsPath);

                    // Increment the busy Raspberry Pi count
                    busyPiCount++;

                    break;

                // If the Raspberry Pi is offline
                case piStatus::offline:
                    // Show a warning that the Raspberry Pi isn't connected
                    warnings.append(QString("%1 is not connected.\n").arg(pi->name));
                    break;

                // If the application is currently running on the Raspberry
                case piStatus::appRunning:
                    // Show a warning that the application is running on the Raspberry Pi
                    warnings.append(QString("An application is running on %1.\n").arg(pi->name));
                    break;

                // If the status of the Raspberry Pi is still being checked
                case piStatus::checking:
                    // Show a warning that the status of the Raspberry Pi is still being checked
                    warnings.append(QString("Still ckecking the status of %1.\n").arg(pi->name));
                    break;
            }
        }
    }

    // Check if at least one Raspberry Pi has been selected
    if (!noPiSelected) {
        // Check if the saving operation is executed on at least one Raspberry Pi 
        if (busyPiCount > 0) {
            // Disable all buttons of the application to not allow another operation during the current saving
            btns_setEnabled(false);

            // Show that the save operation has been started
            ui.lw_infoOutput->addItem("### Save started ###\n");
        }

        // If there are warnings
        if (!warnings.isEmpty())
            // Show the warnings inside a QMessageBox
            QMessageBox::warning(this, "Warning(s)", warnings);
    }
    else
        // If no Raspberry Pi has been selected show a warning
        QMessageBox::warning(this, "Warning", "No Raspberry Pi selected.");
}

void MainWindow::btn_compile_clicked() {
    // Indicates that no Raspberry Pi has been selected from the QTreeWidget
    bool noPiSelected = true;

    // Create an empty string for possible warnings
    QString warnings = "";

    // Iterate through all Raspberry Pis
    for (RaspberryPi* pi : pis) {
        // If the checkbox of the Raspberry Pi QTreeWidgetItem is checked
        if (pi->item->checkState(0) == Qt::CheckState::Checked) {
            // At least one Raspberry Pi has been selected
            noPiSelected = false;

            switch (pi->status) {
                // If the Raspberry Pi is online and the application isn't running on it
                case piStatus::online:
                    // Signal to build the project of the Raspberry Pi
                    pi->SSHcompile();

                    // Increment the busy Raspberry Pi count
                    busyPiCount++;

                    break;

                // If the Raspberry Pi is offline
                case piStatus::offline:
                    // Show a warning that the Raspberry Pi isn't connected
                    warnings.append(QString("%1 is not connected.\n").arg(pi->name));
                    break;

                // If the application is currently running on the Raspberry
                case piStatus::appRunning:
                    // Show a warning that the application is running on the Raspberry Pi
                    warnings.append(QString("An application is running on %1.\n").arg(pi->name));
                    break;

                // If the status of the Raspberry Pi is still being checked
                case piStatus::checking:
                    // Show a warning that the status of the Raspberry Pi is still being checked
                    warnings.append(QString("Still ckecking the status of %1.\n").arg(pi->name));
                    break;
            }
        }
    }

    // Check if at least one Raspberry Pi has been selected
    if (!noPiSelected) {
        // Check if the building operation is executed on at least one Raspberry Pi 
        if (busyPiCount > 0) {
            // Disable all buttons of the application to not allow another operation during the current building
            btns_setEnabled(false);

            // Show that the building operation has been started
            ui.lw_infoOutput->addItem("### Build started ###\n");
        }

        // If there are warnings
        if (!warnings.isEmpty())
            // Show the warnings inside a QMessageBox
            QMessageBox::warning(this, "Warning(s)", warnings);
    }
    else
        // If no Raspberry Pi has been selected show a warning
        QMessageBox::warning(this, "Warning", "No Raspberry Pi selected.");
}

void MainWindow::btn_start_clicked() {
    // Indicates that no Raspberry Pi has been selected from the QTreeWidget
    bool noPiSelected = true;

    // Create an empty string for possible warnings
    QString warnings = "";

    // Iterate through all Raspberry Pis
    for (RaspberryPi* pi : pis) {
        // If the checkbox of the Raspberry Pi QTreeWidgetItem is checked
        if (pi->item->checkState(0) == Qt::CheckState::Checked) {
            // At least one Raspberry Pi has been selected
            noPiSelected = false;

            switch (pi->status) {
                // If the Raspberry Pi is online and the application isn't running on it
                case piStatus::online:
                    // Signal to start the project of the Raspberry Pi
                    pi->SSHstart();

                    // Increment the busy Raspberry Pi count
                    busyPiCount++;

                    break;

                // If the Raspberry Pi is offline
                case piStatus::offline:
                    // Show a warning that the Raspberry Pi isn't connected
                    warnings.append(QString("%1 is not connected.\n").arg(pi->name));
                    break;

                // If the application is already running on the Raspberry
                case piStatus::appRunning:
                    // Show a warning that the application is already running on the Raspberry Pi
                    warnings.append(QString("Application %1 is already running on %2.\n").arg(pi->project, pi->name));
                    break;

                // If the status of the Raspberry Pi is still being checked
                case piStatus::checking:
                    // Show a warning that the status of the Raspberry Pi is still being checked
                    warnings.append(QString("Still ckecking the status of %1.\n").arg(pi->name));
                    break;
            }
        }
    }

    // Check if at least one Raspberry Pi has been selected
    if (!noPiSelected) {
        // Check if the starting operation is executed on at least one Raspberry Pi 
        if (busyPiCount > 0)
            // Disable all buttons of the application to not allow another operation during the current starting
            btns_setEnabled(false);

        // If there are warnings
        if (!warnings.isEmpty())
            // Show the warnings inside a QMessageBox
            QMessageBox::warning(this, "Warning(s)", warnings);
    }
    else
        // If no Raspberry Pi has been selected show a warning
        QMessageBox::warning(this, "Warning", "No Raspberry Pi selected.");
}

void MainWindow::btn_stop_clicked() {
    // Indicates that no Raspberry Pi has been selected from the QTreeWidget
    bool noPiSelected = true;

    // Create an empty string for possible warnings
    QString warnings = "";

    // Iterate through all Raspberry Pis
    for (RaspberryPi* pi : pis) {
        // If the checkbox of the Raspberry Pi QTreeWidgetItem is checked
        if (pi->item->checkState(0) == Qt::CheckState::Checked) {
            // At least one Raspberry Pi has been selected
            noPiSelected = false;

            switch (pi->status) {
                // If the application is already running on the Raspberry
                case piStatus::appRunning:
                    // Signal to stop the project of the Raspberry Pi
                    pi->SSHstop();

                    // Increment the busy Raspberry Pi count
                    busyPiCount++;

                    break;

                // If the Raspberry Pi is offline
                case piStatus::offline:
                    // Show a warning that the Raspberry Pi isn't connected
                    warnings.append(QString("%1 is not connected.\n").arg(pi->name));
                    break;

                // If the Raspberry Pi is online but the application isn't running on it
                case piStatus::online:
                    // Show a warning that the application isn't running on the Raspberry Pi
                    warnings.append(QString("Application %1 is not running on %2.\n").arg(pi->project, pi->name));
                    break;

                // If the status of the Raspberry Pi is still being checked
                case piStatus::checking:
                    // Show a warning that the status of the Raspberry Pi is still being checked
                    warnings.append(QString("Still ckecking the status of %1.\n").arg(pi->name));
                    break;
            }
        }
    }

    // Check if at least one Raspberry Pi has been selected
    if (!noPiSelected) {
        // Check if the stopping operation is executed on at least one Raspberry Pi 
        if (busyPiCount > 0)
            // Disable all buttons of the application to not allow another operation during the current stopping
            btns_setEnabled(false);

        // If there are warnings
        if (!warnings.isEmpty())
            // Show the warnings inside a QMessageBox
            QMessageBox::warning(this, "Warning(s)", warnings);
    }
    else
        // If no Raspberry Pi has been selected show a warning
        QMessageBox::warning(this, "Warning", "No Raspberry Pi selected.");
}

void MainWindow::btn_refresh_clicked() {
    // Signal all RaspberryPi objects to delete themselves
    clearRaspberryPis();

    // Reset the project path and clear the project to arguments map
    projectsPath = "";
    projects_argv.clear();

    // Reset the root path of the FileSystemModel to force a refresh
    fileSystemModel->setRootPath("");

    // Clear the Raspberry Pi list, the QTreeWidgetItem and QTextEdit map, and the QTreeWidget
    pis.clear();
    item2pi.clear();
    output2pi.clear();
    ui.tw_pis->clear();
    
    // Temporary block all signals of the TabWidget so that they are not triggered during the clearing and adding
    QSignalBlocker blocker(ui.tabW_appOutput);

    // Clear the TabWidget to remove all tabs resp. output QTextEdits
    ui.tabW_appOutput->clear();

    // Refresh the Raspberry Pi and file tree
    refreshTrees();
}

void MainWindow::chbx_selectAll_stateChanged(int state) {
    // Iterate through all projects of the QTreeWidget, start at project index i = 1 to ignore the unassigned item
    for (int i = 1; i < ui.tw_pis->topLevelItemCount(); i++)
        // Iterate through all Raspberry Pis of the project
        for (int j = 0; j < ui.tw_pis->topLevelItem(i)->childCount(); j++)
            // Check or uncheck the checkbox of the Raspberry Pi item depending on chbx_selectAll
            ui.tw_pis->topLevelItem(i)->child(j)->setCheckState(0, state ? Qt::Checked : Qt::Unchecked);
}

void MainWindow::tw_pis_itemChanged(QTreeWidgetItem*, int) {
    // Indicates that there is no Raspberry Pi or non with a valid project
    bool noPi = true;

    // Iterate through all projects of the QTreeWidget, start at project index i = 1 to ignore the unassigned item
    for (int i = 1; i < ui.tw_pis->topLevelItemCount(); i++) {
        // Iterate through all Raspberry Pis of the project
        for (int j = 0; j < ui.tw_pis->topLevelItem(i)->childCount(); j++) {
            // There is at least one Raspberry Pi with a valid project
            noPi = false;

            // Check if the checkbox of the current Raspberry is unchecked
            if (ui.tw_pis->topLevelItem(i)->child(j)->checkState(0) == Qt::Unchecked) {
                // If the checkbox of at least one Raspberry Pi is unchecked uncheck chbx_selectAll and return
                ui.chbx_selectAll->setCheckState(Qt::Unchecked);
                return;
            }
        }
    }

    // If there is no Raspberry Pi with a valid project uncheck chbx_selectAll, else check chbx_selectAll
    ui.chbx_selectAll->setCheckState(noPi ? Qt::Unchecked : Qt::Checked);
}

void MainWindow::tw_pis_itemPressed(QTreeWidgetItem* item, int) {
    // Get the Raspberry Pi of the clicked/pressed QTreeWidetItem
    RaspberryPi* pi = item2pi[item];

    // Check if the clicked/pressed item was a Raspberry Pi (and not a project or argument)
    if (pi != nullptr)
        // If the Raspberry Pi is offline show the error message
        ui.lbl_error->setText(pi->status == piStatus::offline ? QString("Error: %1").arg(pi->error) : "");
}

void MainWindow::tw_pis_itemDropped(QTreeWidgetItem*) {
    // Clear the error label because after the drop no Raspberry Pi is selected
    ui.lbl_error->clear();

    // Iterate through all projects of the QTreeWidget, start at project index i = 1 to ignore the unassigned item
    for (int i = 1; i < ui.tw_pis->topLevelItemCount(); i++) {
        // Save the name of the current project
        QString project = ui.tw_pis->topLevelItem(i)->text(0);

        // Iterate through all Raspberry Pis of the project
        for (int j = 0; j < ui.tw_pis->topLevelItem(i)->childCount(); j++) {
            // Get the current QTreeWidgetItem
            QTreeWidgetItem* item = ui.tw_pis->topLevelItem(i)->child(j);

            // Get the RaspberryPi from the current QTreeWidgetItem
            RaspberryPi* pi = item2pi[item];

            // Check if a new Project was selected for the Raspberry Pi
            if (pi->project != project) {
                // Save the project inside the RaspberryPi object and calculate the new argument line for it
                pi->project = project;
                pi->calc_argv(projects_argv[pi->project]);

                // If the Raspberry Pi hadn't a valid project until now
                if (!pi->projectValid) {
                    // The project of the Raspberry Pi is now valid
                    pi->projectValid = true;

                    // Show the checkbox of the item now
                    item->setCheckState(0, Qt::Unchecked);
                }
            }
        }
    }

    // If the "unassigned" item is currently visible
    if (!unassigned->isHidden())
        // If there are no more Raspberry Pis with unassigned projects
        if (unassigned->childCount() == 0)
            // Hide the "unassigned" item
            unassigned->setHidden(true);

    // Write the current Raspberry Pi configuration back to the configuration file
    writeConfig();
}

void MainWindow::tv_files_doubleClicked(QModelIndex index) {
    // If the double-clicked item is a file
    if (!fileSystemModel->isDir(index)) {
        // Get the path of the file
        QString filePath = fileSystemModel->filePath(index);

        // Try to open the file
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(filePath)))
            // If the file couldn't be opened show an error message
            QMessageBox::critical(this, "Error", QString("Couldn't open %1.").arg(filePath));
    }
}

void MainWindow::SSHcurrStatus(RaspberryPi* pi, piStatus currStatus, QByteArray error) {
    // Save the old status of the Raspberry Pi
    piStatus oldStatus = pi->status;

    // If the application is currently running and it wasn't running before
    if (currStatus == piStatus::appRunning && oldStatus != piStatus::appRunning) {
        // Save the current status and clear the error message
        pi->status = currStatus;
        pi->error = "";

        // Change the icon of the QTreeWidgetItem and disable its dragability
        pi->item->setIcon(0, QIcon(":/icons/ApplicationRunning.png"));
        pi->item->setFlags(pi->item->flags() & ~Qt::ItemIsDragEnabled);

        // If the QTreeWidgetItem is currently selected
        if (pi->item->isSelected())
            // Clear the error label
            ui.lbl_error->clear();
    }
    // If the Raspberry Pi is currently online but he wasn't online before or the application was running on him
    else if (currStatus == piStatus::online && oldStatus != piStatus::online) {
        // Save the current status
        pi->status = currStatus;

        // Change the icon of the QTreeWidgetItem and make it dragable
        pi->item->setIcon(0, QIcon(":/icons/StatusOK.png"));
        pi->item->setFlags(pi->item->flags() | Qt::ItemIsDragEnabled);

        // If the Raspberry Pi was offline before
        if (oldStatus == piStatus::offline) {
            // Clear the error message
            pi->error = "";

            // Clear his output QTextEdit because it will be read again
            pi->te_output->clear();

            // If the QTreeWidgetItem is currently selected
            if (pi->item->isSelected())
                // Clear the error label
                ui.lbl_error->clear();
        }
    }
    // If the Raspberry Pi is currently offline
    else if (currStatus == piStatus::offline) {
        // If the Raspberry Pi wasn't offline before
        if (oldStatus != piStatus::offline) {
            // Save the current status
            pi->status = currStatus;

            // Change the icon of the QTreeWidgetItem and make it dragable
            pi->item->setIcon(0, QIcon(":/icons/StatusInvalid.png"));
            pi->item->setFlags(pi->item->flags() | Qt::ItemIsDragEnabled);
        }

        // Save the error message
        pi->error = error;

        // If the QTreeWidgetItem is currently selected
        if (pi->item->isSelected())
            // Show the error message
            ui.lbl_error->setText(QString("Error: %1").arg(pi->error));
    }
}

void MainWindow::SSHsave_finished(RaspberryPi* pi, bool, QByteArray output) {
    // Add the output of the Raspberry Pi to the info list
    ui.lw_infoOutput->addItem(QString("%1:\n%2").arg(pi->name, QString(output)));

    // Decrement the busy Raspberry Pi counter, check if there are no more Raspberry Pis busy
    if (--busyPiCount == 0) {
        // Show that the operation has finished inside the info list
        ui.lw_infoOutput->addItem("### Save finished ###\n");

        // Re-enable all buttons
        btns_setEnabled(true);
    }
}

void MainWindow::SSHcompile_finished(RaspberryPi* pi, bool, QByteArray output) {
    // Add the output of the Raspberry Pi to the info list
    ui.lw_infoOutput->addItem(QString("%1:\n%2").arg(pi->name, QString(output)));

    // Decrement the busy Raspberry Pi counter, check if there are no more Raspberry Pis busy
    if (--busyPiCount == 0) {
        // Show that the operation has finished inside the info list
        ui.lw_infoOutput->addItem("### Build finished ###\n");

        // Re-enable all buttons
        btns_setEnabled(true);
    }
}

void MainWindow::SSHstart_finished(RaspberryPi* pi, bool started, QByteArray output) {
    // If the application has started successfully
    if (started)
        // Clear the output TextEdit for upcoming output
        pi->te_output->clear();

    // Add the output of the Raspberry Pi to the info list
    ui.lw_infoOutput->addItem(QString("%1: %2").arg(pi->name, QString(output)));

    // Decrement the busy Raspberry Pi counter, check if there are no more Raspberry Pis busy
    if (--busyPiCount == 0)
        // Re-enable all buttons
        btns_setEnabled(true);
}

void MainWindow::SSHstop_finished(RaspberryPi* pi, bool, QByteArray output) {
    // Add the output of the Raspberry Pi to the info list
    ui.lw_infoOutput->addItem(QString("%1: %2").arg(pi->name, QString(output)));

    // Decrement the busy Raspberry Pi counter, check if there are no more Raspberry Pis busy
    if (--busyPiCount == 0)
        // Re-enable all buttons
        btns_setEnabled(true);
}

void MainWindow::SSHnewOutput(RaspberryPi* pi, QByteArray output) {
    // Save the pointer to the scrollbar from the QTextEdit of the Raspberry Pi and save the current value of the scrollbar
    QScrollBar* scrollBar = pi->te_output->verticalScrollBar();
    int value = scrollBar->value();

    // Check if the scrollbar is at it's maximum and save the result
    bool scrolledToBottom = value == scrollBar->maximum();

    // Append the output to the end of the QTextEdit
    pi->te_output->moveCursor(QTextCursor::End);
    pi->te_output->insertPlainText(output);

    // If the scrollbar was at it's maximum scroll to the bottom again, else it should keep its old value
    scrollBar->setValue(scrolledToBottom ? scrollBar->maximum() : value);
}
