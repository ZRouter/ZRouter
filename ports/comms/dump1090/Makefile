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
DATADIR=	/usr/local/share

CFLAGS+=	-I${LOCALBASE}/include
LDFLAGS+=	-L${LOCALBASE}/lib -lrtlsdr -lusb -lpthread -lm

PLIST_FILES=	bin/dump1090 \
		${DATADIR}/gmap.html

post-patch:
	@${REINPLACE_CMD} -e "s|gmap.html|${DATADIR}/gmap.html|" ${WRKSRC}/dump1090.c

pre-build:
	@${REINPLACE_CMD} -e "s|CenterLat=45.0|CenterLat=35.65|" ${WRKSRC}/gmap.html
	@${REINPLACE_CMD} -e "s|CenterLon=9.0|CenterLon=139.69|" ${WRKSRC}/gmap.html
	@${REINPLACE_CMD} -e "s|lrtlsdr|lrtlsdr -L${ZWORKROOT}/target/lib/librtlsdr -lusb|" ${WRKSRC}/Makefile

#do-install:
#	${INSTALL_PROGRAM} ${WRKSRC}/${PORTNAME} ${STAGEDIR}${PREFIX}/bin
#	${MKDIR} ${STAGEDIR}${DATADIR}
#	${INSTALL_DATA} ${WRKSRC}/gmap.html ${STAGEDIR}${DATADIR}

do-install:
#       @printenv
	${INSTALL_PROGRAM} ${WRKSRC}/${PORTNAME} ${ZPREFIX}/bin/${PORTNAME}
	mkdir ${ZPREFIX}/share
	${INSTALL_DATA} ${WRKSRC}/gmap.html ${ZPREFIX}/share/gmap.html


.include <bsd.port.mk>