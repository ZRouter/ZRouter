
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Atheros
SOC_CHIP=AR7240
# Maybe used for kernel config and maybe multiple e.g. "cfi nand"
BOARD_FLASH_TYPE=spi
# TODO: size suffixes
BOARD_FLASH_SIZE=8388608


###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}
# Include usb and SoC usb controller drivers
WITH_USB=yes
WITH_IPSEC=yes
#WITH_WIRELESS=yes
# Builded modules
KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib

# Additional utilities
WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade

KERNCONF_OPTIONS+=	ALT_BREAK_TO_DEBUGGER
KERNCONF_OPTIONS+=	BREAK_TO_DEBUGGER

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


#.if !defined(WITHOUT_WIRELESS)
#KERNCONF_OPTIONS+=	IEEE80211_DEBUG
##KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_MESH
##KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_TDMA
#KERNCONF_MODULES_OVERRIDE+=	wlan
#KERNCONF_MODULES_OVERRIDE+=	wlan_amrr
#KERNCONF_MODULES_OVERRIDE+=	wlan_wep
#KERNCONF_MODULES_OVERRIDE+=	wlan_ccmp
#KERNCONF_MODULES_OVERRIDE+=	wlan_tkip
#
#KERNCONF_OPTIONS+=	AH_DEBUG
#KERNCONF_OPTIONS+=	ATH_DEBUG
#KERNCONF_OPTIONS+=	AH_SUPPORT_AR5416
#KERNCONF_OPTIONS+=	AH_RXCFG_SDMAMW_4BYTES
#KERNCONF_MODULES_OVERRIDE+=	ath
##KERNCONF_MODULES_OVERRIDE+=	ath_hal
#KERNCONF_MODULES_OVERRIDE+=	ath_pci
##KERNCONF_MODULES_OVERRIDE+=	ath_rate_sample
#.endif



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

TARGET_PROFILES+=SMALL_ mpd ssh dlink.ua.web dhcp mroute ntpdate dnsmasq racoon openvpn ppp hostap ath
# nfs_client

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

MKULZMA_BLOCKSIZE=65536

PACKING_KERNEL_IMAGE?=kernel.oldlzma.uboot
PACKING_ROOTFS_IMAGE?=rootfs.iso.ulzma

# 64k
PACKING_KERNEL_ROUND?=0x10000

PACKING_ROOTFS_METHOD?=	tar.gz

IMAGE_SUFFIX=zimage

