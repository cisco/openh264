;*!
;* \copy
;*     Copyright (c)  2010-2013, Cisco Systems
;*     All rights reserved.
;*
;*     Redistribution and use in source and binary forms, with or without
;*     modification, are permitted provided that the following conditions
;*     are met:
;*
;*        * Redistributions of source code must retain the above copyright
;*          notice, this list of conditions and the following disclaimer.
;*
;*        * Redistributions in binary form must reproduce the above copyright
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
;*      vaa.asm
;*
;*      Abstract
;*      sse2 for pVaa routines
;*
;*  History
;*      04/14/2010      Created
;*              06/07/2010      Added AnalysisVaaInfoIntra_sse2(ssse3)
;*              06/10/2010      Tune rc_sad_frame_sse2 and got about 40% improvement
;*              08/11/2010      Added abs_difference_mbrow_sse2 & sum_sqrsum_mbrow_sse2
;*
;*************************************************************************/
%include "asm_inc.asm"


;***********************************************************************
; Macros and other preprocessor constants
;***********************************************************************
%macro SUM_SQR_SSE2     3       ; dst, pSrc, zero
    movdqa %1, %2
    punpcklbw %1, %3
    punpckhbw %2, %3
    pmaddwd %1, %1
    pmaddwd %2, %2
    paddd %1, %2
    pshufd %2, %1, 04Eh   ; 01001110 B
    paddd %1, %2
    pshufd %2, %1, 0B1h   ; 10110001 B
    paddd %1, %2
%endmacro       ; END OF SUM_SQR_SSE2

%macro WELS_SAD_16x2_SSE2  3 ;esi :%1 edi:%2 ebx:%3
    movdqa        xmm1,   [%1]
    movdqa        xmm2,   [%2]
    movdqa        xmm3,   [%1+%3]
    movdqa        xmm4,   [%2+%3]
    psadbw        xmm1,   xmm2
    psadbw        xmm3,   xmm4
    paddd xmm6,   xmm1
    paddd xmm6,   xmm3
    lea           %1,     [%1+%3*2]
    lea           %2,     [%2+%3*2]
%endmacro

; by comparing it outperforms than phaddw(SSSE3) sets
%macro SUM_WORD_8x2_SSE2        2       ; dst(pSrc), tmp
    ; @sum_8x2 begin
    pshufd %2, %1, 04Eh   ; 01001110 B
    paddw %1, %2
    pshuflw %2, %1, 04Eh  ; 01001110 B
    paddw %1, %2
    pshuflw %2, %1, 0B1h  ; 10110001 B
    paddw %1, %2
    ; end of @sum_8x2
%endmacro       ; END of SUM_WORD_8x2_SSE2

%macro WELS_SAD_SUM_SQSUM_16x1_SSE2 3 ;esi:%1,edi:%2,ebx:%3
    movdqa        xmm1,   [%1]
    movdqa        xmm2,   [%2]
    movdqa        xmm3,   xmm1
    psadbw        xmm3,   xmm2
    paddd         xmm6,   xmm3

    movdqa        xmm3,   xmm1
    psadbw        xmm3,   xmm0
    paddd         xmm5,   xmm3

    movdqa        xmm2,   xmm1
    punpcklbw     xmm1,   xmm0
    punpckhbw     xmm2,   xmm0
    pmaddwd               xmm1,   xmm1
    pmaddwd               xmm2,   xmm2
    paddd         xmm4,   xmm1
    paddd         xmm4,   xmm2

    add           %1,     %3
    add           %2,     %3
%endmacro

%macro WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 3 ;esi:%1 edi:%2 ebx:%3
    movdqa        xmm1,   [%1]
    movdqa        xmm2,   [%2]
    movdqa        xmm3,   xmm1
    psadbw        xmm3,   xmm2
    paddd         xmm7,   xmm3    ; sad

    movdqa        xmm3,   xmm1
    pmaxub        xmm3,   xmm2
    pminub        xmm2,   xmm1
    psubb xmm3,   xmm2    ; diff

    movdqa        xmm2,   xmm1
    psadbw        xmm2,   xmm0
    paddd xmm6,   xmm2    ; sum

    movdqa                xmm2,   xmm1
    punpcklbw     xmm1,   xmm0
    punpckhbw     xmm2,   xmm0
    pmaddwd               xmm1,   xmm1
    pmaddwd               xmm2,   xmm2
    paddd         xmm5,   xmm1
    paddd         xmm5,   xmm2    ; sqsum

    movdqa                xmm1,   xmm3
    punpcklbw     xmm1,   xmm0
    punpckhbw     xmm3,   xmm0
    pmaddwd               xmm1,   xmm1
    pmaddwd               xmm3,   xmm3
    paddd         xmm4,   xmm1
    paddd         xmm4,   xmm3    ; sqdiff

    add           %1,     %3
    add           %2,     %3
%endmacro

%macro WELS_SAD_SD_MAD_16x1_SSE2       7 ;esi:%5 edi:%6 ebx:%7
%define sad_reg                 %1
%define sum_cur_reg             %2
%define sum_ref_reg             %3
%define mad_reg                 %4
    movdqa        xmm1,           [%5]
    movdqa        xmm2,           [%6]
    movdqa        xmm3,           xmm1
    psadbw        xmm3,           xmm0
    paddd         sum_cur_reg,    xmm3    ; sum_cur
    movdqa        xmm3,           xmm2
    psadbw        xmm3,           xmm0
    paddd sum_ref_reg,                    xmm3    ; sum_ref

    movdqa        xmm3,           xmm1
    pmaxub        xmm3,           xmm2
    pminub        xmm2,           xmm1
    psubb xmm3,           xmm2    ; abs diff
    pmaxub        mad_reg,        xmm3    ; max abs diff

    psadbw        xmm3,           xmm0
    paddd sad_reg,        xmm3    ; sad

    add                   %5,             %7
    add                   %6,             %7
%endmacro


%macro WELS_MAX_REG_SSE2       1       ; xmm1, xmm2, xmm3 can be used
%define max_reg  %1
    movdqa        xmm1,           max_reg
    psrldq        xmm1,           4
    pmaxub        max_reg,        xmm1
    movdqa        xmm1,           max_reg
    psrldq        xmm1,           2
    pmaxub        max_reg,        xmm1
    movdqa        xmm1,           max_reg
    psrldq        xmm1,           1
    pmaxub        max_reg,        xmm1
%endmacro

%macro WELS_SAD_BGD_SQDIFF_16x1_SSE2   7 ;esi:%5 edi:%6 ebx:%7
%define sad_reg         %1
%define sum_reg         %2
%define mad_reg         %3
%define sqdiff_reg      %4
    movdqa                xmm1,           [%5]
    movdqa                xmm2,           xmm1
    movdqa                xmm3,           xmm1
    punpcklbw     xmm2,           xmm0
    punpckhbw     xmm3,           xmm0
    pmaddwd               xmm2,           xmm2
    pmaddwd               xmm3,           xmm3
    paddd         xmm2,           xmm3
    movdqa                xmm3,           xmm2
    psllq         xmm2,           32
    psrlq         xmm3,           32
    psllq         xmm3,           32
    paddd         xmm2,           xmm3
    paddd         sad_reg,        xmm2            ; sqsum

    movdqa        xmm2,           [%6]
    movdqa        xmm3,           xmm1
    psadbw        xmm3,           xmm0
    paddd sum_reg,                        xmm3    ; sum_cur
    movdqa        xmm3,           xmm2
    psadbw        xmm3,           xmm0
    pslldq        xmm3,           4
    paddd sum_reg,                        xmm3    ; sum_ref

    movdqa        xmm3,           xmm1
    pmaxub        xmm3,           xmm2
    pminub        xmm2,           xmm1
    psubb xmm3,           xmm2    ; abs diff
    pmaxub        mad_reg,        xmm3    ; max abs diff

    movdqa        xmm1,           xmm3
    psadbw        xmm3,           xmm0
    paddd sad_reg,        xmm3    ; sad

    movdqa                xmm3,   xmm1
    punpcklbw     xmm1,   xmm0
    punpckhbw     xmm3,   xmm0
    pmaddwd               xmm1,   xmm1
    pmaddwd               xmm3,   xmm3
    paddd         sqdiff_reg,     xmm1
    paddd         sqdiff_reg,     xmm3    ; sqdiff

    add           %5,     %7
    add           %6,     %7
%endmacro


;***********************************************************************
; Code
;***********************************************************************

SECTION .text

%ifdef X86_32

;***********************************************************************
;   void SampleVariance16x16_sse2(      uint8_t * y_ref, int32_t y_ref_stride, uint8_t * y_src, int32_t y_src_stride,SMotionTextureUnit* pMotionTexture );
;***********************************************************************
WELS_EXTERN SampleVariance16x16_sse2
    push esi
    push edi
    push ebx

    sub esp, 16
    %define SUM                   [esp]
    %define SUM_CUR               [esp+4]
    %define SQR                   [esp+8]
    %define SQR_CUR               [esp+12]
    %define PUSH_SIZE     28      ; 12 + 16

    mov edi, [esp+PUSH_SIZE+4]    ; y_ref
    mov edx, [esp+PUSH_SIZE+8]    ; y_ref_stride
    mov esi, [esp+PUSH_SIZE+12]   ; y_src
    mov eax, [esp+PUSH_SIZE+16]   ; y_src_stride
    mov ecx, 010h                         ; height = 16

    pxor xmm7, xmm7
    movdqu SUM, xmm7

.hloops:
    movdqa xmm0, [edi]            ; y_ref
    movdqa xmm1, [esi]            ; y_src
    movdqa xmm2, xmm0             ; store first for future process
    movdqa xmm3, xmm1
    ; sum += diff;
    movdqa xmm4, xmm0
    psadbw xmm4, xmm1             ; 2 parts, [0,..,15], [64,..,79]
    ; to be continued for sum
    pshufd xmm5, xmm4, 0C6h       ; 11000110 B
    paddw xmm4, xmm5
    movd ebx, xmm4
    add SUM, ebx

    ; sqr += diff * diff;
    pmaxub xmm0, xmm1
    pminub xmm1, xmm2
    psubb xmm0, xmm1                              ; diff
    SUM_SQR_SSE2 xmm1, xmm0, xmm7 ; dst, pSrc, zero
    movd ebx, xmm1
    add SQR, ebx

    ; sum_cur += y_src[x];
    movdqa xmm0, xmm3             ; cur_orig
    movdqa xmm1, xmm0
    punpcklbw xmm0, xmm7
    punpckhbw xmm1, xmm7
    paddw xmm0, xmm1              ; 8x2
    SUM_WORD_8x2_SSE2 xmm0, xmm1
    movd ebx, xmm0
    and ebx, 0ffffh
    add SUM_CUR, ebx

    ; sqr_cur += y_src[x] * y_src[x];
    SUM_SQR_SSE2 xmm0, xmm3, xmm7 ; dst, pSrc, zero
    movd ebx, xmm0
    add SQR_CUR, ebx

    lea edi, [edi+edx]
    lea esi, [esi+eax]
    dec ecx
    jnz near .hloops

    mov ebx, 0
    mov bx, word SUM
    sar ebx, 8
    imul ebx, ebx
    mov ecx, SQR
    sar ecx, 8
    sub ecx, ebx
    mov edi, [esp+PUSH_SIZE+20]   ; pMotionTexture
    mov [edi], cx                         ; to store uiMotionIndex
    mov ebx, 0
    mov bx, word SUM_CUR
    sar ebx, 8
    imul ebx, ebx
    mov ecx, SQR_CUR
    sar ecx, 8
    sub ecx, ebx
    mov [edi+2], cx                               ; to store uiTextureIndex

    %undef SUM
    %undef SUM_CUR
    %undef SQR
    %undef SQR_CUR
    %undef PUSH_SIZE

    add esp, 16
    pop ebx
    pop edi
    pop esi

    ret



;*************************************************************************************************************
;void VAACalcSad_sse2( const uint8_t *cur_data, const uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight
;                                                               int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8)
;*************************************************************************************************************


WELS_EXTERN VAACalcSad_sse2
%define         cur_data                        esp + pushsize + 4
%define         ref_data                        esp + pushsize + 8
%define         iPicWidth                       esp + pushsize + 12
%define         iPicHeight                      esp + pushsize + 16
%define         iPicStride                      esp + pushsize + 20
%define         psadframe                       esp + pushsize + 24
%define         psad8x8                         esp + pushsize + 28
%define         pushsize        12
    push  esi
    push  edi
    push  ebx
    mov           esi,    [cur_data]
    mov           edi,    [ref_data]
    mov           ebx,    [iPicStride]
    mov           edx,    [psad8x8]
    mov           eax,    ebx

    shr           dword [iPicWidth],      4                                       ; iPicWidth/16
    shr           dword [iPicHeight],     4                                       ; iPicHeight/16
    shl           eax,    4                                                               ; iPicStride*16
    pxor  xmm0,   xmm0
    pxor  xmm7,   xmm7            ; iFrameSad
height_loop:
    mov           ecx,    dword [iPicWidth]
    push  esi
    push  edi
width_loop:
    pxor  xmm6,   xmm6            ;
    WELS_SAD_16x2_SSE2 esi,edi,ebx
    WELS_SAD_16x2_SSE2 esi,edi,ebx
    WELS_SAD_16x2_SSE2 esi,edi,ebx
    WELS_SAD_16x2_SSE2 esi,edi,ebx
    paddd xmm7,           xmm6
    movd  [edx],          xmm6
    psrldq        xmm6,           8
    movd  [edx+4],        xmm6

    pxor  xmm6,   xmm6
    WELS_SAD_16x2_SSE2 esi,edi,ebx
    WELS_SAD_16x2_SSE2 esi,edi,ebx
    WELS_SAD_16x2_SSE2 esi,edi,ebx
    WELS_SAD_16x2_SSE2 esi,edi,ebx
    paddd xmm7,           xmm6
    movd  [edx+8],        xmm6
    psrldq        xmm6,           8
    movd  [edx+12],       xmm6

    add           edx,    16
    sub           esi,    eax
    sub           edi,    eax
    add           esi,    16
    add           edi,    16

    dec           ecx
    jnz           width_loop

    pop           edi
    pop           esi
    add           esi,    eax
    add           edi,    eax

    dec   dword [iPicHeight]
    jnz           height_loop

    mov           edx,    [psadframe]
    movdqa        xmm5,   xmm7
    psrldq        xmm7,   8
    paddd xmm7,   xmm5
    movd  [edx],  xmm7

%undef          cur_data
%undef          ref_data
%undef          iPicWidth
%undef          iPicHeight
%undef          iPicStride
%undef          psadframe
%undef          psad8x8
%undef          pushsize
    pop           ebx
    pop           edi
    pop           esi
    ret

%else  ;64-bit

;***********************************************************************
;   void SampleVariance16x16_sse2(      uint8_t * y_ref, int32_t y_ref_stride, uint8_t * y_src, int32_t y_src_stride,SMotionTextureUnit* pMotionTexture );
;***********************************************************************
WELS_EXTERN SampleVariance16x16_sse2
    %define SUM                   r10;[esp]
    %define SUM_CUR               r11;[esp+4]
    %define SQR                   r13;[esp+8]
    %define SQR_CUR               r15;[esp+12]

    push r12
    push r13
    push r14
    push r15
    %assign push_num 4
    LOAD_5_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1,r1d
    SIGN_EXTENSION r3,r3d

    mov r12,010h
    pxor xmm7, xmm7
    movq SUM, xmm7
    movq SUM_CUR,xmm7
    movq SQR,xmm7
    movq SQR_CUR,xmm7

