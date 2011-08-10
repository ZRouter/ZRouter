# $Id: Makefile.in,v 1.11 2007/12/12 07:12:22 mk Exp $

srcdir=.
prefix=/usr/local
exec_prefix=${prefix}
sbindir=${exec_prefix}/sbin
sysconfdir=/usr/local/etc/racoon2
mandir=/usr/local/man

INSTALL =/usr/bin/install -c -o root -g wheel 
INSTALL_PROGRAM=install  -s -o root -g wheel -m 555
INSTALL_SCRIPT=install  -o root -g wheel -m 555
INSTALL_DATA=install  -o root -g wheel -m 444

PROG=pskgen

all:

install: all
	$(INSTALL) -d $(sbindir)
	$(INSTALL_SCRIPT) $(PROG) $(sbindir)
	$(INSTALL_DATA) $(PROG).8 $(mandir)/man8
	$(INSTALL) -d $(prefix)/etc/racoon2
	sh ./autogen.spmd.pwd

depend:

clean:

distclean: clean
	-rm -f $(PROG) autogen.spmd.pwd
	-rm -f Makefile config.cache config.status config.log .depend
