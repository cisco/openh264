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
db80h_128:
    times 16 db 80h
shufb_0000000088888888:
    times 8 db 0
    times 8 db 8
shufb_000044448888CCCC:
    times 4 db 0
    times 4 db 4
    times 4 db 8
    times 4 db 12
shufb_mask_low:
    db 00h, 80h, 02h, 80h, 04h, 80h, 06h, 80h, 08h, 80h, 0ah, 80h, 0ch, 80h, 0eh, 80h
shufb_mask_high:
    db 01h, 80h, 03h, 80h, 05h, 80h, 07h, 80h, 09h, 80h, 0bh, 80h, 0dh, 80h, 0fh, 80h
add_extra_half:
    dd 16384,0,0,0

shufb_mask_quarter:
db 00h, 04h, 08h, 0ch, 80h, 80h, 80h, 80h, 01h, 05h, 09h, 0dh, 80h, 80h, 80h, 80h

shufb_mask_onethird_low_1:
db 00h, 03h, 06h, 09h, 0ch, 0fh, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h
shufb_mask_onethird_low_2:
db 80h, 80h, 80h, 80h, 80h, 80h, 02h, 05h, 08h, 0bh, 0eh, 80h, 80h, 80h, 80h, 80h
shufb_mask_onethird_low_3:
db 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 01h, 04h, 07h, 0ah, 0dh

shufb_mask_onethird_high_1:
db 01h, 04h, 07h, 0ah, 0dh, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h
shufb_mask_onethird_high_2:
db 80h, 80h, 80h, 80h, 80h, 00h, 03h, 06h, 09h, 0ch, 0fh, 80h, 80h, 80h, 80h, 80h
shufb_mask_onethird_high_3:
db 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 80h, 02h, 05h, 08h, 0bh, 0eh

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
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif
    sar r5, $01            ; iSrcHeight >> 1

.yloops1:
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif
    sar r4, $01            ; iSrcWidth >> 1
    mov r6, r4        ; iDstWidth restored at ebx
    sar r4, $04            ; (iSrcWidth >> 1) / 16     ; loop count = num_of_mb
    neg r6             ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 32 bytes
.xloops1:
    ; 1st part horizonal loop: x16 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  mm0: d D c C b B a A    mm1: h H g G f F e E
    ;2nd Line Src:  mm2: l L k K j J i I    mm3: p P o O n N m M
    ;=> target:
    ;: H G F E D C B A, P O N M L K J I
    ;: h g f e d c b a, p o n m l k j i
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movq mm0, [r2]         ; 1st pSrc line
    movq mm1, [r2+8]       ; 1st pSrc line + 8
    movq mm2, [r2+r3]     ; 2nd pSrc line
    movq mm3, [r2+r3+8]   ; 2nd pSrc line + 8

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
    movq mm1, [r2+16]      ; 1st pSrc line + 16
    movq mm2, [r2+24]      ; 1st pSrc line + 24
    movq mm3, [r2+r3+16]  ; 2nd pSrc line + 16
    movq mm4, [r2+r3+24]  ; 2nd pSrc line + 24

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

    movq [r0  ], mm0
    movq [r0+8], mm2

    ; next SMB
    lea r2, [r2+32]
    lea r0, [r0+16]

    dec r4
    jg near .xloops1

    ; next line
    lea r2, [r2+2*r3]    ; next end of lines
    lea r2, [r2+2*r6]    ; reset to base 0 [- 2 * iDstWidth]
    lea r0, [r0+r1]
    lea r0, [r0+r6]      ; reset to base 0 [- iDstWidth]

    dec r5
    jg near .yloops1

    WELSEMMS
%ifndef X86_32
    pop r12
%endif
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret

