
#KERNCONF_MODULES_OVERRIDE+=if_tap
#WORLD_SUBDIRS+=		secure/lib/libcrypto secure/lib/libssl
#WORLD_SUBDIRS_ZROUTER+=	target/lib/liblzo2
#WORLD_SUBDIRS_ZROUTER+=	target/sbin/openvpn

#WORLD_SUBDIRS_PORTS+=/usr/ports/security/openvpn

KERNCONF_DEVICES+=tuntap
WORLD_SUBDIRS+=secure/lib/libssl secure/lib/libcrypto
WORLD_SUBDIRS_PORTS+=${ZROUTER_ROOT}/ports/archivers/lzo2
WORLD_SUBDIRS_PORTS+=${ZROUTER_ROOT}/ports/security/openvpn

