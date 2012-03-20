# $Id: Makefile,v 1.20 2009/12/22 09:00:31 amotin Exp $

.NOTPARALLEL:

all:		mpd.8

DOC=		mpd
FORMATS=	html ps

SRCS+=		mpd.sgml version.sgml overview.sgml layers.sgml manorg.sgml
SRCS+=		changes.sgml getting.sgml building.sgml installing.sgml
SRCS+=		invoke.sgml configfmt.sgml cli.sgml nat.sgml
SRCS+=		gencmd.sgml console.sgml web.sgml bundle.sgml
SRCS+=		interface.sgml links.sgml ipcp.sgml  ipv6cp.sgml 
SRCS+=		compression.sgml encryption.sgml modem.sgml nat.sgml
SRCS+=		netgraph.sgml tcp.sgml udp.sgml pptp.sgml l2tp.sgml pppoe.sgml
SRCS+=		chat.sgml trouble.sgml refs.sgml int-auth.sgml
SRCS+=		todo.sgml trafacct.sgml ippool.sgml auth.sgml dynauth.sgml

CLEANFILES+=	version.sgml mpd.8 mpd.ps

.SUFFIXES:	.in

.in:
	VERSION=`grep ^VERSION= ../src/Makefile | awk '{ print $$2 }'`; \
	DATE=`date +'%Y-%m-%d'`; \
	sed \
	  -e "s/@DATE@/$$DATE/g" \
	  -e "s/@VERSION@/$$VERSION/g" \
	  < ${.ALLSRC} > ${.TARGET}

.include "bsd.sgml.mk"
