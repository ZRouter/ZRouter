
.include <bsd.own.mk>

DEBUG?=@echo ""
FREEBSD_SRC_TREE?=/usr/src

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

buildimage:		${BUILD_IMAGE_DEPEND}

basic-tools:
	${MAKE} -f Makefile.tools

basic-tools-clean:
	${MAKE} -f Makefile.tools clean

${KERNELCONFDIR}:
	mkdir -p ${KERNELCONFDIR}

KERNEL_CONFIG_DEFSTR=
.for def in ${KERNEL_CONFIG_DEFS}
KERNEL_CONFIG_DEFSTR+=-D${def}
.endfor

# Generate kernel config file
KERNEL_HINTS_FILE?=${KERNELCONFDIR}/${TARGET_VENDOR}_${TARGET_DEVICE}.hints
KERNEL_CONFIG_FILE?=${KERNELCONFDIR}/${TARGET_VENDOR}_${TARGET_DEVICE}

kernelconfig:	${TARGET_SOCDIR}/${SOC_KERNCONF} ${KERNELCONFDIR}
	@echo -n "# Kernel config for ${SOC_CHIP} on ${TARGET_VENDOR} ${TARGET_DEVICE} board" > ${KERNEL_CONFIG_FILE}
	@echo "machine	${KERNCONF_MACHINE}" >> ${KERNEL_CONFIG_FILE}
	@echo "ident	${KERNCONF_IDENT}" >> ${KERNEL_CONFIG_FILE}
	@echo "cpu	${KERNCONF_CPU}" >> ${KERNEL_CONFIG_FILE}
	@echo "hints	\"${KERNEL_HINTS_FILE}\"" >> ${KERNEL_CONFIG_FILE}
.for file in ${KERNCONF_FILES}
	@echo "files	\"${file}\"" >> ${KERNEL_CONFIG_FILE}
.endfor
.for option in ${KERNCONF_OPTIONS}
	@echo "options	\"${option}\"" >> ${KERNEL_CONFIG_FILE}
.endfor
.for makeoption in ${KERNCONF_MAKEOPTIONS}
	@echo "makeoptions	${makeoption}" >> ${KERNEL_CONFIG_FILE}
.endfor
.for device in ${KERNCONF_DEVICES}
	@echo "device	${device}" >> ${KERNEL_CONFIG_FILE}
.endfor


# Generate .hints file
# TODO: generate hints on MAP partiotion list, GPIO usege list 
.if exists(${TARGET_SOCDIR}/soc.hints)
_SOC_HINTS=${TARGET_SOCDIR}/soc.hints
.endif
.if exists(${TARGET_DEVICEDIR}/board.hints)
_DEVICE_HINTS=${TARGET_DEVICEDIR}/board.hints
.endif

kernelhints:	${_SOC_HINTS} ${_DEVICE_HINTS} ${KERNELCONFDIR}
	cat ${_SOC_HINTS} ${_DEVICE_HINTS} > ${KERNEL_HINTS_FILE}

# Generate .hints file
# TODO: generate hints on MAP partiotion list, GPIO usege list 



.if target(tools/config/config)
SUBDIR=tools/config
.endif

kernel:		kernelconfig kernelhints tools/config/config
	tools/config/config -s ${FREEBSD_SRC_TREE} -d "${KERNELBUILDDIR}" ${KERNEL_CONFIG_FILE}


.if defined(KERNEL_COMPRESSION)
kernel_image:	kernel_deflate kernel
.else
kernel_image:	kernel
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



all:	kernel




.include <bsd.obj.mk>
.include <bsd.subdir.mk>











