TEMPLATE = app
CONFIG -= moc
TARGET = quazip 
DEPENDPATH += .

macx {
    INCLUDEPATH += /usr/local/quazip/include
    CONFIG -= app_bundle
    CONFIG += warn_on static console
}

SOURCES += main.cpp
