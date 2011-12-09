#
# Packing magic
# require PACKING_TARGET_LIST on input
#

.if defined(PACKING_TARGET_LIST) && !empty(PACKING_TARGET_LIST)
.for CURRENT_PACKING_TARGET in ${PACKING_TARGET_LIST}
CURRENT_PACKING_FILE:=${CURRENT_PACKING_TARGET}
_LIST:=${CURRENT_PACKING_FILE:C/\./ /g}

CURRENT_PACKING_FILE_NAME:=${CURRENT_PACKING_FILE}

.for CURRENT_ITEM in ${_LIST}
# Get filename w/o last suffix
NEW_CURRENT_PACKING_FILE_NAME:=${CURRENT_PACKING_FILE_NAME:R}
# Get last suffix
CURRENT_PACKING_FILE_SUFFIX:=${CURRENT_PACKING_FILE_NAME:E}

# Check if we try split dir path intead of filename only
_DIR!=dirname ${CURRENT_PACKING_FILE_NAME}
_NEW_DIR!=dirname ${NEW_CURRENT_PACKING_FILE_NAME}
.if !empty(_DIR) && !empty(_NEW_DIR) && ${_DIR} != ${_NEW_DIR}
.warning "${_DIR}" != "${_NEW_DIR}"
# Reset CURRENT_PACKING_FILE_SUFFIX to break loop
CURRENT_PACKING_FILE_SUFFIX=
.endif

.if !empty(CURRENT_PACKING_FILE_SUFFIX) && ${CURRENT_PACKING_FILE_SUFFIX} != ""

#.warning From file ${NEW_CURRENT_PACKING_FILE_NAME}, to file ${CURRENT_PACKING_FILE_NAME}, with converter "${CURRENT_PACKING_FILE_SUFFIX}".
.if exists(${ZROUTER_ROOT}/share/mk/converters/${CURRENT_PACKING_FILE_SUFFIX}.mk)

.warning Will convert file ${NEW_CURRENT_PACKING_FILE_NAME}, with converter "${CURRENT_PACKING_FILE_SUFFIX}".
# Invoke converter for that suffix
.include "${ZROUTER_ROOT}/share/mk/converters/${CURRENT_PACKING_FILE_SUFFIX}.mk"

.else
.error "Converter for suffix '${CURRENT_PACKING_FILE_SUFFIX}' undefined"
.endif

CURRENT_PACKING_FILE_NAME:=${NEW_CURRENT_PACKING_FILE_NAME}
.endif
.endfor # CURRENT_ITEM in ${LIST}
.endfor # CURRENT_PACKING_TARGET in ${PACKING_TARGET_LIST}
.else # defined(PACKING_TARGET_LIST) && !empty(PACKING_TARGET_LIST)
.error "PACKING_TARGET_LIST not defined or empty"
.endif # defined(PACKING_TARGET_LIST) && !empty(PACKING_TARGET_LIST)