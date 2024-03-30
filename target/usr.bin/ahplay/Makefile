
LOCALBASE?=/usr/local
DEBUG?=-g -DDEBUG=1
PROG=ahplay
MK_MAN=no
#CFLAGS+= ${DEBUG} -Wall -I${LOCALBASE}/include -I${LOCALBASE}/include/fdk-aac
CFLAGS+= ${DEBUG} -Wall -I${ZPREFIX}/include -I${ZPREFIX}/include/fdk-aac
LDFLAGS+= ${DEBUG} -L${ZPREFIX}/lib -lm -lout123 -lfdk-aac
BINDIR?=/usr/local/bin/
#LIBADD+=json
#DPADD+=${LIBJSON}

#.PHONY=test

#test: ${PROG}
#	sh test.sh

#gdb: ${PROG}
#	gdb ${PROG}


.include <bsd.prog.mk>
