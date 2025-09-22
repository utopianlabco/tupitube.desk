QT += opengl
TEMPLATE = app
CONFIG -= moc
TARGET = opengl 

macx {
    CONFIG -= app_bundle
    CONFIG += warn_on static console
}

# Input
SOURCES += main.cpp
