#!/bin/sh

SRCPATH=$1

ROOTUP="/.."
_FOR_ROOTUP=`echo ${SRCPATH} | sed 's/\// /g'`
for r in ${_FOR_ROOTUP}; do
	_ROOTUP_TEST=`realpath ${SRCPATH}/${ROOTUP}`
	if [ "x${_ROOTUP_TEST}" != "x/" ] ; then
		ROOTUP="${ROOTUP}/.."
	fi
done


echo ${ROOTUP}