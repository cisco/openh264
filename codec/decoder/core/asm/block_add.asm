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
;*  block_add.asm
;*
;*  Abstract
;*      add block
;*
;*  History
;*      09/21/2009 Created
;*
;*
;*************************************************************************/

%include  "asm_inc.asm"

BITS 32

;*******************************************************************************
; Macros and other preprocessor constants
;*******************************************************************************

%macro   BLOCK_ADD_16_SSE2   4 
	movdqa    xmm0,       [%2]
	movdqa    xmm1,       [%3]
    movdqa    xmm2,       [%3+10h]
	movdqa    xmm6,       xmm0

	punpcklbw    xmm0,    xmm7
	punpckhbw    xmm6,    xmm7

	paddw        xmm0,    xmm1
	paddw        xmm6,    xmm2

	packuswb     xmm0,    xmm6
	movdqa       [%1],    xmm0

	lea          %2,      [%2+%4]
	lea          %3,      [%3+%4*2]
	lea          %1,      [%1+%4] 
%endmacro

%macro    BLOCK_ADD_8_MMXEXT   4
    movq       mm0,       [%2]
	movq       mm1,       [%3]
	movq       mm2,       [%3+08h]
	movq       mm6,       mm0

	punpcklbw    mm0,     mm7
	punpckhbw    mm6,     mm7

	paddw        mm0,     mm1
	paddw        mm6,     mm2

	packuswb     mm0,     mm6
	movq         [%1],    mm0

	lea          %2,      [%2+%4]
	lea          %3,      [%3+%4*2]
	lea          %1,      [%1+%4]
%endmacro


%macro    BLOCK_ADD_16_STRIDE_SSE2  5
    movdqa    xmm0,       [%2]
	movdqa    xmm1,       [%3]
    movdqa    xmm2,       [%3+10h]
	movdqa    xmm6,       xmm0

	punpcklbw    xmm0,    xmm7
	punpckhbw    xmm6,    xmm7

	paddw        xmm0,    xmm1
	paddw        xmm6,    xmm2

	packuswb     xmm0,    xmm6
	movdqa       [%1],    xmm0

	lea          %2,      [%2+%4]
	lea          %3,      [%3+%5*2]
	lea          %1,      [%1+%4] 
%endmacro


%macro    BLOCK_ADD_8_STRIDE_MMXEXT   5
    movq       mm0,       [%2]
	movq       mm1,       [%3]
	movq       mm2,       [%3+08h]
	movq       mm6,       mm0

	punpcklbw    mm0,     mm7
	punpckhbw    mm6,     mm7

	paddw        mm0,     mm1
	paddw        mm6,     mm2

	packuswb     mm0,     mm6
	movq         [%1],    mm0

	lea          %2,      [%2+%4]
	lea          %3,      [%3+%5*2]
	lea          %1,      [%1+%4]
%endmacro

%macro    BLOCK_ADD_8_STRIDE_2_LINES_SSE2   5    
	movdqa xmm1, [%3]
	movq xmm0, [%2]
	punpcklbw xmm0, xmm7
	paddw xmm0, xmm1
	packuswb xmm0, xmm7
	movq [%1], xmm0	
	
	movdqa xmm3, [%3+%5*2]
	movq xmm2, [%2+%4]
	punpcklbw xmm2, xmm7
	paddw xmm2, xmm3
	packuswb xmm2, xmm7	
	movq [%1+%4], xmm2	
	
	lea %1, [%1+%4*2]
	lea %2, [%2+%4*2]
	lea %3, [%3+%5*4]	
%endmacro

%macro   CHECK_DATA_16_ZERO_SSE4     3
    mov        eax,      0h
	movdqa     xmm0,     [%1]
	movdqa     xmm1,     [%1+10h]
	mov        ebx,       [ecx]

	por		   xmm0,	 xmm1
	ptest      xmm7,     xmm0
	cmovae     eax,      %3
	
	add        %1,       20h
	add        ecx,      04h
	mov        byte [%2+ebx],  al
%endmacro

