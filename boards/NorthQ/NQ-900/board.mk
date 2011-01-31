
###################################################
#
# Board used hardware/chip`s
#
###################################################

SOC_VENDOR=Broadcom
SOC_CHIP=BCM5354
# Maybe used for kernel config and maybe multiple e.g. "cfi nand"
BOARD_FLASH_TYPE=cfi
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304

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
KERNCONF_MAKEOPTIONS+=	"MODULES_OVERRIDE=\"ipfw libalias dummynet usb/umass usb/uplcom usb/u3g\""

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

# Image must not be biggest than GEOM_MAP_P4 (upgrade part.)
FIRMWARE_IMAGE_SIZE_MAX=0x007c0000

