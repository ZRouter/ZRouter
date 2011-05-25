#!/bin/sh


cd /etc/www

echo $$ > /var/run/collector.sh.pid

while true; do
    ./collector.lua
done

