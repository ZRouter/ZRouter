

${ZTOOLS_PATH}_dir:
	mkdir -p ${ZTOOLS_PATH}

${ZTOOLS_PATH}/oldlzma:	${ZTOOLS_PATH}_dir
	${MAKE} -C ${ZROUTER_ROOT}/tools/oldlzma DESTDIR=${ZTOOLS_PATH} BINDIR= BINOWN=${OWN} BINGRP=${GRP} all install

${ZTOOLS_PATH}/packimage:	${ZTOOLS_PATH}_dir
	${MAKE} -C ${ZROUTER_ROOT}/tools/packimage DESTDIR=${ZTOOLS_PATH} BINDIR= BINOWN=${OWN} BINGRP=${GRP} all install

${ZTOOLS_PATH}/trx:	${ZTOOLS_PATH}_dir
	${MAKE} -C ${ZROUTER_ROOT}/tools/trx DESTDIR=${ZTOOLS_PATH} BINDIR= BINOWN=${OWN} BINGRP=${GRP} all install


