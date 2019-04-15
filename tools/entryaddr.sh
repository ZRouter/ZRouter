#!/bin/sh
# this script use in Makefile

if [ $1 = "mips" -o $1  = "mipsel" ]; then
readelf -h $3 | awk '$1=="Entry"{print $4}'
elif [ $1 = "arm" -o $1 = "armv6" -o $1 = "armv7" ]; then
# 0x40000000 + 0xc0000100 = 0x40000100
ENT=`readelf -h $3 | awk '$1=="Entry"{gsub("c","0",$4);print $4}'`
echo $2 ${ENT} | sed 's/....... 0x.//'
fi
