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
;*  satd_sad.asm
;*
;*  Abstract
;*      WelsSampleSatd4x4_sse2
;*      WelsSampleSatd8x8_sse2
;*      WelsSampleSatd16x8_sse2
;*      WelsSampleSatd8x16_sse2
;*      WelsSampleSatd16x16_sse2
;*
;*      WelsSampleSad16x8_sse2
;*      WelsSampleSad16x16_sse2
;*
;*  History
;*      8/5/2009 Created
;*     24/9/2009 modified
;*
;*
;*************************************************************************/

%include "asm_inc.asm"

;***********************************************************************
; Data
;***********************************************************************
SECTION .rodata align=16

align 16
HSumSubDB1:   db 1,1,1,1,1,1,1,1,1,-1,1,-1,1,-1,1,-1
align 16
HSumSubDW1:   dw 1,-1,1,-1,1,-1,1,-1
align 16
PDW1:  dw 1,1,1,1,1,1,1,1
align 16
PDQ2:  dw 2,0,0,0,2,0,0,0
align 16
HSwapSumSubDB1:   times 2 db 1, 1, 1, 1, 1, -1, 1, -1

;***********************************************************************
; Code
;***********************************************************************
SECTION .text

;***********************************************************************
;
;Pixel_satd_wxh_sse2 BEGIN
;
;***********************************************************************
%macro MMX_DW_1_2REG 2
      pxor %1, %1
      pcmpeqw %2, %2
      psubw %1, %2
%endmacro

%macro  SSE2_SumWHorizon1 2
	movdqa      %2, %1
	psrldq      %2, 8
	paddusw     %1, %2
	movdqa      %2, %1
	psrldq      %2, 4
	paddusw     %1, %2
	movdqa      %2, %1
	psrldq      %2, 2
	paddusw     %1, %2
%endmacro

%macro SSE2_HDMTwo4x4 5 ;in: xmm1,xmm2,xmm3,xmm4  pOut: xmm4,xmm2,xmm1,xmm3
   SSE2_SumSub %1, %2, %5
   SSE2_SumSub %3, %4, %5
   SSE2_SumSub %2, %4, %5
   SSE2_SumSub %1, %3, %5
%endmacro

%macro SSE2_SumAbs4 7
	WELS_AbsW %1, %3
	WELS_AbsW %2, %3
	WELS_AbsW %4, %6
	WELS_AbsW %5, %6
	paddusw       %1, %2
	paddusw       %4, %5
	paddusw       %7, %1
	paddusw       %7, %4
%endmacro

%macro  SSE2_SumWHorizon 3
	movhlps		%2, %1			; x2 = xx xx xx xx d7 d6 d5 d4
	paddw		%1, %2			; x1 = xx xx xx xx d37 d26 d15 d04
	punpcklwd	%1, %3			; x1 =  d37  d26 d15 d04
	movhlps		%2, %1			; x2 = xxxx xxxx d37 d26
	paddd		%1, %2			; x1 = xxxx xxxx d1357 d0246
	pshuflw		%2, %1, 0x4e	; x2 = xxxx xxxx d0246 d1357
	paddd		%1, %2			; x1 = xxxx xxxx xxxx  d01234567
%endmacro

%macro SSE2_GetSatd8x8 0
	SSE2_LoadDiff8P    xmm0,xmm4,xmm7,[r0],[r2]
	SSE2_LoadDiff8P    xmm1,xmm5,xmm7,[r0+r1],[r2+r3]
	lea                 r0, [r0+2*r1]
	lea                 r2, [r2+2*r3]
	SSE2_LoadDiff8P    xmm2,xmm4,xmm7,[r0],[r2]
	SSE2_LoadDiff8P    xmm3,xmm5,xmm7,[r0+r1],[r2+r3]

	SSE2_HDMTwo4x4       xmm0,xmm1,xmm2,xmm3,xmm4
	SSE2_TransTwo4x4W     xmm3,xmm1,xmm0,xmm2,xmm4
	SSE2_HDMTwo4x4       xmm3,xmm1,xmm2,xmm4,xmm5
	SSE2_SumAbs4         xmm4,xmm1,xmm0,xmm2,xmm3,xmm5,xmm6

	lea					r0,    [r0+2*r1]
    lea					r2,    [r2+2*r3]
	SSE2_LoadDiff8P    xmm0,xmm4,xmm7,[r0],[r2]
	SSE2_LoadDiff8P    xmm1,xmm5,xmm7,[r0+r1],[r2+r3]
	lea                 r0, [r0+2*r1]
	lea                 r2, [r2+2*r3]
	SSE2_LoadDiff8P    xmm2,xmm4,xmm7,[r0],[r2]
	SSE2_LoadDiff8P    xmm3,xmm5,xmm7,[r0+r1],[r2+r3]

	SSE2_HDMTwo4x4       xmm0,xmm1,xmm2,xmm3,xmm4
	SSE2_TransTwo4x4W     xmm3,xmm1,xmm0,xmm2,xmm4
	SSE2_HDMTwo4x4       xmm3,xmm1,xmm2,xmm4,xmm5
	SSE2_SumAbs4         xmm4,xmm1,xmm0,xmm2,xmm3,xmm5,xmm6
%endmacro

;***********************************************************************
;
;int32_t WelsSampleSatd4x4_sse2( uint8_t *, int32_t, uint8_t *, int32_t );
;
;***********************************************************************
WELS_EXTERN WelsSampleSatd4x4_sse2
align 16
WelsSampleSatd4x4_sse2:
	;push      ebx
	;mov       eax,  [esp+8]
	;mov       ebx,  [esp+12]
	;mov       ecx,  [esp+16]
	;mov       edx,  [esp+20]

	%assign  push_num 0
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
    movd      xmm0, [r0]
    movd      xmm1, [r0+r1]
    lea       r0 , [r0+2*r1]
    movd      xmm2, [r0]
    movd      xmm3, [r0+r1]
    punpckldq xmm0, xmm2
    punpckldq xmm1, xmm3

    movd      xmm4, [r2]
    movd      xmm5, [r2+r3]
    lea       r2 , [r2+2*r3]
    movd      xmm6, [r2]
    movd      xmm7, [r2+r3]
    punpckldq xmm4, xmm6
    punpckldq xmm5, xmm7

    pxor      xmm6, xmm6
    punpcklbw xmm0, xmm6
    punpcklbw xmm1, xmm6
    punpcklbw xmm4, xmm6
    punpcklbw xmm5, xmm6

    psubw     xmm0, xmm4
    psubw     xmm1, xmm5

    movdqa    xmm2, xmm0
    paddw     xmm0, xmm1
    psubw     xmm2, xmm1
    SSE2_XSawp qdq, xmm0, xmm2, xmm3

    movdqa     xmm4, xmm0
    paddw      xmm0, xmm3
    psubw      xmm4, xmm3

    movdqa         xmm2, xmm0
    punpcklwd      xmm0, xmm4
    punpckhwd      xmm4, xmm2

	SSE2_XSawp     dq,  xmm0, xmm4, xmm3
	SSE2_XSawp     qdq, xmm0, xmm3, xmm5

    movdqa         xmm7, xmm0
    paddw          xmm0, xmm5
    psubw          xmm7, xmm5

	SSE2_XSawp     qdq,  xmm0, xmm7, xmm1

    movdqa         xmm2, xmm0
    paddw          xmm0, xmm1
    psubw          xmm2, xmm1

    WELS_AbsW  xmm0, xmm3
    paddusw        xmm6, xmm0
	WELS_AbsW  xmm2, xmm4
    paddusw        xmm6, xmm2
    SSE2_SumWHorizon1  xmm6, xmm4
	movd           retrd,  xmm6
    and            retrd,  0xffff
    shr            retrd,  1
	LOAD_4_PARA_POP
	ret

 ;***********************************************************************
 ;
 ;int32_t WelsSampleSatd8x8_sse2( uint8_t *, int32_t, uint8_t *, int32_t, );
 ;
 ;***********************************************************************
 WELS_EXTERN WelsSampleSatd8x8_sse2
align 16
 WelsSampleSatd8x8_sse2:
	 ;push   ebx
	 ;mov    eax,    [esp+8]
	 ;mov    ebx,    [esp+12]
	 ;mov    ecx,    [esp+16]
	 ;mov    edx,    [esp+20]

	%assign  push_num 0
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	pxor   xmm6,   xmm6
    pxor   xmm7,   xmm7
    SSE2_GetSatd8x8
    psrlw   xmm6,  1
	SSE2_SumWHorizon   xmm6,xmm4,xmm7
	movd    retrd,   xmm6
	LOAD_4_PARA_POP
	ret

 ;***********************************************************************
 ;
 ;int32_t WelsSampleSatd8x16_sse2( uint8_t *, int32_t, uint8_t *, int32_t, );
 ;
 ;***********************************************************************
 WELS_EXTERN WelsSampleSatd8x16_sse2
