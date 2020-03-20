# Created by: Bruce A. Mah <bmah@FreeBSD.org>
# $FreeBSD: head/benchmarks/iperf3/Makefile 435558 2017-03-06 17:31:09Z bmah $

PORTNAME=	iperf
PORTVERSION=	3.7
CATEGORIES=	benchmarks
MASTER_SITES=	http://downloads.es.net/pub/iperf/
PKGNAMESUFFIX=	3

MAINTAINER=	bmah@FreeBSD.org
COMMENT=	Improved tool to measure TCP and UDP bandwidth

LICENSE=	BSD3CLAUSE MIT
LICENSE_COMB=	multi

GNU_CONFIGURE=	yes
USES=		libtool
USE_LDCONFIG=	yes
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

PORTDOCS=	README.md RELEASE_NOTES

# Set ${DOCSDIR} to avoid conflict with benchmarks/iperf ${DOCSDIR}
DOCSDIR=	${PREFIX}/share/doc/${PORTNAME}${PKGNAMESUFFIX}

post-patch:
# Don't build stuff that we're not going to install.  Among other
# things this means we're not going to try to build profiled objects
# and/or executables.
	@${REINPLACE_CMD} -e '/^PROGRAMS /s/\$$(noinst_PROGRAMS)//' ${WRKSRC}/src/Makefile.in

post-configure:
	@${REINPLACE_CMD} -e 's|-I\/usr\/include||' ${WRKSRC}/Makefile
	@${REINPLACE_CMD} -e 's|-L\/usr\/lib||' ${WRKSRC}/Makefile
	@${REINPLACE_CMD} -e 's|-I\/usr\/include||' ${WRKSRC}/src/Makefile
	@${REINPLACE_CMD} -e 's|-L\/usr\/lib||' ${WRKSRC}/src/Makefile
	@${REINPLACE_CMD} -e 's|-I\/usr\/include||' ${WRKSRC}/examples/Makefile
	@${REINPLACE_CMD} -e 's|-L\/usr\/lib||' ${WRKSRC}/examples/Makefile

#post-install:
#	@${MKDIR} ${STAGEDIR}${DOCSDIR}
#	(cd ${WRKSRC} && ${INSTALL_DATA} ${PORTDOCS} ${STAGEDIR}${DOCSDIR})A

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/src/.libs/iperf3 ${ZPREFIX}/bin/iperf3
	${INSTALL_LIB} ${WRKSRC}/src/.libs/libiperf.so.0.0.0 ${ZPREFIX}/lib/libiperf.so.0
	cd ${ZPREFIX}/lib;ln -s libiperf.so.0 libiperf.so


.include <bsd.port.mk>
