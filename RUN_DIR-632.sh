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

echo "build started, enter \`tail -f $0.${DATE}.log\` to view build details"
make TARGET_PAIR=D-Link/DIR-632 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} zimage 2>&1 | tee $0.${DATE}.log > /dev/null
#make TARGET_PAIR=D-Link/DIR-632 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} kernel.oldlzma.uboot rootfs.iso.ulzma 2>&1 | tee $0.${DATE}.log > /dev/null
#cp /usr/obj/usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-632_kernel.oldlzma.uboot /tftpboot/DIR-632/
#cp /usr/obj/usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-632_rootfs_clean.iso.ulzma /tftpboot/DIR-632/


echo ${TARGET_PAIR} firmware build done at `date`

