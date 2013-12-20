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
;*  mc_luma.asm
;*
;*  Abstract
;*      sse2 motion compensation
;*
;*  History
;*      17/08/2009 Created
;*
;*
;*************************************************************************/
%include "asm_inc.asm"

BITS 32

;***********************************************************************
; Macros and other preprocessor constants
;***********************************************************************
%macro SSE_LOAD_8P 3
	movq %1, %3
	punpcklbw %1, %2
%endmacro

%macro FILTER_HV_W8 9
	paddw	%1, %6
	movdqa	%8, %3
	movdqa	%7, %2
	paddw	%1, [h264_w0x10_1]
	paddw	%8, %4
	paddw	%7, %5
	psllw	%8, 2
	psubw	%8, %7
	paddw	%1, %8
	psllw	%8, 2
	paddw	%1, %8
	psraw   %1, 5
	WELS_Zero %8
	packuswb %1, %8
	movq    %9, %1
%endmacro
;***********************************************************************
; Local Data (Read Only)
;***********************************************************************

SECTION .rodata align=16

;***********************************************************************
; Various memory constants (trigonometric values or rounding values)
;***********************************************************************


;***********************************************************************
; Code
;***********************************************************************

SECTION .text

WELS_EXTERN McHorVer22Width8HorFirst_sse2
WELS_EXTERN McHorVer02WidthEq8_sse2
WELS_EXTERN McHorVer20WidthEq16_sse2

ALIGN 16
;***********************************************************************
; void McHorVer20WidthEq16_sse2(  uint8_t *pSrc,
;								int32_t iSrcStride,
;								uint8_t *pDst,
;								int32_t iDstStride,
;								int32_t iHeight,
;                      );
;***********************************************************************
McHorVer20WidthEq16_sse2:
	push	esi
	push	edi


	mov esi, [esp + 12]
	mov eax, [esp + 16]
	mov edi, [esp + 20]
	mov ecx, [esp + 28]
	mov edx, [esp + 24]
	sub esi, 2

	WELS_Zero  xmm7
	movdqa xmm6, [h264_w0x10_1]
.y_loop:

	movq xmm0, [esi]
	punpcklbw xmm0, xmm7
	movq xmm1, [esi+5]
	punpcklbw xmm1, xmm7
	movq xmm2, [esi+1]
	punpcklbw xmm2, xmm7
	movq xmm3, [esi+4]
	punpcklbw xmm3, xmm7
	movq xmm4, [esi+2]
	punpcklbw xmm4, xmm7
	movq xmm5, [esi+3]
	punpcklbw xmm5, xmm7

	paddw xmm2, xmm3
	paddw xmm4, xmm5
	psllw xmm4, 2
	psubw xmm4, xmm2
	paddw xmm0, xmm1
	paddw xmm0, xmm4
	psllw xmm4, 2
	paddw xmm0, xmm4
	paddw xmm0, xmm6
	psraw xmm0, 5
	packuswb xmm0, xmm7
	movq [edi], xmm0

	movq xmm0, [esi+8]
	punpcklbw xmm0, xmm7
	movq xmm1, [esi+5+8]
	punpcklbw xmm1, xmm7
	movq xmm2, [esi+1+8]
	punpcklbw xmm2, xmm7
	movq xmm3, [esi+4+8]
	punpcklbw xmm3, xmm7
	movq xmm4, [esi+2+8]
	punpcklbw xmm4, xmm7
	movq xmm5, [esi+3+8]
	punpcklbw xmm5, xmm7

	paddw xmm2, xmm3
	paddw xmm4, xmm5
	psllw xmm4, 2
	psubw xmm4, xmm2
	paddw xmm0, xmm1
	paddw xmm0, xmm4
	psllw xmm4, 2
	paddw xmm0, xmm4
	paddw xmm0, xmm6
	psraw xmm0, 5
	packuswb xmm0, xmm7
	movq [edi+8], xmm0


	add esi, eax
	add edi, edx
	dec ecx
	jnz .y_loop
	pop edi
	pop esi
	ret


