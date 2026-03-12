QT += opengl core gui svg xml network
TEMPLATE = lib
TARGET = tupifwcore

macx {
    CONFIG += plugin warn_on
} else {
    CONFIG += warn_on dll
}

unix {
    !include(../tupconfig.pri) {
        error("Run ./configure first!")
    }
    # Include libsndfile paths from system
    INCLUDEPATH += /usr/local/libsndfile/include /usr/include
    LIBS += -L/usr/local/libsndfile/lib
}

win32 {
    include(../../../win.pri)
}

INSTALLS += target
target.path = /lib/

contains("DEFINES", "ADD_HEADERS") {
    INSTALLS += headers 
    headers.files += *.h
    headers.path = /include/tupicore
}

HEADERS += talgorithm.h \
           tapplicationproperties.h \
           tconfig.h \
           tglobal.h \
           tuivalues.h \
           # tipdatabase.h \
           tcachehandler.h \
           tapptheme.h \
           taudiosampler.h

SOURCES += talgorithm.cpp \
           tapplicationproperties.cpp \
           tconfig.cpp \
           # tipdatabase.cpp \
           tcachehandler.cpp \
           tapptheme.cpp \
           taudiosampler.cpp

unix {
    LIBS += -lsndfile
}
