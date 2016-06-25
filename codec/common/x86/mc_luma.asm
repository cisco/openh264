;*!
;* \copy
;*     Copyright (c)  2009-2013, Cisco Systems
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
;*  mc_luma.asm
;*
;*  Abstract
;*      sse2 motion compensation
;*
;*  History
;*      17/08/2009 Created
;*
;*
;*************************************************************************/
%include "asm_inc.asm"

;*******************************************************************************
; Local Data (Read Only)
;*******************************************************************************
SECTION .rodata align=16

;*******************************************************************************
; Various memory constants (trigonometric values or rounding values)
;*******************************************************************************

ALIGN 16
h264_w0x10:
    dw 16, 16, 16, 16
ALIGN 16
h264_w0x10_1:
    dw 16, 16, 16, 16, 16, 16, 16, 16
ALIGN 16
h264_mc_hc_32:
    dw 32, 32, 32, 32, 32, 32, 32, 32


;*******************************************************************************
; Code
;*******************************************************************************

SECTION .text



;*******************************************************************************
; void McHorVer20WidthEq4_mmx( const uint8_t *pSrc,
;                       int iSrcStride,
;                       uint8_t *pDst,
;                       int iDstStride,
;                       int iHeight)
;*******************************************************************************
WELS_EXTERN McHorVer20WidthEq4_mmx
    %assign  push_num 0
    LOAD_5_PARA
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r4, r4d

    sub r0, 2
    WELS_Zero mm7
    movq mm6, [h264_w0x10]
.height_loop:
    movd mm0, [r0]
    punpcklbw mm0, mm7
    movd mm1, [r0+5]
    punpcklbw mm1, mm7
    movd mm2, [r0+1]
    punpcklbw mm2, mm7
    movd mm3, [r0+4]
    punpcklbw mm3, mm7
    movd mm4, [r0+2]
    punpcklbw mm4, mm7
    movd mm5, [r0+3]
    punpcklbw mm5, mm7

    paddw mm2, mm3
    paddw mm4, mm5
    psllw mm4, 2
    psubw mm4, mm2
    paddw mm0, mm1
    paddw mm0, mm4
    psllw mm4, 2
    paddw mm0, mm4
    paddw mm0, mm6
    psraw mm0, 5
    packuswb mm0, mm7
    movd [r2], mm0

    add r0, r1
    add r2, r3
    dec r4
    jnz .height_loop

    WELSEMMS
    LOAD_5_PARA_POP
    ret

;*******************************************************************************
; Macros and other preprocessor constants
;*******************************************************************************


%macro SSE_LOAD_8P 3
    movq %1, %3
    punpcklbw %1, %2
%endmacro

%macro FILTER_HV_W8 9
    paddw   %1, %6
    movdqa  %8, %3
    movdqa  %7, %2
    paddw   %1, [h264_w0x10_1]
    paddw   %8, %4
    paddw   %7, %5
    psllw   %8, 2
    psubw   %8, %7
    paddw   %1, %8
    psllw   %8, 2
    paddw   %1, %8
    psraw   %1, 5
    WELS_Zero %8
    packuswb %1, %8
    movq    %9, %1
%endmacro


%macro FILTER_HV_W4 9
paddw   %1, %6
movdqa  %8, %3
movdqa  %7, %2
paddw   %1, [h264_w0x10_1]
paddw   %8, %4
paddw   %7, %5
psllw   %8, 2
psubw   %8, %7
paddw   %1, %8
psllw   %8, 2
paddw   %1, %8
psraw   %1, 5
WELS_Zero %8
packuswb %1, %8
movd    %9, %1
%endmacro


;*******************************************************************************
; Code
;*******************************************************************************

SECTION .text

;***********************************************************************
; void McHorVer22Width8HorFirst_sse2(const int16_t *pSrc,
;                       int16_t iSrcStride,
;                       uint8_t *pDst,
;                       int32_t iDstStride
;                       int32_t iHeight
;                       )
;***********************************************************************
WELS_EXTERN McHorVer22Width8HorFirst_sse2
    %assign  push_num 0
    LOAD_5_PARA
    PUSH_XMM 8
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r4, r4d
    pxor xmm7, xmm7

    sub r0, r1              ;;;;;;;;need more 5 lines.
    sub r0, r1

.yloop_width_8:
    movq xmm0, [r0]
    punpcklbw xmm0, xmm7
    movq xmm1, [r0+5]
    punpcklbw xmm1, xmm7
    movq xmm2, [r0+1]
    punpcklbw xmm2, xmm7
    movq xmm3, [r0+4]
    punpcklbw xmm3, xmm7
    movq xmm4, [r0+2]
    punpcklbw xmm4, xmm7
    movq xmm5, [r0+3]
    punpcklbw xmm5, xmm7

    paddw xmm2, xmm3
    paddw xmm4, xmm5
    psllw xmm4, 2
    psubw xmm4, xmm2
    paddw xmm0, xmm1
    paddw xmm0, xmm4
    psllw xmm4, 2
    paddw xmm0, xmm4
    movdqa [r2], xmm0

    add r0, r1
    add r2, r3
    dec r4
    jnz .yloop_width_8
    POP_XMM
    LOAD_5_PARA_POP
    ret

