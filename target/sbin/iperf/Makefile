
CDIR=${.CURDIR}/../../../contrib/iperf
.PATH: ${CDIR}


BINDIR?=	/sbin/
PROG=		iperf3
NO_MAN=

CFLAGS+=	-I. -I${CDIR} 
CFLAGS+=	-DNDEBUG
CFLAGS+=	-DHAVE_CONFIG_H -O2 -pipe -fno-strict-aliasing

SRCS+= 			\
src/main.c \
src/cjson.c \
src/iperf_api.c \
src/iperf_error.c \
src/iperf_client_api.c \
src/iperf_locale.c \
src/iperf_server_api.c \
src/iperf_tcp.c \
src/iperf_udp.c \
src/iperf_sctp.c \
src/iperf_sctp.h \
src/iperf_util.c \
src/net.c \
src/tcp_info.c \
src/tcp_window_size.c \
src/timer.c \
src/units.c

LIBADD+=		m
#DPADD+=		${LIBM}
LDFLAGS+=	-O2 -pipe -fno-strict-aliasing

ZROUTER_OBJDIR=	${.OBJDIR:C/target\/sbin\/iperf//}

.include <bsd.prog.mk>



