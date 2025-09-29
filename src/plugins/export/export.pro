CONFIG += ordered warn_on
TEMPLATE = subdirs

unix {
    !include(../../../global_variables.pri) {
        error("export.pro: Run ./configure first!")
    }
}

win32 {
    include(../../../win.pri)
}

SUBDIRS += imageplugin 

contains(DEFINES, HAVE_FFMPEG) {
    SUBDIRS += ffmpegplugin
}
