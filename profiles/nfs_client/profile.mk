
WORLD_SUBDIRS_SBIN+= mount_nfs

#KERNCONF_OPTIONS+=NFSCLIENT NFS_ROOT

KERNCONF_MODULES_OVERRIDE+=nfsclient nfslock nfs_common krpc nfscl nfssvc \
    nfscommon

