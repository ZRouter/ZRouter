# $Id: Makefile.in,v 1.4 2007/12/16 06:08:04 harding Exp $
#
# Makefile.  Generated from Makefile.in by configure.

VER=		1.4b

SSH=		/usr/bin/ssh

prefix=		/usr/local
exec_prefix=	${prefix}
bindir=		${exec_prefix}/bin
datadir=	${prefix}/share
mandir=		/usr/local/man

SRCDIR=		.


CC=		cc
CFLAGS=		-O2 -pipe -fno-strict-aliasing -Wall -DVER=\"$(VER)\" -DSSH_PATH=\"$(SSH)\"
CPPFLAGS=	

OFILES=		autossh.o

LD=		@LD@
LDFLAGS=	
LIBS=		

TARGET=		autossh

all:		$(TARGET)


$(TARGET):	$(OFILES)
		$(CC) $(CPPFLAGS) -o $(TARGET) $(OFILES) $(LIBS)

clean:
		- /bin/rm -f *.o *.a *.core *~

allclean:	clean
		- /bin/rm -f $(TARGET)

distclean:	allclean
		- /bin/rm -f config.log config.cache config.status config.h
		- /bin/rm -rf autom4te.cache
		- /bin/rm -f Makefile

install:	$(TARGET)
		mkdir -p -m 755 $(bindir)
		mkdir -p -m 755 $(prefix)/share/doc/autossh
		mkdir -p -m 755 $(datadir)/examples/autossh
		mkdir -p -m 755 $(mandir)/man1
		${BSD_INSTALL_DATA} $(TARGET) $(bindir)
		${BSD_INSTALL_DATA} CHANGES README $(datadir)/doc/autossh
		${BSD_INSTALL_DATA} autossh.host	$(datadir)/examples/autossh
		${BSD_INSTALL_DATA} rscreen $(datadir)/examples/autossh
		${BSD_INSTALL_MAN} autossh.1 $(mandir)/man1
		chmod 755 $(bindir)/$(TARGET)
		chmod 644 $(datadir)/doc/autossh/CHANGES
		chmod 644 $(datadir)/doc/autossh/README
		chmod 644 $(datadir)/examples/autossh/autossh.host 
		chmod 644 $(datadir)/examples/autossh/rscreen
		chmod 644 $(mandir)/man1/autossh.1 
