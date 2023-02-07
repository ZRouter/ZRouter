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

#ifndef _DSPLIB_DSP_H_
#define _DSPLIB_DSP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dsplib_def.h"

typedef struct
{
	int16 re;
	int16 im;
} int16c;


typedef struct
{
	int32 re;
	int32 im;
} int32c;


typedef struct
{
	int16 a1;
	int16 a2;
	int16 b1;
	int16 b2;
} biquad16;


void mips_vec_add16(int16 *outdata, int16 *indata1, int16 *indata2, int N);
void mips_vec_addc16(int16 *outdata, int16 *indata, int16 c, int N);

void mips_vec_sub16(int16 *outdata, int16 *indata1, int16 *indata2, int N);

void mips_vec_mul16(int16 *outdata, int16 *indata1, int16 *indata2, int N);
void mips_vec_mulc16(int16 *outdata, int16 *indata, int16 c, int N);

void mips_vec_abs16(int16 *outdata, int16 *indata, int N);

int16 mips_vec_dotp16(int16 *indata1, int16 *indata2, int N, int scale);
int16 mips_vec_sum_squares16(int16 *indata, int N, int scale);

void mips_fir16_setup(int16 *coeffs2x, int16 *coeffs, int K);
void mips_fir16(int16 *outdata, int16 *indata, int16 *coeffs2x, int16 *delayline,
				int N, int K, int scale);

void __attribute__((deprecated)) mips_fft16_setup(int16c *twiddles, int log2N);
void mips_fft16(int16c *dout, int16c *din, int16c *twiddles, int16c *scratch, int log2N);

void mips_iir16_setup(int16 *coeffs, biquad16 *bq, int B);
int16 mips_iir16(int16 in, int16 *coeffs, int16 *delayline, int B, int scale);

int16 mips_lms16(int16 in, int16 ref, int16 *coeffs, int16 *delayline,
				 int16 *error, int16 K, int mu);

void mips_vec_abs32(int32 *outdata, int32 *indata, int N);

void mips_vec_add32(int32 *outdata, int32 *indata1, int32 *indata2, int N);
void mips_vec_addc32(int32 *outdata, int32 *indata, int32 c, int N);

void mips_vec_mul32(int32 *outdata, int32 *indata1, int32 *indata2, int N);
void mips_vec_mulc32(int32 *outdata, int32 *indata, int32 c, int N);

void mips_vec_sub32(int32 *outdata, int32 *indata1, int32 *indata2, int N);

int32 mips_vec_dotp32(int32 *indata1, int32 *indata2, int N, int scale);
int32 mips_vec_sum_squares32(int32 *indata, int N, int scale);

void __attribute__((deprecated)) mips_fft32_setup(int32c *twiddles, int log2N);
void mips_fft32(int32c *dout, int32c *din, int32c *twiddles, int32c *scratch, int log2N);

#ifdef __cplusplus
}
#endif

#endif
