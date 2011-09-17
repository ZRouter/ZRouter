
.include <bsd.own.mk>

FREEBSD_SRC_TREE?=/usr/src
OBJ_DIR?=/usr/obj

# ZROUTER_ROOT can be set in environment
.if !defined(ZROUTER_ROOT)
ZROUTER_ROOT=${.CURDIR}
.endif

# ZROUTER_OBJ can be set in environment
ZROUTER_OBJ?=${OBJ_DIR}/${ZROUTER_ROOT}
MAKEOBJDIRPREFIX?=${OBJ_DIR}/${ZROUTER_ROOT}/
KERNELBUILDDIR?=${ZROUTER_OBJ}/kernel
KERNELCONFDIR?=${ZROUTER_OBJ}/conf
KERNELDESTDIR=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs
WORLDDESTDIR=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs
SRCROOTUP!=${ZROUTER_ROOT}/tools/rootup.sh ${FREEBSD_SRC_TREE}

# XXX Need found something better or use per profile
ROOTFS_DEPTEST?=${WORLDDESTDIR}/bin/sh

KERNEL_SIZE_MAX!=sh -c 'echo $$((8 * 1024 * 1024))'
PREINSTALLDIRS=/lib

# Board configuration must define used SoC/CPU
.include "boards/boards.mk"

# Set SoC defaults based on SOC_VENDOR/SOC_CHIP
.include "socs/socs.mk"

# Profiles - set of SUBDIRS that need to build
.include "profiles/profiles.mk"
#.endif


menu:
	@/usr/bin/env ZROUTER_ROOT="${ZROUTER_ROOT}" ${ZROUTER_ROOT}/menu.sh

build-verify:
.if !exists(${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/) || !exists(${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/${TARGET_DEVICE}/)
	@echo "Error: No board configuration for pair TARGET_VENDOR/TARGET_DEVICE \`\"${TARGET_VENDOR}\"/\"${TARGET_DEVICE}\"\`"
	@echo "Possible pairs: ${TARGET_PAIRS}"
	@exit 1
.endif
.if !defined(TARGET) || !defined(TARGET_ARCH) || !defined(TARGET_SOCDIR)
	@echo "Error: No board configuration for pair SOC_VENDOR/SOC_CHIP \`\"${SOC_VENDOR}\"/\"${SOC_CHIP}\"\`"
	@echo "Possible pairs: ${SOC_PAIRS}"
	@exit 1
.endif

build-info:
	@echo "++++++++++++++ Selected settings for building ++++++++++++++"
	@echo "Device: ${TARGET_DEVICE}"
	@echo "Vendor: ${TARGET_VENDOR}"
	@echo "SoC Chip: ${SOC_CHIP}"
	@echo "SoC Vendor: ${SOC_VENDOR}"
	@echo

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
	echo "# Kernel config for ${SOC_CHIP} on ${TARGET_VENDOR} ${TARGET_DEVICE} board" > ${KERNEL_CONFIG_FILE}
	echo "machine	${KERNCONF_MACHINE}" >> ${KERNEL_CONFIG_FILE}
	echo "ident	${KERNCONF_IDENT}" >> ${KERNEL_CONFIG_FILE}
.for cpu in ${KERNCONF_CPU}
	echo "cpu	${cpu}" >> ${KERNEL_CONFIG_FILE}
.endfor
	echo "hints	\"${KERNEL_HINTS_FILE}\"" >> ${KERNEL_CONFIG_FILE}
	echo "# makeoptions section" >> ${KERNEL_CONFIG_FILE}
	if [ "x${KERNCONF_MODULES_OVERRIDE}" = "xnone" ] ; then \
		echo "makeoptions	MODULES_OVERRIDE=\"\""  >> ${KERNEL_CONFIG_FILE} ; \
	else \
		if [ "x${KERNCONF_MODULES_OVERRIDE}" != "x" ] ; then \
		echo "makeoptions	MODULES_OVERRIDE=\"${KERNCONF_MODULES_OVERRIDE}\""  >> ${KERNEL_CONFIG_FILE} ; \
		fi ; \
	fi
.for makeoption in ${KERNCONF_MAKEOPTIONS}
	echo "makeoptions	${makeoption}" >> ${KERNEL_CONFIG_FILE}
.endfor
	echo "# files section" >> ${KERNEL_CONFIG_FILE}
.for file in ${KERNCONF_FILES}
	echo "files	\"${file}\"" >> ${KERNEL_CONFIG_FILE}
.endfor
	echo "# options section" >> ${KERNEL_CONFIG_FILE}
.for option in ${KERNCONF_OPTIONS}
	echo "options	${option}" >> ${KERNEL_CONFIG_FILE}
.endfor
	echo "# devices section" >> ${KERNEL_CONFIG_FILE}
.for device in ${KERNCONF_DEVICES}
	echo "device	${device}" >> ${KERNEL_CONFIG_FILE}
.endfor
.for nodevice in ${KERNCONF_NODEVICES}
	echo "nodevice	${nodevice}" >> ${KERNEL_CONFIG_FILE}
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
	WITHOUT_RESCUE=yes \
	WITHOUT_CLANG=yes \
	-DNO_CLEAN


kernel-toolchain:
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} kernel-toolchain

${ZROUTER_OBJ}/tmp/${TARGET}.${TARGET_ARCH}/${FREEBSD_SRC_TREE}/tmp/usr/bin/cc:	kernel-toolchain

kernel-build:	kernelconfig kernelhints ${ZROUTER_OBJ}/tmp/${TARGET}.${TARGET_ARCH}/${FREEBSD_SRC_TREE}/tmp/usr/bin/cc
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} KERNCONF=${KERNEL_CONFIG_FILE} buildkernel

