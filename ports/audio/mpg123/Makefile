# Created by: Carey Jones <mcj@acquiesce.org>

PORTNAME=	mpg123
PORTVERSION=	1.31.3
CATEGORIES=	audio
MASTER_SITES=	SF \
		https://www.mpg123.de/download/

MAINTAINER=	novel@FreeBSD.org
COMMENT=	Command-line player for MPEG Layer 1, 2, and 3 audio files

LICENSE=	LGPL21

USES=		tar:bzip2 libtool localbase pathfix
USE_LDCONFIG=	yes

GNU_CONFIGURE=	yes
#CONFIGURE_ARGS=	--with-optimization=0
CONFIGURE_ARGS=	--with-optimization=3
CONFIGURE_ARGS+= --host=${ARCH}-portbld-freebsd
CONFIGURE_ARGS+= --with-cpu=generic_nofpu
#CONFIGURE_ARGS+= --with-network=internal
INSTALL_TARGET=	install-strip

OPTIONS_DEFINE=		SNDIO
OPTIONS_DEFINE_i386=	OPTCPU
OPTIONS_DEFINE_armv6=	FPU
OPTIONS_DEFINE_armv7=	FPU
OPTIONS_DEFAULT_i386=	OPTCPU
OPTIONS_SUB=	yes

FPU_DESC=	Use fixed point arithmetic and/or NEON instructions
OPTCPU_DESC=	Enable support for optimized CPU instructions

TEST_TARGET=		check

FPU_CONFIGURE_ON=	--with-cpu=arm_fpu
FPU_CONFIGURE_OFF=	--with-cpu=arm_nofpu

SNDIO_CONFIGURE_ON=	--with-audio=sndio,oss
SNDIO_CONFIGURE_OFF=	--with-audio=oss
SNDIO_LIB_DEPENDS=	libsndio.so:audio/sndio

OPTCPU_CONFIGURE_ON=	--with-cpu=x86
OPTCPU_CONFIGURE_OFF=	--with-cpu=generic_fpu

.include <bsd.port.pre.mk>

.if ${ARCH} == "amd64"
#CONFIGURE_ARGS+=	--with-cpu=x86-64
.endif

post-install:
	${INSTALL_PROGRAM} ${STAGEDIR}/${PREFIX}/bin/mpg123 ${ZPREFIX}/bin/
	${INSTALL_LIB} ${STAGEDIR}/${PREFIX}/lib/libmpg123.so.0 ${ZPREFIX}/lib/
	cd ${ZPREFIX}/lib;ln -s libmpg123.so.0 libmpg123.so
	${INSTALL_LIB} ${STAGEDIR}/${PREFIX}/lib/libout123.so.0 ${ZPREFIX}/lib/
	cd ${ZPREFIX}/lib;ln -s libout123.so.0 libout123.so
	${INSTALL_LIB} ${STAGEDIR}/${PREFIX}/lib/libsyn123.so.0 ${ZPREFIX}/lib/
	cd ${ZPREFIX}/lib;ln -s libsyn123.so.0 libsyn123.so
	@${CP} -r ${STAGEDIR}/${PREFIX}/lib/mpg123 ${ZPREFIX}/lib/
	${INSTALL_DATA} ${STAGEDIR}/${PREFIX}/include/fmt123.h ${ZPREFIX}/include/
	${INSTALL_DATA} ${STAGEDIR}/${PREFIX}/include/mpg123.h ${ZPREFIX}/include/
	${INSTALL_DATA} ${STAGEDIR}/${PREFIX}/include/out123.h ${ZPREFIX}/include/
	${INSTALL_DATA} ${STAGEDIR}/${PREFIX}/include/syn123.h ${ZPREFIX}/include/

.include <bsd.port.post.mk>
