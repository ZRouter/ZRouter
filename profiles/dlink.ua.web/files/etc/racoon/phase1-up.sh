#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin
LOG=/tmp/racoon-phase1-up.log

touch ${LOG}
echo "phase1-up.sh $*" >> ${LOG}


echo $@  >>  ${LOG}
echo "LOCAL_ADDR = ${LOCAL_ADDR} LOCAL_PORT = ${LOCAL_PORT} REMOTE_ADDR = ${REMOTE_ADDR} REMOTE_PORT = ${REMOTE_PORT}" >>  ${LOG}

echo >>  ${LOG}

query="cmd=event"
query="${query}&eventtype=linkup"
query="${query}&iface=IPSec0"	# XXX: should use names for IPSec peers
query="${query}&gw=${REMOTE_ADDR}:${REMOTE_PORT}"
query="${query}&ip=${LOCAL_ADDR}:${LOCAL_PORT}"

# Notify configuration handler
fetch -qo - "http://127.0.0.1:8/event.xml?${query}"

exit 0

