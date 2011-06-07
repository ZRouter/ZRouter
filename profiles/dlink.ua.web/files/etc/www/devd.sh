#!/bin/sh


cd /etc/www

echo $$ > /var/run/devd.sh.pid


while true; do
    ./devd.lua
done

