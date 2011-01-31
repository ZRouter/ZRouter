
fstype=cd9660
TDIR=/tmp/makefs_${fstype}_test

makefs_${fstype}:
	# Test if makefs accept ${fstype} filesystem
	STATUS=`mkdir -p ${TDIR}/fsroot \
	touch ${TDIR}/fsroot/test.file \
	makefs -t ${fstype} ${TDIR}/fsroot.${fstype} ${TDIR}/fsroot || echo MAKEFS_FAIL=${fstype} \
	rm -rf ${TDIR}`
	echo ${STATUS}

