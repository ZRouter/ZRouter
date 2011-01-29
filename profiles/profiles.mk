


.if defined(TARGET_PROFILES)

.for profile in ${TARGET_PROFILES}

.if exists(${ZROUTER_ROOT}/profiles/${profile}/) && exists(${ZROUTER_ROOT}/profiles/${profile}/profile.mk)
.include "${ZROUTER_ROOT}/profiles/${profile}/profile.mk"
.else
.error "Profile ${profile} not found"
.endif

.endfor


.endif