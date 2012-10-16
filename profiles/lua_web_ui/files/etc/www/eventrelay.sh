#!/bin/sh


cd /etc/www

echo $$ > /var/run/eventrelay.sh.pid

EVENT_RELAY_HOST="127.0.0.1"
EVENT_RELAY_PORT=1
export EVENT_RELAY_HOST EVENT_RELAY_PORT

# Must be called w/o path, listeners list will define path per listener
kenv EVENTRELAY="http://${EVENT_RELAY_HOST}:${EVENT_RELAY_PORT}/"

./eventrelay.lua > /var/log/eventrelay.sh.log 2>&1
exit 1

while true; do
    ./eventrelay.lua > /var/log/eventrelay.sh.log 2>&1
done

