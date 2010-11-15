

.warning "Broadcom/BCM5354 SoC"
#XXX testing
LZMA=lzma
KERNEL_PATH=/usr/obj/kernel


#SOC_KERNCONF=BCM5354
#SOC_KERNHINTS=BCM5354.hints

KERNCONF_MACHINE=	mips
KERNCONF_IDENT?=	BCM5354
KERNCONF_CPU=		CPU_MIPS4KC
KERNCONF_OPTIONS+=	ISA_MIPS32
KERNCONF_FILES+=	"../bcm47xx/files.bcm47xx"
KERNCONF_HINTS=		"BCM5354.hints"


KERNCONF_MAKEOPTIONS+=	"MIPS_LITTLE_ENDIAN=defined"


# XXX Maybe project definitions, maybe only board

KERNCONF_MAKEOPTIONS+=	"INLINE_LIMIT=768"
KERNCONF_MAKEOPTIONS+=	"KERNLOADADDR=0x80001000"
KERNCONF_OPTIONS+=		MAXUSERS=3
KERNCONF_OPTIONS+=		MAXFILES=512
#KERNCONF_OPTIONS+=		NMBCLUSTERS=1024
KERNCONF_OPTIONS+=		NSFBUFS=256
KERNCONF_OPTIONS+=		SHMALL=128
KERNCONF_OPTIONS+=		MSGBUF_SIZE=65536

# Options for making kernel smallest 
KERNCONF_OPTIONS+=		NO_SYSCTL_DESCR		# No description string of sysctl
KERNCONF_OPTIONS+=		NO_FFS_SNAPSHOT		# Disable Snapshot supporting
KERNCONF_OPTIONS+=		SCSI_NO_SENSE_STRINGS
KERNCONF_OPTIONS+=		SCSI_NO_OP_STRINGS
#KERNCONF_OPTIONS+=		MUTEX_NOINLINE
KERNCONF_OPTIONS+=		RWLOCK_NOINLINE
KERNCONF_OPTIONS+=		SX_NOINLINE
KERNCONF_OPTIONS+=		NO_SWAPPING




# Debug definitions
#KERNCONF_OPTIONS+=		DDB
#KERNCONF_OPTIONS+=		KDB

# Custom build definitions
#KERNCONF_OPTIONS+=	NFSCLIENT
#KERNCONF_OPTIONS+=	NFS_ROOT
#KERNCONF_OPTIONS+= 	FFS
#KERNCONF_OPTIONS+= 	SOFTUPDATES
#KERNCONF_OPTIONS+= 	UFS_ACL
#KERNCONF_OPTIONS+= 	UFS_DIRHASH
#KERNCONF_OPTIONS+= 	UFS_GJOURNAL
#KERNCONF_OPTIONS+= 	MSDOSFS
#KERNCONF_OPTIONS+= 	PROCFS
#KERNCONF_OPTIONS+= 	BOOTP
#KERNCONF_OPTIONS+= 	BOOTP_NFSROOT
#KERNCONF_OPTIONS+= 	BOOTP_NFSV3
#KERNCONF_OPTIONS+= 	BOOTP_WIRED_TO=bfe0
#KERNCONF_OPTIONS+= 	BOOTP_COMPAT
#KERNCONF_OPTIONS+=		ROOTDEVNAME=\"nfs:192.168.0.90:/usr/home/ray/Projects/MIPS/FreeBSD/rootfs-mips\"
KERNCONF_OPTIONS+=		ROOTDEVNAME=\"cd9660:/dev/map/rootfs.ulzma\"


# Board definitions
KERNCONF_OPTIONS+=	INET
KERNCONF_OPTIONS+= 	TMPFS
KERNCONF_OPTIONS+= 	CD9660
KERNCONF_OPTIONS+= 	GEOM_LABEL		# Provides labelization
KERNCONF_DEVICES+=		geom_map
#KERNCONF_DEVICES+=		geom_uzip
KERNCONF_DEVICES+=		geom_ulzma



# Project definitions
KERNCONF_OPTIONS+= 	ZERO_COPY_SOCKETS
KERNCONF_OPTIONS+=		SCHED_4BSD		#4BSD scheduler
KERNCONF_OPTIONS+=		PSEUDOFS		#Pseudo-filesystem framework
KERNCONF_OPTIONS+=		_KPOSIX_PRIORITY_SCHEDULING #Posix P1003_1B real-time extensions








# XXX notyet; need to be auto probed children of siba_cc.
KERNCONF_DEVICES+=		uart

KERNCONF_DEVICES+=		siba			# Sonics SiliconBackplane
#KERNCONF_DEVICES+=		pci			# siba_pcib

KERNCONF_DEVICES+=		siba_cc			# Sonics SiliconBackplane ChipCommon core
KERNCONF_DEVICES+=		siba_mips		# Sonics SiliconBackplane MIPS core
KERNCONF_DEVICES+=		siba_sdram		# Sonics SiliconBackplane SDRAM core
KERNCONF_DEVICES+=		bfe			# FastEthernet 44xx core
KERNCONF_OPTIONS+= 	BFE_PACKET_LIST_CNT=32
#KERNCONF_DEVICES+=		miibus			# attachments
KERNCONF_DEVICES+=		mii			# Only bfeswitch
KERNCONF_DEVICES+=		bfeswitch		# ROBO switch

KERNCONF_DEVICES+=		gpio
KERNCONF_DEVICES+=		gpioled


