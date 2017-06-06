;/*!
; * \copy
; *     Copyright (c)  2013, Cisco Systems
; *     All rights reserved.
; *
; *     Redistribution and use in source and binary forms, with or without
; *     modification, are permitted provided that the following conditions
; *     are met:
; *
; *        * Redistributions of source code must retain the above copyright
; *          notice, this list of conditions and the following disclaimer.
; *
; *        * Redistributions in binary form must reproduce the above copyright
; *          notice, this list of conditions and the following disclaimer in
; *          the documentation and/or other materials provided with the
; *          distribution.
; *
; *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
; *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
; *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
; *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
; *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
; *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
; *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
; *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
; *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; *     POSSIBILITY OF SUCH DAMAGE.
; *
; */

 THUMB

 GET arm_arch_common_macro.S

 AREA	 |.text|, CODE, THUMB


 MACRO
LOAD_4x4_DATA_FOR_DCT $0, $1, $2, $3, $4, $5, $6, $7
;//  {   ;//  input: $0~$3, src1*, src1_stride, src2*, src2_stride
    vld2.16 {$0[0],$1[0]}, [$4], $5
    vld2.16 {$2[0],$3[0]}, [$6], $7
    vld2.16 {$0[1],$1[1]}, [$4], $5
    vld2.16 {$2[1],$3[1]}, [$6], $7

    vld2.16 {$0[2],$1[2]}, [$4], $5
    vld2.16 {$2[2],$3[2]}, [$6], $7
    vld2.16 {$0[3],$1[3]}, [$4], $5
    vld2.16 {$2[3],$3[3]}, [$6], $7
;//  }
MEND

 MACRO
LOAD_8x8_DATA_FOR_DCT $0, $1, $2, $3, $4, $5, $6, $7, $8, $9
;//  {   ;//  input: $0~$3, src1*, src2*; untouched r2:src1_stride &r4:src2_stride
    vld1.64 {$0}, [$8], r2
    vld1.64 {$4}, [$9], r4
    vld1.64 {$1}, [$8], r2
    vld1.64 {$5}, [$9], r4

    vld1.64 {$2}, [$8], r2
    vld1.64 {$6}, [$9], r4
    vld1.64 {$3}, [$8], r2
    vld1.64 {$7}, [$9], r4
;//  }
MEND

 MACRO
DCT_ROW_TRANSFORM_TOTAL_16BITS $0, $1, $2, $3, $4, $5, $6, $7
;//  {   ;//  input: src_d[0]~[3], working: [4]~[7]
    vadd.s16        $4, $0, $3          ;//int16 s[0] = data[i] + data[i3];
    vsub.s16        $7, $0, $3          ;//int16 s[3] = data[i] - data[i3];
    vadd.s16        $5, $1, $2          ;//int16 s[1] = data[i1] + data[i2];
    vsub.s16        $6, $1, $2          ;//int16 s[2] = data[i1] - data[i2];

    vadd.s16        $0, $4, $5          ;//int16 dct[i ] = s[0] + s[1];
    vsub.s16        $2, $4, $5          ;//int16 dct[i2] = s[0] - s[1];
    vshl.s16        $1, $7, #1
    vshl.s16        $3, $6, #1
    vadd.s16        $1, $1, $6          ;//int16 dct[i1] = (s[3] << 1) + s[2];
    vsub.s16        $3, $7, $3          ;//int16 dct[i3] = s[3] - (s[2] << 1);
;//  }
MEND

 MACRO
MATRIX_TRANSFORM_EACH_16BITS $0, $1, $2, $3
;//  {   ;//  input & output: src_d[0]~[3];[0 1 2 3]+[4 5 6 7]+[8 9 10 11]+[12 13 14 15]
    vtrn.s16        $0, $1              ;//[0 1 2 3]+[4 5 6 7]-->[0 4 2 6]+[1 5 3 7]
    vtrn.s16        $2, $3              ;//[8 9 10 11]+[12 13 14 15]-->[8 12 10 14]+[9 13 11 15]
    vtrn.32     $0, $2              ;//[0 4 2 6]+[8 12 10 14]-->[0 4 8 12]+[2 6 10 14]
    vtrn.32     $1, $3              ;//[1 5 3 7]+[9 13 11 15]-->[1 5 9 13]+[3 7 11 15]
;//  }
MEND

 MACRO
NEWQUANT_COEF_EACH_16BITS $0, $1, $2, $3, $4, $5, $6, $7, $8   ;// if coef <= 0, - coef; else , coef;
;//  {   ;//  input:  coef, ff (dst), ff_d0, ff_d1, mf_d0, md_d1
    veor.s16        $6, $6          ;// init 0 , and keep 0;
    vaba.s16        $1, $0, $6      ;// f + abs(coef - 0)
    vmull.s16       $7, $2, $4
    vmull.s16       $8, $3, $5
    vshr.s32        $7, #16
    vshr.s32        $8, #16
    vmovn.s32       $2, $7
    vmovn.s32       $3, $8

    vcgt.s16        $7, $0, #0      ;// if true, location of coef == 11111111
    vbif.s16        $6, $1, $7      ;// if (x<0) reserved part; else keep 0 untouched
    vshl.s16        $6, #1
    vsub.s16        $1, $1, $6      ;// if x > 0, -= 0; else x-= 2x
