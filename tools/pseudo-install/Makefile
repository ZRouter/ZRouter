

test:
	@echo ----------------------------------------------------------------------------------------------------
	    ./install -v Makefile Makefile1
	@echo ----------------------------------------------------------------------------------------------------
	ZROUTER_WORLD_INSTALL_PATH=/usr/obj//usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-320_rootfs \
	    ./install -s -o root -g wheel -m 3555 -v   hostname file csh 1.html csh.1 csh.1.gz \
	    /usr/obj//usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-320_rootfs/bin
	@echo ----------------------------------------------------------------------------------------------------
	ZROUTER_WORLD_INSTALL_PATH=/usr/obj//usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-320_rootfs \
	    ./install -d -o root -g wheel -m rwsr--r-- -v   hostname file csh 1.html csh.1 csh.1.gz \
	    /usr/obj//usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-320_rootfs/bin
	@echo ----------------------------------------------------------------------------------------------------
	ZROUTER_WORLD_INSTALL_PATH=/usr/obj//usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-320_rootfs \
	    ./install -s -oroot -gwheel -m3555 -v   hostname file csh 1.html csh.1 csh.1.gz \
	    /usr/obj//usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-320_rootfs/bin
	@echo --------------------------- Must fail, test of env ZROUTER_WORLD_INSTALL_PATH ----------------------
	-./install -d -oroot -gwheel -mrwsr--r-- -v   hostname file csh 1.html csh.1 csh.1.gz \
	    /usr/obj//usr/home/ray/work/DDTeam.net/ZRouter/zrouter/D-Link_DIR-320_rootfs/bin
	@echo ----------------------------------------------------------------------------------------------------

