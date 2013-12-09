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
;*  score.asm
;*
;*  Abstract
;*      scan/score/count of sse2
;*
;*  History
;*      8/21/2009 Created
;*
;*
;*************************************************************************/

%include "asm_inc.asm"

bits 32

;***********************************************************************
; Macros 
;***********************************************************************

;***********************************************************************
; Local Data (Read Only)
;***********************************************************************
SECTION .rodata align=16

;align 16
;se2_2 dw 2, 2, 2, 2, 2, 2, 2, 2
align 16
sse2_1: dw 1, 1, 1, 1, 1, 1, 1, 1
align 16
sse2_b1: db 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
i_ds_table: db 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
align 16
sse2_plane_inc_minus: dw -7, -6, -5, -4, -3, -2, -1, 0
align 16
sse2_plane_inc: dw 1, 2, 3, 4, 5, 6, 7, 8
align 16
sse2_plane_dec: dw 8, 7, 6, 5, 4, 3, 2, 1
align 16
pb_scanacdc_maska:db 0,1,2,3,8,9,14,15,10,11,4,5,6,7,12,13
align 16
pb_scanacdc_maskb:db 2,3,8,9,10,11,4,5,0,1,6,7,12,13,14,15
align 16
pb_scandc_maska:db 2,3,8,9,14,15,10,11,4,5,6,7,12,13,0,1
align 16
pb_scandc_maskb:db 8,9,10,11,4,5,0,1,6,7,12,13,14,15,128,128

align 16
nozero_count_table:
db  0,1,1,2,1,2,2,3,1,2
db  2,3,2,3,3,4,1,2,2,3
db  2,3,3,4,2,3,3,4,3,4
db  4,5,1,2,2,3,2,3,3,4
db  2,3,3,4,3,4,4,5,2,3
db  3,4,3,4,4,5,3,4,4,5
db  4,5,5,6,1,2,2,3,2,3
db  3,4,2,3,3,4,3,4,4,5
db  2,3,3,4,3,4,4,5,3,4
db  4,5,4,5,5,6,2,3,3,4
db  3,4,4,5,3,4,4,5,4,5
db  5,6,3,4,4,5,4,5,5,6
db  4,5,5,6,5,6,6,7,1,2
db  2,3,2,3,3,4,2,3,3,4
db  3,4,4,5,2,3,3,4,3,4
db  4,5,3,4,4,5,4,5,5,6
db  2,3,3,4,3,4,4,5,3,4
db  4,5,4,5,5,6,3,4,4,5
db  4,5,5,6,4,5,5,6,5,6
db  6,7,2,3,3,4,3,4,4,5
db  3,4,4,5,4,5,5,6,3,4
db  4,5,4,5,5,6,4,5,5,6
db  5,6,6,7,3,4,4,5,4,5
db  5,6,4,5,5,6,5,6,6,7
db  4,5,5,6,5,6,6,7,5,6
db  6,7,6,7,7,8

align 16
high_mask_table:
	db  0, 0, 0, 3, 0, 2, 3, 6, 0, 2
	db  2, 5, 3, 5, 6, 9, 0, 1, 2, 5
	db  2, 4, 5, 8, 3, 5, 5, 8, 6, 8
	db  9,12, 0, 1, 1, 4, 2, 4, 5, 8
	db  2, 4, 4, 7, 5, 7, 8,11, 3, 4
	db  5, 8, 5, 7, 8,11, 6, 8, 8,11
	db  9,11,12,15, 0, 1, 1, 4, 1, 3
	db  4, 7, 2, 4, 4, 7, 5, 7, 8,11
	db  2, 3, 4, 7, 4, 6, 7,10, 5, 7
	db  7,10, 8,10,11,14, 3, 4, 4, 7
	db  5, 7, 8,11, 5, 7, 7,10, 8,10
	db 11,14, 6, 7, 8,11, 8,10,11,14
	db  9,11,11,14,12,14,15,18, 0, 0
	db  1, 4, 1, 3, 4, 7, 1, 3, 3, 6
	db  4, 6, 7,10, 2, 3, 4, 7, 4, 6
	db  7,10, 5, 7, 7,10, 8,10,11,14
	db  2, 3, 3, 6, 4, 6, 7,10, 4, 6
	db  6, 9, 7, 9,10,13, 5, 6, 7,10
	db  7, 9,10,13, 8,10,10,13,11,13
	db 14,17, 3, 4, 4, 7, 4, 6, 7,10
	db  5, 7, 7,10, 8,10,11,14, 5, 6
	db  7,10, 7, 9,10,13, 8,10,10,13
	db 11,13,14,17, 6, 7, 7,10, 8,10
	db 11,14, 8,10,10,13,11,13,14,17
	db  9,10,11,14,11,13,14,17,12,14
	db 14,17,15,17,18,21

