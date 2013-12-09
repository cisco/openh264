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
;*  pixel_sse2.asm
;*
;*  Abstract
;*      WelsSampleSad8x8_sse21
;*
;*  History
;*      8/5/2009 Created
;*
;*
;*************************************************************************/

%include "asm_inc.asm"

BITS 32

;***********************************************************************
; Macros and other preprocessor constants
;***********************************************************************

%macro SAD_8x4 0
	movq   xmm0,   [eax]
	movq   xmm1,   [eax+ebx]
	lea    eax,    [eax+2*ebx]
	movhps xmm0,   [eax]
	movhps xmm1,   [eax+ebx]

	movq   xmm2,   [ecx]
	movq   xmm3,   [ecx+edx]
	lea    ecx,    [ecx+2*edx]
	movhps xmm2,   [ecx]
	movhps xmm3,   [ecx+edx]
	psadbw xmm0,   xmm2
	psadbw xmm1,   xmm3
	paddw  xmm6,   xmm0
	paddw  xmm6,   xmm1
%endmacro


  
%macro CACHE_SPLIT_CHECK 3 ; address, width, cacheline
and    %1,  0x1f|(%3>>1)
cmp    %1,  (32-%2)|(%3>>1)
%endmacro


%macro SSE2_GetSad8x4 0
	movq   xmm0,   [eax]
	movq   xmm1,   [eax+ebx]
	lea    eax,    [eax+2*ebx]
	movhps xmm0,   [eax]
	movhps xmm1,   [eax+ebx]

	movq   xmm2,   [ecx]
	movq   xmm3,   [ecx+edx]
	lea    ecx,    [ecx+2*edx]
	movhps xmm2,   [ecx]
	movhps xmm3,   [ecx+edx]
	psadbw xmm0,   xmm2
	psadbw xmm1,   xmm3
	paddw  xmm6,   xmm0
	paddw  xmm6,   xmm1
%endmacro


;***********************************************************************
; Code
;***********************************************************************
SECTION .text

WELS_EXTERN WelsSampleSad8x8_sse21
WelsSampleSad8x8_sse21:
    mov    ecx,    [esp+12]
	mov    edx,    ecx
    CACHE_SPLIT_CHECK edx, 8, 64
	jle    near   .pixel_sad_8x8_nsplit
	push   ebx
	push   edi
	mov    eax,    [esp+12]
	mov    ebx,    [esp+16]
    
    pxor   xmm7,   xmm7
    
    mov    edi,    ecx
    and    edi,    0x07
    sub    ecx,    edi   
    mov    edx,    8
    sub    edx,    edi
    
    shl    edi,    3
    shl    edx,    3
    movd   xmm5,   edi
    movd   xmm6,   edx
	mov    edi,    8
	add    edi,    ecx
    mov    edx,    [esp+24]
    
    movq   xmm0,   [eax]
	movhps xmm0,   [eax+ebx]
		
	movq   xmm1,   [ecx]
	movq   xmm2,   [edi]
	movhps xmm1,   [ecx+edx]
	movhps xmm2,   [edi+edx]
	psrlq  xmm1,   xmm5
	psllq  xmm2,   xmm6
	por    xmm1,   xmm2
	
	psadbw xmm0,   xmm1
	paddw  xmm7,   xmm0
	
	lea    eax,    [eax+2*ebx]
	lea    ecx,    [ecx+2*edx]
	lea    edi,    [edi+2*edx]
	 
    movq   xmm0,   [eax]
	movhps xmm0,   [eax+ebx]
		
	movq   xmm1,   [ecx]
	movq   xmm2,   [edi]
	movhps xmm1,   [ecx+edx]
	movhps xmm2,   [edi+edx]
	psrlq  xmm1,   xmm5
	psllq  xmm2,   xmm6
	por    xmm1,   xmm2
	
	psadbw xmm0,   xmm1
	paddw  xmm7,   xmm0

	lea    eax,    [eax+2*ebx]
	lea    ecx,    [ecx+2*edx]
	lea    edi,    [edi+2*edx]
	 
    movq   xmm0,   [eax]
	movhps xmm0,   [eax+ebx]
		
	movq   xmm1,   [ecx]
	movq   xmm2,   [edi]
	movhps xmm1,   [ecx+edx]
	movhps xmm2,   [edi+edx]
	psrlq  xmm1,   xmm5
	psllq  xmm2,   xmm6
	por    xmm1,   xmm2
	
	psadbw xmm0,   xmm1
	paddw  xmm7,   xmm0
	
	lea    eax,    [eax+2*ebx]
	lea    ecx,    [ecx+2*edx]
	lea    edi,    [edi+2*edx]
	 
    movq   xmm0,   [eax]
	movhps xmm0,   [eax+ebx]
		
	movq   xmm1,   [ecx]
	movq   xmm2,   [edi]
	movhps xmm1,   [ecx+edx]
	movhps xmm2,   [edi+edx]
	psrlq  xmm1,   xmm5
	psllq  xmm2,   xmm6
	por    xmm1,   xmm2
	
	psadbw xmm0,   xmm1
	paddw  xmm7,   xmm0
	
    movhlps    xmm0, xmm7
	paddw      xmm0, xmm7
	movd       eax,  xmm0
	pop        edi
	jmp        .return
.pixel_sad_8x8_nsplit:
    push   ebx
    mov    eax,    [esp+8]
	mov    ebx,    [esp+12]
	mov    edx,    [esp+20]    
	pxor   xmm6,   xmm6
	SSE2_GetSad8x4
    lea    eax,    [eax+2*ebx]
	lea    ecx,    [ecx+2*edx]
    SSE2_GetSad8x4    
    movhlps    xmm0, xmm6
	paddw      xmm0, xmm6
	movd       eax,  xmm0
.return:
	pop        ebx
	ret