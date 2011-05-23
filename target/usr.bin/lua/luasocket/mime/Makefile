
NAME=mime
CDIR=${.CURDIR}/../../../../../contrib/luasocket/src
.PATH: ${CDIR}/

CFLAGS+=	-I${CDIR}
CFLAGS+=	-I${.CURDIR}/../../lua/

NO_MAN=

SRCS=		mime.c

#INCS=		

SHLIB_NAME= 	${NAME}-core.so
LIBDIR?= /usr/lib/lua

.include <bsd.lib.mk>
