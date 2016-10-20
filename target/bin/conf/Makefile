
LOCALBASE?=/usr/local
DEBUG?=-g -DDEBUG=1
PROG=conf
MK_MAN=no
CFLAGS+= ${DEBUG} -Wall -I${LOCALBASE}/include
LDFLAGS+= ${DEBUG} -L${LOCALBASE}/lib
BINDIR?=/bin/
LIBADD+=json
DPADD+=${LIBJSON}

.PHONY=test

test: ${PROG}
	sh test.sh

gdb: ${PROG}
	gdb ${PROG}


.include <bsd.prog.mk>
