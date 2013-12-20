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
;*  mb_copy.asm
;*
;*  Abstract
;*      mb_copy
;*
;*
;*********************************************************************************************/
%include "asm_inc.asm"
BITS 32

;***********************************************************************
; Macros and other preprocessor constants
;***********************************************************************

;***********************************************************************
; Code
;***********************************************************************

SECTION .text

WELS_EXTERN WelsCopy16x16_sse2
WELS_EXTERN WelsCopy16x16NotAligned_sse2
WELS_EXTERN WelsCopy8x8_mmx
WELS_EXTERN WelsCopy16x8NotAligned_sse2	;
WELS_EXTERN WelsCopy8x16_mmx		;
WELS_EXTERN UpdateMbMv_sse2		;

;***********************************************************************
; void WelsCopy16x16_sse2(	uint8_t* Dst,
;							int32_t  iStrideD,
;							uint8_t* Src,
;							int32_t  iStrideS )
;***********************************************************************
ALIGN 16
WelsCopy16x16_sse2:
	push esi
	push edi
	push ebx

	mov edi, [esp+16]	; Dst
	mov eax, [esp+20]	; iStrideD
	mov esi, [esp+24]	; Src
	mov ecx, [esp+28]	; iStrideS

	lea ebx, [eax+2*eax]	; x3
	lea edx, [ecx+2*ecx]	; x3

	movdqa xmm0, [esi]
	movdqa xmm1, [esi+ecx]
	movdqa xmm2, [esi+2*ecx]
	movdqa xmm3, [esi+edx]
	lea esi, [esi+4*ecx]
	movdqa xmm4, [esi]
	movdqa xmm5, [esi+ecx]
	movdqa xmm6, [esi+2*ecx]
	movdqa xmm7, [esi+edx]
	lea esi, [esi+4*ecx]

	movdqa [edi], xmm0
	movdqa [edi+eax], xmm1
	movdqa [edi+2*eax], xmm2
	movdqa [edi+ebx], xmm3
	lea edi, [edi+4*eax]
	movdqa [edi], xmm4
	movdqa [edi+eax], xmm5
	movdqa [edi+2*eax], xmm6
	movdqa [edi+ebx], xmm7
	lea edi, [edi+4*eax]

	movdqa xmm0, [esi]
	movdqa xmm1, [esi+ecx]
	movdqa xmm2, [esi+2*ecx]
	movdqa xmm3, [esi+edx]
	lea esi, [esi+4*ecx]
	movdqa xmm4, [esi]
	movdqa xmm5, [esi+ecx]
	movdqa xmm6, [esi+2*ecx]
	movdqa xmm7, [esi+edx]

	movdqa [edi], xmm0
	movdqa [edi+eax], xmm1
	movdqa [edi+2*eax], xmm2
	movdqa [edi+ebx], xmm3
	lea edi, [edi+4*eax]
	movdqa [edi], xmm4
	movdqa [edi+eax], xmm5
	movdqa [edi+2*eax], xmm6
	movdqa [edi+ebx], xmm7

	pop ebx
	pop edi
	pop esi
	ret

