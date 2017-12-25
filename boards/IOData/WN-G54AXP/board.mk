
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Idt
SOC_CHIP=79RC32334
# TODO: size suffixes
BOARD_FLASH_SIZE=8388608

KERNCONF_FDT_DTS_FILE=	"WN-G54AXP.dts"

###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}
KERNCONF_KERNLOADADDR?=0x80050000
# Define empty LDSCRIPT_NAME, FreeBSD kernel make process will use his default
KERNCONF_KERN_LDSCRIPT_NAME=
KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/flash/spi0s.rootfs.uzip\\\"


KERNCONF_DEVICES+=	miibus
KERNCONF_DEVICES+=	rl

# Include usb and SoC usb controller drivers

WITHOUT_WIRELESS=yes

KERNCONF_OPTIONS+=	FFS

###################################################
#
#       Limits
#
###################################################

# Image must not be biggest than GEOM_MAP_P2 (upgrade part.)
FIRMWARE_IMAGE_SIZE_MAX=0x007a0000

###################################################
#
#       Firmware Image Options
#
###################################################

KERNEL_COMPRESSION=gz
KERNEL_COMPRESSION_TYPE=gz
UBOOT_KERNEL_COMPRESSION_TYPE=gzip

MKULZMA_BLOCKSIZE=65536
 
PACKING_KERNEL_IMAGE?=kernel.kbin.gz.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage
