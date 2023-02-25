PORTNAME=	mruby
PORTVERSION=	3.2.0
CATEGORIES=	lang
DISTNAME=	${PORTVERSION}
EXTRACT_SUFX=	.tar.gz
MASTER_SITES=	https://github.com/mruby/mruby/archive/

WRKSRC=		${WRKDIR}/${PORTNAME}-${PORTVERSION}
#WRKSRC=		${WRKDIR}/${DISTNAME}

MAINTAINER=	yamori813@yahoo.cp.jp
COMMENT=	lightweight implementation of the Ruby language
DISABLE_VULNERABILITIES=yes

MRB_ENV=	YACC="/usr/local/bin/bison" CC="/usr/bin/cc" LD="/usr/bin/cc" AR="/usr/bin/ar"

do-build:
.if defined(MRUBY_CONFIG)
	@(cd ${WRKSRC} && ${SETENV} ${MRB_ENV} /usr/local/bin/rake)
.else
	${CP} ${FILESDIR}/zrouter.rb ${WRKSRC}/build_config/
	@(cd ${WRKSRC} && ${SETENV} ${MRB_ENV} /usr/local/bin/rake MRUBY_CONFIG=zrouter)
.endif

do-install:
#	@printenv
	${INSTALL_PROGRAM} ${WRKSRC}/build/zrouter/bin/mruby ${ZPREFIX}/bin/mruby
	${INSTALL_PROGRAM} ${WRKSRC}/build/zrouter/bin/mirb ${ZPREFIX}/bin/mirb

.include <bsd.port.mk>
