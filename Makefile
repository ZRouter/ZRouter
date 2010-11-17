
.include <bsd.own.mk>

DEBUG?=@echo ""
FREEBSD_SRC_TREE?=/usr/src
CONFIG_TOOL?=config

#.CURDIR!=pwd

# ZROUTER_ROOT can be set in environment
.if !defined(ZROUTER_ROOT)
ZROUTER_ROOT=${.CURDIR}
.endif
#.warning ${ZROUTER_ROOT}

# ZROUTER_OBJ can be set in environment
ZROUTER_OBJ?=/usr/obj/${ZROUTER_ROOT}
MAKEOBJDIRPREFIX?=/usr/obj/${ZROUTER_ROOT}/
KERNELBUILDDIR?=${ZROUTER_OBJ}/kernel
KERNELCONFDIR?=${ZROUTER_OBJ}/conf

#.if ${.TARGET:C/build//:C/install//} == "world"
# Board configyration must define used SoC/CPU
.include "boards/boards.mk"

# Set SoC defaults based on SOC_VENDOR/SOC_CHIP
.include "socs/socs.mk"

# Profiles - set of SUBDIRS that need to build
.include "profiles/profiles.mk"
#.endif

.if !defined(TARGET) || !defined(TARGET_ARCH)
.warning "${TARGET}.${TARGET_ARCH}"
.error "soc.mk must define both TARGET and TARGET_ARCH"
.endif

FBSD_OBJ=${ZROUTER_OBJ}/tmp/${TARGET}.${TARGET_ARCH}/${FREEBSD_SRC_TREE}


buildimage:		${BUILD_IMAGE_DEPEND}

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

# Generate .hints file
# TODO: generate hints on MAP partiotion list, GPIO usege list 

kernel-config:		kernelconfig kernelhints ${FBSD_OBJ}/tmp/legacy/usr/sbin/config
	cd ${FREEBSD_SRC_TREE}/sys/${TARGET}/conf ; ${CONFIG_TOOL} -d ${KERNELBUILDDIR} ${KERNEL_CONFIG_FILE}

_KERNEL_BUILD_ENV= \
	MACHINE=${TARGET} \
	MACHINE_ARCH=${TARGET_ARCH} \
	MACHINE_CPUARCH=${TARGET} \
	TARGET=${TARGET} \
	TARGET_ARCH=${TARGET_ARCH} \
	TARGET_CPUARCH=${TARGET}

_KERNEL_BUILD_PATH=${FBSD_OBJ}/tmp/legacy/usr/sbin/:${FBSD_OBJ}/tmp/legacy/usr/bin/:${FBSD_OBJ}/tmp/legacy/usr/games/:${FBSD_OBJ}/tmp/usr/sbin/:${FBSD_OBJ}/tmp/usr/bin/:${FBSD_OBJ}/tmp/usr/games/:${PATH}


kernel-build:		kernel-config ${FBSD_OBJ}/tmp/usr/bin/cc
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} buildkernel

#	PATH=${_KERNEL_BUILD_PATH} ${MAKE} ${_KERNEL_BUILD_ENV} -C ${KERNELBUILDDIR} cleandepend
#	PATH=${_KERNEL_BUILD_PATH} ${MAKE} ${_KERNEL_BUILD_ENV} -C ${KERNELBUILDDIR} depend
#	PATH=${_KERNEL_BUILD_PATH} ${MAKE} ${_KERNEL_BUILD_ENV} -C ${KERNELBUILDDIR}


${FBSD_OBJ}/tmp/usr/bin/cc:	kernel-toolchain

${FBSD_OBJ}/tmp/legacy/usr/sbin/config:	kernel-toolchain

kernel-toolchain:
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} -DNO_CLEAN -C ${FREEBSD_SRC_TREE} kernel-toolchain

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



all:	kernel-build




.include <bsd.obj.mk>
.include <bsd.subdir.mk>