;*******************************************************************************
; void McHorVer20WidthEq8_sse2(  const uint8_t *pSrc,
;                       int iSrcStride,
;                                               uint8_t *pDst,
;                                               int iDstStride,
;                                               int iHeight,
;                      );
;*******************************************************************************
WELS_EXTERN McHorVer20WidthEq8_sse2
    %assign  push_num 0
    LOAD_5_PARA
    PUSH_XMM 8
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r4, r4d
    lea r0, [r0-2]            ;pSrc -= 2;

    pxor xmm7, xmm7
    movdqa xmm6, [h264_w0x10_1]
.y_loop:
    movq xmm0, [r0]
    punpcklbw xmm0, xmm7
    movq xmm1, [r0+5]
    punpcklbw xmm1, xmm7
    movq xmm2, [r0+1]
    punpcklbw xmm2, xmm7
    movq xmm3, [r0+4]
    punpcklbw xmm3, xmm7
    movq xmm4, [r0+2]
    punpcklbw xmm4, xmm7
    movq xmm5, [r0+3]
    punpcklbw xmm5, xmm7

    paddw xmm2, xmm3
    paddw xmm4, xmm5
    psllw xmm4, 2
    psubw xmm4, xmm2
    paddw xmm0, xmm1
    paddw xmm0, xmm4
    psllw xmm4, 2
    paddw xmm0, xmm4
    paddw xmm0, xmm6
    psraw xmm0, 5

    packuswb xmm0, xmm7
    movq [r2], xmm0

    lea r2, [r2+r3]
    lea r0, [r0+r1]
    dec r4
    jnz near .y_loop

    POP_XMM
    LOAD_5_PARA_POP
    ret

;*******************************************************************************
; void McHorVer20WidthEq16_sse2(  const uint8_t *pSrc,
;                       int iSrcStride,
;                                               uint8_t *pDst,
;                                               int iDstStride,
;                                               int iHeight,
;                      );
;*******************************************************************************
WELS_EXTERN McHorVer20WidthEq16_sse2
    %assign  push_num 0
    LOAD_5_PARA
    PUSH_XMM 8
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r4, r4d
    lea r0, [r0-2]            ;pSrc -= 2;

    pxor xmm7, xmm7
    movdqa xmm6, [h264_w0x10_1]
.y_loop:

    movq xmm0, [r0]
    punpcklbw xmm0, xmm7
    movq xmm1, [r0+5]
    punpcklbw xmm1, xmm7
    movq xmm2, [r0+1]
    punpcklbw xmm2, xmm7
    movq xmm3, [r0+4]
    punpcklbw xmm3, xmm7
    movq xmm4, [r0+2]
    punpcklbw xmm4, xmm7
    movq xmm5, [r0+3]
    punpcklbw xmm5, xmm7

    paddw xmm2, xmm3
    paddw xmm4, xmm5
    psllw xmm4, 2
    psubw xmm4, xmm2
    paddw xmm0, xmm1
    paddw xmm0, xmm4
    psllw xmm4, 2
    paddw xmm0, xmm4
    paddw xmm0, xmm6
    psraw xmm0, 5
    packuswb xmm0, xmm7
    movq [r2], xmm0

    movq xmm0, [r0+8]
    punpcklbw xmm0, xmm7
    movq xmm1, [r0+5+8]
    punpcklbw xmm1, xmm7
    movq xmm2, [r0+1+8]
    punpcklbw xmm2, xmm7
    movq xmm3, [r0+4+8]
    punpcklbw xmm3, xmm7
    movq xmm4, [r0+2+8]
    punpcklbw xmm4, xmm7
    movq xmm5, [r0+3+8]
    punpcklbw xmm5, xmm7

    paddw xmm2, xmm3
    paddw xmm4, xmm5
    psllw xmm4, 2
    psubw xmm4, xmm2
    paddw xmm0, xmm1
    paddw xmm0, xmm4
    psllw xmm4, 2
    paddw xmm0, xmm4
    paddw xmm0, xmm6
    psraw xmm0, 5
    packuswb xmm0, xmm7
    movq [r2+8], xmm0

    lea r2, [r2+r3]
    lea r0, [r0+r1]
    dec r4
    jnz near .y_loop

    POP_XMM
    LOAD_5_PARA_POP
    ret


;*******************************************************************************
; void McHorVer02WidthEq8_sse2( const uint8_t *pSrc,
;                       int iSrcStride,
;                       uint8_t *pDst,
;                       int iDstStride,
;                       int iHeight )
;*******************************************************************************
WELS_EXTERN McHorVer02WidthEq8_sse2
    %assign  push_num 0
    LOAD_5_PARA
    PUSH_XMM 8
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r4, r4d
    sub r0, r1
    sub r0, r1

    WELS_Zero xmm7

    SSE_LOAD_8P xmm0, xmm7, [r0]
    SSE_LOAD_8P xmm1, xmm7, [r0+r1]
    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm2, xmm7, [r0]
    SSE_LOAD_8P xmm3, xmm7, [r0+r1]
    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm4, xmm7, [r0]
    SSE_LOAD_8P xmm5, xmm7, [r0+r1]

