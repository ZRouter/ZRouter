
PROG=		upgrade
SRCS=		upgrade.c
DPADD=		${LIBMD}
LDADD=		-lmd -lutil
BINDIR?=	/sbin/
CFLAGS+=	-Wall -static
NO_MAN=		yet

.if !defined(WITHOUT_HTTP_MODE)
SRCS+=		http_input.c
CFLAGS+=	-DWITH_HTTP_MODE
.endif
.if !defined(WITHOUT_GEOM_GET_SIZE)
SRCS+=		geom_get_size.c
CFLAGS+=	-DUSE_GEOM_GET_SIZE
LDADD+=		-lgeom -lsbuf -lbsdxml
.endif

.include <bsd.prog.mk>
