
###################################################
#
# Board used hardware/chip`s
#
###################################################

SOC_VENDOR=Infineon
SOC_CHIP=ADM5120P

# TODO: size suffixes
BOARD_FLASH_SIZE=2097152

###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}
# Include usb and SoC usb controller drivers
#WITH_USB=yes
# Builded modules
# device wlan in kernel alredy enable this modules
#KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib

KERNCONF_OPTIONS+=ALT_BREAK_TO_DEBUGGER

###################################################
#
#       Limits
#
###################################################


MKULZMA_BLOCKSIZE=65536

# Image must not be biggest than GEOM_MAP_P4 (upgrade part.)
FIRMWARE_IMAGE_SIZE_MAX=0x007c0000

#KERNEL_COMPRESSION=oldlzma
#KERNEL_COMPRESSION_TYPE=oldlzma
KERNEL_COMPRESSION=bzip2
KERNEL_COMPRESSION_TYPE=bzip2
#UBOOT_KERNEL_COMPRESSION_TYPE=lzma
UBOOT_KERNEL_COMPRESSION_TYPE=bzip2

#PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot.sync
PACKING_KERNEL_IMAGE?=kernel.kbin.bz2.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage
