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
;*  sad.asm
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

;***********************************************************************
; Macros and other preprocessor constants
;***********************************************************************
%macro CACHE_SPLIT_CHECK 3 ; address, width, cacheline
and    %1,  0x1f|(%3>>1)
cmp    %1,  (32-%2)|(%3>>1)
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
; Code
;***********************************************************************
SECTION .text

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