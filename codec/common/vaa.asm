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
	movdqa %1, [r0    ]	; line 0
	movdqa %2, [r0+r1]	; line 1
	movdqa %3, %1
	punpcklbw %1, xmm7
	punpckhbw %3, xmm7
	movdqa %4, %2
	punpcklbw %4, xmm7
	punpckhbw %2, xmm7
	paddw %1, %4
	paddw %2, %3
	movdqa %3, [r0+r2]	; line 2
	movdqa %4, [r0+r3]	; line 3
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
	movdqa %1, [r0    ]	; line 0
	movdqa %2, [r0+r1]	; line 1
	movdqa %3, %1
	punpcklbw %1, xmm7
	punpckhbw %3, xmm7
	movdqa %4, %2
	punpcklbw %4, xmm7
	punpckhbw %2, xmm7
	paddw %1, %4
	paddw %2, %3
	movdqa %3, [r0+r2]	; line 2
	movdqa %4, [r0+r3]	; line 3
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


%macro WELS_SAD_16x2_SSE2  3 ;esi :%1 edi:%2 ebx:%3
	movdqa	xmm1,	[%1]
	movdqa	xmm2,	[%2]
	movdqa	xmm3,	[%1+%3]
	movdqa	xmm4,	[%2+%3]
	psadbw	xmm1,	xmm2
	psadbw	xmm3,	xmm4
	paddd	xmm6,	xmm1
	paddd	xmm6,	xmm3
	lea		%1,	[%1+%3*2]
	lea		%2,	[%2+%3*2]
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

    %assign push_num 0
    LOAD_2_PARA 
    SIGN_EXTENTION r1,r1d

%ifdef X86_32
    push r3
    push r4
    push r5
    push r6
    %assign push_num push_num+4
%endif

    mov  r5,r7
    and  r5,0fh
    sub  r7,r5
    sub  r7,32
    
    
    mov r2,r1    
    sal r2,$1   ;r2 = 2*iLineSize
    mov r3,r2
    add r3,r1   ;r3 = 3*iLineSize
    
    mov r4,r2
    sal r4,$1   ;r4 = 4*iLineSize
    
	pxor xmm7, xmm7

	; loops
	VAA_AVG_BLOCK_SSE2 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
	movq [r7], xmm0

	lea r0, [r0+r4]
	VAA_AVG_BLOCK_SSE2 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
	movq [r7+8], xmm0

	lea r0, [r0+r4]
	VAA_AVG_BLOCK_SSE2 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
	movq [r7+16], xmm0

	lea r0, [r0+r4]
	VAA_AVG_BLOCK_SSE2 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
	movq [r7+24], xmm0

	movdqa xmm0, [r7]		; block 0~7
	movdqa xmm1, [r7+16]	; block 8~15
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

	
	
	movd r2d, xmm0
	and r2, 0ffffh		; effective low work truncated
	mov r3, r2
	imul r2, r3
	sar r2, $4
	movd retrd, xmm1
	sub retrd, r2d
	
	add r7,32
	add r7,r5

%ifdef X86_32
	pop r6
	pop r5
	pop r4
	pop r3
%endif
	
	ret

WELS_EXTERN AnalysisVaaInfoIntra_ssse3
;***********************************************************************
;	int32_t AnalysisVaaInfoIntra_ssse3(	uint8_t *pDataY, const int32_t iLineSize );
;***********************************************************************
ALIGN 16
AnalysisVaaInfoIntra_ssse3:

    %assign push_num 0
    LOAD_2_PARA 
    SIGN_EXTENTION r1,r1d

%ifdef X86_32
    push r3
    push r4
    push r5
    push r6
    %assign push_num push_num+4
%endif
   
    mov  r5,r7
    and  r5,0fh
    sub  r7,r5
    sub  r7,32
    

    mov r2,r1    
    sal r2,$1   ;r2 = 2*iLineSize
    mov r3,r2
    add r3,r1   ;r3 = 3*iLineSize
    
    mov r4,r2
    sal r4,$1   ;r4 = 4*iLineSize
     
	pxor xmm7, xmm7

	; loops
	VAA_AVG_BLOCK_SSSE3 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
    movq [r7],xmm0
    
	lea r0,[r0+r4]
	VAA_AVG_BLOCK_SSSE3 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6
    movq [r7+8],xmm1
    
    
	lea r0,[r0+r4]
	VAA_AVG_BLOCK_SSSE3 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5
    movq [r7+16],xmm0
    
	lea r0,[r0+r4]
	VAA_AVG_BLOCK_SSSE3 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6
    movq [r7+24],xmm1
    
    
	movdqa xmm0,[r7]
	movdqa xmm1,[r7+16]
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

    
    movd r2d, xmm0
    and r2, 0ffffh          ; effective low work truncated
    mov r3, r2
    imul r2, r3
    sar r2, $4
    movd retrd, xmm1
	sub retrd, r2d

	add r7,32
	add r7,r5