align 16
 WelsSampleSatd8x16_sse2:
	 ;push   ebx
	 ;mov    eax,    [esp+8]
	 ;mov    ebx,    [esp+12]
	 ;mov    ecx,    [esp+16]
	 ;mov    edx,    [esp+20]

	 %assign  push_num 0
	 LOAD_4_PARA
	 SIGN_EXTENTION r1, r1d
	 SIGN_EXTENTION r3, r3d
	 pxor   xmm6,   xmm6
     pxor   xmm7,   xmm7

	 SSE2_GetSatd8x8
     lea    r0,    [r0+2*r1]
     lea    r2,    [r2+2*r3]
	 SSE2_GetSatd8x8

	 psrlw   xmm6,  1
	 SSE2_SumWHorizon   xmm6,xmm4,xmm7
	 movd    retrd,   xmm6
	 LOAD_4_PARA_POP
	 ret

;***********************************************************************
;
;int32_t WelsSampleSatd16x8_sse2( uint8_t *, int32_t, uint8_t *, int32_t, );
;
;***********************************************************************
WELS_EXTERN WelsSampleSatd16x8_sse2
align 16
WelsSampleSatd16x8_sse2:
	;push   ebx
	;mov    eax,    [esp+8]
	;mov    ebx,    [esp+12]
	;mov    ecx,    [esp+16]
	;mov    edx,    [esp+20]

	%assign  push_num 0
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	push r0
	push r2
	pxor   xmm6,   xmm6
    pxor   xmm7,   xmm7

	SSE2_GetSatd8x8

	pop r2
	pop r0
	;mov    eax,    [esp+8]
    ;mov    ecx,    [esp+16]
    add    r0,    8
    add    r2,    8
	SSE2_GetSatd8x8

	psrlw   xmm6,  1
	SSE2_SumWHorizon   xmm6,xmm4,xmm7
	movd    retrd,   xmm6
	LOAD_4_PARA_POP
	ret

;***********************************************************************
;
;int32_t WelsSampleSatd16x16_sse2( uint8_t *, int32_t, uint8_t *, int32_t, );
;
;***********************************************************************
WELS_EXTERN WelsSampleSatd16x16_sse2
align 16
WelsSampleSatd16x16_sse2:
	;push   ebx
	;mov    eax,    [esp+8]
	;mov    ebx,    [esp+12]
	;mov    ecx,    [esp+16]
	;mov    edx,    [esp+20]

	%assign  push_num 0
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	push r0
	push r2
	pxor   xmm6,   xmm6
    pxor   xmm7,   xmm7

	SSE2_GetSatd8x8
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	SSE2_GetSatd8x8

	pop r2
	pop r0
	;mov    eax,    [esp+8]
	;mov    ecx,    [esp+16]
	add    r0,    8
	add    r2,    8

	SSE2_GetSatd8x8
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	SSE2_GetSatd8x8

 ; each column sum of SATD is necessarily even, so we don't lose any precision by shifting first.
    psrlw   xmm6,  1
	SSE2_SumWHorizon   xmm6,xmm4,xmm7
	movd    retrd,   xmm6
	LOAD_4_PARA_POP
	ret

;***********************************************************************
;
;Pixel_satd_wxh_sse2 END
;
;***********************************************************************

;***********************************************************************
;
;Pixel_satd_intra_sse2 BEGIN
;
;***********************************************************************

%macro SSE41_I16x16Get8WSumSub 3 ;xmm5 HSumSubDB1, xmm6 HSumSubDW1, xmm7 PDW1 : in %1, pOut %1, %3
	pmaddubsw    %1, xmm5
	movdqa       %2, %1
	pmaddwd      %1, xmm7
	pmaddwd      %2, xmm6
	movdqa       %3, %1
	punpckldq    %1, %2
	punpckhdq    %2, %3
	movdqa       %3, %1
	punpcklqdq   %1, %2
	punpckhqdq   %3, %2
	paddd        xmm4, %1 ;for dc
	paddd        xmm4, %3 ;for dc
	packssdw     %1, %3
	psllw        %1, 2
%endmacro
%macro SSE41_ChromaGet8WSumSub 4 ;xmm5 HSumSubDB1, xmm6 HSumSubDW1, xmm7 PDW1 : in %1, pOut %1, %3 : %4 tempsse2
	pmaddubsw    %1, xmm5
	movdqa       %2, %1
	pmaddwd      %1, xmm7
	pmaddwd      %2, xmm6
	movdqa       %3, %1
	punpckldq    %1, %2
	punpckhdq    %2, %3
	movdqa       %3, %1
	punpcklqdq   %1, %2
	punpckhqdq   %3, %2
;    paddd        xmm4, %1 ;for dc
;	 paddd        xmm4, %3 ;for dc
	movdqa       %4, %1
	punpcklqdq   %4, %3
	packssdw     %1, %3
	psllw        %1, 2
%endmacro

%macro SSE41_GetX38x4SatdDec 0
	pxor        xmm7,   xmm7
	movq        xmm0,   [eax]
	movq        xmm1,   [eax+ebx]
	lea         eax,    [eax+2*ebx]
	movq        xmm2,   [eax]
	movq        xmm3,   [eax+ebx]
	lea         eax,    [eax+2*ebx]
	punpcklbw   xmm0,   xmm7
	punpcklbw   xmm1,   xmm7
	punpcklbw   xmm2,   xmm7
	punpcklbw   xmm3,   xmm7
	SSE2_HDMTwo4x4       xmm0,xmm1,xmm2,xmm3,xmm7
	SSE2_TransTwo4x4W     xmm3,xmm1,xmm0,xmm2,xmm7
	SSE2_HDMTwo4x4       xmm3,xmm1,xmm2,xmm7,xmm0 ;pOut xmm7,xmm1,xmm3,xmm2
	;doesn't need another transpose
%endmacro
%macro SSE41_GetX38x4SatdV 2
	pxor        xmm0,   xmm0
	pinsrw      xmm0,   word[esi+%2],   0
	pinsrw      xmm0,   word[esi+%2+8], 4
	psubsw      xmm0,   xmm7
	pabsw       xmm0,   xmm0
	paddw       xmm4,   xmm0
	pxor        xmm0,   xmm0
	pinsrw      xmm0,   word[esi+%2+2],  0
	pinsrw      xmm0,   word[esi+%2+10], 4
	psubsw      xmm0,   xmm1
	pabsw       xmm0,   xmm0
	paddw       xmm4,   xmm0
	pxor        xmm0,   xmm0
	pinsrw      xmm0,   word[esi+%2+4],  0
	pinsrw      xmm0,   word[esi+%2+12], 4
	psubsw      xmm0,   xmm3
	pabsw       xmm0,   xmm0
	paddw       xmm4,   xmm0
	pxor        xmm0,   xmm0
	pinsrw      xmm0,   word[esi+%2+6],  0
	pinsrw      xmm0,   word[esi+%2+14], 4
	psubsw      xmm0,   xmm2
	pabsw       xmm0,   xmm0
	paddw       xmm4,   xmm0
%endmacro
%macro SSE41_GetX38x4SatdH  3
	movq        xmm0,   [esi+%3+8*%1]
	punpcklqdq  xmm0,   xmm0
	psubsw      xmm0,   xmm7
	pabsw       xmm0,   xmm0
	paddw       xmm5,   xmm0
	pabsw       xmm1,   xmm1
	pabsw       xmm2,   xmm2
	pabsw       xmm3,   xmm3
	paddw       xmm2,   xmm1;for DC
	paddw       xmm2,   xmm3;for DC
	paddw       xmm5,   xmm2
%endmacro
%macro SSE41_I16X16GetX38x4SatdDC 0
	pxor        xmm0,   xmm0
	movq2dq     xmm0,   mm4
	punpcklqdq  xmm0,   xmm0
	psubsw      xmm0,   xmm7
	pabsw       xmm0,   xmm0
	paddw       xmm6,   xmm0
	paddw       xmm6,   xmm2
%endmacro
%macro SSE41_ChromaGetX38x4SatdDC 1
	shl         %1,     4
	movdqa      xmm0,   [esi+32+%1]
	psubsw      xmm0,   xmm7
	pabsw       xmm0,   xmm0
	paddw       xmm6,   xmm0
	paddw       xmm6,   xmm2
%endmacro
%macro SSE41_I16x16GetX38x4Satd 2
	SSE41_GetX38x4SatdDec
	SSE41_GetX38x4SatdV   %1, %2
	SSE41_GetX38x4SatdH   %1, %2, 32
	SSE41_I16X16GetX38x4SatdDC
%endmacro
%macro SSE41_ChromaGetX38x4Satd 2
	SSE41_GetX38x4SatdDec
	SSE41_GetX38x4SatdV   %1, %2
	SSE41_GetX38x4SatdH   %1, %2, 16
	SSE41_ChromaGetX38x4SatdDC %1
%endmacro
%macro SSE41_HSum8W 3
	pmaddwd     %1, %2
	movhlps     %3, %1
	paddd       %1, %3
	pshuflw     %3, %1,0Eh
	paddd       %1, %3
%endmacro


