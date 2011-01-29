TARGET_DEVICE?="NONE"

.if exists(${ZROUTER_ROOT}/socs/${SOC_VENDOR}/${SOC_CHIP}/)
TARGET_SOCDIR= ${ZROUTER_ROOT}/socs/${SOC_VENDOR}/${SOC_CHIP}
.include "${TARGET_SOCDIR}/soc.mk"
.endif

.if !defined(TARGET_SOCDIR)
PAIRS!=ls -d ${ZROUTER_ROOT}/socs/*/* | sed 's/^.*\/socs\///'

.warning "No board configuration for pair SOC_VENDOR/SOC_CHIP `${SOC_VENDOR}/${SOC_CHIP}`"
.error "Posible pairs ${PAIRS}"
.endif