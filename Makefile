
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
#	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} kernel-toolchain

kernel-build:	kernelconfig kernelhints
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} KERNCONF=${KERNEL_CONFIG_FILE} buildkernel

#XXX_BEGIN Only for testing
KMODOWN!=id -u -n
KMODGRP!=id -g -n
_KERNEL_BUILD_ENV+="KMODOWN=${KMODOWN}"
_KERNEL_BUILD_ENV+="KMODGRP=${KMODGRP}"
#XXX_END Only for testing

kernel-install: kernel-install-dir
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



#XXX_BEGIN Only for testing
OWN!=id -u -n
GRP!=id -g -n
_WORLD_INSTALL_ENV+="BINOWN=${OWN}"
_WORLD_INSTALL_ENV+="BINGRP=${GRP}"
_WORLD_INSTALL_ENV+="LIBOWN=${OWN}"
_WORLD_INSTALL_ENV+="LIBGRP=${GRP}"
_WORLD_INSTALL_ENV+="MANOWN=${OWN}"
_WORLD_INSTALL_ENV+="MANGRP=${GRP}"
_WORLD_INSTALL_ENV+=INSTALL="sh ${FREEBSD_SRC_TREE}/tools/install.sh"
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

.for dir in ${WORLD_SUBDIRS_USR_BIN}
WORLD_SUBDIRS+=usr.bin/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_USR_SBIN}
WORLD_SUBDIRS+=usr.sbin/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_LIBEXEC}
WORLD_SUBDIRS+=libexec/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_GNU_LIB}
WORLD_SUBDIRS+=gnu/lib/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_GNU_USR_BIN}
WORLD_SUBDIRS+=gnu/usr.bin/${dir}
.endfor


# Project local tools
.for dir in ${WORLD_SUBDIRS_ZROUTER}
# Prepend reverse path, then buildworld can go out of source tree
WORLD_SUBDIRS+=${SRCROOTUP}/${ZROUTER_ROOT}/${dir}
.endfor

FREEBSD_BUILD_ENV_VARS!=(MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} -C ${FREEBSD_SRC_TREE} buildenvvars)

# Import buildenvvars into our namespace with suffix FREEBSD_BUILD_ENV_
.for var in ${FREEBSD_BUILD_ENV_VARS}
FREEBSD_BUILD_ENV_${var}
.endfor




world-toolchain:
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} -C ${FREEBSD_SRC_TREE} toolchain

world-build:
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} SUBDIR_OVERRIDE="${WORLD_SUBDIRS}" -C ${FREEBSD_SRC_TREE} buildworld

world-install: rootfs-dir
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} ${_WORLD_INSTALL_ENV} SUBDIR_OVERRIDE="${WORLD_SUBDIRS}" \
		DESTDIR=${WORLDDESTDIR} -C ${FREEBSD_SRC_TREE} installworld

world:  world-toolchain world-build world-install
.ORDER: world-toolchain world-build world-install

.include "Mk/zrouter.ports.mk"

rootfs-dir:
	mkdir -p ${WORLDDESTDIR}

kernel-install-dir:
	mkdir -p ${KERNELDESTDIR}

ports: port-build


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


# XXX Must make makefs, mkulzma with [kernel-]toolchain + uboot_mkimage and old lzma ports 

all:	world kernel ports

IMAGE_BUILD_PATHS=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_ztools:${FREEBSD_BUILD_ENV_PATH}
NEW_KERNEL=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_kernel
NEW_ROOTFS=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean


ROOTFS_RMLIST= \
    \\( \\( -type f -or -type l \\) -and \
	    \\( -name kernel -or -name "*.a" \\) \\) -or \
    \\( -type l -and -name sys \\) -or \
    \\( -type d -and \\( \
    -name include -or \
    -name share -or \
    -name libdata -or \
    -name games -or \
    -name src -or \
    -name obj -or \
    -name info -or \
    -name man -or \
    -name zfs \\) \\)

# Move kernel out of rootfs
#		world kernel ports
rootfs:
	rm -f ${NEW_KERNEL}
	cp ${KERNELDESTDIR}/boot/kernel/kernel ${NEW_KERNEL}
	rm -rf ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean
	cp -R ${WORLDDESTDIR} ${NEW_ROOTFS}
	rm -rf `find ${NEW_ROOTFS} ${ROOTFS_RMLIST}`

