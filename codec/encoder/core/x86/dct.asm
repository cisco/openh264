;*!
;* \copy
;*     Copyright (c)  2009-2013, Cisco Systems
;*     All rights reserved.
;*
;*     Redistribution and use in source and binary forms, with or without
;*     modification, are permitted provided that the following conditions
;*     are met:
;*
;*        ?Redistributions of source code must retain the above copyright
;*          notice, this list of conditions and the following disclaimer.
;*
;*        ?Redistributions in binary form must reproduce the above copyright
;*          notice, this list of conditions and the following disclaimer in
;*          the documentation and/or other materials provided with the
;*          distribution.
;*
;*     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;*     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;*     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
;*     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
;*     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
;*     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
;*     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
;*     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
;*     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
;*     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
;*     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
;*     POSSIBILITY OF SUCH DAMAGE.
;*
;*
;*  dct.asm
;*
;*  Abstract
;*      WelsDctFourT4_sse2
;*
;*  History
;*      8/4/2009 Created
;*
;*
;*************************************************************************/

%include "asm_inc.asm"

SECTION .rodata align=16

;***********************************************************************
; Constant
;***********************************************************************

align 16
SSE2_DeQuant8 dw  10, 13, 10, 13, 13, 16, 13, 16,
            dw  10, 13, 10, 13, 13, 16, 13, 16,
            dw  11, 14, 11, 14, 14, 18, 14, 18,
            dw  11, 14, 11, 14, 14, 18, 14, 18,
            dw  13, 16, 13, 16, 16, 20, 16, 20,
            dw  13, 16, 13, 16, 16, 20, 16, 20,
            dw  14, 18, 14, 18, 18, 23, 18, 23,
            dw  14, 18, 14, 18, 18, 23, 18, 23,
            dw  16, 20, 16, 20, 20, 25, 20, 25,
            dw  16, 20, 16, 20, 20, 25, 20, 25,
            dw  18, 23, 18, 23, 23, 29, 23, 29,
            dw  18, 23, 18, 23, 23, 29, 23, 29


;***********************************************************************
; MMX functions
;***********************************************************************

%macro MMX_LoadDiff4P 5
    movd        %1, [%3]
    movd        %2, [%4]
    punpcklbw   %1, %5
    punpcklbw   %2, %5
    psubw       %1, %2
%endmacro

%macro MMX_LoadDiff4x4P 10 ;d0, d1, d2, d3, pix1address, pix1stride, pix2address, pix2stride, tmp(mm), 0(mm)
    MMX_LoadDiff4P %1, %9, %5,    %7,    %10
    MMX_LoadDiff4P %2, %9, %5+%6, %7+%8, %10
    lea  %5, [%5+2*%6]
    lea  %7, [%7+2*%8]
    MMX_LoadDiff4P %3, %9, %5,    %7,    %10
    MMX_LoadDiff4P %4, %9, %5+%6, %7+%8, %10
%endmacro

%macro MMX_SumSubMul2 3
    movq    %3, %1
    psllw   %1, $01
    paddw   %1, %2
    psllw   %2, $01
    psubw   %3, %2
%endmacro

%macro MMX_SumSubDiv2 3
    movq    %3, %2
    psraw   %3, $01
    paddw   %3, %1
    psraw   %1, $01
    psubw   %1, %2
%endmacro

%macro MMX_SumSub 3
    movq    %3, %2
    psubw   %2, %1
    paddw   %1, %3
%endmacro

%macro MMX_DCT 6
    MMX_SumSub      %4, %1, %6
    MMX_SumSub      %3, %2, %6
    MMX_SumSub      %3, %4, %6
    MMX_SumSubMul2  %1, %2, %5
%endmacro

%macro MMX_IDCT 6
    MMX_SumSub      %4, %5, %6
    MMX_SumSubDiv2  %3, %2, %1
    MMX_SumSub      %1, %4, %6
    MMX_SumSub      %3, %5, %6
%endmacro

%macro MMX_StoreDiff4P 6
    movd       %2, %6
    punpcklbw  %2, %4
    paddw      %1, %3
    psraw      %1, $06
    paddsw     %1, %2
    packuswb   %1, %2
    movd       %5, %1
