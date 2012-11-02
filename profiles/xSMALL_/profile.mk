
WORLD_BUILD_ENV_EXTRA+=WITHOUT_OPENSSL=yes

WORLD_SUBDIRS_LIB+= \
	libalias \
	libbsm \
	libc \
	libedit \
	libelf \
	libexpat \
	libcrypt \
	libgeom \
	libkiconv \
	libkvm \
	libmd \
	libmemstat \
	msun \
	ncurses/ncurses \
	libpmc \
	libsbuf \
	libthr \
	libufs \
	libutil \
	libz \
	libfetch \
	libarchive \
	libmp \
	libnetgraph \
	libpam/libpam \
	libpam/modules/pam_unix \
	librt \
	libbz2 \
	liblzma \
	libwrap

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
	mount \
	mount_cd9660 \
	mount_nullfs \
	mount_unionfs \
	newfs \
	ping \
	rcorder \
	reboot \
	route \
	switchctl \
	sysctl \
	umount

WORLD_SUBDIRS_ZROUTER+= \
	target/sbin/cdevd \
	target/usr.bin/hex2bin

WORLD_SUBDIRS_USR_BIN+= \
	basename \
	cap_mkdb \
	dirname \
	fetch \
	find \
	getopt \
	grep \
	head \
	id \
	killall \
	logger \
	login \
	minigzip \
	passwd \
	sed \
	sockstat \
	tail \
	tar \
	tee \
	touch \
	uname \
	vi \
	wc \
	xargs

WORLD_SUBDIRS_GNU_USR_BIN+= \
	sort

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
	ngctl \
	nghook \
	chroot \
	cron \
	pwd_mkdb \
	inetd \
	gpioctl \
	syslogd

.if defined(WITH_USB)
WORLD_SUBDIRS_USR_SBIN+= \
	usbconfig \
	usbdump

WORLD_SUBDIRS_LIB+= \
	libusb
.endif

KERNCONF_MODULES_OVERRIDE+= \
	if_bridge bridgestp \
	libalias/libalias \
	libalias/modules/ftp \
	libalias/modules/pptp \
	geom/geom_label \
	md unionfs ufs

