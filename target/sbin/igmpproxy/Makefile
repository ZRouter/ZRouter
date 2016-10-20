#
# $FreeBSD: src/usr.sbin/audit/Makefile,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
#

CDIR=${.CURDIR}/../../../contrib/igmpproxy/
.PATH: ${CDIR}/src/

CFLAGS+=	-I${CDIR}

PROG=		igmpproxy
MK_MAN=	 	no	

PREFIX?=	/
BINDIR?=	${PREFIX}/sbin


#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT callout.o -MD -MP -MF .deps/callout.Tpo -c -o callout.o callout.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT config.o -MD -MP -MF .deps/config.Tpo -c -o config.o config.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT confread.o -MD -MP -MF .deps/confread.Tpo -c -o confread.o confread.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT ifvc.o -MD -MP -MF .deps/ifvc.Tpo -c -o ifvc.o ifvc.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT igmp.o -MD -MP -MF .deps/igmp.Tpo -c -o igmp.o igmp.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT igmpproxy.o -MD -MP -MF .deps/igmpproxy.Tpo -c -o igmpproxy.o igmpproxy.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT kern.o -MD -MP -MF .deps/kern.Tpo -c -o kern.o kern.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT lib.o -MD -MP -MF .deps/lib.Tpo -c -o lib.o lib.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT mcgroup.o -MD -MP -MF .deps/mcgroup.Tpo -c -o mcgroup.o mcgroup.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT mroute-api.o -MD -MP -MF .deps/mroute-api.Tpo -c -o mroute-api.o mroute-api.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT request.o -MD -MP -MF .deps/request.Tpo -c -o request.o request.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT rttable.o -MD -MP -MF .deps/rttable.Tpo -c -o rttable.o rttable.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT syslog.o -MD -MP -MF .deps/syslog.Tpo -c -o syslog.o syslog.c
#cc -std=gnu99 -DHAVE_CONFIG_H -I. -I..     -O2 -pipe -fno-strict-aliasing -MT udpsock.o -MD -MP -MF .deps/udpsock.Tpo -c -o udpsock.o udpsock.c
#cc -std=gnu99  -O2 -pipe -fno-strict-aliasing   -o igmpproxy callout.o config.o confread.o ifvc.o igmp.o igmpproxy.o kern.o lib.o mcgroup.o mroute-api.o request.o rttable.o syslog.o udpsock.o 

SRCS=		callout.c config.c confread.c ifvc.c igmp.c igmpproxy.c kern.c lib.c mcgroup.c mroute-api.c request.c rttable.c syslog.c udpsock.c 

.include <bsd.prog.mk>