%endmacro
SECTION .text
;***********************************************************************
;   void WelsDctT4_mmx( int16_t *pDct[4], uint8_t *pix1, int32_t i_pix1, uint8_t *pix2, int32_t i_pix2 )
;***********************************************************************
WELS_EXTERN WelsDctT4_mmx
    %assign push_num 0
    LOAD_5_PARA
    SIGN_EXTENSION r2, r2d
    SIGN_EXTENSION r4, r4d
    WELS_Zero    mm7

    MMX_LoadDiff4x4P mm1, mm2, mm3, mm4, r1, r2, r3, r4, mm0, mm7

    MMX_DCT         mm1, mm2, mm3 ,mm4, mm5, mm6
    MMX_Trans4x4W   mm3, mm1, mm4, mm5, mm2

    MMX_DCT         mm3, mm5, mm2 ,mm4, mm1, mm6
    MMX_Trans4x4W   mm2, mm3, mm4, mm1, mm5

    movq    [r0+ 0],   mm2
    movq    [r0+ 8],   mm1
    movq    [r0+16],   mm5
    movq    [r0+24],   mm4
    WELSEMMS
    LOAD_5_PARA_POP
    ret


;***********************************************************************
;   void WelsIDctT4Rec_mmx(uint8_t *rec, int32_t stride, uint8_t *pred, int32_t pred_stride, int16_t *rs)
;***********************************************************************
WELS_EXTERN WelsIDctT4Rec_mmx
    %assign push_num 0
    LOAD_5_PARA
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    movq    mm0, [r4+ 0]
    movq    mm1, [r4+ 8]
    movq    mm2, [r4+16]
    movq    mm3, [r4+24]

    MMX_Trans4x4W       mm0, mm1, mm2, mm3, mm4
    MMX_IDCT            mm1, mm2, mm3, mm4, mm0, mm6
    MMX_Trans4x4W       mm1, mm3, mm0, mm4, mm2
    MMX_IDCT            mm3, mm0, mm4, mm2, mm1, mm6

    WELS_Zero           mm7
    WELS_DW32           mm6

    MMX_StoreDiff4P     mm3, mm0, mm6, mm7, [r0], [r2]
    MMX_StoreDiff4P     mm4, mm0, mm6, mm7, [r0+r1], [r2+r3]
    lea     r0, [r0+2*r1]
    lea     r2, [r2+2*r3]
    MMX_StoreDiff4P     mm1, mm0, mm6, mm7, [r0], [r2]
    MMX_StoreDiff4P     mm2, mm0, mm6, mm7, [r0+r1], [r2+r3]

    WELSEMMS
    LOAD_5_PARA_POP
    ret


;***********************************************************************
; SSE2 functions
;***********************************************************************
%macro SSE2_Store4x8p 6
    SSE2_XSawp qdq, %2, %3, %6
    SSE2_XSawp qdq, %4, %5, %3
    MOVDQ    [%1+0x00], %2
    MOVDQ    [%1+0x10], %4
    MOVDQ    [%1+0x20], %6
    MOVDQ    [%1+0x30], %3
%endmacro

%macro SSE2_Load4x8p 6
    MOVDQ    %2,    [%1+0x00]
    MOVDQ    %4,    [%1+0x10]
    MOVDQ    %6,    [%1+0x20]
    MOVDQ    %3,    [%1+0x30]
    SSE2_XSawp qdq, %4, %3, %5
    SSE2_XSawp qdq, %2, %6, %3
%endmacro

%macro SSE2_SumSubMul2 3
    movdqa  %3, %1
    paddw   %1, %1
    paddw   %1, %2
    psubw   %3, %2
    psubw   %3, %2
%endmacro

%macro SSE2_SumSubDiv2 4
    movdqa  %4, %1
    movdqa  %3, %2
    psraw   %2, $01
    psraw   %4, $01
    paddw   %1, %2
    psubw   %4, %3
%endmacro

%macro SSE2_StoreDiff8p 6
    paddw       %1, %3
    psraw       %1, $06
    movq        %2, %6
    punpcklbw   %2, %4
    paddsw      %2, %1
    packuswb    %2, %2
    movq        %5, %2
%endmacro

%macro SSE2_StoreDiff8p 5
    movq        %2, %5
    punpcklbw   %2, %3
    paddsw      %2, %1
    packuswb    %2, %2
    movq        %4, %2
%endmacro

