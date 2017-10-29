#
#

.include <src.opts.mk>

UVDIR=${.CURDIR}/../../../contrib/libuv
.PATH: ${UVDIR}/

CFLAGS+=	-I${UVDIR}/src -I${UVDIR}/include

NO_MAN=

SRCS=		\
src/fs-poll.c \
src/inet.c \
src/threadpool.c \
src/uv-common.c \
src/version.c \
src/unix/async.c \
src/unix/core.c \
src/unix/dl.c \
src/unix/fs.c \
src/unix/getaddrinfo.c \
src/unix/getnameinfo.c \
src/unix/loop-watcher.c \
src/unix/loop.c \
src/unix/pipe.c \
src/unix/poll.c \
src/unix/process.c \
src/unix/signal.c \
src/unix/stream.c \
src/unix/tcp.c \
src/unix/thread.c \
src/unix/timer.c \
src/unix/tty.c \
src/unix/udp.c \
src/unix/bsd-ifaddrs.c \
src/unix/freebsd.c \
src/unix/kqueue.c \
src/unix/posix-hrtime.c

INCS=		\
include/android-ifaddrs.h \
include/pthread-barrier.h \
include/stdint-msvc2008.h \
include/tree.h \
include/uv-aix.h \
include/uv-bsd.h \
include/uv-darwin.h \
include/uv-errno.h \
include/uv-linux.h \
include/uv-os390.h \
include/uv-posix.h \
include/uv-sunos.h \
include/uv-threadpool.h \
include/uv-unix.h \
include/uv-version.h \
include/uv-win.h \
include/uv.h

LIB=		uv
SHLIB_MAJOR= 	0

#SHLIBDIR?= 	/lib

.include <bsd.lib.mk>
