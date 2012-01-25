
###################################################
#
# Board used hardware/chip`s
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
KERNCONF_MODULES_OVERRIDE+=ipfw dummynet zlib
KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/ucom
KERNCONF_MODULES_OVERRIDE+=usb/umass cam

KERNCONF_MODULES_OVERRIDE+=usb/run usb/rum firmware
KERNCONF_MODULES_OVERRIDE+=wlan wlan_xauth wlan_wep wlan_tkip wlan_acl \
    wlan_amrr wlan_ccmp wlan_rssadapt

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

TARGET_PROFILES+=SMALL_ mpd ssh dlink.ua.web dhcp mroute ntpdate dnsmasq \
    racoon openvpn ppp hostap ath nfs_client

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

MKULZMA_BLOCKSIZE=65536

# 256K block
PACKING_KERNEL_ROUND=0x40000
PACKING_KERNEL_IMAGE?=kernel.strip.gz.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

NEW_IMAGE_TYPE?=zimage

