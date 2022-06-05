# Created by: Sergey Lyubka (valenok@gmail.com)
# $FreeBSD: head/www/shttpd/Makefile 357486 2014-06-11 14:49:59Z tijl $

PORTNAME=	shttpd
PORTVERSION=	1.42
CATEGORIES=	www
MASTER_SITES=	SF

MAINTAINER=	valenok@gmail.com
COMMENT=	Small, fast, embeddable web server with CGI, SSL, Authorization

ALL_TARGET=	unix
LIBS+=		-lpthread
BUILD_WRKSRC=	${WRKSRC}/src

PLIST_FILES=	bin/shttpd man/man1/shttpd.1.gz
#MAKE_ARGS=	CFLAGS="-D_DEBUG"

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/src/shttpd ${ZPREFIX}/bin


.include <bsd.port.mk>
