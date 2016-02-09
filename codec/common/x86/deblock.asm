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
;*  deblock.asm
;*
;*  Abstract
;*      edge loop
;*
;*  History
;*      08/07/2009 Created
;*
;*
;*************************************************************************/
%include "asm_inc.asm"

;*******************************************************************************
; Macros and other preprocessor constants
;*******************************************************************************

SECTION .rodata align=16

ALIGN   16
FOUR_16B_SSE2:   dw   4, 4, 4, 4, 4, 4, 4, 4

ALIGN   16
WELS_DB1_16:
    times 16 db 1
WELS_DB127_16:
    times 16 db 127
WELS_DB96_16:
    times 16 db 96
WELS_SHUFB0000111122223333:
    times 4 db 0
    times 4 db 1
    times 4 db 2
    times 4 db 3


SECTION .text

; Unsigned byte absolute difference.
; a=%1 b=%2 clobber=%3
; Subtract once in each direction with saturation and return the maximum.
%macro SSE2_AbsDiffUB 3
    movdqa   %3, %2
    psubusb  %3, %1
    psubusb  %1, %2
    por      %1, %3
%endmacro

; Unsigned byte compare less than.
; lhs=%1 rhs^0x7f=%2 0x7f=%3
; No unsigned byte lt/gt compare instruction available; xor by 0x7f and use a
; signed compare. Some other options do exist. This one allows modifying the lhs
; without mov and uses a bitwise op which can be executed on most ports on
; common architectures.
%macro SSE2_CmpltUB 3
    pxor     %1, %3
    pcmpgtb  %1, %2
%endmacro

; Unsigned byte compare greater than or equal.
%macro SSE2_CmpgeUB 2
    pminub   %1, %2
    pcmpeqb  %1, %2
%endmacro

; Clip unsigned bytes to ref +/- diff.
; data=%1 ref=%2 maxdiff_from_ref=%3 clobber=%4
%macro SSE2_ClipUB 4
    movdqa   %4, %2
    psubusb  %4, %3
    paddusb  %3, %2
    pmaxub   %1, %4
    pminub   %1, %3
%endmacro

; (a + b + 1 - c) >> 1
; a=%1 b=%2 c=%3 [out:a^b&c]=%4
%macro SSE2_AvgbFloor1 4
    movdqa   %4, %1
    pxor     %4, %2
    pavgb    %1, %2
    pand     %4, %3
    psubb    %1, %4
%endmacro

; (a + b + carry) >> 1
; a=%1 b=%2 carry-1=%3
%macro SSE2_AvgbFloor2 3
    pxor     %1, %3
    pxor     %2, %3
    pavgb    %1, %2
    pxor     %1, %3
%endmacro

; a = (a & m) | (b & ~m)
; a=%1 b=%2 m=%3
%macro SSE2_Blend 3
    pand     %1, %3
    pandn    %3, %2
    por      %1, %3
%endmacro


;*******************************************************************************
;    void DeblockLumaLt4V_ssse3(uint8_t * pPix, int32_t iStride, int32_t iAlpha,
;                                 int32_t iBeta, int8_t * pTC)
;*******************************************************************************

WELS_EXTERN DeblockLumaLt4V_ssse3
    %assign push_num 0
    LOAD_5_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    movd     xmm1, arg3d
    movd     xmm2, arg4d
    pxor     xmm3, xmm3
    pxor     xmm1, [WELS_DB127_16]
    pxor     xmm2, [WELS_DB127_16]
    pshufb   xmm1, xmm3                       ; iAlpha ^ 0x7f
    pshufb   xmm2, xmm3                       ; iBeta  ^ 0x7f
    mov      r2, r1                           ; iStride
    neg      r1                               ; -iStride
    lea      r3, [r0 + r1]                    ; pPix - iStride

    ; Compute masks to enable/disable deblocking.
    MOVDQ    xmm6, [r3 + 0 * r1]              ; p0
    MOVDQ    xmm7, [r3 + 1 * r1]              ; p1
    MOVDQ    xmm0, [r0 + 0 * r2]              ; q0
    movdqa   xmm4, xmm6
    SSE2_AbsDiffUB xmm6, xmm0, xmm3           ; |p0 - q0|
    SSE2_CmpltUB xmm6, xmm1, [WELS_DB127_16]  ; bDeltaP0Q0 = |p0 - q0| < iAlpha
    MOVDQ    xmm1, [r0 + 1 * r2]              ; q1
    SSE2_AbsDiffUB xmm7, xmm4, xmm3           ; |p1 - p0|
    SSE2_AbsDiffUB xmm0, xmm1, xmm3           ; |q1 - q0|
    pmaxub   xmm7, xmm0                       ; max(|p1 - p0|, |q1 - q0|)
    SSE2_CmpltUB xmm7, xmm2, [WELS_DB127_16]  ; bDeltaP1P0 & bDeltaQ1Q0 = max(|p1 - p0|, |q1 - q0|) < iBeta
    pand     xmm6, xmm7                       ; bDeltaP0Q0P1P0Q1Q0 = bDeltaP0Q0 & bDeltaP1P0 & bDeltaQ1Q0
    MOVDQ    xmm7, [r3 + 2 * r1]              ; p2
    movdqa   xmm0, xmm7
    SSE2_AbsDiffUB xmm7, xmm4, xmm3           ; |p2 - p0|
    SSE2_CmpltUB xmm7, xmm2, [WELS_DB127_16]  ; bDeltaP2P0 = |p2 - p0| < iBeta
    MOVDQ    xmm5, [r0 + 2 * r2]              ; q2
    MOVDQ    xmm3, [r0 + 0 * r2]              ; q0
    movdqa   xmm1, xmm5
    SSE2_AbsDiffUB xmm5, xmm3, xmm4           ; |q2 - q0|
    SSE2_CmpltUB xmm5, xmm2, [WELS_DB127_16]  ; bDeltaQ2Q0 = |q2 - q0| < iBeta

    pavgb    xmm3, [r3 + 0 * r1]
    pcmpeqw  xmm2, xmm2  ; FFh
    pxor     xmm3, xmm2
    ; (p2 + ((p0 + q0 + 1) >> 1)) >> 1
    pxor     xmm0, xmm2
    pavgb    xmm0, xmm3
    pxor     xmm0, xmm2
    ; (q2 + ((p0 + q0 + 1) >> 1)) >> 1
    pxor     xmm1, xmm2
    pavgb    xmm1, xmm3
    pxor     xmm1, xmm2

    movd     xmm3, [r4]
    pshufb   xmm3, [WELS_SHUFB0000111122223333] ; iTc
    movdqa   xmm4, xmm3  ; iTc0 = iTc
    pcmpgtb  xmm3, xmm2  ; iTc > -1 ? 0xff : 0x00
    pand     xmm6, xmm3  ; bDeltaP0Q0P1P0Q1Q0 &= iTc > -1
    movdqa   xmm3, xmm4
    psubb    xmm3, xmm7  ; iTc -= bDeltaP2P0 ? -1 : 0
    psubb    xmm3, xmm5  ; iTc -= bDeltaQ2Q0 ? -1 : 0
    pand     xmm3, xmm6  ; iTc &= bDeltaP0Q0P1P0Q1Q0 ? 0xff : 0
    pand     xmm7, xmm6  ; bDeltaP2P0 &= bDeltaP0Q0P1P0Q1Q0
    pand     xmm5, xmm6  ; bDeltaQ2Q0 &= bDeltaP0Q0P1P0Q1Q0
    pand     xmm7, xmm4  ; iTc0 & (bDeltaP2P0 ? 0xff : 0)
    pand     xmm5, xmm4  ; iTc0 & (bDeltaQ2Q0 ? 0xff : 0)

    MOVDQ    xmm4, [r3 + 1 * r1]
    SSE2_ClipUB xmm0, xmm4, xmm7, xmm6  ; clip p1.
    MOVDQ    xmm6, [r0 + 1 * r2]
    MOVDQ    [r3 + 1 * r1], xmm0        ; store p1.
    SSE2_ClipUB xmm1, xmm6, xmm5, xmm7  ; clip q1.
    MOVDQ    [r0 + 1 * r2], xmm1        ; store q1.

    ; (q0 - p0 + ((p1 - q1) >> 2) + 1) >> 1 clipped to [-96, 159] and biased to [0, 255].
    ; A limited range is sufficient because the value is clipped to [-iTc, iTc] later.
    ; Bias so that unsigned saturation can be used.
    ; Get ((p1 - q1) >> 2) + 192 via a pxor and two pavgbs.
    ; q0 - p0 is split into a non-negative and non-positive part. The latter is
    ; subtracted from the biased value.
    MOVDQ    xmm1, [r3 + 0 * r1] ; p0
    MOVDQ    xmm0, [r0 + 0 * r2] ; q0
    movdqa   xmm7, xmm1
    psubusb  xmm7, xmm0  ; clip(p0 - q0, 0, 255)
    ; ((p1 - q1) >> 2) + 0xc0
    pxor     xmm6, xmm2  ; q1 ^ 0xff aka -q1 - 1 & 0xff
    pavgb    xmm4, xmm6  ; (((p1 - q1 + 0x100) >> 1)
    pavgb    xmm4, xmm2  ;  + 0x100) >> 1
    psubusb  xmm4, xmm7  ; -= clip(p0 - q0, 0, 255) saturate.
    psubusb  xmm0, xmm1  ; (clip(q0 - p0, 0, 255)
    pavgb    xmm0, xmm4  ;  + clip(((p1 - q1 + 0x300) >> 2) - clip(p0 - q0, 0, 255), 0, 255) + 1) >> 1

    ; Unbias and split into a non-negative and a non-positive part.
    ; Clip each part to iTc via minub.
    ; Add/subtract each part to/from p0/q0 and clip.
    movdqa   xmm6, [WELS_DB96_16]
    psubusb  xmm6, xmm0
    psubusb  xmm0, [WELS_DB96_16]
    pminub   xmm6, xmm3
    pminub   xmm0, xmm3
    psubusb  xmm1, xmm6
    paddusb  xmm1, xmm0
    paddusb  xmm6, [r0 + 0 * r2]
    psubusb  xmm6, xmm0
    MOVDQ    [r3 + 0 * r1], xmm1  ; store p0.
    MOVDQ    [r0 + 0 * r2], xmm6  ; store q0.

    POP_XMM
    LOAD_5_PARA_POP
    ret


; Deblock 3x16 luma pixels for the eq4 case.
;
; Compose 8-bit averages from pavgbs. Ie. (p1 + p0 + p2 + q0 + 2) >> 2 can be
; written as (((p1 + p0) >> 1) + ((p2 + q0 + (p1 ^ p0 & 1)) >> 1) + 1) >> 1,
; which maps to 3 pavgbs.
;
; pPix=%1 iStride=%2 [in:q0,out:p0]=%3 [in:q1,out:p1]=%4 bDeltaP0Q0P1P0Q1Q0=%5 bDeltaP2P0=%6 clobber=%7,%8,%9,%10 preserve_p0p1=%11 db1=%12
%macro SSE2_DeblockLumaEq4_3x16P 12
    movdqa   %7, %3
    movdqa   %8, %6
    MOVDQ    %10, [%1 + 1 * %2]                      ; p1
    SSE2_Blend %7, %10, %8                           ; t0 = bDeltaP2P0 ? q0 : p1
    movdqa   %8, %6
    MOVDQ    %9, [%1 + 2 * %2]                       ; p2
    SSE2_Blend %9, %4, %8                            ; t1 = bDeltaP2P0 ? p2 : q1
    SSE2_AvgbFloor1 %4,  %9,   %12, %8               ; t1 = (t1 + q1) >> 1
    SSE2_AvgbFloor1 %10, [%1], %12, %8               ; (p0 + p1) >> 1, p0 ^ p1
    pxor     %8, %12
    SSE2_AvgbFloor1 %7, %4, %8, %9                   ; (t0 + t1 + (p0 ^ p1 & 1)) >> 1
    MOVDQ    %9, [%1 + 2 * %2]                       ; p2
    SSE2_AvgbFloor1 %3, %9, %8, %4                   ; (p2 + q0 + (p0 ^ p1 & 1)) >> 1
    pavgb    %7, %10                                 ; p0' = (p0 + p1 + t0 + t1 + 2) >> 2
    movdqa   %8, %10
    pxor     %8, %3                                  ; (p0 + p1) >> 1 ^ (p2 + q0 + (p0 ^ p1 & 1)) >> 1
    pand     %8, %12                                 ; & 1
    pavgb    %10, %3                                 ; p1' = (p0 + p1 + p2 + q0 + 2) >> 2
    pand     %6, %5                                  ; bDeltaP2P0 &= bDeltaP0Q0P1P0Q1Q0
%if %11
    MOVDQ    %3, [%1 + 0 * %2]                       ; p0
    movdqa   %4, %5
    SSE2_Blend %7, %3, %4                            ; p0out = bDeltaP0Q0P1P0Q1Q0 ? p0' : p0
%else
    SSE2_Blend %7, [%1 + 0 * %2], %5                 ; p0out = bDeltaP0Q0P1P0Q1Q0 ? p0' : p0
%endif
    MOVDQ    [%1 + 0 * %2], %7                       ; store p0
    add      %1, %2
    movdqa   %7, %10
    psubb    %10, %8                                 ; (p0 + p1 + p2 + q0) >> 2
    psubb    %8, %12
    MOVDQ    %4, [%1 + (3 - 1) * %2]                 ; p3
    SSE2_AvgbFloor2 %4, %9, %8                       ; (p2 + p3 + ((p0 + p1) >> 1 ^ (p2 + q0 + (p0 ^ p1 & 1)) >> 1 & 1)) >> 1
    pavgb    %10, %4                                 ; p2' = (((p0 + p1 + p2 + q0) >> 1) + p2 + p3 + 2) >> 2
    movdqa   %8, %6
    SSE2_Blend %10, [%1 + (2 - 1) * %2], %8          ; p2out = bDeltaP2P0 ? p2' : p2
    MOVDQ    [%1 + (2 - 1) * %2], %10                ; store p2
%if %11
    MOVDQ    %4, [%1 + (1 - 1) * %2]                 ; p1
    SSE2_Blend %7, %4, %6                            ; p1out = bDeltaP2P0 ? p1' : p1
%else
    SSE2_Blend %7, [%1 + (1 - 1) * %2], %6           ; p1out = bDeltaP2P0 ? p1' : p1
%endif
    MOVDQ    [%1 + (1 - 1) * %2], %7                 ; store p1
%endmacro


;*******************************************************************************
;    void DeblockLumaEq4V_ssse3(uint8_t * pPix, int32_t iStride, int32_t iAlpha,
;                                 int32_t iBeta)
;*******************************************************************************

