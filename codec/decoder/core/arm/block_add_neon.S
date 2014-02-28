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
.macro	ORR_32BYTES_TO_8BYTES
//	{	//	input: smb#a q0 & q1, smb#b q2&q3, q0[0], q0[1], q2[0], q2[1], output: d_0, d_1
		vorr.s16	$0, $1
		vorr.s16	$2, $3		
		vorr.s16	$8, $4, $5
		vorr.s16	$9, $6, $7
//	}
.endm

.macro	ADD_PRED_1BYTE_TO_RESID_2BYTES
//	{	//	input: q0~q3, d0~d3, output: d0~d3;

		vaddw.u8		$0, $4
		vaddw.u8		$1, $5
		vaddw.u8		$2, $6
		vaddw.u8		$3, $7
		
		vqmovun.s16	$4, $0			//saturation
		vqmovun.s16	$6, $2	
		vqmovun.s16	$5, $1
		vqmovun.s16	$7, $3		
//	}
.endm

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

.macro	ADD_AND_CLIP_RS
//	{	//	input: src_q[0]~[1], matrix_max~min, pred_q, working_d[0]:[1], output: q;	
		vrshrn.s32		$5, $0, #6
		vrshrn.s32		$6, $1, #6
		vqadd.s16		$7, $4
		vmin.s16		$7, $7, $2
		vmax.s16		$7, $7, $3
//	}
.endm
#else
.macro	ORR_32BYTES_TO_8BYTES arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9
//	{	//	input: smb#a q0 & q1, smb#b q2&q3, q0[0], q0[1], q2[0], q2[1], output: d_0, d_1
		vorr.s16	\arg0, \arg1
		vorr.s16	\arg2, \arg3		
		vorr.s16	\arg8, \arg4, \arg5
		vorr.s16	\arg9, \arg6, \arg7
//	}
.endm

.macro	ADD_PRED_1BYTE_TO_RESID_2BYTES arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7
//	{	//	input: q0~q3, d0~d3, output: d0~d3;

		vaddw.u8		\arg0, \arg4
		vaddw.u8		\arg1, \arg5
		vaddw.u8		\arg2, \arg6
		vaddw.u8		\arg3, \arg7
		
		vqmovun.s16	\arg4, \arg0			//saturation
		vqmovun.s16	\arg6, \arg2	
		vqmovun.s16	\arg5, \arg1
		vqmovun.s16	\arg7, \arg3		
//	}
.endm

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

.macro	ADD_AND_CLIP_RS arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7
//	{	//	input: src_q[0]~[1], matrix_max~min, pred_q, working_d[0]:[1], output: q;	
		vrshrn.s32		\arg5, \arg0, #6
		vrshrn.s32		\arg6, \arg1, #6
		vqadd.s16		\arg7, \arg4
		vmin.s16		\arg7, \arg7, \arg2
		vmax.s16		\arg7, \arg7, \arg3
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

