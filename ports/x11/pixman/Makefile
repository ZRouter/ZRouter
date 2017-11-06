# $FreeBSD: tags/RELEASE_10_3_0/x11/pixman/Makefile 399873 2015-10-20 18:49:47Z jmmv $

PORTNAME=	pixman
PORTVERSION=	0.32.8
CATEGORIES=	x11

MAINTAINER=	x11@FreeBSD.org
COMMENT=	Low-level pixel manipulation library

LICENSE=	MIT
LICENSE_FILE=	${WRKSRC}/COPYING

PORTSCOUT=	limitw:1,even

#USES=		cpe perl5
#USE_PERL5=	build
XORG_CAT=	lib
#CONFIGURE_ARGS=	--disable-gtk
# host option is no effect
CONFIGURE_ARGS=	--disable-mmx --disable-sse2 --disable-ssse3
INSTALL_TARGET=	install-strip

# Build the library exclusively, thus disabling the demos and the tests (which
# do not get installed anyway).  This is to workaround a problem in powerpc64
# where the build of the tests crashes ld; see PR bin/202284.
MAKE_ARGS=      SUBDIRS=pixman

post-configure:
	@echo "#define WORDS_BIGENDIAN 1" >> ${WRKSRC}/config.h

do-install:
	mkdir ${ZPREFIX}/lib
	${INSTALL_LIB} ${WRKSRC}/pixman/.libs/libpixman-1.so.0.32.8 ${ZPREFIX}/lib/libpixman-1.so.0
	cd ${ZPREFIX}/lib;ln -s libpixman-1.so.0 libpixman-1.so

.include <bsd.port.mk>
