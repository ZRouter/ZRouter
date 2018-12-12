# $FreeBSD: head/devel/libftdi/Makefile 439934 2017-05-02 06:48:10Z jbeich $

PORTNAME=	libftdi
PORTVERSION=	0.20
PORTREVISION=	7
CATEGORIES=	devel
MASTER_SITES=	http://www.intra2net.com/en/developer/libftdi/download/

MAINTAINER=	ports@FreeBSD.org
COMMENT=	Library (using libusb) to talk to FTDI chips

LICENSE=	GPLv2 LGPL21
LICENSE_COMB=	multi
LICENSE_FILE_GPLv2=	${WRKSRC}/COPYING.GPL
LICENSE_FILE_LGPL21=	${WRKSRC}/COPYING.LIB

#USES=		cmake pkgconfig
#CMAKE_ARGS=	-DDOCUMENTATION:BOOL=OFF -DEXAMPLES:BOOL=OFF
#USE_LDCONFIG=	yes

#OPTIONS_DEFINE=	BOOST PYTHON
#OPTIONS_DEFAULT=BOOST PYTHON
#OPTIONS_SUB=	yes

#BOOST_DESC=		C++ bindings (requires Boost)
#BOOST_LIB_DEPENDS=	libboost_system.so:devel/boost-libs
#BOOST_CMAKE_BOOL=	FTDIPP
#BOOST_CXXFLAGS=		-I${LOCALBASE}/include

#PYTHON_DESC=		Python bindings
#PYTHON_BUILD_DEPENDS=	swig2.0:devel/swig20
#PYTHON_CMAKE_BOOL=	PYTHON_BINDINGS
#PYTHON_USES=		python

.include <bsd.port.options.mk>

post-patch:
	echo "all :" > ${WRKSRC}/Makefile
	echo "	cc -c src/ftdi.c" >> ${WRKSRC}/Makefile
	echo "	cc -fPIC -O2 -pipe  -fno-strict-aliasing -O2 -pipe  -fno-strict-aliasing  -shared -Wl,-soname,libftdi.so.1 -o libftdi.so.1.20.0 ftdi.o -lusb" >> ${WRKSRC}/Makefile

do-install:
	${INSTALL_LIB} ${WRKSRC}/libftdi.so.1.20.0 ${ZPREFIX}/lib/libftdi.so.1
	cp ${WRKSRC}/src/ftdi.h ${ZPREFIX}/include/
	cd ${ZPREFIX}/lib;ln -s libftdi.so.1 libftdi.so
.include <bsd.port.mk>
