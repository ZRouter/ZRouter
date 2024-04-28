
###################################################
#
# Board used hardware/chip`s
#
###################################################

SOC_VENDOR=Broadcom
SOC_CHIP=BCM3383
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304

KERNEL_COMPRESSION=lzma.4.17
KERNEL_COMPRESSION_TYPE=gzip

KERNCONF_KERNLOADADDR?=0x80040000

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
# device wlan in kernel alredy enable this modules
#KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt
#KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib

KERNCONF_OPTIONS+=ALT_BREAK_TO_DEBUGGER

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

PACKING_KERNEL_IMAGE?=kernel.kbin
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma
#PACKING_ROOTFS_IMAGE?=rootfs_clean.iso

IMAGE_OPTION=-s 0x3383 -v 8271.8486
IMAGE_SUFFIX=psimage
NEW_IMAGE_TYPE=psimage

