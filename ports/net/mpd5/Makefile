# Created by: Archie Cobbs
# $FreeBSD: head/net/mpd5/Makefile 485853 2018-11-25 12:09:27Z eugen $

PORTNAME=	mpd
DISTVERSION=	5.9
PORTREVISION=	10
CATEGORIES=	net
MASTER_SITES=	SF/${PORTNAME}/Mpd5/Mpd-${PORTVERSION}
PKGNAMESUFFIX=	5
DIST_SUBDIR=	mpd5

MAINTAINER=	eugen@FreeBSD.org
COMMENT=	Multi-link PPP daemon based on netgraph(4)

LICENSE=	BSD3CLAUSE

#OPTIONS_DEFINE=	DOCS NG_IPACCT
NG_IPACCT_DESC=	Use ng_ipacct kernel module from port

HAS_CONFIGURE=	YES
CONFIGURE_WRKSRC=	"${WRKSRC}/src"

USES=		ssl

.include <bsd.port.options.mk>

MAKE_ARGS+=	OSVERSION=${OSVERSION}

.if ${PORT_OPTIONS:MNG_IPACCT}
BUILD_DEPENDS+=	${LOCALBASE}/include/netgraph/ng_ipacct.h:net-mgmt/ng_ipacct
.if exists(${KMODDIR}/ng_ipacct.ko)
RUN_DEPENDS+=	${KMODDIR}/ng_ipacct.ko:net-mgmt/ng_ipacct
.else
RUN_DEPENDS+=	/boot/modules/ng_ipacct.ko:net-mgmt/ng_ipacct
.endif
.else
CONFIGURE_ENV+=	USE_NG_IPACCT=no
MAKE_ARGS+=	USE_NG_IPACCT=no
.endif

USE_RC_SUBR=	mpd5
CONFSUF=	conf secret script
DOCSDIR=	${PREFIX}/share/doc/mpd5
PORTDOCS=	README mpd*
PLIST_FILES=	${CONFSUF:S/^/etc\/mpd5\/mpd./:S/$/.sample/} sbin/mpd5

.if !defined(WITHOUT_MAN)
PLIST_FILES+=	man/man8/mpd5.8.gz
.endif

post-configure:
	@${REINPLACE_CMD} -e 's|-L/usr/lib||g' ${WRKSRC}/src/Makefile

post-install:
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/sbin/mpd5 ${ZPREFIX}/sbin/mpd5

.include <bsd.port.mk>
