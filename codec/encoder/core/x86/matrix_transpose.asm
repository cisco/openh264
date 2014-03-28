;*!
;* \copy
;*     Copyright (c)  2009-2013, Cisco Systems
;*     All rights reserved.
;*
;*     Redistribution and use in source and binary forms, with or without
;*     modification, are permitted provided that the following conditions
;*     are met:
;*
;*        ?Redistributions of source code must retain the above copyright
;*          notice, this list of conditions and the following disclaimer.
;*
;*        ?Redistributions in binary form must reproduce the above copyright
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
;*************************************************************************/

%include "asm_inc.asm"

SECTION .text

WELS_EXTERN transpose_matrix_block_16x16_sse2
; void transpose_matrix_block_16x16_sse2( void *dst/*16x16*/, const int32_t dst_stride, void *src/*16x16*/, const int32_t src_stride );
	;push ebx
	;push esi
	;push edi
	;mov edi, [esp+16]	; dst
	;mov edx, [esp+20]	; dst_stride
	;mov esi, [esp+24]	; src
	;mov ebx, [esp+28]	; src_stride
	push r4
	push r5
	%assign push_num 2
	LOAD_4_PARA
	PUSH_XMM 8
	SIGN_EXTENSION	r1, r1d
	SIGN_EXTENSION	r3, r3d

	;mov eax, esp
	;and eax, 0Fh
	;sub esp, 10h
	;sub esp, eax
	;lea ecx, [ebx+ebx*2]	; src_stride * 3
	mov r4, r7
	and r4, 0Fh
	sub r7, 10h
	sub r7, r4
	lea r5, [r3+r3*2]
	; top 8x16 block
	movdqa xmm0, [r2]
	movdqa xmm1, [r2+r3]	
	movdqa xmm2, [r2+r3*2]
	movdqa xmm3, [r2+r5]
	lea r2, [r2+r3*4]
	movdqa xmm4, [r2]
	movdqa xmm5, [r2+r3]	
	movdqa xmm6, [r2+r3*2]
	
	;in:  m0, m1, m2, m3, m4, m5, m6, m7
	;out: m4, m2, m3, m7, m5, m1, m6, m0
	TRANSPOSE_8x16B_SSE2	xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2+r5], [r7]	
	
	TRANSPOSE8x16_WRITE_SSE2		r0, r1

	; bottom 8x16 block
	lea	r2, [r2+r3*4]	
	movdqa xmm0, [r2]
	movdqa xmm1, [r2+r3]	
	movdqa xmm2, [r2+r3*2]
	movdqa xmm3, [r2+r5]
	lea r2, [r2+r3*4]
	movdqa xmm4, [r2]
	movdqa xmm5, [r2+r3]	
	movdqa xmm6, [r2+r3*2]
	
	;in:  m0, m1, m2, m3, m4, m5, m6, m7
	;out: m4, m2, m3, m7, m5, m1, m6, m0
	TRANSPOSE_8x16B_SSE2	xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2+r5], [r7]

	mov r5, r1
	sal r5, 4
	sub r0, r5
	lea r0, [r0+r1*2+8]
	TRANSPOSE8x16_WRITE_SSE2		r0, r1

	add r7, r4
	add r7, 10h
	POP_XMM
	LOAD_4_PARA_POP
	pop r5
	pop r4
	ret

WELS_EXTERN transpose_matrix_blocks_x16_sse2
; void transpose_matrix_blocks_x16_sse2( void *dst/*W16x16*/, const int32_t dst_stride, void *src/*16xW16*/, const int32_t src_stride, const int32_t num_blocks );
	;%define		push_size		16
	;%define		local_size		4+16
	;%define		dst				esp+eax+push_size+local_size+4
	;%define		dst_stride		esp+eax+push_size+local_size+8
	;%define		src				esp+eax+push_size+local_size+12
	;%define		src_stride		esp+eax+push_size+local_size+16
	;%define		num_blk			esp+eax+push_size+local_size+20
	;%define		alg_mem			esp
	;%define		tmp_num_blk		esp+16
	;push ebx
	;push esi
	;push edi
	;push ebp

	;sub	esp,	local_size

	;mov eax, esp
	;and eax, 0Fh
	;sub esp, eax
	
	;mov edi, [dst]			; dst
	;mov edx, [dst_stride]	; dst_stride
	;mov esi, [src]			; src
	;mov ebx, [src_stride]	; src_stride
	;mov ecx, [num_blk]		; num_blocks_16x16

	;mov	[tmp_num_blk],	ecx
	push r5
	push r6
	%assign push_num 2
	LOAD_5_PARA
	PUSH_XMM 8
	SIGN_EXTENSION  r1, r1d
	SIGN_EXTENSION  r3, r3d
	SIGN_EXTENSION  r4, r4d
	mov r5, r7
	and r5, 0Fh
	sub r7, 10h
	sub r7, r5
TRANSPOSE_LOOP_SSE2:
	; explictly loading next loop data	
	lea	r6, [r2+r3*8]
	push r4	
%rep 8
	mov	r4, [r6]
	mov	r4, [r6+r3]
	lea	r6, [r6+r3*2]
