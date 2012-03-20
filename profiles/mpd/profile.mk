
#WORLD_SUBDIRS_PORTS+=/usr/ports/security/openvpn


#WORLD_SUBDIRS+= \
#secure/lib/libcrypto \
#secure/lib/libssl

#WORLD_SUBDIRS_LIB+= libwrap
WORLD_SUBDIRS_LIB+= libpcap
WORLD_SUBDIRS_ZROUTER+=target/usr.sbin/mpd

KERNCONF_MODULES_OVERRIDE+= \
netgraph/async \
netgraph/bpf \
netgraph/car \
netgraph/deflate \
netgraph/iface \
netgraph/ipfw \
netgraph/l2tp \
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
