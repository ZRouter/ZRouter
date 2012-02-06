
###################################################
#
# Board used hardware/chip`s
#
###################################################

SOC_VENDOR=Broadcom
SOC_CHIP=BCM5354
# TODO: size suffixes
BOARD_FLASH_SIZE=16777216

KERNEL_COMPRESSION=lzma.4.17
KERNEL_COMPRESSION_TYPE=gzip

###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}
# Include usb and SoC usb controller drivers
WITH_USB=yes

# Builded modules
KERNCONF_MODULES_OVERRIDE+=	wlan_xauth wlan_wep wlan_tkip wlan_acl \
    wlan_amrr wlan_ccmp wlan_rssadapt
KERNCONF_MODULES_OVERRIDE+=	usb/run usb/rum firmware
KERNCONF_MODULES_OVERRIDE+=	random zlib
KERNCONF_MODULES_OVERRIDE+=	if_bridge bridgestp
KERNCONF_MODULES_OVERRIDE+=	msdosfs
KERNCONF_MODULES_OVERRIDE+=	md
KERNCONF_MODULES_OVERRIDE+=	ipfw dummynet libalias
KERNCONF_MODULES_OVERRIDE+=	geom/geom_label geom/geom_uncompress
KERNCONF_MODULES_OVERRIDE+=	ufs
KERNCONF_MODULES_OVERRIDE+=	usb/umass cam
KERNCONF_MODULES_OVERRIDE+=	usb/uplcom usb/u3g usb/umodem usb/ucom

WORLD_SUBDIRS_SBIN+=	devctl

KERNCONF_OPTIONS+=	DEVCTL_ATTACH_ENABLED
KERNCONF_OPTIONS+=	ALT_BREAK_TO_DEBUGGER


KERNCONF_DEVICES+=	spibus
KERNCONF_DEVICES+=	gpiospi
KERNCONF_DEVICES+=	mx25l

# Additional utilities
#WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade


# Flash mapping
# ifndef GEOM_MAP_P0_PARENT then GEOM_MAP_P0_PARENT=cfid0
GEOM_MAP_P0_NAME=cfe
GEOM_MAP_P0_START=0x00000000
GEOM_MAP_P0_END=0x00040000
GEOM_MAP_P0_ATTR=RO

GEOM_MAP_P1_NAME="kernel"
GEOM_MAP_P1_START=0x00040000
GEOM_MAP_P1_END=0x00200000
#GEOM_MAP_P1_END="search"
#GEOM_MAP_P1.searchkey="HDR0"
#GEOM_MAP_P1.searchstart=0x00100000
#GEOM_MAP_P1.searchstep=32

GEOM_MAP_P2_END=0x00ff0000
GEOM_MAP_P2_NAME="rootfs"
GEOM_MAP_P2.offset="28"
GEOM_MAP_P2_START=0x00200000
#GEOM_MAP_P2_START="search"
#GEOM_MAP_P2.searchkey="HDR0"
#GEOM_MAP_P2.searchstart=0x00100000
#GEOM_MAP_P2.searchstep=32

GEOM_MAP_P3_NAME="nvram"
GEOM_MAP_P3_START=0x00ff0000
GEOM_MAP_P3_END=0x01000000

GEOM_MAP_P4_NAME=upgrade
GEOM_MAP_P4_START=0x00040000
GEOM_MAP_P4_END=0x01000000




###################################################
#
#       Limits
#
###################################################


# 3Mb max size, since CFE loaded at 0xa0300000
KERNEL_SIZE_MAX=3145728
# KERNEL_COMPRESSED_SIZE_MAX is unknown, limit between kernel and rootfs float 
# (splited by HDR0 key, TRX header)

MKULZMA_BLOCKSIZE=65536

# Image must not be biggest than GEOM_MAP_P4 (upgrade part.)
FIRMWARE_IMAGE_SIZE_MAX=0x007c0000

TARGET_PROFILES+=SMALL_ mpd ssh hostap dhcp net_help nfs_client
TARGET_PROFILES+=shttpd dlink.ua.web

IMAGE_TYPE=trx
IMAGE_SUFFIX=trx

PACKING_KERNEL_IMAGE?=kernel.kbin.gz.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

NEW_IMAGE_TYPE=trximage
