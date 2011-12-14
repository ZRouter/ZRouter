# Generated automatically from Makefile.in by configure.
# $Id: Makefile.in,v 1.35 2007/05/16 11:05:10 fukumoto Exp $

sysconfdir = /usr/local/etc/racoon2

CC = cc
LEX = flex
YACC = bison -y
AR = /usr/bin/ar
RANLIB = ranlib
MKDEP = /usr/bin/mkdep --

LDFLAGS	=  -rpath=/usr/lib:/usr/local/lib -L/usr/lib
CFLAGS	= -O2 -pipe -fno-strict-aliasing  -DENABLE_SECURE -Werror -Wall -Wmissing-prototypes -Wmissing-declarations -g -O  -DINET6=1 -DSTDC_HEADERS=1 -DHAVE_SYS_WAIT_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_NET_IF_VAR_H=1 -DHAVE_NET_ETHERNET_H=1 -DHAVE_LIMITS_H=1 -DHAVE_UNISTD_H=1 -DHAVE_STDARG_H=1 -DHAVE_NETIPSEC_IPSEC_H=1 -DHAVE_NET_PFKEYV2_H=1 -DENABLE_NATT=1 -DTIME_WITH_SYS_TIME=1 -DHAVE_STRFTIME=1 -DHAVE_VPRINTF=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_MKTIME=1 -DHAVE_SOCKET=1 -DHAVE_STRDUP=1 -DHAVE_STRERROR=1 -DHAVE_STRTOL=1 -DHAVE_GETIFADDRS=1 -DHAVE_STRLCPY=1 -DHAVE_STRLCAT=1 -DHAVE_ATOLL=1 -DHAVE_SA_LEN=1 -DHAVE_OPENSSL_EVP_H=1 
CFLAGS	+=-DSYSCONFDIR=\"${sysconfdir}\"
CFLAGS	+=-DCF_DEBUG
# CFLAGS  +=-DSADB_X_MIGRATE
YFLAGS	+=-d

PROG =	libracoon.a
C_SRCS =vmbuf.c rc_type.c rc_net.c rc_str.c \
	if_pfkeyv2.c if_spmd.c \
	cfsetup.c  \
	pidfile.c plog.c safefile.c version.c rbuf.c addresspool.c
OBJS =	$(C_SRCS:.c=.o) cfparse.o cftoken.o 
YYTEST_OBJS = sample.o
YYTEST_SRCS = sample.c

all: $(PROG)

$(PROG): $(OBJS)
	$(AR) cr $(PROG) $(OBJS)
	$(RANLIB) $(PROG)

strlcat.o: missing/strlcat.c
	$(CC) $(CFLAGS) -c $<

strlcpy.o: missing/strlcpy.c
	$(CC) $(CFLAGS) -c $<

atoll.o: missing/atoll.c
	$(CC) $(CFLAGS) -c $<

.c.o:
	$(CC) $(CFLAGS) -c $<

.y.c:
	$(YACC) $(YFLAGS) $<
	mv y.tab.c $*.c
#	mv y.tab.h $*.h

.l.c:
	$(LEX) -t $< > $@

sample: sample.o $(PROG)
	$(CC) $(CFLAGS) $(LDFLAGS) -o sample sample.o $(PROG)

# empty command: current libracoon is NOT expected to be installed.
install: all

clean:
	-rm -f $(PROG) *.o *.core
	-rm -f cfparse.c cftoken.c
	-rm -f y.tab.h y.output lex.yy.c
	-rm -f sample

distclean: clean
	-rm -f Makefile config.cache config.status config.log .depend

depend:
	$(MKDEP) $(CFLAGS) $(C_SRCS)
#-include .depend
# *.h is a bit wasteful, but safe.
cfparse.o cftoken.o: *.h
# y.tab.h may not have been generated when the above wildcard is evaluated.
cftoken.o: y.tab.h
# y.tab.h has no target rule, so cftoken.c is used as a surrogate.
y.tab.h: cfparse.c
