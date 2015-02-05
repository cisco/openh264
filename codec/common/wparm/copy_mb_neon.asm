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
LOAD_ALIGNED_DATA_WITH_STRIDE $0, $1, $2, $3, $4, $5
;//  {   //  input: $0~$3, src*, src_stride
    vld1.64 {$0}, [$4@128], $5
    vld1.64 {$1}, [$4@128], $5
    vld1.64 {$2}, [$4@128], $5
    vld1.64 {$3}, [$4@128], $5
;//  }
MEND

 MACRO
STORE_ALIGNED_DATA_WITH_STRIDE $0, $1, $2, $3, $4, $5
;//  {   //  input: $0~$3, dst*, dst_stride
    vst1.64 {$0}, [$4@128], $5
    vst1.64 {$1}, [$4@128], $5
    vst1.64 {$2}, [$4@128], $5
    vst1.64 {$3}, [$4@128], $5
;//  }
MEND

 MACRO
LOAD_UNALIGNED_DATA_WITH_STRIDE $0, $1, $2, $3, $4, $5
;//  {   //  input: $0~$3, src*, src_stride
    vld1.64 {$0}, [$4], $5
    vld1.64 {$1}, [$4], $5
    vld1.64 {$2}, [$4], $5
    vld1.64 {$3}, [$4], $5
;//  }
MEND

 MACRO
STORE_UNALIGNED_DATA_WITH_STRIDE $0, $1, $2, $3, $4, $5
;//  {   //  input: $0~$3, dst*, dst_stride
    vst1.64 {$0}, [$4], $5
    vst1.64 {$1}, [$4], $5
    vst1.64 {$2}, [$4], $5
    vst1.64 {$3}, [$4], $5
;//  }
MEND

 WELS_ASM_FUNC_BEGIN WelsCopy8x8_neon

    LOAD_UNALIGNED_DATA_WITH_STRIDE d0, d1, d2, d3, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    d0, d1, d2, d3, r0, r1

    LOAD_UNALIGNED_DATA_WITH_STRIDE d4, d5, d6, d7, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    d4, d5, d6, d7, r0, r1

 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsCopy16x16_neon

    LOAD_ALIGNED_DATA_WITH_STRIDE   q0, q1, q2, q3, r2, r3

    STORE_ALIGNED_DATA_WITH_STRIDE  q0, q1, q2, q3, r0, r1

    LOAD_ALIGNED_DATA_WITH_STRIDE   q8, q9, q10, q11, r2, r3

    STORE_ALIGNED_DATA_WITH_STRIDE  q8, q9, q10, q11, r0, r1

    LOAD_ALIGNED_DATA_WITH_STRIDE   q0, q1, q2, q3, r2, r3

    STORE_ALIGNED_DATA_WITH_STRIDE  q0, q1, q2, q3, r0, r1

    LOAD_ALIGNED_DATA_WITH_STRIDE   q8, q9, q10, q11, r2, r3

    STORE_ALIGNED_DATA_WITH_STRIDE  q8, q9, q10, q11, r0, r1

 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsCopy16x16NotAligned_neon

    LOAD_UNALIGNED_DATA_WITH_STRIDE q0, q1, q2, q3, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    q0, q1, q2, q3, r0, r1

    LOAD_UNALIGNED_DATA_WITH_STRIDE q8, q9, q10, q11, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    q8, q9, q10, q11, r0, r1

    LOAD_UNALIGNED_DATA_WITH_STRIDE q0, q1, q2, q3, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    q0, q1, q2, q3, r0, r1

    LOAD_UNALIGNED_DATA_WITH_STRIDE q8, q9, q10, q11, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    q8, q9, q10, q11, r0, r1

 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsCopy16x8NotAligned_neon

    LOAD_UNALIGNED_DATA_WITH_STRIDE q0, q1, q2, q3, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    q0, q1, q2, q3, r0, r1

    LOAD_UNALIGNED_DATA_WITH_STRIDE q8, q9, q10, q11, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    q8, q9, q10, q11, r0, r1

 WELS_ASM_FUNC_END


 WELS_ASM_FUNC_BEGIN WelsCopy8x16_neon

    LOAD_UNALIGNED_DATA_WITH_STRIDE d0, d1, d2, d3, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    d0, d1, d2, d3, r0, r1

    LOAD_UNALIGNED_DATA_WITH_STRIDE d4, d5, d6, d7, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    d4, d5, d6, d7, r0, r1

    LOAD_UNALIGNED_DATA_WITH_STRIDE d0, d1, d2, d3, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    d0, d1, d2, d3, r0, r1

    LOAD_UNALIGNED_DATA_WITH_STRIDE d4, d5, d6, d7, r2, r3

    STORE_UNALIGNED_DATA_WITH_STRIDE    d4, d5, d6, d7, r0, r1

 WELS_ASM_FUNC_END

 end
