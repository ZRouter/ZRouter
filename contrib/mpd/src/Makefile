# $Id: Makefile,v 1.102 2010/12/21 12:14:29 amotin Exp $
#
# Makefile for mpd, multi-link PPP daemon for FreeBSD
#
# Written by Archie Cobbs <archie@freebsd.org>
# Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
# See ``COPYRIGHT.whistle''
#

PROG?=			mpd5
PREFIX?=		/usr/local
BINDIR?=		${PREFIX}/sbin
VERSION=		5.6a1
NOMAN=
NO_MAN=

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

USE_NG_BPF=		yes
USE_NG_CAR=		yes
USE_NG_DEFLATE=		yes
USE_NG_IPACCT=		yes
USE_NG_MPPC=		yes
USE_NG_NAT=		yes
USE_NG_NETFLOW=		yes
USE_NG_PRED1=		yes
USE_NG_TCPMSS=		yes
USE_NG_VJC=		yes
USE_IPFW=		yes
USE_FETCH=		yes
USE_TCP_WRAP=		yes
#USE_AUTH_OPIE=		yes
USE_AUTH_PAM=		yes
USE_AUTH_SYSTEM=	yes

# Build with builtin libpdel

NOLIBPDEL=		yes

# Set syslog logging facility. Change LOG_DAEMON to whatever you like.

SYSLOG_FACILITY=	LOG_DAEMON
COPTS+=			-DPROTO_NAME_LIST

# Compiler & linker flags

.if defined ( USE_TCP_WRAP )
## filter TCP accept connections
LDADD+=		-lwrap
DPADD+=		${LIBWRAP}
COPTS+=		-DUSE_WRAP
.endif

## authentication methods: opie, pam, /etc/password
.if defined ( USE_AUTH_OPIE )
LDADD+=		-lopie
DPADD+=		${LIBOPIE}
COPTS+=		-DUSE_OPIE
.endif

.if defined ( USE_AUTH_PAM )
LDADD+=		-lpam
DPADD+=		${LIBPAM}
COPTS+=		-DUSE_PAM
.endif

.if defined ( USE_AUTH_SYSTEM )
## based on /etc/password
LDADD+=		-lcrypt -lutil
DPADD+=		${LIBCRYPT} ${LIBUTIL}
COPTS+=		-DUSE_SYSTEM
.endif

LDADD+=		-lnetgraph
DPADD+=		${LIBNETGRAPH}

LDADD+=		-L/usr/lib -lradius
DPADD+=		${LIBRADIUS}

## libpdel needs ssl and libexpat (libxml2 cannot be used)
.if !defined ( NOLIBPDEL )
LDADD+=		-L${PREFIX}/lib
LDADD+=		-lpdel
LDADD+=		-lexpat
.endif
LDADD+=		-lssl

MPD_VERSION!=	echo -n "${VERSION} (`id -un`@`uname -n` `LC_TIME=C date +'%R %v'`)"

COPTS+=		-DPATH_CONF_DIR=\"${MPD_CONF_DIR}\"
COPTS+=		-DSYSLOG_FACILITY=${SYSLOG_FACILITY}
COPTS+=		-DMPD_VERSION='"${MPD_VERSION}"'
COPTS+=		-g
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
		util.c vars.c eap.c msoft.c web.c ippool.c

.if defined ( NOLIBPDEL )
COPTS+=		-DNOLIBPDEL -I./contrib/libpdel
PDPATH1=	contrib/libpdel/util:contrib/libpdel/structs
PDPATH2=	contrib/libpdel/http:contrib/libpdel/http/servlet
PDPATH3=	contrib/libpdel/sys:contrib/libpdel/io
PDPATH4=	contrib/libpdel/structs/type
VPATH+=		$(PDPATH1):$(PDPATH2):$(PDPATH3):$(PDPATH4)
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
		http_connection.c \
		http_head.c \
		http_message.c \
		http_mime.c \
		http_request.c \
		http_response.c \
		http_server.c \
		http_servlet_basicauth.c \
		http_ssl.c \
		http_status.c \
		boundary_fp.c \
		ssl_fp.c \
		string_fp.c \
		timeout_fp.c
.endif

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

.if defined ( CCP_DEFLATE ) && defined ( USE_NG_DEFLATE )
SRCS+=		ccp_deflate.c
COPTS+=		-DCCP_DEFLATE
COPTS+=         -DUSE_NG_DEFLATE
.endif

.if defined ( CCP_MPPC ) && defined ( USE_NG_MPPC )
SRCS+=		ccp_mppc.c
COPTS+=		-DCCP_MPPC
COPTS+=         -DUSE_NG_MPPC
.endif

.if defined ( CCP_PRED1 )
SRCS+=		ccp_pred1.c
COPTS+=		-DCCP_PRED1
.if defined ( USE_NG_PRED1 )
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

.if defined ( USE_NG_BPF )
COPTS+=		-DUSE_NG_BPF
LDADD+=		-lpcap
DPADD+=		${LIBPCAP}
.endif
.if defined ( USE_NG_CAR )
COPTS+=         -DUSE_NG_CAR
.endif
.if defined ( USE_NG_IPACCT )
COPTS+=         -DUSE_NG_IPACCT
.endif
.if defined ( USE_NG_NAT )
SRCS+=		nat.c
COPTS+=         -DUSE_NG_NAT
.endif
.if defined ( USE_NG_NETFLOW )
COPTS+=         -DUSE_NG_NETFLOW
.endif
.if defined ( USE_NG_TCPMSS )
COPTS+=         -DUSE_NG_TCPMSS
.endif
.if defined ( USE_NG_VJC )
COPTS+=         -DUSE_NG_VJC
.endif
.if defined ( USE_IPFW )
COPTS+=         -DUSE_IPFW
.endif
.if defined ( USE_FETCH )
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

.if defined ( NOLIBPDEL )
SRCS+=		${PDELSRCS}
.endif

.include <bsd.prog.mk>
