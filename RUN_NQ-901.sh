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

make TARGET_VENDOR=NorthQ TARGET_DEVICE=NQ-901 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} TARGET_PROFILES="SMALL_ zhttpd" makefs_cd9660 kernel-install kernel_bin_oldlzma world-build world-install fwimage 2>&1 | tee $0.${DATE}.log

