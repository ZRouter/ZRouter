
#CFLAGS+= -O2 -pipe -fno-strict-aliasing -c -I ../../../
CFLAGS+= -I ../../../
CPPFLAGS+= -I ../../../
PROG=oldlzma
SRCS= LzmaAlone.cpp LzmaBench.cpp LzmaRam.cpp LzmaRamDecode.c ../LZMA_C/LzmaDecode.c \
../Branch/BranchX86.c ../LZMA/LZMADecoder.cpp ../LZMA/LZMAEncoder.cpp \
../LZ/LZInWindow.cpp ../LZ/LZOutWindow.cpp ../RangeCoder/RangeCoderBit.cpp \
../../Common/InBuffer.cpp ../../Common/OutBuffer.cpp ../../Common/FileStreams.cpp \
../../../Common/Alloc.cpp ../../../Common/C_FileIO.cpp \
../../../Common/CommandLineParser.cpp ../../../Common/CRC.cpp \
../../../Common/String.cpp ../../../Common/StringConvert.cpp \
../../../Common/StringToInt.cpp ../../../Common/Vector.cpp



nope:
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	LzmaAlone.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	LzmaBench.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	LzmaRam.cpp
	cc -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	LzmaRamDecode.c
	cc -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../LZMA_C/LzmaDecode.c
	cc -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../Branch/BranchX86.c
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../LZMA/LZMADecoder.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../LZMA/LZMAEncoder.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../LZ/LZInWindow.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../LZ/LZOutWindow.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../RangeCoder/RangeCoderBit.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../Common/InBuffer.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../Common/OutBuffer.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../Common/FileStreams.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../../Common/Alloc.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../../Common/C_FileIO.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../../Common/CommandLineParser.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../../Common/CRC.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../../Common/String.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../../Common/StringConvert.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../../Common/StringToInt.cpp
	c++ -O2 -pipe -fno-strict-aliasing -c -I ../../../ 	../../../Common/Vector.cpp
	c++ -o lzma  LzmaAlone.o LzmaBench.o LzmaRam.o LzmaRamDecode.o LzmaDecode.o BranchX86.o LZMADecoder.o LZMAEncoder.o LZInWindow.o LZOutWindow.o RangeCoderBit.o InBuffer.o OutBuffer.o FileStreams.o Alloc.o C_FileIO.o CommandLineParser.o CRC.o String.o StringConvert.o StringToInt.o Vector.o  -lm


.include <bsd.prog.mk>