;***********************************************************************
; void WelsCopy16x16NotAligned_sse2(	uint8_t* Dst,
;							int32_t  iStrideD,
;							uint8_t* Src,
;							int32_t  iStrideS )
;***********************************************************************
ALIGN 16
; dst can be align with 16 bytes, but not sure about pSrc, 12/29/2011
WelsCopy16x16NotAligned_sse2:
	push esi
	push edi
	push ebx

	mov edi, [esp+16]	; Dst
	mov eax, [esp+20]	; iStrideD
	mov esi, [esp+24]	; Src
	mov ecx, [esp+28]	; iStrideS

	lea ebx, [eax+2*eax]	; x3
	lea edx, [ecx+2*ecx]	; x3

	movdqu xmm0, [esi]
	movdqu xmm1, [esi+ecx]
	movdqu xmm2, [esi+2*ecx]
	movdqu xmm3, [esi+edx]
	lea esi, [esi+4*ecx]
	movdqu xmm4, [esi]
	movdqu xmm5, [esi+ecx]
	movdqu xmm6, [esi+2*ecx]
	movdqu xmm7, [esi+edx]
	lea esi, [esi+4*ecx]

	movdqa [edi], xmm0
	movdqa [edi+eax], xmm1
	movdqa [edi+2*eax], xmm2
	movdqa [edi+ebx], xmm3
	lea edi, [edi+4*eax]
	movdqa [edi], xmm4
	movdqa [edi+eax], xmm5
	movdqa [edi+2*eax], xmm6
	movdqa [edi+ebx], xmm7
	lea edi, [edi+4*eax]

	movdqu xmm0, [esi]
	movdqu xmm1, [esi+ecx]
	movdqu xmm2, [esi+2*ecx]
	movdqu xmm3, [esi+edx]
	lea esi, [esi+4*ecx]
	movdqu xmm4, [esi]
	movdqu xmm5, [esi+ecx]
	movdqu xmm6, [esi+2*ecx]
	movdqu xmm7, [esi+edx]

	movdqa [edi], xmm0
	movdqa [edi+eax], xmm1
	movdqa [edi+2*eax], xmm2
	movdqa [edi+ebx], xmm3
	lea edi, [edi+4*eax]
	movdqa [edi], xmm4
	movdqa [edi+eax], xmm5
	movdqa [edi+2*eax], xmm6
	movdqa [edi+ebx], xmm7

	pop ebx
	pop edi
	pop esi
	ret

; , 12/29/2011
;***********************************************************************
; void WelsCopy16x8NotAligned_sse2(uint8_t* Dst,
;							int32_t  iStrideD,
;							uint8_t* Src,
;							int32_t  iStrideS )
;***********************************************************************
ALIGN 16
WelsCopy16x8NotAligned_sse2:
	push esi
	push edi
	push ebx

	mov edi, [esp+16]	; Dst
	mov eax, [esp+20]	; iStrideD
	mov esi, [esp+24]	; Src
	mov ecx, [esp+28]	; iStrideS

	lea ebx, [eax+2*eax]	; x3
	lea edx, [ecx+2*ecx]	; x3

	movdqu xmm0, [esi]
	movdqu xmm1, [esi+ecx]
	movdqu xmm2, [esi+2*ecx]
	movdqu xmm3, [esi+edx]
	lea esi, [esi+4*ecx]
	movdqu xmm4, [esi]
	movdqu xmm5, [esi+ecx]
	movdqu xmm6, [esi+2*ecx]
	movdqu xmm7, [esi+edx]

	movdqa [edi], xmm0
	movdqa [edi+eax], xmm1
	movdqa [edi+2*eax], xmm2
	movdqa [edi+ebx], xmm3
	lea edi, [edi+4*eax]
	movdqa [edi], xmm4
	movdqa [edi+eax], xmm5
	movdqa [edi+2*eax], xmm6
	movdqa [edi+ebx], xmm7

	pop ebx
	pop edi
	pop esi
	ret


;***********************************************************************
; void WelsCopy8x16_mmx(uint8_t* Dst,
;                       int32_t  iStrideD,
;                       uint8_t* Src,
;                       int32_t  iStrideS )
;***********************************************************************
ALIGN 16
WelsCopy8x16_mmx:
	push ebx

	mov eax, [esp + 8 ]           ;Dst
	mov ecx, [esp + 12]           ;iStrideD
	mov ebx, [esp + 16]           ;Src
	mov edx, [esp + 20]           ;iStrideS

	movq mm0, [ebx]
	movq mm1, [ebx+edx]
	lea ebx, [ebx+2*edx]
	movq mm2, [ebx]
	movq mm3, [ebx+edx]
	lea ebx, [ebx+2*edx]
	movq mm4, [ebx]
	movq mm5, [ebx+edx]
	lea ebx, [ebx+2*edx]
	movq mm6, [ebx]
	movq mm7, [ebx+edx]
	lea ebx, [ebx+2*edx]

	movq [eax], mm0
	movq [eax+ecx], mm1
	lea eax, [eax+2*ecx]
	movq [eax], mm2
	movq [eax+ecx], mm3
	lea eax, [eax+2*ecx]
	movq [eax], mm4
	movq [eax+ecx], mm5
	lea eax, [eax+2*ecx]
	movq [eax], mm6
	movq [eax+ecx], mm7
	lea eax, [eax+2*ecx]

	movq mm0, [ebx]
	movq mm1, [ebx+edx]
	lea ebx, [ebx+2*edx]
	movq mm2, [ebx]
	movq mm3, [ebx+edx]
	lea ebx, [ebx+2*edx]
	movq mm4, [ebx]
	movq mm5, [ebx+edx]
	lea ebx, [ebx+2*edx]
	movq mm6, [ebx]
	movq mm7, [ebx+edx]

	movq [eax], mm0
	movq [eax+ecx], mm1
	lea eax, [eax+2*ecx]
	movq [eax], mm2
	movq [eax+ecx], mm3
	lea eax, [eax+2*ecx]
	movq [eax], mm4
	movq [eax+ecx], mm5
	lea eax, [eax+2*ecx]
	movq [eax], mm6
	movq [eax+ecx], mm7

	WELSEMMS
	pop ebx
	ret

