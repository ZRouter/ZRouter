# Created by: David Barksdale <amatus@gnu.org>
# $FreeBSD: head/www/libmicrohttpd/Makefile 432286 2017-01-23 18:52:57Z sunpoet $

PORTNAME=	libmicrohttpd
PORTVERSION=	0.9.58
CATEGORIES=	www
MASTER_SITES=	GNU

MAINTAINER=	gaod@hychen.org
COMMENT=	C library for embedding HTTP server functionality

LICENSE=	LGPL21
LICENSE_FILE=	${WRKSRC}/COPYING

GNU_CONFIGURE=	yes
INSTALL_TARGET=	install-strip
#USES=		cpe libtool pathfix
USE_LDCONFIG=	yes

CPE_VENDOR=	gnu

#OPTIONS_DEFINE=	GNUTLS

#GNUTLS_LIB_DEPENDS=	libgcrypt.so:security/libgcrypt \
			libgnutls.so:security/gnutls
#GNUTLS_CONFIGURE_WITH=	libgcrypt-prefix=${LOCALBASE} gnutls=${LOCALBASE}
#GNUTLS_CONFIGURE_ENABLE=https

CONFIGURE_ARGS+= --disable-curl
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

#INFO=		libmicrohttpd libmicrohttpd-tutorial

PLIST_FILES=	include/microhttpd.h \
		lib/libmicrohttpd.a \
		lib/libmicrohttpd.so \
		lib/libmicrohttpd.so.12 \
		lib/libmicrohttpd.so.12.40.0 \
		libdata/pkgconfig/libmicrohttpd.pc \
		man/man3/libmicrohttpd.3.gz

post-patch:
	@${REINPLACE_CMD} -e 's|bin_PROGRAMS =|noinst_PROGRAMS +=|' \
		${WRKSRC}/src/examples/Makefile.in

post-install:
	${INSTALL_LIB} ${STAGEDIR}${PREFIX}/lib/libmicrohttpd.so.12.46.0 ${ZPREFIX}/lib/libmicrohttpd.so.12
	cp ${STAGEDIR}${PREFIX}/include/*.h ${ZPREFIX}/include/
	cd ${ZPREFIX}/lib;ln -s libmicrohttpd.so.12 libmicrohttpd.so

.include <bsd.port.mk>
