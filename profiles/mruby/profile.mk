
# XXX path must not be hardcoded, or must point to zhttpd in ports tree
WORLD_SUBDIRS_PORTS+=${ZROUTER_ROOT}/ports/lang/mruby

# If add mrbgems on custom build setting also add needed add libs to borad or
# vendor mk file. Like as follow line.
#WORLD_SUBDIRS+= secure/lib/libcrypto
#WORLD_SUBDIRS_LIB+=librt
