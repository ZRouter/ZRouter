# Created by: sumikawa

PORTNAME=	racoon2
PORTVERSION=	${RACOON2_COMMIT_DATE}
PORTREVISION=	1
CATEGORIES=	security net
HASH=		7ee335e
RACOON2_COMMIT_DATE=202000908

MAINTAINER=	cy@FreeBSD.org
COMMENT=	Racoon2 IPsec daemon

BROKEN_FreeBSD_11=	Requires openssl to build

CONFLICTS=	racoon2-legacy-*

USE_GITHUB=     yes
GH_ACCOUNT=     zoulasc
GH_TAGNAME=     ${HASH}

#USES=		autoreconf libtool:build perl5 tar:tgz ssl
USES=		autoreconf tar:tgz ssl
CONFIGURE_ARGS+=	--enable-natt \
			--prefix=${STAGEDIR}/${PREFIX}
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd
CONFIGURE_ARGS+=CFLAGS=-I${ZPREFIX}/include/ LDFLAGS=-L${ZPREFIX}/lib/

GNU_CONFIGURE=	yes
CONFIGURE_ENV=	YACC=/usr/bin/yacc perl_bindir=${LOCALBASE}/bin/perl \
		STAGEDIR=${STAGEDIR} PREFIX=${PREFIX}

MAKE_ENV=	STAGEDIR=${STAGEDIR} PREFIX=${PREFIX}

CFLAGS=		-DHAVE_CONFIG_H

#OPTIONS_DEFINE=	DOCS
#OPTIONS_DEFAULT=	DOCS

.include <bsd.port.options.mk>

.if ${PORT_OPTIONS:MDOCS}
PORTDOCS+=	INSTALL USAGE config-usage.ja.txt config-usage.txt
PORTDOCS+=	iked-memo.ja.txt libracoon.ja.txt specification.ja.txt
PORTDOCS+=	spmif.txt style.txt system-message.ja.txt
.endif

post-patch:
	# @${REINPLACE_CMD} 's|/var/run/racoon2|$$(STAGEDIR)&|' \
		${WRKSRC}/samples/Makefile.in
	${REINPLACE_CMD} 's|$$(sysconfdir)|${STAGEDIR}/${PREFIX}/etc|' ${WRKSRC}/samples/rc.d/Makefile.am
	@${LN} -sf ${LOCALBASE}/share/automake-1.16/ylwrap ${WRKSRC}/lib

post-configure:
	@${REINPLACE_CMD} -e 's|-L/usr/lib||g' ${WRKSRC}/lib/Makefile
	@${REINPLACE_CMD} -e 's|-L/usr/lib||g' ${WRKSRC}/iked/Makefile
	@${REINPLACE_CMD} -e 's|-L/usr/lib||g' ${WRKSRC}/spmd/Makefile

post-patch-DOCS-on:
.for FILE in ${PORTDOCS}
	@${REINPLACE_CMD} -e 's|/usr/local/racoon2|${PREFIX}|' ${WRKSRC}/doc/${FILE}
.endfor

post-install:
#	@if [ -z `/sbin/sysctl -a | ${GREP} -q ipsec && echo ipsec` ]; then \
#	    ${ECHO_MSG} "WARNING: IPsec feature is disabled on this host"; \
#	    ${ECHO_MSG} "         You must build the kernel if you want to run racoon on the host"; \
#	fi ;
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/sbin/iked ${ZPREFIX}/bin
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/sbin/spmd ${ZPREFIX}/bin
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/sbin/spmdctl ${ZPREFIX}/bin
	${INSTALL_LIB} ${STAGEDIR}${PREFIX}/lib/libracoon.so.0.0.0 ${ZPREFIX}/lib/
	cd ${ZPREFIX}/lib;ln -s libracoon.so.0.0.0 libracoon.so.0
	cd ${ZPREFIX}/lib;ln -s libracoon.so.0.0.0 libracoon.so

post-install-DOCS-on:
	${MKDIR} ${STAGEDIR}${DOCSDIR}
	${INSTALL_DATA} ${WRKSRC}/COPYRIGHT ${STAGEDIR}${DOCSDIR}
	${INSTALL_DATA} ${WRKSRC}/COPYRIGHT.jp ${STAGEDIR}${DOCSDIR}
	${INSTALL_DATA} ${WRKSRC}/README ${STAGEDIR}${DOCSDIR}
.for FILE in ${PORTDOCS}
	${INSTALL_DATA} ${WRKSRC}/doc/${FILE} ${STAGEDIR}${DOCSDIR}
.endfor

.include <bsd.port.mk>