WELS_EXTERN DeblockLumaEq4V_ssse3
    %assign push_num 0
    LOAD_4_PARA
    PUSH_XMM 10
    SIGN_EXTENSION r1, r1d
    movd     xmm1, arg3d
    movd     xmm2, arg4d
    shr      r2, 2
    add      r2, 1
    movd     xmm3, r2d
    pxor     xmm4, xmm4
    pxor     xmm1, [WELS_DB127_16]
    pxor     xmm2, [WELS_DB127_16]
    pshufb   xmm1, xmm4                       ; iAlpha ^ 0x7f
    pshufb   xmm2, xmm4                       ; iBeta  ^ 0x7f
    pshufb   xmm3, xmm4                       ; (iAlpha >> 2) + 1
    mov      r2, r1                           ; iStride
    neg      r1                               ; -iStride
    lea      r3, [r0 + r1]                    ; pPix - iStride

    ; Compute masks to enable/disable filtering.
    MOVDQ    xmm7, [r3 + 1 * r1]              ; p1
    MOVDQ    xmm6, [r3 + 0 * r1]              ; p0
    MOVDQ    xmm0, [r0 + 0 * r2]              ; q0
    movdqa   xmm4, xmm6
    SSE2_AbsDiffUB xmm6, xmm0, xmm5           ; |p0 - q0|
    SSE2_CmpgeUB xmm3, xmm6                   ; |p0 - q0| < (iAlpha >> 2) + 2
    SSE2_CmpltUB xmm6, xmm1, [WELS_DB127_16]  ; bDeltaP0Q0 = |p0 - q0| < iAlpha
    MOVDQ    xmm1, [r0 + 1 * r2]              ; q1
    SSE2_AbsDiffUB xmm7, xmm4, xmm5           ; |p1 - p0|
    SSE2_AbsDiffUB xmm0, xmm1, xmm5           ; |q1 - q0|
    pmaxub   xmm7, xmm0                       ; max(|p1 - p0|, |q1 - q0|)
    SSE2_CmpltUB xmm7, xmm2, [WELS_DB127_16]  ; bDeltaP1P0 & bDeltaQ1Q0 = max(|p1 - p0|, |q1 - q0|) < iBeta
    pand     xmm6, xmm7                       ; & bDeltaP0Q0

    MOVDQ    xmm7, [r3 + 2 * r1]              ; p2
    SSE2_AbsDiffUB xmm7, xmm4, xmm5           ; |p2 - p0|
    SSE2_CmpltUB xmm7, xmm2, [WELS_DB127_16]  ; bDeltaP2P0 = |p2 - p0| < iBeta
    pand     xmm7, xmm3                       ; &= |p0 - q0| < (iAlpha >> 2) + 2

    MOVDQ    xmm0, [r0 + 0 * r2]              ; q0
    MOVDQ    xmm5, [r0 + 2 * r2]              ; q2
    SSE2_AbsDiffUB xmm5, xmm0, xmm4           ; |q2 - q0|
    SSE2_CmpltUB xmm5, xmm2, [WELS_DB127_16]  ; bDeltaQ2Q0 = |q2 - q0| < iBeta
    pand     xmm5, xmm3                       ; &= |p0 - q0| < (iAlpha >> 2) + 2

%ifdef X86_32
    ; Push xmm5 to free up one register. Align stack so as to ensure that failed
    ; store forwarding penalty cannot occur (up to ~50 cycles for 128-bit on IVB).
    mov      r2, esp
    sub      esp,  16
    and      esp, -16
    movdqa   [esp], xmm5
    SSE2_DeblockLumaEq4_3x16P r3, r1, xmm0, xmm1, xmm6, xmm7, xmm2, xmm3, xmm5, xmm4, 1, [WELS_DB1_16]
    movdqa   xmm5, [esp]
    mov      esp, r2
    neg      r1
    SSE2_DeblockLumaEq4_3x16P r0, r1, xmm0, xmm1, xmm6, xmm5, xmm2, xmm3, xmm7, xmm4, 0, [WELS_DB1_16]
%else
    movdqa   xmm9, [WELS_DB1_16]
    SSE2_DeblockLumaEq4_3x16P r3, r1, xmm0, xmm1, xmm6, xmm7, xmm2, xmm3, xmm8, xmm4, 1, xmm9
    SSE2_DeblockLumaEq4_3x16P r0, r2, xmm0, xmm1, xmm6, xmm5, xmm2, xmm3, xmm7, xmm4, 0, xmm9
%endif

    POP_XMM
    LOAD_4_PARA_POP
    ret


%ifdef  WIN64


WELS_EXTERN DeblockChromaLt4V_ssse3
    mov         rax,rsp
    push        rbx
    push        rdi
    PUSH_XMM 16
    sub         rsp,0C8h
    mov         r10,qword [rax + 30h]  ; pTC
    pxor        xmm1,xmm1
    mov         rbx,rcx
    movsxd      r11,r8d
    movsx       ecx,byte [r10]
    movsx       r8d,byte [r10+2]
    mov         rdi,rdx
    movq        xmm2,[rbx]
    movq        xmm9,[r11+rbx]
    movsx       edx,byte [r10+1]
    mov         word [rsp+2],cx
    mov         word [rsp],cx
    movsx       eax,byte [r10+3]
    mov         word [rsp+6],dx
    mov         word [rsp+4],dx
    movdqa      xmm11,xmm1
    mov         word [rsp+0Eh],ax
    mov         word [rsp+0Ch],ax
    lea         eax,[r11+r11]
    movsxd      rcx,eax
    mov         rax,rbx
    mov         rdx,rdi
    sub         rax,rcx
    mov         word [rsp+0Ah],r8w
    mov         word [rsp+8],r8w
    movdqa      xmm6,[rsp]
    movdqa      xmm7,xmm6
    movq        xmm13, [rax]
    mov         rax,rdi
    sub         rax,rcx
    mov         rcx,rbx
    pcmpgtw     xmm7,xmm1
    psubw       xmm11,xmm6
    sub         rcx,r11
    sub         rdx,r11
    movq        xmm0,[rax]
    movsx       eax,r9w
    movq        xmm15,[rcx]
    punpcklqdq  xmm13,xmm0
    movq        xmm0, [rdx]
    movdqa      xmm4,xmm13
    punpcklqdq  xmm15,xmm0
    movq        xmm0, [rdi]
    punpcklbw   xmm4,xmm1
    movdqa      xmm12,xmm15
    punpcklqdq  xmm2,xmm0
    movq        xmm0, [r11+rdi]
    punpcklbw   xmm12,xmm1
    movdqa      xmm14,xmm2
    punpcklqdq  xmm9,xmm0
    punpckhbw   xmm2,xmm1
    punpcklbw   xmm14,xmm1
    movd        xmm0,eax
    movsx       eax,word [rsp + 0C8h + 38h + 160] ; iBeta
    punpckhbw   xmm13,xmm1
    punpckhbw   xmm15,xmm1
    movdqa      xmm3,xmm9
    movdqa      [rsp+10h],xmm2
    punpcklwd   xmm0,xmm0
    punpckhbw   xmm9,xmm1
    punpcklbw   xmm3,xmm1
    movdqa      xmm1,xmm14
    pshufd      xmm10,xmm0,0
    movd        xmm0,eax
    mov         eax,4
    cwde
    punpcklwd   xmm0,xmm0
    pshufd      xmm8,xmm0,0
    movd        xmm0,eax
    punpcklwd   xmm0,xmm0
    pshufd      xmm5,xmm0,0
    psubw       xmm1,xmm12
    movdqa      xmm2,xmm10
    lea         r11,[rsp+0C8h]
    psllw       xmm1,2
    movdqa      xmm0,xmm4
    psubw       xmm4,xmm12
    psubw       xmm0,xmm3
    psubw       xmm3,xmm14
    paddw       xmm1,xmm0
    paddw       xmm1,xmm5
    movdqa      xmm0,xmm11
    psraw       xmm1,3
    pmaxsw      xmm0,xmm1
    pminsw      xmm6,xmm0
    movdqa      xmm1,xmm8
    movdqa      xmm0,xmm12
    psubw       xmm0,xmm14
    pabsw       xmm0,xmm0
    pcmpgtw     xmm2,xmm0
    pabsw       xmm0,xmm4
    pcmpgtw     xmm1,xmm0
    pabsw       xmm0,xmm3
    movdqa      xmm3,[rsp]
    pand        xmm2,xmm1
    movdqa      xmm1,xmm8
    pcmpgtw     xmm1,xmm0
    movdqa      xmm0,xmm13
    pand        xmm2,xmm1
    psubw       xmm0,xmm9
    psubw       xmm13,xmm15
    pand        xmm2,xmm7
    pand        xmm6,xmm2
    paddw       xmm12,xmm6
    psubw       xmm14,xmm6
    movdqa      xmm2,[rsp+10h]
    movaps      xmm6,[r11-18h]
    movdqa      xmm1,xmm2
    psubw       xmm1,xmm15
    psubw       xmm9,xmm2
    psllw       xmm1,2
    paddw       xmm1,xmm0
    paddw       xmm1,xmm5
    movdqa      xmm0,xmm15
    psubw       xmm0,xmm2
    psraw       xmm1,3
    pmaxsw      xmm11,xmm1
    pabsw       xmm0,xmm0
    movdqa      xmm1,xmm8
    pcmpgtw     xmm10,xmm0
    pabsw       xmm0,xmm13
    pminsw      xmm3,xmm11
    movaps      xmm11,[r11-68h]
    movaps      xmm13,[rsp+40h]
    pcmpgtw     xmm1,xmm0
    pabsw       xmm0,xmm9
    movaps      xmm9, [r11-48h]
    pand        xmm10,xmm1
    pcmpgtw     xmm8,xmm0
    pand        xmm10,xmm8
    pand        xmm10,xmm7
    movaps      xmm8,[r11-38h]
    movaps      xmm7,[r11-28h]
    pand        xmm3,xmm10
    paddw       xmm15,xmm3
    psubw       xmm2,xmm3
    movaps      xmm10,[r11-58h]
    packuswb    xmm12,xmm15
    movaps      xmm15,[rsp+20h]
    packuswb    xmm14,xmm2
    movq        [rcx],xmm12
    movq        [rbx],xmm14
    psrldq      xmm12,8
    psrldq      xmm14,8
    movq        [rdx],xmm12
    movaps      xmm12,[r11-78h]
    movq        [rdi],xmm14
    movaps      xmm14,[rsp+30h]
    mov         rsp,r11
    POP_XMM
    pop         rdi
    pop         rbx
    ret


WELS_EXTERN DeblockChromaEq4V_ssse3
    mov         rax,rsp
    push        rbx
    PUSH_XMM 15
    sub         rsp,90h
    pxor        xmm1,xmm1
    mov         r11,rcx
    mov         rbx,rdx
    mov         r10d,r9d
    movq        xmm13,[r11]
    lea         eax,[r8+r8]
    movsxd      r9,eax
    mov         rax,rcx
    sub         rax,r9
    movq        xmm14,[rax]
    mov         rax,rdx
    sub         rax,r9
    movq        xmm0,[rax]
    movsxd      rax,r8d
    sub         rcx,rax
    sub         rdx,rax
    movq        xmm12,[rax+r11]
    movq        xmm10,[rcx]
    punpcklqdq  xmm14,xmm0
    movdqa      xmm8,xmm14
    movq        xmm0,[rdx]
    punpcklbw   xmm8,xmm1
    punpckhbw   xmm14,xmm1
    punpcklqdq  xmm10,xmm0
    movq        xmm0,[rbx]
    movdqa      xmm5,xmm10
    punpcklqdq  xmm13,xmm0
    movq        xmm0, [rax+rbx]
    punpcklbw   xmm5,xmm1
    movsx       eax,r10w
    movdqa      xmm9,xmm13
    punpcklqdq  xmm12,xmm0
    punpcklbw   xmm9,xmm1
    punpckhbw   xmm10,xmm1
    movd        xmm0,eax
    movsx       eax,word [rsp + 90h + 8h + 28h + 144]   ; iBeta
    punpckhbw   xmm13,xmm1
    movdqa      xmm7,xmm12
    punpcklwd   xmm0,xmm0
    punpckhbw   xmm12,xmm1
    pshufd      xmm11,xmm0,0
    punpcklbw   xmm7,xmm1
    movd        xmm0,eax
    movdqa      xmm1,xmm8
    psubw       xmm1,xmm5
    punpcklwd   xmm0,xmm0
    movdqa      xmm6,xmm11
    pshufd      xmm3,xmm0,0
    movdqa      xmm0,xmm5
    psubw       xmm0,xmm9
    movdqa      xmm2,xmm3
    pabsw       xmm0,xmm0
    pcmpgtw     xmm6,xmm0
    pabsw       xmm0,xmm1
    movdqa      xmm1,xmm3
    pcmpgtw     xmm2,xmm0
    pand        xmm6,xmm2
    movdqa      xmm0,xmm7
    movdqa      xmm2,xmm3
    psubw       xmm0,xmm9
    pabsw       xmm0,xmm0
    pcmpgtw     xmm1,xmm0
    pand        xmm6,xmm1
    movdqa      xmm0,xmm10
    movdqa      xmm1,xmm14
    psubw       xmm0,xmm13
    psubw       xmm1,xmm10
    pabsw       xmm0,xmm0
    pcmpgtw     xmm11,xmm0
    pabsw       xmm0,xmm1
    pcmpgtw     xmm2,xmm0
    pand        xmm11,xmm2
    movdqa      xmm0,xmm12
    movdqa      xmm4,xmm6
    movdqa      xmm1,xmm8
    mov         eax,2
    cwde
    paddw       xmm1,xmm8
    psubw       xmm0,xmm13
    paddw       xmm1,xmm5
    pabsw       xmm0,xmm0
    movdqa      xmm2,xmm14
    paddw       xmm1,xmm7
    pcmpgtw     xmm3,xmm0
    paddw       xmm2,xmm14
    movd        xmm0,eax
    pand        xmm11,xmm3
    paddw       xmm7,xmm7
    paddw       xmm2,xmm10
    punpcklwd   xmm0,xmm0
    paddw       xmm2,xmm12
    paddw       xmm12,xmm12
    pshufd      xmm3,xmm0,0
    paddw       xmm7,xmm9
    paddw       xmm12,xmm13
    movdqa      xmm0,xmm6
    paddw       xmm1,xmm3
    pandn       xmm0,xmm5
    paddw       xmm7,xmm8
    psraw       xmm1,2
    paddw       xmm12,xmm14
    paddw       xmm7,xmm3
    movaps      xmm14,[rsp]
    pand        xmm4,xmm1
    paddw       xmm12,xmm3
    psraw       xmm7,2
    movdqa      xmm1,xmm11
    por         xmm4,xmm0
    psraw       xmm12,2
    paddw       xmm2,xmm3
    movdqa      xmm0,xmm11
    pandn       xmm0,xmm10
    psraw       xmm2,2
    pand        xmm1,xmm2
    por         xmm1,xmm0
    packuswb    xmm4,xmm1
    movdqa      xmm0,xmm11
    movdqa      xmm1,xmm6
    pand        xmm1,xmm7
    movaps      xmm7,[rsp+70h]
    movq        [rcx],xmm4
    pandn       xmm6,xmm9
    pandn       xmm11,xmm13
    pand        xmm0,xmm12
    por         xmm1,xmm6
    por         xmm0,xmm11
    psrldq      xmm4,8
    packuswb    xmm1,xmm0
    movq        [r11],xmm1
    psrldq      xmm1,8
    movq        [rdx],xmm4
    lea         r11,[rsp+90h]
    movaps      xmm6,[r11-10h]
    movaps      xmm8,[r11-30h]
    movaps      xmm9,[r11-40h]
    movq        [rbx],xmm1
    movaps      xmm10,[r11-50h]
    movaps      xmm11,[r11-60h]
    movaps      xmm12,[r11-70h]
    movaps      xmm13,[r11-80h]
    mov         rsp,r11
    POP_XMM
    pop         rbx
    ret





