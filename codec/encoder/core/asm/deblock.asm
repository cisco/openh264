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
;*  deblock.asm
;*
;*  Abstract
;*      edge loop
;*
;*  History
;*      08/07/2009 Created
;*
;*
;*************************************************************************/
%include "asm_inc.asm"
BITS 32

;*******************************************************************************
; Macros and other preprocessor constants
;*******************************************************************************

%ifdef FORMAT_COFF
SECTION .rodata pData
%else
SECTION .rodata align=16
%endif

SECTION .text

;********************************************************************************
;  void DeblockChromaEq4V_sse2(uint8_t * pPixCb, uint8_t * pPixCr, int32_t iStride,
;                             int32_t iAlpha, int32_t iBeta)
;********************************************************************************
WELS_EXTERN   DeblockChromaEq4V_sse2

ALIGN  16
DeblockChromaEq4V_sse2:
  push        ebp  
  mov         ebp,esp 
  and         esp,0FFFFFFF0h 
  sub         esp,68h 
  mov         edx,[ebp+10h]      ;  iStride
  mov         eax,[ebp+8]        ;  pPixCb
  mov         ecx,[ebp+0Ch]      ;  pPixCr
  movq        xmm4,[ecx] 
  movq        xmm5,[edx+ecx] 
  push        esi  
  push        edi  
  lea         esi,[edx+edx] 
  mov         edi,eax 
  sub         edi,esi 
  movq        xmm1,[edi] 
  mov         edi,ecx 
  sub         edi,esi 
  movq        xmm2,[edi] 
  punpcklqdq  xmm1,xmm2 
  mov         esi,eax 
  sub         esi,edx 
  movq        xmm2,[esi] 
  mov         edi,ecx 
  sub         edi,edx 
  movq        xmm3,[edi] 
  punpcklqdq  xmm2,xmm3 
  movq        xmm3,[eax] 
  punpcklqdq  xmm3,xmm4 
  movq        xmm4,[edx+eax] 
  mov       edx, [ebp + 14h] 
  punpcklqdq  xmm4,xmm5 
  movd        xmm5,edx 
  mov       edx, [ebp + 18h] 
  pxor        xmm0,xmm0 
  movdqa      xmm6,xmm5 
  punpcklwd   xmm6,xmm5 
  pshufd      xmm5,xmm6,0 
  movd        xmm6,edx 
  movdqa      xmm7,xmm6 
  punpcklwd   xmm7,xmm6 
  pshufd      xmm6,xmm7,0 
  movdqa      xmm7,xmm1 
  punpckhbw   xmm1,xmm0 
  punpcklbw   xmm7,xmm0 
  movdqa      [esp+40h],xmm1 
  movdqa      [esp+60h],xmm7 
  movdqa      xmm7,xmm2 
  punpcklbw   xmm7,xmm0 
  movdqa      [esp+10h],xmm7 
  movdqa      xmm7,xmm3 
  punpcklbw   xmm7,xmm0 
  punpckhbw   xmm3,xmm0 
  movdqa      [esp+50h],xmm7 
  movdqa      xmm7,xmm4 
  punpckhbw   xmm4,xmm0 
  punpckhbw   xmm2,xmm0 
  punpcklbw   xmm7,xmm0 
  movdqa      [esp+30h],xmm3 
  movdqa      xmm3,[esp+10h] 
  movdqa      xmm1,xmm3 
  psubw       xmm1,[esp+50h] 
  pabsw       xmm1,xmm1 
  movdqa      [esp+20h],xmm4 
  movdqa      xmm0,xmm5 
  pcmpgtw     xmm0,xmm1 
  movdqa      xmm1,[esp+60h] 
  psubw       xmm1,xmm3 
  pabsw       xmm1,xmm1 
  movdqa      xmm4,xmm6 
  pcmpgtw     xmm4,xmm1 
  pand        xmm0,xmm4 
  movdqa      xmm1,xmm7 
  psubw       xmm1,[esp+50h] 
  pabsw       xmm1,xmm1 
  movdqa      xmm4,xmm6 
  pcmpgtw     xmm4,xmm1 
  movdqa      xmm1,xmm2 
  psubw       xmm1,[esp+30h] 
  pabsw       xmm1,xmm1 
  pcmpgtw     xmm5,xmm1 
  movdqa      xmm1,[esp+40h] 
  pand        xmm0,xmm4 
  psubw       xmm1,xmm2 
  pabsw       xmm1,xmm1 
  movdqa      xmm4,xmm6 
  pcmpgtw     xmm4,xmm1 
  movdqa      xmm1,[esp+20h] 
  psubw       xmm1,[esp+30h] 
  pand        xmm5,xmm4 
  pabsw       xmm1,xmm1 
  pcmpgtw     xmm6,xmm1 
  pand        xmm5,xmm6 
  mov         edx,2 
  movsx       edx,dx 
  movd        xmm1,edx 
  movdqa      xmm4,xmm1 
  punpcklwd   xmm4,xmm1 
  pshufd      xmm1,xmm4,0 
  movdqa      xmm4,[esp+60h] 
  movdqa      xmm6,xmm4 
  paddw       xmm6,xmm4 
  paddw       xmm6,xmm3 
  paddw       xmm6,xmm7 
  movdqa      [esp+10h],xmm1 
  paddw       xmm6,[esp+10h] 
  psraw       xmm6,2 
  movdqa      xmm4,xmm0 
  pandn       xmm4,xmm3 
  movdqa      xmm3,[esp+40h] 
  movdqa      xmm1,xmm0 
  pand        xmm1,xmm6 
  por         xmm1,xmm4 
  movdqa      xmm6,xmm3 
  paddw       xmm6,xmm3 
  movdqa      xmm3,[esp+10h] 
  paddw       xmm6,xmm2 
  paddw       xmm6,[esp+20h] 
  paddw       xmm6,xmm3 
  psraw       xmm6,2 
  movdqa      xmm4,xmm5 
  pand        xmm4,xmm6 
  movdqa      xmm6,xmm5 
  pandn       xmm6,xmm2 
  por         xmm4,xmm6 
  packuswb    xmm1,xmm4 
  movdqa      xmm4,[esp+50h] 
  movdqa      xmm6,xmm7 
  paddw       xmm6,xmm7 
  paddw       xmm6,xmm4 
  paddw       xmm6,[esp+60h] 
  paddw       xmm6,xmm3 
  psraw       xmm6,2 
  movdqa      xmm2,xmm0 
  pand        xmm2,xmm6 
  pandn       xmm0,xmm4 
  por         xmm2,xmm0 
  movdqa      xmm0,[esp+20h] 
  movdqa      xmm6,xmm0 
  paddw       xmm6,xmm0 
  movdqa      xmm0,[esp+30h] 
  paddw       xmm6,xmm0 
  paddw       xmm6,[esp+40h] 
  movdqa      xmm4,xmm5 
  paddw       xmm6,xmm3 
  movq        [esi],xmm1 
  psraw       xmm6,2 
  pand        xmm4,xmm6 
  pandn       xmm5,xmm0 
  por         xmm4,xmm5 
  packuswb    xmm2,xmm4 
  movq        [eax],xmm2 
  psrldq      xmm1,8 
  movq        [edi],xmm1 
  pop         edi  
  psrldq      xmm2,8 
  movq        [ecx],xmm2 
  pop         esi  
  mov         esp,ebp 
  pop         ebp  
  ret              

;******************************************************************************
; void DeblockChromaLt4V_sse2(uint8_t * pPixCb, uint8_t * pPixCr, int32_t iStride, 
;                           int32_t iAlpha, int32_t iBeta, int8_t * pTC);
;*******************************************************************************

WELS_EXTERN  DeblockChromaLt4V_sse2

