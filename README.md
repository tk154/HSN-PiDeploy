# HSN_PiDeploy

This repository contains a command line and graphical user interface application for deploying, compiling, and executing software projects on a Raspberry Pi.
<br><br>

## Building on Windows
### Prerequisites
* <a href="https://visualstudio.microsoft.com">Microsoft Visual Studio</a> with <code>Desktop development with C++</code> selected while installing
* <a href="https://www.qt.io/download-qt-installer-oss">Qt</a>
* <a href="https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools-19123">Qt Visual Studio Tools</a>
<br><br>

### Building
* Open <code>Pi_Verwaltung.sln</code> using Visual Studio
* Click <code>Build -> Build Solution</code> to build both the command line and graphical user application.
<br><br>

## Building on Linux
### Prerequisites
* <code>qt6-base</code> and <code>libssh</code> libraries
<br><br>

### Building
* To build the command line tool:
<pre>
cd SSH
make
</pre>
<br>

* To build the graphical user interface:
<pre>
cd Qt_GUI
make
</pre>
<br>

## Usage
There is an example <a href="https://github.com/tk154/HSN_PiDeploy/blob/main/Qt_GUI/testbed_workspace/testbedkonfiguration.txt">configuration file</a> and <a href="https://github.com/tk154/HSN_PiDeploy/tree/main/Qt_GUI/pi_workspace">software projects</a> inside this repo.
