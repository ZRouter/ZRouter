# Created by: Martin Matuska <mm@FreeBSD.org>

PORTNAME=	trinityenabler 
DISTVERSION=	0.1-1
CATEGORIES=	audio

MAINTAINER=	yamori813@yahoo.co.jp
COMMENT=	macOS userland driver for Apple Pro Speakers

LICENSE=	MIT

USE_GITHUB=	yes

GH_ACCOUNT=	yamori813

PLIST_SUB=	VER=${PORTVERSION}

WRKSRC=         ${WRKDIR}/TrinityEnabler-${DISTVERSION}

USES=		gmake

#pre-configure:
#	cd ${WRKSRC};./autogen.sh
#	${CP} ${FILESDIR}/*.h ${WRKSRC}/libFDK/include/
#	${CP} ${FILESDIR}/fft32.S ${WRKSRC}/libFDK/src/mips/

build:
	cd ${WRKSRC};gmake

do-install:
	${INSTALL} ${WRKSRC}/bin/trinityenabler ${ZPREFIX}/bin/

post-install:
#	${MKDIR} ${ZPREFIX}/include/fdk-aac
#	${INSTALL_DATA} ${STAGEDIR}/${PREFIX}/include/fdk-aac/* ${ZPREFIX}/include/fdk-aac/
#	${INSTALL_LIB} ${STAGEDIR}/${PREFIX}/lib/libfdk-aac.so.2.0.2 ${ZPREFIX}/lib/
#	cd ${ZPREFIX}/lib;ln -s libfdk-aac.so.2.0.2 libfdk-aac.so.2
#	cd ${ZPREFIX}/lib;ln -s libfdk-aac.so.2.0.2 libfdk-aac.so

.include <bsd.port.mk>