;***********************************************************************
; void WelsCopy8x8_mmx(  uint8_t* Dst,
;                        int32_t  iStrideD,
;                        uint8_t* Src,
;                        int32_t  iStrideS )
;***********************************************************************
ALIGN 16
WelsCopy8x8_mmx:
	push ebx
	push esi
	mov eax, [esp + 12]           ;Dst
	mov ecx, [esp + 16]           ;iStrideD
	mov esi, [esp + 20]           ;Src
	mov ebx, [esp + 24]           ;iStrideS
	lea edx, [ebx+2*ebx]

	; to prefetch next loop
	prefetchnta [esi+2*ebx]
	prefetchnta [esi+edx]
	movq mm0, [esi]
	movq mm1, [esi+ebx]
	lea esi, [esi+2*ebx]
	; to prefetch next loop
	prefetchnta [esi+2*ebx]
	prefetchnta [esi+edx]
	movq mm2, [esi]
	movq mm3, [esi+ebx]
	lea esi, [esi+2*ebx]
	; to prefetch next loop
	prefetchnta [esi+2*ebx]
	prefetchnta [esi+edx]
	movq mm4, [esi]
	movq mm5, [esi+ebx]
	lea esi, [esi+2*ebx]
	movq mm6, [esi]
	movq mm7, [esi+ebx]

	movq [eax], mm0
	movq [eax+ecx], mm1
	lea eax, [eax+2*ecx]
	movq [eax], mm2
	movq [eax+ecx], mm3
	lea eax, [eax+2*ecx]
	movq [eax], mm4
	movq [eax+ecx], mm5
	lea eax, [eax+2*ecx]
	movq [eax], mm6
	movq [eax+ecx], mm7

	WELSEMMS
	pop esi
	pop ebx
	ret

; (dunhuang@cisco), 12/21/2011
;***********************************************************************
; void UpdateMbMv_sse2( SMVUnitXY *pMvBuffer, const SMVUnitXY sMv )
;***********************************************************************
ALIGN 16
UpdateMbMv_sse2:
	mov eax, [esp+4]	; mv_buffer
	movd xmm0, [esp+8]	; _mv
	pshufd xmm1, xmm0, $0
	movdqa [eax     ], xmm1
	movdqa [eax+0x10], xmm1
	movdqa [eax+0x20], xmm1
	movdqa [eax+0x30], xmm1
	ret



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
;read unaligned memory
%macro SSE2_READ_UNA 2
	movq	%1, [%2]
	movhps	%1,	[%2+8]
%endmacro

;write unaligned memory
%macro SSE2_WRITE_UNA 2
	movq	[%1],	%2
	movhps	[%1+8], %2
%endmacro

ALIGN 16

;***********************************************************************
; Code
;***********************************************************************

SECTION .text

WELS_EXTERN PixelAvgWidthEq8_mmx
WELS_EXTERN PixelAvgWidthEq16_sse2

WELS_EXTERN McCopyWidthEq4_mmx
WELS_EXTERN McCopyWidthEq8_mmx
WELS_EXTERN McCopyWidthEq16_sse2


