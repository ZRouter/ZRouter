#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
#

DHCPDIR=${.CURDIR}/../../../contrib/dhcp
.PATH: ${DHCPDIR}/server

CFLAGS+=	-I${DHCPDIR} -I${DHCPDIR}/includes

PROG=		dhcpd
NO_MAN=

PREFIX?=	/
BINDIR?=	${PREFIX}/sbin
LDADD+=		-ldhcp
DPADD+=		${LIBDHCP}

ZROUTER_OBJDIR=	${.OBJDIR:C/target\/sbin\/dhcpd//}
LDFLAGS+=	-L${ZROUTER_OBJDIR}/target/lib/libdhcp/

SRCS=		bootp.c confpars.c db.c dhcp.c dhcpd.c

.include <bsd.prog.mk>
