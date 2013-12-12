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
;*  expand_picture.asm
;*
;*  Abstract
;*      mmxext/sse for expand_frame
;*
;*  History
;*      09/25/2009 Created
;*
;*
;*************************************************************************/

%include "asm_inc.asm"

BITS 32

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
;%define PADDING_SIZE_ASM 	32 	; PADDING_LENGTH

;***********************************************************************
; Code
;***********************************************************************



SECTION .text

WELS_EXTERN ExpandPictureLuma_sse2
WELS_EXTERN ExpandPictureChromaAlign_sse2	; for chroma alignment
WELS_EXTERN ExpandPictureChromaUnalign_sse2	; for chroma unalignment

;;;;;;;expanding result;;;;;;;

;aaaa|attttttttttttttttb|bbbb
;aaaa|attttttttttttttttb|bbbb
;aaaa|attttttttttttttttb|bbbb
;aaaa|attttttttttttttttb|bbbb
;----------------------------
;aaaa|attttttttttttttttb|bbbb
;llll|l                r|rrrr
;llll|l                r|rrrr
;llll|l                r|rrrr
;llll|l                r|rrrr
;llll|l                r|rrrr
;cccc|ceeeeeeeeeeeeeeeed|dddd
;----------------------------
;cccc|ceeeeeeeeeeeeeeeed|dddd
;cccc|ceeeeeeeeeeeeeeeed|dddd
;cccc|ceeeeeeeeeeeeeeeed|dddd
;cccc|ceeeeeeeeeeeeeeeed|dddd

%macro mov_line_8x4_mmx		3	; dst, stride, mm?
	movq [%1], %3
	movq [%1+%2], %3
	lea %1, [%1+2*%2]
	movq [%1], %3
	movq [%1+%2], %3
	lea %1, [%1+2*%2]
%endmacro

%macro mov_line_end8x4_mmx		3	; dst, stride, mm?
	movq [%1], %3
	movq [%1+%2], %3
	lea %1, [%1+2*%2]
	movq [%1], %3
	movq [%1+%2], %3
	lea %1, [%1+%2]
%endmacro

%macro mov_line_16x4_sse2	4	; dst, stride, xmm?, u/a
	movdq%4 [%1], %3 		; top(bottom)_0
	movdq%4 [%1+%2], %3		; top(bottom)_1
	lea %1, [%1+2*%2]
	movdq%4 [%1], %3 		; top(bottom)_2
	movdq%4 [%1+%2], %3		; top(bottom)_3
	lea %1, [%1+2*%2]
%endmacro

%macro mov_line_end16x4_sse2	4	; dst, stride, xmm?, u/a
	movdq%4 [%1], %3 		; top(bottom)_0
	movdq%4 [%1+%2], %3		; top(bottom)_1
	lea %1, [%1+2*%2]
	movdq%4 [%1], %3 		; top(bottom)_2
	movdq%4 [%1+%2], %3		; top(bottom)_3
	lea %1, [%1+%2]
%endmacro

%macro mov_line_32x4_sse2	3	; dst, stride, xmm?
	movdqa [%1], %3 		; top(bottom)_0
	movdqa [%1+16], %3 		; top(bottom)_0
	movdqa [%1+%2], %3		; top(bottom)_1
	movdqa [%1+%2+16], %3		; top(bottom)_1
	lea %1, [%1+2*%2]
	movdqa [%1], %3 		; top(bottom)_2
	movdqa [%1+16], %3 		; top(bottom)_2
	movdqa [%1+%2], %3		; top(bottom)_3
	movdqa [%1+%2+16], %3		; top(bottom)_3
	lea %1, [%1+2*%2]
%endmacro

%macro mov_line_end32x4_sse2	3	; dst, stride, xmm?
	movdqa [%1], %3 		; top(bottom)_0
	movdqa [%1+16], %3 		; top(bottom)_0
	movdqa [%1+%2], %3		; top(bottom)_1
	movdqa [%1+%2+16], %3		; top(bottom)_1
	lea %1, [%1+2*%2]
	movdqa [%1], %3 		; top(bottom)_2
	movdqa [%1+16], %3 		; top(bottom)_2
	movdqa [%1+%2], %3		; top(bottom)_3
	movdqa [%1+%2+16], %3		; top(bottom)_3
	lea %1, [%1+%2]
