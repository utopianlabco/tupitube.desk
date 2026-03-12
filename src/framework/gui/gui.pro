QT += opengl core gui svg xml network
TEMPLATE = lib
TARGET = tupifwgui

macx {
    CONFIG += plugin warn_on
} else {
    CONFIG += warn_on dll
}

unix {
    !include(../tupconfig.pri) {
        error("Run ./configure first!")
    }
    # Include libsndfile path for taudiosampler.h
    INCLUDEPATH += /usr/local/libsndfile/include /usr/include
}

win32 {
    include(../../../win.pri)
}

INSTALLS += target
target.path = /lib/

contains(DEFINES, ADD_HEADERS) {
    INSTALLS += headers 
    headers.files += *.h
    headers.path = /include/tupigui
}

HEADERS += taction.h \
           tactionmanager.h \
           tanimwidget.h \
           tapplication.h \
           tbackupdialog.h \
           tbutton.h \
           tbuttonbar.h \
           tcellview.h \
           tclicklineedit.h \
           tcollapsiblewidget.h \
           tcolorbutton.h \
           tcombobox.h \
           tconfigurationdialog.h \
           tcontrolnode.h \
           tdoublespinboxcontrol.h \
           tfontchooser.h \
           tformfactory.h \
           timagebutton.h \
           timagelabel.h \
           tinputfield.h \
           titemselector.h \
           tlabel.h \
           tmainwindow.h \
           tmainwindowabstractsettings.h \
           tmouthtarget.h \
           tmoviegenerator.h \
           tmoviegeneratorinterface.h \
           tnodegroup.h \
           tseparator.h \
           toptionaldialog.h \
           tosd.h \
           tpushbutton.h \
           tradiobutton.h \
           tradiobuttongroup.h \
           tresponsiveui.h \
           tspinboxcontrol.h \
           tstylecombobox.h \
           tabbedmainwindow.h \
           tabdialog.h \
           toolview.h \
           treelistwidget.h \
           treewidgetsearchline.h \
           tvhbox.h \
           tviewbutton.h \
           txyspinbox.h \
           tcolorcell.h \
           tslider.h \
           tcolorarrow.xpm \
           tcolorreset.xpm \
           timagedialog.h \
           twaveformwidget.h

SOURCES += taction.cpp \
           tactionmanager.cpp \
           tanimwidget.cpp \
           tapplication.cpp \
           tbackupdialog.cpp \
           tbutton.cpp \
           tbuttonbar.cpp \
           tcellview.cpp \
           tclicklineedit.cpp \
           tcolorbutton.cpp \
           tcollapsiblewidget.cpp \
           tcombobox.cpp \
           tconfigurationdialog.cpp \
           tcontrolnode.cpp \
           tdoublespinboxcontrol.cpp \
           tfontchooser.cpp \
           tformfactory.cpp \
           tlabel.cpp \
           timagebutton.cpp \
           timagelabel.cpp \
           tinputfield.cpp \
           titemselector.cpp \
           tmainwindow.cpp \
           tmouthtarget.cpp \
           tmoviegenerator.cpp \
           tnodegroup.cpp \
           tseparator.cpp \
           toptionaldialog.cpp \
           tosd.cpp \
           tpushbutton.cpp \
           tradiobutton.cpp \
           tradiobuttongroup.cpp \
           tresponsiveui.cpp \
           tspinboxcontrol.cpp \
           tstylecombobox.cpp \
           tabbedmainwindow.cpp \
           tabdialog.cpp \
           toolview.cpp \
           treelistwidget.cpp \
           treewidgetsearchline.cpp \
           tvhbox.cpp \
           tviewbutton.cpp \
           txyspinbox.cpp \
           tcolorcell.cpp \
           tslider.cpp \
           timagedialog.cpp \
           twaveformwidget.cpp

INCLUDEPATH += ../core ../ ../../libbase

linux-g {
    TARGETDEPS += ../core/libtupifwcore.so
}

unix {
    LIBS += -L../core -ltupifwcore
    # INCLUDEPATH += ../core ../ ../../libbase
    INCLUDEPATH += ../core
}

win32 {
    LIBS += -L../core/release/ -ltupifwcore
    INCLUDEPATH += ../core
}
