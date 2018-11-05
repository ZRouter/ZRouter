# shrink the size of the kernel text segment
# shrink about 8% binary size. compress size is 5% down
KERNCONF_OPTIONS+=MUTEX_NOINLINE
KERNCONF_OPTIONS+=RWLOCK_NOINLINE
KERNCONF_OPTIONS+=SX_NOINLINE

WORLD_BUILD_ENV_EXTRA+=WITHOUT_OPENSSL=yes

WORLD_SUBDIRS_LIB+= \
	lib80211 \
	libbz2 \
	libc \
	libcasper \
	libedit \
	libgeom \
	libgpio \
	libipsec \
	libjail \
	libkiconv \
	libkvm \
	liblzma \
	msun \
	libmd \
	ncurses/ncursesw \
	libpam/libpam \
	libpam/modules/pam_unix \
	libpam/modules/pam_permit \
	libsbuf \
	libufs \
	libutil \
	libwrap \
	libxo

# so use
WORLD_SUBDIRS_LIB+= \
	libexpat \
	libelf \
	libnv \
	libthr \

# pam use
WORLD_SUBDIRS_LIB+= \
	libcrypt \


WORLD_SUBDIRS_LIB_DEL+= \
	libypclnt \

# XXX must define NOENABLE_WIDEC someway for libncurses build.
WORLD_SUBDIRS_BIN+= \
	cat \
	cp \
	chmod \
	date \
	dd \
	df \
	expr \
	hostname \
	kenv \
	kill \
	ln \
	ls \
	mkdir \
	mv \
	ps \
	realpath \
	rm \
	sh \
	sleep \
	stty \
	sync \
	uuidgen

WORLD_SUBDIRS_SBIN+= \
	dmesg \
	ifconfig \
	init \
	kldload \
	kldstat \
	kldunload \
	ldconfig \
	md5 \
	mdconfig \
	mdmfs \
	mount \
	mount_cd9660 \
	mount_nullfs \
	mount_unionfs \
	newfs \
	ping \
	rcorder \
	reboot \
	route \
	sysctl \
	umount

WORLD_SUBDIRS_USR_BIN+= \
	w \
	tftp \
	basename \
	cap_mkdb \
	dirname \
	getopt \
	grep \
	id \
	killall \
	logger \
	login \
	passwd \
	tee \
	touch \
	uname \
	xargs

WORLD_SUBDIRS_GNU_LIB+= \
	csu \
	libgcc \
	libregex \
	libssp

WORLD_SUBDIRS_LIBEXEC+= \
	rtld-elf \
	getty \
	telnetd

WORLD_SUBDIRS_USR_SBIN+= \
	gpioctl \
	chroot \
	cron \
	pwd_mkdb \
	inetd \
	syslogd

.if defined(WITH_USB)
WORLD_SUBDIRS_USR_SBIN+= \
	usbconfig \
	usbdump

WORLD_SUBDIRS_LIB+= \
	libusb
.endif

KERNCONF_MODULES_OVERRIDE+= \
	geom/geom_label \
	md unionfs ufs
