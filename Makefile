
.include <bsd.own.mk>

# ZROUTER_ROOT can be set in environment
.if !defined(ZROUTER_ROOT)
ZROUTER_ROOT=${.CURDIR}
.endif

.if exists(${ZROUTER_ROOT}/Makefile.local.opts)
.warning Using options from ${ZROUTER_ROOT}/Makefile.local.opts
.include "${ZROUTER_ROOT}/Makefile.local.opts"
.endif

#
# Defaults
#
FREEBSD_SRC_TREE?=/usr/src
ZOBJ_DIR?=/usr/obj
USE_SYSTEMTOOLS?=yes

#
# Check access to FreeBSD source tree and fetch version variables
#
.if exists(${FREEBSD_SRC_TREE}/sys/conf/newvers.sh)
FREEBSD_VERSION_VARS!=grep -E '(TYPE|REVISION|BRANCH)=\"' \
	${FREEBSD_SRC_TREE}/sys/conf/newvers.sh | sed 's/\"//g' | sed 's/^/FREEBSD_/'
.for var in ${FREEBSD_VERSION_VARS}
VAR_LEFT=${var:C/=.*//}
VAR_RIGHT=${var:C/.*=//}
${VAR_LEFT}:=${VAR_RIGHT}
.endfor
FREEBSD_RELEASE=${FREEBSD_TYPE}-${FREEBSD_REVISION}-${FREEBSD_BRANCH}
.else
.error "missing FreeBSD source tree: FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE}"
.endif

.if ${FREEBSD_REVISION:M12*} != ""
ZROUTER_COMPAT12=yes
.endif

# ZROUTER_OBJ can be set in environment
ZROUTER_OBJ?=${ZOBJ_DIR}/${ZROUTER_ROOT}
MAKEOBJDIRPREFIX?=${ZOBJ_DIR}/${ZROUTER_ROOT}/
KERNELBUILDDIR?=${ZROUTER_OBJ}/kernel
KERNELCONFDIR?=${ZROUTER_OBJ}/conf
KERNELDESTDIR=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs
WORLDDESTDIR=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs
SRCROOTUP!=${ZROUTER_ROOT}/tools/rootup.sh ${FREEBSD_SRC_TREE}
TARGET_CPUARCH?=${TARGET}


# XXX Need found something better or use per profile
ROOTFS_DEPTEST?=${WORLDDESTDIR}/bin/sh

KERNEL_SIZE_MAX!=sh -c 'echo $$((8 * 1024 * 1024))'
PREINSTALLDIRS=/lib

##############################################################################
# Board configuration must define used SoC/CPU
##############################################################################
.ifnmake show-target-pairs
.include "boards/boards.mk"
.endif

##############################################################################
# Local vendor changes
##############################################################################
.include "vendor/vendor.mk"

.if !defined(TARGET_BOARDDIR)
TARGET_PAIRS!=ls -d ${ZROUTER_ROOT}/boards/*/* | sed 's/^.*\/boards\///'
.endif

menu:
	@/usr/bin/env ZROUTER_ROOT="${ZROUTER_ROOT}" ${ZROUTER_ROOT}/menu.sh

show-target-pairs:
	@echo "${TARGET_PAIRS}"

##############################################################################
# Set SoC defaults based on SOC_VENDOR/SOC_CHIP
##############################################################################
.include "socs/socs.mk"

.if ${MACHINE} == ${TARGET} && ${MACHINE_ARCH} == ${TARGET_ARCH} && !defined(CROSS_BUILD_TESTING)
TARGET_ARCH_SUBDIR=	""
.else
TARGET_ARCH_SUBDIR=	${TARGET}.${TARGET_ARCH}
.endif
ZROUTER_FREEBSD_OBJDIR=${ZROUTER_OBJ}/tmp/${TARGET_ARCH_SUBDIR}/${FREEBSD_SRC_TREE}

KERNEL_HINTS+=	hw.soc.vendor=\"${SOC_VENDOR}\"
KERNEL_HINTS+=	hw.soc.model=\"${SOC_CHIP}\"
.if defined(SOC_REVISION)
KERNEL_HINTS+=	hw.soc.revision=\"${SOC_REVISION}\"
.endif
KERNEL_HINTS+=	hw.board.vendor=\"${TARGET_VENDOR}\"
KERNEL_HINTS+=	hw.board.model=\"${TARGET_DEVICE}\"
.if defined(BOARD_REVISION)
KERNEL_HINTS+=	hw.board.revision=\"${BOARD_REVISION}\"
.endif

KERNCONF_OPTIONS+=	NO_SWAPPING

KERNCONF_MAKEOPTIONS+=	"KERNLOADADDR=${KERNCONF_KERNLOADADDR}"
# Allow to undefine LDSCRIPT_NAME if (board|soc).mk was set it to ""
.if !empty(KERNCONF_KERN_LDSCRIPT_NAME)
KERNCONF_MAKEOPTIONS+=	"LDSCRIPT_NAME=${KERNCONF_KERN_LDSCRIPT_NAME}"
.endif

.if defined(KERNCONF_FDT_DTS_FILE)
KERNCONF_MAKEOPTIONS+=	"FDT_DTS_FILE=${KERNCONF_FDT_DTS_FILE}"
.endif

.if defined(ZKERNCONF_FDT_DTS_FILE)
KERNCONF_MAKEOPTIONS+=	"FDT_DTS_FILE=${ZROUTER_ROOT}/${ZKERNCONF_FDT_DTS_FILE}"
.endif

# resolve board flash size with trailing M or K
.if defined(BOARD_FLASH_SIZE)
BOARD_FLASH_SIZE!=echo "${BOARD_FLASH_SIZE}" | \
    sed -e 's/0x/ibase=16; /' -e 's/K/ * 1024/' -e 's/M/ * 1024 * 1024/' | bc
.endif

.if !defined(TARGET_PROFILES) || empty(TARGET_PROFILES)
# if we have flash and it size less than 8M assign profile xSMALL_
.if defined(BOARD_FLASH_SIZE) && !empty(BOARD_FLASH_SIZE) && ${BOARD_FLASH_SIZE} < 8388608
TARGET_PROFILES=xSMALL_
.else
TARGET_PROFILES=SMALL_
.endif
.endif # !defined(TARGET_PROFILES) || empty(TARGET_PROFILES)

target-profiles-list:
	@echo ${TARGET_PROFILES}

##############################################################################
# Profiles - set of SUBDIRS that need to build
##############################################################################
.include "profiles/profiles.mk"

.if defined(IMAGE_TYPE) && ${IMAGE_TYPE} == "trx"
IMAGE_HEADER_EXTRA?=0x1c
.else
IMAGE_HEADER_EXTRA?=0
.endif

.if defined(BUILD_ZROUTER_WITH_CLANG)
CLANG_TC_VARS?=			\
	WITH_CLANG=yes
CLANG_VARS?=			\
	WITH_CLANG=yes		\
	CC=clang		\
	CXX=clang++		\
	CPP=clang-cpp
.else
.if defined(BUILD_ZROUTER_WITH_GCC)
CLANG_TC_VARS?=			\
	WITHOUT_CLANG=yes	\
	WITHOUT_CLANG_BOOTSTRAP=yes	\
	WITHOUT_CLANG_FULL=yes		\
	WITHOUT_CLANG_IS_CC=yes		\
	WITHOUT_LLD=yes		\
	WITH_GCC=yes		\
	WITH_GCC_BOOTSTRAP=yes	\
	WITH_GNUCXX=yes	
CLANG_VARS?=			\
	WITHOUT_CLANG=yes	\
	WITHOUT_CLANG_BOOTSTRAP=yes	\
	WITHOUT_CLANG_FULL=yes		\
	WITHOUT_CLANG_IS_CC=yes		\
	WITHOUT_LLD=yes		\
	WITH_GCC=yes		\
	WITH_GCC_BOOTSTRAP=yes	\
	WITH_GNUCXX=yes	
.else
.if defined(BUILD_ZROUTER_WITH_LLD)
CLANG_TC_VARS?=			\
	WITH_LLD_BOOTSTRAP=yes \
	WITH_LLD_IS_LD=yes
CLANG_VARS?=			\
	WITH_LLD_BOOTSTRAP=yes \
	WITH_LLD_IS_LD=yes
.endif
# use src.opts.mk default
.endif
.endif

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
	@echo "Enabled Profiles: ${TARGET_PROFILES}"
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
KERNEL_CONFIG_FILENAME=${TARGET_VENDOR}_${TARGET_DEVICE}
KERNCONFDIR=${KERNELCONFDIR}

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
.for nooption in ${KERNCONF_NOOPTIONS}
	echo "nooptions	${nooption}" >> ${KERNEL_CONFIG_FILE}
.endfor
	echo "# devices section" >> ${KERNEL_CONFIG_FILE}
.for device in ${KERNCONF_DEVICES}
	echo "device	${device}" >> ${KERNEL_CONFIG_FILE}
.endfor
.for nodevice in ${KERNCONF_NODEVICES}
	echo "nodevice	${nodevice}" >> ${KERNEL_CONFIG_FILE}
.endfor

.if ZROUTER_COMPAT12
	echo "device	random" >> ${KERNEL_CONFIG_FILE}
.endif

# Generate .hints file
# TODO: generate hints on MAP partiotion list, GPIO usege list 
.if exists(${TARGET_SOCDIR}/soc.hints)
_SOC_HINTS=${TARGET_SOCDIR}/soc.hints
.endif
.if exists(${TARGET_BOARDDIR}/board.hints)
_DEVICE_HINTS=${TARGET_BOARDDIR}/board.hints
.endif
.if exists(${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/vendor.hints)
_VENDOR_HINTS=${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/vendor.hints
.endif

kernelhints:	${_SOC_HINTS} ${_DEVICE_HINTS} ${_VENDOR_HINTS} ${KERNELCONFDIR}
	cat /dev/null ${_SOC_HINTS} ${_DEVICE_HINTS} ${_VENDOR_HINTS} > ${KERNEL_HINTS_FILE}
.for hint in ${KERNEL_HINTS}
	echo "${hint}" >> ${KERNEL_HINTS_FILE}
.endfor
# TODO: make dtd file for FDT
#

.if defined(TARGET_CPUTYPE)
_KERNEL_TERGET_ENV= \
	TARGET=${TARGET} \
	TARGET_ARCH=${TARGET_ARCH} \
	TARGET_CPUTYPE=${TARGET_CPUTYPE} \
	TARGET_CPUARCH=${TARGET_CPUARCH}
.else
_KERNEL_TERGET_ENV= \
	TARGET=${TARGET} \
	TARGET_ARCH=${TARGET_ARCH} \
	TARGET_CPUARCH=${TARGET_CPUARCH}
.endif

_KERNEL_TC_BUILD_ENV= \
	${_KERNEL_TERGET_ENV} \
	ZROUTER_ROOT=${ZROUTER_ROOT} \
	WITHOUT_RESCUE=yes \
	${CLANG_TC_VARS}

_KERNEL_BUILD_ENV= \
	${_KERNEL_TERGET_ENV} \
	ZROUTER_ROOT=${ZROUTER_ROOT} \
	WITHOUT_RESCUE=yes \
	KERNCONFDIR=${KERNCONFDIR} \
	${CLANG_VARS}

.if defined(ZROUTER_COMPAT12)
_KERNEL_TC_BUILD_ENV+=-DNO_CLEAN
_KERNEL_BUILD_ENV+=-DNO_CLEAN
.else
_KERNEL_TC_BUILD_ENV+=WITHOUT_CLEAN=yes
_KERNEL_BUILD_ENV+=WITHOUT_CLEAN=yes
.endif

upgrade_checks:
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_TC_BUILD_ENV} -C ${FREEBSD_SRC_TREE} upgrade_checks

kernel-toolchain:	upgrade_checks
.if !defined(SKIP_TOOLCHAIN)
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${PARA_OPTION} ${_KERNEL_TC_BUILD_ENV} -C ${FREEBSD_SRC_TREE} kernel-toolchain
.endif

${ZROUTER_FREEBSD_OBJDIR}/tmp/usr/bin/cc:	kernel-toolchain

kernel-build:	kernelconfig kernelhints ${ZROUTER_FREEBSD_OBJDIR}/tmp/usr/bin/cc
.if defined(WITH_KERNFAST)
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${PARA_OPTION} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} KERNFAST=${KERNEL_CONFIG_FILENAME} buildkernel
.else
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${PARA_OPTION} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} KERNCONF=${KERNEL_CONFIG_FILENAME} buildkernel
.endif

#XXX_BEGIN Only for testing
KMODOWN!=id -u -n
KMODGRP!=id -g -n
_KERNEL_BUILD_ENV+=KMODOWN=${KMODOWN}
_KERNEL_BUILD_ENV+=KMODGRP=${KMODGRP}
#XXX_END Only for testing

kernel:	build-verify build-info kernel-toolchain kernel-build kernel-install-dir kernel-install
.ORDER:	build-verify build-info kernel-toolchain kernel-build kernel-install-dir kernel-install

.if defined(TARGET_CPUTYPE)
_WORLD_TERGET_ENV= \
	TARGET=${TARGET} \
	TARGET_ARCH=${TARGET_ARCH} \
	TARGET_CPUTYPE=${TARGET_CPUTYPE} \
	TARGET_CPUARCH=${TARGET_CPUARCH}
.else
_WORLD_TERGET_ENV= \
	TARGET=${TARGET} \
	TARGET_ARCH=${TARGET_ARCH} \
	TARGET_CPUARCH=${TARGET_CPUARCH}
.endif

_LIBC_OPT= \
	MK_ICONV=no \
	MK_NIS=no

_WORLD_TCBUILD_ENV= \
	${_WORLD_TERGET_ENV} \
	ZROUTER_ROOT=${ZROUTER_ROOT} \
	${CLANG_TC_VARS} \
	WITHOUT_ATM=yes \
	WITHOUT_AUDIT=yes \
	WITHOUT_INFO=yes \
	WITHOUT_IPX=yes \
	WITHOUT_LOCALES=yes \
	WITHOUT_MAN=yes \
	WITHOUT_NLS=yes \
	WITHOUT_PROFILE=yes \
	WITHOUT_RESCUE=yes \
	WITHOUT_CRYPTO=yes \
	WITHOUT_NIS=yes \
	WITHOUT_KERBEROS=yes \
	MALLOC_PRODUCTION=yes \
	$(_LIBC_OPT) \
	MK_OFED=no \
	MK_TESTS=no

_WORLD_BUILD_ENV= \
	SSHDIR=${FREEBSD_SRC_TREE}/crypto/openssh \
	${_WORLD_TERGET_ENV} \
	${CLANG_VARS} \
	ZROUTER_ROOT=${ZROUTER_ROOT} \
	WITHOUT_ASSERT_DEBUG=yes \
	WITHOUT_ATM=yes \
	WITHOUT_AUDIT=yes \
	WITHOUT_INFO=yes \
	WITHOUT_INSTALLLIB=yes \
	WITHOUT_IPX=yes \
	WITHOUT_LOCALES=yes \
	WITHOUT_MAN=yes \
	WITHOUT_NLS=yes \
	WITHOUT_PROFILE=yes \
	WITHOUT_RESCUE=yes \
	MALLOC_PRODUCTION=yes \
	$(_LIBC_OPT) \
	MK_OFED=no \
	MK_TESTS=no

.if defined(ZROUTER_COMPAT12)
_WORLD_TCBUILD_ENV+=-DNO_CLEAN
_WORLD_BUILD_ENV+=-DNO_CLEAN
.else
_WORLD_TCBUILD_ENV+=WITHOUT_CLEAN=yes
_WORLD_BUILD_ENV+=WITHOUT_CLEAN=yes
.endif

.if !defined(DTRACE_ENABLE)
_WORLD_BUILD_ENV+= WITHOUT_CDDL=yes
.endif
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

.if !defined(WITH_IPSEC)
_WORLD_BUILD_ENV+=WITHOUT_IPSEC=yes
_WORLD_INSTALL_ENV+=WITHOUT_IPSEC=yes
.endif

_WORLD_BUILD_ENV+=WITHOUT_NIS=yes

_WORLD_BUILD_ENV+=WITHOUT_BLUETOOTH=yes

.if defined(WITH_WIDEC) && ${WITH_WIDEC} == "yes"
_WORLD_BUILD_ENV+=NOENABLE_WIDEC=yes -DNOENABLE_WIDEC
.endif

_WORLD_BUILD_ENV+=WITHOUT_KERBEROS=yes
_WORLD_BUILD_ENV+=WITHOUT_KERBEROS_SUPPORT=yes
_WORLD_BUILD_ENV+=${WORLD_BUILD_ENV_EXTRA}


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

.for dir in ${WORLD_SUBDIRS_SHARE}
WORLD_SUBDIRS+=share/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_GNU_LIB}
WORLD_SUBDIRS+=gnu/lib/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_GNU_USR_BIN}
WORLD_SUBDIRS+=gnu/usr.bin/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_CDDL_LIB}
WORLD_SUBDIRS+=cddl/lib/${dir}
.endfor

.for dir in ${WORLD_SUBDIRS_CDDL_USR_SBIN}
WORLD_SUBDIRS+=cddl/usr.sbin/${dir}
.endfor


# Project local tools
.for dir in ${WORLD_SUBDIRS_ZROUTER}
# Prepend reverse path, then buildworld can go out of source tree
#WORLD_SUBDIRS+=${SRCROOTUP}/${ZROUTER_ROOT}/${dir}
.endfor

FREEBSD_BUILD_ENV_VARS!=(MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} \
    ${_WORLD_BUILD_ENV} -C ${FREEBSD_SRC_TREE} buildenvvars | \
    sed 's/^/FREEBSD_BUILD_ENV_/')
# Import buildenvvars into our namespace with suffix FREEBSD_BUILD_ENV_
.for var in ${FREEBSD_BUILD_ENV_VARS}
VAR_LEFT=${var:C/=.*//}
VAR_RIGHT=${var:C/.*=//}
${VAR_LEFT}:=${VAR_RIGHT}
.endfor

.warning ${WORLD_SUBDIRS}

#
# World
#
world-toolchain:	upgrade_checks
.if !defined(SKIP_WORLD_INSTALL)
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${PARA_OPTION} ${_WORLD_TCBUILD_ENV} \
	    -C ${FREEBSD_SRC_TREE} toolchain
.endif

world-build:	${ZROUTER_FREEBSD_OBJDIR}/tmp/usr/bin/cc
.if !defined(SKIP_WORLD_INSTALL)
	@echo "XXX: need to find a way to install required includes correctly"
	mkdir -p ${ZROUTER_FREEBSD_OBJDIR}/tmp/usr/include/lzo
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${PARA_OPTION} ${_WORLD_BUILD_ENV} \
	    SUBDIR_OVERRIDE="${WORLD_SUBDIRS}" -C ${FREEBSD_SRC_TREE} \
	    buildworld
.endif

world-install: rootfs-dir
.if !defined(SKIP_WORLD_INSTALL)
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_WORLD_BUILD_ENV} \
	    ${_WORLD_INSTALL_ENV} SUBDIR_OVERRIDE="${WORLD_SUBDIRS}" \
	    DESTDIR=${WORLDDESTDIR} -C ${FREEBSD_SRC_TREE} installworld
.endif

world-fix-lib-links:
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	cd ${WORLDDESTDIR}/usr/lib/ && ${ZROUTER_ROOT}/tools/fix_lib_links.sh

world:  build-verify build-info world-toolchain world-build world-install world-fix-lib-links
.ORDER: build-verify build-info world-toolchain world-build world-install world-fix-lib-links

.if defined(WORLD_SUBDIRS_PORTS) && !empty(WORLD_SUBDIRS_PORTS)
.if !exists(/usr/local/bin/perl)
.error "ports need perl command. Please install that package."
.endif
.if !exists(/usr/local/bin/dialog4ports)
.error "ports need dialog4ports command. Please install that package."
.endif
.include "share/mk/zrouter.ports.mk"
.else
port-build:
	@echo "No ports defined in WORLD_SUBDIRS_PORTS"
.endif

rootfs-dir!
	rm -rf ${WORLDDESTDIR}
	mkdir -p ${WORLDDESTDIR}
	mkdir -p ${WORLDDESTDIR}/usr/lib/lua/ || true
	mkdir -p ${WORLDDESTDIR}/usr/local/bin/ || true
	mkdir -p ${WORLDDESTDIR}/usr/local/sbin/ || true
	mkdir -p ${WORLDDESTDIR}/usr/local/lib/ || true
	mkdir -p ${WORLDDESTDIR}/usr/local/include/ || true
	mkdir -p ${WORLDDESTDIR}/usr/local/libdata/pkgconfig/ || true
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

IMAGE_SUFFIX?=	${NEW_IMAGE_TYPE}
ZTOOLS_PATH=${ZROUTER_OBJ}/ztools
NEW_KERNEL=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_kernel
NEW_ROOTFS=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean
NEW_IMAGE=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}.${IMAGE_SUFFIX}

