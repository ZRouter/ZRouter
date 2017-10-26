
PROG=		upgrade
SRCS=		upgrade.c
DPADD=		${LIBMD}
LIBADD=		md util
BINDIR?=	/sbin/
CFLAGS+=	-Wall -static
MK_MAN=		no	

.if !defined(WITHOUT_HTTP_MODE)
SRCS+=		http_input.c
CFLAGS+=	-DWITH_HTTP_MODE
.endif
.if !defined(WITHOUT_GEOM_GET_SIZE)
SRCS+=		geom_get_size.c
CFLAGS+=	-DUSE_GEOM_GET_SIZE
LIBADD+=	geom sbuf bsdxml
.endif

.include <bsd.prog.mk>
