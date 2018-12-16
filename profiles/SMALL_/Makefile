
.if exists(${SRCTOP}/sys/conf/newvers.sh)
FREEBSD_VERSION_VARS!=grep -E '(TYPE|REVISION|BRANCH)=\"' \
	${SRCTOP}/sys/conf/newvers.sh | sed 's/\"//g' | sed 's/^/FREEBSD_/'
.for var in ${FREEBSD_VERSION_VARS}
VAR_LEFT=${var:C/=.*//}
VAR_RIGHT=${var:C/.*=//}
${VAR_LEFT}:=${VAR_RIGHT}
.endfor
FREEBSD_RELEASE=${FREEBSD_TYPE}-${FREEBSD_REVISION}-${FREEBSD_BRANCH}
.else
.error "missing FreeBSD source tree: FREEBSD_SRC_TREE=${SRCTOP}"
.endif

afterinstall:
	echo "FreeBSD/ZRouter.org project" > ${DESTDIR}/etc/motd
	echo "${FREEBSD_RELEASE}" >> ${DESTDIR}/etc/motd
	echo "Developed by Oleksandr Rybalko <ray@ddteam.net>, Hiroki Mori and many others" >> ${DESTDIR}/etc/motd
	echo "under sponsorship from N Possible (npossible.net)" >> ${DESTDIR}/etc/motd
	echo "and D-Link International Pte Ltd Ukraine." >> ${DESTDIR}/etc/motd
	echo "" >> ${DESTDIR}/etc/motd
	echo "Welcome to FreeBSD!" >> ${DESTDIR}/etc/motd

.include <bsd.prog.mk>

