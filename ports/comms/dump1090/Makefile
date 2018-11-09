# Created by: Steven Kreuzer <skreuzer@FreeBSD.org>
# $FreeBSD: tags/RELEASE_10_3_0/comms/dump1090/Makefile 385521 2015-05-06 15:16:53Z mat $

PORTNAME=	dump1090
PORTVERSION=	20150225
CATEGORIES=	comms

MAINTAINER=	skreuzer@FreeBSD.org
COMMENT=	Simple Mode S decoder for RTLSDR devices

LICENSE=	BSD3CLAUSE

USE_GITHUB=	yes
GH_ACCOUNT=	antirez
GH_TAGNAME=	4c53e25

#LIB_DEPENDS=	librtlsdr.so:${PORTSDIR}/comms/rtl-sdr

#USES=	gmake

CFLAGS+=	-I${ZPREFIX}/include
LDFLAGS+=	-L${ZPREFIX}/lib -lrtlsdr -lusb -lpthread -lm

PLIST_FILES=	bin/dump1090 \
		${DATADIR}/gmap.html

pre-build:
	@${REINPLACE_CMD} -e "s|lrtlsdr|lrtlsdr -L${ZPREFIX}/lib -lusb|" ${WRKSRC}/Makefile

#do-install:
#	${INSTALL_PROGRAM} ${WRKSRC}/${PORTNAME} ${STAGEDIR}${PREFIX}/bin
#	${MKDIR} ${STAGEDIR}${DATADIR}
#	${INSTALL_DATA} ${WRKSRC}/gmap.html ${STAGEDIR}${DATADIR}

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/${PORTNAME} ${ZPREFIX}/bin/${PORTNAME}
	mkdir -p ${ZPREFIX}/share
	${INSTALL_DATA} ${WRKSRC}/gmap.html ${ZPREFIX}/share/gmap.html

.include <bsd.port.mk>
