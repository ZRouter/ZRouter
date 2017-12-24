# $Id: Makefile,v 1.17 2007/08/09 08:35:12 candy Exp $
#

SRCS=jerm.c
OBJS=jerm.o
JUPITER_FLAGS=-DJUPITER
#LIBWRAP_FLAGS=-DUSE_LIBWRAP
CFLAGS?=-g -pipe -Wall $(JUPITER_FLAGS) $(LIBWRAP_FLAGS)
#LDFLAGS=-static
LDLIBS=-lm # -lwrap  # if linux, add '-lnsl'

SRCS=jerm.c
OBJS=jerm.o

all: jerm tiocdtr

jerm: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -f $(OBJS) jerm $(.CURDIR)/errs tiocdtr

V=jerm-8096
FILES=README Makefile $(SRCS) jerm.1 tiocdtr.c \
	getaddrinfo.c getaddrinfo.h init.sh rock.pl
dist:
	cd $(.CURDIR) && \
	rm -rf $(V) && \
	mkdir $(V) && \
	ln $(FILES) $(V) && \
	tar -czf $(V).tar.gz $(V) && \
	rm -rf $(V)
