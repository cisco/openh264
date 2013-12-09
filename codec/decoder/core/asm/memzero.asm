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
;*  memzero.asm
;*
;*  Abstract
;*      
;*
;*  History
;*      9/16/2009 Created
;*
;*
;*************************************************************************/

BITS 32

%include "asm_inc.asm"
;***********************************************************************
; Code
;***********************************************************************

SECTION .text			
		
ALIGN 16
;***********************************************************************
;_inline void __cdecl WelsPrefetchZero_mmx(int8_t const*_A);
;***********************************************************************
WELS_EXTERN WelsPrefetchZero_mmx
WelsPrefetchZero_mmx:
	mov  eax,[esp+4]
	prefetchnta [eax]
	ret 			


ALIGN 16
;***********************************************************************
;   void WelsSetMemZeroAligned64_sse2(void *dst, int32_t size)
;***********************************************************************
WELS_EXTERN WelsSetMemZeroAligned64_sse2
WelsSetMemZeroAligned64_sse2:
		mov		eax,	[esp + 4]          ; dst
		mov		ecx,	[esp + 8]
		neg		ecx
			
		pxor	xmm0,		xmm0
.memzeroa64_sse2_loops:
		movdqa	[eax],		xmm0
		movdqa	[eax+16],	xmm0
		movdqa	[eax+32],	xmm0
		movdqa	[eax+48],	xmm0
		add		eax, 0x40
		
		add ecx, 0x40
		jnz near .memzeroa64_sse2_loops
			
		ret	

ALIGN 16
;***********************************************************************
;   void WelsSetMemZeroSize64_mmx(void *dst, int32_t size)
;***********************************************************************
WELS_EXTERN WelsSetMemZeroSize64_mmx
WelsSetMemZeroSize64_mmx:
		mov		eax,	[esp + 4]          ; dst
		mov		ecx,	[esp + 8]
		neg		ecx
			
		pxor	mm0,		mm0
.memzero64_mmx_loops:
		movq	[eax],		mm0
		movq	[eax+8],	mm0
		movq	[eax+16],	mm0
		movq	[eax+24],	mm0
		movq	[eax+32],	mm0
		movq	[eax+40],	mm0
		movq	[eax+48],	mm0
		movq	[eax+56],	mm0		
		add		eax,		0x40
		
		add ecx, 0x40
		jnz near .memzero64_mmx_loops
			
		WELSEMMS	
		ret	
	
ALIGN 16		
;***********************************************************************
;   void WelsSetMemZeroSize8_mmx(void *dst, int32_t size)
;***********************************************************************
WELS_EXTERN WelsSetMemZeroSize8_mmx
WelsSetMemZeroSize8_mmx:
		mov		eax,	[esp + 4]		; dst
		mov		ecx,	[esp + 8]		; size
		neg		ecx			
		pxor	mm0,		mm0
		
.memzero8_mmx_loops:
		movq	[eax],		mm0
		add		eax,		0x08
	
		add		ecx,		0x08
		jnz near .memzero8_mmx_loops
		
		WELSEMMS	
		ret	

							
