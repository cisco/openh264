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
;*  sse2inc.asm
;*
;*  Abstract
;*      macro and constant
;*
;*  History
;*      8/5/2009 Created
;*
;*
;*************************************************************************/
;***********************************************************************
; Options, for DEBUG
;***********************************************************************

%if 1 
	%define MOVDQ movdqa
%else
	%define MOVDQ movdqu
%endif

%if 1
	%define WELSEMMS	emms
%else
	%define WELSEMMS
%endif

BITS 32

;***********************************************************************
; Macros 
;***********************************************************************

%macro WELS_EXTERN 1
	%ifdef PREFIX
		global _%1
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

%macro WELS_AbsW 2
	pxor        %2, %2
    psubw       %2, %1
    pmaxsw      %1, %2
%endmacro 	

%macro MMX_XSwap  4
    movq		%4, %2
    punpckh%1   %4, %3
    punpckl%1   %2, %3
%endmacro

; pOut mm1, mm4, mm5, mm3
%macro MMX_Trans4x4W 5
    MMX_XSwap wd, %1, %2, %5
    MMX_XSwap wd, %3, %4, %2
    MMX_XSwap dq, %1, %3, %4
    MMX_XSwap dq, %5, %2, %3
%endmacro

;for TRANSPOSE
%macro SSE2_XSawp 4
    movdqa      %4, %2
    punpckl%1   %2, %3
    punpckh%1   %4, %3
%endmacro

; in: xmm1, xmm2, xmm3, xmm4  pOut:  xmm1, xmm4, xmm5, mm3
%macro SSE2_Trans4x4D 5
    SSE2_XSawp dq,  %1, %2, %5
    SSE2_XSawp dq,  %3, %4, %2
    SSE2_XSawp qdq, %1, %3, %4
    SSE2_XSawp qdq, %5, %2, %3
%endmacro

;in: xmm0, xmm1, xmm2, xmm3  pOut:  xmm0, xmm1, xmm3, xmm4 
%macro SSE2_TransTwo4x4W 5
    SSE2_XSawp wd,  %1, %2, %5
    SSE2_XSawp wd,  %3, %4, %2
    SSE2_XSawp dq,  %1, %3, %4
    SSE2_XSawp dq,  %5, %2, %3
    SSE2_XSawp qdq, %1, %5, %2
    SSE2_XSawp qdq, %4, %3, %5
%endmacro

;in:  m1, m2, m3, m4, m5, m6, m7, m8
;pOut: m5, m3, m4, m8, m6, m2, m7, m1
%macro SSE2_TransTwo8x8B 9
	movdqa	%9,	%8
	SSE2_XSawp bw,  %1, %2, %8
	SSE2_XSawp bw,  %3, %4, %2
	SSE2_XSawp bw,  %5, %6, %4
	movdqa	%6, %9
	movdqa	%9, %4
	SSE2_XSawp bw,  %7, %6, %4
	
	SSE2_XSawp wd,  %1, %3, %6	
	SSE2_XSawp wd,  %8, %2, %3
	SSE2_XSawp wd,  %5, %7, %2
	movdqa	%7, %9
	movdqa	%9, %3	
	SSE2_XSawp wd,  %7, %4, %3
	
	SSE2_XSawp dq,  %1, %5, %4	
	SSE2_XSawp dq,  %6, %2, %5
	SSE2_XSawp dq,  %8, %7, %2
	movdqa	%7, %9
	movdqa	%9, %5		
	SSE2_XSawp dq,  %7, %3, %5
	
	SSE2_XSawp qdq,  %1, %8, %3
	SSE2_XSawp qdq,  %4, %2, %8
	SSE2_XSawp qdq,  %6, %7, %2
	movdqa	%7, %9
	movdqa	%9, %1		
	SSE2_XSawp qdq,  %7, %5, %1
	movdqa	%5, %9
%endmacro

;xmm0, xmm6, xmm7, [eax], [ecx]
;xmm7 = 0, eax = pix1, ecx = pix2, xmm0 save the result
%macro SSE2_LoadDiff8P 5
    movq         %1, %4
    punpcklbw    %1, %3
    movq         %2, %5
    punpcklbw    %2, %3
    psubw        %1, %2
%endmacro

; m2 = m1 + m2, m1 = m1 - m2
%macro SSE2_SumSub 3
	movdqa  %3, %2
    paddw   %2, %1
    psubw   %1, %3
%endmacro


%macro butterfly_1to16_sse	3	; xmm? for dst, xmm? for tmp, one byte for pSrc [generic register name: a/b/c/d]
	mov %3h, %3l
	movd %1, e%3x		; i.e, 1% = eax (=b0)
	pshuflw %2, %1, 00h	; ..., b0 b0 b0 b0 b0 b0 b0 b0	
	pshufd %1, %2, 00h	; b0 b0 b0 b0, b0 b0 b0 b0, b0 b0 b0 b0, b0 b0 b0 b0	
%endmacro  

;copy a dw into a xmm for 8 times
%macro  SSE2_Copy8Times 2
		movd	%1, %2
		punpcklwd %1, %1
		pshufd	%1,	%1,	0
%endmacro

;copy a db into a xmm for 16 times
%macro  SSE2_Copy16Times 2
		movd		%1, %2
		pshuflw		%1, %1, 0
		punpcklqdq	%1, %1
		packuswb	%1,	%1
%endmacro



;***********************************************************************
;preprocessor constants
;***********************************************************************
;dw 32,32,32,32,32,32,32,32 for xmm
;dw 32,32,32,32 for mm
%macro WELS_DW32 1
	pcmpeqw %1,%1
	psrlw %1,15
	psllw %1,5
%endmacro

;dw 1, 1, 1, 1, 1, 1, 1, 1 for xmm
;dw 1, 1, 1, 1 for mm
%macro WELS_DW1 1
	pcmpeqw %1,%1
	psrlw %1,15
%endmacro

;all 0 for xmm and mm
%macro	WELS_Zero 1
	pxor %1, %1
%endmacro

;dd 1, 1, 1, 1 for xmm
;dd 1, 1 for mm
%macro WELS_DD1 1
	pcmpeqw %1,%1
	psrld %1,31
%endmacro

;dB 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
%macro WELS_DB1 1
	pcmpeqw %1,%1
	psrlw %1,15
	packuswb %1,%1
%endmacro