ALIGN 16
;***********************************************************************
; void PixelAvgWidthEq8_mmx( uint8_t *dst,  int32_t iDstStride,
;                           uint8_t *pSrc1, int32_t iSrc1Stride,
;                           uint8_t *pSrc2, int32_t iSrc2Stride,
;                           int32_t iHeight );
;***********************************************************************
PixelAvgWidthEq8_mmx:
    push        ebp
    push        ebx
    push        esi
    push        edi

    mov         edi, [esp+20]
    mov         esi, [esp+28]
    mov         edx, [esp+36]
    mov         ebp, [esp+24]
    mov         eax, [esp+32]
    mov         ebx, [esp+40]
    mov         ecx, [esp+44]
	sar			ecx, 2
.height_loop:
	movq        mm0, [esi]
    pavgb       mm0, [edx]
    movq        [edi], mm0
	movq		mm1, [esi+eax]
	pavgb		mm1, [edx+ebx]
	movq		[edi+ebp], mm1
	lea         edi, [edi+2*ebp]
	lea         esi, [esi+2*eax]
	lea         edx, [edx+2*ebx]

	movq        mm2, [esi]
	pavgb       mm2, [edx]
    movq        [edi], mm2
	movq		mm3, [esi+eax]
	pavgb		mm3, [edx+ebx]
	movq		[edi+ebp], mm3
	lea         edi, [edi+2*ebp]
	lea         esi, [esi+2*eax]
	lea         edx, [edx+2*ebx]

	dec         ecx
    jne         .height_loop

	WELSEMMS
    pop         edi
    pop         esi
    pop         ebx
    pop         ebp
    ret


ALIGN 16
;***********************************************************************
; void PixelAvgWidthEq16_sse2( uint8_t *dst,  int32_t iDstStride,
;                          uint8_t *pSrc1, int32_t iSrc1Stride,
;                          uint8_t *pSrc2, int32_t iSrc2Stride,
;                          int32_t iHeight );
;***********************************************************************
PixelAvgWidthEq16_sse2:
	push        ebp
    push        ebx
    push        esi
    push        edi

    mov         edi, [esp+20]
    mov         esi, [esp+28]
    mov         edx, [esp+36]
    mov         ebp, [esp+24]
    mov         eax, [esp+32]
    mov         ebx, [esp+40]
    mov         ecx, [esp+44]
	sar			ecx, 2
.height_loop:
	movdqu      xmm0, [esi]
	movdqu      xmm1, [edx]
	movdqu      xmm2, [esi+eax]
	movdqu      xmm3, [edx+ebx]
	pavgb       xmm0, xmm1
	pavgb       xmm2, xmm3
	movdqu      [edi], xmm0
	movdqu      [edi+ebp], xmm2
	lea			edi, [edi+2*ebp]
	lea			esi, [esi+2*eax]
	lea			edx, [edx+2*ebx]

	movdqu      xmm4, [esi]
	movdqu      xmm5, [edx]
	movdqu      xmm6, [esi+eax]
	movdqu      xmm7, [edx+ebx]
	pavgb       xmm4, xmm5
	pavgb       xmm6, xmm7
	movdqu      [edi], xmm4
	movdqu      [edi+ebp], xmm6
	lea         edi, [edi+2*ebp]
	lea         esi, [esi+2*eax]
    lea         edx, [edx+2*ebx]

	dec         ecx
	jne         .height_loop

    pop         edi
    pop         esi
    pop         ebx
    pop         ebp
    ret


ALIGN 64
avg_w16_align_0_ssse3:
    movdqa  xmm1, [ebx]
    movdqu  xmm2, [ecx]
    pavgb   xmm1, xmm2
    movdqa  [edi], xmm1
    add    ebx, eax
    add    ecx, ebp
    add    edi, esi
    dec    dword [esp+4]
    jg     avg_w16_align_0_ssse3
    ret

    ALIGN 64
