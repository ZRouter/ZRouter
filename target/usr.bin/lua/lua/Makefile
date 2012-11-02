BINDIR?=/usr/bin/

SRCS=	lapi.c lcode.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c \
	lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c  \
	lundump.c lvm.c lzio.c lua.c \
	lauxlib.c lbaselib.c ldblib.c liolib.c lmathlib.c loslib.c ltablib.c \
	lstrlib.c loadlib.c linit.c

# MYCFLAGS="-DLUA_USE_LINUX"

PROG=		lua
NO_MAN=
DPADD=		${LIBM}
LDADD=		-lm
# -ldl
CFLAGS+=	-DLUA_USE_DLOPEN -DLUA_USE_POSIX


.include <bsd.prog.mk>