%endmacro

%macro exp_top_bottom_sse2	1	; iPaddingSize [luma(32)/chroma(16)]
	; ebx [width/16(8)]
	; esi [pSrc+0], edi [pSrc-1], ecx [-stride], 32(16)		; top
	; eax [pSrc+(h-1)*stride], ebp [pSrc+(h+31)*stride], 32(16)	; bottom

%if %1 == 32		; for luma
	sar ebx, 04h 	; width / 16(8) pixels
.top_bottom_loops:
	; top
	movdqa xmm0, [esi]		; first line of picture pData
	mov_line_16x4_sse2 edi, ecx, xmm0, a	; dst, stride, xmm?
	mov_line_16x4_sse2 edi, ecx, xmm0, a
	mov_line_16x4_sse2 edi, ecx, xmm0, a
	mov_line_16x4_sse2 edi, ecx, xmm0, a
	mov_line_16x4_sse2 edi, ecx, xmm0, a	; dst, stride, xmm?
	mov_line_16x4_sse2 edi, ecx, xmm0, a
	mov_line_16x4_sse2 edi, ecx, xmm0, a
	mov_line_end16x4_sse2 edi, ecx, xmm0, a

	; bottom
	movdqa xmm1, [eax] 		; last line of picture pData
	mov_line_16x4_sse2 ebp, ecx, xmm1, a	; dst, stride, xmm?
	mov_line_16x4_sse2 ebp, ecx, xmm1, a
	mov_line_16x4_sse2 ebp, ecx, xmm1, a
	mov_line_16x4_sse2 ebp, ecx, xmm1, a
	mov_line_16x4_sse2 ebp, ecx, xmm1, a	; dst, stride, xmm?
	mov_line_16x4_sse2 ebp, ecx, xmm1, a
	mov_line_16x4_sse2 ebp, ecx, xmm1, a
	mov_line_end16x4_sse2 ebp, ecx, xmm1, a

	lea esi, [esi+16]		; top pSrc
	lea edi, [edi+16]		; top dst
	lea eax, [eax+16]		; bottom pSrc
	lea ebp, [ebp+16]		; bottom dst
	neg ecx 			; positive/negative stride need for next loop?

	dec ebx
	jnz near .top_bottom_loops
%elif %1 == 16	; for chroma ??
	mov edx, ebx
	sar ebx, 04h 	; (width / 16) pixels
.top_bottom_loops:
	; top
	movdqa xmm0, [esi]		; first line of picture pData
	mov_line_16x4_sse2 edi, ecx, xmm0, a	; dst, stride, xmm?
	mov_line_16x4_sse2 edi, ecx, xmm0, a
	mov_line_16x4_sse2 edi, ecx, xmm0, a
	mov_line_end16x4_sse2 edi, ecx, xmm0, a

	; bottom
	movdqa xmm1, [eax] 		; last line of picture pData
	mov_line_16x4_sse2 ebp, ecx, xmm1, a	; dst, stride, xmm?
	mov_line_16x4_sse2 ebp, ecx, xmm1, a
	mov_line_16x4_sse2 ebp, ecx, xmm1, a
	mov_line_end16x4_sse2 ebp, ecx, xmm1, a

	lea esi, [esi+16]		; top pSrc
	lea edi, [edi+16]		; top dst
	lea eax, [eax+16]		; bottom pSrc
	lea ebp, [ebp+16]		; bottom dst
	neg ecx 			; positive/negative stride need for next loop?

	dec ebx
	jnz near .top_bottom_loops

	; for remaining 8 bytes
	and edx, 0fh		; any 8 bytes left?
	test edx, edx
	jz near .to_be_continued	; no left to exit here

	; top
	movq mm0, [esi]		; remained 8 byte
	mov_line_8x4_mmx edi, ecx, mm0	; dst, stride, mm?
	mov_line_8x4_mmx edi, ecx, mm0	; dst, stride, mm?
	mov_line_8x4_mmx edi, ecx, mm0	; dst, stride, mm?
	mov_line_end8x4_mmx edi, ecx, mm0	; dst, stride, mm?
	; bottom
	movq mm1, [eax]
	mov_line_8x4_mmx ebp, ecx, mm1	; dst, stride, mm?
	mov_line_8x4_mmx ebp, ecx, mm1	; dst, stride, mm?
	mov_line_8x4_mmx ebp, ecx, mm1	; dst, stride, mm?
	mov_line_end8x4_mmx ebp, ecx, mm1	; dst, stride, mm?
	WELSEMMS

