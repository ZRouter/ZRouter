# Makefile.in generated by automake 1.11.1 from Makefile.am.
# install-win32/Makefile.  Generated from Makefile.in by configure.

# Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002,
# 2003, 2004, 2005, 2006, 2007, 2008, 2009  Free Software Foundation,
# Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.



#
#  OpenVPN -- An application to securely tunnel IP networks
#             over a single UDP port, with support for SSL/TLS-based
#             session authentication and key exchange,
#             packet encryption, packet authentication, and
#             packet compression.
#
#  Copyright (C) 2002-2010 OpenVPN Technologies, Inc. <sales@openvpn.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2
#  as published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program (see the file COPYING included with this
#  distribution); if not, write to the Free Software Foundation, Inc.,
#  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#


pkgdatadir = $(datadir)/openvpn
pkgincludedir = $(includedir)/openvpn
pkglibdir = $(libdir)/openvpn
pkglibexecdir = $(libexecdir)/openvpn
am__cd = CDPATH="$${ZSH_VERSION+.}$(PATH_SEPARATOR)" && cd
install_sh_DATA = $(install_sh) -c -m 644
install_sh_PROGRAM = $(install_sh) -c
install_sh_SCRIPT = $(install_sh) -c
INSTALL_HEADER = $(INSTALL_DATA)
transform = $(program_transform_name)
NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
build_triplet = i386-portbld-freebsd8.0
host_triplet = i386-portbld-freebsd8.0
target_triplet = i386-portbld-freebsd8.0
am__append_1 = sample.ovpn
subdir = install-win32
DIST_COMMON = $(am__dist_conf_DATA_DIST) $(am__dist_noinst_DATA_DIST) \
	$(srcdir)/Makefile.am $(srcdir)/Makefile.in \
	$(srcdir)/settings.in
ACLOCAL_M4 = $(top_srcdir)/aclocal.m4
am__aclocal_m4_deps = $(top_srcdir)/acinclude.m4 \
	$(top_srcdir)/version.m4 $(top_srcdir)/configure.ac
am__configure_deps = $(am__aclocal_m4_deps) $(CONFIGURE_DEPENDENCIES) \
	$(ACLOCAL_M4)
mkinstalldirs = $(install_sh) -d
CONFIG_HEADER = $(top_builddir)/config.h
CONFIG_CLEAN_FILES = settings
CONFIG_CLEAN_VPATH_FILES =
SOURCES =
DIST_SOURCES =
am__dist_conf_DATA_DIST = sample.ovpn
am__vpath_adj_setup = srcdirstrip=`echo "$(srcdir)" | sed 's|.|.|g'`;
am__vpath_adj = case $$p in \
    $(srcdir)/*) f=`echo "$$p" | sed "s|^$$srcdirstrip/||"`;; \
    *) f=$$p;; \
  esac;
am__strip_dir = f=`echo $$p | sed -e 's|^.*/||'`;
am__install_max = 40
am__nobase_strip_setup = \
  srcdirstrip=`echo "$(srcdir)" | sed 's/[].[^$$\\*|]/\\\\&/g'`
am__nobase_strip = \
  for p in $$list; do echo "$$p"; done | sed -e "s|$$srcdirstrip/||"
am__nobase_list = $(am__nobase_strip_setup); \
  for p in $$list; do echo "$$p $$p"; done | \
  sed "s| $$srcdirstrip/| |;"' / .*\//!s/ .*/ ./; s,\( .*\)/[^/]*$$,\1,' | \
  $(AWK) 'BEGIN { files["."] = "" } { files[$$2] = files[$$2] " " $$1; \
    if (++n[$$2] == $(am__install_max)) \
      { print $$2, files[$$2]; n[$$2] = 0; files[$$2] = "" } } \
    END { for (dir in files) print dir, files[dir] }'
am__base_list = \
  sed '$$!N;$$!N;$$!N;$$!N;$$!N;$$!N;$$!N;s/\n/ /g' | \
  sed '$$!N;$$!N;$$!N;$$!N;s/\n/ /g'
