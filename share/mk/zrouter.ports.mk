# Ports

# that may used for any platform
# we need only say cross-build to configure
PORTS_CONFIGURE_TARGET=--build=i386-portbld-freebsd8.2 --host=mipsel-portbld-freebsd8.2


_TARGET_DEFS = \
	TARGET_VENDOR=${TARGET_VENDOR} \
	TARGET_DEVICE=${TARGET_DEVICE} \
	FREEBSD_SRC_TREE=${FREEBSD_SRC_TREE} \
	TARGET_PROFILES="${TARGET_PROFILES}"



_TARGET_CROSS_DEFS = \
	PATH=${FREEBSD_BUILD_ENV_PATH}:/usr/local/bin:/usr/local/sbin \
	PREFIX=${WORLDDESTDIR} \
	LOCALBASE=${WORLDDESTDIR} \
	PKG_CONFIG_PATH=${WORLDDESTDIR}/libdata/pkgconfig/ \
	DISTDIR=${ZROUTER_OBJ}/distfiles/ \
	GNU_CONFIGURE_PREFIX=${WORLDDESTDIR} \
	GLOBAL_CONFIGURE_ARGS="${PORTS_CONFIGURE_TARGET}" \
	NO_INSTALL_MANPAGES=yes \
	WITHOUT_CHECK=yes \
	NO_PKG_REGISTER=yes \
	NO_DEPENDS=yes \
	NOPORTDOCS=yes \
	BINOWN=ray \
	BINGRP=wheel \
	NOPORTEXAMPLES=yes \
	INSTALL_AS_USER=yes \
	INSTALL="sh ${FREEBSD_SRC_TREE}/tools/install.sh" \
	ac_cv_func_malloc_0_nonnull=yes \
	ac_cv_func_realloc_0_nonnull=yes \
	AUTOTOOLS_LOCALBASE=/usr/local

#	LDADD="-L${WORLDDESTDIR}/lib"
#	LIBTOOL=/usr/local/bin/libtool \
#	-ELIBTOOL

# ac_cv_func_malloc_0_nonnull=yes avoid "undefined reference to `rpl_malloc'"
# ac_cv_func_realloc_0_nonnull=yes avoid "undefined reference to `rpl_realloc'"

port-build:
	mkdir -p ${WORLDDESTDIR}
	@echo "----> Start building ports dependencies ..."
.for dir in ${WORLD_SUBDIRS_PORTS}
	@echo "Start ${dir} port building..."
	cd ${ZROUTER_ROOT} ;${MAKE} ${_TARGET_DEFS} PORT_BUILD_DEPEND_CROSS=${dir} port-build-depend-cross
.endfor
	@echo "----> Ports dependencies build done ..."


# Cross-compilation of dependency, build dependency must be built with host env
# LIB and RUN dependency should be builded with cross environment
# Else (FETCH, EXTRACT, PATCH, BUILD dependency) with host env

