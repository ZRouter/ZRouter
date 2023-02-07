# Created by: Martin Matuska <mm@FreeBSD.org>

PORTNAME=	fdk-aac
DISTVERSIONPREFIX=	v
DISTVERSION=	2.0.2
CATEGORIES=	audio

MAINTAINER=	bofh@FreeBSD.org
COMMENT=	Port of the Fraunhofer FDK AAC Codec Library

LICENSE=	FRAUNHOFER_FDK_AAC
LICENSE_NAME=	Software License for The Fraunhofer FDK AAC Codec Library for Android
LICENSE_FILE=	${WRKSRC}/NOTICE
LICENSE_PERMS=	dist-mirror pkg-mirror auto-accept

NOT_FOR_ARCHS=		sparc64
NOT_FOR_ARCHS_REASON=	not yet ported to this architecture

#USES=		cmake
USE_GITHUB=	yes
USE_LDCONFIG=	yes

GH_ACCOUNT=	mstorsjo

PLIST_SUB=	VER=${PORTVERSION}

GNU_CONFIGURE=	yes
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

#.if "${ZARCH}" == "mips.mips" || "${ZARCH}" == "mips.mipsel"
CONFIGURE_ARGS+=CPPFLAGS="-O3 -DMIPS_DSP_LIB"
# this option use buildin function. but gcc 4.2.1 not support it.
#CONFIGURE_ARGS+=CPPFLAGS="-D__mips_dsp"
#.endif

pre-configure:
	cd ${WRKSRC};./autogen.sh
	${CP} ${FILESDIR}/*.h ${WRKSRC}/libFDK/include/
	${CP} ${FILESDIR}/fft32.S ${WRKSRC}/libFDK/src/mips/

install:

post-install:
	${MKDIR} ${ZPREFIX}/include/fdk-aac
	${INSTALL_DATA} ${STAGEDIR}/${PREFIX}/include/fdk-aac/* ${ZPREFIX}/include/fdk-aac/
	${INSTALL_LIB} ${STAGEDIR}/${PREFIX}/lib/libfdk-aac.so.2.0.2 ${ZPREFIX}/lib/
	cd ${ZPREFIX}/lib;ln -s libfdk-aac.so.2.0.2 libfdk-aac.so.2
	cd ${ZPREFIX}/lib;ln -s libfdk-aac.so.2.0.2 libfdk-aac.so

.include <bsd.port.mk>