DeblockChromaLt4V_sse2:
  push        ebp  
  mov         ebp,esp 
  and         esp,0FFFFFFF0h 
  sub         esp,0E4h 
  push        ebx  
  push        esi  
  mov         esi, [ebp+1Ch]      ;  pTC
  movsx       ebx, byte [esi+2] 
  push        edi  
  movsx       di,byte [esi+3] 
  mov         word [esp+0Ch],bx 
  movsx       bx,byte  [esi+1] 
  movsx       esi,byte  [esi] 
  mov         word  [esp+0Eh],si 
  movzx       esi,di 
  movd        xmm1,esi 
  movzx       esi,di 
  movd        xmm2,esi 
  mov         si,word  [esp+0Ch] 
  mov         edx, [ebp + 10h] 
  mov         eax, [ebp + 08h] 
  movzx       edi,si 
  movzx       esi,si 
  mov         ecx, [ebp + 0Ch] 
  movd        xmm4,esi 
  movzx       esi,bx 
  movd        xmm5,esi 
  movd        xmm3,edi 
  movzx       esi,bx 
  movd        xmm6,esi 
  mov         si,word [esp+0Eh] 
  movzx       edi,si 
  movzx       esi,si 
  punpcklwd   xmm6,xmm2 
  pxor        xmm0,xmm0 
  movdqa      [esp+40h],xmm0 
  movd        xmm7,edi 
  movd        xmm0,esi 
  lea         esi,[edx+edx] 
  mov         edi,eax 
  sub         edi,esi 
  punpcklwd   xmm5,xmm1 
  movdqa      xmm1,[esp+40h] 
  punpcklwd   xmm0,xmm4 
  movq        xmm4,[edx+ecx] 
  punpcklwd   xmm7,xmm3 
  movq        xmm3,[eax] 
  punpcklwd   xmm0,xmm6 
  movq        xmm6,[edi] 
  punpcklwd   xmm7,xmm5 
  punpcklwd   xmm0,xmm7 
  mov         edi,ecx 
  sub         edi,esi 
  movdqa      xmm2,xmm1 
  psubw       xmm2,xmm0 
  movdqa      [esp+60h],xmm2 
  movq        xmm2, [edi] 
  punpcklqdq  xmm6,xmm2 
  mov         esi,eax 
  sub         esi,edx 
  movq        xmm7,[esi] 
  mov         edi,ecx 
  sub         edi,edx 
  movq        xmm2,[edi] 
  punpcklqdq  xmm7,xmm2 
  movq        xmm2,[ecx] 
  punpcklqdq  xmm3,xmm2 
  movq        xmm2,[edx+eax] 
  movsx       edx,word [ebp + 14h] 
  punpcklqdq  xmm2,xmm4 
  movdqa      [esp+0E0h],xmm2 
  movd        xmm2,edx 
  movsx       edx,word [ebp + 18h] 
  movdqa      xmm4,xmm2 
  punpcklwd   xmm4,xmm2 
  movd        xmm2,edx 
  movdqa      xmm5,xmm2 
  punpcklwd   xmm5,xmm2 
  pshufd      xmm2,xmm5,0 
  movdqa      [esp+50h],xmm2 
  movdqa      xmm2,xmm6 
  punpcklbw   xmm2,xmm1 
  movdqa      [esp+0D0h],xmm3 
  pshufd      xmm4,xmm4,0 
  movdqa      [esp+30h],xmm2 
  punpckhbw   xmm6,xmm1 
  movdqa      [esp+80h],xmm6 
  movdqa      xmm6,[esp+0D0h] 
  punpckhbw   xmm6,xmm1 
  movdqa      [esp+70h],xmm6 
  movdqa      xmm6, [esp+0E0h] 
  punpckhbw   xmm6,xmm1 
  movdqa     [esp+90h],xmm6 
  movdqa      xmm5, [esp+0E0h] 
  movdqa      xmm2,xmm7 
  punpckhbw   xmm7,xmm1 
  punpcklbw   xmm5,xmm1 
  movdqa       [esp+0A0h],xmm7 
  punpcklbw   xmm3,xmm1 
  mov         edx,4 
  punpcklbw   xmm2,xmm1 
  movsx       edx,dx 
  movd        xmm6,edx 
  movdqa      xmm7,xmm6 
  punpcklwd   xmm7,xmm6 
  pshufd      xmm6,xmm7,0 
  movdqa      xmm7,[esp+30h] 
  movdqa      [esp+20h],xmm6 
  psubw       xmm7,xmm5 
  movdqa      xmm6,xmm0 
  pcmpgtw     xmm6,xmm1 
  movdqa      xmm1,[esp+60h] 
  movdqa      [esp+40h],xmm6 
  movdqa      xmm6,xmm3 
  psubw       xmm6,xmm2 
  psllw       xmm6,2 
  paddw       xmm6,xmm7 
  paddw       xmm6, [esp+20h] 
  movdqa      xmm7, [esp+50h] 
  psraw       xmm6,3 
  pmaxsw      xmm1,xmm6 
  movdqa      [esp+10h],xmm0 
  movdqa      xmm6, [esp+10h] 
  pminsw      xmm6,xmm1 
  movdqa      [esp+10h],xmm6 
  movdqa      xmm1,xmm2 
  psubw       xmm1,xmm3 
  pabsw       xmm1,xmm1 
  movdqa      xmm6,xmm4 
  pcmpgtw     xmm6,xmm1 
  movdqa      xmm1, [esp+30h] 
  psubw       xmm1,xmm2 
  pabsw       xmm1,xmm1 
  pcmpgtw     xmm7,xmm1 
  movdqa      xmm1,[esp+50h] 
  pand        xmm6,xmm7 
  movdqa      xmm7,[esp+50h] 
  psubw       xmm5,xmm3 
  pabsw       xmm5,xmm5 
  pcmpgtw     xmm1,xmm5 
  movdqa      xmm5,[esp+80h] 
  psubw       xmm5,[esp+90h] 
  pand        xmm6,xmm1 
  pand        xmm6,[esp+40h] 
  movdqa      xmm1,[esp+10h] 
  pand        xmm1,xmm6 
  movdqa      xmm6,[esp+70h] 
  movdqa      [esp+30h],xmm1 
  movdqa      xmm1,[esp+0A0h] 
  psubw       xmm6,xmm1 
  psllw       xmm6,2 
  paddw       xmm6,xmm5 
  paddw       xmm6,[esp+20h] 
  movdqa      xmm5,[esp+60h] 
  psraw       xmm6,3 
  pmaxsw      xmm5,xmm6 
  pminsw      xmm0,xmm5 
  movdqa      xmm5,[esp+70h] 
  movdqa      xmm6,xmm1 
  psubw       xmm6,xmm5 
  pabsw       xmm6,xmm6 
  pcmpgtw     xmm4,xmm6 
  movdqa      xmm6,[esp+80h] 
  psubw       xmm6,xmm1 
  pabsw       xmm6,xmm6 
  pcmpgtw     xmm7,xmm6 
  movdqa      xmm6,[esp+90h] 
  pand        xmm4,xmm7 
  movdqa      xmm7,[esp+50h] 
  psubw       xmm6,xmm5 
  pabsw       xmm6,xmm6 
  pcmpgtw     xmm7,xmm6 
  pand        xmm4,xmm7 
  pand        xmm4,[esp+40h] 
  pand        xmm0,xmm4 
  movdqa      xmm4,[esp+30h] 
  paddw       xmm2,xmm4 
  paddw       xmm1,xmm0 
  packuswb    xmm2,xmm1 
  movq        [esi],xmm2 
  psubw       xmm3,xmm4 
  psubw       xmm5,xmm0 
  packuswb    xmm3,xmm5 
  movq        [eax],xmm3 
  psrldq      xmm2,8 
  movq        [edi],xmm2 
  pop         edi  
  pop         esi  
  psrldq      xmm3,8 
  movq        [ecx],xmm3 
  pop         ebx  
  mov         esp,ebp 
  pop         ebp  
  ret    
  
;***************************************************************************
;  void DeblockChromaEq4H_sse2(uint8_t * pPixCb, uint8_t * pPixCr, int32_t iStride, 
;          int32_t iAlpha, int32_t iBeta)
;***************************************************************************

WELS_EXTERN     DeblockChromaEq4H_sse2

ALIGN  16
  
