#!/bin/sh
# this script use in Makefile

# 0x40000000 + 0xc0000100 = 0x40000100
ENT=`readelf -h $3 | awk '$1=="Entry"{gsub("c","0",$4);print $4}'`
echo $2 ${ENT} | sed 's/....... 0x.//' | awk '{printf("0x%x", $1 - 44)}'
