TARGET_DEVICE?="NONE"

.if exists(${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/) && exists(${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/${TARGET_DEVICE}/)
TARGET_BOARDDIR= ${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/${TARGET_DEVICE}
.include "${TARGET_BOARDDIR}/board.mk"
.endif

.if !defined(TARGET_BOARDDIR)
PAIRS!=ls -d ${ZROUTER_ROOT}/boards/*/* | sed 's/^.*\/boards\///'

.warning "No board configuration for pair TARGET_VENDOR/TARGET_DEVICE `${TARGET_VENDOR}/${TARGET_DEVICE}`"
.error "Posible pairs ${PAIRS}"
.endif