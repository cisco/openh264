;*!
;* \copy
;*     Copyright (c)  2010-2013, Cisco Systems
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
;*	vaa.asm
;*
;*	Abstract
;*      sse2 for pVaa routines
;*
;*  History
;*      04/14/2010	Created
;*
;*************************************************************************/
%include "asm_inc.asm"
BITS 32

;***********************************************************************
; Macros and other preprocessor constants
;***********************************************************************

;%macro SUM_SSE2	4	; dst, pSrc, zero, pack1_8x2
;	movdqa %1, %2
;	punpcklbw %1, %3
;	punpckhbw %2, %3
;	paddw %1, %2
;	pmaddwd %1, %4
;	pshufd %2, %1, 04Eh	; 01001110 B
;	paddd %1, %2
;	pshufd %2, %1, 0B1h	; 10110001 B
;	paddd %1, %2
;%endmacro	; END OF SUM_SSE2

; by comparing it outperforms than phaddw(SSSE3) sets
%macro SUM_WORD_8x2_SSE2	2	; dst(pSrc), tmp
	; @sum_8x2 begin
	pshufd %2, %1, 04Eh	; 01001110 B
	paddw %1, %2
	pshuflw %2, %1, 04Eh	; 01001110 B
	paddw %1, %2
	pshuflw %2, %1, 0B1h	; 10110001 B
	paddw %1, %2
	; end of @sum_8x2
%endmacro	; END of SUM_WORD_8x2_SSE2

%macro SUM_SQR_SSE2	3	; dst, pSrc, zero
	movdqa %1, %2
	punpcklbw %1, %3
	punpckhbw %2, %3
	pmaddwd %1, %1
	pmaddwd %2, %2
	paddd %1, %2
	pshufd %2, %1, 04Eh	; 01001110 B
	paddd %1, %2
	pshufd %2, %1, 0B1h	; 10110001 B
	paddd %1, %2
%endmacro	; END OF SUM_SQR_SSE2

%macro VAA_AVG_BLOCK_SSE2 6 ; dst, t0, t1, t2, t3, t4
	movdqa %1, [esi    ]	; line 0
	movdqa %2, [esi+ecx]	; line 1
	movdqa %3, %1
	punpcklbw %1, xmm7
	punpckhbw %3, xmm7
	movdqa %4, %2
	punpcklbw %4, xmm7
	punpckhbw %2, xmm7
	paddw %1, %4
	paddw %2, %3
	movdqa %3, [esi+ebx]	; line 2
	movdqa %4, [esi+edx]	; line 3
	movdqa %5, %3
	punpcklbw %3, xmm7
	punpckhbw %5, xmm7
	movdqa %6, %4
	punpcklbw %6, xmm7
	punpckhbw %4, xmm7
	paddw %3, %6
	paddw %4, %5
	paddw %1, %3	; block 0, 1
	paddw %2, %4	; block 2, 3
	pshufd %3, %1, 0B1h
	pshufd %4, %2, 0B1h
	paddw %1, %3
	paddw %2, %4
	movdqa %3, %1
	movdqa %4, %2
	pshuflw %5, %1, 0B1h
	pshufhw %6, %3, 0B1h
	paddw %1, %5
	paddw %3, %6
	pshuflw %5, %2, 0B1h
	pshufhw %6, %4, 0B1h
	paddw %2, %5
	paddw %4, %6
	punpcklwd %1, %2
	punpckhwd %3, %4
	punpcklwd %1, %3
	psraw %1, $4
%endmacro

%macro VAA_AVG_BLOCK_SSSE3 6 ; dst, t0, t1, t2, t3, t4
	movdqa %1, [esi    ]	; line 0
	movdqa %2, [esi+ecx]	; line 1
	movdqa %3, %1
	punpcklbw %1, xmm7
	punpckhbw %3, xmm7
	movdqa %4, %2
	punpcklbw %4, xmm7
	punpckhbw %2, xmm7
	paddw %1, %4
	paddw %2, %3
	movdqa %3, [esi+ebx]	; line 2
	movdqa %4, [esi+edx]	; line 3
	movdqa %5, %3
	punpcklbw %3, xmm7
	punpckhbw %5, xmm7
	movdqa %6, %4
	punpcklbw %6, xmm7
	punpckhbw %4, xmm7
	paddw %3, %6
	paddw %4, %5
	paddw %1, %3	; block 0, 1
	paddw %2, %4	; block 2, 3
	phaddw %1, %2	; block[0]: 0-15, 16-31; block[1]: 32-47, 48-63; ..
	phaddw %1, xmm7	; block[0]: 0-15; block[1]: 16-31; block[2]: 32-47; block[3]: 48-63; ....
	psraw %1, $4
%endmacro

%macro WELS_SAD_16x2_SSE2  0
	movdqa	xmm1,	[esi]
	movdqa	xmm2,	[edi]
	movdqa	xmm3,	[esi+ebx]
	movdqa	xmm4,	[edi+ebx]
	psadbw	xmm1,	xmm2
	psadbw	xmm3,	xmm4
	paddd	xmm6,	xmm1
	paddd	xmm6,	xmm3
	lea		esi,	[esi+ebx*2]
	lea		edi,	[edi+ebx*2]	
%endmacro

%macro	WELS_SAD_SUM_SQSUM_16x1_SSE2 0
	movdqa	xmm1,	[esi]
	movdqa	xmm2,	[edi]
	movdqa	xmm3,	xmm1
	psadbw	xmm3,	xmm2
	paddd	xmm6,	xmm3
	
	movdqa	xmm3,	xmm1
	psadbw	xmm3,	xmm0
	paddd	xmm5,	xmm3
	
	movdqa		xmm2,	xmm1
	punpcklbw	xmm1,	xmm0
	punpckhbw	xmm2,	xmm0
	pmaddwd		xmm1,	xmm1
	pmaddwd		xmm2,	xmm2
	paddd		xmm4,	xmm1
	paddd		xmm4,	xmm2
	
	add		esi,	ebx
	add		edi,	ebx
%endmacro