WELS_EXTERN DeblockChromaEq4H_ssse3
    mov         rax,rsp
    mov         [rax+20h],rbx
    push        rdi
    PUSH_XMM 16
    sub         rsp,140h
    mov         rdi,rdx
    lea         eax,[r8*4]
    movsxd      r10,eax
    mov         eax,[rcx-2]
    mov         [rsp+10h],eax
    lea         rbx,[r10+rdx-2]
    lea         r11,[r10+rcx-2]
    movdqa      xmm5,[rsp+10h]
    movsxd      r10,r8d
    mov         eax,[r10+rcx-2]
    lea         rdx,[r10+r10*2]
    mov         [rsp+20h],eax
    mov         eax,[rcx+r10*2-2]
    mov         [rsp+30h],eax
    mov         eax,[rdx+rcx-2]
    movdqa      xmm2,[rsp+20h]
    mov         [rsp+40h],eax
    mov         eax, [rdi-2]
    movdqa      xmm4,[rsp+30h]
    mov         [rsp+50h],eax
    mov         eax,[r10+rdi-2]
    movdqa      xmm3,[rsp+40h]
    mov         [rsp+60h],eax
    mov         eax,[rdi+r10*2-2]
    punpckldq   xmm5,[rsp+50h]
    mov         [rsp+70h],eax
    mov         eax, [rdx+rdi-2]
    punpckldq   xmm2, [rsp+60h]
    mov          [rsp+80h],eax
    mov         eax,[r11]
    punpckldq   xmm4, [rsp+70h]
    mov         [rsp+50h],eax
    mov         eax,[rbx]
    punpckldq   xmm3,[rsp+80h]
    mov         [rsp+60h],eax
    mov         eax,[r10+r11]
    movdqa      xmm0, [rsp+50h]
    punpckldq   xmm0, [rsp+60h]
    punpcklqdq  xmm5,xmm0
    movdqa      [rsp+50h],xmm0
    mov         [rsp+50h],eax
    mov         eax,[r10+rbx]
    movdqa      xmm0,[rsp+50h]
    movdqa      xmm1,xmm5
    mov         [rsp+60h],eax
    mov         eax,[r11+r10*2]
    punpckldq   xmm0, [rsp+60h]
    punpcklqdq  xmm2,xmm0
    punpcklbw   xmm1,xmm2
    punpckhbw   xmm5,xmm2
    movdqa      [rsp+50h],xmm0
    mov         [rsp+50h],eax
    mov         eax,[rbx+r10*2]
    movdqa      xmm0,[rsp+50h]
    mov         [rsp+60h],eax
    mov         eax, [rdx+r11]
    movdqa      xmm15,xmm1
    punpckldq   xmm0,[rsp+60h]
    punpcklqdq  xmm4,xmm0
    movdqa      [rsp+50h],xmm0
    mov         [rsp+50h],eax
    mov         eax, [rdx+rbx]
    movdqa      xmm0,[rsp+50h]
    mov         [rsp+60h],eax
    punpckldq   xmm0, [rsp+60h]
    punpcklqdq  xmm3,xmm0
    movdqa      xmm0,xmm4
    punpcklbw   xmm0,xmm3
    punpckhbw   xmm4,xmm3
    punpcklwd   xmm15,xmm0
    punpckhwd   xmm1,xmm0
    movdqa      xmm0,xmm5
    movdqa      xmm12,xmm15
    punpcklwd   xmm0,xmm4
    punpckhwd   xmm5,xmm4
    punpckldq   xmm12,xmm0
    punpckhdq   xmm15,xmm0
    movdqa      xmm0,xmm1
    movdqa      xmm11,xmm12
    punpckldq   xmm0,xmm5
    punpckhdq   xmm1,xmm5
    punpcklqdq  xmm11,xmm0
    punpckhqdq  xmm12,xmm0
    movsx       eax,r9w
    movdqa      xmm14,xmm15
    punpcklqdq  xmm14,xmm1
    punpckhqdq  xmm15,xmm1
    pxor        xmm1,xmm1
    movd        xmm0,eax
    movdqa      xmm4,xmm12
    movdqa      xmm8,xmm11
    movsx       eax,word [rsp+170h + 160] ; iBeta
    punpcklwd   xmm0,xmm0
    punpcklbw   xmm4,xmm1
    punpckhbw   xmm12,xmm1
    movdqa      xmm9,xmm14
    movdqa      xmm7,xmm15
    movdqa      xmm10,xmm15
    pshufd      xmm13,xmm0,0
    punpcklbw   xmm9,xmm1
    punpckhbw   xmm14,xmm1
    movdqa      xmm6,xmm13
    movd        xmm0,eax
    movdqa      [rsp],xmm11
    mov         eax,2
    cwde
    punpckhbw   xmm11,xmm1
    punpckhbw   xmm10,xmm1
    punpcklbw   xmm7,xmm1
    punpcklwd   xmm0,xmm0
    punpcklbw   xmm8,xmm1
    pshufd      xmm3,xmm0,0
    movdqa      xmm1,xmm8
    movdqa      xmm0,xmm4
    psubw       xmm0,xmm9
    psubw       xmm1,xmm4
    movdqa      xmm2,xmm3
    pabsw       xmm0,xmm0
    pcmpgtw     xmm6,xmm0
    pabsw       xmm0,xmm1
    movdqa      xmm1,xmm3
    pcmpgtw     xmm2,xmm0
    pand        xmm6,xmm2
    movdqa      xmm0,xmm7
    movdqa      xmm2,xmm3
    psubw       xmm0,xmm9
    pabsw       xmm0,xmm0
    pcmpgtw     xmm1,xmm0
    pand        xmm6,xmm1
    movdqa      xmm0,xmm12
    movdqa      xmm1,xmm11
    psubw       xmm0,xmm14
    psubw       xmm1,xmm12
    movdqa      xmm5,xmm6
    pabsw       xmm0,xmm0
    pcmpgtw     xmm13,xmm0
    pabsw       xmm0,xmm1
    movdqa      xmm1,xmm8
    pcmpgtw     xmm2,xmm0
    paddw       xmm1,xmm8
    movdqa      xmm0,xmm10
    pand        xmm13,xmm2
    psubw       xmm0,xmm14
    paddw       xmm1,xmm4
    movdqa      xmm2,xmm11
    pabsw       xmm0,xmm0
    paddw       xmm2,xmm11
    paddw       xmm1,xmm7
    pcmpgtw     xmm3,xmm0
    paddw       xmm2,xmm12
    movd        xmm0,eax
    pand        xmm13,xmm3
    paddw       xmm2,xmm10
    punpcklwd   xmm0,xmm0
    pshufd      xmm3,xmm0,0
    movdqa      xmm0,xmm6
    paddw       xmm1,xmm3
    pandn       xmm0,xmm4
    paddw       xmm2,xmm3
    psraw       xmm1,2
    pand        xmm5,xmm1
    por         xmm5,xmm0
    paddw       xmm7,xmm7
    paddw       xmm10,xmm10
    psraw       xmm2,2
    movdqa      xmm1,xmm13
    movdqa      xmm0,xmm13
    pandn       xmm0,xmm12
    pand        xmm1,xmm2
    paddw       xmm7,xmm9
    por         xmm1,xmm0
    paddw       xmm10,xmm14
    paddw       xmm7,xmm8
    movdqa      xmm0,xmm13
    packuswb    xmm5,xmm1
    paddw       xmm7,xmm3
    paddw       xmm10,xmm11
    movdqa      xmm1,xmm6
    paddw       xmm10,xmm3
    pandn       xmm6,xmm9
    psraw       xmm7,2
    pand        xmm1,xmm7
    psraw       xmm10,2
    pandn       xmm13,xmm14
    pand        xmm0,xmm10
    por         xmm1,xmm6
    movdqa      xmm6,[rsp]
    movdqa      xmm4,xmm6
    por         xmm0,xmm13
    punpcklbw   xmm4,xmm5
    punpckhbw   xmm6,xmm5
    movdqa      xmm3,xmm4
    packuswb    xmm1,xmm0
    movdqa      xmm0,xmm1
    punpckhbw   xmm1,xmm15
    punpcklbw   xmm0,xmm15
    punpcklwd   xmm3,xmm0
    punpckhwd   xmm4,xmm0
    movdqa      xmm0,xmm6
    movdqa      xmm2,xmm3
    punpcklwd   xmm0,xmm1
    punpckhwd   xmm6,xmm1
    movdqa      xmm1,xmm4
    punpckldq   xmm2,xmm0
    punpckhdq   xmm3,xmm0
    punpckldq   xmm1,xmm6
    movdqa      xmm0,xmm2
    punpcklqdq  xmm0,xmm1
    punpckhdq   xmm4,xmm6
    punpckhqdq  xmm2,xmm1
    movdqa      [rsp+10h],xmm0
    movdqa      [rsp+60h],xmm2
    movdqa      xmm0,xmm3
    mov         eax,[rsp+10h]
    mov         [rcx-2],eax
    mov         eax,[rsp+60h]
    punpcklqdq  xmm0,xmm4
    punpckhqdq  xmm3,xmm4
    mov         [r10+rcx-2],eax
    movdqa      [rsp+20h],xmm0
    mov         eax, [rsp+20h]
    movdqa      [rsp+70h],xmm3
    mov         [rcx+r10*2-2],eax
    mov         eax,[rsp+70h]
    mov         [rdx+rcx-2],eax
    mov         eax,[rsp+18h]
    mov         [r11],eax
    mov         eax,[rsp+68h]
    mov         [r10+r11],eax
    mov         eax,[rsp+28h]
    mov         [r11+r10*2],eax
    mov         eax,[rsp+78h]
    mov         [rdx+r11],eax
    mov         eax,[rsp+14h]
    mov         [rdi-2],eax
    mov         eax,[rsp+64h]
    mov         [r10+rdi-2],eax
    mov         eax,[rsp+24h]
    mov         [rdi+r10*2-2],eax
    mov         eax, [rsp+74h]
    mov         [rdx+rdi-2],eax
    mov         eax, [rsp+1Ch]
    mov         [rbx],eax
    mov         eax, [rsp+6Ch]
    mov         [r10+rbx],eax
    mov         eax,[rsp+2Ch]
    mov         [rbx+r10*2],eax
    mov         eax,[rsp+7Ch]
    mov         [rdx+rbx],eax
    lea         rsp,[rsp+140h]
    POP_XMM
    mov         rbx, [rsp+28h]
    pop         rdi
    ret



WELS_EXTERN DeblockChromaLt4H_ssse3
    mov         rax,rsp
    push        rbx
    push        rbp
    push        rsi
    push        rdi
    push        r12
    PUSH_XMM 16
    sub         rsp,170h

    movsxd      rsi,r8d
    lea         eax,[r8*4]
    mov         r11d,r9d
    movsxd      r10,eax
    mov         eax, [rcx-2]
    mov         r12,rdx
    mov         [rsp+40h],eax
    mov         eax, [rsi+rcx-2]
    lea         rbx,[r10+rcx-2]
    movdqa      xmm5,[rsp+40h]
    mov         [rsp+50h],eax
    mov         eax, [rcx+rsi*2-2]
    lea         rbp,[r10+rdx-2]
    movdqa      xmm2, [rsp+50h]
    mov         [rsp+60h],eax
    lea         r10,[rsi+rsi*2]
    mov         rdi,rcx
    mov         eax,[r10+rcx-2]
    movdqa      xmm4,[rsp+60h]
    mov         [rsp+70h],eax
    mov         eax,[rdx-2]
    mov         [rsp+80h],eax
    mov         eax, [rsi+rdx-2]
    movdqa      xmm3,[rsp+70h]
    mov         [rsp+90h],eax
    mov         eax,[rdx+rsi*2-2]
    punpckldq   xmm5,[rsp+80h]
    mov         [rsp+0A0h],eax
    mov         eax, [r10+rdx-2]
    punpckldq   xmm2,[rsp+90h]
    mov         [rsp+0B0h],eax
    mov         eax, [rbx]
    punpckldq   xmm4,[rsp+0A0h]
    mov         [rsp+80h],eax
    mov         eax,[rbp]
    punpckldq   xmm3,[rsp+0B0h]
    mov         [rsp+90h],eax
    mov         eax,[rsi+rbx]
    movdqa      xmm0,[rsp+80h]
    punpckldq   xmm0,[rsp+90h]
    punpcklqdq  xmm5,xmm0
    movdqa      [rsp+80h],xmm0
    mov         [rsp+80h],eax
    mov         eax,[rsi+rbp]
    movdqa      xmm0,[rsp+80h]
    movdqa      xmm1,xmm5
    mov         [rsp+90h],eax
    mov         eax,[rbx+rsi*2]
    punpckldq   xmm0,[rsp+90h]
    punpcklqdq  xmm2,xmm0
    punpcklbw   xmm1,xmm2
    punpckhbw   xmm5,xmm2
    movdqa      [rsp+80h],xmm0
    mov         [rsp+80h],eax
    mov         eax,[rbp+rsi*2]
    movdqa      xmm0, [rsp+80h]
    mov         [rsp+90h],eax
    mov         eax,[r10+rbx]
    movdqa      xmm7,xmm1
    punpckldq   xmm0,[rsp+90h]
    punpcklqdq  xmm4,xmm0
    movdqa      [rsp+80h],xmm0
    mov         [rsp+80h],eax
    mov         eax, [r10+rbp]
    movdqa      xmm0,[rsp+80h]
    mov         [rsp+90h],eax
    punpckldq   xmm0,[rsp+90h]
    punpcklqdq  xmm3,xmm0
    movdqa      xmm0,xmm4
    punpcklbw   xmm0,xmm3
    punpckhbw   xmm4,xmm3
    punpcklwd   xmm7,xmm0
    punpckhwd   xmm1,xmm0
    movdqa      xmm0,xmm5
    movdqa      xmm6,xmm7
    punpcklwd   xmm0,xmm4
    punpckhwd   xmm5,xmm4
    punpckldq   xmm6,xmm0
    punpckhdq   xmm7,xmm0
    movdqa      xmm0,xmm1
    punpckldq   xmm0,xmm5
    mov         rax, [rsp+1C8h+160]    ; pTC
    punpckhdq   xmm1,xmm5
    movdqa      xmm9,xmm6
    punpckhqdq  xmm6,xmm0
    punpcklqdq  xmm9,xmm0
    movdqa      xmm2,xmm7
    movdqa      xmm13,xmm6
    movdqa      xmm4,xmm9
    movdqa      [rsp+10h],xmm9
    punpcklqdq  xmm2,xmm1
    punpckhqdq  xmm7,xmm1
    pxor        xmm1,xmm1
    movsx       ecx,byte [rax+3]
    movsx       edx,byte [rax+2]
    movsx       r8d,byte [rax+1]
    movsx       r9d,byte [rax]
    movdqa      xmm10,xmm1
    movdqa      xmm15,xmm2
    punpckhbw   xmm2,xmm1
    punpckhbw   xmm6,xmm1
    punpcklbw   xmm4,xmm1
    movsx       eax,r11w
    mov         word [rsp+0Eh],cx
    mov         word [rsp+0Ch],cx
    movdqa      xmm3,xmm7
    movdqa      xmm8,xmm7
    movdqa      [rsp+20h],xmm7
    punpcklbw   xmm15,xmm1
    punpcklbw   xmm13,xmm1
    punpcklbw   xmm3,xmm1
    mov         word [rsp+0Ah],dx
    mov         word [rsp+8],dx
    mov         word [rsp+6],r8w
    movd        xmm0,eax
    movdqa      [rsp+30h],xmm6
    punpckhbw   xmm9,xmm1
    punpckhbw   xmm8,xmm1
    punpcklwd   xmm0,xmm0
    movsx       eax,word [rsp+1C0h+160]   ; iBeta
    mov         word [rsp+4],r8w
    mov         word [rsp+2],r9w
    pshufd      xmm12,xmm0,0
    mov         word [rsp],r9w
    movd        xmm0,eax
    mov         eax,4
    cwde
    movdqa      xmm14, [rsp]
    movdqa      [rsp],xmm2
    movdqa      xmm2,xmm12
    punpcklwd   xmm0,xmm0
    pshufd      xmm11,xmm0,0
    psubw       xmm10,xmm14
    movd        xmm0,eax
    movdqa      xmm7,xmm14
    movdqa      xmm6,xmm14
    pcmpgtw     xmm7,xmm1
    punpcklwd   xmm0,xmm0
    pshufd      xmm5,xmm0,0
    movdqa      xmm0,xmm4
    movdqa      xmm1,xmm15
    psubw       xmm4,xmm13
    psubw       xmm0,xmm3
    psubw       xmm1,xmm13
    psubw       xmm3,xmm15
    psllw       xmm1,2
    paddw       xmm1,xmm0
    paddw       xmm1,xmm5
    movdqa      xmm0,xmm10
    psraw       xmm1,3
    pmaxsw      xmm0,xmm1
    pminsw      xmm6,xmm0
    movdqa      xmm1,xmm11
    movdqa      xmm0,xmm13
    psubw       xmm0,xmm15
    pabsw       xmm0,xmm0
    pcmpgtw     xmm2,xmm0
    pabsw       xmm0,xmm4
    pcmpgtw     xmm1,xmm0
    pabsw       xmm0,xmm3
    pand        xmm2,xmm1
    movdqa      xmm1,xmm11
    movdqa      xmm3,[rsp+30h]
    pcmpgtw     xmm1,xmm0
    movdqa      xmm0,xmm9
    pand        xmm2,xmm1
    psubw       xmm0,xmm8
    psubw       xmm9,xmm3
    pand        xmm2,xmm7
    pand        xmm6,xmm2
    psubw       xmm15,xmm6
    paddw       xmm13,xmm6
    movdqa      xmm2,[rsp]
    movdqa      xmm1,xmm2
    psubw       xmm1,xmm3
    psubw       xmm8,xmm2
    psllw       xmm1,2
    paddw       xmm1,xmm0
    paddw       xmm1,xmm5
    movdqa      xmm0,xmm3
    movdqa      xmm5,[rsp+10h]
    psubw       xmm0,xmm2
    psraw       xmm1,3
    movdqa      xmm4,xmm5
    pabsw       xmm0,xmm0
    pmaxsw      xmm10,xmm1
    movdqa      xmm1,xmm11
    pcmpgtw     xmm12,xmm0
    pabsw       xmm0,xmm9
    pminsw      xmm14,xmm10
    pcmpgtw     xmm1,xmm0
    pabsw       xmm0,xmm8
    pcmpgtw     xmm11,xmm0
    pand        xmm12,xmm1
    movdqa      xmm1,[rsp+20h]
    pand        xmm12,xmm11
    pand        xmm12,xmm7
    pand        xmm14,xmm12
    paddw       xmm3,xmm14
    psubw       xmm2,xmm14
    packuswb    xmm13,xmm3
    packuswb    xmm15,xmm2
    punpcklbw   xmm4,xmm13
    punpckhbw   xmm5,xmm13
    movdqa      xmm0,xmm15
    punpcklbw   xmm0,xmm1
    punpckhbw   xmm15,xmm1
    movdqa      xmm3,xmm4
    punpcklwd   xmm3,xmm0
    punpckhwd   xmm4,xmm0
    movdqa      xmm0,xmm5
    movdqa      xmm2,xmm3
    movdqa      xmm1,xmm4
    punpcklwd   xmm0,xmm15
    punpckhwd   xmm5,xmm15
    punpckldq   xmm2,xmm0
    punpckhdq   xmm3,xmm0
    punpckldq   xmm1,xmm5
    movdqa      xmm0,xmm2
    punpcklqdq  xmm0,xmm1
    punpckhdq   xmm4,xmm5
    punpckhqdq  xmm2,xmm1
    movdqa      [rsp+40h],xmm0
    movdqa      xmm0,xmm3
    movdqa      [rsp+90h],xmm2
    mov         eax,[rsp+40h]
    mov         [rdi-2],eax
    mov         eax, [rsp+90h]
    punpcklqdq  xmm0,xmm4
    punpckhqdq  xmm3,xmm4
    mov         [rsi+rdi-2],eax
    movdqa      [rsp+50h],xmm0
    mov         eax,[rsp+50h]
    movdqa      [rsp+0A0h],xmm3
    mov         [rdi+rsi*2-2],eax
    mov         eax,[rsp+0A0h]
    mov         [r10+rdi-2],eax
    mov         eax,[rsp+48h]
    mov         [rbx],eax
    mov         eax,[rsp+98h]
    mov         [rsi+rbx],eax
    mov         eax,[rsp+58h]
    mov         [rbx+rsi*2],eax
    mov         eax, [rsp+0A8h]
    mov         [r10+rbx],eax
    mov         eax, [rsp+44h]
    mov         [r12-2],eax
    mov         eax,[rsp+94h]
    mov         [rsi+r12-2],eax
    mov         eax,[rsp+54h]
    mov         [r12+rsi*2-2],eax
    mov         eax, [rsp+0A4h]
    mov         [r10+r12-2],eax
    mov         eax,[rsp+4Ch]
    mov         [rbp],eax
    mov         eax,[rsp+9Ch]
    mov         [rsi+rbp],eax
    mov         eax, [rsp+5Ch]
    mov         [rbp+rsi*2],eax
    mov         eax,[rsp+0ACh]
    mov         [r10+rbp],eax
    lea         r11,[rsp+170h]
    mov         rsp,r11
    POP_XMM
    pop         r12
    pop         rdi
    pop         rsi
    pop         rbp
    pop         rbx
    ret



