
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Mediatek
SOC_CHIP=MT7621
# TODO: size suffixes
BOARD_FLASH_SIZE=8388608


###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}

KERNCONF_FDT_DTS_FILE=  "WITI.dts"

# Include usb and SoC usb controller drivers
KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/map/rootfs.uzip\\\"

# Sata is delivered by asmedia ASM1061
# Connected to the PCI bus slot 3
KERNCONF_DEVICE+=	ahci
KERNCONF_DEVICE+=	scbus
KERNCONF_OPTIONS+=      FFS
WORLD_SUBDIRS_SBIN+=    fsck fsck_ffs
WORLD_SUBDIRS_SBIN+=    mount geom	# geom contains gpart

# Wifi is generated by 2* MT7206EN (Mediatek) 
# boubtfull if there is a driver
##KERNCONF_OPTIONS+=    RT2860_DEBUG
#KERNCONF_DEVICES+=     rt2860
KERNCONF_DEVICES+=      ral
KERNCONF_DEVICES+=      ralfw
KERNCONF_DEVICES+=      firmware
KERNCONF_DEVICES+=      wlan

WITH_USB=yes
WITHOUT_IPSEC=yes
# Builded modules
KERNCONF_MODULES_OVERRIDE+=usb/umass cam zlib
# KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/ucom

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

