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
make TARGET_PAIR=D-Link/DIR-620Sec FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} zimage 2>&1 | tee $0.${DATE}.log > /dev/null
#make TARGET_PAIR=D-Link/DIR-620Sec FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} kernel.oldlzma.uboot rootfs.iso.ulzma 2>&1 | tee $0.${DATE}.log > /dev/null
cp /usr/obj/usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-620Sec_kernel.oldlzma.uboot /tftpboot/DIR-620Sec/
cp /usr/obj/usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-620Sec_rootfs_clean.iso.ulzma /tftpboot/DIR-620Sec/
#cp /usr/obj/usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-620Sec-?.?-*.*.zimage /linux/var/www/my.ddteam.net/data/files/


echo ${TARGET_PAIR} firmware build done at `date`

