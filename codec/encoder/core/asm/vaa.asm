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
;*		06/07/2010	Added AnalysisVaaInfoIntra_sse2(ssse3)
;*		06/10/2010	Tune rc_sad_frame_sse2 and got about 40% improvement
;*		08/11/2010	Added abs_difference_mbrow_sse2 & sum_sqrsum_mbrow_sse2 
;*
;*************************************************************************/
%include "asm_inc.asm"
BITS 32

;***********************************************************************
; Macros and other preprocessor constants
;***********************************************************************

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

; , 6/7/2010

WELS_EXTERN AnalysisVaaInfoIntra_sse2
;***********************************************************************
;	int32_t AnalysisVaaInfoIntra_sse2(	uint8_t *pDataY, const int32_t iLineSize );
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
	mov ecx, [esp+ebp+PUSH_SIZE+8]	; iLineSize

	mov ebx, ecx
	sal ebx, $1			; iLineSize x 2 [ebx]
	mov edx, ebx
	add edx, ecx		; iLineSize x 3 [edx]
	mov eax, ebx
	sal eax, $1			; iLineSize x 4 [eax]
	
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
;	int32_t AnalysisVaaInfoIntra_ssse3(	uint8_t *pDataY, const int32_t iLineSize );
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
	mov ecx, [esp+ebp+PUSH_SIZE+8]	; iLineSize

	mov ebx, ecx
	sal ebx, $1			; iLineSize x 2 [ebx]
	mov edx, ebx
	add edx, ecx		; iLineSize x 3 [edx]
	mov eax, ebx
	sal eax, $1			; iLineSize x 4 [eax]
	
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
	
WELS_EXTERN MdInterAnalysisVaaInfo_sse41
;***********************************************************************
;	uint8_t MdInterAnalysisVaaInfo_sse41( int32_t *pSad8x8 )
;***********************************************************************
ALIGN 16
MdInterAnalysisVaaInfo_sse41:
	mov eax, [esp+4]
	movdqa xmm0, [eax]	; load 4 sad_8x8	
	pshufd xmm1, xmm0, 01Bh
	paddd xmm1, xmm0
	pshufd xmm2, xmm1, 0B1h
	paddd xmm1, xmm2	
	psrad xmm1, 02h		; iAverageSad
	movdqa xmm2, xmm1
	psrad xmm2, 06h
	movdqa xmm3, xmm0	; iSadBlock
	psrad xmm3, 06h
	psubd xmm3, xmm2
	pmulld xmm3, xmm3	; [comment]: pmulld from SSE4.1 instruction sets	
	pshufd xmm4, xmm3, 01Bh
	paddd xmm4, xmm3
	pshufd xmm3, xmm4, 0B1h
	paddd xmm3, xmm4
	movd eax, xmm3
	cmp eax, 20	; INTER_VARIANCE_SAD_THRESHOLD
	jb near .threshold_exit
	pshufd xmm0, xmm0, 0B1h
	pcmpgtd xmm0, xmm1	; iSadBlock > iAverageSad
	movmskps eax, xmm0
	ret
.threshold_exit:	
	mov eax, 15
	ret

WELS_EXTERN MdInterAnalysisVaaInfo_sse2
;***********************************************************************
;	uint8_t MdInterAnalysisVaaInfo_sse2( int32_t *pSad8x8 )
;***********************************************************************
ALIGN 16
MdInterAnalysisVaaInfo_sse2:
	mov eax, [esp+4]
	movdqa xmm0, [eax]	; load 4 sad_8x8	
	pshufd xmm1, xmm0, 01Bh
	paddd xmm1, xmm0
	pshufd xmm2, xmm1, 0B1h
	paddd xmm1, xmm2	
	psrad xmm1, 02h		; iAverageSad
	movdqa xmm2, xmm1
	psrad xmm2, 06h
	movdqa xmm3, xmm0	; iSadBlock
	psrad xmm3, 06h
	psubd xmm3, xmm2
	
	; to replace pmulld functionality as below
	movdqa xmm2, xmm3	
	pmuludq xmm2, xmm3
	pshufd xmm4, xmm3, 0B1h
	pmuludq xmm4, xmm4
	movdqa xmm5, xmm2
	punpckldq xmm5, xmm4
	punpckhdq xmm2, xmm4
	punpcklqdq xmm5, xmm2	
	
	pshufd xmm4, xmm5, 01Bh
	paddd xmm4, xmm5
	pshufd xmm5, xmm4, 0B1h
	paddd xmm5, xmm4
	movd eax, xmm5
	cmp eax, 20	; INTER_VARIANCE_SAD_THRESHOLD
	jb near .threshold_exit
	pshufd xmm0, xmm0, 0B1h
	pcmpgtd xmm0, xmm1	; iSadBlock > iAverageSad
	movmskps eax, xmm0
	ret
.threshold_exit:	
	mov eax, 15
	ret
