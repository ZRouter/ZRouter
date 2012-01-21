SOC_CHIP?="NONE"
SOC_VENDOR?="NONE"

.if exists(${ZROUTER_ROOT}/socs/${SOC_VENDOR}/${SOC_CHIP}/)
TARGET_SOCDIR= ${ZROUTER_ROOT}/socs/${SOC_VENDOR}/${SOC_CHIP}
.include "${TARGET_SOCDIR}/soc.mk"
.endif

# Defaults
KERNCONF_KERNLOADADDR?=		0x80001000

.if !defined(TARGET_SOCDIR)
SOC_PAIRS!=ls -d ${ZROUTER_ROOT}/socs/*/* | sed 's/^.*\/socs\///'
.endif

.if ${TARGET_ARCH} == "mipsel" || ${TARGET_ARCH} == "mipseb"
KERNCONF_KERN_LDSCRIPT_NAME?=	ldscript.mips.bin
.else
KERNCONF_KERN_LDSCRIPT_NAME?=	ldscript.${TARGET}
.endif