#XXX_BEGIN Only for testing
KMODOWN!=id -u -n
KMODGRP!=id -g -n
_KERNEL_BUILD_ENV+=KMODOWN=${KMODOWN}
_KERNEL_BUILD_ENV+=KMODGRP=${KMODGRP}
#XXX_END Only for testing

kernel:	build-verify build-info kernel-toolchain kernel-build kernel-install-dir kernel-install
.ORDER:	build-verify build-info kernel-toolchain kernel-build kernel-install-dir kernel-install


_WORLD_TCBUILD_ENV= \
	TARGET=${TARGET} \
	TARGET_ARCH=${TARGET_ARCH} \
	TARGET_CPUARCH=${TARGET} \
	ZROUTER_ROOT=${ZROUTER_ROOT} \
	WITHOUT_ATM=yes \
	WITHOUT_INFO=yes \
	WITHOUT_IPX=yes \
	WITHOUT_LOCALES=yes \
	WITHOUT_MAN=yes \
	WITHOUT_NLS=yes \
	WITHOUT_PROFILE=yes \
	WITHOUT_RESCUE=yes \
	WITHOUT_CDDL=yes \
	WITHOUT_CLANG=yes \
	WITHOUT_CRYPTO=yes \
	WITHOUT_NIS=yes \
	WITHOUT_KERBEROS=yes \
	-DNO_CLEAN

_WORLD_BUILD_ENV= \
	TARGET=${TARGET} \
	TARGET_ARCH=${TARGET_ARCH} \
	TARGET_CPUARCH=${TARGET} \
	ZROUTER_ROOT=${ZROUTER_ROOT} \
	WITHOUT_ASSERT_DEBUG=yes \
	WITHOUT_ATM=yes \
	WITHOUT_CLANG=yes \
	WITHOUT_INFO=yes \
	WITHOUT_INSTALLLIB=yes \
	WITHOUT_IPX=yes \
	WITHOUT_LOCALES=yes \
	WITHOUT_MAN=yes \
	WITHOUT_NLS=yes \
	WITHOUT_PROFILE=yes \
	WITHOUT_RESCUE=yes \
	WITHOUT_SSP=yes \
	-DNO_CLEAN


