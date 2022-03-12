
KERNCONF_OPTIONS+=IPSEC
KERNCONF_DEVICES+=enc crypto

WORLD_SUBDIRS_LIB+=../secure/lib/libssl ../secure/lib/libcrypto
WORLD_SUBDIRS_PORTS+=${ZROUTER_ROOT}/ports/devel/libevent
WORLD_SUBDIRS_PORTS+=${ZROUTER_ROOT}/ports/security/openiked-portable


