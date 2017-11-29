###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Ralink
SOC_CHIP=RT3050F_FDT
# TODO: size suffixes
BOARD_FLASH_SIZE=8388608


###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}

KERNCONF_FDT_DTS_FILE?= "CG-WLR300GNV.dts"

KERNCONF_OPTIONS+=     ROOTDEVNAME=\\\"cd9660:cfid0s.rootfs.uzip\\\" 

# Include usb and SoC usb controller drivers
WITH_USB=yes
# Builded modules
# device wlan in kernel alredy enable this modules
#KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt
#KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib
KERNCONF_MODULES_OVERRIDE+=usb/umass cam zlib
#KERNCONF_MODULES_OVERRIDE+=cam zlib
#KERNCONF_MODULES_OVERRIDE+=usb/run runfw


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
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

MKULZMA_BLOCKSIZE=65536

PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage

#KERNCONF_DEVICES+=umass
#KERNCONF_DEVICES+=scbus

KERNCONF_DEVICES+=wlan_xauth
KERNCONF_DEVICES+=wlan_tkip
KERNCONF_DEVICES+=wlan_ccmp
KERNCONF_DEVICES+=wlan_wep
KERNCONF_DEVICES+=wlan_acl
KERNCONF_DEVICES+=wlan_amrr
KERNCONF_DEVICES+=wlan_rssadapt

KERNCONF_DEVICES+=run
KERNCONF_DEVICES+=firmware

KERNCONF_DEVICES+=iic
KERNCONF_DEVICES+=iicbb
KERNCONF_DEVICES+=iicbus
#KERNCONF_DEVICES+=gpioiic
