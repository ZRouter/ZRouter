TARGET_DEVICE?="NONE"

.if exists(${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/) && exists(${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/${TARGET_DEVICE}/)
TARGET_DEVICEDIR= ${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/${TARGET_DEVICE}
.include "${TARGET_DEVICEDIR}/board.mk"
.endif

.if !defined(TARGET_DEVICEDIR)
PAIRS!=ls -d ${ZROUTER_ROOT}/boards/*/* | sed 's/^.*\/boards\///'

.warning "No board configuration for pair TARGET_VENDOR/TARGET_DEVICE `${TARGET_VENDOR}/${TARGET_DEVICE}`"
.error "Posible pairs ${PAIRS}"
.endif