


makefs_cd9660:
	# Test if makefs accept cd9660 filesystem
	#if [  ] ; then fi

${ZTOOLS_PATH}/oldlzma:
	${MAKE} -C ${ZROUTER_ROOT}/tools/oldlzma DESTDIR=${ZTOOLS_PATH} BINDIR= BINOWN=${OWN} BINGRP=${GRP} all install

${ZTOOLS_PATH}/packimage:
	${MAKE} -C ${ZROUTER_ROOT}/tools/packimage DESTDIR=${ZTOOLS_PATH} BINDIR= BINOWN=${OWN} BINGRP=${GRP} all install

${ZTOOLS_PATH}/trx:
	${MAKE} -C ${ZROUTER_ROOT}/tools/trx DESTDIR=${ZTOOLS_PATH} BINDIR= BINOWN=${OWN} BINGRP=${GRP} all install


