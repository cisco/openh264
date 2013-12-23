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

