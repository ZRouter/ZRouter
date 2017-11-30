
###################################################
#
# Board used hardware/chip`s
#
###################################################


SOC_VENDOR=Intel
SOC_CHIP=i386


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
#KERNCONF_MODULES_OVERRIDE+=usb/uplcom usb/u3g usb/umodem usb/umass usb/ucom cam zlib


###################################################
#
#       Limits
#
###################################################





###################################################
#
#       Firmware Image Options
#
###################################################

TARGET_PROFILES+=SMALL_ mpd ssh lua_web_ui dhcp ng_igmp_fwd ntpdate dnsmasq racoon openvpn ppp nfs_client
# hostap
# racoon

32M!=sh -c 'echo $$((32 * 1024 * 1024))'
ROOTFS_WITH_KERNEL=yes
ROOTFS_MEDIA_SIZE?=${32M}
PACKING_KERNEL_IMAGE?=kernel
PACKING_ROOTFS_IMAGE?=rootfs

IMAGE_SUFFIX=zimage
NEW_IMAGE_TYPE=split_kernel_rootfs
