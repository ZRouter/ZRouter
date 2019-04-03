
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Atheros
#SOC_CHIP=AR7240
SOC_CHIP=AR9132
# TODO: size suffixes
BOARD_FLASH_SIZE=32M
#4194304


###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}
# Include usb and SoC usb controller drivers
# USB ports on DIR-615 E4 unwired
WITH_USB=yes
#WITH_IPSEC=yes
#WITH_WIRELESS=yes
# Builded modules
#KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib

KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/map/rootfs.uzip\\\"

# Additional utilities
#WORLD_SUBDIRS_ZROUTER+=	target/sbin/upgrade
#WORLD_SUBDIRS_SBIN+=	devctl

#KERNCONF_OPTIONS+=	DEVCTL_ATTACH_ENABLED
KERNCONF_OPTIONS+=	ALT_BREAK_TO_DEBUGGER
KERNCONF_OPTIONS+=	BREAK_TO_DEBUGGER

KERNCONF_DEVICES+=      pckbd
KERNCONF_DEVICES+=      evdev
KERNCONF_DEVICES+=      uinput
KERNCONF_OPTIONS+=	EVDEV_SUPPORT

KERNCONF_OPTIONS+=	NBUF=1024

KERNCONF_OPTIONS+=	AR71XX_ENV_UBOOT

KERNCONF_OPTIONS+=	AR71XX_REALMEM=(64*1024*1024)

KERNCONF_DEVICES+=      gpioiic
KERNCONF_DEVICES+=      iicbb
KERNCONF_DEVICES+=      iicbus
KERNCONF_DEVICES+=      iic

KERNCONF_OPTIONS+=	ARGE_MDIO
KERNCONF_DEVICES+=      etherswitch
KERNCONF_DEVICES+=      miiproxy
KERNCONF_DEVICES+=      rtl8366rb
WORLD_SUBDIRS_SBIN+=	etherswitchcfg

KERNCONF_DEVICES+=      cfi
KERNCONF_DEVICES+=      cfid
KERNCONF_OPTIONS+=	CFI_HARDWAREBYTESWAP

.if !defined(WITHOUT_WIRELESS)
KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt
KERNCONF_OPTIONS+=	IEEE80211_DEBUG
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_MESH
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_TDMA
KERNCONF_DEVICES+=	wlan
KERNCONF_DEVICES+=	wlan_amrr
KERNCONF_DEVICES+=	wlan_wep
KERNCONF_DEVICES+=	wlan_ccmp
KERNCONF_DEVICES+=	wlan_tkip

#KERNCONF_OPTIONS+=	ATH_DEBUG
KERNCONF_OPTIONS+=	ATH_DIAGAPI
KERNCONF_OPTIONS+=	ATH_ENABLE_11N

#KERNCONF_OPTIONS+=	AH_DEBUG
KERNCONF_OPTIONS+=	AH_SUPPORT_AR9130
#KERNCONF_OPTIONS+=	AH_DEBUG_ALQ
#KERNCONF_OPTIONS+=	AH_RXCFG_SDMAMW_4BYTES
# interrupt mitigation not possible on AR9130
# option		AH_AR5416_INTERRUPT_MITIGATION
KERNCONF_DEVICES+=	alq
KERNCONF_DEVICES+=	ath
#KERNCONF_DEVICES+=	ath_hal
KERNCONF_DEVICES+=	ath_rate_sample
KERNCONF_DEVICES+=	ath_ahb
KERNCONF_DEVICES+=	ath_ar5212
KERNCONF_DEVICES+=	ath_ar5416
KERNCONF_DEVICES+=	ath_ar9130
.endif

###################################################
#
#       Limits
#
###################################################

# Image must not be biggest than GEOM_MAP_P2 (upgrade part.)
# 3538944
FIRMWARE_IMAGE_SIZE_MAX=0x00f60000

###################################################
#
#       Firmware Image Options
#
###################################################

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

MKULZMA_BLOCKSIZE=131072

PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage
