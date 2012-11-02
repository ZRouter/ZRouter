#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
#

CDIR=${.CURDIR}/../../contrib/uboot_mkimage
.PATH: ${CDIR}/common ${CDIR}/lib ${CDIR}/lib/libfdt ${CDIR}/tools

PROG=		uboot_mkimage
CFLAGS+=	-D__MACH__=__FreeBSD__
CFLAGS+=	-I${CDIR}/tools -I${CDIR}/include -DUSE_HOSTCC
NO_MAN=

SRCS=			\
	mkimage.c	\
	crc32.c		\
	default_image.c	\
	fit_image.c	\
	image.c		\
	imximage.c	\
	kwbimage.c	\
	md5.c		\
	os_support.c	\
	sha1.c		\
	fdt.c		\
	fdt_ro.c	\
	fdt_rw.c	\
	fdt_strerror.c	\
	fdt_wip.c

.include <bsd.prog.mk>