.start:
    FILTER_HV_W8 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
    dec r4
    jz near .xx_exit

    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm6, xmm7, [r0]
    FILTER_HV_W8 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, [r2+r3]
    dec r4
    jz near .xx_exit

    lea r2, [r2+2*r3]
    SSE_LOAD_8P xmm7, xmm0, [r0+r1]
    FILTER_HV_W8 xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [r2]
    dec r4
    jz near .xx_exit

    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm0, xmm1, [r0]
    FILTER_HV_W8 xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, [r2+r3]
    dec r4
    jz near .xx_exit

    lea r2, [r2+2*r3]
    SSE_LOAD_8P xmm1, xmm2, [r0+r1]
    FILTER_HV_W8 xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, [r2]
    dec r4
    jz near .xx_exit

    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm2, xmm3, [r0]
    FILTER_HV_W8 xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, [r2+r3]
    dec r4
    jz near .xx_exit

    lea r2, [r2+2*r3]
    SSE_LOAD_8P xmm3, xmm4, [r0+r1]
    FILTER_HV_W8 xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, [r2]
    dec r4
    jz near .xx_exit

    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm4, xmm5, [r0]
    FILTER_HV_W8 xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, [r2+r3]
    dec r4
    jz near .xx_exit

    lea r2, [r2+2*r3]
    SSE_LOAD_8P xmm5, xmm6, [r0+r1]
    jmp near .start

.xx_exit:
    POP_XMM
    LOAD_5_PARA_POP
    ret

;***********************************************************************
; Code
;***********************************************************************

SECTION .text



;***********************************************************************
; void McHorVer02Height9Or17_sse2(  const uint8_t *pSrc,
;                       int32_t iSrcStride,
;                       uint8_t *pDst,
;                       int32_t iDstStride,
;                       int32_t iWidth,
;                       int32_t iHeight )
;***********************************************************************
WELS_EXTERN McHorVer02Height9Or17_sse2
    %assign  push_num 0
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r4, r4d
    SIGN_EXTENSION  r5, r5d

%ifndef X86_32
    push r12
    push r13
    push r14
    mov  r12, r0
    mov  r13, r2
    mov  r14, r5
%endif

    shr r4, 3
    sub r0, r1
    sub r0, r1

.xloop:
    WELS_Zero xmm7
    SSE_LOAD_8P xmm0, xmm7, [r0]
    SSE_LOAD_8P xmm1, xmm7, [r0+r1]
    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm2, xmm7, [r0]
    SSE_LOAD_8P xmm3, xmm7, [r0+r1]
    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm4, xmm7, [r0]
    SSE_LOAD_8P xmm5, xmm7, [r0+r1]

    FILTER_HV_W8 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
    dec r5
    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm6, xmm7, [r0]
    movdqa xmm0,xmm1
    movdqa xmm1,xmm2
    movdqa xmm2,xmm3
    movdqa xmm3,xmm4
    movdqa xmm4,xmm5
    movdqa xmm5,xmm6
    add r2, r3
    sub r0, r1