.if !defined(JAIL_ENABLE)
_WORLD_BUILD_ENV+= WITHOUT_JAIL=yes
.endif
.if !defined(IPV6_ENABLE)
_WORLD_BUILD_ENV+= WITHOUT_INET6=yes WITHOUT_INET6_SUPPORT=yes
.endif
.if !defined(INSTALL_MAN)
_WORLD_BUILD_ENV+= WITHOUT_MAN=yes
.endif
.if !defined(INSTALL_INFO)
_WORLD_BUILD_ENV+= WITHOUT_INFO=yes
.endif
.if !defined(INSTALL_NLS)
_WORLD_BUILD_ENV+= WITHOUT_NLS=yes
.endif

.if !defined(INSTALL_TOOLCHAIN)
_WORLD_INSTALL_ENV+=NO_STATIC_LIB=yes
_WORLD_INSTALL_ENV+=WITHOUT_TOOLCHAIN=yes
.endif

_WORLD_BUILD_ENV+=WITHOUT_CDDL=yes

_WORLD_BUILD_ENV+=WITHOUT_NIS=yes

_WORLD_BUILD_ENV+=WITHOUT_BLUETOOTH=yes

_WORLD_BUILD_ENV+=NOENABLE_WIDEC=yes -DNOENABLE_WIDEC

_WORLD_BUILD_ENV+=WITHOUT_KERBEROS=yes
_WORLD_BUILD_ENV+=WITHOUT_KERBEROS_SUPPORT=yes


#XXX_BEGIN Only for testing
OWN!=id -u -n
GRP!=id -g -n
_WORLD_INSTALL_ENV+=BINOWN=${OWN}
_WORLD_INSTALL_ENV+=BINGRP=${GRP}
_WORLD_INSTALL_ENV+=LIBOWN=${OWN}
_WORLD_INSTALL_ENV+=LIBGRP=${GRP}
_WORLD_INSTALL_ENV+=MANOWN=${OWN}
_WORLD_INSTALL_ENV+=MANGRP=${GRP}
_WORLD_INSTALL_ENV+=INSTALL="sh ${ZROUTER_ROOT}/tools/install.sh"
#XXX_END Only for testing

WORLD_SUBDIRS+=include

.for lib in ${WORLD_SUBDIRS_LIB}
WORLD_SUBDIRS+=lib/${lib}
.endfor

.for dir in ${WORLD_SUBDIRS_BIN}
WORLD_SUBDIRS+=bin/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_SBIN}
WORLD_SUBDIRS+=sbin/${dir}
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
#.warning ${FREEBSD_BUILD_ENV_VARS}
#FREEBSD_BUILD_ENV_VARS_SECOND!=${FREEBSD_BUILD_ENV_VARS}
#.warning ${FREEBSD_BUILD_ENV_VARS_SECOND}

# Import buildenvvars into our namespace with suffix FREEBSD_BUILD_ENV_
#.for var in ${FREEBSD_BUILD_ENV_VARS_SECOND}
.for var in ${FREEBSD_BUILD_ENV_VARS}
FREEBSD_BUILD_ENV_${var}
.endfor


.if ${MACHINE} == ${TARGET} && ${MACHINE_ARCH} == ${TARGET_ARCH} && !defined(CROSS_BUILD_TESTING)
TARGET_ARCH_SUBDIR=	""
.else
TARGET_ARCH_SUBDIR=	${TARGET}.${TARGET_ARCH}
.endif


#
# World
#
world-toolchain:
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_TCBUILD_ENV} -C ${FREEBSD_SRC_TREE} toolchain

world-build:	${ZROUTER_OBJ}/tmp/${TARGET}.${TARGET_ARCH}/${FREEBSD_SRC_TREE}/tmp/usr/bin/cc
	@echo "XXX: need to find a way to install required includes correctly"
	mkdir -p ${ZROUTER_OBJ}/tmp/${TARGET_ARCH_SUBDIR}/${FREEBSD_SRC_TREE}/tmp/usr/include/lzo
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} SUBDIR_OVERRIDE="${WORLD_SUBDIRS}" -C ${FREEBSD_SRC_TREE} buildworld

