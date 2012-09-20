
#XXX testing
#LZMA=lzma
#KERNEL_PATH=/usr/obj/kernel

TARGET=			mips
TARGET_ARCH=		mips

KERNCONF_MACHINE=	${TARGET} ${TARGET_ARCH}
KERNCONF_IDENT?=	AR7242
KERNCONF_CPU=		CPU_MIPS4KC

KERNCONF_FILES+=	"../atheros/files.ar71xx"
KERNCONF_HINTS=		"AR7242.hints"

KERNCONF_MAKEOPTIONS+=	"MIPS_BIG_ENDIAN=defined"

##KERNCONF_OPTIONS+=		MAXUSERS=3
#KERNCONF_OPTIONS+=		MAXFILES=512
##KERNCONF_OPTIONS+=		NSFBUFS=1024
#KERNCONF_OPTIONS+=		SHMALL=128
#KERNCONF_OPTIONS+=		MSGBUF_SIZE=65536

# Options for making kernel smallest 
KERNCONF_OPTIONS+=		NO_SYSCTL_DESCR
KERNCONF_OPTIONS+=		NO_FFS_SNAPSHOT
KERNCONF_OPTIONS+=		SCSI_NO_SENSE_STRINGS
KERNCONF_OPTIONS+=		SCSI_NO_OP_STRINGS

# Debug definitions
#KERNCONF_OPTIONS+=		DDB
#KERNCONF_OPTIONS+=		KDB
#KERNCONF_OPTIONS+=		PREEMPTION

# Board definitions
KERNCONF_OPTIONS+=	INET
KERNCONF_OPTIONS+= 	IPSTEALTH
KERNCONF_OPTIONS+= 	CD9660
#KERNCONF_OPTIONS+= 	SCHED_ULE
KERNCONF_OPTIONS+= 	SCHED_4BSD
KERNCONF_OPTIONS+= 	PSEUDOFS
KERNCONF_OPTIONS+=	IPFIREWALL_DEFAULT_TO_ACCEPT
#KERNCONF_OPTIONS+= 	ZERO_COPY_SOCKETS
KERNCONF_OPTIONS+=	_KPOSIX_PRIORITY_SCHEDULING
KERNCONF_OPTIONS+=	ROOTDEVNAME=\\\"cd9660:/dev/map/rootfs.uncompress\\\"

KERNCONF_DEVICES+=	uart
KERNCONF_DEVICES+=	random
KERNCONF_DEVICES+=	loop
KERNCONF_DEVICES+=	pty
KERNCONF_DEVICES+=	ether
KERNCONF_DEVICES+=	bpf
KERNCONF_DEVICES+=	vlan
KERNCONF_DEVICES+=	uart
KERNCONF_DEVICES+=	tun

KERNCONF_DEVICES+=	ar724x_pci
KERNCONF_DEVICES+=	pci
KERNCONF_DEVICES+=	ar71xx_wdog
KERNCONF_DEVICES+=	spibus
KERNCONF_DEVICES+=	ar71xx_spi
KERNCONF_DEVICES+=	mx25l
KERNCONF_DEVICES+=	mii
KERNCONF_DEVICES+=	arge
#KERNCONF_NODEVICES+=	ukphy
KERNCONF_DEVICES+=	switch
KERNCONF_DEVICES+=	switch_ar8x16

KERNCONF_DEVICES+=	gpio
KERNCONF_DEVICES+=	gpioled

KERNCONF_DEVICES+=	nvram2env
KERNCONF_DEVICES+=	geom_map
KERNCONF_DEVICES+=	geom_uncompress

KERNCONF_OPTIONS+=	SCSI_DELAY=1000

.if defined(WITH_IPSEC)
KERNCONF_OPTIONS+=         IPSEC
KERNCONF_OPTIONS+=         IPSEC_NAT_T

KERNCONF_DEVICES+=		enc
#KERNCONF_DEVICES+=		gif
KERNCONF_DEVICES+=		ipsec
KERNCONF_DEVICES+=		crypto
.endif


.if defined(WITH_USB)
KERNCONF_DEVICES+=	usb
KERNCONF_DEVICES+=	ehci
.endif






