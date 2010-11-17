
.include <bsd.own.mk>

FREEBSD_SRC_TREE?=/usr/src

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
KERNELDESTDIR=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs

#.if ${.TARGET:C/build//:C/install//} == "world"
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




all:	kernel




.include <bsd.obj.mk>
.include <bsd.subdir.mk>