%macro	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2 0
	movdqa	xmm1,	[esi]
	movdqa	xmm2,	[edi]
	movdqa	xmm3,	xmm1
	psadbw	xmm3,	xmm2
	paddd	xmm7,	xmm3	; sad
	
	movdqa	xmm3,	xmm1
	pmaxub	xmm3,	xmm2
	pminub	xmm2,	xmm1
	psubb	xmm3,	xmm2	; diff
	
	movdqa	xmm2,	xmm1
	psadbw	xmm2,	xmm0
	paddd	xmm6,	xmm2	; sum
	
	movdqa		xmm2,	xmm1
	punpcklbw	xmm1,	xmm0
	punpckhbw	xmm2,	xmm0
	pmaddwd		xmm1,	xmm1
	pmaddwd		xmm2,	xmm2
	paddd		xmm5,	xmm1
	paddd		xmm5,	xmm2	; sqsum
	
	movdqa		xmm1,	xmm3
	punpcklbw	xmm1,	xmm0
	punpckhbw	xmm3,	xmm0
	pmaddwd		xmm1,	xmm1
	pmaddwd		xmm3,	xmm3
	paddd		xmm4,	xmm1
	paddd		xmm4,	xmm3	; sqdiff
	
	add		esi,	ebx
	add		edi,	ebx
%endmacro

%macro	WELS_SAD_SD_MAD_16x1_SSE2	4
%define sad_reg			%1
%define	sum_cur_reg		%2
%define sum_ref_reg		%3
%define	mad_reg			%4
	movdqa	xmm1,		[esi]
	movdqa	xmm2,		[edi]
	movdqa	xmm3,		xmm1
	psadbw	xmm3,		xmm0
	paddd	sum_cur_reg,			xmm3	; sum_cur
	movdqa	xmm3,		xmm2
	psadbw	xmm3,		xmm0
	paddd	sum_ref_reg,			xmm3	; sum_ref
	
	movdqa	xmm3,		xmm1
	pmaxub	xmm3,		xmm2
	pminub	xmm2,		xmm1
	psubb	xmm3,		xmm2	; abs diff
	pmaxub	mad_reg,	xmm3	; max abs diff
	
	psadbw	xmm3,		xmm0
	paddd	sad_reg,	xmm3	; sad
	
	add			esi,		ebx
	add			edi,		ebx
%endmacro


%macro	WELS_MAX_REG_SSE2	1	; xmm1, xmm2, xmm3 can be used
%define max_reg  %1
	movdqa	xmm1,		max_reg
	psrldq	xmm1,		4
	pmaxub	max_reg,	xmm1
	movdqa	xmm1,		max_reg
	psrldq	xmm1,		2
	pmaxub	max_reg,	xmm1
	movdqa	xmm1,		max_reg
	psrldq	xmm1,		1
	pmaxub	max_reg,	xmm1
%endmacro

%macro	WELS_SAD_BGD_SQDIFF_16x1_SSE2	4
%define sad_reg		%1
%define	sum_reg		%2
%define mad_reg		%3
%define sqdiff_reg	%4
	movdqa		xmm1,		[esi]
	movdqa		xmm2,		xmm1
	movdqa		xmm3,		xmm1
	punpcklbw	xmm2,		xmm0
	punpckhbw	xmm3,		xmm0
	pmaddwd		xmm2,		xmm2
	pmaddwd		xmm3,		xmm3
	paddd		xmm2,		xmm3
	movdqa		xmm3,		xmm2
	psllq		xmm2,		32
	psrlq		xmm3,		32
	psllq		xmm3,		32
	paddd		xmm2,		xmm3
	paddd		sad_reg,	xmm2		; sqsum
	
	movdqa	xmm2,		[edi]
	movdqa	xmm3,		xmm1
	psadbw	xmm3,		xmm0
	paddd	sum_reg,			xmm3	; sum_cur
	movdqa	xmm3,		xmm2
	psadbw	xmm3,		xmm0
	pslldq	xmm3,		4
	paddd	sum_reg,			xmm3	; sum_ref
	
	movdqa	xmm3,		xmm1
	pmaxub	xmm3,		xmm2
	pminub	xmm2,		xmm1
	psubb	xmm3,		xmm2	; abs diff
	pmaxub	mad_reg,	xmm3	; max abs diff
	
	movdqa	xmm1,		xmm3
	psadbw	xmm3,		xmm0
	paddd	sad_reg,	xmm3	; sad

	movdqa		xmm3,	xmm1
	punpcklbw	xmm1,	xmm0
	punpckhbw	xmm3,	xmm0
	pmaddwd		xmm1,	xmm1
	pmaddwd		xmm3,	xmm3
	paddd		sqdiff_reg,	xmm1
	paddd		sqdiff_reg,	xmm3	; sqdiff
	
	add		esi,	ebx
	add		edi,	ebx
%endmacro


;***********************************************************************
; Local Data (Read Only)
;***********************************************************************

;SECTION .rodata align=16

;ALIGN 16
;pack1_8x2:
;	dw 1, 1, 1, 1, 1, 1, 1, 1

;***********************************************************************
; Code
;***********************************************************************

SECTION .text

WELS_EXTERN rc_sad_frame_sse2
;***********************************************************************
;	uint32_t rc_sad_frame_sse2(	uint8_t *ref_orig, uint8_t *cur_orig, const int mb_width, const int iPicHeight, const int iPicStride );
;***********************************************************************
ALIGN 16
rc_sad_frame_sse2:
	push esi
	push edi
	push ebp
	push ebx
	push edx

	mov esi, [esp+24]
	mov edi, [esp+28]
	mov ebx, [esp+32]
	mov ecx, [esp+36]
	mov edx, [esp+40]
	pxor xmm0, xmm0	
.hloop:
	mov eax, ebx
	mov ebp, $0
.wloop:
	movdqa xmm1, [esi+ebp]
	movdqa xmm2, [edi+ebp]
	psadbw xmm1, xmm2
	pshufd xmm2, xmm1, 0f6h	; 11110110 B ; movhlps for float
	paddd xmm1, xmm2
	paddd xmm0, xmm1	
	add ebp, 010h
	dec eax
	jnz near .wloop
	lea esi, [esi+edx]
	lea edi, [edi+edx]
	dec ecx
	jnz near .hloop

	movd eax, xmm0
	pop edx
	pop ebx
	pop ebp
	pop edi
	pop esi
	ret


WELS_EXTERN SampleVariance16x16_sse2
;***********************************************************************
;   void SampleVariance16x16_sse2(	uint8_t * y_ref, int32_t y_ref_stride, uint8_t * y_src, int32_t y_src_stride,SMotionTextureUnit* pMotionTexture );
;***********************************************************************
ALIGN 16
SampleVariance16x16_sse2:	
	push esi
	push edi
	push ebx
	
	sub esp, 16
	%define SUM			[esp]
	%define SUM_CUR		[esp+4]
	%define SQR			[esp+8]
	%define SQR_CUR		[esp+12]
	%define PUSH_SIZE	28	; 12 + 16	

	mov edi, [esp+PUSH_SIZE+4]	; y_ref
	mov edx, [esp+PUSH_SIZE+8]	; y_ref_stride	
	mov esi, [esp+PUSH_SIZE+12]	; y_src
	mov eax, [esp+PUSH_SIZE+16]	; y_src_stride
	mov ecx, 010h				; height = 16

	pxor xmm7, xmm7
	movdqu SUM, xmm7

