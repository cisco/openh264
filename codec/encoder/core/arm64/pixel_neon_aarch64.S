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

#ifdef HAVE_NEON_AARCH64
.text
#include "arm_arch64_common_macro.S"

.macro CALC_AND_STORE_SAD
    uaddlp  v2.4s, v2.8h
    addp    v2.4s, v2.4s, v2.4s
    addp    v2.2s, v2.2s, v2.2s
    umov    w0, v2.s[0]
.endm

.macro CALC_AND_STORE_SAD_FOUR
    addp    v0.8h, v28.8h, v29.8h
    addp    v1.8h, v30.8h, v31.8h
    addp    v0.8h, v0.8h, v1.8h
    addp    v0.8h, v0.8h, v0.8h
    eor     v1.8b, v1.8b, v1.8b
    saddl   v0.4s, v0.4h, v1.4h
    st1     {v0.4s}, [x4]
.endm

.macro LOAD_8X8_1
    ld1     {v0.8b}, [x0], x1
    ld1     {v1.8b}, [x0], x1
    ld1     {v2.8b}, [x0], x1
    ld1     {v3.8b}, [x0], x1
    ld1     {v4.8b}, [x0], x1
    ld1     {v5.8b}, [x0], x1
    ld1     {v6.8b}, [x0], x1
    ld1     {v7.8b}, [x0], x1
.endm

.macro LOAD_16X8_1
    ld1     {v0.16b}, [x0], x1
    ld1     {v1.16b}, [x0], x1
    ld1     {v2.16b}, [x0], x1
    ld1     {v3.16b}, [x0], x1
    ld1     {v4.16b}, [x0], x1
    ld1     {v5.16b}, [x0], x1
    ld1     {v6.16b}, [x0], x1
    ld1     {v7.16b}, [x0], x1
.endm

#ifdef __APPLE__
.macro LOAD_8X8_2
    ld1     {v16.8b}, [$0], x3
    ld1     {v17.8b}, [$0], x3
    ld1     {v18.8b}, [$0], x3
    ld1     {v19.8b}, [$0], x3
    ld1     {v20.8b}, [$0], x3
    ld1     {v21.8b}, [$0], x3
    ld1     {v22.8b}, [$0], x3
    ld1     {v23.8b}, [$0], x3
.endm

.macro CALC_ABS_8X8_1
    uab$1l  $0, v0.8b, v16.8b
    uabal   $0, v1.8b, v17.8b
    uabal   $0, v2.8b, v18.8b
    uabal   $0, v3.8b, v19.8b
    uabal   $0, v4.8b, v20.8b
    uabal   $0, v5.8b, v21.8b
    uabal   $0, v6.8b, v22.8b
    uabal   $0, v7.8b, v23.8b
.endm

.macro CALC_ABS_8X8_2
    uab$0l  v29.8h, v0.8b, v18.8b
    uabal   v29.8h, v1.8b, v19.8b
    uabal   v29.8h, v2.8b, v20.8b
    uabal   v29.8h, v3.8b, v21.8b
    uabal   v29.8h, v4.8b, v22.8b
    uabal   v29.8h, v5.8b, v23.8b
    uabal   v29.8h, v6.8b, v24.8b
    uabal   v29.8h, v7.8b, v25.8b
.endm

.macro LOAD_16X8_2
    ld1     {v16.16b}, [$0], x3
    ld1     {v17.16b}, [$0], x3
    ld1     {v18.16b}, [$0], x3
    ld1     {v19.16b}, [$0], x3
    ld1     {v20.16b}, [$0], x3
    ld1     {v21.16b}, [$0], x3
    ld1     {v22.16b}, [$0], x3
    ld1     {v23.16b}, [$0], x3
.endm

.macro CALC_ABS_16X8_1
    uab$1l  $0, v0.8b, v16.8b
    uabal2  $0, v0.16b,v16.16b
    uabal   $0, v1.8b, v17.8b
    uabal2  $0, v1.16b,v17.16b
    uabal   $0, v2.8b, v18.8b
    uabal2  $0, v2.16b,v18.16b
    uabal   $0, v3.8b, v19.8b
    uabal2  $0, v3.16b,v19.16b
    uabal   $0, v4.8b, v20.8b
    uabal2  $0, v4.16b,v20.16b
    uabal   $0, v5.8b, v21.8b
    uabal2  $0, v5.16b,v21.16b
    uabal   $0, v6.8b, v22.8b
    uabal2  $0, v6.16b,v22.16b
    uabal   $0, v7.8b, v23.8b
    uabal2  $0, v7.16b,v23.16b
.endm