%macro  CHECK_RS_4x4_BLOCK_2_ZERO_SSE4   5
    movdqa     xmm0,      [%1]
    movdqa     xmm1,      [%1+%3]
    movdqa     xmm2,      [%1+%3*2]
    movdqa     xmm3,      [%1+%4]
    
    mov        eax,       0h
    mov        ebx,       0h
    movdqa     xmm4,      xmm0
    movdqa     xmm5,      xmm2
    
    punpcklqdq  xmm0,     xmm1
    punpckhqdq  xmm4,     xmm1
    punpcklqdq  xmm2,     xmm3
    punpckhqdq  xmm5,     xmm3

	por			xmm0,	  xmm2
	por			xmm4,	  xmm5
    
    ptest       xmm7,     xmm0
    cmovae      eax,      %5
    ptest       xmm7,     xmm4
    cmovae      ebx,      %5    
    
    mov     byte [%2],    al
    mov     byte [%2+1],  bl
%endmacro

%macro   DATA_COPY_16x2_SSE2      3
    movdqa     xmm0,    [%1]
	movdqa     xmm1,    [%1+10h]
	movdqa     xmm2,    [%1+%3]
	movdqa     xmm3,    [%1+%3+10h]

	movdqa     [%2],    xmm0
	movdqa     [%2+10h],  xmm1
	movdqa     [%2+20h],  xmm2
	movdqa     [%2+30h],  xmm3

	lea        %1,      [%1+%3*2]
	lea        %2,      [%2+40h]
%endmacro


%macro   DATA_COPY_8x4_SSE2      4
    movdqa     xmm0,         [%1]
	movdqa     xmm1,         [%1+%3]
	movdqa     xmm2,         [%1+%3*2]
	movdqa     xmm3,         [%1+%4]

	movdqa     [%2],         xmm0
	movdqa     [%2+10h],     xmm1
	movdqa     [%2+20h],     xmm2
	movdqa     [%2+30h],     xmm3

	lea        %1,           [%1+%3*4]
	lea        %2,           [%2+40h]
%endmacro


%macro   CHECK_DATA_16_ZERO_SSE2   3
    mov        eax,       0h
    movdqa     xmm0,      [%1]
    movdqa     xmm1,      [%1+10h]
    mov        ebx,       [ecx]
    
    pcmpeqw    xmm0,      xmm7
    pcmpeqw    xmm1,      xmm7
    packsswb   xmm0,      xmm1
    pmovmskb   edx,       xmm0    
    sub        edx,       0ffffh
    
    cmovb      eax,       ebp   
    add        ecx,       4
    add        %1,        20h
    mov      byte [%2+ebx],    al
%endmacro
    


%macro   CHECK_RS_4x4_BLOCK_2_ZERO_SSE2    5
    movdqa    xmm0,      [%1]
    movdqa    xmm1,      [%1 + %3]
    movdqa    xmm2,      [%1 + %3*2]
    movdqa    xmm3,      [%1 + %4]    
    
    movdqa    xmm4,       xmm0
    movdqa    xmm5,       xmm2
    
    punpcklqdq   xmm0,    xmm1
    punpckhqdq   xmm4,    xmm1
    punpcklqdq   xmm2,    xmm3
    punpckhqdq   xmm5,    xmm3
    
    pcmpeqw      xmm0,    xmm7
    pcmpeqw      xmm2,    xmm7
    pcmpeqw      xmm4,    xmm7
    pcmpeqw      xmm5,    xmm7
    
    packsswb     xmm0,    xmm2
    packsswb     xmm4,    xmm5
    pmovmskb     eax,     xmm0
    pmovmskb     ebx,     xmm4
    
    sub          eax,     0ffffh
    mov          eax,     0
    cmovb        eax,     %5
    sub          ebx,     0ffffh
    mov          ebx,     0
    cmovb        ebx,     %5
    mov       byte [%2],    al
    mov       byte [%2+1],  bl        
%endmacro

;*******************************************************************************
; Data
;*******************************************************************************

