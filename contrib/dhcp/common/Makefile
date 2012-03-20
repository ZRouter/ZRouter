# Makefile.conf
#
# Copyright (c) 1996, 1997, 1998, 1999 The Internet Software Consortium.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of The Internet Software Consortium nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
# CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
# THE INTERNET SOFTWARE CONSORTIUM OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.
#

## Defaults...
SCRIPT = none
BINDIR = /usr/sbin
CLIENTBINDIR=/sbin
INSTALL = install -c
CHMOD = chmod
ETC = /etc
VARRUN = /var/run
VARDB = /var/db

SRC    = raw.c parse.c nit.c icmp.c dispatch.c conflex.c upf.c bpf.c socket.c \
       	 lpf.c packet.c memory.c print.c options.c inet.c convert.c \
	 tree.c tables.c hash.c alloc.c errwarn.c inet_addr.c dlpi.c \
	 tr.c ethernet.c
OBJ    = raw.o parse.o nit.o icmp.o dispatch.o conflex.o upf.o bpf.o socket.o \
	 lpf.o packet.o memory.o print.o options.o inet.o convert.o \
	 tree.o tables.o hash.o alloc.o errwarn.o inet_addr.o dlpi.o \
	 tr.o ethernet.o

INCLUDES = -I.. -I../includes
COPTS=-Os -Dlint
CFLAGS = -fPIC $(DEBUG) $(PREDEFINES) $(INCLUDES) $(COPTS)

all:	libdhcp.so

libdhcp.so:	$(OBJ)
	rm -f libdhcp.so
	cc -shared -Wl,-soname,libdhcp.so -o libdhcp.so $(CFLAGS) $(OBJ)

install: all


clean:
	-rm -f $(OBJ)
	
realclean: clean
	-rm -f libdhcp.so *~ #*



# Dependencies (semi-automatically-generated)

raw.o: raw.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
parse.o: parse.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h \
 ../includes/dhcp.h ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h ../includes/dhctoken.h
nit.o: nit.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
icmp.o: icmp.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h ../includes/netinet/ip.h \
 ../includes/netinet/ip_icmp.h
dispatch.o: dispatch.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h \
 conflex.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h ../includes/dhctoken.h
upf.o: upf.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
bpf.o: bpf.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h ../includes/netinet/ip.h ../includes/netinet/udp.h \
 ../includes/netinet/if_ether.h
socket.o: socket.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
lpf.o: lpf.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
packet.o: packet.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h ../includes/netinet/ip.h \
 ../includes/netinet/udp.h ../includes/netinet/if_ether.h
memory.o: memory.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
print.o: print.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
options.o: options.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
inet.o: inet.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
convert.o: convert.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
tree.o: tree.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
tables.o: tables.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
hash.o: hash.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
alloc.o: alloc.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
errwarn.o: errwarn.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
inet_addr.o: inet_addr.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
dlpi.o: dlpi.c ../includes/dhcpd.h \
 ../includes/cdefs.h ../includes/osdep.h ../includes/site.h \
 ../includes/cf/netbsd.h ../includes/dhcp.h \
 ../includes/tree.h ../includes/hash.h ../includes/inet.h \
 ../includes/sysconf.h
