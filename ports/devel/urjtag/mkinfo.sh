#!/bin/sh

#FILENAME=urjtag-himori-20170118.tgz
FILENAME=urjtag.tgz

sha256 /usr/ports/distfiles/${FILENAME} | sed 's/\/.*\///' > distinfo

SIZE=`ls -l /usr/ports/distfiles/${FILENAME}  | awk '{print $5}'`
echo "SIZE (${FILENAME}) = ${SIZE}" >> distinfo