ALIGN 16
;***********************************************************************
; void McHorVer22Width8HorFirst_sse2( uint8_t*pSrc,
;									int32_t iSrcStride,
;									uint8_t* pTap,
;									int32_t iTapStride,
;									int32_t iHeight);
;***********************************************************************
McHorVer22Width8HorFirst_sse2:
	push esi
	push edi
	push ebx
	mov esi, [esp+16]     ;pSrc
	mov eax, [esp+20]	;src_stride
	mov edi, [esp+24]		;tap
	mov edx, [esp+28]	;tap_stride
	mov ebx, [esp+32]	;i_height
	pxor xmm7, xmm7

	sub esi, eax				;;;;;;;;need more 5 lines.
	sub esi, eax

.yloop_width_8:
	movq xmm0, [esi]
	punpcklbw xmm0, xmm7
	movq xmm1, [esi+5]
	punpcklbw xmm1, xmm7
	movq xmm2, [esi+1]
	punpcklbw xmm2, xmm7
	movq xmm3, [esi+4]
	punpcklbw xmm3, xmm7
	movq xmm4, [esi+2]
	punpcklbw xmm4, xmm7
	movq xmm5, [esi+3]
	punpcklbw xmm5, xmm7

	paddw xmm2, xmm3
	paddw xmm4, xmm5
	psllw xmm4, 2
	psubw xmm4, xmm2
	paddw xmm0, xmm1
	paddw xmm0, xmm4
	psllw xmm4, 2
	paddw xmm0, xmm4
	movdqa [edi], xmm0

	add esi, eax
	add edi, edx
	dec ebx
	jnz .yloop_width_8
	pop ebx
	pop edi
	pop esi
	ret

;***********************************************************************
; void McHorVer02WidthEq8_sse2( uint8_t *pSrc,
;                       int32_t iSrcStride,
;                       uint8_t *pDst,
;                       int32_t iDstStride,
;                       int32_t iHeight )
;***********************************************************************
ALIGN 16
McHorVer02WidthEq8_sse2:
	push esi
	push edi

	mov esi, [esp + 12]
	mov edx, [esp + 16]
	mov edi, [esp + 20]
	mov eax, [esp + 24]
	mov ecx, [esp + 28]

	sub esi, edx
	sub esi, edx

	WELS_Zero xmm7

	SSE_LOAD_8P xmm0, xmm7, [esi]
	SSE_LOAD_8P xmm1, xmm7, [esi+edx]
	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm2, xmm7, [esi]
	SSE_LOAD_8P xmm3, xmm7, [esi+edx]
	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm4, xmm7, [esi]
	SSE_LOAD_8P xmm5, xmm7, [esi+edx]

.start:
	FILTER_HV_W8 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [edi]
	dec ecx
	jz near .xx_exit

	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm6, xmm7, [esi]
	FILTER_HV_W8 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, [edi+eax]
	dec ecx
	jz near .xx_exit

	lea edi, [edi+2*eax]
	SSE_LOAD_8P xmm7, xmm0, [esi+edx]
	FILTER_HV_W8 xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [edi]
	dec ecx
	jz near .xx_exit

	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm0, xmm1, [esi]
	FILTER_HV_W8 xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, [edi+eax]
	dec ecx
	jz near .xx_exit

	lea edi, [edi+2*eax]
	SSE_LOAD_8P xmm1, xmm2, [esi+edx]
	FILTER_HV_W8 xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, [edi]
	dec ecx
	jz near .xx_exit

	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm2, xmm3, [esi]
	FILTER_HV_W8 xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, [edi+eax]
	dec ecx
	jz near .xx_exit

	lea edi, [edi+2*eax]
	SSE_LOAD_8P xmm3, xmm4, [esi+edx]
	FILTER_HV_W8 xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, [edi]
	dec ecx
	jz near .xx_exit

	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm4, xmm5, [esi]
	FILTER_HV_W8 xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, [edi+eax]
	dec ecx
	jz near .xx_exit

	lea edi, [edi+2*eax]
	SSE_LOAD_8P xmm5, xmm6, [esi+edx]
	jmp near .start

