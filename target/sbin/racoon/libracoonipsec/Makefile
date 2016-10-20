
CDIR=		${.CURDIR}/../../../../contrib/racoon/
.PATH: 		${CDIR}/src/libipsec


CFLAGS+=	-I${CDIR} -I${CDIR}/src/libipsec
CFLAGS+=	-DHAVE_CONFIG_H
#CFLAGS+=	-Dyylval=libipseclval
#CFLAGS+=	-D__libipsectext=yytext

YFLAGS+=	-p __libipsec
LFLAGS+=	-P__libipsec
CLEANFILES+=	policy_parse.c policy_token.c y.tab.h

#/bin/sh ../../ylwrap policy_parse.y y.tab.c policy_parse.c y.tab.h policy_parse.h y.output policy_parse.output -- bison -y  -d -p __libipsec


MK_MAN=		no

SHLIB_MAJOR= 	0
SHLIBDIR?= 	/lib


LIB=		racoonipsec
CFLAGS+=	-DENABLE_SECURE
#libtool: compile:  cc -DHAVE_CONFIG_H -I. -I../.. -I./../../src/racoon/missing -O2 -pipe -fno-strict-aliasing -Wall -Werror -Wno-unused -MT ipsec_dump_policy.lo -MD -MP -MF .deps/ipsec_dump_policy.Tpo -c ipsec_dump_policy.c  -fPIC -DPIC -o .libs/ipsec_dump_policy.o

SRCS+=			\
	ipsec_dump_policy.c	\
	ipsec_get_policylen.c	\
	ipsec_strerror.c	\
	key_debug.c		\
	pfkey.c			\
	pfkey_dump.c		\
	policy_parse.y		\
	policy_token.l

#> gmake policy_parse.c
#/bin/sh ../../ylwrap policy_parse.y y.tab.c policy_parse.c y.tab.h policy_parse.h y.output policy_parse.output -- bison -y  -d -p __libipsec
#updating policy_parse.h

#> gmake policy_token.c
#/bin/sh ../../ylwrap policy_token.l lex.yy.c policy_token.c -- flex  -P__libipsec -olex.yy.c


.include <bsd.lib.mk>

