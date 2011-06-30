
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Ralink
SOC_CHIP=RT3052F
# Maybe used for kernel config and maybe multiple e.g. "cfi nand"
BOARD_FLASH_TYPE=cfi
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

TARGET_PROFILES+=SMALL_ mpd ssh dlink.ua.web dhcp mroute ntpdate dnsmasq racoon openvpn
# hostap
# nfs_client
# racoon

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
