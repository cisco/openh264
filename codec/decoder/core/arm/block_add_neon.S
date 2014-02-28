/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#ifdef HAVE_NEON
.text
#include "arm_arch_common_macro.S"
#ifdef APPLE_IOS

.macro	ROW_TRANSFORM_1_STEP
//	{	//	input: src_d[0]~[3], output: e_q[0]~[3]; working: $8 $9
		vaddl.s16		$4, $0, $2			//int32 e[i][0] = src[0] + src[2];
		vsubl.s16		$5, $0, $2			//int32 e[i][1] = src[0] - src[2];
		vshr.s16		$8, $1, #1
		vshr.s16		$9, $3, #1
		vsubl.s16		$6, $8, $3			//int32 e[i][2] = (src[1]>>1)-src[3];	
		vaddl.s16		$7, $1, $9			//int32 e[i][3] = src[1] + (src[3]>>1);		
//	}
.endm

.macro	TRANSFORM_4BYTES	// both row & col transform used
//	{	//	output: f_q[0]~[3], input: e_q[0]~[3];
		vadd.s32		$0, $4, $7			//int16 f[i][0] = e[i][0] + e[i][3];
		vadd.s32		$1, $5, $6			//int16 f[i][1] = e[i][1] + e[i][2];
		vsub.s32		$2, $5, $6			//int16 f[i][2] = e[i][1] - e[i][2];
		vsub.s32		$3, $4, $7			//int16 f[i][3] = e[i][0] - e[i][3];
//	}
.endm

.macro	COL_TRANSFORM_1_STEP
//	{	//	input: src_q[0]~[3], output: e_q[0]~[3];
		vadd.s32		$4, $0, $2			//int32 e[0][j] = f[0][j] + f[2][j];
		vsub.s32		$5, $0, $2			//int32 e[1][j] = f[0][j] - f[2][j];
		vshr.s32		$6, $1, #1
		vshr.s32		$7, $3, #1
		vsub.s32		$6, $6, $3			//int32 e[2][j] = (f[1][j]>>1) - f[3][j];	
		vadd.s32		$7, $1, $7			//int32 e[3][j] = f[1][j] + (f[3][j]>>1);
//	}
.endm

#else

.macro	ROW_TRANSFORM_1_STEP arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9
//	{	//	input: src_d[0]~[3], output: e_q[0]~[3]; working: $8 $9
		vaddl.s16		\arg4, \arg0, \arg2			//int32 e[i][0] = src[0] + src[2];
		vsubl.s16		\arg5, \arg0, \arg2			//int32 e[i][1] = src[0] - src[2];
		vshr.s16		\arg8, \arg1, #1
		vshr.s16		\arg9, \arg3, #1
		vsubl.s16		\arg6, \arg8, \arg3			//int32 e[i][2] = (src[1]>>1)-src[3];	
		vaddl.s16		\arg7, \arg1, \arg9			//int32 e[i][3] = src[1] + (src[3]>>1);		
//	}
.endm

.macro	TRANSFORM_4BYTES  arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7 // both row & col transform used
//	{	//	output: f_q[0]~[3], input: e_q[0]~[3];
		vadd.s32		\arg0, \arg4, \arg7			//int16 f[i][0] = e[i][0] + e[i][3];
		vadd.s32		\arg1, \arg5, \arg6			//int16 f[i][1] = e[i][1] + e[i][2];
		vsub.s32		\arg2, \arg5, \arg6			//int16 f[i][2] = e[i][1] - e[i][2];
		vsub.s32		\arg3, \arg4, \arg7			//int16 f[i][3] = e[i][0] - e[i][3];
//	}
.endm

.macro	COL_TRANSFORM_1_STEP arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7
//	{	//	input: src_q[0]~[3], output: e_q[0]~[3];
		vadd.s32		\arg4, \arg0, \arg2			//int32 e[0][j] = f[0][j] + f[2][j];
		vsub.s32		\arg5, \arg0, \arg2			//int32 e[1][j] = f[0][j] - f[2][j];
		vshr.s32		\arg6, \arg1, #1
		vshr.s32		\arg7, \arg3, #1
		vsub.s32		\arg6, \arg6, \arg3			//int32 e[2][j] = (f[1][j]>>1) - f[3][j];	
		vadd.s32		\arg7, \arg1, \arg7			//int32 e[3][j] = f[1][j] + (f[3][j]>>1);
