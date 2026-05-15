# Created by: Rodrigo Osorio <rodrigo@FreeBSD.org>

PORTNAME=	rtl-433
PORTVERSION=	21.05
PORTREVISION=	1
CATEGORIES=	comms hamradio

MAINTAINER=	rodrigo@FreeBSD.org
COMMENT=	Generic RF data receiver and decoder for ISM band devices

LICENSE=	GPLv2
LICENSE_FILE=	${WRKSRC}/COPYING

#LIB_DEPENDS=	librtlsdr.so:comms/rtl-sdr

#USES=		cmake

USE_GITHUB=	yes
GH_ACCOUNT=	merbanan
GH_PROJECT=	rtl_433

build:
	cd ${WRKSRC}; cmake -DCMAKE_PREFIX_PATH=${ZPREFIX} -B build -DENABLE_OPENSSL=OFF
	cd ${WRKSRC}/build;make

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/build/src/rtl_433 ${ZPREFIX}/bin/

.include <bsd.port.mk>
