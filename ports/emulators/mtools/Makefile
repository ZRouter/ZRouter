# Created by: jmz

PORTNAME=	mtools
PORTVERSION=	4.0.26
PORTREVISION=	1
CATEGORIES=	emulators
MASTER_SITES=	GNU

MAINTAINER=	skreuzer@FreeBSD.org
COMMENT=	Collection of tools for manipulating MS-DOS files

LICENSE=	GPLv3

CONFLICTS_INSTALL=	multimarkdown

#USES=		gmake makeinfo tar:bzip2
USES=		gmake tar:bzip2
GNU_CONFIGURE=	yes
CONFIGURE_ENV=	ac_cv_header_malloc_h=no
CONFIGURE_ARGS=	--enable-xdf
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

SUB_FILES=	pkg-message

#INFO=		mtools

#OPTIONS_DEFINE=	ICONV X11
#OPTIONS_DEFAULT=ICONV X11

.include <bsd.port.options.mk>

.if ${PORT_OPTIONS:MICONV}
USES+=		iconv
CPPFLAGS+=	-I${LOCALBASE}/include
LDFLAGS+=	-L${LOCALBASE}/lib ${ICONV_LIB}
LIB_DEPENDS+=	libiconv.so:converters/libiconv
.else
CONFIGURE_ENV+=	ac_cv_header_iconv_h=no
.endif

.if ${PORT_OPTIONS:MX11}
USES+=		xorg
USE_XORG=	x11 sm ice xau
PLIST_SUB+=	X11=""
.else
CONFIGURE_ARGS+=	--without-x
PLIST_SUB+=	X11="@comment "
MAKE_ENV+=	WITHOUT_X11=yes
.endif

post-patch:
	@${REINPLACE_CMD} -e \
		's|^#define MT_SIZE_T size_t|#define MT_SIZE_T off_t|g' \
		${WRKSRC}/llong.h
	@${REINPLACE_CMD} -e 's|/etc/mtools.conf|${PREFIX}/etc/mtools.conf|g' \
		${WRKSRC}/mformat.c
	@${REINPLACE_CMD} -e 's|^\(SAMPLE FILE\)$$|#\1|' \
		${WRKSRC}/mtools.conf
.for f in mtools.5 mtools.texi
	@${REINPLACE_CMD} -e 's|/usr/local/etc/|${PREFIX}/etc/|g' \
		${WRKSRC}/${f}
.endfor

# delete install-info terget
post-configure:
	@${REINPLACE_CMD} -e \
		's| install-info||' \
		${WRKSRC}/Makefile

post-install:
#	${INSTALL_DATA} ${WRKSRC}/mtools.conf ${STAGEDIR}${PREFIX}/etc/mtools.conf.sample
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/bin/mtools ${ZPREFIX}/bin/
	cd ${ZPREFIX}/bin/;ln -s mtools mattrib
	cd ${ZPREFIX}/bin/;ln -s mtools mbadblocks
	cd ${ZPREFIX}/bin/;ln -s mtools mcat
	cd ${ZPREFIX}/bin/;ln -s mtools mcd
	cd ${ZPREFIX}/bin/;ln -s mtools mclasserase
	cd ${ZPREFIX}/bin/;ln -s mtools mcopy
	cd ${ZPREFIX}/bin/;ln -s mtools mdel
	cd ${ZPREFIX}/bin/;ln -s mtools mdeltree
	cd ${ZPREFIX}/bin/;ln -s mtools mdir
	cd ${ZPREFIX}/bin/;ln -s mtools mdu
	cd ${ZPREFIX}/bin/;ln -s mtools mformat
	cd ${ZPREFIX}/bin/;ln -s mtools minfo
	cd ${ZPREFIX}/bin/;ln -s mtools mlabel
	cd ${ZPREFIX}/bin/;ln -s mtools mmd
	cd ${ZPREFIX}/bin/;ln -s mtools mmount
	cd ${ZPREFIX}/bin/;ln -s mtools mmove
	cd ${ZPREFIX}/bin/;ln -s mtools mpartition
	cd ${ZPREFIX}/bin/;ln -s mtools mrd
	cd ${ZPREFIX}/bin/;ln -s mtools mren
	cd ${ZPREFIX}/bin/;ln -s mtools mshortname
	cd ${ZPREFIX}/bin/;ln -s mtools mshowfat
	cd ${ZPREFIX}/bin/;ln -s mtools mtoolstest
	cd ${ZPREFIX}/bin/;ln -s mtools mtype
	cd ${ZPREFIX}/bin/;ln -s mtools mzip

.include <bsd.port.mk>
