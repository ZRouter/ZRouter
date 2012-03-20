#!/bin/sh

SRC=$1

if [ "x${SRC}" = "x" ]; then
	echo "Usage: $0 \${U-Boot source code root dir}"
	exit 1;
fi

# Only tools subdir used to build mkimage utility.

mkdir -p tmp

DEST=tmp

mkdir -p ${DEST}/common
mkdir -p ${DEST}/include/u-boot
mkdir -p ${DEST}/lib/libfdt
mkdir -p ${DEST}/tools


LIST="
common/image.c			\
include/compiler.h		\
include/fdt.h			\
include/fdt_support.h		\
include/image.h			\
include/libfdt.h		\
include/libfdt_env.h		\
include/sha1.h			\
include/u-boot/crc.h		\
include/u-boot/md5.h		\
include/u-boot/zlib.h		\
include/watchdog.h		\
lib/crc32.c			\
lib/libfdt/fdt.c		\
lib/libfdt/fdt_ro.c		\
lib/libfdt/fdt_rw.c		\
lib/libfdt/fdt_strerror.c	\
lib/libfdt/fdt_wip.c		\
lib/libfdt/libfdt_internal.h	\
lib/md5.c			\
lib/sha1.c			\
tools/default_image.c		\
tools/fdt_host.h		\
tools/fit_image.c		\
tools/imximage.c		\
tools/imximage.h		\
tools/kwbimage.c		\
tools/kwbimage.h		\
tools/mkimage.c			\
tools/mkimage.h			\
tools/os_support.c		\
tools/os_support.h"

echo 1
for _FILE_ in ${LIST}; do
echo 2
	echo cp ${SRC}/${_FILE_} ${DEST}/${_FILE_}
	cp ${SRC}/${_FILE_} ${DEST}/${_FILE_}
done

