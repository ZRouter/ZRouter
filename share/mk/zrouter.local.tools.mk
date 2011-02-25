

${ZTOOLS_PATH}_dir:
	mkdir -p ${ZTOOLS_PATH}

${ZTOOLS_PATH}/oldlzma:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/oldlzma; \
	    ${MAKE} MAKEOBJDIRPREFIX=/usr/obj/${.TARGET}/oldlzma DESTDIR=${ZTOOLS_PATH} BINDIR= BINOWN=${OWN} BINGRP=${GRP} all install

${ZTOOLS_PATH}/packimage:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/packimage ; \
	    ${MAKE} MAKEOBJDIRPREFIX=/usr/obj/${.TARGET} DESTDIR=${ZTOOLS_PATH} BINDIR= BINOWN=${OWN} BINGRP=${GRP} all install

${ZTOOLS_PATH}/trx:	${ZTOOLS_PATH}_dir
	cd ${ZROUTER_ROOT}/tools/trx; \
	    ${MAKE} MAKEOBJDIRPREFIX=/usr/obj/${.TARGET} DESTDIR=${ZTOOLS_PATH} BINDIR= BINOWN=${OWN} BINGRP=${GRP} all install


