
ZTOOLS_INSTALL_VAR= \
	DESTDIR=${ZTOOLS_PATH} \
	BINDIR= \
	BINOWN=${OWN} \
	BINGRP=${GRP}

${ZTOOLS_PATH}_dir:
	mkdir -p ${ZTOOLS_PATH}

${ZTOOLS_PATH}/oldlzma:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/oldlzma; \
	    MAKEOBJDIRPREFIX=/usr/obj/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/packimage:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/packimage ; \
	    MAKEOBJDIRPREFIX=/usr/obj/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/trx:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/trx; \
	    MAKEOBJDIRPREFIX=/usr/obj/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/asustrx:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/asustrx; \
	    MAKEOBJDIRPREFIX=/usr/obj/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install

${ZTOOLS_PATH}/make_dlink_dsr_image:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/make_dlink_dsr_image; \
	    MAKEOBJDIRPREFIX=/usr/obj/ ${MAKE} ${ZTOOLS_INSTALL_VAR} all install


