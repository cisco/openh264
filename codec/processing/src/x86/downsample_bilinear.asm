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
;*  upsampling.asm
;*
;*  Abstract
;*      SIMD for pixel domain down sampling
;*
;*  History
;*      10/22/2009  Created
;*
;*************************************************************************/
%include "asm_inc.asm"
%ifdef X86_32
;***********************************************************************
; Macros and other preprocessor constants
;***********************************************************************


;***********************************************************************
; Some constants
;***********************************************************************

;***********************************************************************
; Local Data (Read Only)
;***********************************************************************

SECTION .rodata align=16

;***********************************************************************
; Various memory constants (trigonometric values or rounding values)
;***********************************************************************

ALIGN 16
shufb_mask_low:
    db 00h, 80h, 02h, 80h, 04h, 80h, 06h, 80h, 08h, 80h, 0ah, 80h, 0ch, 80h, 0eh, 80h
shufb_mask_high:
    db 01h, 80h, 03h, 80h, 05h, 80h, 07h, 80h, 09h, 80h, 0bh, 80h, 0dh, 80h, 0fh, 80h


;***********************************************************************
; Code
;***********************************************************************

SECTION .text

;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx32_sse( unsigned char* pDst, const int iDstStride,
;                   unsigned char* pSrc, const int iSrcStride,
;                   const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx32_sse
    push ebx
    push edx
    push esi
    push edi
    push ebp

    mov edi, [esp+24]   ; pDst
    mov edx, [esp+28]   ; iDstStride
    mov esi, [esp+32]   ; pSrc
    mov ecx, [esp+36]   ; iSrcStride
    mov ebp, [esp+44]   ; iSrcHeight

    sar ebp, $01            ; iSrcHeight >> 1

.yloops:
    mov eax, [esp+40]   ; iSrcWidth
    sar eax, $01            ; iSrcWidth >> 1
    mov ebx, eax        ; iDstWidth restored at ebx
    sar eax, $04            ; (iSrcWidth >> 1) / 16     ; loop count = num_of_mb
    neg ebx             ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 32 bytes
.xloops:
    ; 1st part horizonal loop: x16 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  mm0: d D c C b B a A    mm1: h H g G f F e E
    ;2nd Line Src:  mm2: l L k K j J i I    mm3: p P o O n N m M
    ;=> target:
    ;: H G F E D C B A, P O N M L K J I
    ;: h g f e d c b a, p o n m l k j i
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movq mm0, [esi]         ; 1st pSrc line
    movq mm1, [esi+8]       ; 1st pSrc line + 8
    movq mm2, [esi+ecx]     ; 2nd pSrc line
    movq mm3, [esi+ecx+8]   ; 2nd pSrc line + 8

    ; to handle mm0, mm1, mm2, mm3
    pshufw mm4, mm0, 0d8h   ; d D b B c C a A ; 11011000 B
    pshufw mm5, mm4, 04eh   ; c C a A d D b B ; 01001110 B
    punpcklbw mm4, mm5      ; d c D C b a B A
    pshufw mm4, mm4, 0d8h   ; d c b a D C B A ; 11011000 B: mm4

    pshufw mm5, mm1, 0d8h   ; h H f F g G e E ; 11011000 B
    pshufw mm6, mm5, 04eh   ; g G e E h H f F ; 01001110 B
    punpcklbw mm5, mm6      ; h g H G f e F E
    pshufw mm5, mm5, 0d8h   ; h g f e H G F E ; 11011000 B: mm5

    pshufw mm6, mm2, 0d8h   ; l L j J k K i I ; 11011000 B
    pshufw mm7, mm6, 04eh   ; k K i I l L j J ; 01001110 B
    punpcklbw mm6, mm7      ; l k L K j i J I
    pshufw mm6, mm6, 0d8h   ; l k j i L K J I ; 11011000 B: mm6

    pshufw mm7, mm3, 0d8h   ; p P n N o O m M ; 11011000 B
    pshufw mm0, mm7, 04eh   ; o O m M p P n N ; 01001110 B
    punpcklbw mm7, mm0      ; p o P O n m N M
    pshufw mm7, mm7, 0d8h   ; p o n m P O N M ; 11011000 B: mm7

    ; to handle mm4, mm5, mm6, mm7
    movq mm0, mm4       ;
    punpckldq mm0, mm5  ; H G F E D C B A
    punpckhdq mm4, mm5  ; h g f e d c b a

    movq mm1, mm6
    punpckldq mm1, mm7  ; P O N M L K J I
    punpckhdq mm6, mm7  ; p o n m l k j i

    ; avg within MB horizon width (16 x 2 lines)
    pavgb mm0, mm4      ; (A+a+1)>>1, .., (H+h+1)>>1, temp_row1
    pavgb mm1, mm6      ; (I+i+1)>>1, .., (P+p+1)>>1, temp_row2
    pavgb mm0, mm1      ; (temp_row1+temp_row2+1)>>1, pending here and wait another horizonal part done then write memory once

    ; 2nd part horizonal loop: x16 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  mm0: d D c C b B a A    mm1: h H g G f F e E
    ;2nd Line Src:  mm2: l L k K j J i I    mm3: p P o O n N m M
    ;=> target:
    ;: H G F E D C B A, P O N M L K J I
    ;: h g f e d c b a, p o n m l k j i
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movq mm1, [esi+16]      ; 1st pSrc line + 16
    movq mm2, [esi+24]      ; 1st pSrc line + 24
    movq mm3, [esi+ecx+16]  ; 2nd pSrc line + 16
    movq mm4, [esi+ecx+24]  ; 2nd pSrc line + 24

    ; to handle mm1, mm2, mm3, mm4
    pshufw mm5, mm1, 0d8h   ; d D b B c C a A ; 11011000 B
    pshufw mm6, mm5, 04eh   ; c C a A d D b B ; 01001110 B
    punpcklbw mm5, mm6      ; d c D C b a B A
    pshufw mm5, mm5, 0d8h   ; d c b a D C B A ; 11011000 B: mm5

    pshufw mm6, mm2, 0d8h   ; h H f F g G e E ; 11011000 B
    pshufw mm7, mm6, 04eh   ; g G e E h H f F ; 01001110 B
    punpcklbw mm6, mm7      ; h g H G f e F E
    pshufw mm6, mm6, 0d8h   ; h g f e H G F E ; 11011000 B: mm6

    pshufw mm7, mm3, 0d8h   ; l L j J k K i I ; 11011000 B
    pshufw mm1, mm7, 04eh   ; k K i I l L j J ; 01001110 B
    punpcklbw mm7, mm1      ; l k L K j i J I
    pshufw mm7, mm7, 0d8h   ; l k j i L K J I ; 11011000 B: mm7

    pshufw mm1, mm4, 0d8h   ; p P n N o O m M ; 11011000 B
    pshufw mm2, mm1, 04eh   ; o O m M p P n N ; 01001110 B
    punpcklbw mm1, mm2      ; p o P O n m N M
    pshufw mm1, mm1, 0d8h   ; p o n m P O N M ; 11011000 B: mm1

    ; to handle mm5, mm6, mm7, mm1
    movq mm2, mm5
    punpckldq mm2, mm6  ; H G F E D C B A
    punpckhdq mm5, mm6  ; h g f e d c b a

    movq mm3, mm7
    punpckldq mm3, mm1  ; P O N M L K J I
    punpckhdq mm7, mm1  ; p o n m l k j i

    ; avg within MB horizon width (16 x 2 lines)
    pavgb mm2, mm5      ; (A+a+1)>>1, .., (H+h+1)>>1, temp_row1
    pavgb mm3, mm7      ; (I+i+1)>>1, .., (P+p+1)>>1, temp_row2
    pavgb mm2, mm3      ; (temp_row1+temp_row2+1)>>1, done in another 2nd horizonal part

    movq [edi  ], mm0
    movq [edi+8], mm2

    ; next SMB
    lea esi, [esi+32]
    lea edi, [edi+16]

    dec eax
    jg near .xloops

    ; next line
    lea esi, [esi+2*ecx]    ; next end of lines
    lea esi, [esi+2*ebx]    ; reset to base 0 [- 2 * iDstWidth]
    lea edi, [edi+edx]
    lea edi, [edi+ebx]      ; reset to base 0 [- iDstWidth]

    dec ebp
    jg near .yloops

    WELSEMMS
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ebx
    ret

;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx16_sse( unsigned char* pDst, const int iDstStride,
;                     unsigned char* pSrc, const int iSrcStride,
;                     const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx16_sse
    push ebx
    push edx
    push esi
    push edi
    push ebp

    mov edi, [esp+24]   ; pDst
    mov edx, [esp+28]   ; iDstStride
    mov esi, [esp+32]   ; pSrc
    mov ecx, [esp+36]   ; iSrcStride
    mov ebp, [esp+44]   ; iSrcHeight

    sar ebp, $01        ; iSrcHeight >> 1

.yloops:
    mov eax, [esp+40]   ; iSrcWidth
    sar eax, $01        ; iSrcWidth >> 1
    mov ebx, eax        ; iDstWidth restored at ebx
    sar eax, $03        ; (iSrcWidth >> 1) / 8      ; loop count = num_of_mb
    neg ebx         ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 16 bytes
.xloops:
    ; 1st part horizonal loop: x16 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  mm0: d D c C b B a A    mm1: h H g G f F e E
    ;2nd Line Src:  mm2: l L k K j J i I    mm3: p P o O n N m M
    ;=> target:
    ;: H G F E D C B A, P O N M L K J I
    ;: h g f e d c b a, p o n m l k j i
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movq mm0, [esi]         ; 1st pSrc line
    movq mm1, [esi+8]       ; 1st pSrc line + 8
    movq mm2, [esi+ecx]     ; 2nd pSrc line
    movq mm3, [esi+ecx+8]   ; 2nd pSrc line + 8

    ; to handle mm0, mm1, mm2, mm3
    pshufw mm4, mm0, 0d8h   ; d D b B c C a A ; 11011000 B
    pshufw mm5, mm4, 04eh   ; c C a A d D b B ; 01001110 B
    punpcklbw mm4, mm5      ; d c D C b a B A
    pshufw mm4, mm4, 0d8h   ; d c b a D C B A ; 11011000 B: mm4

    pshufw mm5, mm1, 0d8h   ; h H f F g G e E ; 11011000 B
    pshufw mm6, mm5, 04eh   ; g G e E h H f F ; 01001110 B
    punpcklbw mm5, mm6      ; h g H G f e F E
    pshufw mm5, mm5, 0d8h   ; h g f e H G F E ; 11011000 B: mm5

    pshufw mm6, mm2, 0d8h   ; l L j J k K i I ; 11011000 B
    pshufw mm7, mm6, 04eh   ; k K i I l L j J ; 01001110 B
    punpcklbw mm6, mm7      ; l k L K j i J I
    pshufw mm6, mm6, 0d8h   ; l k j i L K J I ; 11011000 B: mm6

    pshufw mm7, mm3, 0d8h   ; p P n N o O m M ; 11011000 B
    pshufw mm0, mm7, 04eh   ; o O m M p P n N ; 01001110 B
    punpcklbw mm7, mm0      ; p o P O n m N M
    pshufw mm7, mm7, 0d8h   ; p o n m P O N M ; 11011000 B: mm7

    ; to handle mm4, mm5, mm6, mm7
    movq mm0, mm4       ;
    punpckldq mm0, mm5  ; H G F E D C B A
    punpckhdq mm4, mm5  ; h g f e d c b a

    movq mm1, mm6
    punpckldq mm1, mm7  ; P O N M L K J I
    punpckhdq mm6, mm7  ; p o n m l k j i

    ; avg within MB horizon width (16 x 2 lines)
    pavgb mm0, mm4      ; (A+a+1)>>1, .., (H+h+1)>>1, temp_row1
    pavgb mm1, mm6      ; (I+i+1)>>1, .., (P+p+1)>>1, temp_row2
    pavgb mm0, mm1      ; (temp_row1+temp_row2+1)>>1, pending here and wait another horizonal part done then write memory once

    movq [edi  ], mm0

    ; next SMB
    lea esi, [esi+16]
    lea edi, [edi+8]

    dec eax
    jg near .xloops

    ; next line
    lea esi, [esi+2*ecx]    ; next end of lines
    lea esi, [esi+2*ebx]    ; reset to base 0 [- 2 * iDstWidth]
    lea edi, [edi+edx]
    lea edi, [edi+ebx]      ; reset to base 0 [- iDstWidth]

    dec ebp
    jg near .yloops

    WELSEMMS
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ebx
    ret

;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx8_sse( unsigned char* pDst, const int iDstStride,
;                     unsigned char* pSrc, const int iSrcStride,
;                     const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx8_sse
    push ebx
    push edx
    push esi
    push edi
    push ebp

    mov edi, [esp+24]   ; pDst
    mov edx, [esp+28]   ; iDstStride
    mov esi, [esp+32]   ; pSrc
    mov ecx, [esp+36]   ; iSrcStride
    mov ebp, [esp+44]   ; iSrcHeight

    sar ebp, $01        ; iSrcHeight >> 1

.yloops:
    mov eax, [esp+40]   ; iSrcWidth
    sar eax, $01        ; iSrcWidth >> 1
    mov ebx, eax        ; iDstWidth restored at ebx
    sar eax, $02        ; (iSrcWidth >> 1) / 4      ; loop count = num_of_mb
    neg ebx         ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 8 bytes
.xloops:
    ; 1st part horizonal loop: x8 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  mm0: d D c C b B a A
    ;2nd Line Src:  mm1: h H g G f F e E
    ;=> target:
    ;: H G F E D C B A
    ;: h g f e d c b a
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movq mm0, [esi]         ; 1st pSrc line
    movq mm1, [esi+ecx]     ; 2nd pSrc line

    ; to handle mm0, mm1, mm2, mm3
    pshufw mm2, mm0, 0d8h   ; d D b B c C a A ; 11011000 B
    pshufw mm3, mm2, 04eh   ; c C a A d D b B ; 01001110 B
    punpcklbw mm2, mm3      ; d c D C b a B A
    pshufw mm2, mm2, 0d8h   ; d c b a D C B A ; 11011000 B: mm4

    pshufw mm4, mm1, 0d8h   ; h H f F g G e E ; 11011000 B
    pshufw mm5, mm4, 04eh   ; g G e E h H f F ; 01001110 B
    punpcklbw mm4, mm5      ; h g H G f e F E
    pshufw mm4, mm4, 0d8h   ; h g f e H G F E ; 11011000 B: mm5

    ; to handle mm2, mm4
    movq mm0, mm2       ;
    punpckldq mm0, mm4  ; H G F E D C B A
    punpckhdq mm2, mm4  ; h g f e d c b a

    ; avg within MB horizon width (16 x 2 lines)
    pavgb mm0, mm2      ; (H+h+1)>>1, .., (A+a+1)>>1, temp_row1, 2
    pshufw mm1, mm0, 04eh   ; 01001110 B
    pavgb mm0, mm1      ; (temp_row1+temp_row2+1)>>1, pending here and wait another horizonal part done then write memory once

    movd [edi], mm0

    ; next unit
    lea esi, [esi+8]
    lea edi, [edi+4]

    dec eax
    jg near .xloops

    ; next line
    lea esi, [esi+2*ecx]    ; next end of lines
    lea esi, [esi+2*ebx]    ; reset to base 0 [- 2 * iDstWidth]
    lea edi, [edi+edx]
    lea edi, [edi+ebx]      ; reset to base 0 [- iDstWidth]

    dec ebp
    jg near .yloops

    WELSEMMS
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ebx
    ret



; got about 50% improvement over DyadicBilinearDownsamplerWidthx32_sse
;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx32_ssse3(   unsigned char* pDst, const int iDstStride,
;                   unsigned char* pSrc, const int iSrcStride,
;                   const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx32_ssse3
    push ebx
    push edx
    push esi
    push edi
    push ebp

    mov edi, [esp+24]   ; pDst
    mov edx, [esp+28]   ; iDstStride
    mov esi, [esp+32]   ; pSrc
    mov ecx, [esp+36]   ; iSrcStride
    mov ebp, [esp+44]   ; iSrcHeight

    sar ebp, $01            ; iSrcHeight >> 1

    movdqa xmm7, [shufb_mask_low]   ; mask low
    movdqa xmm6, [shufb_mask_high]  ; mask high

.yloops:
    mov eax, [esp+40]   ; iSrcWidth
    sar eax, $01            ; iSrcWidth >> 1
    mov ebx, eax        ; iDstWidth restored at ebx
    sar eax, $04            ; (iSrcWidth >> 1) / 16     ; loop count = num_of_mb
    neg ebx             ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 32 bytes
.xloops:
    ; 1st part horizonal loop: x16 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  xmm0: h H g G f F e E d D c C b B a A
    ;               xmm1: p P o O n N m M l L k K j J i I
    ;2nd Line Src:  xmm2: h H g G f F e E d D c C b B a A
    ;               xmm3: p P o O n N m M l L k K j J i I
    ;=> target:
    ;: P O N M L K J I H G F E D C B A
    ;: p o n m l k j i h g f e d c b a
    ;: P ..                          A
    ;: p ..                          a

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movdqa xmm0, [esi]          ; 1st_src_line
    movdqa xmm1, [esi+16]       ; 1st_src_line + 16
    movdqa xmm2, [esi+ecx]      ; 2nd_src_line
    movdqa xmm3, [esi+ecx+16]   ; 2nd_src_line + 16

    ; packing & avg
    movdqa xmm4, xmm0           ; h H g G f F e E d D c C b B a A
    pshufb xmm0, xmm7           ; 0 H 0 G 0 F 0 E 0 D 0 C 0 B 0 A
    pshufb xmm4, xmm6           ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
    ; another implementation for xmm4 high bits
;   psubb xmm4, xmm0            ; h 0 g 0 f 0 e 0 d 0 c 0 b 0 a 0
;   psrlw xmm4, 8               ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
    pavgb xmm0, xmm4

    movdqa xmm5, xmm1
    pshufb xmm1, xmm7
    pshufb xmm5, xmm6
;   psubb xmm5, xmm1
;   psrlw xmm5, 8
    pavgb xmm1, xmm5

    movdqa xmm4, xmm2
    pshufb xmm2, xmm7
    pshufb xmm4, xmm6
;   psubb xmm4, xmm2
;   psrlw xmm4, 8
    pavgb xmm2, xmm4

    movdqa xmm5, xmm3
    pshufb xmm3, xmm7
    pshufb xmm5, xmm6
;   psubb xmm5, xmm3
;   psrlw xmm5, 8
    pavgb xmm3, xmm5

    packuswb xmm0, xmm1
    packuswb xmm2, xmm3
    pavgb xmm0, xmm2

    ; write pDst
    movdqa [edi], xmm0

    ; next SMB
    lea esi, [esi+32]
    lea edi, [edi+16]

    dec eax
    jg near .xloops

    ; next line
    lea esi, [esi+2*ecx]    ; next end of lines
    lea esi, [esi+2*ebx]    ; reset to base 0 [- 2 * iDstWidth]
    lea edi, [edi+edx]
    lea edi, [edi+ebx]      ; reset to base 0 [- iDstWidth]

    dec ebp
    jg near .yloops

    pop ebp
    pop edi
    pop esi
    pop edx
    pop ebx
    ret

;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx16_ssse3( unsigned char* pDst, const int iDstStride,
;                     unsigned char* pSrc, const int iSrcStride,
;                     const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx16_ssse3
    push ebx
    push edx
    push esi
    push edi
    push ebp

    mov edi, [esp+24]   ; pDst
    mov edx, [esp+28]   ; iDstStride
    mov esi, [esp+32]   ; pSrc
    mov ecx, [esp+36]   ; iSrcStride
    mov ebp, [esp+44]   ; iSrcHeight

    sar ebp, $01        ; iSrcHeight >> 1
    movdqa xmm7, [shufb_mask_low]   ; mask low
    movdqa xmm6, [shufb_mask_high]  ; mask high

.yloops:
    mov eax, [esp+40]   ; iSrcWidth
    sar eax, $01        ; iSrcWidth >> 1
    mov ebx, eax        ; iDstWidth restored at ebx
    sar eax, $03        ; (iSrcWidth >> 1) / 8      ; loop count = num_of_mb
    neg ebx         ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 16 bytes
.xloops:
    ; horizonal loop: x16 bytes by source
    ;               mem  hi<-       ->lo
    ;1st line pSrc: xmm0: h H g G f F e E d D c C b B a A
    ;2nd line pSrc:  xmm1: p P o O n N m M l L k K j J i I
    ;=> target:
    ;: H G F E D C B A, P O N M L K J I
    ;: h g f e d c b a, p o n m l k j i

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movdqa xmm0, [esi]          ; 1st_src_line
    movdqa xmm1, [esi+ecx]      ; 2nd_src_line

    ; packing & avg
    movdqa xmm2, xmm0           ; h H g G f F e E d D c C b B a A
    pshufb xmm0, xmm7           ; 0 H 0 G 0 F 0 E 0 D 0 C 0 B 0 A
    pshufb xmm2, xmm6           ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
    ; another implementation for xmm2 high bits
;   psubb xmm2, xmm0            ; h 0 g 0 f 0 e 0 d 0 c 0 b 0 a 0
;   psrlw xmm2, 8               ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
    pavgb xmm0, xmm2

    movdqa xmm3, xmm1
    pshufb xmm1, xmm7
    pshufb xmm3, xmm6
;   psubb xmm3, xmm1
;   psrlw xmm3, 8
    pavgb xmm1, xmm3

    pavgb xmm0, xmm1
    packuswb xmm0, xmm1

    ; write pDst
    movq [edi], xmm0

    ; next SMB
    lea esi, [esi+16]
    lea edi, [edi+8]

    dec eax
    jg near .xloops

    ; next line
    lea esi, [esi+2*ecx]    ; next end of lines
    lea esi, [esi+2*ebx]    ; reset to base 0 [- 2 * iDstWidth]
    lea edi, [edi+edx]
    lea edi, [edi+ebx]      ; reset to base 0 [- iDstWidth]

    dec ebp
    jg near .yloops

    pop ebp
    pop edi
    pop esi
    pop edx
    pop ebx
    ret

; got about 65% improvement over DyadicBilinearDownsamplerWidthx32_sse
;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx32_sse4(    unsigned char* pDst, const int iDstStride,
;                   unsigned char* pSrc, const int iSrcStride,
;                   const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx32_sse4
    push ebx
    push edx
    push esi
    push edi
    push ebp

    mov edi, [esp+24]   ; pDst
    mov edx, [esp+28]   ; iDstStride
    mov esi, [esp+32]   ; pSrc
    mov ecx, [esp+36]   ; iSrcStride
    mov ebp, [esp+44]   ; iSrcHeight

    sar ebp, $01            ; iSrcHeight >> 1

    movdqa xmm7, [shufb_mask_low]   ; mask low
    movdqa xmm6, [shufb_mask_high]  ; mask high

.yloops:
    mov eax, [esp+40]   ; iSrcWidth
    sar eax, $01            ; iSrcWidth >> 1
    mov ebx, eax        ; iDstWidth restored at ebx
    sar eax, $04            ; (iSrcWidth >> 1) / 16     ; loop count = num_of_mb
    neg ebx             ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 32 bytes
.xloops:
    ; 1st part horizonal loop: x16 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  xmm0: h H g G f F e E d D c C b B a A
    ;               xmm1: p P o O n N m M l L k K j J i I
    ;2nd Line Src:  xmm2: h H g G f F e E d D c C b B a A
    ;               xmm3: p P o O n N m M l L k K j J i I
    ;=> target:
    ;: P O N M L K J I H G F E D C B A
    ;: p o n m l k j i h g f e d c b a
    ;: P ..                          A
    ;: p ..                          a

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movntdqa xmm0, [esi]            ; 1st_src_line
    movntdqa xmm1, [esi+16]     ; 1st_src_line + 16
    movntdqa xmm2, [esi+ecx]        ; 2nd_src_line
    movntdqa xmm3, [esi+ecx+16] ; 2nd_src_line + 16

    ; packing & avg
    movdqa xmm4, xmm0           ; h H g G f F e E d D c C b B a A
    pshufb xmm0, xmm7           ; 0 H 0 G 0 F 0 E 0 D 0 C 0 B 0 A
    pshufb xmm4, xmm6           ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
;   psubb xmm4, xmm0            ; h 0 g 0 f 0 e 0 d 0 c 0 b 0 a 0
;   psrlw xmm4, 8               ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
    pavgb xmm0, xmm4

    movdqa xmm5, xmm1
    pshufb xmm1, xmm7
    pshufb xmm5, xmm6
;   psubb xmm5, xmm1
;   psrlw xmm5, 8
    pavgb xmm1, xmm5

    movdqa xmm4, xmm2
    pshufb xmm2, xmm7
    pshufb xmm4, xmm6
;   psubb xmm4, xmm2
;   psrlw xmm4, 8
    pavgb xmm2, xmm4

    movdqa xmm5, xmm3
    pshufb xmm3, xmm7
    pshufb xmm5, xmm6
;   psubb xmm5, xmm3
;   psrlw xmm5, 8
    pavgb xmm3, xmm5

    packuswb xmm0, xmm1
    packuswb xmm2, xmm3
    pavgb xmm0, xmm2

    ; write pDst
    movdqa [edi], xmm0

    ; next SMB
    lea esi, [esi+32]
    lea edi, [edi+16]

    dec eax
    jg near .xloops

    ; next line
    lea esi, [esi+2*ecx]    ; next end of lines
    lea esi, [esi+2*ebx]    ; reset to base 0 [- 2 * iDstWidth]
    lea edi, [edi+edx]
    lea edi, [edi+ebx]      ; reset to base 0 [- iDstWidth]

    dec ebp
    jg near .yloops

    pop ebp
    pop edi
    pop esi
    pop edx
    pop ebx
    ret

;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx16_sse4( unsigned char* pDst, const int iDstStride,
;                     unsigned char* pSrc, const int iSrcStride,
;                     const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx16_sse4
    push ebx
    push edx
    push esi
    push edi
    push ebp

    mov edi, [esp+24]   ; pDst
    mov edx, [esp+28]   ; iDstStride
    mov esi, [esp+32]   ; pSrc
    mov ecx, [esp+36]   ; iSrcStride
    mov ebp, [esp+44]   ; iSrcHeight

    sar ebp, $01        ; iSrcHeight >> 1
    movdqa xmm7, [shufb_mask_low]   ; mask low
    movdqa xmm6, [shufb_mask_high]  ; mask high

.yloops:
    mov eax, [esp+40]   ; iSrcWidth
    sar eax, $01        ; iSrcWidth >> 1
    mov ebx, eax        ; iDstWidth restored at ebx
    sar eax, $03        ; (iSrcWidth >> 1) / 8      ; loop count = num_of_mb
    neg ebx         ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 16 bytes
.xloops:
    ; horizonal loop: x16 bytes by source
    ;               mem  hi<-       ->lo
    ;1st line pSrc: xmm0: h H g G f F e E d D c C b B a A
    ;2nd line pSrc:  xmm1: p P o O n N m M l L k K j J i I
    ;=> target:
    ;: H G F E D C B A, P O N M L K J I
    ;: h g f e d c b a, p o n m l k j i

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movntdqa xmm0, [esi]            ; 1st_src_line
    movntdqa xmm1, [esi+ecx]        ; 2nd_src_line

    ; packing & avg
    movdqa xmm2, xmm0           ; h H g G f F e E d D c C b B a A
    pshufb xmm0, xmm7           ; 0 H 0 G 0 F 0 E 0 D 0 C 0 B 0 A
    pshufb xmm2, xmm6           ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
;   psubb xmm2, xmm0            ; h 0 g 0 f 0 e 0 d 0 c 0 b 0 a 0
;   psrlw xmm2, 8               ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
    pavgb xmm0, xmm2

    movdqa xmm3, xmm1
    pshufb xmm1, xmm7
    pshufb xmm3, xmm6
;   psubb xmm3, xmm1
;   psrlw xmm3, 8
    pavgb xmm1, xmm3

    pavgb xmm0, xmm1
    packuswb xmm0, xmm1

    ; write pDst
    movq [edi], xmm0

    ; next SMB
    lea esi, [esi+16]
    lea edi, [edi+8]

    dec eax
    jg near .xloops

    ; next line
    lea esi, [esi+2*ecx]    ; next end of lines
    lea esi, [esi+2*ebx]    ; reset to base 0 [- 2 * iDstWidth]
    lea edi, [edi+edx]
    lea edi, [edi+ebx]      ; reset to base 0 [- iDstWidth]

    dec ebp
    jg near .yloops

    pop ebp
    pop edi
    pop esi
    pop edx
    pop ebx
    ret





;**************************************************************************************************************
;int GeneralBilinearAccurateDownsampler_sse2(   unsigned char* pDst, const int iDstStride, const int iDstWidth, const int iDstHeight,
;                           unsigned char* pSrc, const int iSrcStride, const int iSrcWidth, const int iSrcHeight,
;                           unsigned int uiScaleX, unsigned int uiScaleY );
;{
;**************************************************************************************************************

WELS_EXTERN GeneralBilinearAccurateDownsampler_sse2
    push    ebp
    push    esi
    push    edi
    push    ebx
%define     pushsize    16
%define     localsize   28
%define     pDstData        esp + pushsize + localsize + 4
%define     dwDstStride     esp + pushsize + localsize + 8
%define     dwDstWidth      esp + pushsize + localsize + 12
%define     dwDstHeight     esp + pushsize + localsize + 16
%define     pSrcData        esp + pushsize + localsize + 20
%define     dwSrcStride     esp + pushsize + localsize + 24
%define     dwSrcWidth      esp + pushsize + localsize + 28
%define     dwSrcHeight     esp + pushsize + localsize + 32
%define     scale           esp + 0
%define     uiScaleX            esp + pushsize + localsize + 36
%define     uiScaleY            esp + pushsize + localsize + 40
%define     tmpHeight       esp + 12
%define     yInverse        esp + 16
%define     xInverse        esp + 20
%define     dstStep         esp + 24
    sub     esp,            localsize

    pxor    xmm0,   xmm0
    mov     edx,    32767
    mov     eax,    [uiScaleX]
    and     eax,    32767
    mov     ebx,    eax
    neg     ebx
    and     ebx,    32767
    movd    xmm1,       eax                     ; uinc(uiScaleX mod 32767)
    movd    xmm2,       ebx                     ; -uinc
    psllq   xmm1,       32
    por     xmm1,       xmm2                    ; 0 0  uinc  -uinc   (dword)
    pshufd  xmm7,       xmm1,   01000100b       ; xmm7: uinc -uinc uinc -uinc

    mov     eax,    [uiScaleY]
    and     eax,    32767
    mov     ebx,    eax
    neg     ebx
    and     ebx,    32767
    movd    xmm6,       eax                     ; vinc(uiScaleY mod 32767)
    movd    xmm2,       ebx                     ; -vinc
    psllq   xmm6,       32
    por     xmm6,       xmm2                    ; 0 0 vinc -vinc (dword)
    pshufd  xmm6,       xmm6,   01010000b       ; xmm6: vinc vinc -vinc -vinc

    mov     edx,        40003fffh
    movd    xmm5,       edx
    punpcklwd   xmm5,   xmm0                    ; 16384 16383
    pshufd  xmm5,       xmm5,   01000100b       ; xmm5: 16384 16383 16384 16383


DOWNSAMPLE:

    mov     eax,            [dwDstHeight]
    mov     edi,            [pDstData]
    mov     edx,            [dwDstStride]
    mov     ecx,            [dwDstWidth]
    sub     edx,            ecx
    mov     [dstStep],  edx             ; stride - width
    dec     eax
    mov     [tmpHeight],    eax
    mov     eax,            16384
    mov     [yInverse],     eax

    pshufd  xmm4,       xmm5,   01010000b   ; initial v to 16384 16384 16383 16383

HEIGHT:
    mov     eax,    [yInverse]
    mov     esi,    [pSrcData]
    shr     eax,    15
    mul     dword [dwSrcStride]
    add     esi,    eax                 ; get current row address
    mov     ebp,    esi
    add     ebp,    [dwSrcStride]

    mov     eax,        16384
    mov     [xInverse],     eax
    mov     ecx,            [dwDstWidth]
    dec     ecx

    movdqa  xmm3,       xmm5            ; initial u to 16384 16383 16384 16383

WIDTH:
    mov     eax,        [xInverse]
    shr     eax,        15

    movd    xmm1,       [esi+eax]       ; xxxxxxba
    movd    xmm2,       [ebp+eax]       ; xxxxxxdc
    pxor    xmm0,       xmm0
    punpcklwd   xmm1,   xmm2            ; xxxxdcba
    punpcklbw   xmm1,   xmm0            ; 0d0c0b0a
    punpcklwd   xmm1,   xmm0            ; 000d000c000b000a

    movdqa  xmm2,   xmm4    ; xmm2:  vv(1-v)(1-v)  tmpv
    pmaddwd xmm2,   xmm3    ; mul u(1-u)u(1-u) on xmm2
    movdqa  xmm0,   xmm2
    pmuludq xmm2,   xmm1
    psrlq   xmm0,   32
    psrlq   xmm1,   32
    pmuludq xmm0,   xmm1
    paddq   xmm2,   xmm0
    pshufd  xmm1,   xmm2,   00001110b
    paddq   xmm2,   xmm1
    psrlq   xmm2,   29

    movd    eax,    xmm2
    inc     eax
    shr     eax,    1
    mov     [edi],  al
    inc     edi

    mov     eax,        [uiScaleX]
    add     [xInverse], eax

    paddw   xmm3,       xmm7            ; inc u
    psllw   xmm3,       1
    psrlw   xmm3,       1

    loop    WIDTH

WIDTH_END:
    mov     eax,        [xInverse]
    shr     eax,        15
    mov     cl,         [esi+eax]
    mov     [edi],      cl
    inc     edi

    mov     eax,        [uiScaleY]
    add     [yInverse], eax
    add     edi,        [dstStep]

    paddw   xmm4,   xmm6                ; inc v
    psllw   xmm4,   1
    psrlw   xmm4,   1

    dec     dword [tmpHeight]
    jg      HEIGHT


LAST_ROW:
    mov     eax,    [yInverse]
    mov     esi,    [pSrcData]
    shr     eax,    15
    mul     dword [dwSrcStride]
    add     esi,    eax                 ; get current row address

    mov     eax,        16384
    mov     [xInverse],     eax
    mov     ecx,            [dwDstWidth]

LAST_ROW_WIDTH:
    mov     eax,        [xInverse]
    shr     eax,        15

    mov     al,         [esi+eax]
    mov     [edi],  al
    inc     edi

    mov     eax,        [uiScaleX]
    add     [xInverse], eax

    loop    LAST_ROW_WIDTH

LAST_ROW_END:

    add     esp,            localsize
    pop     ebx
    pop     edi
    pop     esi
    pop     ebp
%undef      pushsize
%undef      localsize
%undef      pSrcData
%undef      dwSrcWidth
%undef      dwSrcHeight
%undef      dwSrcStride
%undef      pDstData
%undef      dwDstWidth
%undef      dwDstHeight
%undef      dwDstStride
%undef      scale
%undef      uiScaleX
%undef      uiScaleY
%undef      tmpHeight
%undef      yInverse
%undef      xInverse
%undef      dstStep
    ret




;**************************************************************************************************************
;int GeneralBilinearFastDownsampler_sse2(   unsigned char* pDst, const int iDstStride, const int iDstWidth, const int iDstHeight,
;               unsigned char* pSrc, const int iSrcStride, const int iSrcWidth, const int iSrcHeight,
;               unsigned int uiScaleX, unsigned int uiScaleY );
;{
;**************************************************************************************************************

WELS_EXTERN GeneralBilinearFastDownsampler_sse2
    push    ebp
    push    esi
    push    edi
    push    ebx
%define     pushsize    16
%define     localsize   28
%define     pDstData        esp + pushsize + localsize + 4
%define     dwDstStride     esp + pushsize + localsize + 8
%define     dwDstWidth      esp + pushsize + localsize + 12
%define     dwDstHeight     esp + pushsize + localsize + 16
%define     pSrcData        esp + pushsize + localsize + 20
%define     dwSrcStride     esp + pushsize + localsize + 24
%define     dwSrcWidth      esp + pushsize + localsize + 28
%define     dwSrcHeight     esp + pushsize + localsize + 32
%define     scale           esp + 0
%define     uiScaleX            esp + pushsize + localsize + 36
%define     uiScaleY            esp + pushsize + localsize + 40
%define     tmpHeight       esp + 12
%define     yInverse        esp + 16
%define     xInverse        esp + 20
%define     dstStep         esp + 24
    sub     esp,            localsize

    pxor    xmm0,   xmm0
    mov     edx,    65535
    mov     eax,    [uiScaleX]
    and     eax,    edx
    mov     ebx,    eax
    neg     ebx
    and     ebx,    65535
    movd    xmm1,       eax                     ; uinc(uiScaleX mod 65536)
    movd    xmm2,       ebx                     ; -uinc
    psllq   xmm1,       32
    por     xmm1,       xmm2                    ; 0 uinc 0 -uinc
    pshuflw xmm7,       xmm1,   10001000b       ; xmm7: uinc -uinc uinc -uinc

    mov     eax,    [uiScaleY]
    and     eax,    32767
    mov     ebx,    eax
    neg     ebx
    and     ebx,    32767
    movd    xmm6,       eax                     ; vinc(uiScaleY mod 32767)
    movd    xmm2,       ebx                     ; -vinc
    psllq   xmm6,       32
    por     xmm6,       xmm2                    ; 0 vinc 0 -vinc
    pshuflw xmm6,       xmm6,   10100000b       ; xmm6: vinc vinc -vinc -vinc

    mov     edx,        80007fffh               ; 32768 32767
    movd    xmm5,       edx
    pshuflw xmm5,       xmm5,       01000100b   ; 32768 32767 32768 32767
    mov     ebx,        16384


FAST_DOWNSAMPLE:

    mov     eax,            [dwDstHeight]
    mov     edi,            [pDstData]
    mov     edx,            [dwDstStride]
    mov     ecx,            [dwDstWidth]
    sub     edx,            ecx
    mov     [dstStep],  edx             ; stride - width
    dec     eax
    mov     [tmpHeight],    eax
    mov     eax,        16384
    mov     [yInverse],     eax

    pshuflw xmm4,       xmm5,   01010000b
    psrlw   xmm4,       1               ; initial v to 16384 16384 16383 16383

FAST_HEIGHT:
    mov     eax,    [yInverse]
    mov     esi,    [pSrcData]
    shr     eax,    15
    mul     dword [dwSrcStride]
    add     esi,    eax                 ; get current row address
    mov     ebp,    esi
    add     ebp,    [dwSrcStride]

    mov     eax,        32768
    mov     [xInverse],     eax
    mov     ecx,            [dwDstWidth]
    dec     ecx

    movdqa  xmm3,       xmm5            ; initial u to 32768 32767 32768 32767

FAST_WIDTH:
    mov     eax,        [xInverse]
    shr     eax,        16

    movd    xmm1,       [esi+eax]       ; xxxxxxba
    movd    xmm2,       [ebp+eax]       ; xxxxxxdc
    punpcklwd   xmm1,   xmm2            ; xxxxdcba
    punpcklbw   xmm1,   xmm0            ; 0d0c0b0a

    movdqa  xmm2,   xmm4    ; xmm2:  vv(1-v)(1-v)  tmpv
    pmulhuw xmm2,   xmm3    ; mul u(1-u)u(1-u) on xmm2
    pmaddwd     xmm2,   xmm1
    pshufd  xmm1,   xmm2,   00000001b
    paddd   xmm2,   xmm1
    movd    xmm1,   ebx
    paddd   xmm2,   xmm1
    psrld   xmm2,   15

    packuswb    xmm2,   xmm0
    movd    eax,    xmm2
    mov     [edi],  al
    inc     edi

    mov     eax,        [uiScaleX]
    add     [xInverse], eax

    paddw   xmm3,       xmm7            ; inc u

    loop    FAST_WIDTH

FAST_WIDTH_END:
    mov     eax,        [xInverse]
    shr     eax,        16
    mov     cl,         [esi+eax]
    mov     [edi],      cl
    inc     edi

    mov     eax,        [uiScaleY]
    add     [yInverse], eax
    add     edi,        [dstStep]

    paddw   xmm4,   xmm6                ; inc v
    psllw   xmm4,   1
    psrlw   xmm4,   1

    dec     dword [tmpHeight]
    jg      FAST_HEIGHT


FAST_LAST_ROW:
    mov     eax,    [yInverse]
    mov     esi,    [pSrcData]
    shr     eax,    15
    mul     dword [dwSrcStride]
    add     esi,    eax                 ; get current row address

    mov     eax,        32768
    mov     [xInverse],     eax
    mov     ecx,            [dwDstWidth]

FAST_LAST_ROW_WIDTH:
    mov     eax,        [xInverse]
    shr     eax,        16

    mov     al,         [esi+eax]
    mov     [edi],  al
    inc     edi

    mov     eax,        [uiScaleX]
    add     [xInverse], eax

    loop    FAST_LAST_ROW_WIDTH

FAST_LAST_ROW_END:

    add     esp,            localsize
    pop     ebx
    pop     edi
    pop     esi
    pop     ebp
%undef      pushsize
%undef      localsize
%undef      pSrcData
%undef      dwSrcWidth
%undef      dwSrcHeight
%undef      dwSrcStride
%undef      pDstData
%undef      dwDstWidth
%undef      dwDstHeight
%undef      dwDstStride
%undef      scale
%undef      uiScaleX
%undef      uiScaleY
%undef      tmpHeight
%undef      yInverse
%undef      xInverse
%undef      dstStep
    ret
%endif