KERNCONF_DEVICES+=		loop			# Network loopback
KERNCONF_DEVICES+=		ether			# Ethernet support
#KERNCONF_DEVICES+= 		tun			# Packet tunnel.
KERNCONF_DEVICES+= 		pty			# Pseudo-ttys (telnet etc)
#KERNCONF_DEVICES+=		md			# Memory "disks"
KERNCONF_DEVICES+= 		bpf			# Berkeley packet filter
KERNCONF_DEVICES+=		vlan
#KERNCONF_DEVICES+=		lagg
#KERNCONF_DEVICES+=          if_bridge


KERNCONF_DEVICES+=		cfi			# Detect Flash memmory
KERNCONF_DEVICES+=		cfid


#KERNCONF_DEVICES+=		wlan
#KERNCONF_DEVICES+=		wlan_wep
#KERNCONF_DEVICES+=		wlan_ccmp
#KERNCONF_DEVICES+=		wlan_tkip
#KERNCONF_DEVICES+=		wlan_xauth
#KERNCONF_DEVICES+=		wlan_acl
#KERNCONF_DEVICES+=		wlan_amrr

#KERNCONF_DEVICES+=		firmware
#KERNCONF_DEVICES+=		bwn
#KERNCONF_DEVICES+=		bwi

KERNCONF_OPTIONS+=         IPSEC
KERNCONF_OPTIONS+=         IPSEC_NAT_T

KERNCONF_DEVICES+=		random
KERNCONF_DEVICES+=		enc
#KERNCONF_DEVICES+=		gif
KERNCONF_DEVICES+=		ipsec
KERNCONF_DEVICES+=		crypto
#KERNCONF_DEVICES+=		cryptodev	# /dev/crypto for access to h/w

##KERNCONF_DEVICES+=		rndtest		# FIPS 140-2 entropy tester
#KERNCONF_DEVICES+=		ubsec_siba


#KERNCONF_OPTIONS+=		USB_EHCI_BIG_ENDIAN_DESC # XXX
#KERNCONF_OPTIONS+=		USB_DEBUG
#KERNCONF_OPTIONS+=		USB_VERBOSE

KERNCONF_DEVICES+=		usb			# USB Bus (required)
KERNCONF_DEVICES+=		ehci			# EHCI interface (USB 2.0)
KERNCONF_DEVICES+=		ohci			# OHCI interface (USB 1.1)



#KERNCONF_DEVICES+=		udbp		# USB Double Bulk Pipe devices
#KERNCONF_DEVICES+=		uhid		# "Human Interface Devices"
#KERNCONF_DEVICES+=		ukbd		# Keyboard
#KERNCONF_DEVICES+=		ulpt		# Printer
#KERNCONF_DEVICES+=		umass		# Disks/Mass storage - Requires scbus and da
#KERNCONF_DEVICES+=		ums		# Mouse
#KERNCONF_DEVICES+=		ural		# Ralink Technology RT2500USB wireless NICs
#KERNCONF_DEVICES+=		rum		# Ralink Technology RT2501USB wireless NICs
#KERNCONF_DEVICES+=		zyd		# ZyDAS zb1211/zb1211b wireless NICs
#KERNCONF_DEVICES+=		urio		# Diamond Rio 500 MP3 player
# USB Serial devices
#KERNCONF_DEVICES+=		u3g		# USB-based 3G modems (Option, Huawei, Sierra)
#KERNCONF_DEVICES+=		umodem		# USB-based 3G modems (Option, Huawei, Sierra)
#KERNCONF_DEVICES+=		uark		# Technologies ARK3116 based serial adapters
#KERNCONF_DEVICES+=		ubsa		# Belkin F5U103 and compatible serial adapters
#KERNCONF_DEVICES+=		uftdi		# For FTDI usb serial adapters
#KERNCONF_DEVICES+=		uipaq		# Some WinCE based devices
#KERNCONF_DEVICES+=		uplcom		# Prolific PL-2303 serial adapters
#KERNCONF_DEVICES+=		uslcom		# SI Labs CP2101/CP2102 serial adapters
#KERNCONF_DEVICES+=		uvisor		# Visor and Palm devices
#KERNCONF_DEVICES+=		uvscom		# USB serial support for DDI pocket's PHS
# USB Ethernet, requires miibus
#KERNCONF_DEVICES+=		aue		# ADMtek USB Ethernet
#KERNCONF_DEVICES+=		axe		# ASIX Electronics USB Ethernet
#KERNCONF_DEVICES+=		cdce		# Generic USB over Ethernet
#KERNCONF_DEVICES+=		cue		# CATC USB Ethernet
#KERNCONF_DEVICES+=		kue		# Kawasaki LSI USB Ethernet
#KERNCONF_DEVICES+=		rue		# RealTek RTL8150 USB Ethernet
#KERNCONF_DEVICES+=		udav		# Davicom DM9601E USB



# SCSI peripherals
#KERNCONF_DEVICES+=          scbus           # SCSI bus (required for SCSI)
#KERNCONF_DEVICES+=          da              # Direct Access (disks)
#KERNCONF_DEVICES+=          sa              # Sequential Access (tape etc)
#KERNCONF_DEVICES+=          cd              # CD
#KERNCONF_DEVICES+=          pass            # Passthrough device (direct SCSI access)
##KERNCONF_DEVICES+=		cam




kernel_deflate:	${DEPEND_ON_LZMA}
	${DEBUG}${LZMA} e ${KERNEL_PATH} ${KERNEL_PATH}.lzma
	${DEBUG}cp ${KERNEL_PATH} ${KERNEL_PATH}.unpacked
	${DEBUG}cp ${KERNEL_PATH}.lzma ${KERNEL_PATH}
