#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
#

MPDDIR=${.CURDIR}/../../../contrib/mpd
.PATH: ${MPDDIR}/src/

CFLAGS+=	-I${MPDDIR}

PROG=	mpd
NO_MAN=	mpd.8

PREFIX?=		/
BINDIR?=		${PREFIX}/sbin
VERSION=		5.6a1

# Default configuration directory

MPD_CONF_DIR?=		${PREFIX}/etc/mpd5

# Define supported physical layer types

PHYSTYPE_MODEM=		yes
PHYSTYPE_TCP=		yes
PHYSTYPE_UDP=		yes
PHYSTYPE_NG_SOCKET=	yes
PHYSTYPE_PPTP=		yes
PHYSTYPE_PPPOE=		yes
PHYSTYPE_L2TP=		yes

# Define supported CCP types

CCP_DEFLATE=		yes
CCP_MPPC=		yes
CCP_PRED1=		yes

# Define supported ECP types

ECP_DES=		yes

# Define system features wanted to use (if present)

MPD_USE_NG_BPF?=	yes
MPD_USE_NG_CAR?=	yes
MPD_USE_NG_DEFLATE?=	yes
MPD_USE_NG_IPACCT?=	yes
MPD_USE_NG_MPPC?=	yes
MPD_USE_NG_NAT?=	yes
MPD_USE_NG_NETFLOW?=	yes
MPD_USE_NG_PRED1?=	yes
MPD_USE_NG_TCPMSS?=	yes
MPD_USE_NG_VJC?=	yes
MPD_USE_IPFW?=		yes
MPD_USE_FETCH?=		no
MPD_USE_TCP_WRAP?=	yes
#MPD_USE_AUTH_OPIE?=	yes
MPD_USE_AUTH_PAM?=	yes
MPD_USE_AUTH_SYSTEM?=	yes

# Set syslog logging facility. Change LOG_DAEMON to whatever you like.

SYSLOG_FACILITY=	LOG_DAEMON
COPTS+=			-DPROTO_NAME_LIST

# Compiler & linker flags

.if defined ( MPD_USE_TCP_WRAP )
## filter TCP accept connections
LDADD+=		-lwrap
DPADD+=		${LIBWRAP}
COPTS+=		-DUSE_WRAP
.endif

## authentication methods: opie, pam, /etc/password
.if defined ( MPD_USE_AUTH_OPIE )
LDADD+=		-lopie
DPADD+=		${LIBOPIE}
COPTS+=		-DUSE_OPIE
.endif

.if defined ( MPD_USE_AUTH_PAM )
LDADD+=		-lpam
DPADD+=		${LIBPAM}
COPTS+=		-DUSE_PAM
.endif

.if defined ( MPD_USE_AUTH_SYSTEM )
## based on /etc/password
LDADD+=		-lcrypt -lutil
DPADD+=		${LIBCRYPT} ${LIBUTIL}
COPTS+=		-DUSE_SYSTEM
.endif

LDADD+=		-lnetgraph
DPADD+=		${LIBNETGRAPH}

LDADD+=		-L/usr/lib -lradius
DPADD+=		${LIBRADIUS}

LDADD+=		-lssl
DPADD+=		${LIBSSL}

MPD_VERSION=	mpd-${VERSION}

COPTS+=		-DPATH_CONF_DIR=\"${MPD_CONF_DIR}\"
COPTS+=		-DSYSLOG_FACILITY=${SYSLOG_FACILITY}
COPTS+=		-DMPD_VERSION='"${MPD_VERSION}"'
#COPTS+=		-g
COPTS+=		-Wall \
		-Wcast-align \
		-Wchar-subscripts \
		-Wformat \
		-Winline \
		-Wmissing-declarations \
		-Wmissing-prototypes \
		-Wnested-externs \
		-Wpointer-arith \
		-Wwrite-strings \
		-pthread \
		-I${PREFIX}/include

.if defined ( MPD_VENDOR )
COPTS+=		'-DMPD_VENDOR="${MPD_VENDOR}"'
.endif

# Standard sources

STDSRCS=        assert.c auth.c bund.c rep.c ccp.c chap.c \
		console.c command.c ecp.c event.c fsm.c iface.c input.c \
		ip.c ipcp.c ipv6cp.c lcp.c link.c log.c main.c mbuf.c mp.c \
		msg.c ngfunc.c pap.c phys.c proto.c radius.c radsrv.c timer.c \
		util.c vars.c eap.c msoft.c ippool.c


COPTS+=		-DNOLIBPDEL -I${MPDDIR}/src/contrib/libpdel

