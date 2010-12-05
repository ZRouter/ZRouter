#!/bin/sh

HOST=`hostname`
echo "'${HOST}'"
if [ "x${HOST}" = "xrnote.ddteam.net" ] ; then
FREEBSD_SRC_TREE=/usr/home/ray/work/@HG/http_my_ddteam_net_hg_BASE_/head/
else
FREEBSD_SRC_TREE=/usr/1/MIPS_FreeBSD/HEAD/head
fi

make TARGET_VENDOR=D-Link TARGET_DEVICE=DIR-320 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} TARGET_PROFILES="SMALL_ openvpn zhttpd"
#make TARGET_VENDOR=D-Link TARGET_DEVICE=DIR-320 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} TARGET_PROFILES="SMALL_ openvpn" port-build
#make TARGET_VENDOR=D-Link TARGET_DEVICE=DIR-320 FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} TARGET_PROFILES="SMALL_ openvpn"    port-build-depend-host PORT_BUILD_DEPEND_HOST=/usr/ports/security/openscep 
