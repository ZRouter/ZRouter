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
make TARGET_PAIR=Soekris/4501 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} kernel rootfs.ffs 2>&1 | tee $0.${DATE}.log > /dev/null


echo ${TARGET_PAIR} firmware build done at `date`