am__installdirs = "$(DESTDIR)$(confdir)" "$(DESTDIR)$(confdir)" \
	"$(DESTDIR)$(docdir)" "$(DESTDIR)$(easyrsadir)" \
	"$(DESTDIR)$(keysdir)"
am__dist_noinst_DATA_DIST = openssl GetWindowsVersion.nsi \
	build-pkcs11-helper.sh buildinstaller ddk-common doclean \
	dosname.pl getgui getopenssl getpkcs11helper getprebuilt \
	getxgui ifdef.pl m4todef.pl macro.pl makeopenvpn maketap \
	maketapinstall maketext openvpn.nsi setpath.nsi settings.in \
	trans.pl u2d.c winconfig sample.ovpn
DATA = $(dist_conf_DATA) $(dist_noinst_DATA) $(nodist_conf_DATA) \
	$(nodist_doc_DATA) $(nodist_easyrsa_DATA) $(nodist_keys_DATA)
DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)
ACLOCAL = ${SHELL} /usr/ports/security/openvpn/work/openvpn-2.1.4/missing --run aclocal-1.11
AMTAR = ${SHELL} /usr/ports/security/openvpn/work/openvpn-2.1.4/missing --run tar
AUTOCONF = ${SHELL} /usr/ports/security/openvpn/work/openvpn-2.1.4/missing --run autoconf
AUTOHEADER = ${SHELL} /usr/ports/security/openvpn/work/openvpn-2.1.4/missing --run autoheader
AUTOMAKE = ${SHELL} /usr/ports/security/openvpn/work/openvpn-2.1.4/missing --run automake-1.11
AWK = gawk
CC = cc
CCDEPMODE = depmode=gcc3
CFLAGS = -O2 -pipe -fno-strict-aliasing
CPP = cpp
CPPFLAGS =  -I/usr/local/include -I.
CYGPATH_W = echo
DEFS = -DHAVE_CONFIG_H
DEPDIR = .deps
ECHO_C = 
ECHO_N = -n
ECHO_T = 
EGREP = /usr/bin/grep -E
EXEEXT = 
GREP = /usr/bin/grep
IFCONFIG = /sbin/ifconfig
INSTALL = /usr/bin/install -c -o root -g wheel
INSTALL_DATA = install  -o root -g wheel -m 444
INSTALL_PROGRAM = install  -s -o root -g wheel -m 555
INSTALL_SCRIPT = install  -o root -g wheel -m 555
INSTALL_STRIP_PROGRAM = $(install_sh) -c -s
IPROUTE = ip
LDFLAGS =  -rpath=/usr/lib:/usr/local/lib -L/usr/local/lib
LIBOBJS = 
LIBS = -lssl -lcrypto -llzo2 
LTLIBOBJS = 
MAKEINFO = ${SHELL} /usr/ports/security/openvpn/work/openvpn-2.1.4/missing --run makeinfo
MAN2HTML = 
MKDIR_P = /usr/local/bin/gmkdir -p
OBJEXT = o
PACKAGE = openvpn
PACKAGE_BUGREPORT = openvpn-users@lists.sourceforge.net
PACKAGE_NAME = OpenVPN
PACKAGE_STRING = OpenVPN 2.1.4
PACKAGE_TARNAME = openvpn
PACKAGE_VERSION = 2.1.4
PATH_SEPARATOR = :
PTHREAD_CC = 
PTHREAD_CFLAGS = 
PTHREAD_LIBS = 
ROUTE = /sbin/route
SET_MAKE = 
SHELL = /bin/sh
STRIP = 
TAP_ID = tap0901
TAP_WIN32_MIN_MAJOR = 9
TAP_WIN32_MIN_MINOR = 1
VERSION = 2.1.4
abs_builddir = /usr/ports/security/openvpn/work/openvpn-2.1.4/install-win32
abs_srcdir = /usr/ports/security/openvpn/work/openvpn-2.1.4/install-win32
abs_top_builddir = /usr/ports/security/openvpn/work/openvpn-2.1.4
abs_top_srcdir = /usr/ports/security/openvpn/work/openvpn-2.1.4
ac_ct_CC = cc
am__include = include
am__leading_dot = .
am__quote = 
am__tar = ${AMTAR} chof - "$$tardir"
am__untar = ${AMTAR} xf -
bindir = ${exec_prefix}/bin
build = i386-portbld-freebsd8.0
build_alias = i386-portbld-freebsd8.0
build_cpu = i386
build_os = freebsd8.0
build_vendor = portbld
builddir = .
datadir = ${datarootdir}
datarootdir = ${prefix}/share
docdir = ${datarootdir}/doc/${PACKAGE_TARNAME}
dvidir = ${docdir}
exec_prefix = ${prefix}
host = i386-portbld-freebsd8.0
host_alias = 
host_cpu = i386
host_os = freebsd8.0
host_vendor = portbld
htmldir = ${docdir}
includedir = ${prefix}/include
infodir = /usr/local/info
install_sh = ${SHELL} /usr/ports/security/openvpn/work/openvpn-2.1.4/install-sh
libdir = ${exec_prefix}/lib
libexecdir = ${exec_prefix}/libexec
localedir = ${datarootdir}/locale
localstatedir = ${prefix}/var
mandir = /usr/local/man
mkdir_p = /usr/local/bin/gmkdir -p
oldincludedir = /usr/include
pdfdir = ${docdir}
prefix = /usr/local
program_transform_name = s,x,x,
psdir = ${docdir}
sbindir = ${exec_prefix}/sbin
sharedstatedir = ${prefix}/com
srcdir = .
sysconfdir = ${prefix}/etc
target = i386-portbld-freebsd8.0
target_alias = 
target_cpu = i386
target_os = freebsd8.0
target_vendor = portbld
top_build_prefix = ../
top_builddir = ..
top_srcdir = ..
win32datadir = ${datadir}/openvpn-win32
MAINTAINERCLEANFILES = $(srcdir)/Makefile.in
dist_noinst_DATA = openssl GetWindowsVersion.nsi \
	build-pkcs11-helper.sh buildinstaller ddk-common doclean \
	dosname.pl getgui getopenssl getpkcs11helper getprebuilt \
	getxgui ifdef.pl m4todef.pl macro.pl makeopenvpn maketap \
	maketapinstall maketext openvpn.nsi setpath.nsi settings.in \
	trans.pl u2d.c winconfig $(am__append_1)
