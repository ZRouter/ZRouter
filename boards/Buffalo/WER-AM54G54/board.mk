
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Atheros
SOC_CHIP=AR5312
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304


###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}
#KERNCONF_KERNLOADADDR?=		0x80010000
KERNCONF_KERNLOADADDR?=		0x80004000
KERNCONF_KERN_LDSCRIPT_NAME=	ldscript.mips.bin

KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/redboot/rootfs.uzip\\\"
KERNCONF_DEVICES+=	geom_redboot

# HY57V281620HCT-H * 2
#KERNCONF_OPTIONS+=     AR531X_REALMEM=(32*1024*1024)

KERNCONF_OPTIONS+=	CFI_HARDWAREBYTESWAP

#WITH_IPSEC=yes

WITHOUT_WIRELESS=yes

# This module have KS8995XA. But no contole connect from cpu.
KERNCONF_OPTIONS+=	ARE_MDIO
KERNCONF_DEVICES+=	miiproxy
KERNCONF_DEVICES+=	mdio
KERNCONF_DEVICES+=	etherswitch
KERNCONF_DEVICES+=	ukswitch
WORLD_SUBDIRS_SBIN+=	etherswitchcfg

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
PACKING_KERNEL_IMAGE?=kernel.kbin.gz.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage

UBNT_VERSION?=FON.ar5315.Zrouter.V1

NEW_IMAGE_TYPE=ubntimage

