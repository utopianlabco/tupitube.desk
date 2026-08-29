QT += opengl core gui svg xml network
CONFIG += plugin warn_on
TEMPLATE = lib
TARGET = tupisheartool

unix {
    !include(../../../../global_variables.pri) {
        error("shear.pro: Run ./configure first!")
    }
}

win32 {
    include(../../../../win.pri)
}

INSTALLS += target 
target.path = /plugins/

HEADERS += \
           shear_configurator.h \
           shear_settings.h \
           shear_tweener.h

SOURCES += \
           shear_configurator.cpp \
           shear_settings.cpp \
           shear_tweener.cpp

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
