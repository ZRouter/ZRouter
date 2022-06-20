PORTNAME=	uhubctl
PORTVERSION=	2.4.0
DISTVERSIONPREFIX=      v
CATEGORIES=	comms

MAINTAINER=	yamori813@yahoo.co.jp
COMMENT=	USB hub per-port power control 

LICENSE=	GPLv2
#LICENSE_FILE=	${WRKSRC}/COPYING

USE_GITHUB=	yes
GH_ACCOUNT=	mvp

USES=		gmake

ALL_TARGET=	${PORTNAME}

do-install:
	${INSTALL_PROGRAM} ${WRKDIR}/${PORTNAME}-${PORTVERSION}/uhubctl ${ZPREFIX}/sbin/

.include <bsd.port.mk>