%macro SSE2_Load8DC 6
    movdqa      %1,     %6      ; %1 = dc0 dc1
    paddw       %1,     %5
    psraw       %1,     $06     ; (dc + 32) >> 6

    movdqa      %2,     %1
    psrldq      %2,     4
    punpcklwd   %2,     %2
    punpckldq   %2,     %2      ; %2 = dc2 dc2 dc2 dc2 dc3 dc3 dc3 dc3

    movdqa      %3,     %1
    psrldq      %3,     8
    punpcklwd   %3,     %3
    punpckldq   %3,     %3      ; %3 = dc4 dc4 dc4 dc4 dc5 dc5 dc5 dc5

    movdqa      %4,     %1
    psrldq      %4,     12
    punpcklwd   %4,     %4
    punpckldq   %4,     %4      ; %4 = dc6 dc6 dc6 dc6 dc7 dc7 dc7 dc7

    punpcklwd   %1,     %1
    punpckldq   %1,     %1      ; %1 = dc0 dc0 dc0 dc0 dc1 dc1 dc1 dc1
%endmacro

%macro SSE2_DCT 6
    SSE2_SumSub     %6, %3, %5
    SSE2_SumSub     %1, %2, %5
    SSE2_SumSub     %3, %2, %5
    SSE2_SumSubMul2     %6, %1, %4
%endmacro

%macro SSE2_IDCT 7
    SSE2_SumSub       %7, %2, %6
    SSE2_SumSubDiv2     %1, %3, %5, %4
    SSE2_SumSub      %2, %1, %5
    SSE2_SumSub      %7, %4, %5
%endmacro

;***********************************************************************
; void WelsDctFourT4_sse2(int16_t *pDct, uint8_t *pix1, int32_t i_pix1, uint8_t *pix2, int32_t i_pix2 )
;***********************************************************************
WELS_EXTERN WelsDctFourT4_sse2
    %assign push_num 0
    LOAD_5_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r2, r2d
    SIGN_EXTENSION r4, r4d
    pxor    xmm7, xmm7
    ;Load 4x8
    SSE2_LoadDiff8P    xmm0, xmm6, xmm7, [r1], [r3]
    SSE2_LoadDiff8P    xmm1, xmm6, xmm7, [r1+r2], [r3+r4]
    lea     r1, [r1 + 2 * r2]
    lea     r3, [r3 + 2 * r4]
    SSE2_LoadDiff8P    xmm2, xmm6, xmm7, [r1], [r3]
    SSE2_LoadDiff8P    xmm3, xmm6, xmm7, [r1+r2], [r3+r4]

    SSE2_DCT            xmm1, xmm2, xmm3, xmm4, xmm5, xmm0
    SSE2_TransTwo4x4W   xmm2, xmm0, xmm3, xmm4, xmm1
    SSE2_DCT            xmm0, xmm4, xmm1, xmm3, xmm5, xmm2
    SSE2_TransTwo4x4W   xmm4, xmm2, xmm1, xmm3, xmm0

    SSE2_Store4x8p r0, xmm4, xmm2, xmm3, xmm0, xmm5

    lea     r1, [r1 + 2 * r2]
    lea     r3, [r3 + 2 * r4]

    ;Load 4x8
    SSE2_LoadDiff8P    xmm0, xmm6, xmm7, [r1      ], [r3    ]
    SSE2_LoadDiff8P    xmm1, xmm6, xmm7, [r1+r2  ], [r3+r4]
    lea     r1, [r1 + 2 * r2]
    lea     r3, [r3 + 2 * r4]
    SSE2_LoadDiff8P    xmm2, xmm6, xmm7, [r1], [r3]
    SSE2_LoadDiff8P    xmm3, xmm6, xmm7, [r1+r2], [r3+r4]

    SSE2_DCT            xmm1, xmm2, xmm3, xmm4, xmm5, xmm0
    SSE2_TransTwo4x4W   xmm2, xmm0, xmm3, xmm4, xmm1
    SSE2_DCT            xmm0, xmm4, xmm1, xmm3, xmm5, xmm2
    SSE2_TransTwo4x4W   xmm4, xmm2, xmm1, xmm3, xmm0

    lea     r0, [r0+64]
    SSE2_Store4x8p r0, xmm4, xmm2, xmm3, xmm0, xmm5

    POP_XMM
    LOAD_5_PARA_POP
    ret