#nodist_doc_DATA = tmp/license.txt
#confdir = $(win32datadir)/config
#nodist_conf_DATA = \
#	tmp/openssl.cnf.sample \
#	tmp/client.ovpn \
#	tmp/server.ovpn

#dist_conf_DATA = \
#	sample.ovpn

#easyrsadir = $(win32datadir)/easy-rsa/Windows
#nodist_easyrsa_DATA = \
#	$(top_srcdir)/easy-rsa/Windows/*

#keysdir = $(win32datadir)/sample-keys
#nodist_keys_DATA = \
#	$(top_srcdir)/sample-keys/*

all: all-am

.SUFFIXES:
$(srcdir)/Makefile.in:  $(srcdir)/Makefile.am  $(am__configure_deps)
	@for dep in $?; do \
	  case '$(am__configure_deps)' in \
	    *$$dep*) \
	      ( cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh ) \
	        && { if test -f $@; then exit 0; else break; fi; }; \
	      exit 1;; \
	  esac; \
	done; \
	echo ' cd $(top_srcdir) && $(AUTOMAKE) --gnu install-win32/Makefile'; \
	$(am__cd) $(top_srcdir) && \
	  $(AUTOMAKE) --gnu install-win32/Makefile
.PRECIOUS: Makefile
Makefile: $(srcdir)/Makefile.in $(top_builddir)/config.status
	@case '$?' in \
	  *config.status*) \
	    cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh;; \
	  *) \
	    echo ' cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@ $(am__depfiles_maybe)'; \
	    cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@ $(am__depfiles_maybe);; \
	esac;

$(top_builddir)/config.status: $(top_srcdir)/configure $(CONFIG_STATUS_DEPENDENCIES)
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh

$(top_srcdir)/configure:  $(am__configure_deps)
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh
$(ACLOCAL_M4):  $(am__aclocal_m4_deps)
	cd $(top_builddir) && $(MAKE) $(AM_MAKEFLAGS) am--refresh
$(am__aclocal_m4_deps):
settings: $(top_builddir)/config.status $(srcdir)/settings.in
	cd $(top_builddir) && $(SHELL) ./config.status $(subdir)/$@
install-dist_confDATA: $(dist_conf_DATA)
	@$(NORMAL_INSTALL)
	test -z "$(confdir)" || $(MKDIR_P) "$(DESTDIR)$(confdir)"
	@list='$(dist_conf_DATA)'; test -n "$(confdir)" || list=; \
	for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  echo "$$d$$p"; \
	done | $(am__base_list) | \
	while read files; do \
	  echo " $(INSTALL_DATA) $$files '$(DESTDIR)$(confdir)'"; \
	  $(INSTALL_DATA) $$files "$(DESTDIR)$(confdir)" || exit $$?; \
	done

uninstall-dist_confDATA:
	@$(NORMAL_UNINSTALL)
	@list='$(dist_conf_DATA)'; test -n "$(confdir)" || list=; \
	files=`for p in $$list; do echo $$p; done | sed -e 's|^.*/||'`; \
	test -n "$$files" || exit 0; \
	echo " ( cd '$(DESTDIR)$(confdir)' && rm -f" $$files ")"; \
	cd "$(DESTDIR)$(confdir)" && rm -f $$files
install-nodist_confDATA: $(nodist_conf_DATA)
	@$(NORMAL_INSTALL)
	test -z "$(confdir)" || $(MKDIR_P) "$(DESTDIR)$(confdir)"
	@list='$(nodist_conf_DATA)'; test -n "$(confdir)" || list=; \
	for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  echo "$$d$$p"; \
	done | $(am__base_list) | \
	while read files; do \
	  echo " $(INSTALL_DATA) $$files '$(DESTDIR)$(confdir)'"; \
	  $(INSTALL_DATA) $$files "$(DESTDIR)$(confdir)" || exit $$?; \
	done

uninstall-nodist_confDATA:
	@$(NORMAL_UNINSTALL)
	@list='$(nodist_conf_DATA)'; test -n "$(confdir)" || list=; \
	files=`for p in $$list; do echo $$p; done | sed -e 's|^.*/||'`; \
	test -n "$$files" || exit 0; \
	echo " ( cd '$(DESTDIR)$(confdir)' && rm -f" $$files ")"; \
	cd "$(DESTDIR)$(confdir)" && rm -f $$files
install-nodist_docDATA: $(nodist_doc_DATA)
	@$(NORMAL_INSTALL)
	test -z "$(docdir)" || $(MKDIR_P) "$(DESTDIR)$(docdir)"
	@list='$(nodist_doc_DATA)'; test -n "$(docdir)" || list=; \
	for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  echo "$$d$$p"; \
	done | $(am__base_list) | \
	while read files; do \
	  echo " $(INSTALL_DATA) $$files '$(DESTDIR)$(docdir)'"; \
	  $(INSTALL_DATA) $$files "$(DESTDIR)$(docdir)" || exit $$?; \
	done

uninstall-nodist_docDATA:
	@$(NORMAL_UNINSTALL)
	@list='$(nodist_doc_DATA)'; test -n "$(docdir)" || list=; \
	files=`for p in $$list; do echo $$p; done | sed -e 's|^.*/||'`; \
	test -n "$$files" || exit 0; \
	echo " ( cd '$(DESTDIR)$(docdir)' && rm -f" $$files ")"; \
	cd "$(DESTDIR)$(docdir)" && rm -f $$files
install-nodist_easyrsaDATA: $(nodist_easyrsa_DATA)
	@$(NORMAL_INSTALL)
	test -z "$(easyrsadir)" || $(MKDIR_P) "$(DESTDIR)$(easyrsadir)"
	@list='$(nodist_easyrsa_DATA)'; test -n "$(easyrsadir)" || list=; \
	for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  echo "$$d$$p"; \
	done | $(am__base_list) | \
	while read files; do \
	  echo " $(INSTALL_DATA) $$files '$(DESTDIR)$(easyrsadir)'"; \
	  $(INSTALL_DATA) $$files "$(DESTDIR)$(easyrsadir)" || exit $$?; \
	done

