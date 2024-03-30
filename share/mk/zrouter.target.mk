target-build:
	@echo "----> Start building target ..."
.for dir in ${ZROUTER_TARGET}
	@echo "Start ${dir} target building..." ;
	mkdir -p ${ZROUTER_OBJ}/tmp/${TARGET}.${TARGET_ARCH}/${dir}
	@cd ${dir} ; ${MAKE} ${_TARGET_CROSS_ENV} MAKEOBJDIR=${ZROUTER_OBJ}/tmp/${TARGET}.${TARGET_ARCH}/${dir} MK_DEBUG_FILES=no ZPREFIX=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs/usr/local
	@cd ${dir} ; ${MAKE} ${_TARGET_CROSS_ENV} MAKEOBJDIR=${ZROUTER_OBJ}/tmp/${TARGET}.${TARGET_ARCH}/${dir} DESTDIR=${ZROUTER_OBJ}/${TARGET_VENDOR}_${TARGET_DEVICE}_rootfs/ BINOWN=${OWN} BINGRP=${GRP} MK_DEBUG_FILES=no install
.endfor