world-install: rootfs-dir
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} ${_WORLD_INSTALL_ENV} SUBDIR_OVERRIDE="${WORLD_SUBDIRS}" \
		DESTDIR=${WORLDDESTDIR} -C ${FREEBSD_SRC_TREE} installworld

world-fix-lib-links:
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	cd ${WORLDDESTDIR}/usr/lib/ && ${ZROUTER_ROOT}/tools/fix_lib_links.sh

world:  build-verify build-info world-toolchain world-build world-install world-fix-lib-links
.ORDER: build-verify build-info world-toolchain world-build world-install world-fix-lib-links

.include "share/mk/zrouter.ports.mk"

rootfs-dir:
	mkdir -p ${WORLDDESTDIR}
	mkdir -p ${WORLDDESTDIR}/usr/lib/lua/
	for dir in ${PREINSTALLDIRS}; do mkdir -p ${WORLDDESTDIR}/$${dir}; done

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
IMAGE_SUFFIX?=trx
ZTOOLS_PATH=${ZROUTER_OBJ}/ztools
NEW_KERNEL=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_kernel
NEW_ROOTFS=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean
NEW_IMAGE=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}.${IMAGE_SUFFIX}

IMAGE_BUILD_PATHS=${ZTOOLS_PATH}:${FREEBSD_BUILD_ENV_PATH}

.include "share/mk/zrouter.local.tools.mk"
.include "share/mk/zrouter.base.tools.mk"

.if !defined(ROOTFS_WITH_KERNEL)
_FIND_MATCH_KERNEL=-name kernel -or
.endif

ROOTFS_RMLIST= \
    \\( \\( -type f -or -type l \\) -and \
	\\( ${_FIND_MATCH_KERNEL} -name "*.a" -or -name "crt*.o" \\) \\) -or \
    \\( -type l -and -name sys \\) -or \
    \\( -type d -and \\( \
    -name include -or \
    -name libdata -or \
    -name games -or \
    -name src -or \
    -name obj -or \
    -name info -or \
    -name man -or \
    -name zfs \\) \\)

ROOTFS_RMFILES+=calendar dict doc examples groff_font locale me mk nls openssl \
	pc-sysinstall security sendmail skel syscons tabset tmac vi zoneinfo

# Move kernel out of rootfs
#		world kernel ports
rootfs:		${KERNELDESTDIR}/boot/kernel/kernel ${ROOTFS_DEPTEST}
	for d in ${ROOTFS_COPY_DIRS} ; do \
		for f in `( cd $${d} ; find . -type f )` ; do \
			mkdir -p `dirname ${WORLDDESTDIR}/$${f}` ; \
			cp $${d}/$${f} ${WORLDDESTDIR}/$${f} ; \
		done ; \
	done
	rm -f ${NEW_KERNEL}
	cp ${KERNELDESTDIR}/boot/kernel/kernel ${NEW_KERNEL}
	rm -rf ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean
	cp -R ${WORLDDESTDIR} ${NEW_ROOTFS}
	rm -rf `find ${NEW_ROOTFS} ${ROOTFS_RMLIST}`
	for file in ${ROOTFS_RMFILES} ; do rm -rf ${NEW_ROOTFS}/usr/share/$${file} ; done
	rm -rf ${NEW_ROOTFS}/var
	ln -s /tmp/var ${NEW_ROOTFS}/var
	rm -f ${NEW_ROOTFS}/etc/resolv.conf
	ln -s /tmp/etc/resolv.conf ${NEW_ROOTFS}/etc/resolv.conf
	cd ${NEW_ROOTFS}/bin/ ; \
	 ln -sf rm unlink ; \
	 ln -sf ln link
	cd ${NEW_ROOTFS}/sbin/ ; \
	 ln -sf reboot halt ; \
	 ln -sf reboot fastboot ; \
	 ln -sf reboot fasthalt ; \
	 ln -sf md5 rmd160 ; \
	 ln -sf md5 sha1 ; \
	 ln -sf md5 sha256
	cd ${NEW_ROOTFS}/usr/bin/ ; \
	 ln -sf id groups ; \
	 ln -sf id whoami ; \
	 ln -sf bsdgrep grep ; \
	 ln -sf bsdgrep egrep ; \
	 ln -sf bsdgrep fgrep ; \
	 ln -sf bsdgrep zgrep ; \
	 ln -sf bsdgrep zegrep ; \
	 ln -sf bsdgrep zfgrep ; \
	 ln -sf ssh slogin ; \
	 ln -sf vi nvi ; \
	 ln -sf vi ex ; \
	 ln -sf vi nex ; \
	 ln -sf vi view ; \
	 ln -sf vi nview
	rm -rf ${NEW_ROOTFS}/etc/mpd
	ln -s /tmp/etc/mpd ${NEW_ROOTFS}/etc/mpd
	cd ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean ; find ./usr/ -type d -empty -delete

