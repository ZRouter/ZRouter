
LOCALBASE?=/usr/local
#DEBUG?=-g -DDEBUG=1
PROG=conf
NO_MAN=MAN
CFLAGS+= ${DEBUG} -Wall -I${LOCALBASE}/include
LDFLAGS+= ${DEBUG} -L${LOCALBASE}/lib
BINDIR?=/bin/
LDADD+=-ljson
DPADD+=${LIBJSON}

.PHONY=test

test: ${PROG}
	sh test.sh

gdb: ${PROG}
	gdb ${PROG}


.include <bsd.prog.mk>