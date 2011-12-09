CONVERT_FROM_XZ:=${NEW_CURRENT_PACKING_FILE_NAME}
CONVERT_TO_XZ:=${CURRENT_PACKING_FILE_NAME}

# Let user to override
CONVERTER_XZ?=xz
CONVERTER_XZ_FLAGS?=--stdout

${CONVERT_TO_XZ}:		${CONVERT_FROM_XZ}
	@echo ========== Convert ${CONVERT_FROM_XZ} to ${CONVERT_TO_XZ} with ${CONVERTER_XZ} ============
	${CONVERTER_XZ} ${CONVERTER_XZ_FLAGS} ${CONVERT_FROM_XZ} > ${CONVERT_TO_XZ}