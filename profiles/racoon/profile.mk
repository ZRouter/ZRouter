
#WORLD_SUBDIRS_LIB+=	../secure/lib/libcrypto ../secure/lib/libssl
#WORLD_SUBDIRS_SBIN+=	setkey
#WORLD_SUBDIRS_ZROUTER+=	target/sbin/racoon
WORLD_SUBDIRS+=secure/lib/libssl
WORLD_SUBDIRS_PORTS+=${ZROUTER_ROOT}/ports/security/ipsec-tools


