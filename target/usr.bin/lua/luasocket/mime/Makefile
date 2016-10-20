
CDIR=${.CURDIR}/../../../../../contrib/luasocket/src
.PATH: ${CDIR}/

NAME=		mime
SRCS=		mime.c
CFLAGS+=	-I${CDIR}
CFLAGS+=	-I${.CURDIR}/../../lua/
SHLIB_NAME= 	${NAME}-core.so
LIBDIR?= 	/usr/lib/lua

MK_MAN= no 

.include <bsd.lib.mk>
