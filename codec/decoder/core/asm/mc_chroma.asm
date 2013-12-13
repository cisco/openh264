;*!
;* \copy
;*     Copyright (c)  2004-2013, Cisco Systems
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
;*  mc_chroma.asm
;*
;*  Abstract
;*      mmx motion compensation for chroma
;*
;*  History
;*      10/13/2004 Created
;*
;*
;*************************************************************************/
%include "asm_inc.asm"

BITS 32

;***********************************************************************
; Local Data (Read Only)
;***********************************************************************

SECTION .rodata align=16

;***********************************************************************
; Various memory constants (trigonometric values or rounding values)
;***********************************************************************

ALIGN 16
h264_d0x20_sse2:
	dw 32,32,32,32,32,32,32,32
ALIGN 16
h264_d0x20_mmx:
	dw 32,32,32,32


;=============================================================================
; Code
;=============================================================================

SECTION .text

ALIGN 16
;*******************************************************************************
; void McChromaWidthEq4_mmx( uint8_t *src, 
;							int32_t iSrcStride, 
;							uint8_t *pDst, 
;							int32_t iDstStride, 
;							uint8_t *pABCD, 
;							int32_t iHeigh );
;*******************************************************************************
WELS_EXTERN McChromaWidthEq4_mmx
McChromaWidthEq4_mmx:
	push esi
	push edi
	push ebx
	
	mov eax, [esp +12 + 20]
	movd mm3, [eax]
	WELS_Zero mm7
	punpcklbw mm3, mm3
	movq      mm4, mm3
	punpcklwd mm3, mm3       
	punpckhwd mm4, mm4		 
	
	movq	  mm5, mm3
	punpcklbw mm3, mm7
	punpckhbw mm5, mm7
	
	movq	  mm6, mm4
	punpcklbw mm4, mm7
	punpckhbw mm6, mm7
	
	mov esi, [esp +12+ 4]   
	mov eax, [esp + 12 + 8]   
	mov edi, [esp + 12 + 12]  
	mov edx, [esp + 12 + 16]  
    mov ecx, [esp + 12 + 24]   
		
	lea ebx, [esi + eax]
	movd mm0, [esi]
	movd mm1, [esi+1]
	punpcklbw mm0, mm7
	punpcklbw mm1, mm7
.xloop:
	
	pmullw mm0, mm3
	pmullw mm1, mm5
	paddw  mm0, mm1
	
	movd  mm1, [ebx]
	punpcklbw mm1, mm7
	movq mm2, mm1
	pmullw mm1, mm4
	paddw mm0, mm1
	
	movd mm1, [ebx+1]
	punpcklbw mm1, mm7
	movq mm7, mm1
	pmullw mm1,mm6
	paddw mm0, mm1
	movq mm1,mm7

	paddw mm0, [h264_d0x20_mmx]
	psrlw mm0, 6
	
	WELS_Zero mm7
	packuswb mm0, mm7
	movd [edi], mm0	

	movq mm0, mm2
	
	lea edi, [edi +edx  ]
	lea ebx, [ebx + eax]

	dec ecx
	jnz near .xloop
	WELSEMMS
	pop ebx
	pop edi
	pop esi
	ret


ALIGN 16
;*******************************************************************************
; void McChromaWidthEq8_sse2( uint8_t *pSrc, 
;						int32_t iSrcStride, 
;						uint8_t *pDst, 
;						int32_t iDstStride, 
;						uint8_t *pABCD, 
;						int32_t iheigh );
;*******************************************************************************
WELS_EXTERN McChromaWidthEq8_sse2
McChromaWidthEq8_sse2:
	push esi
	push edi
	push ebx
	
	mov eax, [esp +12 + 20]
	movd xmm3, [eax]
	WELS_Zero xmm7
	punpcklbw  xmm3, xmm3
	punpcklwd  xmm3, xmm3
	
	movdqa	   xmm4, xmm3
	punpckldq  xmm3, xmm3
	punpckhdq  xmm4, xmm4
	movdqa     xmm5, xmm3
	movdqa	   xmm6, xmm4
	
	punpcklbw  xmm3, xmm7
	punpckhbw  xmm5, xmm7
	punpcklbw  xmm4, xmm7
	punpckhbw  xmm6, xmm7
	
	mov esi, [esp +12+ 4]   
	mov eax, [esp + 12 + 8]   
	mov edi, [esp + 12 + 12]  
	mov edx, [esp + 12 + 16]  
    mov ecx, [esp + 12 + 24]   
		
	lea ebx, [esi + eax]
	movq xmm0, [esi]
	movq xmm1, [esi+1]
	punpcklbw xmm0, xmm7
	punpcklbw xmm1, xmm7