.hloops:
    mov r14,0
    movdqa xmm0, [r0]             ; y_ref
    movdqa xmm1, [r2]             ; y_src
    movdqa xmm2, xmm0             ; store first for future process
    movdqa xmm3, xmm1
    ; sum += diff;
    movdqa xmm4, xmm0
    psadbw xmm4, xmm1             ; 2 parts, [0,..,15], [64,..,79]
    ; to be continued for sum
    pshufd xmm5, xmm4, 0C6h       ; 11000110 B
    paddw xmm4, xmm5
    movd r14d, xmm4
    add SUM, r14

    ; sqr += diff * diff;
    pmaxub xmm0, xmm1
    pminub xmm1, xmm2
    psubb xmm0, xmm1                              ; diff
    SUM_SQR_SSE2 xmm1, xmm0, xmm7 ; dst, pSrc, zero
    movd r14d, xmm1
    add SQR, r14

    ; sum_cur += y_src[x];
    movdqa xmm0, xmm3             ; cur_orig
    movdqa xmm1, xmm0
    punpcklbw xmm0, xmm7
    punpckhbw xmm1, xmm7
    paddw xmm0, xmm1              ; 8x2
    SUM_WORD_8x2_SSE2 xmm0, xmm1
    movd r14d, xmm0
    and r14, 0ffffh
    add SUM_CUR, r14

    ; sqr_cur += y_src[x] * y_src[x];
    SUM_SQR_SSE2 xmm0, xmm3, xmm7 ; dst, pSrc, zero
    movd r14d, xmm0
    add SQR_CUR, r14

    lea r0, [r0+r1]
    lea r2, [r2+r3]
    dec r12
    jnz near .hloops

    mov r0, SUM
    sar r0, 8
    imul r0, r0
    mov r1, SQR
    sar r1, 8
    sub r1, r0
    mov [r4], r1w                         ; to store uiMotionIndex
    mov r0, SUM_CUR
    sar r0, 8
    imul r0, r0
    mov r1, SQR_CUR
    sar r1, 8
    sub r1, r0
    mov [r4+2], r1w                               ; to store uiTextureIndex

    POP_XMM
    LOAD_5_PARA_POP
    pop r15
    pop r14
    pop r13
    pop r12


    %assign push_num 0

    ret


;*************************************************************************************************************
;void VAACalcSad_sse2( const uint8_t *cur_data, const uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight
;                                                               int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8)
;*************************************************************************************************************


WELS_EXTERN VAACalcSad_sse2
%define         cur_data                        r0
%define         ref_data                        r1
%define         iPicWidth                       r2
%define         iPicHeight              r3
%define         iPicStride              r4
%define         psadframe                       r5
%define         psad8x8                         r6

    push r12
    push r13
    %assign push_num 2
    LOAD_7_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r2,r2d
    SIGN_EXTENSION r3,r3d
    SIGN_EXTENSION r4,r4d

    mov   r12,r4
    shr           r2,     4                                       ; iPicWidth/16
    shr           r3,     4                                       ; iPicHeight/16

    shl           r12,    4                                                               ; iPicStride*16
    pxor  xmm0,   xmm0
    pxor  xmm7,   xmm7            ; iFrameSad
height_loop:
    mov           r13,    r2
    push  r0
    push  r1
width_loop:
    pxor  xmm6,   xmm6
    WELS_SAD_16x2_SSE2 r0,r1,r4
    WELS_SAD_16x2_SSE2 r0,r1,r4
    WELS_SAD_16x2_SSE2 r0,r1,r4
    WELS_SAD_16x2_SSE2 r0,r1,r4
    paddd xmm7,           xmm6
    movd  [r6],           xmm6
    psrldq        xmm6,           8
    movd  [r6+4], xmm6

    pxor  xmm6,   xmm6
    WELS_SAD_16x2_SSE2 r0,r1,r4
    WELS_SAD_16x2_SSE2 r0,r1,r4
    WELS_SAD_16x2_SSE2 r0,r1,r4
    WELS_SAD_16x2_SSE2 r0,r1,r4
    paddd xmm7,           xmm6
    movd  [r6+8], xmm6
    psrldq        xmm6,           8
    movd  [r6+12],        xmm6

    add           r6,     16
    sub           r0,     r12
    sub           r1,     r12
    add           r0,     16
    add           r1,     16

    dec           r13
    jnz           width_loop

    pop           r1
    pop           r0
    add           r0,     r12
    add           r1,     r12

    dec   r3
    jnz           height_loop

    ;mov          r13,    [psadframe]
    movdqa        xmm5,   xmm7
    psrldq        xmm7,   8
    paddd xmm7,   xmm5
    movd  [psadframe],    xmm7

%undef          cur_data
%undef          ref_data
%undef          iPicWidth
%undef          iPicHeight
%undef          iPicStride
%undef          psadframe
%undef          psad8x8
%undef          pushsize
    POP_XMM
    LOAD_7_PARA_POP
    pop r13
    pop r12
    %assign push_num 0
    ret

%endif


%ifdef X86_32
;*************************************************************************************************************
;void VAACalcSadVar_sse2( const uint8_t *cur_data, const uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight
;               int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8, int32_t *psum16x16, int32_t *psqsum16x16)
;*************************************************************************************************************


WELS_EXTERN VAACalcSadVar_sse2
%define         localsize               8
%define         cur_data                        esp + pushsize + localsize + 4
%define         ref_data                        esp + pushsize + localsize + 8
%define         iPicWidth                       esp + pushsize + localsize + 12
%define         iPicHeight                      esp + pushsize + localsize + 16
%define         iPicStride                      esp + pushsize + localsize + 20
%define         psadframe                       esp + pushsize + localsize + 24
%define         psad8x8                         esp + pushsize + localsize + 28
%define         psum16x16                       esp + pushsize + localsize + 32
%define         psqsum16x16                     esp + pushsize + localsize + 36
%define         tmp_esi                         esp + 0
%define         tmp_edi                         esp + 4
%define         pushsize                16
    push  ebp
    push  esi
    push  edi
    push  ebx
    sub           esp,    localsize
    mov           esi,    [cur_data]
    mov           edi,    [ref_data]
    mov           ebx,    [iPicStride]
    mov           edx,    [psad8x8]
    mov           eax,    ebx

    shr           dword [iPicWidth],      4                                       ; iPicWidth/16
    shr           dword [iPicHeight],     4                                       ; iPicHeight/16
    shl           eax,    4                                                       ; iPicStride*16
    pxor  xmm0,   xmm0
    pxor  xmm7,   xmm7            ; iFrameSad
var_height_loop:
    mov           ecx,    dword [iPicWidth]
    mov           [tmp_esi],      esi
    mov           [tmp_edi],      edi
var_width_loop:
    pxor  xmm6,   xmm6            ; hiQuad_loQuad pSad8x8
    pxor  xmm5,   xmm5            ; pSum16x16
    pxor  xmm4,   xmm4            ; sqsum_16x16
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    paddd xmm7,           xmm6
    movd  [edx],          xmm6
    psrldq        xmm6,           8
    movd  [edx+4],        xmm6

    pxor  xmm6,   xmm6
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_16x1_SSE2 esi,edi,ebx
    paddd xmm7,           xmm6
    movd  [edx+8],        xmm6
    psrldq        xmm6,           8
    movd  [edx+12],       xmm6

    mov           ebp,    [psum16x16]
    movdqa        xmm1,   xmm5
    psrldq        xmm1,   8
    paddd xmm5,   xmm1
    movd  [ebp],  xmm5
    add           dword [psum16x16], 4

    movdqa        xmm5,   xmm4
    psrldq        xmm5,   8
    paddd xmm4,   xmm5
    movdqa        xmm3,   xmm4
    psrldq        xmm3,   4
    paddd xmm4,   xmm3

    mov           ebp,    [psqsum16x16]
    movd  [ebp],  xmm4
    add           dword [psqsum16x16], 4

    add           edx,    16
    sub           esi,    eax
    sub           edi,    eax
    add           esi,    16
    add           edi,    16

    dec           ecx
    jnz           var_width_loop

    mov           esi,    [tmp_esi]
    mov           edi,    [tmp_edi]
    add           esi,    eax
    add           edi,    eax

    dec   dword [iPicHeight]
    jnz           var_height_loop

    mov           edx,    [psadframe]
    movdqa        xmm5,   xmm7
    psrldq        xmm7,   8
    paddd xmm7,   xmm5
    movd  [edx],  xmm7

    add           esp,    localsize
    pop           ebx
    pop           edi
    pop           esi
    pop           ebp
