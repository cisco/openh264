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

;*******************************************************************************
; Macros and other preprocessor constants
;*******************************************************************************
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

%macro MMX_IDCT 6
    MMX_SumSub      %4, %5, %6
    MMX_SumSubDiv2  %3, %2, %1
    MMX_SumSub      %1, %4, %6
    MMX_SumSub      %3, %5, %6
%endmacro


%macro MMX_StoreDiff4P 5
    movd       %2, %5
    punpcklbw  %2, %4
    paddw      %1, %3
    psraw      %1, $06
    paddsw     %1, %2
    packuswb   %1, %2
    movd       %5, %1
%endmacro

;*******************************************************************************
; Code
;*******************************************************************************

SECTION .text

;*******************************************************************************
;   void IdctResAddPred_mmx( uint8_t *pPred, const int32_t kiStride, int16_t *pRs )
;*******************************************************************************

WELS_EXTERN IdctResAddPred_mmx
    %assign push_num 0
    LOAD_3_PARA
    SIGN_EXTENSION r1, r1d
    movq    mm0, [r2+ 0]
    movq    mm1, [r2+ 8]
    movq    mm2, [r2+16]
    movq    mm3, [r2+24]

    MMX_Trans4x4W        mm0, mm1, mm2, mm3, mm4
    MMX_IDCT            mm1, mm2, mm3, mm4, mm0, mm6
    MMX_Trans4x4W        mm1, mm3, mm0, mm4, mm2
    MMX_IDCT            mm3, mm0, mm4, mm2, mm1, mm6

    WELS_Zero           mm7
    WELS_DW32           mm6

    MMX_StoreDiff4P    mm3, mm0, mm6, mm7, [r0]
    MMX_StoreDiff4P    mm4, mm0, mm6, mm7, [r0+r1]
    lea     r0, [r0+2*r1]
    MMX_StoreDiff4P    mm1, mm0, mm6, mm7, [r0]
    MMX_StoreDiff4P    mm2, mm0, mm6, mm7, [r0+r1]


    emms
    ret

;void WelsBlockZero16x16_sse2(int16_t * block, int32_t stride);
WELS_EXTERN WelsBlockZero16x16_sse2
    %assign  push_num 0
    LOAD_2_PARA
    SIGN_EXTENSION r1, r1d
    shl     r1, 1
    pxor    xmm0, xmm0
%rep 16
    movdqa  [r0], xmm0
    movdqa  [r0+16], xmm0
    add     r0, r1
%endrep
    ret

;void WelsBlockZero8x8_sse2(int16_t * block, int32_t stride);
WELS_EXTERN WelsBlockZero8x8_sse2
    %assign  push_num 0
    LOAD_2_PARA
    SIGN_EXTENSION r1, r1d
    shl     r1, 1
    pxor    xmm0, xmm0
%rep 8
    movdqa  [r0], xmm0
    add     r0, r1
%endrep
    ret