;//  }
MEND

 MACRO
NEWQUANT_COEF_EACH_16BITS_MAX  $0, $1, $2, $3, $4, $5, $6, $7, $8, $9  ;// if coef <= 0, - coef; else , coef;
;//  {   ;//  input:  coef, ff (dst), ff_d0, ff_d1, mf_d0(max), md_d1
    veor.s16        $6, $6          ;// init 0 , and keep 0;
    vaba.s16        $1, $0, $6      ;// f + abs(coef - 0)
    vmull.s16       $7, $2, $4
    vmull.s16       $8, $3, $5
    vshr.s32        $7, #16
    vshr.s32        $8, #16
    vmovn.s32       $2, $7
    vmovn.s32       $3, $8

    vcgt.s16        $7, $0, #0      ;// if true, location of coef == 11111111
    vbif.s16        $6, $1, $7      ;// if (x<0) reserved part; else keep 0 untouched
    vshl.s16        $6, #1
    vmax.s16        $9, $2, $3
    vsub.s16        $1, $1, $6      ;// if x > 0, -= 0; else x-= 2x
;//  }
MEND

 MACRO
QUANT_DUALWORD_COEF_EACH_16BITS  $0, $1, $2, $3, $4 ;// if coef <= 0, - coef; else , coef;
;//  {   ;//  input:  coef, ff (dst), mf , working_d (all 0), working_q
    vaba.s16        $1, $0, $3      ;// f + abs(coef - 0)
    vmull.s16       $4, $1, $2      ;// *= mf
    vshr.s32        $4, #16
    vmovn.s32       $1, $4          ;// >> 16

    vcgt.s16        $2, $0, #0      ;// if true, location of coef == 11111111
    vbif.s16        $3, $1, $2      ;// if (x<0) reserved part; else keep 0 untouched
    vshl.s16        $3, #1
    vsub.s16        $1, $1, $3      ;// if x > 0, -= 0; else x-= 2x
;//  }
MEND

 MACRO
DC_ZERO_COUNT_IN_DUALWORD $0, $1, $2
;//  {   ;//  input:  coef, dst_d, working_d (all 0x01)
    vceq.s16    $1, $0, #0
    vand.s16    $1, $2
    vpadd.s16   $1, $1, $1
    vpadd.s16   $1, $1, $1
;//  }
MEND

 MACRO
SELECT_MAX_IN_ABS_COEF $0, $1, $2, $3, $4
;//  {   ;//  input:  coef_0, coef_1, max_q (identy to follow two)
    vmax.s16        $2, $0, $1      ;// max 1st in $3 & max 2nd in $4
    vpmax.s16       $3, $3, $4      ;// max 1st in $3[0][1] & max 2nd in $3[2][3]
    vpmax.s16       $3, $3, $4      ;// max 1st in $3[0][1]
;//  }
MEND

 MACRO
ZERO_COUNT_IN_2_QUARWORD  $0, $1, $2, $3, $4, $5, $6
;//  {   ;//  input:  coef_0 (identy to $3 $4), coef_1(identy to $5 $6), mask_q
    vceq.s16    $0, #0
    vceq.s16    $1, #0
    vand.s16    $0, $2
    vand.s16    $1, $2

    vpadd.s16   $3, $3, $5
    vpadd.s16   $4, $4, $6
    vpadd.s16   $3, $3, $4      ;// 8-->4
    vpadd.s16   $3, $3, $3
    vpadd.s16   $3, $3, $3
;//  }
MEND

 MACRO
HDM_QUANT_2x2_TOTAL_16BITS $0, $1, $2
;//  {   ;//  input: src_d[0]~[3], working_d, dst_d
    vshr.s64    $1, $0, #32
    vadd.s16    $2, $0, $1      ;// [0] = rs[0] + rs[32];[1] = rs[16] + rs[48];
    vsub.s16    $1, $0, $1      ;// [0] = rs[0] - rs[32];[1] = rs[16] - rs[48];
    vtrn.s16    $2, $1
    vtrn.s32    $2, $1
;//  }
MEND

 MACRO
