#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
#

DIR=${.CURDIR}/../../../contrib/ser2net
.PATH: ${DIR}/

CFLAGS+=	-I${DIR} -I${.CURDIR}

PROG=		ser2net
MK_MAN=		no	

PREFIX?=		/
BINDIR?=		${PREFIX}/sbin

CFLAGS+=			\
	-DPACKAGE_NAME=\"\"	\
	-DPACKAGE_TARNAME=\"\"	\
	-DPACKAGE_VERSION=\"\"	\
	-DPACKAGE_STRING=\"\"	\
	-DPACKAGE_BUGREPORT=\"\"\
	-DPACKAGE=\"ser2net\"	\
	-DVERSION=\"2.5\"	\
	-DSTDC_HEADERS=1	\
	-DHAVE_SYS_TYPES_H=1	\
	-DHAVE_SYS_STAT_H=1	\
	-DHAVE_STDLIB_H=1	\
	-DHAVE_STRING_H=1	\
	-DHAVE_MEMORY_H=1	\
	-DHAVE_STRINGS_H=1	\
	-DHAVE_INTTYPES_H=1	\
	-DHAVE_STDINT_H=1	\
	-DHAVE_UNISTD_H=1	\
	-DHAVE_DLFCN_H=1	\
	-DUSE_UUCP_LOCKING=1	\
	-DSTDC_HEADERS=1

SRCS=			\
	controller.c	\
	dataxfer.c	\
	devcfg.c	\
	readconfig.c	\
	selector.c	\
	ser2net.c	\
	utils.c		\
	telnet.c

.include <bsd.prog.mk>
