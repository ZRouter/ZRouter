




WORLD_SUBDIRS_LIB+= \
libalias \
libbsm \
libc \
libedit \
libelf \
libexpat \
libcrypt \
libgeom \
libipsec \
libjail \
libkiconv \
libkvm \
libmd \
libmemstat \
msun \
libpmc \
libsbuf \
libtacplus \
libthr \
libufs \
libutil \
libz \
ncurses/ncurses \
ncurses/form \
ncurses/menu \
ncurses/panel

WORLD_SUBDIRS+= \
secure/lib/libcrypto \
secure/lib/libssl

#libipx \
# Compile ping without libipsec and ifconfig without libjail,libipx
# "libcrypto.so.6" not found, required by "bsdtar"
# "libssl.so.6" not found, required by "fetch"
# "libbsm.so.3" not found, required by "groups"
# "libbsm.so.3" not found, required by "id"
# "libbsm.so.3" not found, required by "login"
# "libmemstat.so.3" not found, required by "netstat"

#libgcc_s.so.1

# XXX must define NOENABLE_WIDEC someway

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
ipfw \
kldload \
kldstat \
kldunload \
ldconfig \
md5 \
mdconfig \
mount \
mount_cd9660 \
mount_msdosfs \
mount_nullfs \
mount_unionfs \
newfs \
newfs_msdos \
ping \
rcorder \
reboot \
route \
switchctl \
sysctl \
umount

#/sbin/oinit



WORLD_SUBDIRS_ZROUTER+=target/sbin/cdevd
WORLD_SUBDIRS_ZROUTER+=target/usr.bin/hex2bin




WORLD_SUBDIRS_USR_BIN+= \
basename \
dirname \
fetch \
find \
getopt \
grep \
head \
id \
killall \
login \
minigzip \
netstat \
passwd \
sed \
tail \
tar \
tee \
telnet \
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
libssp \
libstdc++

#libreadline \
# XXX: libreadline must be replaced with libedit


WORLD_SUBDIRS_LIB+= \
libfetch \
libradius \
libarchive \
libmp \
libnetgraph \
libpam/libpam \
libpam/modules/pam_unix \
librt \
libbz2 \
liblzma \
libwrap

WORLD_SUBDIRS_LIBEXEC+= \
rtld-elf \
getty \
telnetd

#ppp \
WORLD_SUBDIRS_USR_SBIN+= \
ngctl \
nghook \
chroot \
cron \
pwd_mkdb \
inetd \
gpioctl

.if defined(WITH_USB)
WORLD_SUBDIRS_USR_SBIN+= \
usbconfig \
usbdump

WORLD_SUBDIRS_LIB+= \
libusb
.endif


KERNCONF_MODULES_OVERRIDE+=if_bridge bridgestp ipfw dummynet
KERNCONF_MODULES_OVERRIDE+=libalias/libalias
#KERNCONF_MODULES_OVERRIDE+=libalias/modules/cuseeme
KERNCONF_MODULES_OVERRIDE+=libalias/modules/ftp
KERNCONF_MODULES_OVERRIDE+=libalias/modules/pptp
#KERNCONF_MODULES_OVERRIDE+=libalias/modules/smedia
#KERNCONF_MODULES_OVERRIDE+=libalias/modules/dummy
#KERNCONF_MODULES_OVERRIDE+=libalias/modules/irc
#KERNCONF_MODULES_OVERRIDE+=libalias/modules/nbt
#KERNCONF_MODULES_OVERRIDE+=libalias/modules/skinny
KERNCONF_MODULES_OVERRIDE+=geom/geom_label
KERNCONF_MODULES_OVERRIDE+=md unionfs msdosfs ufs

