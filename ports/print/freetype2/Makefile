# Created by: jseger@FreeBSD.org
# $FreeBSD: tags/RELEASE_10_3_0/print/freetype2/Makefile 403474 2015-12-10 22:13:58Z kwm $

PORTNAME=	freetype2
PORTVERSION=	2.8
CATEGORIES=	print
MASTER_SITES=	http://savannah.nongnu.org/download/freetype/ \
		SF/freetype/${PORTNAME}/${PORTVERSION:C/^([0-9]+\.[0-9]+\.[0-9]+).*/\1/}/ \
		http://sunsite.cnlab-switch.ch/ftp/mirror/freetype/freetype2/ \
		http://www.funet.fi/pub/mirrors/ftp.freetype.org/freetype2/ \
		http://ftp.sunet.se/pub/text-processing/freetype/freetype2/ \
		ftp://ftp.freetype.org/freetype/freetype2/
DISTNAME=	${PORTNAME:S/2//}-${PORTVERSION}

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	Free and portable TrueType font rendering engine

USES=		cpe gmake libtool tar:bzip2
MAKE_ENV=	TOP=""
USE_LDCONFIG=	yes
GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--without-harfbuzz --without-png CFLAGS=-DFT_CONFIG_OPTION_SUBPIXEL_RENDERING 
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd
CONFIGURE_WRKSRC=	${WRKSRC}/builds/unix

CPE_PRODUCT=	freetype
CPE_VENDOR=	freetype

#OPTIONS_DEFINE=		LCD_FILTERING PNG
#OPTIONS_DEFAULT=	LCD_FILTERING
#LCD_FILTERING_DESC?=	Sub-pixel rendering (patented)
#PNG_DESC=		Png compressed OpenType embedded bitmaps support

LCD_FILTERING_CFLAGS=	-DFT_CONFIG_OPTION_SUBPIXEL_RENDERING

PNG_LIB_DEPENDS=	libpng.so:${PORTSDIR}/graphics/png
PNG_CONFIGURE_OFF=	--without-png

pre-patch:
	@${REINPLACE_CMD} -e 's|[(]libdir[)]/pkgconfig|(prefix)/libdata/pkgconfig|g' \
		${WRKSRC}/builds/unix/install.mk

post-configure:
	@(cd ${BUILD_WRKSRC}; ${SETENV} ${MAKE_ENV} \
		${MAKE_CMD} ${MAKE_FLAGS} ${MAKEFILE} ${MAKE_ARGS} setup)
	@${REINPLACE_CMD} -e 's|^CCraw_build.*|CCraw_build  := /usr/bin/cc|' \
		${WRKSRC}/builds/unix/unix-cc.mk

#post-install:
#	@${STRIP_CMD} ${STAGEDIR}${PREFIX}/lib/libfreetype.so.*

post-install:
	@${STRIP_CMD} ${STAGEDIR}${PREFIX}/lib/libfreetype.so.*
	${INSTALL_LIB} ${STAGEDIR}${PREFIX}/lib/libfreetype.so.6.14.0 ${ZPREFIX}/lib/libfreetype.so.6
	cd ${ZPREFIX}/lib;ln -s libfreetype.so.6 libfreetype.so
	cp ${STAGEDIR}${PREFIX}/include/freetype2/* ${ZPREFIX}/include/
	${MKDIR} ${ZPREFIX}/libdata/pkgconfig
	cp ${STAGEDIR}${PREFIX}/libdata/pkgconfig/freetype2.pc ${ZPREFIX}/libdata/pkgconfig

.include <bsd.port.mk>
