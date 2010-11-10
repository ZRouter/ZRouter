

.warning "Broadcom/BCM5354 SoC"
#XXX testing
LZMA=lzma
KERNEL_PATH=/usr/obj/kernel


SOC_KERNCONF=BCM5354
SOC_KERNHINTS=BCM5354.hints


kernel_deflate:	${DEPEND_ON_LZMA}
	${DEBUG}${LZMA} e ${KERNEL_PATH} ${KERNEL_PATH}.lzma
	${DEBUG}cp ${KERNEL_PATH} ${KERNEL_PATH}.unpacked
	${DEBUG}cp ${KERNEL_PATH}.lzma ${KERNEL_PATH}
