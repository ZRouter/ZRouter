PORTNAME=	openiked
PORTVERSION=	6.9.0
CATEGORIES=	security net
MASTER_SITES=	OPENBSD/OpenIKED
PKGNAMESUFFIX=	-portable
PORTREVISION=	1

MAINTAINER=	david@lapinbilly.eu
COMMENT=	IKEv2 daemon

LICENSE=	ISCL

#LIB_DEPENDS=	libevent.so:devel/libevent

CONFLICTS_INSTALL=	openiked-[0-9]*
#USES=			cmake ssl

#USE_RC_SUBR=	iked
#USERS=		_iked
#GROUPS=		_iked

# workaround "make[3]: cannot open Makefile." at install
NO_INSTALL=	yes

.include <bsd.port.pre.mk>

.if ${OSREL:R} < 12 && ${SSL_DEFAULT} == "base"
BROKEN=	requires OpenSSL 1.1.1, upgrade to FreeBSD 12.x/13.x or add DEFAULT_VERSIONS+=ssl=[openssl|libressl*] to /etc/make.conf
.endif

post-patch:
	@${REINPLACE_CMD} -e 's|/usr/local/include|${ZPREFIX}/include|' \
		${WRKSRC}/CMakeLists.txt
	@${REINPLACE_CMD} -e 's|/usr/local/lib|${ZPREFIX}/lib|' \
		${WRKSRC}/CMakeLists.txt

build:
	mkdir ${WRKSRC}/build
	cd ${WRKSRC}/build;cmake ..
	cd ${WRKSRC}/build;make
	${INSTALL_PROGRAM} ${WRKSRC}/build/iked/iked ${ZPREFIX}/sbin
	${INSTALL_PROGRAM} ${WRKSRC}/build/ikectl/ikectl ${ZPREFIX}/sbin

post-install:
#	${MV} ${STAGEDIR}${PREFIX}/etc/iked.conf \
#	    ${STAGEDIR}${PREFIX}/etc/iked.conf.sample

.include <bsd.port.post.mk>
