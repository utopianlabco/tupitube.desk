CONFIG += ordered warn_on
TEMPLATE = subdirs

win32|macx {
  SUBDIRS = json-c \
            libmypaint \
            qtmypaint \
            raster
} else {
  defined(DEBIAN_OS, var) {
    SUBDIRS = libmypaint \
              qtmypaint \
              raster
  } else {
    SUBDIRS = json-c \
              libmypaint \
              qtmypaint \
              raster
  }
}