.xx_exit:
	pop edi
	pop esi
	ret




;***********************************************************************
; Local Data (Read Only)
;***********************************************************************

%ifdef FORMAT_COFF
SECTION .rodata pData
%else
SECTION .rodata align=16
%endif

;***********************************************************************
; Various memory constants (trigonometric values or rounding values)
;***********************************************************************
ALIGN 16
h264_w0x10_1:
	dw 16, 16, 16, 16, 16, 16, 16, 16
ALIGN 16
h264_mc_hc_32:
	dw 32, 32, 32, 32, 32, 32, 32, 32

;***********************************************************************
; Code
;***********************************************************************

SECTION .text

WELS_EXTERN McHorVer20_sse2
WELS_EXTERN McHorVer02_sse2
WELS_EXTERN McHorVer22VerLastAlign_sse2
WELS_EXTERN McHorVer22VerLastUnAlign_sse2
WELS_EXTERN McHorVer22HorFirst_sse2


;***********************************************************************
; void McHorVer02_sse2(	uint8_t *pSrc,
;                       int32_t iSrcStride,
;                       uint8_t *pDst,
;                       int32_t iDstStride,
;						int32_t iWidth,
;                       int32_t iHeight )
;***********************************************************************
ALIGN 16
McHorVer02_sse2:
	push esi
	push edi
	push ebx

	mov esi, [esp + 16]
	mov edx, [esp + 20]
	mov edi, [esp + 24]
	mov eax, [esp + 28]
	mov ecx, [esp + 36]
	mov ebx, [esp + 32]
	shr ebx, 3
	sub esi, edx
	sub esi, edx

.xloop:
	WELS_Zero xmm7
	SSE_LOAD_8P xmm0, xmm7, [esi]
	SSE_LOAD_8P xmm1, xmm7, [esi+edx]
	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm2, xmm7, [esi]
	SSE_LOAD_8P xmm3, xmm7, [esi+edx]
	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm4, xmm7, [esi]
	SSE_LOAD_8P xmm5, xmm7, [esi+edx]

	FILTER_HV_W8 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [edi]
	dec ecx
	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm6, xmm7, [esi]
	movdqa xmm0,xmm1
	movdqa xmm1,xmm2
	movdqa xmm2,xmm3
	movdqa xmm3,xmm4
	movdqa xmm4,xmm5
	movdqa xmm5,xmm6
	add edi, eax
	sub esi, edx

.start:
	FILTER_HV_W8 xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm6, xmm7, [esi]
	FILTER_HV_W8 xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, [edi+eax]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*eax]
	SSE_LOAD_8P xmm7, xmm0, [esi+edx]
	FILTER_HV_W8 xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm0, xmm1, [esi]
	FILTER_HV_W8 xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, [edi+eax]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*eax]
	SSE_LOAD_8P xmm1, xmm2, [esi+edx]
	FILTER_HV_W8 xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, [edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm2, xmm3, [esi]
	FILTER_HV_W8 xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, [edi+eax]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*eax]
	SSE_LOAD_8P xmm3, xmm4, [esi+edx]
	FILTER_HV_W8 xmm6, xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, [edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*edx]
	SSE_LOAD_8P xmm4, xmm5, [esi]
	FILTER_HV_W8 xmm7, xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, [edi+eax]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*eax]
	SSE_LOAD_8P xmm5, xmm6, [esi+edx]
	jmp near .start

.x_loop_dec:
	dec ebx
	jz  near .xx_exit
	mov esi, [esp + 16]
	mov edi, [esp + 24]
	sub esi, edx
	sub esi, edx
	add esi, 8
	add edi, 8
	mov ecx, [esp + 36]
	jmp near .xloop