align 16
low_mask_table:
    db  0, 3, 2, 6, 2, 5, 5, 9, 1, 5
    db  4, 8, 5, 8, 8,12, 1, 4, 4, 8
    db  4, 7, 7,11, 4, 8, 7,11, 8,11
    db 11,15, 1, 4, 3, 7, 4, 7, 7,11
    db  3, 7, 6,10, 7,10,10,14, 4, 7 
    db  7,11, 7,10,10,14, 7,11,10,14
    db 11,14,14,18, 0, 4, 3, 7, 3, 6
    db  6,10, 3, 7, 6,10, 7,10,10,14
    db  3, 6, 6,10, 6, 9, 9,13, 6,10
    db  9,13,10,13,13,17, 4, 7, 6,10
    db  7,10,10,14, 6,10, 9,13,10,13
    db 13,17, 7,10,10,14,10,13,13,17
    db 10,14,13,17,14,17,17,21, 0, 3
    db  3, 7, 3, 6, 6,10, 2, 6, 5, 9
    db  6, 9, 9,13, 3, 6, 6,10, 6, 9
    db  9,13, 6,10, 9,13,10,13,13,17
    db  3, 6, 5, 9, 6, 9, 9,13, 5, 9
    db  8,12, 9,12,12,16, 6, 9, 9,13
    db  9,12,12,16, 9,13,12,16,13,16
    db 16,20, 3, 7, 6,10, 6, 9, 9,13
    db  6,10, 9,13,10,13,13,17, 6, 9
    db  9,13, 9,12,12,16, 9,13,12,16
    db 13,16,16,20, 7,10, 9,13,10,13
    db 13,17, 9,13,12,16,13,16,16,20
    db 10,13,13,17,13,16,16,20,13,17
    db 16,20,17,20,20,24


SECTION .text

;***********************************************************************
;void WelsScan4x4DcAc_sse2( int16_t level[16], int16_t *pDct )
;***********************************************************************
ALIGN 16
WELS_EXTERN WelsScan4x4DcAc_sse2
WelsScan4x4DcAc_sse2:

	mov        eax, [esp+8]
	movdqa     xmm0, [eax]			; 7 6 5 4 3 2 1 0
	movdqa     xmm1, [eax+16]		; f e d c b a 9 8
	pextrw     ecx, xmm0, 7			; ecx = 7
	pextrw     edx, xmm1, 2			; edx = a
	pextrw     eax, xmm0, 5			; eax = 5
	pinsrw     xmm1, ecx, 2			; f e d c b 7 9 8
	pinsrw     xmm0, eax, 7			; 5 6 5 4 3 2 1 0
	pextrw     ecx, xmm1, 0			; ecx = 8
	pinsrw     xmm0, ecx, 5			; 5 6 8 4 3 2 1 0
	pinsrw     xmm1, edx, 0			; f e d c b 7 9 a
	pshufd     xmm2, xmm0, 0xd8		; 5 6 3 2 8 4 1 0
	pshufd     xmm3, xmm1, 0xd8		; f e b 7 d c 9 a
	pshufhw    xmm0, xmm2, 0x93		; 6 3 2 5 8 4 1 0
	pshuflw    xmm1, xmm3, 0x39		; f e b 7 a d c 9
	mov        eax,  [esp+4]
	movdqa     [eax],xmm0
	movdqa     [eax+16], xmm1
	ret
	