DeblockChromaEq4H_sse2:
  push        ebp  
  mov         ebp,esp 
  and         esp,0FFFFFFF0h 
  sub         esp,0C8h  
  mov         ecx,dword [ebp+8] 
  mov         edx,dword [ebp+0Ch] 
  mov         eax,dword [ebp+10h] 
  sub         ecx,2 
  sub         edx,2 
  push        esi  
  lea         esi,[eax+eax*2] 
  mov         dword [esp+18h],ecx 
  mov         dword [esp+4],edx 
  lea         ecx,[ecx+eax*4] 
  lea         edx,[edx+eax*4] 
  lea         eax,[esp+7Ch] 
  push        edi  
  mov         dword [esp+14h],esi 
  mov         dword [esp+18h],ecx 
  mov         dword [esp+0Ch],edx 
  mov         dword [esp+10h],eax 
  mov         esi,dword [esp+1Ch] 
  mov         ecx,dword [ebp+10h] 
  mov         edx,dword [esp+14h] 
  movd        xmm0,dword [esi] 
  movd        xmm1,dword [esi+ecx] 
  movd        xmm2,dword [esi+ecx*2] 
  movd        xmm3,dword [esi+edx] 
  mov         esi,dword  [esp+8] 
  movd        xmm4,dword [esi] 
  movd        xmm5,dword [esi+ecx] 
  movd        xmm6,dword [esi+ecx*2] 
  movd        xmm7,dword [esi+edx] 
  punpckldq   xmm0,xmm4 
  punpckldq   xmm1,xmm5 
  punpckldq   xmm2,xmm6 
  punpckldq   xmm3,xmm7 
  mov         esi,dword [esp+18h] 
  mov         edi,dword [esp+0Ch] 
  movd        xmm4,dword [esi] 
  movd        xmm5,dword [edi] 
  punpckldq   xmm4,xmm5 
  punpcklqdq  xmm0,xmm4 
  movd        xmm4,dword [esi+ecx] 
  movd        xmm5,dword [edi+ecx] 
  punpckldq   xmm4,xmm5 
  punpcklqdq  xmm1,xmm4 
  movd        xmm4,dword [esi+ecx*2] 
  movd        xmm5,dword [edi+ecx*2] 
  punpckldq   xmm4,xmm5 
  punpcklqdq  xmm2,xmm4 
  movd        xmm4,dword [esi+edx] 
  movd        xmm5,dword [edi+edx] 
  punpckldq   xmm4,xmm5 
  punpcklqdq  xmm3,xmm4 
  movdqa      xmm6,xmm0 
  punpcklbw   xmm0,xmm1 
  punpckhbw   xmm6,xmm1 
  movdqa      xmm7,xmm2 
  punpcklbw   xmm2,xmm3 
  punpckhbw   xmm7,xmm3 
  movdqa      xmm4,xmm0 
  movdqa      xmm5,xmm6 
  punpcklwd   xmm0,xmm2 
  punpckhwd   xmm4,xmm2 
  punpcklwd   xmm6,xmm7 
  punpckhwd   xmm5,xmm7 
  movdqa      xmm1,xmm0 
  movdqa      xmm2,xmm4 
  punpckldq   xmm0,xmm6 
  punpckhdq   xmm1,xmm6 
  punpckldq   xmm4,xmm5 
  punpckhdq   xmm2,xmm5 
  movdqa      xmm5,xmm0 
  movdqa      xmm6,xmm1 
  punpcklqdq  xmm0,xmm4 
  punpckhqdq  xmm5,xmm4 
  punpcklqdq  xmm1,xmm2 
  punpckhqdq  xmm6,xmm2 
  mov         edi,dword [esp+10h] 
  movdqa      [edi],xmm0 
  movdqa      [edi+10h],xmm5 
  movdqa      [edi+20h],xmm1 
  movdqa      [edi+30h],xmm6 
  movsx       ecx,word [ebp+14h] 
  movsx       edx,word [ebp+18h] 
  movdqa      xmm6,[esp+80h] 
  movdqa      xmm4,[esp+90h] 
  movdqa      xmm5,[esp+0A0h] 
  movdqa      xmm7,[esp+0B0h] 
  pxor        xmm0,xmm0 
  movd        xmm1,ecx 
  movdqa      xmm2,xmm1 
  punpcklwd   xmm2,xmm1 
  pshufd      xmm1,xmm2,0 
  movd        xmm2,edx 
  movdqa      xmm3,xmm2 
  punpcklwd   xmm3,xmm2 
  pshufd      xmm2,xmm3,0 
  movdqa      xmm3,xmm6 
  punpckhbw   xmm6,xmm0 
  movdqa      [esp+60h],xmm6 
  movdqa      xmm6,[esp+90h] 
  punpckhbw   xmm6,xmm0 
  movdqa      [esp+30h],xmm6 
  movdqa      xmm6,[esp+0A0h] 
  punpckhbw   xmm6,xmm0 
  movdqa      [esp+40h],xmm6 
  movdqa      xmm6,[esp+0B0h] 
  punpckhbw   xmm6,xmm0 
  movdqa      [esp+70h],xmm6 
  punpcklbw   xmm7,xmm0 
  punpcklbw   xmm4,xmm0 
  punpcklbw   xmm5,xmm0 
  punpcklbw   xmm3,xmm0 
  movdqa      [esp+50h],xmm7 
  movdqa      xmm6,xmm4 
  psubw       xmm6,xmm5 
  pabsw       xmm6,xmm6 
  movdqa      xmm0,xmm1 
  pcmpgtw     xmm0,xmm6 
  movdqa      xmm6,xmm3 
  psubw       xmm6,xmm4 
  pabsw       xmm6,xmm6 
  movdqa      xmm7,xmm2 
  pcmpgtw     xmm7,xmm6 
  movdqa      xmm6,[esp+50h] 
  psubw       xmm6,xmm5 
  pabsw       xmm6,xmm6 
  pand        xmm0,xmm7 
  movdqa      xmm7,xmm2 
  pcmpgtw     xmm7,xmm6 
  movdqa      xmm6,[esp+30h] 
  psubw       xmm6,[esp+40h] 
  pabsw       xmm6,xmm6 
  pcmpgtw     xmm1,xmm6 
  movdqa      xmm6,[esp+60h] 
  psubw       xmm6,[esp+30h] 
  pabsw       xmm6,xmm6 
  pand        xmm0,xmm7 
  movdqa      xmm7,xmm2 
  pcmpgtw     xmm7,xmm6 
  movdqa      xmm6,[esp+70h] 
  psubw       xmm6,[esp+40h] 
  pabsw       xmm6,xmm6 
  pand        xmm1,xmm7 
  pcmpgtw     xmm2,xmm6 
  pand        xmm1,xmm2 
  mov         eax,2 
  movsx       ecx,ax 
  movd        xmm2,ecx 
  movdqa      xmm6,xmm2 
  punpcklwd   xmm6,xmm2 
  pshufd      xmm2,xmm6,0 
  movdqa      [esp+20h],xmm2 
  movdqa      xmm2,xmm3 
  paddw       xmm2,xmm3 
  paddw       xmm2,xmm4 
  paddw       xmm2,[esp+50h] 
  paddw       xmm2,[esp+20h] 
  psraw       xmm2,2 
  movdqa      xmm6,xmm0 
  pand        xmm6,xmm2 
  movdqa      xmm2,xmm0 
  pandn       xmm2,xmm4 
  por         xmm6,xmm2 
  movdqa      xmm2,[esp+60h] 
  movdqa      xmm7,xmm2 
  paddw       xmm7,xmm2 
  paddw       xmm7,[esp+30h] 
  paddw       xmm7,[esp+70h] 
  paddw       xmm7,[esp+20h] 
  movdqa      xmm4,xmm1 
  movdqa      xmm2,xmm1 
  pandn       xmm2,[esp+30h] 
  psraw       xmm7,2 
  pand        xmm4,xmm7 
  por         xmm4,xmm2 
  movdqa      xmm2,[esp+50h] 
  packuswb    xmm6,xmm4 
  movdqa      [esp+90h],xmm6 
  movdqa      xmm6,xmm2 
  paddw       xmm6,xmm2 
  movdqa      xmm2,[esp+20h] 
  paddw       xmm6,xmm5 
  paddw       xmm6,xmm3 
  movdqa      xmm4,xmm0 
  pandn       xmm0,xmm5 
  paddw       xmm6,xmm2 
  psraw       xmm6,2 
  pand        xmm4,xmm6 
  por         xmm4,xmm0 
  movdqa      xmm0,[esp+70h] 
  movdqa      xmm5,xmm0 
  paddw       xmm5,xmm0 
  movdqa      xmm0,[esp+40h] 
  paddw       xmm5,xmm0 
  paddw       xmm5,[esp+60h] 
  movdqa      xmm3,xmm1 
  paddw       xmm5,xmm2 
  psraw       xmm5,2 
  pand        xmm3,xmm5 
  pandn       xmm1,xmm0 
  por         xmm3,xmm1 
  packuswb    xmm4,xmm3 
  movdqa      [esp+0A0h],xmm4 
  mov         esi,dword [esp+10h] 
  movdqa      xmm0,[esi] 
  movdqa      xmm1,[esi+10h] 
  movdqa      xmm2,[esi+20h] 
  movdqa      xmm3,[esi+30h] 
  movdqa      xmm6,xmm0 
  punpcklbw   xmm0,xmm1 
  punpckhbw   xmm6,xmm1 
  movdqa      xmm7,xmm2 
  punpcklbw   xmm2,xmm3 
  punpckhbw   xmm7,xmm3 
  movdqa      xmm4,xmm0 
  movdqa      xmm5,xmm6 
  punpcklwd   xmm0,xmm2 
  punpckhwd   xmm4,xmm2 
  punpcklwd   xmm6,xmm7 
  punpckhwd   xmm5,xmm7 
  movdqa      xmm1,xmm0 
  movdqa      xmm2,xmm4 
  punpckldq   xmm0,xmm6 
  punpckhdq   xmm1,xmm6 
  punpckldq   xmm4,xmm5 
  punpckhdq   xmm2,xmm5 
  movdqa      xmm5,xmm0 
  movdqa      xmm6,xmm1 
  punpcklqdq  xmm0,xmm4 
  punpckhqdq  xmm5,xmm4 
  punpcklqdq  xmm1,xmm2 
  punpckhqdq  xmm6,xmm2 
  mov         esi,dword [esp+1Ch] 
  mov         ecx,dword [ebp+10h] 
  mov         edx,dword [esp+14h] 
  mov         edi,dword [esp+8] 
  movd        dword [esi],xmm0 
  movd        dword [esi+ecx],xmm5 
  movd        dword [esi+ecx*2],xmm1 
  movd        dword [esi+edx],xmm6 
  psrldq      xmm0,4 
  psrldq      xmm5,4 
  psrldq      xmm1,4 
  psrldq      xmm6,4 
  mov         esi,dword [esp+18h] 
  movd        dword [edi],xmm0 
  movd        dword [edi+ecx],xmm5 
  movd        dword [edi+ecx*2],xmm1 
  movd        dword [edi+edx],xmm6 
  psrldq      xmm0,4 
  psrldq      xmm5,4 
  psrldq      xmm1,4 
  psrldq      xmm6,4 
  movd        dword [esi],xmm0 
  movd        dword [esi+ecx],xmm5 
  movd        dword [esi+ecx*2],xmm1 
  movd        dword [esi+edx],xmm6 
  psrldq      xmm0,4 
  psrldq      xmm5,4 
  psrldq      xmm1,4 
  psrldq      xmm6,4 
  mov         edi,dword [esp+0Ch] 
  movd        dword [edi],xmm0 
  movd        dword [edi+ecx],xmm5 
  movd        dword [edi+ecx*2],xmm1 
  movd        dword [edi+edx],xmm6 
  pop         edi  
  pop         esi  
  mov         esp,ebp 
  pop         ebp  
  ret              
  
;*******************************************************************************
;    void DeblockChromaLt4H_sse2(uint8_t * pPixCb, uint8_t * pPixCr, int32_t iStride, 
;                                int32_t iAlpha, int32_t iBeta, int8_t * pTC);
;*******************************************************************************
  
WELS_EXTERN  DeblockChromaLt4H_sse2
  
ALIGN  16