.xloop:
	
	pmullw xmm0, xmm3
	pmullw xmm1, xmm5
	paddw  xmm0, xmm1
	
	movq  xmm1, [ebx]
	punpcklbw xmm1, xmm7
	movdqa xmm2, xmm1
	pmullw xmm1, xmm4
	paddw xmm0, xmm1
	
	movq xmm1, [ebx+1]
	punpcklbw xmm1, xmm7
	movdqa xmm7, xmm1
	pmullw xmm1, xmm6
	paddw xmm0, xmm1
	movdqa xmm1,xmm7

	paddw xmm0, [h264_d0x20_sse2]
	psrlw xmm0, 6
	
	WELS_Zero xmm7
	packuswb xmm0, xmm7
	movq [edi], xmm0	

	movdqa xmm0, xmm2
	
	lea edi, [edi +edx  ]
	lea ebx, [ebx + eax]

	dec ecx
	jnz near .xloop
	
	pop ebx
	pop edi
	pop esi
	ret




ALIGN 16
;***********************************************************************
; void McChromaWidthEq8_ssse3( uint8_t *pSrc,
;						 int32_t iSrcStride, 
;                        uint8_t *pDst,  
;                        int32_t iDstStride,
;                        uint8_t *pABCD,
;					     int32_t iHeigh);
;***********************************************************************
WELS_EXTERN McChromaWidthEq8_ssse3
McChromaWidthEq8_ssse3:
	push ebx
	push esi
	push edi
		
	mov eax, [esp + 12 + 20]

    pxor      xmm7, xmm7
    movd   xmm5, [eax]   
    punpcklwd xmm5, xmm5  
    punpckldq xmm5, xmm5 
    movdqa    xmm6, xmm5
    punpcklqdq xmm5, xmm5
    punpckhqdq xmm6, xmm6    
    
	mov eax, [esp + 12 + 4]   
	mov edx, [esp + 12 + 8]   
	mov esi, [esp + 12 + 12]  
	mov edi, [esp + 12 + 16]  
    mov ecx, [esp + 12 + 24]   
    
    sub esi, edi
    sub esi, edi
	movdqa xmm7, [h264_d0x20_sse2]

	movdqu xmm0, [eax]
	movdqa xmm1, xmm0
	psrldq xmm1, 1
	punpcklbw xmm0, xmm1
	
.hloop_chroma:	
	lea	esi, [esi+2*edi]
	
	movdqu xmm2, [eax+edx]
	movdqa xmm3, xmm2
	psrldq xmm3, 1
	punpcklbw xmm2, xmm3
	movdqa      xmm4, xmm2
	
    pmaddubsw  xmm0, xmm5
    pmaddubsw  xmm2, xmm6
    paddw      xmm0, xmm2
    paddw      xmm0, xmm7
	psrlw      xmm0, 6
    packuswb   xmm0, xmm0
    movq       [esi],xmm0	
    
    lea eax, [eax+2*edx]
    movdqu xmm2, [eax]
    movdqa xmm3, xmm2
    psrldq xmm3, 1
    punpcklbw xmm2, xmm3
    movdqa      xmm0, xmm2
    
    pmaddubsw  xmm4, xmm5
    pmaddubsw  xmm2, xmm6
    paddw      xmm4, xmm2
    paddw      xmm4, xmm7
	psrlw      xmm4, 6
    packuswb   xmm4, xmm4
    movq       [esi+edi],xmm4	
	
	sub ecx, 2
	jnz .hloop_chroma
	pop edi
	pop esi
	pop ebx

	ret


