#!/bin/sh

for file in boards/*/*/*mk; do
 SOC=`awk -F'=' '/SOC_VENDOR/{VEN=$2} /SOC_CHIP/{CHP=$2} END{print VEN"/"CHP}' ${file}`
 MODEL=`echo ${file} | sed 's/boards.//;s/.board.mk//'`
 echo "${SOC}	${MODEL}"
done

