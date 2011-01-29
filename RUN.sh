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

make TARGET_VENDOR=D-Link TARGET_DEVICE=DIR-320 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} TARGET_PROFILES="SMALL_ zhttpd" oldlzma rootfs.iso.ulzma kernel_bin_oldlzma kernel_bin_xz kernel_bin_gz kernel_bin_bz2  2>&1 | tee $0.${DATE}.log
