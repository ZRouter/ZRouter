#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
#

.PATH: ${.CURDIR}/../../../contrib/shttpd/src

PROG=		shttpd
MAN=		shttpd.1

PREFIX?=	/
BINDIR?=	${PREFIX}/sbin
VERSION=	5.6a1

LDADD+=		-lpthread
DPADD+=		${LIBPTHREAD}

CFLAGS+=	-g -I${.CURDIR}/../../../contrib/shttpd/src
SRCS+= 		\
	string.c \
	shttpd.c \
	log.c \
	auth.c \
	md5.c \
	cgi.c \
	io_ssi_zrouter.c \
	io_file.c \
	io_socket.c \
	io_ssl.c \
	io_emb.c \
	io_dir.c \
	io_cgi.c \
	compat_unix.c \
	standalone.c

.include <bsd.prog.mk>
