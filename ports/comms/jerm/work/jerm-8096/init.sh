#!/bin/sh

dev=/dev/cuad0
speed=4800

case $1 in
bin)
	awk 'BEGIN { printf("$PRWIIPRO,,RBIN\r\n") }' | jerm -b ${speed} ${dev}
	sleep 4
	awk 'BEGIN {
		printf("\xff\x81\xe8\x03\x00\x00\x00\x80\x19\xfa") # 1000
		printf("\xff\x81\xea\x03\x00\x00\x00\x80\x17\xfa") # 1002
		printf("\xff\x81\xeb\x03\x00\x00\x00\x80\x16\xfa") # 1003
		printf("\xff\x81\x6f\x04\x00\x00\x00\x80\x92\xf9") # 1135
	}' | jerm -b ${speed} ${dev}
	;;
nmea)
	(awk 'BEGIN { printf("\xff\x81\x33\x05\x03\x00\x00\x00\xcb\x78\x00\x00\x00\x00\x01\x00\xff\xff") }'; sleep 4) | jerm -b ${speed} ${dev}
	sleep 4
	awk 'BEGIN {
		printf("$PRWIILOG,GGA,V,,,\r\n")
		printf("$PRWIILOG,GSA,V,,,\r\n")
		printf("$PRWIILOG,GSV,V,,,\r\n")
		printf("$PRWIILOG,ZCH,V,,,\r\n")
	}' | jerm -b ${speed} ${dev}
	;;
esac
