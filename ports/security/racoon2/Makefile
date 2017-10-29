# Created by: sumikawa
# $FreeBSD: tags/RELEASE_10_3_0/security/racoon2/Makefile 403913 2015-12-17 17:19:48Z mat $

PORTNAME=	racoon2
PORTVERSION=	20100526a
PORTREVISION=	7
CATEGORIES=	security net ipv6
MASTER_SITES=	ftp://ftp.racoon2.wide.ad.jp/pub/racoon2/
EXTRACT_SUFX=	.tgz

MAINTAINER= 	cy@FreeBSD.org
COMMENT=	Racoon2 IPsec daemon

#USES=		perl5 tar:tgz
USE_RC_SUBR=	YES
USE_OPENSSL=	YES

USE_AUTOTOOLS=	autoconf
GNU_CONFIGURE=	yes
CONFIGURE_ARGS+=	--prefix=${STAGEDIR}${PREFIX}
CONFIGURE_ARGS+=	--sysconfdir=${PREFIX}/etc/racoon2
CONFIGURE_ENV=	YACC=/usr/bin/yacc


#OPTIONS_DEFINE=	KINK DOCS
#KINK_DESC=	Enable KINK support

.include <bsd.port.pre.mk>

PLIST_SUB+=	PFX=${PREFIX}

pre-configure:
.if ${PORT_OPTIONS:MKINK}
.if !exists(/usr/lib/libkrb5.a)
	@${ECHO_MSG} 'Required Kerberos5 is not in this system.  Disable KINK support.'
CONFIGURE_ARGS+=	--disable-kinkd
PLIST_SUB+=	KINK="@comment "
.else
CONFIGURE_ARGS+=	--enable-kinkd
MAKE_ENV=	WITH_KINK=yes
PLIST_SUB+=	KINK=""
.endif
.else
CONFIGURE_ARGS+=	--disable-kinkd
PLIST_SUB+=	KINK="@comment "
.endif

.if ${PORT_OPTIONS:MDOCS}
PORTDOCS+=	INSTALL USAGE config-usage.ja.txt config-usage.txt
PORTDOCS+=	iked-memo.ja.txt libracoon.ja.txt specification.ja.txt
PORTDOCS+=	spmif.txt style.txt system-message.ja.txt
.if ${PORT_OPTIONS:MKINK}
PORTDOCS+=	kinkd-data-struct.obj kinkd-impl.ja.txt
PORTDOCS+=	kinkd-install.ja.txt kinkd-state-txn.obj
.endif
.endif

post-patch-DOCS-on:
.for FILE in ${PORTDOCS}
	@${REINPLACE_CMD} -e 's|/usr/local/racoon2|${PREFIX}|' ${WRKSRC}/doc/${FILE}
.endfor

post-patch:
	@printenv

post-configure:
	@${REINPLACE_CMD} -e 's|-L/usr/lib||g' ${WRKSRC}/iked/Makefile
	@${REINPLACE_CMD} -e 's|-L/usr/lib||g' ${WRKSRC}/lib/Makefile
	@${REINPLACE_CMD} -e 's|-L/usr/lib||g' ${WRKSRC}/spmd/Makefile
	@${FIND} ${WRKSRC} -name Makefile | ${XARGS} ${REINPLACE_CMD} -e '/INSTALL/s|\$$([a-z].*dir)|${STAGEDIR}&|'
	@${REINPLACE_CMD} -e '/INSTALL/s|\$$(prefix)|${STAGEDIR}&|' ${WRKSRC}/pskgen/Makefile
	@${REINPLACE_CMD} -e '/^prefix=/s|/usr/local|${STAGEDIR}${PREFIX}|;/\/var\/run\/racoon2/d' ${WRKSRC}/samples/Makefile

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/iked/iked ${ZPREFIX}/sbin/iked
	${INSTALL_PROGRAM} ${WRKSRC}/spmd/spmd ${ZPREFIX}/sbin/spmd
	${INSTALL_PROGRAM} ${WRKSRC}/spmd/spmdctl ${ZPREFIX}/sbin/spmdctl
#	${INSTALL_SCRIPT} ${WRKSRC}/pskgen/pskgen ${ZPREFIX}/sbin/pskgen

.include <bsd.port.post.mk>
