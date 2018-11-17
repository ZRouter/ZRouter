
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Mediatek
SOC_CHIP=MT7621_FDT
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
KERNCONF_OPTIONS+=     ROOTDEVNAME=\\\"cd9660:/dev/map/rootfs.uzip\\\"
# KERNCONF_OPTIONS+=     GEOM_UZIP_DEBUG	# very verbose logging

WITH_USB=yes
WITHOUT_IPSEC=yes
WITHOUT_WIRELESS=yes
# Builded modules
KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib

# Additional utilities
WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade

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

TARGET_PROFILES+=SMALL_ ssh lua_web_ui dhcp ntpdate dnsmasq 
# openvpn 
# hostap
# nfs_client
# racoon

# KERNEL_COMPRESSION=oldlzma
# KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=none

MKULZMA_BLOCKSIZE=65536

PACKING_KERNEL_IMAGE?=kernel.kbin.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage

