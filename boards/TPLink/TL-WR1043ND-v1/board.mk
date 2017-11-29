
###################################################
#
# Board used hardware/chips
#
###################################################

SOC_VENDOR=Atheros
SOC_CHIP=AR9132
BOARD_FLASH_SIZE=8M


###################################################
#
# Vars for kernel config
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}

# Enable the uboot environment stuff rather then the redboot stuff.
KERNCONF_OPTIONS+=	AR71XX_ENV_UBOOT

# Force the board memory - 32MB
KERNCONF_OPTIONS+=	AR71XX_REALMEM=32*1024*1024

KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/map/rootfs.uzip\\\"

# Include usb and SoC usb controller drivers
WITH_USB=yes

# Builded modules
KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib

KERNCONF_DEVICES+=	gpioiic
KERNCONF_DEVICES+=	iicbb
KERNCONF_DEVICES+=	iicbus
KERNCONF_DEVICES+=	iic

KERNCONF_OPTIONS+=	ARGE_MDIO
KERNCONF_DEVICES+=	etherswitch
KERNCONF_DEVICES+=	miiproxy
KERNCONF_DEVICES+=	rtl8366rb
WORLD_SUBDIRS_SBIN+=	etherswitchcfg

.if !defined(WITHOUT_WIRELESS)
KERNCONF_OPTIONS+=	IEEE80211_DEBUG
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_MESH
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_TDMA
KERNCONF_DEVICES+=	wlan
# ? not for ath?
#KERNCONF_DEVICES+=	wlan_amrr
KERNCONF_DEVICES+=	wlan_wep
KERNCONF_DEVICES+=	wlan_ccmp
KERNCONF_DEVICES+=	wlan_tkip
KERNCONF_DEVICES+=	wlan_xauth

KERNCONF_OPTIONS+=	AH_DEBUG
KERNCONF_OPTIONS+=	ATH_DEBUG
KERNCONF_OPTIONS+=	ATH_DIAGAPI
KERNCONF_OPTIONS+=	AH_SUPPORT_AR5416
# Makes other chipsets not function! 
KERNCONF_OPTIONS+=	AH_SUPPORT_AR9130
KERNCONF_OPTIONS+=	AH_RXCFG_SDMAMW_4BYTES
KERNCONF_OPTIONS+=	ATH_ENABLE_11N

KERNCONF_DEVICES+=	ath
KERNCONF_DEVICES+=	ath_rate_sample
KERNCONF_DEVICES+=	ath_ahb
# not needed anymore?
#KERNCONF_DEVICES+=	ath_hal
KERNCONF_DEVICES+=	ath_ar5212
KERNCONF_DEVICES+=	ath_ar5416
KERNCONF_DEVICES+=	ath_ar9130
.endif


###################################################
#
#       Limits
#
###################################################

# Image must not be bigger than 'upgrade' geom_map partition
FIRMWARE_IMAGE_SIZE_MAX=0x007c0000

# the hint.map.?.start address from board.hints where hint.map.?.name="kernel"
KERNEL_MAP_START=0x00020000


###################################################
#
#       Firmware Image Options
#
###################################################

#TARGET_PROFILES+=SMALL_ mpd ssh shttpd lua_web_ui hostap dhcp nfs_client
TARGET_PROFILES+=xSMALL_

MKULZMA_BLOCKSIZE=65536

PACKING_KERNEL_IMAGE=kernel.strip.kbin.gz.sync
PACKING_ROOTFS_IMAGE=rootfs_clean.iso.ulzma

TPLINK_BOARDTYPE="TL-WR1043NDv1"

IMAGE_SUFFIX=bin
NEW_IMAGE_TYPE=tplink_image

