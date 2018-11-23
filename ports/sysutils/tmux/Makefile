# Created by: Wen Heping <wenheping@gmail.com>
# $FreeBSD: head/sysutils/tmux/Makefile 482972 2018-10-25 14:05:59Z mat $

PORTNAME=	tmux
PORTVERSION=	2.8
CATEGORIES=	sysutils
MASTER_SITES=	https://github.com/tmux/tmux/releases/download/${PORTVERSION}/
DISTFILES=	${DISTNAME}${EXTRACT_SUFX}

MAINTAINER=	mat@FreeBSD.org
COMMENT=	Terminal Multiplexer

LICENSE=	ISCL
LICENSE_FILE=	${WRKSRC}/COPYING

CONFLICTS_INSTALL=	tmux23

USES=		pkgconfig ncurses

USE_GITHUB=	nodefault
GH_TUPLE=	imomaliev:tmux-bash-completion:ef56d3e:bash

GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--sysconfdir=${PREFIX}/etc --enable-utempter \
		CFLAGS=-I${ZPREFIX}/include/ LDFLAGS=-L${ZPREFIX}/lib/
CONFIGURE_ARGS+=--host=${ARCH}-portbld-freebsd

#		PKG_CONFIG_PATH=${ZPREFIX}/libdata/pkgconfig/ \

PLIST_FILES=	bin/tmux \
		man/man1/tmux.1.gz \
		etc/bash_completion.d/tmux

PORTDOCS=	CHANGES README TODO
PORTEXAMPLES=	*

#OPTIONS_DEFINE=	DOCS EXAMPLES LIBEVENT_STATIC BACKSPACE

LIBEVENT_STATIC_DESC=	Build with static libevent
BACKSPACE_DESC=	Build with tty/keys patch

BACKSPACE_EXTRA_PATCHES=	${PATCHDIR}/extra-patch-tty-keys.c

LIBEVENT_STATIC_LIB_DEPENDS_OFF=	libevent.so:devel/libevent
LIBEVENT_STATIC_BUILD_DEPENDS=	${LOCALBASE}/lib/libevent.a:devel/libevent
LIBEVENT_STATIC_VARS=	LESTATIC=${LOCALBASE}/lib/libevent.a

post-patch:
	@${REINPLACE_CMD} -e 's|/etc/tmux.conf|${PREFIX}/etc/tmux.conf|g' ${WRKSRC}/CHANGES \
		${WRKSRC}/tmux.h

post-configure-LIBEVENT_STATIC-on:
	${REINPLACE_CMD} -e '/LIBS/s|-levent[^[:blank:]]*|${LESTATIC}|' \
		${WRKSRC}/${MAKEFILE}

post-configure:
	${REINPLACE_CMD} -e 's|LIBS =|LIBS = compat/fdforkpty.o compat/freezero.o compat/getdtablecount.o compat/imsg-buffer.o compat/imsg.o compat/recallocarray.o compat/unvis.o compat/vis.o |' \
		${WRKSRC}/${MAKEFILE}

# for ZRouter
post-install:
	@${MKDIR} ${STAGEDIR}${PREFIX}/etc/bash_completion.d
	${INSTALL_DATA} ${WRKSRC_bash}/completions/tmux ${STAGEDIR}${PREFIX}/etc/bash_completion.d/tmux
	${INSTALL_PROGRAM} ${STAGEDIR}${PREFIX}/bin/tmux ${ZPREFIX}/bin/

post-install-DOCS-on:
	@${MKDIR} ${STAGEDIR}${DOCSDIR}
	(cd ${WRKSRC} && ${INSTALL_DATA} ${PORTDOCS} ${STAGEDIR}${DOCSDIR})

post-install-EXAMPLES-on:
	@${MKDIR} ${STAGEDIR}${EXAMPLESDIR}
	${INSTALL_DATA} ${WRKSRC}/example* ${STAGEDIR}${EXAMPLESDIR}

.include <bsd.port.mk>
