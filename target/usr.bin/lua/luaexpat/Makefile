LUA_VERSION_NUM= 500

#LIB=	lxp
SHLIB_NAME?=lxp.so
LIBDIR?= /usr/lib/lua
NO_MAN=
SRCS=	lxplib.c
INCS=	lxplib.h
#NO_WERROR=
CFLAGS+=	-I${.CURDIR}/../lua/
#DPADD=		${LIBBSDXML}
LDADD=		-lbsdxml

.include <bsd.lib.mk>


