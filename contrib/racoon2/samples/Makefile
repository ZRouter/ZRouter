# $Id: Makefile.in,v 1.28 2010/05/16 18:30:20 kamada Exp $

srcdir=.
prefix=/usr/local
sysconfdir=/usr/local/etc/racoon2
INSTALL =/usr/bin/install -c -o root -g wheel 
INSTALL_PROGRAM=install  -s -o root -g wheel -m 555
INSTALL_DATA=install  -o root -g wheel -m 444
INSTALL_SCRIPT=install  -o root -g wheel -m 555

# empty
all:

install: all install-startup-rc-d install-samples install-hook
	$(INSTALL) -d -o 0 -g 0 -m 700 /var/run/racoon2

install-samples:
	$(INSTALL) -d $(sysconfdir)
	$(INSTALL) -m 600 racoon2.conf $(sysconfdir)/racoon2.conf.sample
	$(INSTALL) -m 600 vals.conf $(sysconfdir)/vals.conf.sample
	$(INSTALL) -m 600 default.conf $(sysconfdir)/default.conf.sample
	$(INSTALL) -m 600 transport_ike.conf $(sysconfdir)/transport_ike.conf.sample
	$(INSTALL) -m 600 tunnel_ike.conf $(sysconfdir)/tunnel_ike.conf.sample
	$(INSTALL) -m 600 tunnel_ike_natt.conf $(sysconfdir)/tunnel_ike_natt.conf.sample
	$(INSTALL) -m 600 transport_kink.conf $(sysconfdir)/transport_kink.conf.sample
	$(INSTALL) -m 600 tunnel_kink.conf $(sysconfdir)/tunnel_kink.conf.sample
	$(INSTALL) -m 600 local-test.conf $(sysconfdir)/local-test.conf.sample
	$(INSTALL) -d -m 700 $(sysconfdir)/psk
	$(INSTALL) -d -m 700 $(sysconfdir)/cert

install-startup-rc-d:
	$(INSTALL_SCRIPT) rc.d-iked $(prefix)/etc/rc.d/iked
.if defined (WITH_KINK)
	$(INSTALL_SCRIPT) rc.d-kinkd $(prefix)/etc/rc.d/kinkd
.endif
	$(INSTALL_SCRIPT) rc.d-spmd $(prefix)/etc/rc.d/spmd

install-startup-init-d:
	$(INSTALL) -d $(sysconfdir)/init.d
	$(INSTALL_SCRIPT) init.d-iked $(sysconfdir)/init.d/iked
	$(INSTALL_SCRIPT) init.d-kinkd $(sysconfdir)/init.d/kinkd
	$(INSTALL_SCRIPT) init.d-spmd $(sysconfdir)/init.d/spmd

install-startup-none:

install-hook:
	$(INSTALL) -d $(sysconfdir)/hook
	$(INSTALL_DATA) functions $(sysconfdir)/hook
	$(INSTALL) -d $(sysconfdir)/hook/ikesa-up.d
	$(INSTALL_SCRIPT) ikesa-up $(sysconfdir)/hook
	$(INSTALL) -d $(sysconfdir)/hook/ikesa-down.d
	$(INSTALL_SCRIPT) ikesa-down $(sysconfdir)/hook
	$(INSTALL) -d $(sysconfdir)/hook/ikesa-rekey.d
	$(INSTALL_SCRIPT) ikesa-rekey $(sysconfdir)/hook
	$(INSTALL) -d $(sysconfdir)/hook/child-up.d
	$(INSTALL_SCRIPT) child-up $(sysconfdir)/hook
	$(INSTALL) -d $(sysconfdir)/hook/child-down.d
	$(INSTALL_SCRIPT) child-down $(sysconfdir)/hook
	$(INSTALL) -d $(sysconfdir)/hook/child-rekey.d
	$(INSTALL_SCRIPT) child-rekey $(sysconfdir)/hook
	$(INSTALL) -d $(sysconfdir)/hook/migration.d
	$(INSTALL_SCRIPT) migration $(sysconfdir)/hook
	$(INSTALL_SCRIPT) 00ikesaup_sample $(sysconfdir)/hook/ikesa-up.d
	$(INSTALL_SCRIPT) 00childup_sample $(sysconfdir)/hook/child-up.d
	$(INSTALL) -d $(sysconfdir)/hook/ph1-up.d
	$(INSTALL_SCRIPT) ph1-up $(sysconfdir)/hook
	$(INSTALL) -d $(sysconfdir)/hook/ph1-down.d
	$(INSTALL_SCRIPT) ph1-down $(sysconfdir)/hook

# empty
depend:

clean:
	-rm -f *~

distclean: clean
	-rm -f Makefile racoon2.conf vals.conf \
		init.d-iked init.d-kinkd init.d-spmd \
		rc.d-iked rc.d-kinkd rc.d-racoon2 rc.d-spmd \
		ikesa-up ikesa-down ikesa-rekey \
		child-up child-down child-rekey \
		migration ph1-up ph1-down