.start:
    FILTER_HV_W8 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm6, xmm7, [r0]
    FILTER_HV_W8 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, [r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    SSE_LOAD_8P xmm7, xmm0, [r0+r1]
    FILTER_HV_W8 xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm0, xmm1, [r0]
    FILTER_HV_W8 xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, [r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    SSE_LOAD_8P xmm1, xmm2, [r0+r1]
    FILTER_HV_W8 xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, [r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm2, xmm3, [r0]
    FILTER_HV_W8 xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, [r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    SSE_LOAD_8P xmm3, xmm4, [r0+r1]
    FILTER_HV_W8 xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, [r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    SSE_LOAD_8P xmm4, xmm5, [r0]
    FILTER_HV_W8 xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, [r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    SSE_LOAD_8P xmm5, xmm6, [r0+r1]
    jmp near .start

.x_loop_dec:
    dec r4
    jz  near .xx_exit
%ifdef X86_32
    mov r0, arg1
    mov r2, arg3
    mov r5, arg6
%else
    mov r0, r12
    mov r2, r13
    mov r5, r14
%endif
    sub r0, r1
    sub r0, r1
    add r0, 8
    add r2, 8
    jmp near .xloop

.xx_exit:
%ifndef X86_32
    pop r14
    pop r13
    pop r12
%endif
    POP_XMM
    LOAD_6_PARA_POP
    ret


;***********************************************************************
; void McHorVer02Height5_sse2(  const uint8_t *pSrc,
;                       int32_t iSrcStride,
;                       uint8_t *pDst,
;                       int32_t iDstStride,
;                       int32_t iWidth,
;                       int32_t iHeight )
;***********************************************************************
WELS_EXTERN McHorVer02Height5_sse2
%assign  push_num 0
LOAD_6_PARA
PUSH_XMM 8
SIGN_EXTENSION  r1, r1d
SIGN_EXTENSION  r3, r3d
SIGN_EXTENSION  r4, r4d
SIGN_EXTENSION  r5, r5d

%ifndef X86_32
push r12
push r13
push r14
mov  r12, r0
mov  r13, r2
mov  r14, r5
%endif

shr r4, 2
sub r0, r1
sub r0, r1

.xloop:
WELS_Zero xmm7
SSE_LOAD_8P xmm0, xmm7, [r0]
SSE_LOAD_8P xmm1, xmm7, [r0+r1]
lea r0, [r0+2*r1]
SSE_LOAD_8P xmm2, xmm7, [r0]
SSE_LOAD_8P xmm3, xmm7, [r0+r1]
lea r0, [r0+2*r1]
SSE_LOAD_8P xmm4, xmm7, [r0]
SSE_LOAD_8P xmm5, xmm7, [r0+r1]

FILTER_HV_W4 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
dec r5
lea r0, [r0+2*r1]
SSE_LOAD_8P xmm6, xmm7, [r0]
movdqa xmm0,xmm1
movdqa xmm1,xmm2
movdqa xmm2,xmm3
movdqa xmm3,xmm4
movdqa xmm4,xmm5
movdqa xmm5,xmm6
add r2, r3
sub r0, r1

.start:
FILTER_HV_W4 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
SSE_LOAD_8P xmm6, xmm7, [r0]
FILTER_HV_W4 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, [r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
SSE_LOAD_8P xmm7, xmm0, [r0+r1]
FILTER_HV_W4 xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
SSE_LOAD_8P xmm0, xmm1, [r0]
FILTER_HV_W4 xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, [r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
SSE_LOAD_8P xmm1, xmm2, [r0+r1]
FILTER_HV_W4 xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, [r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
SSE_LOAD_8P xmm2, xmm3, [r0]
FILTER_HV_W4 xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, [r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
SSE_LOAD_8P xmm3, xmm4, [r0+r1]
FILTER_HV_W4 xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, [r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
SSE_LOAD_8P xmm4, xmm5, [r0]
FILTER_HV_W4 xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, [r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
SSE_LOAD_8P xmm5, xmm6, [r0+r1]
jmp near .start

.x_loop_dec:
dec r4
jz  near .xx_exit
%ifdef X86_32
mov r0, arg1
mov r2, arg3
mov r5, arg6
%else
mov r0, r12
mov r2, r13
mov r5, r14
%endif
sub r0, r1
sub r0, r1
add r0, 4
add r2, 4
jmp near .xloop

.xx_exit:
%ifndef X86_32
pop r14
pop r13
pop r12
%endif
POP_XMM
LOAD_6_PARA_POP
ret


;***********************************************************************
; void McHorVer20Width9Or17_sse2(       const uint8_t *pSrc,
;                       int32_t iSrcStride,
;                       uint8_t *pDst,
;                       int32_t iDstStride,
;                       int32_t iWidth,
;                       int32_t iHeight
;                      );
;***********************************************************************
WELS_EXTERN McHorVer20Width9Or17_sse2
    %assign  push_num 0
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r4, r4d
    SIGN_EXTENSION  r5, r5d
    sub r0, 2
    pxor xmm7, xmm7

    cmp r4, 9
    jne near .width_17

.yloop_width_9:
    movq xmm0, [r0]
    punpcklbw xmm0, xmm7
    movq xmm1, [r0+5]
    punpcklbw xmm1, xmm7
    movq xmm2, [r0+1]
    punpcklbw xmm2, xmm7
    movq xmm3, [r0+4]
    punpcklbw xmm3, xmm7
    movq xmm4, [r0+2]
    punpcklbw xmm4, xmm7
    movq xmm5, [r0+3]
    punpcklbw xmm5, xmm7

    movdqa xmm7, xmm2
    paddw   xmm7, xmm3
    movdqa xmm6, xmm4
    paddw   xmm6, xmm5
    psllw xmm6, 2
    psubw xmm6, xmm7
    paddw xmm0, xmm1
    paddw xmm0, xmm6
    psllw xmm6, 2
    paddw xmm0, xmm6
    paddw xmm0, [h264_w0x10_1]
    psraw  xmm0, 5
    packuswb xmm0, xmm0
    movd [r2], xmm0

    pxor  xmm7, xmm7
    movq xmm0, [r0+6]
    punpcklbw xmm0, xmm7

    paddw xmm4, xmm1
    paddw xmm5, xmm3
    psllw xmm5, 2
    psubw xmm5, xmm4
    paddw xmm2, xmm0
    paddw xmm2, xmm5
    psllw xmm5, 2
    paddw xmm2, xmm5
    paddw xmm2, [h264_w0x10_1]
    psraw  xmm2, 5
    packuswb xmm2, xmm2
    movq [r2+1], xmm2

    add r0, r1
    add r2, r3
    dec r5
    jnz .yloop_width_9
    POP_XMM
    LOAD_6_PARA_POP
    ret


.width_17:
.yloop_width_17:
    movq xmm0, [r0]
    punpcklbw xmm0, xmm7
    movq xmm1, [r0+5]
    punpcklbw xmm1, xmm7
    movq xmm2, [r0+1]
    punpcklbw xmm2, xmm7
    movq xmm3, [r0+4]
    punpcklbw xmm3, xmm7
    movq xmm4, [r0+2]
    punpcklbw xmm4, xmm7
    movq xmm5, [r0+3]
    punpcklbw xmm5, xmm7

    paddw xmm2, xmm3
    paddw xmm4, xmm5
    psllw xmm4, 2
    psubw xmm4, xmm2
    paddw xmm0, xmm1
    paddw xmm0, xmm4
    psllw xmm4, 2
    paddw xmm0, xmm4
    paddw xmm0, [h264_w0x10_1]
    psraw  xmm0, 5
    packuswb xmm0, xmm0
    movq [r2], xmm0

    movq xmm0, [r0+8]
    punpcklbw xmm0, xmm7
    movq xmm1, [r0+5+8]
    punpcklbw xmm1, xmm7
    movq xmm2, [r0+1+8]
    punpcklbw xmm2, xmm7
    movq xmm3, [r0+4+8]
    punpcklbw xmm3, xmm7
    movq xmm4, [r0+2+8]
    punpcklbw xmm4, xmm7
    movq xmm5, [r0+3+8]
    punpcklbw xmm5, xmm7

    movdqa xmm7, xmm2
    paddw   xmm7, xmm3
    movdqa xmm6, xmm4
    paddw   xmm6, xmm5
    psllw xmm6, 2
    psubw xmm6, xmm7
    paddw xmm0, xmm1
    paddw xmm0, xmm6
    psllw xmm6, 2
    paddw xmm0, xmm6
    paddw xmm0, [h264_w0x10_1]
    psraw  xmm0, 5
    packuswb xmm0, xmm0
    movd [r2+8], xmm0


    pxor  xmm7, xmm7
    movq xmm0, [r0+6+8]
    punpcklbw xmm0, xmm7

    paddw xmm4, xmm1
    paddw xmm5, xmm3
    psllw xmm5, 2
    psubw xmm5, xmm4
    paddw xmm2, xmm0
    paddw xmm2, xmm5
    psllw xmm5, 2
    paddw xmm2, xmm5
    paddw xmm2, [h264_w0x10_1]
    psraw  xmm2, 5
    packuswb xmm2, xmm2
    movq [r2+9], xmm2
    add r0, r1
    add r2, r3
    dec r5
    jnz .yloop_width_17
    POP_XMM
    LOAD_6_PARA_POP
    ret


;***********************************************************************
; void McHorVer20Width5_sse2(       const uint8_t *pSrc,
;                       int32_t iSrcStride,
;                       uint8_t *pDst,
;                       int32_t iDstStride,
;                       int32_t iWidth,
;                       int32_t iHeight
;                      );
;***********************************************************************
WELS_EXTERN McHorVer20Width5_sse2
%assign  push_num 0
LOAD_6_PARA
PUSH_XMM 8
SIGN_EXTENSION  r1, r1d
SIGN_EXTENSION  r3, r3d
SIGN_EXTENSION  r4, r4d
SIGN_EXTENSION  r5, r5d
sub r0, 2
pxor xmm7, xmm7

.yloop_width_5:
movq xmm0, [r0]
punpcklbw xmm0, xmm7
movq xmm1, [r0+5]
punpcklbw xmm1, xmm7
movq xmm2, [r0+1]
punpcklbw xmm2, xmm7
movq xmm3, [r0+4]
punpcklbw xmm3, xmm7
movq xmm4, [r0+2]
punpcklbw xmm4, xmm7
movq xmm5, [r0+3]
punpcklbw xmm5, xmm7

movdqa xmm7, xmm2
paddw   xmm7, xmm3
movdqa xmm6, xmm4
paddw   xmm6, xmm5
psllw xmm6, 2
psubw xmm6, xmm7
paddw xmm0, xmm1
paddw xmm0, xmm6
psllw xmm6, 2
paddw xmm0, xmm6
paddw xmm0, [h264_w0x10_1]
psraw  xmm0, 5
packuswb xmm0, xmm0
movd [r2], xmm0

pxor  xmm7, xmm7
movq xmm0, [r0+6]
punpcklbw xmm0, xmm7

paddw xmm4, xmm1
paddw xmm5, xmm3
psllw xmm5, 2
psubw xmm5, xmm4
paddw xmm2, xmm0
paddw xmm2, xmm5
psllw xmm5, 2
paddw xmm2, xmm5
paddw xmm2, [h264_w0x10_1]
psraw  xmm2, 5
packuswb xmm2, xmm2
movd [r2+1], xmm2

add r0, r1
add r2, r3
dec r5
jnz .yloop_width_5
POP_XMM
LOAD_6_PARA_POP
ret


;***********************************************************************
;void McHorVer22HorFirst_sse2
;                           (const uint8_t *pSrc,
;                           int32_t iSrcStride,
;                           uint8_t * pTap,
;                           int32_t iTapStride,
;                           int32_t iWidth,int32_t iHeight);
;***********************************************************************
WELS_EXTERN McHorVer22HorFirst_sse2
    %assign  push_num 0
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r4, r4d
    SIGN_EXTENSION  r5, r5d
    pxor xmm7, xmm7
    sub r0, r1              ;;;;;;;;need more 5 lines.
    sub r0, r1

    cmp r4, 9
    jne near .width_17

.yloop_width_9:
    movq xmm0, [r0]
    punpcklbw xmm0, xmm7
    movq xmm1, [r0+5]
    punpcklbw xmm1, xmm7
    movq xmm2, [r0+1]
    punpcklbw xmm2, xmm7
    movq xmm3, [r0+4]
    punpcklbw xmm3, xmm7
    movq xmm4, [r0+2]
    punpcklbw xmm4, xmm7
    movq xmm5, [r0+3]
    punpcklbw xmm5, xmm7

    movdqa xmm7, xmm2
    paddw   xmm7, xmm3
    movdqa xmm6, xmm4
    paddw   xmm6, xmm5
    psllw xmm6, 2
    psubw xmm6, xmm7
    paddw xmm0, xmm1
    paddw xmm0, xmm6
    psllw xmm6, 2
    paddw xmm0, xmm6
    movd [r2], xmm0

    pxor  xmm7, xmm7
    movq xmm0, [r0+6]
    punpcklbw xmm0, xmm7

    paddw xmm4, xmm1
    paddw xmm5, xmm3
    psllw xmm5, 2
    psubw xmm5, xmm4
    paddw xmm2, xmm0
    paddw xmm2, xmm5
    psllw xmm5, 2
    paddw xmm2, xmm5
    movq [r2+2], xmm2
    movhps [r2+2+8], xmm2

    add r0, r1
    add r2, r3
    dec r5
    jnz .yloop_width_9
    POP_XMM
    LOAD_6_PARA_POP
    ret


.width_17:
.yloop_width_17:
    movq xmm0, [r0]
    punpcklbw xmm0, xmm7
    movq xmm1, [r0+5]
    punpcklbw xmm1, xmm7
    movq xmm2, [r0+1]
    punpcklbw xmm2, xmm7
    movq xmm3, [r0+4]
    punpcklbw xmm3, xmm7
    movq xmm4, [r0+2]
    punpcklbw xmm4, xmm7
    movq xmm5, [r0+3]
    punpcklbw xmm5, xmm7

    paddw xmm2, xmm3
    paddw xmm4, xmm5
    psllw xmm4, 2
    psubw xmm4, xmm2
    paddw xmm0, xmm1
    paddw xmm0, xmm4
    psllw xmm4, 2
    paddw xmm0, xmm4
    movdqa [r2], xmm0

    movq xmm0, [r0+8]
    punpcklbw xmm0, xmm7
    movq xmm1, [r0+5+8]
    punpcklbw xmm1, xmm7
    movq xmm2, [r0+1+8]
    punpcklbw xmm2, xmm7
    movq xmm3, [r0+4+8]
    punpcklbw xmm3, xmm7
    movq xmm4, [r0+2+8]
    punpcklbw xmm4, xmm7
    movq xmm5, [r0+3+8]
    punpcklbw xmm5, xmm7

    movdqa xmm7, xmm2
    paddw   xmm7, xmm3
    movdqa xmm6, xmm4
    paddw   xmm6, xmm5
    psllw xmm6, 2
    psubw xmm6, xmm7
    paddw xmm0, xmm1
    paddw xmm0, xmm6
    psllw xmm6, 2
    paddw xmm0, xmm6
    movd [r2+16], xmm0


    pxor  xmm7, xmm7
    movq xmm0, [r0+6+8]
    punpcklbw xmm0, xmm7

    paddw xmm4, xmm1
    paddw xmm5, xmm3
    psllw xmm5, 2
    psubw xmm5, xmm4
    paddw xmm2, xmm0
    paddw xmm2, xmm5
    psllw xmm5, 2
    paddw xmm2, xmm5
    movq [r2+18], xmm2
    movhps [r2+18+8], xmm2

    add r0, r1
    add r2, r3
    dec r5
    jnz .yloop_width_17
    POP_XMM
    LOAD_6_PARA_POP
    ret


%macro FILTER_VER 9
    paddw  %1, %6
    movdqa %7, %2
    movdqa %8, %3


    paddw %7, %5
    paddw %8, %4

    psubw  %1, %7
    psraw   %1, 2
    paddw  %1, %8
    psubw  %1, %7
    psraw   %1, 2
    paddw  %8, %1
    paddw  %8, [h264_mc_hc_32]
    psraw   %8, 6
    packuswb %8, %8
    movq %9, %8
%endmacro
;***********************************************************************
;void McHorVer22Width8VerLastAlign_sse2(
;                                           const uint8_t *pTap,
;                                           int32_t iTapStride,
;                                           uint8_t * pDst,
;                                           int32_t iDstStride,
;                                           int32_t iWidth,
;                                           int32_t iHeight);
;***********************************************************************

WELS_EXTERN McHorVer22Width8VerLastAlign_sse2
    %assign  push_num 0
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r4, r4d
    SIGN_EXTENSION  r5, r5d
%ifndef X86_32
    push r12
    push r13
    push r14
    mov  r12, r0
    mov  r13, r2
    mov  r14, r5
%endif

    shr r4, 3

.width_loop:
    movdqa xmm0, [r0]
    movdqa xmm1, [r0+r1]
    lea r0, [r0+2*r1]
    movdqa xmm2, [r0]
    movdqa xmm3, [r0+r1]
    lea r0, [r0+2*r1]
    movdqa xmm4, [r0]
    movdqa xmm5, [r0+r1]

    FILTER_VER xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
    dec r5
    lea r0, [r0+2*r1]
    movdqa xmm6, [r0]

    movdqa xmm0, xmm1
    movdqa xmm1, xmm2
    movdqa xmm2, xmm3
    movdqa xmm3, xmm4
    movdqa xmm4, xmm5
    movdqa xmm5, xmm6

    add r2, r3
    sub r0, r1

.start:
    FILTER_VER xmm0,xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    movdqa xmm6, [r0]
    FILTER_VER xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,[r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    movdqa xmm7, [r0+r1]
    FILTER_VER  xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    movdqa xmm0, [r0]
    FILTER_VER  xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,[r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    movdqa xmm1, [r0+r1]
    FILTER_VER  xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,[r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    movdqa xmm2, [r0]
    FILTER_VER  xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,[r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    movdqa xmm3, [r0+r1]
    FILTER_VER  xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,[r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    movdqa xmm4, [r0]
    FILTER_VER  xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,xmm6, [r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    movdqa xmm5, [r0+r1]
    jmp near .start

.x_loop_dec:
    dec r4
    jz near .exit
%ifdef X86_32
    mov r0, arg1
    mov r2, arg3
    mov r5, arg6
%else
    mov r0, r12
    mov r2, r13
    mov r5, r14
%endif
    add r0, 16
    add r2, 8
    jmp .width_loop

.exit:
%ifndef X86_32
    pop r14
    pop r13
    pop r12
%endif
    POP_XMM
    LOAD_6_PARA_POP
    ret

;***********************************************************************
;void McHorVer22Width8VerLastUnAlign_sse2(
;                                           const uint8_t *pTap,
;                                           int32_t iTapStride,
;                                           uint8_t * pDst,
;                                           int32_t iDstStride,
;                                           int32_t iWidth,
;                                           int32_t iHeight);
;***********************************************************************

WELS_EXTERN McHorVer22Width8VerLastUnAlign_sse2
    %assign  push_num 0
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r4, r4d
    SIGN_EXTENSION  r5, r5d
%ifndef X86_32
    push r12
    push r13
    push r14
    mov  r12, r0
    mov  r13, r2
    mov  r14, r5
%endif
    shr r4, 3

.width_loop:
    movdqu xmm0, [r0]
    movdqu xmm1, [r0+r1]
    lea r0, [r0+2*r1]
    movdqu xmm2, [r0]
    movdqu xmm3, [r0+r1]
    lea r0, [r0+2*r1]
    movdqu xmm4, [r0]
    movdqu xmm5, [r0+r1]

    FILTER_VER xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
    dec r5
    lea r0, [r0+2*r1]
    movdqu xmm6, [r0]

    movdqa xmm0, xmm1
    movdqa xmm1, xmm2
    movdqa xmm2, xmm3
    movdqa xmm3, xmm4
    movdqa xmm4, xmm5
    movdqa xmm5, xmm6

    add r2, r3
    sub r0, r1

.start:
    FILTER_VER xmm0,xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    movdqu xmm6, [r0]
    FILTER_VER xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,[r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    movdqu xmm7, [r0+r1]
    FILTER_VER  xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    movdqu xmm0, [r0]
    FILTER_VER  xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,[r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    movdqu xmm1, [r0+r1]
    FILTER_VER  xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,[r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    movdqu xmm2, [r0]
    FILTER_VER  xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,[r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    movdqu xmm3, [r0+r1]
    FILTER_VER  xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,[r2]
    dec r5
    jz near .x_loop_dec

    lea r0, [r0+2*r1]
    movdqu xmm4, [r0]
    FILTER_VER  xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,xmm6, [r2+r3]
    dec r5
    jz near .x_loop_dec

    lea r2, [r2+2*r3]
    movdqu xmm5, [r0+r1]
    jmp near .start

.x_loop_dec:
    dec r4
    jz near .exit
%ifdef X86_32
    mov r0, arg1
    mov r2, arg3
    mov r5, arg6
%else
    mov r0, r12
    mov r2, r13
    mov r5, r14
%endif
    add r0, 16
    add r2, 8
    jmp .width_loop

.exit:
%ifndef X86_32
    pop r14
    pop r13
    pop r12
%endif
    POP_XMM
    LOAD_6_PARA_POP
    ret


;***********************************************************************
;void McHorVer22Width5HorFirst_sse2
;                           (const uint8_t *pSrc,
;                           int32_t iSrcStride,
;                           uint8_t * pTap,
;                           int32_t iTapStride,
;                           int32_t iWidth,int32_t iHeight);
;***********************************************************************
WELS_EXTERN McHorVer22Width5HorFirst_sse2
%assign  push_num 0
LOAD_6_PARA
PUSH_XMM 8
SIGN_EXTENSION  r1, r1d
SIGN_EXTENSION  r3, r3d
SIGN_EXTENSION  r4, r4d
SIGN_EXTENSION  r5, r5d
pxor xmm7, xmm7
sub r0, r1              ;;;;;;;;need more 5 lines.
sub r0, r1

.yloop_width_5:
movq xmm0, [r0]
punpcklbw xmm0, xmm7
movq xmm1, [r0+5]
punpcklbw xmm1, xmm7
movq xmm2, [r0+1]
punpcklbw xmm2, xmm7
movq xmm3, [r0+4]
punpcklbw xmm3, xmm7
movq xmm4, [r0+2]
punpcklbw xmm4, xmm7
movq xmm5, [r0+3]
punpcklbw xmm5, xmm7

movdqa xmm7, xmm2
paddw   xmm7, xmm3
movdqa xmm6, xmm4
paddw   xmm6, xmm5
psllw xmm6, 2
psubw xmm6, xmm7
paddw xmm0, xmm1
paddw xmm0, xmm6
psllw xmm6, 2
paddw xmm0, xmm6
movd [r2], xmm0

pxor  xmm7, xmm7
movq xmm0, [r0+6]
punpcklbw xmm0, xmm7

paddw xmm4, xmm1
paddw xmm5, xmm3
psllw xmm5, 2
psubw xmm5, xmm4
paddw xmm2, xmm0
paddw xmm2, xmm5
psllw xmm5, 2
paddw xmm2, xmm5
movq [r2+2], xmm2
movhps [r2+2+8], xmm2

add r0, r1
add r2, r3
dec r5
jnz .yloop_width_5
POP_XMM
LOAD_6_PARA_POP
ret


%macro FILTER_VER_4 9
paddw  %1, %6
movdqa %7, %2
movdqa %8, %3


paddw %7, %5
paddw %8, %4

psubw  %1, %7
psraw   %1, 2
paddw  %1, %8
psubw  %1, %7
psraw   %1, 2
paddw  %8, %1
paddw  %8, [h264_mc_hc_32]
psraw   %8, 6
packuswb %8, %8
movd %9, %8
%endmacro


;***********************************************************************
;void McHorVer22Width4VerLastAlign_sse2(
;                                           const uint8_t *pTap,
;                                           int32_t iTapStride,
;                                           uint8_t * pDst,
;                                           int32_t iDstStride,
;                                           int32_t iWidth,
;                                           int32_t iHeight);
;***********************************************************************

WELS_EXTERN McHorVer22Width4VerLastAlign_sse2
%assign  push_num 0
LOAD_6_PARA
PUSH_XMM 8
SIGN_EXTENSION  r1, r1d
SIGN_EXTENSION  r3, r3d
SIGN_EXTENSION  r4, r4d
SIGN_EXTENSION  r5, r5d
%ifndef X86_32
push r12
push r13
push r14
mov  r12, r0
mov  r13, r2
mov  r14, r5
%endif

shr r4, 2

.width_loop:
movdqa xmm0, [r0]
movdqa xmm1, [r0+r1]
lea r0, [r0+2*r1]
movdqa xmm2, [r0]
movdqa xmm3, [r0+r1]
lea r0, [r0+2*r1]
movdqa xmm4, [r0]
movdqa xmm5, [r0+r1]

FILTER_VER_4 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
dec r5
lea r0, [r0+2*r1]
movdqa xmm6, [r0]

movdqa xmm0, xmm1
movdqa xmm1, xmm2
movdqa xmm2, xmm3
movdqa xmm3, xmm4
movdqa xmm4, xmm5
movdqa xmm5, xmm6

add r2, r3
sub r0, r1

.start:
FILTER_VER_4 xmm0,xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
movdqa xmm6, [r0]
FILTER_VER_4 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,[r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
movdqa xmm7, [r0+r1]
FILTER_VER_4  xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
movdqa xmm0, [r0]
FILTER_VER_4  xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,[r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
movdqa xmm1, [r0+r1]
FILTER_VER_4  xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,[r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
movdqa xmm2, [r0]
FILTER_VER_4  xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,[r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
movdqa xmm3, [r0+r1]
FILTER_VER_4  xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,[r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
movdqa xmm4, [r0]
FILTER_VER_4  xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,xmm6, [r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
movdqa xmm5, [r0+r1]
jmp near .start

.x_loop_dec:
dec r4
jz near .exit
%ifdef X86_32
mov r0, arg1
mov r2, arg3
mov r5, arg6
%else
mov r0, r12
mov r2, r13
mov r5, r14
%endif
add r0, 8
add r2, 4
jmp .width_loop

.exit:
%ifndef X86_32
pop r14
pop r13
pop r12
%endif
POP_XMM
LOAD_6_PARA_POP
ret


;***********************************************************************
;void McHorVer22Width4VerLastUnAlign_sse2(
;                                           const uint8_t *pTap,
;                                           int32_t iTapStride,
;                                           uint8_t * pDst,
;                                           int32_t iDstStride,
;                                           int32_t iWidth,
;                                           int32_t iHeight);
;***********************************************************************

WELS_EXTERN McHorVer22Width4VerLastUnAlign_sse2
%assign  push_num 0
LOAD_6_PARA
PUSH_XMM 8
SIGN_EXTENSION  r1, r1d
SIGN_EXTENSION  r3, r3d
SIGN_EXTENSION  r4, r4d
SIGN_EXTENSION  r5, r5d
%ifndef X86_32
push r12
push r13
push r14
mov  r12, r0
mov  r13, r2
mov  r14, r5
%endif
shr r4, 2

.width_loop:
movdqu xmm0, [r0]
movdqu xmm1, [r0+r1]
lea r0, [r0+2*r1]
movdqu xmm2, [r0]
movdqu xmm3, [r0+r1]
lea r0, [r0+2*r1]
movdqu xmm4, [r0]
movdqu xmm5, [r0+r1]

FILTER_VER_4 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
dec r5
lea r0, [r0+2*r1]
movdqu xmm6, [r0]

movdqa xmm0, xmm1
movdqa xmm1, xmm2
movdqa xmm2, xmm3
movdqa xmm3, xmm4
movdqa xmm4, xmm5
movdqa xmm5, xmm6

add r2, r3
sub r0, r1

.start:
FILTER_VER_4 xmm0,xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
movdqu xmm6, [r0]
FILTER_VER_4 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,[r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
movdqu xmm7, [r0+r1]
FILTER_VER_4  xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
movdqu xmm0, [r0]
FILTER_VER_4  xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,[r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
movdqu xmm1, [r0+r1]
FILTER_VER_4  xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,[r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
movdqu xmm2, [r0]
FILTER_VER_4  xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,[r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
movdqu xmm3, [r0+r1]
FILTER_VER_4  xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,[r2]
dec r5
jz near .x_loop_dec

lea r0, [r0+2*r1]
movdqu xmm4, [r0]
FILTER_VER_4  xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,xmm6, [r2+r3]
dec r5
jz near .x_loop_dec

lea r2, [r2+2*r3]
movdqu xmm5, [r0+r1]
jmp near .start

.x_loop_dec:
dec r4
jz near .exit
%ifdef X86_32
mov r0, arg1
mov r2, arg3
mov r5, arg6
%else
mov r0, r12
mov r2, r13
mov r5, r14
%endif
add r0, 8
add r2, 4
jmp .width_loop

.exit:
%ifndef X86_32
pop r14
pop r13
pop r12
%endif
POP_XMM
LOAD_6_PARA_POP
ret

