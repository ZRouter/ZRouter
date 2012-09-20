#!/bin/sh


cd /etc/www

echo $$ > /var/run/collector.sh.pid

INTERNET_LED=$(kenv -q INTERNET_LED)
INTERNET_LED_INVERT=$(kenv -q INTERNET_LED_INVERT)

export INTERNET_LED INTERNET_LED_INVERT

./collector.lua > /var/log/collector.sh.log 2>&1
exit 1

while true; do
    ./collector.lua > /var/log/collector.sh.log 2>&1
done