%elifdef  UNIX64


WELS_EXTERN DeblockChromaLt4V_ssse3
    mov         rax,rsp
    push        rbx
    push        rbp
    mov         r10,  rdx
    mov         r11,  rcx
    mov         rcx,  rdi
    mov         rdx,  rsi
    mov         rsi,  r10
    mov         r10,  r9
    mov         rbp,  r8
    mov         r8,   rsi
    mov         r9,   r11
    sub         rsp,0C8h
    pxor        xmm1,xmm1
    mov         rbx,rcx
    movsxd      r11,r8d
    movsx       ecx,byte [r10]
    movsx       r8d,byte [r10+2]
    mov         rdi,rdx
    movq        xmm2,[rbx]
    movq        xmm9,[r11+rbx]
    movsx       edx,byte [r10+1]
    mov         word [rsp+2],cx
    mov         word [rsp],cx
    movsx       eax,byte [r10+3]
    mov         word [rsp+6],dx
    mov         word [rsp+4],dx
    movdqa      xmm11,xmm1
    mov         word [rsp+0Eh],ax
    mov         word [rsp+0Ch],ax
    lea         eax,[r11+r11]
    movsxd      rcx,eax
    mov         rax,rbx
    mov         rdx,rdi
    sub         rax,rcx
    mov         word [rsp+0Ah],r8w
    mov         word [rsp+8],r8w
    movdqa      xmm6,[rsp]
    movdqa      xmm7,xmm6
    movq        xmm13, [rax]
    mov         rax,rdi
    sub         rax,rcx
    mov         rcx,rbx
    pcmpgtw     xmm7,xmm1
    psubw       xmm11,xmm6
    sub         rcx,r11
    sub         rdx,r11
    movq        xmm0,[rax]
    movsx       eax,r9w
    movq        xmm15,[rcx]
    punpcklqdq  xmm13,xmm0
    movq        xmm0, [rdx]
    movdqa      xmm4,xmm13
    punpcklqdq  xmm15,xmm0
    movq        xmm0, [rdi]
    punpcklbw   xmm4,xmm1
    movdqa      xmm12,xmm15
    punpcklqdq  xmm2,xmm0
    movq        xmm0, [r11+rdi]
    punpcklbw   xmm12,xmm1
    movdqa      xmm14,xmm2
    punpcklqdq  xmm9,xmm0
    punpckhbw   xmm2,xmm1
    punpcklbw   xmm14,xmm1
    movd        xmm0,eax
    mov         eax, ebp ; iBeta
    punpckhbw   xmm13,xmm1
    punpckhbw   xmm15,xmm1
    movdqa      xmm3,xmm9
    movdqa      [rsp+10h],xmm2
    punpcklwd   xmm0,xmm0
    punpckhbw   xmm9,xmm1
    punpcklbw   xmm3,xmm1
    movdqa      xmm1,xmm14
    pshufd      xmm10,xmm0,0
    movd        xmm0,eax
    mov         eax,4
    cwde
    punpcklwd   xmm0,xmm0
    pshufd      xmm8,xmm0,0
    movd        xmm0,eax
    punpcklwd   xmm0,xmm0
    pshufd      xmm5,xmm0,0
    psubw       xmm1,xmm12
    movdqa      xmm2,xmm10
    lea         r11,[rsp+0C8h]
    psllw       xmm1,2
    movdqa      xmm0,xmm4
    psubw       xmm4,xmm12
    psubw       xmm0,xmm3
    psubw       xmm3,xmm14
    paddw       xmm1,xmm0
    paddw       xmm1,xmm5
    movdqa      xmm0,xmm11
    psraw       xmm1,3
    pmaxsw      xmm0,xmm1
    pminsw      xmm6,xmm0
    movdqa      xmm1,xmm8
    movdqa      xmm0,xmm12
    psubw       xmm0,xmm14
    pabsw       xmm0,xmm0
    pcmpgtw     xmm2,xmm0
    pabsw       xmm0,xmm4
    pcmpgtw     xmm1,xmm0
    pabsw       xmm0,xmm3
    movdqa      xmm3,[rsp]
    pand        xmm2,xmm1
    movdqa      xmm1,xmm8
    pcmpgtw     xmm1,xmm0
    movdqa      xmm0,xmm13
    pand        xmm2,xmm1
    psubw       xmm0,xmm9
    psubw       xmm13,xmm15
    pand        xmm2,xmm7
    pand        xmm6,xmm2
    paddw       xmm12,xmm6
    psubw       xmm14,xmm6
    movdqa      xmm2,[rsp+10h]
    movaps      xmm6,[r11-18h]
    movdqa      xmm1,xmm2
    psubw       xmm1,xmm15
    psubw       xmm9,xmm2
    psllw       xmm1,2
    paddw       xmm1,xmm0
    paddw       xmm1,xmm5
    movdqa      xmm0,xmm15
    psubw       xmm0,xmm2
    psraw       xmm1,3
    pmaxsw      xmm11,xmm1
    pabsw       xmm0,xmm0
    movdqa      xmm1,xmm8
    pcmpgtw     xmm10,xmm0
    pabsw       xmm0,xmm13
    pminsw      xmm3,xmm11
    movaps      xmm11,[r11-68h]
    movaps      xmm13,[rsp+40h]
    pcmpgtw     xmm1,xmm0
    pabsw       xmm0,xmm9
    movaps      xmm9, [r11-48h]
    pand        xmm10,xmm1
    pcmpgtw     xmm8,xmm0
    pand        xmm10,xmm8
    pand        xmm10,xmm7
    movaps      xmm8,[r11-38h]
    movaps      xmm7,[r11-28h]
    pand        xmm3,xmm10
    paddw       xmm15,xmm3
    psubw       xmm2,xmm3
    movaps      xmm10,[r11-58h]
    packuswb    xmm12,xmm15
    movaps      xmm15,[rsp+20h]
    packuswb    xmm14,xmm2
    movq        [rcx],xmm12
    movq        [rbx],xmm14
    psrldq      xmm12,8
    psrldq      xmm14,8
    movq        [rdx],xmm12
    movaps      xmm12,[r11-78h]
    movq        [rdi],xmm14
    movaps      xmm14,[rsp+30h]
    mov         rsp,r11
    pop         rbp
    pop         rbx
    ret

WELS_EXTERN DeblockChromaEq4V_ssse3
    mov         rax,rsp
    push        rbx
    push        rbp

    mov         rbp, r8
    mov         r8, rdx
    mov         r9, rcx
    mov         rcx, rdi
    mov         rdx, rsi

    sub         rsp,90h
    pxor        xmm1,xmm1
    mov         r11,rcx
    mov         rbx,rdx
    mov         r10d,r9d
    movq        xmm13,[r11]
    lea         eax,[r8+r8]
    movsxd      r9,eax
    mov         rax,rcx
    sub         rax,r9
    movq        xmm14,[rax]
    mov         rax,rdx
    sub         rax,r9
    movq        xmm0,[rax]
    movsxd      rax,r8d
    sub         rcx,rax
    sub         rdx,rax
    movq        xmm12,[rax+r11]
    movq        xmm10,[rcx]
    punpcklqdq  xmm14,xmm0
    movdqa      xmm8,xmm14
    movq        xmm0,[rdx]
    punpcklbw   xmm8,xmm1
    punpckhbw   xmm14,xmm1
    punpcklqdq  xmm10,xmm0
    movq        xmm0,[rbx]
    movdqa      xmm5,xmm10
    punpcklqdq  xmm13,xmm0
    movq        xmm0, [rax+rbx]
    punpcklbw   xmm5,xmm1
    movsx       eax,r10w
    movdqa      xmm9,xmm13
    punpcklqdq  xmm12,xmm0
    punpcklbw   xmm9,xmm1
    punpckhbw   xmm10,xmm1
    movd        xmm0,eax
    mov         eax, ebp   ; iBeta
    punpckhbw   xmm13,xmm1
    movdqa      xmm7,xmm12
    punpcklwd   xmm0,xmm0
    punpckhbw   xmm12,xmm1
    pshufd      xmm11,xmm0,0
    punpcklbw   xmm7,xmm1
    movd        xmm0,eax
    movdqa      xmm1,xmm8
    psubw       xmm1,xmm5
    punpcklwd   xmm0,xmm0
    movdqa      xmm6,xmm11
    pshufd      xmm3,xmm0,0
    movdqa      xmm0,xmm5
    psubw       xmm0,xmm9
    movdqa      xmm2,xmm3
    pabsw       xmm0,xmm0
    pcmpgtw     xmm6,xmm0
    pabsw       xmm0,xmm1
    movdqa      xmm1,xmm3
    pcmpgtw     xmm2,xmm0
    pand        xmm6,xmm2
    movdqa      xmm0,xmm7
    movdqa      xmm2,xmm3
    psubw       xmm0,xmm9
    pabsw       xmm0,xmm0
    pcmpgtw     xmm1,xmm0
    pand        xmm6,xmm1
    movdqa      xmm0,xmm10
    movdqa      xmm1,xmm14
    psubw       xmm0,xmm13
    psubw       xmm1,xmm10
    pabsw       xmm0,xmm0
    pcmpgtw     xmm11,xmm0
    pabsw       xmm0,xmm1
    pcmpgtw     xmm2,xmm0
    pand        xmm11,xmm2
    movdqa      xmm0,xmm12
    movdqa      xmm4,xmm6
    movdqa      xmm1,xmm8
    mov         eax,2
    cwde
    paddw       xmm1,xmm8
    psubw       xmm0,xmm13
    paddw       xmm1,xmm5
    pabsw       xmm0,xmm0
    movdqa      xmm2,xmm14
    paddw       xmm1,xmm7
    pcmpgtw     xmm3,xmm0
    paddw       xmm2,xmm14
    movd        xmm0,eax
    pand        xmm11,xmm3
    paddw       xmm7,xmm7
    paddw       xmm2,xmm10
    punpcklwd   xmm0,xmm0
    paddw       xmm2,xmm12
    paddw       xmm12,xmm12
    pshufd      xmm3,xmm0,0
    paddw       xmm7,xmm9
    paddw       xmm12,xmm13
    movdqa      xmm0,xmm6
    paddw       xmm1,xmm3
    pandn       xmm0,xmm5
    paddw       xmm7,xmm8
    psraw       xmm1,2
    paddw       xmm12,xmm14
    paddw       xmm7,xmm3
    ;movaps      xmm14,[rsp]
    pand        xmm4,xmm1
    paddw       xmm12,xmm3
    psraw       xmm7,2
    movdqa      xmm1,xmm11
    por         xmm4,xmm0
    psraw       xmm12,2
    paddw       xmm2,xmm3
    movdqa      xmm0,xmm11
    pandn       xmm0,xmm10
    psraw       xmm2,2
    pand        xmm1,xmm2
    por         xmm1,xmm0
    packuswb    xmm4,xmm1
    movdqa      xmm0,xmm11
    movdqa      xmm1,xmm6
    pand        xmm1,xmm7
    movq        [rcx],xmm4
    pandn       xmm6,xmm9
    pandn       xmm11,xmm13
    pand        xmm0,xmm12
    por         xmm1,xmm6
    por         xmm0,xmm11
    psrldq      xmm4,8
    packuswb    xmm1,xmm0
    movq        [r11],xmm1
    psrldq      xmm1,8
    movq        [rdx],xmm4
    lea         r11,[rsp+90h]
    movq        [rbx],xmm1
    mov         rsp,r11
    pop         rbp
    pop         rbx
    ret

