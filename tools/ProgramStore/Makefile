
OBJS = \
	7znew/7zlzma.o	\
        alloc.o         \
        decompress/decompress.o    \
	7znew/AriBitCoder.o	\
        7znew/IInOutStreams.o \
        7znew/InByte.o        \
        7znew/LenCoder.o      \
        7znew/LiteralCoder.o  \
        7znew/LZMA.o          \
        7znew/LZMADecoder.o   \
        7znew/LZMAEncoder.o   \
        minilzo.o       \
        n2b_99.o        \
        n2d_99.o        \
        decompress/nrv2d_d.o       \
        7znew/OutByte.o       \
        ProgramStore.o  \
        StdAfx.o        \
        7znew/WindowIn.o      \
        7znew/WindowOut.o

CPPFLAGS += -DGNU -I7z -fpermissive

VPATH += 7z decompress


CPPFLAGS += -static -O2

all: $(OBJS)
	$(CXX) $(OBJS) $(CPPFLAGS) -o ProgramStore -Wl,-Map -Wl,ps.map

clean:
	rm -f $(OBJS) ProgramStore

