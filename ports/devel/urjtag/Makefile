# Created by: Bruce M. Simpson <bms@FreeBSD.org>
# $FreeBSD: tags/RELEASE_10_3_0/devel/urjtag/Makefile 373967 2014-12-04 16:04:40Z amdmi3 $

PORTNAME=	urjtag
PORTVERSION=	0.10
PORTREVISION=	5
CATEGORIES=	devel
MASTER_SITES=	SF
#DISTNAME=	urjtag-0.10
#EXTRACT_SUFX=	.tar.bz2
DISTNAME=	urjtag
EXTRACT_SUFX=	.tgz
MAKE_JOBS_UNSAFE=yes

MAINTAINER=	ports@FreeBSD.org
COMMENT=	Extended utility to work with JTAG-aware boards

LICENSE=	GPLv2

#RUN_DEPENDS=	bash:${PORTSDIR}/shells/bash

#USES=		gmake readline shebangfix tar:bzip2
#SHEBANG_FILES=	src/bsdl2jtag
#USE_CSTD=	c99
GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--disable-bsdl --without-libintl-prefix --with-libedit --disable-python --disable-shared --without-libusb --disable-svf

# for header check
CFLAGS+=	-I${OBJ_DIR}/${OLDPWD}/tmp/${ZARCH}/${FREEBSD_SRC_TREE}/tmp/usr/include/ -I${OBJ_DIR}/${OLDPWD}/tmp/${ZARCH}/${FREEBSD_SRC_TREE}/tmp/usr/include/edit
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

do-install:
#       @printenv
	${INSTALL_PROGRAM} ${WRKSRC}/src/apps/jtag/jtag ${ZPREFIX}/bin/jtag
	${MKDIR} ${ZPREFIX}/share/urjtag
	(cd ${WRKSRC}/data && ${COPYTREE_SHARE} . ${ZPREFIX}/share/urjtag)


#.include <bsd.port.post.mk>
.include <bsd.port.mk>