.hloops:
	movdqa xmm0, [edi]		; y_ref
	movdqa xmm1, [esi]		; y_src
	movdqa xmm2, xmm0		; store first for future process
	movdqa xmm3, xmm1
	; sum += diff;
	movdqa xmm4, xmm0
	psadbw xmm4, xmm1		; 2 parts, [0,..,15], [64,..,79]
	; to be continued for sum
	pshufd xmm5, xmm4, 0C6h	; 11000110 B
	paddw xmm4, xmm5
	movd ebx, xmm4
	add SUM, ebx

	; sqr += diff * diff;
	pmaxub xmm0, xmm1
	pminub xmm1, xmm2
	psubb xmm0, xmm1				; diff	
	SUM_SQR_SSE2 xmm1, xmm0, xmm7	; dst, pSrc, zero
	movd ebx, xmm1
	add SQR, ebx

	; sum_cur += y_src[x];
	movdqa xmm0, xmm3		; cur_orig
	movdqa xmm1, xmm0
	punpcklbw xmm0, xmm7
	punpckhbw xmm1, xmm7
	paddw xmm0, xmm1		; 8x2
	SUM_WORD_8x2_SSE2 xmm0, xmm1	
	movd ebx, xmm0
	and ebx, 0ffffh
	add SUM_CUR, ebx

	; sqr_cur += y_src[x] * y_src[x];
	SUM_SQR_SSE2 xmm0, xmm3, xmm7	; dst, pSrc, zero
	movd ebx, xmm0
	add SQR_CUR, ebx
	
	lea edi, [edi+edx]
	lea esi, [esi+eax]
	dec ecx
	jnz near .hloops
	
	mov ebx, 0
	mov bx, word SUM
	sar ebx, 8
	imul ebx, ebx
	mov ecx, SQR
	sar ecx, 8
	sub ecx, ebx
	mov edi, [esp+PUSH_SIZE+20]	; pMotionTexture
	mov [edi], cx				; to store uiMotionIndex
	mov ebx, 0
	mov bx, word SUM_CUR
	sar ebx, 8
	imul ebx, ebx
	mov ecx, SQR_CUR
	sar ecx, 8
	sub ecx, ebx
	mov [edi+2], cx				; to store uiTextureIndex
	
	%undef SUM
	%undef SUM_CUR
	%undef SQR
	%undef SQR_CUR
	%undef PUSH_SIZE

	add esp, 16	
	pop ebx
	pop edi
	pop esi	

	ret

; , 6/7/2010

%ifndef NO_DYNAMIC_VP
WELS_EXTERN AnalysisVaaInfoIntra_sse2
;***********************************************************************
;	int32_t AnalysisVaaInfoIntra_sse2(	uint8_t *pDataY, const int32_t linesize );
;***********************************************************************
ALIGN 16
AnalysisVaaInfoIntra_sse2:
	push ebx
	push edx
	push esi
	push edi
	push ebp

	mov ebp, esp
	and ebp, 0fh
	sub esp, ebp
	sub esp, 32	
	%define PUSH_SIZE	52	; 20 + 32

	mov esi, [esp+ebp+PUSH_SIZE+4]	; data_y
	mov ecx, [esp+ebp+PUSH_SIZE+8]	; linesize

	mov ebx, ecx
	sal ebx, $1			; linesize x 2 [ebx]
	mov edx, ebx
	add edx, ecx		; linesize x 3 [edx]
	mov eax, ebx
	sal eax, $1			; linesize x 4 [eax]
	
	pxor xmm7, xmm7
	
	; loops
	VAA_AVG_BLOCK_SSE2 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
	movq [esp], xmm0	

	lea esi, [esi+eax]
	VAA_AVG_BLOCK_SSE2 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
	movq [esp+8], xmm0	

	lea esi, [esi+eax]
	VAA_AVG_BLOCK_SSE2 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
	movq [esp+16], xmm0	

	lea esi, [esi+eax]
	VAA_AVG_BLOCK_SSE2 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
	movq [esp+24], xmm0
		
	movdqa xmm0, [esp]		; block 0~7
	movdqa xmm1, [esp+16]	; block 8~15
	movdqa xmm2, xmm0
	paddw xmm0, xmm1
	SUM_WORD_8x2_SSE2 xmm0, xmm3
	
	pmullw xmm1, xmm1
	pmullw xmm2, xmm2
	movdqa xmm3, xmm1
	movdqa xmm4, xmm2
	punpcklwd xmm1, xmm7
	punpckhwd xmm3, xmm7
	punpcklwd xmm2, xmm7
	punpckhwd xmm4, xmm7
	paddd xmm1, xmm2
	paddd xmm3, xmm4
	paddd xmm1, xmm3
	pshufd xmm2, xmm1, 01Bh
	paddd xmm1, xmm2
	pshufd xmm2, xmm1, 0B1h
	paddd xmm1, xmm2
	
	movd ebx, xmm0
	and ebx, 0ffffh		; effective low word truncated
	mov ecx, ebx
	imul ebx, ecx
	sar ebx, $4
	movd eax, xmm1
	sub eax, ebx
	
	%undef PUSH_SIZE
	add esp, 32
	add esp, ebp
	pop ebp
	pop edi
	pop esi
	pop edx
	pop ebx
	ret
        