DeblockChromaLt4H_sse2:
  push        ebp  
  mov         ebp,esp 
  and         esp,0FFFFFFF0h 
  sub         esp,108h   
  mov         ecx,dword [ebp+8] 
  mov         edx,dword [ebp+0Ch] 
  mov         eax,dword [ebp+10h] 
  sub         ecx,2 
  sub         edx,2 
  push        esi  
  lea         esi,[eax+eax*2] 
  mov         dword [esp+10h],ecx 
  mov         dword [esp+4],edx 
  lea         ecx,[ecx+eax*4] 
  lea         edx,[edx+eax*4] 
  lea         eax,[esp+6Ch] 
  push        edi  
  mov         dword [esp+0Ch],esi 
  mov         dword [esp+18h],ecx 
  mov         dword [esp+10h],edx 
  mov         dword [esp+1Ch],eax 
  mov         esi,dword [esp+14h] 
  mov         ecx,dword [ebp+10h] 
  mov         edx,dword [esp+0Ch] 
  movd        xmm0,dword [esi] 
  movd        xmm1,dword [esi+ecx] 
  movd        xmm2,dword [esi+ecx*2] 
  movd        xmm3,dword [esi+edx] 
  mov         esi,dword [esp+8] 
  movd        xmm4,dword [esi] 
  movd        xmm5,dword [esi+ecx] 
  movd        xmm6,dword [esi+ecx*2] 
  movd        xmm7,dword [esi+edx] 
  punpckldq   xmm0,xmm4 
  punpckldq   xmm1,xmm5 
  punpckldq   xmm2,xmm6 
  punpckldq   xmm3,xmm7 
  mov         esi,dword [esp+18h] 
  mov         edi,dword [esp+10h] 
  movd        xmm4,dword [esi] 
  movd        xmm5,dword [edi] 
  punpckldq   xmm4,xmm5 
  punpcklqdq  xmm0,xmm4 
  movd        xmm4,dword [esi+ecx] 
  movd        xmm5,dword [edi+ecx] 
  punpckldq   xmm4,xmm5 
  punpcklqdq  xmm1,xmm4 
  movd        xmm4,dword [esi+ecx*2] 
  movd        xmm5,dword [edi+ecx*2] 
  punpckldq   xmm4,xmm5 
  punpcklqdq  xmm2,xmm4 
  movd        xmm4,dword [esi+edx] 
  movd        xmm5,dword [edi+edx] 
  punpckldq   xmm4,xmm5 
  punpcklqdq  xmm3,xmm4 
  movdqa      xmm6,xmm0 
  punpcklbw   xmm0,xmm1 
  punpckhbw   xmm6,xmm1 
  movdqa      xmm7,xmm2 
  punpcklbw   xmm2,xmm3 
  punpckhbw   xmm7,xmm3 
  movdqa      xmm4,xmm0 
  movdqa      xmm5,xmm6 
  punpcklwd   xmm0,xmm2 
  punpckhwd   xmm4,xmm2 
  punpcklwd   xmm6,xmm7 
  punpckhwd   xmm5,xmm7 
  movdqa      xmm1,xmm0 
  movdqa      xmm2,xmm4 
  punpckldq   xmm0,xmm6 
  punpckhdq   xmm1,xmm6 
  punpckldq   xmm4,xmm5 
  punpckhdq   xmm2,xmm5 
  movdqa      xmm5,xmm0 
  movdqa      xmm6,xmm1 
  punpcklqdq  xmm0,xmm4 
  punpckhqdq  xmm5,xmm4 
  punpcklqdq  xmm1,xmm2 
  punpckhqdq  xmm6,xmm2 
  mov         edi,dword [esp+1Ch] 
  movdqa      [edi],xmm0 
  movdqa      [edi+10h],xmm5 
  movdqa      [edi+20h],xmm1 
  movdqa      [edi+30h],xmm6 
  mov         eax,dword [ebp+1Ch] 
  movsx       cx,byte [eax+3] 
  movsx       dx,byte [eax+2] 
  movsx       si,byte [eax+1] 
  movsx       ax,byte [eax] 
  movzx       edi,cx 
  movzx       ecx,cx 
  movd        xmm2,ecx 
  movzx       ecx,dx 
  movzx       edx,dx 
  movd        xmm3,ecx 
  movd        xmm4,edx 
  movzx       ecx,si 
  movzx       edx,si 
  movd        xmm5,ecx 
  pxor        xmm0,xmm0 
  movd        xmm6,edx 
  movzx       ecx,ax 
  movdqa      [esp+60h],xmm0 
  movzx       edx,ax 
  movsx       eax,word [ebp+14h] 
  punpcklwd   xmm6,xmm2 
  movd        xmm1,edi 
  movd        xmm7,ecx 
  movsx       ecx,word [ebp+18h] 
  movd        xmm0,edx 
  punpcklwd   xmm7,xmm3 
  punpcklwd   xmm5,xmm1 
  movdqa      xmm1,[esp+60h] 
  punpcklwd   xmm7,xmm5 
  movdqa      xmm5,[esp+0A0h] 
  punpcklwd   xmm0,xmm4 
  punpcklwd   xmm0,xmm6 
  movdqa      xmm6, [esp+70h] 
  punpcklwd   xmm0,xmm7 
  movdqa      xmm7,[esp+80h] 
  movdqa      xmm2,xmm1 
  psubw       xmm2,xmm0 
  movdqa      [esp+0D0h],xmm2 
  movd        xmm2,eax 
  movdqa      xmm3,xmm2 
  punpcklwd   xmm3,xmm2 
  pshufd      xmm4,xmm3,0 
  movd        xmm2,ecx 
  movdqa      xmm3,xmm2 
  punpcklwd   xmm3,xmm2 
  pshufd      xmm2,xmm3,0 
  movdqa      xmm3, [esp+90h] 
  movdqa      [esp+50h],xmm2 
  movdqa      xmm2,xmm6 
  punpcklbw   xmm2,xmm1 
  punpckhbw   xmm6,xmm1 
  movdqa      [esp+40h],xmm2 
  movdqa      [esp+0B0h],xmm6 
  movdqa      xmm6,[esp+90h] 
  movdqa      xmm2,xmm7 
  punpckhbw   xmm7,xmm1 
  punpckhbw   xmm6,xmm1 
  punpcklbw   xmm2,xmm1 
  punpcklbw   xmm3,xmm1 
  punpcklbw   xmm5,xmm1 
  movdqa      [esp+0F0h],xmm7 
  movdqa      [esp+0C0h],xmm6 
  movdqa      xmm6, [esp+0A0h] 
  punpckhbw   xmm6,xmm1 
  movdqa      [esp+0E0h],xmm6 
  mov         edx,4 
  movsx       eax,dx 
  movd        xmm6,eax 
  movdqa      xmm7,xmm6 
  punpcklwd   xmm7,xmm6 
  pshufd      xmm6,xmm7,0 
  movdqa      [esp+30h],xmm6 
  movdqa      xmm7, [esp+40h] 
  psubw       xmm7,xmm5 
  movdqa      xmm6,xmm0 
  pcmpgtw     xmm6,xmm1 
  movdqa      [esp+60h],xmm6 
  movdqa      xmm1, [esp+0D0h] 
  movdqa      xmm6,xmm3 
  psubw       xmm6,xmm2 
  psllw       xmm6,2 
  paddw       xmm6,xmm7 
  paddw       xmm6,[esp+30h] 
  psraw       xmm6,3 
  pmaxsw      xmm1,xmm6 
  movdqa      xmm7,[esp+50h] 
  movdqa      [esp+20h],xmm0 
  movdqa      xmm6, [esp+20h] 
  pminsw      xmm6,xmm1 
  movdqa      [esp+20h],xmm6 
  movdqa      xmm6,xmm4 
  movdqa      xmm1,xmm2 
  psubw       xmm1,xmm3 
  pabsw       xmm1,xmm1 
  pcmpgtw     xmm6,xmm1 
  movdqa      xmm1, [esp+40h] 
  psubw       xmm1,xmm2 
  pabsw       xmm1,xmm1 
  pcmpgtw     xmm7,xmm1 
  movdqa      xmm1, [esp+50h] 
  pand        xmm6,xmm7 
  movdqa      xmm7, [esp+50h] 
  psubw       xmm5,xmm3 
  pabsw       xmm5,xmm5 
  pcmpgtw     xmm1,xmm5 
  movdqa      xmm5, [esp+0B0h] 
  psubw       xmm5,[esp+0E0h] 
  pand        xmm6,xmm1 
  pand        xmm6, [esp+60h] 
  movdqa      xmm1, [esp+20h] 
  pand        xmm1,xmm6 
  movdqa      xmm6, [esp+0C0h] 
  movdqa      [esp+40h],xmm1 
  movdqa      xmm1, [esp+0F0h] 
  psubw       xmm6,xmm1 
  psllw       xmm6,2 
  paddw       xmm6,xmm5 
  paddw       xmm6, [esp+30h] 
  movdqa      xmm5, [esp+0D0h] 
  psraw       xmm6,3 
  pmaxsw      xmm5,xmm6 
  pminsw      xmm0,xmm5 
  movdqa      xmm5,[esp+0C0h] 
  movdqa      xmm6,xmm1 
  psubw       xmm6,xmm5 
  pabsw       xmm6,xmm6 
  pcmpgtw     xmm4,xmm6 
  movdqa      xmm6,[esp+0B0h] 
  psubw       xmm6,xmm1 
  pabsw       xmm6,xmm6 
  pcmpgtw     xmm7,xmm6 
  movdqa      xmm6, [esp+0E0h] 
  pand        xmm4,xmm7 
  movdqa      xmm7, [esp+50h] 
  psubw       xmm6,xmm5 
  pabsw       xmm6,xmm6 
  pcmpgtw     xmm7,xmm6 
  pand        xmm4,xmm7 
  pand        xmm4,[esp+60h] 
  pand        xmm0,xmm4 
  movdqa      xmm4, [esp+40h] 
  paddw       xmm2,xmm4 
  paddw       xmm1,xmm0 
  psubw       xmm3,xmm4 
  psubw       xmm5,xmm0 
  packuswb    xmm2,xmm1 
  packuswb    xmm3,xmm5 
  movdqa      [esp+80h],xmm2 
  movdqa      [esp+90h],xmm3 
  mov         esi,dword [esp+1Ch] 
  movdqa      xmm0, [esi] 
  movdqa      xmm1, [esi+10h] 
  movdqa      xmm2, [esi+20h] 
  movdqa      xmm3, [esi+30h] 
  movdqa      xmm6,xmm0 
  punpcklbw   xmm0,xmm1 
  punpckhbw   xmm6,xmm1 
  movdqa      xmm7,xmm2 
  punpcklbw   xmm2,xmm3 
  punpckhbw   xmm7,xmm3 
  movdqa      xmm4,xmm0 
  movdqa      xmm5,xmm6 
  punpcklwd   xmm0,xmm2 
  punpckhwd   xmm4,xmm2 
  punpcklwd   xmm6,xmm7 
  punpckhwd   xmm5,xmm7 
  movdqa      xmm1,xmm0 
  movdqa      xmm2,xmm4 
  punpckldq   xmm0,xmm6 
  punpckhdq   xmm1,xmm6 
  punpckldq   xmm4,xmm5 
  punpckhdq   xmm2,xmm5 
  movdqa      xmm5,xmm0 
  movdqa      xmm6,xmm1 
  punpcklqdq  xmm0,xmm4 
  punpckhqdq  xmm5,xmm4 
  punpcklqdq  xmm1,xmm2 
  punpckhqdq  xmm6,xmm2 
  mov         esi,dword [esp+14h] 
  mov         ecx,dword [ebp+10h] 
  mov         edx,dword [esp+0Ch] 
  mov         edi,dword [esp+8] 
  movd        dword [esi],xmm0 
  movd        dword [esi+ecx],xmm5 
  movd        dword [esi+ecx*2],xmm1 
  movd        dword [esi+edx],xmm6 
  psrldq      xmm0,4 
  psrldq      xmm5,4 
  psrldq      xmm1,4 
  psrldq      xmm6,4 
  mov         esi,dword [esp+18h] 
  movd        dword [edi],xmm0 
  movd        dword [edi+ecx],xmm5 
  movd        dword [edi+ecx*2],xmm1 
  movd        dword [edi+edx],xmm6 
  psrldq      xmm0,4 
  psrldq      xmm5,4 
  psrldq      xmm1,4 
  psrldq      xmm6,4 
  movd        dword [esi],xmm0 
  movd        dword [esi+ecx],xmm5 
  movd        dword [esi+ecx*2],xmm1 
  movd        dword [esi+edx],xmm6 
  psrldq      xmm0,4 
  psrldq      xmm5,4 
  psrldq      xmm1,4 
  psrldq      xmm6,4 
  mov         edi,dword [esp+10h] 
  movd        dword [edi],xmm0 
  movd        dword [edi+ecx],xmm5 
  movd        dword [edi+ecx*2],xmm1 
  movd        dword [edi+edx],xmm6  
  pop         edi  
  pop         esi   
  mov         esp,ebp 
  pop         ebp  
  ret     
  
  
  
