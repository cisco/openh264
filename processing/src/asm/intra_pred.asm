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
;*  intra_pred.asm
;*
;*  Abstract
;*      sse2 function for intra predict operations
;*
;*  History
;*      18/09/2009 Created
;*
;*
;*************************************************************************/
%include "../../src/asm/asm_inc.asm"

BITS 32
;***********************************************************************
; Local Data (Read Only)
;***********************************************************************

%ifdef FORMAT_COFF
SECTION .rodata data
%else
SECTION .rodata align=16
%endif


align 16
mmx_01bytes:		times 16	db 1

;***********************************************************************
; macros
;***********************************************************************
%macro  COPY_16_TIMES 2
		movdqa		%2,	[%1-16]
		psrldq		%2,	15
		pmuludq		%2,	[mmx_01bytes]
		pshufd		%2,	%2, 0
%endmacro

%macro  COPY_16_TIMESS 3
		movdqa		%2,	[%1+%3-16]
		psrldq		%2,	15
		pmuludq		%2,	[mmx_01bytes]
		pshufd		%2,	%2, 0
%endmacro

;***********************************************************************
; Code
;***********************************************************************

SECTION .text

;***********************************************************************
; void WelsI16x16LumaPredH_sse2(uint8_t *pred, uint8_t *pRef, int32_t stride);
;***********************************************************************

%macro SSE2_PRED_H_16X16_TWO_LINE 1
    lea     eax,	[eax+ecx*2]
    
    COPY_16_TIMES eax,	xmm0
    movdqa  [edx+%1],	xmm0
    COPY_16_TIMESS eax,	xmm0,	ecx
    movdqa  [edx+%1+0x10],	xmm0
%endmacro

WELS_EXTERN WelsI16x16LumaPredH_sse2
WelsI16x16LumaPredH_sse2:
    mov     edx, [esp+4]    ; pred
    mov     eax, [esp+8]	; pRef
    mov     ecx, [esp+12]   ; stride
    
    COPY_16_TIMES eax,	xmm0
    movdqa  [edx],		xmm0
    COPY_16_TIMESS eax,	xmm0,	ecx
    movdqa  [edx+0x10],	xmm0
    
	SSE2_PRED_H_16X16_TWO_LINE   0x20 
	SSE2_PRED_H_16X16_TWO_LINE   0x40
	SSE2_PRED_H_16X16_TWO_LINE   0x60
	SSE2_PRED_H_16X16_TWO_LINE   0x80
	SSE2_PRED_H_16X16_TWO_LINE   0xa0
	SSE2_PRED_H_16X16_TWO_LINE   0xc0
	SSE2_PRED_H_16X16_TWO_LINE   0xe0
   
    ret
    
;***********************************************************************
; void WelsI16x16LumaPredV_sse2(uint8_t *pred, uint8_t *pRef, int32_t stride);
;***********************************************************************
WELS_EXTERN WelsI16x16LumaPredV_sse2
WelsI16x16LumaPredV_sse2:
    mov     edx, [esp+4]    ; pred
    mov     eax, [esp+8]	; pRef
    mov     ecx, [esp+12]   ; stride
    
    sub     eax, ecx
    movdqa  xmm0, [eax]
    
    movdqa  [edx], xmm0
    movdqa  [edx+10h], xmm0
    movdqa  [edx+20h], xmm0
    movdqa  [edx+30h], xmm0
    movdqa  [edx+40h], xmm0
    movdqa  [edx+50h], xmm0
    movdqa  [edx+60h], xmm0
    movdqa  [edx+70h], xmm0
    movdqa  [edx+80h], xmm0
    movdqa  [edx+90h], xmm0
    movdqa  [edx+160], xmm0 
	movdqa  [edx+176], xmm0
    movdqa  [edx+192], xmm0
    movdqa  [edx+208], xmm0
    movdqa  [edx+224], xmm0
    movdqa  [edx+240], xmm0
    
    ret