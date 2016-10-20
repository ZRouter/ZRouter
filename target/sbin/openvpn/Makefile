
CDIR=${.CURDIR}/../../../contrib/openvpn
.PATH: ${CDIR}


BINDIR?=	/sbin/
PROG=		openvpn
#MAN=		openvpn.8

CFLAGS+=	-I. -I${CDIR} 
CFLAGS+=	-DHAVE_CONFIG_H

SRCS+= 			\
	base64.c	\
	buffer.c	\
	crypto.c	\
	dhcp.c		\
	error.c		\
	event.c		\
	fdmisc.c	\
	forward.c	\
	fragment.c	\
	gremlin.c	\
	helper.c	\
	httpdigest.c	\
	lladdr.c	\
	init.c		\
	interval.c	\
	list.c		\
	lzo.c		\
	manage.c	\
	mbuf.c		\
	misc.c		\
	mroute.c	\
	mss.c		\
	mtcp.c		\
	mtu.c		\
	mudp.c		\
	multi.c		\
	ntlm.c		\
	occ.c		\
	pkcs11.c	\
	openvpn.c	\
	options.c	\
	otime.c		\
	packet_id.c	\
	perf.c		\
	pf.c		\
	ping.c		\
	plugin.c	\
	pool.c		\
	proto.c		\
	proxy.c		\
	ieproxy.c	\
	ps.c		\
	push.c		\
	reliable.c	\
	route.c		\
	schedule.c	\
	session_id.c	\
	shaper.c	\
	sig.c		\
	socket.c	\
	socks.c		\
	ssl.c		\
	status.c	\
	thread.c	\
	tun.c		\
	win32.c		\
	cryptoapi.c

#LDFLAGS+=	-rpath=/usr/lib:/usr/local/lib
#LDFLAGS+=	-L/usr/lib


LIBADD+=	ssl crypto lzo2
DPADD+=		${LIBSSL} ${LIBCRYPTO} ${LIBLZO2}

ZROUTER_OBJDIR=	${.OBJDIR:C/target\/sbin\/openvpn//}
CFLAGS+=	-I${ZROUTER_OBJDIR}/contrib/lzo2/include/
LDFLAGS+=	-L${ZROUTER_OBJDIR}/target/lib/liblzo2/

.include <bsd.prog.mk>



