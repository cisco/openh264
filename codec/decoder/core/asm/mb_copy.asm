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
;*      mb_copy and mb_copy1
;*
;*  History
;*      15/09/2009 Created
;*		12/28/2009 Modified with larger throughput
;*		12/29/2011 Tuned WelsCopy16x16NotAligned_sse2, added UpdateMbMv_sse2 WelsCopy16x8NotAligned_sse2, 
;*				   WelsCopy16x8_mmx, WelsCopy8x16_mmx etc;
;*
;*
;*********************************************************************************************/
%include "asm_inc.asm"
BITS 32

;*******************************************************************************
; Macros and other preprocessor constants
;*******************************************************************************

;*******************************************************************************
; Local Data (Read Only)
;*******************************************************************************

;SECTION .rodata data align=16

;*******************************************************************************
; Various memory constants (trigonometric values or rounding values)
;*******************************************************************************

ALIGN 16

;*******************************************************************************
; Code
;*******************************************************************************

SECTION .text

WELS_EXTERN PixelAvgWidthEq4_mmx
WELS_EXTERN PixelAvgWidthEq8_mmx
WELS_EXTERN PixelAvgWidthEq16_sse2

WELS_EXTERN McCopyWidthEq4_mmx
WELS_EXTERN McCopyWidthEq8_mmx
WELS_EXTERN McCopyWidthEq16_sse2


ALIGN 16
;*******************************************************************************
; void_t PixelAvgWidthEq4_mmx( uint8_t *pDst,  int iDstStride,
;                           uint8_t *pSrcA, int iSrcAStride,
;                           uint8_t *pSrcB, int iSrcBStride,
;                           int iHeight );
;*******************************************************************************
PixelAvgWidthEq4_mmx:
   
    push        esi
    push        edi
    push        ebp
    push        ebx

    mov         edi, [esp+20]       ; pDst
    mov         eax, [esp+24]       ; iDstStride
    mov         esi, [esp+28]       ; pSrcA
    mov         ecx, [esp+32]       ; iSrcAStride
    mov         ebp, [esp+36]       ; pSrcB
    mov         edx, [esp+40]       ; iSrcBStride
    mov         ebx, [esp+44]       ; iHeight
ALIGN 4
.height_loop:
	movd        mm0, [ebp]
    pavgb       mm0, [esi]
    movd        [edi], mm0
   
    dec         ebx
    lea         edi, [edi+eax]
    lea         esi, [esi+ecx]
    lea         ebp, [ebp+edx]
    jne         .height_loop

	WELSEMMS
    pop         ebx
    pop         ebp
    pop         edi
    pop         esi
    ret
                          
ALIGN 16
;*******************************************************************************
; void_t PixelAvgWidthEq8_mmx( uint8_t *pDst,  int iDstStride,
;                           uint8_t *pSrcA, int iSrcAStride,
;                           uint8_t *pSrcB, int iSrcBStride,
;                           int iHeight );
;*******************************************************************************
PixelAvgWidthEq8_mmx:
    
    push        esi
    push        edi
    push        ebp
    push        ebx

    mov         edi, [esp+20]       ; pDst
    mov         eax, [esp+24]       ; iDstStride
    mov         esi, [esp+28]       ; pSrcA
    mov         ecx, [esp+32]       ; iSrcAStride
    mov         ebp, [esp+36]       ; pSrcB
    mov         edx, [esp+40]       ; iSrcBStride
    mov         ebx, [esp+44]       ; iHeight
ALIGN 4
.height_loop:
	movq        mm0, [esi]
    pavgb       mm0, [ebp]
    movq        [edi], mm0
    movq        mm0, [esi+ecx]
    pavgb       mm0, [ebp+edx]
    movq		[edi+eax], mm0
    
    lea			esi,  [esi+2*ecx]
    lea			ebp, [ebp+2*edx]
    lea			edi,  [edi+2*eax]
    
    sub           ebx, 2
    jnz         .height_loop
	
	WELSEMMS
    pop         ebx
    pop         ebp
    pop         edi
    pop         esi
    ret



ALIGN 16
;*******************************************************************************
; void_t PixelAvgWidthEq16_sse2( uint8_t *pDst,  int iDstStride,
;                          uint8_t *pSrcA, int iSrcAStride,
;                          uint8_t *pSrcB, int iSrcBStride,
;                          int iHeight );
;*******************************************************************************
PixelAvgWidthEq16_sse2:
    push        esi
    push        edi
    push        ebp
    push        ebx
    

    mov         edi, [esp+20]       ; pDst
    mov         eax, [esp+24]       ; iDstStride
    mov         esi, [esp+28]       ; pSrcA
    mov         ecx, [esp+32]       ; iSrcAStride
    mov         ebp, [esp+36]       ; pSrcB
    mov         edx, [esp+40]       ; iSrcBStride
    mov         ebx, [esp+44]       ; iHeight
ALIGN 4
.height_loop:
	movdqu      xmm0, [esi]
	pavgb         xmm0, [ebp]
    movdqu      [edi], xmm0
    
	movdqu      xmm0, [esi+ecx]
	pavgb         xmm0, [ebp+edx]
    movdqu      [edi+eax], xmm0
	
	movdqu      xmm0, [esi+2*ecx]
	pavgb         xmm0, [ebp+2*edx]
    movdqu      [edi+2*eax], xmm0
    
    lea              esi,  [esi+2*ecx]
    lea			   ebp, [ebp+2*edx]
    lea			   edi,  [edi+2*eax]
     
	movdqu      xmm0, [esi+ecx]
	pavgb         xmm0, [ebp+edx]
    movdqu      [edi+eax], xmm0
    
    lea              esi,  [esi+2*ecx]
    lea			   ebp, [ebp+2*edx]
    lea			   edi,  [edi+2*eax]
	    
    
    sub         ebx, 4
    jne         .height_loop

	WELSEMMS
	pop         ebx
    pop         ebp
    pop         edi
    pop         esi

    ret


ALIGN 16
;*******************************************************************************
;  void_t McCopyWidthEq4_mmx( uint8_t *pSrc, int iSrcStride,
;                          uint8_t *pDst, int iDstStride, int iHeight )
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
;   void_t McCopyWidthEq8_mmx( uint8_t *pSrc, int iSrcStride,
;                           uint8_t *pDst, int iDstStride, int iHeight )
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
;*******************************************************************************
;   void_t McCopyWidthEq16_sse2( uint8_t *pSrc, int iSrcStride, uint8_t *pDst, int iDstStride, int iHeight )
;*******************************************************************************
;read unaligned memory
%macro SSE_READ_UNA 2
	movq	%1, [%2]
	movhps	%1,	[%2+8]
%endmacro

;write unaligned memory
%macro SSE_WRITE_UNA 2
	movq	[%1],	%2
	movhps	[%1+8], %2
%endmacro
McCopyWidthEq16_sse2:
    push    esi
    push    edi

    mov     esi, [esp+12]       ; pSrc
    mov     eax, [esp+16]       ; iSrcStride    
    mov     edi, [esp+20]       ; pDst
    mov     edx, [esp+24]       ; iDstStride
    mov     ecx, [esp+28]       ; iHeight

ALIGN 4
.height_loop:
    SSE_READ_UNA	xmm0, esi
    SSE_READ_UNA	xmm1, esi+eax
    SSE_WRITE_UNA	edi, xmm0
    SSE_WRITE_UNA	edi+edx, xmm1

	sub		ecx,	2
    lea     esi, [esi+eax*2]
    lea     edi, [edi+edx*2]
    jnz     .height_loop
  
    pop     edi
    pop     esi
    ret
