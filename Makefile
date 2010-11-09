.CURDIR!=pwd

# ZROUTER_ROOT can be set in environment
.if !defined(ZROUTER_ROOT)
ZROUTER_ROOT=${.CURDIR}
.endif

# Board configyration must define used SoC/CPU
.include "boards/boards.mk"

# Set SoC defaults based on SOC_VENDOR/SOC_CHIP
.include "socs/socs.mk"

