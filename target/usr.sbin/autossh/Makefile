#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
#

PROG=		autossh

DIR=${.CURDIR}/../../../contrib/${PROG}
.PATH: ${DIR}/

VER=		1.4b
SSH=		/usr/bin/ssh
CFLAGS+=		\
	-I${DIR}	\
	-I${.CURDIR}	\
	-DVER=\"$(VER)\" \
	-DSSH_PATH=\"$(SSH)\"

MK_MAN=		no 

PREFIX?=		/
BINDIR?=		${PREFIX}/sbin


.include <bsd.prog.mk>
