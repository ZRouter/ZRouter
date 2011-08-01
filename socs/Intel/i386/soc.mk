
#XXX testing
LZMA=lzma
KERNEL_PATH=/usr/obj/kernel

TARGET=			i386
TARGET_ARCH=		i386

KERNCONF_MACHINE=	${TARGET} ${TARGET_ARCH}
KERNCONF_IDENT?=	i386
KERNCONF_CPU=		I686_CPU
KERNCONF_HINTS=		"i386.hints"


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
#KERNCONF_OPTIONS+=		DDB
#KERNCONF_OPTIONS+=		KDB
##KERNCONF_OPTIONS+=		PREEMPTION
##KERNCONF_OPTIONS+=		KTRACE
#KERNCONF_OPTIONS+=		LOCK_PROFILING
#KERNCONF_OPTIONS+=		KTR

# Board definitions
KERNCONF_OPTIONS+=	INET
KERNCONF_OPTIONS+= 	IPSTEALTH
KERNCONF_OPTIONS+= 	FFS
KERNCONF_OPTIONS+= 	SOFTUPDATES
KERNCONF_OPTIONS+= 	UFS_ACL
KERNCONF_OPTIONS+= 	UFS_DIRHASH
#KERNCONF_OPTIONS+= 	TMPFS
KERNCONF_OPTIONS+= 	CD9660
#KERNCONF_OPTIONS+= 	GEOM_LABEL
KERNCONF_OPTIONS+= 	SCHED_ULE
#KERNCONF_OPTIONS+= 	SCHED_4BSD
#KERNCONF_OPTIONS+= 	NFSCLIENT
#KERNCONF_OPTIONS+= 	NFS_ROOT
KERNCONF_OPTIONS+= 	PSEUDOFS
KERNCONF_OPTIONS+=	IPFIREWALL_DEFAULT_TO_ACCEPT
KERNCONF_OPTIONS+= 	ZERO_COPY_SOCKETS
KERNCONF_OPTIONS+=	_KPOSIX_PRIORITY_SCHEDULING
KERNCONF_OPTIONS+=		ROOTDEVNAME=\\\"cd9660:/dev/map/rootfs.uncompress\\\"

KERNCONF_OPTIONS+= 	SMP

KERNCONF_DEVICES+=	apic
KERNCONF_DEVICES+=	cpufreq
KERNCONF_DEVICES+=	acpi
KERNCONF_DEVICES+=	eisa
KERNCONF_DEVICES+=	pci
KERNCONF_DEVICES+=	random
KERNCONF_DEVICES+=	loop
KERNCONF_DEVICES+=	pty
KERNCONF_DEVICES+=	ether
KERNCONF_DEVICES+=	bpf
KERNCONF_DEVICES+=	vlan
KERNCONF_DEVICES+=	uart
KERNCONF_DEVICES+=	tun

KERNCONF_OPTIONS+= 	SC_PIXEL_MODE

KERNCONF_DEVICES+=	\
        ahci	\
        scbus	\
        ch	\
        da	\
        sa	\
        cd	\
        pass	\
        ses	\
        atkbdc	\
        atkbd	\
        psm	\
        kbdmux	\
        vga	\
        splash	\
        sc	\
        agp	\
        pmtimer	\
        uart	\
        ppc	\
        ppbus	\
        lpt	\
        plip	\
        ppi	\
        miibus	\
        alc	\
        bce	\
        bge	\
        dc	\
        fxp	\
        lge	\
        msk	\
        re	\
        rl	\
        sf	\
        sk	\
        vr	\
        xl	\
        ed	\
        uhci	\
        ohci	\
        ehci	\
        xhci	\
        usb	\
        uhid	\
        ukbd	\
        ulpt	\
        umass	\
        u3g	\
        ubsa	\
        uftdi	\
        uplcom	\
        uslcom	\
        aue	\
        axe	\
        cdce	\
        cue	\
        kue	\
        rue	\
        udav


KERNCONF_OPTIONS+=	SCSI_DELAY=1000

.if defined(WITH_IPSEC)
KERNCONF_OPTIONS+=         IPSEC
KERNCONF_OPTIONS+=         IPSEC_NAT_T

KERNCONF_DEVICES+=		random
KERNCONF_DEVICES+=		enc
KERNCONF_DEVICES+=		ipsec
KERNCONF_DEVICES+=		crypto
.endif

WORLD_SUBDIRS+=			\
	sys/boot/i386/btx	\
	sys/boot/i386/pmbr	\
	sys/boot/i386/gptboot

