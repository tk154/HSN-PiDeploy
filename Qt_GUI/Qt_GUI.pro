QT += core gui widgets

HEADERS += Headers/MainWindow.h Headers/qtreewidgetpis.h Headers/RaspberryPi.h Headers/SSH.h
SOURCES += Sources/MainWindow.cpp Sources/qtreewidgetpis.cpp Sources/RaspberryPi.cpp Sources/SSH.cpp Sources/main.cpp
FORMS   += MainWindow.ui

UI_DIR = Headers
RC_ICONS = pi.ico
RESOURCES = Qt_GUI.qrc
