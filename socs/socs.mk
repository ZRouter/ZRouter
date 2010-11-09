TARGET_DEVICE?="NONE"

.if exists(${ZROUTER_ROOT}/socs/${SOC_VENDOR}/${SOC_CHIP}/)
TARGET_DEVICEDIR= ${ZROUTER_ROOT}/socs/${SOC_VENDOR}/${SOC_CHIP}
.include "${TARGET_DEVICEDIR}/soc.mk"
.endif

.if !defined(TARGET_DEVICEDIR)
PAIRS!=ls -d ${ZROUTER_ROOT}/socs/*/* | sed 's/^.*\/socs\///'

.warning "No board configuration for pair SOC_VENDOR/SOC_CHIP `${SOC_VENDOR}/${SOC_CHIP}`"
.error "Posible pairs ${PAIRS}"
.endif