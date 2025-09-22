CONFIG += ordered warn_on
TEMPLATE = subdirs

win32|macx {
  SUBDIRS = json-c \
            libmypaint \
            qtmypaint \
            raster
} else {
  # Linux
  SUBDIRS = libmypaint \
            qtmypaint \
            raster
}