WELS_EXTERN AnalysisVaaInfoIntra_ssse3
;***********************************************************************
;	int32_t AnalysisVaaInfoIntra_ssse3(	uint8_t *pDataY, const int32_t linesize );
;***********************************************************************
ALIGN 16
AnalysisVaaInfoIntra_ssse3:
	push ebx
	push edx
	push esi
	push edi
	push ebp

	mov ebp, esp
	and ebp, 0fh
	sub esp, ebp
	sub esp, 32	
	%define PUSH_SIZE	52	; 20 + 32

	mov esi, [esp+ebp+PUSH_SIZE+4]	; data_y
	mov ecx, [esp+ebp+PUSH_SIZE+8]	; linesize

	mov ebx, ecx
	sal ebx, $1			; linesize x 2 [ebx]
	mov edx, ebx
	add edx, ecx		; linesize x 3 [edx]
	mov eax, ebx
	sal eax, $1			; linesize x 4 [eax]
	
	pxor xmm7, xmm7
	
	; loops
	VAA_AVG_BLOCK_SSSE3 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
	movq [esp], xmm0	

	lea esi, [esi+eax]
	VAA_AVG_BLOCK_SSSE3 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6
	movq [esp+8], xmm1	

	lea esi, [esi+eax]
	VAA_AVG_BLOCK_SSSE3 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
	movq [esp+16], xmm0	

	lea esi, [esi+eax]
	VAA_AVG_BLOCK_SSSE3 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6
	movq [esp+24], xmm1
		
	movdqa xmm0, [esp]		; block 0~7
	movdqa xmm1, [esp+16]	; block 8~15
	movdqa xmm2, xmm0
	paddw xmm0, xmm1
	SUM_WORD_8x2_SSE2 xmm0, xmm3	; better performance than that of phaddw sets

	pmullw xmm1, xmm1
	pmullw xmm2, xmm2
	movdqa xmm3, xmm1
	movdqa xmm4, xmm2
	punpcklwd xmm1, xmm7
	punpckhwd xmm3, xmm7
	punpcklwd xmm2, xmm7
	punpckhwd xmm4, xmm7
	paddd xmm1, xmm2
	paddd xmm3, xmm4
	paddd xmm1, xmm3
	pshufd xmm2, xmm1, 01Bh
	paddd xmm1, xmm2
	pshufd xmm2, xmm1, 0B1h
	paddd xmm1, xmm2
	
	movd ebx, xmm0
	and ebx, 0ffffh		; effective low work truncated
	mov ecx, ebx
	imul ebx, ecx
	sar ebx, $4
	movd eax, xmm1
	sub eax, ebx
	
	%undef PUSH_SIZE
	add esp, 32
	add esp, ebp
	pop ebp
	pop edi
	pop esi
	pop edx
	pop ebx
	ret
%endif
	
	

WELS_EXTERN abs_difference_mbrow_sse2
;*************************************************************************************************************
;void abs_difference_mbrow_sse2( uint8_t *ref_orig, uint8_t *cur_orig, int32_t iPicStride, 
;								 int32_t gom_pixel_num, int32_t *pSum)
;*************************************************************************************************************
ALIGN 16
abs_difference_mbrow_sse2:
%define		ref_orig			esp + pushsize + 4
%define		cur_orig			esp + pushsize + 8
%define		iPicStride			esp + pushsize + 12
%define		gom_pixel_num		esp + pushsize + 16
%define		pSum				esp + pushsize + 20
%define		pushsize	12
	push	esi
	push	edi
	push	ebx
	mov		esi,	[ref_orig]
	mov		edi,	[cur_orig]
	mov		ebx,	[iPicStride]
	mov		eax,	[gom_pixel_num]
	mov		ecx,	16					;MB_WIDTH_LUMA
	pxor	xmm0,	xmm0
mb_width_loop_p:
	mov		edx,	esi
	add		edx,	eax			; end address
gom_row_loop_p:
	movdqa	xmm1,	[esi]
	movdqa	xmm2,	[edi]
	psadbw	xmm1,	xmm2
	paddd	xmm0,	xmm1
	add		esi,	16
	add		edi,	16
	cmp		esi,	edx
	jl		gom_row_loop_p
	
	sub		esi,	eax
	sub		edi,	eax
	add		esi,	ebx
	add		edi,	ebx
	loop	mb_width_loop_p
	
	movdqa	xmm1,	xmm0
	psrldq	xmm1,	8
	paddd	xmm1,	xmm0
	movd	eax,	xmm1
	mov		edx,	[pSum]	; pSum
	add		[edx],	eax

%undef		ref_orig
%undef		cur_orig
%undef		iPicStride
%undef		gom_pixel_num
%undef		pSum
%undef		pushsize	
	pop		ebx
	pop		edi
	pop		esi
	ret




WELS_EXTERN sum_sqrsum_mbrow_sse2
;*************************************************************************************************************
;void sum_sqrsum_mbrow_sse2( uint8_t *cur_orig, int32_t iPicStride, 
;							 int32_t gom_pixel_num, int32_t *pSum, int32_t *pSqrSum)
;*************************************************************************************************************
ALIGN 16
sum_sqrsum_mbrow_sse2:
%define		cur_orig			esp + pushsize + 4
%define		iPicStride			esp + pushsize + 8
%define		gom_pixel_num		esp + pushsize + 12
%define		pSum				esp + pushsize + 16
%define		pSqrSum				esp + pushsize + 20
%define		pushsize			8
	push		esi
	push		ebx
	mov			esi,	[cur_orig]
	mov			eax,	[gom_pixel_num]
	mov			ebx,	[iPicStride]
	mov			ecx,	16					;MB_WIDTH_LUMA
	pxor		xmm0,	xmm0				; zero
	pxor		xmm1,	xmm1				; sum
	pxor		xmm2,	xmm2				; sqr sum
mb_width_loop_i:
	mov			edx,	esi
	add			edx,	eax			; end address
gom_row_loop_i:
	movdqa		xmm3,	[esi]
	movdqa		xmm4,	xmm3
	psadbw		xmm4,	xmm0
	paddd		xmm1,	xmm4
	movdqa		xmm4,	xmm3
	punpcklbw	xmm4,	xmm0
	punpckhbw	xmm3,	xmm0
	pmaddwd		xmm4,	xmm4
	pmaddwd		xmm3,	xmm3
	paddd		xmm2,	xmm3
	paddd		xmm2,	xmm4
	add			esi,	16
	cmp			esi,	edx
	jl			gom_row_loop_i
	
	sub			esi,	eax
	add			esi,	ebx
	loop		mb_width_loop_i
	
	movdqa		xmm3,	xmm1
	psrldq		xmm3,	8
	paddd		xmm1,	xmm3
	movd		eax,	xmm1
	mov			edx,	[pSum]
	add			[edx],	eax
	
	movdqa		xmm3,	xmm2
	psrldq		xmm3,	8
	paddd		xmm2,	xmm3
	movdqa		xmm3,	xmm2
	psrldq		xmm3,	4
	paddd		xmm2,	xmm3
	movd		eax,	xmm2
	mov			edx,	[pSqrSum]
	add			[edx],	eax


%undef		cur_orig
%undef		iPicStride
%undef		gom_pixel_num
%undef		pSum
%undef		pSqrSum
%undef		pushsize	
	pop			ebx
	pop			esi
	ret



