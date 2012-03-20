

TARGET=			mips
TARGET_ARCH=		mipseb

KERNCONF_MACHINE=	${TARGET} ${TARGET_ARCH}
KERNCONF_IDENT?=	AR9132
KERNCONF_CPU=		CPU_MIPS4KC
KERNCONF_FILES+=	"../atheros/files.ar71xx"


# XXX Maybe project definitions, maybe only board

KERNCONF_MAKEOPTIONS+=	"INLINE_LIMIT=768"
KERNCONF_KERNLOADADDR?=		0x80050000
KERNCONF_OPTIONS+=		HZ=1000
KERNCONF_OPTIONS+=		MAXUSERS=3
KERNCONF_OPTIONS+=		MAXFILES=512
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
#KERNCONF_OPTIONS+=		ALT_BREAK_TO_DEBUGGER
#KERNCONF_OPTIONS+=		KTRACE

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
KERNCONF_OPTIONS+=		ROOTDEVNAME=\\\"cd9660:/dev/map/rootfs.uncompress\\\"


# Board definitions
KERNCONF_OPTIONS+=	INET
KERNCONF_OPTIONS+= 	TMPFS
KERNCONF_OPTIONS+= 	CD9660
#KERNCONF_OPTIONS+= 	GEOM_LABEL
KERNCONF_DEVICES+=	geom_map
KERNCONF_DEVICES+=	geom_uncompress
#KERNCONF_DEVICES+=	nvram2env


# Project definitions
KERNCONF_OPTIONS+=	SCHED_4BSD
KERNCONF_OPTIONS+=	PSEUDOFS
KERNCONF_OPTIONS+=	_KPOSIX_PRIORITY_SCHEDULING


KERNCONF_DEVICES+=	uart
KERNCONF_DEVICES+=	random
KERNCONF_DEVICES+=	loop
KERNCONF_DEVICES+=	ether
KERNCONF_DEVICES+= 	tun
KERNCONF_DEVICES+= 	pty
KERNCONF_DEVICES+= 	bpf
KERNCONF_DEVICES+=	vlan
#KERNCONF_DEVICES+=	md
#KERNCONF_DEVICES+=	lagg
#KERNCONF_DEVICES+=     if_bridge


KERNCONF_DEVICES+=	mii
KERNCONF_DEVICES+=	arge

KERNCONF_DEVICES+=	gpio
KERNCONF_DEVICES+=	gpioled

KERNCONF_DEVICES+=	mx25l
KERNCONF_DEVICES+=	spibus
KERNCONF_DEVICES+=	ar71xx_spi
KERNCONF_DEVICES+=	ar71xx_wdog


.if defined(WITH_IPSEC)
KERNCONF_OPTIONS+=      IPSEC
KERNCONF_OPTIONS+=      IPSEC_NAT_T

KERNCONF_DEVICES+=	enc
#KERNCONF_DEVICES+=	gif
KERNCONF_DEVICES+=	ipsec
KERNCONF_DEVICES+=	crypto
#KERNCONF_DEVICES+=	cryptodev

#KERNCONF_DEVICES+=	rndtest
.endif


.if defined(WITH_USB)
KERNCONF_OPTIONS+=	USB_EHCI_BIG_ENDIAN_DESC
KERNCONF_OPTIONS+=	USB_HOST_ALIGN=32
#KERNCONF_OPTIONS+=	USB_DEBUG
#KERNCONF_OPTIONS+=	USB_VERBOSE

KERNCONF_DEVICES+=	usb
KERNCONF_DEVICES+=	ehci


#KERNCONF_DEVICES+=	udbp
#KERNCONF_DEVICES+=	uhid
#KERNCONF_DEVICES+=	ukbd
#KERNCONF_DEVICES+=	ulpt
KERNCONF_DEVICES+=	umass
#KERNCONF_DEVICES+=	ums
#KERNCONF_DEVICES+=	ural
#KERNCONF_DEVICES+=	rum
#KERNCONF_DEVICES+=	zyd
#KERNCONF_DEVICES+=	urio
# USB Serial devices
#KERNCONF_DEVICES+=	u3g
#KERNCONF_DEVICES+=	umodem
#KERNCONF_DEVICES+=	uark
#KERNCONF_DEVICES+=	ubsa
#KERNCONF_DEVICES+=	uftdi
#KERNCONF_DEVICES+=	uipaq
#KERNCONF_DEVICES+=	uplcom
#KERNCONF_DEVICES+=	uslcom
#KERNCONF_DEVICES+=	uvisor
#KERNCONF_DEVICES+=	uvscom
# USB Ethernet, requires miibus
#KERNCONF_DEVICES+=	aue
#KERNCONF_DEVICES+=	axe
#KERNCONF_DEVICES+=	cdce
#KERNCONF_DEVICES+=	cue
#KERNCONF_DEVICES+=	kue
#KERNCONF_DEVICES+=	rue
#KERNCONF_DEVICES+=	udav


# SCSI peripherals
KERNCONF_DEVICES+=     scbus
KERNCONF_DEVICES+=     da
#KERNCONF_DEVICES+=     sa
#KERNCONF_DEVICES+=     cd
#KERNCONF_DEVICES+=     pass
##KERNCONF_DEVICES+=	cam
.endif