#${ROOTFS_DEPTEST}:
${ROOTFS_DEPTEST}:		world	ports
	@echo "++++++++++++++ Making $@ ++++++++++++++"

${ZROUTER_OBJ}/tmp/${TARGET}.${TARGET_ARCH}/${FREEBSD_SRC_TREE}/sys/${KERNEL_CONFIG_FILE}/kernel:	kernel-build
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	echo "XXXXXXXXXXXXX ${ZROUTER_OBJ}/tmp/${TARGET}.${TARGET_ARCH}/${FREEBSD_SRC_TREE}/sys/${KERNEL_CONFIG_FILE}/kernel"

kernel-install:				${KERNELDESTDIR}/boot/kernel/kernel

${KERNELDESTDIR}/boot/kernel/kernel:	${ZROUTER_OBJ}/tmp/${TARGET}.${TARGET_ARCH}/${FREEBSD_SRC_TREE}/sys/${KERNEL_CONFIG_FILE}/kernel kernel-install-dir
	@echo "++++++++++++++ Making $@ ++++++++++++++"
.if !empty(KERNELDESTDIR)
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} DESTDIR=${KERNELDESTDIR} KERNCONF=${KERNEL_CONFIG_FILE} installkernel
.else
.error "KERNELDESTDIR must be set for kernel-install, since we always do cross-build"
.endif

${NEW_KERNEL}:		${KERNELDESTDIR}/boot/kernel/kernel
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	rm -f ${NEW_KERNEL}
	cp ${KERNELDESTDIR}/boot/kernel/kernel ${NEW_KERNEL}

rootfs.iso ${NEW_ROOTFS}.iso:	rootfs makefs_cd9660
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} makefs -d 255 -t cd9660 -F ${ZROUTER_ROOT}/tools/rootfs.mtree -o "rockridge" ${NEW_ROOTFS}.iso ${NEW_ROOTFS}

#.if ${TARGET_ARCH} == "armeb"
#ROOTFS_ENDIAN_FLAGS=-B big
#.endif

rootfs.ffs ${NEW_ROOTFS}.ffs:	rootfs makefs_ffs
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} makefs -t ffs -d 255 -F ${ZROUTER_ROOT}/tools/rootfs.mtree -s ${ROOTFS_MEDIA_SIZE} -o minfree=0,version=1 ${ROOTFS_ENDIAN_FLAGS} ${NEW_ROOTFS}.ffs ${NEW_ROOTFS}

#	blocks=$(($ROOTFS_MEDIA_SIZE / ${BLOCKSIZE} + 256))
#	dd if=/dev/zero of=${NEW_ROOTFS}.ffs bs=${BLOCKSIZE} count=${blocks}
#	if [ $? -ne 0 ]; then
#	  echo "creation of image file failed"
#	  exit 1
#	fi
#
#	unit=`mdconfig -a -t vnode -f ${NEW_ROOTFS}.ffs`
#	if [ $? -ne 0 ]; then
#	  echo "mdconfig failed"
#	  exit 1
#	fi
#
#	gpart create -s GPT ${unit}
#	gpart add -t freebsd-boot -s 64K ${unit}
#	gpart bootcode -b ${NEW_ROOTFS}/boot/pmbr -p ${NEW_ROOTFS}/boot/gptboot -i 1 ${unit}
#	gpart add -t freebsd-ufs -l FreeBSD_Install ${unit}
#
#	dd if=${tempfile} of=/dev/${unit}p2 bs=$BLOCKSIZE conv=sync
#	if [ $? -ne 0 ]; then
#	  echo "copying filesystem into image file failed"
#	  exit 1
#	fi
#
#	mdconfig -d -u ${unit}




