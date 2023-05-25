# Created by: Mikolaj Golub <to.my.trociny@gmail.com>
# $FreeBSD: head/net-mgmt/bsnmp-ucd/Makefile 508246 2019-08-06 10:44:28Z trociny $

PORTNAME=	bsnmp-ucd
PORTVERSION=	0.4.5
CATEGORIES=	net-mgmt

MAINTAINER=	trociny@FreeBSD.org
COMMENT=	bsnmpd module that implements parts of UCD-SNMP-MIB

LICENSE=	BSD2CLAUSE

USE_GITHUB=	yes
GH_ACCOUNT=	trociny

USES=		uidfix
MAKE_ENV+=	NO_OBJ=yes

#OPTIONS_DEFINE=	EXAMPLES

#post-install-EXAMPLES-on:
#	@${MKDIR} ${STAGEDIR}${EXAMPLESDIR}
#	${INSTALL_DATA} ${WRKSRC}/snmpd.config.sample ${STAGEDIR}${EXAMPLESDIR}

do-install:
	${INSTALL_LIB} ${WRKSRC}/snmp_ucd.so.1 ${ZPREFIX}/lib/snmp_ucd.so.1
	cd ${ZPREFIX}/lib;ln -s snmp_ucd.so.1 snmp_ucd.so

.include <bsd.port.mk>
