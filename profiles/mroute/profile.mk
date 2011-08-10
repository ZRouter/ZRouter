

#WORLD_SUBDIRS_USR_SBIN+=wpa/hostapd wpa/hostapd_cli wpa/wpa_cli wpa/wpa_passphrase wpa/wpa_supplicant

#KERNCONF_MODULES_OVERRIDE+=ip_mroute_mod

KERNCONF_OPTIONS+=MROUTING
WORLD_SUBDIRS_ZROUTER+=target/sbin/igmpproxy
