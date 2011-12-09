#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
#

CDIR=${.CURDIR}/../../contrib/ubnt-mkfwimage
.PATH: ${CDIR}

PROG=	ubnt-mkfwimage
LDADD=-lz
DPADD=${LIBZ}
NO_MAN=

SRCS=	mkfwimage.c

.include <bsd.prog.mk>