IHDM_4x4_TOTAL_16BITS  $0, $1, $2
;//  {   ;//  input: each src_d[0]~[3](dst), working_q0, working_q1, working_q2
    vshr.s64    $1, $0, #32
    vadd.s16    $2, $0, $1      ;// [0] = rs[0] + rs[2];[1] = rs[1] + rs[3];
    vsub.s16    $1, $0, $1      ;// [0] = rs[0] - rs[2];[1] = rs[1] - rs[3];
    vtrn.s16    $2, $1
    vrev32.16   $1, $1
    vtrn.s32    $2, $1          ;// [0] = rs[0] + rs[2];[1] = rs[0] - rs[2];[2] = rs[1] - rs[3];[3] = rs[1] + rs[3];

    vrev64.16   $1, $2
    vadd.s16    $0, $2, $1      ;// [0] = rs[0] + rs[3];[1] = rs[1] + rs[2];
    vsub.s16    $1, $2, $1
    vrev32.16   $1, $1          ;// [0] = rs[1] - rs[2];[1] = rs[0] - rs[3];
    vtrn.s32    $0, $1          ;// [0] = rs[0] + rs[3];[1] = rs[1] + rs[2];[2] = rs[1] - rs[2];[3] = rs[0] - rs[3];
;//  }
MEND

 MACRO
MB_PRED_8BITS_ADD_DCT_16BITS_CLIP $0, $1, $2, $3, $4, $5
;//  {   ;//  input: pred_d[0]/[1](output), dct_q0/1, working_q0/1;
    vmovl.u8        $4,$0
    vmovl.u8        $5,$1
    vadd.s16        $4,$2
    vadd.s16        $5,$3
    vqmovun.s16 $0,$4
    vqmovun.s16 $1,$5
;//  }
MEND

 MACRO
ROW_TRANSFORM_1_STEP_TOTAL_16BITS $0, $1, $2, $3, $4, $5, $6, $7
;//  {   ;//  input: src_d[0]~[3], output: e_d[0]~[3];
    vadd.s16        $4, $0, $2          ;//int16 e[i][0] = src[0] + src[2];
    vsub.s16        $5, $0, $2          ;//int16 e[i][1] = src[0] - src[2];
    vshr.s16        $6, $1, #1
    vshr.s16        $7, $3, #1
    vsub.s16        $6, $6, $3          ;//int16 e[i][2] = (src[1]>>1)-src[3];
    vadd.s16        $7, $1, $7          ;//int16 e[i][3] = src[1] + (src[3]>>1);
;//  }
MEND

 MACRO
TRANSFORM_TOTAL_16BITS   $0, $1, $2, $3, $4, $5, $6, $7  ;// both row & col transform used
;//  {   ;//  output: f_q[0]~[3], input: e_q[0]~[3];
    vadd.s16        $0, $4, $7          ;//int16 f[i][0] = e[i][0] + e[i][3];
    vadd.s16        $1, $5, $6          ;//int16 f[i][1] = e[i][1] + e[i][2];
    vsub.s16        $2, $5, $6          ;//int16 f[i][2] = e[i][1] - e[i][2];
    vsub.s16        $3, $4, $7          ;//int16 f[i][3] = e[i][0] - e[i][3];
;//  }
MEND


 MACRO
ROW_TRANSFORM_0_STEP  $0, $1, $2, $3, $4, $5, $6, $7
;//  {   ;//  input: src_d[0]~[3], output: e_q[0]~[3];
    vaddl.s16       $4, $0, $2          ;//int32 e[i][0] = src[0] + src[2];
    vsubl.s16       $5, $0, $2          ;//int32 e[i][1] = src[0] - src[2];
    vsubl.s16       $6, $1, $3          ;//int32 e[i][2] = src[1] - src[3];
    vaddl.s16       $7, $1, $3          ;//int32 e[i][3] = src[1] + src[3];
;//  }
MEND

 MACRO
ROW_TRANSFORM_1_STEP $0, $1, $2, $3, $4, $5, $6, $7, $8, $9
;//  {   ;//  input: src_d[0]~[3], output: e_q[0]~[3]; working: $8 $9
    vaddl.s16       $4, $0, $2          ;//int32 e[i][0] = src[0] + src[2];
    vsubl.s16       $5, $0, $2          ;//int32 e[i][1] = src[0] - src[2];
    vshr.s16        $8, $1, #1
    vshr.s16        $9, $3, #1
    vsubl.s16       $6, $8, $3          ;//int32 e[i][2] = (src[1]>>1)-src[3];
    vaddl.s16       $7, $1, $9          ;//int32 e[i][3] = src[1] + (src[3]>>1);
;//  }
MEND

 MACRO
TRANSFORM_4BYTES  $0, $1, $2, $3, $4, $5, $6, $7  ;// both row & col transform used
;//  {   ;//  output: f_q[0]~[3], input: e_q[0]~[3];
    vadd.s32        $0, $4, $7          ;//int16 f[i][0] = e[i][0] + e[i][3];
    vadd.s32        $1, $5, $6          ;//int16 f[i][1] = e[i][1] + e[i][2];
    vsub.s32        $2, $5, $6          ;//int16 f[i][2] = e[i][1] - e[i][2];
    vsub.s32        $3, $4, $7          ;//int16 f[i][3] = e[i][0] - e[i][3];