.macro CALC_ABS_16X8_2
    uab$0l  v29.8h, v0.8b, v18.8b
    uabal2  v29.8h, v0.16b,v18.16b
    uabal   v29.8h, v1.8b, v19.8b
    uabal2  v29.8h, v1.16b,v19.16b
    uabal   v29.8h, v2.8b, v20.8b
    uabal2  v29.8h, v2.16b,v20.16b
    uabal   v29.8h, v3.8b, v21.8b
    uabal2  v29.8h, v3.16b,v21.16b
    uabal   v29.8h, v4.8b, v22.8b
    uabal2  v29.8h, v4.16b,v22.16b
    uabal   v29.8h, v5.8b, v23.8b
    uabal2  v29.8h, v5.16b,v23.16b
    uabal   v29.8h, v6.8b, v24.8b
    uabal2  v29.8h, v6.16b,v24.16b
    uabal   v29.8h, v7.8b, v25.8b
    uabal2  v29.8h, v7.16b,v25.16b
.endm
#else
.macro LOAD_8X8_2 arg0
    ld1     {v16.8b}, [\arg0], x3
    ld1     {v17.8b}, [\arg0], x3
    ld1     {v18.8b}, [\arg0], x3
    ld1     {v19.8b}, [\arg0], x3
    ld1     {v20.8b}, [\arg0], x3
    ld1     {v21.8b}, [\arg0], x3
    ld1     {v22.8b}, [\arg0], x3
    ld1     {v23.8b}, [\arg0], x3
.endm

.macro CALC_ABS_8X8_1 arg0, arg1
    uab\arg1\()l    \arg0, v0.8b, v16.8b
    uabal   \arg0, v1.8b, v17.8b
    uabal   \arg0, v2.8b, v18.8b
    uabal   \arg0, v3.8b, v19.8b
    uabal   \arg0, v4.8b, v20.8b
    uabal   \arg0, v5.8b, v21.8b
    uabal   \arg0, v6.8b, v22.8b
    uabal   \arg0, v7.8b, v23.8b
.endm

.macro CALC_ABS_8X8_2 arg0
    uab\arg0\()l    v29.8h, v0.8b, v18.8b
    uabal   v29.8h, v1.8b, v19.8b
    uabal   v29.8h, v2.8b, v20.8b
    uabal   v29.8h, v3.8b, v21.8b
    uabal   v29.8h, v4.8b, v22.8b
    uabal   v29.8h, v5.8b, v23.8b
    uabal   v29.8h, v6.8b, v24.8b
    uabal   v29.8h, v7.8b, v25.8b
.endm

.macro LOAD_16X8_2 arg0
    ld1     {v16.16b}, [\arg0], x3
    ld1     {v17.16b}, [\arg0], x3
    ld1     {v18.16b}, [\arg0], x3
    ld1     {v19.16b}, [\arg0], x3
    ld1     {v20.16b}, [\arg0], x3
    ld1     {v21.16b}, [\arg0], x3
    ld1     {v22.16b}, [\arg0], x3
    ld1     {v23.16b}, [\arg0], x3
.endm

.macro CALC_ABS_16X8_1 arg0, arg1
    uab\arg1\()l  \arg0, v0.8b, v16.8b
    uabal2  \arg0, v0.16b,v16.16b
    uabal   \arg0, v1.8b, v17.8b
    uabal2  \arg0, v1.16b,v17.16b
    uabal   \arg0, v2.8b, v18.8b
    uabal2  \arg0, v2.16b,v18.16b
    uabal   \arg0, v3.8b, v19.8b
    uabal2  \arg0, v3.16b,v19.16b
    uabal   \arg0, v4.8b, v20.8b
    uabal2  \arg0, v4.16b,v20.16b
    uabal   \arg0, v5.8b, v21.8b
    uabal2  \arg0, v5.16b,v21.16b
    uabal   \arg0, v6.8b, v22.8b
    uabal2  \arg0, v6.16b,v22.16b
    uabal   \arg0, v7.8b, v23.8b
    uabal2  \arg0, v7.16b,v23.16b
.endm

.macro CALC_ABS_16X8_2 arg0
    uab\arg0\()l  v29.8h, v0.8b, v18.8b
    uabal2  v29.8h, v0.16b,v18.16b
    uabal   v29.8h, v1.8b, v19.8b
    uabal2  v29.8h, v1.16b,v19.16b
    uabal   v29.8h, v2.8b, v20.8b
    uabal2  v29.8h, v2.16b,v20.16b
    uabal   v29.8h, v3.8b, v21.8b
    uabal2  v29.8h, v3.16b,v21.16b
    uabal   v29.8h, v4.8b, v22.8b
    uabal2  v29.8h, v4.16b,v22.16b
    uabal   v29.8h, v5.8b, v23.8b
    uabal2  v29.8h, v5.16b,v23.16b
    uabal   v29.8h, v6.8b, v24.8b
    uabal2  v29.8h, v6.16b,v24.16b
    uabal   v29.8h, v7.8b, v25.8b
    uabal2  v29.8h, v7.16b,v25.16b
