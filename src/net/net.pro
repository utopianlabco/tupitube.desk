QT += opengl core gui svg xml network
TEMPLATE = lib
TARGET = tupinet

macx {
    CONFIG += plugin warn_on
} else {
    CONFIG += dll warn_on
}

unix {
    !include(../../global_variables.pri) {
        error("net.pro: Run ./configure first!")
    }
}

win32 {
    include(../../win.pri)
}

INSTALLS += target
target.path = /lib/

contains("DEFINES", "ADD_HEADERS") {
    INSTALLS += headers
    headers.target = .
    headers.commands = cp *.h $(INSTALL_ROOT)/include/tupinet
    headers.path = /include/tupinet/
}

HEADERS += tupnetprojectmanagerparams.h \
           tupnetprojectmanagerhandler.h \
           tupnetsocket.h \
           tupconnectpackage.h \
           tuplistpackage.h \
           tupopenpackage.h \
           tupsavepackage.h \
           tupnewprojectpackage.h \
           tupnetfilemanager.h \
           tupchat.h \
           tupconnectdialog.h \
           tupprojectparser.h \
           tuplistprojectspackage.h \
           tupprojectlistparser.h \
           tupprojectlistdialog.h \
           tupnotificationparser.h \
           tupackparser.h \
           tupimportprojectpackage.h \
           tupchatpackage.h \
           tupnotice.h \
           tupcollaboratorslist.h \
           # tupnoticepackage.h \
           tupcommunicationparser.h \
           tupimageexportpackage.h \
           tupvideoexportpackage.h \
           tupstoryboardupdatepackage.h \
           tupstoryboardexportpackage.h \
           tupstoryboardparser.h

SOURCES += tupnetprojectmanagerparams.cpp \
           tupnetprojectmanagerhandler.cpp \
           tupnetsocket.cpp \
           tupconnectpackage.cpp \
           tuplistpackage.cpp \
           tupopenpackage.cpp \
           tupsavepackage.cpp \
           tupnewprojectpackage.cpp \
           tupnetfilemanager.cpp \
           tupchat.cpp \
           tupconnectdialog.cpp \
           tupprojectparser.cpp \
           tuplistprojectspackage.cpp \
           tupprojectlistparser.cpp \
           tupprojectlistdialog.cpp \
           tupnotificationparser.cpp \
           tupackparser.cpp \
           tupimportprojectpackage.cpp \
           tupchatpackage.cpp \
           tupnotice.cpp \
           tupcollaboratorslist.cpp \
           # tupnoticepackage.cpp \
           tupcommunicationparser.cpp \
           tupimageexportpackage.cpp \
           tupvideoexportpackage.cpp \
           tupstoryboardupdatepackage.cpp \
           tupstoryboardexportpackage.cpp \
           tupstoryboardparser.cpp

include(net_config.pri)

FRAMEWORK_DIR = "../framework"
include($$FRAMEWORK_DIR/framework.pri)

