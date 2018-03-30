CDIR=${.CURDIR}/../../../../contrib/racoon/
.PATH: ${CDIR}/src/racoon/ ${.CURDIR}


BINDIR?=	/sbin/
PROG=		racoon
MAN=		racoon.8

CFLAGS+=	-I. -I${CDIR}/src/racoon/ -I${CDIR}/src/libipsec/ -I${CDIR}/src/ -I${CDIR}/src/racoon/missing -I${CDIR}
# -I${CDIR}/missing -I${CDIR}/lib 
CFLAGS+=	-DSYSCONFDIR=\"/etc/racoon\"
#CFLAGS+=	-g -DDEBUG -DDEBUG_TRACE
CFLAGS+=	-DHAVE_CONFIG_H
CFLAGS+=	-DADMINPORTDIR=\"/var/db/racoon\"
CFLAGS+=	-D_GNU_SOURCE

#	isakmp_cfg.c

SRCS1+= \
	main.c		\
	session.c	\
	isakmp.c	\
	handler.c	\
	isakmp_ident.c	\
	isakmp_agg.c	\
	isakmp_base.c	\
	isakmp_quick.c	\
	isakmp_inf.c	\
	isakmp_newg.c	\
	ipsec_doi.c	\
	gssapi.c	\
	dnssec.c	\
	getcertsbyname.c \
	privsep.c	\
	pfkey.c		\
	admin.c		\
	evt.c		\
	oakley.c	\
	grabmyaddr.c	\
	vendorid.c	\
	policy.c	\
	localconf.c	\
	remoteconf.c	\
	crypto_openssl.c \
	algorithm.c	\
	proposal.c	\
	sainfo.c	\
	strnames.c	\
	schedule.c	\
	safefile.c	\
	backupsa.c	\
	genlist.c	\
	rsalist.c	\
	cftoken.c	\
	cfparse.c	\
	y.tab.h		\
	plog.c		\
	nattraversal.c	\
	isakmp_frag.c	\
	str2val.c	\
	prsa_par.y
	
SRCS = \
	main.c session.c isakmp.c handler.c \
	isakmp_ident.c isakmp_agg.c isakmp_base.c \
	isakmp_quick.c isakmp_inf.c isakmp_newg.c \
	gssapi.c dnssec.c getcertsbyname.c privsep.c \
	pfkey.c admin.c evt.c ipsec_doi.c oakley.c grabmyaddr.c vendorid.c \
	policy.c localconf.c remoteconf.c crypto_openssl.c algorithm.c \
	proposal.c sainfo.c strnames.c \
	plog.c logger.c schedule.c str2val.c \
	safefile.c backupsa.c genlist.c rsalist.c \
	cftoken.l cfparse.y prsa_tok.c prsa_par.c \
	nattraversal.c	\
	isakmp_frag.c

CFLAGS+=	-D"yywrap()=1"
#YFLAGS+=	-p prsa
#LFLAGS+=	-Pprsa
CLEANFILES+=	prsa_par.c prsa_par.h prsa_tok.c

prsa_tok.c: prsa_tok.l
	${LEX} ${LFLAGS} -Pprsa -o${.TARGET} ${.ALLSRC}

prsa_par.c: prsa_par.y
	${YACC} ${YFLAGS} -pprsa -o${.TARGET} ${.ALLSRC}



#LDFLAGS+=	-rpath=/lib:/usr/lib:../libracoonipsec/:../libracoon/

LDFLAGS+=	-ledit
LDFLAGS+=	-L../libracoonipsec/ -lracoonipsec
#LDFLAGS+=	-lipsec


LDFLAGS+=	-L../libracoon/ -lracoon
LDFLAGS+=	-lcrypto

.include <bsd.prog.mk>



