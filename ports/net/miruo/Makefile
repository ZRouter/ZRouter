PORTNAME=	miruo
PORTVERSION=	0.9.6b
CATEGORIES=	net

MAINTAINER=	meta@FreeBSD.org
COMMENT=	Pretty-print TCP session monitor/analyzer

LICENSE=	GPLv3
LICENSE_FILE=	${WRKSRC}/COPYING

CONFIGURE_ARGS+= --host=${ARCH}-portbld-freebsd

USE_GITHUB=	yes
GH_ACCOUNT=	KLab

GNU_CONFIGURE=	yes
PLIST_FILES=	sbin/miruo

post-install:
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/sbin/miruo ${ZPREFIX}/sbin/

.include <bsd.port.mk>