%ifdef X86_32
	pop r6
	pop r5
	pop r4
	pop r3
%endif
	
	ret

WELS_EXTERN MdInterAnalysisVaaInfo_sse41
;***********************************************************************
;	uint8_t MdInterAnalysisVaaInfo_sse41( int32_t *pSad8x8 )
;***********************************************************************
ALIGN 16
MdInterAnalysisVaaInfo_sse41:
	%assign push_num 0
	LOAD_1_PARA
	movdqa xmm0,[r0]
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
	movd r0d, xmm3
	cmp r0d, 20	; INTER_VARIANCE_SAD_THRESHOLD
	
	jb near .threshold_exit
	pshufd xmm0, xmm0, 01Bh
	pcmpgtd xmm0, xmm1	; iSadBlock > iAverageSad
	movmskps retrd, xmm0
	ret
.threshold_exit:
	mov retrd, 15
	ret

WELS_EXTERN MdInterAnalysisVaaInfo_sse2
;***********************************************************************
;	uint8_t MdInterAnalysisVaaInfo_sse2( int32_t *pSad8x8 )
;***********************************************************************
ALIGN 16
MdInterAnalysisVaaInfo_sse2:
	%assign push_num 0
	LOAD_1_PARA
	movdqa xmm0, [r0]
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
	
	movd r0d, xmm5
	cmp r0d, 20	; INTER_VARIANCE_SAD_THRESHOLD
	jb near .threshold_exit
	pshufd xmm0, xmm0, 01Bh
	pcmpgtd xmm0, xmm1	; iSadBlock > iAverageSad
	movmskps retrd, xmm0
	ret
.threshold_exit:
	mov retrd, 15
	ret


%ifdef X86_32

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
	WELS_SAD_16x2_SSE2 esi,edi,ebx
	WELS_SAD_16x2_SSE2 esi,edi,ebx
	WELS_SAD_16x2_SSE2 esi,edi,ebx
	WELS_SAD_16x2_SSE2 esi,edi,ebx
	paddd	xmm7,		xmm6
	movd	[edx],		xmm6
	psrldq	xmm6,		8
	movd	[edx+4],	xmm6

	pxor	xmm6,	xmm6
	WELS_SAD_16x2_SSE2 esi,edi,ebx
	WELS_SAD_16x2_SSE2 esi,edi,ebx
	WELS_SAD_16x2_SSE2 esi,edi,ebx
	WELS_SAD_16x2_SSE2 esi,edi,ebx
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
	
%else  ;64-bit 

WELS_EXTERN SampleVariance16x16_sse2
;***********************************************************************
;   void SampleVariance16x16_sse2(	uint8_t * y_ref, int32_t y_ref_stride, uint8_t * y_src, int32_t y_src_stride,SMotionTextureUnit* pMotionTexture );
;***********************************************************************
ALIGN 16
SampleVariance16x16_sse2:
	%define SUM			r10;[esp]
	%define SUM_CUR		r11;[esp+4]
	%define SQR			r13;[esp+8]
	%define SQR_CUR		r15;[esp+12]

  movdqa xmm14,xmm6
  movdqa xmm15,xmm7 
	push r12
	push r13
	push r14
	push r15
	%assign push_num 4
	LOAD_5_PARA
	SIGN_EXTENTION r1,r1d
	SIGN_EXTENTION r3,r3d
	
	mov r12,010h
	pxor xmm7, xmm7
	movq SUM, xmm7
	movq SUM_CUR,xmm7
	movq SQR,xmm7
	movq SQR_CUR,xmm7

