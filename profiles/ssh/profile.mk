
#WORLD_SUBDIRS_PORTS+=/usr/ports/security/openvpn


#WORLD_SUBDIRS+= \
#secure/lib/libcrypto \
#secure/lib/libssl

#WORLD_SUBDIRS_LIB+= libwrap
#WORLD_SUBDIRS_LIB+= libpcap
#WORLD_SUBDIRS_ZROUTER+=target/usr.sbin/mpd

WORLD_SUBDIRS+= \
secure/lib/libssh \
secure/usr.bin/scp \
secure/usr.bin/ssh \
secure/usr.bin/ssh-keygen \
secure/usr.sbin/sshd

