
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

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

MKULZMA_BLOCKSIZE=65536

###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}
# Include usb and SoC usb controller drivers
WITH_USB=yes
# Builded modules
#KERNCONF_MAKEOPTIONS+=	"MODULES_OVERRIDE=\"ipfw usb/umass usb/uplcom usb/u3g dummynet \""
KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt
KERNCONF_MODULES_OVERRIDE+=random if_bridge bridgestp ipfw dummynet libalias
KERNCONF_MODULES_OVERRIDE+=geom/geom_label geom/geom_uncompress
KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib
KERNCONF_MODULES_OVERRIDE+=md unionfs msdosfs ufs

# Additional utilities
WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade


###################################################
#
#       Limits
#
###################################################



# Image must not be biggest than GEOM_MAP_P2 (upgrade part.)
FIRMWARE_IMAGE_SIZE_MAX=0x007a0000

