
ZTOOLS_INSTALL_VAR= \
	DESTDIR=${ZTOOLS_PATH} \
	BINDIR= \
	BINOWN=${OWN} \
	BINGRP=${GRP}

${ZTOOLS_PATH}_dir:
	mkdir -p ${ZTOOLS_PATH}

${ZTOOLS_PATH}/oldlzma:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/oldlzma; mkdir -p ${ZROUTER_OBJ}/`pwd`; \
	    MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/packimage:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/packimage; mkdir -p ${ZROUTER_OBJ}/`pwd`;  \
	    MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/trx:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/trx; mkdir -p ${ZROUTER_OBJ}/`pwd`;  \
	    MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/asustrx:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/asustrx; mkdir -p ${ZROUTER_OBJ}/`pwd`;  \
	    MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/make_dlink_dsr_image:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/make_dlink_dsr_image; mkdir -p ${ZROUTER_OBJ}/`pwd`;  \
	    MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/ubnt-mkfwimage:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/ubnt-mkfwimage; mkdir -p ${ZROUTER_OBJ}/`pwd`;  \
	    MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/uboot_mkimage:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/uboot_mkimage; mkdir -p ${ZROUTER_OBJ}/`pwd`;  \
	    MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/mktplinkfw:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/mktplinkfw; mkdir -p ${ZROUTER_OBJ}/`pwd`;  \
	    MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/ProgramStore:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/ProgramStore; mkdir -p ${ZROUTER_OBJ}/`pwd`;  \
	    mkdir -p ${ZROUTER_OBJ}/`pwd`/7znew; \
	    mkdir -p ${ZROUTER_OBJ}/`pwd`/decompress; \
	    MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