%ifdef FORMAT_COFF
SECTION .rodata data
%else
SECTION .rodata align=16
%endif

ALIGN  16
SubMbScanIdx:
     dd    0x0,  0x1,  0x4,  0x5, 
	 dd    0x2,  0x3,  0x6,  0x7,
	 dd    0x8,  0x9,  0xc,  0xd,
	 dd    0xa,  0xb,  0xe,  0xf,
	 dd    0x10, 0x11, 0x14, 0x15,
	 dd    0x12, 0x13, 0x16, 0x17,     

;*******************************************************************************
; Code
;*******************************************************************************

SECTION .text


WELS_EXTERN   WelsResBlockZero16x16_sse2

ALIGN    16
;*******************************************************************************
;  void_t WelsResBlockZero16x16_sse2(int16_t* pBlock,int32_t iStride)
;*******************************************************************************
WelsResBlockZero16x16_sse2:
    push     esi	

	mov      esi,        [esp+08h]
	mov      ecx,        [esp+0ch]	
	lea      ecx,        [ecx*2]
	lea      eax,        [ecx*3]

	pxor     xmm7,       xmm7

    ; four  lines
	movdqa   [esi],      xmm7
	movdqa   [esi+10h],  xmm7

	movdqa   [esi+ecx],  xmm7
	movdqa   [esi+ecx+10h],     xmm7

    movdqa   [esi+ecx*2],   xmm7
	movdqa   [esi+ecx*2+10h],   xmm7

	movdqa   [esi+eax],     xmm7
	movdqa   [esi+eax+10h],     xmm7

    ;  four lines
	lea      esi,       [esi+ecx*4]
	movdqa   [esi],      xmm7
	movdqa   [esi+10h],  xmm7

	movdqa   [esi+ecx],  xmm7
	movdqa   [esi+ecx+10h],     xmm7

    movdqa   [esi+ecx*2],   xmm7
	movdqa   [esi+ecx*2+10h],   xmm7

	movdqa   [esi+eax],     xmm7
	movdqa   [esi+eax+10h],     xmm7

	;  four lines
	lea      esi,       [esi+ecx*4]
	movdqa   [esi],      xmm7
	movdqa   [esi+10h],  xmm7

	movdqa   [esi+ecx],  xmm7
	movdqa   [esi+ecx+10h],     xmm7

    movdqa   [esi+ecx*2],   xmm7
	movdqa   [esi+ecx*2+10h],   xmm7

	movdqa   [esi+eax],     xmm7
	movdqa   [esi+eax+10h],     xmm7

	;  four lines
	lea      esi,       [esi+ecx*4]
	movdqa   [esi],      xmm7
	movdqa   [esi+10h],  xmm7

	movdqa   [esi+ecx],  xmm7
	movdqa   [esi+ecx+10h],     xmm7

    movdqa   [esi+ecx*2],   xmm7
	movdqa   [esi+ecx*2+10h],   xmm7

	movdqa   [esi+eax],     xmm7
	movdqa   [esi+eax+10h],     xmm7
    
    pop      esi
	ret


WELS_EXTERN   WelsResBlockZero8x8_sse2

ALIGN    16
;*******************************************************************************
;  void_t WelsResBlockZero8x8_sse2(int16_t * pBlock, int32_t iStride)
;*******************************************************************************
WelsResBlockZero8x8_sse2: 
	  push      esi

      mov       esi,     [esp+08h]
	  mov       ecx,     [esp+0ch]
	  lea       ecx,     [ecx*2]
	  lea       eax,     [ecx*3]

	  pxor      xmm7,          xmm7

	  movdqa    [esi],         xmm7
	  movdqa    [esi+ecx],     xmm7
	  movdqa    [esi+ecx*2],   xmm7
	  movdqa    [esi+eax],     xmm7

	  lea       esi,     [esi+ecx*4]
	  movdqa    [esi],         xmm7
	  movdqa    [esi+ecx],     xmm7
	  movdqa    [esi+ecx*2],   xmm7
	  movdqa    [esi+eax],     xmm7

	  
	  pop       esi
	  ret

