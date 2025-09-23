QT += core gui widgets xml

TEMPLATE = lib
CONFIG += dll warn_on

TARGET = qtmypaint

HEADERS += mpbrush.h \
           mptile.h \
           mpsurface.h \
           mphandler.h

SOURCES += mpbrush.cpp \
           mptile.cpp \
           mpsurface.cpp \
           mphandler.cpp

FRAMEWORK_DIR = "../../framework"
include($$FRAMEWORK_DIR/framework.pri)

# --- json-c ---
win32|macx {
  LIBS += -L../json-c -ljson-c
  win32:CONFIG(release, debug|release): LIBS += -L../json-c/release/ -ljson-c
  else:win32:CONFIG(debug, debug|release): LIBS += -L../json-c/debug/ -ljson-c
  else:unix: LIBS += -L../json-c -ljson-c

  INCLUDEPATH += ../json-c
  DEPENDPATH += ../json-c
} else {
  defined(DEBIAN_OS, var) {
    INCLUDEPATH += /usr/include/json-c
    LIBS += -L/usr/lib/x86_64-linux-gnu -ljson-c
  } else {
    INCLUDEPATH += ../json-c
    LIBS += -L../json-c -ljson-c
  }
}

# --- libmypaint ---
LIBS += -L../libmypaint -llibmypaint
win32:CONFIG(release, debug|release): LIBS += -L../libmypaint/release/ -llibmypaint
else:win32:CONFIG(debug, debug|release): LIBS += -L../libmypaint/debug/ -llibmypaint
else:unix: LIBS += -L../libmypaint -llibmypaint

INCLUDEPATH += ../libmypaint
DEPENDPATH += ../libmypaint

win32 {
    include(../../../win.pri)
}

macx {
    INSTALLS += target
    target.path = /lib

    !include(../../../global_variables.pri) {
        error("raster/qtmainpaint.pro: Run ./configure first!")
    }
}

unix:!mac {
    INSTALLS += target
    target.path = /lib/raster

    !include(../../../global_variables.pri) {
        error("raster/qtmainpaint.pro: Run ./configure first!")
    }
}   

QMAKE_CFLAGS += -Wno-unknown-pragmas

