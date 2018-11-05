##################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Ralink
SOC_CHIP=RT2880F_FDT
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304

WITHOUT_WIRELESS=yes

###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}

KERNCONF_FDT_DTS_FILE?=	"MZK-W300NAG.dts"

KERNCONF_DEVICES+=	gpioiic
KERNCONF_DEVICES+=	iicbb

KERNCONF_DEVICES+=	iicbus
KERNCONF_DEVICES+=	iic
KERNCONF_DEVICES+=	mtk_iic

#KERNCONF_DEVICES+=	mdio
KERNCONF_DEVICES+=	etherswitch
KERNCONF_DEVICES+=	miiproxy
KERNCONF_DEVICES+=	rtl8366rb
WORLD_SUBDIRS_SBIN+=	etherswitchcfg

KERNCONF_OPTIONS+=     ROOTDEVNAME=\\\"cd9660:cfid0s.rootfs.uzip\\\" 

# Builded modules
# device wlan in kernel alredy enable this modules
#KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt

###################################################
#
#       Limits
#
###################################################



# Image must not be biggest than GEOM_MAP_P2 (upgrade part.)
FIRMWARE_IMAGE_SIZE_MAX=0x00390000


###################################################
#
#       Firmware Image Options
#
###################################################

#WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

# This module always boot from bc450014. Than add space to head.
UBOOT_HEAD_WHITESPACE=20

MKULZMA_BLOCKSIZE=65536

PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage


