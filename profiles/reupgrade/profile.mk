WORLD_SUBDIRS_USR_BIN+=awk mkfifo

KERNCONF_MAKEOPTIONS+=	TMPFS_PAGES_MINRESERVED=(2*1024/4)

KERNCONF_OPTIONS+=   TMPFS