WELS_EXTERN VAACalcSad_sse2
;*************************************************************************************************************
;void VAACalcSad_sse2( uint8_t *cur_data, uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight
;								int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8)
;*************************************************************************************************************


ALIGN 16
VAACalcSad_sse2:
%define		cur_data			esp + pushsize + 4
%define		ref_data			esp + pushsize + 8
%define		iPicWidth			esp + pushsize + 12
%define		iPicHeight			esp + pushsize + 16
%define		iPicStride			esp + pushsize + 20
%define		psadframe			esp + pushsize + 24
%define		psad8x8				esp + pushsize + 28
%define		pushsize	12
	push	esi
	push	edi
	push	ebx
	mov		esi,	[cur_data]
	mov		edi,	[ref_data]
	mov		ebx,	[iPicStride]
	mov		edx,	[psad8x8]
	mov		eax,	ebx
	
	shr		dword [iPicWidth],	4					; iPicWidth/16
	shr		dword [iPicHeight],	4					; iPicHeight/16
	shl		eax,	4								; iPicStride*16
	pxor	xmm0,	xmm0
	pxor	xmm7,	xmm7		; iFrameSad
height_loop:
	mov		ecx,	dword [iPicWidth]
	push	esi
	push	edi
width_loop:
	pxor	xmm6,	xmm6		;
	WELS_SAD_16x2_SSE2
	WELS_SAD_16x2_SSE2
	WELS_SAD_16x2_SSE2
	WELS_SAD_16x2_SSE2
	paddd	xmm7,		xmm6
	movd	[edx],		xmm6
	psrldq	xmm6,		8
	movd	[edx+4],	xmm6
	
	pxor	xmm6,	xmm6
	WELS_SAD_16x2_SSE2
	WELS_SAD_16x2_SSE2
	WELS_SAD_16x2_SSE2
	WELS_SAD_16x2_SSE2
	paddd	xmm7,		xmm6
	movd	[edx+8],	xmm6
	psrldq	xmm6,		8
	movd	[edx+12],	xmm6
	
	add		edx,	16
	sub		esi,	eax
	sub		edi,	eax
	add		esi,	16
	add		edi,	16
	
	dec		ecx
	jnz		width_loop
	
	pop		edi
	pop		esi
	add		esi,	eax
	add		edi,	eax
	
	dec	dword [iPicHeight]
	jnz		height_loop
	
	mov		edx,	[psadframe]
	movdqa	xmm5,	xmm7
	psrldq	xmm7,	8
	paddd	xmm7,	xmm5
	movd	[edx],	xmm7

%undef		cur_data
%undef		ref_data
%undef		iPicWidth
%undef		iPicHeight
%undef		iPicStride
%undef		psadframe
%undef		psad8x8
%undef		pushsize	
	pop		ebx
	pop		edi
	pop		esi
	ret
	
	
WELS_EXTERN VAACalcSadVar_sse2
;*************************************************************************************************************
;void VAACalcSadVar_sse2( uint8_t *cur_data, uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight 
;		int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8, int32_t *psum16x16, int32_t *psqsum16x16)
;*************************************************************************************************************


ALIGN 16
VAACalcSadVar_sse2:
%define		localsize		8
%define		cur_data			esp + pushsize + localsize + 4
%define		ref_data			esp + pushsize + localsize + 8
%define		iPicWidth			esp + pushsize + localsize + 12
%define		iPicHeight			esp + pushsize + localsize + 16
%define		iPicStride			esp + pushsize + localsize + 20
%define		psadframe			esp + pushsize + localsize + 24
%define		psad8x8				esp + pushsize + localsize + 28
%define		psum16x16			esp + pushsize + localsize + 32
%define		psqsum16x16			esp + pushsize + localsize + 36
%define		tmp_esi				esp + 0
%define		tmp_edi				esp + 4
%define		pushsize		16
	push	ebp
	push	esi
	push	edi
	push	ebx
	sub		esp,	localsize
	mov		esi,	[cur_data]
	mov		edi,	[ref_data]
	mov		ebx,	[iPicStride]
	mov		edx,	[psad8x8]
	mov		eax,	ebx
	
	shr		dword [iPicWidth],	4					; iPicWidth/16
	shr		dword [iPicHeight],	4					; iPicHeight/16
	shl		eax,	4							; iPicStride*16
	pxor	xmm0,	xmm0
	pxor	xmm7,	xmm7		; iFrameSad
var_height_loop:
	mov		ecx,	dword [iPicWidth]
	mov		[tmp_esi],	esi
	mov		[tmp_edi],	edi
var_width_loop:
	pxor	xmm6,	xmm6		; hiQuad_loQuad pSad8x8
	pxor	xmm5,	xmm5		; pSum16x16
	pxor	xmm4,	xmm4		; sqsum_16x16
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	paddd	xmm7,		xmm6
	movd	[edx],		xmm6
	psrldq	xmm6,		8
	movd	[edx+4],	xmm6
	
	pxor	xmm6,	xmm6
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	WELS_SAD_SUM_SQSUM_16x1_SSE2
	paddd	xmm7,		xmm6
	movd	[edx+8],	xmm6
	psrldq	xmm6,		8
	movd	[edx+12],	xmm6
	
	mov		ebp,	[psum16x16]
	movdqa	xmm1,	xmm5
	psrldq	xmm1,	8
	paddd	xmm5,	xmm1
	movd	[ebp],	xmm5
	add		dword [psum16x16], 4
	
	movdqa	xmm5,	xmm4
	psrldq	xmm5,	8
	paddd	xmm4,	xmm5
	movdqa	xmm3,	xmm4
	psrldq	xmm3,	4
	paddd	xmm4,	xmm3
	
	mov		ebp,	[psqsum16x16]
	movd	[ebp],	xmm4
	add		dword [psqsum16x16], 4
	
	add		edx,	16
	sub		esi,	eax
	sub		edi,	eax
	add		esi,	16
	add		edi,	16
	
	dec		ecx
	jnz		var_width_loop
	
	mov		esi,	[tmp_esi]
	mov		edi,	[tmp_edi]
	add		esi,	eax
	add		edi,	eax
	
	dec	dword [iPicHeight]
	jnz		var_height_loop
	
	mov		edx,	[psadframe]
	movdqa	xmm5,	xmm7
	psrldq	xmm7,	8
	paddd	xmm7,	xmm5
	movd	[edx],	xmm7

	add		esp,	localsize	
	pop		ebx
	pop		edi
	pop		esi
	pop		ebp