%ifdef X86_32
WELS_EXTERN WelsIntra16x16Combined3Satd_sse41
WelsIntra16x16Combined3Satd_sse41:
	push   ebx
	push   esi
	push   edi
	mov    ecx,    [esp+16]
	mov    edx,    [esp+20]
	mov    eax,    [esp+24]
	mov    ebx,    [esp+28]
	mov    esi,    [esp+40] ;temp_satd
	pxor        xmm4,   xmm4
	movdqa      xmm5,   [HSumSubDB1]
	movdqa      xmm6,   [HSumSubDW1]
	movdqa      xmm7,   [PDW1]
	sub         ecx,    edx
	movdqu 		xmm0,   [ecx]
	movhlps		xmm1,   xmm0
	punpcklqdq  xmm0,   xmm0
	punpcklqdq  xmm1,   xmm1
	SSE41_I16x16Get8WSumSub xmm0, xmm2, xmm3
	SSE41_I16x16Get8WSumSub xmm1, xmm2, xmm3
	movdqa      [esi],  xmm0 ;V
	movdqa      [esi+16], xmm1
	add         ecx,    edx
	pinsrb      xmm0,   byte[ecx-1], 0
	pinsrb      xmm0,   byte[ecx+edx-1], 1
	lea         ecx,    [ecx+2*edx]
	pinsrb      xmm0,   byte[ecx-1],     2
	pinsrb      xmm0,   byte[ecx+edx-1], 3
	lea         ecx,    [ecx+2*edx]
	pinsrb      xmm0,   byte[ecx-1],     4
	pinsrb      xmm0,   byte[ecx+edx-1], 5
	lea         ecx,    [ecx+2*edx]
	pinsrb      xmm0,   byte[ecx-1],     6
	pinsrb      xmm0,   byte[ecx+edx-1], 7
	lea         ecx,    [ecx+2*edx]
	pinsrb      xmm0,   byte[ecx-1],     8
	pinsrb      xmm0,   byte[ecx+edx-1], 9
	lea         ecx,    [ecx+2*edx]
	pinsrb      xmm0,   byte[ecx-1],     10
	pinsrb      xmm0,   byte[ecx+edx-1], 11
	lea         ecx,    [ecx+2*edx]
	pinsrb      xmm0,   byte[ecx-1],     12
	pinsrb      xmm0,   byte[ecx+edx-1], 13
	lea         ecx,    [ecx+2*edx]
	pinsrb      xmm0,   byte[ecx-1],     14
	pinsrb      xmm0,   byte[ecx+edx-1], 15
	movhlps		xmm1,   xmm0
	punpcklqdq  xmm0,   xmm0
	punpcklqdq  xmm1,   xmm1
	SSE41_I16x16Get8WSumSub xmm0, xmm2, xmm3
	SSE41_I16x16Get8WSumSub xmm1, xmm2, xmm3
	movdqa      [esi+32], xmm0 ;H
	movdqa      [esi+48], xmm1
	movd        ecx,    xmm4 ;dc
	add         ecx,    16   ;(sum+16)
	shr         ecx,    5    ;((sum+16)>>5)
	shl         ecx,    4    ;
	movd        mm4,    ecx  ; mm4 copy DC
	pxor        xmm4,   xmm4 ;V
	pxor        xmm5,   xmm5 ;H
	pxor        xmm6,   xmm6 ;DC
	mov         ecx,    0
	mov         edi,    0
.loop16x16_get_satd:
.loopStart1:
	SSE41_I16x16GetX38x4Satd ecx, edi
	inc          ecx
	cmp         ecx, 4
	jl          .loopStart1
	cmp         edi, 16
	je          .loop16x16_get_satd_end
	mov         eax, [esp+24]
	add         eax, 8
	mov         ecx, 0
	add         edi, 16
	jmp         .loop16x16_get_satd
 .loop16x16_get_satd_end:
	MMX_DW_1_2REG    xmm0, xmm1
	psrlw       xmm4, 1 ;/2
	psrlw       xmm5, 1 ;/2
	psrlw       xmm6, 1 ;/2
	SSE41_HSum8W     xmm4, xmm0, xmm1
	SSE41_HSum8W     xmm5, xmm0, xmm1
	SSE41_HSum8W     xmm6, xmm0, xmm1

	; comparing order: DC H V
	movd      ebx, xmm6 ;DC
	movd      edi, xmm5 ;H
	movd      ecx, xmm4 ;V
	mov      edx, [esp+36]
	shl       edx, 1
	add       edi, edx
	add       ebx, edx
	mov       edx, [esp+32]
	cmp       ebx, edi
	jge near   not_dc_16x16
	cmp        ebx, ecx
	jge near   not_dc_h_16x16

	; for DC mode
	mov       dword[edx], 2;I16_PRED_DC
	mov       eax, ebx
	jmp near return_satd_intra_16x16_x3
not_dc_16x16:
	; for H mode
	cmp       edi, ecx
	jge near   not_dc_h_16x16
	mov       dword[edx], 1;I16_PRED_H
	mov       eax, edi
	jmp near return_satd_intra_16x16_x3
not_dc_h_16x16:
	; for V mode
	mov       dword[edx], 0;I16_PRED_V
	mov       eax, ecx
return_satd_intra_16x16_x3:
	WELSEMMS
	pop         edi
	pop         esi
	pop         ebx
ret

%macro SSE41_ChromaGetX38x8Satd 0
	movdqa      xmm5,   [HSumSubDB1]
	movdqa      xmm6,   [HSumSubDW1]
	movdqa      xmm7,   [PDW1]
	sub         ecx,    edx
	movq 		xmm0,   [ecx]
	punpcklqdq  xmm0,   xmm0
	SSE41_ChromaGet8WSumSub xmm0, xmm2, xmm3, xmm4
	movdqa      [esi],  xmm0 ;V
	add         ecx,    edx
	pinsrb      xmm0,   byte[ecx-1], 0
	pinsrb      xmm0,   byte[ecx+edx-1], 1
	lea         ecx,    [ecx+2*edx]
	pinsrb      xmm0,   byte[ecx-1],     2
	pinsrb      xmm0,   byte[ecx+edx-1], 3
	lea         ecx,    [ecx+2*edx]
	pinsrb      xmm0,   byte[ecx-1],     4
	pinsrb      xmm0,   byte[ecx+edx-1], 5
	lea         ecx,    [ecx+2*edx]
	pinsrb      xmm0,   byte[ecx-1],     6
	pinsrb      xmm0,   byte[ecx+edx-1], 7
	punpcklqdq  xmm0,   xmm0
	SSE41_ChromaGet8WSumSub xmm0, xmm2, xmm3, xmm1
	movdqa      [esi+16], xmm0 ;H
;(sum+2)>>2
	movdqa      xmm6,   [PDQ2]
	movdqa      xmm5,   xmm4
	punpckhqdq  xmm5,   xmm1
	paddd       xmm5,   xmm6
	psrld       xmm5,   2
;(sum1+sum2+4)>>3
	paddd       xmm6,   xmm6
	paddd       xmm4,   xmm1
	paddd       xmm4,   xmm6
	psrld       xmm4,   3
;satd *16
	pslld       xmm5,   4
	pslld       xmm4,   4
;temp satd
	movdqa      xmm6,   xmm4
	punpcklqdq  xmm4,   xmm5
	psllq       xmm4,   32
	psrlq       xmm4,   32
	movdqa      [esi+32], xmm4
	punpckhqdq  xmm5,   xmm6
	psllq       xmm5,   32
	psrlq       xmm5,   32
	movdqa      [esi+48], xmm5

	pxor        xmm4,   xmm4 ;V
	pxor        xmm5,   xmm5 ;H
	pxor        xmm6,   xmm6 ;DC
	mov         ecx,    0
loop_chroma_satdx3_cb_cr:
	SSE41_ChromaGetX38x4Satd ecx, 0
	inc             ecx
	cmp             ecx, 2
	jl              loop_chroma_satdx3_cb_cr
%endmacro

%macro SSEReg2MMX 3
	movdq2q     %2, %1
	movhlps     %1, %1
	movdq2q     %3, %1
%endmacro
%macro MMXReg2SSE 4
	movq2dq     %1, %3
	movq2dq     %2, %4
	punpcklqdq  %1, %2
%endmacro
;for reduce the code size of WelsIntraChroma8x8Combined3Satd_sse41

WELS_EXTERN WelsIntraChroma8x8Combined3Satd_sse41
WelsIntraChroma8x8Combined3Satd_sse41:
	push   ebx
	push   esi
	push   edi
	mov    ecx,    [esp+16]
	mov    edx,    [esp+20]
	mov    eax,    [esp+24]
	mov    ebx,    [esp+28]
	mov    esi,    [esp+40] ;temp_satd
	xor    edi,    edi
loop_chroma_satdx3:
	SSE41_ChromaGetX38x8Satd
	cmp             edi, 1
	je              loop_chroma_satdx3end
	inc             edi
	SSEReg2MMX  xmm4, mm0,mm1
	SSEReg2MMX  xmm5, mm2,mm3
	SSEReg2MMX  xmm6, mm5,mm6
	mov         ecx,  [esp+44]
	mov         eax,  [esp+48]
	jmp         loop_chroma_satdx3