//	}
.endm
#endif
// r0    int16_t* block,
// r1    int8_t* non_zero_count,
  WELS_ASM_FUNC_BEGIN SetNonZeroCount_neon
	
	vld1.64	{d0-d2}, [r1]
		
	vceq.s8	q0, q0, #0
	vceq.s8	d2, d2, #0
	vmvn	q0, q0
	vmvn	d2, d2
	vabs.s8	q0, q0
	vabs.s8	d2, d2
	
	vst1.64	{d0-d2}, [r1]
  WELS_ASM_FUNC_END


//	r0 int16_t * block, 
//	r1	int32_t stride
  WELS_ASM_FUNC_BEGIN WelsResBlockZero16x16_neon// can use for 256*sizeof(int16_t)
	push		{r2}
	mov			r2, #16
// each row 16 elements, 16*sizeof(int16_t)
//	memset(ptr_dest, 0, 16*sizeof(int16_t));
//	ptr_dest += stride;	
	lsl			r1, r1, #1	// r1 = 2*r1
	veor.i16	q0, q0, q0
	veor.i16	q1, q1, q1
			
block_zero_16x16_luma_loop:	
	vst1.i16	{q0, q1}, [r0], r1
	subs		r2,	r2, #2
	vst1.i16	{q0, q1}, [r0], r1	
	bne			block_zero_16x16_luma_loop
	
	pop		{r2}
  WELS_ASM_FUNC_END
	
  WELS_ASM_FUNC_BEGIN WelsResBlockZero8x8_neon// can use for 64*sizeof(int16_t)
	push		{r2}
	mov			r2, #8
// each row 8 elements, 8*sizeof(int16_t)
//	memset(ptr_dest, 0, 8*sizeof(int16_t));
//	ptr_dest += stride;	
	lsl			r1, r1, #1
	veor.i16	q0, q0, q0
		
block_zero_8x8_chma_loop:	
	vst1.i16	{q0}, [r0], r1
	subs		r2,	r2, #2
	vst1.i16	{q0}, [r0], r1	
	bne			block_zero_8x8_chma_loop
	
	pop		{r2}
  WELS_ASM_FUNC_END


//	uint8_t *pred, const int32_t stride, int16_t *rs
  WELS_ASM_FUNC_BEGIN IdctResAddPred_neon

	vld4.s16		{d0, d1, d2, d3}, [r2]		// cost 3 cycles!	
	
	ROW_TRANSFORM_1_STEP		d0, d1, d2, d3, q4, q5, q6, q7, d4, d5
	
	TRANSFORM_4BYTES		q0, q1, q2, q3, q4, q5, q6, q7	
	
	// transform element 32bits
	vtrn.s32		q0, q1				//[0 1 2 3]+[4 5 6 7]-->[0 4 2 6]+[1 5 3 7]
	vtrn.s32		q2, q3				//[8 9 10 11]+[12 13 14 15]-->[8 12 10 14]+[9 13 11 15]
	vswp			d1, d4				//[0 4 2 6]+[8 12 10 14]-->[0 4 8 12]+[2 6 10 14]
	vswp			d3, d6				//[1 5 3 7]+[9 13 11 15]-->[1 5 9 13]+[3 7 11 15]

	COL_TRANSFORM_1_STEP		q0, q1, q2, q3, q4, q5, q6, q7
	
	TRANSFORM_4BYTES		q0, q1, q2, q3, q4, q5, q6, q7	
			
	//after clip_table[MAX_NEG_CROP] into [0, 255]
	mov			r2, r0
	vld1.32		{d12[0]},[r0],r1
	vld1.32		{d12[1]},[r0],r1
	vld1.32		{d14[0]},[r0],r1
	vld1.32		{d14[1]},[r0]

	vrshrn.s32		d8, q0, #6
	vrshrn.s32		d9, q1, #6
	vrshrn.s32		d10, q2, #6
	vrshrn.s32		d11, q3, #6
		
	vmovl.u8		q0,d12
	vmovl.u8		q1,d14
	vadd.s16		q0,q4
	vadd.s16		q1,q5

	vqmovun.s16		d12,q0
	vqmovun.s16		d14,q1

	vst1.32		{d12[0]},[r2],r1
	vst1.32		{d12[1]},[r2],r1
	vst1.32		{d14[0]},[r2],r1
	vst1.32		{d14[1]},[r2]
  WELS_ASM_FUNC_END
#endif