.endm
#endif

WELS_ASM_ARCH64_FUNC_BEGIN WelsSampleSad4x4_AArch64_neon
    sxtw    x1, w1
    sxtw    x3, w3
    ld1     {v0.s}[0], [x0], x1
    ld1     {v1.s}[0], [x2], x3
    uabdl   v2.8h, v0.8b, v1.8b
.rept 3
    ld1     {v0.s}[0], [x0], x1
    ld1     {v1.s}[0], [x2], x3
    uabal   v2.8h, v0.8b, v1.8b
.endr
    uaddlp  v2.2s, v2.4h
    addp    v2.2s, v2.2s, v2.2s
    umov    w0, v2.s[0]
WELS_ASM_ARCH64_FUNC_END

WELS_ASM_ARCH64_FUNC_BEGIN WelsSampleSad8x8_AArch64_neon
    sxtw    x1, w1
    sxtw    x3, w3
    ld1     {v0.8b}, [x0], x1
    ld1     {v1.8b}, [x2], x3
    uabdl   v2.8h, v0.8b, v1.8b
.rept 7
    ld1     {v0.8b}, [x0], x1
    ld1     {v1.8b}, [x2], x3
    uabal   v2.8h, v0.8b, v1.8b
.endr
    CALC_AND_STORE_SAD
WELS_ASM_ARCH64_FUNC_END

WELS_ASM_ARCH64_FUNC_BEGIN WelsSampleSad8x16_AArch64_neon
    sxtw    x1, w1
    sxtw    x3, w3
    ld1     {v0.8b}, [x0], x1
    ld1     {v1.8b}, [x2], x3
    uabdl   v2.8h, v0.8b, v1.8b
.rept 15
    ld1     {v0.8b}, [x0], x1
    ld1     {v1.8b}, [x2], x3
    uabal   v2.8h, v0.8b, v1.8b
.endr
    CALC_AND_STORE_SAD
WELS_ASM_ARCH64_FUNC_END

WELS_ASM_ARCH64_FUNC_BEGIN WelsSampleSad16x8_AArch64_neon
    sxtw    x1, w1
    sxtw    x3, w3
    ld1     {v0.16b}, [x0], x1
    ld1     {v1.16b}, [x2], x3
    uabdl   v2.8h, v0.8b, v1.8b
    uabal2  v2.8h, v0.16b, v1.16b
.rept 7
    ld1     {v0.16b}, [x0], x1
    ld1     {v1.16b}, [x2], x3
    uabal   v2.8h, v0.8b, v1.8b
    uabal2  v2.8h, v0.16b, v1.16b
.endr
    CALC_AND_STORE_SAD
WELS_ASM_ARCH64_FUNC_END

WELS_ASM_ARCH64_FUNC_BEGIN WelsSampleSad16x16_AArch64_neon
    sxtw    x1, w1
    sxtw    x3, w3
    ld1     {v0.16b}, [x0], x1
    ld1     {v1.16b}, [x2], x3
    uabdl   v2.8h, v0.8b, v1.8b
    uabal2  v2.8h, v0.16b, v1.16b
.rept 15
    ld1     {v0.16b}, [x0], x1
    ld1     {v1.16b}, [x2], x3
    uabal   v2.8h, v0.8b, v1.8b
    uabal2  v2.8h, v0.16b, v1.16b
.endr
    CALC_AND_STORE_SAD
WELS_ASM_ARCH64_FUNC_END

WELS_ASM_ARCH64_FUNC_BEGIN WelsSampleSadFour4x4_AArch64_neon
    sxtw    x1, w1
    sxtw    x3, w3
    ld1     {v0.s}[0], [x0], x1
    ld1     {v0.s}[1], [x0], x1
    ld1     {v1.s}[0], [x0], x1
    ld1     {v1.s}[1], [x0]
    sub     x0, x2, x3
    ld1     {v2.s}[0], [x0], x3
    ld1     {v2.s}[1], [x0], x3
    ld1     {v3.s}[0], [x0], x3
    ld1     {v3.s}[1], [x0], x3
    ld1     {v4.s}[0], [x0], x3
    ld1     {v4.s}[1], [x0], x3

    uabdl   v28.8h, v0.8b, v2.8b
    uabal   v28.8h, v1.8b, v3.8b

    uabdl   v29.8h, v0.8b, v3.8b
    uabal   v29.8h, v1.8b, v4.8b

    sub     x0, x2, #1
    ld1     {v2.s}[0], [x0], x3
    ld1     {v2.s}[1], [x0], x3
    ld1     {v3.s}[0], [x0], x3
    ld1     {v3.s}[1], [x0]
    uabdl   v30.8h, v0.8b, v2.8b
    uabal   v30.8h, v1.8b, v3.8b

    add     x0, x2, #1
    ld1     {v2.s}[0], [x0], x3
    ld1     {v2.s}[1], [x0], x3
    ld1     {v3.s}[0], [x0], x3
    ld1     {v3.s}[1], [x0]
    uabdl   v31.8h, v0.8b, v2.8b
    uabal   v31.8h, v1.8b, v3.8b

    CALC_AND_STORE_SAD_FOUR
