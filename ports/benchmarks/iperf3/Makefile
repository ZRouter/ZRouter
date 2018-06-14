# Created by: Bruce A. Mah <bmah@FreeBSD.org>
# $FreeBSD: head/benchmarks/iperf3/Makefile 435558 2017-03-06 17:31:09Z bmah $

PORTNAME=	iperf
PORTVERSION=	3.1.7
CATEGORIES=	benchmarks ipv6
MASTER_SITES=	http://downloads.es.net/pub/iperf/
PKGNAMESUFFIX=	3

MAINTAINER=	bmah@FreeBSD.org
COMMENT=	Improved tool to measure TCP and UDP bandwidth

LICENSE=	BSD3CLAUSE MIT
LICENSE_COMB=	multi

GNU_CONFIGURE=	yes
USES=		libtool
USE_LDCONFIG=	yes

PORTDOCS=	README.md RELEASE_NOTES

# Set ${DOCSDIR} to avoid conflict with benchmarks/iperf ${DOCSDIR}
DOCSDIR=	${PREFIX}/share/doc/${PORTNAME}${PKGNAMESUFFIX}

post-patch:
# Don't build stuff that we're not going to install.  Among other
# things this means we're not going to try to build profiled objects
# and/or executables.
	@${REINPLACE_CMD} -e '/^PROGRAMS /s/\$$(noinst_PROGRAMS)//' ${WRKSRC}/src/Makefile.in

#post-install:
#	@${MKDIR} ${STAGEDIR}${DOCSDIR}
#	(cd ${WRKSRC} && ${INSTALL_DATA} ${PORTDOCS} ${STAGEDIR}${DOCSDIR})A

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/src/.libs/iperf3 ${ZPREFIX}/sbin/iperf3
	mkdir -p ${ZPREFIX}/lib
	${INSTALL_LIB} ${WRKSRC}/src/.libs/libiperf.so.0.0.0 ${ZPREFIX}/lib/libiperf.so.0
	cd ${ZPREFIX}/lib;ln -s libiperf.so.0 libiperf.so


.include <bsd.port.mk>
