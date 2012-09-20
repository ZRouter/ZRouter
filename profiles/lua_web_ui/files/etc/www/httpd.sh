#!/bin/sh


cd /etc/www

echo $$ > /var/run/httpd.sh.pid

./httpd.lua > /var/log/httpd.sh.log 2>&1
echo -n "./httpd.lua exit with status code $? at " >> /var/log/httpd.sh.log 2>&1
date >> /var/log/httpd.sh.log 2>&1
echo "./httpd.lua exit " > /dev/console
exit 1

while true; do
    ./httpd.lua
done

