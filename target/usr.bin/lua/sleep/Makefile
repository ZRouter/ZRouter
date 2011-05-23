LUA_VERSION_NUM= 500

SHLIB_NAME?=	sleep.so
LIBDIR?= 	/usr/lib/lua
SRCS=		sleep.c
CFLAGS+=	-I${.CURDIR}/../lua/
#CFLAGS+=	-I/usr/local/include/lua51
#CFLAGS+=	-L/usr/local/lib/ -llua-5.1
NO_MAN=

.include <bsd.lib.mk>