.PATH:	${MPDDIR}/src/contrib/libpdel/util
.PATH:  ${MPDDIR}/src/contrib/libpdel/structs
#.PATH:	${MPDDIR}/src/contrib/libpdel/http
#.PATH:  ${MPDDIR}/src/contrib/libpdel/http/servlet
.PATH:	${MPDDIR}/src/contrib/libpdel/sys
.PATH:  ${MPDDIR}/src/contrib/libpdel/io
.PATH:	${MPDDIR}/src/contrib/libpdel/structs/type

PDELSRCS=	typed_mem.c \
		pevent.c \
		paction.c \
		ghash.c \
		gtree.c \
		mesg_port.c \
		structs.c \
		structs_generic.c \
		structs_type_array.c \
		structs_type_int.c \
		structs_type_string.c \
		structs_type_struct.c \
		boundary_fp.c \
		ssl_fp.c \
		string_fp.c \
		timeout_fp.c

COPTS+=		-DNOWEB

# Add sources and flags for the various physical layer types

.if defined ( PHYSTYPE_MODEM )
SRCS+=		modem.c chat.c
COPTS+=		-DPHYSTYPE_MODEM
.endif
.if defined ( PHYSTYPE_UDP )
SRCS+=		udp.c
COPTS+=		-DPHYSTYPE_UDP
.endif
.if defined ( PHYSTYPE_TCP )
SRCS+=		tcp.c
COPTS+=		-DPHYSTYPE_TCP
.endif
.if defined ( PHYSTYPE_NG_SOCKET )
SRCS+=		ng.c
COPTS+=		-DPHYSTYPE_NG_SOCKET
.endif
.if defined ( PHYSTYPE_PPTP )
SRCS+=		pptp.c pptp_ctrl.c
COPTS+=		-DPHYSTYPE_PPTP
COPTS+=		-DRANDOMIZE_CID
#COPTS+=		-DLOOK_LIKE_NT
.endif
.if defined ( PHYSTYPE_PPPOE )
SRCS+=		pppoe.c
COPTS+=		-DPHYSTYPE_PPPOE
.endif
.if defined ( PHYSTYPE_L2TP )
SRCS+=		l2tp.c l2tp_avp.c l2tp_ctrl.c
COPTS+=		-DPHYSTYPE_L2TP
.endif

# CCP

.if defined ( CCP_DEFLATE ) && defined ( MPD_USE_NG_DEFLATE )
SRCS+=		ccp_deflate.c
COPTS+=		-DCCP_DEFLATE
COPTS+=         -DUSE_NG_DEFLATE
.endif

.if defined ( CCP_MPPC ) && defined ( MPD_USE_NG_MPPC )
SRCS+=		ccp_mppc.c
COPTS+=		-DCCP_MPPC
COPTS+=         -DUSE_NG_MPPC
.endif

.if defined ( CCP_PRED1 )
SRCS+=		ccp_pred1.c
COPTS+=		-DCCP_PRED1
.if defined ( MPD_USE_NG_PRED1 )
COPTS+=         -DUSE_NG_PRED1
.endif
.endif

# ECP

.if defined ( ECP_DES )
SRCS+=		ecp_dese.c
SRCS+=		ecp_dese_bis.c
COPTS+=		-DECP_DES
NEED_DES=	yes
.endif

# Features

.if defined ( MPD_USE_NG_BPF )
COPTS+=		-DUSE_NG_BPF
LDADD+=		-lpcap
DPADD+=		${LIBPCAP}
.endif
.if defined ( MPD_USE_NG_CAR )
COPTS+=         -DUSE_NG_CAR
.endif
.if defined ( MPD_USE_NG_IPACCT )
COPTS+=         -DUSE_NG_IPACCT
.endif
.if defined ( MPD_USE_NG_NAT )
SRCS+=		nat.c
COPTS+=         -DUSE_NG_NAT
.endif
.if defined ( MPD_USE_NG_NETFLOW )
COPTS+=         -DUSE_NG_NETFLOW
.endif
.if defined ( MPD_USE_NG_TCPMSS )
COPTS+=         -DUSE_NG_TCPMSS
.endif
.if defined ( MPD_USE_NG_VJC )
COPTS+=         -DUSE_NG_VJC
.endif
.if defined ( MPD_USE_IPFW )
COPTS+=         -DUSE_IPFW
.endif
.if defined ( MPD_USE_FETCH )
COPTS+=         -DUSE_FETCH
LDADD+=		-lfetch
DPADD+=		${LIBFETCH}
.endif

# Add in required support files and libraries

.if defined ( NEED_DES )
LDADD+=		-lcrypto
DPADD+=		${LIBCRYPTO}
.endif

# Now add standard sources

SRCS+=		${STDSRCS}

SRCS+=		${PDELSRCS}

CFLAGS+=	${COPTS}

.include <bsd.prog.mk>
