# Created by: Jeremie Le Hen <jlh@FreeBSD.org>

PORTNAME=	bsdrcmds
PORTVERSION=	20171003.01
CATEGORIES=	net

USE_GITHUB=	yes
GH_ACCOUNT=	jlehen

MAINTAINER=	jlh@FreeBSD.org
COMMENT=	FreeBSD rcmds (rcp/rlogin/rlogind/rsh/rshd)

LICENSE=	BSD3CLAUSE

#USES=		uidfix

MAKE_ARGS+=		BINOWN=${BINOWN} BINMODE=${BINMODE}

#OPTIONS_DEFINE=		IPV6 LIBBLACKLIST
#OPTIONS_DEFAULT=	LIBBLACKLIST
#.if !exists(/usr/include/blacklist.h)
#OPTIONS_EXCLUDE=	LIBBLACKLIST
#.endif

#IPV6_MAKE_ARGS=		USE_IPV6=yes

LIBBLACKLIST_DESC=	libblacklist(3) support for rlogind(8) and rshd(8)
LIBBLACKLIST_MAKE_ARGS=	USE_LIBBLACKLIST=yes

.include <bsd.port.options.mk>

.if ${OPSYS} == FreeBSD && ${OSVERSION} >= 1300134
MAKE_ENV+=		MK_WERROR=no
.else
MAKE_ENV+=		NO_WERROR=1
.endif

post-install:
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/bin/rsh ${ZPREFIX}/bin/rsh
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/bin/rlogin ${ZPREFIX}/bin/rlogin
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/bin/rcp ${ZPREFIX}/bin/rcp
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/sbin/rlogind ${ZPREFIX}/sbin/rlogind
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/sbin/rshd ${ZPREFIX}/sbin/rshd

.include <bsd.port.mk>