.to_be_continued:
%endif
%endmacro

%macro exp_left_right_sse2	2	; iPaddingSize [luma(32)/chroma(16)], u/a
	; ecx [height]
	; esi [pSrc+0], 	   edi [pSrc-32], edx [stride], 32(16)	; left
	; ebx [pSrc+(w-1)], ebp [pSrc+w], 32(16)			; right
;	xor eax, eax 	; for pixel pData (uint8_t)		; make sure eax=0 at least high 24 bits of eax = 0

%if %1 == 32		; for luma
.left_right_loops:
	; left
	mov al, byte [esi]		; pixel pData for left border
	butterfly_1to16_sse	xmm0, xmm1, a				; dst, tmp, pSrc [generic register name: a/b/c/d]
	movdqa [edi], xmm0
	movdqa [edi+16], xmm0

	; right
	mov al, byte [ebx]
	butterfly_1to16_sse	xmm1, xmm2, a				; dst, tmp, pSrc [generic register name: a/b/c/d]
	movdqa [ebp], xmm1
	movdqa [ebp+16], xmm1

	lea esi, [esi+edx]		; left pSrc
	lea edi, [edi+edx]		; left dst
	lea ebx, [ebx+edx]		; right pSrc
	lea ebp, [ebp+edx]		; right dst

	dec ecx
	jnz near .left_right_loops
%elif %1 == 16	; for chroma ??
.left_right_loops:
	; left
	mov al, byte [esi]		; pixel pData for left border
	butterfly_1to16_sse	xmm0, xmm1, a				; dst, tmp, pSrc [generic register name: a/b/c/d]
	movdqa [edi], xmm0

	; right
	mov al, byte [ebx]
	butterfly_1to16_sse	xmm1, xmm2, a				; dst, tmp, pSrc [generic register name: a/b/c/d]
	movdq%2 [ebp], xmm1								; might not be aligned 16 bytes in case chroma planes

	lea esi, [esi+edx]		; left pSrc
	lea edi, [edi+edx]		; left dst
	lea ebx, [ebx+edx]		; right pSrc
	lea ebp, [ebp+edx]		; right dst

	dec ecx
	jnz near .left_right_loops
%endif
%endmacro

%macro exp_cross_sse2	2	; iPaddingSize [luma(32)/chroma(16)], u/a
	; top-left: (x)mm3, top-right: (x)mm4, bottom-left: (x)mm5, bottom-right: (x)mm6
	; edi: TL, ebp: TR, eax: BL, ebx: BR, ecx, -stride
