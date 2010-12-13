# New ports collection makefile for:	libmicrohttpd
# Date created:		12 February 2008
# Whom:			David Barksdale <amatus@gnu.org>
#
# $FreeBSD: ports/www/libmicrohttpd/Makefile,v 1.5 2009/10/17 23:14:46 itetcu Exp $
#

PORTNAME=	libmicrohttpd
PORTVERSION=	0.4.2
CATEGORIES=	www
MASTER_SITES=	http://gnunet.org/libmicrohttpd/download/

MAINTAINER=	amatus@gnu.org
COMMENT=	C library for embedding HTTP server functionality

USE_LDCONFIG=	yes
GNU_CONFIGURE=	yes
MAKE_ARGS+=	pkgconfigdir=${PREFIX}/libdata/pkgconfig
CONFIGURE_ARGS= --without-libgcrypt --without-libcurl --disable-https

PLIST_FILES=	lib/libmicrohttpd.so.6 \
		lib/libmicrohttpd.la \
		lib/libmicrohttpd.so \
		lib/libmicrohttpd.a \
		libdata/pkgconfig/libmicrohttpd.pc \
		include/microhttpd.h \
		info/microhttpd.info

MAN3=		libmicrohttpd.3

.include <bsd.port.mk>
