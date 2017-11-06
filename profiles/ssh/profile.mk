# ssh
WORLD_SUBDIRS+= \
secure/lib/libssh \
secure/usr.bin/scp \
secure/usr.bin/ssh
WORLD_SUBDIRS_LIB+=libldns
# sshd
WORLD_SUBDIRS+= \
secure/usr.bin/ssh-keygen \
secure/usr.sbin/sshd
WORLD_SUBDIRS_LIB+=libblacklist
