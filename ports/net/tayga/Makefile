# $FreeBSD: head/net/tayga/Makefile 363371 2014-07-29 18:41:15Z adamw $

PORTNAME=	tayga
PORTVERSION=	0.9.2
CATEGORIES=	net
MASTER_SITES=	http://www.litech.org/tayga/

MAINTAINER=	melifaro@FreeBSD.org
COMMENT=	Userland stateless NAT64 daemon

LICENSE=	GPLv2

USES=		gmake tar:bzip2
GNU_CONFIGURE=	yes

CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/tayga ${ZPREFIX}/sbin/tayga

.include <bsd.port.mk>
