CDIR=${.CURDIR}/../../../../contrib/racoon/
.PATH: ${CDIR}/src/racoon/ ${.CURDIR}


BINDIR?=	/sbin/
PROG=		racoonctl
MAN=		racoonctl.8

CFLAGS+=	-I. -I${CDIR}/src/racoon/ -I${CDIR}/src/libipsec/ -I${CDIR}/src/ -I${CDIR}/src/racoon/missing -I${CDIR}
# -I${CDIR}/missing -I${CDIR}/lib 
CFLAGS+=	-DSYSCONFDIR=\"/etc/racoon\"
#CFLAGS+=	-g -DDEBUG -DDEBUG_TRACE
CFLAGS+=	-DHAVE_CONFIG_H
CFLAGS+=	-DADMINPORTDIR=\"/var/db/racoon\"
CFLAGS+=	-D_GNU_SOURCE

SRCS = racoonctl.c str2val.c 
#racoonctl_LDADD = libracoon.la ../libipsec/libipsec.la 
#libracoon_la_SOURCES = kmpstat.c vmbuf.c sockmisc.c misc.c
#plainrsa_gen_SOURCES = plainrsa-gen.c plog.c \
#	crypto_openssl.c logger.c 

LDFLAGS+=	-rpath=/lib:/usr/lib

LDFLAGS+=	-ledit
LDFLAGS+=	-L../libracoonipsec/ -lracoonipsec
LDFLAGS+=	-L../libracoon/ -lracoon
LDFLAGS+=	-lcrypto

.include <bsd.prog.mk>



