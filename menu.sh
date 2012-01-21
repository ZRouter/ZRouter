#!/bin/sh

#
# Copyright (c) 2011, ZRouter Project
# Copyright (c) 2011, Luiz Otavio O Souza <loos.br@gmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice unmodified, this list of conditions, and the following
#    disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#

#
# The main menu
#
main_menu() {
	TMPOPTIONSFILE=$(mktemp -t zrouter-build-menu)
	trap "${RM} -f ${TMPOPTIONSFILE}; exit 1" 1 2 3 5 10 13 15
	${SH} -c "${DIALOG} --title \"ZRouter build menu ${PROFILE_NAME}\" --menu ' ' 13 60 6 \
	    Device \"Select the target device\" \
	    Targets \"Select the build targets\" \
	    Paths \"Set the paths of sources and build objects\" \
	    Save \"Save build profile\" \
	    Load \"Load build profile\" \
	    Build \"Build ZRouter !\" \
	    2> ${TMPOPTIONSFILE}"
	status=$?
	if [ ${status} -ne 0 ] ; then
		${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
		echo "Canceled... exiting."
		exit 0
	fi
	MENU_OPTION=`${CAT} ${TMPOPTIONSFILE} 2> /dev/null`
	${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
}

#
# Provide a menu with every possible vendor/device pairs
#
get_target_device() {
	TARGET_PAIRS=`ls -d ${ZROUTER_ROOT}/boards/*/* 2> /dev/null | ${SED} 's/^.*boards\///'`
	set -- ${TARGET_PAIRS} XXX
	MENU_SIZE=${#}
	if [ ${MENU_SIZE} -gt 1 ]; then
		MENU_SIZE=$((${MENU_SIZE} - 1))
	fi
	if [ ${MENU_SIZE} -gt 15 ]; then
		MENU_SIZE=15
	fi
	BOX_SIZE=$((${MENU_SIZE} + 7))
	DEFOPTIONS=""
	while [ ${#} -gt 1 ]; do
		TARGET_DEVICE=`echo "${1}" | ${SED} 's/^.*\///'`
		DEFOPTIONS="${DEFOPTIONS} ${TARGET_DEVICE} ${1}"
		shift 1
	done
	TMPOPTIONSFILE=$(mktemp -t zrouter-build-menu)
	trap "${RM} -f ${TMPOPTIONSFILE}; exit 1" 1 2 3 5 10 13 15
	${SH} -c "${DIALOG} --title \"Please, select the target device\" --menu ' ' \
	    ${BOX_SIZE} 60 ${MENU_SIZE} ${DEFOPTIONS} 2> ${TMPOPTIONSFILE}"
	status=$?
	if [ ${status} -ne 0 ] ; then
		${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
		return 1
	fi
	TARGET_DEVICE=`${CAT} ${TMPOPTIONSFILE} 2> /dev/null`
	${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
	if [ -z "${TARGET_DEVICE}" ]; then
		return 1
	fi
	for PAIR in ${TARGET_PAIRS}; do
		DEVICE=`echo "${PAIR}" | ${SED} 's/^.*\///'`
		if [ "${TARGET_DEVICE}" = "${DEVICE}" ]; then
			TARGET_PAIR="${PAIR}"
			return 0
		fi
	done
	return 1
}

#
# Provide a checklist menu with all possible build targets
#
select_build_targets() {
	TMPOPTIONSFILE=$(mktemp -t zrouter-build-menu)
	trap "${RM} -f ${TMPOPTIONSFILE}; exit 1" 1 2 3 5 10 13 15
	${SH} -c "${DIALOG} --title \"Please, select the build target(s)\" --checklist ' ' \
	    19 75 12 \
	    kernel_xz \"Build a xz compressed kernel\" OFF \
	    kernel_bin_xz \"Build a binary xz compressed kernel\" OFF \
	    kernel.xz.uboot \"Build a xz compressed uboot kernel image\" OFF \
	    kernel_bz2 \"Build a bzip2 compressed kernel\" OFF \
	    kernel_bin_bz2 \"Build a binary bzip2 compressed kernel\" OFF \
	    kernel.bz2.uboot \"Build a bzip2 compressed uboot kernel image\" OFF \
	    kernel_oldlzma \"Build a LZMA compressed kernel\" OFF \
	    kernel_bin_oldlzma \"Build a binary LZMA compressed kernel\" OFF \
	    kernel.oldlzma.uboot \"Build a LZMA compressed uboot kernel image\" ON \
	    fwimage \"Build a fwimage kernel\" OFF \
	    zimage \"Build a zimage kernel\" OFF \
	    rootfs.iso.ulzma \"Build the compressed root file system image\" ON \
	    2> ${TMPOPTIONSFILE}"
	status=$?
	if [ ${status} -ne 0 ] ; then
		${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
		return 1
	fi
	TARGETS=`${CAT} ${TMPOPTIONSFILE} 2> /dev/null | ${SED} 's/\"//g'`
	${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
	return 0
}

#
# Read the FreeBSD tree source path
#
get_src_path() {
	TMPOPTIONSFILE=$(mktemp -t zrouter-build-menu)
	trap "${RM} -f ${TMPOPTIONSFILE}; exit 1" 1 2 3 5 10 13 15
	${SH} -c "${DIALOG} --title \"Please, enter the build object directory\" \
	    --inputbox ' ' 8 60 "${FREEBSD_SRC_TREE}" 2> ${TMPOPTIONSFILE}"
	status=$?
	if [ ${status} -ne 0 ] ; then
		${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
		return 1
	fi
	FREEBSD_SRC_TREE=`${CAT} ${TMPOPTIONSFILE} 2> /dev/null`
	${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
	return 0
}

#
# Read the objects directory path
#
get_obj_path() {
	TMPOPTIONSFILE=$(mktemp -t zrouter-build-menu)
	trap "${RM} -f ${TMPOPTIONSFILE}; exit 1" 1 2 3 5 10 13 15
	${SH} -c "${DIALOG} --title \"Please, enter the build object directory\" \
	    --inputbox ' ' 8 60 "${OBJ_DIR}" 2> ${TMPOPTIONSFILE}"
	status=$?
	if [ ${status} -ne 0 ] ; then
		${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
		return 1
	fi
	OBJ_DIR=`${CAT} ${TMPOPTIONSFILE} 2> /dev/null`
	${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
	return 0
}

#
# Provide a inputmenu for source and obj directories selection
#
get_src_dirs() {
	TMPOPTIONSFILE=$(mktemp -t zrouter-build-menu)
	trap "${RM} -f ${TMPOPTIONSFILE}; exit 1" 1 2 3 5 10 13 15
	while [ 1 ]; do
		${SH} -c "${DIALOG} --title \"Please, set the source and obj directories\" \
		    --menu ' ' 10 60 3 \
		    Sources \"${FREEBSD_SRC_TREE}\" \
		    Objects \"${OBJ_DIR}\" \
		    Back \"Back to main menu\" \
		    2> ${TMPOPTIONSFILE}"
		status=$?
		if [ ${status} -ne 0 ] ; then
			${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
			return 1
		fi
		RESULT=`${CAT} ${TMPOPTIONSFILE} 2> /dev/null`
		${RM} -f ${TMPOPTIONSFILE} 2> /dev/null
		if [ -z "${RESULT}" ]; then
			return 1
		fi
		if [ "${RESULT}" = "Sources" ]; then
			get_src_path
		fi
		if [ "${RESULT}" = "Objects" ]; then
			get_obj_path
		fi
		if [ "${RESULT}" = "Back" ]; then
			break
		fi
	done
	return 0
}

#
# Verify if the build profiles directory exists. Create it if don't.
#
check_build_profiles_dir() {
	if [ ! -d ${BUILD_PROFILES_DIR} -a ! -e ${BUILD_PROFILES_DIR} ]; then
		${MKDIR} ${BUILD_PROFILES_DIR} 2> /dev/null
	fi
	if [ ! -d ${BUILD_PROFILES_DIR} ]; then
		echo "Warning: invalid directory for build profiles: ${BUILD_PROFILES_DIR}"
		exit 0
	fi
}

#
# Verify if the selected profile name already exists
#
check_new_profile_exists() {
	if [ -f "${BUILD_PROFILES_DIR}/${NEW_PROFILE}.conf" ]; then
		${SH} -c "${DIALOG} --title \"Profile already exists\" --yesno \
		    \"\nDo you want to overwrite the ${NEW_PROFILE} profile ?\" 7 60"
		status=$?
		if [ ${status} -ne 0 ] ; then
			return 1
		fi
	fi
	return 0
}

#
# Save a build profile
#
save_profile() {
	if [ "${PROFILE}" != "NONE" ]; then
		SAVE_PROFILE="${PROFILE}"
	else
		SAVE_PROFILE=""
	fi
	TMPOPTIONSFILE=$(mktemp -t zrouter-build-menu)
	trap "${RM} -f ${TMPOPTIONSFILE}; exit 1" 1 2 3 5 10 13 15
	${SH} -c "${DIALOG} --title \"Please, enter the profile name\" --inputbox ' ' \
	    8 60 \"${SAVE_PROFILE}\" 2> ${TMPOPTIONSFILE}"
	status=$?
	if [ ${status} -ne 0 ] ; then
		${RM} -f ${TMPOPTIONSFILE}
		return 0
	fi
	NEW_PROFILE=`${CAT} ${TMPOPTIONSFILE} 2> /dev/null | ${SED} 's/[^[:alnum:]]/\_/g'`
	${RM} -f ${TMPOPTIONSFILE}

	if [ -z "${NEW_PROFILE}" ]; then
		return 1
	fi

	check_build_profiles_dir

	check_new_profile_exists
	if [ ${?} -ne 0 ]; then
		return 1 
	fi
	PROFILE_CONF="${ZROUTER_ROOT}/${BUILD_PROFILES_DIR}/${NEW_PROFILE}.conf"
	PROFILE="${NEW_PROFILE}"

	${CAT} << EOF > ${PROFILE_CONF}
#
# This file is auto generated by ZRouter build tool
#
PROFILE="${PROFILE}"
TARGET_PAIR="${TARGET_PAIR}"
TARGETS="${TARGETS}"
FREEBSD_SRC_TREE="${FREEBSD_SRC_TREE}"
OBJ_DIR="${OBJ_DIR}"
EOF
	PROFILE_NAME="(${PROFILE})"
	return 0
}

#
# Load a build profile
#
load_profile() {
	ZROUTER_ROOT_ESCAPED=`echo "${ZROUTER_ROOT}" | ${SED} 's/\//\\\\\\//g'`
	BUILD_PROFILES=`ls ${ZROUTER_ROOT}/${BUILD_PROFILES_DIR}/*.conf 2> /dev/null`
	set -- ${BUILD_PROFILES} XXX
	MENU_SIZE=${#}
	if [ ${MENU_SIZE} -gt 1 ]; then
		MENU_SIZE=$((${MENU_SIZE} - 1))
	fi
	if [ ${MENU_SIZE} -gt 15 ]; then
		MENU_SIZE=15
	fi
	DEFOPTIONS=""
	if [ ${#} -eq 1 ]; then
		DEFOPTIONS="'*' 'no profiles to load'"
		MENU_SIZE=1
	fi
	BOX_SIZE=$((${MENU_SIZE} + 7))
	while [ ${#} -gt 1 ]; do
		PROFILE_NAME=`echo "${1}" | \
		    ${SED} "s/${ZROUTER_ROOT_ESCAPED}\/${BUILD_PROFILES_DIR}\///" | \
		    ${SED} 's/\.conf//'`
		PROFILE_PATH=`echo "${1}" | ${SED} "s/${ZROUTER_ROOT_ESCAPED}\///"`
		DEFOPTIONS="${DEFOPTIONS} ${PROFILE_NAME} '${PROFILE_PATH}'"
		shift 1
	done
	TMPOPTIONSFILE=$(mktemp -t zrouter-build-menu)
	trap "${RM} -f ${TMPOPTIONSFILE}; exit 1" 1 2 3 5 10 13 15
	${SH} -c "${DIALOG} --title \"Please, select one profile file\" --menu ' ' \
	    $BOX_SIZE 60 $MENU_SIZE ${DEFOPTIONS} 2> ${TMPOPTIONSFILE}"
	status=$?
	if [ ${status} -ne 0 ] ; then
		${RM} -f ${TMPOPTIONSFILE}
		return 1
	fi
	LOAD_PROFILE=`${CAT} ${TMPOPTIONSFILE} 2> /dev/null`
	${RM} -f ${TMPOPTIONSFILE}
	if [ -z "${LOAD_PROFILE}" -o "${LOAD_PROFILE}" = "*" ]; then
		return 1
	fi
	LOAD_PROFILE="${ZROUTER_ROOT}/${BUILD_PROFILES_DIR}/${LOAD_PROFILE}.conf"
	if [ -f "${LOAD_PROFILE}" ]; then
		. ${LOAD_PROFILE}
	else
		${SH} -c "${DIALOG} --title \"Problem reading the profile\" --msgbox \
		    \"\nThere is a problem with the selected profile (not a file ?).\nThe file could not be read.\" 8 65"
		return 1
	fi
	PROFILE_NAME="(${PROFILE})"
	return 0
}

build_info() {
	set -- ${TARGETS} XXX
	BOX_SIZE=${#}
	if [ ${BOX_SIZE} -gt 3 ]; then
		BOX_SIZE=$(($BOX_SIZE / 3))
		BOX_SIZE=$(($BOX_SIZE + 12))
	else
		BOX_SIZE=$(($BOX_SIZE + 10))
	fi
	${SH} -c "${DIALOG} --title \"ZRouter build settings ${PROFILE_NAME}\" --yesno \
	    \"\nPROFILE: ${PROFILE} \
	    \nTARGET_PAIR: ${TARGET_PAIR} \
	    \nTARGETS: ${TARGETS} \
	    \nFREEBSD_SRC_TREE: ${FREEBSD_SRC_TREE} \
	    \nOBJ_DIR: ${OBJ_DIR} \
	    \n\nContinue with build ?\" \
	    ${BOX_SIZE} 60"
	status=$?
	if [ ${status} -ne 0 ] ; then
		return 1
	fi
	return 0
}

#
# Main()
#
# Set defaults
#
: ${BUILD_PROFILES_DIR:="build_profiles"}
: ${FREEBSD_SRC_TREE:="/usr/src"}
: ${OBJ_DIR:="/usr/obj"}
: ${TARGET_PAIR:="NONE"}
: ${PROFILE:="NONE"}
: ${TARGETS:="kernel.oldlzma.uboot rootfs.iso.ulzma"}
PROFILE_NAME="(${PROFILE})"
if [ -z "${ZROUTER_ROOT}" ]; then
	ZROUTER_ROOT=`/bin/pwd`
fi

#
# Tools paths
#
RM="/bin/rm"
SH="/bin/sh"
CAT="/bin/cat"
SED="/usr/bin/sed"
MKDIR="/bin/mkdir"
DIALOG="/usr/bin/dialog"


#
# Main menu
#
while [ 1 ]; do
	main_menu

	# Select the target device
	if [ "${MENU_OPTION}" = "Device" ]; then
		get_target_device
	fi

	# Select the build targets
	if [ "${MENU_OPTION}" = "Targets" ]; then
		select_build_targets
	fi

	# Select the sources and obj directory
	if [ "${MENU_OPTION}" = "Paths" ]; then
		get_src_dirs
	fi

	# Save the build profile
	if [ "${MENU_OPTION}" = "Save" ]; then
		save_profile
	fi

	# Load the build profile
	if [ "${MENU_OPTION}" = "Load" ]; then
		load_profile
	fi

	# Build ZRouter
	if [ "${MENU_OPTION}" = "Build" ]; then
		build_info
		status=${?}
		if [ ${status} -eq 0 ]; then
			break
		fi
	fi
done

echo "==> building zrouter !!!"
make -C "${ZROUTER_ROOT}" TARGET_PAIR=${TARGET_PAIR} \
	FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} OBJ_DIR=${OBJ_DIR} ${TARGETS}