rootfs.iso:	rootfs
	PATH=${IMAGE_BUILD_PATHS} makefs -t cd9660 -o "rockridge" ${NEW_ROOTFS}.iso ${NEW_ROOTFS}

MKULZMA_FLAGS?=-v
MKULZMA_BLOCKSIZE?=131072

rootfs.iso.ulzma:	rootfs.iso
	PATH=${IMAGE_BUILD_PATHS} mkulzma ${MKULZMA_FLAGS} -s ${MKULZMA_BLOCKSIZE} -o ${NEW_ROOTFS}.iso.ulzma ${NEW_ROOTFS}.iso

kernel_bin ${NEW_KERNEL}.bin:	${NEW_KERNEL}
	PATH=${IMAGE_BUILD_PATHS} objcopy -S -O binary ${NEW_KERNEL} ${NEW_KERNEL}.bin

kernel_bin_oldlzma ${NEW_KERNEL}.bin.oldlzma:	${NEW_KERNEL}.bin
	PATH=${IMAGE_BUILD_PATHS} oldlzma e ${OLDLZMA_COMPRESS_FLAGS} ${NEW_KERNEL}.bin ${NEW_KERNEL}.bin.oldlzma

kernel_oldlzma ${NEW_KERNEL}.oldlzma:		${NEW_KERNEL}
	PATH=${IMAGE_BUILD_PATHS} oldlzma e ${OLDLZMA_COMPRESS_FLAGS} ${NEW_KERNEL} ${NEW_KERNEL}.oldlzma

kernel_bin_xz ${NEW_KERNEL}.bin.xz:		${NEW_KERNEL}.bin
	PATH=${IMAGE_BUILD_PATHS} xz ${XZ_COMPRESS_FLAGS} ${NEW_KERNEL}.bin

kernel_xz ${NEW_KERNEL}.xz:		${NEW_KERNEL}
	PATH=${IMAGE_BUILD_PATHS} xz ${XZ_COMPRESS_FLAGS} ${NEW_KERNEL}

kernel_bin_bz2 ${NEW_KERNEL}.bin.bz2:		${NEW_KERNEL}.bin
	PATH=${IMAGE_BUILD_PATHS} bzip2 ${BZIP2_COMPRESS_FLAGS} ${NEW_KERNEL}.bin

kernel_bz2 ${NEW_KERNEL}.bz2:		${NEW_KERNEL}
	PATH=${IMAGE_BUILD_PATHS} bzip2 ${BZIP2_COMPRESS_FLAGS} ${NEW_KERNEL}

kernel_bin_gz ${NEW_KERNEL}.bin.gz:		${NEW_KERNEL}.bin
	PATH=${IMAGE_BUILD_PATHS} gzip ${GZIP_COMPRESS_FLAGS} ${NEW_KERNEL}.bin

kernel_gz ${NEW_KERNEL}.gz:		${NEW_KERNEL}
	PATH=${IMAGE_BUILD_PATHS} gzip ${GZIP_COMPRESS_FLAGS} ${NEW_KERNEL}

UBOOT_KERNEL_LOAD_ADDRESS=80001000
UBOOT_KERNEL_ENTRY_POINT=${UBOOT_KERNEL_LOAD_ADDRESS}


kernel.${KERNEL_COMPRESSION_TYPE}.uboot: kernel.${KERNEL_COMPRESSION_TYPE}
	uboot_mkimage -A ${TARGET} -O linux -T kernel \
	    -C ${UBOOT_KERNEL_COMPRESSION_TYPE} \
	    -a ${UBOOT_KERNEL_LOAD_ADDRESS} \
	    -e ${UBOOT_KERNEL_ENTRY_POINT} \
	    -n 'FreeBSD Kernel Image' \
	    -d ${KTFTP}/kernel.bin.${KERNEL_COMPRESSION_TYPE} \
	    ${KTFTP}/kernel.${KERNEL_COMPRESSION_TYPE}.uboot



.include <bsd.obj.mk>
#.include <bsd.subdir.mk>











