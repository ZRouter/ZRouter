###################################################
#
# Board used hardware/chip`s
#
###################################################

SOC_VENDOR=Atheros
SOC_CHIP=AR7161
BOARD_FLASH_SIZE=32M

###################################################
#
# Vars for kernel config
#
###################################################

# ident
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}

# Board have 128M of RAM
KERNCONF_OPTIONS+=	AR71XX_REALMEM=128*1024*1024

KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/map/rootfs.uzip\\\"
KERNCONF_DEVICES+=	geom_map

# Include usb and SoC usb controller drivers
WITH_USB=yes
#WITH_IPSEC=yes
#WITH_WIRELESS=yes
# Builded modules
KERNCONF_MODULES_OVERRIDE+=	usb/uplcom usb/u3g usb/umodem usb/umass \
    usb/ucom cam zlib

# Additional utilities
#WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade

KERNCONF_OPTIONS+=	DDB
KERNCONF_OPTIONS+=	KDB
KERNCONF_OPTIONS+=	ALT_BREAK_TO_DEBUGGER
KERNCONF_OPTIONS+=	BREAK_TO_DEBUGGER

KERNCONF_OPTIONS+=	ARGE_MDIO
KERNCONF_DEVICES+=	etherswitch
KERNCONF_DEVICES+=	miiproxy
KERNCONF_DEVICES+=	arswitch
WORLD_SUBDIRS_SBIN+=    etherswitchcfg

.if !defined(WITHOUT_WIRELESS)
KERNCONF_MODULES_OVERRIDE+=	wlan_xauth wlan_wep wlan_tkip wlan_acl \
    wlan_amrr wlan_ccmp wlan_rssadapt
KERNCONF_OPTIONS+=	IEEE80211_DEBUG
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_MESH
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_TDMA
KERNCONF_DEVICES+=	wlan
KERNCONF_DEVICES+=	wlan_amrr
KERNCONF_DEVICES+=	wlan_wep
KERNCONF_DEVICES+=	wlan_ccmp
KERNCONF_DEVICES+=	wlan_tkip

KERNCONF_OPTIONS+=	AR71XX_ATH_EEPROM
KERNCONF_OPTIONS+=	ATH_EEPROM_FIRMWARE
KERNCONF_DEVICES+=	firmware

KERNCONF_OPTIONS+=	AH_DEBUG
KERNCONF_OPTIONS+=	ATH_DEBUG
KERNCONF_OPTIONS+=	ATH_DIAGAPI
KERNCONF_OPTIONS+=	ATH_ENABLE_11N
KERNCONF_OPTIONS+=	AH_SUPPORT_AR5416
#KERNCONF_OPTIONS+=	AH_SUPPORT_AR9130
KERNCONF_OPTIONS+=	AH_RXCFG_SDMAMW_4BYTES
# interrupt mitigation not possible on AR9130
# option		AH_AR5416_INTERRUPT_MITIGATION
KERNCONF_DEVICES+=	ath
KERNCONF_DEVICES+=	ath_hal
KERNCONF_DEVICES+=	ath_pci
KERNCONF_DEVICES+=	ath_rate_sample
.endif

###################################################
#
#       Limits
#
###################################################

# Image must not be biggest than GEOM_MAP_P2 (upgrade part.)
FIRMWARE_IMAGE_SIZE_MAX=0x00760000

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