// r0    int16_t* block,
// r1    int8_t* non_zero_count,
  WELS_ASM_FUNC_BEGIN svc_non_zero_count_neon
	push		{r2-r4}
	mov			r4, #3
	mov			r3, #64
	add			r2, r0, #32
	pld			[r0, #512]
non_zero_count_two_8x8_loop:

	vld1.64	{q0, q1}, [r0,:128], r3
	vld1.64	{q2, q3}, [r2,:128], r3
	vld1.64	{q4, q5}, [r0,:128], r3
	vld1.64	{q6, q7}, [r2,:128], r3
	vld1.64	{q8, q9}, [r0,:128], r3
	vld1.64	{q10, q11}, [r2,:128], r3//load #0 8x8 block resi data,	
	vld1.64	{q12, q13}, [r0,:128], r3
	vld1.64	{q14, q15}, [r2,:128], r3//load #1 8x8 block resi data, 
	pld			[r0, #512]
	
	ORR_32BYTES_TO_8BYTES	q0, q1, q2, q3, d0, d1, d4, d5, d2, d3	// output q1
//	vceq.i16	q1, q1, #0	
	
	ORR_32BYTES_TO_8BYTES	q8, q9,q10,q11,d16,d17,d20,d21,d4,d5	// output q2
//	vceq.i16	q2, q2, #0	
	
	ORR_32BYTES_TO_8BYTES	 q4, q5, q6, q7, d8, d9, d12, d13, d10, d11	// output q5
//	vceq.i16	q5, q5, #0	

	ORR_32BYTES_TO_8BYTES	q12,q13,q14,q15,d24,d25, d28, d29, d12, d13	// output q6
//	vceq.i16	q6, q6, #0	

	vqmovn.u64	d0, q1		// 8bytes-->4bytes
	vqmovn.u64	d8, q5	
	vqmovn.u64	d1, q2					
	vqmovn.u64	d9, q6
		
	vqmovn.u32	d2, q0		// 4bytes-->2bytes
	vqmovn.u32	d3, q4

	vceq.i16	q0, q1, #0	
	vmvn    	q0, q0
	vabs.s16	q2, q0
	vmovn.u16	d6, q2		// 2bytes-->1bytes
	vst1.u8	{d6}, [r1]!
		
//	pld			[r0]
	subs		r4,	r4, #1
	bne			non_zero_count_two_8x8_loop

	pop		{r2-r4}
  WELS_ASM_FUNC_END

// r0    int16_t* block,
// r1    int8_t* non_zero_count,
  WELS_ASM_FUNC_BEGIN svc_rs_non_zero_count_neon

	vld1.i16	{q0, q1}, [r0]!		// block is unaligned!!!
	vld1.i16	{q2, q3}, [r0]!
	vld1.i16	{q4, q5}, [r0]!
	vld1.i16	{q6, q7}, [r0]!
	
	vld1.i16	{q8, q9}, [r0]!
	vld1.i16	{q10, q11}, [r0]!
	vld1.i16	{q12, q13}, [r0]!
	vld1.i16	{q14, q15}, [r0]!
	
	ORR_32BYTES_TO_8BYTES	q0, q2, q1, q3, q4, q6, q5, q7, q4, q5
	vorr.s16	q0, q4
	vorr.s16	q1, q5			// output d0~d3	
	ORR_32BYTES_TO_8BYTES	q8, q10, q9, q11, q12, q14, q13, q15, q12, q13
	vorr.s16	q6, q8, q12
	vorr.s16	q7, q9, q13	// output d12~d15
	
	vqmovn.u64	d4, q0		// 8bytes-->4bytes
	vqmovn.u64	d6, q6	
	vqmovn.u64	d5, q1
	vqmovn.u64	d7, q7
		
	vqmovn.u32	d8, q2		// 4bytes-->2bytes
	vqmovn.u32	d9, q3

	vceq.i16	q5, q4, #0	
	vmvn    	q5, q5
	vabs.s16	q5, q5
	vmovn.u16	d10, q5	// 2bytes-->1bytes
	vst1.u8	{d10}, [r1]!			

	vld1.i16	{q0, q1}, [r0]!
	vld1.i16	{q2, q3}, [r0]!
	vld1.i16	{q4, q5}, [r0]!
	vld1.i16	{q6, q7}, [r0]!
	
	vld1.i16	{q8, q9}, [r0]!
	vld1.i16	{q10, q11}, [r0]!
	vld1.i16	{q12, q13}, [r0]!
	vld1.i16	{q14, q15}, [r0]!
	
	ORR_32BYTES_TO_8BYTES	q0, q2, q1, q3, q4, q6, q5, q7, q4, q5
	vorr.s16	q0, q4
	vorr.s16	q1, q5			// output d0~d3	
	ORR_32BYTES_TO_8BYTES	q8, q10, q9, q11, q12, q14, q13, q15, q12, q13
	vorr.s16	q6, q8, q12
	vorr.s16	q7, q9, q13	// output d12~d15
	
	vqmovn.u64	d4, q0		// 8bytes-->4bytes
	vqmovn.u64	d6, q6	
	vqmovn.u64	d5, q1
	vqmovn.u64	d7, q7
		
	vqmovn.u32	d8, q2		// 4bytes-->2bytes
	vqmovn.u32	d9, q3

	vceq.i16	q5, q4, #0	
	vmvn    	q5, q5
	vabs.s16	q5, q5
	vmovn.u16	d10, q5	// 2bytes-->1bytes
	vst1.u8	{d10}, [r1]!
	
//	Chroma
	vld1.i16	{q0, q1}, [r0]!
	vld1.i16	{q2, q3}, [r0]!
	vld1.i16	{q4, q5}, [r0]!
	vld1.i16	{q6, q7}, [r0]!	//load Cb block,
	
	vld1.i16	{q8, q9}, [r0]!
	vld1.i16	{q10, q11}, [r0]!		
	vld1.i16	{q12, q13}, [r0]!
	vld1.i16	{q14, q15}, [r0]!	//load Cr block, 

	ORR_32BYTES_TO_8BYTES	q0, q1, q2, q3, q4, q5, q6, q7, q4, q6
	vorr.s16	q0, q2
	vorr.s16	q1, q4, q6			// output d0~d3
	ORR_32BYTES_TO_8BYTES	q8, q9, q10, q11, q12, q13, q14, q15, q12, q14
	vorr.s16	q2, q8, q10
	vorr.s16	q3, q12, q14		// output d4~d7			
		
	vqmovn.u64	d8, q0		// 8bytes-->4bytes
	vqmovn.u64	d10, q2	
	vqmovn.u64	d9, q1
	vqmovn.u64	d11, q3
		
	vqmovn.u32	d12, q4		// 4bytes-->2bytes
	vqmovn.u32	d13, q5

	vceq.i16	q7, q6, #0	
	vmvn    	q7, q7	
	vabs.s16	q7, q7
	vmovn.u16	d10, q7	// 2bytes-->1bytes
	vst1.u8	{d10}, [r1]!		
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

//	r0	int8_t* dst_addr, 
//	r1	memset_value
//	r2	int32_t bytes_nmb,

  WELS_ASM_FUNC_BEGIN svc_block_memset_neon// dst should continue
	vdup.u8	q0, r1
	vdup.u8	q1, r1
		
block_memset_loop:	
	vst1.64	{q0, q1}, [r0,:64]!
	subs		r2,	r2, #64
	vst1.64	{q0, q1}, [r0,:64]!
	bne			block_memset_loop
  WELS_ASM_FUNC_END

//	int16_t* dst, 
//	int16_t* src,
//	int32_t stride	
  WELS_ASM_FUNC_BEGIN svc_block_copy_16x16_neon
	push		{r3}
	mov			r3, #16
// each element is sizeof(int16_t)
	lsl			r2, r2, #1	// r2 = 2*r2

block_copy_16x16_luma_loop:	
	vld1.i16	{q0, q1}, [r1], r2
	subs		r3,	r3, #1
	vst1.i16	{q0, q1}, [r0]!
	bne			block_copy_16x16_luma_loop
	
	pop		{r3}
  WELS_ASM_FUNC_END
	
  WELS_ASM_FUNC_BEGIN svc_block_copy_8x8_neon
	push		{r3}
	mov			r3, #8
// each element is sizeof(int16_t)
	lsl			r2, r2, #1	// r2 = 2*r2

block_copy_8x8_chma_loop:	
	vld1.i16	{q0}, [r1], r2
	subs		r3,	r3, #1
	vst1.i16	{q0}, [r0]!
	bne			block_copy_8x8_chma_loop
	
	pop		{r3}
  WELS_ASM_FUNC_END

// r0    uint8_t * dest,
// r1    uint8_t * pred,
// r2    int16_t * res,
// r3    int32_t stride,
  WELS_ASM_FUNC_BEGIN svc_block_add_16x16_neon
	push		{r4}
	mov		r4, #16
	pld		[r1]	
block_recon_16x16_luma_loop:

	vld1.64		{d16,d17}, [r1,:64], r3		//load 16 pred data, update addr
	vld1.s16		{q0, q1}, [r2]!				//load 8+8 resi data, update addr
	vld1.64		{d18,d19}, [r1,:64], r3
	vld1.s16		{q2, q3}, [r2]!
	ADD_PRED_1BYTE_TO_RESID_2BYTES		q0, q1, q2, q3, d16, d17, d18, d19
	pld		[r1]
	vst1.64         {q8}, [r0], r3      //store result		
	vst1.64         {q9}, [r0], r3
//#ifdef	DEBUG_NEON
//	vst1.u8         {q8}, [r0]!		
//	vst1.u8         {q9}, [r0]!
//#endif

	vld1.64		{d20,d21}, [r1,:64], r3		//load 16 pred data, update addr
	vld1.s16		{q4, q5}, [r2]!			//load 8+8 resi data, update addr
	vld1.64		{d22,d23}, [r1,:64], r3
	vld1.s16		{q6, q7}, [r2]!
	ADD_PRED_1BYTE_TO_RESID_2BYTES		q4, q5, q6, q7, d20, d21, d22, d23
	pld		[r1]
	vst1.64         {q10}, [r0], r3
	vst1.64         {q11}, [r0], r3
//#ifdef	DEBUG_NEON
//	vst1.u8         {q10}, [r0]!
//	vst1.u8         {q11}, [r0]!
//#endif

	subs		r4, r4, #4
	bne		block_recon_16x16_luma_loop

	pop		{r4}
  WELS_ASM_FUNC_END


  WELS_ASM_FUNC_BEGIN svc_block_add_8x8_neon

	vld1.u8		{d24}, [r1], r3		//load 8 pred data
	vld1.i16		{q8, q9}, [r2]!		//load 8+8 resi data, update addr	
	vld1.u8		{d25}, [r1], r3		//load 8 pred data, q12	
	vld1.i16		{q10, q11}, [r2]!		//load 8+8 resi data, update addr
	vld1.u8		{d26}, [r1], r3		//load 8 pred data
	vld1.u8		{d27}, [r1], r3		//load 8 pred data, q13

	ADD_PRED_1BYTE_TO_RESID_2BYTES		q8, q9, q10, q11, d24, d25, d26, d27
	pld		[r1]
	vst1.u8         {d24}, [r0], r3      //store result	 
	vst1.u8         {d25}, [r0], r3      //store result	 
	vst1.u8         {d26}, [r0], r3      //store result	 
	vst1.u8         {d27}, [r0], r3      //store result		
//#ifdef	DEBUG_NEON
//	vst1.u8         {d24}, [r0]!
//#endif
	
	vld1.u8		{d24}, [r1], r3		//load 8 pred data
	vld1.i16		{q8, q9}, [r2]!		//load 8+8 resi data, update addr	
	vld1.u8		{d25}, [r1], r3		//load 8 pred data, q12	
	vld1.i16		{q10, q11}, [r2]!		//load 8+8 resi data, update addr
	vld1.u8		{d26}, [r1], r3		//load 8 pred data
	vld1.u8		{d27}, [r1], r3		//load 8 pred data, q13

	ADD_PRED_1BYTE_TO_RESID_2BYTES		q8, q9, q10, q11, d24, d25, d26, d27
	vst1.u8         {d24}, [r0], r3      //store result	 
	vst1.u8         {d25}, [r0], r3      //store result	 
	vst1.u8         {d26}, [r0], r3      //store result	 
	vst1.u8         {d27}, [r0], r3      //store result		
//#ifdef	DEBUG_NEON
//	vst1.u8         {d24}, [r0]!
//#endif
  WELS_ASM_FUNC_END


//	int16_t* dst,
//	int16_t* src,
//	int stride
  WELS_ASM_FUNC_BEGIN svc_simple_idct4x4_neon

	vld4.s16		{d0, d1, d2, d3}, [r1]	// cost 3 cycles!
	lsl			r2, r2, #1	

	ROW_TRANSFORM_1_STEP		d0, d1, d2, d3, q4, q5, q6, q7, d4, d5
	
	TRANSFORM_4BYTES		q0, q1, q2, q3, q4, q5, q6, q7
	
	// transform element 32bits
	vtrn.s32		q0, q1				//[0 1 2 3]+[4 5 6 7]-->[0 4 2 6]+[1 5 3 7]
	vtrn.s32		q2, q3				//[8 9 10 11]+[12 13 14 15]-->[8 12 10 14]+[9 13 11 15]
	vswp			d1, d4				//[0 4 2 6]+[8 12 10 14]-->[0 4 8 12]+[2 6 10 14]
	vswp			d3, d6				//[1 5 3 7]+[9 13 11 15]-->[1 5 9 13]+[3 7 11 15]

	COL_TRANSFORM_1_STEP		q0, q1, q2, q3, q4, q5, q6, q7
	
	TRANSFORM_4BYTES		q0, q1, q2, q3, q4, q5, q6, q7	

	vrshrn.s32		d0, q0, #6	
	vst1.s16		{d0}, [r0], r2	//store			
	vrshrn.s32		d1, q1, #6	
	vst1.s16		{d1}, [r0], r2	//store	
	vrshrn.s32		d2, q2, #6
	vst1.s16		{d2}, [r0], r2	//store				
	vrshrn.s32		d3, q3, #6	
	vst1.s16		{d3}, [r0], r2	//store			

  WELS_ASM_FUNC_END
//	int16_t* dst,
//	int16_t* src,
//	int stride
  WELS_ASM_FUNC_BEGIN svc_idct4x4_add_neon

	vld4.s16		{d0, d1, d2, d3}, [r1]		// cost 3 cycles!	
	lsl			r2, r2, #1	
	
	ROW_TRANSFORM_1_STEP		d0, d1, d2, d3, q4, q5, q6, q7, d4, d5
	
	TRANSFORM_4BYTES		q0, q1, q2, q3, q4, q5, q6, q7	
	
	// transform element 32bits
	vtrn.s32		q0, q1				//[0 1 2 3]+[4 5 6 7]-->[0 4 2 6]+[1 5 3 7]
	vtrn.s32		q2, q3				//[8 9 10 11]+[12 13 14 15]-->[8 12 10 14]+[9 13 11 15]
	vswp			d1, d4				//[0 4 2 6]+[8 12 10 14]-->[0 4 8 12]+[2 6 10 14]
	vswp			d3, d6				//[1 5 3 7]+[9 13 11 15]-->[1 5 9 13]+[3 7 11 15]

	COL_TRANSFORM_1_STEP		q0, q1, q2, q3, q4, q5, q6, q7
	
	TRANSFORM_4BYTES		q0, q1, q2, q3, q4, q5, q6, q7	
			
	//see draft G.8.5.3 , after clip_rs() into [-255, 255]
	vmov.i16		q10,#0xFF
	veor			q11, q11
	vsub.i16		q11, q11,q10
//	vmvn.i16		q11,#0xFF

	mov			r1, r0
	vld1.s16		{d16}, [r0], r2	
	vld1.s16		{d17}, [r0], r2
	ADD_AND_CLIP_RS	q0, q1, q10, q11, q8, d8, d9, q4
	vst1.s16		{d8}, [r1], r2	//store
	vst1.s16		{d9}, [r1], r2	//store	
			
	vld1.s16		{d18}, [r0], r2	
	vld1.s16		{d19}, [r0], r2
	ADD_AND_CLIP_RS	q2, q3, q10, q11, q9, d10, d11, q5	
	vst1.s16		{d10}, [r1], r2	//store
	vst1.s16		{d11}, [r1], r2	//store
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
