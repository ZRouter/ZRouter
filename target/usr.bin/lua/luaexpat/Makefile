LUA_VERSION_NUM= 500

#LIB=	lxp
SHLIB_NAME?=lxp.so
LIBDIR?= /usr/lib/lua
MK_MAN=		no	
SRCS=	lxplib.c
INCS=	lxplib.h
#NO_WERROR=
CFLAGS+=	-I${.CURDIR}/../lua/
#DPADD=		${LIBBSDXML}
LIBADD=		bsdxml

.include <bsd.lib.mk>