;***********************************************************************
; void WelsIDctFourT4Rec_sse2(uint8_t *rec, int32_t stride, uint8_t *pred, int32_t pred_stride, int16_t *rs);
;***********************************************************************
WELS_EXTERN WelsIDctFourT4Rec_sse2
    %assign push_num 0
    LOAD_5_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    ;Load 4x8
    SSE2_Load4x8p  r4, xmm0, xmm1, xmm4, xmm2, xmm5

    SSE2_TransTwo4x4W   xmm0, xmm1, xmm4, xmm2, xmm3
    SSE2_IDCT           xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm0
    SSE2_TransTwo4x4W   xmm1, xmm4, xmm0, xmm2, xmm3
    SSE2_IDCT           xmm4, xmm2, xmm3, xmm0, xmm5, xmm6, xmm1

    WELS_Zero           xmm7
    WELS_DW32           xmm6

    SSE2_StoreDiff8p   xmm4, xmm5, xmm6, xmm7, [r0      ],  [r2]
    SSE2_StoreDiff8p   xmm0, xmm5, xmm6, xmm7, [r0 + r1 ],  [r2 + r3]
    lea     r0, [r0 + 2 * r1]
    lea     r2, [r2 + 2 * r3]
    SSE2_StoreDiff8p   xmm1, xmm5, xmm6, xmm7, [r0],            [r2]
    SSE2_StoreDiff8p   xmm2, xmm5, xmm6, xmm7, [r0 + r1 ],  [r2 + r3]

    add     r4, 64
    lea     r0, [r0 + 2 * r1]
    lea     r2, [r2 + 2 * r3]
    SSE2_Load4x8p  r4, xmm0, xmm1, xmm4, xmm2, xmm5

    SSE2_TransTwo4x4W   xmm0, xmm1, xmm4, xmm2, xmm3
    SSE2_IDCT           xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm0
    SSE2_TransTwo4x4W   xmm1, xmm4, xmm0, xmm2, xmm3
    SSE2_IDCT           xmm4, xmm2, xmm3, xmm0, xmm5, xmm6, xmm1

    WELS_Zero           xmm7
    WELS_DW32           xmm6

    SSE2_StoreDiff8p   xmm4, xmm5, xmm6, xmm7, [r0      ],  [r2]
    SSE2_StoreDiff8p   xmm0, xmm5, xmm6, xmm7, [r0 + r1 ],  [r2 + r3]
    lea     r0, [r0 + 2 * r1]
    lea     r2, [r2 + 2 * r3]
    SSE2_StoreDiff8p   xmm1, xmm5, xmm6, xmm7, [r0],            [r2]
    SSE2_StoreDiff8p   xmm2, xmm5, xmm6, xmm7, [r0 + r1],   [r2 + r3]
    POP_XMM
    LOAD_5_PARA_POP
    ; pop        esi
    ; pop        ebx
    ret

%macro SSE2_StoreDiff4x8p 8
    SSE2_StoreDiff8p    %1, %3, %4, [%5],           [%6]
    SSE2_StoreDiff8p    %1, %3, %4, [%5 + %7],      [%6 + %8]
    SSE2_StoreDiff8p    %2, %3, %4, [%5 + 8],       [%6 + 8]
    SSE2_StoreDiff8p    %2, %3, %4, [%5 + %7 + 8],  [%6 + %8 + 8]
%endmacro

 ;***********************************************************************
