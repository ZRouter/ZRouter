
# Params
WITH_XML_PARSER?=1


LDFLAGS+=	-ledit
CFLAGS+=	-g -I. -DINDENT=\"\\t\"
SRCS=		conf_main.c  conf_node.c conf_nodelist.c conf_parsers.c conf_conf.c conf_strbuf.c
HEADERS=	conf_param.h conf_node.h conf_nodelist.h conf_parsers.h conf_conf.h conf_strbuf.h

.if ${WITH_XML_PARSER} == 1
CFLAGS+= 	-DWITH_XML_PARSER
LIBADD+=	bsdxml
SRCS+=		conf_xml.c
HEADERS+=	conf_xml.h
.endif

SRCS+=		${HEADERS}
PROG=		conf
MK_MAN=	 no	

test:	${PROG}
	./${PROG}

gdb:	${PROG}
	gdb ./${PROG} ./${PROG}.core

.include <bsd.prog.mk>