port-build-depend-cross:
.for dir in ${PORT_BUILD_DEPEND_CROSS}
	@echo "--------> Start ${dir} port building ..."
	@echo "------------> Test FETCH EXTRACT PATCH BUILD dependency for ${dir}..."
	_DEPENDS=$$(cd ${dir} ; ${MAKE} -VFETCH_DEPENDS -VEXTRACT_DEPENDS -VPATCH_DEPENDS -VBUILD_DEPENDS) ; \
	if [ "x$${_DEPENDS}" != "x" ] ; then \
		echo "$${_DEPENDS}" ; \
		${MAKE} -f /usr/ports/Mk/bsd.port.mk BUILD_DEPENDS="$${_DEPENDS}" depends ; \
	fi
	@echo "------------> Test LIB dependency for ${dir}..."
	@_DEPENDS=$$(cd ${dir} ; ${MAKE} -VLIB_DEPENDS) ; \
	echo "LIB_DEPENDS=$${_DEPENDS}" ; \
	for _DEP in $${_DEPENDS} ; do \
		_DEPTEST=$${_DEP%%:*} ; \
		echo "Test if $${_DEPTEST} present" ; \
		LIBNAME=$${_DEPTEST%.*} ; \
		LIBVER=$${_DEPTEST#*.} ; \
		if [ "$${LIBNAME}" = "$${LIBVER}" ] ; then LIBVER="" ; else LIBVER=".$${LIBVER}" ; fi ; \
		SONAME=lib$${LIBNAME}.so$${LIBVER} ; \
		echo Search for $${SONAME} ; \
		MATCHED_LIBS=$$(find ${WORLDDESTDIR}/lib ${WORLDDESTDIR}/usr/lib -name $${SONAME}) ; \
		_DEPPATH=$${_DEP#*:} ; \
		if [ -z $${MATCHED_LIBS} ] ; then \
			cd ${ZROUTER_ROOT} ; ${MAKE} ${_TARGET_DEFS} PORT_BUILD_DEPEND_CROSS=$${_DEPPATH} port-build-depend-cross ; \
		fi ; \
	done
	@echo "------------> Test RUN dependency for ${dir}..."
	@_DEPENDS=$$(cd ${dir} ; ${MAKE} -VRUN_DEPENDS) ; \
	echo "RUN_DEPENDS=$${_DEPENDS}" ; \
	for _DEP in $${_DEPENDS} ; do \
		_DEPTEST=$${_DEP%%:*} ; \
		echo "$${_DEPTEST} is pkg-config?" ; \
		if [ "$${_DEPTEST}" = "pkg-config" ] ; then  continue ; fi ; \
		echo "Test if $${_DEPTEST} present" ; \
		_DEPPATH=$${_DEP#*:} ; \
		cd ${ZROUTER_ROOT} ; ${MAKE} ${_TARGET_DEFS} PORT_BUILD_DEPEND_CROSS=$${_DEPPATH} port-build-depend-cross ; \
	done
	@echo "------------> Build ${dir}..."
	@cd ${dir} ; ${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} generate-plist
	@PORT_PLIST=$$( cd ${dir} ; ${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} -VTMPPLIST ) ; \
	    PORT_STATUS=$$( ${ZROUTER_ROOT}/tools/checkdep.pl libs $${PORT_PLIST} ${WORLDDESTDIR} ) ; \
	    if [ $${PORT_STATUS} -lt 50 ] ; then \
		    echo "$${PORT_STATUS}% of files matched, do install" ; \
		    rm -f ${ZROUTER_OBJ}/ports/${dir}/.install* ; \
		    echo cd ${dir} ; echo ${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} install ; \
		    cd ${dir} ; ${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} install || \
			    ( ${MAKE} WRKDIR=${ZROUTER_OBJ}/ports/${dir} clean && \
			    echo ${MAKE} WRKDIR=${ZROUTER_OBJ}/ports/${dir} configure && \
			    ${MAKE} WRKDIR=${ZROUTER_OBJ}/ports/${dir} configure && \
			    mv `${MAKE} WRKDIR=${ZROUTER_OBJ}/ports/${dir} -VPATCH_COOKIE` `${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} -VPATCH_COOKIE` && \
			    mv `${MAKE} WRKDIR=${ZROUTER_OBJ}/ports/${dir} -VEXTRACT_COOKIE` `${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} -VEXTRACT_COOKIE` && \
			    mv `${MAKE} WRKDIR=${ZROUTER_OBJ}/ports/${dir} -VCONFIGURE_COOKIE` `${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} -VCONFIGURE_COOKIE` && \
			    echo ${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} all && \
			    ${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} all && \
			    echo ${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} install && \
			    ${MAKE} ${_TARGET_CROSS_DEFS} WRKDIR=${ZROUTER_OBJ}/ports/${dir} install ) ; \
	    fi
.endfor
	@echo "--------> Done building ${dir} port ..."


# Host tools required for extract, patch, configure, build etc.
# All dependency should be builded and installed with host environment
# so now we don`t care about dependency type.

port-build-depend-host:
	@echo "Start PORTNAME port building..."
.for dir in ${PORT_BUILD_DEPEND_HOST}
	@echo "---------> build/install/clean for port ${dir} as dependency with host environment"
	cd ${dir} ; ${MAKE} install clean
	@echo "---------> port ${dir} done (dependency)"
.endfor

