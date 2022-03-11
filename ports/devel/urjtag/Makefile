# Created by: Bruce M. Simpson <bms@FreeBSD.org>
# $FreeBSD: tags/RELEASE_10_3_0/devel/urjtag/Makefile 373967 2014-12-04 16:04:40Z amdmi3 $

PORTNAME=	urjtag
PORTVERSION=	2018.09
PORTREVISION=	1
CATEGORIES=	devel
MASTER_SITES=	SF

EXTRACT_SUFX=	.tar.xz

MAINTAINER=	ports@FreeBSD.org
COMMENT=	Extended utility to work with JTAG-aware boards

LICENSE=	GPLv2

#RUN_DEPENDS=	bash:${PORTSDIR}/shells/bash

#USES=		gmake readline shebangfix tar:bzip2
#SHEBANG_FILES=	src/bsdl2jtag
#USE_CSTD=	c99
GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--disable-bsdl --without-libintl-prefix --without-readline --disable-python --disable-shared --without-libusb --disable-svf
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

# for header check
#CFLAGS+=	-I${LOCALBASE}/include
#LDFLAGS+=	-L${LOCALBASE}/lib
#LDFLAGS+=	-L${LOCALBASE}/lib

#OPTIONS_DEFINE=		FTDI JEDEC NLS USB
#OPTIONS_DEFAULT=	FTDI USB
#OPTIONS_SUB=		yes

#FTDI_DESC=		FTDI-based USB JTAG adapters support
#FTDI_LIB_DEPENDS=	libftdi.so:${PORTSDIR}/devel/libftdi
#FTDI_CONFIGURE_WITH=	libftdi

#JEDEC_DESC=		Experimental JEDEC flash detection
#JEDEC_CONFIGURE_ENABLE=	jedec

#NLS_USES=		gettext
#NLS_CONFIGURE_ENABLE=	nls

#USB_DESC=		USB JTAG adapters support
#USB_CONFIGURE_ON=	--with-libusb=/usr

#.include <bsd.port.pre.mk>

#.if ${OPSYS} == FreeBSD && ${OSVERSION} < 1000033
#BUILD_DEPENDS+=	flex>0:${PORTSDIR}/textproc/flex
#CONFIGURE_ENV+=	LEX=${LOCALBASE}/bin/flex
#.endif

post-patch:
	@printenv
# Suppress svn(1) binary detection
#	@${REINPLACE_CMD} -e \ #
#		's|^SVN=.*|SVN=''| ; \ #
#		 s|-Lwith_libusb |-L$$with_libusb/lib | ; \ #
#		 s|-I$$with_libusb"|-I$$with_libusb/include"|' \ #
#		${WRKSRC}/configure

post-install:
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/bin/jtag ${ZPREFIX}/bin/jtag
	${MKDIR} ${ZPREFIX}/share/urjtag
	(cd ${STAGEDIR}${PREFIX}/share/urjtag && cp -r * ${ZPREFIX}/share/urjtag)


#.include <bsd.port.post.mk>
.include <bsd.port.mk>
