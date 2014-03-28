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
;*************************************************************************/
%include "asm_inc.asm"

SECTION .text

;**********************************************************************************************************************************
;
;	uint32_t pixel_sad_16x16_hor8_sse41( uint8_t *src, int32_t stride_src, uint8_t *ref, int32_t stride_ref, uint16 base_cost[8], int32_t *index_min_cost )
;
;	\note:
;		src meed align with 16 bytes, ref is optional
;	\return value:
;		return minimal SAD cost, according index carried by index_min_cost
;**********************************************************************************************************************************
; try 8 mv via offset
; xmm7 store sad costs
%macro   SAD_16x16_LINE_SSE41  4	; src, ref, stride_src, stride_ref
    movdqa		xmm0, [%1]
    movdqu		xmm1, [%2]
    movdqu		xmm2, [%2+8h]
    movdqa		xmm3, xmm1
    movdqa		xmm4, xmm2
    
    mpsadbw		xmm1, xmm0, 0	; 000 B
    paddw		xmm7, xmm1		; accumulate cost
    
    mpsadbw		xmm3, xmm0, 5	; 101 B
    paddw		xmm7, xmm3		; accumulate cost
     
    mpsadbw		xmm2, xmm0, 2	; 010 B
    paddw		xmm7, xmm2		; accumulate cost
    
    mpsadbw		xmm4, xmm0, 7	; 111 B    
    paddw		xmm7, xmm4		; accumulate cost
    
    add			%1, %3
    add			%2, %4               
%endmacro	; end of SAD_16x16_LINE_SSE41
%macro   SAD_16x16_LINE_SSE41E  4	; src, ref, stride_src, stride_ref
    movdqa		xmm0, [%1]
    movdqu		xmm1, [%2]
    movdqu		xmm2, [%2+8h]
    movdqa		xmm3, xmm1
    movdqa		xmm4, xmm2
    
    mpsadbw		xmm1, xmm0, 0	; 000 B
    paddw		xmm7, xmm1		; accumulate cost
    
    mpsadbw		xmm3, xmm0, 5	; 101 B
    paddw		xmm7, xmm3		; accumulate cost
     
    mpsadbw		xmm2, xmm0, 2	; 010 B
    paddw		xmm7, xmm2		; accumulate cost
    
    mpsadbw		xmm4, xmm0, 7	; 111 B    
    paddw		xmm7, xmm4		; accumulate cost                   
%endmacro	; end of SAD_16x16_LINE_SSE41E

WELS_EXTERN pixel_sad_16x16_hor8_sse41
    ;push ebx
    ;push esi	
    ;mov eax, [esp+12]	;   src
    ;mov ecx, [esp+16]	;   stride_src
    ;mov ebx, [esp+20]	;   ref
    ;mov edx, [esp+24]	;   stride_ref
    ;mov esi, [esp+28]	;   base_cost
    %assign  push_num 0
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION	r1, r1d
    SIGN_EXTENSION	r3, r3d
    pxor	xmm7,	xmm7
    
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41	r0, r2, r1, r3
    SAD_16x16_LINE_SSE41E	r0, r2, r1, r3

    pxor	xmm0,	xmm0
    movdqa	xmm6,	xmm7
    punpcklwd	xmm6,	xmm0
    punpckhwd	xmm7,	xmm0

    movdqa	xmm5,	[r4]
    movdqa	xmm4,	xmm5
    punpcklwd	xmm4,	xmm0
    punpckhwd	xmm5,	xmm0

    paddd	xmm4,	xmm6
    paddd	xmm5,	xmm7
    movdqa	xmm3,	xmm4
    pminud	xmm3,	xmm5
    pshufd	xmm2,	xmm3,	01001110B
    pminud	xmm2,	xmm3
    pshufd	xmm3,	xmm2,	10110001B
    pminud	xmm2,	xmm3
    movd	retrd,	xmm2
    pcmpeqd 	xmm4,	xmm2
    movmskps	r2d, xmm4	
    bsf		r1d,	r2d
    jnz	near WRITE_INDEX

    pcmpeqd 	xmm5,	xmm2
    movmskps	r2d, xmm5	
    bsf		r1d,	r2d
    add		r1d,	4

WRITE_INDEX:	
    mov		[r5],	r1d
    POP_XMM
    LOAD_6_PARA_POP	
    ret

;**********************************************************************************************************************************
;
;	uint32_t pixel_sad_8x8_hor8_sse41( uint8_t *src, int32_t stride_src, uint8_t *ref, int32_t stride_ref, uint16_t base_cost[8], int32_t *index_min_cost )
;
;	\note:
;		src and ref is optional to align with 16 due inter 8x8
;	\return value:
;		return minimal SAD cost, according index carried by index_min_cost
;
;**********************************************************************************************************************************
; try 8 mv via offset
; xmm7 store sad costs
%macro   SAD_8x8_LINE_SSE41  4	; src, ref, stride_src, stride_ref
    movdqu		xmm0, [%1]
    movdqu		xmm1, [%2]
    movdqa		xmm2, xmm1
    
    mpsadbw		xmm1, xmm0, 0	; 000 B
    paddw		xmm7, xmm1		; accumulate cost
    
    mpsadbw		xmm2, xmm0, 5	; 101 B
    paddw		xmm7, xmm2		; accumulate cost
    
    add			%1, %3
    add			%2, %4               
%endmacro	; end of SAD_8x8_LINE_SSE41
%macro   SAD_8x8_LINE_SSE41E  4	; src, ref, stride_src, stride_ref
    movdqu		xmm0, [%1]
    movdqu		xmm1, [%2]
    movdqa		xmm2, xmm1
    
    mpsadbw		xmm1, xmm0, 0	; 000 B
    paddw		xmm7, xmm1		; accumulate cost
    
    mpsadbw		xmm2, xmm0, 5	; 101 B
    paddw		xmm7, xmm2		; accumulate cost    
%endmacro	; end of SAD_8x8_LINE_SSE41E

WELS_EXTERN pixel_sad_8x8_hor8_sse41	
    %assign  push_num 0
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION	r1, r1d
    SIGN_EXTENSION	r3, r3d   
    movdqa xmm7, [r4]	;	load base cost list    
    
    SAD_8x8_LINE_SSE41	r0, r2, r1, r3
    SAD_8x8_LINE_SSE41	r0, r2, r1, r3
    SAD_8x8_LINE_SSE41	r0, r2, r1, r3
    SAD_8x8_LINE_SSE41	r0, r2, r1, r3
    
    SAD_8x8_LINE_SSE41	r0, r2, r1, r3
    SAD_8x8_LINE_SSE41	r0, r2, r1, r3
    SAD_8x8_LINE_SSE41	r0, r2, r1, r3
    SAD_8x8_LINE_SSE41E	r0, r2, r1, r3
    
    phminposuw	xmm0, xmm7	; horizon search the minimal sad cost and its index
    movd	retrd, xmm0	; for return: DEST[15:0] <- MIN, DEST[31:16] <- INDEX
    mov		r1d, retrd
    and		retrd, 0xFFFF
    sar		r1d, 16
    mov		[r5], r1d
        
    POP_XMM
    LOAD_6_PARA_POP	
    ret