%if %1 == 32		; luma
	; TL
	mov_line_32x4_sse2	edi, ecx, xmm3	; dst, stride, xmm?
	mov_line_32x4_sse2	edi, ecx, xmm3	; dst, stride, xmm?
	mov_line_32x4_sse2	edi, ecx, xmm3	; dst, stride, xmm?
	mov_line_32x4_sse2	edi, ecx, xmm3	; dst, stride, xmm?
	mov_line_32x4_sse2	edi, ecx, xmm3	; dst, stride, xmm?
	mov_line_32x4_sse2	edi, ecx, xmm3	; dst, stride, xmm?
	mov_line_32x4_sse2	edi, ecx, xmm3	; dst, stride, xmm?
	mov_line_end32x4_sse2	edi, ecx, xmm3	; dst, stride, xmm?

	; TR
	mov_line_32x4_sse2	ebp, ecx, xmm4	; dst, stride, xmm?
	mov_line_32x4_sse2	ebp, ecx, xmm4	; dst, stride, xmm?
	mov_line_32x4_sse2	ebp, ecx, xmm4	; dst, stride, xmm?
	mov_line_32x4_sse2	ebp, ecx, xmm4	; dst, stride, xmm?
	mov_line_32x4_sse2	ebp, ecx, xmm4	; dst, stride, xmm?
	mov_line_32x4_sse2	ebp, ecx, xmm4	; dst, stride, xmm?
	mov_line_32x4_sse2	ebp, ecx, xmm4	; dst, stride, xmm?
	mov_line_end32x4_sse2	ebp, ecx, xmm4	; dst, stride, xmm?

	; BL
	mov_line_32x4_sse2	eax, ecx, xmm5	; dst, stride, xmm?
	mov_line_32x4_sse2	eax, ecx, xmm5	; dst, stride, xmm?
	mov_line_32x4_sse2	eax, ecx, xmm5	; dst, stride, xmm?
	mov_line_32x4_sse2	eax, ecx, xmm5	; dst, stride, xmm?
	mov_line_32x4_sse2	eax, ecx, xmm5	; dst, stride, xmm?
	mov_line_32x4_sse2	eax, ecx, xmm5	; dst, stride, xmm?
	mov_line_32x4_sse2	eax, ecx, xmm5	; dst, stride, xmm?
	mov_line_end32x4_sse2	eax, ecx, xmm5	; dst, stride, xmm?

	; BR
	mov_line_32x4_sse2	ebx, ecx, xmm6	; dst, stride, xmm?
	mov_line_32x4_sse2	ebx, ecx, xmm6	; dst, stride, xmm?
	mov_line_32x4_sse2	ebx, ecx, xmm6	; dst, stride, xmm?
	mov_line_32x4_sse2	ebx, ecx, xmm6	; dst, stride, xmm?
	mov_line_32x4_sse2	ebx, ecx, xmm6	; dst, stride, xmm?
	mov_line_32x4_sse2	ebx, ecx, xmm6	; dst, stride, xmm?
	mov_line_32x4_sse2	ebx, ecx, xmm6	; dst, stride, xmm?
	mov_line_end32x4_sse2	ebx, ecx, xmm6	; dst, stride, xmm?
%elif %1 == 16	; chroma
	; TL
	mov_line_16x4_sse2	edi, ecx, xmm3, a	; dst, stride, xmm?
	mov_line_16x4_sse2	edi, ecx, xmm3, a	; dst, stride, xmm?
	mov_line_16x4_sse2	edi, ecx, xmm3, a	; dst, stride, xmm?
	mov_line_end16x4_sse2	edi, ecx, xmm3, a	; dst, stride, xmm?

	; TR
	mov_line_16x4_sse2	ebp, ecx, xmm4, %2	; dst, stride, xmm?
	mov_line_16x4_sse2	ebp, ecx, xmm4, %2	; dst, stride, xmm?
	mov_line_16x4_sse2	ebp, ecx, xmm4, %2	; dst, stride, xmm?
	mov_line_end16x4_sse2 ebp, ecx, xmm4, %2	; dst, stride, xmm?

	; BL
	mov_line_16x4_sse2	eax, ecx, xmm5, a	; dst, stride, xmm?
	mov_line_16x4_sse2	eax, ecx, xmm5, a	; dst, stride, xmm?
	mov_line_16x4_sse2	eax, ecx, xmm5, a	; dst, stride, xmm?
	mov_line_end16x4_sse2	eax, ecx, xmm5, a	; dst, stride, xmm?

	; BR
	mov_line_16x4_sse2	ebx, ecx, xmm6, %2	; dst, stride, xmm?
	mov_line_16x4_sse2	ebx, ecx, xmm6, %2	; dst, stride, xmm?
	mov_line_16x4_sse2	ebx, ecx, xmm6, %2	; dst, stride, xmm?
	mov_line_end16x4_sse2	ebx, ecx, xmm6, %2	; dst, stride, xmm?