.xx_exit:
	pop ebx
	pop edi
	pop esi
	ret


ALIGN 16
;***********************************************************************
; void McHorVer20_sse2(		uint8_t *pSrc,
;                       int32_t iSrcStride,
;						uint8_t *pDst,
;						int32_t iDstStride,
;						int32_t iWidth,
;						int32_t iHeight
;                      );
;***********************************************************************
McHorVer20_sse2:
	push esi
	push edi
	push ebx
	mov esi, [esp+16]
	mov eax, [esp+20]
	mov edi, [esp+24]
	mov edx, [esp+28]
	mov ecx, [esp+32]
	mov ebx, [esp+36]
	sub esi, 2
	pxor xmm7, xmm7

	cmp ecx, 9
	jne near .width_17

.yloop_width_9:
	movq xmm0, [esi]
	punpcklbw xmm0, xmm7
	movq xmm1, [esi+5]
	punpcklbw xmm1, xmm7
	movq xmm2, [esi+1]
	punpcklbw xmm2, xmm7
	movq xmm3, [esi+4]
	punpcklbw xmm3, xmm7
	movq xmm4, [esi+2]
	punpcklbw xmm4, xmm7
	movq xmm5, [esi+3]
	punpcklbw xmm5, xmm7

	movdqa xmm7, xmm2
	paddw   xmm7, xmm3
	movdqa xmm6, xmm4
	paddw   xmm6, xmm5
	psllw xmm6, 2
	psubw xmm6, xmm7
	paddw xmm0, xmm1
	paddw xmm0, xmm6
	psllw xmm6, 2
	paddw xmm0, xmm6
	paddw xmm0, [h264_w0x10_1]
	psraw  xmm0, 5
	packuswb xmm0, xmm0
	movd [edi], xmm0

	pxor  xmm7, xmm7
	movq xmm0, [esi+6]
	punpcklbw xmm0, xmm7

	paddw xmm4, xmm1
	paddw xmm5, xmm3
	psllw xmm5, 2
	psubw xmm5, xmm4
	paddw xmm2, xmm0
	paddw xmm2, xmm5
	psllw xmm5, 2
	paddw xmm2, xmm5
	paddw xmm2, [h264_w0x10_1]
	psraw  xmm2, 5
	packuswb xmm2, xmm2
	movq [edi+1], xmm2

	add esi, eax
	add edi, edx
	dec ebx
	jnz .yloop_width_9
	pop ebx
	pop edi
	pop esi
	ret


.width_17:
.yloop_width_17:
	movq xmm0, [esi]
	punpcklbw xmm0, xmm7
	movq xmm1, [esi+5]
	punpcklbw xmm1, xmm7
	movq xmm2, [esi+1]
	punpcklbw xmm2, xmm7
	movq xmm3, [esi+4]
	punpcklbw xmm3, xmm7
	movq xmm4, [esi+2]
	punpcklbw xmm4, xmm7
	movq xmm5, [esi+3]
	punpcklbw xmm5, xmm7

	paddw xmm2, xmm3
	paddw xmm4, xmm5
	psllw xmm4, 2
	psubw xmm4, xmm2
	paddw xmm0, xmm1
	paddw xmm0, xmm4
	psllw xmm4, 2
	paddw xmm0, xmm4
	paddw xmm0, [h264_w0x10_1]
	psraw  xmm0, 5
	packuswb xmm0, xmm0
	movq [edi], xmm0

	movq xmm0, [esi+8]
	punpcklbw xmm0, xmm7
	movq xmm1, [esi+5+8]
	punpcklbw xmm1, xmm7
	movq xmm2, [esi+1+8]
	punpcklbw xmm2, xmm7
	movq xmm3, [esi+4+8]
	punpcklbw xmm3, xmm7
	movq xmm4, [esi+2+8]
	punpcklbw xmm4, xmm7
	movq xmm5, [esi+3+8]
	punpcklbw xmm5, xmm7

	movdqa xmm7, xmm2
	paddw   xmm7, xmm3
	movdqa xmm6, xmm4
	paddw   xmm6, xmm5
	psllw xmm6, 2
	psubw xmm6, xmm7
	paddw xmm0, xmm1
	paddw xmm0, xmm6
	psllw xmm6, 2
	paddw xmm0, xmm6
	paddw xmm0, [h264_w0x10_1]
	psraw  xmm0, 5
	packuswb xmm0, xmm0
	movd [edi+8], xmm0


	pxor  xmm7, xmm7
	movq xmm0, [esi+6+8]
	punpcklbw xmm0, xmm7

	paddw xmm4, xmm1
	paddw xmm5, xmm3
	psllw xmm5, 2
	psubw xmm5, xmm4
	paddw xmm2, xmm0
	paddw xmm2, xmm5
	psllw xmm5, 2
	paddw xmm2, xmm5
	paddw xmm2, [h264_w0x10_1]
	psraw  xmm2, 5
	packuswb xmm2, xmm2
	movq [edi+9], xmm2
	add esi, eax
	add edi, edx
	dec ebx
	jnz .yloop_width_17
	pop ebx
	pop edi
	pop esi
	ret



