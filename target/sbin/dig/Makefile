
CDIR=${.CURDIR}/../../../contrib/spcdns/src
.PATH: ${CDIR}


BINDIR?=	/sbin/
PROG=		dig
MK_MAN=		no	

CFLAGS+=	-I. -I${CDIR} 
CFLAGS+=	-DNDEBUG

SRCS+= 			\
	codec.c		\
	mappings.c	\
	netsimple.c	\
	test.c

LIBADD+=		m
DPADD+=		${LIBM}

ZROUTER_OBJDIR=	${.OBJDIR:C/target\/sbin\/dig//}

.include <bsd.prog.mk>