loop_chroma_satdx3end:
	MMXReg2SSE  xmm0, xmm3, mm0, mm1
	MMXReg2SSE  xmm1, xmm3, mm2, mm3
	MMXReg2SSE  xmm2, xmm3, mm5, mm6

	paddw       xmm4, xmm0
	paddw       xmm5, xmm1
	paddw       xmm6, xmm2

	MMX_DW_1_2REG    xmm0, xmm1
	psrlw       xmm4, 1 ;/2
	psrlw       xmm5, 1 ;/2
	psrlw       xmm6, 1 ;/2
	SSE41_HSum8W     xmm4, xmm0, xmm1
	SSE41_HSum8W     xmm5, xmm0, xmm1
	SSE41_HSum8W     xmm6, xmm0, xmm1
	; comparing order: DC H V
	movd      ebx, xmm6 ;DC
	movd      edi, xmm5 ;H
	movd      ecx, xmm4 ;V
	mov       edx, [esp+36]
	shl       edx, 1
	add       edi, edx
	add       ecx, edx
	mov       edx, [esp+32]
	cmp       ebx, edi
	jge near   not_dc_8x8
	cmp        ebx, ecx
	jge near   not_dc_h_8x8

	; for DC mode
	mov       dword[edx], 0;I8_PRED_DC
	mov       eax, ebx
	jmp near return_satd_intra_8x8_x3
not_dc_8x8:
	; for H mode
	cmp       edi, ecx
	jge near   not_dc_h_8x8
	mov       dword[edx], 1;I8_PRED_H
	mov       eax, edi
	jmp near return_satd_intra_8x8_x3
not_dc_h_8x8:
	; for V mode
	mov       dword[edx], 2;I8_PRED_V
	mov       eax, ecx
return_satd_intra_8x8_x3:
	WELSEMMS
	pop         edi
	pop         esi
	pop         ebx
ret


;***********************************************************************
;
;Pixel_satd_intra_sse2 END
;
;***********************************************************************
%macro SSSE3_Get16BSadHVDC 2
  movd        xmm6,%1
  pshufb      xmm6,xmm1
  movdqa      %1,  xmm6
  movdqa      xmm0,%2
  psadbw      xmm0,xmm7
  paddw       xmm4,xmm0
  movdqa      xmm0,%2
  psadbw      xmm0,xmm5
  paddw       xmm2,xmm0
  psadbw      xmm6,%2
  paddw       xmm3,xmm6
%endmacro
%macro WelsAddDCValue 4
    movzx   %2, byte %1
    mov    %3, %2
    add     %4, %2
%endmacro

;***********************************************************************
;
;Pixel_sad_intra_ssse3 BEGIN
;
;***********************************************************************
WELS_EXTERN WelsIntra16x16Combined3Sad_ssse3
WelsIntra16x16Combined3Sad_ssse3:
	push   ebx
	push   esi
	push   edi
	mov    ecx,    [esp+16]
	mov    edx,    [esp+20]
	mov    edi,    [esp+40] ;temp_sad
	sub    ecx,    edx
    movdqa      xmm5,[ecx]
    pxor        xmm0,xmm0
    psadbw      xmm0,xmm5
    movhlps     xmm1,xmm0
    paddw       xmm0,xmm1
    movd        eax,xmm0

    add         ecx,edx
    lea         ebx, [edx+2*edx]
    WelsAddDCValue [ecx-1      ], esi, [edi   ], eax
    WelsAddDCValue [ecx-1+edx  ], esi, [edi+16], eax
    WelsAddDCValue [ecx-1+edx*2], esi, [edi+32], eax
    WelsAddDCValue [ecx-1+ebx  ], esi, [edi+48], eax
    lea         ecx, [ecx+4*edx]
    add         edi, 64
    WelsAddDCValue [ecx-1      ], esi, [edi   ], eax
    WelsAddDCValue [ecx-1+edx  ], esi, [edi+16], eax
    WelsAddDCValue [ecx-1+edx*2], esi, [edi+32], eax
    WelsAddDCValue [ecx-1+ebx  ], esi, [edi+48], eax
    lea         ecx, [ecx+4*edx]
    add         edi, 64
    WelsAddDCValue [ecx-1      ], esi, [edi   ], eax
    WelsAddDCValue [ecx-1+edx  ], esi, [edi+16], eax
    WelsAddDCValue [ecx-1+edx*2], esi, [edi+32], eax
    WelsAddDCValue [ecx-1+ebx  ], esi, [edi+48], eax
    lea         ecx, [ecx+4*edx]
    add         edi, 64
    WelsAddDCValue [ecx-1      ], esi, [edi   ], eax
    WelsAddDCValue [ecx-1+edx  ], esi, [edi+16], eax
    WelsAddDCValue [ecx-1+edx*2], esi, [edi+32], eax
    WelsAddDCValue [ecx-1+ebx  ], esi, [edi+48], eax
    sub        edi, 192
    add         eax,10h
    shr         eax,5
    movd        xmm7,eax
    pxor        xmm1,xmm1
    pshufb      xmm7,xmm1
    pxor        xmm4,xmm4
    pxor        xmm3,xmm3
    pxor        xmm2,xmm2
;sad begin
	mov    eax,    [esp+24]
	mov    ebx,    [esp+28]
    lea         esi, [ebx+2*ebx]
    SSSE3_Get16BSadHVDC [edi], [eax]
    SSSE3_Get16BSadHVDC [edi+16], [eax+ebx]
    SSSE3_Get16BSadHVDC [edi+32], [eax+2*ebx]
    SSSE3_Get16BSadHVDC [edi+48], [eax+esi]
    add         edi, 64
    lea         eax, [eax+4*ebx]
    SSSE3_Get16BSadHVDC [edi], [eax]
    SSSE3_Get16BSadHVDC [edi+16], [eax+ebx]
    SSSE3_Get16BSadHVDC [edi+32], [eax+2*ebx]
    SSSE3_Get16BSadHVDC [edi+48], [eax+esi]
    add         edi, 64
    lea         eax, [eax+4*ebx]
    SSSE3_Get16BSadHVDC [edi], [eax]
    SSSE3_Get16BSadHVDC [edi+16], [eax+ebx]
    SSSE3_Get16BSadHVDC [edi+32], [eax+2*ebx]
    SSSE3_Get16BSadHVDC [edi+48], [eax+esi]
    add         edi, 64
    lea         eax, [eax+4*ebx]
    SSSE3_Get16BSadHVDC [edi], [eax]
    SSSE3_Get16BSadHVDC [edi+16], [eax+ebx]
    SSSE3_Get16BSadHVDC [edi+32], [eax+2*ebx]
    SSSE3_Get16BSadHVDC [edi+48], [eax+esi]

    pslldq      xmm3,4
    por         xmm3,xmm2
    movhlps     xmm1,xmm3
    paddw       xmm3,xmm1
    movhlps     xmm0,xmm4
    paddw       xmm4,xmm0
; comparing order: DC H V
	movd        ebx, xmm4 ;DC
	movd        ecx, xmm3 ;V
	psrldq      xmm3, 4
	movd        esi, xmm3 ;H
	mov         eax, [esp+36] ;lamda
	shl         eax, 1
	add         esi, eax
	add         ebx, eax
	mov         edx, [esp+32]
	cmp         ebx, esi
	jge near   not_dc_16x16_sad
	cmp        ebx, ecx
	jge near   not_dc_h_16x16_sad
	; for DC mode
	mov       dword[edx], 2;I16_PRED_DC
	mov       eax, ebx
    sub        edi, 192
%assign x 0
%rep 16
    movdqa    [edi+16*x], xmm7
%assign x x+1
%endrep
	jmp near return_sad_intra_16x16_x3
not_dc_16x16_sad:
	; for H mode
	cmp       esi, ecx
	jge near   not_dc_h_16x16_sad
	mov       dword[edx], 1;I16_PRED_H
	mov       eax, esi
	jmp near return_sad_intra_16x16_x3
not_dc_h_16x16_sad:
	; for V mode
	mov       dword[edx], 0;I16_PRED_V
	mov       eax, ecx
    sub       edi, 192
%assign x 0
%rep 16
    movdqa    [edi+16*x], xmm5
%assign x x+1
%endrep
return_sad_intra_16x16_x3:
	pop    edi
	pop    esi
	pop    ebx
	ret
%endif
;***********************************************************************
;
;Pixel_sad_intra_ssse3 END
;
;***********************************************************************
;***********************************************************************
;
;Pixel_satd_wxh_sse41 BEGIN
;
;***********************************************************************

;SSE4.1
%macro SSE41_GetSatd8x4 0
	movq             xmm0, [r0]
	punpcklqdq       xmm0, xmm0
	pmaddubsw        xmm0, xmm7
	movq             xmm1, [r0+r1]
	punpcklqdq       xmm1, xmm1
	pmaddubsw        xmm1, xmm7
	movq             xmm2, [r2]
	punpcklqdq       xmm2, xmm2
	pmaddubsw        xmm2, xmm7
	movq             xmm3, [r2+r3]
	punpcklqdq       xmm3, xmm3
	pmaddubsw        xmm3, xmm7
	psubsw           xmm0, xmm2
	psubsw           xmm1, xmm3
	movq             xmm2, [r0+2*r1]
	punpcklqdq       xmm2, xmm2
	pmaddubsw        xmm2, xmm7
	movq             xmm3, [r0+r4]
	punpcklqdq       xmm3, xmm3
	pmaddubsw        xmm3, xmm7
	movq             xmm4, [r2+2*r3]
	punpcklqdq       xmm4, xmm4
	pmaddubsw        xmm4, xmm7
	movq             xmm5, [r2+r5]
	punpcklqdq       xmm5, xmm5
	pmaddubsw        xmm5, xmm7
	psubsw           xmm2, xmm4
	psubsw           xmm3, xmm5
	SSE2_HDMTwo4x4   xmm0, xmm1, xmm2, xmm3, xmm4
	pabsw            xmm0, xmm0
	pabsw            xmm2, xmm2
	pabsw            xmm1, xmm1
	pabsw            xmm3, xmm3
	movdqa           xmm4, xmm3
	pblendw          xmm3, xmm1, 0xAA
	pslld            xmm1, 16
	psrld            xmm4, 16
	por              xmm1, xmm4
	pmaxuw           xmm1, xmm3
	paddw            xmm6, xmm1
	movdqa           xmm4, xmm0
	pblendw          xmm0, xmm2, 0xAA
	pslld            xmm2, 16
	psrld            xmm4, 16
	por              xmm2, xmm4
	pmaxuw           xmm0, xmm2
	paddw            xmm6, xmm0
