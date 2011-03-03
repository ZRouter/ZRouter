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

#make TARGET_PAIR=NorthQ/NQ-900 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} TARGET_PROFILES="SMALL_ mpd dhcp ssh nfs_client" kernel_bin_gz rootfs.iso.ulzma  2>&1 | tee $0.${DATE}.log
# TARGET_PROFILES="SMALL_ mpd dhcp ssh"
if [ "x$1" != "x" ]; then
	make TARGET_PAIR=NorthQ/NQ-900 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} $1  2>&1 | tee $0.${DATE}.log
else
	make TARGET_PAIR=NorthQ/NQ-900 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} fwimage  2>&1 | tee $0.${DATE}.log
fi
cp /usr/obj/usr/home/ray/work/DDTeam.net/ZRouter/zrouter/NorthQ_NQ-900.trx /tftpboot/NQ-900/
