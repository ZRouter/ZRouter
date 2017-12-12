#
# $FreeBSD$
#

DHCPDIR=${.CURDIR}/../../../contrib/dhcp
.PATH: ${DHCPDIR}/relay

CFLAGS+=	-I${DHCPDIR} -I${DHCPDIR}/includes

PROG=		dhcrelay
MK_MAN=	 	no	

PREFIX?=	/
BINDIR?=	${PREFIX}/sbin

LOCAL_LIBRARIES=	dhcp
LIBADD=			dhcp

ZROUTER_OBJDIR=	${.OBJDIR:C/target\/sbin\/dhcrelay//}
LDFLAGS+=	-L${ZROUTER_OBJDIR}/target/lib/libdhcp/

SRCS=		dhcrelay.c

.include <bsd.prog.mk>
