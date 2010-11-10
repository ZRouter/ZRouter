
.include <bsd.own.mk>

DEBUG?=@echo ""
FREEBSD_SRC_TREE?=/usr/src

.CURDIR!=pwd

# ZROUTER_ROOT can be set in environment
.if !defined(ZROUTER_ROOT)
ZROUTER_ROOT=${.CURDIR}
.endif

# ZROUTER_OBJ can be set in environment
ZROUTER_OBJ?=/usr/obj/${ZROUTER_ROOT}
MAKEOBJDIRPREFIX?=/usr/obj/${ZROUTER_ROOT}
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


kernelconfig:	${TARGET_SOCDIR}/${SOC_KERNCONF} ${BOARD_KERNCONF}
	${DEBUG}cat "${TARGET_SOCDIR}/${SOC_KERNCONF}" "${BOARD_KERNCONF}" > "${KERNELCONFDIR}/${TARGET_VENDOR}_${TARGET_DEVICE}"


kernelhints:	${TARGET_SOCDIR}/${SOC_KERNHINTS} ${BOARD_KERNHINTS}
	${DEBUG}cat "${TARGET_SOCDIR}/${SOC_KERNHINTS}" "${BOARD_KERNHINTS}" > "${KERNELCONFDIR}/${TARGET_VENDOR}_${TARGET_DEVICE}.hints"

tools/config/config:
	${DEBUG}${MAKE} -C tools/config

kernel:		kernelconfig kernelhints tools/config/config
	${DEBUG}tools/config/config -s ${FREEBSD_SRC_TREE} -d "${KERNELBUILDDIR}" "${KERNELCONFDIR}/${TARGET_VENDOR}_${TARGET_DEVICE}"


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

.include <bsd.subdir.mk>











