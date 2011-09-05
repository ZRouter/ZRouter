
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Intel
SOC_CHIP=ixp435
# Maybe used for kernel config and maybe multiple e.g. "cfi nand"
BOARD_FLASH_TYPE=cfi
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304



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
KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/ucom zlib ata/atadisk ata/atapci/chipsets/ataatp8620

# Additional utilities
WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade

KERNCONF_OPTIONS+=	ALT_BREAK_TO_DEBUGGER
KERNCONF_OPTIONS+=	BREAK_TO_DEBUGGER

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

TARGET_PROFILES+=SMALL_ mpd ssh dlink.ua.web dhcp mroute ntpdate dnsmasq racoon openvpn ppp hostap
# nfs_client
128M!=sh -c 'echo $$((128 * 1024 * 1024))'
ROOTFS_WITH_KERNEL=yes
ROOTFS_MEDIA_SIZE?=${128M}
PACKING_KERNEL_IMAGE?=kernel
PACKING_ROOTFS_IMAGE?=rootfs


IMAGE_SUFFIX=zimage

