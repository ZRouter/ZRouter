# Created by: Ganael Laplanche <ganael.laplanche@martymac.org>
# $FreeBSD: head/comms/rtl-sdr/Makefile 433346 2017-02-05 03:30:43Z jbeich $

PORTNAME=	rtl-sdr
PORTVERSION=	0.6.0
PORTREVISION=	3
PORTEPOCH=	1
CATEGORIES=	comms hamradio

MAINTAINER=	martymac@FreeBSD.org
COMMENT=	Commandline tool that performs SDR with RTL2832-based USB sticks

LICENSE=	GPLv2

#USES=		autoreconf gmake pkgconfig libtool pathfix tar:tgz
USES=		autoreconf

USE_GITHUB=	yes
GH_ACCOUNT=	steve-m
GH_PROJECT=	librtlsdr

GNU_CONFIGURE=	yes
#INSTALL_TARGET=	install-strip
#CPPFLAGS+=	-I${LOCALBASE}/include
#LDFLAGS+=	-L${LOCALBASE}/lib
#USE_LDCONFIG=	yes
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

#DOCSDIR=	${PREFIX}/share/doc/librtlsdr

#OPTIONS_DEFINE=	DOXYGEN
#OPTIONS_SUB=	yes

#DOXYGEN_BUILD_DEPENDS=	doxygen:devel/doxygen
DOXYGEN_EXTRA_PATCHES_OFF=	${FILESDIR}/extra-patch-Makefile.am

post-install:
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/bin/rtl_sdr ${ZPREFIX}/bin/
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/bin/rtl_fm ${ZPREFIX}/bin/
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/bin/rtl_tcp ${ZPREFIX}/bin/
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/bin/rtl_power ${ZPREFIX}/bin/
	${INSTALL_LIB} ${STAGEDIR}${PREFIX}/lib/librtlsdr.so.0.0.5 ${ZPREFIX}/lib/librtlsdr.so.0
	cp ${STAGEDIR}${PREFIX}/include/*.h ${ZPREFIX}/include/
	cd ${ZPREFIX}/lib;ln -s librtlsdr.so.0 librtlsdr.so

.include <bsd.port.mk>
