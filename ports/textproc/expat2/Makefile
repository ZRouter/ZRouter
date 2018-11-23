# Created by: Dirk Froemberg <dirk@FreeBSD.org>
# $FreeBSD: head/textproc/expat2/Makefile 428144 2016-12-08 17:40:41Z tijl $

PORTNAME=	expat
PORTVERSION=	2.2.0
PORTREVISION=	1
CATEGORIES=	textproc
MASTER_SITES=	SF

MAINTAINER=	kuriyama@FreeBSD.org
COMMENT=	XML 1.0 parser written in C

LICENSE=	MIT
LICENSE_FILE=	${WRKSRC}/COPYING

GNU_CONFIGURE=	yes
ALL_TARGET=	default
USES=		libtool pathfix tar:bz2
USE_LDCONFIG=	yes

CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

post-install:
	${INSTALL_LIB} ${STAGEDIR}${PREFIX}/lib/libexpat.so.1.6.2 ${ZPREFIX}/lib/libexpat.so.1
	cp ${STAGEDIR}${PREFIX}/include/*.h ${ZPREFIX}/include/
	cd ${ZPREFIX}/lib;ln -s libexpat.so.1 libexpat.so

.include <bsd.port.mk>
