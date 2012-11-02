
###################################################
#
# Board used hardware/chip`s
#
###################################################

SOC_VENDOR=Broadcom
SOC_CHIP=BCM5354
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304

KERNEL_COMPRESSION=lzma.4.17

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
KERNCONF_MODULES_OVERRIDE+=	usb/umass usb/uplcom usb/u3g

# Additional utilities
WORLD_SUBDIRS_ZROUTER+=		target/sbin/upgrade

###################################################
#
#       Limits
#
###################################################


# 3Mb max size, since CFE loaded at 0xa0300000
KERNEL_SIZE_MAX=3145728
# KERNEL_COMPRESSED_SIZE_MAX is unknown, limit between kernel and rootfs float 
# (splited by --PaCkImGs-- key)

# Image must not be biggest than GEOM_MAP_P2 (upgrade part.)
FIRMWARE_IMAGE_SIZE_MAX=0x003a0000

PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

NEW_IMAGE_TYPE=trximage