%endmacro

%macro SSSE3_SumWHorizon 4 ;eax, srcSSE, tempSSE, tempSSE
	MMX_DW_1_2REG    %3, %4
	pmaddwd     %2, %3
	movhlps     %4, %2
	paddd       %2, %4
	pshuflw     %4, %2,0Eh
	paddd       %2, %4
	movd		%1, %2
%endmacro
;***********************************************************************
;
;int32_t WelsSampleSatd4x4_sse41( uint8_t *, int32_t, uint8_t *, int32_t );
;
;***********************************************************************
WELS_EXTERN WelsSampleSatd4x4_sse41
WelsSampleSatd4x4_sse41:
	;push        ebx
	;mov         eax,[esp+8]
	;mov         ebx,[esp+12]
	;mov         ecx,[esp+16]
	;mov         edx,[esp+20]

	%assign  push_num 0
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	movdqa      xmm4,[HSwapSumSubDB1]
	movd        xmm2,[r2]
	movd        xmm5,[r2+r3]
	shufps      xmm2,xmm5,0
	movd        xmm3,[r2+r3*2]
	lea         r2, [r3*2+r2]
	movd        xmm5,[r2+r3]
	shufps      xmm3,xmm5,0
	movd        xmm0,[r0]
	movd        xmm5,[r0+r1]
	shufps      xmm0,xmm5,0
	movd        xmm1,[r0+r1*2]
	lea         r0, [r1*2+r0]
	movd        xmm5,[r0+r1]
	shufps      xmm1,xmm5,0
	pmaddubsw   xmm0,xmm4
	pmaddubsw   xmm1,xmm4
	pmaddubsw   xmm2,xmm4
	pmaddubsw   xmm3,xmm4
	psubw       xmm0,xmm2
	psubw       xmm1,xmm3
	movdqa      xmm2,xmm0
	paddw       xmm0,xmm1
	psubw       xmm1,xmm2
	movdqa      xmm2,xmm0
	punpcklqdq  xmm0,xmm1
	punpckhqdq  xmm2,xmm1
	movdqa      xmm1,xmm0
	paddw       xmm0,xmm2
	psubw       xmm2,xmm1
	movdqa      xmm1,xmm0
	pblendw     xmm0,xmm2,0AAh
	pslld       xmm2,16
	psrld       xmm1,16
	por         xmm2,xmm1
	pabsw       xmm0,xmm0
	pabsw       xmm2,xmm2
	pmaxsw      xmm0,xmm2
	SSSE3_SumWHorizon retrd, xmm0, xmm5, xmm7
	LOAD_4_PARA_POP
	ret

;***********************************************************************
;
;int32_t WelsSampleSatd8x8_sse41( uint8_t *, int32_t, uint8_t *, int32_t, );
;
;***********************************************************************
WELS_EXTERN WelsSampleSatd8x8_sse41
align 16
WelsSampleSatd8x8_sse41:
	;push   ebx
	;push   esi
	;push   edi
	;mov    eax,    [esp+16]
	;mov    ebx,    [esp+20]
	;mov    ecx,    [esp+24]
	;mov    edx,    [esp+28]
%ifdef X86_32
	push  r4
	push  r5
%endif
	%assign  push_num 2
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	movdqa      xmm7, [HSumSubDB1]
	lea         r4,  [r1+r1*2]
	lea         r5,  [r3+r3*2]
	pxor		xmm6, xmm6
	SSE41_GetSatd8x4
	lea			r0,	 [r0+4*r1]
	lea			r2,  [r2+4*r3]
	SSE41_GetSatd8x4
	SSSE3_SumWHorizon retrd, xmm6, xmm5, xmm7
	LOAD_4_PARA_POP
%ifdef X86_32
	pop  r5
	pop  r4
%endif
	ret

;***********************************************************************
;
;int32_t WelsSampleSatd8x16_sse41( uint8_t *, int32_t, uint8_t *, int32_t, );
;
;***********************************************************************
WELS_EXTERN WelsSampleSatd8x16_sse41
align 16
WelsSampleSatd8x16_sse41:
	;push   ebx
	;push   esi
	;push   edi
	;push   ebp
	;%define pushsize   16
	;mov    eax,    [esp+pushsize+4]
	;mov    ebx,    [esp+pushsize+8]
	;mov    ecx,    [esp+pushsize+12]
	;mov    edx,    [esp+pushsize+16]
%ifdef X86_32
	push  r4
	push  r5
	push  r6
%endif
	%assign  push_num 3
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	movdqa      xmm7, [HSumSubDB1]
	lea         r4,  [r1+r1*2]
	lea         r5,  [r3+r3*2]
	pxor        xmm6, xmm6
	mov         r6,    0
loop_get_satd_8x16:
	SSE41_GetSatd8x4
	lea			r0,  [r0+4*r1]
	lea			r2,  [r2+4*r3]
	inc         r6
	cmp         r6,  4
	jl          loop_get_satd_8x16
	SSSE3_SumWHorizon retrd, xmm6, xmm5, xmm7
	LOAD_4_PARA_POP
%ifdef X86_32
	pop  r6
	pop  r5
	pop  r4
%endif
	ret

;***********************************************************************
;
;int32_t WelsSampleSatd16x8_sse41( uint8_t *, int32_t, uint8_t *, int32_t, );
;
;***********************************************************************
WELS_EXTERN WelsSampleSatd16x8_sse41
align 16
WelsSampleSatd16x8_sse41:
	;push   ebx
	;push   esi
	;push   edi
	;mov    eax,    [esp+16]
	;mov    ebx,    [esp+20]
	;mov    ecx,    [esp+24]
	;mov    edx,    [esp+28]
%ifdef X86_32
	push  r4
	push  r5
%endif
	%assign  push_num 2
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	push  r0
	push  r2

	movdqa      xmm7, [HSumSubDB1]
	lea         r4,  [r1+r1*2]
	lea         r5,  [r3+r3*2]
	pxor		xmm6,   xmm6
	SSE41_GetSatd8x4
	lea			r0,  [r0+4*r1]
	lea			r2,  [r2+4*r3]
	SSE41_GetSatd8x4

	pop  r2
	pop  r0
	;mov			eax,    [esp+16]
	;mov			ecx,    [esp+24]
	add			r0,    8
	add			r2,    8
	SSE41_GetSatd8x4
	lea			r0,  [r0+4*r1]
	lea			r2,  [r2+4*r3]
	SSE41_GetSatd8x4
	SSSE3_SumWHorizon retrd, xmm6, xmm5, xmm7
	LOAD_4_PARA_POP
%ifdef X86_32
	pop  r5
	pop  r4
%endif
	ret

;***********************************************************************
;
;int32_t WelsSampleSatd16x16_sse41( uint8_t *, int32_t, uint8_t *, int32_t, );
;
;***********************************************************************

WELS_EXTERN WelsSampleSatd16x16_sse41
align 16
WelsSampleSatd16x16_sse41:
	;push   ebx
	;push   esi
	;push   edi
	;push   ebp
	;%define pushsize   16
	;mov    eax,    [esp+pushsize+4]
	;mov    ebx,    [esp+pushsize+8]
	;mov    ecx,    [esp+pushsize+12]
	;mov    edx,    [esp+pushsize+16]
%ifdef X86_32
	push  r4
	push  r5
	push  r6
%endif
	%assign  push_num 3
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d

	push  r0
	push  r2

	movdqa      xmm7, [HSumSubDB1]
	lea         r4,  [r1+r1*2]
	lea         r5,  [r3+r3*2]
	pxor		xmm6,   xmm6
	mov         r6,    0
loop_get_satd_16x16_left:
	SSE41_GetSatd8x4
	lea			r0,  [r0+4*r1]
	lea			r2,  [r2+4*r3]
	inc         r6
	cmp         r6,  4
	jl          loop_get_satd_16x16_left

	pop  r2
	pop  r0
	;mov			eax,    [esp+pushsize+4]
	;mov			ecx,    [esp+pushsize+12]
	add			r0,    8
	add			r2,    8
	mov         r6,    0
loop_get_satd_16x16_right:
	SSE41_GetSatd8x4
	lea			r0,  [r0+4*r1]
	lea			r2,  [r2+4*r3]
	inc         r6
	cmp         r6,  4
	jl          loop_get_satd_16x16_right
	SSSE3_SumWHorizon retrd, xmm6, xmm5, xmm7
	;%undef pushsize
	LOAD_4_PARA_POP
