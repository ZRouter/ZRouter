
MK_MAN=no
PROG_CXX=	ProgramStore

.PATH: ${.CURDIR}/
SRCS = \
        alloc.c         \
        minilzo.c       \
        n2b_99.c        \
        n2d_99.c        \
        ProgramStore.cpp \
        StdAfx.cpp

.PATH: ${.CURDIR}/decompress
SRCS+=	\
        decompress.cpp    \
        nrv2d_d.c

.PATH: ${.CURDIR}/7znew
SRCS+=	\
	7zlzma.cpp	\
	AriBitCoder.cpp	\
        IInOutStreams.cpp \
        InByte.cpp        \
        LenCoder.cpp      \
        LiteralCoder.cpp  \
        LZMA.cpp          \
        LZMADecoder.cpp   \
        LZMAEncoder.cpp \
        OutByte.cpp       \
        WindowIn.cpp      \
        WindowOut.cpp

CPPFLAGS += -DGNU -I7z -fpermissive

#VPATH += 7z decompress

CPPFLAGS += -static -O2

LDFLAGS = -Wl,-Map -Wl,ps.map

#all: $(OBJS)
#	$(CXX) $(OBJS) $(CPPFLAGS) -o ProgramStore

#clean:
#	rm -f $(OBJS) ProgramStore

.include <bsd.prog.mk>
