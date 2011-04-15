###################################################
#
# Board used hardware/chip`s
#
###################################################

SOC_VENDOR=Ralink
SOC_CHIP=RT3052F


# Maybe used for kernel config and maybe multiple e.g. "cfi nand"
BOARD_FLASH_TYPE=cfi
# TODO: size suffixes
BOARD_FLASH_SIZE=8388608

KERNEL_COMPRESSION=lzma.4.17
KERNEL_COMPRESSION_TYPE=oldlzma

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
#KERNCONF_MAKEOPTIONS+=	"MODULES_OVERRIDE=\"ipfw libalias dummynet usb/umass usb/uplcom usb/u3g\""
KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt random if_bridge bridgestp msdosfs md ipfw dummynet libalias geom/geom_label geom/geom_uncompress ufs usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib

# Additional utilities
#WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade


# Flash mapping
# ifndef GEOM_MAP_P0_PARENT then GEOM_MAP_P0_PARENT=cfid0
#GEOM_MAP_P0_NAME=uboot
#GEOM_MAP_P0_START=0x00000000
#GEOM_MAP_P0_END=0x00030000
#GEOM_MAP_P0_ATTR=RO




###################################################
#
#       Limits
#
###################################################


# 3Mb max size, since CFE loaded at 0xa0300000
#KERNEL_SIZE_MAX=3145728
# KERNEL_COMPRESSED_SIZE_MAX is unknown, limit between kernel and rootfs float 
# (splited by HDR0 key, TRX header)

# Image must not be biggest than GEOM_MAP_P4 (upgrade part.)
FIRMWARE_IMAGE_SIZE_MAX=0x007b0000
