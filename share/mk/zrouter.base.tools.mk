
fstype=cd9660
TDIR=/tmp/makefs_${fstype}_test

#makefs_${fstype}:
#	# Test if makefs accept ${fstype} filesystem
#	STATUS=`mkdir -p ${TDIR}/fsroot \
#	touch ${TDIR}/fsroot/test.file \
#	makefs -t ${fstype} ${TDIR}/fsroot.${fstype} ${TDIR}/fsroot || echo MAKEFS_FAIL=${fstype} \
#	rm -rf ${TDIR}`
#	echo ${STATUS}


makefs_cd9660:	${ZTOOLS_PATH}/makefs

${ZTOOLS_PATH}/makefs:
	mkdir -p ${ZTOOLS_PATH}
	cd ${FREEBSD_SRC_TREE}/usr.sbin/makefs; \
	make MAKEOBJDIRPREFIX=${MAKEOBJDIRPREFIX} DESTDIR=${ZTOOLS_PATH}; \
	make MAKEOBJDIRPREFIX=${MAKEOBJDIRPREFIX} DESTDIR=${ZTOOLS_PATH} BINDIR= BINOWN=${OWN} BINGRP=${GRP} WITHOUT_MAN=yes WITHOUT_INFO=yes WITHOUT_NLS=yes install