; void WelsIDctRecI16x16Dc_sse2(uint8_t *rec, int32_t stride, uint8_t *pred, int32_t pred_stride, int16_t *dct_dc)
;***********************************************************************
WELS_EXTERN WelsIDctRecI16x16Dc_sse2
    %assign push_num 0
    LOAD_5_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    pxor        xmm7,       xmm7
    WELS_DW32   xmm6

    SSE2_Load8DC            xmm0, xmm1, xmm2, xmm3, xmm6, [r4]
    SSE2_StoreDiff4x8p      xmm0, xmm1, xmm5, xmm7, r0, r2, r1, r3

    lea         r0,     [r0 + 2 * r1]
    lea         r2,     [r2 + 2 * r3]
    SSE2_StoreDiff4x8p      xmm0, xmm1, xmm5, xmm7, r0, r2, r1, r3

    lea         r0,     [r0 + 2 * r1]
    lea         r2,     [r2 + 2 * r3]
    SSE2_StoreDiff4x8p      xmm2, xmm3, xmm5, xmm7, r0, r2, r1, r3

    lea         r0,     [r0 + 2 * r1]
    lea         r2,     [r2 + 2 * r3]
    SSE2_StoreDiff4x8p      xmm2, xmm3, xmm5, xmm7, r0, r2, r1, r3

    SSE2_Load8DC            xmm0, xmm1, xmm2, xmm3, xmm6, [r4 + 16]
    lea         r0,     [r0 + 2 * r1]
    lea         r2,     [r2 + 2 * r3]
    SSE2_StoreDiff4x8p      xmm0, xmm1, xmm5, xmm7, r0, r2, r1, r3

    lea         r0,     [r0 + 2 * r1]
    lea         r2,     [r2 + 2 * r3]
    SSE2_StoreDiff4x8p      xmm0, xmm1, xmm5, xmm7, r0, r2, r1, r3

    lea         r0,     [r0 + 2 * r1]
    lea         r2,     [r2 + 2 * r3]
    SSE2_StoreDiff4x8p      xmm2, xmm3, xmm5, xmm7, r0, r2, r1, r3

    lea         r0,     [r0 + 2 * r1]
    lea         r2,     [r2 + 2 * r3]
    SSE2_StoreDiff4x8p      xmm2, xmm3, xmm5, xmm7, r0, r2, r1, r3
    POP_XMM
    LOAD_5_PARA_POP
    ret



%macro SSE2_SumSubD 3
    movdqa  %3, %2
    paddd   %2, %1
    psubd   %1, %3
%endmacro

%macro SSE2_SumSubDiv2D 4
    paddd   %1, %2
    paddd   %1, %3
    psrad   %1,  1
    movdqa  %4, %1
    psubd   %4, %2
%endmacro
%macro SSE2_Load4Col    5
    movsx       r2,     WORD[%5]
    movd        %1,         r2d
    movsx       r2,     WORD[%5 + 0x20]
    movd        %2,         r2d
    punpckldq   %1,         %2
    movsx       r2,     WORD[%5 + 0x80]
    movd        %3,         r2d
    movsx       r2,     WORD[%5 + 0xa0]
    movd        %4,         r2d
    punpckldq   %3,         %4
    punpcklqdq  %1,         %3
%endmacro

;***********************************************************************
;void WelsHadamardT4Dc_sse2( int16_t *luma_dc, int16_t *pDct)
;***********************************************************************
WELS_EXTERN WelsHadamardT4Dc_sse2
    %assign push_num 0
    LOAD_2_PARA
    PUSH_XMM 8
    SSE2_Load4Col       xmm1, xmm5, xmm6, xmm0, r1
    SSE2_Load4Col       xmm2, xmm5, xmm6, xmm0, r1 + 0x40
    SSE2_Load4Col       xmm3, xmm5, xmm6, xmm0, r1 + 0x100
    SSE2_Load4Col       xmm4, xmm5, xmm6, xmm0, r1 + 0x140

    SSE2_SumSubD        xmm1, xmm2, xmm7
    SSE2_SumSubD        xmm3, xmm4, xmm7
    SSE2_SumSubD        xmm2, xmm4, xmm7
    SSE2_SumSubD        xmm1, xmm3, xmm7

    SSE2_Trans4x4D      xmm4, xmm2, xmm1, xmm3, xmm5    ; pOut: xmm4,xmm3,xmm5,xmm1

    SSE2_SumSubD        xmm4, xmm3, xmm7
    SSE2_SumSubD        xmm5, xmm1, xmm7

    WELS_DD1 xmm6
    SSE2_SumSubDiv2D    xmm3, xmm1, xmm6, xmm0          ; pOut: xmm3 = (xmm3+xmm1+1)/2, xmm0 = (xmm3-xmm1+1)/2
    SSE2_SumSubDiv2D    xmm4, xmm5, xmm6, xmm1          ; pOut: xmm4 = (xmm4+xmm5+1)/2, xmm1 = (xmm4-xmm5+1)/2
    SSE2_Trans4x4D      xmm3, xmm0, xmm1, xmm4, xmm2    ; pOut: xmm3,xmm4,xmm2,xmm1

    packssdw    xmm3,   xmm4
    packssdw    xmm2,   xmm1
    movdqa  [r0+ 0],   xmm3
    movdqa  [r0+16],   xmm2

    POP_XMM
    ret
