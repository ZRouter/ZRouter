
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Allwinner
SOC_CHIP=A10
# TODO: size suffixes
BOARD_FLASH_SIZE=4194304

#KERNCONF_FDT_DTS_FILE?=	"sun4i-a10-cubieboard.dts"
ZKERNCONF_FDT_DTS_FILE?=	"dts/arm/pcduino.dts"
KERNCONF_DEVICES+=umass scbus da
# board has axp209 but hang up on boot
#KERNCONF_DEVICES+=axp209

#BUILD_ZROUTER_WITH_GCC=yes
CONVERTER_KBIN=/usr/local/arm-elf/bin/objcopy

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

#KERNCONF_DEVICES+=	geom_map
#KERNCONF_DEVICES+=	xz
#KERNCONF_DEVICES+=	geom_uzip
KERNCONF_DEVICES+=	geom_mbr
KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/mmcsd0s2\\\"

# Additional utilities
#WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade

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
FIRMWARE_IMAGE_SIZE_MAX=0x003c0000


###################################################
#
#       Firmware Image Options
#
###################################################

UBOOT_KERNEL_COMPRESSION_TYPE=none

MKULZMA_BLOCKSIZE=65536

PACKING_KERNEL_IMAGE?=kernel.kbin.uboot
PACKING_ROOTFS_IMAGE?=rootfs_clean.iso

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=zimage
