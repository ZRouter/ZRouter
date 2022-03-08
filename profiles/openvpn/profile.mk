
KERNCONF_DEVICES+=tuntap

WORLD_SUBDIRS_LIB+=../secure/lib/libssl ../secure/lib/libcrypto
WORLD_SUBDIRS_PORTS+=${ZROUTER_ROOT}/ports/archivers/lzo2
WORLD_SUBDIRS_PORTS+=${ZROUTER_ROOT}/ports/security/openvpn

