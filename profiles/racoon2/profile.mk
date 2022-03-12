
KERNCONF_OPTIONS+=IPSEC
KERNCONF_DEVICES+=enc crypto

WORLD_SUBDIRS_LIB+= ../secure/lib/libcrypto ../secure/lib/libssl
WORLD_SUBDIRS_PORTS+=${ZROUTER_ROOT}/ports/security/racoon2

