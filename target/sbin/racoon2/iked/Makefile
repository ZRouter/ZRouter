
CDIR=${.CURDIR}/../../../../contrib/racoon2/
.PATH: ${CDIR}/iked/ ${CDIR}/iked/ikev1/


BINDIR?=	/sbin/
PROG=		iked
MAN=		iked.8

CFLAGS+=	-I. -I${CDIR}/iked -I${CDIR} -I${CDIR}/missing -I${CDIR}/lib
CFLAGS+=	-DSYSCONFDIR=\"/etc/racoon2\"
#CFLAGS+=	-g -DDEBUG -DDEBUG_TRACE
CFLAGS+=	-DHAVE_CONFIG_H

SRCS+= \
	main.c \
	dh.c \
	ike_sa.c \
	ike_conf.c \
	isakmp.c \
	authenticator.c \
	encryptor.c \
	crypto_openssl.c \
	str2val.c \
	keyed_hash.c \
	sockmisc.c \
	ike_pfkey.c \
	ike_spmif.c \
	schedule.c \
	ratelimit.c \
	script.c \
	ikev2.c \
	ikev2_auth.c \
	ikev2_child.c \
	ikev2_cookie.c \
	ikev2_notify.c \
	ikev2_packet.c \
	ikev2_payload.c \
	ikev2_proposal.c \
	ikev2_rekey.c \
	ikev2_config.c \
	nattraversal.c

SRCS+= \
	ikev1.c \
	genlist.c \
	ipsec_doi.c \
	isakmp_ident.c \
	isakmp_quick.c \
	isakmp_inf.c \
	handler.c \
	strnames.c \
	oakley.c \
	vendorid.c \
	algorithm.c \
	proposal.c \
	pfkey.c \
	ikev1_natt.c

LDFLAGS+=	-L../libracoon/
#LDFLAGS+=	-rpath=/usr/lib:/usr/local/lib
#LDFLAGS+=	-L/usr/lib

LDFLAGS+=	-lracoon
LDFLAGS+=	-lcrypto

.include <bsd.prog.mk>



