PORTNAME=	zhttpd
PORTVERSION=	11
CATEGORIES=	www
MASTER_SITES=	http://files.petri.cc/zhttpd/
DISTNAME=	zhttpd-release-${PORTVERSION}
EXTRACT_SUFX=	.tgz

MAINTAINER=	npp@petri.cc
COMMENT=	A simple HTTP server for embedded devices


PLIST_FILES=	sbin/zhttpd

LIB_DEPENDS+=    microhttpd.6:${PORTSDIR}/www/libmicrohttpd
LIB_DEPENDS+=    json.0:${PORTSDIR}/devel/json-c

.include <bsd.port.mk>
