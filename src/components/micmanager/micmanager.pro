QT += widgets multimedia
CONFIG += dll warn_on
TEMPLATE = lib 
TARGET = tupimicmanager

INSTALLS += target
target.path = /lib/

FRAMEWORK_DIR = "../../framework"
include($$FRAMEWORK_DIR/framework.pri)

unix {
  !include(../../../global_variables.pri) {
    error("micmanager.pro: Run ./configure first!")
  }
}

win32 {
  include(../../../win.pri)
}

HEADERS = tupmicmanager.h \
          tupmiclevel.h

SOURCES = tupmicmanager.cpp \
          tupmiclevel.cpp
