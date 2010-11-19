




WORLD_SUBDIRS_LIB+= \
libc \
libedit \
libcrypt \
libmd \
libkvm \
msun \
libsbuf \
libthr \
libutil \
libz \
ncurses

# XXX must define NOENABLE_WIDEC someway

WORLD_SUBDIRS_BIN+= \
cat \
cp \
chmod \
date \
dd \
expr \
hostname \
kenv \
kill \
ln \
ls \
mkdir \
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
ipfw \
kldload \
kldstat \
kldunload \
ldconfig \
mount \
mount_cd9660 \
mount_msdosfs \
ping \
rcorder \
route \
sysctl \
umount




WORLD_SUBDIRS+=../../../../../${ZROUTER_ROOT}/target/sbin/cdevd


.warning "SMALL_ profile used"
