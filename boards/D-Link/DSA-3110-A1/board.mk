
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
NFS_KERNEL_MODULES=nfscl nfsd nfslock nfssvc krpc nfscommon
USB_KERNEL_MODULES=usb/uplcom usb/u3g usb/umodem usb/ucom
ATA_KERNEL_MODULES=ata/atadisk ata/atapci/chipsets/ataatp8620 
#I2C_KERNEL_MODULES=i2c/iictest i2c/ds133x
KERNCONF_MODULES_OVERRIDE+=${USB_KERNEL_MODULES} ${ATA_KERNEL_MODULES} ${I2C_KERNEL_MODULES} ${NFS_KERNEL_MODULES} zlib libiconv ext2fs

# Additional utilities
#WORLD_SUBDIRS_ZROUTER+=target/sbin/upgrade

KERNCONF_OPTIONS+=	ALT_BREAK_TO_DEBUGGER
KERNCONF_OPTIONS+=	BREAK_TO_DEBUGGER

#INSTALL_TOOLCHAIN=yes

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

TARGET_PROFILES+=SMALL_ mpd ssh dlink.ua.web dhcp mroute ntpdate dnsmasq racoon ppp hostap
# nfs_client openvpn
128M!=sh -c 'echo $$((128 * 1024 * 1024))'
ROOTFS_WITH_KERNEL=yes
ROOTFS_MEDIA_SIZE?=${128M}
PACKING_KERNEL_IMAGE?=kernel
PACKING_ROOTFS_IMAGE?=rootfs


IMAGE_SUFFIX=zimage

