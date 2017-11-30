
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Ralink
SOC_CHIP=RT1310A
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304

KERNCONF_FDT_DTS_FILE?=	"wzr2-g300n.dts"

BUILD_ZROUTER_WITH_GCC=yes

###################################################
#
# Vars for kernel config 
#
###################################################

# ident 
KERNCONF_IDENT=${TARGET_VENDOR}_${TARGET_DEVICE}
# Include usb and SoC usb controller drivers
#WITH_USB=yes
#WITH_IPSEC=yes
#WITH_WIRELESS=yes
# Builded modules
#NFS_KERNEL_MODULES=nfscl nfsd nfslock nfssvc krpc nfscommon
#USB_KERNEL_MODULES=usb/uplcom usb/u3g usb/umodem usb/ucom
#ATA_KERNEL_MODULES=ata/atadisk ata/atapci/chipsets/ataatp8620 
#I2C_KERNEL_MODULES=i2c/iictest i2c/ds133x
#kERNCONF_MODULES_OVERRIDE+=${USB_KERNEL_MODULES} ${ATA_KERNEL_MODULES} ${I2C_KERNEL_MODULES} ${NFS_KERNEL_MODULES} zlib libiconv ext2fs

KERNCONF_DEVICES+=	geom_map
KERNCONF_DEVICES+=	geom_uzip
#KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/map/rootfs.uzip\\\"
KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/cfid0s.rootfs.uzip\\\"

KERNCONF_OPTIONS+=	ALT_BREAK_TO_DEBUGGER
KERNCONF_OPTIONS+=	BREAK_TO_DEBUGGER

#INSTALL_TOOLCHAIN=yes

KERNCONF_DEVICES+=	cfi
KERNCONF_DEVICES+=	cfid

# define FV_MDIO in if_fv.c
#KERNCONF_DEVICES+=	mdio
#KERNCONF_DEVICES+=	etherswitch
#KERNCONF_DEVICES+=	miiproxy
#KERNCONF_DEVICES+=	ip17x
#WORLD_SUBDIRS_SBIN+=	etherswitchcfg

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

#128M!=sh -c 'echo $$((128 * 1024 * 1024))'
#ROOTFS_WITH_KERNEL=yes
#ROOTFS_MEDIA_SIZE?=${128M}
#PACKING_KERNEL_IMAGE?=kernel
#PACKING_ROOTFS_IMAGE?=rootfs

#IMAGE_SUFFIX=zimage
#NEW_IMAGE_TYPE=split_kernel_rootfs

KERNEL_COMPRESSION=oldlzma
KERNEL_COMPRESSION_TYPE=oldlzma
UBOOT_KERNEL_COMPRESSION_TYPE=lzma

MKULZMA_BLOCKSIZE=131072

PACKING_KERNEL_IMAGE?=kernel.kbin.oldlzma.uboot.sync
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso.ulzma

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage
