
LOCALBASE?=/usr/local
DEBUG?=-g
PROG=conf
NO_MAN=MAN
CFLAGS+= ${DEBUG} -Wall -I${LOCALBASE}/include
LDFLAGS+= ${DEBUG} -L${LOCALBASE}/lib
BINDIR?=/bin/
LDADD+=-ljson
DPADD+=${LIBJSON}

.include <bsd.prog.mk>