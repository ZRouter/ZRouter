# Created by: asami

PORTNAME=	igmpproxy
DISTVERSION=	0.3
PORTEPOCH=	1
CATEGORIES=	net

MAINTAINER=	franco@opnsense.org
COMMENT=	Multicast forwarding IGMP proxy

LICENSE=	GPLv2+
LICENSE_FILE=	${WRKSRC}/COPYING

USES=		autoreconf
USE_GITHUB=	yes
GH_ACCOUNT=	pali
GNU_CONFIGURE=	yes
USE_RC_SUBR=	igmpproxy

CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

post-install:
#	${INSTALL_DATA} ${WRKSRC}/igmpproxy.conf \
#	    ${STAGEDIR}${PREFIX}/etc/igmpproxy.conf.sample
        ${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/sbin/igmpproxy ${ZPREFIX}/sbin

.include <bsd.port.mk>
