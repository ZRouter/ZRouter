
###################################################
#
# Board used hardware/chip`s
#
###################################################

SOC_VENDOR=Cavium
SOC_CHIP=CN5010
# TODO: size suffixes
BOARD_FLASH_SIZE=33554432

#?????KERNEL_COMPRESSION=lzma.4.17

###################################################
#
# Vars for kernel config 
#
###################################################
KERNCONF_OPTIONS+=	OCTEON_VENDOR_D_LINK
KERNCONF_OPTIONS+=	OCTEON_BOARD_DSR_1000N

KERNCONF_DEVICES+=	switch
KERNCONF_DEVICES+=	switch_bcm5325

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}
# Include usb and SoC usb controller drivers
WITH_USB=yes
WITH_IPSEC=yes
# Builded modules
KERNCONF_MODULES_OVERRIDE+=ipfw usb/umass usb/uplcom usb/u3g dummynet

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



# Additional utilities ????
WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade

###################################################
#
#       Limits
#
###################################################


# Image must not be biggest than GEOM_MAP_P2 (upgrade part.)
#????## FIRMWARE_IMAGE_SIZE_MAX=0x003a0000

###################################################
#
#       Firmware Image Options
#
###################################################

TARGET_PROFILES+=SMALL_ mpd ssh dlink.ua.web dhcp mroute ntpdate dnsmasq racoon openvpn ppp hostap ath nfs_client

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

MKULZMA_BLOCKSIZE=65536

PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

NEW_IMAGE_TYPE=split_kernel_rootfs