%undef          cur_data
%undef          ref_data
%undef          iPicWidth
%undef          iPicHeight
%undef          iPicStride
%undef          psadframe
%undef          psad8x8
%undef          psum16x16
%undef          psqsum16x16
%undef          tmp_esi
%undef          tmp_edi
%undef          pushsize
%undef          localsize
    ret

%else  ;64-bit

;*************************************************************************************************************
;void VAACalcSadVar_sse2( const uint8_t *cur_data, const uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight
;               int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8, int32_t *psum16x16, int32_t *psqsum16x16)
;*************************************************************************************************************


WELS_EXTERN VAACalcSadVar_sse2
%define         cur_data                        arg1 ;r0
%define         ref_data                        arg2 ;r1
%define         iPicWidth                       arg3 ;r2
%define         iPicHeight                  arg4 ;r3
%define         iPicStride                  arg5
%define         psadframe                       arg6
%define         psad8x8                         arg7
%define         psum16x16                       arg8
%define         psqsum16x16                 arg9

    push r12
    push r13
    push r14
    push r15
    %assign push_num 4
    PUSH_XMM 8

%ifdef WIN64
    mov r4, arg5  ;iPicStride
    mov r5, arg6  ;psad8x8
%endif
    mov r14,arg7
    SIGN_EXTENSION r2,r2d
    SIGN_EXTENSION r3,r3d
    SIGN_EXTENSION r4,r4d

    mov   r13,r4
    shr   r2,4
    shr   r3,4

    shl   r13,4   ; iPicStride*16
    pxor  xmm0,   xmm0
    pxor  xmm7,   xmm7            ; iFrameSad
var_height_loop:
    push    r2
    %assign push_num push_num+1
    mov           r11,    r0
    mov           r12,    r1
var_width_loop:
    pxor  xmm6,   xmm6            ; hiQuad_loQuad pSad8x8
    pxor  xmm5,   xmm5            ; pSum16x16
    pxor  xmm4,   xmm4            ; sqsum_16x16
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    paddd xmm7,           xmm6
    movd  [r14],          xmm6
    psrldq        xmm6,           8
    movd  [r14+4],        xmm6

    pxor  xmm6,   xmm6
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_16x1_SSE2 r0,r1,r4
    paddd   xmm7,           xmm6
    movd    [r14+8],        xmm6
    psrldq  xmm6,           8
    movd    [r14+12],       xmm6

    mov             r15,    psum16x16
    movdqa  xmm1,   xmm5
    psrldq  xmm1,   8
    paddd   xmm5,   xmm1
    movd    [r15],  xmm5
    add             dword psum16x16, 4

    movdqa  xmm5,   xmm4
    psrldq  xmm5,   8
    paddd   xmm4,   xmm5
    movdqa  xmm3,   xmm4
    psrldq  xmm3,   4
    paddd   xmm4,   xmm3

    mov             r15,    psqsum16x16
    movd    [r15],  xmm4
    add             dword psqsum16x16, 4

    add             r14,16
    sub             r0,     r13
    sub             r1,     r13
    add             r0,     16
    add             r1,     16

    dec             r2
    jnz             var_width_loop

    pop     r2
    %assign push_num push_num-1
    mov             r0,     r11
    mov             r1,     r12
    add             r0,     r13
    add             r1,     r13
    dec     r3
    jnz             var_height_loop

    mov             r15,    psadframe
    movdqa  xmm5,   xmm7
    psrldq  xmm7,   8
    paddd   xmm7,   xmm5
    movd    [r15],  xmm7

    POP_XMM
    pop r15
    pop r14
    pop r13
    pop r12
%assign push_num 0
%undef          cur_data
%undef          ref_data
%undef          iPicWidth
%undef          iPicHeight
%undef          iPicStride
%undef          psadframe
%undef          psad8x8
%undef          psum16x16
%undef          psqsum16x16
%undef          tmp_esi
%undef          tmp_edi
%undef          pushsize
%undef          localsize
    ret

%endif

%ifdef X86_32

;*************************************************************************************************************
;void VAACalcSadSsd_sse2(const uint8_t *cur_data, const uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight,
;       int32_t iPicStride,int32_t *psadframe, int32_t *psad8x8, int32_t *psum16x16, int32_t *psqsum16x16, int32_t *psqdiff16x16)
;*************************************************************************************************************


WELS_EXTERN VAACalcSadSsd_sse2
%define         localsize               12
%define         cur_data                        esp + pushsize + localsize + 4
%define         ref_data                        esp + pushsize + localsize + 8
%define         iPicWidth                       esp + pushsize + localsize + 12
%define         iPicHeight                      esp + pushsize + localsize + 16
%define         iPicStride                      esp + pushsize + localsize + 20
%define         psadframe                       esp + pushsize + localsize + 24
%define         psad8x8                         esp + pushsize + localsize + 28
%define         psum16x16                       esp + pushsize + localsize + 32
%define         psqsum16x16                     esp + pushsize + localsize + 36
%define         psqdiff16x16            esp + pushsize + localsize + 40
%define         tmp_esi                         esp + 0
%define         tmp_edi                         esp + 4
%define         tmp_sadframe            esp + 8
%define         pushsize                16
    push    ebp
    push    esi
    push    edi
    push    ebx
    sub             esp,    localsize

    mov             ecx,    [iPicWidth]
    mov             ecx,    [iPicHeight]
    mov             esi,    [cur_data]
    mov             edi,    [ref_data]
    mov             ebx,    [iPicStride]
    mov             edx,    [psad8x8]
    mov             eax,    ebx

    shr             dword [iPicWidth],      4                                       ; iPicWidth/16
    shr             dword [iPicHeight],     4                                       ; iPicHeight/16
    shl             eax,    4                                                       ; iPicStride*16
    mov             ecx,    [iPicWidth]
    mov             ecx,    [iPicHeight]
    pxor    xmm0,   xmm0
    movd    [tmp_sadframe], xmm0
sqdiff_height_loop:
    mov             ecx,    dword [iPicWidth]
    mov             [tmp_esi],      esi
    mov             [tmp_edi],      edi
sqdiff_width_loop:
    pxor    xmm7,   xmm7            ; hiQuad_loQuad pSad8x8
    pxor    xmm6,   xmm6            ; pSum16x16
    pxor    xmm5,   xmm5            ; sqsum_16x16  four dword
    pxor    xmm4,   xmm4            ; sqdiff_16x16  four Dword
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    movdqa  xmm1,           xmm7
    movd    [edx],          xmm7
    psrldq  xmm7,           8
    paddd   xmm1,           xmm7
    movd    [edx+4],        xmm7
    movd    ebp,            xmm1
    add             [tmp_sadframe], ebp

    pxor    xmm7,   xmm7
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 esi,edi,ebx
    movdqa  xmm1,           xmm7
    movd    [edx+8],        xmm7
    psrldq  xmm7,           8
    paddd   xmm1,           xmm7
    movd    [edx+12],       xmm7
    movd    ebp,            xmm1
    add             [tmp_sadframe], ebp

    mov             ebp,    [psum16x16]
    movdqa  xmm1,   xmm6
    psrldq  xmm1,   8
    paddd   xmm6,   xmm1
    movd    [ebp],  xmm6
    add             dword [psum16x16], 4

    mov             ebp,    [psqsum16x16]
    pshufd  xmm6,   xmm5,   14 ;00001110
    paddd   xmm6,   xmm5
    pshufd  xmm5,   xmm6,   1  ;00000001
    paddd   xmm5,   xmm6
    movd    [ebp],  xmm5
    add             dword [psqsum16x16], 4

    mov             ebp,    [psqdiff16x16]
    pshufd  xmm5,   xmm4,   14      ; 00001110
    paddd   xmm5,   xmm4
    pshufd  xmm4,   xmm5,   1       ; 00000001
    paddd   xmm4,   xmm5
    movd    [ebp],  xmm4
    add             dword   [psqdiff16x16], 4

    add             edx,    16
    sub             esi,    eax
    sub             edi,    eax
    add             esi,    16
    add             edi,    16

    dec             ecx
    jnz             sqdiff_width_loop

    mov             esi,    [tmp_esi]
    mov             edi,    [tmp_edi]
    add             esi,    eax
    add             edi,    eax

    dec     dword [iPicHeight]
    jnz             sqdiff_height_loop

    mov             ebx,    [tmp_sadframe]
    mov             eax,    [psadframe]
    mov             [eax],  ebx

    add             esp,    localsize
    pop             ebx
    pop             edi
    pop             esi
    pop             ebp
