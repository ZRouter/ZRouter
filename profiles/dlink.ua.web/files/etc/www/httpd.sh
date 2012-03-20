#!/bin/sh


cd /etc/www

echo $$ > /var/run/httpd.sh.pid


while true; do
    ./httpd.lua
done

