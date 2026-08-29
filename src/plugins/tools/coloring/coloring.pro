QT += opengl core gui svg xml network
CONFIG += plugin warn_on
TEMPLATE = lib
TARGET = tupicoloringtool

unix {
    !include(../../../../global_variables.pri) {
        error("coloring.pro: Run ./configure first!")
    }
}

win32 {
    include(../../../../win.pri)
}

INSTALLS += target 
target.path = /plugins/

HEADERS += \
           coloring_configurator.h \
           coloring_settings.h \
           coloring_tweener.h

SOURCES += \
           coloring_configurator.cpp \
           coloring_settings.cpp \
           coloring_tweener.cpp

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
