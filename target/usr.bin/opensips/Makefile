BINDIR?=/usr/bin

SRCS=	action.c flags.c proxy.c sliblist.c \
	async.c forward.c pt.c socket_info.c \
	bin_interface.c io_wait.c pvar.c sr_module_deps.c \
	blacklists.c ip_addr.c qvalue.c sr_module.c \
	cfg.tab.c lex.yy.c re.c statistics.c \
	context.c main.c reactor.c strcommon.c \
	core_stats.c map.c receive.c time_rec.c \
	crc.c md5.c regexp.c timer.c \
	daemonize.c md5utils.c resolve.c transformations.c \
	data_lump_rpl.c mod_fix.c route_struct.c tsend.c \
	data_lump.c modparam.c route.c usr_avp.c \
	dprint.c msg_callbacks.c script_cb.c ut.c \
	dset.c msg_translator.c script_var.c xlog.c \
	errinfo.c name_alias.c serialize.c \
	error.c prime_hash.c sha1.c

PROG=opensips
CFLAGS+=-Wall -DFAST_LOCK -DADAPTIVE_WAIT -DADAPTIVE_WAIT_LOOPS=1024 -DDBG_MALLOC -DOS='"freebsd"' -D__CPU_mips2 -D__OS_freebsd -DNAME='"opensips"' -DVERSION='"2.1.4"' -DARCH='"mips2"' -DCOMPILER=4.8.5
#DPADD=	${LIBM}
#LIBDIR=	. net mem evi
#SUBDIR=  net mem evi 
SUBDIR= modules


.include <bsd.prog.mk>