avg_w16_align_1_ssse3:
    movdqa  xmm1, [ebx+16]
    movdqu  xmm2, [ecx]
    palignr xmm1, [ebx], 1
    pavgb   xmm1, xmm2
    movdqa  [edi], xmm1
    add    ebx, eax
    add    ecx, ebp
    add    edi, esi
    dec    dword [esp+4]
    jg     avg_w16_align_1_ssse3
    ret


ALIGN 16
;***********************************************************************
; void PixelAvgWidthEq16_ssse3(uint8_t *pDst,  int32_t iDstStride,
;                          uint8_t *pSrc1, int32_t iSrc1Stride,
;                          uint8_t *pSrc2, int32_t iSrc2Stride,
;                          int32_t iHeight );
;***********************************************************************
WELS_EXTERN PixelAvgWidthEq16_ssse3
PixelAvgWidthEq16_ssse3:
    push        ebp
    push        ebx
    push        esi
    push        edi

    mov         edi, [esp+20]       ; dst
    mov         ebx, [esp+28]       ; src1
    mov         ecx, [esp+36]       ; src2
    mov         esi, [esp+24]       ; i_dst_stride

     %define avg_w16_offset (avg_w16_align_1_ssse3-avg_w16_align_0_ssse3)
    mov edx, ebx
    and edx, 0x01
    lea eax, [avg_w16_align_0_ssse3]
    lea ebp, [avg_w16_offset]
    imul ebp, edx
    lea edx, [ebp+eax]

    mov eax, [esp+32]
    mov ebp, [esp+44]
    push ebp
    mov ebp, [esp+44]
    and ebx, 0xfffffff0
    call edx
	pop		   ebp
    pop         edi
    pop         esi
    pop         ebx
    pop         ebp
    ret


ALIGN 16
;*******************************************************************************
;  void McCopyWidthEq4_mmx( uint8_t *pSrc, int32_t iSrcStride,
;                          uint8_t *pDst, int32_t iDstStride, int32_t iHeight )
;*******************************************************************************
McCopyWidthEq4_mmx:
    push    esi
    push    edi
    push    ebx


    mov esi,  [esp+16]
    mov eax, [esp+20]
    mov edi,  [esp+24]
    mov ecx,  [esp+28]
    mov edx,  [esp+32]
ALIGN 4
.height_loop:
	mov ebx, [esi]
	mov [edi], ebx

	add esi, eax
	add edi, ecx
	dec edx
	jnz .height_loop
	WELSEMMS
	pop	   ebx
    pop     edi
    pop     esi
    ret

ALIGN 16
;*******************************************************************************
;   void McCopyWidthEq8_mmx( uint8_t *pSrc, int32_t iSrcStride,
;                           uint8_t *pDst, int32_t iDstStride, int32_t iHeight )
;*******************************************************************************
McCopyWidthEq8_mmx:
    push  esi
    push  edi
	mov  esi, [esp+12]
	mov eax, [esp+16]
	mov edi, [esp+20]
	mov ecx, [esp+24]
	mov edx, [esp+28]

ALIGN 4
.height_loop:
	movq mm0, [esi]
	movq [edi], mm0
	add esi, eax
	add edi, ecx
	dec edx
	jnz .height_loop

	WELSEMMS
    pop     edi
    pop     esi
    ret

ALIGN 16
;***********************************************************************
;   void McCopyWidthEq16_sse2( uint8_t *pSrc, int32_t iSrcStride, uint8_t *pDst, int32_t iDstStride, int32_t iHeight )
;***********************************************************************
McCopyWidthEq16_sse2:
    push    esi
    push    edi

    mov     esi, [esp+12]
    mov     eax, [esp+16]
    mov     edi, [esp+20]
    mov     edx, [esp+24]
    mov     ecx, [esp+28]

ALIGN 4
.height_loop:
    SSE2_READ_UNA	xmm0, esi
    SSE2_READ_UNA	xmm1, esi+eax
    SSE2_WRITE_UNA	edi, xmm0
    SSE2_WRITE_UNA	edi+edx, xmm1

	sub		ecx,	2
    lea     esi, [esi+eax*2]
    lea     edi, [edi+edx*2]
    jnz     .height_loop

    pop     edi
    pop     esi
    ret