;***********************************************************************
;void WelsScan4x4DcAc_ssse3( int16_t level[16], int16_t *pDct )
;***********************************************************************
ALIGN 16
WELS_EXTERN WelsScan4x4DcAc_ssse3
WelsScan4x4DcAc_ssse3:
	mov        eax, [esp+8]
	movdqa     xmm0, [eax]
	movdqa     xmm1, [eax+16]
	pextrw		ecx,  xmm0, 7			; ecx = [7]
	pextrw		eax,  xmm1, 0			; eax = [8]
	pinsrw		xmm0, eax, 7			; xmm0[7]	=	[8]
	pinsrw		xmm1, ecx, 0			; xmm1[0]	=	[7]
	pshufb		xmm1, [pb_scanacdc_maskb]
	pshufb		xmm0, [pb_scanacdc_maska]	

	mov        eax,  [esp+4]
	movdqa     [eax],xmm0
	movdqa     [eax+16], xmm1
	ret
;***********************************************************************
;void WelsScan4x4Ac_sse2( int16_t* zig_value, int16_t* pDct )
;***********************************************************************
ALIGN 16
WELS_EXTERN WelsScan4x4Ac_sse2
WelsScan4x4Ac_sse2:
	mov        eax, [esp+8]
	movdqa     xmm0, [eax]
	movdqa     xmm1, [eax+16]
	movdqa     xmm2, xmm0
	punpcklqdq xmm0, xmm1
	punpckhqdq xmm2, xmm1
	
	movdqa     xmm3, xmm0
	punpckldq  xmm0, xmm2
	punpckhdq  xmm3, xmm2
	pextrw     eax , xmm0, 3
	pextrw     edx , xmm0, 7
	pinsrw     xmm0, eax,  7
	pextrw     eax,  xmm3, 4
	pinsrw     xmm3, edx,  4
	pextrw     edx,  xmm3, 0
	pinsrw     xmm3, eax,  0
	pinsrw     xmm0, edx,  3
	
	pshufhw    xmm1, xmm0, 0x93
	pshuflw    xmm2, xmm3, 0x39
    
    movdqa     xmm3, xmm2
    psrldq     xmm1, 2
    pslldq     xmm3, 14
    por        xmm1, xmm3
    psrldq     xmm2, 2
	mov        eax,  [esp+4]
	movdqa     [eax],xmm1
	movdqa     [eax+16], xmm2
	ret


;***********************************************************************
;void int32_t WelsCalculateSingleCtr4x4_sse2( int16_t *pDct );
;***********************************************************************
ALIGN 16
WELS_EXTERN WelsCalculateSingleCtr4x4_sse2 
WelsCalculateSingleCtr4x4_sse2:
	push      ebx
	mov       eax,  [esp+8]
	movdqa    xmm0, [eax]
	movdqa    xmm1, [eax+16]
	
	packsswb  xmm0, xmm1

    pxor      xmm3, xmm3
    pcmpeqb   xmm0, xmm3
    pmovmskb  edx,  xmm0

    xor       edx,  0xffff

	xor       eax,  eax
	mov       ecx,  7
	mov       ebx,  8
.loop_low8_find1:
	bt        edx,  ecx
	jc        .loop_high8_find1
	loop      .loop_low8_find1
.loop_high8_find1:
	bt        edx, ebx
	jc        .find1end
	inc       ebx
	cmp       ebx,16
	jb        .loop_high8_find1
.find1end:
	sub       ebx, ecx
	sub       ebx, 1
	add       al,  [i_ds_table+ebx]
	mov       ebx, edx
	and       edx, 0xff
	shr       ebx, 8
	and       ebx, 0xff
	add       al,  [low_mask_table +edx]
	add       al,  [high_mask_table+ebx]

	pop       ebx
	ret


;***********************************************************************
; int32_t WelsGetNoneZeroCount_sse2(int16_t* level);
;***********************************************************************
ALIGN 16
WELS_EXTERN WelsGetNoneZeroCount_sse2
WelsGetNoneZeroCount_sse2:
	mov       eax,  [esp+4]
	movdqa    xmm0, [eax]
	movdqa    xmm1, [eax+16]
	pxor      xmm2, xmm2
	pcmpeqw   xmm0, xmm2
	pcmpeqw   xmm1, xmm2
	packsswb  xmm1, xmm0
	pmovmskb  edx,  xmm1
	xor       edx,  0xffff
	mov       ecx,  edx
	and       edx,  0xff
	shr       ecx,  8
;	and       ecx,  0xff	; we do not need this due to high 16bits equal to 0 yet
	xor       eax,  eax	
	add       al,  [nozero_count_table+ecx]
	add       al,  [nozero_count_table+edx]
	ret