ALIGN 16
;***********************************************************************
;void McHorVer22HorFirst_sse2
;							(uint8_t *pSrc,
;							int32_t iSrcStride,
;							uint8_t * pTap,
;							int32_t iTapStride,
;							int32_t iWidth,int32_t iHeight);
;***********************************************************************
McHorVer22HorFirst_sse2:
	push esi
	push edi
	push ebx
	mov esi, [esp+16]
	mov eax, [esp+20]
	mov edi, [esp+24]
	mov edx, [esp+28]
	mov ecx, [esp+32]
	mov ebx, [esp+36]
	pxor xmm7, xmm7

	sub esi, eax				;;;;;;;;need more 5 lines.
	sub esi, eax

	cmp ecx, 9
	jne near .width_17

.yloop_width_9:
	movq xmm0, [esi]
	punpcklbw xmm0, xmm7
	movq xmm1, [esi+5]
	punpcklbw xmm1, xmm7
	movq xmm2, [esi+1]
	punpcklbw xmm2, xmm7
	movq xmm3, [esi+4]
	punpcklbw xmm3, xmm7
	movq xmm4, [esi+2]
	punpcklbw xmm4, xmm7
	movq xmm5, [esi+3]
	punpcklbw xmm5, xmm7

	movdqa xmm7, xmm2
	paddw   xmm7, xmm3
	movdqa xmm6, xmm4
	paddw   xmm6, xmm5
	psllw xmm6, 2
	psubw xmm6, xmm7
	paddw xmm0, xmm1
	paddw xmm0, xmm6
	psllw xmm6, 2
	paddw xmm0, xmm6
	movd [edi], xmm0

	pxor  xmm7, xmm7
	movq xmm0, [esi+6]
	punpcklbw xmm0, xmm7

	paddw xmm4, xmm1
	paddw xmm5, xmm3
	psllw xmm5, 2
	psubw xmm5, xmm4
	paddw xmm2, xmm0
	paddw xmm2, xmm5
	psllw xmm5, 2
	paddw xmm2, xmm5
	movq [edi+2], xmm2
	movhps [edi+2+8], xmm2

	add esi, eax
	add edi, edx
	dec ebx
	jnz .yloop_width_9
	pop ebx
	pop edi
	pop esi
	ret


