###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Ralink
SOC_CHIP=RT3050F_FDT
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304

###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}

ZKERNCONF_FDT_DTS_FILE?=  "dts/mips/WHR-G300N.dts"

.if defined(WITH_USBROOTFS)
KERNCONF_OPTIONS+=     ROOTDEVNAME=\\\"cd9660:da0\\\" 
.else
KERNCONF_OPTIONS+=     ROOTDEVNAME=\\\"cd9660:cfid0s.rootfs.uzip\\\" 
.endif

KERNCONF_DEVICES+=	mtk_iic
KERNCONF_DEVICES+=	iicbus
KERNCONF_DEVICES+=	iic
WORLD_SUBDIRS_USR_SBIN+=	i2c

# Include usb and SoC usb controller drivers
#WITH_USB=yes
WITHOUT_WIRELESS=yes
# Builded modules
# device wlan in kernel alredy enable this modules
#KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt
#KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib
#KERNCONF_MODULES_OVERRIDE+=usb/umass cam zlib
#KERNCONF_MODULES_OVERRIDE+=cam zlib
#KERNCONF_MODULES_OVERRIDE+=usb/run runfw

###################################################
#
#       Limits
#
###################################################



# Image must not be biggest than GEOM_MAP_P2 (upgrade part.)
FIRMWARE_IMAGE_SIZE_MAX=0x003b0000


###################################################
#
#       Firmware Image Options
#
###################################################

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

MKULZMA_BLOCKSIZE=65536

.if defined(WITH_USBROOTFS)
PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso
.else
PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma
.endif
IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage
