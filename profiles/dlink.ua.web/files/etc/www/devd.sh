#!/bin/sh


cd /etc/www

echo $$ > /var/run/devd.sh.pid

./devd.lua > /var/log/devd.sh.log 2>&1
exit 1

while true; do
    ./devd.lua
done