;//  }
MEND

 MACRO
COL_TRANSFORM_0_STEP  $0, $1, $2, $3, $4, $5, $6, $7
;//  {   ;//  input: src_q[0]~[3], output: e_q[0]~[3];
    vadd.s32        $4, $0, $2          ;//int32 e[0][j] = f[0][j] + f[2][j];
    vsub.s32        $5, $0, $2          ;//int32 e[1][j] = f[0][j] - f[2][j];
    vsub.s32        $6, $1, $3          ;//int32 e[2][j] = (f[1][j]>>1) - f[3][j];
    vadd.s32        $7, $1, $3          ;//int32 e[3][j] = f[1][j] + (f[3][j]>>1);
;//  }
MEND

 MACRO
COL_TRANSFORM_1_STEP  $0, $1, $2, $3, $4, $5, $6, $7
;//  {   ;//  input: src_q[0]~[3], output: e_q[0]~[3];
    vadd.s32        $4, $0, $2          ;//int32 e[0][j] = f[0][j] + f[2][j];
    vsub.s32        $5, $0, $2          ;//int32 e[1][j] = f[0][j] - f[2][j];
    vshr.s32        $6, $1, #1
    vshr.s32        $7, $3, #1
    vsub.s32        $6, $6, $3          ;//int32 e[2][j] = (f[1][j]>>1) - f[3][j];
    vadd.s32        $7, $1, $7          ;//int32 e[3][j] = f[1][j] + (f[3][j]>>1);
