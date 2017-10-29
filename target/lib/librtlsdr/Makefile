#
#

.include <src.opts.mk>

RTLSDRDIR=${.CURDIR}/../../../contrib/rtl-sdr
.PATH: ${RTLSDRDIR}/

CFLAGS+=	-I${RTLSDRDIR}/src -I${RTLSDRDIR}/include

NO_MAN=

SRCS=		\
src/librtlsdr.c \
src/tuner_e4k.c \
src/tuner_fc0012.c \
src/tuner_fc0013.c \
src/tuner_fc2580.c \
src/tuner_r82xx.c

INCS=		\
include/rtl-sdr.h

LIB=		rtlsdr
SHLIB_MAJOR= 	0

#SHLIBDIR?= 	/lib

.include <bsd.lib.mk>
