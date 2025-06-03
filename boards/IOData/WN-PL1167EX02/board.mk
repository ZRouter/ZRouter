###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Mediatek
SOC_CHIP=MT7621
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304

# use for mac address in flash
WORLD_SUBDIRS_USR_BIN+=awk strings

###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}

ZKERNCONF_FDT_DTS_FILE?= "dts/mips/WN-PL1167EX02.dts"

KERNCONF_OPTIONS+=     ROOTDEVNAME=\\\"cd9660:md0.uzip\\\" 

WITHOUT_WIRELESS=yes

# Include usb and SoC usb controller drivers
#WITH_USB=yes
# Builded modules
# device wlan in kernel alredy enable this modules
#KERNCONF_MODULES_OVERRIDE+=wlan_xauth wlan_wep wlan_tkip wlan_acl wlan_amrr wlan_ccmp wlan_rssadapt
#KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib
#KERNCONF_MODULES_OVERRIDE+=usb/umass cam zlib
#KERNCONF_MODULES_OVERRIDE+=cam zlib
#KERNCONF_MODULES_OVERRIDE+=usb/run runfw

###################################################
#
#       Limits
#
###################################################



# Image must not be biggest than GEOM_MAP_P2 (upgrade part.)
# Original is MX25L25635(32M). But I reprace MX25L6405D.
# 8M(MX25L6405D) - Z-LOADER(3M)
FIRMWARE_IMAGE_SIZE_MAX=0x01000000


###################################################
#
#       Firmware Image Options
#
###################################################

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

MKULZMA_BLOCKSIZE=65536

PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=split_kernel_rootfs