%endif
%endmacro

ALIGN 16
;***********************************************************************----------------
; void ExpandPictureLuma_sse2(	uint8_t *pDst,
;									const int32_t iStride,
;									const int32_t iWidth,
;									const int32_t iHeight	);
;***********************************************************************----------------
ExpandPictureLuma_sse2:
	push ebx
	push edx
	push esi
	push edi
	push ebp

	; for both top and bottom border
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov esi, [esp+24]						; p_dst
	mov edx, [esp+28]						; stride
	mov ebx, [esp+32]						; width
	mov eax, [esp+36]						; height
	; also prepare for cross border pData top-left: xmm3
;	xor ecx, ecx
	mov cl, byte [esi]
	butterfly_1to16_sse xmm3, xmm4, c		; dst, tmp, pSrc [generic register name: a/b/c/d]
	; load top border
	mov ecx, edx							; stride
	neg ecx 								; -stride
	lea edi, [esi+ecx]						; last line of top border
	; load bottom border
	dec eax									; h-1
	imul eax, edx 							; (h-1)*stride
	lea eax, [esi+eax]						; last line of picture pData
	sal edx, 05h							; 32*stride
	lea ebp, [eax+edx]						; last line of bottom border, (h-1)*stride + 32 * stride
	; also prepare for cross border pData: bottom-left with xmm5, bottom-right xmm6
	dec ebx									; width-1
	lea ebx, [eax+ebx]						; dst[w-1][h-1]
;	xor edx, edx
	mov dl, byte [eax]						; bottom-left
	butterfly_1to16_sse xmm5, xmm6, d		; dst, tmp, pSrc [generic register name: a/b/c/d]
	mov dl, byte [ebx]						; bottom-right
	butterfly_1to16_sse xmm6, xmm4, d		; dst, tmp, pSrc [generic register name: a/b/c/d]
	; for top & bottom expanding
	mov ebx, [esp+32]						; width
	exp_top_bottom_sse2	32

	; for both left and right border
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov esi, [esp+24]						; p_dst: left border pSrc
	mov edx, [esp+28]						; stride
	mov ebx, [esp+32]						; width
	mov ecx, [esp+36]						; height
	; load left border
	mov eax, -32 							; luma=-32, chroma=-16
	lea edi, [esi+eax]						; left border dst
	dec ebx
	lea ebx, [esi+ebx]						; right border pSrc, (p_dst + width - 1)
	lea ebp, [ebx+1]						; right border dst
	; prepare for cross border pData: top-right with xmm4
;	xor eax, eax
	mov al, byte [ebx]						; top-right
	butterfly_1to16_sse xmm4, xmm0, a		; dst, tmp, pSrc [generic register name: a/b/c/d]
	; for left & right border expanding
	exp_left_right_sse2	32, a

	; for cross border [top-left, top-right, bottom-left, bottom-right]
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov esi, [esp+24]						; p_dst
	mov ecx, [esp+28]						; stride
	mov ebx, [esp+32]						; width
	mov edx, [esp+36]						; height
	; have done xmm3,..,xmm6 cross pData initialization above, perform pading as below, To be continued..
	mov eax, -32							; luma=-32, chroma=-16
	neg ecx										; -stride
	lea edi, [esi+eax]
	lea edi, [edi+ecx]				; last line of top-left border
	lea ebp, [esi+ebx]
	lea ebp, [ebp+ecx]				; last line of top-right border
	add edx, 32								; height+32(16), luma=32, chroma=16
	mov ecx, [esp+28]					; stride
	imul edx, ecx							; (height+32(16)) * stride
	lea eax, [edi+edx]						; last line of bottom-left border
	lea ebx, [ebp+edx]						; last line of bottom-right border
	neg ecx										; -stride
	; for left & right border expanding
	exp_cross_sse2		32, a

