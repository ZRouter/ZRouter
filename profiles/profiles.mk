


.if defined(TARGET_PROFILES)

.for profile in ${TARGET_PROFILES}

.if exists(${ZROUTER_ROOT}/profiles/${profile}/) && exists(${ZROUTER_ROOT}/profiles/${profile}/profile.mk)
.include "${ZROUTER_ROOT}/profiles/${profile}/profile.mk"

.if exists(${ZROUTER_ROOT}/profiles/${profile}/Makefile)
# If profile has Makefile, it seems want to be built. Adding it to subdirs.
WORLD_SUBDIRS_ZROUTER+=profiles/${profile}
.endif

.if exists(${ZROUTER_ROOT}/profiles/${profile}/files)
ROOTFS_COPY_DIRS+=${ZROUTER_ROOT}/profiles/${profile}/files
.endif

.else
.error "Profile ${profile} not found"
.endif # exists(${ZROUTER_ROOT}/profiles/${profile}/) && profile.mk

.endfor

.endif