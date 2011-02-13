




WORLD_SUBDIRS_LIB+= \
libalias \
libbsm \
libc \
libedit \
libelf \
libexpat \
libcrypt \
libipsec \
libipx \
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
libutil \
libz \
ncurses

WORLD_SUBDIRS+= \
secure/lib/libcrypto \
secure/lib/libssl

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
init \
ipfw \
kldload \
kldstat \
kldunload \
ldconfig \
md5 \
mount \
mount_cd9660 \
mount_msdosfs \
ping \
rcorder \
reboot \
route \
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
login \
minigzip \
netstat \
passwd \
sed \
tail \
tar \
tee \
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
libreadline \
libssp \
libstdc++
# XXX: libreadline must be replaced with libedit


WORLD_SUBDIRS_LIB+= \
libfetch \
libradius \
libarchive \
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

WORLD_SUBDIRS_USR_SBIN+= \
ppp \
ngctl \
nghook \
chroot \
cron \
pwd_mkdb \
inetd \
gpioctl
