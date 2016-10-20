
CDIR=		${.CURDIR}/../../../../contrib/racoon/
.PATH: 		${CDIR}/src/racoon


CFLAGS+=	-I${CDIR} -I${CDIR}/src/libipsec -I${CDIR}/src/racoon
# -I${CDIR}/lib/
CFLAGS+=	-DHAVE_CONFIG_H


MK_MAN=		no 

SHLIB_MAJOR= 	0
SHLIBDIR?= 	/lib


LIB=		racoon
CFLAGS+=	-DENABLE_SECURE
#libtool: compile:  cc -DHAVE_CONFIG_H -I. -I../.. -I./../../src/racoon/missing -O2 -pipe -fno-strict-aliasing -Wall -Werror -Wno-unused -MT ipsec_dump_policy.lo -MD -MP -MF .deps/ipsec_dump_policy.Tpo -c ipsec_dump_policy.c  -fPIC -DPIC -o .libs/ipsec_dump_policy.o



SRCS+=			\
	kmpstat.c	\
	vmbuf.c		\
	sockmisc.c	\
	misc.c

.include <bsd.lib.mk>