%undef          cur_data
%undef          ref_data
%undef          iPicWidth
%undef          iPicHeight
%undef          iPicStride
%undef          psadframe
%undef          psad8x8
%undef          psum16x16
%undef          psqsum16x16
%undef          psqdiff16x16
%undef          tmp_esi
%undef          tmp_edi
%undef          tmp_sadframe
%undef          pushsize
%undef          localsize
    ret

%else


;*************************************************************************************************************
;void VAACalcSadSsd_sse2(const uint8_t *cur_data, const uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight,
;       int32_t iPicStride,int32_t *psadframe, int32_t *psad8x8, int32_t *psum16x16, int32_t *psqsum16x16, int32_t *psqdiff16x16)
;*************************************************************************************************************


WELS_EXTERN VAACalcSadSsd_sse2
%define         localsize               12
%define         cur_data                        arg1;r0
%define         ref_data                        arg2;r1
%define         iPicWidth                       arg3;r2
%define         iPicHeight                      arg4;r3
%define         iPicStride                      arg5;
%define         psadframe                       arg6;
%define         psad8x8                         arg7;
%define         psum16x16                       arg8;
%define         psqsum16x16                     arg9;
%define         psqdiff16x16                    arg10

    push r12
    push r13
    push r14
    push r15
    %assign push_num 4
    PUSH_XMM 10

%ifdef WIN64
    mov r4,arg5
%endif
    mov r14,arg7
    SIGN_EXTENSION r2,r2d
    SIGN_EXTENSION r3,r3d
    SIGN_EXTENSION r4,r4d

    mov        r13,r4
    shr     r2,4   ; iPicWidth/16
    shr     r3,4   ; iPicHeight/16
    shl     r13,4   ; iPicStride*16
    pxor    xmm0,   xmm0
    pxor  xmm8, xmm8  ;framesad
    pxor  xmm9, xmm9
sqdiff_height_loop:
    ;mov            ecx,    dword [iPicWidth]
    ;mov      r14,r2
    push r2
    %assign push_num push_num +1
    mov             r10,    r0
    mov             r11,    r1
sqdiff_width_loop:
    pxor    xmm7,   xmm7            ; hiQuad_loQuad pSad8x8
    pxor    xmm6,   xmm6            ; pSum16x16
    pxor    xmm5,   xmm5            ; sqsum_16x16  four dword
    pxor    xmm4,   xmm4            ; sqdiff_16x16  four Dword
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    movdqa  xmm1,           xmm7
    movd    [r14],          xmm7
    psrldq  xmm7,           8
    paddd   xmm1,           xmm7
    movd    [r14+4],        xmm7
    movd    r15d,           xmm1
    movd  xmm9, r15d
    paddd xmm8,xmm9


    pxor    xmm7,   xmm7
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 r0,r1,r4
    movdqa  xmm1,           xmm7
    movd    [r14+8],        xmm7
    psrldq  xmm7,           8
    paddd   xmm1,           xmm7
    movd    [r14+12],       xmm7
    movd    r15d,           xmm1
    movd  xmm9, r15d
    paddd xmm8,xmm9

    mov             r15,    psum16x16
    movdqa  xmm1,   xmm6
    psrldq  xmm1,   8
    paddd   xmm6,   xmm1
    movd    [r15],  xmm6
    add             dword psum16x16, 4

    mov             r15,    psqsum16x16
    pshufd  xmm6,   xmm5,   14 ;00001110
    paddd   xmm6,   xmm5
    pshufd  xmm5,   xmm6,   1  ;00000001
    paddd   xmm5,   xmm6
    movd    [r15],  xmm5
    add             dword psqsum16x16, 4

    mov             r15,    psqdiff16x16
    pshufd  xmm5,   xmm4,   14      ; 00001110
    paddd   xmm5,   xmm4
    pshufd  xmm4,   xmm5,   1       ; 00000001
    paddd   xmm4,   xmm5
    movd    [r15],  xmm4
    add             dword   psqdiff16x16,   4

    add             r14,16
    sub             r0,     r13
    sub             r1,     r13
    add             r0,     16
    add             r1,     16

    dec             r2
    jnz             sqdiff_width_loop

    pop r2
    %assign push_num push_num -1

    mov             r0,     r10
    mov             r1,     r11
    add             r0,     r13
    add             r1,     r13

    dec     r3
    jnz             sqdiff_height_loop

    mov             r13,    psadframe
    movd    [r13],  xmm8

    POP_XMM
    pop r15
    pop r14
    pop r13
    pop r12
    %assign push_num 0

%undef          cur_data
%undef          ref_data
%undef          iPicWidth
%undef          iPicHeight
%undef          iPicStride
%undef          psadframe
%undef          psad8x8
%undef          psum16x16
%undef          psqsum16x16
%undef          psqdiff16x16
%undef          tmp_esi
%undef          tmp_edi
%undef          tmp_sadframe
%undef          pushsize
%undef          localsize
    ret



%endif

%ifdef X86_32
;*************************************************************************************************************
;void VAACalcSadBgd_sse2(const uint8_t *cur_data, const uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight,
;                               int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8, int32_t *p_sd8x8, uint8_t *p_mad8x8)
;*************************************************************************************************************


WELS_EXTERN VAACalcSadBgd_sse2
%define         localsize               12
%define         cur_data                        esp + pushsize + localsize + 4
%define         ref_data                        esp + pushsize + localsize + 8
%define         iPicWidth                       esp + pushsize + localsize + 12
%define         iPicHeight                      esp + pushsize + localsize + 16
%define         iPicStride                      esp + pushsize + localsize + 20
%define         psadframe                       esp + pushsize + localsize + 24
%define         psad8x8                         esp + pushsize + localsize + 28
%define         p_sd8x8                         esp + pushsize + localsize + 32
%define         p_mad8x8                        esp + pushsize + localsize + 36
%define         tmp_esi                         esp + 0
%define         tmp_edi                         esp + 4
%define         tmp_ecx                         esp + 8
%define         pushsize                16
    push    ebp
    push    esi
    push    edi
    push    ebx
    sub             esp,    localsize
    mov             esi,    [cur_data]
    mov             edi,    [ref_data]
    mov             ebx,    [iPicStride]
    mov             eax,    ebx

    shr             dword [iPicWidth],      4                                       ; iPicWidth/16
    shr             dword [iPicHeight],     4                                       ; iPicHeight/16
    shl             eax,    4                                                       ; iPicStride*16
    xor             ebp,    ebp
    pxor    xmm0,   xmm0
bgd_height_loop:
    mov             ecx,    dword [iPicWidth]
    mov             [tmp_esi],      esi
    mov             [tmp_edi],      edi