;*******************************************************************************
;    void DeblockLumaLt4V_sse2(uint8_t * pPix, int32_t iStride, int32_t iAlpha, 
;                                 int32_t iBeta, int8_t * pTC)
;*******************************************************************************
  

WELS_EXTERN  DeblockLumaLt4V_sse2
  
ALIGN  16

DeblockLumaLt4V_sse2:
    push	ebp
	mov	ebp, esp
	and	esp, -16				; fffffff0H
	sub	esp, 420				; 000001a4H
	mov	eax, dword [ebp+8]
	mov	ecx, dword [ebp+12]

	pxor	xmm0, xmm0
	push	ebx
	mov	edx, dword [ebp+24]
	movdqa	[esp+424-384], xmm0
	push	esi

	lea	esi, [ecx+ecx*2]
	push	edi
	mov	edi, eax
	sub	edi, esi
	movdqa	xmm0, [edi]

	lea	esi, [ecx+ecx]
	movdqa	[esp+432-208], xmm0
	mov	edi, eax
	sub	edi, esi
	movdqa	xmm0, [edi]
	movdqa	[esp+448-208], xmm0

	mov	ebx, eax
	sub	ebx, ecx
	movdqa	xmm0, [ebx]
	movdqa	[esp+464-208], xmm0

	movdqa	xmm0, [eax]

	add	ecx, eax
	movdqa	[esp+480-208], xmm0
	movdqa	xmm0, [ecx]
	mov	dword [esp+432-404], ecx

	movsx	ecx, word [ebp+16]
	movdqa	[esp+496-208], xmm0
	movdqa	xmm0, [esi+eax]

	movsx	si, byte [edx]
	movdqa	[esp+512-208], xmm0
	movd	xmm0, ecx
	movsx	ecx, word [ebp+20]
	movdqa	xmm1, xmm0
	punpcklwd xmm1, xmm0
	pshufd	xmm0, xmm1, 0
	movdqa	[esp+432-112], xmm0
	movd	xmm0, ecx
	movsx	cx, byte [edx+1]
	movdqa	xmm1, xmm0
	punpcklwd xmm1, xmm0
	mov	dword [esp+432-408], ebx
	movzx	ebx, cx
	pshufd	xmm0, xmm1, 0
	movd	xmm1, ebx
	movzx	ebx, cx
	movd	xmm2, ebx
	movzx	ebx, cx
	movzx	ecx, cx
	movd	xmm4, ecx
	movzx	ecx, si
	movd	xmm5, ecx
	movzx	ecx, si
	movd	xmm6, ecx
	movzx	ecx, si
	movd	xmm7, ecx
	movzx	ecx, si
	movdqa	[esp+432-336], xmm0
	movd	xmm0, ecx

	movsx	cx, byte [edx+3]
	movsx	dx, byte [edx+2]
	movd	xmm3, ebx
	punpcklwd xmm0, xmm4
	movzx	esi, cx
	punpcklwd xmm6, xmm2
	punpcklwd xmm5, xmm1
	punpcklwd xmm0, xmm6
	punpcklwd xmm7, xmm3
	punpcklwd xmm7, xmm5
	punpcklwd xmm0, xmm7
	movdqa	[esp+432-400], xmm0
	movd	xmm0, esi
	movzx	esi, cx
	movd	xmm2, esi
	movzx	esi, cx
	movzx	ecx, cx
	movd	xmm4, ecx
	movzx	ecx, dx
	movd	xmm3, esi
	movd	xmm5, ecx
	punpcklwd xmm5, xmm0

	movdqa	xmm0, [esp+432-384]
	movzx	ecx, dx
	movd	xmm6, ecx
	movzx	ecx, dx
	movzx	edx, dx
	punpcklwd xmm6, xmm2
	movd	xmm7, ecx
	movd	xmm1, edx

	movdqa	xmm2, [esp+448-208]
	punpcklbw xmm2, xmm0

	mov	ecx, 4
	movsx	edx, cx
	punpcklwd xmm7, xmm3
	punpcklwd xmm7, xmm5
	movdqa	xmm5, [esp+496-208]
	movdqa	xmm3, [esp+464-208]
	punpcklbw xmm5, xmm0
	movdqa	[esp+432-240], xmm5
	movdqa	xmm5, [esp+512-208]
	punpcklbw xmm5, xmm0
	movdqa	[esp+432-352], xmm5
	punpcklwd xmm1, xmm4
	movdqa	xmm4, [esp+432-208]
	punpcklwd xmm1, xmm6
	movdqa	xmm6, [esp+480-208]
	punpcklwd xmm1, xmm7
	punpcklbw xmm6, xmm0
	punpcklbw xmm3, xmm0
	punpcklbw xmm4, xmm0
	movdqa	xmm7, xmm3
	psubw	xmm7, xmm4
	pabsw	xmm7, xmm7
	movdqa	[esp+432-272], xmm4
	movdqa	xmm4, [esp+432-336]
	movdqa	xmm5, xmm4
	pcmpgtw	xmm5, xmm7
	movdqa	[esp+432-288], xmm5
	movdqa	xmm7, xmm6
	psubw	xmm7, [esp+432-352]
	pabsw	xmm7, xmm7
	movdqa	xmm5, xmm4
	pcmpgtw	xmm5, xmm7
	movdqa	[esp+432-256], xmm5
	movdqa	xmm5, xmm3
	pavgw	xmm5, xmm6
	movdqa	[esp+432-304], xmm5
	movdqa	xmm5, [esp+432-400]
	psubw	xmm5, [esp+432-288]
	psubw	xmm5, [esp+432-256]
	movdqa	[esp+432-224], xmm5
	movdqa	xmm5, xmm6
	psubw	xmm5, xmm3
	movdqa	[esp+432-32], xmm6
	psubw	xmm6, [esp+432-240]
	movdqa	xmm7, xmm5
	movdqa	[esp+432-384], xmm5
	movdqa	xmm5, [esp+432-112]
	pabsw	xmm7, xmm7
	pcmpgtw	xmm5, xmm7
	pabsw	xmm6, xmm6
	movdqa	xmm7, xmm4
	pcmpgtw	xmm7, xmm6

	pand	xmm5, xmm7
	movdqa	xmm6, xmm3
	psubw	xmm6, xmm2
	pabsw	xmm6, xmm6
	movdqa	xmm7, xmm4
	pcmpgtw	xmm7, xmm6
	movdqa	xmm6, [esp+432-400]
	pand	xmm5, xmm7
	movdqa	xmm7, xmm6
	pcmpeqw	xmm6, xmm0
	pcmpgtw	xmm7, xmm0
	por	xmm7, xmm6
	pand	xmm5, xmm7
	movdqa	[esp+432-320], xmm5
	movd	xmm5, edx
	movdqa	xmm6, xmm5
	punpcklwd xmm6, xmm5
	pshufd	xmm5, xmm6, 0
	movdqa	[esp+432-336], xmm5
	movdqa	xmm5, [esp+432-224]
	movdqa	[esp+432-368], xmm5
	movdqa	xmm6, xmm0
	psubw	xmm6, xmm5
	movdqa	xmm5, [esp+432-384]
	psllw	xmm5, 2
	movdqa	xmm7, xmm2
	psubw	xmm7, [esp+432-240]
	paddw	xmm7, xmm5
	paddw	xmm7, [esp+432-336]
	movdqa	xmm5, [esp+432-368]
	psraw	xmm7, 3
	pmaxsw	xmm6, xmm7
	pminsw	xmm5, xmm6

	pand	xmm5, [esp+432-320]
	movdqa	xmm6, [esp+432-400]
	movdqa	[esp+432-64], xmm5
	movdqa	[esp+432-384], xmm6
	movdqa	xmm5, xmm0
	psubw	xmm5, xmm6
	movdqa	[esp+432-368], xmm5
	movdqa	xmm6, xmm5
	movdqa	xmm5, [esp+432-272]
	paddw	xmm5, [esp+432-304]
	movdqa	xmm7, xmm2
	paddw	xmm7, xmm2
	psubw	xmm5, xmm7
	psraw	xmm5, 1
	pmaxsw	xmm6, xmm5
	movdqa	xmm5, [esp+432-384]
	pminsw	xmm5, xmm6

	pand	xmm5, [esp+432-320]
	pand	xmm5, [esp+432-288]
	movdqa	xmm6, [esp+432-240]
	movdqa	[esp+432-96], xmm5
	movdqa	xmm5, [esp+432-352]
	paddw	xmm5, [esp+432-304]
	movdqa	xmm7, xmm6
	paddw	xmm7, xmm6
	movdqa	xmm6, [esp+432-368]
	psubw	xmm5, xmm7

	movdqa	xmm7, [esp+496-208]
	psraw	xmm5, 1
	pmaxsw	xmm6, xmm5
	movdqa	xmm5, [esp+432-400]
	pminsw	xmm5, xmm6
	pand	xmm5, [esp+432-320]
	pand	xmm5, [esp+432-256]
	movdqa	xmm6, [esp+448-208]
	punpckhbw xmm7, xmm0
	movdqa	[esp+432-352], xmm7

	movdqa	xmm7, [esp+512-208]
	punpckhbw xmm6, xmm0
	movdqa	[esp+432-48], xmm5
	movdqa	xmm5, [esp+432-208]
	movdqa	[esp+432-368], xmm6
	movdqa	xmm6, [esp+464-208]
	punpckhbw xmm7, xmm0
	punpckhbw xmm5, xmm0
	movdqa	[esp+432-384], xmm7
	punpckhbw xmm6, xmm0
	movdqa	[esp+432-400], xmm6

	movdqa	xmm7, [esp+432-400]
	movdqa	xmm6, [esp+480-208]
	psubw	xmm7, xmm5
	movdqa	[esp+432-16], xmm5
	pabsw	xmm7, xmm7
	punpckhbw xmm6, xmm0
	movdqa	xmm5, xmm4
	pcmpgtw	xmm5, xmm7
	movdqa	[esp+432-288], xmm5

	movdqa	xmm7, xmm6
	psubw	xmm7, [esp+432-384]
	pabsw	xmm7, xmm7
	movdqa	xmm5, xmm4
	pcmpgtw	xmm5, xmm7
	movdqa	[esp+432-256], xmm5

	movdqa	xmm5, [esp+432-400]
	movdqa	[esp+432-80], xmm6
	pavgw	xmm5, xmm6
	movdqa	[esp+432-304], xmm5

	movdqa	xmm5, xmm1
	psubw	xmm5, [esp+432-288]
	psubw	xmm5, [esp+432-256]
	movdqa	[esp+432-224], xmm5
	movdqa	xmm5, xmm6
	psubw	xmm5, [esp+432-400]
	psubw	xmm6, [esp+432-352]
	movdqa	[esp+432-272], xmm5
	movdqa	xmm7, xmm5
	movdqa	xmm5, [esp+432-112]
	pabsw	xmm7, xmm7
	pcmpgtw	xmm5, xmm7
	movdqa	xmm7, xmm4
	pabsw	xmm6, xmm6
	pcmpgtw	xmm7, xmm6
	movdqa	xmm6, [esp+432-368]

	pand	xmm5, xmm7
	movdqa	xmm7, [esp+432-400]
	psubw	xmm7, xmm6
	psubw	xmm6, [esp+432-352]
	pabsw	xmm7, xmm7
	pcmpgtw	xmm4, xmm7
	pand	xmm5, xmm4

	paddw	xmm2, [esp+432-96]
	movdqa	xmm4, xmm1
	pcmpgtw	xmm4, xmm0
	movdqa	xmm7, xmm1
	pcmpeqw	xmm7, xmm0
	por	xmm4, xmm7
	pand	xmm5, xmm4
	movdqa	xmm4, [esp+432-224]
	movdqa	[esp+432-320], xmm5
	movdqa	xmm5, [esp+432-272]
	movdqa	xmm7, xmm0
	psubw	xmm7, xmm4
	psubw	xmm0, xmm1
	psllw	xmm5, 2
	paddw	xmm6, xmm5
	paddw	xmm6, [esp+432-336]
	movdqa	xmm5, [esp+432-368]
	movdqa	[esp+432-336], xmm0
	psraw	xmm6, 3
	pmaxsw	xmm7, xmm6
	pminsw	xmm4, xmm7
	pand	xmm4, [esp+432-320]
	movdqa	xmm6, xmm0
	movdqa	xmm0, [esp+432-16]
	paddw	xmm0, [esp+432-304]
	movdqa	[esp+432-272], xmm4
	movdqa	xmm4, [esp+432-368]
	paddw	xmm4, xmm4
	psubw	xmm0, xmm4

	movdqa	xmm4, [esp+432-64]
	psraw	xmm0, 1
	pmaxsw	xmm6, xmm0
	movdqa	xmm0, [esp+432-400]
	movdqa	xmm7, xmm1
	pminsw	xmm7, xmm6
	movdqa	xmm6, [esp+432-320]
	pand	xmm7, xmm6
	pand	xmm7, [esp+432-288]
	paddw	xmm5, xmm7
	packuswb xmm2, xmm5
	movdqa	xmm5, [esp+432-272]
	paddw	xmm0, xmm5
	paddw	xmm3, xmm4
	packuswb xmm3, xmm0

	movdqa	xmm0, [esp+432-32]
	psubw	xmm0, xmm4
	movdqa	xmm4, [esp+432-80]
	psubw	xmm4, xmm5

	movdqa	xmm5, [esp+432-240]
	paddw	xmm5, [esp+432-48]
	packuswb xmm0, xmm4
	movdqa	xmm4, [esp+432-384]
	paddw	xmm4, [esp+432-304]
	movdqa	[esp+480-208], xmm0
	movdqa	xmm0, [esp+432-352]
	movdqa	xmm7, xmm0
	paddw	xmm0, xmm0

	mov	ecx, dword [esp+432-408]

	mov	edx, dword [esp+432-404]
	psubw	xmm4, xmm0
	movdqa	xmm0, [esp+432-336]
	movdqa	[edi], xmm2
	psraw	xmm4, 1
	pmaxsw	xmm0, xmm4
	pminsw	xmm1, xmm0
	movdqa	xmm0, [esp+480-208]

	pop	edi
	pand	xmm1, xmm6
	pand	xmm1, [esp+428-256]
	movdqa	[ecx], xmm3
	paddw	xmm7, xmm1
	pop	esi
	packuswb xmm5, xmm7
	movdqa	[eax], xmm0
	movdqa	[edx], xmm5
	pop	ebx
	mov	esp, ebp
	pop	ebp
	ret