.hloops:
  mov r14,0
	movdqa xmm0, [r0]		; y_ref
	movdqa xmm1, [r2]		; y_src
	movdqa xmm2, xmm0		; store first for future process
	movdqa xmm3, xmm1
	; sum += diff;
	movdqa xmm4, xmm0
	psadbw xmm4, xmm1		; 2 parts, [0,..,15], [64,..,79]
	; to be continued for sum
	pshufd xmm5, xmm4, 0C6h	; 11000110 B
	paddw xmm4, xmm5
	movd r14d, xmm4
	add SUM, r14

	; sqr += diff * diff;
	pmaxub xmm0, xmm1
	pminub xmm1, xmm2
	psubb xmm0, xmm1				; diff
	SUM_SQR_SSE2 xmm1, xmm0, xmm7	; dst, pSrc, zero
	movd r14d, xmm1
	add SQR, r14

	; sum_cur += y_src[x];
	movdqa xmm0, xmm3		; cur_orig
	movdqa xmm1, xmm0
	punpcklbw xmm0, xmm7
	punpckhbw xmm1, xmm7
	paddw xmm0, xmm1		; 8x2
	SUM_WORD_8x2_SSE2 xmm0, xmm1
	movd r14d, xmm0
	and r14, 0ffffh
	add SUM_CUR, r14

	; sqr_cur += y_src[x] * y_src[x];
	SUM_SQR_SSE2 xmm0, xmm3, xmm7	; dst, pSrc, zero
	movd r14d, xmm0
	add SQR_CUR, r14

	lea r0, [r0+r1]
	lea r2, [r2+r3]
	dec r12
	jnz near .hloops
	
	mov r0, SUM
	sar r0, 8
	imul r0, r0
	mov r1, SQR
	sar r1, 8
	sub r1, r0
	mov [r4], r1w				; to store uiMotionIndex
	mov r0, SUM_CUR
	sar r0, 8
	imul r0, r0
	mov r1, SQR_CUR
	sar r1, 8
	sub r1, r0
	mov [r4+2], r1w				; to store uiTextureIndex

	LOAD_5_PARA_POP
	pop r15
	pop r14
	pop r13
	pop r12
  
  movdqa xmm6,xmm14
  movdqa xmm7,xmm15
  
	%assign push_num 0
 
	ret
	
	
	WELS_EXTERN VAACalcSad_sse2
;*************************************************************************************************************
;void VAACalcSad_sse2( uint8_t *cur_data, uint8_t *ref_data, int32_t iPicWidth, int32_t iPicHeight
;								int32_t iPicStride, int32_t *psadframe, int32_t *psad8x8)
;*************************************************************************************************************


ALIGN 16
VAACalcSad_sse2:
%define		cur_data			r0
%define		ref_data			r1
%define		iPicWidth			r2
%define		iPicHeight		r3
%define		iPicStride		r4
%define		psadframe			r5
%define		psad8x8				r6

  push r12
  push r13
  %assign push_num 2
  LOAD_7_PARA
  SIGN_EXTENTION r2,r2d
  SIGN_EXTENTION r3,r3d
  SIGN_EXTENTION r4,r4d
  
    mov   r12,r4
	shr		r2,	4					; iPicWidth/16
	shr		r3,	4					; iPicHeight/16
	
	shl		r12,	4								; iPicStride*16
	pxor	xmm0,	xmm0
	pxor	xmm7,	xmm7		; iFrameSad
height_loop:
	mov		r13,	r2
	push	r0
	push	r1
width_loop:
	pxor	xmm6,	xmm6		
	WELS_SAD_16x2_SSE2 r0,r1,r4
	WELS_SAD_16x2_SSE2 r0,r1,r4
	WELS_SAD_16x2_SSE2 r0,r1,r4
	WELS_SAD_16x2_SSE2 r0,r1,r4
	paddd	xmm7,		xmm6
	movd	[r6],		xmm6
	psrldq	xmm6,		8
	movd	[r6+4],	xmm6

	pxor	xmm6,	xmm6
	WELS_SAD_16x2_SSE2 r0,r1,r4
	WELS_SAD_16x2_SSE2 r0,r1,r4
	WELS_SAD_16x2_SSE2 r0,r1,r4
	WELS_SAD_16x2_SSE2 r0,r1,r4
	paddd	xmm7,		xmm6
	movd	[r6+8],	xmm6
	psrldq	xmm6,		8
	movd	[r6+12],	xmm6

	add		r6,	16
	sub		r0,	r12
	sub		r1,	r12
	add		r0,	16
	add		r1,	16

	dec		r13
	jnz		width_loop

	pop		r1
	pop		r0
	add		r0,	r12
	add		r1,	r12

	dec	r3
	jnz		height_loop

	;mov		r13,	[psadframe]
	movdqa	xmm5,	xmm7
	psrldq	xmm7,	8
	paddd	xmm7,	xmm5
    movd	[psadframe],	xmm7

%undef		cur_data
%undef		ref_data
%undef		iPicWidth
%undef		iPicHeight
%undef		iPicStride
%undef		psadframe
%undef		psad8x8
%undef		pushsize
  LOAD_7_PARA_POP
  pop r13
  pop r12
  %assign push_num 0
  ret
		
%endif


