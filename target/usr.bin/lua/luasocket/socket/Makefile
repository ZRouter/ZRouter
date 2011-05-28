
CDIR=${.CURDIR}/../../../../../contrib/luasocket/src
.PATH: ${CDIR}/

NAME=		socket
SRCS=		\
	luasocket.c \
	timeout.c \
	buffer.c \
	io.c \
	auxiliar.c \
	options.c \
	inet.c \
	tcp.c \
	udp.c \
	except.c \
	select.c \
	usocket.c

SHLIB_NAME= 	${NAME}-core.so
LIBDIR?= 	/usr/lib/lua
CFLAGS+=	-I${CDIR}
CFLAGS+=	-I${.CURDIR}/../../lua/

NO_MAN=

.include <bsd.lib.mk>