%endrep
	pop r4
	; top 8x16 block
	movdqa xmm0, [r2]
	movdqa xmm1, [r2+r3]	
	lea r2, [r2+r3*2]
	movdqa xmm2, [r2]
	movdqa xmm3, [r2+r3]
	lea r2, [r2+r3*2]
	movdqa xmm4, [r2]
	movdqa xmm5, [r2+r3]
	lea r2, [r2+r3*2]
	movdqa xmm6, [r2]
	
	;in:  m0, m1, m2, m3, m4, m5, m6, m7
	;out: m4, m2, m3, m7, m5, m1, m6, m0
	TRANSPOSE_8x16B_SSE2	xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2+r3], [r7]		
	TRANSPOSE8x16_WRITE_ALT_SSE2		r0, r1, r6
	lea	r2, [r2+r3*2]

	; bottom 8x16 block	
	movdqa xmm0, [r2]
	movdqa xmm1, [r2+r3]
	lea	r2, [r2+r3*2]
	movdqa xmm2, [r2]
	movdqa xmm3, [r2+r3]
	lea r2, [r2+r3*2]
	movdqa xmm4, [r2]
	movdqa xmm5, [r2+r3]
	lea	r2, [r2+r3*2]
	movdqa xmm6, [r2]
	
	;in:  m0, m1, m2, m3, m4, m5, m6, m7
	;out: m4, m2, m3, m7, m5, m1, m6, m0
	TRANSPOSE_8x16B_SSE2	xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r2+r3], [r7]	
	TRANSPOSE8x16_WRITE_ALT_SSE2		r0+8, r1, r6
	lea	r2, [r2+r3*2]
	lea r0, [r0+16]
	dec r4
	jg near TRANSPOSE_LOOP_SSE2
	
	add r7, r5
	add r7, 10h
	POP_XMM
	LOAD_5_PARA_POP
	pop r6
	pop r5
	ret

WELS_EXTERN transpose_matrix_block_8x8_mmx
; void transpose_matrix_block_8x8_mmx( void *dst/*8x8*/, const int32_t dst_stride, void *src/*8x8*/, const int32_t src_stride );
	;push ebx

	;mov edx, [esp+8]	; dst
	;mov ebx, [esp+12]	; dst_stride
	;mov eax, [esp+16]	; src
	;mov ecx, [esp+20]	; src_stride
	%assign push_num 0
	LOAD_4_PARA
	SIGN_EXTENSION  r1, r1d
	SIGN_EXTENSION  r3, r3d
	sub	r7, 8

	movq mm0, [r2]
	movq mm1, [r2+r3]
	lea r2, [r2+2*r3]
	movq mm2, [r2]
	movq mm3, [r2+r3]
	lea r2, [r2+2*r3]
	movq mm4, [r2]
	movq mm5, [r2+r3]
	lea r2, [r2+2*r3]
	movq mm6, [r2]	
	
	;in:  m0, m1, m2, m3, m4, m5, m6, m7
	;out: m0, m3, m5, m2, m7, m1, m6, m4
	TRANSPOSE_8x8B_MMX mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7, [r2+r3], [r7]
	
	TRANSPOSE8x8_WRITE_MMX r0, r1
	
	emms
	add r7, 8
	LOAD_4_PARA_POP
	ret

WELS_EXTERN transpose_matrix_blocks_x8_mmx
; void transpose_matrix_blocks_x8_mmx( void *dst/*8xW8*/, const int32_t dst_stride, void *src/*W8x8*/, const int32_t src_stride, const int32_t num_blocks );
	;%define _PUSH	16
	;push ebx	
	;push esi
	;push edi
	;push ebp
	
	;mov edi, [esp+_PUSH+4]	; dst
	;mov ebx, [esp+_PUSH+8]	; dst_stride
	;mov esi, [esp+_PUSH+12]	; src
	;mov eax, [esp+_PUSH+16]	; src_stride
	;mov ecx, [esp+_PUSH+20]	; num_blocks_x8
	push r5
	push r6
	%assign push_num 2
	LOAD_5_PARA
	SIGN_EXTENSION  r1, r1d
	SIGN_EXTENSION  r3, r3d
	SIGN_EXTENSION  r4, r4d	
	sub	r7, 8

	lea	r5, [r2+r3*8]

TRANSPOSE_BLOCKS_X8_LOOP_MMX:
	; explictly loading next loop data
%rep 4
	mov r6, [r5]
	mov r6, [r5+r3]
	lea	r5, [r5+r3*2]
%endrep
	movq mm0, [r2]
	movq mm1, [r2+r3]
	lea r2, [r2+2*r3]
	movq mm2, [r2]
	movq mm3, [r2+r3]
	lea r2, [r2+2*r3]
	movq mm4, [r2]
	movq mm5, [r2+r3]
	lea r2, [r2+2*r3]
	movq mm6, [r2]	
	
	;in:  m0, m1, m2, m3, m4, m5, m6, m7
	;out: m0, m3, m5, m2, m7, m1, m6, m4
	TRANSPOSE_8x8B_MMX mm0, mm1, mm2, mm3, mm4, mm5, mm6, mm7, [r2+r3], [r7]
	
	TRANSPOSE8x8_WRITE_ALT_MMX r0, r1, r6
	lea r0, [r0+8]
	lea r2, [r2+2*r3]
	dec r4
	jg near TRANSPOSE_BLOCKS_X8_LOOP_MMX
	
	emms
	add r7, 8
	LOAD_5_PARA_POP
	pop r6
	pop r5
	ret
