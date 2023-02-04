# Created by: Sean McGovern <sean@sfarc.net>

PORTNAME=	faad2
PORTVERSION=	2.10.0
PORTEPOCH=	1
CATEGORIES=	audio

MAINTAINER=	multimedia@FreeBSD.org
COMMENT=	MPEG-2 and MPEG-4 AAC audio decoder

LICENSE=	GPLv2+
LICENSE_FILE=	${WRKSRC}/COPYING

USES=		autoreconf cpe gmake libtool localbase:ldflags
CPE_VENDOR=	audiocoding
CPE_PRODUCT=	freeware_advanced_audio_decoder_2

INSTALL_TARGET=	install-strip

GNU_CONFIGURE=	yes
LDFLAGS+=	-lpthread
USE_LDCONFIG=	yes

USE_GITHUB=	yes
GH_ACCOUNT=	knik0
GH_TAGNAME=	c626366

CONFIGURE_ARGS+= --host=${ARCH}-portbld-freebsd
CONFIGURE_ARGS+= CFLAGS=-DFIXED_POINT

post-extract:
	@${LN} -sf faad.man ${WRKSRC}/frontend/faad.1

post-patch:
	@${REINPLACE_CMD} -e 's|faad.man|faad.1|g' \
		${WRKSRC}/frontend/Makefile.am
	@${REINPLACE_CMD} -e 's|^AM_CFLAGS|AM_CPPFLAGS|g ; \
		 s|-iquote |-I|g' ${WRKSRC}/libfaad/Makefile.am
	@${REINPLACE_CMD} -E -e 's|(FAAD2_VERSION +)"unknown"|\1"${PORTVERSION:R}"|' \
		${WRKSRC}/include/neaacdec.h

install:

post-install:
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/bin/faad ${ZPREFIX}/bin/
	${INSTALL_LIB} ${STAGEDIR}/${PREFIX}/lib/libfaad.so.2.0.0 ${ZPREFIX}/lib/
	cd ${ZPREFIX}/lib;ln -s libfaad.so.2.0.0 libfaad.so
	cd ${ZPREFIX}/lib;ln -s libfaad.so.2.0.0 libfaad.so.2

.include <bsd.port.mk>