;	sfence									; commit cache write back memory

	pop ebp
	pop edi
	pop esi
	pop edx
	pop ebx

	ret

ALIGN 16
;***********************************************************************----------------
; void ExpandPictureChromaAlign_sse2(	uint8_t *pDst,
;										const int32_t iStride,
;										const int32_t iWidth,
;										const int32_t iHeight	);
;***********************************************************************----------------
ExpandPictureChromaAlign_sse2:
	push ebx
	push edx
	push esi
	push edi
	push ebp

	; for both top and bottom border
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov esi, [esp+24]						; p_dst
	mov edx, [esp+28]						; stride
	mov ebx, [esp+32]						; width
	mov eax, [esp+36]						; height
	; also prepare for cross border pData top-left: xmm3
;	xor ecx, ecx
	mov cl, byte [esi]
	butterfly_1to16_sse xmm3, xmm4, c		; dst, tmp, pSrc [generic register name: a/b/c/d]
	; load top border
	mov ecx, edx							; stride
	neg ecx 								; -stride
	lea edi, [esi+ecx]						; last line of top border
	; load bottom border
	dec eax									; h-1
	imul eax, edx 							; (h-1)*stride
	lea eax, [esi+eax]						; last line of picture pData
	sal edx, 04h							; 16*stride
	lea ebp, [eax+edx]						; last line of bottom border, (h-1)*stride + 16 * stride
	; also prepare for cross border pData: bottom-left with xmm5, bottom-right xmm6
	dec ebx									; width-1
	lea ebx, [eax+ebx]						; dst[w-1][h-1]
;	xor edx, edx
	mov dl, byte [eax]						; bottom-left
	butterfly_1to16_sse xmm5, xmm6, d		; dst, tmp, pSrc [generic register name: a/b/c/d]
	mov dl, byte [ebx]						; bottom-right
	butterfly_1to16_sse xmm6, xmm4, d		; dst, tmp, pSrc [generic register name: a/b/c/d]
	; for top & bottom expanding
	mov ebx, [esp+32]						; width
	exp_top_bottom_sse2	16

	; for both left and right border
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov esi, [esp+24]						; p_dst: left border pSrc
	mov edx, [esp+28]						; stride
	mov ebx, [esp+32]						; width
	mov ecx, [esp+36]						; height
	; load left border
	mov eax, -16 							; luma=-32, chroma=-16
	lea edi, [esi+eax]						; left border dst
	dec ebx
	lea ebx, [esi+ebx]						; right border pSrc, (p_dst + width - 1)
	lea ebp, [ebx+1]						; right border dst
	; prepare for cross border pData: top-right with xmm4
;	xor eax, eax
	mov al, byte [ebx]						; top-right
	butterfly_1to16_sse xmm4, xmm0, a		; dst, tmp, pSrc [generic register name: a/b/c/d]
	; for left & right border expanding
	exp_left_right_sse2	16, a

	; for cross border [top-left, top-right, bottom-left, bottom-right]
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov esi, [esp+24]						; p_dst
	mov ecx, [esp+28]						; stride
	mov ebx, [esp+32]						; width
	mov edx, [esp+36]						; height
	; have done xmm3,..,xmm6 cross pData initialization above, perform pading as below, To be continued..
	mov eax, -16							; chroma=-16
	neg ecx										; -stride
	lea edi, [esi+eax]
	lea edi, [edi+ecx]				; last line of top-left border
	lea ebp, [esi+ebx]
	lea ebp, [ebp+ecx]				; last line of top-right border
	mov ecx, [esp+28]						; stride
	add edx, 16							; height+16, luma=32, chroma=16
	imul edx, ecx							; (height+16) * stride
	lea eax, [edi+edx]						; last line of bottom-left border
	lea ebx, [ebp+edx]						; last line of bottom-right border
	neg ecx										; -stride
	; for left & right border expanding
	exp_cross_sse2		16, a