WELS_EXTERN DeblockChromaEq4H_ssse3
    mov         rax,rsp
    push        rbx
    push        rbp
    push        r12

    mov         rbp,   r8
    mov         r8,    rdx
    mov         r9,    rcx
    mov         rcx,   rdi
    mov         rdx,   rsi
    mov         rdi,   rdx

    sub         rsp,140h
    lea         eax,[r8*4]
    movsxd      r10,eax
    mov         eax,[rcx-2]
    mov         [rsp+10h],eax
    lea         rbx,[r10+rdx-2]
    lea         r11,[r10+rcx-2]

    movdqa      xmm5,[rsp+10h]
    movsxd      r10,r8d
    mov         eax,[r10+rcx-2]
    lea         rdx,[r10+r10*2]
    mov         [rsp+20h],eax
    mov         eax,[rcx+r10*2-2]
    mov         [rsp+30h],eax
    mov         eax,[rdx+rcx-2]
    movdqa      xmm2,[rsp+20h]
    mov         [rsp+40h],eax
    mov         eax, [rdi-2]
    movdqa      xmm4,[rsp+30h]
    mov         [rsp+50h],eax
    mov         eax,[r10+rdi-2]
    movdqa      xmm3,[rsp+40h]
    mov         [rsp+60h],eax
    mov         eax,[rdi+r10*2-2]
    punpckldq   xmm5,[rsp+50h]
    mov         [rsp+70h],eax
    mov         eax, [rdx+rdi-2]
    punpckldq   xmm2, [rsp+60h]
    mov          [rsp+80h],eax
    mov         eax,[r11]
    punpckldq   xmm4, [rsp+70h]
    mov         [rsp+50h],eax
    mov         eax,[rbx]
    punpckldq   xmm3,[rsp+80h]
    mov         [rsp+60h],eax
    mov         eax,[r10+r11]
    movdqa      xmm0, [rsp+50h]
    punpckldq   xmm0, [rsp+60h]
    punpcklqdq  xmm5,xmm0
    movdqa      [rsp+50h],xmm0
    mov         [rsp+50h],eax
    mov         eax,[r10+rbx]
    movdqa      xmm0,[rsp+50h]
    movdqa      xmm1,xmm5
    mov         [rsp+60h],eax
    mov         eax,[r11+r10*2]
    punpckldq   xmm0, [rsp+60h]
    punpcklqdq  xmm2,xmm0
    punpcklbw   xmm1,xmm2
    punpckhbw   xmm5,xmm2
    movdqa      [rsp+50h],xmm0
    mov         [rsp+50h],eax
    mov         eax,[rbx+r10*2]
    movdqa      xmm0,[rsp+50h]
    mov         [rsp+60h],eax
    mov         eax, [rdx+r11]
    movdqa      xmm15,xmm1
    punpckldq   xmm0,[rsp+60h]
    punpcklqdq  xmm4,xmm0
    movdqa      [rsp+50h],xmm0
    mov         [rsp+50h],eax
    mov         eax, [rdx+rbx]
    movdqa      xmm0,[rsp+50h]
    mov         [rsp+60h],eax
    punpckldq   xmm0, [rsp+60h]
    punpcklqdq  xmm3,xmm0
    movdqa      xmm0,xmm4
    punpcklbw   xmm0,xmm3
    punpckhbw   xmm4,xmm3
    punpcklwd   xmm15,xmm0
    punpckhwd   xmm1,xmm0
    movdqa      xmm0,xmm5
    movdqa      xmm12,xmm15
    punpcklwd   xmm0,xmm4
    punpckhwd   xmm5,xmm4
    punpckldq   xmm12,xmm0
    punpckhdq   xmm15,xmm0
    movdqa      xmm0,xmm1
    movdqa      xmm11,xmm12
    punpckldq   xmm0,xmm5
    punpckhdq   xmm1,xmm5
    punpcklqdq  xmm11,xmm0
    punpckhqdq  xmm12,xmm0
    movsx       eax,r9w
    movdqa      xmm14,xmm15
    punpcklqdq  xmm14,xmm1
    punpckhqdq  xmm15,xmm1
    pxor        xmm1,xmm1
    movd        xmm0,eax
    movdqa      xmm4,xmm12
    movdqa      xmm8,xmm11
    mov         eax, ebp ; iBeta
    punpcklwd   xmm0,xmm0
    punpcklbw   xmm4,xmm1
    punpckhbw   xmm12,xmm1
    movdqa      xmm9,xmm14
    movdqa      xmm7,xmm15
    movdqa      xmm10,xmm15
    pshufd      xmm13,xmm0,0
    punpcklbw   xmm9,xmm1
    punpckhbw   xmm14,xmm1
    movdqa      xmm6,xmm13
    movd        xmm0,eax
    movdqa      [rsp],xmm11
    mov         eax,2
    cwde
    punpckhbw   xmm11,xmm1
    punpckhbw   xmm10,xmm1
    punpcklbw   xmm7,xmm1
    punpcklwd   xmm0,xmm0
    punpcklbw   xmm8,xmm1
    pshufd      xmm3,xmm0,0
    movdqa      xmm1,xmm8
    movdqa      xmm0,xmm4
    psubw       xmm0,xmm9
    psubw       xmm1,xmm4
    movdqa      xmm2,xmm3
    pabsw       xmm0,xmm0
    pcmpgtw     xmm6,xmm0
    pabsw       xmm0,xmm1
    movdqa      xmm1,xmm3
    pcmpgtw     xmm2,xmm0
    pand        xmm6,xmm2
    movdqa      xmm0,xmm7
    movdqa      xmm2,xmm3
    psubw       xmm0,xmm9
    pabsw       xmm0,xmm0
    pcmpgtw     xmm1,xmm0
    pand        xmm6,xmm1
    movdqa      xmm0,xmm12
    movdqa      xmm1,xmm11
    psubw       xmm0,xmm14
    psubw       xmm1,xmm12
    movdqa      xmm5,xmm6
    pabsw       xmm0,xmm0
    pcmpgtw     xmm13,xmm0
    pabsw       xmm0,xmm1
    movdqa      xmm1,xmm8
    pcmpgtw     xmm2,xmm0
    paddw       xmm1,xmm8
    movdqa      xmm0,xmm10
    pand        xmm13,xmm2
    psubw       xmm0,xmm14
    paddw       xmm1,xmm4
    movdqa      xmm2,xmm11
    pabsw       xmm0,xmm0
    paddw       xmm2,xmm11
    paddw       xmm1,xmm7
    pcmpgtw     xmm3,xmm0
    paddw       xmm2,xmm12
    movd        xmm0,eax
    pand        xmm13,xmm3
    paddw       xmm2,xmm10
    punpcklwd   xmm0,xmm0
    pshufd      xmm3,xmm0,0
    movdqa      xmm0,xmm6
    paddw       xmm1,xmm3
    pandn       xmm0,xmm4
    paddw       xmm2,xmm3
    psraw       xmm1,2
    pand        xmm5,xmm1
    por         xmm5,xmm0
    paddw       xmm7,xmm7
    paddw       xmm10,xmm10
    psraw       xmm2,2
    movdqa      xmm1,xmm13
    movdqa      xmm0,xmm13
    pandn       xmm0,xmm12
    pand        xmm1,xmm2
    paddw       xmm7,xmm9
    por         xmm1,xmm0
    paddw       xmm10,xmm14
    paddw       xmm7,xmm8
    movdqa      xmm0,xmm13
    packuswb    xmm5,xmm1
    paddw       xmm7,xmm3
    paddw       xmm10,xmm11
    movdqa      xmm1,xmm6
    paddw       xmm10,xmm3
    pandn       xmm6,xmm9
    psraw       xmm7,2
    pand        xmm1,xmm7
    psraw       xmm10,2
    pandn       xmm13,xmm14
    pand        xmm0,xmm10
    por         xmm1,xmm6
    movdqa      xmm6,[rsp]
    movdqa      xmm4,xmm6
    por         xmm0,xmm13
    punpcklbw   xmm4,xmm5
    punpckhbw   xmm6,xmm5
    movdqa      xmm3,xmm4
    packuswb    xmm1,xmm0
    movdqa      xmm0,xmm1
    punpckhbw   xmm1,xmm15
    punpcklbw   xmm0,xmm15
    punpcklwd   xmm3,xmm0
    punpckhwd   xmm4,xmm0
    movdqa      xmm0,xmm6
    movdqa      xmm2,xmm3
    punpcklwd   xmm0,xmm1
    punpckhwd   xmm6,xmm1
    movdqa      xmm1,xmm4
    punpckldq   xmm2,xmm0
    punpckhdq   xmm3,xmm0
    punpckldq   xmm1,xmm6
    movdqa      xmm0,xmm2
    punpcklqdq  xmm0,xmm1
    punpckhdq   xmm4,xmm6
    punpckhqdq  xmm2,xmm1
    movdqa      [rsp+10h],xmm0
    movdqa      [rsp+60h],xmm2
    movdqa      xmm0,xmm3
    mov         eax,[rsp+10h]
    mov         [rcx-2],eax
    mov         eax,[rsp+60h]
    punpcklqdq  xmm0,xmm4
    punpckhqdq  xmm3,xmm4
    mov         [r10+rcx-2],eax
    movdqa      [rsp+20h],xmm0
    mov         eax, [rsp+20h]
    movdqa      [rsp+70h],xmm3
    mov         [rcx+r10*2-2],eax
    mov         eax,[rsp+70h]
    mov         [rdx+rcx-2],eax
    mov         eax,[rsp+18h]
    mov         [r11],eax
    mov         eax,[rsp+68h]
    mov         [r10+r11],eax
    mov         eax,[rsp+28h]
    mov         [r11+r10*2],eax
    mov         eax,[rsp+78h]
    mov         [rdx+r11],eax
    mov         eax,[rsp+14h]
    mov         [rdi-2],eax
    mov         eax,[rsp+64h]
    mov         [r10+rdi-2],eax
    mov         eax,[rsp+24h]
    mov         [rdi+r10*2-2],eax
    mov         eax, [rsp+74h]
    mov         [rdx+rdi-2],eax
    mov         eax, [rsp+1Ch]
    mov         [rbx],eax
    mov         eax, [rsp+6Ch]
    mov         [r10+rbx],eax
    mov         eax,[rsp+2Ch]
    mov         [rbx+r10*2],eax
    mov         eax,[rsp+7Ch]
    mov         [rdx+rbx],eax
    lea         r11,[rsp+140h]
    mov         rbx, [r11+28h]
    mov         rsp,r11
    pop         r12
    pop         rbp
    pop         rbx
    ret


WELS_EXTERN DeblockChromaLt4H_ssse3
    mov         rax,rsp
    push        rbx
    push        rbp
    push        r12
    push        r13
    push        r14
    sub         rsp,170h

    mov         r13,   r8
    mov         r14,   r9
    mov         r8,    rdx
    mov         r9,    rcx
    mov         rdx,   rdi
    mov         rcx,   rsi

    movsxd      rsi,r8d
    lea         eax,[r8*4]
    mov         r11d,r9d
    movsxd      r10,eax
    mov         eax, [rcx-2]
    mov         r12,rdx
    mov         [rsp+40h],eax
    mov         eax, [rsi+rcx-2]
    lea         rbx,[r10+rcx-2]
    movdqa      xmm5,[rsp+40h]
    mov         [rsp+50h],eax
    mov         eax, [rcx+rsi*2-2]
    lea         rbp,[r10+rdx-2]
    movdqa      xmm2, [rsp+50h]
    mov         [rsp+60h],eax
    lea         r10,[rsi+rsi*2]
    mov         rdi,rcx
    mov         eax,[r10+rcx-2]
    movdqa      xmm4,[rsp+60h]
    mov         [rsp+70h],eax
    mov         eax,[rdx-2]
    mov         [rsp+80h],eax
    mov         eax, [rsi+rdx-2]
    movdqa      xmm3,[rsp+70h]
    mov         [rsp+90h],eax
    mov         eax,[rdx+rsi*2-2]
    punpckldq   xmm5,[rsp+80h]
    mov         [rsp+0A0h],eax
    mov         eax, [r10+rdx-2]
    punpckldq   xmm2,[rsp+90h]
    mov         [rsp+0B0h],eax
    mov         eax, [rbx]
    punpckldq   xmm4,[rsp+0A0h]
    mov         [rsp+80h],eax
    mov         eax,[rbp]
    punpckldq   xmm3,[rsp+0B0h]
    mov         [rsp+90h],eax
    mov         eax,[rsi+rbx]
    movdqa      xmm0,[rsp+80h]
    punpckldq   xmm0,[rsp+90h]
    punpcklqdq  xmm5,xmm0
    movdqa      [rsp+80h],xmm0
    mov         [rsp+80h],eax
    mov         eax,[rsi+rbp]
    movdqa      xmm0,[rsp+80h]
    movdqa      xmm1,xmm5
    mov         [rsp+90h],eax
    mov         eax,[rbx+rsi*2]
    punpckldq   xmm0,[rsp+90h]
    punpcklqdq  xmm2,xmm0
    punpcklbw   xmm1,xmm2
    punpckhbw   xmm5,xmm2
    movdqa      [rsp+80h],xmm0
    mov         [rsp+80h],eax
    mov         eax,[rbp+rsi*2]
    movdqa      xmm0, [rsp+80h]
    mov         [rsp+90h],eax
    mov         eax,[r10+rbx]
    movdqa      xmm7,xmm1
    punpckldq   xmm0,[rsp+90h]
    punpcklqdq  xmm4,xmm0
    movdqa      [rsp+80h],xmm0
    mov         [rsp+80h],eax
    mov         eax, [r10+rbp]
    movdqa      xmm0,[rsp+80h]
    mov         [rsp+90h],eax
    punpckldq   xmm0,[rsp+90h]
    punpcklqdq  xmm3,xmm0
    movdqa      xmm0,xmm4
    punpcklbw   xmm0,xmm3
    punpckhbw   xmm4,xmm3
    punpcklwd   xmm7,xmm0
    punpckhwd   xmm1,xmm0
    movdqa      xmm0,xmm5
    movdqa      xmm6,xmm7
    punpcklwd   xmm0,xmm4
    punpckhwd   xmm5,xmm4
    punpckldq   xmm6,xmm0
    punpckhdq   xmm7,xmm0
    movdqa      xmm0,xmm1
    punpckldq   xmm0,xmm5
    mov         rax, r14    ; pTC
    punpckhdq   xmm1,xmm5
    movdqa      xmm9,xmm6
    punpckhqdq  xmm6,xmm0
    punpcklqdq  xmm9,xmm0
    movdqa      xmm2,xmm7
    movdqa      xmm13,xmm6
    movdqa      xmm4,xmm9
    movdqa      [rsp+10h],xmm9
    punpcklqdq  xmm2,xmm1
    punpckhqdq  xmm7,xmm1
    pxor        xmm1,xmm1
    movsx       ecx,byte [rax+3]
    movsx       edx,byte [rax+2]
    movsx       r8d,byte [rax+1]
    movsx       r9d,byte [rax]
    movdqa      xmm10,xmm1
    movdqa      xmm15,xmm2
    punpckhbw   xmm2,xmm1
    punpckhbw   xmm6,xmm1
    punpcklbw   xmm4,xmm1
    movsx       eax,r11w
    mov         word [rsp+0Eh],cx
    mov         word [rsp+0Ch],cx
    movdqa      xmm3,xmm7
    movdqa      xmm8,xmm7
    movdqa      [rsp+20h],xmm7
    punpcklbw   xmm15,xmm1
    punpcklbw   xmm13,xmm1
    punpcklbw   xmm3,xmm1
    mov         word [rsp+0Ah],dx
    mov         word [rsp+8],dx
    mov         word [rsp+6],r8w
    movd        xmm0,eax
    movdqa      [rsp+30h],xmm6
    punpckhbw   xmm9,xmm1
    punpckhbw   xmm8,xmm1
    punpcklwd   xmm0,xmm0
    mov         eax, r13d   ; iBeta
    mov         word [rsp+4],r8w
    mov         word [rsp+2],r9w
    pshufd      xmm12,xmm0,0
    mov         word [rsp],r9w
    movd        xmm0,eax
    mov         eax,4
    cwde
    movdqa      xmm14, [rsp]
    movdqa      [rsp],xmm2
    movdqa      xmm2,xmm12
    punpcklwd   xmm0,xmm0
    pshufd      xmm11,xmm0,0
    psubw       xmm10,xmm14
    movd        xmm0,eax
    movdqa      xmm7,xmm14
    movdqa      xmm6,xmm14
    pcmpgtw     xmm7,xmm1
    punpcklwd   xmm0,xmm0
    pshufd      xmm5,xmm0,0
    movdqa      xmm0,xmm4
    movdqa      xmm1,xmm15
    psubw       xmm4,xmm13
    psubw       xmm0,xmm3
    psubw       xmm1,xmm13
    psubw       xmm3,xmm15
    psllw       xmm1,2
    paddw       xmm1,xmm0
    paddw       xmm1,xmm5
    movdqa      xmm0,xmm10
    psraw       xmm1,3
    pmaxsw      xmm0,xmm1
    pminsw      xmm6,xmm0
    movdqa      xmm1,xmm11
    movdqa      xmm0,xmm13
    psubw       xmm0,xmm15
    pabsw       xmm0,xmm0
    pcmpgtw     xmm2,xmm0
    pabsw       xmm0,xmm4
    pcmpgtw     xmm1,xmm0
    pabsw       xmm0,xmm3
    pand        xmm2,xmm1
    movdqa      xmm1,xmm11
    movdqa      xmm3,[rsp+30h]
    pcmpgtw     xmm1,xmm0
    movdqa      xmm0,xmm9
    pand        xmm2,xmm1
    psubw       xmm0,xmm8
    psubw       xmm9,xmm3
    pand        xmm2,xmm7
    pand        xmm6,xmm2
    psubw       xmm15,xmm6
    paddw       xmm13,xmm6
    movdqa      xmm2,[rsp]
    movdqa      xmm1,xmm2
    psubw       xmm1,xmm3
    psubw       xmm8,xmm2
    psllw       xmm1,2
    paddw       xmm1,xmm0
    paddw       xmm1,xmm5
    movdqa      xmm0,xmm3
    movdqa      xmm5,[rsp+10h]
    psubw       xmm0,xmm2
    psraw       xmm1,3
    movdqa      xmm4,xmm5
    pabsw       xmm0,xmm0
    pmaxsw      xmm10,xmm1
    movdqa      xmm1,xmm11
    pcmpgtw     xmm12,xmm0
    pabsw       xmm0,xmm9
    pminsw      xmm14,xmm10
    pcmpgtw     xmm1,xmm0
    pabsw       xmm0,xmm8
    pcmpgtw     xmm11,xmm0
    pand        xmm12,xmm1
    movdqa      xmm1,[rsp+20h]
    pand        xmm12,xmm11
    pand        xmm12,xmm7
    pand        xmm14,xmm12
    paddw       xmm3,xmm14
    psubw       xmm2,xmm14
    packuswb    xmm13,xmm3
    packuswb    xmm15,xmm2
    punpcklbw   xmm4,xmm13
    punpckhbw   xmm5,xmm13
    movdqa      xmm0,xmm15
    punpcklbw   xmm0,xmm1
    punpckhbw   xmm15,xmm1
    movdqa      xmm3,xmm4
    punpcklwd   xmm3,xmm0
    punpckhwd   xmm4,xmm0
    movdqa      xmm0,xmm5
    movdqa      xmm2,xmm3
    movdqa      xmm1,xmm4
    punpcklwd   xmm0,xmm15
    punpckhwd   xmm5,xmm15
    punpckldq   xmm2,xmm0
    punpckhdq   xmm3,xmm0
    punpckldq   xmm1,xmm5
    movdqa      xmm0,xmm2
    punpcklqdq  xmm0,xmm1
    punpckhdq   xmm4,xmm5
    punpckhqdq  xmm2,xmm1
    movdqa      [rsp+40h],xmm0
    movdqa      xmm0,xmm3
    movdqa      [rsp+90h],xmm2
    mov         eax,[rsp+40h]
    mov         [rdi-2],eax
    mov         eax, [rsp+90h]
    punpcklqdq  xmm0,xmm4
    punpckhqdq  xmm3,xmm4
    mov         [rsi+rdi-2],eax
    movdqa      [rsp+50h],xmm0
    mov         eax,[rsp+50h]
    movdqa      [rsp+0A0h],xmm3
    mov         [rdi+rsi*2-2],eax
    mov         eax,[rsp+0A0h]
    mov         [r10+rdi-2],eax
    mov         eax,[rsp+48h]
    mov         [rbx],eax
    mov         eax,[rsp+98h]
    mov         [rsi+rbx],eax
    mov         eax,[rsp+58h]
    mov         [rbx+rsi*2],eax
    mov         eax, [rsp+0A8h]
    mov         [r10+rbx],eax
    mov         eax, [rsp+44h]
    mov         [r12-2],eax
    mov         eax,[rsp+94h]
    mov         [rsi+r12-2],eax
    mov         eax,[rsp+54h]
    mov         [r12+rsi*2-2],eax
    mov         eax, [rsp+0A4h]
    mov         [r10+r12-2],eax
    mov         eax,[rsp+4Ch]
    mov         [rbp],eax
    mov         eax,[rsp+9Ch]
    mov         [rsi+rbp],eax
    mov         eax, [rsp+5Ch]
    mov         [rbp+rsi*2],eax
    mov         eax,[rsp+0ACh]
    mov         [r10+rbp],eax
    lea         r11,[rsp+170h]
    mov         rsp,r11
    pop         r14
    pop         r13
    pop         r12
    pop         rbp
    pop         rbx
    ret