uninstall-nodist_easyrsaDATA:
	@$(NORMAL_UNINSTALL)
	@list='$(nodist_easyrsa_DATA)'; test -n "$(easyrsadir)" || list=; \
	files=`for p in $$list; do echo $$p; done | sed -e 's|^.*/||'`; \
	test -n "$$files" || exit 0; \
	echo " ( cd '$(DESTDIR)$(easyrsadir)' && rm -f" $$files ")"; \
	cd "$(DESTDIR)$(easyrsadir)" && rm -f $$files
install-nodist_keysDATA: $(nodist_keys_DATA)
	@$(NORMAL_INSTALL)
	test -z "$(keysdir)" || $(MKDIR_P) "$(DESTDIR)$(keysdir)"
	@list='$(nodist_keys_DATA)'; test -n "$(keysdir)" || list=; \
	for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  echo "$$d$$p"; \
	done | $(am__base_list) | \
	while read files; do \
	  echo " $(INSTALL_DATA) $$files '$(DESTDIR)$(keysdir)'"; \
	  $(INSTALL_DATA) $$files "$(DESTDIR)$(keysdir)" || exit $$?; \
	done

uninstall-nodist_keysDATA:
	@$(NORMAL_UNINSTALL)
	@list='$(nodist_keys_DATA)'; test -n "$(keysdir)" || list=; \
	files=`for p in $$list; do echo $$p; done | sed -e 's|^.*/||'`; \
	test -n "$$files" || exit 0; \
	echo " ( cd '$(DESTDIR)$(keysdir)' && rm -f" $$files ")"; \
	cd "$(DESTDIR)$(keysdir)" && rm -f $$files
tags: TAGS
TAGS:

ctags: CTAGS
CTAGS:


distdir: $(DISTFILES)
	@srcdirstrip=`echo "$(srcdir)" | sed 's/[].[^$$\\*]/\\\\&/g'`; \
	topsrcdirstrip=`echo "$(top_srcdir)" | sed 's/[].[^$$\\*]/\\\\&/g'`; \
	list='$(DISTFILES)'; \
	  dist_files=`for file in $$list; do echo $$file; done | \
	  sed -e "s|^$$srcdirstrip/||;t" \
	      -e "s|^$$topsrcdirstrip/|$(top_builddir)/|;t"`; \
	case $$dist_files in \
	  */*) $(MKDIR_P) `echo "$$dist_files" | \
			   sed '/\//!d;s|^|$(distdir)/|;s,/[^/]*$$,,' | \
			   sort -u` ;; \
	esac; \
	for file in $$dist_files; do \
	  if test -f $$file || test -d $$file; then d=.; else d=$(srcdir); fi; \
	  if test -d $$d/$$file; then \
	    dir=`echo "/$$file" | sed -e 's,/[^/]*$$,,'`; \
	    if test -d "$(distdir)/$$file"; then \
	      find "$(distdir)/$$file" -type d ! -perm -700 -exec chmod u+rwx {} \;; \
	    fi; \
	    if test -d $(srcdir)/$$file && test $$d != $(srcdir); then \
	      cp -fpR $(srcdir)/$$file "$(distdir)$$dir" || exit 1; \
	      find "$(distdir)/$$file" -type d ! -perm -700 -exec chmod u+rwx {} \;; \
	    fi; \
	    cp -fpR $$d/$$file "$(distdir)$$dir" || exit 1; \
	  else \
	    test -f "$(distdir)/$$file" \
	    || cp -p $$d/$$file "$(distdir)/$$file" \
	    || exit 1; \
	  fi; \
	done
check-am: all-am
check: check-am
all-am: Makefile $(DATA)
installdirs:
	for dir in "$(DESTDIR)$(confdir)" "$(DESTDIR)$(confdir)" "$(DESTDIR)$(docdir)" "$(DESTDIR)$(easyrsadir)" "$(DESTDIR)$(keysdir)"; do \
	  test -z "$$dir" || $(MKDIR_P) "$$dir"; \
	done
install: install-am
install-exec: install-exec-am
install-data: install-data-am
uninstall: uninstall-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

installcheck: installcheck-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	  install_sh_PROGRAM="$(INSTALL_STRIP_PROGRAM)" INSTALL_STRIP_FLAG=-s \
	  `test -z '$(STRIP)' || \
	    echo "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'"` install
mostlyclean-generic:

clean-generic:

distclean-generic:
	-test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)
	-test . = "$(srcdir)" || test -z "$(CONFIG_CLEAN_VPATH_FILES)" || rm -f $(CONFIG_CLEAN_VPATH_FILES)

maintainer-clean-generic:
	@echo "This command is intended for maintainers to use"
	@echo "it deletes files that may require special tools to rebuild."
	-test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
clean-local:
clean: clean-am

clean-am: clean-generic clean-local mostlyclean-am

distclean: distclean-am
	-rm -f Makefile
distclean-am: clean-am distclean-generic

dvi: dvi-am

dvi-am:

html: html-am

html-am:

info: info-am

info-am:

install-data-am: install-dist_confDATA install-nodist_confDATA \
	install-nodist_docDATA install-nodist_easyrsaDATA \
	install-nodist_keysDATA

install-dvi: install-dvi-am

install-dvi-am:

install-exec-am:

install-html: install-html-am

install-html-am:

install-info: install-info-am

install-info-am:

install-man:

install-pdf: install-pdf-am

install-pdf-am:

install-ps: install-ps-am

install-ps-am:

installcheck-am:

maintainer-clean: maintainer-clean-am
	-rm -f Makefile
maintainer-clean-am: distclean-am maintainer-clean-generic

mostlyclean: mostlyclean-am

mostlyclean-am: mostlyclean-generic

pdf: pdf-am

pdf-am:

ps: ps-am

ps-am:

uninstall-am: uninstall-dist_confDATA uninstall-nodist_confDATA \
	uninstall-nodist_docDATA uninstall-nodist_easyrsaDATA \
	uninstall-nodist_keysDATA

.MAKE: install-am install-strip

.PHONY: all all-am check check-am clean clean-generic clean-local \
	distclean distclean-generic distdir dvi dvi-am html html-am \
	info info-am install install-am install-data install-data-am \
	install-dist_confDATA install-dvi install-dvi-am install-exec \
	install-exec-am install-html install-html-am install-info \
	install-info-am install-man install-nodist_confDATA \
	install-nodist_docDATA install-nodist_easyrsaDATA \
	install-nodist_keysDATA install-pdf install-pdf-am install-ps \
	install-ps-am install-strip installcheck installcheck-am \
	installdirs maintainer-clean maintainer-clean-generic \
	mostlyclean mostlyclean-generic pdf pdf-am ps ps-am uninstall \
	uninstall-am uninstall-dist_confDATA uninstall-nodist_confDATA \
	uninstall-nodist_docDATA uninstall-nodist_easyrsaDATA \
	uninstall-nodist_keysDATA


#tmp:
#	mkdir tmp

#tmp/client.ovpn:	tmp $(top_srcdir)/sample-config-files/client.conf
#	cp $(top_srcdir)/sample-config-files/client.conf tmp/client.ovpn

#tmp/server.ovpn:	tmp $(top_srcdir)/sample-config-files/server.conf
#	cp $(top_srcdir)/sample-config-files/server.conf tmp/server.ovpn

#tmp/license.txt:	tmp $(top_srcdir)/COPYING $(top_srcdir)/COPYRIGHT.GPL
#	cat $(top_srcdir)/COPYING $(top_srcdir)/COPYRIGHT.GPL > tmp/license.txt

#tmp/openssl.cnf.sample:	tmp $(top_srcdir)/easy-rsa/2.0/openssl.cnf
#	cp $(top_srcdir)/easy-rsa/2.0/openssl.cnf tmp/openssl.cnf.sample

#clean-local:
#	-rm -fr tmp

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
