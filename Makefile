
.include <bsd.own.mk>

FREEBSD_SRC_TREE?=/usr/src

# ZROUTER_ROOT can be set in environment
.if !defined(ZROUTER_ROOT)
ZROUTER_ROOT=${.CURDIR}
.endif

# ZROUTER_OBJ can be set in environment
ZROUTER_OBJ?=/usr/obj/${ZROUTER_ROOT}
MAKEOBJDIRPREFIX?=/usr/obj/${ZROUTER_ROOT}/
KERNELBUILDDIR?=${ZROUTER_OBJ}/kernel
KERNELCONFDIR?=${ZROUTER_OBJ}/conf
KERNELDESTDIR=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs
WORLDDESTDIR=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs
BLACKHOLEDIR=/../${TARGET_VENDOR}_${TARGET_DEVICE}_blackhole/
SRCROOTUP!=${ZROUTER_ROOT}/tools/rootup.sh ${FREEBSD_SRC_TREE}

# Board configyration must define used SoC/CPU
.include "boards/boards.mk"

# Set SoC defaults based on SOC_VENDOR/SOC_CHIP
.include "socs/socs.mk"

# Profiles - set of SUBDIRS that need to build
.include "profiles/profiles.mk"
#.endif

.if !defined(TARGET) || !defined(TARGET_ARCH)
.error "soc.mk must define both TARGET and TARGET_ARCH"
.endif


basic-tools:
	ZROUTER_ROOT=${ZROUTER_ROOT} MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} -f ${ZROUTER_ROOT}/Makefile.tools

basic-tools-clean:
	ZROUTER_ROOT=${ZROUTER_ROOT} MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} -f ${ZROUTER_ROOT}/Makefile.tools clean

${KERNELCONFDIR}:
	mkdir -p ${KERNELCONFDIR}


# Generate kernel config file
KERNEL_HINTS_FILE?=${KERNELCONFDIR}/${TARGET_VENDOR}_${TARGET_DEVICE}.hints
KERNEL_CONFIG_FILE?=${KERNELCONFDIR}/${TARGET_VENDOR}_${TARGET_DEVICE}

kernelconfig:	${TARGET_SOCDIR}/${SOC_KERNCONF} ${KERNELCONFDIR}
	@echo "# Kernel config for ${SOC_CHIP} on ${TARGET_VENDOR} ${TARGET_DEVICE} board" > ${KERNEL_CONFIG_FILE}
	@echo "machine	${KERNCONF_MACHINE}" >> ${KERNEL_CONFIG_FILE}
	@echo "ident	${KERNCONF_IDENT}" >> ${KERNEL_CONFIG_FILE}
	@echo "cpu	${KERNCONF_CPU}" >> ${KERNEL_CONFIG_FILE}
	@echo "hints	\"${KERNEL_HINTS_FILE}\"" >> ${KERNEL_CONFIG_FILE}
	@echo "# makeoptions section" >> ${KERNEL_CONFIG_FILE}
.for makeoption in ${KERNCONF_MAKEOPTIONS}
	@echo "makeoptions	${makeoption}" >> ${KERNEL_CONFIG_FILE}
.endfor
	@echo "# files section" >> ${KERNEL_CONFIG_FILE}
.for file in ${KERNCONF_FILES}
	@echo "files	\"${file}\"" >> ${KERNEL_CONFIG_FILE}
.endfor
	@echo "# options section" >> ${KERNEL_CONFIG_FILE}
.for option in ${KERNCONF_OPTIONS}
	@echo "options	${option}" >> ${KERNEL_CONFIG_FILE}
.endfor
	@echo "# devices section" >> ${KERNEL_CONFIG_FILE}
.for device in ${KERNCONF_DEVICES}
	@echo "device	${device}" >> ${KERNEL_CONFIG_FILE}
.endfor

# Generate .hints file
# TODO: generate hints on MAP partiotion list, GPIO usege list 
.if exists(${TARGET_SOCDIR}/soc.hints)
_SOC_HINTS=${TARGET_SOCDIR}/soc.hints
.endif
.if exists(${TARGET_BOARDDIR}/board.hints)
_DEVICE_HINTS=${TARGET_BOARDDIR}/board.hints
.endif

kernelhints:	${_SOC_HINTS} ${_DEVICE_HINTS} ${KERNELCONFDIR}
	cat /dev/null ${_SOC_HINTS} ${_DEVICE_HINTS} > ${KERNEL_HINTS_FILE}

