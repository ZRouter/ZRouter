
KERNCONF_DEVICES+= \
	uhid

WORLD_SUBDIRS_LIB+= \
	libusbhid

WORLD_SUBDIRS_USR_BIN+= \
	usbhidctl \
	usbhidaction
