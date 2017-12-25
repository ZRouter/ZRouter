
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Atheros
SOC_CHIP=AR5315
# TODO: size suffixes
BOARD_FLASH_SIZE=8388608


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
KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/redboot/rootfs.uzip\\\"
KERNCONF_DEVICES+=	geom_redboot

# Include usb and SoC usb controller drivers

WITHOUT_WIRELESS=yes

KERNCONF_OPTIONS+=	ARE_MDIO
KERNCONF_DEVICES+=	miiproxy
KERNCONF_DEVICES+=	mdio
KERNCONF_DEVICES+=	etherswitch
WORLD_SUBDIRS_SBIN+=	etherswitchcfg

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

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma

MKULZMA_BLOCKSIZE=65536

UBNT_KERNEL_LOAD_ADDRESS?=0x80050000
PACKING_KERNEL_IMAGE?=kernel.gz.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage

UBNT_VERSION?=FON.ar5315.Zrouter.V1

NEW_IMAGE_TYPE=ubntimage

