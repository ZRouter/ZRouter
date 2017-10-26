PORTNAME=	mruby
PORTVERSION=	1.2.0
#PORTVERSION=	1.3.0
CATEGORIES=	lang
MASTER_SITES=	http://mruby.org/
#DISTNAME=	mruby-master
DISTNAME=	${PORTVERSION}
EXTRACT_SUFX=	.zip
MASTER_SITES=	https://github.com/mruby/mruby/archive/

WRKSRC=		${WRKDIR}/${PORTNAME}-${PORTVERSION}

MAINTAINER=	yamori813@yahoo.cp.jp
COMMENT=	lightweight implementation of the Ruby language

# not effect ?
#MAKE_ENV+=  YACC='/usr/local/bin/bison'

#post-patch:
#	@${REINPLACE_CMD} -e 's|ruby|/usr/local/bin/ruby|g' ${WRKSRC}/Makefile

do-install:
#	@printenv
	${INSTALL_PROGRAM} ${WRKSRC}/build/zrouter/bin/mruby ${ZPREFIX}/bin/mruby
	${INSTALL_PROGRAM} ${WRKSRC}/build/zrouter/bin/mirb ${ZPREFIX}/bin/mirb

.include <bsd.port.mk>
