# Created by: Shinsuke Matsui <smatsui@karashi.org>
# $FreeBSD: head/comms/jerm/Makefile 394778 2015-08-19 13:29:59Z mat $

PORTNAME=	jerm
PORTVERSION=	8096
CATEGORIES=	comms
MASTER_SITES=	http://www.bsddiary.net/jerm/

MAINTAINER=	smatsui@karashi.org
COMMENT=	Communication terminal through serial and TCP/IP interfaces

#OPTIONS_DEFINE=	DOCS JUPITER

#JUPITER_DESC=	With Jupiter support

#OPTIONS_SUB=	yes

#PORTDOCS=	README

#JUPITER_MAKE_ARGS_OFF=	JUPITER_FLAGS=

post-patch:
	@${REINPLACE_CMD} -e 's|^CFLAGS=|CFLAGS?=|' \
		${WRKSRC}/Makefile
	@${REINPLACE_CMD} -e 's|sys/termios.h|termios.h|' \
		${WRKSRC}/tiocdtr.c

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/jerm ${ZPREFIX}/bin
	${INSTALL_PROGRAM} ${WRKSRC}/tiocdtr ${ZPREFIX}/bin 

.include <bsd.port.mk>