%undef		cur_data
%undef		ref_data
%undef		iPicWidth
%undef		iPicHeight
%undef		iPicStride
%undef		psadframe
%undef		psad8x8
%undef		psum16x16
%undef		psqsum16x16
%undef		tmp_esi
%undef		tmp_edi
%undef		pushsize
%undef		localsize
	ret
	
	

WELS_EXTERN VAACalcSadSsd_sse2
;*************************************************************************************************************
;void VAACalcSadSsd_sse2(uint8_t *cur_data, uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight,  
;	int32_t iPicStride,int32_t *psadframe, int32_t *psad8x8, int32_t *psum16x16, int32_t *psqsum16x16, int32_t *psqdiff16x16)
;*************************************************************************************************************


ALIGN 16
VAACalcSadSsd_sse2:
%define		localsize		12
%define		cur_data			esp + pushsize + localsize + 4
%define		ref_data			esp + pushsize + localsize + 8
%define		iPicWidth			esp + pushsize + localsize + 12
%define		iPicHeight			esp + pushsize + localsize + 16
%define		iPicStride			esp + pushsize + localsize + 20
%define		psadframe			esp + pushsize + localsize + 24
%define		psad8x8				esp + pushsize + localsize + 28
%define		psum16x16			esp + pushsize + localsize + 32
%define		psqsum16x16			esp + pushsize + localsize + 36
%define		psqdiff16x16		esp + pushsize + localsize + 40
%define		tmp_esi				esp + 0
%define		tmp_edi				esp + 4
%define		tmp_sadframe		esp + 8
%define		pushsize		16
	push	ebp
	push	esi
	push	edi
	push	ebx
	sub		esp,	localsize
	mov		ecx,	[iPicWidth]
	mov		ecx,	[iPicHeight]
	mov		esi,	[cur_data]
	mov		edi,	[ref_data]
	mov		ebx,	[iPicStride]
	mov		edx,	[psad8x8]
	mov		eax,	ebx
	
	shr		dword [iPicWidth],	4					; iPicWidth/16
	shr		dword [iPicHeight],	4					; iPicHeight/16
	shl		eax,	4							; iPicStride*16
	mov		ecx,	[iPicWidth]
	mov		ecx,	[iPicHeight]
	pxor	xmm0,	xmm0
	movd	[tmp_sadframe],	xmm0
sqdiff_height_loop:
	mov		ecx,	dword [iPicWidth]
	mov		[tmp_esi],	esi
	mov		[tmp_edi],	edi
sqdiff_width_loop:
	pxor	xmm7,	xmm7		; hiQuad_loQuad pSad8x8
	pxor	xmm6,	xmm6		; pSum16x16
	pxor	xmm5,	xmm5		; sqsum_16x16  four dword
	pxor	xmm4,	xmm4		; sqdiff_16x16	four Dword
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	movdqa	xmm1,		xmm7
	movd	[edx],		xmm7
	psrldq	xmm7,		8
	paddd	xmm1,		xmm7
	movd	[edx+4],	xmm7
	movd	ebp,		xmm1
	add		[tmp_sadframe],	ebp
	
	pxor	xmm7,	xmm7
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	WELS_SAD_SUM_SQSUM_SQDIFF_16x1_SSE2
	movdqa	xmm1,		xmm7
	movd	[edx+8],	xmm7
	psrldq	xmm7,		8
	paddd	xmm1,		xmm7
	movd	[edx+12],	xmm7
	movd	ebp,		xmm1
	add		[tmp_sadframe],	ebp
	
	mov		ebp,	[psum16x16]
	movdqa	xmm1,	xmm6
	psrldq	xmm1,	8
	paddd	xmm6,	xmm1
	movd	[ebp],	xmm6
	add		dword [psum16x16], 4
	
	mov		ebp,	[psqsum16x16]
	pshufd	xmm6,	xmm5,	14 ;00001110
	paddd	xmm6,	xmm5
	pshufd	xmm5,	xmm6,	1  ;00000001
	paddd	xmm5,	xmm6
	movd	[ebp],	xmm5
	add		dword [psqsum16x16], 4
	
	mov		ebp,	[psqdiff16x16]
	pshufd	xmm5,	xmm4,	14	; 00001110
	paddd	xmm5,	xmm4
	pshufd	xmm4,	xmm5,	1	; 00000001
	paddd	xmm4,	xmm5
	movd	[ebp],	xmm4
	add		dword	[psqdiff16x16],	4
	
	add		edx,	16
	sub		esi,	eax
	sub		edi,	eax
	add		esi,	16
	add		edi,	16
	
	dec		ecx
	jnz		sqdiff_width_loop
	
	mov		esi,	[tmp_esi]
	mov		edi,	[tmp_edi]
	add		esi,	eax
	add		edi,	eax
	
	dec	dword [iPicHeight]
	jnz		sqdiff_height_loop
	
	mov		ebx,	[tmp_sadframe]
	mov		eax,	[psadframe]
	mov		[eax],	ebx

	add		esp,	localsize	
	pop		ebx
	pop		edi
	pop		esi
	pop		ebp
%undef		cur_data
%undef		ref_data
%undef		iPicWidth
%undef		iPicHeight
%undef		iPicStride
%undef		psadframe
%undef		psad8x8
%undef		psum16x16
%undef		psqsum16x16
%undef		psqdiff16x16
%undef		tmp_esi
%undef		tmp_edi
%undef		tmp_sadframe
%undef		pushsize
%undef		localsize
	ret
	
	
	
	

WELS_EXTERN VAACalcSadBgd_sse2
;*************************************************************************************************************
;void VAACalcSadBgd_sse2(uint8_t *cur_data, uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight, 
;				int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8, int32_t *p_sd8x8, uint8_t *p_mad8x8)
;*************************************************************************************************************


ALIGN 16
VAACalcSadBgd_sse2:
%define		localsize		12
%define		cur_data			esp + pushsize + localsize + 4
%define		ref_data			esp + pushsize + localsize + 8
%define		iPicWidth			esp + pushsize + localsize + 12
%define		iPicHeight			esp + pushsize + localsize + 16
%define		iPicStride			esp + pushsize + localsize + 20
%define		psadframe			esp + pushsize + localsize + 24
%define		psad8x8				esp + pushsize + localsize + 28
%define		p_sd8x8				esp + pushsize + localsize + 32
%define		p_mad8x8			esp + pushsize + localsize + 36
%define		tmp_esi				esp + 0
%define		tmp_edi				esp + 4
%define		tmp_ecx				esp + 8
%define		pushsize		16
	push	ebp
	push	esi
	push	edi
	push	ebx
	sub		esp,	localsize
	mov		esi,	[cur_data]
	mov		edi,	[ref_data]
	mov		ebx,	[iPicStride]
	mov		eax,	ebx
	
	shr		dword [iPicWidth],	4					; iPicWidth/16
	shr		dword [iPicHeight],	4					; iPicHeight/16
	shl		eax,	4							; iPicStride*16
	xor		ebp,	ebp
	pxor	xmm0,	xmm0
