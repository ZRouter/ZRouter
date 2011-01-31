#!/bin/sh

HOST=`hostname`
echo "'${HOST}'"
if [ "x${HOST}" = "xrnote.ddteam.net" ] ; then
FREEBSD_SRC_TREE=/usr/home/ray/work/@HG/http_my_ddteam_net_hg_BASE_/head/
else
FREEBSD_SRC_TREE=/usr/1/MIPS_FreeBSD/HEAD/head
fi
DATE=`date +%Y-%m-%d_%H:%M`


__MAKE_CONF=/dev/null
SRCCONF=/dev/null

export __MAKE_CONF
export SRCCONF

_TARGET_VARS="TARGET_VENDOR=D-Link TARGET_DEVICE=DIR-320 TARGET_PROFILES=\"SMALL_ zhttpd\""
_SETTINGS="FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE}"
_TARGETS=kernel-build kernel-install rootfs kernel_bin_gz_trx rootfs.iso.ulzma

make ${_TARGET_VARS} ${_SETTINGS} ${_TARGETS} 2>&1 | tee $0.${DATE}.log

#make 
#TARGET_VENDOR=D-Link TARGET_DEVICE=DIR-320 TARGET_PROFILES="SMALL_ zhttpd" 
#FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE}
#rootfs.iso.ulzma kernel_bin_oldlzma kernel_bin_xz kernel_bin_gz kernel_bin_bz2  2>&1 | tee $0.${DATE}.log
