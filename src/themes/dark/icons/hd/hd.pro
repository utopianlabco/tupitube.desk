INSTALLS = hd

hd.target = .
hd.commands = chmod 755 *png; cp *.png $(INSTALL_ROOT)/themes/dark/icons/hd
hd.path = /themes/dark/icons/hd

CONFIG += warn_on

TEMPLATE = subdirs
