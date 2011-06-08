CDIR=${.CURDIR}/../../../../contrib/racoon2/
.PATH: ${CDIR}/spmd/


BINDIR?=	/sbin/
PROG=		spmd
MAN=		spmd.8

CFLAGS+=	-I. -I${CDIR}/spmd -I${CDIR} -I${CDIR}/missing -I${CDIR}/lib
CFLAGS+=	-DSYSCONFDIR=\"/etc/racoon2\"
#CFLAGS+=	-g -DDEBUG -DDEBUG_TRACE
CFLAGS+=	-DHAVE_CONFIG_H

SRCS+= \
	main.c		\
	dns.c		\
	udp.c		\
	query.c		\
	task.c		\
	signal.c	\
	shell.c		\
	utils.c		\
	cache.c		\
	spmd_pfkey.c	\
	fqdn_query.c


LDFLAGS+=	-L../libracoon/
#LDFLAGS+=	-rpath=/usr/lib:/usr/local/lib
#LDFLAGS+=	-L/usr/lib

LDFLAGS+=	-lracoon
LDFLAGS+=	-lcrypto

.include <bsd.prog.mk>