%ifdef X86_32
	pop  r6
	pop  r5
	pop  r4
%endif
	ret

;***********************************************************************
;
;Pixel_satd_wxh_sse41 END
;
;***********************************************************************

;***********************************************************************
;
;Pixel_sad_wxh_sse2 BEGIN
;
;***********************************************************************

%macro SSE2_GetSad2x16 0
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqu xmm1,   [r2]
	MOVDQ  xmm2,   [r0];[eax] must aligned 16
	psadbw xmm1,   xmm2
	paddw  xmm0,   xmm1
	movdqu xmm1,   [r2+r3]
	MOVDQ  xmm2,   [r0+r1]
	psadbw xmm1,   xmm2
	paddw  xmm0,   xmm1
%endmacro


%macro SSE2_GetSad4x16 0
	movdqu xmm0,   [r2]
	MOVDQ  xmm2,   [r0]
	psadbw xmm0,   xmm2
	paddw  xmm7,   xmm0
	movdqu xmm1,   [r2+r3]
	MOVDQ  xmm2,   [r0+r1]
	psadbw xmm1,   xmm2
	paddw  xmm7,   xmm1
	movdqu xmm1,   [r2+2*r3]
	MOVDQ  xmm2,   [r0+2*r1];[eax] must aligned 16
	psadbw xmm1,   xmm2
	paddw  xmm7,   xmm1
	movdqu xmm1,   [r2+r5]
	MOVDQ  xmm2,   [r0+r4]
	psadbw xmm1,   xmm2
	paddw  xmm7,   xmm1
%endmacro


%macro SSE2_GetSad8x4 0
	movq   xmm0,   [r0]
	movq   xmm1,   [r0+r1]
	lea    r0,     [r0+2*r1]
	movhps xmm0,   [r0]
	movhps xmm1,   [r0+r1]

	movq   xmm2,   [r2]
	movq   xmm3,   [r2+r3]
	lea    r2,     [r2+2*r3]
	movhps xmm2,   [r2]
	movhps xmm3,   [r2+r3]
	psadbw xmm0,   xmm2
	psadbw xmm1,   xmm3
	paddw  xmm6,   xmm0
	paddw  xmm6,   xmm1
%endmacro

;***********************************************************************
;
;int32_t WelsSampleSad16x16_sse2( uint8_t *, int32_t, uint8_t *, int32_t, )
;First parameter can align to 16 bytes,
;In wels, the third parameter can't align to 16 bytes.
;
;***********************************************************************
WELS_EXTERN WelsSampleSad16x16_sse2
align 16
WelsSampleSad16x16_sse2:
	;push ebx
	;push edi
	;push esi
	;%define _STACK_SIZE		12
	;mov eax, [esp+_STACK_SIZE+4 ]
	;mov	ebx, [esp+_STACK_SIZE+8 ]
	;mov ecx, [esp+_STACK_SIZE+12]
	;mov edx, [esp+_STACK_SIZE+16]
%ifdef X86_32
	push  r4
	push  r5
%endif

	%assign  push_num 2
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	lea r4, [3*r1]
	lea r5, [3*r3]

	pxor   xmm7,   xmm7
	SSE2_GetSad4x16
	lea	   r0,  [r0+4*r1]
	lea	   r2,  [r2+4*r3]
	SSE2_GetSad4x16
	lea	   r0,  [r0+4*r1]
	lea	   r2,  [r2+4*r3]
	SSE2_GetSad4x16
	lea	   r0,  [r0+4*r1]
	lea	   r2,  [r2+4*r3]
	SSE2_GetSad4x16
	movhlps xmm0, xmm7
	paddw xmm0, xmm7
	movd retrd, xmm0
	LOAD_4_PARA_POP
%ifdef X86_32
	pop  r5
	pop  r4
%endif
	ret

;***********************************************************************
;
;int32_t WelsSampleSad16x8_sse2( uint8_t *, int32_t, uint8_t *, int32_t, )
;First parameter can align to 16 bytes,
;In wels, the third parameter can't align to 16 bytes.
;
;***********************************************************************
WELS_EXTERN WelsSampleSad16x8_sse2
align 16
WelsSampleSad16x8_sse2:
	;push   ebx
	;mov    eax,    [esp+8]
	;mov    ebx,    [esp+12]
	;mov    ecx,    [esp+16]
	;mov    edx,    [esp+20]

	%assign  push_num 0
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	movdqu xmm0,   [r2]
	MOVDQ  xmm2,   [r0]
	psadbw xmm0,   xmm2
	movdqu xmm1,   [r2+r3]
	MOVDQ  xmm2,   [r0+r1]
	psadbw xmm1,   xmm2
	paddw  xmm0,   xmm1

	SSE2_GetSad2x16
	SSE2_GetSad2x16
	SSE2_GetSad2x16

	movhlps     xmm1, xmm0
	paddw       xmm0, xmm1
	movd        retrd,  xmm0
	LOAD_4_PARA_POP
	ret



WELS_EXTERN WelsSampleSad8x16_sse2
WelsSampleSad8x16_sse2:
	;push   ebx
	;mov    eax,    [esp+8]
	;mov    ebx,    [esp+12]
	;mov    ecx,    [esp+16]
	;mov    edx,    [esp+20]

	%assign  push_num 0
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
    pxor   xmm6,   xmm6

	SSE2_GetSad8x4
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
    SSE2_GetSad8x4
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	SSE2_GetSad8x4
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
    SSE2_GetSad8x4

    movhlps    xmm0, xmm6
	paddw      xmm0, xmm6
	movd       retrd,  xmm0
	LOAD_4_PARA_POP
	ret


%macro CACHE_SPLIT_CHECK 3 ; address, width, cacheline
and    %1,  0x1f|(%3>>1)
cmp    %1,  (32-%2)|(%3>>1)
%endmacro

WELS_EXTERN WelsSampleSad8x8_sse21
WelsSampleSad8x8_sse21:
    ;mov    ecx,    [esp+12]
	;mov    edx,    ecx
    ;CACHE_SPLIT_CHECK edx, 8, 64
	;jle    near   .pixel_sad_8x8_nsplit
	;push   ebx
	;push   edi
	;mov    eax,    [esp+12]
	;mov    ebx,    [esp+16]

	%assign  push_num 0
	mov		r2,  arg3
	push	r2
	CACHE_SPLIT_CHECK r2, 8, 64
	jle    near   .pixel_sad_8x8_nsplit
	pop		r2
%ifdef X86_32
	push	r3
	push	r4
	push	r5
%endif
	%assign  push_num 3
	mov		r0,  arg1
	mov		r1,  arg2
	SIGN_EXTENTION r1, r1d
    pxor   xmm7,   xmm7

    ;ecx r2, edx r4, edi r5

    mov    r5,    r2
    and    r5,    0x07
    sub    r2,    r5
    mov    r4,    8
    sub    r4,    r5

    shl    r5,    3
    shl    r4,    3
    movd   xmm5,   r5d
    movd   xmm6,   r4d
	mov    r5,    8
	add    r5,    r2
    mov    r3,    arg4
	SIGN_EXTENTION r3, r3d
    movq   xmm0,   [r0]
	movhps xmm0,   [r0+r1]

	movq   xmm1,   [r2]
	movq   xmm2,   [r5]
	movhps xmm1,   [r2+r3]
	movhps xmm2,   [r5+r3]
	psrlq  xmm1,   xmm5
	psllq  xmm2,   xmm6
	por    xmm1,   xmm2

	psadbw xmm0,   xmm1
	paddw  xmm7,   xmm0

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	lea    r5,    [r5+2*r3]

    movq   xmm0,   [r0]
	movhps xmm0,   [r0+r1]

	movq   xmm1,   [r2]
	movq   xmm2,   [r5]
	movhps xmm1,   [r2+r3]
	movhps xmm2,   [r5+r3]
	psrlq  xmm1,   xmm5
	psllq  xmm2,   xmm6
	por    xmm1,   xmm2

	psadbw xmm0,   xmm1
	paddw  xmm7,   xmm0

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	lea    r5,    [r5+2*r3]

    movq   xmm0,   [r0]
	movhps xmm0,   [r0+r1]

	movq   xmm1,   [r2]
	movq   xmm2,   [r5]
	movhps xmm1,   [r2+r3]
	movhps xmm2,   [r5+r3]
	psrlq  xmm1,   xmm5
	psllq  xmm2,   xmm6
	por    xmm1,   xmm2

	psadbw xmm0,   xmm1
	paddw  xmm7,   xmm0

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	lea    r5,    [r5+2*r3]

    movq   xmm0,   [r0]
	movhps xmm0,   [r0+r1]

	movq   xmm1,   [r2]
	movq   xmm2,   [r5]
	movhps xmm1,   [r2+r3]
	movhps xmm2,   [r5+r3]
	psrlq  xmm1,   xmm5
	psllq  xmm2,   xmm6
	por    xmm1,   xmm2

	psadbw xmm0,   xmm1
	paddw  xmm7,   xmm0

    movhlps    xmm0, xmm7
	paddw      xmm0, xmm7
	movd       retrd,  xmm0