;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx16_sse( unsigned char* pDst, const int iDstStride,
;                     unsigned char* pSrc, const int iSrcStride,
;                     const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx16_sse
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif
    sar r5, $01            ; iSrcHeight >> 1

.yloops2:
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif
    sar r4, $01            ; iSrcWidth >> 1
    mov r6, r4        ; iDstWidth restored at ebx
    sar r4, $03            ; (iSrcWidth >> 1) / 8     ; loop count = num_of_mb
    neg r6             ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 16 bytes
.xloops2:
    ; 1st part horizonal loop: x16 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  mm0: d D c C b B a A    mm1: h H g G f F e E
    ;2nd Line Src:  mm2: l L k K j J i I    mm3: p P o O n N m M
    ;=> target:
    ;: H G F E D C B A, P O N M L K J I
    ;: h g f e d c b a, p o n m l k j i
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movq mm0, [r2]         ; 1st pSrc line
    movq mm1, [r2+8]       ; 1st pSrc line + 8
    movq mm2, [r2+r3]     ; 2nd pSrc line
    movq mm3, [r2+r3+8]   ; 2nd pSrc line + 8

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

    movq [r0  ], mm0

    ; next SMB
    lea r2, [r2+16]
    lea r0, [r0+8]

    dec r4
    jg near .xloops2

    ; next line
    lea r2, [r2+2*r3]    ; next end of lines
    lea r2, [r2+2*r6]    ; reset to base 0 [- 2 * iDstWidth]
    lea r0, [r0+r1]
    lea r0, [r0+r6]      ; reset to base 0 [- iDstWidth]

    dec r5
    jg near .yloops2

    WELSEMMS
%ifndef X86_32
    pop r12
%endif
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret

;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx8_sse( unsigned char* pDst, const int iDstStride,
;                     unsigned char* pSrc, const int iSrcStride,
;                     const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx8_sse
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif
    sar r5, $01            ; iSrcHeight >> 1

.yloops3:
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif
    sar r4, $01            ; iSrcWidth >> 1
    mov r6, r4        ; iDstWidth restored at ebx
    sar r4, $02            ; (iSrcWidth >> 1) / 4     ; loop count = num_of_mb
    neg r6             ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 8 bytes
.xloops3:
    ; 1st part horizonal loop: x8 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  mm0: d D c C b B a A
    ;2nd Line Src:  mm1: h H g G f F e E
    ;=> target:
    ;: H G F E D C B A
    ;: h g f e d c b a
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movq mm0, [r2]         ; 1st pSrc line
    movq mm1, [r2+r3]     ; 2nd pSrc line

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

    movd [r0], mm0

    ; next unit
    lea r2, [r2+8]
    lea r0, [r0+4]

    dec r4
    jg near .xloops3

    ; next line
    lea r2, [r2+2*r3]    ; next end of lines
    lea r2, [r2+2*r6]    ; reset to base 0 [- 2 * iDstWidth]
    lea r0, [r0+r1]
    lea r0, [r0+r6]      ; reset to base 0 [- iDstWidth]

    dec r5
    jg near .yloops3

    WELSEMMS
%ifndef X86_32
    pop r12
%endif
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret



; got about 50% improvement over DyadicBilinearDownsamplerWidthx32_sse
;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx32_ssse3(   unsigned char* pDst, const int iDstStride,
;                   unsigned char* pSrc, const int iSrcStride,
;                   const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx32_ssse3
    ;push ebx
    ;push edx
    ;push esi
    ;push edi
    ;push ebp

    ;mov edi, [esp+24]   ; pDst
    ;mov edx, [esp+28]   ; iDstStride
    ;mov esi, [esp+32]   ; pSrc
    ;mov ecx, [esp+36]   ; iSrcStride
    ;mov ebp, [esp+44]   ; iSrcHeight
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif
    sar r5, $01            ; iSrcHeight >> 1

    movdqa xmm7, [shufb_mask_low]   ; mask low
    movdqa xmm6, [shufb_mask_high]  ; mask high

.yloops4:
    ;mov eax, [esp+40]   ; iSrcWidth
    ;sar eax, $01            ; iSrcWidth >> 1
    ;mov ebx, eax        ; iDstWidth restored at ebx
    ;sar eax, $04            ; (iSrcWidth >> 1) / 16     ; loop count = num_of_mb
    ;neg ebx             ; - (iSrcWidth >> 1)
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif
    sar r4, $01            ; iSrcWidth >> 1
    mov r6, r4        ; iDstWidth restored at ebx
    sar r4, $04            ; (iSrcWidth >> 1) / 16     ; loop count = num_of_mb
    neg r6             ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 32 bytes
.xloops4:
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
    movdqa xmm0, [r2]          ; 1st_src_line
    movdqa xmm1, [r2+16]       ; 1st_src_line + 16
    movdqa xmm2, [r2+r3]      ; 2nd_src_line
    movdqa xmm3, [r2+r3+16]   ; 2nd_src_line + 16

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
    movdqa [r0], xmm0

    ; next SMB
    lea r2, [r2+32]
    lea r0, [r0+16]

    dec r4
    jg near .xloops4

    ; next line
    lea r2, [r2+2*r3]    ; next end of lines
    lea r2, [r2+2*r6]    ; reset to base 0 [- 2 * iDstWidth]
    lea r0, [r0+r1]
    lea r0, [r0+r6]      ; reset to base 0 [- iDstWidth]

    dec r5
    jg near .yloops4

%ifndef X86_32
    pop r12
%endif

    POP_XMM
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret

;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx16_ssse3( unsigned char* pDst, const int iDstStride,
;                     unsigned char* pSrc, const int iSrcStride,
;                     const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx16_ssse3
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    PUSH_XMM 6
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif
    sar r5, $01            ; iSrcHeight >> 1
    movdqa xmm5, [shufb_mask_low]   ; mask low
    movdqa xmm4, [shufb_mask_high]  ; mask high

.yloops5:
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif
    sar r4, $01            ; iSrcWidth >> 1
    mov r6, r4        ; iDstWidth restored at ebx
    sar r4, $03            ; (iSrcWidth >> 1) / 8     ; loop count = num_of_mb
    neg r6             ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 16 bytes
.xloops5:
    ; horizonal loop: x16 bytes by source
    ;               mem  hi<-       ->lo
    ;1st line pSrc: xmm0: h H g G f F e E d D c C b B a A
    ;2nd line pSrc:  xmm1: p P o O n N m M l L k K j J i I
    ;=> target:
    ;: H G F E D C B A, P O N M L K J I
    ;: h g f e d c b a, p o n m l k j i

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movdqa xmm0, [r2]          ; 1st_src_line
    movdqa xmm1, [r2+r3]      ; 2nd_src_line

    ; packing & avg
    movdqa xmm2, xmm0           ; h H g G f F e E d D c C b B a A
    pshufb xmm0, xmm5           ; 0 H 0 G 0 F 0 E 0 D 0 C 0 B 0 A
    pshufb xmm2, xmm4           ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
    ; another implementation for xmm2 high bits
;   psubb xmm2, xmm0            ; h 0 g 0 f 0 e 0 d 0 c 0 b 0 a 0
;   psrlw xmm2, 8               ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
    pavgb xmm0, xmm2

    movdqa xmm3, xmm1
    pshufb xmm1, xmm5
    pshufb xmm3, xmm4
;   psubb xmm3, xmm1
;   psrlw xmm3, 8
    pavgb xmm1, xmm3

    pavgb xmm0, xmm1
    packuswb xmm0, xmm1

    ; write pDst
    movq [r0], xmm0

    ; next SMB
    lea r2, [r2+16]
    lea r0, [r0+8]

    dec r4
    jg near .xloops5

    lea r2, [r2+2*r3]    ; next end of lines
    lea r2, [r2+2*r6]    ; reset to base 0 [- 2 * iDstWidth]
    lea r0, [r0+r1]
    lea r0, [r0+r6]      ; reset to base 0 [- iDstWidth]

    dec r5
    jg near .yloops5

%ifndef X86_32
    pop r12
%endif

    POP_XMM
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret

; got about 65% improvement over DyadicBilinearDownsamplerWidthx32_sse
;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx32_sse4(    unsigned char* pDst, const int iDstStride,
;                   unsigned char* pSrc, const int iSrcStride,
;                   const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx32_sse4
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif
    sar r5, $01            ; iSrcHeight >> 1

    movdqa xmm7, [shufb_mask_low]   ; mask low
    movdqa xmm6, [shufb_mask_high]  ; mask high

.yloops6:
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif
    sar r4, $01            ; iSrcWidth >> 1
    mov r6, r4        ; iDstWidth restored at ebx
    sar r4, $04            ; (iSrcWidth >> 1) / 16     ; loop count = num_of_mb
    neg r6             ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 32 bytes
.xloops6:
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
    movntdqa xmm0, [r2]            ; 1st_src_line
    movntdqa xmm1, [r2+16]     ; 1st_src_line + 16
    movntdqa xmm2, [r2+r3]        ; 2nd_src_line
    movntdqa xmm3, [r2+r3+16] ; 2nd_src_line + 16

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
    movdqa [r0], xmm0

    ; next SMB
    lea r2, [r2+32]
    lea r0, [r0+16]

    dec r4
    jg near .xloops6

    lea r2, [r2+2*r3]    ; next end of lines
    lea r2, [r2+2*r6]    ; reset to base 0 [- 2 * iDstWidth]
    lea r0, [r0+r1]
    lea r0, [r0+r6]      ; reset to base 0 [- iDstWidth]

    dec r5
    jg near .yloops6

%ifndef X86_32
    pop r12
%endif

    POP_XMM
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret

;***********************************************************************
;   void DyadicBilinearDownsamplerWidthx16_sse4( unsigned char* pDst, const int iDstStride,
;                     unsigned char* pSrc, const int iSrcStride,
;                     const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearDownsamplerWidthx16_sse4
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    PUSH_XMM 6
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif
    sar r5, $01            ; iSrcHeight >> 1
    movdqa xmm5, [shufb_mask_low]   ; mask low
    movdqa xmm4, [shufb_mask_high]  ; mask high

.yloops7:
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif
    sar r4, $01            ; iSrcWidth >> 1
    mov r6, r4        ; iDstWidth restored at ebx
    sar r4, $03            ; (iSrcWidth >> 1) / 8     ; loop count = num_of_mb
    neg r6             ; - (iSrcWidth >> 1)
    ; each loop = source bandwidth: 16 bytes
.xloops7:
    ; horizonal loop: x16 bytes by source
    ;               mem  hi<-       ->lo
    ;1st line pSrc: xmm0: h H g G f F e E d D c C b B a A
    ;2nd line pSrc:  xmm1: p P o O n N m M l L k K j J i I
    ;=> target:
    ;: H G F E D C B A, P O N M L K J I
    ;: h g f e d c b a, p o n m l k j i

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movntdqa xmm0, [r2]            ; 1st_src_line
    movntdqa xmm1, [r2+r3]        ; 2nd_src_line

    ; packing & avg
    movdqa xmm2, xmm0           ; h H g G f F e E d D c C b B a A
    pshufb xmm0, xmm5           ; 0 H 0 G 0 F 0 E 0 D 0 C 0 B 0 A
    pshufb xmm2, xmm4           ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
;   psubb xmm2, xmm0            ; h 0 g 0 f 0 e 0 d 0 c 0 b 0 a 0
;   psrlw xmm2, 8               ; 0 h 0 g 0 f 0 e 0 d 0 c 0 b 0 a
    pavgb xmm0, xmm2

    movdqa xmm3, xmm1
    pshufb xmm1, xmm5
    pshufb xmm3, xmm4
;   psubb xmm3, xmm1
;   psrlw xmm3, 8
    pavgb xmm1, xmm3

    pavgb xmm0, xmm1
    packuswb xmm0, xmm1

    ; write pDst
    movq [r0], xmm0

    ; next SMB
    lea r2, [r2+16]
    lea r0, [r0+8]

    dec r4
    jg near .xloops7

    ; next line
    lea r2, [r2+2*r3]    ; next end of lines
    lea r2, [r2+2*r6]    ; reset to base 0 [- 2 * iDstWidth]
    lea r0, [r0+r1]
    lea r0, [r0+r6]      ; reset to base 0 [- iDstWidth]

    dec r5
    jg near .yloops7

%ifndef X86_32
    pop r12
%endif

    POP_XMM
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret


%ifdef X86_32
;**************************************************************************************************************
;int GeneralBilinearAccurateDownsampler_sse2(   unsigned char* pDst, const int iDstStride, const int iDstWidth, const int iDstHeight,
;                           unsigned char* pSrc, const int iSrcStride,
;                           unsigned int uiScaleX, unsigned int uiScaleY );
;{
;**************************************************************************************************************

WELS_EXTERN GeneralBilinearAccurateDownsampler_sse2
    push    ebp
    push    esi
    push    edi
    push    ebx
%define     pushsize    16
%define     localsize   16
%define     pDstData        esp + pushsize + localsize + 4
%define     dwDstStride     esp + pushsize + localsize + 8
%define     dwDstWidth      esp + pushsize + localsize + 12
%define     dwDstHeight     esp + pushsize + localsize + 16
%define     pSrcData        esp + pushsize + localsize + 20
%define     dwSrcStride     esp + pushsize + localsize + 24
%define     uiScaleX            esp + pushsize + localsize + 28
%define     uiScaleY            esp + pushsize + localsize + 32
%define     tmpHeight       esp + 0
%define     yInverse        esp + 4
%define     xInverse        esp + 8
%define     dstStep         esp + 12
    sub     esp,            localsize

    pxor    xmm0,   xmm0
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
%undef      uiScaleX
%undef      uiScaleY
%undef      tmpHeight
%undef      yInverse
%undef      xInverse
%undef      dstStep
    ret




;**************************************************************************************************************
;int GeneralBilinearFastDownsampler_sse2(   unsigned char* pDst, const int iDstStride, const int iDstWidth, const int iDstHeight,
;               unsigned char* pSrc, const int iSrcStride,
;               unsigned int uiScaleX, unsigned int uiScaleY );
;{
;**************************************************************************************************************

WELS_EXTERN GeneralBilinearFastDownsampler_sse2
    push    ebp
    push    esi
    push    edi
    push    ebx
%define     pushsize    16
%define     localsize   16
%define     pDstData        esp + pushsize + localsize + 4
%define     dwDstStride     esp + pushsize + localsize + 8
%define     dwDstWidth      esp + pushsize + localsize + 12
%define     dwDstHeight     esp + pushsize + localsize + 16
%define     pSrcData        esp + pushsize + localsize + 20
%define     dwSrcStride     esp + pushsize + localsize + 24
%define     uiScaleX            esp + pushsize + localsize + 28
%define     uiScaleY            esp + pushsize + localsize + 32
%define     tmpHeight       esp + 0
%define     yInverse        esp + 4
%define     xInverse        esp + 8
%define     dstStep         esp + 12
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
%undef      dwDstStride
%undef      uiScaleX
%undef      uiScaleY
%undef      tmpHeight
%undef      yInverse
%undef      xInverse
%undef      dstStep
    ret

%elifdef  WIN64

;**************************************************************************************************************
;int GeneralBilinearAccurateDownsampler_sse2(   unsigned char* pDst, const int iDstStride, const int iDstWidth, const int iDstHeight,
;                           unsigned char* pSrc, const int iSrcStride,
;                           unsigned int uiScaleX, unsigned int uiScaleY );
;{
;**************************************************************************************************************

WELS_EXTERN GeneralBilinearAccurateDownsampler_sse2
    push    r12
    push    r13
    push    r14
    push    r15
    push    rsi
    push    rdi
    push    rbx
    push    rbp
    %assign push_num 8
    LOAD_7_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r2, r2d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r5, r5d
    SIGN_EXTENSION r6, r6d

    pxor    xmm0,   xmm0
    mov     r12d,   r6d
    and     r12d,   32767
    mov     r13d,   r12d
    neg     r13d
    and     r13d,   32767
    movd    xmm1,   r12d                     ; uinc(uiScaleX mod 32767)
    movd    xmm2,   r13d                     ; -uinc
    psllq   xmm1,   32
    por     xmm1,   xmm2                    ; 0 0  uinc  -uinc   (dword)
    pshufd  xmm7,   xmm1,   01000100b       ; xmm7: uinc -uinc uinc -uinc

    mov     r12,    arg8
    SIGN_EXTENSION r12, r12d
    mov     rbp,    r12
    and     r12d,   32767
    mov     r13d,   r12d
    neg     r13d
    and     r13d,   32767
    movd    xmm6,       r12d                     ; vinc(uiScaleY mod 32767)
    movd    xmm2,       r13d                     ; -vinc
    psllq   xmm6,       32
    por     xmm6,       xmm2                    ; 0 0 vinc -vinc (dword)
    pshufd  xmm6,       xmm6,   01010000b       ; xmm6: vinc vinc -vinc -vinc

    mov     r12d,        40003fffh
    movd    xmm5,       r12d
    punpcklwd   xmm5,   xmm0                    ; 16384 16383
    pshufd  xmm5,       xmm5,   01000100b       ; xmm5: 16384 16383 16384 16383

DOWNSAMPLE:
    sub     r1, r2                   ; stride - width
    dec     r3
    mov     r14,16384
    pshufd  xmm4,       xmm5,   01010000b   ; initial v to 16384 16384 16383 16383

HEIGHT:
    ;mov     r12, r4
    mov     r12, r14
    shr     r12,    15
    imul    r12,    r5
    add     r12,    r4                 ; get current row address
    mov     r13,    r12
    add     r13,    r5

    mov     r15, 16384
    mov     rsi, r2
    dec     rsi
    movdqa  xmm3,       xmm5            ; initial u to 16384 16383 16384 16383

WIDTH:
    mov     rdi,        r15
    shr     rdi,        15

    movd    xmm1,       [r12+rdi]       ; xxxxxxba
    movd    xmm2,       [r13+rdi]       ; xxxxxxdc
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

    movd    ebx,    xmm2
    inc     ebx
    shr     ebx,    1
    mov     [r0],   bl
    inc     r0

    add      r15, r6
    paddw   xmm3,       xmm7            ; inc u
    psllw   xmm3,       1
    psrlw   xmm3,       1

    dec     rsi
    jg      WIDTH

WIDTH_END:
    shr     r15, 15
    mov     bl,  [r12+r15]
    mov     [r0],bl
    inc     r0
    add     r14, rbp
    add     r0,  r1

    paddw   xmm4,   xmm6                ; inc v
    psllw   xmm4,   1
    psrlw   xmm4,   1

    dec     r3
    jg      HEIGHT

LAST_ROW:
    shr     r14, 15
    imul    r14, r5
    add     r4, r14
    mov     r15, 16384

LAST_ROW_WIDTH:
    mov     rdi, r15
    shr     rdi, 15
    mov     bl,  [r4+rdi]
    mov     [r0],bl
    inc     r0

    add     r15, r6
    dec     r2
    jg    LAST_ROW_WIDTH

LAST_ROW_END:

    POP_XMM
    pop     rbp
    pop     rbx
    pop     rdi
    pop     rsi
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    ret

;**************************************************************************************************************
;int GeneralBilinearFastDownsampler_sse2(   unsigned char* pDst, const int iDstStride, const int iDstWidth, const int iDstHeight,
;               unsigned char* pSrc, const int iSrcStride,
;               unsigned int uiScaleX, unsigned int uiScaleY );
;{
;**************************************************************************************************************

WELS_EXTERN GeneralBilinearFastDownsampler_sse2
    push    r12
    push    r13
    push    r14
    push    r15
    push    rsi
    push    rdi
    push    rbx
    push    rbp
    %assign push_num 8
    LOAD_7_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r2, r2d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r5, r5d
    SIGN_EXTENSION r6, r6d

    pxor    xmm0,   xmm0
    mov     r12d,   r6d
    and     r12d,   65535
    mov     r13d,   r12d
    neg     r13d
    and     r13d,   65535
    movd    xmm1,   r12d                     ; uinc(uiScaleX mod 65536)
    movd    xmm2,   r13d                     ; -uinc
    psllq   xmm1,   32
    por     xmm1,   xmm2                    ; 0 uinc 0 -uinc
    pshuflw xmm7,   xmm1,   10001000b       ; xmm7: uinc -uinc uinc -uinc

    mov     r12,    arg8
    SIGN_EXTENSION r12, r12d
    mov     rbp,    r12
    and     r12d,   32767
    mov     r13d,   r12d
    neg     r13d
    and     r13d,   32767
    movd    xmm6,       r12d                     ; vinc(uiScaleY mod 32767)
    movd    xmm2,       r13d                     ; -vinc
    psllq   xmm6,       32
    por     xmm6,       xmm2                    ; 0 vinc 0 -vinc
    pshuflw xmm6,       xmm6,   10100000b       ; xmm6: vinc vinc -vinc -vinc

    mov     r12d,       80007fffh               ; 32768 32767
    movd    xmm5,       r12d
    pshuflw xmm5,       xmm5,       01000100b   ; 32768 32767 32768 32767

FAST_DOWNSAMPLE:
    sub     r1, r2                   ; stride - width
    dec     r3
    mov     r14,16384

    pshuflw xmm4,       xmm5,   01010000b
    psrlw   xmm4,       1               ; initial v to 16384 16384 16383 16383

FAST_HEIGHT:
    mov     r12, r14
    shr     r12,    15
    imul    r12,    r5
    add     r12,    r4                 ; get current row address
    mov     r13,    r12
    add     r13,    r5

    mov     r15, 32768
    mov     rsi, r2
    dec     rsi

    movdqa  xmm3,       xmm5            ; initial u to 32768 32767 32768 32767

FAST_WIDTH:
    mov     rdi,        r15
    shr     rdi,        16

    movd    xmm1,       [r12+rdi]       ; xxxxxxba
    movd    xmm2,       [r13+rdi]       ; xxxxxxdc
    punpcklwd   xmm1,   xmm2            ; xxxxdcba
    punpcklbw   xmm1,   xmm0            ; 0d0c0b0a

    movdqa  xmm2,   xmm4    ; xmm2:  vv(1-v)(1-v)  tmpv
    pmulhuw xmm2,   xmm3    ; mul u(1-u)u(1-u) on xmm2
    pmaddwd     xmm2,   xmm1
    pshufd  xmm1,   xmm2,   00000001b
    paddd   xmm2,   xmm1
    movdqa  xmm1,   [add_extra_half]
    paddd   xmm2,   xmm1
    psrld   xmm2,   15

    packuswb    xmm2,   xmm0
    movd    ebx,    xmm2
    mov     [r0],  bl
    inc     r0

    add     r15, r6

    paddw   xmm3,       xmm7            ; inc u
    dec     rsi
    jg      FAST_WIDTH

FAST_WIDTH_END:
    shr     r15, 16
    mov     bl,  [r12+r15]
    mov     [r0],bl
    inc     r0
    add     r14, rbp
    add     r0,  r1

    paddw   xmm4,   xmm6                ; inc v
    psllw   xmm4,   1
    psrlw   xmm4,   1

    dec     r3
    jg      FAST_HEIGHT


FAST_LAST_ROW:
    shr     r14, 15
    imul    r14, r5
    add     r4, r14
    mov     r15, 32768

FAST_LAST_ROW_WIDTH:
    mov     rdi, r15
    shr     rdi, 16
    mov     bl,  [r4+rdi]
    mov     [r0],bl
    inc     r0

    add     r15, r6
    dec     r2
    jg      FAST_LAST_ROW_WIDTH

FAST_LAST_ROW_END:

    POP_XMM
    pop     rbp
    pop     rbx
    pop     rdi
    pop     rsi
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    ret

%elifdef  UNIX64

;**************************************************************************************************************
;int GeneralBilinearAccurateDownsampler_sse2(   unsigned char* pDst, const int iDstStride, const int iDstWidth, const int iDstHeight,
;                           unsigned char* pSrc, const int iSrcStride,
;                           unsigned int uiScaleX, unsigned int uiScaleY );
;{
;**************************************************************************************************************

WELS_EXTERN GeneralBilinearAccurateDownsampler_sse2
    push    r12
    push    r13
    push    r14
    push    r15
    push    rbx
    push    rbp
    %assign push_num 6
    LOAD_7_PARA
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r2, r2d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r5, r5d
    SIGN_EXTENSION r6, r6d

    pxor    xmm0,   xmm0
    mov     r12d,   r6d
    and     r12d,   32767
    mov     r13d,   r12d
    neg     r13d
    and     r13d,   32767
    movd    xmm1,   r12d                     ; uinc(uiScaleX mod 32767)
    movd    xmm2,   r13d                     ; -uinc
    psllq   xmm1,   32
    por     xmm1,   xmm2                    ; 0 0  uinc  -uinc   (dword)
    pshufd  xmm7,   xmm1,   01000100b       ; xmm7: uinc -uinc uinc -uinc

    mov     r12,    arg8
    SIGN_EXTENSION r12, r12d
    mov     rbp,    r12
    and     r12d,   32767
    mov     r13d,   r12d
    neg     r13d
    and     r13d,   32767
    movd    xmm6,       r12d                     ; vinc(uiScaleY mod 32767)
    movd    xmm2,       r13d                     ; -vinc
    psllq   xmm6,       32
    por     xmm6,       xmm2                    ; 0 0 vinc -vinc (dword)
    pshufd  xmm6,       xmm6,   01010000b       ; xmm6: vinc vinc -vinc -vinc

    mov     r12d,        40003fffh
    movd    xmm5,       r12d
    punpcklwd   xmm5,   xmm0                    ; 16384 16383
    pshufd  xmm5,       xmm5,   01000100b       ; xmm5: 16384 16383 16384 16383

DOWNSAMPLE:
    sub     r1, r2                   ; stride - width
    dec     r3
    mov     r14,16384
    pshufd  xmm4,       xmm5,   01010000b   ; initial v to 16384 16384 16383 16383

HEIGHT:
    ;mov     r12, r4
    mov     r12, r14
    shr     r12,    15
    imul    r12,    r5
    add     r12,    r4                 ; get current row address
    mov     r13,    r12
    add     r13,    r5

    mov     r15, 16384
    mov     rax, r2
    dec     rax
    movdqa  xmm3,       xmm5            ; initial u to 16384 16383 16384 16383

WIDTH:
    mov     r11,        r15
    shr     r11,        15

    movd    xmm1,       [r12+r11]       ; xxxxxxba
    movd    xmm2,       [r13+r11]       ; xxxxxxdc
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

    movd    ebx,    xmm2
    inc     ebx
    shr     ebx,    1
    mov     [r0],   bl
    inc     r0

    add      r15, r6
    paddw   xmm3,       xmm7            ; inc u
    psllw   xmm3,       1
    psrlw   xmm3,       1

    dec     rax
    jg      WIDTH

WIDTH_END:
    shr     r15, 15
    mov     bl,  [r12+r15]
    mov     [r0],bl
    inc     r0
    add     r14, rbp
    add     r0,  r1

    paddw   xmm4,   xmm6                ; inc v
    psllw   xmm4,   1
    psrlw   xmm4,   1

    dec     r3
    jg      HEIGHT

LAST_ROW:
    shr     r14, 15
    imul    r14, r5
    add     r4, r14
    mov     r15, 16384

LAST_ROW_WIDTH:
    mov     r11, r15
    shr     r11, 15
    mov     bl,  [r4+r11]
    mov     [r0],bl
    inc     r0

    add     r15, r6
    dec     r2
    jg    LAST_ROW_WIDTH

LAST_ROW_END:

    pop     rbp
    pop     rbx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    ret

;**************************************************************************************************************
;int GeneralBilinearFastDownsampler_sse2(   unsigned char* pDst, const int iDstStride, const int iDstWidth, const int iDstHeight,
;               unsigned char* pSrc, const int iSrcStride,
;               unsigned int uiScaleX, unsigned int uiScaleY );
;{
;**************************************************************************************************************

WELS_EXTERN GeneralBilinearFastDownsampler_sse2
    push    r12
    push    r13
    push    r14
    push    r15
    push    rbx
    push    rbp
    %assign push_num 6
    LOAD_7_PARA
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r2, r2d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r5, r5d
    SIGN_EXTENSION r6, r6d

    pxor    xmm0,   xmm0
    mov     r12d,   r6d
    and     r12d,   65535
    mov     r13d,   r12d
    neg     r13d
    and     r13d,   65535
    movd    xmm1,   r12d                     ; uinc(uiScaleX mod 65536)
    movd    xmm2,   r13d                     ; -uinc
    psllq   xmm1,   32
    por     xmm1,   xmm2                    ; 0 uinc 0 -uinc
    pshuflw xmm7,   xmm1,   10001000b       ; xmm7: uinc -uinc uinc -uinc

    mov     r12,    arg8
    SIGN_EXTENSION r12, r12d
    mov     rbp,    r12
    and     r12d,   32767
    mov     r13d,   r12d
    neg     r13d
    and     r13d,   32767
    movd    xmm6,       r12d                     ; vinc(uiScaleY mod 32767)
    movd    xmm2,       r13d                     ; -vinc
    psllq   xmm6,       32
    por     xmm6,       xmm2                    ; 0 vinc 0 -vinc
    pshuflw xmm6,       xmm6,   10100000b       ; xmm6: vinc vinc -vinc -vinc

    mov     r12d,       80007fffh               ; 32768 32767
    movd    xmm5,       r12d
    pshuflw xmm5,       xmm5,       01000100b   ; 32768 32767 32768 32767

FAST_DOWNSAMPLE:
    sub     r1, r2                   ; stride - width
    dec     r3
    mov     r14,16384

    pshuflw xmm4,       xmm5,   01010000b
    psrlw   xmm4,       1               ; initial v to 16384 16384 16383 16383

FAST_HEIGHT:
    mov     r12, r14
    shr     r12,    15
    imul    r12,    r5
    add     r12,    r4                 ; get current row address
    mov     r13,    r12
    add     r13,    r5

    mov     r15, 32768
    mov     rax, r2
    dec     rax

    movdqa  xmm3,       xmm5            ; initial u to 32768 32767 32768 32767

FAST_WIDTH:
    mov     r11,        r15
    shr     r11,        16

    movd    xmm1,       [r12+r11]       ; xxxxxxba
    movd    xmm2,       [r13+r11]       ; xxxxxxdc
    punpcklwd   xmm1,   xmm2            ; xxxxdcba
    punpcklbw   xmm1,   xmm0            ; 0d0c0b0a

    movdqa  xmm2,   xmm4    ; xmm2:  vv(1-v)(1-v)  tmpv
    pmulhuw xmm2,   xmm3    ; mul u(1-u)u(1-u) on xmm2
    pmaddwd     xmm2,   xmm1
    pshufd  xmm1,   xmm2,   00000001b
    paddd   xmm2,   xmm1
    movdqa  xmm1,   [add_extra_half]
    paddd   xmm2,   xmm1
    psrld   xmm2,   15

    packuswb    xmm2,   xmm0
    movd    ebx,    xmm2
    mov     [r0],  bl
    inc     r0

    add     r15, r6

    paddw   xmm3,       xmm7            ; inc u
    dec     rax
    jg      FAST_WIDTH

FAST_WIDTH_END:
    shr     r15, 16
    mov     bl,  [r12+r15]
    mov     [r0],bl
    inc     r0
    add     r14, rbp
    add     r0,  r1

    paddw   xmm4,   xmm6                ; inc v
    psllw   xmm4,   1
    psrlw   xmm4,   1

    dec     r3
    jg      FAST_HEIGHT


FAST_LAST_ROW:
    shr     r14, 15
    imul    r14, r5
    add     r4, r14
    mov     r15, 32768

FAST_LAST_ROW_WIDTH:
    mov     r11, r15
    shr     r11, 16
    mov     bl,  [r4+r11]
    mov     [r0],bl
    inc     r0

    add     r15, r6
    dec     r2
    jg      FAST_LAST_ROW_WIDTH

FAST_LAST_ROW_END:

    pop     rbp
    pop     rbx
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    ret
%endif

;***********************************************************************
;   void DyadicBilinearOneThirdDownsampler_ssse3(    unsigned char* pDst, const int iDstStride,
;                   unsigned char* pSrc, const int iSrcStride,
;                   const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearOneThirdDownsampler_ssse3
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif

    mov r6, r1             ;Save the tailer for the unasigned size
    imul r6, r5
    add r6, r0
    movdqa xmm7, [r6]

.yloops_onethird_sse3:
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif

    mov r6, r0        ;save base address
    ; each loop = source bandwidth: 48 bytes
.xloops_onethird_sse3:
    ; 1st part horizonal loop: x48 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  xmm0: F * e E * d D * c C * b B * a A
    ;               xmm2: k K * j J * i I * h H * g G * f
    ;               xmm2: * p P * o O * n N * m M * l L *
    ;
    ;2nd Line Src:  xmm2: F' *  e' E' *  d' D' *  c' C' *  b' B' *  a' A'
    ;               xmm1: k' K' *  j' J' *  i' I' *  h' H' *  g' G' *  f'
    ;               xmm1: *  p' P' *  o' O' *  n' N' *  m' M' *  l' L' *
    ;=> target:
    ;: P O N M L K J I H G F E D C B A
    ;: p o n m l k j i h g f e d c b a
    ;: P' ..                          A'
    ;: p' ..                          a'

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;1st line
    movdqa xmm0, [r2]                         ;F * e E * d D * c C * b B * a A
    movdqa xmm1, xmm0
    movdqa xmm5, [shufb_mask_onethird_low_1]
    movdqa xmm6, [shufb_mask_onethird_high_1]
    pshufb xmm0, xmm5                           ;0 0 0 0 0 0 0 0 0 0 F E D C B A -> xmm0
    pshufb xmm1, xmm6                           ;0 0 0 0 0 0 0 0 0 0 0 e d c b a -> xmm1

    movdqa xmm2, [r2+16]                      ;k K * j J * i I * h H * g G * f
    movdqa xmm3, xmm2
    movdqa xmm5, [shufb_mask_onethird_low_2]
    movdqa xmm6, [shufb_mask_onethird_high_2]
    pshufb xmm2, xmm5                           ;0 0 0 0 0 K J I H G 0 0 0 0 0 0 -> xmm2
    pshufb xmm3, xmm6                           ;0 0 0 0 0 k j i h g f 0 0 0 0 0 -> xmm3

    paddusb xmm0, xmm2                          ;0 0 0 0 0 K J I H G F E D C B A -> xmm0
    paddusb xmm1, xmm3                          ;0 0 0 0 0 k j i h g f e d c b a -> xmm1

    movdqa xmm2, [r2+32]                      ;* p P * o O * n N * m M * l L *
    movdqa xmm3, xmm2
    movdqa xmm5, [shufb_mask_onethird_low_3]
    movdqa xmm6, [shufb_mask_onethird_high_3]
    pshufb xmm2, xmm5                           ;P O N M L 0 0 0 0 0 0 0 0 0 0 0 -> xmm2
    pshufb xmm3, xmm6                           ;p o n m l 0 0 0 0 0 0 0 0 0 0 0 -> xmm3

    paddusb xmm0, xmm2                          ;P O N M L K J I H G F E D C B A -> xmm0
    paddusb xmm1, xmm3                          ;p o n m l k j i h g f e d c b a -> xmm1
    pavgb xmm0, xmm1                            ;1st line average                -> xmm0

    ;2nd line
    movdqa xmm2, [r2+r3]                      ;F' *  e' E' *  d' D' *  c' C' *  b' B' *  a' A'
    movdqa xmm3, xmm2
    movdqa xmm5, [shufb_mask_onethird_low_1]
    movdqa xmm6, [shufb_mask_onethird_high_1]
    pshufb xmm2, xmm5                           ;0 0 0 0 0 0 0 0 0 0 F' E' D' C' B' A' -> xmm2
    pshufb xmm3, xmm6                           ;0 0 0 0 0 0 0 0 0 0 0  e' d' c' b' a' -> xmm3

    movdqa xmm1, [r2+r3+16]                   ;k' K' *  j' J' *  i' I' *  h' H' *  g' G' *  f'
    movdqa xmm4, xmm1
    movdqa xmm5, [shufb_mask_onethird_low_2]
    movdqa xmm6, [shufb_mask_onethird_high_2]
    pshufb xmm1, xmm5                           ;0 0 0 0 0 K' J' I' H' G' 0  0 0 0 0 0 -> xmm1
    pshufb xmm4, xmm6                           ;0 0 0 0 0 k' j' i' h' g' f' 0 0 0 0 0 -> xmm4

    paddusb xmm2, xmm1                          ;0 0 0 0 0 K' J' I' H' G' F' E' D' C' B' A' -> xmm2
    paddusb xmm3, xmm4                          ;0 0 0 0 0 k' j' i' h' g' f' e' d' c' b' a' -> xmm3

    movdqa xmm1, [r2+r3+32]                   ; *  p' P' *  o' O' *  n' N' *  m' M' *  l' L' *
    movdqa xmm4, xmm1
    movdqa xmm5, [shufb_mask_onethird_low_3]
    movdqa xmm6, [shufb_mask_onethird_high_3]
    pshufb xmm1, xmm5                           ;P' O' N' M' L' 0 0 0 0 0 0 0 0 0 0 0 -> xmm1
    pshufb xmm4, xmm6                           ;p' o' n' m' l' 0 0 0 0 0 0 0 0 0 0 0 -> xmm4

    paddusb xmm2, xmm1                          ;P' O' N' M' L' K' J' I' H' G' F' E' D' C' B' A' -> xmm2
    paddusb xmm3, xmm4                          ;p' o' n' m' l' k' j' i' h' g' f' e' d' c' b' a' -> xmm3
    pavgb xmm2, xmm3                            ;2nd line average                                -> xmm2

    pavgb xmm0, xmm2                            ; bytes-average(1st line , 2nd line )

    ; write pDst
    movdqa [r0], xmm0                           ;write result in dst

    ; next SMB
    lea r2, [r2+48]                             ;current src address
    lea r0, [r0+16]                             ;current dst address

    sub r4, 48                                  ;xloops counter
    cmp r4, 0
    jg near .xloops_onethird_sse3

    sub r6, r0                                  ;offset = base address - current address
    lea r2, [r2+2*r3]                           ;
    lea r2, [r2+r3]                             ;
    lea r2, [r2+2*r6]                           ;current line + 3 lines
    lea r2, [r2+r6]
    lea r0, [r0+r1]
    lea r0, [r0+r6]                             ;current dst lien + 1 line

    dec r5
    jg near .yloops_onethird_sse3

    movdqa [r0], xmm7                           ;restore the tailer for the unasigned size

%ifndef X86_32
    pop r12
%endif

    POP_XMM
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret

;***********************************************************************
;   void DyadicBilinearOneThirdDownsampler_sse4(    unsigned char* pDst, const int iDstStride,
;                   unsigned char* pSrc, const int iSrcStride,
;                   const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearOneThirdDownsampler_sse4
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif

    mov r6, r1             ;Save the tailer for the unasigned size
    imul r6, r5
    add r6, r0
    movdqa xmm7, [r6]

.yloops_onethird_sse4:
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif

    mov r6, r0        ;save base address
    ; each loop = source bandwidth: 48 bytes
.xloops_onethird_sse4:
    ; 1st part horizonal loop: x48 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  xmm0: F * e E * d D * c C * b B * a A
    ;               xmm2: k K * j J * i I * h H * g G * f
    ;               xmm2: * p P * o O * n N * m M * l L *
    ;
    ;2nd Line Src:  xmm2: F' *  e' E' *  d' D' *  c' C' *  b' B' *  a' A'
    ;               xmm1: k' K' *  j' J' *  i' I' *  h' H' *  g' G' *  f'
    ;               xmm1: *  p' P' *  o' O' *  n' N' *  m' M' *  l' L' *
    ;=> target:
    ;: P O N M L K J I H G F E D C B A
    ;: p o n m l k j i h g f e d c b a
    ;: P' ..                          A'
    ;: p' ..                          a'

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;1st line
    movntdqa xmm0, [r2]                         ;F * e E * d D * c C * b B * a A
    movdqa xmm1, xmm0
    movdqa xmm5, [shufb_mask_onethird_low_1]
    movdqa xmm6, [shufb_mask_onethird_high_1]
    pshufb xmm0, xmm5                           ;0 0 0 0 0 0 0 0 0 0 F E D C B A -> xmm0
    pshufb xmm1, xmm6                           ;0 0 0 0 0 0 0 0 0 0 0 e d c b a -> xmm1

    movntdqa xmm2, [r2+16]                      ;k K * j J * i I * h H * g G * f
    movdqa xmm3, xmm2
    movdqa xmm5, [shufb_mask_onethird_low_2]
    movdqa xmm6, [shufb_mask_onethird_high_2]
    pshufb xmm2, xmm5                           ;0 0 0 0 0 K J I H G 0 0 0 0 0 0 -> xmm2
    pshufb xmm3, xmm6                           ;0 0 0 0 0 k j i h g f 0 0 0 0 0 -> xmm3

    paddusb xmm0, xmm2                          ;0 0 0 0 0 K J I H G F E D C B A -> xmm0
    paddusb xmm1, xmm3                          ;0 0 0 0 0 k j i h g f e d c b a -> xmm1

    movntdqa xmm2, [r2+32]                      ;* p P * o O * n N * m M * l L *
    movdqa xmm3, xmm2
    movdqa xmm5, [shufb_mask_onethird_low_3]
    movdqa xmm6, [shufb_mask_onethird_high_3]
    pshufb xmm2, xmm5                           ;P O N M L 0 0 0 0 0 0 0 0 0 0 0 -> xmm2
    pshufb xmm3, xmm6                           ;p o n m l 0 0 0 0 0 0 0 0 0 0 0 -> xmm3

    paddusb xmm0, xmm2                          ;P O N M L K J I H G F E D C B A -> xmm0
    paddusb xmm1, xmm3                          ;p o n m l k j i h g f e d c b a -> xmm1
    pavgb xmm0, xmm1                            ;1st line average                -> xmm0

    ;2nd line
    movntdqa xmm2, [r2+r3]                      ;F' *  e' E' *  d' D' *  c' C' *  b' B' *  a' A'
    movdqa xmm3, xmm2
    movdqa xmm5, [shufb_mask_onethird_low_1]
    movdqa xmm6, [shufb_mask_onethird_high_1]
    pshufb xmm2, xmm5                           ;0 0 0 0 0 0 0 0 0 0 F' E' D' C' B' A' -> xmm2
    pshufb xmm3, xmm6                           ;0 0 0 0 0 0 0 0 0 0 0  e' d' c' b' a' -> xmm3

    movntdqa xmm1, [r2+r3+16]                   ;k' K' *  j' J' *  i' I' *  h' H' *  g' G' *  f'
    movdqa xmm4, xmm1
    movdqa xmm5, [shufb_mask_onethird_low_2]
    movdqa xmm6, [shufb_mask_onethird_high_2]
    pshufb xmm1, xmm5                           ;0 0 0 0 0 K' J' I' H' G' 0  0 0 0 0 0 -> xmm1
    pshufb xmm4, xmm6                           ;0 0 0 0 0 k' j' i' h' g' f' 0 0 0 0 0 -> xmm4

    paddusb xmm2, xmm1                          ;0 0 0 0 0 K' J' I' H' G' F' E' D' C' B' A' -> xmm2
    paddusb xmm3, xmm4                          ;0 0 0 0 0 k' j' i' h' g' f' e' d' c' b' a' -> xmm3

    movntdqa xmm1, [r2+r3+32]                   ; *  p' P' *  o' O' *  n' N' *  m' M' *  l' L' *
    movdqa xmm4, xmm1
    movdqa xmm5, [shufb_mask_onethird_low_3]
    movdqa xmm6, [shufb_mask_onethird_high_3]
    pshufb xmm1, xmm5                           ;P' O' N' M' L' 0 0 0 0 0 0 0 0 0 0 0 -> xmm1
    pshufb xmm4, xmm6                           ;p' o' n' m' l' 0 0 0 0 0 0 0 0 0 0 0 -> xmm4

    paddusb xmm2, xmm1                          ;P' O' N' M' L' K' J' I' H' G' F' E' D' C' B' A' -> xmm2
    paddusb xmm3, xmm4                          ;p' o' n' m' l' k' j' i' h' g' f' e' d' c' b' a' -> xmm3
    pavgb xmm2, xmm3                            ;2nd line average                                -> xmm2

    pavgb xmm0, xmm2                            ; bytes-average(1st line , 2nd line )

    ; write pDst
    movdqa [r0], xmm0                           ;write result in dst

    ; next SMB
    lea r2, [r2+48]                             ;current src address
    lea r0, [r0+16]                             ;current dst address

    sub r4, 48                                  ;xloops counter
    cmp r4, 0
    jg near .xloops_onethird_sse4

    sub r6, r0                                  ;offset = base address - current address
    lea r2, [r2+2*r3]                           ;
    lea r2, [r2+r3]                             ;
    lea r2, [r2+2*r6]                           ;current line + 3 lines
    lea r2, [r2+r6]
    lea r0, [r0+r1]
    lea r0, [r0+r6]                             ;current dst lien + 1 line

    dec r5
    jg near .yloops_onethird_sse4

    movdqa [r0], xmm7                           ;restore the tailer for the unasigned size

%ifndef X86_32
    pop r12
%endif

    POP_XMM
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret

;***********************************************************************
;   void DyadicBilinearQuarterDownsampler_sse( unsigned char* pDst, const int iDstStride,
;                   unsigned char* pSrc, const int iSrcStride,
;                   const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearQuarterDownsampler_sse
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif
    sar r5, $02            ; iSrcHeight >> 2

    mov r6, r1             ;Save the tailer for the unasigned size
    imul r6, r5
    add r6, r0
    movq xmm7, [r6]

.yloops_quarter_sse:
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif

    mov r6, r0        ;save base address
    ; each loop = source bandwidth: 32 bytes
.xloops_quarter_sse:
    ; 1st part horizonal loop: x16 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  mm0: d D c C b B a A    mm1: h H g G f F e E
    ;2nd Line Src:  mm2: l L k K j J i I    mm3: p P o O n N m M
    ;
    ;=> target:
    ;: G E C A,
    ;:
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movq mm0, [r2]         ; 1st pSrc line
    movq mm1, [r2+8]       ; 1st pSrc line + 8
    movq mm2, [r2+r3]     ; 2nd pSrc line
    movq mm3, [r2+r3+8]   ; 2nd pSrc line + 8

    pshufw mm0, mm0, 0d8h    ; x X x X c C a A
    pshufw mm1, mm1, 0d8h    ; x X x X g G e E
    pshufw mm2, mm2, 0d8h    ; x X x X k K i I
    pshufw mm3, mm3, 0d8h    ; x X x X o O m M

    punpckldq mm0, mm1       ; g G e E c C a A
    punpckldq mm2, mm3       ; o O m M k K i I

    ; to handle mm0,mm2
    pshufw mm4, mm0, 0d8h       ;g G c C e E a A
    pshufw mm5, mm4, 04eh       ;e E a A g G c C
    punpcklbw mm4, mm5          ;g e G E c a C A  -> mm4
    pshufw mm4, mm4, 0d8h       ;g e c a G E C A  -> mm4

    pshufw mm5, mm2, 0d8h       ;o O k K m M i I
    pshufw mm6, mm5, 04eh       ;m M i I o O k K
    punpcklbw mm5, mm6          ;o m O M k i K I
    pshufw mm5, mm5, 0d8h       ;o m k i O M K I  -> mm5

    ; to handle mm4, mm5
    movq mm0, mm4
    punpckldq mm0, mm6          ;x x x x G E C A
    punpckhdq mm4, mm6          ;x x x x g e c a

    movq mm1, mm5
    punpckldq mm1, mm6          ;x x x x O M K I
    punpckhdq mm5, mm6          ;x x x x o m k i

    ; avg within MB horizon width (8 x 2 lines)
    pavgb mm0, mm4      ; (A+a+1)>>1, .., (H+h+1)>>1, temp_row1
    pavgb mm1, mm5      ; (I+i+1)>>1, .., (P+p+1)>>1, temp_row2
    pavgb mm0, mm1      ; (temp_row1+temp_row2+1)>>1, pending here and wait another horizonal part done then write memory once

    ; 2nd part horizonal loop: x16 bytes
    movq mm1, [r2+16]      ; 1st pSrc line + 16
    movq mm2, [r2+24]      ; 1st pSrc line + 24
    movq mm3, [r2+r3+16]  ; 2nd pSrc line + 16
    movq mm4, [r2+r3+24]  ; 2nd pSrc line + 24

    pshufw mm1, mm1, 0d8h
    pshufw mm2, mm2, 0d8h
    pshufw mm3, mm3, 0d8h
    pshufw mm4, mm4, 0d8h

    punpckldq mm1, mm2
    punpckldq mm3, mm4

    ; to handle mm1, mm3
    pshufw mm4, mm1, 0d8h
    pshufw mm5, mm4, 04eh
    punpcklbw mm4, mm5
    pshufw mm4, mm4, 0d8h

    pshufw mm5, mm3, 0d8h
    pshufw mm6, mm5, 04eh
    punpcklbw mm5, mm6
    pshufw mm5, mm5, 0d8h

    ; to handle mm4, mm5
    movq mm2, mm4
    punpckldq mm2, mm6
    punpckhdq mm4, mm6

    movq mm3, mm5
    punpckldq mm3, mm6
    punpckhdq mm5, mm6

    ; avg within MB horizon width (8 x 2 lines)
    pavgb mm2, mm4      ; (A+a+1)>>1, .., (H+h+1)>>1, temp_row1
    pavgb mm3, mm5      ; (I+i+1)>>1, .., (P+p+1)>>1, temp_row2
    pavgb mm2, mm3      ; (temp_row1+temp_row2+1)>>1, done in another 2nd horizonal part

    movd [r0  ], mm0
    movd [r0+4], mm2

    ; next SMB
    lea r2, [r2+32]
    lea r0, [r0+8]

    sub r4, 32
    cmp r4, 0
    jg near .xloops_quarter_sse

    sub  r6, r0
    ; next line
    lea r2, [r2+4*r3]    ; next 4 end of lines
    lea r2, [r2+4*r6]    ; reset to base 0 [- 4 * iDstWidth]
    lea r0, [r0+r1]
    lea r0, [r0+r6]      ; reset to base 0 [- iDstWidth]

    dec r5
    jg near .yloops_quarter_sse

    movq [r0], xmm7      ;restored the tailer for the unasigned size

    WELSEMMS
%ifndef X86_32
    pop r12
%endif
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret

;***********************************************************************
;   void DyadicBilinearQuarterDownsampler_ssse3(   unsigned char* pDst, const int iDstStride,
;                   unsigned char* pSrc, const int iSrcStride,
;                   const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearQuarterDownsampler_ssse3
    ;push ebx
    ;push edx
    ;push esi
    ;push edi
    ;push ebp

    ;mov edi, [esp+24]   ; pDst
    ;mov edx, [esp+28]   ; iDstStride
    ;mov esi, [esp+32]   ; pSrc
    ;mov ecx, [esp+36]   ; iSrcStride
    ;mov ebp, [esp+44]   ; iSrcHeight
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif
    sar r5, $02            ; iSrcHeight >> 2

    mov r6, r1             ;Save the tailer for the unasigned size
    imul r6, r5
    add r6, r0
    movq xmm7, [r6]

    movdqa xmm6, [shufb_mask_quarter]
.yloops_quarter_sse3:
    ;mov eax, [esp+40]   ; iSrcWidth
    ;sar eax, $02            ; iSrcWidth >> 2
    ;mov ebx, eax        ; iDstWidth restored at ebx
    ;sar eax, $04            ; (iSrcWidth >> 2) / 16     ; loop count = num_of_mb
    ;neg ebx             ; - (iSrcWidth >> 2)
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif

    mov r6, r0
    ; each loop = source bandwidth: 32 bytes
.xloops_quarter_sse3:
    ; 1st part horizonal loop: x32 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  xmm0: h H g G f F e E d D c C b B a A
    ;               xmm1: p P o O n N m M l L k K j J i I
    ;2nd Line Src:  xmm2: h H g G f F e E d D c C b B a A
    ;               xmm3: p P o O n N m M l L k K j J i I

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movdqa xmm0, [r2]          ; 1st_src_line
    movdqa xmm1, [r2+16]       ; 1st_src_line + 16
    movdqa xmm2, [r2+r3]       ; 2nd_src_line
    movdqa xmm3, [r2+r3+16]    ; 2nd_src_line + 16

    pshufb xmm0, xmm6           ;1st line: 0 0 0 0 g e c a 0 0 0 0 G E C A
    pshufb xmm1, xmm6           ;1st line: 0 0 0 0 o m k i 0 0 0 0 O M K I
    pshufb xmm2, xmm6           ;2nd line: 0 0 0 0 g e c a 0 0 0 0 G E C A
    pshufb xmm3, xmm6           ;2nd line: 0 0 0 0 o m k i 0 0 0 0 O M K I

    movdqa xmm4, xmm0
    movdqa xmm5, xmm2
    punpckldq xmm0, xmm1        ;1st line: 0 0 0 0 0 0 0 0 O M K I G E C A -> xmm0
    punpckhdq xmm4, xmm1        ;1st line: 0 0 0 0 0 0 0 0 o m k i g e c a -> xmm4
    punpckldq xmm2, xmm3        ;2nd line: 0 0 0 0 0 0 0 0 O M K I G E C A -> xmm2
    punpckhdq xmm5, xmm3        ;2nd line: 0 0 0 0 0 0 0 0 o m k i g e c a -> xmm5

    pavgb xmm0, xmm4
    pavgb xmm2, xmm5
    pavgb xmm0, xmm2            ;average

    ; write pDst
    movq [r0], xmm0

    ; next SMB
    lea r2, [r2+32]
    lea r0, [r0+8]

    sub r4, 32
    cmp r4, 0
    jg near .xloops_quarter_sse3

    sub r6, r0
    ; next line
    lea r2, [r2+4*r3]    ; next end of lines
    lea r2, [r2+4*r6]    ; reset to base 0 [- 4 * iDstWidth]
    lea r0, [r0+r1]
    lea r0, [r0+r6]      ; reset to base 0 [- iDstWidth]

    dec r5
    jg near .yloops_quarter_sse3

    movq [r0], xmm7      ;restored the tailer for the unasigned size

%ifndef X86_32
    pop r12
%endif

    POP_XMM
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret

;***********************************************************************
;   void DyadicBilinearQuarterDownsampler_sse4(    unsigned char* pDst, const int iDstStride,
;                   unsigned char* pSrc, const int iSrcStride,
;                   const int iSrcWidth, const int iSrcHeight );
;***********************************************************************
WELS_EXTERN DyadicBilinearQuarterDownsampler_sse4
%ifdef X86_32
    push r6
    %assign push_num 1
%else
    %assign push_num 0
%endif
    LOAD_6_PARA
    PUSH_XMM 8
    SIGN_EXTENSION r1, r1d
    SIGN_EXTENSION r3, r3d
    SIGN_EXTENSION r4, r4d
    SIGN_EXTENSION r5, r5d

%ifndef X86_32
    push r12
    mov r12, r4
%endif
    sar r5, $02            ; iSrcHeight >> 2

    mov r6, r1             ;Save the tailer for the unasigned size
    imul r6, r5
    add r6, r0
    movq xmm7, [r6]

    movdqa xmm6, [shufb_mask_quarter]    ;mask

.yloops_quarter_sse4:
%ifdef X86_32
    mov r4, arg5
%else
    mov r4, r12
%endif

    mov r6, r0
    ; each loop = source bandwidth: 32 bytes
.xloops_quarter_sse4:
    ; 1st part horizonal loop: x16 bytes
    ;               mem  hi<-       ->lo
    ;1st Line Src:  xmm0: h H g G f F e E d D c C b B a A
    ;               xmm1: p P o O n N m M l L k K j J i I
    ;2nd Line Src:  xmm2: h H g G f F e E d D c C b B a A
    ;               xmm3: p P o O n N m M l L k K j J i I

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    movntdqa xmm0, [r2]            ; 1st_src_line
    movntdqa xmm1, [r2+16]         ; 1st_src_line + 16
    movntdqa xmm2, [r2+r3]         ; 2nd_src_line
    movntdqa xmm3, [r2+r3+16]      ; 2nd_src_line + 16

    pshufb xmm0, xmm6               ;1st line: 0 0 0 0 g e c a 0 0 0 0 G E C A
    pshufb xmm1, xmm6               ;1st line: 0 0 0 0 o m k i 0 0 0 0 O M K I
    pshufb xmm2, xmm6               ;2nd line: 0 0 0 0 g e c a 0 0 0 0 G E C A
    pshufb xmm3, xmm6               ;2nd line: 0 0 0 0 o m k i 0 0 0 0 O M K I

    movdqa xmm4, xmm0
    movdqa xmm5, xmm2
    punpckldq xmm0, xmm1            ;1st line: 0 0 0 0 0 0 0 0 O M K I G E C A -> xmm0
    punpckhdq xmm4, xmm1            ;1st line: 0 0 0 0 0 0 0 0 o m k i g e c a -> xmm4
    punpckldq xmm2, xmm3            ;2nd line: 0 0 0 0 0 0 0 0 O M K I G E C A -> xmm2
    punpckhdq xmm5, xmm3            ;2nd line: 0 0 0 0 0 0 0 0 o m k i g e c a -> xmm5

    pavgb xmm0, xmm4
    pavgb xmm2, xmm5
    pavgb xmm0, xmm2                ;average

    ; write pDst
    movq [r0], xmm0

    ; next SMB
    lea r2, [r2+32]
    lea r0, [r0+8]

    sub r4, 32
    cmp r4, 0
    jg near .xloops_quarter_sse4

    sub r6, r0
    lea r2, [r2+4*r3]    ; next end of lines
    lea r2, [r2+4*r6]    ; reset to base 0 [- 2 * iDstWidth]
    lea r0, [r0+r1]
    lea r0, [r0+r6]      ; reset to base 0 [- iDstWidth]

    dec r5
    jg near .yloops_quarter_sse4

    movq [r0], xmm7      ;restore the tailer for the unasigned size

%ifndef X86_32
    pop r12
%endif

    POP_XMM
    LOAD_6_PARA_POP
%ifdef X86_32
    pop r6
%endif
    ret

; xpos_int=%1 xpos_frac=%2 inc_int+1=%3 inc_frac=%4 tmp=%5
%macro SSE2_BilinearIncXposuw 5
    movdqa          %5, %2
    paddw           %2, %4
    paddusw         %5, %4
    pcmpeqw         %5, %2
    paddb           %1, %3
    paddb           %1, %5  ; subtract 1 if no carry
%endmacro

; outl=%1 outh=%2 in=%3
%macro SSE2_UnpckXFracuw 3
    pcmpeqw         %1, %1
    pxor            %1, %3
    movdqa          %2, %1
    punpcklwd       %1, %3
    punpckhwd       %2, %3
%endmacro

; [in:xfrac out:xyfrac0]=%1 [out:xyfrac1]=%2 yfrac0=%3 yfrac1=%4
%macro SSE2_BilinearFastCalcXYFrac 4
    movdqa          %2, %1
    pmulhuw         %1, %3
    pmulhuw         %2, %4
%endmacro

; [in:dwordsl out:bytes] dwordsh=%2 zero=%3
%macro SSE2_BilinearFastPackDwordsToBytes 3
    psrld           %1, 14
    psrld           %2, 14
    packssdw        %1, %2
    pavgw           %1, %3
    packuswb        %1, %1
%endmacro

%macro SSSE3_BilinearFastDownsample2xOrLess_8px 0
    movdqa          xmm_tmp0, xmm_xpos_int
    pshufb          xmm_tmp0, xmm_0
    psubb           xmm_xpos_int, xmm_tmp0
    SSE2_UnpckXFracuw xmm_tmp0, xmm_tmp1, xmm_xpos_frac
    mov             r_tmp0, i_xpos
    lea             i_xpos, [i_xpos + 8 * i_scalex]
    shr             r_tmp0, 16
    movdqu          xmm_tmp4, [p_src_row0 + r_tmp0]
    pshufb          xmm_tmp4, xmm_xpos_int
    movdqa          xmm_tmp5, xmm_tmp4
    punpcklbw       xmm_tmp4, xmm_0
    punpckhbw       xmm_tmp5, xmm_0
    SSE2_BilinearFastCalcXYFrac xmm_tmp0, xmm_tmp2, xmm_yfrac0, xmm_yfrac1
    SSE2_BilinearFastCalcXYFrac xmm_tmp1, xmm_tmp3, xmm_yfrac0, xmm_yfrac1
    pmaddwd         xmm_tmp0, xmm_tmp4
    pmaddwd         xmm_tmp1, xmm_tmp5
    movdqu          xmm_tmp4, [p_src_row1 + r_tmp0]
    pshufb          xmm_tmp4, xmm_xpos_int
    movdqa          xmm_tmp5, xmm_tmp4
    punpcklbw       xmm_tmp4, xmm_0
    punpckhbw       xmm_tmp5, xmm_0
    pmaddwd         xmm_tmp2, xmm_tmp4
    pmaddwd         xmm_tmp3, xmm_tmp5
    paddd           xmm_tmp0, xmm_tmp2
    paddd           xmm_tmp1, xmm_tmp3
    SSE2_BilinearFastPackDwordsToBytes xmm_tmp0, xmm_tmp1, xmm_0
    movlps          [p_dst], xmm_tmp0
    add             p_dst, 8
    SSE2_BilinearIncXposuw xmm_xpos_int, xmm_xpos_frac, xmm_xpos_int_inc, xmm_xpos_frac_inc, xmm_tmp0
%endmacro

%macro SSSE3_BilinearFastDownsample4xOrLess_8px 0
    movdqa          xmm_tmp0, xmm_xpos_int
    pshufb          xmm_tmp0, [shufb_0000000088888888]
    psubb           xmm_xpos_int, xmm_tmp0
    SSE2_UnpckXFracuw xmm_tmp0, xmm_tmp1, xmm_xpos_frac
    mov             r_tmp0, i_xpos
    shr             r_tmp0, 16
    movdqu          xmm_tmp3, [p_src_row0 + r_tmp0]
    movdqu          xmm_tmp4, [p_src_row1 + r_tmp0]
    movdqa          xmm_tmp2, xmm_xpos_int
    punpcklbw       xmm_tmp2, [db80h_128]
    pshufb          xmm_tmp3, xmm_tmp2
    pshufb          xmm_tmp4, xmm_tmp2
    SSE2_BilinearFastCalcXYFrac xmm_tmp0, xmm_tmp2, xmm_yfrac0, xmm_yfrac1
    pmaddwd         xmm_tmp0, xmm_tmp3
    pmaddwd         xmm_tmp2, xmm_tmp4
    paddd           xmm_tmp0, xmm_tmp2
    lea             r_tmp0, [i_xpos + 4 * i_scalex]
    lea             i_xpos, [i_xpos + 8 * i_scalex]
    shr             r_tmp0, 16
    movdqu          xmm_tmp3, [p_src_row0 + r_tmp0]
    movdqu          xmm_tmp4, [p_src_row1 + r_tmp0]
    movdqa          xmm_tmp2, xmm_xpos_int
    punpckhbw       xmm_tmp2, [db80h_128]
    pshufb          xmm_tmp3, xmm_tmp2
    pshufb          xmm_tmp4, xmm_tmp2
    SSE2_BilinearFastCalcXYFrac xmm_tmp1, xmm_tmp2, xmm_yfrac0, xmm_yfrac1
    pmaddwd         xmm_tmp1, xmm_tmp3
    pmaddwd         xmm_tmp2, xmm_tmp4
    paddd           xmm_tmp1, xmm_tmp2
    SSE2_BilinearFastPackDwordsToBytes xmm_tmp0, xmm_tmp1, xmm_0
    movlps          [p_dst], xmm_tmp0
    add             p_dst, 8
    SSE2_BilinearIncXposuw xmm_xpos_int, xmm_xpos_frac, xmm_xpos_int_inc, xmm_xpos_frac_inc, xmm_tmp0
%endmacro

%macro SSE2_GeneralBilinearFastDownsample_8px 0
    mov             r_tmp0, i_xpos
    shr             r_tmp0, 16
    movd            xmm_tmp3, [p_src_row0 + r_tmp0]
    movd            xmm_tmp4, [p_src_row1 + r_tmp0]
    lea             r_tmp0, [i_xpos + i_scalex]
    shr             r_tmp0, 16
    pinsrw          xmm_tmp3, [p_src_row0 + r_tmp0], 1
    pinsrw          xmm_tmp4, [p_src_row1 + r_tmp0], 1
    lea             r_tmp0, [i_xpos + 2 * i_scalex]
    lea             i_xpos, [i_xpos + 4 * i_scalex]
    shr             r_tmp0, 16
    pinsrw          xmm_tmp3, [p_src_row0 + r_tmp0], 2
    pinsrw          xmm_tmp4, [p_src_row1 + r_tmp0], 2
    mov             r_tmp0, i_xpos
    sub             r_tmp0, i_scalex
    shr             r_tmp0, 16
    pinsrw          xmm_tmp3, [p_src_row0 + r_tmp0], 3
    pinsrw          xmm_tmp4, [p_src_row1 + r_tmp0], 3
    punpcklbw       xmm_tmp3, xmm_0
    punpcklbw       xmm_tmp4, xmm_0
    movdqa          xmm_tmp0, xmm_xfrac0
    SSE2_BilinearFastCalcXYFrac xmm_tmp0, xmm_tmp2, xmm_yfrac0, xmm_yfrac1
    pmaddwd         xmm_tmp0, xmm_tmp3
    pmaddwd         xmm_tmp2, xmm_tmp4
    paddd           xmm_tmp0, xmm_tmp2
    mov             r_tmp0, i_xpos
    shr             r_tmp0, 16
    movd            xmm_tmp3, [p_src_row0 + r_tmp0]
    movd            xmm_tmp4, [p_src_row1 + r_tmp0]
    lea             r_tmp0, [i_xpos + i_scalex]
    shr             r_tmp0, 16
    pinsrw          xmm_tmp3, [p_src_row0 + r_tmp0], 1
    pinsrw          xmm_tmp4, [p_src_row1 + r_tmp0], 1
    lea             r_tmp0, [i_xpos + 2 * i_scalex]
    lea             i_xpos, [i_xpos + 4 * i_scalex]
    shr             r_tmp0, 16
    pinsrw          xmm_tmp3, [p_src_row0 + r_tmp0], 2
    pinsrw          xmm_tmp4, [p_src_row1 + r_tmp0], 2
    mov             r_tmp0, i_xpos
    sub             r_tmp0, i_scalex
    shr             r_tmp0, 16
    pinsrw          xmm_tmp3, [p_src_row0 + r_tmp0], 3
    pinsrw          xmm_tmp4, [p_src_row1 + r_tmp0], 3
    punpcklbw       xmm_tmp3, xmm_0
    punpcklbw       xmm_tmp4, xmm_0
    movdqa          xmm_tmp1, xmm_xfrac1
    SSE2_BilinearFastCalcXYFrac xmm_tmp1, xmm_tmp2, xmm_yfrac0, xmm_yfrac1
    pmaddwd         xmm_tmp1, xmm_tmp3
    pmaddwd         xmm_tmp2, xmm_tmp4
    paddd           xmm_tmp1, xmm_tmp2
    SSE2_BilinearFastPackDwordsToBytes xmm_tmp0, xmm_tmp1, xmm_0
    movlps          [p_dst], xmm_tmp0
    add             p_dst, 8
    paddw           xmm_xfrac0, xmm_xfrac_inc
    paddw           xmm_xfrac1, xmm_xfrac_inc
%endmacro

; xpos_int=%1 xpos_frac=%2 inc_int=%3 inc_frac=%4 7FFFh=%5 tmp=%6
%macro SSE2_BilinearIncXposw 6
    pxor            %6, %6
    paddw           %2, %4
    pcmpgtw         %6, %2
    paddb           %1, %3
    psubb           %1, %6  ; add carry
    pand            %2, %5
%endmacro

; outl=%1 outh=%2 in=%3 7FFFh=%4
%macro SSE2_UnpckXFracw 4
    movdqa          %1, %3
    pxor            %1, %4
    movdqa          %2, %1
    punpcklwd       %1, %3
    punpckhwd       %2, %3
%endmacro

; res>>29=%1 data0=%2 data1=%3 frac0=%4 frac1=%5 tmp=%6
%macro SSE41_LinearAccurateInterpolateVerticalDwords 6
    pshufd          %1, %2, 10110001b
    pshufd          %6, %3, 10110001b
    pmuludq         %1, %4
    pmuludq         %6, %5
    paddq           %1, %6
    pmuludq         %2, %4
    pmuludq         %3, %5
    paddq           %2, %3
    psllq           %1,  3
    psrlq           %2, 29
    blendps         %1, %2, 0101b
%endmacro

%macro SSE41_BilinearAccurateDownsample2xOrLess_8px 0
    movdqa          xmm_tmp0, xmm_xpos_int
    pshufb          xmm_tmp0, xmm_0
    psubb           xmm_xpos_int, xmm_tmp0
    SSE2_UnpckXFracw xmm_tmp0, xmm_tmp1, xmm_xpos_frac, xmm_7fff
    mov             r_tmp0, i_xpos
    lea             i_xpos, [i_xpos + 8 * i_scalex]
    shr             r_tmp0, 16
    movdqu          xmm_tmp4, [p_src_row0 + r_tmp0]
    pshufb          xmm_tmp4, xmm_xpos_int
    movdqa          xmm_tmp5, xmm_tmp4
    punpcklbw       xmm_tmp4, xmm_0
    punpckhbw       xmm_tmp5, xmm_0
    pmaddwd         xmm_tmp4, xmm_tmp0
    pmaddwd         xmm_tmp5, xmm_tmp1
    movdqu          xmm_tmp2, [p_src_row1 + r_tmp0]
    pshufb          xmm_tmp2, xmm_xpos_int
    movdqa          xmm_tmp3, xmm_tmp2
    punpcklbw       xmm_tmp2, xmm_0
    punpckhbw       xmm_tmp3, xmm_0
    pmaddwd         xmm_tmp2, xmm_tmp0
    pmaddwd         xmm_tmp3, xmm_tmp1
    SSE41_LinearAccurateInterpolateVerticalDwords xmm_tmp0, xmm_tmp4, xmm_tmp2, xmm_yfrac0, xmm_yfrac1, xmm_tmp1
    SSE41_LinearAccurateInterpolateVerticalDwords xmm_tmp1, xmm_tmp5, xmm_tmp3, xmm_yfrac0, xmm_yfrac1, xmm_tmp2
    packssdw        xmm_tmp0, xmm_tmp1
    pavgw           xmm_tmp0, xmm_0
    packuswb        xmm_tmp0, xmm_tmp0
    movlps          [p_dst], xmm_tmp0
    add             p_dst, 8
    SSE2_BilinearIncXposw xmm_xpos_int, xmm_xpos_frac, xmm_xpos_int_inc, xmm_xpos_frac_inc, xmm_7fff, xmm_tmp0
%endmacro

%macro SSE41_BilinearAccurateDownsample4xOrLess_8px 0
    movdqa          xmm_tmp0, xmm_xpos_int
    pshufb          xmm_tmp0, [shufb_0000000088888888]
    psubb           xmm_xpos_int, xmm_tmp0
    SSE2_UnpckXFracw xmm_tmp0, xmm_tmp1, xmm_xpos_frac, xmm_7fff
    mov             r_tmp0, i_xpos
    shr             r_tmp0, 16
    movdqa          xmm_tmp3, xmm_xpos_int
    punpcklbw       xmm_tmp3, [db80h_128]
    movdqu          xmm_tmp4, [p_src_row0 + r_tmp0]
    movdqu          xmm_tmp2, [p_src_row1 + r_tmp0]
    lea             r_tmp0, [i_xpos + 4 * i_scalex]
    lea             i_xpos, [i_xpos + 8 * i_scalex]
    shr             r_tmp0, 16
    pshufb          xmm_tmp4, xmm_tmp3
    pshufb          xmm_tmp2, xmm_tmp3
    pmaddwd         xmm_tmp4, xmm_tmp0
    pmaddwd         xmm_tmp2, xmm_tmp0
    SSE41_LinearAccurateInterpolateVerticalDwords xmm_tmp0, xmm_tmp4, xmm_tmp2, xmm_yfrac0, xmm_yfrac1, xmm_tmp3
    movdqa          xmm_tmp2, xmm_xpos_int
    punpckhbw       xmm_tmp2, [db80h_128]
    movdqu          xmm_tmp4, [p_src_row0 + r_tmp0]
    movdqu          xmm_tmp3, [p_src_row1 + r_tmp0]
    pshufb          xmm_tmp4, xmm_tmp2
    pshufb          xmm_tmp3, xmm_tmp2
    pmaddwd         xmm_tmp4, xmm_tmp1
    pmaddwd         xmm_tmp3, xmm_tmp1
    SSE41_LinearAccurateInterpolateVerticalDwords xmm_tmp1, xmm_tmp4, xmm_tmp3, xmm_yfrac0, xmm_yfrac1, xmm_tmp2
    packssdw        xmm_tmp0, xmm_tmp1
    pavgw           xmm_tmp0, xmm_0
    packuswb        xmm_tmp0, xmm_tmp0
    movlps          [p_dst], xmm_tmp0
    add             p_dst, 8
    SSE2_BilinearIncXposw xmm_xpos_int, xmm_xpos_frac, xmm_xpos_int_inc, xmm_xpos_frac_inc, xmm_7fff, xmm_tmp0
%endmacro

%macro SSE41_GeneralBilinearAccurateDownsample_8px 0
    mov             r_tmp0, i_xpos
    shr             r_tmp0, 16
    movd            xmm_tmp4, [p_src_row0 + r_tmp0]
    movd            xmm_tmp2, [p_src_row1 + r_tmp0]
    lea             r_tmp0, [i_xpos + 1 * i_scalex]
    shr             r_tmp0, 16
    pinsrw          xmm_tmp4, [p_src_row0 + r_tmp0], 1
    pinsrw          xmm_tmp2, [p_src_row1 + r_tmp0], 1
    lea             r_tmp0, [i_xpos + 2 * i_scalex]
    lea             i_xpos, [i_xpos + 4 * i_scalex]
    shr             r_tmp0, 16
    pinsrw          xmm_tmp4, [p_src_row0 + r_tmp0], 2
    pinsrw          xmm_tmp2, [p_src_row1 + r_tmp0], 2
    mov             r_tmp0, i_xpos
    sub             r_tmp0, i_scalex
    shr             r_tmp0, 16
    pinsrw          xmm_tmp4, [p_src_row0 + r_tmp0], 3
    pinsrw          xmm_tmp2, [p_src_row1 + r_tmp0], 3
    punpcklbw       xmm_tmp4, xmm_0
    punpcklbw       xmm_tmp2, xmm_0
    pmaddwd         xmm_tmp4, xmm_xfrac0
    pmaddwd         xmm_tmp2, xmm_xfrac0
    SSE41_LinearAccurateInterpolateVerticalDwords xmm_tmp0, xmm_tmp4, xmm_tmp2, xmm_yfrac0, xmm_yfrac1, xmm_tmp3
    mov             r_tmp0, i_xpos
    shr             r_tmp0, 16
    movd            xmm_tmp4, [p_src_row0 + r_tmp0]
    movd            xmm_tmp3, [p_src_row1 + r_tmp0]
    lea             r_tmp0, [i_xpos + 1 * i_scalex]
    shr             r_tmp0, 16
    pinsrw          xmm_tmp4, [p_src_row0 + r_tmp0], 1
    pinsrw          xmm_tmp3, [p_src_row1 + r_tmp0], 1
    lea             r_tmp0, [i_xpos + 2 * i_scalex]
    lea             i_xpos, [i_xpos + 4 * i_scalex]
    shr             r_tmp0, 16
    pinsrw          xmm_tmp4, [p_src_row0 + r_tmp0], 2
    pinsrw          xmm_tmp3, [p_src_row1 + r_tmp0], 2
    mov             r_tmp0, i_xpos
    sub             r_tmp0, i_scalex
    shr             r_tmp0, 16
    pinsrw          xmm_tmp4, [p_src_row0 + r_tmp0], 3
    pinsrw          xmm_tmp3, [p_src_row1 + r_tmp0], 3
    punpcklbw       xmm_tmp4, xmm_0
    punpcklbw       xmm_tmp3, xmm_0
    pmaddwd         xmm_tmp4, xmm_xfrac1
    pmaddwd         xmm_tmp3, xmm_xfrac1
    SSE41_LinearAccurateInterpolateVerticalDwords xmm_tmp1, xmm_tmp4, xmm_tmp3, xmm_yfrac0, xmm_yfrac1, xmm_tmp2
    packssdw        xmm_tmp0, xmm_tmp1
    pavgw           xmm_tmp0, xmm_0
    packuswb        xmm_tmp0, xmm_tmp0
    movlps          [p_dst], xmm_tmp0
    add             p_dst, 8
    paddw           xmm_xfrac0, xmm_xfrac_inc
    paddw           xmm_xfrac1, xmm_xfrac_inc
    pand            xmm_xfrac0, xmm_7fff
    pand            xmm_xfrac1, xmm_7fff
%endmacro

; downsample_8px_macro=%1 b_fast=%2
%macro SSE2_GeneralBilinearDownsampler_loop 2
%%height:
    mov             p_src_row0, i_ypos
    shr             p_src_row0, 15
    imul            p_src_row0, i_src_stride
    add             p_src_row0, p_src
    mov             p_src_row1, p_src_row0
    add             p_src_row1, i_src_stride
    movd            xmm_tmp1, i_yposd
%if %2
    pshuflw         xmm_tmp1, xmm_tmp1, 0
    psllw           xmm_tmp1, 1
    psrlw           xmm_tmp1, 1
%else
    pslld           xmm_tmp1, 17
    psrld           xmm_tmp1, 17
%endif
%ifdef X86_32
    pshufd          xmm_tmp1, xmm_tmp1, 0
    pcmpeqw         xmm_tmp0, xmm_tmp0
%if %2
    psrlw           xmm_tmp0, 1
%else
    psrld           xmm_tmp0, 17
%endif
    pxor            xmm_tmp0, xmm_tmp1
    movdqa          xmm_yfrac0, xmm_tmp0
    movdqa          xmm_yfrac1, xmm_tmp1
%else
    pshufd          xmm_yfrac1, xmm_tmp1, 0
    pcmpeqw         xmm_yfrac0, xmm_yfrac0
%if %2
    psrlw           xmm_yfrac0, 1
%else
    psrld           xmm_yfrac0, 17
%endif
    pxor            xmm_yfrac0, xmm_yfrac1
%endif

    mov             i_xpos, 1 << 15
    mov             i_width_cnt, i_dst_width
    sub             i_width_cnt, 1

%ifdef xmm_xpos_int
    movdqa          xmm_xpos_int, xmm_xpos_int_begin
    movdqa          xmm_xpos_frac, xmm_xpos_frac_begin
%else
    movdqa          xmm_xfrac0, xmm_xfrac0_begin
    movdqa          xmm_xfrac1, xmm_xfrac1_begin
%endif

%%width:
    %1
    sub             i_width_cnt, 8
    jg              %%width

    lea             p_dst, [p_dst + i_width_cnt + 1]
    imul            i_width_cnt, i_scalex
    add             i_xpos, i_width_cnt
    shr             i_xpos, 16
    movzx           r_tmp0, byte [p_src_row0 + i_xpos]
    mov             [p_dst - 1], r_tmp0b
%ifdef X86_32
    mov             r_tmp0, i_scaleyd
    add             i_yposd, r_tmp0
%else
    add             i_yposd, i_scaleyd
%endif
    add             p_dst, i_dst_stride_less_width
    sub             i_dst_height, 1
    jg              %%height
%endmacro

;**************************************************************************************************************
;void GeneralBilinearFastDownsampler_ssse3 (uint8_t* pDst, int32_t iDstStride, int32_t iDstWidth,
;    int32_t iDstHeight, uint8_t* pSrc, int32_t iSrcStride, uint32_t uiScaleX,
;    uint32_t uiScaleY);
;
;**************************************************************************************************************

WELS_EXTERN GeneralBilinearFastDownsampler_ssse3
    %assign push_num 0
%ifndef X86_32
    push            r12
    push            r13
    push            rbx
    push            rbp
    %assign push_num 4
%ifdef WIN64
    push            rdi
    push            rsi
    %assign push_num push_num + 2
%endif
%endif
    LOAD_7_PARA
    PUSH_XMM 16
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r2, r2d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r5, r5d
    ZERO_EXTENSION  r6d
    sub             r1, r2                                            ; dst_stride - dst_width
%ifdef X86_32
    movd            xmm0, arg8
    movd            xmm1, esp
    and             esp, -16
    sub             esp, 8 * 4 + 7 * 16
    movd            [esp], xmm1
    %define p_dst                   r0
    %define i_dst_stride_less_width [esp + 1 * 4]
    %define i_dst_width             [esp + 2 * 4]
    %define i_dst_height            dword [esp + 3 * 4]
    %define p_src                   [esp + 4 * 4]
    %define i_src_stride            [esp + 5 * 4]
    %define i_scalex                r6
    %define i_scalexd               r6d
    %define i_scaleyd               [esp + 6 * 4]
    %define i_xpos                  r2
    %define i_ypos                  dword [esp + 7 * 4]
    %define i_yposd                 dword [esp + 7 * 4]
    %define p_src_row0              r3
    %define p_src_row1              r4
    %define i_width_cnt             r5
    %define r_tmp0                  r1
    %define r_tmp0b                 r1b
    %define xmm_xpos_frac           xmm1
    %define xmm_xpos_frac_inc       [esp + 8 * 4]
    %define xmm_xpos_int            xmm3
    %define xmm_xpos_int_inc        [esp + 8 * 4 + 1 * 16]
    %define xmm_yfrac0              [esp + 8 * 4 + 2 * 16]
    %define xmm_yfrac1              [esp + 8 * 4 + 3 * 16]
    %define xmm_tmp0                xmm7
    %define xmm_tmp1                xmm0
    %define xmm_tmp2                xmm2
    %define xmm_tmp3                xmm4
    %define xmm_tmp4                xmm5
    %define xmm_tmp5                xmm6
    %define xmm_0                   [esp + 8 * 4 + 4 * 16]
    %define xmm_xpos_int_begin      [esp + 8 * 4 + 5 * 16]
    %define xmm_xpos_frac_begin     [esp + 8 * 4 + 6 * 16]
    mov             i_dst_stride_less_width, r1
    mov             i_dst_width, r2
    mov             i_dst_height, r3
    mov             p_src, r4
    mov             i_src_stride, r5
    movd            i_scaleyd, xmm0
    pxor            xmm_tmp0, xmm_tmp0
    movdqa          xmm_0, xmm_tmp0
%else
    %define p_dst                   r0
    %define i_dst_stride_less_width r1
    %define i_dst_width             r2
    %define i_dst_height            r3
    %define p_src                   r4
    %define i_src_stride            r5
    %define i_scalex                r6
    %define i_scalexd               r6d
    %define i_scaleyd               dword arg8d
    %define i_xpos                  r12
    %define i_ypos                  r13
    %define i_yposd                 r13d
    %define p_src_row0              rbp
%ifdef WIN64
    %define p_src_row1              rsi
    %define i_width_cnt             rdi
%else
    %define p_src_row1              r11
    %define i_width_cnt             rax
%endif
    %define r_tmp0                  rbx
    %define r_tmp0b                 bl
    %define xmm_0                   xmm0
    %define xmm_xpos_frac           xmm1
    %define xmm_xpos_frac_inc       xmm8
    %define xmm_xpos_int            xmm3
    %define xmm_xpos_int_inc        xmm10
    %define xmm_yfrac0              xmm11
    %define xmm_yfrac1              xmm12
    %define xmm_tmp0                xmm7
    %define xmm_tmp1                xmm2
    %define xmm_tmp2                xmm9
    %define xmm_tmp3                xmm4
    %define xmm_tmp4                xmm5
    %define xmm_tmp5                xmm6
    %define xmm_xpos_int_begin      xmm14
    %define xmm_xpos_frac_begin     xmm15
    pxor            xmm_0, xmm_0
%endif

    sub             i_dst_height, 1
    je              .final_row
    jl              .done

    mov             i_ypos, 1 << 14
    movd            xmm_xpos_frac, i_scalexd
    pshufd          xmm_xpos_frac, xmm_xpos_frac, 0
    movdqa          xmm_tmp0, xmm_xpos_frac
    pslld           xmm_tmp0, 2
    pslldq          xmm_xpos_frac, 4
    paddd           xmm_tmp0, xmm_xpos_frac
    movdqa          xmm_tmp1, xmm_xpos_frac
    pslldq          xmm_tmp1, 4
    paddd           xmm_xpos_frac, xmm_tmp1
    paddd           xmm_tmp0, xmm_tmp1
    pslldq          xmm_tmp1, 4
    paddd           xmm_xpos_frac, xmm_tmp1
    paddd           xmm_tmp0, xmm_tmp1
    pcmpeqw         xmm_tmp1, xmm_tmp1
    psrld           xmm_tmp1, 31
    pslld           xmm_tmp1, 15
    paddd           xmm_xpos_frac, xmm_tmp1
    paddd           xmm_tmp0, xmm_tmp1
    movdqa          xmm_xpos_int, xmm_xpos_frac
    movdqa          xmm_tmp1, xmm_tmp0
    psrld           xmm_xpos_int, 16
    psrld           xmm_tmp1, 16
    packssdw        xmm_xpos_int, xmm_tmp1
    packuswb        xmm_xpos_int, xmm_xpos_int
    movdqa          xmm_tmp1, xmm_xpos_int
    pcmpeqw         xmm_tmp2, xmm_tmp2
    psubb           xmm_tmp1, xmm_tmp2
    punpcklbw       xmm_xpos_int, xmm_tmp1
    pslld           xmm_xpos_frac, 16
    pslld           xmm_tmp0, 16
    psrad           xmm_xpos_frac, 16
    psrad           xmm_tmp0, 16
    packssdw        xmm_xpos_frac, xmm_tmp0
    movd            xmm_tmp0, i_scalexd
    pslld           xmm_tmp0, 3
    movdqa          xmm_tmp1, xmm_tmp0
    punpcklwd       xmm_tmp0, xmm_tmp0
    pshufd          xmm_tmp0, xmm_tmp0, 0
    movdqa          xmm_xpos_frac_inc, xmm_tmp0
    psrld           xmm_tmp1, 16
    psubw           xmm_tmp1, xmm_tmp2
    pxor            xmm_tmp2, xmm_tmp2
    pshufb          xmm_tmp1, xmm_tmp2
    movdqa          xmm_xpos_int_inc, xmm_tmp1
    movdqa          xmm_xpos_int_begin, xmm_xpos_int
    movdqa          xmm_xpos_frac_begin, xmm_xpos_frac

    cmp             i_scalex, 4 << 16
    ja              .scalex_above4
    cmp             i_scalex, 2 << 16
    ja              .scalex_above2_beloweq4
    SSE2_GeneralBilinearDownsampler_loop SSSE3_BilinearFastDownsample2xOrLess_8px, 1
    jmp             .final_row
%ifdef X86_32
    %undef xmm_yfrac0
    %xdefine xmm_yfrac0 xmm_tmp5
    %undef xmm_tmp5
%endif
.scalex_above2_beloweq4:
    SSE2_GeneralBilinearDownsampler_loop SSSE3_BilinearFastDownsample4xOrLess_8px, 1
    jmp             .final_row
.scalex_above4:
%xdefine xmm_xfrac0 xmm_xpos_frac
%xdefine xmm_xfrac1 xmm_xpos_int
%xdefine xmm_xfrac0_begin xmm_xpos_int_begin
%xdefine xmm_xfrac1_begin xmm_xpos_frac_begin
%xdefine xmm_xfrac_inc xmm_xpos_frac_inc
%undef xmm_xpos_int
%undef xmm_xpos_frac
%undef xmm_xpos_int_begin
%undef xmm_xpos_frac_begin
%undef xmm_xpos_int_inc
%undef xmm_xpos_frac_inc
    SSE2_UnpckXFracuw xmm_tmp0, xmm_xfrac1, xmm_xfrac0
    movdqa          xmm_xfrac0, xmm_tmp0
    movdqa          xmm_xfrac0_begin, xmm_xfrac0
    movdqa          xmm_xfrac1_begin, xmm_xfrac1
    pcmpeqw         xmm_tmp0, xmm_tmp0
    pmullw          xmm_tmp0, xmm_xfrac_inc
    punpcklwd       xmm_tmp0, xmm_xfrac_inc
    movdqa          xmm_xfrac_inc, xmm_tmp0
    SSE2_GeneralBilinearDownsampler_loop SSE2_GeneralBilinearFastDownsample_8px, 1

.final_row:
    mov             p_src_row0, i_ypos
    shr             p_src_row0, 15
    imul            p_src_row0, i_src_stride
    add             p_src_row0, p_src
    mov             i_xpos, 1 << 15
    mov             i_width_cnt, i_dst_width

.final_row_width:
    mov             r_tmp0, i_xpos
    shr             r_tmp0, 16
    movzx           r_tmp0, byte [p_src_row0 + r_tmp0]
    mov             [p_dst], r_tmp0b
    add             p_dst, 1
    add             i_xpos, i_scalex
    sub             i_width_cnt, 1
    jg              .final_row_width

.done:
%ifdef X86_32
    mov             esp, [esp]
%endif
    POP_XMM
    LOAD_7_PARA_POP
%ifndef X86_32
%ifdef WIN64
    pop             rsi
    pop             rdi
%endif
    pop             rbp
    pop             rbx
    pop             r13
    pop             r12
%endif
    ret
%undef p_dst
%undef i_dst_stride_less_width
%undef i_dst_width
%undef i_dst_height
%undef p_src
%undef i_src_stride
%undef i_scalex
%undef i_scalexd
%undef i_scaleyd
%undef i_xpos
%undef i_ypos
%undef i_yposd
%undef p_src_row0
%undef p_src_row1
%undef i_width_cnt
%undef r_tmp0
%undef r_tmp0b
%undef xmm_0
%undef xmm_xpos_frac
%undef xmm_xpos_frac_inc
%undef xmm_xpos_int
%undef xmm_xpos_int_inc
%undef xmm_yfrac0
%undef xmm_yfrac1
%undef xmm_tmp0
%undef xmm_tmp1
%undef xmm_tmp2
%undef xmm_tmp3
%undef xmm_tmp4
%undef xmm_tmp5
%undef xmm_xpos_int_begin
%undef xmm_xpos_frac_begin
%undef xmm_xfrac0
%undef xmm_xfrac1
%undef xmm_xfrac0_begin
%undef xmm_xfrac1_begin
%undef xmm_xfrac_inc

;**************************************************************************************************************
;void GeneralBilinearAccurateDownsampler_sse41 (uint8_t* pDst, int32_t iDstStride, int32_t iDstWidth,
;    int32_t iDstHeight, uint8_t* pSrc, int32_t iSrcStride, uint32_t uiScaleX,
;    uint32_t uiScaleY);
;
;**************************************************************************************************************

WELS_EXTERN GeneralBilinearAccurateDownsampler_sse41
    %assign push_num 0
%ifndef X86_32
    push            r12
    push            r13
    push            rbx
    push            rbp
    %assign push_num 4
%ifdef WIN64
    push            rdi
    push            rsi
    %assign push_num push_num + 2
%endif
%endif
    LOAD_7_PARA
    PUSH_XMM 16
    SIGN_EXTENSION  r1, r1d
    SIGN_EXTENSION  r2, r2d
    SIGN_EXTENSION  r3, r3d
    SIGN_EXTENSION  r5, r5d
    ZERO_EXTENSION  r6d
    sub             r1, r2                                            ; dst_stride - dst_width
    add             r6, r6                                            ; 2 * scalex
%ifdef X86_32
    movd            xmm0, arg8
    movd            xmm1, esp
    and             esp, -16
    sub             esp, 8 * 4 + 8 * 16
    movd            [esp], xmm1
    %define p_dst                   r0
    %define i_dst_stride_less_width [esp + 1 * 4]
    %define i_dst_width             [esp + 2 * 4]
    %define i_dst_height            dword [esp + 3 * 4]
    %define p_src                   [esp + 4 * 4]
    %define i_src_stride            [esp + 5 * 4]
    %define i_scalex                r6
    %define i_scalexd               r6d
    %define i_scaleyd               [esp + 6 * 4]
    %define i_xpos                  r2
    %define i_ypos                  dword [esp + 7 * 4]
    %define i_yposd                 dword [esp + 7 * 4]
    %define p_src_row0              r3
    %define p_src_row1              r4
    %define i_width_cnt             r5
    %define r_tmp0                  r1
    %define r_tmp0b                 r1b
    %define xmm_xpos_frac           xmm1
    %define xmm_xpos_frac_inc       [esp + 8 * 4]
    %define xmm_xpos_int            xmm3
    %define xmm_xpos_int_inc        [esp + 8 * 4 + 1 * 16]
    %define xmm_yfrac0              [esp + 8 * 4 + 2 * 16]
    %define xmm_yfrac1              [esp + 8 * 4 + 3 * 16]
    %define xmm_tmp0                xmm7
    %define xmm_tmp1                xmm0
    %define xmm_tmp2                xmm2
    %define xmm_tmp3                xmm4
    %define xmm_tmp4                xmm5
    %define xmm_tmp5                xmm6
    %define xmm_0                   [esp + 8 * 4 + 4 * 16]
    %define xmm_7fff                [esp + 8 * 4 + 5 * 16]
    %define xmm_xpos_int_begin      [esp + 8 * 4 + 6 * 16]
    %define xmm_xpos_frac_begin     [esp + 8 * 4 + 7 * 16]
    mov             i_dst_stride_less_width, r1
    mov             i_dst_width, r2
    mov             i_dst_height, r3
    mov             p_src, r4
    mov             i_src_stride, r5
    movd            i_scaleyd, xmm0
    pxor            xmm_tmp5, xmm_tmp5
    movdqa          xmm_0, xmm_tmp5
    pcmpeqw         xmm_tmp5, xmm_tmp5
    psrlw           xmm_tmp5, 1
    movdqa          xmm_7fff, xmm_tmp5
%else
    %define p_dst                   r0
    %define i_dst_stride_less_width r1
    %define i_dst_width             r2
    %define i_dst_height            r3
    %define p_src                   r4
    %define i_src_stride            r5
    %define i_scalex                r6
    %define i_scalexd               r6d
    %define i_scaleyd               dword arg8d
    %define i_xpos                  r12
    %define i_ypos                  r13
    %define i_yposd                 r13d
    %define p_src_row0              rbp
%ifdef WIN64
    %define p_src_row1              rsi
    %define i_width_cnt             rdi
%else
    %define p_src_row1              r11
    %define i_width_cnt             rax
%endif
    %define r_tmp0                  rbx
    %define r_tmp0b                 bl
    %define xmm_0                   xmm0
    %define xmm_xpos_frac           xmm1
    %define xmm_xpos_frac_inc       xmm8
    %define xmm_xpos_int            xmm3
    %define xmm_xpos_int_inc        xmm10
    %define xmm_yfrac0              xmm11
    %define xmm_yfrac1              xmm12
    %define xmm_tmp0                xmm7
    %define xmm_tmp1                xmm2
    %define xmm_tmp2                xmm9
    %define xmm_tmp3                xmm4
    %define xmm_tmp4                xmm5
    %define xmm_tmp5                xmm6
    %define xmm_7fff                xmm13
    %define xmm_xpos_int_begin      xmm14
    %define xmm_xpos_frac_begin     xmm15
    pxor            xmm_0, xmm_0
    pcmpeqw         xmm_7fff, xmm_7fff
    psrlw           xmm_7fff, 1
%endif

    sub             i_dst_height, 1
    je              .final_row
    jl              .done

    mov             i_ypos, 1 << 14
    movd            xmm_xpos_frac, i_scalexd
    pshufd          xmm_xpos_frac, xmm_xpos_frac, 0
    movdqa          xmm_tmp0, xmm_xpos_frac
    pslld           xmm_tmp0, 2
    pslldq          xmm_xpos_frac, 4
    paddd           xmm_tmp0, xmm_xpos_frac
    movdqa          xmm_tmp1, xmm_xpos_frac
    pslldq          xmm_tmp1, 4
    paddd           xmm_xpos_frac, xmm_tmp1
    paddd           xmm_tmp0, xmm_tmp1
    pslldq          xmm_tmp1, 4
    paddd           xmm_xpos_frac, xmm_tmp1
    paddd           xmm_tmp0, xmm_tmp1
    pcmpeqw         xmm_tmp1, xmm_tmp1
    psrld           xmm_tmp1, 31
    pslld           xmm_tmp1, 15
    paddd           xmm_xpos_frac, xmm_tmp1
    paddd           xmm_tmp0, xmm_tmp1
    movdqa          xmm_xpos_int, xmm_xpos_frac
    movdqa          xmm_tmp1, xmm_tmp0
    psrld           xmm_xpos_int, 16
    psrld           xmm_tmp1, 16
    packssdw        xmm_xpos_int, xmm_tmp1
    packuswb        xmm_xpos_int, xmm_xpos_int
    movdqa          xmm_tmp1, xmm_xpos_int
    pcmpeqw         xmm_tmp2, xmm_tmp2
    psubb           xmm_tmp1, xmm_tmp2
    punpcklbw       xmm_xpos_int, xmm_tmp1
    pslld           xmm_xpos_frac, 16
    pslld           xmm_tmp0, 16
    psrad           xmm_xpos_frac, 16
    psrad           xmm_tmp0, 16
    packssdw        xmm_xpos_frac, xmm_tmp0
    psrlw           xmm_xpos_frac, 1
    movd            xmm_tmp0, i_scalexd
    pslld           xmm_tmp0, 3
    movdqa          xmm_tmp1, xmm_tmp0
    punpcklwd       xmm_tmp0, xmm_tmp0
    pshufd          xmm_tmp0, xmm_tmp0, 0
    psrlw           xmm_tmp0, 1
    movdqa          xmm_xpos_frac_inc, xmm_tmp0
    psrld           xmm_tmp1, 16
    pxor            xmm_tmp2, xmm_tmp2
    pshufb          xmm_tmp1, xmm_tmp2
    movdqa          xmm_xpos_int_inc, xmm_tmp1
    movdqa          xmm_xpos_int_begin, xmm_xpos_int
    movdqa          xmm_xpos_frac_begin, xmm_xpos_frac

    cmp             i_scalex, 4 << 16
    ja              .scalex_above4
    cmp             i_scalex, 2 << 16
    ja              .scalex_above2_beloweq4
    SSE2_GeneralBilinearDownsampler_loop SSE41_BilinearAccurateDownsample2xOrLess_8px, 0
    jmp             .final_row
%ifdef X86_32
    %undef xmm_yfrac0
    %xdefine xmm_yfrac0 xmm_tmp5
    %undef xmm_tmp5
%endif
.scalex_above2_beloweq4:
    SSE2_GeneralBilinearDownsampler_loop SSE41_BilinearAccurateDownsample4xOrLess_8px, 0
    jmp             .final_row
.scalex_above4:
%xdefine xmm_xfrac0 xmm_xpos_frac
%xdefine xmm_xfrac1 xmm_xpos_int
%xdefine xmm_xfrac0_begin xmm_xpos_int_begin
%xdefine xmm_xfrac1_begin xmm_xpos_frac_begin
%xdefine xmm_xfrac_inc xmm_xpos_frac_inc
%undef xmm_xpos_int
%undef xmm_xpos_frac
%undef xmm_xpos_int_begin
%undef xmm_xpos_frac_begin
%undef xmm_xpos_int_inc
%undef xmm_xpos_frac_inc
    SSE2_UnpckXFracw xmm_tmp0, xmm_xfrac1, xmm_xfrac0, xmm_7fff
    movdqa          xmm_xfrac0, xmm_tmp0
    movdqa          xmm_xfrac0_begin, xmm_xfrac0
    movdqa          xmm_xfrac1_begin, xmm_xfrac1
    pcmpeqw         xmm_tmp0, xmm_tmp0
    pmullw          xmm_tmp0, xmm_xfrac_inc
    punpcklwd       xmm_tmp0, xmm_xfrac_inc
    movdqa          xmm_xfrac_inc, xmm_tmp0
    SSE2_GeneralBilinearDownsampler_loop SSE41_GeneralBilinearAccurateDownsample_8px, 0

.final_row:
    mov             p_src_row0, i_ypos
    shr             p_src_row0, 15
    imul            p_src_row0, i_src_stride
    add             p_src_row0, p_src
    mov             i_xpos, 1 << 15
    mov             i_width_cnt, i_dst_width

.final_row_width:
    mov             r_tmp0, i_xpos
    shr             r_tmp0, 16
    movzx           r_tmp0, byte [p_src_row0 + r_tmp0]
    mov             [p_dst], r_tmp0b
    add             p_dst, 1
    add             i_xpos, i_scalex
    sub             i_width_cnt, 1
    jg              .final_row_width

.done:
%ifdef X86_32
    mov             esp, [esp]
%endif
    POP_XMM
    LOAD_7_PARA_POP
%ifndef X86_32
%ifdef WIN64
    pop             rsi
    pop             rdi
%endif
    pop             rbp
    pop             rbx
    pop             r13
    pop             r12
%endif
    ret
%undef p_dst
%undef i_dst_stride_less_width
%undef i_dst_width
%undef i_dst_height
%undef p_src
%undef i_src_stride
%undef i_scalex
%undef i_scalexd
%undef i_scaleyd
%undef i_xpos
%undef i_ypos
%undef i_yposd
%undef p_src_row0
%undef p_src_row1
%undef i_width_cnt
%undef r_tmp0
%undef r_tmp0b
%undef xmm_0
%undef xmm_xpos_frac
%undef xmm_xpos_frac_inc
%undef xmm_xpos_int
%undef xmm_xpos_int_inc
%undef xmm_yfrac0
%undef xmm_yfrac1
%undef xmm_tmp0
%undef xmm_tmp1
%undef xmm_tmp2
%undef xmm_tmp3
%undef xmm_tmp4
%undef xmm_tmp5
%undef xmm_7fff
%undef xmm_xpos_int_begin
%undef xmm_xpos_frac_begin
%undef xmm_xfrac0
%undef xmm_xfrac1
%undef xmm_xfrac0_begin
%undef xmm_xfrac1_begin
%undef xmm_xfrac_inc
