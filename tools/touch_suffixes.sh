#!/bin/sh

TARGET=$1
echo ${TARGET}

DIRNAME=`dirname ${TARGET}`
FILENAME=`basename ${TARGET}`
SUFFIXES=`echo ${FILENAME} | sed 's/\\./ /g'`

FILE=""
for p in ${SUFFIXES} ; do
	if [ -z "${FILE}" ]; then
		FILE="${p}"
		continue;
	fi
	FILE="${FILE}.${p}"
	if [ ! -f ${DIRNAME}/${FILE} ]; then
		# Make it very old
		touch -t 197000000000 ${DIRNAME}/${FILE}
	fi
done


exit 0