MKULZMA_FLAGS?=-v
MKULZMA_BLOCKSIZE?=131072

oldlzma:	${ZTOOLS_PATH}/oldlzma
	@echo "++++++++++++++ Making $@ ++++++++++++++"

rootfs.iso.ulzma ${NEW_ROOTFS}.iso.ulzma:	rootfs.iso
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} mkulzma ${MKULZMA_FLAGS} -s ${MKULZMA_BLOCKSIZE} -o ${NEW_ROOTFS}.iso.ulzma ${NEW_ROOTFS}.iso

#
# Convert kernel from ELF to BIN
#
#kernel_bin 
${NEW_KERNEL}.bin:	${NEW_KERNEL}
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} objcopy -S -O binary ${NEW_KERNEL} ${NEW_KERNEL}.bin
	@if [ -n "${KERNEL_SIZE_MAX}" -a $$(stat -f %z ${NEW_KERNEL}.bin) -ge ${KERNEL_SIZE_MAX} ] ; then \
		echo "${NEW_KERNEL}.bin size ($$(stat -f %z ${NEW_KERNEL}.bin)) greater than KERNEL_SIZE_MAX (${KERNEL_SIZE_MAX}), will delete it"; \
		rm -f ${NEW_KERNEL}.bin ; \
		exit 1; \
	fi

#
# Compress kernel with oldlzma
#
kernel_bin_oldlzma:	${NEW_KERNEL}.bin.oldlzma

${NEW_KERNEL}.bin.oldlzma:	${NEW_KERNEL}.bin	${ZTOOLS_PATH}/oldlzma
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} oldlzma e ${OLDLZMA_COMPRESS_FLAGS} ${NEW_KERNEL}.bin ${NEW_KERNEL}.bin.oldlzma

kernel_oldlzma:		${NEW_KERNEL}.oldlzma

${NEW_KERNEL}.oldlzma:		${NEW_KERNEL}	${ZTOOLS_PATH}/oldlzma
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} oldlzma e ${OLDLZMA_COMPRESS_FLAGS} ${NEW_KERNEL} ${NEW_KERNEL}.oldlzma

#
# Compress kernel with xz
#
kernel_bin_xz ${NEW_KERNEL}.bin.xz:		${NEW_KERNEL}.bin
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} xz --stdout ${XZ_COMPRESS_FLAGS} ${NEW_KERNEL}.bin > ${NEW_KERNEL}.bin.xz

kernel_xz ${NEW_KERNEL}.xz:		${NEW_KERNEL}
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} xz --stdout ${XZ_COMPRESS_FLAGS} ${NEW_KERNEL} > ${NEW_KERNEL}.xz

#
# Compress kernel with bz2
#
kernel_bin_bz2 ${NEW_KERNEL}.bin.bz2:		${NEW_KERNEL}.bin
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} bzip2 --stdout ${BZIP2_COMPRESS_FLAGS} ${NEW_KERNEL}.bin > ${NEW_KERNEL}.bin.bz2

kernel_bz2 ${NEW_KERNEL}.bz2:		${NEW_KERNEL}
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} bzip2 --stdout ${BZIP2_COMPRESS_FLAGS} ${NEW_KERNEL} > ${NEW_KERNEL}.bz2

#
# Compress kernel with gz
#
kernel_bin_gz ${NEW_KERNEL}.bin.gz:		${NEW_KERNEL}.bin
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} gzip --stdout ${GZIP_COMPRESS_FLAGS} ${NEW_KERNEL}.bin > ${NEW_KERNEL}.bin.gz