;*******************************************************************************
;    void DeblockLumaEq4V_sse2(uint8_t * pPix, int32_t iStride, int32_t iAlpha, 
;                                 int32_t iBeta)
;*******************************************************************************

WELS_EXTERN  DeblockLumaEq4V_sse2
  
ALIGN  16

DeblockLumaEq4V_sse2:

	push	ebp
	mov	ebp, esp
	and	esp, -16				; fffffff0H
	sub	esp, 628				; 00000274H
	mov	eax, dword [ebp+8]
	mov	ecx, dword [ebp+12]
	push	ebx
	push	esi

	lea	edx, [ecx*4]
	pxor	xmm0, xmm0
	movdqa	xmm2, xmm0

	movdqa	xmm0, [ecx+eax]
	mov	esi, eax
	sub	esi, edx
	movdqa	xmm3, [esi]
	movdqa	xmm5, [eax]
	push	edi
	lea	edi, [ecx+ecx]
	lea	ebx, [ecx+ecx*2]
	mov	dword [esp+640-600], edi
	mov	esi, eax
	sub	esi, edi
	movdqa	xmm1, [esi]
	movdqa	 [esp+720-272], xmm0
	mov	edi, eax
	sub	edi, ecx
	movdqa	xmm4, [edi]
	add	ecx, eax
	mov	dword [esp+640-596], ecx

	mov	ecx, dword [esp+640-600]
	movdqa	xmm0, [ecx+eax]
	movdqa	 [esp+736-272], xmm0

	movdqa	xmm0, [eax+ebx]
	mov	edx, eax
	sub	edx, ebx

	movsx	ebx, word [ebp+16]
	movdqa	xmm6, [edx]
	add	ecx, eax
	movdqa	 [esp+752-272], xmm0
	movd	xmm0, ebx

	movsx	ebx, word [ebp+20]
	movdqa	xmm7, xmm0
	punpcklwd xmm7, xmm0
	pshufd	xmm0, xmm7, 0
	movdqa	 [esp+640-320], xmm0
	movd	xmm0, ebx
	movdqa	xmm7, xmm0
	punpcklwd xmm7, xmm0
	pshufd	xmm0, xmm7, 0

	movdqa	xmm7, [esp+736-272]
	punpcklbw xmm7, xmm2
	movdqa	 [esp+640-416], xmm7
	movdqa	 [esp+640-512], xmm0
	movdqa	xmm0, xmm1
	movdqa	 [esp+672-272], xmm1
	movdqa	xmm1, xmm4
	movdqa	 [esp+704-272], xmm5
	punpcklbw xmm5, xmm2
	punpcklbw xmm1, xmm2

	movdqa	xmm7, xmm5
	psubw	xmm7, xmm1
	pabsw	xmm7, xmm7
	movdqa	 [esp+640-560], xmm7
	punpcklbw xmm0, xmm2
	movdqa	 [esp+688-272], xmm4
	movdqa	xmm4, [esp+720-272]
	movdqa	 [esp+640-480], xmm0

	movdqa	xmm7, xmm1
	psubw	xmm7, xmm0

	movdqa	xmm0, [esp+640-512]
	pabsw	xmm7, xmm7
	punpcklbw xmm4, xmm2
	pcmpgtw	xmm0, xmm7
	movdqa	 [esp+640-384], xmm4
	movdqa	xmm7, xmm5
	psubw	xmm7, xmm4
	movdqa	xmm4, [esp+640-512]
	movdqa	 [esp+656-272], xmm6
	punpcklbw xmm6, xmm2
	pabsw	xmm7, xmm7
	movdqa	 [esp+640-48], xmm2
	movdqa	 [esp+640-368], xmm6
	movdqa	 [esp+640-144], xmm1
	movdqa	 [esp+640-400], xmm5
	pcmpgtw	xmm4, xmm7
	pand	xmm0, xmm4
	movdqa	xmm4, [esp+640-320]
	pcmpgtw	xmm4, [esp+640-560]
	pand	xmm0, xmm4

	mov	ebx, 2
	movsx	ebx, bx
	movd	xmm4, ebx
	movdqa	xmm7, xmm4
	punpcklwd xmm7, xmm4
	movdqa	xmm4, [esp+640-320]
	psraw	xmm4, 2
	pshufd	xmm7, xmm7, 0
	paddw	xmm4, xmm7
	movdqa	 [esp+640-576], xmm4
	pcmpgtw	xmm4, [esp+640-560]
	movdqa	 [esp+640-560], xmm4

	movdqa	xmm4, [esp+640-512]
	movdqa	 [esp+640-624], xmm7
	movdqa	xmm7, xmm1
	psubw	xmm7, xmm6
	pabsw	xmm7, xmm7
	pcmpgtw	xmm4, xmm7

	pand	xmm4, [esp+640-560]
	movdqa	 [esp+640-544], xmm4
	movdqa	xmm4, [esp+640-512]
	movdqa	xmm7, xmm5
	psubw	xmm7, [esp+640-416]
	pabsw	xmm7, xmm7
	pcmpgtw	xmm4, xmm7

	pand	xmm4, [esp+640-560]
	movdqa	 [esp+640-560], xmm4

	movdqa	xmm4, [esp+640-544]
	pandn	xmm4, xmm6
	movdqa	 [esp+640-16], xmm4
	mov	ebx, 4
	movsx	ebx, bx
	movd	xmm4, ebx
	movdqa	xmm7, xmm4
	punpcklwd xmm7, xmm4
	movdqa	xmm4, xmm3
	punpcklbw xmm4, xmm2
	psllw	xmm4, 1
	paddw	xmm4, xmm6
	paddw	xmm4, xmm6
	paddw	xmm4, xmm6
	paddw	xmm4, [esp+640-480]

	movdqa	xmm6, [esp+640-560]
	pshufd	xmm7, xmm7, 0
	paddw	xmm4, xmm1
	movdqa	 [esp+640-592], xmm7
	paddw	xmm4, xmm5
	paddw	xmm4, xmm7
	movdqa	xmm7, [esp+640-416]
	pandn	xmm6, xmm7
	movdqa	 [esp+640-80], xmm6
	movdqa	xmm6, [esp+752-272]
	punpcklbw xmm6, xmm2
	psllw	xmm6, 1
	paddw	xmm6, xmm7
	paddw	xmm6, xmm7
	paddw	xmm6, xmm7
	paddw	xmm6, [esp+640-384]

	movdqa	xmm7, [esp+640-480]
	paddw	xmm6, xmm5
	paddw	xmm6, xmm1
	paddw	xmm6, [esp+640-592]
	psraw	xmm6, 3
	pand	xmm6, [esp+640-560]
	movdqa	 [esp+640-112], xmm6
	movdqa	xmm6, [esp+640-544]
	pandn	xmm6, xmm7
	movdqa	 [esp+640-336], xmm6
	movdqa	xmm6, [esp+640-544]
	movdqa	 [esp+640-528], xmm6
	movdqa	xmm6, [esp+640-368]
	paddw	xmm6, xmm7
	movdqa	xmm7, xmm1
	psraw	xmm4, 3
	pand	xmm4, [esp+640-544]
	paddw	xmm7, xmm5
	paddw	xmm6, xmm7
	paddw	xmm6, [esp+640-624]
	movdqa	xmm7, [esp+640-528]

	paddw	xmm5, xmm1
	psraw	xmm6, 2
	pand	xmm7, xmm6

	movdqa	xmm6, [esp+640-384]
	movdqa	 [esp+640-64], xmm7
	movdqa	xmm7, [esp+640-560]
	pandn	xmm7, xmm6
	movdqa	 [esp+640-304], xmm7
	movdqa	xmm7, [esp+640-560]
	movdqa	 [esp+640-528], xmm7
	movdqa	xmm7, [esp+640-416]
	paddw	xmm7, xmm6
	paddw	xmm7, xmm5
	paddw	xmm7, [esp+640-624]
	movdqa	xmm5, [esp+640-528]
	psraw	xmm7, 2
	pand	xmm5, xmm7
	movdqa	 [esp+640-32], xmm5

	movdqa	xmm5, [esp+640-544]
	movdqa	 [esp+640-528], xmm5
	movdqa	xmm5, [esp+640-480]
	movdqa	xmm7, xmm5
	paddw	xmm7, xmm5
	movdqa	xmm5, xmm1
	paddw	xmm5, xmm6
	paddw	xmm6, [esp+640-592]
	paddw	xmm7, xmm5
	paddw	xmm7, [esp+640-624]
	movdqa	xmm5, [esp+640-528]
	psraw	xmm7, 2
	pandn	xmm5, xmm7
	movdqa	xmm7, [esp+640-480]
	paddw	xmm7, xmm1
	paddw	xmm7, [esp+640-400]
	movdqa	xmm1, [esp+640-544]
	movdqa	 [esp+640-352], xmm5
	movdqa	xmm5, [esp+640-368]
	psllw	xmm7, 1
	paddw	xmm7, xmm6
	paddw	xmm5, xmm7

	movdqa	xmm7, [esp+640-400]
	psraw	xmm5, 3
	pand	xmm1, xmm5
	movdqa	xmm5, [esp+640-480]
	movdqa	 [esp+640-96], xmm1
	movdqa	xmm1, [esp+640-560]
	movdqa	 [esp+640-528], xmm1
	movdqa	xmm1, [esp+640-384]
	movdqa	xmm6, xmm1
	paddw	xmm6, xmm1
	paddw	xmm1, [esp+640-400]
	paddw	xmm1, [esp+640-144]
	paddw	xmm7, xmm5
	paddw	xmm5, [esp+640-592]
	paddw	xmm6, xmm7
	paddw	xmm6, [esp+640-624]
	movdqa	xmm7, [esp+640-528]
	psraw	xmm6, 2
	psllw	xmm1, 1
	paddw	xmm1, xmm5

	movdqa	xmm5, [esp+656-272]
	pandn	xmm7, xmm6
	movdqa	xmm6, [esp+640-416]
	paddw	xmm6, xmm1
	movdqa	xmm1, [esp+640-560]
	psraw	xmm6, 3
	pand	xmm1, xmm6

	movdqa	xmm6, [esp+704-272]
	movdqa	 [esp+640-128], xmm1
	movdqa	xmm1, [esp+672-272]
	punpckhbw xmm1, xmm2
	movdqa	 [esp+640-448], xmm1
	movdqa	xmm1, [esp+688-272]
	punpckhbw xmm1, xmm2
	punpckhbw xmm6, xmm2
	movdqa	 [esp+640-288], xmm7
	punpckhbw xmm5, xmm2
	movdqa	 [esp+640-496], xmm1
	movdqa	 [esp+640-432], xmm6

	movdqa	xmm7, [esp+720-272]
	punpckhbw xmm7, xmm2
	movdqa	 [esp+640-464], xmm7

	movdqa	xmm7, [esp+736-272]
	punpckhbw xmm7, xmm2
	movdqa	 [esp+640-528], xmm7

	movdqa	xmm7, xmm6

	psubw	xmm6, [esp+640-464]
	psubw	xmm7, xmm1
	pabsw	xmm7, xmm7
	movdqa	 [esp+640-560], xmm7
	por	xmm4, [esp+640-16]
	pabsw	xmm6, xmm6
	movdqa	xmm7, xmm1
	psubw	xmm7, [esp+640-448]

	movdqa	xmm1, [esp+640-512]
	pabsw	xmm7, xmm7
	pcmpgtw	xmm1, xmm7
	movdqa	xmm7, [esp+640-512]
	pcmpgtw	xmm7, xmm6
	movdqa	xmm6, [esp+640-320]
	pand	xmm1, xmm7
	movdqa	xmm7, [esp+640-560]
	pcmpgtw	xmm6, xmm7
	pand	xmm1, xmm6

	movdqa	xmm6, [esp+640-576]
	pcmpgtw	xmm6, xmm7

	movdqa	xmm7, [esp+640-496]
	punpckhbw xmm3, xmm2
	movdqa	 [esp+640-560], xmm6
	movdqa	xmm6, [esp+640-512]
	psubw	xmm7, xmm5
	pabsw	xmm7, xmm7
	pcmpgtw	xmm6, xmm7

	pand	xmm6, [esp+640-560]
	movdqa	xmm7, [esp+640-432]
	psubw	xmm7, [esp+640-528]

	psllw	xmm3, 1
	movdqa	 [esp+640-544], xmm6
	movdqa	xmm6, [esp+640-512]

	movdqa	xmm2, [esp+640-544]
	paddw	xmm3, xmm5
	paddw	xmm3, xmm5
	paddw	xmm3, xmm5
	paddw	xmm3, [esp+640-448]
	paddw	xmm3, [esp+640-496]
	pabsw	xmm7, xmm7
	pcmpgtw	xmm6, xmm7
	pand	xmm6, [esp+640-560]
	movdqa	 [esp+640-560], xmm6

	movdqa	xmm6, xmm0
	pand	xmm6, xmm4
	movdqa	xmm4, xmm0
	pandn	xmm4, [esp+640-368]
	por	xmm6, xmm4
	movdqa	xmm4, [esp+640-432]
	paddw	xmm3, xmm4
	paddw	xmm3, [esp+640-592]
	psraw	xmm3, 3
	pand	xmm3, xmm2
	pandn	xmm2, xmm5
	por	xmm3, xmm2
	movdqa	xmm7, xmm1
	pand	xmm7, xmm3
	movdqa	xmm3, [esp+640-64]
	por	xmm3, [esp+640-336]
	movdqa	xmm2, xmm1
	pandn	xmm2, xmm5
	por	xmm7, xmm2

	movdqa	xmm2, xmm0
	pand	xmm2, xmm3
	movdqa	xmm3, xmm0
	pandn	xmm3, [esp+640-480]
	por	xmm2, xmm3
	packuswb xmm6, xmm7
	movdqa	 [esp+640-336], xmm2
	movdqa	 [esp+656-272], xmm6
	movdqa	xmm6, [esp+640-544]
	movdqa	xmm2, xmm5
	paddw	xmm2, [esp+640-448]
	movdqa	xmm3, xmm1
	movdqa	xmm7, [esp+640-496]
	paddw	xmm7, xmm4
	paddw	xmm2, xmm7
	paddw	xmm2, [esp+640-624]
	movdqa	xmm7, [esp+640-544]
	psraw	xmm2, 2
	pand	xmm6, xmm2
	movdqa	xmm2, [esp+640-448]
	pandn	xmm7, xmm2
	por	xmm6, xmm7
	pand	xmm3, xmm6
	movdqa	xmm6, xmm1
	pandn	xmm6, xmm2
	paddw	xmm2, [esp+640-496]
	paddw	xmm2, xmm4
	por	xmm3, xmm6
	movdqa	xmm6, [esp+640-336]
	packuswb xmm6, xmm3
	psllw	xmm2, 1
	movdqa	 [esp+672-272], xmm6
	movdqa	xmm6, [esp+640-96]
	por	xmm6, [esp+640-352]

	movdqa	xmm3, xmm0
	pand	xmm3, xmm6
	movdqa	xmm6, xmm0
	pandn	xmm6, [esp+640-144]
	por	xmm3, xmm6
	movdqa	xmm6, [esp+640-544]
	movdqa	 [esp+640-352], xmm3
	movdqa	xmm3, [esp+640-464]
	paddw	xmm3, [esp+640-592]
	paddw	xmm2, xmm3
	movdqa	xmm3, [esp+640-448]
	paddw	xmm5, xmm2
	movdqa	xmm2, [esp+640-496]
	psraw	xmm5, 3
	pand	xmm6, xmm5
	movdqa	xmm5, [esp+640-464]
	paddw	xmm2, xmm5
	paddw	xmm5, [esp+640-432]
	movdqa	xmm4, xmm3
	paddw	xmm4, xmm3
	paddw	xmm4, xmm2
	paddw	xmm4, [esp+640-624]
	movdqa	xmm2, [esp+640-544]
	paddw	xmm3, [esp+640-592]
	psraw	xmm4, 2
	pandn	xmm2, xmm4
	por	xmm6, xmm2
	movdqa	xmm7, xmm1
	pand	xmm7, xmm6
	movdqa	xmm6, [esp+640-496]
	movdqa	xmm2, xmm1
	pandn	xmm2, xmm6
	por	xmm7, xmm2
	movdqa	xmm2, [esp+640-352]
	packuswb xmm2, xmm7
	movdqa	 [esp+688-272], xmm2
	movdqa	xmm2, [esp+640-128]
	por	xmm2, [esp+640-288]

	movdqa	xmm4, xmm0
	pand	xmm4, xmm2
	paddw	xmm5, xmm6
	movdqa	xmm2, xmm0
	pandn	xmm2, [esp+640-400]
	por	xmm4, xmm2
	movdqa	xmm2, [esp+640-528]
	psllw	xmm5, 1
	paddw	xmm5, xmm3
	movdqa	xmm3, [esp+640-560]
	paddw	xmm2, xmm5
	psraw	xmm2, 3
	movdqa	 [esp+640-288], xmm4
	movdqa	xmm4, [esp+640-560]
	pand	xmm4, xmm2
	movdqa	xmm2, [esp+640-464]
	movdqa	xmm5, xmm2
	paddw	xmm5, xmm2
	movdqa	xmm2, [esp+640-432]
	paddw	xmm2, [esp+640-448]
	movdqa	xmm7, xmm1
	paddw	xmm5, xmm2
	paddw	xmm5, [esp+640-624]
	movdqa	xmm6, [esp+640-560]
	psraw	xmm5, 2
	pandn	xmm3, xmm5
	por	xmm4, xmm3
	movdqa	xmm3, [esp+640-32]
	por	xmm3, [esp+640-304]
	pand	xmm7, xmm4
	movdqa	xmm4, [esp+640-432]
	movdqa	xmm5, [esp+640-464]
	movdqa	xmm2, xmm1
	pandn	xmm2, xmm4
	paddw	xmm4, [esp+640-496]
	por	xmm7, xmm2
	movdqa	xmm2, [esp+640-288]
	packuswb xmm2, xmm7
	movdqa	 [esp+704-272], xmm2

	movdqa	xmm2, xmm0
	pand	xmm2, xmm3
	movdqa	xmm3, xmm0
	pandn	xmm3, [esp+640-384]
	por	xmm2, xmm3
	movdqa	 [esp+640-304], xmm2
	movdqa	xmm2, [esp+640-528]
	movdqa	xmm3, xmm2
	paddw	xmm3, [esp+640-464]
	paddw	xmm3, xmm4
	paddw	xmm3, [esp+640-624]
	psraw	xmm3, 2
	pand	xmm6, xmm3
	movdqa	xmm3, [esp+640-560]
	movdqa	xmm4, xmm3
	pandn	xmm4, xmm5
	por	xmm6, xmm4
	movdqa	xmm7, xmm1
	pand	xmm7, xmm6
	movdqa	xmm6, [esp+640-304]
	movdqa	xmm4, xmm1
	pandn	xmm4, xmm5
	por	xmm7, xmm4

	movdqa	xmm4, xmm0
	pandn	xmm0, [esp+640-416]
	packuswb xmm6, xmm7
	movdqa	xmm7, [esp+640-112]
	por	xmm7, [esp+640-80]
	pand	xmm4, xmm7
	por	xmm4, xmm0
	movdqa	xmm0, [esp+752-272]
	punpckhbw xmm0, [esp+640-48]
	psllw	xmm0, 1
	paddw	xmm0, xmm2
	paddw	xmm0, xmm2
	paddw	xmm0, xmm2
	paddw	xmm0, xmm5
	paddw	xmm0, [esp+640-432]
	paddw	xmm0, [esp+640-496]
	paddw	xmm0, [esp+640-592]
	psraw	xmm0, 3
	pand	xmm0, xmm3
	movdqa	xmm7, xmm1
	pandn	xmm3, xmm2
	por	xmm0, xmm3
	pand	xmm7, xmm0

	movdqa	xmm0, [esp+656-272]
	movdqa	 [edx], xmm0

	movdqa	xmm0, [esp+672-272]

	mov	edx, dword [esp+640-596]
	movdqa	 [esi], xmm0
	movdqa	xmm0, [esp+688-272]
	movdqa	 [edi], xmm0
	movdqa	xmm0, [esp+704-272]

	pop	edi
	pandn	xmm1, xmm2
	movdqa	 [eax], xmm0
	por	xmm7, xmm1
	pop	esi
	packuswb xmm4, xmm7
	movdqa	 [edx], xmm6
	movdqa	 [ecx], xmm4
	pop	ebx
	mov	esp, ebp
	pop	ebp
	ret
  
    