%elifdef  X86_32

;********************************************************************************
;  void DeblockChromaEq4V_ssse3(uint8_t * pPixCb, uint8_t * pPixCr, int32_t iStride,
;                             int32_t iAlpha, int32_t iBeta)
;********************************************************************************
WELS_EXTERN DeblockChromaEq4V_ssse3
    push        ebp
    mov         ebp,esp
    and         esp,0FFFFFFF0h
    sub         esp,68h
    mov         edx,[ebp+10h]      ;  iStride
    mov         eax,[ebp+8]        ;  pPixCb
    mov         ecx,[ebp+0Ch]      ;  pPixCr
    movq        xmm4,[ecx]
    movq        xmm5,[edx+ecx]
    push        esi
    push        edi
    lea         esi,[edx+edx]
    mov         edi,eax
    sub         edi,esi
    movq        xmm1,[edi]
    mov         edi,ecx
    sub         edi,esi
    movq        xmm2,[edi]
    punpcklqdq  xmm1,xmm2
    mov         esi,eax
    sub         esi,edx
    movq        xmm2,[esi]
    mov         edi,ecx
    sub         edi,edx
    movq        xmm3,[edi]
    punpcklqdq  xmm2,xmm3
    movq        xmm3,[eax]
    punpcklqdq  xmm3,xmm4
    movq        xmm4,[edx+eax]
    mov       edx, [ebp + 14h]
    punpcklqdq  xmm4,xmm5
    movd        xmm5,edx
    mov       edx, [ebp + 18h]
    pxor        xmm0,xmm0
    movdqa      xmm6,xmm5
    punpcklwd   xmm6,xmm5
    pshufd      xmm5,xmm6,0
    movd        xmm6,edx
    movdqa      xmm7,xmm6
    punpcklwd   xmm7,xmm6
    pshufd      xmm6,xmm7,0
    movdqa      xmm7,xmm1
    punpckhbw   xmm1,xmm0
    punpcklbw   xmm7,xmm0
    movdqa      [esp+40h],xmm1
    movdqa      [esp+60h],xmm7
    movdqa      xmm7,xmm2
    punpcklbw   xmm7,xmm0
    movdqa      [esp+10h],xmm7
    movdqa      xmm7,xmm3
    punpcklbw   xmm7,xmm0
    punpckhbw   xmm3,xmm0
    movdqa      [esp+50h],xmm7
    movdqa      xmm7,xmm4
    punpckhbw   xmm4,xmm0
    punpckhbw   xmm2,xmm0
    punpcklbw   xmm7,xmm0
    movdqa      [esp+30h],xmm3
    movdqa      xmm3,[esp+10h]
    movdqa      xmm1,xmm3
    psubw       xmm1,[esp+50h]
    pabsw       xmm1,xmm1
    movdqa      [esp+20h],xmm4
    movdqa      xmm0,xmm5
    pcmpgtw     xmm0,xmm1
    movdqa      xmm1,[esp+60h]
    psubw       xmm1,xmm3
    pabsw       xmm1,xmm1
    movdqa      xmm4,xmm6
    pcmpgtw     xmm4,xmm1
    pand        xmm0,xmm4
    movdqa      xmm1,xmm7
    psubw       xmm1,[esp+50h]
    pabsw       xmm1,xmm1
    movdqa      xmm4,xmm6
    pcmpgtw     xmm4,xmm1
    movdqa      xmm1,xmm2
    psubw       xmm1,[esp+30h]
    pabsw       xmm1,xmm1
    pcmpgtw     xmm5,xmm1
    movdqa      xmm1,[esp+40h]
    pand        xmm0,xmm4
    psubw       xmm1,xmm2
    pabsw       xmm1,xmm1
    movdqa      xmm4,xmm6
    pcmpgtw     xmm4,xmm1
    movdqa      xmm1,[esp+20h]
    psubw       xmm1,[esp+30h]
    pand        xmm5,xmm4
    pabsw       xmm1,xmm1
    pcmpgtw     xmm6,xmm1
    pand        xmm5,xmm6
    mov         edx,2
    movsx       edx,dx
    movd        xmm1,edx
    movdqa      xmm4,xmm1
    punpcklwd   xmm4,xmm1
    pshufd      xmm1,xmm4,0
    movdqa      xmm4,[esp+60h]
    movdqa      xmm6,xmm4
    paddw       xmm6,xmm4
    paddw       xmm6,xmm3
    paddw       xmm6,xmm7
    movdqa      [esp+10h],xmm1
    paddw       xmm6,[esp+10h]
    psraw       xmm6,2
    movdqa      xmm4,xmm0
    pandn       xmm4,xmm3
    movdqa      xmm3,[esp+40h]
    movdqa      xmm1,xmm0
    pand        xmm1,xmm6
    por         xmm1,xmm4
    movdqa      xmm6,xmm3
    paddw       xmm6,xmm3
    movdqa      xmm3,[esp+10h]
    paddw       xmm6,xmm2
    paddw       xmm6,[esp+20h]
    paddw       xmm6,xmm3
    psraw       xmm6,2
    movdqa      xmm4,xmm5
    pand        xmm4,xmm6
    movdqa      xmm6,xmm5
    pandn       xmm6,xmm2
    por         xmm4,xmm6
    packuswb    xmm1,xmm4
    movdqa      xmm4,[esp+50h]
    movdqa      xmm6,xmm7
    paddw       xmm6,xmm7
    paddw       xmm6,xmm4
    paddw       xmm6,[esp+60h]
    paddw       xmm6,xmm3
    psraw       xmm6,2
    movdqa      xmm2,xmm0
    pand        xmm2,xmm6
    pandn       xmm0,xmm4
    por         xmm2,xmm0
    movdqa      xmm0,[esp+20h]
    movdqa      xmm6,xmm0
    paddw       xmm6,xmm0
    movdqa      xmm0,[esp+30h]
    paddw       xmm6,xmm0
    paddw       xmm6,[esp+40h]
    movdqa      xmm4,xmm5
    paddw       xmm6,xmm3
    movq        [esi],xmm1
    psraw       xmm6,2
    pand        xmm4,xmm6
    pandn       xmm5,xmm0
    por         xmm4,xmm5
    packuswb    xmm2,xmm4
    movq        [eax],xmm2
    psrldq      xmm1,8
    movq        [edi],xmm1
    pop         edi
    psrldq      xmm2,8
    movq        [ecx],xmm2
    pop         esi
    mov         esp,ebp
    pop         ebp
    ret

;******************************************************************************
; void DeblockChromaLt4V_ssse3(uint8_t * pPixCb, uint8_t * pPixCr, int32_t iStride,
;                           int32_t iAlpha, int32_t iBeta, int8_t * pTC);
;*******************************************************************************

WELS_EXTERN DeblockChromaLt4V_ssse3
    push        ebp
    mov         ebp,esp
    and         esp,0FFFFFFF0h
    sub         esp,0E4h
    push        ebx
    push        esi
    mov         esi, [ebp+1Ch]      ;  pTC
    movsx       ebx, byte [esi+2]
    push        edi
    movsx       di,byte [esi+3]
    mov         word [esp+0Ch],bx
    movsx       bx,byte  [esi+1]
    movsx       esi,byte  [esi]
    mov         word  [esp+0Eh],si
    movzx       esi,di
    movd        xmm1,esi
    movzx       esi,di
    movd        xmm2,esi
    mov         si,word  [esp+0Ch]
    mov         edx, [ebp + 10h]
    mov         eax, [ebp + 08h]
    movzx       edi,si
    movzx       esi,si
    mov         ecx, [ebp + 0Ch]
    movd        xmm4,esi
    movzx       esi,bx
    movd        xmm5,esi
    movd        xmm3,edi
    movzx       esi,bx
    movd        xmm6,esi
    mov         si,word [esp+0Eh]
    movzx       edi,si
    movzx       esi,si
    punpcklwd   xmm6,xmm2
    pxor        xmm0,xmm0
    movdqa      [esp+40h],xmm0
    movd        xmm7,edi
    movd        xmm0,esi
    lea         esi,[edx+edx]
    mov         edi,eax
    sub         edi,esi
    punpcklwd   xmm5,xmm1
    movdqa      xmm1,[esp+40h]
    punpcklwd   xmm0,xmm4
    movq        xmm4,[edx+ecx]
    punpcklwd   xmm7,xmm3
    movq        xmm3,[eax]
    punpcklwd   xmm0,xmm6
    movq        xmm6,[edi]
    punpcklwd   xmm7,xmm5
    punpcklwd   xmm0,xmm7
    mov         edi,ecx
    sub         edi,esi
    movdqa      xmm2,xmm1
    psubw       xmm2,xmm0
    movdqa      [esp+60h],xmm2
    movq        xmm2, [edi]
    punpcklqdq  xmm6,xmm2
    mov         esi,eax
    sub         esi,edx
    movq        xmm7,[esi]
    mov         edi,ecx
    sub         edi,edx
    movq        xmm2,[edi]
    punpcklqdq  xmm7,xmm2
    movq        xmm2,[ecx]
    punpcklqdq  xmm3,xmm2
    movq        xmm2,[edx+eax]
    movsx       edx,word [ebp + 14h]
    punpcklqdq  xmm2,xmm4
    movdqa      [esp+0E0h],xmm2
    movd        xmm2,edx
    movsx       edx,word [ebp + 18h]
    movdqa      xmm4,xmm2
    punpcklwd   xmm4,xmm2
    movd        xmm2,edx
    movdqa      xmm5,xmm2
    punpcklwd   xmm5,xmm2
    pshufd      xmm2,xmm5,0
    movdqa      [esp+50h],xmm2
    movdqa      xmm2,xmm6
    punpcklbw   xmm2,xmm1
    movdqa      [esp+0D0h],xmm3
    pshufd      xmm4,xmm4,0
    movdqa      [esp+30h],xmm2
    punpckhbw   xmm6,xmm1
    movdqa      [esp+80h],xmm6
    movdqa      xmm6,[esp+0D0h]
    punpckhbw   xmm6,xmm1
    movdqa      [esp+70h],xmm6
    movdqa      xmm6, [esp+0E0h]
    punpckhbw   xmm6,xmm1
    movdqa     [esp+90h],xmm6
    movdqa      xmm5, [esp+0E0h]
    movdqa      xmm2,xmm7
    punpckhbw   xmm7,xmm1
    punpcklbw   xmm5,xmm1
    movdqa       [esp+0A0h],xmm7
    punpcklbw   xmm3,xmm1
    mov         edx,4
    punpcklbw   xmm2,xmm1
    movsx       edx,dx
    movd        xmm6,edx
    movdqa      xmm7,xmm6
    punpcklwd   xmm7,xmm6
    pshufd      xmm6,xmm7,0
    movdqa      xmm7,[esp+30h]
    movdqa      [esp+20h],xmm6
    psubw       xmm7,xmm5
    movdqa      xmm6,xmm0
    pcmpgtw     xmm6,xmm1
    movdqa      xmm1,[esp+60h]
    movdqa      [esp+40h],xmm6
    movdqa      xmm6,xmm3
    psubw       xmm6,xmm2
    psllw       xmm6,2
    paddw       xmm6,xmm7
    paddw       xmm6, [esp+20h]
    movdqa      xmm7, [esp+50h]
    psraw       xmm6,3
    pmaxsw      xmm1,xmm6
    movdqa      [esp+10h],xmm0
    movdqa      xmm6, [esp+10h]
    pminsw      xmm6,xmm1
    movdqa      [esp+10h],xmm6
    movdqa      xmm1,xmm2
    psubw       xmm1,xmm3
    pabsw       xmm1,xmm1
    movdqa      xmm6,xmm4
    pcmpgtw     xmm6,xmm1
    movdqa      xmm1, [esp+30h]
    psubw       xmm1,xmm2
    pabsw       xmm1,xmm1
    pcmpgtw     xmm7,xmm1
    movdqa      xmm1,[esp+50h]
    pand        xmm6,xmm7
    movdqa      xmm7,[esp+50h]
    psubw       xmm5,xmm3
    pabsw       xmm5,xmm5
    pcmpgtw     xmm1,xmm5
    movdqa      xmm5,[esp+80h]
    psubw       xmm5,[esp+90h]
    pand        xmm6,xmm1
    pand        xmm6,[esp+40h]
    movdqa      xmm1,[esp+10h]
    pand        xmm1,xmm6
    movdqa      xmm6,[esp+70h]
    movdqa      [esp+30h],xmm1
    movdqa      xmm1,[esp+0A0h]
    psubw       xmm6,xmm1
    psllw       xmm6,2
    paddw       xmm6,xmm5
    paddw       xmm6,[esp+20h]
    movdqa      xmm5,[esp+60h]
    psraw       xmm6,3
    pmaxsw      xmm5,xmm6
    pminsw      xmm0,xmm5
    movdqa      xmm5,[esp+70h]
    movdqa      xmm6,xmm1
    psubw       xmm6,xmm5
    pabsw       xmm6,xmm6
    pcmpgtw     xmm4,xmm6
    movdqa      xmm6,[esp+80h]
    psubw       xmm6,xmm1
    pabsw       xmm6,xmm6
    pcmpgtw     xmm7,xmm6
    movdqa      xmm6,[esp+90h]
    pand        xmm4,xmm7
    movdqa      xmm7,[esp+50h]
    psubw       xmm6,xmm5
    pabsw       xmm6,xmm6
    pcmpgtw     xmm7,xmm6
    pand        xmm4,xmm7
    pand        xmm4,[esp+40h]
    pand        xmm0,xmm4
    movdqa      xmm4,[esp+30h]
    paddw       xmm2,xmm4
    paddw       xmm1,xmm0
    packuswb    xmm2,xmm1
    movq        [esi],xmm2
    psubw       xmm3,xmm4
    psubw       xmm5,xmm0
    packuswb    xmm3,xmm5
    movq        [eax],xmm3
    psrldq      xmm2,8
    movq        [edi],xmm2
    pop         edi
    pop         esi
    psrldq      xmm3,8
    movq        [ecx],xmm3
    pop         ebx
    mov         esp,ebp
    pop         ebp
    ret

