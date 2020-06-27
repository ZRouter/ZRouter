PORTNAME=	mruby
PORTVERSION=	2.1.1
CATEGORIES=	lang
MASTER_SITES=	http://mruby.org/
DISTNAME=	${PORTVERSION}
#DISTNAME=	mruby-master
EXTRACT_SUFX=	.tar.gz
MASTER_SITES=	https://github.com/mruby/mruby/archive/

WRKSRC=		${WRKDIR}/${PORTNAME}-${PORTVERSION}
#WRKSRC=		${WRKDIR}/${DISTNAME}

MAINTAINER=	yamori813@yahoo.cp.jp
COMMENT=	lightweight implementation of the Ruby language
DISABLE_VULNERABILITIES=yes

MRB_ENV=	YACC="/usr/local/bin/bison" CC="/usr/bin/cc" LD="/usr/bin/cc" AR="/usr/bin/ar"

do-build:
	@(cd ${WRKSRC} && ${SETENV} ${MRB_ENV} /usr/local/bin/ruby ./minirake)

do-install:
#	@printenv
	${INSTALL_PROGRAM} ${WRKSRC}/build/zrouter/bin/mruby ${ZPREFIX}/bin/mruby
	${INSTALL_PROGRAM} ${WRKSRC}/build/zrouter/bin/mirb ${ZPREFIX}/bin/mirb

.include <bsd.port.mk>