;********************************************************************************
;
;   void DeblockLumaTransposeH2V_sse2(uint8_t * pPixY, int32_t iStride, uint8_t * pDst);     
;
;********************************************************************************

WELS_EXTERN  DeblockLumaTransposeH2V_sse2

ALIGN  16

DeblockLumaTransposeH2V_sse2:
    push    ebp
    push    ebx
    mov     ebp,   esp
    and     esp,0FFFFFFF0h
    sub     esp,   10h    
    
    mov     eax,   [ebp + 0Ch]  
    mov     ecx,   [ebp + 10h]
    lea     edx,   [eax + ecx * 8]
    lea     ebx,   [ecx*3]
    
    movq    xmm0,  [eax] 
    movq    xmm7,  [edx]
    punpcklqdq   xmm0,  xmm7  
    movq    xmm1,  [eax + ecx]
    movq    xmm7,  [edx + ecx]
    punpcklqdq   xmm1,  xmm7
    movq    xmm2,  [eax + ecx*2] 
    movq    xmm7,  [edx + ecx*2]
    punpcklqdq   xmm2,  xmm7
    movq    xmm3,  [eax + ebx]
    movq    xmm7,  [edx + ebx]
    punpcklqdq   xmm3,  xmm7
    
    lea     eax,   [eax + ecx * 4]
    lea     edx,   [edx + ecx * 4]
    movq    xmm4,  [eax] 
    movq    xmm7,  [edx]
    punpcklqdq   xmm4,  xmm7  
    movq    xmm5,  [eax + ecx]
    movq    xmm7,  [edx + ecx]
    punpcklqdq   xmm5,  xmm7
    movq    xmm6,  [eax + ecx*2] 
    movq    xmm7,  [edx + ecx*2]
    punpcklqdq   xmm6,  xmm7
    
    movdqa  [esp],   xmm0
    movq    xmm7,  [eax + ebx]
    movq    xmm0,  [edx + ebx]
    punpcklqdq   xmm7,  xmm0
    movdqa  xmm0,   [esp]
    
    SSE2_TransTwo8x8B  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [esp]
    ;pOut: m5, m3, m4, m8, m6, m2, m7, m1
    
    mov    eax,   [ebp + 14h]
    movdqa  [eax],    xmm4 
    movdqa  [eax + 10h],  xmm2
    movdqa  [eax + 20h],  xmm3
    movdqa  [eax + 30h],  xmm7
    movdqa  [eax + 40h],  xmm5
    movdqa  [eax + 50h],  xmm1
    movdqa  [eax + 60h],  xmm6
    movdqa  [eax + 70h],  xmm0   
    
    mov     esp,   ebp
    pop     ebx
    pop     ebp
    ret
    
    
    
