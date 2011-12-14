SOC_CHIP?="NONE"
SOC_VENDOR?="NONE"

# Defaults
KERNCONF_KERNLOADADDR?=0x80001000

.if exists(${ZROUTER_ROOT}/socs/${SOC_VENDOR}/${SOC_CHIP}/)
TARGET_SOCDIR= ${ZROUTER_ROOT}/socs/${SOC_VENDOR}/${SOC_CHIP}
.include "${TARGET_SOCDIR}/soc.mk"
.endif

.if !defined(TARGET_SOCDIR)
SOC_PAIRS!=ls -d ${ZROUTER_ROOT}/socs/*/* | sed 's/^.*\/socs\///'
.endif
