BINDIR?=/usr/bin/

CFLAGS+= -I${.CURDIR}/SRC
CXXFLAGS+= -I${.CURDIR}/SRC
NO_MAN=
PROG_CXX=oldlzma

.PATH: ${.CURDIR}/SRC/7zip/Compress/LZMA_Alone
SRCS=LzmaAlone.cpp LzmaBench.cpp LzmaRam.cpp LzmaRamDecode.c

.PATH: ${.CURDIR}/SRC/7zip/Compress/LZMA_C
SRCS+=LzmaDecode.c

.PATH: ${.CURDIR}/SRC/7zip/Compress/Branch
SRCS+=BranchX86.c

.PATH: ${.CURDIR}/SRC/7zip/Compress/LZMA
SRCS+=LZMADecoder.cpp
SRCS+=LZMAEncoder.cpp

.PATH: ${.CURDIR}/SRC/7zip/Compress/LZ
SRCS+=LZInWindow.cpp
SRCS+=LZOutWindow.cpp

.PATH: ${.CURDIR}/SRC/7zip/Compress/RangeCoder
SRCS+=RangeCoderBit.cpp

.PATH: ${.CURDIR}/SRC/7zip/Common
SRCS+=InBuffer.cpp
SRCS+=OutBuffer.cpp
SRCS+=FileStreams.cpp

.PATH: ${.CURDIR}/SRC/Common
SRCS+=Alloc.cpp
SRCS+=C_FileIO.cpp
SRCS+=CommandLineParser.cpp
SRCS+=CRC.cpp
SRCS+=String.cpp
SRCS+=StringConvert.cpp
SRCS+=StringToInt.cpp
SRCS+=Vector.cpp

.include <bsd.prog.mk>