TARGET_DEVICE?="NONE"
TARGET_VENDOR?="NONE"

.if !empty(TARGET_PAIR)
TARGET_VENDOR=${TARGET_PAIR:C/\/.*//}
TARGET_DEVICE=${TARGET_PAIR:C/.*\///}
.endif

#### Per vendor
.if exists(${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/)

.if exists(${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/files)
# Vendor wide files
ROOTFS_COPY_DIRS+=${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/files
.endif # exists(vendor/TARGET_VENDOR/files)

# Vendor wide tunables
.if exists(${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/vendor.mk)
.include "${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/vendor.mk"
.endif # exists(vendor/TARGET_VENDOR/vendor.mk)

#### Per device
.if exists(${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/${TARGET_DEVICE}/)
TARGET_VENDOR_BOARDDIR= ${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/${TARGET_DEVICE}

# Vendor files for exact device.
.if exists(${TARGET_VENDOR_BOARDDIR}/files)
# Will be processed after soc/boards/profiles defined
ROOTFS_COPY_DIRS+=${TARGET_VENDOR_BOARDDIR}/files
.endif # exists(TARGET_VENDOR_BOARDDIR/files)

# Vendor device tunables
.if exists(${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/vendor.mk)
.include "${ZROUTER_ROOT}/vendor/${TARGET_VENDOR}/vendor.mk"
.endif # exists(vendor/TARGET_VENDOR/vendor.mk)

.endif # exists DEVICE directory.

.endif # exists VENDOR directory.