WELS_ASM_ARCH64_FUNC_END

WELS_ASM_ARCH64_FUNC_BEGIN WelsSampleSadFour8x8_AArch64_neon
    sxtw    x1, w1
    sxtw    x3, w3
    LOAD_8X8_1
    sub     x0, x2, x3
    LOAD_8X8_2 x0
    ld1     {v24.8b}, [x0], x3
    ld1     {v25.8b}, [x0]

    CALC_ABS_8X8_1 v28.8h, d
    CALC_ABS_8X8_2 d

    sub     x0, x2, #1
    LOAD_8X8_2 x0
    CALC_ABS_8X8_1 v30.8h, d

    add     x0, x2, #1
    LOAD_8X8_2 x0
    CALC_ABS_8X8_1 v31.8h, d

    CALC_AND_STORE_SAD_FOUR
WELS_ASM_ARCH64_FUNC_END

WELS_ASM_ARCH64_FUNC_BEGIN WelsSampleSadFour8x16_AArch64_neon
    sxtw    x1, w1
    sxtw    x3, w3
    LOAD_8X8_1
    sub     x5, x2, x3
    LOAD_8X8_2 x5
    ld1     {v24.8b}, [x5], x3
    ld1     {v25.8b}, [x5], x3

    CALC_ABS_8X8_1 v28.8h, d
    CALC_ABS_8X8_2 d

    sub     x6, x2, #1
    LOAD_8X8_2 x6
    CALC_ABS_8X8_1 v30.8h, d

    add     x7, x2, #1
    LOAD_8X8_2 x7
    CALC_ABS_8X8_1 v31.8h, d

    LOAD_8X8_1
    sub     x5, x5, x3
    sub     x5, x5, x3
    LOAD_8X8_2 x5
    ld1     {v24.8b}, [x5], x3
    ld1     {v25.8b}, [x5]

    CALC_ABS_8X8_1 v28.8h, a
    CALC_ABS_8X8_2 a

    LOAD_8X8_2 x6
    CALC_ABS_8X8_1 v30.8h, a

    LOAD_8X8_2 x7
    CALC_ABS_8X8_1 v31.8h, a

    CALC_AND_STORE_SAD_FOUR
WELS_ASM_ARCH64_FUNC_END

WELS_ASM_ARCH64_FUNC_BEGIN WelsSampleSadFour16x8_AArch64_neon
    sxtw    x1, w1
    sxtw    x3, w3
    LOAD_16X8_1
    sub     x0, x2, x3
    LOAD_16X8_2 x0
    ld1     {v24.16b}, [x0], x3
    ld1     {v25.16b}, [x0]

    CALC_ABS_16X8_1 v28.8h, d
    CALC_ABS_16X8_2 d

    sub     x0, x2, #1
    LOAD_16X8_2 x0
    CALC_ABS_16X8_1 v30.8h, d

    add     x0, x2, #1
    LOAD_16X8_2 x0
    CALC_ABS_16X8_1 v31.8h, d

    CALC_AND_STORE_SAD_FOUR
WELS_ASM_ARCH64_FUNC_END

WELS_ASM_ARCH64_FUNC_BEGIN WelsSampleSadFour16x16_AArch64_neon
    sxtw    x1, w1
    sxtw    x3, w3

    LOAD_16X8_1
    sub     x5, x2, x3
    LOAD_16X8_2 x5
    ld1     {v24.16b}, [x5], x3
    ld1     {v25.16b}, [x5], x3

    CALC_ABS_16X8_1 v28.8h, d
    CALC_ABS_16X8_2 d

    sub     x6, x2, #1
    LOAD_16X8_2 x6
    CALC_ABS_16X8_1 v30.8h, d

    add     x7, x2, #1
    LOAD_16X8_2 x7
    CALC_ABS_16X8_1 v31.8h, d

    LOAD_16X8_1
    sub     x5, x5, x3
    sub     x5, x5, x3
    LOAD_16X8_2 x5
    ld1     {v24.16b}, [x5], x3
    ld1     {v25.16b}, [x5]

    CALC_ABS_16X8_1 v28.8h, a
    CALC_ABS_16X8_2 a

    LOAD_16X8_2 x6
    CALC_ABS_16X8_1 v30.8h, a

    LOAD_16X8_2 x7
    CALC_ABS_16X8_1 v31.8h, a

    CALC_AND_STORE_SAD_FOUR
WELS_ASM_ARCH64_FUNC_END
#endif