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
 */

#if defined(HAVE_NEON_AARCH64) && defined(__aarch64__)
#include "arm_arch64_common_macro.S"

.extern DetectStartCodePrefixC

WELS_ASM_AARCH64_FUNC_BEGIN DetectStartCodePrefixNEON
    cmp     w2, #79
    b.gt    .L_init
    b       DetectStartCodePrefixC

.L_init:
    stp     x29, x30, [sp, #-32]!
    stp     x20, x19, [sp, #16]
    mov     x29, sp
    mov     x9, #0
    add     x8, x0, w2, sxtw
    sub     x10, x8, #80
    b       .L_simd

.L_advance:
    add     x9, x9, #64
    add     x8, x0, x9
    cmp     x8, x10
    b.hi    .L_fallback

.L_simd:
    add     x8, x0, x9
    ldp     q1, q2, [x8]
    ldp     q3, q0, [x8, #32]
    ext     v4.16b, v0.16b, v1.16b, #15
    ext     v5.16b, v1.16b, v2.16b, #15
    ext     v6.16b, v2.16b, v3.16b, #15
    ext     v7.16b, v3.16b, v0.16b, #15
    orr     v1.16b, v4.16b, v1.16b
    orr     v2.16b, v5.16b, v2.16b
    orr     v3.16b, v6.16b, v3.16b
    orr     v4.16b, v7.16b, v0.16b
    umin    v1.16b, v1.16b, v2.16b
    umin    v2.16b, v3.16b, v4.16b
    umin    v1.16b, v1.16b, v2.16b
    uminv   b1, v1.16b
    fmov    w11, s1
    cbz     w11, .L_call_c

    umov    w8, v0.b[15]
    cbnz    w8, .L_advance

    add     x8, x0, x9
    ldrb    w11, [x8, #64]
    cbnz    w11, .L_advance

    ldrb    w8, [x8, #65]
    cmp     w8, #1
    b.ne    .L_advance

    add     w8, w9, #66
    str     w8, [x1]
    add     x0, x0, w8, sxtw

.L_return:
    ldp     x20, x19, [sp, #16]
    ldp     x29, x30, [sp], #32
    ret

.L_call_c:
    mov     x19, x0
    mov     x0, x8
    mov     x20, x1
    mov     w2, #65
    bl      DetectStartCodePrefixC
    cbnz    x0, .L_found
    b       .L_return

.L_fallback:
    mov     x19, x0
    sub     x0, x8, #1
    sub     w8, w2, w9
    add     w2, w8, #1
    mov     x20, x1
    bl      DetectStartCodePrefixC
    cbz     x0, .L_return

.L_found:
    sub     w8, w0, w19
    str     w8, [x20]
    ldp     x20, x19, [sp, #16]
    ldp     x29, x30, [sp], #32
    ret

WELS_ASM_AARCH64_FUNC_END

#endif
