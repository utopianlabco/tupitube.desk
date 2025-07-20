INSTALLS = icons 

icons.target = .
icons.commands = chmod 755 *png; cp *.png $(INSTALL_ROOT)/themes/dark/icons
icons.path = /themes/dark/icons

CONFIG += warn_on

TEMPLATE = subdirs
