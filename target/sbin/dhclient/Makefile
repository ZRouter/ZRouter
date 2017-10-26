#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
#

DHCPDIR=${.CURDIR}/../../../contrib/dhcp
.PATH: ${DHCPDIR}/client

CFLAGS+=	-I${DHCPDIR} -I${DHCPDIR}/includes 

PROG=		dhclient
MK_MAN=	 no	

PREFIX?=	/
BINDIR?=	${PREFIX}/sbin

ZROUTER_OBJDIR=	${.OBJDIR:C/target\/sbin\/dhclient//}
LDFLAGS+=	-L${ZROUTER_OBJDIR}/target/lib/libdhcp/

SRCS=		clparse.c dhclient.c

.include <bsd.prog.mk>