.width_17:
.yloop_width_17:
	movq xmm0, [esi]
	punpcklbw xmm0, xmm7
	movq xmm1, [esi+5]
	punpcklbw xmm1, xmm7
	movq xmm2, [esi+1]
	punpcklbw xmm2, xmm7
	movq xmm3, [esi+4]
	punpcklbw xmm3, xmm7
	movq xmm4, [esi+2]
	punpcklbw xmm4, xmm7
	movq xmm5, [esi+3]
	punpcklbw xmm5, xmm7

	paddw xmm2, xmm3
	paddw xmm4, xmm5
	psllw xmm4, 2
	psubw xmm4, xmm2
	paddw xmm0, xmm1
	paddw xmm0, xmm4
	psllw xmm4, 2
	paddw xmm0, xmm4
	movdqa [edi], xmm0

	movq xmm0, [esi+8]
	punpcklbw xmm0, xmm7
	movq xmm1, [esi+5+8]
	punpcklbw xmm1, xmm7
	movq xmm2, [esi+1+8]
	punpcklbw xmm2, xmm7
	movq xmm3, [esi+4+8]
	punpcklbw xmm3, xmm7
	movq xmm4, [esi+2+8]
	punpcklbw xmm4, xmm7
	movq xmm5, [esi+3+8]
	punpcklbw xmm5, xmm7

	movdqa xmm7, xmm2
	paddw   xmm7, xmm3
	movdqa xmm6, xmm4
	paddw   xmm6, xmm5
	psllw xmm6, 2
	psubw xmm6, xmm7
	paddw xmm0, xmm1
	paddw xmm0, xmm6
	psllw xmm6, 2
	paddw xmm0, xmm6
	movd [edi+16], xmm0


	pxor  xmm7, xmm7
	movq xmm0, [esi+6+8]
	punpcklbw xmm0, xmm7

	paddw xmm4, xmm1
	paddw xmm5, xmm3
	psllw xmm5, 2
	psubw xmm5, xmm4
	paddw xmm2, xmm0
	paddw xmm2, xmm5
	psllw xmm5, 2
	paddw xmm2, xmm5
	movq [edi+18], xmm2
	movhps [edi+18+8], xmm2

	add esi, eax
	add edi, edx
	dec ebx
	jnz .yloop_width_17
	pop ebx
	pop edi
	pop esi
	ret


%macro FILTER_VER 9
	paddw  %1, %6
	movdqa %7, %2
	movdqa %8, %3


	paddw %7, %5
	paddw %8, %4

	psubw  %1, %7
	psraw   %1, 2
	paddw  %1, %8
	psubw  %1, %7
	psraw   %1, 2
	paddw  %8, %1
	paddw  %8, [h264_mc_hc_32]
	psraw   %8, 6
	packuswb %8, %8
	movq %9, %8
%endmacro
;***********************************************************************
;void McHorVer22VerLastAlign_sse2(
;											uint8_t *pTap,
;											int32_t iTapStride,
;											uint8_t * pDst,
;											int32_t iDstStride,
;											int32_t iWidth,
;											int32_t iHeight);
;***********************************************************************

 McHorVer22VerLastAlign_sse2:
	push esi
	push edi
	push ebx
	push ebp

	mov esi, [esp+20]
	mov eax, [esp+24]
	mov edi, [esp+28]
	mov edx, [esp+32]
	mov ebx, [esp+36]
	mov ecx, [esp+40]
	shr ebx, 3

.width_loop:
	movdqa xmm0, [esi]
	movdqa xmm1, [esi+eax]
	lea esi, [esi+2*eax]
	movdqa xmm2, [esi]
	movdqa xmm3, [esi+eax]
	lea esi, [esi+2*eax]
	movdqa xmm4, [esi]
	movdqa xmm5, [esi+eax]

	FILTER_VER xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [edi]
	dec ecx
	lea esi, [esi+2*eax]
	movdqa xmm6, [esi]

	movdqa xmm0, xmm1
	movdqa xmm1, xmm2
	movdqa xmm2, xmm3
	movdqa xmm3, xmm4
	movdqa xmm4, xmm5
	movdqa xmm5, xmm6

	add edi, edx
	sub esi, eax