%ifdef X86_32
	pop	 r5
	pop	 r4
	pop	 r3
%endif
	jmp        .return

.pixel_sad_8x8_nsplit:
    ;push   ebx
    ;mov    eax,    [esp+8]
	;mov    ebx,    [esp+12]
	;mov    edx,    [esp+20]

	pop r2
	%assign  push_num 0
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	pxor   xmm6,   xmm6
	SSE2_GetSad8x4
    lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
    SSE2_GetSad8x4
    movhlps    xmm0, xmm6
	paddw      xmm0, xmm6
	movd       retrd,  xmm0
	LOAD_4_PARA_POP
.return:
	ret


;***********************************************************************
;
;Pixel_sad_wxh_sse2 END
;
;***********************************************************************


;***********************************************************************
;
;Pixel_sad_4_wxh_sse2 BEGIN
;
;***********************************************************************


%macro SSE2_Get4LW16Sad 5 ;s-1l, s, s+1l, d, address
	psadbw %1,   %4
	paddw  xmm5, %1
	psadbw %4,   %3
	paddw  xmm4, %4
	movdqu %4,   [%5-1]
	psadbw %4,   %2
	paddw  xmm6, %4
	movdqu %4,   [%5+1]
	psadbw %4,   %2
	paddw  xmm7, %4
%endmacro
WELS_EXTERN WelsSampleSadFour16x16_sse2
WelsSampleSadFour16x16_sse2:
	;push ebx
	;mov    eax,    [esp+8]
	;mov    ebx,    [esp+12]
	;mov    ecx,    [esp+16]
	;mov    edx,    [esp+20]

	%assign  push_num 0
	LOAD_5_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	pxor   xmm4,   xmm4    ;sad pRefMb-i_stride_ref
	pxor   xmm5,   xmm5    ;sad pRefMb+i_stride_ref
	pxor   xmm6,   xmm6    ;sad pRefMb-1
	pxor   xmm7,   xmm7    ;sad pRefMb+1
	movdqa xmm0,   [r0]
	sub    r2,    r3
	movdqu xmm3,   [r2]
	psadbw xmm3,   xmm0
	paddw  xmm4,   xmm3

	movdqa xmm1,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	psadbw xmm3,   xmm1
	paddw  xmm4,   xmm3

	movdqu xmm2,   [r2+r3-1]
	psadbw xmm2,   xmm0
	paddw  xmm6,   xmm2

	movdqu xmm3,   [r2+r3+1]
	psadbw xmm3,   xmm0
	paddw  xmm7,   xmm3

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqa xmm2,   [r0]
	movdqu xmm3,   [r2]
	SSE2_Get4LW16Sad xmm0, xmm1, xmm2, xmm3, r2
	movdqa xmm0,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	SSE2_Get4LW16Sad xmm1, xmm2, xmm0, xmm3, r2+r3
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqa xmm1,   [r0]
	movdqu xmm3,   [r2]
	SSE2_Get4LW16Sad xmm2, xmm0, xmm1, xmm3, r2
	movdqa xmm2,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	SSE2_Get4LW16Sad xmm0, xmm1, xmm2, xmm3, r2+r3
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqa xmm0,   [r0]
	movdqu xmm3,   [r2]
	SSE2_Get4LW16Sad xmm1, xmm2, xmm0, xmm3, r2
	movdqa xmm1,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	SSE2_Get4LW16Sad xmm2, xmm0, xmm1, xmm3, r2+r3
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqa xmm2,   [r0]
	movdqu xmm3,   [r2]
	SSE2_Get4LW16Sad xmm0, xmm1, xmm2, xmm3, r2
	movdqa xmm0,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	SSE2_Get4LW16Sad xmm1, xmm2, xmm0, xmm3, r2+r3
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqa xmm1,   [r0]
	movdqu xmm3,   [r2]
	SSE2_Get4LW16Sad xmm2, xmm0, xmm1, xmm3, r2
	movdqa xmm2,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	SSE2_Get4LW16Sad xmm0, xmm1, xmm2, xmm3, r2+r3
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqa xmm0,   [r0]
	movdqu xmm3,   [r2]
	SSE2_Get4LW16Sad xmm1, xmm2, xmm0, xmm3, r2
	movdqa xmm1,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	SSE2_Get4LW16Sad xmm2, xmm0, xmm1, xmm3, r2+r3
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqa xmm2,   [r0]
	movdqu xmm3,   [r2]
	SSE2_Get4LW16Sad xmm0, xmm1, xmm2, xmm3, r2
	movdqa xmm0,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	SSE2_Get4LW16Sad xmm1, xmm2, xmm0, xmm3, r2+r3
	lea    r2,    [r2+2*r3]
	movdqu xmm3,   [r2]
	psadbw xmm2,   xmm3
	paddw xmm5,   xmm2

	movdqu xmm2,   [r2-1]
	psadbw xmm2,   xmm0
	paddw xmm6,   xmm2

	movdqu xmm3,   [r2+1]
	psadbw xmm3,   xmm0
	paddw xmm7,   xmm3

	movdqu xmm3,   [r2+r3]
	psadbw xmm0,   xmm3
	paddw xmm5,   xmm0

	;mov        ecx,  [esp+24]
	movhlps    xmm0, xmm4
	paddw      xmm4, xmm0
	movhlps    xmm0, xmm5
	paddw      xmm5, xmm0
	movhlps    xmm0, xmm6
	paddw      xmm6, xmm0
	movhlps    xmm0, xmm7
	paddw      xmm7, xmm0
	punpckldq  xmm4, xmm5
	punpckldq  xmm6, xmm7
	punpcklqdq xmm4, xmm6
	movdqa     [r4],xmm4
	LOAD_5_PARA_POP
	ret


WELS_EXTERN WelsSampleSadFour16x8_sse2
WelsSampleSadFour16x8_sse2:
	;push ebx
	;push edi
	;mov    eax,    [esp+12]
	;mov    ebx,    [esp+16]
	;mov    edi,    [esp+20]
	;mov    edx,    [esp+24]

	%assign  push_num 0
	LOAD_5_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	pxor   xmm4,   xmm4    ;sad pRefMb-i_stride_ref
	pxor   xmm5,   xmm5    ;sad pRefMb+i_stride_ref
	pxor   xmm6,   xmm6    ;sad pRefMb-1
	pxor   xmm7,   xmm7    ;sad pRefMb+1
	movdqa xmm0,   [r0]
	sub    r2,    r3
	movdqu xmm3,   [r2]
	psadbw xmm3,   xmm0
	paddw xmm4,   xmm3

	movdqa xmm1,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	psadbw xmm3,   xmm1
	paddw xmm4,   xmm3

	movdqu xmm2,   [r2+r3-1]
	psadbw xmm2,   xmm0
	paddw xmm6,   xmm2

	movdqu xmm3,   [r2+r3+1]
	psadbw xmm3,   xmm0
	paddw xmm7,   xmm3

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqa xmm2,   [r0]
	movdqu xmm3,   [r2]
	SSE2_Get4LW16Sad xmm0, xmm1, xmm2, xmm3, r2
	movdqa xmm0,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	SSE2_Get4LW16Sad xmm1, xmm2, xmm0, xmm3, r2+r3
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqa xmm1,   [r0]
	movdqu xmm3,   [r2]
	SSE2_Get4LW16Sad xmm2, xmm0, xmm1, xmm3, r2
	movdqa xmm2,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	SSE2_Get4LW16Sad xmm0, xmm1, xmm2, xmm3, r2+r3
	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movdqa xmm0,   [r0]
	movdqu xmm3,   [r2]
	SSE2_Get4LW16Sad xmm1, xmm2, xmm0, xmm3, r2
	movdqa xmm1,   [r0+r1]
	movdqu xmm3,   [r2+r3]
	SSE2_Get4LW16Sad xmm2, xmm0, xmm1, xmm3, r2+r3
	lea    r2,    [r2+2*r3]
	movdqu xmm3,   [r2]
	psadbw xmm0,   xmm3
	paddw xmm5,   xmm0

	movdqu xmm0,   [r2-1]
	psadbw xmm0,   xmm1
	paddw xmm6,   xmm0

	movdqu xmm3,   [r2+1]
	psadbw xmm3,   xmm1
	paddw xmm7,   xmm3

	movdqu xmm3,   [r2+r3]
	psadbw xmm1,   xmm3
	paddw xmm5,   xmm1

	;mov        edi,  [esp+28]
	movhlps    xmm0, xmm4
	paddw      xmm4, xmm0
	movhlps    xmm0, xmm5
	paddw      xmm5, xmm0
	movhlps    xmm0, xmm6
	paddw      xmm6, xmm0
	movhlps    xmm0, xmm7
	paddw      xmm7, xmm0
	punpckldq  xmm4, xmm5
	punpckldq  xmm6, xmm7
	punpcklqdq xmm4, xmm6
	movdqa     [r4],xmm4
	LOAD_5_PARA_POP
	ret