# TODO: make dtd file for FDT
#

_KERNEL_BUILD_ENV= \
	TARGET=${TARGET} \
	TARGET_ARCH=${TARGET_ARCH} \
	TARGET_CPUARCH=${TARGET} \
	ZROUTER_ROOT=${ZROUTER_ROOT} \
	-DNO_CLEAN


kernel-toolchain:
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} kernel-toolchain

kernel-build:	kernelconfig kernelhints
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} KERNCONF=${KERNEL_CONFIG_FILE} buildkernel

#XXX_BEGIN Only for testing
KMODOWN!=id -u -n
KMODGRP!=id -g -n
_KERNEL_BUILD_ENV+="KMODOWN=${KMODOWN}"
_KERNEL_BUILD_ENV+="KMODGRP=${KMODGRP}"
#XXX_END Only for testing

kernel-install:
.if !empty(KERNELDESTDIR)
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} DESTDIR=${KERNELDESTDIR} KERNCONF=${KERNEL_CONFIG_FILE} installkernel
.else
.error "KERNELDESTDIR must be set for kernel-install, since we always do cross-build"
.endif

kernel:	kernel-toolchain kernel-build kernel-install
.ORDER:	kernel-toolchain kernel-build kernel-install



_WORLD_BUILD_ENV= \
	TARGET=${TARGET} \
	TARGET_ARCH=${TARGET_ARCH} \
	TARGET_CPUARCH=${TARGET} \
	ZROUTER_ROOT=${ZROUTER_ROOT} \
	WITHOUT_INFO=yes \
	WITHOUT_LOCALES=yes \
	WITHOUT_MAN=yes \
	WITHOUT_NLS=yes \
	WITHOUT_PROFILE=yes \
	WITHOUT_RESCUE=yes \
	-DNO_CLEAN

.if !defined(INSTALL_MAN)
_WORLD_BUILD_ENV+= WITHOUT_MAN=yes
.endif
.if !defined(INSTALL_INFO)
_WORLD_BUILD_ENV+= WITHOUT_INFO=yes
.endif
.if !defined(INSTALL_NLS)
_WORLD_BUILD_ENV+= WITHOUT_NLS=yes
.endif

_WORLD_INSTALL_ENV+="NO_STATIC_LIB=yes"
_WORLD_INSTALL_ENV+="WITHOUT_TOOLCHAIN=yes"

.if !defined(INSTALL_DOC)
_WORLD_INSTALL_ENV+= DOCDIR=${BLACKHOLEDIR}
.endif


#XXX_BEGIN Only for testing
OWN!=id -u -n
GRP!=id -g -n
_WORLD_INSTALL_ENV+="BINOWN=${OWN}"
_WORLD_INSTALL_ENV+="BINGRP=${GRP}"
_WORLD_INSTALL_ENV+="LIBOWN=${OWN}"
_WORLD_INSTALL_ENV+="LIBGRP=${GRP}"
_WORLD_INSTALL_ENV+="MANOWN=${OWN}"
_WORLD_INSTALL_ENV+="MANGRP=${GRP}"
#XXX_END Only for testing

.for dir in ${WORLD_SUBDIRS_BIN}
WORLD_SUBDIRS+=bin/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_SBIN}
WORLD_SUBDIRS+=sbin/${dir}
.endfor

.for lib in ${WORLD_SUBDIRS_LIB}
WORLD_SUBDIRS+=lib/${lib}
.endfor

.for dir in ${WORLD_SUBDIRS_USRBIN}
WORLD_SUBDIRS+=usr.bin/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_USRSBIN}
WORLD_SUBDIRS+=usr.sbin/${dir}
.endfor

# Project local tools
.for dir in ${WORLD_SUBDIRS_ZROUTER}
# Prepend reverse path, then buildworld can go out of source tree
WORLD_SUBDIRS+=${SRCROOTUP}/${ZROUTER_ROOT}/${dir}
.endfor




blackhole:
	mkdir -p ${WORLDDESTDIR}${BLACKHOLEDIR}
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} DESTDIR=${WORLDDESTDIR}${BLACKHOLEDIR} -C ${FREEBSD_SRC_TREE} hierarchy

world-toolchain:
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} -C ${FREEBSD_SRC_TREE} toolchain

