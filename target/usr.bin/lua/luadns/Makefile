CDIR=${.CURDIR}/../../../../contrib/spcdns/src
.PATH: ${CDIR}

LUA_VERSION_NUM= 500

NAME=		dns
SHLIB_NAME?=	dns.so
LIBDIR?= 	/usr/lib/lua
SRCS+= 			\
	codec.c		\
	luadns.c	\
	mappings.c	\
	netsimple.c	\
	test.c
CFLAGS+=	-I. -I${CDIR} -I${.CURDIR}/../lua/
CFLAGS+=	-DNDEBUG
NO_MAN=

.include <bsd.lib.mk>




#LDADD+=		-lm
#DPADD+=		${LIBM}

#ZROUTER_OBJDIR=	${.OBJDIR:C/target\/sbin\/dig//}