bgd_width_loop:
    pxor    xmm7,   xmm7            ; pSad8x8
    pxor    xmm6,   xmm6            ; sum_cur_8x8
    pxor    xmm5,   xmm5            ; sum_ref_8x8
    pxor    xmm4,   xmm4            ; pMad8x8
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx


    mov                     edx,            [p_mad8x8]
    WELS_MAX_REG_SSE2       xmm4

    ;movdqa         xmm1,   xmm4
    ;punpcklbw      xmm1,   xmm0
    ;punpcklwd      xmm1,   xmm0
    ;movd           [edx],  xmm1
    ;punpckhbw      xmm4,   xmm0
    ;punpcklwd      xmm4,   xmm0
    ;movd           [edx+4],        xmm4
    ;add                    edx,            8
    ;mov                    [p_mad8x8],     edx
    mov                     [tmp_ecx],      ecx
    movhlps         xmm1,   xmm4
    movd            ecx,    xmm4
    mov                     [edx],  cl
    movd            ecx,    xmm1
    mov                     [edx+1],cl
    add                     edx,    2
    mov                     [p_mad8x8],     edx


    pslldq          xmm7,   4
    pslldq          xmm6,   4
    pslldq          xmm5,   4


    pxor    xmm4,   xmm4            ; pMad8x8
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,esi ,edi, ebx

    mov                     edx,            [p_mad8x8]
    WELS_MAX_REG_SSE2       xmm4

    ;movdqa         xmm1,   xmm4
    ;punpcklbw      xmm1,   xmm0
    ;punpcklwd      xmm1,   xmm0
    ;movd           [edx],  xmm1
    ;punpckhbw      xmm4,   xmm0
    ;punpcklwd      xmm4,   xmm0
    ;movd           [edx+4],        xmm4
    ;add                    edx,            8
    ;mov                    [p_mad8x8],     edx
    movhlps         xmm1,   xmm4
    movd            ecx,    xmm4
    mov                     [edx],  cl
    movd            ecx,    xmm1
    mov                     [edx+1],cl
    add                     edx,    2
    mov                     [p_mad8x8],     edx

    ; data in xmm7, xmm6, xmm5:  D1 D3 D0 D2

    mov             edx,    [psad8x8]
    pshufd  xmm1,   xmm7,   10001101b               ; D3 D2 D1 D0
    movdqa  [edx],  xmm1
    add             edx,    16
    mov             [psad8x8],      edx                                     ; sad8x8

    paddd   xmm1,   xmm7                                    ; D1+3 D3+2 D0+1 D2+0
    pshufd  xmm2,   xmm1,   00000011b
    paddd   xmm1,   xmm2
    movd    edx,    xmm1
    add             ebp,    edx                                             ; sad frame

    mov             edx,    [p_sd8x8]
    psubd   xmm6,   xmm5
    pshufd  xmm1,   xmm6,   10001101b
    movdqa  [edx],  xmm1
    add             edx,    16
    mov             [p_sd8x8],      edx


    add             edx,    16
    sub             esi,    eax
    sub             edi,    eax
    add             esi,    16
    add             edi,    16

    mov             ecx,    [tmp_ecx]
    dec             ecx
    jnz             bgd_width_loop

    mov             esi,    [tmp_esi]
    mov             edi,    [tmp_edi]
    add             esi,    eax
    add             edi,    eax

    dec             dword [iPicHeight]
    jnz             bgd_height_loop

    mov             edx,    [psadframe]
    mov             [edx],  ebp

    add             esp,    localsize
    pop             ebx
    pop             edi
    pop             esi
    pop             ebp
%undef          cur_data
%undef          ref_data
%undef          iPicWidth
%undef          iPicHeight
%undef          iPicStride
%undef          psadframe
%undef          psad8x8
%undef          p_sd8x8
%undef          p_mad8x8
%undef          tmp_esi
%undef          tmp_edi
%undef          pushsize
%undef          localsize
    ret



;*************************************************************************************************************
;void VAACalcSadSsdBgd_sse2(const uint8_t *cur_data, const uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight,
;                int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8, int32_t *psum16x16, int32_t *psqsum16x16,
;                       int32_t *psqdiff16x16, int32_t *p_sd8x8, uint8_t *p_mad8x8)
;*************************************************************************************************************


WELS_EXTERN VAACalcSadSsdBgd_sse2
%define         localsize               16
%define         cur_data                        esp + pushsize + localsize + 4
%define         ref_data                        esp + pushsize + localsize + 8
%define         iPicWidth                       esp + pushsize + localsize + 12
%define         iPicHeight                      esp + pushsize + localsize + 16
%define         iPicStride                      esp + pushsize + localsize + 20
%define         psadframe                       esp + pushsize + localsize + 24
%define         psad8x8                         esp + pushsize + localsize + 28
%define         psum16x16                       esp + pushsize + localsize + 32
%define         psqsum16x16                     esp + pushsize + localsize + 36
%define         psqdiff16x16            esp + pushsize + localsize + 40
%define         p_sd8x8                         esp + pushsize + localsize + 44
%define         p_mad8x8                        esp + pushsize + localsize + 48
%define         tmp_esi                         esp + 0
%define         tmp_edi                         esp + 4
%define         tmp_sadframe            esp + 8
%define         tmp_ecx                         esp + 12
%define         pushsize                16
    push    ebp
    push    esi
    push    edi
    push    ebx
    sub             esp,    localsize
    mov             esi,    [cur_data]
    mov             edi,    [ref_data]
    mov             ebx,    [iPicStride]
    mov             eax,    ebx

    shr             dword [iPicWidth],      4                                       ; iPicWidth/16
    shr             dword [iPicHeight],     4                                       ; iPicHeight/16
    shl             eax,    4                                                       ; iPicStride*16
    pxor    xmm0,   xmm0
    movd    [tmp_sadframe], xmm0
sqdiff_bgd_height_loop:
    mov             ecx,    dword [iPicWidth]
    mov             [tmp_esi],      esi
    mov             [tmp_edi],      edi
