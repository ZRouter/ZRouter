CDIR=${.CURDIR}/../../../../contrib/racoon/
.PATH: ${CDIR}/src/setkey/


BINDIR?=	/sbin/
PROG=		setkey
MAN=		setkey.8

CFLAGS+=	-I. -I${CDIR}/src/setkey/ -I${CDIR}/src/libipsec/ -I${CDIR}/src/  -I${CDIR}
# -I${CDIR}/missing -I${CDIR}/lib
CFLAGS+=	-DSYSCONFDIR=\"/etc/racoon\"
#CFLAGS+=	-g -DDEBUG -DDEBUG_TRACE
CFLAGS+=	-DHAVE_CONFIG_H
CFLAGS+=	-D"yywrap()=1"

SRCS+= \
	setkey.c	\
	parse.y		\
	token.l

LDFLAGS+=	-rpath=/usr/lib:/lib

LDFLAGS+=	-ledit
LDFLAGS+=	-L../libracoonipsec/ -lracoonipsec
LDFLAGS+=	-lcrypto

.include <bsd.prog.mk>



