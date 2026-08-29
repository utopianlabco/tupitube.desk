QT += opengl core gui svg xml network
CONFIG += plugin warn_on
TEMPLATE = lib
TARGET = tupirotationtool

unix {
    !include(../../../../global_variables.pri) {
        error("rotation.pro: Run ./configure first!")
    }
}

win32 {
    include(../../../../win.pri)
}

INSTALLS += target 
target.path = /plugins/

HEADERS += \
           rotation_configurator.h \
           rotation_settings.h \
           rotation_tweener.h

SOURCES += \
           rotation_configurator.cpp \
           rotation_settings.cpp \
           rotation_tweener.cpp

FRAMEWORK_DIR = "../../../framework"
include($$FRAMEWORK_DIR/framework.pri)

LIBBASE_DIR = ../../../libbase
STORE_DIR = ../../../store
LIBTUPI_DIR = ../../../libtupi
COMMON_DIR = ../common

include($$LIBBASE_DIR/libbase.pri)	
include($$STORE_DIR/store.pri)
include($$LIBTUPI_DIR/libtupi.pri)
include($$COMMON_DIR/common.pri)
