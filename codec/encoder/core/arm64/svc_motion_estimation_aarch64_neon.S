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

#ifdef  HAVE_NEON_AARCH64
.text
#include "arm_arch64_common_macro.S"

WELS_ASM_AARCH64_FUNC_BEGIN SumOf8x8SingleBlock_AArch64_neon
    ld1 {v0.d}[0], [x0], x1
    ld1 {v0.d}[1], [x0], x1
    ld1 {v1.d}[0], [x0], x1
    ld1 {v1.d}[1], [x0], x1
    ld1 {v2.d}[0], [x0], x1
    ld1 {v2.d}[1], [x0], x1
    ld1 {v3.d}[0], [x0], x1
    ld1 {v3.d}[1], [x0]
    uaddlp v0.8h, v0.16b
    uadalp v0.8h, v1.16b
    uadalp v0.8h, v2.16b
    uadalp v0.8h, v3.16b
    uaddlv s0, v0.8h
    mov    x0, v0.d[0]
WELS_ASM_AARCH64_FUNC_END

WELS_ASM_AARCH64_FUNC_BEGIN SumOf16x16SingleBlock_AArch64_neon
    ld1 {v0.16b}, [x0], x1
    uaddlp v0.8h, v0.16b
.rept 15
    ld1 {v1.16b}, [x0], x1
    uadalp v0.8h, v1.16b
.endr
    uaddlv s0, v0.8h
    mov    x0, v0.d[0]
WELS_ASM_AARCH64_FUNC_END

WELS_ASM_AARCH64_FUNC_BEGIN SumOf8x8BlockOfFrame_AArch64_neon
//(uint8_t* pRefPicture, const int32_t kiWidth, const int32_t kiHeight,const int32_t kiRefStride,uint16_t* pFeatureOfBlock, uint32_t pTimesOfFeatureValue[])
    //x5: pTimesOfFeatureValue
    //x4: pFeatureOfBlock

    mov x8, x0
    mov x6, x1
    add x8, x8, x6
    add x4, x4, x6, lsl #1

    mov x7, x6
_width_loop8x8_1:
    subs x0, x8, x7
    ld1 {v0.d}[0], [x0], x3
    ld1 {v0.d}[1], [x0], x3
    ld1 {v1.d}[0], [x0], x3
    ld1 {v1.d}[1], [x0], x3
    ld1 {v2.d}[0], [x0], x3
    ld1 {v2.d}[1], [x0], x3
    ld1 {v3.d}[0], [x0], x3
    ld1 {v3.d}[1], [x0]
    uaddlp v0.8h, v0.16b
    uadalp v0.8h, v1.16b
    uadalp v0.8h, v2.16b
    uadalp v0.8h, v3.16b
    uaddlv s0, v0.8h

    subs x1, x4, x7, lsl #1
    st1 {v0.h}[0], [x1] // sum -> pFeatureOfBlock[i]
    mov w0, #0
    ins v0.s[1], w0
    mov    x0, v0.d[0]
    add x1, x5, x0, lsl #2
    ldr w0, [x1]
    add w0, w0, #1
    str w0, [x1]
    subs x7, x7, #1
    cbnz x7, _width_loop8x8_1

    add x8, x8, x3
    add x4, x4, x6, lsl #1
    subs x2, x2, #1
    cbz x2, _SumOf8x8BlockOfFrame_AArch64_neon_end

_height_loop8x8:
    mov x7, x6
_width_loop8x8_2:
    subs x0, x8, x7
    subs x1, x4, x7, lsl #1
    subs x9, x1, x6, lsl #1 // last line of pFeatureOfBlock[i]
    ldrh  w10, [x9] // sum of last line of pFeatureOfBlock[i]

    subs x11, x0, x3
    ld1 {v0.d}[1], [x11]
    add x0, x11, x3, lsl #3
    ld1 {v0.d}[0], [x0] //

    uaddlp v0.8h, v0.16b
    addp v0.8h, v0.8h, v1.8h
    uaddlp v0.4s, v0.8h
    umov w11, v0.s[0]
    umov w12, v0.s[1]

    subs w10, w10, w12
    mov x0, #0
    add w0, w10, w11
    strh w0, [x1] // sum -> pFeatureOfBlock[i]
    add x1, x5, x0, lsl #2
    ldr w0, [x1]
    add w0, w0, #1
    str w0, [x1]
    subs x7, x7, #1
    cbnz x7, _width_loop8x8_2

    add x8, x8, x3
    add x4, x4, x6, lsl #1
    subs x2, x2, #1
    cbnz x2, _height_loop8x8
_SumOf8x8BlockOfFrame_AArch64_neon_end:
WELS_ASM_AARCH64_FUNC_END

WELS_ASM_AARCH64_FUNC_BEGIN SumOf16x16BlockOfFrame_AArch64_neon
//(uint8_t* pRefPicture, const int32_t kiWidth, const int32_t kiHeight,const int32_t kiRefStride,uint16_t* pFeatureOfBlock, uint32_t pTimesOfFeatureValue[])
    //x5: pTimesOfFeatureValue
    //x4: pFeatureOfBlock

    mov x8, x0
    mov x6, x1
    add x8, x8, x6
    add x4, x4, x6, lsl #1

    mov x7, x6
_width_loop16x16_1:
    subs x0, x8, x7
    ld1 {v0.16b}, [x0], x3
    uaddlp v0.8h, v0.16b
.rept 15
    ld1 {v1.16b}, [x0], x3
    uadalp v0.8h, v1.16b
.endr
    uaddlv s0, v0.8h

    subs x1, x4, x7, lsl #1
    st1 {v0.h}[0], [x1] // sum -> pFeatureOfBlock[i]
    mov w0, #0
    ins v0.s[1], w0
    mov    x0, v0.d[0]
    add x1, x5, x0, lsl #2
    ldr w0, [x1]
    add w0, w0, #1
    str w0, [x1]
    subs x7, x7, #1
    cbnz x7, _width_loop16x16_1

    add x8, x8, x3
    add x4, x4, x6, lsl #1
    subs x2, x2, #1
    cbz x2, _SumOf16x16BlockOfFrame_AArch64_neon_end

_height_loop16x16:
    mov x7, x6
_width_loop16x16_2:
    subs x0, x8, x7

    subs x1, x4, x7, lsl #1
    subs x9, x1, x6, lsl #1 // last line of pFeatureOfBlock[i]
    ldrh  w10, [x9] // sum of last line of pFeatureOfBlock[i]

    subs x11, x0, x3
    ld1 {v1.16b}, [x11]
    add x0, x11, x3, lsl #4
    ld1 {v0.16b}, [x0] //

    uaddlv h0, v0.16b
    uaddlv h1, v1.16b
    umov w11, v0.h[0]
    umov w12, v1.h[0]

    subs w10, w10, w12
    mov x0, #0
    add w0, w10, w11
    strh w0, [x1] // sum -> pFeatureOfBlock[i]
    add x1, x5, x0, lsl #2
    ldr w0, [x1]
    add w0, w0, #1
    str w0, [x1]
    subs x7, x7, #1
    cbnz x7, _width_loop16x16_2

    add x8, x8, x3
    add x4, x4, x6, lsl #1
    subs x2, x2, #1
    cbnz x2, _height_loop16x16
_SumOf16x16BlockOfFrame_AArch64_neon_end:
WELS_ASM_AARCH64_FUNC_END
#endif