#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
#

CDIR=${.CURDIR}/../../../contrib/dnsmasq
.PATH: ${CDIR}/src

CFLAGS+=	-DNO_IPV6
#CFLAGS+=	 -DHAVE_IDN

PROG=		dnsmasq
MK_MAN= no
	
PREFIX?=	/
BINDIR?=	${PREFIX}/sbin

SRCS=	\
	cache.c \
	rfc1035.c \
	util.c \
	option.c \
	forward.c \
	network.c \
	dnsmasq.c \
	dhcp.c \
	lease.c \
	rfc2131.c \
	netlink.c \
	dbus.c \
	bpf.c \
	helper.c \
	tftp.c \
	log.c

.include <bsd.prog.mk>

