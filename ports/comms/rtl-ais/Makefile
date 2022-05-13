# Created by: Steven Kreuzer <skreuzer@FreeBSD.org>
# $FreeBSD: tags/RELEASE_10_3_0/comms/dump1090/Makefile 385521 2015-05-06 15:16:53Z mat $

PORTNAME=	rtl-ais
PORTVERSION=	20220513
CATEGORIES=	comms

MAINTAINER=	yamori813@yahoo.co.jp
COMMENT=	a simple AIS tuner and generic dual-frequency FM demodulator

LICENSE=	GPLv2

USE_GITHUB=	yes
GH_ACCOUNT=	dgiardini
GH_TAGNAME=	09ca36c

#LIB_DEPENDS=	librtlsdr.so:${PORTSDIR}/comms/rtl-sdr

USES=	gmake

CFLAGS+=	-I${ZPREFIX}/include
LDFLAGS+=	-L${ZPREFIX}/lib -lrtlsdr -lusb -lpthread -lm

PLIST_FILES=	bin/rtl-ais

pre-build:
	@${REINPLACE_CMD} -e "s|Darwin|FreeBSD|" ${WRKSRC}/Makefile
	@${REINPLACE_CMD} -e "s|lusb-1.0|lusb|" ${WRKSRC}/Makefile

post-install:
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/bin/rtl_ais ${ZPREFIX}/bin/

.include <bsd.port.mk>