IMAGE_BUILD_PATHS=${ZTOOLS_PATH}:${FREEBSD_BUILD_ENV_PATH}:${ZROUTER_FREEBSD_OBJDIR}/tmp/usr/bin:${PATH}

.include "share/mk/zrouter.local.tools.mk"
.if ${USE_SYSTEMTOOLS} != "yes"
.include "share/mk/zrouter.base.tools.mk"
.endif

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
    -name tests -or \
    -name zfs \\) \\)

ROOTFS_RMFILES+=calendar dict doc examples groff_font me mk nls openssl \
	pc-sysinstall security sendmail skel syscons tabset tmac vi zoneinfo

# Move kernel out of rootfs
#		world kernel ports
rootfs:		${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean

ROOTFS_CLEAN_MTREE_FILE=    ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean.mtree
.if defined(SEPARATE_LOCALFS)
LOCALFS_CLEAN_MTREE_FILE=    ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_localfs.mtree
.endif

${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean:		${KERNELDESTDIR}/boot/kernel/kernel ${ROOTFS_DEPTEST} ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs
	for d in ${ROOTFS_COPY_DIRS} ; do \
		for f in `( cd $${d} ; find . -type f )` ; do \
			mkdir -p `dirname ${WORLDDESTDIR}/$${f}` ; \
			cp $${d}/$${f} ${WORLDDESTDIR}/$${f} ; \
		done ; \
	done
.if defined(ROOTFS_COPY_OVERWRITE_DIRS)
	for d in ${ROOTFS_COPY_OVERWRITE_DIRS} ; do \
		for f in `( cd $${d} ; find . -type f )` ; do \
			mkdir -p `dirname ${WORLDDESTDIR}/$${f}` ; \
			cp $${d}/$${f} ${WORLDDESTDIR}/$${f} ; \
		done ; \
	done
.endif
	rm -f ${NEW_KERNEL}
	cp ${KERNELDESTDIR}/boot/kernel/kernel ${NEW_KERNEL}
	rm -rf ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean
	cp -R ${WORLDDESTDIR} ${NEW_ROOTFS}
.if defined(SEPARATE_LOCALFS)
	rm -rf ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_localfs
	cp -R ${WORLDDESTDIR}/usr/local ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_localfs
	rm -rf ${NEW_ROOTFS}/usr/local
.endif
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
	 ln -sf w uptime ; \
	 ln -sf id groups ; \
	 ln -sf id whoami ; \
	 ln -sf bsdgrep grep ; \
	 ln -sf bsdgrep egrep ; \
	 ln -sf bsdgrep fgrep ; \
	 ln -sf bsdgrep zgrep ; \
	 ln -sf bsdgrep zegrep ; \
	 ln -sf bsdgrep zfgrep ; \
	 ln -sf bsdgrep lzegrep ; \
	 ln -sf bsdgrep lzfgrep ; \
	 ln -sf bsdgrep lzgrep ; \
	 ln -sf bsdgrep xzegrep ; \
	 ln -sf bsdgrep xzfgrep ; \
	 ln -sf bsdgrep xzgrep
.if ${TARGET_PROFILES} == "SMALL_"
	cd ${NEW_ROOTFS}/usr/bin/ ; \
	 ln -sf ssh slogin ; \
	 ln -sf vi nvi ; \
	 ln -sf vi ex ; \
	 ln -sf vi nex ; \
	 ln -sf vi view ; \
	 ln -sf vi nview
.endif
	rm -rf ${NEW_ROOTFS}/etc/mpd
	ln -s /tmp/etc/mpd ${NEW_ROOTFS}/etc/mpd
#	hg --repository "${ZROUTER_ROOT}" tip \ #
#	    --template 'revision="{rev}"\ndate="{date|isodate}"\n' > \ #
#	    "${NEW_ROOTFS}/etc/zrouter_version"
	LANG=C date '+build="%Y-%m-%d %H:%M:%S"' >> \
	    "${NEW_ROOTFS}/etc/zrouter_version"
	cd ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean ; \
	    find ./usr/ -type d -empty -delete
.if defined(SEPARATE_LOCALFS)
	cd ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean ; \
	    mkdir ./usr/local
.endif
	cd ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean ; \
	    mtree -c -i -n -k uname,gname,mode,nochange | \
		sed -E 's/uname=[[:alnum:]]+/uname=root/' | \
		sed -E 's/gname=[[:alnum:]]+/gname=wheel/' > \
		    ${ROOTFS_CLEAN_MTREE_FILE}
	cd ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs_clean ; \
	    rm -rf usr/lib/debug boot/kernel/kernel*
.if defined(SEPARATE_LOCALFS)
	cd ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_localfs ; \
	    find . -type d -empty -delete
	cd ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_localfs ; \
	    mtree -c -i -n -k uname,gname,mode,nochange | \
		sed -E 's/uname=[[:alnum:]]+/uname=root/' | \
		sed -E 's/gname=[[:alnum:]]+/gname=wheel/' > \
		    ${LOCALFS_CLEAN_MTREE_FILE}
.endif

${ROOTFS_DEPTEST}:		world	ports
	@echo "++++++++++++++ Making $@ ++++++++++++++"

${ZROUTER_FREEBSD_OBJDIR}/sys/${KERNEL_CONFIG_FILENAME}/kernel:	kernel-build
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	echo "XXXXXXXXXXXXX ${ZROUTER_FREEBSD_OBJDIR}/sys/${KERNEL_CONFIG_FILENAME}/kernel"

kernel-install:				${KERNELDESTDIR}/boot/kernel/kernel

${KERNELDESTDIR}/boot/kernel/kernel:	${ZROUTER_FREEBSD_OBJDIR}/sys/${KERNEL_CONFIG_FILENAME}/kernel kernel-install-dir
	@echo "++++++++++++++ Making $@ ++++++++++++++"
.if !empty(KERNELDESTDIR)
	MAKEOBJDIRPREFIX=${ZROUTER_OBJ}/tmp/ ${MAKE} ${_KERNEL_BUILD_ENV} -C ${FREEBSD_SRC_TREE} DESTDIR=${KERNELDESTDIR} KERNCONF=${KERNEL_CONFIG_FILENAME} installkernel
.else
.error "KERNELDESTDIR must be set for kernel-install, since we always do cross-build"
.endif

${NEW_KERNEL}:		${KERNELDESTDIR}/boot/kernel/kernel
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	rm -rf ${NEW_KERNEL}*
	cp ${KERNELDESTDIR}/boot/kernel/kernel ${NEW_KERNEL}

MKULZMA_FLAGS?=-v
MKULZMA_BLOCKSIZE?=131072

ZROUTER_VERSION?=		0.1-ALPHA

KERNCONF_KERNENTRYPOINT?=	${KERNCONF_KERNLOADADDR}

.warning	Load address: ${KERNCONF_KERNLOADADDR} Entry point: ${KERNCONF_KERNENTRYPOINT}

.if !defined(PHYSADDR)
.if ${TARGET_ARCH} == "mipsel" || ${TARGET_ARCH} == "mips"
PHYSADDR=0x80000000
.else 
# arm
PHYSADDR=0x00000000
.endif
.endif
UBOOT_KERNEL_ENTRY_POINT=`${ZROUTER_ROOT}/tools/entryaddr.sh ${TARGET_ARCH} ${PHYSADDR} ${NEW_KERNEL}`
.if defined(KERNEL_HAS_BUILDID)
UBOOT_KERNEL_LOAD_ADDRESS=`${ZROUTER_ROOT}/tools/loadaddr.sh ${TARGET_ARCH} ${PHYSADDR} ${NEW_KERNEL}`
.else
UBOOT_KERNEL_LOAD_ADDRESS=${UBOOT_KERNEL_ENTRY_POINT}
.endif


UBNT_FIRMWARE_IMAGE_SIZE_MAX?=	${FIRMWARE_IMAGE_SIZE_MAX}
UBNT_KERNEL_LOAD_ADDRESS?=	${KERNCONF_KERNLOADADDR}
UBNT_KERNEL_ENTRY_POINT?=	${KERNCONF_KERNENTRYPOINT}
UBNT_KERNEL_FLASH_BASE?=	0xbf030000

TPLINK_KERN_LOADADDR?=	${UBOOT_KERNEL_LOAD_ADDRESS}
TPLINK_KERN_STARTADDR?=	${UBOOT_KERNEL_ENTRY_POINT}
TPLINK_IMG_NAME?=	ZRouter.org
TPLINK_IMG_VERSION?=	${ZROUTER_VERSION}

KERNEL_PACKED_NAME=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_${PACKING_KERNEL_IMAGE}
ROOTFS_PACKED_NAME=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_${PACKING_ROOTFS_IMAGE}
.if defined(SEPARATE_LOCALFS)
LOCALFS_PACKED_NAME=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_${PACKING_LOCALFS_IMAGE}
.endif

.if ${.TARGETS} == "localfs"
PACKING_TARGET_LIST:=${LOCALFS_PACKED_NAME}
.else
PACKING_TARGET_LIST:=${KERNEL_PACKED_NAME} ${ROOTFS_PACKED_NAME}
.endif

.warning "PACKING_KERNEL_IMAGE=${PACKING_TARGET_LIST}"
.include "share/mk/zrouter.packing.mk"

localfs:	${LOCALFS_PACKED_NAME}

#
# TODO: comment here
#
trximage ${NEW_MAGE}:  ${KERNEL_PACKED_NAME} ${ROOTFS_PACKED_NAME}	${ZTOOLS_PATH}/asustrx
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} asustrx -o ${NEW_IMAGE} ${KERNEL_PACKED_NAME} ${ROOTFS_PACKED_NAME}


# zimage used when it possible to use any formats (CFI devices must use trx 
# format, but U-Boot devices must use only kernel in U-Boot format )
zimage:		${KERNEL_PACKED_NAME} ${ROOTFS_PACKED_NAME}
	@cat ${KERNEL_PACKED_NAME} ${ROOTFS_PACKED_NAME} ${BOARD_FIRMWARE_SIGNATURE_FILE} > ${NEW_IMAGE}
	@echo "==="
	@echo "New image: " ${TARGET_VENDOR}_${TARGET_DEVICE}.${IMAGE_SUFFIX}
	@echo -n "/etc/zrouter_version : "
	@cat ${NEW_ROOTFS}/etc/zrouter_version
.if defined(FIRMWARE_IMAGE_SIZE_MAX) && defined(MKULZMA_BLOCKSIZE)
	@NEW_IMAGE_SIZE=`ls -l ${NEW_IMAGE} | awk '{print $$5}'` ; \
	REMAIN_BLOCK=`printf "(%d - %d)/${MKULZMA_BLOCKSIZE}\n" ${FIRMWARE_IMAGE_SIZE_MAX} $${NEW_IMAGE_SIZE} | bc` ; \
	echo "Firmware remain blocks: "$${REMAIN_BLOCK}
.endif
	@IMGMD5=`md5 ${NEW_IMAGE} | cut -f4 -d' '` ; \
	cp ${NEW_IMAGE} ${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}-${ZROUTER_VERSION}.$${IMGMD5}.${IMAGE_SUFFIX} ; echo "MD5: " $${IMGMD5}

.if target(ubntimage)
.if !defined(UBNT_VERSION) || empty(UBNT_VERSION)
.error Specify UBNT_VERSION otherwise device will not accept firmware
.endif
.endif

ubntimage:	${KERNEL_PACKED_NAME} ${ROOTFS_PACKED_NAME} ${ZTOOLS_PATH}/ubnt-mkfwimage
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	PATH=${IMAGE_BUILD_PATHS} ubnt-mkfwimage	\
	    -v "${UBNT_VERSION}"			\
	    -s "${UBNT_KERNEL_FLASH_BASE}"		\
	    -l "${UBNT_KERNEL_LOAD_ADDRESS}"		\
	    -e "${UBNT_KERNEL_ENTRY_POINT}"		\
	    -m "${UBNT_FIRMWARE_IMAGE_SIZE_MAX}"	\
	    -k "${KERNEL_PACKED_NAME}"			\
	    -r "${ROOTFS_PACKED_NAME}"			\
	    -o "${NEW_IMAGE}"

# some TP-Link boards have a modified u-boot bootloader
# and need "mktplinkfw" for building the firmware
tplink_image: ${KERNEL_PACKED_NAME} ${ROOTFS_PACKED_NAME} ${ZTOOLS_PATH}/mktplinkfw
	@echo "++++++++++++++ Making $@ ++++++++++++++"
	@if [ "x${TPLINK_BOARDTYPE}" = "x" ] ; then \
		echo "TPLINK_BOARDTYPE must be defined"; \
		exit 1; \
	fi
	@if [ "x${KERNEL_MAP_START}" = "x" ] ; then \
		echo "KERNEL_MAP_START must be defined, this is the hint.map.?.start ";\
		echo "address from board.hints where hint.map.?.name='kernel'"; \
		exit 1; \
	fi
	KERNEL_PACKED_SIZE=`stat -f %z "${KERNEL_PACKED_NAME}"`; \
	TPLINK_ROOTFS_START=`printf "%#x" $$(( ${KERNEL_MAP_START} + $${KERNEL_PACKED_SIZE} ))`; \
	PATH=${IMAGE_BUILD_PATHS} mktplinkfw \
	    -B ${TPLINK_BOARDTYPE} \
	    -R $${TPLINK_ROOTFS_START} \
	    -L ${TPLINK_KERN_LOADADDR} \
	    -E ${TPLINK_KERN_STARTADDR} \
	    -k "${KERNEL_PACKED_NAME}" \
	    -N ${TPLINK_IMG_NAME} \
	    -V ${TPLINK_IMG_VERSION} \
	    -r "${ROOTFS_PACKED_NAME}" \
	    -o "${NEW_IMAGE}" && \
	PATH=${IMAGE_BUILD_PATHS} mktplinkfw -i "${NEW_IMAGE}"

tplink_new_image: ${KERNEL_PACKED_NAME} ${ROOTFS_PACKED_NAME} ${ZTOOLS_PATH}/mktplinkfw
	PATH=${IMAGE_BUILD_PATHS} mktplinkfw \
            -H ${TPLINK_HARDWARE_ID} \
            -W ${TPLINK_HARDWARE_VER} \
            -F ${TPLINK_HARDWARE_FLASHID} \
            -L ${TPLINK_KERN_LOADADDR} \
            -E ${TPLINK_KERN_STARTADDR} \
            -N ${TPLINK_IMG_NAME} \
            -V ${TPLINK_IMG_VERSION} \
            -s \
	    -r "${ROOTFS_PACKED_NAME}" \
            -a 0x10000 \
            -X ${TPLINK_FIRMWARE_RESERV} \
            -k ${KERNEL_PACKED_NAME} \
	    -o "${NEW_IMAGE}"

split_kernel_rootfs:	${KERNEL_PACKED_NAME} ${ROOTFS_PACKED_NAME}
	touch "${NEW_IMAGE}"

${NEW_IMAGE}:	${NEW_IMAGE_TYPE}

all:	world kernel ports ${NEW_IMAGE}

.include <bsd.obj.mk>











