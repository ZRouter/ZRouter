
###################################################
#
# Board used hardware/chip's
#
###################################################

SOC_VENDOR=Cavium
SOC_CHIP=CN5010
# TODO: size suffixes
BOARD_FLASH_SIZE=33554432

###################################################
#
# Vars for kernel config
#
###################################################
KERNCONF_OPTIONS+=	OCTEON_VENDOR_D_LINK
KERNCONF_OPTIONS+=	OCTEON_BOARD_DSR_1000N

KERNCONF_DEVICES+=	brgphy
KERNCONF_DEVICES+=	switch
KERNCONF_DEVICES+=	switch_bcm5325

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}

# Include usb and SoC usb controller drivers
WITH_USB=yes
WITH_IPSEC=yes
# Builded modules
KERNCONF_MODULES_OVERRIDE+=zlib
KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/ucom
KERNCONF_MODULES_OVERRIDE+=usb/umass cam
# TODO: enable it if someone require
KERNCONF_MODULES_OVERRIDE+=firmware


.if !defined(WITHOUT_WIRELESS)
KERNCONF_MODULES_OVERRIDE+=usb/run usb/rum
KERNCONF_MODULES_OVERRIDE+=wlan wlan_xauth wlan_wep wlan_tkip wlan_acl \
    wlan_amrr wlan_ccmp wlan_rssadapt
KERNCONF_OPTIONS+=	IEEE80211_DEBUG
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_MESH
KERNCONF_OPTIONS+=	IEEE80211_SUPPORT_TDMA
KERNCONF_DEVICES+=	wlan
KERNCONF_DEVICES+=	wlan_amrr
KERNCONF_DEVICES+=	wlan_wep
KERNCONF_DEVICES+=	wlan_ccmp

KERNCONF_OPTIONS+=	AH_DEBUG
KERNCONF_OPTIONS+=	ATH_DEBUG
KERNCONF_OPTIONS+=	ATH_DIAGAPI
KERNCONF_OPTIONS+=	ATH_ENABLE_11N
KERNCONF_OPTIONS+=	AH_SUPPORT_AR5416
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


###################################################
#
#       Firmware Image Options
#
###################################################

TARGET_PROFILES+=SMALL_ ipfw mpd ssh lua_web_ui dhcp ng_igmp_fwd ntpdate \
	dnsmasq racoon openvpn ppp hostap ath nfs_client net_help

MKULZMA_BLOCKSIZE=65536

# 256K block
PACKING_KERNEL_ROUND=0x40000
PACKING_KERNEL_IMAGE?=kernel.strip.gz.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

NEW_IMAGE_TYPE?=zimage