world-build:
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} SUBDIR_OVERRIDE="${WORLD_SUBDIRS}" -C ${FREEBSD_SRC_TREE} buildworld

FREEBSD_BUILD_ENV_VARS!=(MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} -C ${FREEBSD_SRC_TREE} buildenvvars)
# that maybe used for any platform
# we need only say cross-build to configure
PORTS_CONFIGURE_TARGET=--build=i386-portbld-freebsd8 --host=mipsel-portbld-freebsd81

# Import buildenvvars into our namespace with suffix FREEBSD_BUILD_ENV_
.for var in ${FREEBSD_BUILD_ENV_VARS}
FREEBSD_BUILD_ENV_${var}
.endfor


# Ports
port-build:
	@echo "Start PORTNAME port building..."
.for dir in ${WORLD_SUBDIRS_PORTS}
.for dep in EXTRACT PATCH FETCH BUILD
# Run build with hostenv
${dir}_${dep}_DEPENDS!=cd ${dir} ; ${MAKE} -V${dep}_DEPENDS
.warning "${dir}_${dep}_DEPENDS = ${${dir}_${dep}_DEPENDS}"
.endfor
.for dep in LIB RUN
# Run build with crossenv
${dir}_${dep}_DEPENDS!=cd ${dir} ; ${MAKE} -V${dep}_DEPENDS
_DEP=${${dir}_${dep}_DEPENDS}
.warning ${dir}_${dep}_DEPENDS test if not ${_DEP:C,^([^:]*):.*$,\1,} then build ${_DEP:C,^[^:]*:([^:]*).*$,\1,}
.endfor
.endfor




oldport-build:
.for dir in ${WORLD_SUBDIRS_PORTS}
	@echo "----> <PORTS> Making port ${dir}"
#PORT_ALLDEP!=(cd ${dir} ; ${MAKE} all-depends-list)
#PORT_RUNDEP!=(cd ${dir} ; ${MAKE} run-depends-list)
#__PORT_RUNDEPF!=(echo ${PORT_RUNDEP} | sed 's/\s+/\|/g')
#PORT_BUILDDEP!=(echo ${PORT_ALLDEP} | perl -pe 's/\b(${__PORT_RUNDEPF})\b//g')
#.error ALL=${PORT_ALLDEP} BUILD=${PORT_BUILDDEP} RUN=${PORT_RUNDEP} "(echo ${PORT_ALLDEP} | perl -pe 's|\b(${__PORT_RUNDEPF})\b||g')"
	cd ${dir} ; \
		PATH=${FREEBSD_BUILD_ENV_PATH} \
		PREFIX=${WORLDDESTDIR}${BLACKHOLEDIR} \
		LOCALBASE=${WORLDDESTDIR}${BLACKHOLEDIR} \
		GLOBAL_CONFIGURE_ARGS="${PORTS_CONFIGURE_TARGET}" \
		ALWAYS_BUILD_DEPENDS=yes \
		WITHOUT_CHECK=yes \
		NO_PKG_REGISTER=yes \
		    ${MAKE} install clean
.endfor


world-install:		blackhole
	sudo -E MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} ${_WORLD_INSTALL_ENV} SUBDIR_OVERRIDE="${WORLD_SUBDIRS}" \
		DESTDIR=${WORLDDESTDIR} -C ${FREEBSD_SRC_TREE} installworld
## Ports
#.for dir in ${WORLD_SUBDIRS_PORTS}
#WORLD_SUBDIRS+=${SRCROOTUP}/${dir}
#.endfor

world:  world-toolchain world-build world-install
.ORDER: world-toolchain world-build world-install



.if defined(KERNEL_COMPRESSION)
kernel_image:	kernel_deflate kernel
.else
kernel_image:	kernel-build
.endif


.if target(bootloader_image)
FIRMWARE_IMAGE_DEPEND+=bootloader_image
.endif
.if target(config_image)
FIRMWARE_IMAGE_DEPEND+=config_image
.endif
.if target(kernel_image)
FIRMWARE_IMAGE_DEPEND+=kernel_image
.endif
.if target(rootfs_image)
FIRMWARE_IMAGE_DEPEND+=rootfs_image
.endif

firmware_image: ${FIRMWARE_IMAGE_DEPEND}

image: firmware_image

buildimage:	${BUILD_IMAGE_DEPEND}




all:	kernel	world




.include <bsd.obj.mk>
.include <bsd.subdir.mk>











