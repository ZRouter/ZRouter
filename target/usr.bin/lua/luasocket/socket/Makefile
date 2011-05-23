
NAME=socket
CDIR=${.CURDIR}/../../../../../contrib/luasocket/src
.PATH: ${CDIR}/

CFLAGS+=	-I${CDIR}
CFLAGS+=	-I${.CURDIR}/../../lua/

NO_MAN=

SRCS=	\
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

#INCS=		

SHLIB_NAME= 	${NAME}-core.so
LIBDIR?= /usr/lib/lua

.include <bsd.lib.mk>
