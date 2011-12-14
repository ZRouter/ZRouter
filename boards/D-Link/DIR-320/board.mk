
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
KERNCONF_MAKEOPTIONS+=	"MODULES_OVERRIDE=\"ipfw usb/umass usb/uplcom usb/u3g dummynet\""

# Additional utilities
WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade


# Flash mapping
# ifndef GEOM_MAP_P0_PARENT then GEOM_MAP_P0_PARENT=cfid0
GEOM_MAP_P0_NAME=cfe
GEOM_MAP_P0_START=0x00000000
GEOM_MAP_P0_END=0x00030000
GEOM_MAP_P0_ATTR=RO

GEOM_MAP_P1_NAME=rgdb
GEOM_MAP_P1_START=0x00030000
GEOM_MAP_P1_END=0x00040000

GEOM_MAP_P2_NAME=upgrade
GEOM_MAP_P2_START=0x00040000
GEOM_MAP_P2_END=0x003e0000

GEOM_MAP_P3_NAME="nvram"
GEOM_MAP_P3_START=0x003f8000
GEOM_MAP_P3_END=0x00400000

GEOM_MAP_P4_NAME="bdcfg"
GEOM_MAP_P4_START=0x003f6000
GEOM_MAP_P4_END=0x003f8000

GEOM_MAP_P5_NAME="kernel"
GEOM_MAP_P5_START=0x00040000
GEOM_MAP_P5_END="search"
GEOM_MAP_P5.searchkey="--PaCkImGs--"
GEOM_MAP_P5.searchstart="524384" # 8 64K blocks + AlphaHeaderSize( 96B )
GEOM_MAP_P5.searchstep="65536"

GEOM_MAP_P6_END=0x003e0000
GEOM_MAP_P6_NAME="rootfs"
GEOM_MAP_P6.offset="32"
GEOM_MAP_P6_START="search"
GEOM_MAP_P6.searchkey="--PaCkImGs--"
GEOM_MAP_P6.searchstart="524384" # 8 64K blocks + AlphaHeaderSize( 96B )
GEOM_MAP_P6.searchstep="65536"

GEOM_MAP_P7_NAME="langpack"
GEOM_MAP_P7_START=0x003e0000
GEOM_MAP_P7_END=0x003f0000



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

NEW_IMAGE_TYPE=trximage
