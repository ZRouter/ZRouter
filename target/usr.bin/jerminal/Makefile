
CDIR=${.CURDIR}/../../../contrib/jerminal/
.PATH: ${CDIR}


BINDIR?=	/usr/bin/
PROG=		jerm
NO_MAN=

CFLAGS+=	-I. -I${CDIR} 
CFLAGS+=	-DNDEBUG
CFLAGS+=	-DHAVE_CONFIG_H -O2 -pipe -fno-strict-aliasing

SRCS+= 			\
	jerm.c
	

LIBADD+=		m
#DPADD+=		${LIBM}
#LDFLAGS+=	-O2 -pipe -fno-strict-aliasing

ZROUTER_OBJDIR=	${.OBJDIR:C/target\/usr.sbin\/jerminal//}

.include <bsd.prog.mk>



