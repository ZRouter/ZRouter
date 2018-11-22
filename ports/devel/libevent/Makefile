# Created by: Martin Matuska <mm@FreeBSD.org>
# $FreeBSD: head/devel/libevent/Makefile 478104 2018-08-26 07:56:39Z delphij $

PORTNAME=	libevent
PORTVERSION=	2.1.8
PORTREVISION=	2
DISTVERSIONPREFIX=	release-
DISTVERSIONSUFFIX=	-stable
CATEGORIES=	devel

MAINTAINER=	mm@FreeBSD.org
COMMENT=	API for executing callback functions on events or timeouts

LICENSE=	BSD3CLAUSE
LICENSE_FILE=	${WRKSRC}/LICENSE

USES=		autoreconf libtool pathfix python:2.7,env shebangfix
SHEBANG_FILES=	event_rpcgen.py
USE_GITHUB=	yes
GNU_CONFIGURE=	yes
INSTALL_TARGET=	install-strip
TEST_TARGET=	check
USE_LDCONFIG=	yes
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

#OPTIONS_DEFINE=	OPENSSL THREADS
#OPTIONS_DEFAULT=	OPENSSL THREADS
#OPTIONS_SUB=	yes

#OPENSSL_USES=	ssl
#OPENSSL_CPPFLAGS=	-I${OPENSSLINC}
#OPENSSL_LDFLAGS=	-L${OPENSSLLIB}
#OPENSSL_CONFIGURE_ENABLE=	openssl

THREADS_CONFIGURE_ENABLE=	thread-support

post-install:
		${INSTALL_LIB} ${STAGEDIR}${PREFIX}/lib/libevent-2.1.so.6.0.2 ${ZPREFIX}/lib/libevent-2.1.so.6
		cp ${STAGEDIR}${PREFIX}/include/*.h ${ZPREFIX}/include/
		cp -r ${STAGEDIR}${PREFIX}/include/event2 ${ZPREFIX}/include/
		cd ${ZPREFIX}/lib;ln -s libevent-2.1.so.6 libevent.so

.include <bsd.port.mk>
