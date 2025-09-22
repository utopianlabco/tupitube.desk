TEMPLATE = app
CONFIG -= moc
TARGET = ffmpeg 

macx {
    CONFIG -= app_bundle
    CONFIG += warn_on static console
    INCLUDEPATH += . /usr/local/ffmpeg/include
    LIBS += -L/usr/local/ffmpeg/lib -lavdevice -lavformat -lavfilter -lavcodec -lavutil -lswresample -lswscale
}

# Input
SOURCES += main.cpp
