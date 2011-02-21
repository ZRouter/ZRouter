#!/bin/sh

echo f1 > /dev/led/status

cd /tmp
echo "Dumping etc to tar.gz"
/usr/bin/tar cv --exclude '*pwd.db' -zf /tmp/etc.tar.gz etc/* -C /tmp
echo "Saving /etc"
/bin/dd if=/tmp/etc.tar.gz of=/dev/map/rgdb bs=64k count=1 conv=sync
echo '.'

echo f9 > /dev/led/status

