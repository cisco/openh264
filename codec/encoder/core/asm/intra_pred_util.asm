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
;*  intra_pred_util.asm
;*
;*  Abstract
;*      mmxext/sse for WelsFillingPred8to16, WelsFillingPred8x2to16 and
;*		WelsFillingPred1to16 etc.
;*
;*  History
;*      09/29/2009 Created
;*
;*
;*************************************************************************/

%include "asm_inc.asm"

BITS 32

;***********************************************************************
; Macros and other preprocessor constants
;***********************************************************************

;***********************************************************************
; Local Data (Read Only)
;***********************************************************************

;SECTION .rodata pData align=16

;***********************************************************************
; Various memory constants (trigonometric values or rounding values)
;***********************************************************************

;***********************************************************************
; Code
;***********************************************************************


SECTION .text

WELS_EXTERN WelsFillingPred8to16_mmx
WELS_EXTERN WelsFillingPred8x2to16_mmx
WELS_EXTERN WelsFillingPred1to16_mmx
WELS_EXTERN WelsFillingPred8x2to16_sse2
WELS_EXTERN WelsFillingPred1to16_sse2


ALIGN 16
;***********************************************************************----------------
; void WelsFillingPred8to16_mmx( uint8_t *pred, uint8_t *v );
;***********************************************************************----------------
WelsFillingPred8to16_mmx:
	mov eax, [esp+4]	; pred
	mov ecx, [esp+8]	; v

	movq mm0, [ecx]
	movq [eax  ], mm0
	movq [eax+8], mm0

	WELSEMMS
	ret

ALIGN 16
;***********************************************************************----------------
; void WelsFillingPred8x2to16_mmx( uint8_t *pred, uint8_t *v );
;***********************************************************************----------------
WelsFillingPred8x2to16_mmx:
	mov eax, [esp+4]	; pred
	mov ecx, [esp+8]	; v

	movq mm0, [ecx  ]
	movq mm1, [ecx+8]
	movq [eax  ], mm0
	movq [eax+8], mm1

	WELSEMMS

	ret

%macro butterfly_1to8_mmx	3	; mm? for dst, mm? for tmp, one byte for pSrc [generic register name: a/b/c/d]
	mov %3h, %3l
	movd %2, e%3x		; i.e, 1% = eax (=b0)
	pshufw %1, %2, 00h	; b0 b0 b0 b0, b0 b0 b0 b0
%endmacro

ALIGN 16
;***********************************************************************----------------
; void WelsFillingPred1to16_mmx( uint8_t *pred, const uint8_t v );
;***********************************************************************----------------
WelsFillingPred1to16_mmx:
	mov eax, [esp+4]		; pred

	mov cl, byte [esp+8]	; v
	butterfly_1to8_mmx	mm0, mm1, c	; mm? for dst, mm? for tmp, one byte for pSrc [generic register name: a/b/c/d]

	movq [eax  ], mm0
	movq [eax+8], mm0

	WELSEMMS

	ret

ALIGN 16
;***********************************************************************----------------
; void WelsFillingPred8x2to16_sse2( uint8_t *pred, uint8_t *v );
;***********************************************************************----------------
WelsFillingPred8x2to16_sse2:
	mov eax, [esp+4]	; pred
	mov ecx, [esp+8]	; v

	movdqa xmm0, [ecx]
	movdqa [eax], xmm0

	ret

ALIGN 16
;***********************************************************************----------------
; void WelsFillingPred1to16_sse2( uint8_t *pred, const uint8_t v );
;***********************************************************************----------------
WelsFillingPred1to16_sse2:
	mov eax, [esp+4]		; pred

	mov cl, byte [esp+8]	; v
	butterfly_1to16_sse	xmm0, xmm1, c		; dst, tmp, pSrc [generic register name: a/b/c/d]

	movdqa [eax], xmm0

	ret