WELS_EXTERN WelsSampleSadFour8x16_sse2
WelsSampleSadFour8x16_sse2:
	;push ebx
	;push edi
	;mov    eax,    [esp+12]
	;mov    ebx,    [esp+16]
	;mov    edi,    [esp+20]
	;mov    edx,    [esp+24]

	%assign  push_num 0
	LOAD_5_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	pxor   xmm4,   xmm4    ;sad pRefMb-i_stride_ref
	pxor   xmm5,   xmm5    ;sad pRefMb+i_stride_ref
	pxor   xmm6,   xmm6    ;sad pRefMb-1
	pxor   xmm7,   xmm7    ;sad pRefMb+1
	movq   xmm0,   [r0]
	movhps xmm0,   [r0+r1]
	sub    r2,    r3
	movq   xmm3,   [r2]
	movhps xmm3,   [r2+r3]
	psadbw xmm3,   xmm0
	paddw  xmm4,   xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]
	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	movq   xmm0,  [r0]
	movhps xmm0,  [r0+r1]
	psadbw xmm3,  xmm0
	paddw  xmm4,  xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]

	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	movq   xmm0,  [r0]
	movhps xmm0,  [r0+r1]
	psadbw xmm3,  xmm0
	paddw  xmm4,  xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]

	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	movq   xmm0,  [r0]
	movhps xmm0,  [r0+r1]
	psadbw xmm3,  xmm0
	paddw  xmm4,  xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]

	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	movq   xmm0,  [r0]
	movhps xmm0,  [r0+r1]
	psadbw xmm3,  xmm0
	paddw  xmm4,  xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]

	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	movq   xmm0,  [r0]
	movhps xmm0,  [r0+r1]
	psadbw xmm3,  xmm0
	paddw  xmm4,  xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]

	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	movq   xmm0,  [r0]
	movhps xmm0,  [r0+r1]
	psadbw xmm3,  xmm0
	paddw  xmm4,  xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]

	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	movq   xmm0,  [r0]
	movhps xmm0,  [r0+r1]
	psadbw xmm3,  xmm0
	paddw  xmm4,  xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]

	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	;mov        edi,  [esp+28]
	movhlps    xmm0, xmm4
	paddw      xmm4, xmm0
	movhlps    xmm0, xmm5
	paddw      xmm5, xmm0
	movhlps    xmm0, xmm6
	paddw      xmm6, xmm0
	movhlps    xmm0, xmm7
	paddw      xmm7, xmm0
	punpckldq  xmm4, xmm5
	punpckldq  xmm6, xmm7
	punpcklqdq xmm4, xmm6
	movdqa     [r4],xmm4
	LOAD_5_PARA_POP
	ret


WELS_EXTERN WelsSampleSadFour8x8_sse2
WelsSampleSadFour8x8_sse2:
	;push ebx
	;push edi
	;mov    eax,    [esp+12]
	;mov    ebx,    [esp+16]
	;mov    edi,    [esp+20]
	;mov    edx,    [esp+24]

	%assign  push_num 0
	LOAD_5_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	pxor   xmm4,   xmm4    ;sad pRefMb-i_stride_ref
	pxor   xmm5,   xmm5    ;sad pRefMb+i_stride_ref
	pxor   xmm6,   xmm6    ;sad pRefMb-1
	pxor   xmm7,   xmm7    ;sad pRefMb+1
	movq   xmm0,   [r0]
	movhps xmm0,   [r0+r1]
	sub    r2,    r3
	movq   xmm3,   [r2]
	movhps xmm3,   [r2+r3]
	psadbw xmm3,   xmm0
	paddw  xmm4,   xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]
	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	movq   xmm0,  [r0]
	movhps xmm0,  [r0+r1]
	psadbw xmm3,  xmm0
	paddw  xmm4,  xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]

	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	movq   xmm0,  [r0]
	movhps xmm0,  [r0+r1]
	psadbw xmm3,  xmm0
	paddw  xmm4,  xmm3

	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]

	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	movq   xmm0,  [r0]
	movhps xmm0,  [r0+r1]
	psadbw xmm3,  xmm0
	paddw  xmm4,  xmm3


	movq   xmm1,  [r2+r3-1]
	movq   xmm3,  [r2+r3+1]

	lea    r0,    [r0+2*r1]
	lea    r2,    [r2+2*r3]
	movhps xmm1,  [r2-1]
	movhps xmm3,  [r2+1]

	psadbw xmm1,  xmm0
	paddw  xmm6,  xmm1
	psadbw xmm3,  xmm0
	paddw  xmm7,  xmm3

	movq   xmm3,  [r2]
	movhps xmm3,  [r2+r3]
	psadbw xmm0,  xmm3
	paddw  xmm5,  xmm0

	;mov        edi,  [esp+28]
	movhlps    xmm0, xmm4
	paddw      xmm4, xmm0
	movhlps    xmm0, xmm5
	paddw      xmm5, xmm0
	movhlps    xmm0, xmm6
	paddw      xmm6, xmm0
	movhlps    xmm0, xmm7
	paddw      xmm7, xmm0
	punpckldq  xmm4, xmm5
	punpckldq  xmm6, xmm7
	punpcklqdq xmm4, xmm6
	movdqa     [r4],xmm4
	LOAD_5_PARA_POP
	ret

WELS_EXTERN WelsSampleSadFour4x4_sse2
WelsSampleSadFour4x4_sse2:
	;push ebx
	;push edi
	;mov    eax,    [esp+12]
	;mov    ebx,    [esp+16]
	;mov    edi,    [esp+20]
	;mov    edx,    [esp+24]

	%assign  push_num 0
	LOAD_5_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	movd   xmm0,   [r0]
	movd   xmm1,   [r0+r1]
	lea        r0,    [r0+2*r1]
	movd       xmm2,   [r0]
	movd       xmm3,   [r0+r1]
	punpckldq  xmm0, xmm1
	punpckldq  xmm2, xmm3
	punpcklqdq xmm0, xmm2
	sub        r2,  r3
	movd       xmm1, [r2]
	movd       xmm2, [r2+r3]
	punpckldq  xmm1, xmm2
	movd       xmm2, [r2+r3-1]
	movd       xmm3, [r2+r3+1]

	lea        r2,  [r2+2*r3]

	movd       xmm4, [r2]
	movd       xmm5, [r2-1]
	punpckldq  xmm2, xmm5
	movd       xmm5, [r2+1]
	punpckldq  xmm3, xmm5

	movd       xmm5, [r2+r3]
	punpckldq  xmm4, xmm5

	punpcklqdq xmm1, xmm4 ;-L

	movd       xmm5, [r2+r3-1]
	movd       xmm6, [r2+r3+1]

	lea        r2,  [r2+2*r3]
	movd       xmm7, [r2-1]
	punpckldq  xmm5, xmm7
	punpcklqdq xmm2, xmm5 ;-1
	movd       xmm7, [r2+1]
	punpckldq  xmm6, xmm7
	punpcklqdq xmm3, xmm6 ;+1
	movd       xmm6, [r2]
	movd       xmm7, [r2+r3]
	punpckldq  xmm6, xmm7
	punpcklqdq xmm4, xmm6 ;+L
	psadbw     xmm1, xmm0
	psadbw     xmm2, xmm0
	psadbw     xmm3, xmm0
	psadbw     xmm4, xmm0

	movhlps    xmm0, xmm1
	paddw      xmm1, xmm0
	movhlps    xmm0, xmm2
	paddw      xmm2, xmm0
	movhlps    xmm0, xmm3
	paddw      xmm3, xmm0
	movhlps    xmm0, xmm4
	paddw      xmm4, xmm0
	;mov        edi,  [esp+28]
	punpckldq  xmm1, xmm4
	punpckldq  xmm2, xmm3
	punpcklqdq xmm1, xmm2
	movdqa     [r4],xmm1
	LOAD_5_PARA_POP
	ret

;***********************************************************************
;
;Pixel_sad_4_wxh_sse2 END
;
;***********************************************************************

WELS_EXTERN WelsSampleSad4x4_mmx

align 16
;***********************************************************************
;   int32_t WelsSampleSad4x4_mmx (uint8_t *, int32_t, uint8_t *, int32_t )
;***********************************************************************
WelsSampleSad4x4_mmx:
    ;push    ebx
	;%define pushsize     4
	;%define pix1address	 esp+pushsize+4
	;%define pix1stride   esp+pushsize+8
	;%define pix2address  esp+pushsize+12
	;%define pix2stride   esp+pushsize+16
    ;mov		  eax, [pix1address]
    ;mov		  ebx, [pix1stride ]
    ;mov		  ecx, [pix2address]
    ;mov		  edx, [pix2stride ]

    %assign  push_num 0
	LOAD_4_PARA
	SIGN_EXTENTION r1, r1d
	SIGN_EXTENTION r3, r3d
	movd	  mm0, [r0]
	movd	  mm1, [r0+r1]
	punpckldq mm0, mm1

	movd      mm3, [r2]
	movd      mm4, [r2+r3]
	punpckldq mm3, mm4
	psadbw    mm0, mm3

	lea       r0, [r0+2*r1]
	lea       r2, [r2+2*r3]

	movd      mm1, [r0]
	movd      mm2, [r0+r1]
	punpckldq mm1, mm2

	movd      mm3, [r2]
	movd      mm4, [r2+r3]
	punpckldq mm3, mm4
	psadbw    mm1, mm3
	paddw     mm0, mm1

    movd      retrd, mm0

	WELSEMMS
    LOAD_4_PARA_POP
    ret
