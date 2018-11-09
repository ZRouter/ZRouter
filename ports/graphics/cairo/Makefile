# Created by: Michael Johnson <ahze@FreeBSD.org>
# $FreeBSD: tags/RELEASE_10_3_0/graphics/cairo/Makefile 403473 2015-12-10 21:50:14Z kwm $

PORTNAME=	cairo
PORTVERSION=	1.14.6
PORTEPOCH=	2
CATEGORIES=	graphics
MASTER_SITES=	http://cairographics.org/releases/

MAINTAINER=	gnome@FreeBSD.org
COMMENT=	Vector graphics library with cross-device output support

#LICENSE=	LGPL21 MPL
#LICENSE_COMB=	dual

USES=		tar:xz

.if !defined(REFERENCE_PORT)

#LIB_DEPENDS=	libfreetype.so:${PORTSDIR}/print/freetype2 \ #
#		libpng.so:${PORTSDIR}/graphics/png \ #
#		libfontconfig.so:${PORTSDIR}/x11-fonts/fontconfig

#PORTSCOUT=	skipv:5c-1.1,5c-1.2,5c-1.5

GNU_CONFIGURE=	yes
CONFIGURE_ARGS+=	--enable-gtk-doc-html=no \
		--disable-directfb \
		--enable-svg=no \
		--disable-xlib \
		--disable-xcb \
		--disable-svg \
		--disable-ps \
		--disable-script \
		--disable-trace \
		--disable-interpreter \
		--disable-pdf \
		--enable-script=no \
		--enable-gobject=no \
		--enable-tee=no \
		--enable-ft=yes \
		CFLAGS=-I${ZPREFIX}/include/ \
		LDFLAGS=-L${ZPREFIX}/lib/
#USES+=		cpe libtool pathfix pkgconfig
#CPE_VENDOR=	cairographics
#USE_XORG=	pixman
#USE_GNOME=	referencehack
#USE_LDCONFIG=	yes
#INSTALL_TARGET=	install-strip

.include <bsd.port.options.mk>

post-patch:
	@${REINPLACE_CMD} -e '/strings/s/ - / -a /' \
		-e '/test/s/==/=/g' \
		-e '/LIBS/s/-lrt//' -e '/LIBS/s|-ldld||' \
		-e '/LIBS/s|-ldl||' \
		${WRKSRC}/configure
	@${REINPLACE_CMD} -e 's|-lcairo|-lcairo -lpthread|' \
		${WRKSRC}/src/*.pc.in
	@${REINPLACE_CMD} -e '/@CAIRO_HAS_PNG_FUNCTIONS_TRUE@.*=/d' \
		${WRKSRC}/Makefile.in
	@${REINPLACE_CMD} -e 's|-ldl||g' \
		${WRKSRC}/util/*/Makefile.in \
		${WRKSRC}/boilerplate/Makefile.in

post-configure:
	@printenv
	@${REINPLACE_CMD} -e 's|HAVE___UINT128_T 1|HAVE___UINT128_T 0|' \
		${WRKSRC}/config.h
	@${REINPLACE_CMD} -e 's|HAVE_CXX11_ATOMIC_PRIMITIVES 1|HAVE_CXX11_ATOMIC_PRIMITIVES 0|' \
		${WRKSRC}/config.h
	@${REINPLACE_CMD} -e 's|^CAIRO_LIBS.*|CAIRO_LIBS = -lpixman-1 -L${ZPREFIX}/lib|' \
		${WRKSRC}/Makefile
	@${REINPLACE_CMD} -e 's|^CAIRO_LIBS.*|CAIRO_LIBS = -lpixman-1 -L${ZPREFIX}/lib|' \
		${WRKSRC}/src/Makefile
	@${REINPLACE_CMD} -e 's|^CAIRO_LIBS.*|CAIRO_LIBS = -lpixman-1 -L${ZPREFIX}/lib|' \
		${WRKSRC}/util/*/Makefile
	@echo "#define WORDS_BIGENDIAN 1" >> ${WRKSRC}/config.h
	@echo "#define FLOAT_WORDS_BIGENDIAN 1" >> ${WRKSRC}/config.h

post-install:
	${INSTALL_LIB} ${STAGEDIR}${PREFIX}/lib/libcairo.so.11402 ${ZPREFIX}/lib/libcairo.so.11402
	cp -r ${STAGEDIR}${PREFIX}/include/cairo ${ZPREFIX}/include/
	cd ${ZPREFIX}/lib;ln -s libcairo.so.11402 libcairo.so

.include <bsd.port.mk>

.endif