kernel_gz ${NEW_KERNEL}.gz:		${NEW_KERNEL}
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} gzip --stdout ${GZIP_COMPRESS_FLAGS} ${NEW_KERNEL} > ${NEW_KERNEL}.gz

UBOOT_KERNEL_LOAD_ADDRESS?=80001000
UBOOT_KERNEL_ENTRY_POINT?=${UBOOT_KERNEL_LOAD_ADDRESS}

kernel.${KERNEL_COMPRESSION_TYPE}.uboot:	${NEW_KERNEL}.${KERNEL_COMPRESSION_TYPE}.uboot

${NEW_KERNEL}.${KERNEL_COMPRESSION_TYPE}.uboot: ${NEW_KERNEL}.bin.${KERNEL_COMPRESSION_TYPE}
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	uboot_mkimage -A ${TARGET} -O linux -T kernel \
	    -C ${UBOOT_KERNEL_COMPRESSION_TYPE} \
	    -a ${UBOOT_KERNEL_LOAD_ADDRESS} \
	    -e ${UBOOT_KERNEL_ENTRY_POINT} \
	    -n 'FreeBSD Kernel Image' \
	    -d ${NEW_KERNEL}.bin.${KERNEL_COMPRESSION_TYPE} \
	    ${NEW_KERNEL}.${KERNEL_COMPRESSION_TYPE}.uboot

#	${ZTOOLS_PATH}/packimage

kernel.${KERNEL_COMPRESSION_TYPE}.trx: kernel.${KERNEL_COMPRESSION_TYPE}	${ZTOOLS_PATH}/trx
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} trx -o kernel.${KERNEL_COMPRESSION_TYPE}.trx kernel.${KERNEL_COMPRESSION_TYPE}

# XXX: temporary
kernel_bin_gz_trx ${NEW_KERNEL}.bin.gz.trx: ${NEW_KERNEL}.bin.gz	${ZTOOLS_PATH}/trx
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} trx -o ${NEW_KERNEL}.bin.gz.trx ${NEW_KERNEL}.bin.gz

${NEW_KERNEL}.bin.gz.sync:	${NEW_KERNEL}.bin.gz
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	dd if=${NEW_KERNEL}.bin.gz of=${NEW_KERNEL}.bin.gz.sync bs=64k conv=sync

fwimage ${NEW_IMAGE}:  ${NEW_KERNEL}.bin.gz.sync ${NEW_ROOTFS}.iso.ulzma	${ZTOOLS_PATH}/asustrx
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} asustrx -o ${NEW_IMAGE} ${NEW_KERNEL}.bin.gz.sync ${NEW_ROOTFS}.iso.ulzma

# zimage used when it possible to use any formats (CFI devices must use trx 
# format, but U-Boot devices must use only kernel in U-Boot format )
ZROUTER_VERSION?=0.1-ALPHA

zimage:		${NEW_KERNEL}.${KERNEL_COMPRESSION_TYPE}.uboot.sync ${NEW_ROOTFS}.iso.ulzma
	cat ${NEW_KERNEL}.${KERNEL_COMPRESSION_TYPE}.uboot.sync ${NEW_ROOTFS}.iso.ulzma > ${NEW_IMAGE}
	IMGMD5=`md5 ${NEW_IMAGE} | cut -f4 -d' '` ; \
	cp ${NEW_IMAGE} ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}-${ZROUTER_VERSION}.$${IMGMD5}.${IMAGE_SUFFIX}

${NEW_KERNEL}.${KERNEL_COMPRESSION_TYPE}.uboot.sync:	${NEW_KERNEL}.${KERNEL_COMPRESSION_TYPE}.uboot
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	dd if=${NEW_KERNEL}.${KERNEL_COMPRESSION_TYPE}.uboot of=${NEW_KERNEL}.${KERNEL_COMPRESSION_TYPE}.uboot.sync bs=64k conv=sync

# Howto
# PACKING_KERNEL_ROUND=0x10000
# echo $(( (${KERNEL_SIZE} + ${PACKING_KERNEL_ROUND}) & ~(${PACKING_KERNEL_ROUND} - 1) ))

.include <bsd.obj.mk>











