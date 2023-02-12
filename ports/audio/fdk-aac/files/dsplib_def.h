/*
 * ****************************Begin Copyright 1D**********************************
 * Unpublished work (c) MIPS Technologies, Inc.  All rights reserved.
 * Unpublished rights reserved under the copyright laws of the United States
 * of America and other countries.
 *
 * This code is confidential and proprietary to MIPS Technologies, Inc. ("MIPS
 * Technologies") and  may be disclosed only as permitted in writing by MIPS
 * Technologies or an authorized third party.  Any copying, reproducing,
 * modifying, use or disclosure of this code (in whole or in part) that is not
 * expressly permitted in writing by MIPS Technologies or an authorized third
 * party is strictly prohibited.  At a minimum, this code is protected under
 * trade secret, unfair competition, and copyright laws.  Violations thereof
 * may result in criminal penalties and fines.
 *
 * MIPS Technologies reserves the right to change this code to improve function,
 * design or otherwise.  MIPS Technologies does not assume any liability arising
 * out of the application or use of this code, or of any error or omission in
 * such code.  Any warranties, whether express, statutory, implied or otherwise,
 * including but not limited to the implied warranties of merchantability or
 * fitness for a particular purpose, are excluded.  Except as expressly provided
 * in any written license agreement from MIPS Technologies or an authorized third
 * party, the furnishing of this code does not give recipient any license to any
 * intellectual property rights, including any patent rights, that cover this code.
 *
 * This code shall not be exported or transferred for the purpose of reexporting
 * in violation of any U.S. or non-U.S. regulation, treaty, Executive Order, law,
 * statute, amendment or supplement thereto.
 *
 * This code may only be disclosed to the United States government ("Government"),
 * or to Government users, with prior written consent from MIPS Technologies or an
 * authorized third party.  This code constitutes one or more of the following:
 * commercial computer software, commercial computer software documentation or
 * other commercial items.  If the user of this code, or any related documentation
 * of any kind, including related technical data or manuals, is an agency,
 * department, or other entity of the Government, the use, duplication,
 * reproduction, release, modification, disclosure, or transfer of this code, or
 * any related documentation of any kind, is restricted in accordance with Federal
 * Acquisition Regulation 12.212 for civilian agencies and Defense Federal
 * Acquisition Regulation Supplement 227.7202 for military agencies.  The use of
 * this code by the Government is further restricted in accordance with the terms
 * of the license agreement(s) and/or applicable contract terms and conditions
 * covering this code from MIPS Technologies or an authorized third party.
 * *******************************End Copyright************************************
 */

#ifndef _DSPLIB_DEF_H_
#define _DSPLIB_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;


static inline int16 mul16(int16 a, int16 b)
{
	return (a * b) >> 15;
}


static inline int16 mul16r(int16 a, int16 b)
{
	return (a * b + 0x4000) >> 15;
}


static inline int32 mul32(int32 a, int32 b)
{
	return ((int64)a * b) >> 31;
}


#define MIN16 ((int16) 0x8000)
#define MAX16 ((int16) 0x7FFF)

#define MIN32 ((int32) 0x80000000)
#define MAX32 ((int32) 0x7FFFFFFF)


// #define SAT16(x) ((int16)(((x) < MIN16) ? MIN16 : (((x) > MAX16) ? MAX16 : x)))
// #define SAT32(x) ((int32)(((x) < MIN32) ? MIN32 : (((x) > MAX32) ? MAX32 : x)))


static inline int32 SAT16P(int32 x)
{
	return (x > MAX16) ? MAX16 : x;
}


static inline int32 SAT16N(int32 x)
{
	return (x < MIN16) ? MIN16 : x;
}


static inline int32 SAT16(int32 x)
{
	int32 y = SAT16P(x);
	return SAT16N(y);
}

#ifdef __cplusplus
}
#endif


#endif

