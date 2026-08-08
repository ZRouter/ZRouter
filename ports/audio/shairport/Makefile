PORTNAME=	shairport-lp
DISTVERSION=	0.0.1
CATEGORIES=	audio

MAINTAINER=	yamori813@yahoo.co.jp
COMMENT=	AirPlay audio player

LICENSE=	BSD3CLAUSE

USES=		gmake
USE_GITHUB=	yes

GH_ACCOUNT=	yamori813
GH_TAGNAME=	a690914

pre-build:
	@${REINPLACE_CMD} -e 's|-dirty|${GH_TAGNAME}|g' \
                ${WRKSRC}/Makefile

do-build:
	cd ${BUILD_WRKSRC}; ${MAKE_CMD}

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/shairport ${ZPREFIX}/bin/

.include <bsd.port.mk>
