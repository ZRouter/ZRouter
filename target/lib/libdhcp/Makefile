#
#

DHCPDIR=${.CURDIR}/../../../contrib/dhcp
.PATH: ${DHCPDIR}/common

CFLAGS+=	-I${DHCPDIR} -I${DHCPDIR}/includes

NO_MAN=

SRCS=		raw.c parse.c nit.c icmp.c dispatch.c conflex.c upf.c bpf.c \
	socket.c lpf.c packet.c memory.c print.c options.c inet.c convert.c \
	tree.c tables.c hash.c alloc.c errwarn.c inet_addr.c dlpi.c tr.c    \
	ethernet.c

#INCS=		

LIB=		dhcp
SHLIB_MAJOR= 	0
SHLIBDIR?= 	/lib

.include <bsd.lib.mk>
