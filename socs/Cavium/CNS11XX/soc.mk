
#XXX testing
KERNEL_PATH=/usr/obj/kernel

TARGET=			arm
TARGET_ARCH=		arm
TARGET_CPUTYPE=		armv4

# clang is not support armv4
#BUILD_ZROUTER_WITH_GCC=yes

KERNCONF_MACHINE=	${TARGET} ${TARGET_ARCH}
KERNCONF_IDENT?=	CNS11XX
KERNCONF_CPU+= 		CPU_FA526
KERNCONF_FILES+=	"../cavium/cns11xx/files.econa"
#KERNCONF_MAKEOPTIONS+=	"CONF_CFLAGS=\"-march=armv4 -mno-thumb-interwork\""
#KERNCONF_MAKEOPTIONS+=	"CONF_CFLAGS=\"-mno-thumb-interwork\""
WORLD_BUILD_ENV_EXTRA+=	"CFLAGS+=\"-march=armv4 -mno-thumb-interwork\""

#KERNCONF_OPTIONS+=	VERBOSE_SYSINIT

KERNCONF_OPTIONS+=	FDT
KERNCONF_OPTIONS+=	FDT_DTB_STATIC
#KERNCONF_MAKEOPTIONS+=	"FDT_DTS_FILE=foo.dts"

KERNCONF_OPTIONS+=	INTRNG

#KERNCONF_OPTIONS+=	NO_EVENTTIMERS

KERNCONF_OPTIONS+=	HZ=100
#KERNCONF_OPTIONS+=	DEVICE_POLLING

KERNCONF_OPTIONS+=	INVARIANTS
KERNCONF_OPTIONS+=	INVARIANT_SUPPORT
KERNCONF_OPTIONS+=	WITNESS
KERNCONF_OPTIONS+=	WITNESS_SKIPSPIN
KERNCONF_OPTIONS+=	DEBUG_LOCKS
KERNCONF_OPTIONS+=	DIAGNOSTIC

KERNCONF_MAKEOPTIONS+=	"KERNPHYSADDR=0x00000000"
KERNCONF_MAKEOPTIONS+=	"KERNVIRTADDR=0xc0050000"

KERNCONF_OPTIONS+=	PHYSADDR=0x00000000
KERNCONF_OPTIONS+=	KERNVIRTADDR=0xc0000000

PHYSADDR=		0x00000000

#KERNCONF_OPTIONS+=	FLASHADDR=0xD0000000
#KERNCONF_OPTIONS+=	LOADERRAMADDR=0x00000000

# map to 64MByte at locore-v4.S for debug
# map to 0xcc000000 - 0xcfffffff from 0x1c000000 - 0x1fffffff
#KERNCONF_OPTIONS+=	SOCDEV_PA=0x1c000000
#KERNCONF_OPTIONS+=	SOCDEV_VA=0xcc000000
#KERNCONF_OPTIONS+=	EARLY_PRINTF;

#KERNCONF_OPTIONS+=	FLASHADDR=0x50000000
#KERNCONF_OPTIONS+=	LOADERRAMADDR=0x00000000
#KERNCONF_OPTIONS+=	STARTUP_PAGETABLE_ADDR=0x00000000

#KERNCONF_OPTIONS+=	LINUX_BOOT_ABI

#KERNCONF_OPTIONS+= 	HWPMC_HOOKS
KERNCONF_OPTIONS+= 	VERBOSE_INIT_ARM
#KERNCONF_OPTIONS+= 	IXP4XX_FLASH_SIZE=0x00400000
#KERNCONF_OPTIONS+= 	ATA_CAM
#KERNCONF_OPTIONS+= 	ATA_STATIC_ID
#KERNCONF_OPTIONS+= 	USB_EHCI_BIG_ENDIAN_DESC

KERNCONF_OPTIONS+=		MAXUSERS=3
KERNCONF_OPTIONS+=		MAXFILES=512
KERNCONF_OPTIONS+=		NSFBUFS=1024
KERNCONF_OPTIONS+=		SHMALL=128
KERNCONF_OPTIONS+=		MSGBUF_SIZE=65536

# Options for making kernel smallest 
KERNCONF_OPTIONS+=		NO_SYSCTL_DESCR
KERNCONF_OPTIONS+=		NO_FFS_SNAPSHOT
KERNCONF_OPTIONS+=		SCSI_NO_SENSE_STRINGS
KERNCONF_OPTIONS+=		SCSI_NO_OP_STRINGS


# Debug definitions
##KERNCONF_MAKEOPTIONS+=	"DEBUG=-g"
KERNCONF_OPTIONS+=		DDB
KERNCONF_OPTIONS+=		KDB
#KERNCONF_OPTIONS+=		PREEMPTION
##KERNCONF_OPTIONS+=		KTRACE
#KERNCONF_OPTIONS+=		LOCK_PROFILING
#KERNCONF_OPTIONS+=		KTR

# Board definitions
KERNCONF_OPTIONS+=	INET
KERNCONF_OPTIONS+= 	IPSTEALTH
#KERNCONF_OPTIONS+= 	FFS
#KERNCONF_OPTIONS+= 	SOFTUPDATES
#KERNCONF_OPTIONS+= 	UFS_ACL
#KERNCONF_OPTIONS+= 	UFS_DIRHASH
#KERNCONF_OPTIONS+= 	TMPFS
KERNCONF_OPTIONS+= 	CD9660
#KERNCONF_OPTIONS+= 	GEOM_LABEL
#KERNCONF_OPTIONS+= 	SCHED_ULE
KERNCONF_OPTIONS+= 	SCHED_4BSD
#KERNCONF_OPTIONS+= 	NFSCLIENT
#KERNCONF_OPTIONS+= 	NFS_ROOT
KERNCONF_OPTIONS+= 	PSEUDOFS
KERNCONF_OPTIONS+=	_KPOSIX_PRIORITY_SCHEDULING
KERNCONF_OPTIONS+=	MUTEX_NOINLINE
KERNCONF_OPTIONS+=	RWLOCK_NOINLINE
KERNCONF_OPTIONS+=	SX_NOINLINE


#KERNCONF_OPTIONS+= 	SMP

#KERNCONF_DEVICES+=	pci
KERNCONF_DEVICES+=	random
KERNCONF_DEVICES+=	loop
KERNCONF_DEVICES+=	pty
KERNCONF_DEVICES+=	ether
KERNCONF_DEVICES+=	bpf
KERNCONF_DEVICES+=	vlan
KERNCONF_DEVICES+=	uart
KERNCONF_DEVICES+=	uart_ns8250
KERNCONF_DEVICES+=	tuntap

KERNCONF_DEVICES+=	gpio
KERNCONF_DEVICES+=	gpioled

KERNCONF_DEVICES+=	mii

.if defined(WITH_USB)
KERNCONF_DEVICES+=	usb
KERNCONF_OPTIONS+=	USB_DEBUG
KERNCONF_DEVICES+=	ohci
KERNCONF_DEVICES+=	ehci

KERNCONF_DEVICES+=	umass
KERNCONF_DEVICES+=	scbus
KERNCONF_DEVICES+=	da
.endif

#KERNCONF_OPTIONS+= 	SC_PIXEL_MODE

KERNCONF_OPTIONS+= 	CAMDEBUG

KERNCONF_OPTIONS+=	SCSI_DELAY=1000

.if defined(WITH_IPSEC)
KERNCONF_OPTIONS+=         IPSEC

KERNCONF_DEVICES+=		enc
KERNCONF_DEVICES+=		ipsec
KERNCONF_DEVICES+=		crypto
.endif

