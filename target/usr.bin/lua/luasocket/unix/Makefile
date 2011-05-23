
NAME=unix
CDIR=${.CURDIR}/../../../../../contrib/luasocket/src
.PATH: ${CDIR}/

CFLAGS+=	-I${CDIR}
CFLAGS+=	-I${.CURDIR}/../../lua/

NO_MAN=

SRCS=	\
	buffer.c \
	auxiliar.c \
	options.c \
	timeout.c \
	io.c \
	usocket.c \
	unix.c

#INCS=		

SHLIB_NAME= 	${NAME}-core.so
LIBDIR?= /usr/lib/lua

.include <bsd.lib.mk>
