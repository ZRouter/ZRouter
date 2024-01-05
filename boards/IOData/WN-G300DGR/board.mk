###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Ralink
SOC_CHIP=RT3050F_FDT
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304

# use for mac address in flash
WORLD_SUBDIRS_USR_BIN+=hexdump awk

###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}

ZKERNCONF_FDT_DTS_FILE?="dts/mips/WN-G300DGR.dts"

KERNCONF_OPTIONS+=     ROOTDEVNAME=\\\"cd9660:cfid0s.rootfs.uzip\\\" 

KERNCONF_DEVICES+=      gpioiic
KERNCONF_DEVICES+=      iicbb
#KERNCONF_DEVICES+=      mtk_iic

KERNCONF_DEVICES+=      iicbus
KERNCONF_DEVICES+=      iic
WORLD_SUBDIRS_USR_SBIN+=        i2c

WITHOUT_SPI=yes

KERNCONF_DEVICES+=     mdio
KERNCONF_DEVICES+=      etherswitch
KERNCONF_DEVICES+=      miiproxy
KERNCONF_DEVICES+=      rtl8366rb
WORLD_SUBDIRS_SBIN+=    etherswitchcfg
#KERNCONF_OPTIONS+=	RTL8366_SOFT_RESET

# Include usb and SoC usb controller drivers
# This module have usb but not no space in flash
#WITH_USB=yes
# Builded modules
# device wlan in kernel alredy enable this modules
#KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt
#KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib
KERNCONF_MODULES_OVERRIDE+=usb/umass cam zlib
#KERNCONF_MODULES_OVERRIDE+=cam zlib
#KERNCONF_MODULES_OVERRIDE+=usb/run runfw

# Additional utilities
#WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade


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

PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage

KERNCONF_DEVICES+=iic
KERNCONF_DEVICES+=iicbb
KERNCONF_DEVICES+=iicbus
#KERNCONF_DEVICES+=gpioiic
