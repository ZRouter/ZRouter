#
#

CDIR=${.CURDIR}/../../../contrib/lzo2
.PATH: ${CDIR}/src ${CDIR}/include

CFLAGS+=	-I${CDIR} -I${CDIR}/include

NO_MAN=

SRCS=		\
    lzo_crc.c lzo_init.c lzo_ptr.c lzo_str.c lzo_util.c \
    lzo1.c lzo1_99.c \
    lzo1a.c lzo1a_99.c \
    lzo1b_1.c lzo1b_2.c lzo1b_3.c lzo1b_4.c lzo1b_5.c lzo1b_6.c \
    lzo1b_7.c lzo1b_8.c lzo1b_9.c lzo1b_99.c lzo1b_9x.c \
    lzo1b_cc.c lzo1b_d1.c lzo1b_d2.c lzo1b_rr.c lzo1b_xx.c \
    lzo1c_1.c lzo1c_2.c lzo1c_3.c lzo1c_4.c lzo1c_5.c lzo1c_6.c \
    lzo1c_7.c lzo1c_8.c lzo1c_9.c lzo1c_99.c lzo1c_9x.c \
    lzo1c_cc.c lzo1c_d1.c lzo1c_d2.c lzo1c_rr.c lzo1c_xx.c \
    lzo1f_1.c lzo1f_9x.c lzo1f_d1.c lzo1f_d2.c \
    lzo1x_1.c lzo1x_9x.c lzo1x_d1.c lzo1x_d2.c lzo1x_d3.c \
    lzo1x_o.c lzo1x_1k.c lzo1x_1l.c lzo1x_1o.c \
    lzo1y_1.c lzo1y_9x.c lzo1y_d1.c lzo1y_d2.c lzo1y_d3.c lzo1y_o.c \
    lzo1z_9x.c lzo1z_d1.c lzo1z_d2.c lzo1z_d3.c \
    lzo2a_9x.c lzo2a_d1.c lzo2a_d2.c

INCS=	lzo/lzo1.h	\
	lzo/lzo1b.h	\
	lzo/lzo1f.h	\
	lzo/lzo1y.h	\
	lzo/lzo2a.h	\
	lzo/lzoconf.h	\
	lzo/lzoutil.h	\
	lzo/lzo1a.h	\
	lzo/lzo1c.h	\
	lzo/lzo1x.h	\
	lzo/lzo1z.h	\
	lzo/lzo_asm.h	\
	lzo/lzodefs.h

INCSDIR=	/usr/include/lzo
# /usr/obj//usr/home/ray/work/DDTeam.net/ZRouter/zrouter/tmp//usr/1/MIPS_FreeBSD/HEAD/head/tmp/usr/include/lzo
# /usr/obj/
# /usr/home/ray/work/DDTeam.net/ZRouter/zrouter
# /tmp/
# /usr/1/MIPS_FreeBSD/HEAD/head
# /tmp
# /usr/include/lzo

LIB=		lzo2
SHLIB_MAJOR= 	0
SHLIBDIR?= 	/lib

.include <bsd.lib.mk>
