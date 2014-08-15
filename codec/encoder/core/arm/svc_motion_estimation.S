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

#ifdef  HAVE_NEON
.text
#include "arm_arch_common_macro.S"


WELS_ASM_FUNC_BEGIN SumOf8x8SingleBlock_neon
    vld1.64 {d0}, [r0], r1
    vld1.64 {d1}, [r0], r1
    vld1.64 {d2}, [r0], r1
    vld1.64 {d3}, [r0], r1
    vld1.64 {d4}, [r0], r1
    vld1.64 {d5}, [r0], r1
    vld1.64 {d6}, [r0], r1
    vld1.64 {d7}, [r0]
    vpaddl.u8 q0, q0
    vpadal.u8 q0, q1
    vpadal.u8 q0, q2
    vpadal.u8 q0, q3

    vpaddl.u16 q0, q0
    vpadd.i32 d0, d1
    vpadd.i32 d0, d0
    vmov    r0, r1, d0
WELS_ASM_FUNC_END


WELS_ASM_FUNC_BEGIN SumOf16x16SingleBlock_neon
    vld1.64 {q0}, [r0], r1
    vpaddl.u8 q0, q0
.rept 15
    vld1.64 {q1}, [r0], r1
    vpadal.u8 q0, q1
.endr
    vpaddl.u16 q0, q0
    vpadd.i32 d0, d1
    vpadd.i32 d0, d0
    vmov    r0, r1, d0
WELS_ASM_FUNC_END


WELS_ASM_FUNC_BEGIN SumOf8x8BlockOfFrame_neon
//(uint8_t* pRefPicture, const int32_t kiWidth, const int32_t kiHeight,const int32_t kiRefStride,uint16_t* pFeatureOfBlock, uint32_t pTimesOfFeatureValue[])
    stmdb sp!, {r4-r12}
    ldr	r5, [sp, #40] //pTimesOfFeatureValue
    ldr	r4, [sp, #36] //pFeatureOfBlock

    mov r8, r0
    mov r6, r1
    add r8, r6
    add r4, r4, r6, lsl #1

    mov r7, r6
_width_loop8x8_1:
    subs r0, r8, r7
    vld1.64 {d0}, [r0], r3
    vld1.64 {d1}, [r0], r3
    vld1.64 {d2}, [r0], r3
    vld1.64 {d3}, [r0], r3
    vld1.64 {d4}, [r0], r3
    vld1.64 {d5}, [r0], r3
    vld1.64 {d6}, [r0], r3
    vld1.64 {d7}, [r0]

    vpaddl.u8 q0, q0
    vpadal.u8 q0, q1
    vpadal.u8 q0, q2
    vpadal.u8 q0, q3
    vpaddl.u16 q0, q0
    vpadd.i32 d0, d1
    vpadd.i32 d0, d0

    subs r1, r4, r7, lsl #1
    vst1.16 {d0[0]}, [r1] // sum -> pFeatureOfBlock[i]
    vmov    r0, r1, d0
    add r1, r5, r0, lsl #2
    ldr r0, [r1]
    add r0, #1
    str r0, [r1]

    subs r7, #1
    bne _width_loop8x8_1

    add r8, r3
    add r4, r4, r6, lsl #1
    subs r2, #1
    beq _SumOf8x8BlockOfFrame_end


_height_loop8x8:
    mov r7, r6
_width_loop8x8_2:
    subs r0, r8, r7
    subs r1, r4, r7, lsl #1

    subs r9, r1, r6, lsl #1 // last line of pFeatureOfBlock[i]
    ldrh  r10, [r9] // sum of last line of pFeatureOfBlock[i]

    subs r11, r0, r3
    vld1.64 {d1}, [r11]
    add r0, r11, r3, lsl #3
    vld1.64 {d0}, [r0] //

    vpaddl.u8 q0, q0
    vpadd.u16 d0, d0, d1
    vpaddl.u16 d0, d0
    vmov r11, r12, d0
    subs r10, r12
    add r0, r10, r11

    strh r0, [r1] // sum -> pFeatureOfBlock[i]

    add r1, r5, r0, lsl #2
    ldr r0, [r1]
    add r0, #1
    str r0, [r1]
    subs r7, #1
    bne _width_loop8x8_2

    add r8, r3
    add r4, r4, r6, lsl #1
    subs r2, #1
    bne _height_loop8x8
_SumOf8x8BlockOfFrame_end:
    ldmia sp!, {r4-r12}
WELS_ASM_FUNC_END

WELS_ASM_FUNC_BEGIN SumOf16x16BlockOfFrame_neon
//(uint8_t* pRefPicture, const int32_t kiWidth, const int32_t kiHeight,const int32_t kiRefStride,uint16_t* pFeatureOfBlock, uint32_t pTimesOfFeatureValue[])
    stmdb sp!, {r4-r12}
    ldr	r5, [sp, #40] //pTimesOfFeatureValue
    ldr	r4, [sp, #36] //pFeatureOfBlock

    mov r8, r0
    mov r6, r1
    add r8, r6
    add r4, r4, r6, lsl #1

    mov r7, r6
_width_loop16x16_1:
    subs r0, r8, r7
    vld1.64 {q0}, [r0], r3
    vpaddl.u8 q0, q0
.rept 15
    vld1.64 {q1}, [r0], r3
    vpadal.u8 q0, q1
.endr
    vpaddl.u16 q0, q0
    vpadd.i32 d0, d1
    vpadd.i32 d0, d0

    subs r1, r4, r7, lsl #1
    vst1.16 {d0[0]}, [r1] // sum -> pFeatureOfBlock[i]
    vmov    r0, r1, d0
    add r1, r5, r0, lsl #2
    ldr r0, [r1]
    add r0, #1
    str r0, [r1]

    subs r7, #1
    bne _width_loop16x16_1
    add r8, r3
    add r4, r4, r6, lsl #1
    subs r2, #1
    beq _SumOf16x16BlockOfFrame_neon_end

_height_loop16x16:
    mov r7, r6
_width_loop16x16_2:
    subs r0, r8, r7
    subs r1, r4, r7, lsl #1
    subs r9, r1, r6, lsl #1 // last line of pFeatureOfBlock[i]
    ldrh  r10, [r9] // sum of last line of pFeatureOfBlock[i]

    subs r11, r0, r3
    vld1.64 {q1}, [r11]
    add r0, r11, r3, lsl #4
    vld1.64 {q0}, [r0] //

    vpaddl.u8 q0, q0
    vpaddl.u8 q1, q1
    vpadd.u16 d0, d0, d1
    vpadd.u16 d1, d2, d3
    vpadd.u16 d0, d0, d1
    vpaddl.u16 d0, d0

    vmov r11, r12, d0
    subs r10, r12
    add r0, r10, r11

    strh r0, [r1] // sum -> pFeatureOfBlock[i]
    add r1, r5, r0, lsl #2
    ldr r0, [r1]
    add r0, #1
    str r0, [r1]

    subs r7, #1
    bne _width_loop16x16_2

    add r8, r3
    add r4, r4, r6, lsl #1
    subs r2, #1
    bne _height_loop16x16
_SumOf16x16BlockOfFrame_neon_end:
    ldmia sp!, {r4-r12}
WELS_ASM_FUNC_END
#endif