;//  }
MEND


 WELS_ASM_FUNC_BEGIN WelsDctT4_neon
    push        {r4}
    ldr         r4, [sp, #4]

    LOAD_4x4_DATA_FOR_DCT   d4, d5, d6, d7, r1, r2, r3, r4

    vsubl.u8    q0, d4, d6
    vsubl.u8    q1, d5, d7
    vtrn.s32    q0, q1
    vswp        d1, d2

    ;// horizontal transform
    DCT_ROW_TRANSFORM_TOTAL_16BITS      d0, d1, d2, d3, d4, d5, d6, d7

    ;// transform element
    MATRIX_TRANSFORM_EACH_16BITS    d0, d1, d2, d3

    ;//  vertical transform
    DCT_ROW_TRANSFORM_TOTAL_16BITS      d0, d1, d2, d3, d4, d5, d6, d7

    ;// transform element
    MATRIX_TRANSFORM_EACH_16BITS    d0, d1, d2, d3

    vst1.s16        {q0, q1}, [r0]!

    pop     {r4}
 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsDctFourT4_neon
    push        {r4}
    ldr         r4, [sp, #4]

    LOAD_8x8_DATA_FOR_DCT   d16, d17, d18, d19, d20, d21, d22, d23, r1, r3

    vsubl.u8    q0, d16, d20
    vsubl.u8    q1, d17, d21
    vsubl.u8    q2, d18, d22
    vsubl.u8    q3, d19, d23
    MATRIX_TRANSFORM_EACH_16BITS    q0, q1, q2, q3

    ;// horizontal transform
    DCT_ROW_TRANSFORM_TOTAL_16BITS      q0, q1, q2, q3, q8, q9, q10, q11

    ;// transform element
    MATRIX_TRANSFORM_EACH_16BITS    q0, q1, q2, q3

    ;//  vertical transform
    DCT_ROW_TRANSFORM_TOTAL_16BITS      q0, q1, q2, q3, q8, q9, q10, q11

    vswp        d1, d2
    vswp        d5, d6
    vswp        q1, q2
    vst1.s16        {q0, q1}, [r0]!
    vst1.s16        {q2, q3}, [r0]!

    ;//;//;//;//;//;//;//;//
    LOAD_8x8_DATA_FOR_DCT   d16, d17, d18, d19, d20, d21, d22, d23, r1, r3

    vsubl.u8    q0, d16, d20
    vsubl.u8    q1, d17, d21
    vsubl.u8    q2, d18, d22
    vsubl.u8    q3, d19, d23
    MATRIX_TRANSFORM_EACH_16BITS    q0, q1, q2, q3

    ;// horizontal transform
    DCT_ROW_TRANSFORM_TOTAL_16BITS      q0, q1, q2, q3, q8, q9, q10, q11

    ;// transform element
    MATRIX_TRANSFORM_EACH_16BITS    q0, q1, q2, q3

    ;//  vertical transform
    DCT_ROW_TRANSFORM_TOTAL_16BITS      q0, q1, q2, q3, q8, q9, q10, q11

    vswp        d1, d2
    vswp        d5, d6
    vswp        q1, q2
    vst1.s16        {q0, q1}, [r0]!
    vst1.s16        {q2, q3}, [r0]!

    pop     {r4}
 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsQuant4x4_neon
    vld1.s16        {q2}, [r1]
    vld1.s16        {q0, q1}, [r0]
    vld1.s16        {q3}, [r2]

    vmov            q8, q2

    NEWQUANT_COEF_EACH_16BITS   q0, q2, d4, d5, d6, d7, q9, q10, q11
    vst1.s16        {q2}, [r0]!

    NEWQUANT_COEF_EACH_16BITS   q1, q8, d16, d17, d6, d7, q9, q10, q11
    vst1.s16        {q8}, [r0]!

 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsQuant4x4Dc_neon

    vld1.s16        {q0, q1}, [r0]
    vdup.s16        q2, r1      ;// even ff range [0, 768]
    vdup.s16        q3, r2

    vmov            q8, q2

    NEWQUANT_COEF_EACH_16BITS   q0, q2, d4, d5, d6, d7, q9, q10, q11
    vst1.s16        {q2}, [r0]!

    NEWQUANT_COEF_EACH_16BITS   q1, q8, d16, d17, d6, d7, q9, q10, q11
    vst1.s16        {q8}, [r0]!

 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsQuantFour4x4_neon
    vld1.s16        {q2}, [r1]
    vld1.s16        {q3}, [r2]
    mov             r1, r0

    vld1.s16        {q0, q1}, [r0]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS   q0, q8, d16, d17, d6, d7, q9, q10, q11
    vst1.s16        {q8}, [r1]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS   q1, q8, d16, d17, d6, d7, q9, q10, q11
    vst1.s16        {q8}, [r1]!

    vld1.s16        {q0, q1}, [r0]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS   q0, q8, d16, d17, d6, d7, q9, q10, q11
    vst1.s16        {q8}, [r1]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS   q1, q8, d16, d17, d6, d7, q9, q10, q11
    vst1.s16        {q8}, [r1]!

    vld1.s16        {q0, q1}, [r0]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS   q0, q8, d16, d17, d6, d7, q9, q10, q11
    vst1.s16        {q8}, [r1]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS   q1, q8, d16, d17, d6, d7, q9, q10, q11
    vst1.s16        {q8}, [r1]!

    vld1.s16        {q0, q1}, [r0]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS   q0, q8, d16, d17, d6, d7, q9, q10, q11
    vst1.s16        {q8}, [r1]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS   q1, q8, d16, d17, d6, d7, q9, q10, q11
    vst1.s16        {q8}, [r1]!

 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsQuantFour4x4Max_neon
    vld1.s16        {q2}, [r1]
    vld1.s16        {q3}, [r2]
    mov             r1, r0

    vld1.s16        {q0, q1}, [r0]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS_MAX   q0, q8, d16, d17, d6, d7, q9, q10, q11, d26
    vst1.s16        {q8}, [r1]!
    vmov            q12, q2
    NEWQUANT_COEF_EACH_16BITS_MAX   q1, q12, d24, d25, d6, d7, q9, q10, q11, d28
    vst1.s16        {q12}, [r1]!        ;// then 1st 16 elem in d26 & d28

    vld1.s16        {q0, q1}, [r0]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS_MAX   q0, q8, d16, d17, d6, d7, q9, q10, q11, d27
    vst1.s16        {q8}, [r1]!
    vmov            q12, q2
    NEWQUANT_COEF_EACH_16BITS_MAX   q1, q12, d24, d25, d6, d7, q9, q10, q11, d29
    vst1.s16        {q12}, [r1]!    ;// then 2nd 16 elem in d27 & d29

    SELECT_MAX_IN_ABS_COEF  q13, q14, q0, d0, d1
    vst1.s32        {d0[0]}, [r3]!

    ;//;//;//;//;///
    vld1.s16        {q0, q1}, [r0]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS_MAX   q0, q8, d16, d17, d6, d7, q9, q10, q11, d26
    vst1.s16        {q8}, [r1]!
    vmov            q12, q2
    NEWQUANT_COEF_EACH_16BITS_MAX   q1, q12, d24, d25, d6, d7, q9, q10, q11, d28
    vst1.s16        {q12}, [r1]!        ;// then 3rd 16 elem in d26 & d28

    vld1.s16        {q0, q1}, [r0]!
    vmov            q8, q2
    NEWQUANT_COEF_EACH_16BITS_MAX   q0, q8, d16, d17, d6, d7, q9, q10, q11, d27
    vst1.s16        {q8}, [r1]!
    vmov            q12, q2
    NEWQUANT_COEF_EACH_16BITS_MAX   q1, q12, d24, d25, d6, d7, q9, q10, q11, d29
    vst1.s16        {q12}, [r1]!    ;// then 4th 16 elem in d27 & d29

    SELECT_MAX_IN_ABS_COEF  q13, q14, q0, d0, d1
    vst1.s32        {d0[0]}, [r3]!

 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsHadamardT4Dc_neon
    push    {r2,r3}
    mov     r2, #64 ;// 2*16*sizeof(int16_t)
    add     r3, r1, #32

    vld1.s16        {d0}, [r1], r2
    vld1.s16        {d1}, [r3], r2
    vld1.s16        {d4}, [r1], r2
    vld1.s16        {d5}, [r3], r2
    vld1.s16        {d2}, [r1], r2
    vld1.s16        {d3}, [r3], r2
    vld1.s16        {d6}, [r1], r2
    vld1.s16        {d7}, [r3], r2
    vtrn.16     q0, q2      ;// d0[0 4], d1[1 5]
    vtrn.16     q1, q3      ;// d2[2 6], d3[3 7]

    vld1.s16        {d16}, [r1], r2
    vld1.s16        {d17}, [r3], r2
    vld1.s16        {d20}, [r1], r2
    vld1.s16        {d21}, [r3], r2
    vld1.s16        {d18}, [r1], r2
    vld1.s16        {d19}, [r3], r2
    vld1.s16        {d22}, [r1], r2
    vld1.s16        {d23}, [r3], r2
    vtrn.16     q8, q10     ;//d16[08 12],d17[09 13]
    vtrn.16     q9, q11     ;//d18[10 14],d19[11 15]

    vtrn.32     q0, q8      ;// d0 [0 4 08 12] = dct[idx],       d1[1 5 09 13] = dct[idx+16]
    vtrn.32     q1, q9      ;// d2 [2 6 10 14] = dct[idx+64],    d3[3 7 11 15] = dct[idx+80]

    ROW_TRANSFORM_0_STEP    d0, d1, d3, d2, q8, q11, q10, q9

    TRANSFORM_4BYTES        q0, q1, q3, q2, q8, q11, q10, q9

    ;// transform element 32bits
    vtrn.s32        q0, q1              ;//[0 1 2 3]+[4 5 6 7]-->[0 4 2 6]+[1 5 3 7]
    vtrn.s32        q2, q3              ;//[8 9 10 11]+[12 13 14 15]-->[8 12 10 14]+[9 13 11 15]
    vswp            d1, d4              ;//[0 4 2 6]+[8 12 10 14]-->[0 4 8 12]+[2 6 10 14]
    vswp            d3, d6              ;//[1 5 3 7]+[9 13 11 15]-->[1 5 9 13]+[3 7 11 15]

    COL_TRANSFORM_0_STEP    q0, q1, q3, q2, q8, q11, q10, q9

    TRANSFORM_4BYTES        q0, q1, q3, q2, q8, q11, q10, q9

    vrshrn.s32      d16, q0, #1
    vrshrn.s32      d17, q1, #1
    vrshrn.s32      d18, q2, #1
    vrshrn.s32      d19, q3, #1
    vst1.16 {q8, q9}, [r0]  ;//store

    pop     {r2,r3}
 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsHadamardQuant2x2_neon

    vdup.s16    d1, r1              ;//ff
    vdup.s16    d2, r2              ;//mf
    veor        d3, d3

    mov         r1, #32
    mov         r2, r0

    vld1.s16    {d0[0]}, [r0], r1       ;//rs[00]
    vst1.s16    {d3[0]}, [r2], r1       ;//rs[00]=0
    vld1.s16    {d0[1]}, [r0], r1       ;//rs[16]
    vst1.s16    {d3[0]}, [r2], r1       ;//rs[16]=0
    vld1.s16    {d0[2]}, [r0], r1       ;//rs[32]
    vst1.s16    {d3[0]}, [r2], r1       ;//rs[32]=0
    vld1.s16    {d0[3]}, [r0], r1       ;//rs[48]
    vst1.s16    {d3[0]}, [r2], r1       ;//rs[48]=0

    HDM_QUANT_2x2_TOTAL_16BITS  d0, d4, d5      ;// output d5

    HDM_QUANT_2x2_TOTAL_16BITS  d5, d4, d0      ;// output d0

    QUANT_DUALWORD_COEF_EACH_16BITS d0, d1, d2, d3, q2

    vst1.s16    d1, [r3]        ;// store to dct
    ldr         r2, [sp, #0]
    vst1.s16    d1, [r2]        ;// store to block

    mov         r1, #1
    vdup.s16    d3, r1
    DC_ZERO_COUNT_IN_DUALWORD   d1, d0, d3

    vmov    r0, r1, d0
    and     r0, #0x07       ;// range [0~4]
    rsb     r0, #4
 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsHadamardQuant2x2SkipKernel_neon

    vdup.s16    d3, r1
    mov         r1, #32
    vld1.s16    {d0[0]}, [r0], r1       ;//rs[00]
    vld1.s16    {d0[1]}, [r0], r1       ;//rs[16]
    vld1.s16    {d0[2]}, [r0], r1       ;//rs[32]
    vld1.s16    {d0[3]}, [r0], r1       ;//rs[48]

    HDM_QUANT_2x2_TOTAL_16BITS  d0, d1, d2      ;// output d2

    HDM_QUANT_2x2_TOTAL_16BITS  d2, d1, d0      ;// output d0

    vabs.s16    d1, d0
    vcgt.s16    d1, d1, d3      ;// abs(dct[i])>threshold;
    vmov    r0, r1, d1
    orr     r0, r1
 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsGetNoneZeroCount_neon
    push    {r1}
    vld1.s16    {q0, q1}, [r0]
    vmov.s16    q8, #1

    ZERO_COUNT_IN_2_QUARWORD    q0, q1, q8, d0, d1, d2, d3
    vmov    r0, r1, d0
    and     r0, #0x1F   ;// range [0~16]
    rsb     r0, #16
    pop     {r1}
 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsDequant4x4_neon
    vld1.s16    {q0, q1}, [r0]
    vld1.u16    {q2}, [r1]

    vmul.s16    q8, q0, q2
    vmul.s16    q9, q1, q2

    vst1.s16    {q8, q9}, [r0]
 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsDequantFour4x4_neon
    vld1.u16    {q12}, [r1]
    mov     r1, r0
    vld1.s16    {q0, q1}, [r0]!
    vld1.s16    {q2, q3}, [r0]!
    vmul.s16    q0, q0, q12
    vld1.s16    {q8, q9}, [r0]!
    vmul.s16    q1, q1, q12
    vld1.s16    {q10, q11}, [r0]!

    vst1.s16    {q0, q1}, [r1]!

    vmul.s16    q2, q2, q12
    vmul.s16    q3, q3, q12
    vmul.s16    q8, q8, q12
    vst1.s16    {q2, q3}, [r1]!

    vmul.s16    q9, q9, q12
    vmul.s16    q10, q10, q12
    vmul.s16    q11, q11, q12
    vst1.s16    {q8, q9}, [r1]!
    vst1.s16    {q10, q11}, [r1]!

 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsDequantIHadamard4x4_neon

    vld1.s16    {q0, q1}, [r0]
    vdup.s16    q8, r1

    IHDM_4x4_TOTAL_16BITS   q0, q2, q3
    IHDM_4x4_TOTAL_16BITS   q1, q2, q3

    MATRIX_TRANSFORM_EACH_16BITS    d0, d1, d2, d3

    IHDM_4x4_TOTAL_16BITS   q0, q2, q3
    vmul.s16    q0, q8

    IHDM_4x4_TOTAL_16BITS   q1, q2, q3
    vmul.s16    q1, q8

    MATRIX_TRANSFORM_EACH_16BITS    d0, d1, d2, d3
    vst1.s16    {q0, q1}, [r0]
 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsIDctT4Rec_neon
    vld1.u32        {d16[0]}, [r2], r3
    push            {r4}
    ldr             r4, [sp, #4]
    vld1.u32        {d16[1]}, [r2], r3

    vld4.s16        {d0, d1, d2, d3}, [r4]      ;// cost 3 cycles!
    vld1.u32        {d17[0]}, [r2], r3
    vld1.u32        {d17[1]}, [r2], r3          ;// q7 is pred

    ROW_TRANSFORM_1_STEP_TOTAL_16BITS       d0, d1, d2, d3, d4, d5, d6, d7

    TRANSFORM_TOTAL_16BITS      d0, d1, d2, d3, d4, d5, d6, d7

    MATRIX_TRANSFORM_EACH_16BITS    d0, d1, d2, d3

    ROW_TRANSFORM_1_STEP_TOTAL_16BITS       d0, d1, d2, d3, d4, d5, d6, d7

    TRANSFORM_TOTAL_16BITS      d0, d1, d2, d3, d4, d5, d6, d7
    vrshr.s16       d0, d0, #6
    vrshr.s16       d1, d1, #6
    vrshr.s16       d2, d2, #6
    vrshr.s16       d3, d3, #6

    ;//after rounding 6, clip into [0, 255]
    vmovl.u8        q2,d16
    vadd.s16        q0,q2
    vqmovun.s16 d16,q0
    vst1.32     {d16[0]},[r0],r1
    vst1.32     {d16[1]},[r0],r1

    vmovl.u8        q2,d17
    vadd.s16        q1,q2
    vqmovun.s16 d17,q1
    vst1.32     {d17[0]},[r0],r1
    vst1.32     {d17[1]},[r0]

    pop         {r4}
 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsIDctFourT4Rec_neon

    vld1.u64        {d24}, [r2], r3
    push            {r4}
    ldr             r4, [sp, #4]
    vld1.u64        {d25}, [r2], r3

    vld4.s16        {d0, d1, d2, d3}, [r4]!     ;// cost 3 cycles!
    vld1.u64        {d26}, [r2], r3
    vld1.u64        {d27}, [r2], r3
    vld4.s16        {d4, d5, d6, d7}, [r4]!     ;// cost 3 cycles!
    vswp            d1, d4
    vswp            d3, d6
    vswp            q1, q2                      ;// q0~q3

    ROW_TRANSFORM_1_STEP_TOTAL_16BITS       q0, q1, q2, q3, q8, q9, q10, q11

    TRANSFORM_TOTAL_16BITS      q0, q1, q2, q3, q8, q9, q10, q11

    MATRIX_TRANSFORM_EACH_16BITS    q0, q1, q2, q3

    ROW_TRANSFORM_1_STEP_TOTAL_16BITS       q0, q1, q2, q3, q8, q9, q10, q11

    TRANSFORM_TOTAL_16BITS      q0, q1, q2, q3, q8, q9, q10, q11
    vrshr.s16       q0, q0, #6
    vrshr.s16       q1, q1, #6
    vrshr.s16       q2, q2, #6
    vrshr.s16       q3, q3, #6

    ;//after rounding 6, clip into [0, 255]
    vmovl.u8        q8,d24
    vadd.s16        q0,q8
    vqmovun.s16 d24,q0
    vst1.u8     {d24},[r0],r1

    vmovl.u8        q8,d25
    vadd.s16        q1,q8
    vqmovun.s16 d25,q1
    vst1.u8     {d25},[r0],r1

    vmovl.u8        q8,d26
    vadd.s16        q2,q8
    vqmovun.s16 d26,q2
    vst1.u8     {d26},[r0],r1

    vmovl.u8        q8,d27
    vadd.s16        q3,q8
    vqmovun.s16 d27,q3
    vst1.u8     {d27},[r0],r1

    vld1.u64        {d24}, [r2], r3
    vld1.u64        {d25}, [r2], r3

    vld4.s16        {d0, d1, d2, d3}, [r4]!     ;// cost 3 cycles!
    vld1.u64        {d26}, [r2], r3
    vld1.u64        {d27}, [r2], r3
    vld4.s16        {d4, d5, d6, d7}, [r4]!     ;// cost 3 cycles!
    vswp            d1, d4
    vswp            d3, d6
    vswp            q1, q2                      ;// q0~q3

    ROW_TRANSFORM_1_STEP_TOTAL_16BITS       q0, q1, q2, q3, q8, q9, q10, q11

    TRANSFORM_TOTAL_16BITS      q0, q1, q2, q3, q8, q9, q10, q11

    MATRIX_TRANSFORM_EACH_16BITS    q0, q1, q2, q3

    ROW_TRANSFORM_1_STEP_TOTAL_16BITS       q0, q1, q2, q3, q8, q9, q10, q11

    TRANSFORM_TOTAL_16BITS      q0, q1, q2, q3, q8, q9, q10, q11
    vrshr.s16       q0, q0, #6
    vrshr.s16       q1, q1, #6
    vrshr.s16       q2, q2, #6
    vrshr.s16       q3, q3, #6

    ;//after rounding 6, clip into [0, 255]
    vmovl.u8        q8,d24
    vadd.s16        q0,q8
    vqmovun.s16 d24,q0
    vst1.u8     {d24},[r0],r1

    vmovl.u8        q8,d25
    vadd.s16        q1,q8
    vqmovun.s16 d25,q1
    vst1.u8     {d25},[r0],r1

    vmovl.u8        q8,d26
    vadd.s16        q2,q8
    vqmovun.s16 d26,q2
    vst1.u8     {d26},[r0],r1

    vmovl.u8        q8,d27
    vadd.s16        q3,q8
    vqmovun.s16 d27,q3
    vst1.u8     {d27},[r0],r1

    pop         {r4}
 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsIDctRecI16x16Dc_neon
    push        {r4}
    ldr         r4, [sp, #4]

    vld1.s16    {q8,q9}, [r4]
    vrshr.s16       q8, q8, #6
    vrshr.s16       q9, q9, #6

    vdup.s16    d20, d16[0]
    vdup.s16    d21, d16[1]
    vdup.s16    d22, d16[2]
    vdup.s16    d23, d16[3]

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vdup.s16    d20, d17[0]
    vdup.s16    d21, d17[1]
    vdup.s16    d22, d17[2]
    vdup.s16    d23, d17[3]

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vdup.s16    d20, d18[0]
    vdup.s16    d21, d18[1]
    vdup.s16    d22, d18[2]
    vdup.s16    d23, d18[3]

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vdup.s16    d20, d19[0]
    vdup.s16    d21, d19[1]
    vdup.s16    d22, d19[2]
    vdup.s16    d23, d19[3]

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    vld1.u8 {q0}, [r2], r3
    MB_PRED_8BITS_ADD_DCT_16BITS_CLIP   d0, d1, q10, q11, q12, q13
    vst1.u8 {q0}, [r0], r1

    pop         {r4}
 WELS_ASM_FUNC_END

 end
