# Generated automatically from Makefile.in by configure.
# Generated automatically from Makefile.in by configure.
# $Id: Makefile.in,v 1.51 2009/03/27 07:24:26 mk Exp $

srcdir =	.
prefix =	/usr/local
exec_prefix =	${prefix}
bindir =	${exec_prefix}/bin
sbindir =	${exec_prefix}/sbin
libexecdir =	${exec_prefix}/libexec
datadir =	${prefix}/share
sysconfdir =	/usr/local/etc/racoon2
sharedstatedir = ${prefix}/com
localstatedir =	${prefix}/var
libdir =	${exec_prefix}/lib
includedir =	${prefix}/include
oldincludedir =	/usr/include
infodir =	/usr/local/info/
mandir =	/usr/local/man
host =		
host_alias =	build_alias=i386-portbld-freebsd8.0
host_cpu =	
host_vendor =	
host_os =	
CC =	cc
INSTALL =	/usr/bin/install -c -o root -g wheel 
INSTALL_PROGRAM = install  -s -o root -g wheel -m 555
INSTALL_SCRIPT=	install  -o root -g wheel -m 555
INSTALL_DATA =	install  -o root -g wheel -m 444
LDFLAGS =	-L../lib  -rpath=/usr/lib:/usr/local/lib -L/usr/lib
CPPFLAGS =	-I. -I./missing
CPPFLAGS +=	-I../lib 
CPPFLAGS += -DSYSCONFDIR=\"${sysconfdir}\"
CPPFLAGS += -DDEBUG
CPPFLAGS += -DDEBUG_TRACE
# CPPFLAGS += -DSELFTEST
CPPFLAGS +=  -DHAVE_CONFIG_H
# CPPFLAGS += -DSADB_X_MIGRATE

CFLAGS = -O2 -pipe -fno-strict-aliasing -Wall -g 

PROG = iked
TESTPROG = eaytest

IKEV1_SRC= ikev1/ikev1.c ikev1/genlist.c ikev1/ipsec_doi.c ikev1/isakmp_ident.c ikev1/isakmp_quick.c ikev1/isakmp_inf.c ikev1/handler.c ikev1/strnames.c ikev1/oakley.c ikev1/vendorid.c ikev1/algorithm.c ikev1/proposal.c  ikev1/pfkey.c 
IKEV1_NATT_SRC = ikev1/ikev1_natt.c

IKEV2_SRC = ikev2.c ikev2_auth.c ikev2_child.c ikev2_cookie.c ikev2_notify.c ikev2_packet.c ikev2_payload.c ikev2_proposal.c ikev2_rekey.c ikev2_config.c 
IKEV2_NATT_SRC = nattraversal.c 

SRCS = main.c dh.c ike_sa.c ike_conf.c isakmp.c authenticator.c encryptor.c crypto_openssl.c str2val.c keyed_hash.c sockmisc.c ike_pfkey.c ike_spmif.c schedule.c ratelimit.c script.c  $(IKEV1_SRC) $(IKEV1_NATT_SRC) $(IKEV2_SRC) $(IKEV2_NATT_SRC)

OBJS = $(SRCS:%.c=%.o)
OBJS += 
OBJS += 

TESTSRC = eaytest.c
TESTOBJ = $(TESTSRC:%.c=%.o)

LIBS = -lracoon -lcrypto 
LIBRARIES = ../lib/libracoon.a

MAN = iked.8

all:  $(PROG) $(TESTPROG)

install: install-prog install-doc

install-prog: $(PROG)
	$(INSTALL) -d $(sbindir)
	$(INSTALL_PROGRAM) $(PROG) $(sbindir)

install-doc:
	$(INSTALL) -d $(mandir)/man8
# not friendly with -n  :-(
	sysconfdir="$$(echo '$(sysconfdir)' | sed 's/%/\\\%/g')"; \
	for f in $(MAN); do \
		sed -e s%\@sysconfdir\@%"$$sysconfdir"%g < $$f > $${f}.tmp; \
		$(INSTALL_DATA) $${f}.tmp $(mandir)/man$${f##*.}/$$f; \
		rm $${f}.tmp; \
	done

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -M $(SRCS) > .depend

#-include .depend

iked: $(OBJS) $(LIBRARIES)
	$(CC) $(LDFLAGS) -o iked $(OBJS) $(LIBS)

eaytest: eaytest.o crypto_openssl.o str2val.o $(LIBRARIES)
	$(CC) $(LDFLAGS) -o eaytest eaytest.o crypto_openssl.o str2val.o $(LIBS)
	if ! ./eaytest ;then \
		echo "### WARNING: #################################################"; \
		echo "### WARNING: #################################################"; \
		echo "### WARNING: "; \
		echo "### WARNING: Your OpenSSL library has a BUG of DH calculation."; \
		echo "### WARNING: Please read racoon2/BUGS."; \
		echo "### WARNING: "; \
		echo "### WARNING: #################################################"; \
		echo "### WARNING: #################################################"; \
	fi

# override default .c.o rule in case it doesn't include CPPFLAGS (FreeBSD)
.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(PROG) $(TESTPROG) $(TESTOBJ)

distclean: clean
	-rm -f Makefile config.cache config.status config.log config.h .depend
