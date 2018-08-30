# Created by: Thomas Spreng <spreng@socket.ch>
# $FreeBSD: head/net-mgmt/kismet/Makefile 417965 2016-07-03 10:38:12Z pi $

PORTNAME=	kismet
DISTVERSION=	2016-07-R1
PORTEPOCH=	1
CATEGORIES=	net-mgmt
MASTER_SITES=	http://www.kismetwireless.net/code/

MAINTAINER=	pi@FreeBSD.org
COMMENT=	802.11 layer2 wireless network detector, sniffer, and IDS

LICENSE=	GPLv2

GNU_CONFIGURE=	yes
USES=		gmake tar:xz
CPPFLAGS+=	-I${LOCALBASE}/include
LDFLAGS+=	-L${LOCALBASE}/lib
CONFIGURE_ARGS=	--disable-pcre

SETUID_DESC=		Use privilege separation

WRKSRC=	${WRKDIR}/${DISTNAME:S/a$$//}

post-patch:
	@${REINPLACE_CMD} -e "/gps=/s/true/false/" \
		-e "/ouifile=/{/wireshark/d;s|/etc|${DOCSDIR}|;}" \
			${WRKSRC}/conf/kismet.conf.in \
			${WRKSRC}/conf/kismet_drone.conf
	@${REINPLACE_CMD} -e "s/plugin-/plugin-[aps]/" \
		-e "s/make/${MAKE_CMD}/" \
		-e "s/\$$(MAKE)/${MAKE_CMD}/" \
		${WRKSRC}/Makefile.in
	@${REINPLACE_CMD} -e "s/CXXFLAGS/CPPFLAGS/" \
		${WRKSRC}/plugin-alertsyslog/Makefile
	@${FIND} ${WRKSRC} -type f -name 'Makefile' -print0 | ${XARGS} -0 \
		${REINPLACE_CMD} -E \
		'/C(XX)?FLAGS[[:blank:]]*\+=/s/ -g / /'
	@${FIND} ${WRKSRC} -type f \( -name Makefile -o -name Makefile.in \) -print0 | ${XARGS} -0 \
		${REINPLACE_CMD} -e 's,-o $$(INSTUSR) -g $$(INSTGRP),,' \
		-e 's,-o $$(INSTUSR) -g $$(MANGRP),,' \
		-e 's,-o $$(INSTUSR) -g $$(SUIDGROUP) -m 4550,,'

post-configure:
	@${REINPLACE_CMD} -e 's|-L/usr/local/lib|-L${ZPREFIX}/lib|' \
		${WRKSRC}/Makefile.inc

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/kismet_client ${ZPREFIX}/bin/kismet_client
	${INSTALL_PROGRAM} ${WRKSRC}/kismet_drone ${ZPREFIX}/bin/kismet_drone
	${INSTALL_PROGRAM} ${WRKSRC}/kismet_server ${ZPREFIX}/bin/kismet_server
	${INSTALL_PROGRAM} ${WRKSRC}/kismet_capture ${ZPREFIX}/bin/kismet_capture

.include <bsd.port.mk>
