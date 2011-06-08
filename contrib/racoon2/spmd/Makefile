# Generated automatically from Makefile.in by configure.
CC = cc
MKDEP = /usr/bin/mkdep --
CFLAGS = -g -Wall -DSPMD_DEBUG -I../lib  -DHAVE_CONFIG_H -DSYSCONFDIR=\"${sysconfdir}\" 
# CFLAGS +=-DSADB_X_MIGRATE
LDFLAGS =  -rpath=/usr/lib:/usr/local/lib -L../lib -L/usr/lib
LIBS = -lcrypto -lracoon -lcrypto 
INSTALL=/usr/bin/install -c -o root -g wheel 
INSTALL_PROGRAM=install  -s -o root -g wheel -m 555
INSTALL_DATA=install  -o root -g wheel -m 444
INSTALL_DIR=${INSTALL} -d

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
sysconfdir = /usr/local/etc/racoon2
mandir = /usr/local/man
man8dir = $(mandir)/man8

SRCS := main.c dns.c udp.c query.c task.c signal.c shell.c utils.c cache.c spmd_pfkey.c fqdn_query.c
OBJS := ${SRCS:c=o}

DAEMON = spmd
COMMAND = spmdctl
MANFILES = spmd.8 spmdctl.8
TARGET = $(DAEMON) $(COMMAND)

all: $(TARGET)

$(DAEMON): $(OBJS) ../lib/libracoon.a
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(LDFLAGS) $(LIBS)

$(OBJS): spmd_includes.h spmd_internal.h task.h dns.h query.h utils.h cache.h

spmdctl: spmdctl.o ../lib/libracoon.a
	$(CC) -o $@ $(CFLAGS) spmdctl.o $(LDFLAGS) $(LIBS)

spmdctl.o: spmd_internal.h

install: all
	$(INSTALL_DIR) $(sbindir)
	$(INSTALL_PROGRAM) $(DAEMON) $(sbindir)
	$(INSTALL_PROGRAM) $(COMMAND) $(sbindir)
	$(INSTALL_DIR) $(man8dir)
	$(INSTALL_DATA) $(MANFILES) $(man8dir)

clean:
	-rm -f $(TARGET) *.o $(COMMAND)

distclean: clean
	-rm -f config.cache config.log config.status config.h Makefile .depend

depend:
	$(MKDEP) $(CFLAGS) $(SRCS)
#-include .depend
