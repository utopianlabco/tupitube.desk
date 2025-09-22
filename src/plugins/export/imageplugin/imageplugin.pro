QT += opengl core gui svg xml network
CONFIG += plugin warn_on
TEMPLATE = lib
TARGET = tupiimageplugin

unix {
    !include(../../../../global_variables.pri) {
        error("imageplugin.pro: Run ./configure first!")
    }
}

win32 {
    include(../../../../win.pri)
}

INSTALLS += target 
target.path = /plugins/ 

HEADERS += imageplugin.h
SOURCES += imageplugin.cpp

FRAMEWORK_DIR = "../../../framework"
include($$FRAMEWORK_DIR/framework.pri)

LIBBASE_DIR = ../../../libbase
STORE_DIR = ../../../store
LIBTUPI_DIR = ../../../libtupi

include($$LIBBASE_DIR/libbase.pri)
include($$STORE_DIR/store.pri)
include($$LIBTUPI_DIR/libtupi.pri)
