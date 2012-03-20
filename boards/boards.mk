TARGET_DEVICE?="NONE"
TARGET_VENDOR?="NONE"

.if !empty(TARGET_PAIR)
TARGET_VENDOR=${TARGET_PAIR:C/\/.*//}
TARGET_DEVICE=${TARGET_PAIR:C/.*\///}
.endif

.if exists(${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/) && exists(${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/${TARGET_DEVICE}/)
TARGET_BOARDDIR= ${ZROUTER_ROOT}/boards/${TARGET_VENDOR}/${TARGET_DEVICE}
.include "${TARGET_BOARDDIR}/board.mk"

.if exists(${TARGET_BOARDDIR}/files)
ROOTFS_COPY_DIRS+=${TARGET_BOARDDIR}/files
.endif

.if exists(${TARGET_BOARDDIR}/board_firmware.sig)
BOARD_FIRMWARE_SIGNATURE_FILE=${TARGET_BOARDDIR}/board_firmware.sig
.endif

.endif

