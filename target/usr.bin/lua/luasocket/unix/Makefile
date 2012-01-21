
CDIR=${.CURDIR}/../../../../../contrib/luasocket/src
.PATH: ${CDIR}/

NAME=		unix
SHLIB_NAME= 	${NAME}-core.so
LIBDIR?= 	/usr/lib/lua
SRCS=		\
	buffer.c \
	auxiliar.c \
	options.c \
	timeout.c \
	io.c \
	usocket.c \
	unix.c

CFLAGS+=	-I${CDIR}
CFLAGS+=	-I${.CURDIR}/../../lua/

NO_MAN=



.include <bsd.lib.mk>