sqdiff_bgd_width_loop:
    pxor    xmm7,   xmm7            ; pSad8x8 interleaves sqsum16x16:  sqsum1 sad1 sqsum0 sad0
    pxor    xmm6,   xmm6            ; sum_8x8 interleaves cur and pRef in Dword,  Sref1 Scur1 Sref0 Scur0
    pxor    xmm5,   xmm5            ; pMad8x8
    pxor    xmm4,   xmm4            ; sqdiff_16x16  four Dword
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx

    mov             edx,            [psad8x8]
    movdqa  xmm2,           xmm7
    pshufd  xmm1,           xmm2,           00001110b
    movd    [edx],          xmm2
    movd    [edx+4],        xmm1
    add             edx,            8
    mov             [psad8x8],      edx                     ; sad8x8

    paddd   xmm1,                           xmm2
    movd    edx,                            xmm1
    add             [tmp_sadframe],         edx                     ; iFrameSad

    mov             edx,            [psum16x16]
    movdqa  xmm1,           xmm6
    pshufd  xmm2,           xmm1,           00001110b
    paddd   xmm1,           xmm2
    movd    [edx],          xmm1                            ; sum

    mov             edx,            [p_sd8x8]
    pshufd  xmm1,           xmm6,           11110101b                       ; Sref1 Sref1 Sref0 Sref0
    psubd   xmm6,           xmm1            ; 00 diff1 00 diff0
    pshufd  xmm1,           xmm6,           00001000b                       ;  xx xx diff1 diff0
    movq    [edx],          xmm1
    add             edx,            8
    mov             [p_sd8x8],      edx

    mov                     edx,            [p_mad8x8]
    WELS_MAX_REG_SSE2       xmm5
    ;movdqa         xmm1,   xmm5
    ;punpcklbw      xmm1,   xmm0
    ;punpcklwd      xmm1,   xmm0
    ;movd           [edx],  xmm1
    ;punpckhbw      xmm5,   xmm0
    ;punpcklwd      xmm5,   xmm0
    ;movd           [edx+4],        xmm5
    ;add                    edx,            8
    ;mov                    [p_mad8x8],     edx
    mov                     [tmp_ecx],      ecx
    movhlps         xmm1,   xmm5
    movd            ecx,    xmm5
    mov                     [edx],  cl
    movd            ecx,    xmm1
    mov                     [edx+1],cl
    add                     edx,    2
    mov                     [p_mad8x8],     edx

    psrlq   xmm7,   32
    psllq   xmm7,   32                      ; clear sad
    pxor    xmm6,   xmm6            ; sum_8x8 interleaves cur and pRef in Dword,  Sref1 Scur1 Sref0 Scur0
    pxor    xmm5,   xmm5            ; pMad8x8
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, esi , edi , ebx

    mov             edx,            [psad8x8]
    movdqa  xmm2,           xmm7
    pshufd  xmm1,           xmm2,           00001110b
    movd    [edx],          xmm2
    movd    [edx+4],        xmm1
    add             edx,            8
    mov             [psad8x8],      edx                     ; sad8x8

    paddd   xmm1,                           xmm2
    movd    edx,                            xmm1
    add             [tmp_sadframe],         edx                     ; iFrameSad

    mov             edx,                    [psum16x16]
    movdqa  xmm1,                   xmm6
    pshufd  xmm2,                   xmm1,           00001110b
    paddd   xmm1,                   xmm2
    movd    ebp,                    xmm1                            ; sum
    add             [edx],                  ebp
    add             edx,                    4
    mov             [psum16x16],    edx

    mov             edx,                    [psqsum16x16]
    psrlq   xmm7,                   32
    pshufd  xmm2,                   xmm7,           00001110b
    paddd   xmm2,                   xmm7
    movd    [edx],                  xmm2                            ; sqsum
    add             edx,                    4
    mov             [psqsum16x16],  edx

    mov             edx,            [p_sd8x8]
    pshufd  xmm1,           xmm6,           11110101b                       ; Sref1 Sref1 Sref0 Sref0
    psubd   xmm6,           xmm1            ; 00 diff1 00 diff0
    pshufd  xmm1,           xmm6,           00001000b                       ;  xx xx diff1 diff0
    movq    [edx],          xmm1
    add             edx,            8
    mov             [p_sd8x8],      edx

    mov             edx,            [p_mad8x8]
    WELS_MAX_REG_SSE2       xmm5
    ;movdqa         xmm1,   xmm5
    ;punpcklbw      xmm1,   xmm0
    ;punpcklwd      xmm1,   xmm0
    ;movd           [edx],  xmm1
    ;punpckhbw      xmm5,   xmm0
    ;punpcklwd      xmm5,   xmm0
    ;movd           [edx+4],        xmm5
    ;add                    edx,            8
    ;mov                    [p_mad8x8],     edx
    movhlps         xmm1,   xmm5
    movd            ecx,    xmm5
    mov                     [edx],  cl
    movd            ecx,    xmm1
    mov                     [edx+1],cl
    add                     edx,    2
    mov                     [p_mad8x8],     edx

    mov             edx,            [psqdiff16x16]
    pshufd  xmm1,           xmm4,           00001110b
    paddd   xmm4,           xmm1
    pshufd  xmm1,           xmm4,           00000001b
    paddd   xmm4,           xmm1
    movd    [edx],          xmm4
    add             edx,            4
    mov             [psqdiff16x16], edx

    add             edx,    16
    sub             esi,    eax
    sub             edi,    eax
    add             esi,    16
    add             edi,    16

    mov             ecx,    [tmp_ecx]
    dec             ecx
    jnz             sqdiff_bgd_width_loop

    mov             esi,    [tmp_esi]
    mov             edi,    [tmp_edi]
    add             esi,    eax
    add             edi,    eax

    dec     dword [iPicHeight]
    jnz             sqdiff_bgd_height_loop

    mov             edx,    [psadframe]
    mov             ebp,    [tmp_sadframe]
    mov             [edx],  ebp

    add             esp,    localsize
    pop             ebx
    pop             edi
    pop             esi
    pop             ebp
%undef          cur_data
%undef          ref_data
%undef          iPicWidth
%undef          iPicHeight
%undef          iPicStride
%undef          psadframe
%undef          psad8x8
%undef          psum16x16
%undef          psqsum16x16
%undef          psqdiff16x16
%undef          p_sd8x8
%undef          p_mad8x8
%undef          tmp_esi
%undef          tmp_edi
%undef          pushsize
%undef          localsize
    ret
%else

;*************************************************************************************************************
;void VAACalcSadBgd_sse2(const uint8_t *cur_data, const uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight,
;                               int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8, int32_t *p_sd8x8, uint8_t *p_mad8x8)
;*************************************************************************************************************


WELS_EXTERN VAACalcSadBgd_sse2
%define         cur_data                        arg1;
%define         ref_data                        arg2;
%define         iPicWidth                       arg3;
%define         iPicHeight                      arg4;
%define         iPicStride                      arg5;
%define         psadframe                       arg6;
%define         psad8x8                         arg7;
%define         p_sd8x8                         arg8;
%define         p_mad8x8                        arg9;

    push r12
    push r13
    push r14
    push r15
%assign push_num 4
    PUSH_XMM 10
%ifdef WIN64
    mov r4,arg5
    ;  mov r5,arg6
%endif
    mov r14,arg7
    SIGN_EXTENSION r2,r2d
    SIGN_EXTENSION r3,r3d
    SIGN_EXTENSION r4,r4d


    mov     r13,r4
    mov     r15,r0
    shr     r2,4
    shr     r3,4
    shl     r13,4
    pxor    xmm0,   xmm0
    pxor    xmm8,   xmm8
    pxor    xmm9,   xmm9
bgd_height_loop:
    ;mov            ecx,    dword [iPicWidth]
    push r2
    %assign push_num push_num+1
    mov             r10,    r15
    mov             r11,    r1
bgd_width_loop:
    pxor    xmm7,   xmm7            ; pSad8x8
    pxor    xmm6,   xmm6            ; sum_cur_8x8
    pxor    xmm5,   xmm5            ; sum_ref_8x8
    pxor    xmm4,   xmm4            ; pMad8x8
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4


    mov                     r14,            p_mad8x8
    WELS_MAX_REG_SSE2       xmm4

    ;mov                    [tmp_ecx],      ecx
    movhlps         xmm1,   xmm4
    movd            r0d,    xmm4


    mov                     [r14],  r0b
    movd            r0d,    xmm1
    mov                     [r14+1],r0b
    add                     r14,    2
    ;mov                     p_mad8x8,       r14


    pslldq          xmm7,   4
    pslldq          xmm6,   4
    pslldq          xmm5,   4


    pxor    xmm4,   xmm4            ; pMad8x8
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4
    WELS_SAD_SD_MAD_16x1_SSE2       xmm7,   xmm6,   xmm5,   xmm4 ,r15 ,r1, r4

    ;mov                     r14,            [p_mad8x8]
    WELS_MAX_REG_SSE2       xmm4

    movhlps         xmm1,   xmm4
    movd            r0d,    xmm4
    mov                     [r14],  r0b
    movd            r0d,    xmm1
    mov                     [r14+1],r0b
    add                     r14,    2
    mov                     p_mad8x8,       r14

    ; data in xmm7, xmm6, xmm5:  D1 D3 D0 D2

    mov             r14,    psad8x8
    pshufd  xmm1,   xmm7,   10001101b               ; D3 D2 D1 D0
    movdqa  [r14],  xmm1
    add             r14,    16
    mov             psad8x8,        r14                                     ; sad8x8

    paddd   xmm1,   xmm7                                    ; D1+3 D3+2 D0+1 D2+0
    pshufd  xmm2,   xmm1,   00000011b
    paddd   xmm1,   xmm2
    movd    r14d,   xmm1
    movd    xmm9, r14d
    paddd   xmm8,   xmm9                                            ; sad frame

    mov             r14,    p_sd8x8
    psubd   xmm6,   xmm5
    pshufd  xmm1,   xmm6,   10001101b
    movdqa  [r14],  xmm1
    add             r14,    16
    mov             p_sd8x8,        r14


    ;add            edx,    16
    sub             r15,    r13
    sub             r1,     r13
    add             r15,    16
    add             r1,     16


    dec             r2
    jnz             bgd_width_loop
    pop     r2