;***************************************************************************
;  void DeblockChromaEq4H_ssse3(uint8_t * pPixCb, uint8_t * pPixCr, int32_t iStride,
;          int32_t iAlpha, int32_t iBeta)
;***************************************************************************

WELS_EXTERN DeblockChromaEq4H_ssse3
    push        ebp
    mov         ebp,esp
    and         esp,0FFFFFFF0h
    sub         esp,0C8h
    mov         ecx,dword [ebp+8]
    mov         edx,dword [ebp+0Ch]
    mov         eax,dword [ebp+10h]
    sub         ecx,2
    sub         edx,2
    push        esi
    lea         esi,[eax+eax*2]
    mov         dword [esp+18h],ecx
    mov         dword [esp+4],edx
    lea         ecx,[ecx+eax*4]
    lea         edx,[edx+eax*4]
    lea         eax,[esp+7Ch]
    push        edi
    mov         dword [esp+14h],esi
    mov         dword [esp+18h],ecx
    mov         dword [esp+0Ch],edx
    mov         dword [esp+10h],eax
    mov         esi,dword [esp+1Ch]
    mov         ecx,dword [ebp+10h]
    mov         edx,dword [esp+14h]
    movd        xmm0,dword [esi]
    movd        xmm1,dword [esi+ecx]
    movd        xmm2,dword [esi+ecx*2]
    movd        xmm3,dword [esi+edx]
    mov         esi,dword  [esp+8]
    movd        xmm4,dword [esi]
    movd        xmm5,dword [esi+ecx]
    movd        xmm6,dword [esi+ecx*2]
    movd        xmm7,dword [esi+edx]
    punpckldq   xmm0,xmm4
    punpckldq   xmm1,xmm5
    punpckldq   xmm2,xmm6
    punpckldq   xmm3,xmm7
    mov         esi,dword [esp+18h]
    mov         edi,dword [esp+0Ch]
    movd        xmm4,dword [esi]
    movd        xmm5,dword [edi]
    punpckldq   xmm4,xmm5
    punpcklqdq  xmm0,xmm4
    movd        xmm4,dword [esi+ecx]
    movd        xmm5,dword [edi+ecx]
    punpckldq   xmm4,xmm5
    punpcklqdq  xmm1,xmm4
    movd        xmm4,dword [esi+ecx*2]
    movd        xmm5,dword [edi+ecx*2]
    punpckldq   xmm4,xmm5
    punpcklqdq  xmm2,xmm4
    movd        xmm4,dword [esi+edx]
    movd        xmm5,dword [edi+edx]
    punpckldq   xmm4,xmm5
    punpcklqdq  xmm3,xmm4
    movdqa      xmm6,xmm0
    punpcklbw   xmm0,xmm1
    punpckhbw   xmm6,xmm1
    movdqa      xmm7,xmm2
    punpcklbw   xmm2,xmm3
    punpckhbw   xmm7,xmm3
    movdqa      xmm4,xmm0
    movdqa      xmm5,xmm6
    punpcklwd   xmm0,xmm2
    punpckhwd   xmm4,xmm2
    punpcklwd   xmm6,xmm7
    punpckhwd   xmm5,xmm7
    movdqa      xmm1,xmm0
    movdqa      xmm2,xmm4
    punpckldq   xmm0,xmm6
    punpckhdq   xmm1,xmm6
    punpckldq   xmm4,xmm5
    punpckhdq   xmm2,xmm5
    movdqa      xmm5,xmm0
    movdqa      xmm6,xmm1
    punpcklqdq  xmm0,xmm4
    punpckhqdq  xmm5,xmm4
    punpcklqdq  xmm1,xmm2
    punpckhqdq  xmm6,xmm2
    mov         edi,dword [esp+10h]
    movdqa      [edi],xmm0
    movdqa      [edi+10h],xmm5
    movdqa      [edi+20h],xmm1
    movdqa      [edi+30h],xmm6
    movsx       ecx,word [ebp+14h]
    movsx       edx,word [ebp+18h]
    movdqa      xmm6,[esp+80h]
    movdqa      xmm4,[esp+90h]
    movdqa      xmm5,[esp+0A0h]
    movdqa      xmm7,[esp+0B0h]
    pxor        xmm0,xmm0
    movd        xmm1,ecx
    movdqa      xmm2,xmm1
    punpcklwd   xmm2,xmm1
    pshufd      xmm1,xmm2,0
    movd        xmm2,edx
    movdqa      xmm3,xmm2
    punpcklwd   xmm3,xmm2
    pshufd      xmm2,xmm3,0
    movdqa      xmm3,xmm6
    punpckhbw   xmm6,xmm0
    movdqa      [esp+60h],xmm6
    movdqa      xmm6,[esp+90h]
    punpckhbw   xmm6,xmm0
    movdqa      [esp+30h],xmm6
    movdqa      xmm6,[esp+0A0h]
    punpckhbw   xmm6,xmm0
    movdqa      [esp+40h],xmm6
    movdqa      xmm6,[esp+0B0h]
    punpckhbw   xmm6,xmm0
    movdqa      [esp+70h],xmm6
    punpcklbw   xmm7,xmm0
    punpcklbw   xmm4,xmm0
    punpcklbw   xmm5,xmm0
    punpcklbw   xmm3,xmm0
    movdqa      [esp+50h],xmm7
    movdqa      xmm6,xmm4
    psubw       xmm6,xmm5
    pabsw       xmm6,xmm6
    movdqa      xmm0,xmm1
    pcmpgtw     xmm0,xmm6
    movdqa      xmm6,xmm3
    psubw       xmm6,xmm4
    pabsw       xmm6,xmm6
    movdqa      xmm7,xmm2
    pcmpgtw     xmm7,xmm6
    movdqa      xmm6,[esp+50h]
    psubw       xmm6,xmm5
    pabsw       xmm6,xmm6
    pand        xmm0,xmm7
    movdqa      xmm7,xmm2
    pcmpgtw     xmm7,xmm6
    movdqa      xmm6,[esp+30h]
    psubw       xmm6,[esp+40h]
    pabsw       xmm6,xmm6
    pcmpgtw     xmm1,xmm6
    movdqa      xmm6,[esp+60h]
    psubw       xmm6,[esp+30h]
    pabsw       xmm6,xmm6
    pand        xmm0,xmm7
    movdqa      xmm7,xmm2
    pcmpgtw     xmm7,xmm6
    movdqa      xmm6,[esp+70h]
    psubw       xmm6,[esp+40h]
    pabsw       xmm6,xmm6
    pand        xmm1,xmm7
    pcmpgtw     xmm2,xmm6
    pand        xmm1,xmm2
    mov         eax,2
    movsx       ecx,ax
    movd        xmm2,ecx
    movdqa      xmm6,xmm2
    punpcklwd   xmm6,xmm2
    pshufd      xmm2,xmm6,0
    movdqa      [esp+20h],xmm2
    movdqa      xmm2,xmm3
    paddw       xmm2,xmm3
    paddw       xmm2,xmm4
    paddw       xmm2,[esp+50h]
    paddw       xmm2,[esp+20h]
    psraw       xmm2,2
    movdqa      xmm6,xmm0
    pand        xmm6,xmm2
    movdqa      xmm2,xmm0
    pandn       xmm2,xmm4
    por         xmm6,xmm2
    movdqa      xmm2,[esp+60h]
    movdqa      xmm7,xmm2
    paddw       xmm7,xmm2
    paddw       xmm7,[esp+30h]
    paddw       xmm7,[esp+70h]
    paddw       xmm7,[esp+20h]
    movdqa      xmm4,xmm1
    movdqa      xmm2,xmm1
    pandn       xmm2,[esp+30h]
    psraw       xmm7,2
    pand        xmm4,xmm7
    por         xmm4,xmm2
    movdqa      xmm2,[esp+50h]
    packuswb    xmm6,xmm4
    movdqa      [esp+90h],xmm6
    movdqa      xmm6,xmm2
    paddw       xmm6,xmm2
    movdqa      xmm2,[esp+20h]
    paddw       xmm6,xmm5
    paddw       xmm6,xmm3
    movdqa      xmm4,xmm0
    pandn       xmm0,xmm5
    paddw       xmm6,xmm2
    psraw       xmm6,2
    pand        xmm4,xmm6
    por         xmm4,xmm0
    movdqa      xmm0,[esp+70h]
    movdqa      xmm5,xmm0
    paddw       xmm5,xmm0
    movdqa      xmm0,[esp+40h]
    paddw       xmm5,xmm0
    paddw       xmm5,[esp+60h]
    movdqa      xmm3,xmm1
    paddw       xmm5,xmm2
    psraw       xmm5,2
    pand        xmm3,xmm5
    pandn       xmm1,xmm0
    por         xmm3,xmm1
    packuswb    xmm4,xmm3
    movdqa      [esp+0A0h],xmm4
    mov         esi,dword [esp+10h]
    movdqa      xmm0,[esi]
    movdqa      xmm1,[esi+10h]
    movdqa      xmm2,[esi+20h]
    movdqa      xmm3,[esi+30h]
    movdqa      xmm6,xmm0
    punpcklbw   xmm0,xmm1
    punpckhbw   xmm6,xmm1
    movdqa      xmm7,xmm2
    punpcklbw   xmm2,xmm3
    punpckhbw   xmm7,xmm3
    movdqa      xmm4,xmm0
    movdqa      xmm5,xmm6
    punpcklwd   xmm0,xmm2
    punpckhwd   xmm4,xmm2
    punpcklwd   xmm6,xmm7
    punpckhwd   xmm5,xmm7
    movdqa      xmm1,xmm0
    movdqa      xmm2,xmm4
    punpckldq   xmm0,xmm6
    punpckhdq   xmm1,xmm6
    punpckldq   xmm4,xmm5
    punpckhdq   xmm2,xmm5
    movdqa      xmm5,xmm0
    movdqa      xmm6,xmm1
    punpcklqdq  xmm0,xmm4
    punpckhqdq  xmm5,xmm4
    punpcklqdq  xmm1,xmm2
    punpckhqdq  xmm6,xmm2
    mov         esi,dword [esp+1Ch]
    mov         ecx,dword [ebp+10h]
    mov         edx,dword [esp+14h]
    mov         edi,dword [esp+8]
    movd        dword [esi],xmm0
    movd        dword [esi+ecx],xmm5
    movd        dword [esi+ecx*2],xmm1
    movd        dword [esi+edx],xmm6
    psrldq      xmm0,4
    psrldq      xmm5,4
    psrldq      xmm1,4
    psrldq      xmm6,4
    mov         esi,dword [esp+18h]
    movd        dword [edi],xmm0
    movd        dword [edi+ecx],xmm5
    movd        dword [edi+ecx*2],xmm1
    movd        dword [edi+edx],xmm6
    psrldq      xmm0,4
    psrldq      xmm5,4
    psrldq      xmm1,4
    psrldq      xmm6,4
    movd        dword [esi],xmm0
    movd        dword [esi+ecx],xmm5
    movd        dword [esi+ecx*2],xmm1
    movd        dword [esi+edx],xmm6
    psrldq      xmm0,4
    psrldq      xmm5,4
    psrldq      xmm1,4
    psrldq      xmm6,4
    mov         edi,dword [esp+0Ch]
    movd        dword [edi],xmm0
    movd        dword [edi+ecx],xmm5
    movd        dword [edi+ecx*2],xmm1
    movd        dword [edi+edx],xmm6
    pop         edi
    pop         esi
    mov         esp,ebp
    pop         ebp
    ret

;*******************************************************************************
;    void DeblockChromaLt4H_ssse3(uint8_t * pPixCb, uint8_t * pPixCr, int32_t iStride,
;                                int32_t iAlpha, int32_t iBeta, int8_t * pTC);
;*******************************************************************************

