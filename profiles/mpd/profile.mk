
#WORLD_SUBDIRS_PORTS+=/usr/ports/security/openvpn


#WORLD_SUBDIRS+= \
#secure/lib/libcrypto \
#secure/lib/libssl

#WORLD_SUBDIRS_LIB+= libwrap
WORLD_SUBDIRS_LIB+= libpcap
WORLD_SUBDIRS_LIB+= libradius libfetch
#WORLD_SUBDIRS_ZROUTER+=target/usr.sbin/mpd
WORLD_SUBDIRS_PORTS+=${ZROUTER_ROOT}/ports/net/mpd5
WORLD_SUBDIRS_USR_SBIN+=ngctl
WORLD_SUBDIRS_LIB+=libnetgraph

KERNCONF_MODULES_OVERRIDE+= \
netgraph/async \
netgraph/bpf \
netgraph/car \
netgraph/deflate \
netgraph/ether \
netgraph/iface \
netgraph/ipfw \
netgraph/l2tp \
netgraph/ksocket \
netgraph/nat \
netgraph/netflow \
netgraph/netgraph \
netgraph/ppp \
netgraph/pppoe \
netgraph/pptpgre \
netgraph/pred1 \
netgraph/socket \
netgraph/tcpmss \
netgraph/tee \
netgraph/tty \
netgraph/vjc

KERNCONF_MODULES_OVERRIDE+= \
netgraph/mppc \
rc4