%assign push_num push_num-1
    mov             r15,    r10
    mov             r1,     r11
    add             r15,    r13
    add             r1,     r13

    dec             r3
    jnz             bgd_height_loop

    mov             r13,    psadframe
    movd    [r13],  xmm8

    POP_XMM
    pop r15
    pop r14
    pop r13
    pop r12
%assign push_num 0
%undef          cur_data
%undef          ref_data
%undef          iPicWidth
%undef          iPicHeight
%undef          iPicStride
%undef          psadframe
%undef          psad8x8
%undef          p_sd8x8
%undef          p_mad8x8
%undef          tmp_esi
%undef          tmp_edi
%undef          pushsize
%undef          localsize
    ret



;*************************************************************************************************************
;void VAACalcSadSsdBgd_sse2(const uint8_t *cur_data, const uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight,
;                int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8, int32_t *psum16x16, int32_t *psqsum16x16,
;                       int32_t *psqdiff16x16, int32_t *p_sd8x8, uint8_t *p_mad8x8)
;*************************************************************************************************************


WELS_EXTERN VAACalcSadSsdBgd_sse2
%define         cur_data                        arg1;
%define         ref_data                        arg2;
%define         iPicWidth                       arg3;
%define         iPicHeight                      arg4;
%define         iPicStride                      arg5;
%define         psadframe                       arg6;
%define         psad8x8                         arg7;
%define         psum16x16                       arg8;
%define         psqsum16x16                     arg9;
%define         psqdiff16x16                    arg10;
%define         p_sd8x8                         arg11
%define         p_mad8x8                        arg12

    push r12
    push r13
    push r14
    push r15
%assign push_num 4
    PUSH_XMM 10
%ifdef WIN64
    mov r4,arg5
    ;mov r5,arg6
%endif
    SIGN_EXTENSION r2,r2d
    SIGN_EXTENSION r3,r3d
    SIGN_EXTENSION r4,r4d

    mov     r13,r4
    shr             r2,     4                                       ; iPicWidth/16
    shr             r3,     4                                       ; iPicHeight/16
    shl             r13,    4                                                       ; iPicStride*16
    pxor    xmm0,   xmm0
    pxor    xmm8,   xmm8
    pxor    xmm9,   xmm9


sqdiff_bgd_height_loop:
    mov             r10,    r0
    mov             r11,    r1
    push r2
%assign push_num push_num+1
sqdiff_bgd_width_loop:

    pxor    xmm7,   xmm7            ; pSad8x8 interleaves sqsum16x16:  sqsum1 sad1 sqsum0 sad0
    pxor    xmm6,   xmm6            ; sum_8x8 interleaves cur and pRef in Dword,  Sref1 Scur1 Sref0 Scur0
    pxor    xmm5,   xmm5            ; pMad8x8
    pxor    xmm4,   xmm4            ; sqdiff_16x16  four Dword
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4

    mov             r14,            psad8x8
    movdqa  xmm2,           xmm7
    pshufd  xmm1,           xmm2,           00001110b
    movd    [r14],          xmm2
    movd    [r14+4],        xmm1
    add             r14,            8
    mov             psad8x8,        r14                     ; sad8x8

    paddd   xmm1,                           xmm2
    movd    r14d,                           xmm1
    movd    xmm9,r14d
    paddd           xmm8,           xmm9                    ; iFrameSad

    mov             r14,            psum16x16
    movdqa  xmm1,           xmm6
    pshufd  xmm2,           xmm1,           00001110b
    paddd   xmm1,           xmm2
    movd    [r14],          xmm1                            ; sum

    mov             r14,            p_sd8x8
    pshufd  xmm1,           xmm6,           11110101b                       ; Sref1 Sref1 Sref0 Sref0
    psubd   xmm6,           xmm1            ; 00 diff1 00 diff0
    pshufd  xmm1,           xmm6,           00001000b                       ;  xx xx diff1 diff0
    movq    [r14],          xmm1
    add             r14,            8
    mov             p_sd8x8,        r14

    mov                     r14,            p_mad8x8
    WELS_MAX_REG_SSE2       xmm5

    movhlps         xmm1,   xmm5
    push r0
    movd            r0d,    xmm5
    mov                     [r14],  r0b
    movd            r0d,    xmm1
    mov                     [r14+1],r0b
    pop r0
    add                     r14,    2
    mov                     p_mad8x8,       r14

    psrlq   xmm7,   32
    psllq   xmm7,   32                      ; clear sad
    pxor    xmm6,   xmm6            ; sum_8x8 interleaves cur and pRef in Dword,  Sref1 Scur1 Sref0 Scur0
    pxor    xmm5,   xmm5            ; pMad8x8
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4
    WELS_SAD_BGD_SQDIFF_16x1_SSE2   xmm7,   xmm6,   xmm5,   xmm4, r0 , r1 , r4

    mov             r14,            psad8x8
    movdqa  xmm2,           xmm7
    pshufd  xmm1,           xmm2,           00001110b
    movd    [r14],          xmm2
    movd    [r14+4],        xmm1
    add             r14,            8
    mov             psad8x8,        r14                     ; sad8x8

    paddd   xmm1,                           xmm2
    movd    r14d,                           xmm1
    movd    xmm9, r14d
    paddd   xmm8,           xmm9            ; iFrameSad

    mov             r14,                    psum16x16
    movdqa  xmm1,                   xmm6
    pshufd  xmm2,                   xmm1,           00001110b
    paddd   xmm1,                   xmm2
    movd    r15d,                   xmm1                            ; sum
    add             [r14],                  r15d
    add             r14,                    4
    mov             psum16x16,      r14

    mov             r14,                    psqsum16x16
    psrlq   xmm7,                   32
    pshufd  xmm2,                   xmm7,           00001110b
    paddd   xmm2,                   xmm7
    movd    [r14],                  xmm2                            ; sqsum
    add             r14,                    4
    mov             psqsum16x16,    r14

    mov             r14,            p_sd8x8
    pshufd  xmm1,           xmm6,           11110101b                       ; Sref1 Sref1 Sref0 Sref0
    psubd   xmm6,           xmm1            ; 00 diff1 00 diff0
    pshufd  xmm1,           xmm6,           00001000b                       ;  xx xx diff1 diff0
    movq    [r14],          xmm1
    add             r14,            8
    mov             p_sd8x8,        r14

    mov             r14,            p_mad8x8
    WELS_MAX_REG_SSE2       xmm5


    movhlps         xmm1,   xmm5
    push r0
    movd            r0d,    xmm5
    mov                     [r14],  r0b
    movd            r0d,    xmm1
    mov                     [r14+1],r0b
    pop r0
    add                     r14,    2
    mov                     p_mad8x8,       r14

    mov             r14,            psqdiff16x16
    pshufd  xmm1,           xmm4,           00001110b
    paddd   xmm4,           xmm1
    pshufd  xmm1,           xmm4,           00000001b
    paddd   xmm4,           xmm1
    movd    [r14],          xmm4
    add             r14,            4
    mov             psqdiff16x16,   r14

    add             r14,    16
    sub             r0,     r13
    sub             r1,     r13
    add             r0,     16
    add             r1,     16

    dec             r2
    jnz             sqdiff_bgd_width_loop
    pop r2
    %assign push_num push_num-1
    mov             r0,     r10
    mov             r1,     r11
    add             r0,     r13
    add             r1,     r13

    dec     r3
    jnz             sqdiff_bgd_height_loop

    mov             r14,    psadframe
    movd    [r14],  xmm8

    POP_XMM
    pop r15
    pop r14
    pop r13
    pop r12
%assign push_num 0
%undef          cur_data
%undef          ref_data
%undef          iPicWidth
%undef          iPicHeight
%undef          iPicStride
%undef          psadframe
%undef          psad8x8
%undef          psum16x16
%undef          psqsum16x16
%undef          psqdiff16x16
%undef          p_sd8x8
%undef          p_mad8x8
%undef          tmp_esi
%undef          tmp_edi
%undef          pushsize
%undef          localsize
    ret
%endif
