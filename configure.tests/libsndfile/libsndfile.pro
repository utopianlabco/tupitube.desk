TEMPLATE = app
CONFIG -= moc
TARGET = libsndfile 

macx {
    CONFIG -= app_bundle
    CONFIG += warn_on static console
    INCLUDEPATH += . /usr/local/libsndfile/include
    LIBS += -L/usr/local/libsndfile/lib -lsndfile
}

# Input
SOURCES += main.cpp