bgd_height_loop:
	mov		ecx,	dword [iPicWidth]
	mov		[tmp_esi],	esi
	mov		[tmp_edi],	edi
bgd_width_loop:
	pxor	xmm7,	xmm7		; pSad8x8
	pxor	xmm6,	xmm6		; sum_cur_8x8
	pxor	xmm5,	xmm5		; sum_ref_8x8
	pxor	xmm4,	xmm4		; pMad8x8
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	
	
	mov			edx,		[p_mad8x8]
	WELS_MAX_REG_SSE2	xmm4
	
	;movdqa		xmm1,	xmm4
	;punpcklbw	xmm1,	xmm0
	;punpcklwd	xmm1,	xmm0
	;movd		[edx],	xmm1
	;punpckhbw	xmm4,	xmm0
	;punpcklwd	xmm4,	xmm0
	;movd		[edx+4],	xmm4
	;add			edx,		8
	;mov			[p_mad8x8],	edx	
	mov			[tmp_ecx],	ecx
	movhlps		xmm1,	xmm4
	movd		ecx,	xmm4
	mov			[edx],	cl
	movd		ecx,	xmm1
	mov			[edx+1],cl
	add			edx,	2
	mov			[p_mad8x8],	edx

	
	pslldq		xmm7,	4
	pslldq		xmm6,	4
	pslldq		xmm5,	4
	
	
	pxor	xmm4,	xmm4		; pMad8x8
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_SD_MAD_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	
	mov			edx,		[p_mad8x8]
	WELS_MAX_REG_SSE2	xmm4
	
	;movdqa		xmm1,	xmm4
	;punpcklbw	xmm1,	xmm0
	;punpcklwd	xmm1,	xmm0
	;movd		[edx],	xmm1
	;punpckhbw	xmm4,	xmm0
	;punpcklwd	xmm4,	xmm0
	;movd		[edx+4],	xmm4
	;add			edx,		8
	;mov			[p_mad8x8],	edx	
	movhlps		xmm1,	xmm4
	movd		ecx,	xmm4
	mov			[edx],	cl
	movd		ecx,	xmm1
	mov			[edx+1],cl
	add			edx,	2
	mov			[p_mad8x8],	edx
	
	; data in xmm7, xmm6, xmm5:  D1 D3 D0 D2
	
	mov		edx,	[psad8x8]
	pshufd	xmm1,	xmm7,	10001101b		; D3 D2 D1 D0
	movdqa	[edx],	xmm1					
	add		edx,	16
	mov		[psad8x8],	edx					; sad8x8
	
	paddd	xmm1,	xmm7					; D1+3 D3+2 D0+1 D2+0
	pshufd	xmm2,	xmm1,	00000011b
	paddd	xmm1,	xmm2
	movd	edx,	xmm1
	add		ebp,	edx						; sad frame
	
	mov		edx,	[p_sd8x8]
	psubd	xmm6,	xmm5
	pshufd	xmm1,	xmm6,	10001101b
	movdqa	[edx],	xmm1
	add		edx,	16
	mov		[p_sd8x8],	edx
	
	
	add		edx,	16
	sub		esi,	eax
	sub		edi,	eax
	add		esi,	16
	add		edi,	16
	
	mov		ecx,	[tmp_ecx]
	dec		ecx
	jnz		bgd_width_loop
	
	mov		esi,	[tmp_esi]
	mov		edi,	[tmp_edi]
	add		esi,	eax
	add		edi,	eax
	
	dec		dword [iPicHeight]
	jnz		bgd_height_loop
	
	mov		edx,	[psadframe]
	mov		[edx],	ebp

	add		esp,	localsize	
	pop		ebx
	pop		edi
	pop		esi
	pop		ebp
%undef		cur_data
%undef		ref_data
%undef		iPicWidth
%undef		iPicHeight
%undef		iPicStride
%undef		psadframe
%undef		psad8x8
%undef		p_sd8x8
%undef		p_mad8x8
%undef		tmp_esi
%undef		tmp_edi
%undef		pushsize
%undef		localsize
	ret



WELS_EXTERN VAACalcSadSsdBgd_sse2
;*************************************************************************************************************
;void VAACalcSadSsdBgd_sse2(uint8_t *cur_data, uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight, 
;		 int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8, int32_t *psum16x16, int32_t *psqsum16x16, 
;			int32_t *psqdiff16x16, int32_t *p_sd8x8, uint8_t *p_mad8x8)
;*************************************************************************************************************


ALIGN 16
VAACalcSadSsdBgd_sse2:
%define		localsize		16
%define		cur_data			esp + pushsize + localsize + 4
%define		ref_data			esp + pushsize + localsize + 8
%define		iPicWidth			esp + pushsize + localsize + 12
%define		iPicHeight			esp + pushsize + localsize + 16
%define		iPicStride			esp + pushsize + localsize + 20
%define		psadframe			esp + pushsize + localsize + 24
%define		psad8x8				esp + pushsize + localsize + 28
%define		psum16x16			esp + pushsize + localsize + 32
%define		psqsum16x16			esp + pushsize + localsize + 36
%define		psqdiff16x16		esp + pushsize + localsize + 40
%define		p_sd8x8				esp + pushsize + localsize + 44
%define		p_mad8x8			esp + pushsize + localsize + 48
%define		tmp_esi				esp + 0
%define		tmp_edi				esp + 4
%define		tmp_sadframe		esp + 8
%define		tmp_ecx				esp + 12
%define		pushsize		16
	push	ebp
	push	esi
	push	edi
	push	ebx
	sub		esp,	localsize
	mov		esi,	[cur_data]
	mov		edi,	[ref_data]
	mov		ebx,	[iPicStride]
	mov		eax,	ebx
	
	shr		dword [iPicWidth],	4					; iPicWidth/16
	shr		dword [iPicHeight],	4					; iPicHeight/16
	shl		eax,	4							; iPicStride*16
	pxor	xmm0,	xmm0
	movd	[tmp_sadframe],	xmm0
sqdiff_bgd_height_loop:
	mov		ecx,	dword [iPicWidth]
	mov		[tmp_esi],	esi
	mov		[tmp_edi],	edi
