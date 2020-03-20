#!/bin/sh
# this script use in Makefile
# $1=arch(not use), $2=phys addr, $3=kernel elf

# 0x40000000 + 0xc0000100 = 0x40000100
ENT=`readelf -h $3 | awk '$1=="Entry"{gsub("c","0",$4);print $4}'`
LOAD=`readelf -Wn $3 | awk '/Notes at offset/{print $4}' | head -1`
KVMENT=`echo $2 ${ENT} | sed 's/....... 0x.//'`
echo ${KVMENT} ${LOAD} | sed 's/... 0x.....//'
