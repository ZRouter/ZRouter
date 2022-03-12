# Created by: Angel Carpintero <ack@telefonica.net>
# $FreeBSD: head/multimedia/motion/Makefile 439367 2017-04-25 08:20:30Z riggs $

PORTNAME=	motion
PORTVERSION=	4.3.2
PORTREVISION=	1
DISTVERSIONPREFIX=	release-
CATEGORIES=	multimedia

MAINTAINER=	ports@FreeBSD.org
COMMENT=	Motion detection application

LICENSE=	GPLv2+
#LICENSE_FILE=	${WRKSRC}/COPYING

USE_GITHUB=	yes
GH_ACCOUNT=	Motion-Project
USE_RC_SUBR=	${PORTNAME}

NLS=		off

#USES=		autoreconf gmake jpeg localbase pkgconfig gettext
USES=		autoreconf gmake
GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--without-v4l2 --without-bktr  --without-ffmpeg --without-sqlite3
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

CONFIGURE_ARGS+=CFLAGS=-I${ZPREFIX}/include/ LDFLAGS="-L${ZPREFIX}/lib/ -lmicrohttpd"


PLIST_FILES=	bin/motion \
		man/man1/motion.1.gz \
		"@sample ${ETCDIR}/motion.conf.sample" \
		"@dir ${ETCDIR}/conf.d"
PORTDOCS=	CHANGELOG CREDITS README.md README.FreeBSD \
		motion_guide.html mask1.png normal.jpg \
		outputmotion1.jpg outputnormal1.jpg
PORTEXAMPLES=	camera1-dist.conf camera2-dist.conf \
		camera3-dist.conf camera4-dist.conf

.include <bsd.port.pre.mk>

post-configure:
	@${REINPLACE_CMD} -e \
		's|L/usr/local/lib|L${ZPREFIX}/lib|g' ${WRKSRC}/Makefile
	@${REINPLACE_CMD} -e \
		's|-lintl||g' ${WRKSRC}/Makefile
	@${REINPLACE_CMD} -e \
		's|#define HAVE_INTL 1||g' ${WRKSRC}/config.h

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/src/${PORTNAME} ${ZPREFIX}/bin/${PORTNAME}


.include <bsd.port.post.mk>
