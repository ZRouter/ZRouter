RELEASE=0.5.0
LUA_VERSION_NUM= 500

LIB=	libhttpd
SHLIB_NAME?=libhttpd.so
LIBDIR?= /usr/lib/lua
NO_MAN=
SRCS=	libhttpd.c
#NO_WERROR=
CFLAGS+=	-I${.CURDIR}/../lua/ -DRELEASE=\"0.5.0\"
.include <bsd.lib.mk>