WELS_EXTERN DeblockChromaLt4H_ssse3
    push        ebp
    mov         ebp,esp
    and         esp,0FFFFFFF0h
    sub         esp,108h
    mov         ecx,dword [ebp+8]
    mov         edx,dword [ebp+0Ch]
    mov         eax,dword [ebp+10h]
    sub         ecx,2
    sub         edx,2
    push        esi
    lea         esi,[eax+eax*2]
    mov         dword [esp+10h],ecx
    mov         dword [esp+4],edx
    lea         ecx,[ecx+eax*4]
    lea         edx,[edx+eax*4]
    lea         eax,[esp+6Ch]
    push        edi
    mov         dword [esp+0Ch],esi
    mov         dword [esp+18h],ecx
    mov         dword [esp+10h],edx
    mov         dword [esp+1Ch],eax
    mov         esi,dword [esp+14h]
    mov         ecx,dword [ebp+10h]
    mov         edx,dword [esp+0Ch]
    movd        xmm0,dword [esi]
    movd        xmm1,dword [esi+ecx]
    movd        xmm2,dword [esi+ecx*2]
    movd        xmm3,dword [esi+edx]
    mov         esi,dword [esp+8]
    movd        xmm4,dword [esi]
    movd        xmm5,dword [esi+ecx]
    movd        xmm6,dword [esi+ecx*2]
    movd        xmm7,dword [esi+edx]
    punpckldq   xmm0,xmm4
    punpckldq   xmm1,xmm5
    punpckldq   xmm2,xmm6
    punpckldq   xmm3,xmm7
    mov         esi,dword [esp+18h]
    mov         edi,dword [esp+10h]
    movd        xmm4,dword [esi]
    movd        xmm5,dword [edi]
    punpckldq   xmm4,xmm5
    punpcklqdq  xmm0,xmm4
    movd        xmm4,dword [esi+ecx]
    movd        xmm5,dword [edi+ecx]
    punpckldq   xmm4,xmm5
    punpcklqdq  xmm1,xmm4
    movd        xmm4,dword [esi+ecx*2]
    movd        xmm5,dword [edi+ecx*2]
    punpckldq   xmm4,xmm5
    punpcklqdq  xmm2,xmm4
    movd        xmm4,dword [esi+edx]
    movd        xmm5,dword [edi+edx]
    punpckldq   xmm4,xmm5
    punpcklqdq  xmm3,xmm4
    movdqa      xmm6,xmm0
    punpcklbw   xmm0,xmm1
    punpckhbw   xmm6,xmm1
    movdqa      xmm7,xmm2
    punpcklbw   xmm2,xmm3
    punpckhbw   xmm7,xmm3
    movdqa      xmm4,xmm0
    movdqa      xmm5,xmm6
    punpcklwd   xmm0,xmm2
    punpckhwd   xmm4,xmm2
    punpcklwd   xmm6,xmm7
    punpckhwd   xmm5,xmm7
    movdqa      xmm1,xmm0
    movdqa      xmm2,xmm4
    punpckldq   xmm0,xmm6
    punpckhdq   xmm1,xmm6
    punpckldq   xmm4,xmm5
    punpckhdq   xmm2,xmm5
    movdqa      xmm5,xmm0
    movdqa      xmm6,xmm1
    punpcklqdq  xmm0,xmm4
    punpckhqdq  xmm5,xmm4
    punpcklqdq  xmm1,xmm2
    punpckhqdq  xmm6,xmm2
    mov         edi,dword [esp+1Ch]
    movdqa      [edi],xmm0
    movdqa      [edi+10h],xmm5
    movdqa      [edi+20h],xmm1
    movdqa      [edi+30h],xmm6
    mov         eax,dword [ebp+1Ch]
    movsx       cx,byte [eax+3]
    movsx       dx,byte [eax+2]
    movsx       si,byte [eax+1]
    movsx       ax,byte [eax]
    movzx       edi,cx
    movzx       ecx,cx
    movd        xmm2,ecx
    movzx       ecx,dx
    movzx       edx,dx
    movd        xmm3,ecx
    movd        xmm4,edx
    movzx       ecx,si
    movzx       edx,si
    movd        xmm5,ecx
    pxor        xmm0,xmm0
    movd        xmm6,edx
    movzx       ecx,ax
    movdqa      [esp+60h],xmm0
    movzx       edx,ax
    movsx       eax,word [ebp+14h]
    punpcklwd   xmm6,xmm2
    movd        xmm1,edi
    movd        xmm7,ecx
    movsx       ecx,word [ebp+18h]
    movd        xmm0,edx
    punpcklwd   xmm7,xmm3
    punpcklwd   xmm5,xmm1
    movdqa      xmm1,[esp+60h]
    punpcklwd   xmm7,xmm5
    movdqa      xmm5,[esp+0A0h]
    punpcklwd   xmm0,xmm4
    punpcklwd   xmm0,xmm6
    movdqa      xmm6, [esp+70h]
    punpcklwd   xmm0,xmm7
    movdqa      xmm7,[esp+80h]
    movdqa      xmm2,xmm1
    psubw       xmm2,xmm0
    movdqa      [esp+0D0h],xmm2
    movd        xmm2,eax
    movdqa      xmm3,xmm2
    punpcklwd   xmm3,xmm2
    pshufd      xmm4,xmm3,0
    movd        xmm2,ecx
    movdqa      xmm3,xmm2
    punpcklwd   xmm3,xmm2
    pshufd      xmm2,xmm3,0
    movdqa      xmm3, [esp+90h]
    movdqa      [esp+50h],xmm2
    movdqa      xmm2,xmm6
    punpcklbw   xmm2,xmm1
    punpckhbw   xmm6,xmm1
    movdqa      [esp+40h],xmm2
    movdqa      [esp+0B0h],xmm6
    movdqa      xmm6,[esp+90h]
    movdqa      xmm2,xmm7
    punpckhbw   xmm7,xmm1
    punpckhbw   xmm6,xmm1
    punpcklbw   xmm2,xmm1
    punpcklbw   xmm3,xmm1
    punpcklbw   xmm5,xmm1
    movdqa      [esp+0F0h],xmm7
    movdqa      [esp+0C0h],xmm6
    movdqa      xmm6, [esp+0A0h]
    punpckhbw   xmm6,xmm1
    movdqa      [esp+0E0h],xmm6
    mov         edx,4
    movsx       eax,dx
    movd        xmm6,eax
    movdqa      xmm7,xmm6
    punpcklwd   xmm7,xmm6
    pshufd      xmm6,xmm7,0
    movdqa      [esp+30h],xmm6
    movdqa      xmm7, [esp+40h]
    psubw       xmm7,xmm5
    movdqa      xmm6,xmm0
    pcmpgtw     xmm6,xmm1
    movdqa      [esp+60h],xmm6
    movdqa      xmm1, [esp+0D0h]
    movdqa      xmm6,xmm3
    psubw       xmm6,xmm2
    psllw       xmm6,2
    paddw       xmm6,xmm7
    paddw       xmm6,[esp+30h]
    psraw       xmm6,3
    pmaxsw      xmm1,xmm6
    movdqa      xmm7,[esp+50h]
    movdqa      [esp+20h],xmm0
    movdqa      xmm6, [esp+20h]
    pminsw      xmm6,xmm1
    movdqa      [esp+20h],xmm6
    movdqa      xmm6,xmm4
    movdqa      xmm1,xmm2
    psubw       xmm1,xmm3
    pabsw       xmm1,xmm1
    pcmpgtw     xmm6,xmm1
    movdqa      xmm1, [esp+40h]
    psubw       xmm1,xmm2
    pabsw       xmm1,xmm1
    pcmpgtw     xmm7,xmm1
    movdqa      xmm1, [esp+50h]
    pand        xmm6,xmm7
    movdqa      xmm7, [esp+50h]
    psubw       xmm5,xmm3
    pabsw       xmm5,xmm5
    pcmpgtw     xmm1,xmm5
    movdqa      xmm5, [esp+0B0h]
    psubw       xmm5,[esp+0E0h]
    pand        xmm6,xmm1
    pand        xmm6, [esp+60h]
    movdqa      xmm1, [esp+20h]
    pand        xmm1,xmm6
    movdqa      xmm6, [esp+0C0h]
    movdqa      [esp+40h],xmm1
    movdqa      xmm1, [esp+0F0h]
    psubw       xmm6,xmm1
    psllw       xmm6,2
    paddw       xmm6,xmm5
    paddw       xmm6, [esp+30h]
    movdqa      xmm5, [esp+0D0h]
    psraw       xmm6,3
    pmaxsw      xmm5,xmm6
    pminsw      xmm0,xmm5
    movdqa      xmm5,[esp+0C0h]
    movdqa      xmm6,xmm1
    psubw       xmm6,xmm5
    pabsw       xmm6,xmm6
    pcmpgtw     xmm4,xmm6
    movdqa      xmm6,[esp+0B0h]
    psubw       xmm6,xmm1
    pabsw       xmm6,xmm6
    pcmpgtw     xmm7,xmm6
    movdqa      xmm6, [esp+0E0h]
    pand        xmm4,xmm7
    movdqa      xmm7, [esp+50h]
    psubw       xmm6,xmm5
    pabsw       xmm6,xmm6
    pcmpgtw     xmm7,xmm6
    pand        xmm4,xmm7
    pand        xmm4,[esp+60h]
    pand        xmm0,xmm4
    movdqa      xmm4, [esp+40h]
    paddw       xmm2,xmm4
    paddw       xmm1,xmm0
    psubw       xmm3,xmm4
    psubw       xmm5,xmm0
    packuswb    xmm2,xmm1
    packuswb    xmm3,xmm5
    movdqa      [esp+80h],xmm2
    movdqa      [esp+90h],xmm3
    mov         esi,dword [esp+1Ch]
    movdqa      xmm0, [esi]
    movdqa      xmm1, [esi+10h]
    movdqa      xmm2, [esi+20h]
    movdqa      xmm3, [esi+30h]
    movdqa      xmm6,xmm0
    punpcklbw   xmm0,xmm1
    punpckhbw   xmm6,xmm1
    movdqa      xmm7,xmm2
    punpcklbw   xmm2,xmm3
    punpckhbw   xmm7,xmm3
    movdqa      xmm4,xmm0
    movdqa      xmm5,xmm6
    punpcklwd   xmm0,xmm2
    punpckhwd   xmm4,xmm2
    punpcklwd   xmm6,xmm7
    punpckhwd   xmm5,xmm7
    movdqa      xmm1,xmm0
    movdqa      xmm2,xmm4
    punpckldq   xmm0,xmm6
    punpckhdq   xmm1,xmm6
    punpckldq   xmm4,xmm5
    punpckhdq   xmm2,xmm5
    movdqa      xmm5,xmm0
    movdqa      xmm6,xmm1
    punpcklqdq  xmm0,xmm4
    punpckhqdq  xmm5,xmm4
    punpcklqdq  xmm1,xmm2
    punpckhqdq  xmm6,xmm2
    mov         esi,dword [esp+14h]
    mov         ecx,dword [ebp+10h]
    mov         edx,dword [esp+0Ch]
    mov         edi,dword [esp+8]
    movd        dword [esi],xmm0
    movd        dword [esi+ecx],xmm5
    movd        dword [esi+ecx*2],xmm1
    movd        dword [esi+edx],xmm6
    psrldq      xmm0,4
    psrldq      xmm5,4
    psrldq      xmm1,4
    psrldq      xmm6,4
    mov         esi,dword [esp+18h]
    movd        dword [edi],xmm0
    movd        dword [edi+ecx],xmm5
    movd        dword [edi+ecx*2],xmm1
    movd        dword [edi+edx],xmm6
    psrldq      xmm0,4
    psrldq      xmm5,4
    psrldq      xmm1,4
    psrldq      xmm6,4
    movd        dword [esi],xmm0
    movd        dword [esi+ecx],xmm5
    movd        dword [esi+ecx*2],xmm1
    movd        dword [esi+edx],xmm6
    psrldq      xmm0,4
    psrldq      xmm5,4
    psrldq      xmm1,4
    psrldq      xmm6,4
    mov         edi,dword [esp+10h]
    movd        dword [edi],xmm0
    movd        dword [edi+ecx],xmm5
    movd        dword [edi+ecx*2],xmm1
    movd        dword [edi+edx],xmm6
    pop         edi
    pop         esi
    mov         esp,ebp
    pop         ebp
    ret


%endif



;********************************************************************************
;
;   void DeblockLumaTransposeH2V_sse2(uint8_t * pPixY, int32_t iStride, uint8_t * pDst);
;
;********************************************************************************

WELS_EXTERN DeblockLumaTransposeH2V_sse2
    push     r3
    push     r4
    push     r5

%assign   push_num   3
    LOAD_3_PARA
    PUSH_XMM 8

    SIGN_EXTENSION   r1, r1d

    mov      r5,    r7
    mov      r3,    r7
    and      r3,    0Fh
    sub      r7,    r3
    sub      r7,    10h

    lea      r3,    [r0 + r1 * 8]
    lea      r4,    [r1 * 3]

    movq    xmm0,  [r0]
    movq    xmm7,  [r3]
    punpcklqdq   xmm0,  xmm7
    movq    xmm1,  [r0 + r1]
    movq    xmm7,  [r3 + r1]
    punpcklqdq   xmm1,  xmm7
    movq    xmm2,  [r0 + r1*2]
    movq    xmm7,  [r3 + r1*2]
    punpcklqdq   xmm2,  xmm7
    movq    xmm3,  [r0 + r4]
    movq    xmm7,  [r3 + r4]
    punpcklqdq   xmm3,  xmm7

    lea     r0,   [r0 + r1 * 4]
    lea     r3,   [r3 + r1 * 4]
    movq    xmm4,  [r0]
    movq    xmm7,  [r3]
    punpcklqdq   xmm4,  xmm7
    movq    xmm5,  [r0 + r1]
    movq    xmm7,  [r3 + r1]
    punpcklqdq   xmm5,  xmm7
    movq    xmm6,  [r0 + r1*2]
    movq    xmm7,  [r3 + r1*2]
    punpcklqdq   xmm6,  xmm7

    movdqa  [r7],   xmm0
    movq    xmm7,  [r0 + r4]
    movq    xmm0,  [r3 + r4]
    punpcklqdq   xmm7,  xmm0
    movdqa  xmm0,   [r7]

    SSE2_TransTwo8x8B  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r7]
    ;pOut: m5, m3, m4, m8, m6, m2, m7, m1

    movdqa  [r2],    xmm4
    movdqa  [r2 + 10h],  xmm2
    movdqa  [r2 + 20h],  xmm3
    movdqa  [r2 + 30h],  xmm7
    movdqa  [r2 + 40h],  xmm5
    movdqa  [r2 + 50h],  xmm1
    movdqa  [r2 + 60h],  xmm6
    movdqa  [r2 + 70h],  xmm0

    mov     r7,   r5
    POP_XMM
    pop     r5
    pop     r4
    pop     r3
    ret


;*******************************************************************************************
;
;   void DeblockLumaTransposeV2H_sse2(uint8_t * pPixY, int32_t iStride, uint8_t * pSrc);
;
;*******************************************************************************************

WELS_EXTERN DeblockLumaTransposeV2H_sse2
    push     r3
    push     r4

%assign  push_num 2
    LOAD_3_PARA
    PUSH_XMM 8

    SIGN_EXTENSION   r1, r1d

    mov      r4,    r7
    mov      r3,    r7
    and      r3,    0Fh
    sub      r7,    r3
    sub      r7,    10h

    movdqa   xmm0,   [r2]
    movdqa   xmm1,   [r2 + 10h]
    movdqa   xmm2,   [r2 + 20h]
    movdqa   xmm3,   [r2 + 30h]
    movdqa   xmm4,   [r2 + 40h]
    movdqa   xmm5,   [r2 + 50h]
    movdqa   xmm6,   [r2 + 60h]
    movdqa   xmm7,   [r2 + 70h]

    SSE2_TransTwo8x8B  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r7]
    ;pOut: m5, m3, m4, m8, m6, m2, m7, m1

    lea      r2,   [r1 * 3]

    movq     [r0],  xmm4
    movq     [r0 + r1],  xmm2
    movq     [r0 + r1*2],  xmm3
    movq     [r0 + r2],  xmm7

    lea      r0,   [r0 + r1*4]
    movq     [r0],  xmm5
    movq     [r0 + r1],  xmm1
    movq     [r0 + r1*2],  xmm6
    movq     [r0 + r2],  xmm0

    psrldq    xmm4,   8
    psrldq    xmm2,   8
    psrldq    xmm3,   8
    psrldq    xmm7,   8
    psrldq    xmm5,   8
    psrldq    xmm1,   8
    psrldq    xmm6,   8
    psrldq    xmm0,   8

    lea       r0,  [r0 + r1*4]
    movq     [r0],  xmm4
    movq     [r0 + r1],  xmm2
    movq     [r0 + r1*2],  xmm3
    movq     [r0 + r2],  xmm7

    lea      r0,   [r0 + r1*4]
    movq     [r0],  xmm5
    movq     [r0 + r1],  xmm1
    movq     [r0 + r1*2],  xmm6
    movq     [r0 + r2],  xmm0


    mov      r7,   r4
    POP_XMM
    pop      r4
    pop      r3
    ret

WELS_EXTERN WelsNonZeroCount_sse2
    %assign  push_num 0
    LOAD_1_PARA
    movdqu  xmm0, [r0]
    movq    xmm1, [r0+16]
    WELS_DB1 xmm2
    pminub  xmm0, xmm2
    pminub  xmm1, xmm2
    movdqu  [r0], xmm0
    movq    [r0+16], xmm1
    ret