;*******************************************************************************************
;
;   void DeblockLumaTransposeV2H_sse2(uint8_t * pPixY, int32_t iStride, uint8_t * pSrc);
;
;*******************************************************************************************

WELS_EXTERN   DeblockLumaTransposeV2H_sse2

ALIGN  16

DeblockLumaTransposeV2H_sse2:
    push     ebp
    mov      ebp,   esp
    
    and     esp,  0FFFFFFF0h
    sub     esp,   10h  
    
    mov      eax,   [ebp + 10h]  
    mov      ecx,   [ebp + 0Ch]
    mov      edx,   [ebp + 08h]
      
    movdqa   xmm0,  [eax]
    movdqa   xmm1,  [eax + 10h]
    movdqa   xmm2,  [eax + 20h]
    movdqa   xmm3,	[eax + 30h]
    movdqa   xmm4,	[eax + 40h]
    movdqa   xmm5,	[eax + 50h]
    movdqa   xmm6,	[eax + 60h]
    movdqa   xmm7,	[eax + 70h]
    
    SSE2_TransTwo8x8B  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [esp]
    ;pOut: m5, m3, m4, m8, m6, m2, m7, m1
    
    lea      eax,   [ecx * 3]
    
    movq     [edx],  xmm4 
    movq     [edx + ecx],  xmm2
    movq     [edx + ecx*2],  xmm3
    movq     [edx + eax],  xmm7
    
    lea      edx,   [edx + ecx*4]
    movq     [edx],  xmm5 
    movq     [edx + ecx],  xmm1
    movq     [edx + ecx*2],  xmm6
    movq     [edx + eax],  xmm0    
    
    psrldq    xmm4,   8
    psrldq    xmm2,   8
    psrldq    xmm3,   8
    psrldq    xmm7,   8
    psrldq    xmm5,   8
    psrldq    xmm1,   8
    psrldq    xmm6,   8
    psrldq    xmm0,   8
    
    lea       edx,  [edx + ecx*4]
    movq     [edx],  xmm4 
    movq     [edx + ecx],  xmm2
    movq     [edx + ecx*2],  xmm3
    movq     [edx + eax],  xmm7
    
    lea      edx,   [edx + ecx*4]
    movq     [edx],  xmm5 
    movq     [edx + ecx],  xmm1
    movq     [edx + ecx*2],  xmm6
    movq     [edx + eax],  xmm0   
    
    
    mov      esp,   ebp
    pop      ebp
    ret