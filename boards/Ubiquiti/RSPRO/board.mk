
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Atheros
SOC_CHIP=AR7161
# TODO: size suffixes
BOARD_FLASH_SIZE=16777216


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
# device wlan in kernel alredy enable this modules
#KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt
#KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib

# Define empty LDSCRIPT_NAME, FreeBSD kernel make process will use his default
KERNCONF_KERN_LDSCRIPT_NAME=

# Additional utilities
WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade

.if !defined(WITHOUT_WIRELESS)
KERNCONF_OPTIONS+=	IEEE80211_DEBUG
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_MESH
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_TDMA
KERNCONF_DEVICES+=	wlan
KERNCONF_DEVICES+=	wlan_amrr
KERNCONF_DEVICES+=	wlan_wep
KERNCONF_DEVICES+=	wlan_ccmp
KERNCONF_DEVICES+=	wlan_tkip

KERNCONF_OPTIONS+=	AH_DEBUG
KERNCONF_OPTIONS+=	ATH_DEBUG
KERNCONF_OPTIONS+=	AH_SUPPORT_AR5416
KERNCONF_OPTIONS+=	AH_RXCFG_SDMAMW_4BYTES
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
FIRMWARE_IMAGE_SIZE_MAX=0x00B00000

###################################################
#
#       Firmware Image Options
#
###################################################

TARGET_PROFILES+=SMALL_ mpd ssh shttpd dlink.ua.web hostap dhcp

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma

MKULZMA_BLOCKSIZE=65536

#UBNT_KERNEL_LOAD_ADDRESS?=0x80050000
#UBNT_KERNEL_ENTRY_POINT?=0x80050100
#KERNCONF_KERNLOADADDR?=0x80050000
KERNCONF_KERNLOADADDR?=0x80050000
PACKING_KERNEL_IMAGE?=kernel.kbin.gz.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage

UBNT_VERSION?=RSPRO.ar7100pro.V1

NEW_IMAGE_TYPE=ubntimage