sqdiff_bgd_width_loop:
	pxor	xmm7,	xmm7		; pSad8x8 interleaves sqsum16x16:  sqsum1 sad1 sqsum0 sad0
	pxor	xmm6,	xmm6		; sum_8x8 interleaves cur and pRef in Dword,  Sref1 Scur1 Sref0 Scur0
	pxor	xmm5,	xmm5		; pMad8x8
	pxor	xmm4,	xmm4		; sqdiff_16x16	four Dword
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	
	mov		edx,		[psad8x8]
	movdqa	xmm2,		xmm7
	pshufd	xmm1,		xmm2,		00001110b
	movd	[edx],		xmm2
	movd	[edx+4],	xmm1
	add		edx,		8
	mov		[psad8x8],	edx			; sad8x8
	
	paddd	xmm1,				xmm2
	movd	edx,				xmm1
	add		[tmp_sadframe],		edx			; iFrameSad
	
	mov		edx,		[psum16x16]
	movdqa	xmm1,		xmm6
	pshufd	xmm2,		xmm1,		00001110b
	paddd	xmm1,		xmm2
	movd	[edx],		xmm1				; sum
	
	mov		edx,		[p_sd8x8]
	pshufd	xmm1,		xmm6,		11110101b			; Sref1 Sref1 Sref0 Sref0
	psubd	xmm6,		xmm1		; 00 diff1 00 diff0
	pshufd	xmm1,		xmm6,		00001000b			;  xx xx diff1 diff0
	movq	[edx],		xmm1
	add		edx,		8
	mov		[p_sd8x8],	edx
	
	mov			edx,		[p_mad8x8]
	WELS_MAX_REG_SSE2	xmm5
	;movdqa		xmm1,	xmm5
	;punpcklbw	xmm1,	xmm0
	;punpcklwd	xmm1,	xmm0
	;movd		[edx],	xmm1
	;punpckhbw	xmm5,	xmm0
	;punpcklwd	xmm5,	xmm0
	;movd		[edx+4],	xmm5
	;add			edx,		8
	;mov			[p_mad8x8],	edx
	mov			[tmp_ecx],	ecx
	movhlps		xmm1,	xmm5
	movd		ecx,	xmm5
	mov			[edx],	cl
	movd		ecx,	xmm1
	mov			[edx+1],cl
	add			edx,	2
	mov			[p_mad8x8],	edx
	
	psrlq	xmm7,	32
	psllq	xmm7,	32			; clear sad
	pxor	xmm6,	xmm6		; sum_8x8 interleaves cur and pRef in Dword,  Sref1 Scur1 Sref0 Scur0
	pxor	xmm5,	xmm5		; pMad8x8
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	WELS_SAD_BGD_SQDIFF_16x1_SSE2	xmm7,	xmm6,	xmm5,	xmm4
	
	mov		edx,		[psad8x8]
	movdqa	xmm2,		xmm7
	pshufd	xmm1,		xmm2,		00001110b
	movd	[edx],		xmm2
	movd	[edx+4],	xmm1
	add		edx,		8
	mov		[psad8x8],	edx			; sad8x8
	
	paddd	xmm1,				xmm2
	movd	edx,				xmm1
	add		[tmp_sadframe],		edx			; iFrameSad
	
	mov		edx,			[psum16x16]
	movdqa	xmm1,			xmm6
	pshufd	xmm2,			xmm1,		00001110b
	paddd	xmm1,			xmm2
	movd	ebp,			xmm1				; sum
	add		[edx],			ebp
	add		edx,			4
	mov		[psum16x16],	edx
	
	mov		edx,			[psqsum16x16]
	psrlq	xmm7,			32
	pshufd	xmm2,			xmm7,		00001110b
	paddd	xmm2,			xmm7
	movd	[edx],			xmm2				; sqsum
	add		edx,			4
	mov		[psqsum16x16],	edx
	
	mov		edx,		[p_sd8x8]
	pshufd	xmm1,		xmm6,		11110101b			; Sref1 Sref1 Sref0 Sref0
	psubd	xmm6,		xmm1		; 00 diff1 00 diff0
	pshufd	xmm1,		xmm6,		00001000b			;  xx xx diff1 diff0
	movq	[edx],		xmm1
	add		edx,		8
	mov		[p_sd8x8],	edx
	
	mov		edx,		[p_mad8x8]
	WELS_MAX_REG_SSE2	xmm5
	;movdqa		xmm1,	xmm5
	;punpcklbw	xmm1,	xmm0
	;punpcklwd	xmm1,	xmm0
	;movd		[edx],	xmm1
	;punpckhbw	xmm5,	xmm0
	;punpcklwd	xmm5,	xmm0
	;movd		[edx+4],	xmm5
	;add			edx,		8
	;mov			[p_mad8x8],	edx	
	movhlps		xmm1,	xmm5
	movd		ecx,	xmm5
	mov			[edx],	cl
	movd		ecx,	xmm1
	mov			[edx+1],cl
	add			edx,	2
	mov			[p_mad8x8],	edx
	
	mov		edx,		[psqdiff16x16]
	pshufd	xmm1,		xmm4,		00001110b
	paddd	xmm4,		xmm1
	pshufd	xmm1,		xmm4,		00000001b
	paddd	xmm4,		xmm1
	movd	[edx],		xmm4
	add		edx,		4
	mov		[psqdiff16x16],	edx
	
	add		edx,	16
	sub		esi,	eax
	sub		edi,	eax
	add		esi,	16
	add		edi,	16
	
	mov		ecx,	[tmp_ecx]
	dec		ecx
	jnz		sqdiff_bgd_width_loop
	
	mov		esi,	[tmp_esi]
	mov		edi,	[tmp_edi]
	add		esi,	eax
	add		edi,	eax
	
	dec	dword [iPicHeight]
	jnz		sqdiff_bgd_height_loop
	
	mov		edx,	[psadframe]
	mov		ebp,	[tmp_sadframe]
	mov		[edx],	ebp

	add		esp,	localsize	
	pop		ebx
	pop		edi
	pop		esi
	pop		ebp
%undef		cur_data
%undef		ref_data
%undef		iPicWidth
%undef		iPicHeight
%undef		iPicStride
%undef		psadframe
%undef		psad8x8
%undef		psum16x16
%undef		psqsum16x16
%undef		psqdiff16x16
%undef		p_sd8x8
%undef		p_mad8x8
%undef		tmp_esi
%undef		tmp_edi
%undef		pushsize
%undef		localsize
	ret