.start:
	FILTER_VER xmm0,xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*eax]
	movdqa xmm6, [esi]
	FILTER_VER xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,[edi+edx]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*edx]
	movdqa xmm7, [esi+eax]
	FILTER_VER  xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*eax]
	movdqa xmm0, [esi]
	FILTER_VER  xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,[edi+edx]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*edx]
	movdqa xmm1, [esi+eax]
	FILTER_VER  xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,[edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*eax]
	movdqa xmm2, [esi]
	FILTER_VER  xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,[edi+edx]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*edx]
	movdqa xmm3, [esi+eax]
	FILTER_VER  xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,[edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*eax]
	movdqa xmm4, [esi]
	FILTER_VER  xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,xmm6, [edi+edx]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*edx]
	movdqa xmm5, [esi+eax]
	jmp near .start

.x_loop_dec:
	dec ebx
	jz near .exit
	mov esi, [esp+20]
	mov edi, [esp+28]
	mov ecx, [esp+40]
	add esi, 16
	add edi, 8
	jmp .width_loop



.exit:
	pop ebp
	pop ebx
	pop edi
	pop esi
	ret

;***********************************************************************
;void McHorVer22VerLastUnAlign_sse2(
;											uint8_t *pTap,
;											int32_t iTapStride,
;											uint8_t * pDst,
;											int32_t iDstStride,
;											int32_t iWidth,
;											int32_t iHeight);
;***********************************************************************

 McHorVer22VerLastUnAlign_sse2:
	push esi
	push edi
	push ebx
	push ebp

	mov esi, [esp+20]
	mov eax, [esp+24]
	mov edi, [esp+28]
	mov edx, [esp+32]
	mov ebx, [esp+36]
	mov ecx, [esp+40]
	shr ebx, 3

.width_loop:
	movdqu xmm0, [esi]
	movdqu xmm1, [esi+eax]
	lea esi, [esi+2*eax]
	movdqu xmm2, [esi]
	movdqu xmm3, [esi+eax]
	lea esi, [esi+2*eax]
	movdqu xmm4, [esi]
	movdqu xmm5, [esi+eax]

	FILTER_VER xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [edi]
	dec ecx
	lea esi, [esi+2*eax]
	movdqu xmm6, [esi]

	movdqa xmm0, xmm1
	movdqa xmm1, xmm2
	movdqa xmm2, xmm3
	movdqa xmm3, xmm4
	movdqa xmm4, xmm5
	movdqa xmm5, xmm6

	add edi, edx
	sub esi, eax

.start:
	FILTER_VER xmm0,xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*eax]
	movdqu xmm6, [esi]
	FILTER_VER xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0,[edi+edx]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*edx]
	movdqu xmm7, [esi+eax]
	FILTER_VER  xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, [edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*eax]
	movdqu xmm0, [esi]
	FILTER_VER  xmm3, xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2,[edi+edx]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*edx]
	movdqu xmm1, [esi+eax]
	FILTER_VER  xmm4, xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,[edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*eax]
	movdqu xmm2, [esi]
	FILTER_VER  xmm5, xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,[edi+edx]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*edx]
	movdqu xmm3, [esi+eax]
	FILTER_VER  xmm6, xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,[edi]
	dec ecx
	jz near .x_loop_dec

	lea esi, [esi+2*eax]
	movdqu xmm4, [esi]
	FILTER_VER  xmm7, xmm0, xmm1, xmm2, xmm3,xmm4,xmm5,xmm6, [edi+edx]
	dec ecx
	jz near .x_loop_dec

	lea edi, [edi+2*edx]
	movdqu xmm5, [esi+eax]
	jmp near .start

.x_loop_dec:
	dec ebx
	jz near .exit
	mov esi, [esp+20]
	mov edi, [esp+28]
	mov ecx, [esp+40]
	add esi, 16
	add edi, 8
	jmp .width_loop



.exit:
	pop ebp
	pop ebx
	pop edi
	pop esi
	ret
