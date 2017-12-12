#
# $FreeBSD$
#

DHCPDIR=${.CURDIR}/../../../contrib/dhcp
.PATH: ${DHCPDIR}/client

CFLAGS+=	-I${DHCPDIR} -I${DHCPDIR}/includes 

PROG=		dhclient
MK_MAN=		no

PREFIX?=	/
BINDIR?=	${PREFIX}/sbin

LOCAL_LIBRARIES=	dhcp
LIBADD=			dhcp

ZROUTER_OBJDIR=	${.OBJDIR:C/target\/sbin\/dhclient//}
LDFLAGS+=	-L${ZROUTER_OBJDIR}/target/lib/libdhcp/

SRCS=		clparse.c dhclient.c

.include <bsd.prog.mk>
