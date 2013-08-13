#!/bin/sh

SRCPATH=$1

if [ -x /bin/realpath ] ; then
	REALPATH="realpath"
else
	REALPATH="readlink -f"
fi

ROOTUP="/.."
_FOR_ROOTUP=`echo ${SRCPATH} | sed 's/\// /g'`
for r in ${_FOR_ROOTUP}; do
	_ROOTUP_TEST=`${REALPATH} ${SRCPATH}/${ROOTUP}`
	if [ "x${_ROOTUP_TEST}" != "x/" ] ; then
		ROOTUP="${ROOTUP}/.."
	fi
done


echo ${ROOTUP}