;	sfence									; commit cache write back memory

	pop ebp
	pop edi
	pop esi
	pop edx
	pop ebx

	ret

ALIGN 16
;***********************************************************************----------------
; void ExpandPictureChromaUnalign_sse2(	uint8_t *pDst,
;										const int32_t iStride,
;										const int32_t iWidth,
;										const int32_t iHeight	);
;***********************************************************************----------------
ExpandPictureChromaUnalign_sse2:
	push ebx
	push edx
	push esi
	push edi
	push ebp

	; for both top and bottom border
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov esi, [esp+24]						; p_dst
	mov edx, [esp+28]						; stride
	mov ebx, [esp+32]						; width
	mov eax, [esp+36]						; height
	; also prepare for cross border pData top-left: xmm3
;	xor ecx, ecx
	mov cl, byte [esi]
	butterfly_1to16_sse xmm3, xmm4, c		; dst, tmp, pSrc [generic register name: a/b/c/d]
	; load top border
	mov ecx, edx							; stride
	neg ecx 								; -stride
	lea edi, [esi+ecx]						; last line of top border
	; load bottom border
	dec eax									; h-1
	imul eax, edx 							; (h-1)*stride
	lea eax, [esi+eax]						; last line of picture pData
	sal edx, 04h							; 16*stride
	lea ebp, [eax+edx]						; last line of bottom border, (h-1)*stride + 16 * stride
	; also prepare for cross border pData: bottom-left with xmm5, bottom-right xmm6
	dec ebx									; width-1
	lea ebx, [eax+ebx]						; dst[w-1][h-1]
;	xor edx, edx
	mov dl, byte [eax]						; bottom-left
	butterfly_1to16_sse xmm5, xmm6, d		; dst, tmp, pSrc [generic register name: a/b/c/d]
	mov dl, byte [ebx]						; bottom-right
	butterfly_1to16_sse xmm6, xmm4, d		; dst, tmp, pSrc [generic register name: a/b/c/d]
	; for top & bottom expanding
	mov ebx, [esp+32]						; width
	exp_top_bottom_sse2	16

	; for both left and right border
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov esi, [esp+24]						; p_dst: left border pSrc
	mov edx, [esp+28]						; stride
	mov ebx, [esp+32]						; width
	mov ecx, [esp+36]						; height
	; load left border
	mov eax, -16 							; luma=-32, chroma=-16
	lea edi, [esi+eax]						; left border dst
	dec ebx
	lea ebx, [esi+ebx]						; right border pSrc, (p_dst + width - 1)
	lea ebp, [ebx+1]						; right border dst
	; prepare for cross border pData: top-right with xmm4
;	xor eax, eax
	mov al, byte [ebx]						; top-right
	butterfly_1to16_sse xmm4, xmm0, a		; dst, tmp, pSrc [generic register name: a/b/c/d]
	; for left & right border expanding
	exp_left_right_sse2	16, u

	; for cross border [top-left, top-right, bottom-left, bottom-right]
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	mov esi, [esp+24]						; p_dst
	mov ecx, [esp+28]						; stride
	mov ebx, [esp+32]						; width
	mov edx, [esp+36]						; height
	; have done xmm3,..,xmm6 cross pData initialization above, perform pading as below, To be continued..
	neg ecx									; -stride
	mov eax, -16							; chroma=-16
	lea edi, [esi+eax]
	lea edi, [edi+ecx]				; last line of top-left border
	lea ebp, [esi+ebx]
	lea ebp, [ebp+ecx]				; last line of top-right border
	mov ecx, [esp+28]						; stride
	add edx, 16							; height+16, luma=32, chroma=16
	imul edx, ecx							; (height+16) * stride
	lea eax, [edi+edx]						; last line of bottom-left border
	lea ebx, [ebp+edx]						; last line of bottom-right border
	neg ecx									; -stride
	; for left & right border expanding
	exp_cross_sse2		16, u

;	sfence									; commit cache write back memory

	pop ebp
	pop edi
	pop esi
	pop edx
	pop ebx

	ret

