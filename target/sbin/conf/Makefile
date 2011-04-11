
#
LDADD+=	-lbsdxml
LDFLAGS+=-ledit
CFLAGS+=-g -I. -DUSE_XML_PARSER -DINDENT=\"\\t\"
SRCS=main.c node.c nodelist.c parsers.c conf.c xml.c
HEADERS=node.h nodelist.h param.h parsers.h conf.h xml.h
SRCS+=${HEADERS}
PROG=conf
NO_MAN=NO_MAN

test:	${PROG}
	./${PROG}

gdb:	${PROG}
	gdb ./${PROG} ./${PROG}.core

.include <bsd.prog.mk>