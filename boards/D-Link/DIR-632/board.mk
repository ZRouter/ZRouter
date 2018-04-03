
###################################################
#
# Board used hardware/chip`s
#
###################################################

BOARD_REVISION=A1

SOC_VENDOR=Atheros
SOC_CHIP=AR7242
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

# 8M
#KERNCONF_OPTIONS+=	VM_KMEM_SIZE_MAX=8388608
# 4M
KERNCONF_OPTIONS+=	VM_KMEM_SIZE_MAX=4194304
KERNCONF_OPTIONS+=	NBUF=128

# Additional utilities
WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade

KERNCONF_OPTIONS+=	ALT_BREAK_TO_DEBUGGER
KERNCONF_OPTIONS+=	BREAK_TO_DEBUGGER

KERNCONF_OPTIONS+=	KDB
KERNCONF_OPTIONS+=	DDB
#KERNCONF_OPTIONS+=	KTR
#KERNCONF_OPTIONS+=	KTR_ENTRIES=1024
#KERNCONF_OPTIONS+=	KTR_COMPILE=(KTR_INTR)
#KERNCONF_OPTIONS+=	KTR_MASK=KTR_INTR
#KERNCONF_OPTIONS+=	ALQ
#KERNCONF_OPTIONS+=	KTR_ALQ

KERNCONF_DEVICES+=	switch_rtl830x


.if !defined(WITHOUT_WIRELESS)
KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt
#KERNCONF_OPTIONS+=	IEEE80211_DEBUG
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_MESH
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_TDMA
KERNCONF_OPTIONS+=	IEEE80211_AMPDU_AGE
KERNCONF_DEVICES+=	wlan
KERNCONF_DEVICES+=	wlan_amrr
KERNCONF_DEVICES+=	wlan_wep
KERNCONF_DEVICES+=	wlan_ccmp
KERNCONF_DEVICES+=	wlan_tkip

KERNCONF_OPTIONS+=	AR71XX_ATH_EEPROM
KERNCONF_OPTIONS+=	ATH_EEPROM_FIRMWARE
KERNCONF_DEVICES+=	firmware

#KERNCONF_OPTIONS+=	AH_DEBUG
#KERNCONF_OPTIONS+=	ATH_DEBUG
#KERNCONF_OPTIONS+=	ATH_DIAGAPI
KERNCONF_OPTIONS+=	ATH_ENABLE_11N
KERNCONF_OPTIONS+=	AH_SUPPORT_AR5416
KERNCONF_OPTIONS+=	AH_RXCFG_SDMAMW_4BYTES
KERNCONF_OPTIONS+=	AH_AR5416_INTERRUPT_MITIGATION
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
FIRMWARE_IMAGE_SIZE_MAX=0x007a0000

###################################################
#
#       Firmware Image Options
#
###################################################

TARGET_PROFILES+= 	\
	SMALL_		\
	dhclient	\
	dig_spcdns	\
	lua_web_ui	\
	dnsmasq		\
	hostap		\
	ipfw		\
	mpd		\
	ntpdate		\
	ppp		\
	racoon		\
	ssh		\
	watchdogd

#	ng_igmp_fwd
#	net_help
#	nfs_client

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

MKULZMA_BLOCKSIZE=65536

PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage
