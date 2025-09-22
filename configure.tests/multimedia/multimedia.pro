QT += multimedia multimediawidgets
TEMPLATE = app
CONFIG -= moc
TARGET = multimedia

macx {
    CONFIG -= app_bundle
    CONFIG += warn_on static console
}

# Input
SOURCES += main.cpp
