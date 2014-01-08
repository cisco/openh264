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

;*******************************************************************************
; Macros and other preprocessor constants
;*******************************************************************************

%ifdef FORMAT_COFF
SECTION .rodata pData
%else
SECTION .rodata align=16
%endif

ALIGN   16
FOUR_16B_SSE2:   dw   4, 4, 4, 4, 4, 4, 4, 4


SECTION .text

%ifdef  WIN64 


WELS_EXTERN   DeblockLumaLt4V_sse2

DeblockLumaLt4V_sse2:
  push        rbp      
  mov         r11,[esp + 16 + 20h]  ; pTC                                                    
  sub         rsp,1B0h                                                       
  lea         rbp,[rsp+20h]                                                  
  movd        xmm4,r8d                                                                                                  
  movd        xmm2,r9d                                                       
  mov         qword [rbp+180h],r12                                       
  mov         r10,rcx                                                        
  movsxd      r12,edx                                                        
  add         edx,edx                                                        
  movsxd      rdx,edx                                                        
  sub         r10,r12                                                        
  movsx       r8d,byte [r11]                                             
  pxor        xmm3,xmm3                                                      
  punpcklwd   xmm2,xmm2                                                      
  movaps      [rbp+50h],xmm14                                    
  lea         rax,[r12+r12*2]                                                
  movdqa      xmm14,[rdx+rcx]                                    
  neg         rax                                                            
  pshufd      xmm0,xmm2,0                                                    
  movd        xmm2,r8d                                                       
  movsx       edx,byte [r11+1]                                           
  movsx       r8d,byte [r11+2]                                           
  movsx       r11d,byte [r11+3]                                          
  movaps      [rbp+70h],xmm12                                    
  movd        xmm1,edx                                                       
  movaps      [rbp+80h],xmm11                                    
  movd        xmm12,r8d                                                      
  movd        xmm11,r11d                                                     
  movdqa      xmm5, [rax+rcx]                                     
  lea         rax,[r12+r12]                                                  
  punpcklwd   xmm12,xmm12                                                    
  neg         rax                                                            
  punpcklwd   xmm11,xmm11                                                    
  movaps      [rbp],xmm8                                         
  movdqa      xmm8, [r10]                                         
  punpcklwd   xmm2,xmm2                                                      
  punpcklwd   xmm1,xmm1                                                      
  punpcklqdq  xmm12,xmm12                                                    
  punpcklqdq  xmm11,xmm11                                                    
  punpcklqdq  xmm2,xmm2                                                      
  punpcklqdq  xmm1,xmm1                                                      
  shufps      xmm12,xmm11,88h                                                
  movdqa      xmm11,xmm8                                                     
  movaps      [rbp+30h],xmm9                                     
  movdqa      xmm9,[rcx]                                         
  shufps      xmm2,xmm1,88h                                                  
  movdqa      xmm1,xmm5                                                      
  punpcklbw   xmm11,xmm3                                                     
  movaps      [rbp+20h],xmm6                                     
  movaps      [rbp+60h],xmm13                                    
  movdqa      xmm13,xmm11                                                    
  movaps      [rbp+90h],xmm10                                    
  movdqa      xmm10,xmm9                                                     
  movdqa      xmm6,[rax+rcx]                                     
  punpcklbw   xmm1,xmm3                                                      
  movaps      [rbp+0A0h],xmm12                                   
  psubw       xmm13,xmm1                                                     
  movaps      [rbp+40h],xmm15                                    
  movdqa      xmm15,xmm14                                                    
  movaps      [rbp+10h],xmm7                                     
  movdqa      xmm7,xmm6                                                      
  punpcklbw   xmm10,xmm3                                                     
  movdqa      xmm12,[r12+rcx]                                    
  punpcklbw   xmm7,xmm3                                                      
  punpcklbw   xmm12,xmm3                                                     
  punpcklbw   xmm15,xmm3                                                     
  pabsw       xmm3,xmm13                                                     
  movdqa      xmm13,xmm10                                                    
  psubw       xmm13,xmm15                                                    
  movdqa      [rbp+0F0h],xmm15                                   
  pabsw       xmm15,xmm13                                                    
  movdqa      xmm13,xmm11                                                    
  movdqa      [rbp+0B0h],xmm1                                    
  movdqa      xmm1,xmm0                                                      
  pavgw       xmm13,xmm10                                                    
  pcmpgtw     xmm1,xmm3                                                      
  movdqa      [rbp+120h],xmm13                                   
  movaps      xmm13,xmm2                                                     
  punpcklwd   xmm4,xmm4                                                      
  movdqa      xmm3,xmm0                                                      
  movdqa      [rbp+100h],xmm1                                    
  psubw       xmm13,xmm1                                                     
  movdqa      xmm1,xmm10                                                     
  pcmpgtw     xmm3,xmm15                                                     
  pshufd      xmm4,xmm4,0                                                    
  psubw       xmm1,xmm11                                                     
  movdqa      [rbp+0D0h],xmm10                                   
  psubw       xmm13,xmm3                                                     
  movdqa      [rbp+110h],xmm3                                    
  pabsw       xmm15,xmm1                                                     
  movdqa      xmm3,xmm4                                                      
  psubw       xmm10,xmm12                                                    
  pcmpgtw     xmm3,xmm15                                                     
  pabsw       xmm15,xmm10                                                    
  movdqa      xmm10,xmm0                                                     
  psllw       xmm1,2                                                         
  movdqa      [rbp+0C0h],xmm11                                   
  psubw       xmm11,xmm7                                                     
  pcmpgtw     xmm10,xmm15                                                    
  pabsw       xmm11,xmm11                                                    
  movdqa      xmm15,xmm0                                                     
  pand        xmm3,xmm10                                                     
  pcmpgtw     xmm15,xmm11                                                    
  movaps      xmm11,xmm2                                                     
  pxor        xmm10,xmm10                                                    
  pand        xmm3,xmm15                                                     
  pcmpgtw     xmm11,xmm10                                                    
  pcmpeqw     xmm10,xmm2                                                     
  por         xmm11,xmm10                                                    
  pand        xmm3,xmm11                                                     
  movdqa      xmm11,xmm7                                                     
  psubw       xmm11,xmm12                                                    
  pxor        xmm15,xmm15                                                    
  paddw       xmm11,xmm1                                                     
  psubw       xmm15,xmm13                                                    
  movdqa      [rbp+0E0h],xmm12                                   
  paddw       xmm11,[FOUR_16B_SSE2] 
  pxor        xmm12,xmm12                                                    
  psraw       xmm11,3                                                        
  punpckhbw   xmm8,xmm12                                                     
  pmaxsw      xmm15,xmm11                                                    
  punpckhbw   xmm5,xmm12                                                     
  movdqa      xmm11,xmm8                                                     
  pminsw      xmm13,xmm15                                                    
  psubw       xmm11,xmm5                                                     
  punpckhbw   xmm9,xmm12                                                     
  pand        xmm13,xmm3                                                     
  movdqa      [rbp+130h],xmm13                                   
  pabsw       xmm13,xmm11                                                    
  punpckhbw   xmm14,xmm12                                                    
  movdqa      xmm11,xmm9                                                     
  psubw       xmm11,xmm14                                                    
  movdqa      xmm15,xmm0                                                     
  movdqa      [rbp+140h],xmm14                                   
  pabsw       xmm14,xmm11                                                    
  movdqa      xmm11,xmm8                                                     
  pcmpgtw     xmm15,xmm14                                                    
  movdqa      xmm1,[r12+rcx]                                     
  pavgw       xmm11,xmm9                                                     
  movdqa      [rbp+170h],xmm11                                   
  movdqa      xmm10,xmm9                                                     
  punpckhbw   xmm6,xmm12                                                     
  psubw       xmm10,xmm8                                                     
  punpckhbw   xmm1,xmm12                                                     
  movdqa      xmm12,xmm0                                                     
  movaps      xmm11,[rbp+0A0h]                                   
  pcmpgtw     xmm12,xmm13                                                    
  movaps      xmm13,xmm11                                                    
  psubw       xmm13,xmm12                                                    
  movdqa      [rbp+160h],xmm15                                   
  psubw       xmm13,xmm15                                                    
  movdqa      xmm15,xmm9                                                     
  psubw       xmm15,xmm1                                                     
  movdqa      [rbp+150h],xmm12                                   
  pabsw       xmm12,xmm10                                                    
  pabsw       xmm14,xmm15                                                    
  movdqa      xmm15,xmm8                                                     
  pcmpgtw     xmm4,xmm12                                                     
  movdqa      xmm12,xmm0                                                     
  psubw       xmm15,xmm6                                                     
  pcmpgtw     xmm12,xmm14                                                    
  pabsw       xmm14,xmm15                                                    
  psllw       xmm10,2                                                        
  pcmpgtw     xmm0,xmm14                                                     
  movdqa      xmm14,xmm6                                                     
  psubw       xmm14,xmm1                                                     
  pand        xmm4,xmm12                                                     
  paddw       xmm14,xmm10                                                    
  pand        xmm4,xmm0                                                      
  paddw       xmm14,[FOUR_16B_SSE2] 
  pxor        xmm15,xmm15                                                    
  movaps      xmm12,xmm11                                                    
  psubw       xmm15,xmm13                                                    
  pxor        xmm0,xmm0                                                      
  psraw       xmm14,3                                                        
  pcmpgtw     xmm12,xmm0                                                     
  pcmpeqw     xmm0,xmm11                                                     
  pmaxsw      xmm15,xmm14                                                    
  por         xmm12,xmm0                                                     
  movdqa      xmm0,[rbp+120h]                                    
  pminsw      xmm13,xmm15                                                    
  movdqa      xmm15,[rbp+0B0h]                                   
  movdqa      xmm10,xmm7                                                     
  pand        xmm4,xmm12                                                     
  paddw       xmm15,xmm0                                                     
  pxor        xmm12,xmm12                                                    
  paddw       xmm10,xmm7                                                     
  movdqa      xmm14,xmm12                                                    
  psubw       xmm15,xmm10                                                    
  psubw       xmm14,xmm2                                                     
  psraw       xmm15,1                                                        
  pmaxsw      xmm15,xmm14                                                    
  movdqa      xmm10,xmm6                                                     
  pminsw      xmm15,xmm2                                                     
  paddw       xmm10,xmm6                                                     
  pand        xmm15,xmm3                                                     
  psubw       xmm12,xmm11                                                    
  pand        xmm15,[rbp+100h]                                   
  pand        xmm13,xmm4                                                     
  paddw       xmm7,xmm15                                                     
  paddw       xmm8,xmm13                                                     
  movdqa      xmm15,[rbp+170h]                                   
  psubw       xmm9,xmm13                                                     
  paddw       xmm5,xmm15                                                     
  psubw       xmm5,xmm10                                                     
  psraw       xmm5,1                                                         
  pmaxsw      xmm5,xmm12                                                     
  pminsw      xmm5,xmm11                                                     
  pand        xmm5,xmm4                                                      
  pand        xmm5,[rbp+150h]                                    
  paddw       xmm6,xmm5                                                      
  movdqa      xmm5,[rbp+0C0h]                                    
  packuswb    xmm7,xmm6                                                      
  movdqa      xmm6,[rbp+130h]                                    
  paddw       xmm5,xmm6                                                      
  packuswb    xmm5,xmm8                                                      
  movdqa      xmm8,[rbp+0D0h]                                    
  psubw       xmm8,xmm6                                                      
  movdqa      xmm6,[rbp+0F0h]                                    
  paddw       xmm6,xmm0                                                      
  movdqa      xmm0,[rbp+0E0h]                                    
  packuswb    xmm8,xmm9                                                      
  movdqa      xmm9,xmm0                                                      
  paddw       xmm9,xmm0                                                      
  psubw       xmm6,xmm9                                                      
  psraw       xmm6,1                                                         
  pmaxsw      xmm14,xmm6                                                     
  pminsw      xmm2,xmm14                                                     
  pand        xmm2,xmm3                                                      
  pand        xmm2,[rbp+110h]                                    
  paddw       xmm0,xmm2                                                      
  movdqa      xmm2,[rbp+140h]                                    
  paddw       xmm2,xmm15                                                     
  movdqa      xmm15,xmm1                                                     
  paddw       xmm15,xmm1                                                     
  psubw       xmm2,xmm15                                                     
  psraw       xmm2,1                                                         
  pmaxsw      xmm12,xmm2                                                     
  pminsw      xmm11,xmm12                                                    
  pand        xmm11,xmm4                                                     
  pand        xmm11,[rbp+160h]                                   
  paddw       xmm1,xmm11                                                     
  movdqa      [rax+rcx],xmm7                                     
  movdqa      [r10],xmm5                                         
  packuswb    xmm0,xmm1                                                      
  movdqa      [rcx],xmm8                                         
  movdqa      [r12+rcx],xmm0                                                                        
  mov         r12,qword [rbp+180h]                                       
  lea         rsp,[rbp+190h]                                                 
  pop         rbp                                                            
  ret                                                                        


WELS_EXTERN   DeblockLumaEq4V_sse2

ALIGN  16
DeblockLumaEq4V_sse2:
  mov         rax,rsp 
  push        rbx  
  push        rbp  
  push        rsi  
  push        rdi  
  sub         rsp,1D8h 
  movaps      [rax-38h],xmm6 
  movaps      [rax-48h],xmm7 
  movaps      [rax-58h],xmm8 
  pxor        xmm1,xmm1 
  movsxd      r10,edx 
  mov         rbp,rcx 
  mov         r11d,r8d 
  mov         rdx,rcx 
  mov         rdi,rbp 
  mov         rbx,rbp 
  movdqa      xmm5,[rbp] 
  movaps      [rax-68h],xmm9 
  movaps      [rax-78h],xmm10 
  punpcklbw   xmm5,xmm1 
  movaps      [rax-88h],xmm11 
  movaps      [rax-98h],xmm12 
  movaps      [rax-0A8h],xmm13 
  movaps      [rax-0B8h],xmm14 
  movdqa      xmm14,[r10+rbp] 
  movaps      [rax-0C8h],xmm15 
  lea         eax,[r10*4] 
  movsxd      r8,eax 
  lea         eax,[r10+r10*2] 
  movsxd      rcx,eax 
  lea         eax,[r10+r10] 
  sub         rdx,r8 
  punpcklbw   xmm14,xmm1 
  movdqa      [rsp+90h],xmm5 
  movdqa      [rsp+30h],xmm14 
  movsxd      rsi,eax 
  movsx       eax,r11w 
  sub         rdi,rcx 
  sub         rbx,rsi 
  mov         r8,rbp 
  sub         r8,r10 
  movd        xmm0,eax 
  movsx       eax,r9w 
  movdqa      xmm12,[rdi] 
  movdqa      xmm6, [rsi+rbp] 
  movdqa      xmm13,[rbx] 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm11,xmm0,0 
  punpcklbw   xmm13,xmm1 
  punpcklbw   xmm6,xmm1 
  movdqa      xmm8,[r8] 
  movd        xmm0,eax 
  movdqa      xmm10,xmm11 
  mov         eax,2 
  punpcklbw   xmm8,xmm1 
  punpcklbw   xmm12,xmm1 
  cwde             
  punpcklwd   xmm0,xmm0 
  psraw       xmm10,2 
  movdqa      xmm1,xmm8 
  movdqa      [rsp+0F0h],xmm13 
  movdqa      [rsp+0B0h],xmm8 
  pshufd      xmm7,xmm0,0 
  psubw       xmm1,xmm13 
  movdqa      xmm0,xmm5 
  movdqa      xmm4,xmm7 
  movdqa      xmm2,xmm7 
  psubw       xmm0,xmm8 
  pabsw       xmm3,xmm0 
  pabsw       xmm0,xmm1 
  movdqa      xmm1,xmm5 
  movdqa      [rsp+40h],xmm7 
  movdqa      [rsp+60h],xmm6 
  pcmpgtw     xmm4,xmm0 
  psubw       xmm1,xmm14 
  pabsw       xmm0,xmm1 
  pcmpgtw     xmm2,xmm0 
  pand        xmm4,xmm2 
  movdqa      xmm0,xmm11 
  pcmpgtw     xmm0,xmm3 
  pand        xmm4,xmm0 
  movd        xmm0,eax 
  movdqa      [rsp+20h],xmm4 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm2,xmm0,0 
  paddw       xmm10,xmm2 
  movdqa      [rsp+0A0h],xmm2 
  movdqa      xmm15,xmm7 
  pxor        xmm4,xmm4 
  movdqa      xmm0,xmm8 
  psubw       xmm0,xmm12 
  mov         eax,4 
  pabsw       xmm0,xmm0 
  movdqa      xmm1,xmm10 
  cwde             
  pcmpgtw     xmm15,xmm0 
  pcmpgtw     xmm1,xmm3 
  movdqa      xmm3,xmm7 
  movdqa      xmm7,[rdx] 
  movdqa      xmm0,xmm5 
  psubw       xmm0,xmm6 
  pand        xmm15,xmm1 
  punpcklbw   xmm7,xmm4 
  movdqa      xmm9,xmm15 
  pabsw       xmm0,xmm0 
  psllw       xmm7,1 
  pandn       xmm9,xmm12 
  pcmpgtw     xmm3,xmm0 
  paddw       xmm7,xmm12 
  movd        xmm0,eax 
  pand        xmm3,xmm1 
  paddw       xmm7,xmm12 
  punpcklwd   xmm0,xmm0 
  paddw       xmm7,xmm12 
  pshufd      xmm1,xmm0,0 
  paddw       xmm7,xmm13 
  movdqa      xmm0,xmm3 
  pandn       xmm0,xmm6 
  paddw       xmm7,xmm8 
  movdqa      [rsp+70h],xmm1 
  paddw       xmm7,xmm5 
  movdqa      [rsp+120h],xmm0 
  movdqa      xmm0,[rcx+rbp] 
  punpcklbw   xmm0,xmm4 
  paddw       xmm7,xmm1 
  movdqa      xmm4,xmm15 
  psllw       xmm0,1 
  psraw       xmm7,3 
  paddw       xmm0,xmm6 
  pand        xmm7,xmm15 
  paddw       xmm0,xmm6 
  paddw       xmm0,xmm6 
  paddw       xmm0,xmm14 
  movdqa      xmm6,xmm15 
  paddw       xmm0,xmm5 
  pandn       xmm6,xmm13 
  paddw       xmm0,xmm8 
  paddw       xmm0,xmm1 
  psraw       xmm0,3 
  movdqa      xmm1,xmm12 
  paddw       xmm1,xmm13 
  pand        xmm0,xmm3 
  movdqa      [rsp+100h],xmm0 
  movdqa      xmm0,xmm8 
  paddw       xmm0,xmm5 
  paddw       xmm1,xmm0 
  movdqa      xmm0,xmm3 
  paddw       xmm1,xmm2 
  psraw       xmm1,2 
  pandn       xmm0,xmm14 
  pand        xmm4,xmm1 
  movdqa      [rsp+0E0h],xmm0 
  movdqa      xmm0,xmm5 
  paddw       xmm0,xmm8 
  movdqa      xmm1,[rsp+60h] 
  paddw       xmm1,xmm14 
  movdqa      xmm14,xmm3 
  paddw       xmm1,xmm0 
  movdqa      xmm0,xmm8 
  paddw       xmm0,[rsp+30h] 
  paddw       xmm1,xmm2 
  psraw       xmm1,2 
  pand        xmm14,xmm1 
  movdqa      xmm1,xmm13 
  paddw       xmm1,xmm13 
  paddw       xmm1,xmm0 
  paddw       xmm1,xmm2 
  psraw       xmm1,2 
  movdqa      xmm0,[rsp+30h] 
  movdqa      xmm2,xmm13 
  movdqa      xmm5,xmm15 
  paddw       xmm0,[rsp+70h] 
  pandn       xmm5,xmm1 
  paddw       xmm2,xmm8 
  movdqa      xmm8,[rsp+90h] 
  movdqa      xmm1,xmm12 
  paddw       xmm2,xmm8 
  psllw       xmm2,1 
  paddw       xmm2,xmm0 
  paddw       xmm1,xmm2 
  movdqa      xmm0,xmm8 
  movdqa      xmm8,xmm3 
  movdqa      xmm2,[rsp+30h] 
  paddw       xmm0,xmm13 
  psraw       xmm1,3 
  pand        xmm15,xmm1 
  movdqa      xmm1,xmm2 
  paddw       xmm1,xmm2 
  paddw       xmm2,[rsp+90h] 
  paddw       xmm2,[rsp+0B0h] 
  paddw       xmm1,xmm0 
  movdqa      xmm0,xmm13 
  movdqa      xmm13,[r8] 
  paddw       xmm0, [rsp+70h] 
  paddw       xmm1, [rsp+0A0h] 
  psllw       xmm2,1 
  paddw       xmm2,xmm0 
  psraw       xmm1,2 
  movdqa      xmm0, [rdi] 
  pandn       xmm8,xmm1 
  movdqa      xmm1, [rsp+60h] 
  paddw       xmm1,xmm2 
  movdqa      xmm2, [rbx] 
  psraw       xmm1,3 
  pand        xmm3,xmm1 
  movdqa      xmm1, [rbp] 
  movdqa      [rsp+0D0h],xmm3 
  pxor        xmm3,xmm3 
  punpckhbw   xmm0,xmm3 
  punpckhbw   xmm1,xmm3 
  punpckhbw   xmm13,xmm3 
  movdqa      [rsp+0C0h],xmm0 
  movdqa      xmm0,[r10+rbp] 
  movdqa      [rsp],xmm1 
  punpckhbw   xmm0,xmm3 
  punpckhbw   xmm2,xmm3 
  movdqa      [rsp+80h],xmm0 
  movdqa      xmm0,[rsi+rbp] 
  movdqa      [rsp+10h],xmm13 
  punpckhbw   xmm0,xmm3 
  movdqa      [rsp+50h],xmm0 
  movdqa      xmm0,xmm1 
  movdqa      xmm1,xmm13 
  psubw       xmm0,xmm13 
  psubw       xmm1,xmm2 
  pabsw       xmm3,xmm0 
  pabsw       xmm0,xmm1 
  movdqa      xmm1,[rsp] 
  movdqa      xmm13,[rsp+40h] 
  movdqa      [rsp+110h],xmm2 
  psubw       xmm1, [rsp+80h] 
  pcmpgtw     xmm13,xmm0 
  pcmpgtw     xmm11,xmm3 
  pabsw       xmm0,xmm1 
  pcmpgtw     xmm10,xmm3 
  movdqa      xmm1, [rsp+40h] 
  movdqa      xmm2,xmm1 
  movdqa      xmm3,xmm1 
  pcmpgtw     xmm2,xmm0 
  movdqa      xmm0, [rsp+10h] 
  pand        xmm13,xmm2 
  pand        xmm13,xmm11 
  movdqa      xmm11,[rsp+0C0h] 
  psubw       xmm0,xmm11 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm3,xmm0 
  pand        xmm3,xmm10 
  movdqa      xmm0,[rsp] 
  psubw       xmm0,[rsp+50h] 
  movdqa      xmm2,[rdx] 
  pabsw       xmm0,xmm0 
  por         xmm7,xmm9 
  movdqa      xmm9,[rsp+20h] 
  pcmpgtw     xmm1,xmm0 
  pand        xmm9,xmm7 
  movdqa      xmm7,[rsp+20h] 
  movdqa      xmm0,xmm7 
  pandn       xmm0,xmm12 
  movdqa      xmm12,[rsp+110h] 
  pand        xmm1,xmm10 
  movdqa      xmm10,[rsp+70h] 
  movdqa      [rsp+40h],xmm1 
  movdqa      xmm1,xmm13 
  por         xmm9,xmm0 
  pxor        xmm0,xmm0 
  por         xmm4,xmm6 
  movdqa      xmm6,xmm7 
  punpckhbw   xmm2,xmm0 
  por         xmm15,xmm5 
  movdqa      xmm5,[rsp+20h] 
  movdqa      xmm0,xmm3 
  psllw       xmm2,1 
  pandn       xmm0,xmm11 
  pand        xmm6,xmm4 
  movdqa      xmm4,[rsp] 
  paddw       xmm2,xmm11 
  pand        xmm5,xmm15 
  movdqa      xmm15,[rsp+20h] 
  paddw       xmm2,xmm11 
  paddw       xmm2,xmm11 
  paddw       xmm2,xmm12 
  paddw       xmm2,[rsp+10h] 
  paddw       xmm2,[rsp] 
  paddw       xmm2,xmm10 
  psraw       xmm2,3 
  pand        xmm2,xmm3 
  por         xmm2,xmm0 
  pand        xmm1,xmm2 
  movdqa      xmm0,xmm13 
  movdqa      xmm2,xmm11 
  pandn       xmm0,xmm11 
  paddw       xmm2,xmm12 
  por         xmm1,xmm0 
  packuswb    xmm9,xmm1 
  movdqa      xmm0,xmm7 
  movdqa      xmm7,[rsp+0A0h] 
  pandn       xmm0,[rsp+0F0h] 
  movdqa      xmm1,xmm3 
  por         xmm6,xmm0 
  movdqa      xmm0,[rsp+10h] 
  paddw       xmm0,xmm4 
  paddw       xmm2,xmm0 
  paddw       xmm2,xmm7 
  movdqa      xmm0,xmm3 
  pandn       xmm0,xmm12 
  psraw       xmm2,2 
  pand        xmm1,xmm2 
  por         xmm1,xmm0 
  movdqa      xmm2,xmm13 
  movdqa      xmm0,xmm13 
  pand        xmm2,xmm1 
  pandn       xmm0,xmm12 
  movdqa      xmm1,xmm12 
  paddw       xmm1,[rsp+10h] 
  por         xmm2,xmm0 
  movdqa      xmm0,xmm15 
  pandn       xmm0,[rsp+0B0h] 
  paddw       xmm1,xmm4 
  packuswb    xmm6,xmm2 
  movdqa      xmm2,xmm3 
  psllw       xmm1,1 
  por         xmm5,xmm0 
  movdqa      xmm0,[rsp+80h] 
  paddw       xmm0,xmm10 
  paddw       xmm1,xmm0 
  paddw       xmm11,xmm1 
  psraw       xmm11,3 
  movdqa      xmm1,xmm12 
  pand        xmm2,xmm11 
  paddw       xmm1,xmm12 
  movdqa      xmm11,[rsp+80h] 
  movdqa      xmm0, [rsp+10h] 
  por         xmm14,[rsp+0E0h] 
  paddw       xmm0,xmm11 
  movdqa      xmm4,xmm15 
  paddw       xmm1,xmm0 
  movdqa      xmm0,xmm13 
  paddw       xmm1,xmm7 
  psraw       xmm1,2 
  pandn       xmm3,xmm1 
  por         xmm2,xmm3 
  movdqa      xmm1,xmm13 
  movdqa      xmm3,[rsp+10h] 
  pandn       xmm0,xmm3 
  pand        xmm1,xmm2 
  movdqa      xmm2,xmm11 
  paddw       xmm2,[rsp] 
  por         xmm1,xmm0 
  movdqa      xmm0,[rsp+0D0h] 
  por         xmm0,xmm8 
  paddw       xmm2,xmm3 
  packuswb    xmm5,xmm1 
  movdqa      xmm8,[rsp+40h] 
  movdqa      xmm1,[rsp+50h] 
  movdqa      xmm3,xmm8 
  pand        xmm4,xmm0 
  psllw       xmm2,1 
  movdqa      xmm0,xmm15 
  pandn       xmm0,[rsp+90h] 
  por         xmm4,xmm0 
  movdqa      xmm0,xmm12 
  paddw       xmm0,xmm10 
  paddw       xmm2,xmm0 
  paddw       xmm1,xmm2 
  movdqa      xmm0,[rsp] 
  movdqa      xmm2,xmm11 
  paddw       xmm0,xmm12 
  movdqa      xmm12,[rsp] 
  paddw       xmm2,xmm11 
  paddw       xmm2,xmm0 
  psraw       xmm1,3 
  movdqa      xmm0,xmm8 
  pand        xmm3,xmm1 
  paddw       xmm2,xmm7 
  movdqa      xmm1,xmm13 
  psraw       xmm2,2 
  pandn       xmm0,xmm2 
  por         xmm3,xmm0 
  movdqa      xmm2,[rsp+50h] 
  movdqa      xmm0,xmm13 
  pandn       xmm0,xmm12 
  pand        xmm1,xmm3 
  paddw       xmm2,xmm11 
  movdqa      xmm3,xmm15 
  por         xmm1,xmm0 
  pand        xmm3,xmm14 
  movdqa      xmm14,[rsp+10h] 
  movdqa      xmm0,xmm15 
  pandn       xmm0,[rsp+30h] 
  packuswb    xmm4,xmm1 
  movdqa      xmm1,xmm8 
  por         xmm3,xmm0 
  movdqa      xmm0,xmm12 
  paddw       xmm0,xmm14 
  paddw       xmm2,xmm0 
  paddw       xmm2,xmm7 
  movdqa      xmm0,xmm8 
  pandn       xmm0,xmm11 
  psraw       xmm2,2 
  pand        xmm1,xmm2 
  por         xmm1,xmm0 
  movdqa      xmm2,xmm13 
  movdqa      xmm0,xmm13 
  pandn       xmm0,xmm11 
  pand        xmm2,xmm1 
  movdqa      xmm1,xmm15 
  por         xmm2,xmm0 
  packuswb    xmm3,xmm2 
  movdqa      xmm0,[rsp+100h] 
  por         xmm0,[rsp+120h] 
  pand        xmm1,xmm0 
  movdqa      xmm2,[rcx+rbp] 
  movdqa      xmm7,[rsp+50h] 
  pandn       xmm15,[rsp+60h] 
  lea         r11,[rsp+1D8h] 
  pxor        xmm0,xmm0 
  por         xmm1,xmm15 
  movaps      xmm15,[r11-0A8h] 
  movdqa      [rdi],xmm9 
  movaps      xmm9,[r11-48h] 
  punpckhbw   xmm2,xmm0 
  psllw       xmm2,1 
  paddw       xmm2,xmm7 
  paddw       xmm2,xmm7 
  movdqa      [rbx],xmm6 
  movaps      xmm6,[r11-18h] 
  paddw       xmm2,xmm7 
  paddw       xmm2,xmm11 
  movaps      xmm11,[r11-68h] 
  paddw       xmm2,xmm12 
  movaps      xmm12,[r11-78h] 
  paddw       xmm2,xmm14 
  paddw       xmm2,xmm10 
  psraw       xmm2,3 
  movaps      xmm10,[r11-58h] 
  movaps      xmm14,[r11-98h] 
  movdqa      xmm0,xmm13 
  pand        xmm2,xmm8 
  pandn       xmm8,xmm7 
  pandn       xmm13,xmm7 
  por         xmm2,xmm8 
  movaps      xmm7,[r11-28h] 
  movaps      xmm8,[r11-38h] 
  movdqa      [r8],xmm5 
  pand        xmm0,xmm2 
  por         xmm0,xmm13 
  packuswb    xmm1,xmm0 
  movaps      xmm13,[r11-88h] 
  movdqa      [rbp],xmm4 
  movdqa      [r10+rbp],xmm3 
  movdqa      [rsi+rbp],xmm1 
  mov         rsp,r11 
  pop         rdi  
  pop         rsi  
  pop         rbp  
  pop         rbx  
  ret


WELS_EXTERN  DeblockChromaLt4V_sse2

ALIGN  16
DeblockChromaLt4V_sse2:
  mov         rax,rsp 
  push        rbx  
  push        rdi     
  sub         rsp,0C8h 
  mov         r10,qword [rax + 30h]  ; pTC
  pxor        xmm1,xmm1 
  mov         rbx,rcx 
  movsxd      r11,r8d 
  movsx       ecx,byte [r10] 
  movsx       r8d,byte [r10+2] 
  mov         rdi,rdx 
  movq        xmm2,[rbx] 
  movq        xmm9,[r11+rbx] 
  movsx       edx,byte [r10+1] 
  mov         word [rsp+2],cx 
  mov         word [rsp],cx 
  movsx       eax,byte [r10+3] 
  mov         word [rsp+6],dx 
  mov         word [rsp+4],dx 
  movdqa      xmm11,xmm1 
  mov         word [rsp+0Eh],ax 
  mov         word [rsp+0Ch],ax 
  lea         eax,[r11+r11] 
  movsxd      rcx,eax 
  mov         rax,rbx 
  mov         rdx,rdi 
  sub         rax,rcx 
  mov         word [rsp+0Ah],r8w 
  mov         word [rsp+8],r8w 
  movdqa      xmm6,[rsp] 
  movdqa      xmm7,xmm6 
  movq        xmm13, [rax] 
  mov         rax,rdi 
  sub         rax,rcx 
  mov         rcx,rbx 
  pcmpgtw     xmm7,xmm1 
  psubw       xmm11,xmm6 
  sub         rcx,r11 
  sub         rdx,r11 
  movq        xmm0,[rax] 
  movsx       eax,r9w 
  movq        xmm15,[rcx] 
  punpcklqdq  xmm13,xmm0 
  movq        xmm0, [rdx] 
  movdqa      xmm4,xmm13 
  punpcklqdq  xmm15,xmm0 
  movq        xmm0, [rdi] 
  punpcklbw   xmm4,xmm1 
  movdqa      xmm12,xmm15 
  punpcklqdq  xmm2,xmm0 
  movq        xmm0, [r11+rdi] 
  punpcklbw   xmm12,xmm1 
  movdqa      xmm14,xmm2 
  punpcklqdq  xmm9,xmm0 
  punpckhbw   xmm2,xmm1 
  punpcklbw   xmm14,xmm1 
  movd        xmm0,eax 
  movsx       eax,word [rsp + 0C8h + 38h] ; iBeta
  punpckhbw   xmm13,xmm1 
  punpckhbw   xmm15,xmm1 
  movdqa      xmm3,xmm9 
  movdqa      [rsp+10h],xmm2 
  punpcklwd   xmm0,xmm0 
  punpckhbw   xmm9,xmm1 
  punpcklbw   xmm3,xmm1 
  movdqa      xmm1,xmm14 
  pshufd      xmm10,xmm0,0 
  movd        xmm0,eax 
  mov         eax,4 
  cwde             
  punpcklwd   xmm0,xmm0 
  pshufd      xmm8,xmm0,0 
  movd        xmm0,eax 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm5,xmm0,0 
  psubw       xmm1,xmm12 
  movdqa      xmm2,xmm10 
  lea         r11,[rsp+0C8h] 
  psllw       xmm1,2 
  movdqa      xmm0,xmm4 
  psubw       xmm4,xmm12 
  psubw       xmm0,xmm3 
  psubw       xmm3,xmm14 
  paddw       xmm1,xmm0 
  paddw       xmm1,xmm5 
  movdqa      xmm0,xmm11 
  psraw       xmm1,3 
  pmaxsw      xmm0,xmm1 
  pminsw      xmm6,xmm0 
  movdqa      xmm1,xmm8 
  movdqa      xmm0,xmm12 
  psubw       xmm0,xmm14 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm2,xmm0 
  pabsw       xmm0,xmm4 
  pcmpgtw     xmm1,xmm0 
  pabsw       xmm0,xmm3 
  movdqa      xmm3,[rsp] 
  pand        xmm2,xmm1 
  movdqa      xmm1,xmm8 
  pcmpgtw     xmm1,xmm0 
  movdqa      xmm0,xmm13 
  pand        xmm2,xmm1 
  psubw       xmm0,xmm9 
  psubw       xmm13,xmm15 
  pand        xmm2,xmm7 
  pand        xmm6,xmm2 
  paddw       xmm12,xmm6 
  psubw       xmm14,xmm6 
  movdqa      xmm2,[rsp+10h] 
  movaps      xmm6,[r11-18h] 
  movdqa      xmm1,xmm2 
  psubw       xmm1,xmm15 
  psubw       xmm9,xmm2 
  psllw       xmm1,2 
  paddw       xmm1,xmm0 
  paddw       xmm1,xmm5 
  movdqa      xmm0,xmm15 
  psubw       xmm0,xmm2 
  psraw       xmm1,3 
  pmaxsw      xmm11,xmm1 
  pabsw       xmm0,xmm0 
  movdqa      xmm1,xmm8 
  pcmpgtw     xmm10,xmm0 
  pabsw       xmm0,xmm13 
  pminsw      xmm3,xmm11 
  movaps      xmm11,[r11-68h] 
  movaps      xmm13,[rsp+40h] 
  pcmpgtw     xmm1,xmm0 
  pabsw       xmm0,xmm9 
  movaps      xmm9, [r11-48h] 
  pand        xmm10,xmm1 
  pcmpgtw     xmm8,xmm0 
  pand        xmm10,xmm8 
  pand        xmm10,xmm7 
  movaps      xmm8,[r11-38h] 
  movaps      xmm7,[r11-28h] 
  pand        xmm3,xmm10 
  paddw       xmm15,xmm3 
  psubw       xmm2,xmm3 
  movaps      xmm10,[r11-58h] 
  packuswb    xmm12,xmm15 
  movaps      xmm15,[rsp+20h] 
  packuswb    xmm14,xmm2 
  movq        [rcx],xmm12 
  movq        [rbx],xmm14 
  psrldq      xmm12,8 
  psrldq      xmm14,8 
  movq        [rdx],xmm12 
  movaps      xmm12,[r11-78h] 
  movq        [rdi],xmm14 
  movaps      xmm14,[rsp+30h] 
  mov         rsp,r11 
  pop         rdi  
  pop         rbx  
  ret


WELS_EXTERN   DeblockChromaEq4V_sse2
ALIGN 16
DeblockChromaEq4V_sse2:
  mov         rax,rsp 
  push        rbx  
  sub         rsp,90h 
  pxor        xmm1,xmm1 
  mov         r11,rcx 
  mov         rbx,rdx 
  mov         r10d,r9d   
  movq        xmm13,[r11] 
  lea         eax,[r8+r8] 
  movsxd      r9,eax 
  mov         rax,rcx 
  sub         rax,r9 
  movq        xmm14,[rax] 
  mov         rax,rdx 
  sub         rax,r9 
  movq        xmm0,[rax] 
  movsxd      rax,r8d 
  sub         rcx,rax 
  sub         rdx,rax 
  movq        xmm12,[rax+r11] 
  movq        xmm10,[rcx] 
  punpcklqdq  xmm14,xmm0 
  movdqa      xmm8,xmm14 
  movq        xmm0,[rdx] 
  punpcklbw   xmm8,xmm1 
  punpckhbw   xmm14,xmm1 
  punpcklqdq  xmm10,xmm0 
  movq        xmm0,[rbx] 
  movdqa      xmm5,xmm10 
  punpcklqdq  xmm13,xmm0 
  movq        xmm0, [rax+rbx] 
  punpcklbw   xmm5,xmm1 
  movsx       eax,r10w 
  movdqa      xmm9,xmm13 
  punpcklqdq  xmm12,xmm0 
  punpcklbw   xmm9,xmm1 
  punpckhbw   xmm10,xmm1 
  movd        xmm0,eax 
  movsx       eax,word [rsp + 90h + 8h + 28h]   ; iBeta
  punpckhbw   xmm13,xmm1 
  movdqa      xmm7,xmm12 
  punpcklwd   xmm0,xmm0 
  punpckhbw   xmm12,xmm1 
  pshufd      xmm11,xmm0,0 
  punpcklbw   xmm7,xmm1 
  movd        xmm0,eax 
  movdqa      xmm1,xmm8 
  psubw       xmm1,xmm5 
  punpcklwd   xmm0,xmm0 
  movdqa      xmm6,xmm11 
  pshufd      xmm3,xmm0,0 
  movdqa      xmm0,xmm5 
  psubw       xmm0,xmm9 
  movdqa      xmm2,xmm3 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm6,xmm0 
  pabsw       xmm0,xmm1 
  movdqa      xmm1,xmm3 
  pcmpgtw     xmm2,xmm0 
  pand        xmm6,xmm2 
  movdqa      xmm0,xmm7 
  movdqa      xmm2,xmm3 
  psubw       xmm0,xmm9 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm1,xmm0 
  pand        xmm6,xmm1 
  movdqa      xmm0,xmm10 
  movdqa      xmm1,xmm14 
  psubw       xmm0,xmm13 
  psubw       xmm1,xmm10 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm11,xmm0 
  pabsw       xmm0,xmm1 
  pcmpgtw     xmm2,xmm0 
  pand        xmm11,xmm2 
  movdqa      xmm0,xmm12 
  movdqa      xmm4,xmm6 
  movdqa      xmm1,xmm8 
  mov         eax,2 
  cwde             
  paddw       xmm1,xmm8 
  psubw       xmm0,xmm13 
  paddw       xmm1,xmm5 
  pabsw       xmm0,xmm0 
  movdqa      xmm2,xmm14 
  paddw       xmm1,xmm7 
  pcmpgtw     xmm3,xmm0 
  paddw       xmm2,xmm14 
  movd        xmm0,eax 
  pand        xmm11,xmm3 
  paddw       xmm7,xmm7 
  paddw       xmm2,xmm10 
  punpcklwd   xmm0,xmm0 
  paddw       xmm2,xmm12 
  paddw       xmm12,xmm12 
  pshufd      xmm3,xmm0,0 
  paddw       xmm7,xmm9 
  paddw       xmm12,xmm13 
  movdqa      xmm0,xmm6 
  paddw       xmm1,xmm3 
  pandn       xmm0,xmm5 
  paddw       xmm7,xmm8 
  psraw       xmm1,2 
  paddw       xmm12,xmm14 
  paddw       xmm7,xmm3 
  movaps      xmm14,[rsp] 
  pand        xmm4,xmm1 
  paddw       xmm12,xmm3 
  psraw       xmm7,2 
  movdqa      xmm1,xmm11 
  por         xmm4,xmm0 
  psraw       xmm12,2 
  paddw       xmm2,xmm3 
  movdqa      xmm0,xmm11 
  pandn       xmm0,xmm10 
  psraw       xmm2,2 
  pand        xmm1,xmm2 
  por         xmm1,xmm0 
  packuswb    xmm4,xmm1 
  movdqa      xmm0,xmm11 
  movdqa      xmm1,xmm6 
  pand        xmm1,xmm7 
  movaps      xmm7,[rsp+70h] 
  movq        [rcx],xmm4 
  pandn       xmm6,xmm9 
  pandn       xmm11,xmm13 
  pand        xmm0,xmm12 
  por         xmm1,xmm6 
  por         xmm0,xmm11 
  psrldq      xmm4,8 
  packuswb    xmm1,xmm0 
  movq        [r11],xmm1 
  psrldq      xmm1,8 
  movq        [rdx],xmm4 
  lea         r11,[rsp+90h] 
  movaps      xmm6,[r11-10h] 
  movaps      xmm8,[r11-30h] 
  movaps      xmm9,[r11-40h] 
  movq        [rbx],xmm1 
  movaps      xmm10,[r11-50h] 
  movaps      xmm11,[r11-60h] 
  movaps      xmm12,[r11-70h] 
  movaps      xmm13,[r11-80h] 
  mov         rsp,r11 
  pop         rbx  
  ret





WELS_EXTERN   DeblockChromaEq4H_sse2
ALIGN  16
DeblockChromaEq4H_sse2:
  mov         rax,rsp 
  mov         [rax+20h],rbx 
  push        rdi  
  sub         rsp,140h    
  mov         rdi,rdx 
  lea         eax,[r8*4] 
  movsxd      r10,eax 
  mov         eax,[rcx-2] 
  mov         [rsp+10h],eax 
  lea         rbx,[r10+rdx-2] 
  lea         r11,[r10+rcx-2] 
  movdqa      xmm5,[rsp+10h] 
  movsxd      r10,r8d 
  mov         eax,[r10+rcx-2] 
  lea         rdx,[r10+r10*2] 
  mov         [rsp+20h],eax 
  mov         eax,[rcx+r10*2-2] 
  mov         [rsp+30h],eax 
  mov         eax,[rdx+rcx-2] 
  movdqa      xmm2,[rsp+20h] 
  mov         [rsp+40h],eax 
  mov         eax, [rdi-2] 
  movdqa      xmm4,[rsp+30h] 
  mov         [rsp+50h],eax 
  mov         eax,[r10+rdi-2] 
  movdqa      xmm3,[rsp+40h] 
  mov         [rsp+60h],eax 
  mov         eax,[rdi+r10*2-2] 
  punpckldq   xmm5,[rsp+50h] 
  mov         [rsp+70h],eax 
  mov         eax, [rdx+rdi-2] 
  punpckldq   xmm2, [rsp+60h] 
  mov          [rsp+80h],eax 
  mov         eax,[r11] 
  punpckldq   xmm4, [rsp+70h] 
  mov         [rsp+50h],eax 
  mov         eax,[rbx] 
  punpckldq   xmm3,[rsp+80h] 
  mov         [rsp+60h],eax 
  mov         eax,[r10+r11] 
  movdqa      xmm0, [rsp+50h] 
  punpckldq   xmm0, [rsp+60h] 
  punpcklqdq  xmm5,xmm0 
  movdqa      [rsp+50h],xmm0 
  mov         [rsp+50h],eax 
  mov         eax,[r10+rbx] 
  movdqa      xmm0,[rsp+50h] 
  movdqa      xmm1,xmm5 
  mov         [rsp+60h],eax 
  mov         eax,[r11+r10*2] 
  punpckldq   xmm0, [rsp+60h] 
  punpcklqdq  xmm2,xmm0 
  punpcklbw   xmm1,xmm2 
  punpckhbw   xmm5,xmm2 
  movdqa      [rsp+50h],xmm0 
  mov         [rsp+50h],eax 
  mov         eax,[rbx+r10*2] 
  movdqa      xmm0,[rsp+50h] 
  mov         [rsp+60h],eax 
  mov         eax, [rdx+r11] 
  movdqa      xmm15,xmm1 
  punpckldq   xmm0,[rsp+60h] 
  punpcklqdq  xmm4,xmm0 
  movdqa      [rsp+50h],xmm0 
  mov         [rsp+50h],eax 
  mov         eax, [rdx+rbx] 
  movdqa      xmm0,[rsp+50h] 
  mov         [rsp+60h],eax 
  punpckldq   xmm0, [rsp+60h] 
  punpcklqdq  xmm3,xmm0 
  movdqa      xmm0,xmm4 
  punpcklbw   xmm0,xmm3 
  punpckhbw   xmm4,xmm3 
  punpcklwd   xmm15,xmm0 
  punpckhwd   xmm1,xmm0 
  movdqa      xmm0,xmm5 
  movdqa      xmm12,xmm15 
  punpcklwd   xmm0,xmm4 
  punpckhwd   xmm5,xmm4 
  punpckldq   xmm12,xmm0 
  punpckhdq   xmm15,xmm0 
  movdqa      xmm0,xmm1 
  movdqa      xmm11,xmm12 
  punpckldq   xmm0,xmm5 
  punpckhdq   xmm1,xmm5 
  punpcklqdq  xmm11,xmm0 
  punpckhqdq  xmm12,xmm0 
  movsx       eax,r9w 
  movdqa      xmm14,xmm15 
  punpcklqdq  xmm14,xmm1 
  punpckhqdq  xmm15,xmm1 
  pxor        xmm1,xmm1 
  movd        xmm0,eax 
  movdqa      xmm4,xmm12 
  movdqa      xmm8,xmm11 
  movsx       eax,word [rsp+170h] ; iBeta
  punpcklwd   xmm0,xmm0 
  punpcklbw   xmm4,xmm1 
  punpckhbw   xmm12,xmm1 
  movdqa      xmm9,xmm14 
  movdqa      xmm7,xmm15 
  movdqa      xmm10,xmm15 
  pshufd      xmm13,xmm0,0 
  punpcklbw   xmm9,xmm1 
  punpckhbw   xmm14,xmm1 
  movdqa      xmm6,xmm13 
  movd        xmm0,eax 
  movdqa      [rsp],xmm11 
  mov         eax,2 
  cwde             
  punpckhbw   xmm11,xmm1 
  punpckhbw   xmm10,xmm1 
  punpcklbw   xmm7,xmm1 
  punpcklwd   xmm0,xmm0 
  punpcklbw   xmm8,xmm1 
  pshufd      xmm3,xmm0,0 
  movdqa      xmm1,xmm8 
  movdqa      xmm0,xmm4 
  psubw       xmm0,xmm9 
  psubw       xmm1,xmm4 
  movdqa      xmm2,xmm3 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm6,xmm0 
  pabsw       xmm0,xmm1 
  movdqa      xmm1,xmm3 
  pcmpgtw     xmm2,xmm0 
  pand        xmm6,xmm2 
  movdqa      xmm0,xmm7 
  movdqa      xmm2,xmm3 
  psubw       xmm0,xmm9 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm1,xmm0 
  pand        xmm6,xmm1 
  movdqa      xmm0,xmm12 
  movdqa      xmm1,xmm11 
  psubw       xmm0,xmm14 
  psubw       xmm1,xmm12 
  movdqa      xmm5,xmm6 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm13,xmm0 
  pabsw       xmm0,xmm1 
  movdqa      xmm1,xmm8 
  pcmpgtw     xmm2,xmm0 
  paddw       xmm1,xmm8 
  movdqa      xmm0,xmm10 
  pand        xmm13,xmm2 
  psubw       xmm0,xmm14 
  paddw       xmm1,xmm4 
  movdqa      xmm2,xmm11 
  pabsw       xmm0,xmm0 
  paddw       xmm2,xmm11 
  paddw       xmm1,xmm7 
  pcmpgtw     xmm3,xmm0 
  paddw       xmm2,xmm12 
  movd        xmm0,eax 
  pand        xmm13,xmm3 
  paddw       xmm2,xmm10 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm3,xmm0,0 
  movdqa      xmm0,xmm6 
  paddw       xmm1,xmm3 
  pandn       xmm0,xmm4 
  paddw       xmm2,xmm3 
  psraw       xmm1,2 
  pand        xmm5,xmm1 
  por         xmm5,xmm0 
  paddw       xmm7,xmm7 
  paddw       xmm10,xmm10 
  psraw       xmm2,2 
  movdqa      xmm1,xmm13 
  movdqa      xmm0,xmm13 
  pandn       xmm0,xmm12 
  pand        xmm1,xmm2 
  paddw       xmm7,xmm9 
  por         xmm1,xmm0 
  paddw       xmm10,xmm14 
  paddw       xmm7,xmm8 
  movdqa      xmm0,xmm13 
  packuswb    xmm5,xmm1 
  paddw       xmm7,xmm3 
  paddw       xmm10,xmm11 
  movdqa      xmm1,xmm6 
  paddw       xmm10,xmm3 
  pandn       xmm6,xmm9 
  psraw       xmm7,2 
  pand        xmm1,xmm7 
  psraw       xmm10,2 
  pandn       xmm13,xmm14 
  pand        xmm0,xmm10 
  por         xmm1,xmm6 
  movdqa      xmm6,[rsp] 
  movdqa      xmm4,xmm6 
  por         xmm0,xmm13 
  punpcklbw   xmm4,xmm5 
  punpckhbw   xmm6,xmm5 
  movdqa      xmm3,xmm4 
  packuswb    xmm1,xmm0 
  movdqa      xmm0,xmm1 
  punpckhbw   xmm1,xmm15 
  punpcklbw   xmm0,xmm15 
  punpcklwd   xmm3,xmm0 
  punpckhwd   xmm4,xmm0 
  movdqa      xmm0,xmm6 
  movdqa      xmm2,xmm3 
  punpcklwd   xmm0,xmm1 
  punpckhwd   xmm6,xmm1 
  movdqa      xmm1,xmm4 
  punpckldq   xmm2,xmm0 
  punpckhdq   xmm3,xmm0 
  punpckldq   xmm1,xmm6 
  movdqa      xmm0,xmm2 
  punpcklqdq  xmm0,xmm1 
  punpckhdq   xmm4,xmm6 
  punpckhqdq  xmm2,xmm1 
  movdqa      [rsp+10h],xmm0 
  movdqa      [rsp+60h],xmm2 
  movdqa      xmm0,xmm3 
  mov         eax,[rsp+10h] 
  mov         [rcx-2],eax 
  mov         eax,[rsp+60h] 
  punpcklqdq  xmm0,xmm4 
  punpckhqdq  xmm3,xmm4 
  mov         [r10+rcx-2],eax 
  movdqa      [rsp+20h],xmm0 
  mov         eax, [rsp+20h] 
  movdqa      [rsp+70h],xmm3 
  mov         [rcx+r10*2-2],eax 
  mov         eax,[rsp+70h] 
  mov         [rdx+rcx-2],eax 
  mov         eax,[rsp+18h] 
  mov         [r11],eax 
  mov         eax,[rsp+68h] 
  mov         [r10+r11],eax 
  mov         eax,[rsp+28h] 
  mov         [r11+r10*2],eax 
  mov         eax,[rsp+78h] 
  mov         [rdx+r11],eax 
  mov         eax,[rsp+14h] 
  mov         [rdi-2],eax 
  mov         eax,[rsp+64h] 
  mov         [r10+rdi-2],eax 
  mov         eax,[rsp+24h] 
  mov         [rdi+r10*2-2],eax 
  mov         eax, [rsp+74h] 
  mov         [rdx+rdi-2],eax 
  mov         eax, [rsp+1Ch] 
  mov         [rbx],eax 
  mov         eax, [rsp+6Ch] 
  mov         [r10+rbx],eax 
  mov         eax,[rsp+2Ch] 
  mov         [rbx+r10*2],eax 
  mov         eax,[rsp+7Ch] 
  mov         [rdx+rbx],eax  
  lea         r11,[rsp+140h] 
  mov         rbx, [r11+28h]    
  mov         rsp,r11 
  pop         rdi  
  ret



WELS_EXTERN DeblockChromaLt4H_sse2
ALIGN  16
DeblockChromaLt4H_sse2:
  mov         rax,rsp 
  push        rbx  
  push        rbp  
  push        rsi  
  push        rdi  
  push        r12  
  sub         rsp,170h  
  
  movsxd      rsi,r8d 
  lea         eax,[r8*4] 
  mov         r11d,r9d 
  movsxd      r10,eax 
  mov         eax, [rcx-2] 
  mov         r12,rdx 
  mov         [rsp+40h],eax 
  mov         eax, [rsi+rcx-2] 
  lea         rbx,[r10+rcx-2] 
  movdqa      xmm5,[rsp+40h] 
  mov         [rsp+50h],eax 
  mov         eax, [rcx+rsi*2-2] 
  lea         rbp,[r10+rdx-2] 
  movdqa      xmm2, [rsp+50h] 
  mov         [rsp+60h],eax 
  lea         r10,[rsi+rsi*2] 
  mov         rdi,rcx 
  mov         eax,[r10+rcx-2] 
  movdqa      xmm4,[rsp+60h] 
  mov         [rsp+70h],eax 
  mov         eax,[rdx-2] 
  mov         [rsp+80h],eax 
  mov         eax, [rsi+rdx-2] 
  movdqa      xmm3,[rsp+70h] 
  mov         [rsp+90h],eax 
  mov         eax,[rdx+rsi*2-2] 
  punpckldq   xmm5,[rsp+80h] 
  mov         [rsp+0A0h],eax 
  mov         eax, [r10+rdx-2] 
  punpckldq   xmm2,[rsp+90h] 
  mov         [rsp+0B0h],eax 
  mov         eax, [rbx] 
  punpckldq   xmm4,[rsp+0A0h] 
  mov         [rsp+80h],eax 
  mov         eax,[rbp] 
  punpckldq   xmm3,[rsp+0B0h] 
  mov         [rsp+90h],eax 
  mov         eax,[rsi+rbx] 
  movdqa      xmm0,[rsp+80h] 
  punpckldq   xmm0,[rsp+90h] 
  punpcklqdq  xmm5,xmm0 
  movdqa      [rsp+80h],xmm0 
  mov         [rsp+80h],eax 
  mov         eax,[rsi+rbp] 
  movdqa      xmm0,[rsp+80h] 
  movdqa      xmm1,xmm5 
  mov         [rsp+90h],eax 
  mov         eax,[rbx+rsi*2] 
  punpckldq   xmm0,[rsp+90h] 
  punpcklqdq  xmm2,xmm0 
  punpcklbw   xmm1,xmm2 
  punpckhbw   xmm5,xmm2 
  movdqa      [rsp+80h],xmm0 
  mov         [rsp+80h],eax 
  mov         eax,[rbp+rsi*2] 
  movdqa      xmm0, [rsp+80h] 
  mov         [rsp+90h],eax 
  mov         eax,[r10+rbx] 
  movdqa      xmm7,xmm1 
  punpckldq   xmm0,[rsp+90h] 
  punpcklqdq  xmm4,xmm0 
  movdqa      [rsp+80h],xmm0 
  mov         [rsp+80h],eax 
  mov         eax, [r10+rbp] 
  movdqa      xmm0,[rsp+80h] 
  mov         [rsp+90h],eax 
  punpckldq   xmm0,[rsp+90h] 
  punpcklqdq  xmm3,xmm0 
  movdqa      xmm0,xmm4 
  punpcklbw   xmm0,xmm3 
  punpckhbw   xmm4,xmm3 
  punpcklwd   xmm7,xmm0 
  punpckhwd   xmm1,xmm0 
  movdqa      xmm0,xmm5 
  movdqa      xmm6,xmm7 
  punpcklwd   xmm0,xmm4 
  punpckhwd   xmm5,xmm4 
  punpckldq   xmm6,xmm0 
  punpckhdq   xmm7,xmm0 
  movdqa      xmm0,xmm1 
  punpckldq   xmm0,xmm5 
  mov         rax, [rsp+1C8h]    ; pTC
  punpckhdq   xmm1,xmm5 
  movdqa      xmm9,xmm6 
  punpckhqdq  xmm6,xmm0 
  punpcklqdq  xmm9,xmm0 
  movdqa      xmm2,xmm7 
  movdqa      xmm13,xmm6 
  movdqa      xmm4,xmm9 
  movdqa      [rsp+10h],xmm9 
  punpcklqdq  xmm2,xmm1 
  punpckhqdq  xmm7,xmm1 
  pxor        xmm1,xmm1 
  movsx       ecx,byte [rax+3] 
  movsx       edx,byte [rax+2] 
  movsx       r8d,byte [rax+1] 
  movsx       r9d,byte [rax] 
  movdqa      xmm10,xmm1 
  movdqa      xmm15,xmm2 
  punpckhbw   xmm2,xmm1 
  punpckhbw   xmm6,xmm1 
  punpcklbw   xmm4,xmm1 
  movsx       eax,r11w 
  mov         word [rsp+0Eh],cx 
  mov         word [rsp+0Ch],cx 
  movdqa      xmm3,xmm7 
  movdqa      xmm8,xmm7 
  movdqa      [rsp+20h],xmm7 
  punpcklbw   xmm15,xmm1 
  punpcklbw   xmm13,xmm1 
  punpcklbw   xmm3,xmm1 
  mov         word [rsp+0Ah],dx 
  mov         word [rsp+8],dx 
  mov         word [rsp+6],r8w 
  movd        xmm0,eax 
  movdqa      [rsp+30h],xmm6 
  punpckhbw   xmm9,xmm1 
  punpckhbw   xmm8,xmm1 
  punpcklwd   xmm0,xmm0 
  movsx       eax,word [rsp+1C0h]   ; iBeta
  mov         word [rsp+4],r8w 
  mov         word [rsp+2],r9w 
  pshufd      xmm12,xmm0,0 
  mov         word [rsp],r9w 
  movd        xmm0,eax 
  mov         eax,4 
  cwde             
  movdqa      xmm14, [rsp] 
  movdqa      [rsp],xmm2 
  movdqa      xmm2,xmm12 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm11,xmm0,0 
  psubw       xmm10,xmm14 
  movd        xmm0,eax 
  movdqa      xmm7,xmm14 
  movdqa      xmm6,xmm14 
  pcmpgtw     xmm7,xmm1 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm5,xmm0,0 
  movdqa      xmm0,xmm4 
  movdqa      xmm1,xmm15 
  psubw       xmm4,xmm13 
  psubw       xmm0,xmm3 
  psubw       xmm1,xmm13 
  psubw       xmm3,xmm15 
  psllw       xmm1,2 
  paddw       xmm1,xmm0 
  paddw       xmm1,xmm5 
  movdqa      xmm0,xmm10 
  psraw       xmm1,3 
  pmaxsw      xmm0,xmm1 
  pminsw      xmm6,xmm0 
  movdqa      xmm1,xmm11 
  movdqa      xmm0,xmm13 
  psubw       xmm0,xmm15 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm2,xmm0 
  pabsw       xmm0,xmm4 
  pcmpgtw     xmm1,xmm0 
  pabsw       xmm0,xmm3 
  pand        xmm2,xmm1 
  movdqa      xmm1,xmm11 
  movdqa      xmm3,[rsp+30h] 
  pcmpgtw     xmm1,xmm0 
  movdqa      xmm0,xmm9 
  pand        xmm2,xmm1 
  psubw       xmm0,xmm8 
  psubw       xmm9,xmm3 
  pand        xmm2,xmm7 
  pand        xmm6,xmm2 
  psubw       xmm15,xmm6 
  paddw       xmm13,xmm6 
  movdqa      xmm2,[rsp] 
  movdqa      xmm1,xmm2 
  psubw       xmm1,xmm3 
  psubw       xmm8,xmm2 
  psllw       xmm1,2 
  paddw       xmm1,xmm0 
  paddw       xmm1,xmm5 
  movdqa      xmm0,xmm3 
  movdqa      xmm5,[rsp+10h] 
  psubw       xmm0,xmm2 
  psraw       xmm1,3 
  movdqa      xmm4,xmm5 
  pabsw       xmm0,xmm0 
  pmaxsw      xmm10,xmm1 
  movdqa      xmm1,xmm11 
  pcmpgtw     xmm12,xmm0 
  pabsw       xmm0,xmm9 
  pminsw      xmm14,xmm10 
  pcmpgtw     xmm1,xmm0 
  pabsw       xmm0,xmm8 
  pcmpgtw     xmm11,xmm0 
  pand        xmm12,xmm1 
  movdqa      xmm1,[rsp+20h] 
  pand        xmm12,xmm11 
  pand        xmm12,xmm7 
  pand        xmm14,xmm12 
  paddw       xmm3,xmm14 
  psubw       xmm2,xmm14 
  packuswb    xmm13,xmm3 
  packuswb    xmm15,xmm2 
  punpcklbw   xmm4,xmm13 
  punpckhbw   xmm5,xmm13 
  movdqa      xmm0,xmm15 
  punpcklbw   xmm0,xmm1 
  punpckhbw   xmm15,xmm1 
  movdqa      xmm3,xmm4 
  punpcklwd   xmm3,xmm0 
  punpckhwd   xmm4,xmm0 
  movdqa      xmm0,xmm5 
  movdqa      xmm2,xmm3 
  movdqa      xmm1,xmm4 
  punpcklwd   xmm0,xmm15 
  punpckhwd   xmm5,xmm15 
  punpckldq   xmm2,xmm0 
  punpckhdq   xmm3,xmm0 
  punpckldq   xmm1,xmm5 
  movdqa      xmm0,xmm2 
  punpcklqdq  xmm0,xmm1 
  punpckhdq   xmm4,xmm5 
  punpckhqdq  xmm2,xmm1 
  movdqa      [rsp+40h],xmm0 
  movdqa      xmm0,xmm3 
  movdqa      [rsp+90h],xmm2 
  mov         eax,[rsp+40h] 
  mov         [rdi-2],eax 
  mov         eax, [rsp+90h] 
  punpcklqdq  xmm0,xmm4 
  punpckhqdq  xmm3,xmm4 
  mov         [rsi+rdi-2],eax 
  movdqa      [rsp+50h],xmm0 
  mov         eax,[rsp+50h] 
  movdqa      [rsp+0A0h],xmm3 
  mov         [rdi+rsi*2-2],eax 
  mov         eax,[rsp+0A0h] 
  mov         [r10+rdi-2],eax 
  mov         eax,[rsp+48h] 
  mov         [rbx],eax 
  mov         eax,[rsp+98h] 
  mov         [rsi+rbx],eax 
  mov         eax,[rsp+58h] 
  mov         [rbx+rsi*2],eax 
  mov         eax, [rsp+0A8h] 
  mov         [r10+rbx],eax 
  mov         eax, [rsp+44h] 
  mov         [r12-2],eax 
  mov         eax,[rsp+94h] 
  mov         [rsi+r12-2],eax 
  mov         eax,[rsp+54h] 
  mov         [r12+rsi*2-2],eax 
  mov         eax, [rsp+0A4h] 
  mov         [r10+r12-2],eax 
  mov         eax,[rsp+4Ch] 
  mov         [rbp],eax 
  mov         eax,[rsp+9Ch] 
  mov         [rsi+rbp],eax 
  mov         eax, [rsp+5Ch] 
  mov         [rbp+rsi*2],eax 
  mov         eax,[rsp+0ACh] 
  mov         [r10+rbp],eax   
  lea         r11,[rsp+170h]    
  mov         rsp,r11 
  pop         r12  
  pop         rdi  
  pop         rsi  
  pop         rbp  
  pop         rbx  
  ret 



%elifdef  UNIX64


WELS_EXTERN   DeblockLumaLt4V_sse2

DeblockLumaLt4V_sse2:
  push        rbp      
  mov         r11,r8  ; pTC                                                    
  sub         rsp,1B0h                                                       
  lea         rbp,[rsp+20h]                                                  
  movd        xmm4,edx                                                                                                  
  movd        xmm2,ecx                                                       
  mov         qword [rbp+180h],r12                                       
  mov         r10,rdi                                                        
  movsxd      r12,esi                                                        
  add         rsi,rsi
  movsxd      rdx,esi 
  sub         r10,r12                                                        
  movsx       r8d,byte [r11]                                             
  pxor        xmm3,xmm3                                                      
  punpcklwd   xmm2,xmm2                                                      
  movaps      [rbp+50h],xmm14                                    
  lea         rax,[r12+r12*2]                                                
  movdqa      xmm14,[rdx+rdi]                                    
  neg         rax                                                            
  pshufd      xmm0,xmm2,0                                                    
  movd        xmm2,r8d                                                       
  movsx       rsi,byte [r11+1]                                           
  movsx       r8d,byte [r11+2]                                           
  movsx       r11d,byte [r11+3]                                          
  movaps      [rbp+70h],xmm12                                    
  movd        xmm1,esi                                                      
  movaps      [rbp+80h],xmm11                                    
  movd        xmm12,r8d                                                      
  movd        xmm11,r11d                                                     
  movdqa      xmm5, [rax+rdi]                                     
  lea         rax,[r12+r12]                                                  
  punpcklwd   xmm12,xmm12                                                    
  neg         rax                                                            
  punpcklwd   xmm11,xmm11                                                    
  movaps      [rbp],xmm8                                         
  movdqa      xmm8, [r10]                                         
  punpcklwd   xmm2,xmm2                                                      
  punpcklwd   xmm1,xmm1                                                      
  punpcklqdq  xmm12,xmm12                                                    
  punpcklqdq  xmm11,xmm11                                                    
  punpcklqdq  xmm2,xmm2                                                      
  punpcklqdq  xmm1,xmm1                                                      
  shufps      xmm12,xmm11,88h                                                
  movdqa      xmm11,xmm8                                                     
  movaps      [rbp+30h],xmm9                                     
  movdqa      xmm9,[rdi]                                         
  shufps      xmm2,xmm1,88h                                                  
  movdqa      xmm1,xmm5                                                      
  punpcklbw   xmm11,xmm3                                                     
  movaps      [rbp+20h],xmm6                                     
  movaps      [rbp+60h],xmm13                                    
  movdqa      xmm13,xmm11                                                    
  movaps      [rbp+90h],xmm10                                    
  movdqa      xmm10,xmm9                                                     
  movdqa      xmm6,[rax+rdi]                                     
  punpcklbw   xmm1,xmm3                                                      
  movaps      [rbp+0A0h],xmm12                                   
  psubw       xmm13,xmm1                                                     
  movaps      [rbp+40h],xmm15                                    
  movdqa      xmm15,xmm14                                                    
  movaps      [rbp+10h],xmm7                                     
  movdqa      xmm7,xmm6                                                      
  punpcklbw   xmm10,xmm3                                                     
  movdqa      xmm12,[r12+rdi]                                    
  punpcklbw   xmm7,xmm3                                                      
  punpcklbw   xmm12,xmm3                                                     
  punpcklbw   xmm15,xmm3                                                     
  pabsw       xmm3,xmm13                                                     
  movdqa      xmm13,xmm10                                                    
  psubw       xmm13,xmm15                                                    
  movdqa      [rbp+0F0h],xmm15                                   
  pabsw       xmm15,xmm13                                                    
  movdqa      xmm13,xmm11                                                    
  movdqa      [rbp+0B0h],xmm1                                    
  movdqa      xmm1,xmm0                                                      
  pavgw       xmm13,xmm10                                                    
  pcmpgtw     xmm1,xmm3                                                      
  movdqa      [rbp+120h],xmm13                                   
  movaps      xmm13,xmm2                                                     
  punpcklwd   xmm4,xmm4                                                      
  movdqa      xmm3,xmm0                                                      
  movdqa      [rbp+100h],xmm1                                    
  psubw       xmm13,xmm1                                                     
  movdqa      xmm1,xmm10                                                     
  pcmpgtw     xmm3,xmm15                                                     
  pshufd      xmm4,xmm4,0                                                    
  psubw       xmm1,xmm11                                                     
  movdqa      [rbp+0D0h],xmm10                                   
  psubw       xmm13,xmm3                                                     
  movdqa      [rbp+110h],xmm3                                    
  pabsw       xmm15,xmm1                                                     
  movdqa      xmm3,xmm4                                                      
  psubw       xmm10,xmm12                                                    
  pcmpgtw     xmm3,xmm15                                                     
  pabsw       xmm15,xmm10                                                    
  movdqa      xmm10,xmm0                                                     
  psllw       xmm1,2                                                         
  movdqa      [rbp+0C0h],xmm11                                   
  psubw       xmm11,xmm7                                                     
  pcmpgtw     xmm10,xmm15                                                    
  pabsw       xmm11,xmm11                                                    
  movdqa      xmm15,xmm0                                                     
  pand        xmm3,xmm10                                                     
  pcmpgtw     xmm15,xmm11                                                    
  movaps      xmm11,xmm2                                                     
  pxor        xmm10,xmm10                                                    
  pand        xmm3,xmm15                                                     
  pcmpgtw     xmm11,xmm10                                                    
  pcmpeqw     xmm10,xmm2                                                     
  por         xmm11,xmm10                                                    
  pand        xmm3,xmm11                                                     
  movdqa      xmm11,xmm7                                                     
  psubw       xmm11,xmm12                                                    
  pxor        xmm15,xmm15                                                    
  paddw       xmm11,xmm1                                                     
  psubw       xmm15,xmm13                                                    
  movdqa      [rbp+0E0h],xmm12                                   
  paddw       xmm11,[FOUR_16B_SSE2] 
  pxor        xmm12,xmm12                                                    
  psraw       xmm11,3                                                        
  punpckhbw   xmm8,xmm12                                                     
  pmaxsw      xmm15,xmm11                                                    
  punpckhbw   xmm5,xmm12                                                     
  movdqa      xmm11,xmm8                                                     
  pminsw      xmm13,xmm15                                                    
  psubw       xmm11,xmm5                                                     
  punpckhbw   xmm9,xmm12                                                     
  pand        xmm13,xmm3                                                     
  movdqa      [rbp+130h],xmm13                                   
  pabsw       xmm13,xmm11                                                    
  punpckhbw   xmm14,xmm12                                                    
  movdqa      xmm11,xmm9                                                     
  psubw       xmm11,xmm14                                                    
  movdqa      xmm15,xmm0                                                     
  movdqa      [rbp+140h],xmm14                                   
  pabsw       xmm14,xmm11                                                    
  movdqa      xmm11,xmm8                                                     
  pcmpgtw     xmm15,xmm14                                                    
  movdqa      xmm1,[r12+rdi]                                     
  pavgw       xmm11,xmm9                                                     
  movdqa      [rbp+170h],xmm11                                   
  movdqa      xmm10,xmm9                                                     
  punpckhbw   xmm6,xmm12                                                     
  psubw       xmm10,xmm8                                                     
  punpckhbw   xmm1,xmm12                                                     
  movdqa      xmm12,xmm0                                                     
  movaps      xmm11,[rbp+0A0h]                                   
  pcmpgtw     xmm12,xmm13                                                    
  movaps      xmm13,xmm11                                                    
  psubw       xmm13,xmm12                                                    
  movdqa      [rbp+160h],xmm15                                   
  psubw       xmm13,xmm15                                                    
  movdqa      xmm15,xmm9                                                     
  psubw       xmm15,xmm1                                                     
  movdqa      [rbp+150h],xmm12                                   
  pabsw       xmm12,xmm10                                                    
  pabsw       xmm14,xmm15                                                    
  movdqa      xmm15,xmm8                                                     
  pcmpgtw     xmm4,xmm12                                                     
  movdqa      xmm12,xmm0                                                     
  psubw       xmm15,xmm6                                                     
  pcmpgtw     xmm12,xmm14                                                    
  pabsw       xmm14,xmm15                                                    
  psllw       xmm10,2                                                        
  pcmpgtw     xmm0,xmm14                                                     
  movdqa      xmm14,xmm6                                                     
  psubw       xmm14,xmm1                                                     
  pand        xmm4,xmm12                                                     
  paddw       xmm14,xmm10                                                    
  pand        xmm4,xmm0                                                      
  paddw       xmm14,[FOUR_16B_SSE2] 
  pxor        xmm15,xmm15                                                    
  movaps      xmm12,xmm11                                                    
  psubw       xmm15,xmm13                                                    
  pxor        xmm0,xmm0                                                      
  psraw       xmm14,3                                                        
  pcmpgtw     xmm12,xmm0                                                     
  pcmpeqw     xmm0,xmm11                                                     
  pmaxsw      xmm15,xmm14                                                    
  por         xmm12,xmm0                                                     
  movdqa      xmm0,[rbp+120h]                                    
  pminsw      xmm13,xmm15                                                    
  movdqa      xmm15,[rbp+0B0h]                                   
  movdqa      xmm10,xmm7                                                     
  pand        xmm4,xmm12                                                     
  paddw       xmm15,xmm0                                                     
  pxor        xmm12,xmm12                                                    
  paddw       xmm10,xmm7                                                     
  movdqa      xmm14,xmm12                                                    
  psubw       xmm15,xmm10                                                    
  psubw       xmm14,xmm2                                                     
  psraw       xmm15,1                                                        
  pmaxsw      xmm15,xmm14                                                    
  movdqa      xmm10,xmm6                                                     
  pminsw      xmm15,xmm2                                                     
  paddw       xmm10,xmm6                                                     
  pand        xmm15,xmm3                                                     
  psubw       xmm12,xmm11                                                    
  pand        xmm15,[rbp+100h]                                   
  pand        xmm13,xmm4                                                     
  paddw       xmm7,xmm15                                                     
  paddw       xmm8,xmm13                                                     
  movdqa      xmm15,[rbp+170h]                                   
  psubw       xmm9,xmm13                                                     
  paddw       xmm5,xmm15                                                     
  psubw       xmm5,xmm10                                                     
  psraw       xmm5,1                                                         
  pmaxsw      xmm5,xmm12                                                     
  pminsw      xmm5,xmm11                                                     
  pand        xmm5,xmm4                                                      
  pand        xmm5,[rbp+150h]                                    
  paddw       xmm6,xmm5                                                      
  movdqa      xmm5,[rbp+0C0h]                                    
  packuswb    xmm7,xmm6                                                      
  movdqa      xmm6,[rbp+130h]                                    
  paddw       xmm5,xmm6                                                      
  packuswb    xmm5,xmm8                                                      
  movdqa      xmm8,[rbp+0D0h]                                    
  psubw       xmm8,xmm6                                                      
  movdqa      xmm6,[rbp+0F0h]                                    
  paddw       xmm6,xmm0                                                      
  movdqa      xmm0,[rbp+0E0h]                                    
  packuswb    xmm8,xmm9                                                      
  movdqa      xmm9,xmm0                                                      
  paddw       xmm9,xmm0                                                      
  psubw       xmm6,xmm9                                                      
  psraw       xmm6,1                                                         
  pmaxsw      xmm14,xmm6                                                     
  pminsw      xmm2,xmm14                                                     
  pand        xmm2,xmm3                                                      
  pand        xmm2,[rbp+110h]                                    
  paddw       xmm0,xmm2                                                      
  movdqa      xmm2,[rbp+140h]                                    
  paddw       xmm2,xmm15                                                     
  movdqa      xmm15,xmm1                                                     
  paddw       xmm15,xmm1                                                     
  psubw       xmm2,xmm15                                                     
  psraw       xmm2,1                                                         
  pmaxsw      xmm12,xmm2                                                     
  pminsw      xmm11,xmm12                                                    
  pand        xmm11,xmm4                                                     
  pand        xmm11,[rbp+160h]                                   
  paddw       xmm1,xmm11                                                     
  movdqa      [rax+rdi],xmm7                                     
  movdqa      [r10],xmm5                                         
  packuswb    xmm0,xmm1                                                      
  movdqa      [rdi],xmm8                                         
  movdqa      [r12+rdi],xmm0                                                                        
  mov         r12,qword [rbp+180h]                                       
  lea         rsp,[rbp+190h]                                                 
  pop         rbp                                                            
  ret 


WELS_EXTERN DeblockLumaEq4V_sse2

ALIGN  16
DeblockLumaEq4V_sse2:
  mov         rax,rsp 
  push        rbx  
  push        rbp   
  mov         r8,   rdx
  mov         r9,   rcx
  mov         rcx,  rdi
  mov         rdx,  rsi
  sub         rsp,1D8h 
  movaps      [rax-38h],xmm6 
  movaps      [rax-48h],xmm7 
  movaps      [rax-58h],xmm8 
  pxor        xmm1,xmm1 
  movsxd      r10,edx 
  mov         rbp,rcx 
  mov         r11d,r8d 
  mov         rdx,rcx 
  mov         rdi,rbp 
  mov         rbx,rbp 
  movdqa      xmm5,[rbp] 
  movaps      [rax-68h],xmm9 
  movaps      [rax-78h],xmm10 
  punpcklbw   xmm5,xmm1 
  movaps      [rax-88h],xmm11 
  movaps      [rax-98h],xmm12 
  movaps      [rax-0A8h],xmm13 
  movaps      [rax-0B8h],xmm14 
  movdqa      xmm14,[r10+rbp] 
  movaps      [rax-0C8h],xmm15 
  lea         eax,[r10*4] 
  movsxd      r8,eax 
  lea         eax,[r10+r10*2] 
  movsxd      rcx,eax 
  lea         eax,[r10+r10] 
  sub         rdx,r8 
  punpcklbw   xmm14,xmm1 
  movdqa      [rsp+90h],xmm5 
  movdqa      [rsp+30h],xmm14 
  movsxd      rsi,eax 
  movsx       eax,r11w 
  sub         rdi,rcx 
  sub         rbx,rsi 
  mov         r8,rbp 
  sub         r8,r10 
  movd        xmm0,eax 
  movsx       eax,r9w 
  movdqa      xmm12,[rdi] 
  movdqa      xmm6, [rsi+rbp] 
  movdqa      xmm13,[rbx] 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm11,xmm0,0 
  punpcklbw   xmm13,xmm1 
  punpcklbw   xmm6,xmm1 
  movdqa      xmm8,[r8] 
  movd        xmm0,eax 
  movdqa      xmm10,xmm11 
  mov         eax,2 
  punpcklbw   xmm8,xmm1 
  punpcklbw   xmm12,xmm1 
  cwde             
  punpcklwd   xmm0,xmm0 
  psraw       xmm10,2 
  movdqa      xmm1,xmm8 
  movdqa      [rsp+0F0h],xmm13 
  movdqa      [rsp+0B0h],xmm8 
  pshufd      xmm7,xmm0,0 
  psubw       xmm1,xmm13 
  movdqa      xmm0,xmm5 
  movdqa      xmm4,xmm7 
  movdqa      xmm2,xmm7 
  psubw       xmm0,xmm8 
  pabsw       xmm3,xmm0 
  pabsw       xmm0,xmm1 
  movdqa      xmm1,xmm5 
  movdqa      [rsp+40h],xmm7 
  movdqa      [rsp+60h],xmm6 
  pcmpgtw     xmm4,xmm0 
  psubw       xmm1,xmm14 
  pabsw       xmm0,xmm1 
  pcmpgtw     xmm2,xmm0 
  pand        xmm4,xmm2 
  movdqa      xmm0,xmm11 
  pcmpgtw     xmm0,xmm3 
  pand        xmm4,xmm0 
  movd        xmm0,eax 
  movdqa      [rsp+20h],xmm4 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm2,xmm0,0 
  paddw       xmm10,xmm2 
  movdqa      [rsp+0A0h],xmm2 
  movdqa      xmm15,xmm7 
  pxor        xmm4,xmm4 
  movdqa      xmm0,xmm8 
  psubw       xmm0,xmm12 
  mov         eax,4 
  pabsw       xmm0,xmm0 
  movdqa      xmm1,xmm10 
  cwde             
  pcmpgtw     xmm15,xmm0 
  pcmpgtw     xmm1,xmm3 
  movdqa      xmm3,xmm7 
  movdqa      xmm7,[rdx] 
  movdqa      xmm0,xmm5 
  psubw       xmm0,xmm6 
  pand        xmm15,xmm1 
  punpcklbw   xmm7,xmm4 
  movdqa      xmm9,xmm15 
  pabsw       xmm0,xmm0 
  psllw       xmm7,1 
  pandn       xmm9,xmm12 
  pcmpgtw     xmm3,xmm0 
  paddw       xmm7,xmm12 
  movd        xmm0,eax 
  pand        xmm3,xmm1 
  paddw       xmm7,xmm12 
  punpcklwd   xmm0,xmm0 
  paddw       xmm7,xmm12 
  pshufd      xmm1,xmm0,0 
  paddw       xmm7,xmm13 
  movdqa      xmm0,xmm3 
  pandn       xmm0,xmm6 
  paddw       xmm7,xmm8 
  movdqa      [rsp+70h],xmm1 
  paddw       xmm7,xmm5 
  movdqa      [rsp+120h],xmm0 
  movdqa      xmm0,[rcx+rbp] 
  punpcklbw   xmm0,xmm4 
  paddw       xmm7,xmm1 
  movdqa      xmm4,xmm15 
  psllw       xmm0,1 
  psraw       xmm7,3 
  paddw       xmm0,xmm6 
  pand        xmm7,xmm15 
  paddw       xmm0,xmm6 
  paddw       xmm0,xmm6 
  paddw       xmm0,xmm14 
  movdqa      xmm6,xmm15 
  paddw       xmm0,xmm5 
  pandn       xmm6,xmm13 
  paddw       xmm0,xmm8 
  paddw       xmm0,xmm1 
  psraw       xmm0,3 
  movdqa      xmm1,xmm12 
  paddw       xmm1,xmm13 
  pand        xmm0,xmm3 
  movdqa      [rsp+100h],xmm0 
  movdqa      xmm0,xmm8 
  paddw       xmm0,xmm5 
  paddw       xmm1,xmm0 
  movdqa      xmm0,xmm3 
  paddw       xmm1,xmm2 
  psraw       xmm1,2 
  pandn       xmm0,xmm14 
  pand        xmm4,xmm1 
  movdqa      [rsp+0E0h],xmm0 
  movdqa      xmm0,xmm5 
  paddw       xmm0,xmm8 
  movdqa      xmm1,[rsp+60h] 
  paddw       xmm1,xmm14 
  movdqa      xmm14,xmm3 
  paddw       xmm1,xmm0 
  movdqa      xmm0,xmm8 
  paddw       xmm0,[rsp+30h] 
  paddw       xmm1,xmm2 
  psraw       xmm1,2 
  pand        xmm14,xmm1 
  movdqa      xmm1,xmm13 
  paddw       xmm1,xmm13 
  paddw       xmm1,xmm0 
  paddw       xmm1,xmm2 
  psraw       xmm1,2 
  movdqa      xmm0,[rsp+30h] 
  movdqa      xmm2,xmm13 
  movdqa      xmm5,xmm15 
  paddw       xmm0,[rsp+70h] 
  pandn       xmm5,xmm1 
  paddw       xmm2,xmm8 
  movdqa      xmm8,[rsp+90h] 
  movdqa      xmm1,xmm12 
  paddw       xmm2,xmm8 
  psllw       xmm2,1 
  paddw       xmm2,xmm0 
  paddw       xmm1,xmm2 
  movdqa      xmm0,xmm8 
  movdqa      xmm8,xmm3 
  movdqa      xmm2,[rsp+30h] 
  paddw       xmm0,xmm13 
  psraw       xmm1,3 
  pand        xmm15,xmm1 
  movdqa      xmm1,xmm2 
  paddw       xmm1,xmm2 
  paddw       xmm2,[rsp+90h] 
  paddw       xmm2,[rsp+0B0h] 
  paddw       xmm1,xmm0 
  movdqa      xmm0,xmm13 
  movdqa      xmm13,[r8] 
  paddw       xmm0, [rsp+70h] 
  paddw       xmm1, [rsp+0A0h] 
  psllw       xmm2,1 
  paddw       xmm2,xmm0 
  psraw       xmm1,2 
  movdqa      xmm0, [rdi] 
  pandn       xmm8,xmm1 
  movdqa      xmm1, [rsp+60h] 
  paddw       xmm1,xmm2 
  movdqa      xmm2, [rbx] 
  psraw       xmm1,3 
  pand        xmm3,xmm1 
  movdqa      xmm1, [rbp] 
  movdqa      [rsp+0D0h],xmm3 
  pxor        xmm3,xmm3 
  punpckhbw   xmm0,xmm3 
  punpckhbw   xmm1,xmm3 
  punpckhbw   xmm13,xmm3 
  movdqa      [rsp+0C0h],xmm0 
  movdqa      xmm0,[r10+rbp] 
  movdqa      [rsp],xmm1 
  punpckhbw   xmm0,xmm3 
  punpckhbw   xmm2,xmm3 
  movdqa      [rsp+80h],xmm0 
  movdqa      xmm0,[rsi+rbp] 
  movdqa      [rsp+10h],xmm13 
  punpckhbw   xmm0,xmm3 
  movdqa      [rsp+50h],xmm0 
  movdqa      xmm0,xmm1 
  movdqa      xmm1,xmm13 
  psubw       xmm0,xmm13 
  psubw       xmm1,xmm2 
  pabsw       xmm3,xmm0 
  pabsw       xmm0,xmm1 
  movdqa      xmm1,[rsp] 
  movdqa      xmm13,[rsp+40h] 
  movdqa      [rsp+110h],xmm2 
  psubw       xmm1, [rsp+80h] 
  pcmpgtw     xmm13,xmm0 
  pcmpgtw     xmm11,xmm3 
  pabsw       xmm0,xmm1 
  pcmpgtw     xmm10,xmm3 
  movdqa      xmm1, [rsp+40h] 
  movdqa      xmm2,xmm1 
  movdqa      xmm3,xmm1 
  pcmpgtw     xmm2,xmm0 
  movdqa      xmm0, [rsp+10h] 
  pand        xmm13,xmm2 
  pand        xmm13,xmm11 
  movdqa      xmm11,[rsp+0C0h] 
  psubw       xmm0,xmm11 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm3,xmm0 
  pand        xmm3,xmm10 
  movdqa      xmm0,[rsp] 
  psubw       xmm0,[rsp+50h] 
  movdqa      xmm2,[rdx] 
  pabsw       xmm0,xmm0 
  por         xmm7,xmm9 
  movdqa      xmm9,[rsp+20h] 
  pcmpgtw     xmm1,xmm0 
  pand        xmm9,xmm7 
  movdqa      xmm7,[rsp+20h] 
  movdqa      xmm0,xmm7 
  pandn       xmm0,xmm12 
  movdqa      xmm12,[rsp+110h] 
  pand        xmm1,xmm10 
  movdqa      xmm10,[rsp+70h] 
  movdqa      [rsp+40h],xmm1 
  movdqa      xmm1,xmm13 
  por         xmm9,xmm0 
  pxor        xmm0,xmm0 
  por         xmm4,xmm6 
  movdqa      xmm6,xmm7 
  punpckhbw   xmm2,xmm0 
  por         xmm15,xmm5 
  movdqa      xmm5,[rsp+20h] 
  movdqa      xmm0,xmm3 
  psllw       xmm2,1 
  pandn       xmm0,xmm11 
  pand        xmm6,xmm4 
  movdqa      xmm4,[rsp] 
  paddw       xmm2,xmm11 
  pand        xmm5,xmm15 
  movdqa      xmm15,[rsp+20h] 
  paddw       xmm2,xmm11 
  paddw       xmm2,xmm11 
  paddw       xmm2,xmm12 
  paddw       xmm2,[rsp+10h] 
  paddw       xmm2,[rsp] 
  paddw       xmm2,xmm10 
  psraw       xmm2,3 
  pand        xmm2,xmm3 
  por         xmm2,xmm0 
  pand        xmm1,xmm2 
  movdqa      xmm0,xmm13 
  movdqa      xmm2,xmm11 
  pandn       xmm0,xmm11 
  paddw       xmm2,xmm12 
  por         xmm1,xmm0 
  packuswb    xmm9,xmm1 
  movdqa      xmm0,xmm7 
  movdqa      xmm7,[rsp+0A0h] 
  pandn       xmm0,[rsp+0F0h] 
  movdqa      xmm1,xmm3 
  por         xmm6,xmm0 
  movdqa      xmm0,[rsp+10h] 
  paddw       xmm0,xmm4 
  paddw       xmm2,xmm0 
  paddw       xmm2,xmm7 
  movdqa      xmm0,xmm3 
  pandn       xmm0,xmm12 
  psraw       xmm2,2 
  pand        xmm1,xmm2 
  por         xmm1,xmm0 
  movdqa      xmm2,xmm13 
  movdqa      xmm0,xmm13 
  pand        xmm2,xmm1 
  pandn       xmm0,xmm12 
  movdqa      xmm1,xmm12 
  paddw       xmm1,[rsp+10h] 
  por         xmm2,xmm0 
  movdqa      xmm0,xmm15 
  pandn       xmm0,[rsp+0B0h] 
  paddw       xmm1,xmm4 
  packuswb    xmm6,xmm2 
  movdqa      xmm2,xmm3 
  psllw       xmm1,1 
  por         xmm5,xmm0 
  movdqa      xmm0,[rsp+80h] 
  paddw       xmm0,xmm10 
  paddw       xmm1,xmm0 
  paddw       xmm11,xmm1 
  psraw       xmm11,3 
  movdqa      xmm1,xmm12 
  pand        xmm2,xmm11 
  paddw       xmm1,xmm12 
  movdqa      xmm11,[rsp+80h] 
  movdqa      xmm0, [rsp+10h] 
  por         xmm14,[rsp+0E0h] 
  paddw       xmm0,xmm11 
  movdqa      xmm4,xmm15 
  paddw       xmm1,xmm0 
  movdqa      xmm0,xmm13 
  paddw       xmm1,xmm7 
  psraw       xmm1,2 
  pandn       xmm3,xmm1 
  por         xmm2,xmm3 
  movdqa      xmm1,xmm13 
  movdqa      xmm3,[rsp+10h] 
  pandn       xmm0,xmm3 
  pand        xmm1,xmm2 
  movdqa      xmm2,xmm11 
  paddw       xmm2,[rsp] 
  por         xmm1,xmm0 
  movdqa      xmm0,[rsp+0D0h] 
  por         xmm0,xmm8 
  paddw       xmm2,xmm3 
  packuswb    xmm5,xmm1 
  movdqa      xmm8,[rsp+40h] 
  movdqa      xmm1,[rsp+50h] 
  movdqa      xmm3,xmm8 
  pand        xmm4,xmm0 
  psllw       xmm2,1 
  movdqa      xmm0,xmm15 
  pandn       xmm0,[rsp+90h] 
  por         xmm4,xmm0 
  movdqa      xmm0,xmm12 
  paddw       xmm0,xmm10 
  paddw       xmm2,xmm0 
  paddw       xmm1,xmm2 
  movdqa      xmm0,[rsp] 
  movdqa      xmm2,xmm11 
  paddw       xmm0,xmm12 
  movdqa      xmm12,[rsp] 
  paddw       xmm2,xmm11 
  paddw       xmm2,xmm0 
  psraw       xmm1,3 
  movdqa      xmm0,xmm8 
  pand        xmm3,xmm1 
  paddw       xmm2,xmm7 
  movdqa      xmm1,xmm13 
  psraw       xmm2,2 
  pandn       xmm0,xmm2 
  por         xmm3,xmm0 
  movdqa      xmm2,[rsp+50h] 
  movdqa      xmm0,xmm13 
  pandn       xmm0,xmm12 
  pand        xmm1,xmm3 
  paddw       xmm2,xmm11 
  movdqa      xmm3,xmm15 
  por         xmm1,xmm0 
  pand        xmm3,xmm14 
  movdqa      xmm14,[rsp+10h] 
  movdqa      xmm0,xmm15 
  pandn       xmm0,[rsp+30h] 
  packuswb    xmm4,xmm1 
  movdqa      xmm1,xmm8 
  por         xmm3,xmm0 
  movdqa      xmm0,xmm12 
  paddw       xmm0,xmm14 
  paddw       xmm2,xmm0 
  paddw       xmm2,xmm7 
  movdqa      xmm0,xmm8 
  pandn       xmm0,xmm11 
  psraw       xmm2,2 
  pand        xmm1,xmm2 
  por         xmm1,xmm0 
  movdqa      xmm2,xmm13 
  movdqa      xmm0,xmm13 
  pandn       xmm0,xmm11 
  pand        xmm2,xmm1 
  movdqa      xmm1,xmm15 
  por         xmm2,xmm0 
  packuswb    xmm3,xmm2 
  movdqa      xmm0,[rsp+100h] 
  por         xmm0,[rsp+120h] 
  pand        xmm1,xmm0 
  movdqa      xmm2,[rcx+rbp] 
  movdqa      xmm7,[rsp+50h] 
  pandn       xmm15,[rsp+60h] 
  lea         r11,[rsp+1D8h] 
  pxor        xmm0,xmm0 
  por         xmm1,xmm15 
  movaps      xmm15,[r11-0A8h] 
  movdqa      [rdi],xmm9 
  movaps      xmm9,[r11-48h] 
  punpckhbw   xmm2,xmm0 
  psllw       xmm2,1 
  paddw       xmm2,xmm7 
  paddw       xmm2,xmm7 
  movdqa      [rbx],xmm6 
  movaps      xmm6,[r11-18h] 
  paddw       xmm2,xmm7 
  paddw       xmm2,xmm11 
  movaps      xmm11,[r11-68h] 
  paddw       xmm2,xmm12 
  movaps      xmm12,[r11-78h] 
  paddw       xmm2,xmm14 
  paddw       xmm2,xmm10 
  psraw       xmm2,3 
  movaps      xmm10,[r11-58h] 
  movaps      xmm14,[r11-98h] 
  movdqa      xmm0,xmm13 
  pand        xmm2,xmm8 
  pandn       xmm8,xmm7 
  pandn       xmm13,xmm7 
  por         xmm2,xmm8 
  movaps      xmm7,[r11-28h] 
  movaps      xmm8,[r11-38h] 
  movdqa      [r8],xmm5 
  pand        xmm0,xmm2 
  por         xmm0,xmm13 
  packuswb    xmm1,xmm0 
  movaps      xmm13,[r11-88h] 
  movdqa      [rbp],xmm4 
  movdqa      [r10+rbp],xmm3 
  movdqa      [rsi+rbp],xmm1 
  mov         rsp,r11   
  pop         rbp  
  pop         rbx  
  ret

WELS_EXTERN  DeblockChromaLt4V_sse2
ALIGN  16 
DeblockChromaLt4V_sse2: 
  mov         rax,rsp 
  push        rbx  
  push        rbp    
  mov         r10,  rdx
  mov         r11,  rcx
  mov         rcx,  rdi
  mov         rdx,  rsi  
  mov         rsi,  r10
  mov         r10,  r9
  mov         rbp,  r8
  mov         r8,   rsi
  mov         r9,   r11
  sub         rsp,0C8h   
  pxor        xmm1,xmm1 
  mov         rbx,rcx 
  movsxd      r11,r8d 
  movsx       ecx,byte [r10] 
  movsx       r8d,byte [r10+2] 
  mov         rdi,rdx 
  movq        xmm2,[rbx] 
  movq        xmm9,[r11+rbx] 
  movsx       edx,byte [r10+1] 
  mov         word [rsp+2],cx 
  mov         word [rsp],cx 
  movsx       eax,byte [r10+3] 
  mov         word [rsp+6],dx 
  mov         word [rsp+4],dx 
  movdqa      xmm11,xmm1 
  mov         word [rsp+0Eh],ax 
  mov         word [rsp+0Ch],ax 
  lea         eax,[r11+r11] 
  movsxd      rcx,eax 
  mov         rax,rbx 
  mov         rdx,rdi 
  sub         rax,rcx 
  mov         word [rsp+0Ah],r8w 
  mov         word [rsp+8],r8w 
  movdqa      xmm6,[rsp] 
  movdqa      xmm7,xmm6 
  movq        xmm13, [rax] 
  mov         rax,rdi 
  sub         rax,rcx 
  mov         rcx,rbx 
  pcmpgtw     xmm7,xmm1 
  psubw       xmm11,xmm6 
  sub         rcx,r11 
  sub         rdx,r11 
  movq        xmm0,[rax] 
  movsx       eax,r9w 
  movq        xmm15,[rcx] 
  punpcklqdq  xmm13,xmm0 
  movq        xmm0, [rdx] 
  movdqa      xmm4,xmm13 
  punpcklqdq  xmm15,xmm0 
  movq        xmm0, [rdi] 
  punpcklbw   xmm4,xmm1 
  movdqa      xmm12,xmm15 
  punpcklqdq  xmm2,xmm0 
  movq        xmm0, [r11+rdi] 
  punpcklbw   xmm12,xmm1 
  movdqa      xmm14,xmm2 
  punpcklqdq  xmm9,xmm0 
  punpckhbw   xmm2,xmm1 
  punpcklbw   xmm14,xmm1 
  movd        xmm0,eax 
  mov         eax, ebp ; iBeta
  punpckhbw   xmm13,xmm1 
  punpckhbw   xmm15,xmm1 
  movdqa      xmm3,xmm9 
  movdqa      [rsp+10h],xmm2 
  punpcklwd   xmm0,xmm0 
  punpckhbw   xmm9,xmm1 
  punpcklbw   xmm3,xmm1 
  movdqa      xmm1,xmm14 
  pshufd      xmm10,xmm0,0 
  movd        xmm0,eax 
  mov         eax,4 
  cwde             
  punpcklwd   xmm0,xmm0 
  pshufd      xmm8,xmm0,0 
  movd        xmm0,eax 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm5,xmm0,0 
  psubw       xmm1,xmm12 
  movdqa      xmm2,xmm10 
  lea         r11,[rsp+0C8h] 
  psllw       xmm1,2 
  movdqa      xmm0,xmm4 
  psubw       xmm4,xmm12 
  psubw       xmm0,xmm3 
  psubw       xmm3,xmm14 
  paddw       xmm1,xmm0 
  paddw       xmm1,xmm5 
  movdqa      xmm0,xmm11 
  psraw       xmm1,3 
  pmaxsw      xmm0,xmm1 
  pminsw      xmm6,xmm0 
  movdqa      xmm1,xmm8 
  movdqa      xmm0,xmm12 
  psubw       xmm0,xmm14 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm2,xmm0 
  pabsw       xmm0,xmm4 
  pcmpgtw     xmm1,xmm0 
  pabsw       xmm0,xmm3 
  movdqa      xmm3,[rsp] 
  pand        xmm2,xmm1 
  movdqa      xmm1,xmm8 
  pcmpgtw     xmm1,xmm0 
  movdqa      xmm0,xmm13 
  pand        xmm2,xmm1 
  psubw       xmm0,xmm9 
  psubw       xmm13,xmm15 
  pand        xmm2,xmm7 
  pand        xmm6,xmm2 
  paddw       xmm12,xmm6 
  psubw       xmm14,xmm6 
  movdqa      xmm2,[rsp+10h] 
  movaps      xmm6,[r11-18h] 
  movdqa      xmm1,xmm2 
  psubw       xmm1,xmm15 
  psubw       xmm9,xmm2 
  psllw       xmm1,2 
  paddw       xmm1,xmm0 
  paddw       xmm1,xmm5 
  movdqa      xmm0,xmm15 
  psubw       xmm0,xmm2 
  psraw       xmm1,3 
  pmaxsw      xmm11,xmm1 
  pabsw       xmm0,xmm0 
  movdqa      xmm1,xmm8 
  pcmpgtw     xmm10,xmm0 
  pabsw       xmm0,xmm13 
  pminsw      xmm3,xmm11 
  movaps      xmm11,[r11-68h] 
  movaps      xmm13,[rsp+40h] 
  pcmpgtw     xmm1,xmm0 
  pabsw       xmm0,xmm9 
  movaps      xmm9, [r11-48h] 
  pand        xmm10,xmm1 
  pcmpgtw     xmm8,xmm0 
  pand        xmm10,xmm8 
  pand        xmm10,xmm7 
  movaps      xmm8,[r11-38h] 
  movaps      xmm7,[r11-28h] 
  pand        xmm3,xmm10 
  paddw       xmm15,xmm3 
  psubw       xmm2,xmm3 
  movaps      xmm10,[r11-58h] 
  packuswb    xmm12,xmm15 
  movaps      xmm15,[rsp+20h] 
  packuswb    xmm14,xmm2 
  movq        [rcx],xmm12 
  movq        [rbx],xmm14 
  psrldq      xmm12,8 
  psrldq      xmm14,8 
  movq        [rdx],xmm12 
  movaps      xmm12,[r11-78h] 
  movq        [rdi],xmm14 
  movaps      xmm14,[rsp+30h] 
  mov         rsp,r11 
  pop         rbp  
  pop         rbx  
  ret

WELS_EXTERN   DeblockChromaEq4V_sse2
ALIGN 16
DeblockChromaEq4V_sse2:
  mov         rax,rsp 
  push        rbx  
  push        rbp

  mov         rbp, r8
  mov         r8, rdx
  mov         r9, rcx
  mov         rcx, rdi
  mov         rdx, rsi
  
  sub         rsp,90h 
  pxor        xmm1,xmm1 
  mov         r11,rcx 
  mov         rbx,rdx 
  mov         r10d,r9d   
  movq        xmm13,[r11] 
  lea         eax,[r8+r8] 
  movsxd      r9,eax 
  mov         rax,rcx 
  sub         rax,r9 
  movq        xmm14,[rax] 
  mov         rax,rdx 
  sub         rax,r9 
  movq        xmm0,[rax] 
  movsxd      rax,r8d 
  sub         rcx,rax 
  sub         rdx,rax 
  movq        xmm12,[rax+r11] 
  movq        xmm10,[rcx] 
  punpcklqdq  xmm14,xmm0 
  movdqa      xmm8,xmm14 
  movq        xmm0,[rdx] 
  punpcklbw   xmm8,xmm1 
  punpckhbw   xmm14,xmm1 
  punpcklqdq  xmm10,xmm0 
  movq        xmm0,[rbx] 
  movdqa      xmm5,xmm10 
  punpcklqdq  xmm13,xmm0 
  movq        xmm0, [rax+rbx] 
  punpcklbw   xmm5,xmm1 
  movsx       eax,r10w 
  movdqa      xmm9,xmm13 
  punpcklqdq  xmm12,xmm0 
  punpcklbw   xmm9,xmm1 
  punpckhbw   xmm10,xmm1 
  movd        xmm0,eax 
  mov         eax, ebp   ; iBeta
  punpckhbw   xmm13,xmm1 
  movdqa      xmm7,xmm12 
  punpcklwd   xmm0,xmm0 
  punpckhbw   xmm12,xmm1 
  pshufd      xmm11,xmm0,0 
  punpcklbw   xmm7,xmm1 
  movd        xmm0,eax 
  movdqa      xmm1,xmm8 
  psubw       xmm1,xmm5 
  punpcklwd   xmm0,xmm0 
  movdqa      xmm6,xmm11 
  pshufd      xmm3,xmm0,0 
  movdqa      xmm0,xmm5 
  psubw       xmm0,xmm9 
  movdqa      xmm2,xmm3 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm6,xmm0 
  pabsw       xmm0,xmm1 
  movdqa      xmm1,xmm3 
  pcmpgtw     xmm2,xmm0 
  pand        xmm6,xmm2 
  movdqa      xmm0,xmm7 
  movdqa      xmm2,xmm3 
  psubw       xmm0,xmm9 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm1,xmm0 
  pand        xmm6,xmm1 
  movdqa      xmm0,xmm10 
  movdqa      xmm1,xmm14 
  psubw       xmm0,xmm13 
  psubw       xmm1,xmm10 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm11,xmm0 
  pabsw       xmm0,xmm1 
  pcmpgtw     xmm2,xmm0 
  pand        xmm11,xmm2 
  movdqa      xmm0,xmm12 
  movdqa      xmm4,xmm6 
  movdqa      xmm1,xmm8 
  mov         eax,2 
  cwde             
  paddw       xmm1,xmm8 
  psubw       xmm0,xmm13 
  paddw       xmm1,xmm5 
  pabsw       xmm0,xmm0 
  movdqa      xmm2,xmm14 
  paddw       xmm1,xmm7 
  pcmpgtw     xmm3,xmm0 
  paddw       xmm2,xmm14 
  movd        xmm0,eax 
  pand        xmm11,xmm3 
  paddw       xmm7,xmm7 
  paddw       xmm2,xmm10 
  punpcklwd   xmm0,xmm0 
  paddw       xmm2,xmm12 
  paddw       xmm12,xmm12 
  pshufd      xmm3,xmm0,0 
  paddw       xmm7,xmm9 
  paddw       xmm12,xmm13 
  movdqa      xmm0,xmm6 
  paddw       xmm1,xmm3 
  pandn       xmm0,xmm5 
  paddw       xmm7,xmm8 
  psraw       xmm1,2 
  paddw       xmm12,xmm14 
  paddw       xmm7,xmm3 
  ;movaps      xmm14,[rsp] 
  pand        xmm4,xmm1 
  paddw       xmm12,xmm3 
  psraw       xmm7,2 
  movdqa      xmm1,xmm11 
  por         xmm4,xmm0 
  psraw       xmm12,2 
  paddw       xmm2,xmm3 
  movdqa      xmm0,xmm11 
  pandn       xmm0,xmm10 
  psraw       xmm2,2 
  pand        xmm1,xmm2 
  por         xmm1,xmm0 
  packuswb    xmm4,xmm1 
  movdqa      xmm0,xmm11 
  movdqa      xmm1,xmm6 
  pand        xmm1,xmm7 
  movq        [rcx],xmm4 
  pandn       xmm6,xmm9 
  pandn       xmm11,xmm13 
  pand        xmm0,xmm12 
  por         xmm1,xmm6 
  por         xmm0,xmm11 
  psrldq      xmm4,8 
  packuswb    xmm1,xmm0 
  movq        [r11],xmm1 
  psrldq      xmm1,8 
  movq        [rdx],xmm4 
  lea         r11,[rsp+90h] 
  movq        [rbx],xmm1 
  mov         rsp,r11 
  pop         rbp
  pop         rbx  
  ret


WELS_EXTERN   DeblockChromaEq4H_sse2
ALIGN  16
DeblockChromaEq4H_sse2:
  mov         rax,rsp 
  push        rbx 
  push        rbp 
  push        r12
  
  mov         rbp,   r8  
  mov         r8,    rdx
  mov         r9,    rcx
  mov         rcx,   rdi
  mov         rdx,   rsi  
  mov         rdi,   rdx

  sub         rsp,140h     
  lea         eax,[r8*4] 
  movsxd      r10,eax 
  mov         eax,[rcx-2] 
  mov         [rsp+10h],eax 
  lea         rbx,[r10+rdx-2] 
  lea         r11,[r10+rcx-2] 

  movdqa      xmm5,[rsp+10h] 
  movsxd      r10,r8d 
  mov         eax,[r10+rcx-2] 
  lea         rdx,[r10+r10*2] 
  mov         [rsp+20h],eax 
  mov         eax,[rcx+r10*2-2] 
  mov         [rsp+30h],eax 
  mov         eax,[rdx+rcx-2]
  movdqa      xmm2,[rsp+20h] 
  mov         [rsp+40h],eax 
  mov         eax, [rdi-2] 
  movdqa      xmm4,[rsp+30h] 
  mov         [rsp+50h],eax 
  mov         eax,[r10+rdi-2] 
  movdqa      xmm3,[rsp+40h] 
  mov         [rsp+60h],eax 
  mov         eax,[rdi+r10*2-2] 
  punpckldq   xmm5,[rsp+50h] 
  mov         [rsp+70h],eax 
  mov         eax, [rdx+rdi-2] 
  punpckldq   xmm2, [rsp+60h] 
  mov          [rsp+80h],eax 
  mov         eax,[r11] 
  punpckldq   xmm4, [rsp+70h] 
  mov         [rsp+50h],eax 
  mov         eax,[rbx] 
  punpckldq   xmm3,[rsp+80h] 
  mov         [rsp+60h],eax 
  mov         eax,[r10+r11] 
  movdqa      xmm0, [rsp+50h] 
  punpckldq   xmm0, [rsp+60h] 
  punpcklqdq  xmm5,xmm0 
  movdqa      [rsp+50h],xmm0 
  mov         [rsp+50h],eax 
  mov         eax,[r10+rbx] 
  movdqa      xmm0,[rsp+50h] 
  movdqa      xmm1,xmm5 
  mov         [rsp+60h],eax 
  mov         eax,[r11+r10*2] 
  punpckldq   xmm0, [rsp+60h] 
  punpcklqdq  xmm2,xmm0 
  punpcklbw   xmm1,xmm2 
  punpckhbw   xmm5,xmm2 
  movdqa      [rsp+50h],xmm0 
  mov         [rsp+50h],eax 
  mov         eax,[rbx+r10*2] 
  movdqa      xmm0,[rsp+50h] 
  mov         [rsp+60h],eax 
  mov         eax, [rdx+r11] 
  movdqa      xmm15,xmm1 
  punpckldq   xmm0,[rsp+60h] 
  punpcklqdq  xmm4,xmm0 
  movdqa      [rsp+50h],xmm0 
  mov         [rsp+50h],eax 
  mov         eax, [rdx+rbx] 
  movdqa      xmm0,[rsp+50h] 
  mov         [rsp+60h],eax 
  punpckldq   xmm0, [rsp+60h] 
  punpcklqdq  xmm3,xmm0 
  movdqa      xmm0,xmm4 
  punpcklbw   xmm0,xmm3 
  punpckhbw   xmm4,xmm3 
  punpcklwd   xmm15,xmm0 
  punpckhwd   xmm1,xmm0 
  movdqa      xmm0,xmm5 
  movdqa      xmm12,xmm15 
  punpcklwd   xmm0,xmm4 
  punpckhwd   xmm5,xmm4 
  punpckldq   xmm12,xmm0 
  punpckhdq   xmm15,xmm0 
  movdqa      xmm0,xmm1 
  movdqa      xmm11,xmm12 
  punpckldq   xmm0,xmm5 
  punpckhdq   xmm1,xmm5 
  punpcklqdq  xmm11,xmm0 
  punpckhqdq  xmm12,xmm0 
  movsx       eax,r9w 
  movdqa      xmm14,xmm15 
  punpcklqdq  xmm14,xmm1 
  punpckhqdq  xmm15,xmm1 
  pxor        xmm1,xmm1 
  movd        xmm0,eax 
  movdqa      xmm4,xmm12 
  movdqa      xmm8,xmm11 
  mov         eax, ebp ; iBeta
  punpcklwd   xmm0,xmm0 
  punpcklbw   xmm4,xmm1 
  punpckhbw   xmm12,xmm1 
  movdqa      xmm9,xmm14 
  movdqa      xmm7,xmm15 
  movdqa      xmm10,xmm15 
  pshufd      xmm13,xmm0,0 
  punpcklbw   xmm9,xmm1 
  punpckhbw   xmm14,xmm1 
  movdqa      xmm6,xmm13 
  movd        xmm0,eax 
  movdqa      [rsp],xmm11 
  mov         eax,2 
  cwde             
  punpckhbw   xmm11,xmm1 
  punpckhbw   xmm10,xmm1 
  punpcklbw   xmm7,xmm1 
  punpcklwd   xmm0,xmm0 
  punpcklbw   xmm8,xmm1 
  pshufd      xmm3,xmm0,0 
  movdqa      xmm1,xmm8 
  movdqa      xmm0,xmm4 
  psubw       xmm0,xmm9 
  psubw       xmm1,xmm4 
  movdqa      xmm2,xmm3 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm6,xmm0 
  pabsw       xmm0,xmm1 
  movdqa      xmm1,xmm3 
  pcmpgtw     xmm2,xmm0 
  pand        xmm6,xmm2 
  movdqa      xmm0,xmm7 
  movdqa      xmm2,xmm3 
  psubw       xmm0,xmm9 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm1,xmm0 
  pand        xmm6,xmm1 
  movdqa      xmm0,xmm12 
  movdqa      xmm1,xmm11 
  psubw       xmm0,xmm14 
  psubw       xmm1,xmm12 
  movdqa      xmm5,xmm6 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm13,xmm0 
  pabsw       xmm0,xmm1 
  movdqa      xmm1,xmm8 
  pcmpgtw     xmm2,xmm0 
  paddw       xmm1,xmm8 
  movdqa      xmm0,xmm10 
  pand        xmm13,xmm2 
  psubw       xmm0,xmm14 
  paddw       xmm1,xmm4 
  movdqa      xmm2,xmm11 
  pabsw       xmm0,xmm0 
  paddw       xmm2,xmm11 
  paddw       xmm1,xmm7 
  pcmpgtw     xmm3,xmm0 
  paddw       xmm2,xmm12 
  movd        xmm0,eax 
  pand        xmm13,xmm3 
  paddw       xmm2,xmm10 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm3,xmm0,0 
  movdqa      xmm0,xmm6 
  paddw       xmm1,xmm3 
  pandn       xmm0,xmm4 
  paddw       xmm2,xmm3 
  psraw       xmm1,2 
  pand        xmm5,xmm1 
  por         xmm5,xmm0 
  paddw       xmm7,xmm7 
  paddw       xmm10,xmm10 
  psraw       xmm2,2 
  movdqa      xmm1,xmm13 
  movdqa      xmm0,xmm13 
  pandn       xmm0,xmm12 
  pand        xmm1,xmm2 
  paddw       xmm7,xmm9 
  por         xmm1,xmm0 
  paddw       xmm10,xmm14 
  paddw       xmm7,xmm8 
  movdqa      xmm0,xmm13 
  packuswb    xmm5,xmm1 
  paddw       xmm7,xmm3 
  paddw       xmm10,xmm11 
  movdqa      xmm1,xmm6 
  paddw       xmm10,xmm3 
  pandn       xmm6,xmm9 
  psraw       xmm7,2 
  pand        xmm1,xmm7 
  psraw       xmm10,2 
  pandn       xmm13,xmm14 
  pand        xmm0,xmm10 
  por         xmm1,xmm6 
  movdqa      xmm6,[rsp] 
  movdqa      xmm4,xmm6 
  por         xmm0,xmm13 
  punpcklbw   xmm4,xmm5 
  punpckhbw   xmm6,xmm5 
  movdqa      xmm3,xmm4 
  packuswb    xmm1,xmm0 
  movdqa      xmm0,xmm1 
  punpckhbw   xmm1,xmm15 
  punpcklbw   xmm0,xmm15 
  punpcklwd   xmm3,xmm0 
  punpckhwd   xmm4,xmm0 
  movdqa      xmm0,xmm6 
  movdqa      xmm2,xmm3 
  punpcklwd   xmm0,xmm1 
  punpckhwd   xmm6,xmm1 
  movdqa      xmm1,xmm4 
  punpckldq   xmm2,xmm0 
  punpckhdq   xmm3,xmm0 
  punpckldq   xmm1,xmm6 
  movdqa      xmm0,xmm2 
  punpcklqdq  xmm0,xmm1 
  punpckhdq   xmm4,xmm6 
  punpckhqdq  xmm2,xmm1 
  movdqa      [rsp+10h],xmm0 
  movdqa      [rsp+60h],xmm2 
  movdqa      xmm0,xmm3 
  mov         eax,[rsp+10h] 
  mov         [rcx-2],eax 
  mov         eax,[rsp+60h] 
  punpcklqdq  xmm0,xmm4 
  punpckhqdq  xmm3,xmm4 
  mov         [r10+rcx-2],eax 
  movdqa      [rsp+20h],xmm0 
  mov         eax, [rsp+20h] 
  movdqa      [rsp+70h],xmm3 
  mov         [rcx+r10*2-2],eax 
  mov         eax,[rsp+70h] 
  mov         [rdx+rcx-2],eax 
  mov         eax,[rsp+18h] 
  mov         [r11],eax 
  mov         eax,[rsp+68h] 
  mov         [r10+r11],eax 
  mov         eax,[rsp+28h] 
  mov         [r11+r10*2],eax 
  mov         eax,[rsp+78h] 
  mov         [rdx+r11],eax 
  mov         eax,[rsp+14h] 
  mov         [rdi-2],eax 
  mov         eax,[rsp+64h] 
  mov         [r10+rdi-2],eax 
  mov         eax,[rsp+24h] 
  mov         [rdi+r10*2-2],eax 
  mov         eax, [rsp+74h] 
  mov         [rdx+rdi-2],eax 
  mov         eax, [rsp+1Ch] 
  mov         [rbx],eax 
  mov         eax, [rsp+6Ch] 
  mov         [r10+rbx],eax 
  mov         eax,[rsp+2Ch] 
  mov         [rbx+r10*2],eax 
  mov         eax,[rsp+7Ch] 
  mov         [rdx+rbx],eax  
  lea         r11,[rsp+140h] 
  mov         rbx, [r11+28h]    
  mov         rsp,r11
  pop         r12
  pop         rbp
  pop         rbx
  ret


WELS_EXTERN DeblockChromaLt4H_sse2
ALIGN  16
DeblockChromaLt4H_sse2:
  mov         rax,rsp 
  push        rbx  
  push        rbp  
  push        r12  
  push        r13
  push        r14
  sub         rsp,170h  
  
  mov         r13,   r8
  mov         r14,   r9
  mov         r8,    rdx
  mov         r9,    rcx
  mov         rdx,   rdi
  mov         rcx,   rsi

  movsxd      rsi,r8d 
  lea         eax,[r8*4] 
  mov         r11d,r9d 
  movsxd      r10,eax 
  mov         eax, [rcx-2] 
  mov         r12,rdx 
  mov         [rsp+40h],eax 
  mov         eax, [rsi+rcx-2] 
  lea         rbx,[r10+rcx-2] 
  movdqa      xmm5,[rsp+40h] 
  mov         [rsp+50h],eax 
  mov         eax, [rcx+rsi*2-2] 
  lea         rbp,[r10+rdx-2] 
  movdqa      xmm2, [rsp+50h] 
  mov         [rsp+60h],eax 
  lea         r10,[rsi+rsi*2] 
  mov         rdi,rcx 
  mov         eax,[r10+rcx-2] 
  movdqa      xmm4,[rsp+60h] 
  mov         [rsp+70h],eax 
  mov         eax,[rdx-2] 
  mov         [rsp+80h],eax 
  mov         eax, [rsi+rdx-2] 
  movdqa      xmm3,[rsp+70h] 
  mov         [rsp+90h],eax 
  mov         eax,[rdx+rsi*2-2] 
  punpckldq   xmm5,[rsp+80h] 
  mov         [rsp+0A0h],eax 
  mov         eax, [r10+rdx-2] 
  punpckldq   xmm2,[rsp+90h] 
  mov         [rsp+0B0h],eax 
  mov         eax, [rbx] 
  punpckldq   xmm4,[rsp+0A0h] 
  mov         [rsp+80h],eax 
  mov         eax,[rbp] 
  punpckldq   xmm3,[rsp+0B0h] 
  mov         [rsp+90h],eax 
  mov         eax,[rsi+rbx] 
  movdqa      xmm0,[rsp+80h] 
  punpckldq   xmm0,[rsp+90h] 
  punpcklqdq  xmm5,xmm0 
  movdqa      [rsp+80h],xmm0 
  mov         [rsp+80h],eax 
  mov         eax,[rsi+rbp] 
  movdqa      xmm0,[rsp+80h] 
  movdqa      xmm1,xmm5 
  mov         [rsp+90h],eax 
  mov         eax,[rbx+rsi*2] 
  punpckldq   xmm0,[rsp+90h] 
  punpcklqdq  xmm2,xmm0 
  punpcklbw   xmm1,xmm2 
  punpckhbw   xmm5,xmm2 
  movdqa      [rsp+80h],xmm0 
  mov         [rsp+80h],eax 
  mov         eax,[rbp+rsi*2] 
  movdqa      xmm0, [rsp+80h] 
  mov         [rsp+90h],eax 
  mov         eax,[r10+rbx] 
  movdqa      xmm7,xmm1 
  punpckldq   xmm0,[rsp+90h] 
  punpcklqdq  xmm4,xmm0 
  movdqa      [rsp+80h],xmm0 
  mov         [rsp+80h],eax 
  mov         eax, [r10+rbp] 
  movdqa      xmm0,[rsp+80h] 
  mov         [rsp+90h],eax 
  punpckldq   xmm0,[rsp+90h] 
  punpcklqdq  xmm3,xmm0 
  movdqa      xmm0,xmm4 
  punpcklbw   xmm0,xmm3 
  punpckhbw   xmm4,xmm3 
  punpcklwd   xmm7,xmm0 
  punpckhwd   xmm1,xmm0 
  movdqa      xmm0,xmm5 
  movdqa      xmm6,xmm7 
  punpcklwd   xmm0,xmm4 
  punpckhwd   xmm5,xmm4 
  punpckldq   xmm6,xmm0 
  punpckhdq   xmm7,xmm0 
  movdqa      xmm0,xmm1 
  punpckldq   xmm0,xmm5 
  mov         rax, r14    ; pTC
  punpckhdq   xmm1,xmm5 
  movdqa      xmm9,xmm6 
  punpckhqdq  xmm6,xmm0 
  punpcklqdq  xmm9,xmm0 
  movdqa      xmm2,xmm7 
  movdqa      xmm13,xmm6 
  movdqa      xmm4,xmm9 
  movdqa      [rsp+10h],xmm9 
  punpcklqdq  xmm2,xmm1 
  punpckhqdq  xmm7,xmm1 
  pxor        xmm1,xmm1 
  movsx       ecx,byte [rax+3] 
  movsx       edx,byte [rax+2] 
  movsx       r8d,byte [rax+1] 
  movsx       r9d,byte [rax] 
  movdqa      xmm10,xmm1 
  movdqa      xmm15,xmm2 
  punpckhbw   xmm2,xmm1 
  punpckhbw   xmm6,xmm1 
  punpcklbw   xmm4,xmm1 
  movsx       eax,r11w 
  mov         word [rsp+0Eh],cx 
  mov         word [rsp+0Ch],cx 
  movdqa      xmm3,xmm7 
  movdqa      xmm8,xmm7 
  movdqa      [rsp+20h],xmm7 
  punpcklbw   xmm15,xmm1 
  punpcklbw   xmm13,xmm1 
  punpcklbw   xmm3,xmm1 
  mov         word [rsp+0Ah],dx 
  mov         word [rsp+8],dx 
  mov         word [rsp+6],r8w 
  movd        xmm0,eax 
  movdqa      [rsp+30h],xmm6 
  punpckhbw   xmm9,xmm1 
  punpckhbw   xmm8,xmm1 
  punpcklwd   xmm0,xmm0 
  mov         eax, r13d   ; iBeta
  mov         word [rsp+4],r8w 
  mov         word [rsp+2],r9w 
  pshufd      xmm12,xmm0,0 
  mov         word [rsp],r9w 
  movd        xmm0,eax 
  mov         eax,4 
  cwde             
  movdqa      xmm14, [rsp] 
  movdqa      [rsp],xmm2 
  movdqa      xmm2,xmm12 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm11,xmm0,0 
  psubw       xmm10,xmm14 
  movd        xmm0,eax 
  movdqa      xmm7,xmm14 
  movdqa      xmm6,xmm14 
  pcmpgtw     xmm7,xmm1 
  punpcklwd   xmm0,xmm0 
  pshufd      xmm5,xmm0,0 
  movdqa      xmm0,xmm4 
  movdqa      xmm1,xmm15 
  psubw       xmm4,xmm13 
  psubw       xmm0,xmm3 
  psubw       xmm1,xmm13 
  psubw       xmm3,xmm15 
  psllw       xmm1,2 
  paddw       xmm1,xmm0 
  paddw       xmm1,xmm5 
  movdqa      xmm0,xmm10 
  psraw       xmm1,3 
  pmaxsw      xmm0,xmm1 
  pminsw      xmm6,xmm0 
  movdqa      xmm1,xmm11 
  movdqa      xmm0,xmm13 
  psubw       xmm0,xmm15 
  pabsw       xmm0,xmm0 
  pcmpgtw     xmm2,xmm0 
  pabsw       xmm0,xmm4 
  pcmpgtw     xmm1,xmm0 
  pabsw       xmm0,xmm3 
  pand        xmm2,xmm1 
  movdqa      xmm1,xmm11 
  movdqa      xmm3,[rsp+30h] 
  pcmpgtw     xmm1,xmm0 
  movdqa      xmm0,xmm9 
  pand        xmm2,xmm1 
  psubw       xmm0,xmm8 
  psubw       xmm9,xmm3 
  pand        xmm2,xmm7 
  pand        xmm6,xmm2 
  psubw       xmm15,xmm6 
  paddw       xmm13,xmm6 
  movdqa      xmm2,[rsp] 
  movdqa      xmm1,xmm2 
  psubw       xmm1,xmm3 
  psubw       xmm8,xmm2 
  psllw       xmm1,2 
  paddw       xmm1,xmm0 
  paddw       xmm1,xmm5 
  movdqa      xmm0,xmm3 
  movdqa      xmm5,[rsp+10h] 
  psubw       xmm0,xmm2 
  psraw       xmm1,3 
  movdqa      xmm4,xmm5 
  pabsw       xmm0,xmm0 
  pmaxsw      xmm10,xmm1 
  movdqa      xmm1,xmm11 
  pcmpgtw     xmm12,xmm0 
  pabsw       xmm0,xmm9 
  pminsw      xmm14,xmm10 
  pcmpgtw     xmm1,xmm0 
  pabsw       xmm0,xmm8 
  pcmpgtw     xmm11,xmm0 
  pand        xmm12,xmm1 
  movdqa      xmm1,[rsp+20h] 
  pand        xmm12,xmm11 
  pand        xmm12,xmm7 
  pand        xmm14,xmm12 
  paddw       xmm3,xmm14 
  psubw       xmm2,xmm14 
  packuswb    xmm13,xmm3 
  packuswb    xmm15,xmm2 
  punpcklbw   xmm4,xmm13 
  punpckhbw   xmm5,xmm13 
  movdqa      xmm0,xmm15 
  punpcklbw   xmm0,xmm1 
  punpckhbw   xmm15,xmm1 
  movdqa      xmm3,xmm4 
  punpcklwd   xmm3,xmm0 
  punpckhwd   xmm4,xmm0 
  movdqa      xmm0,xmm5 
  movdqa      xmm2,xmm3 
  movdqa      xmm1,xmm4 
  punpcklwd   xmm0,xmm15 
  punpckhwd   xmm5,xmm15 
  punpckldq   xmm2,xmm0 
  punpckhdq   xmm3,xmm0 
  punpckldq   xmm1,xmm5 
  movdqa      xmm0,xmm2 
  punpcklqdq  xmm0,xmm1 
  punpckhdq   xmm4,xmm5 
  punpckhqdq  xmm2,xmm1 
  movdqa      [rsp+40h],xmm0 
  movdqa      xmm0,xmm3 
  movdqa      [rsp+90h],xmm2 
  mov         eax,[rsp+40h] 
  mov         [rdi-2],eax 
  mov         eax, [rsp+90h] 
  punpcklqdq  xmm0,xmm4 
  punpckhqdq  xmm3,xmm4 
  mov         [rsi+rdi-2],eax 
  movdqa      [rsp+50h],xmm0 
  mov         eax,[rsp+50h] 
  movdqa      [rsp+0A0h],xmm3 
  mov         [rdi+rsi*2-2],eax 
  mov         eax,[rsp+0A0h] 
  mov         [r10+rdi-2],eax 
  mov         eax,[rsp+48h] 
  mov         [rbx],eax 
  mov         eax,[rsp+98h] 
  mov         [rsi+rbx],eax 
  mov         eax,[rsp+58h] 
  mov         [rbx+rsi*2],eax 
  mov         eax, [rsp+0A8h] 
  mov         [r10+rbx],eax 
  mov         eax, [rsp+44h] 
  mov         [r12-2],eax 
  mov         eax,[rsp+94h] 
  mov         [rsi+r12-2],eax 
  mov         eax,[rsp+54h] 
  mov         [r12+rsi*2-2],eax 
  mov         eax, [rsp+0A4h] 
  mov         [r10+r12-2],eax 
  mov         eax,[rsp+4Ch] 
  mov         [rbp],eax 
  mov         eax,[rsp+9Ch] 
  mov         [rsi+rbp],eax 
  mov         eax, [rsp+5Ch] 
  mov         [rbp+rsi*2],eax 
  mov         eax,[rsp+0ACh] 
  mov         [r10+rbp],eax   
  lea         r11,[rsp+170h]    
  mov         rsp,r11 
  pop         r14
  pop         r13
  pop         r12  
  pop         rbp  
  pop         rbx  
  ret 



%elifdef  X86_32

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
    
%endif



;********************************************************************************
;
;   void DeblockLumaTransposeH2V_sse2(uint8_t * pPixY, int32_t iStride, uint8_t * pDst);
;
;********************************************************************************

WELS_EXTERN  DeblockLumaTransposeH2V_sse2

ALIGN  16

DeblockLumaTransposeH2V_sse2:
    push     r3 
    push     r4  
    push     r5

%assign   push_num   3 
    LOAD_3_PARA    

    SIGN_EXTENTION   r1, r1d

    mov      r5,    r7 
    mov      r3,    r7
    and      r3,    0Fh
    sub      r7,    r3
    sub      r7,    10h

    lea      r3,    [r0 + r1 * 8]
    lea      r4,    [r1 * 3]

    movq    xmm0,  [r0]
    movq    xmm7,  [r3]
    punpcklqdq   xmm0,  xmm7
    movq    xmm1,  [r0 + r1]
    movq    xmm7,  [r3 + r1]
    punpcklqdq   xmm1,  xmm7
    movq    xmm2,  [r0 + r1*2]
    movq    xmm7,  [r3 + r1*2]
    punpcklqdq   xmm2,  xmm7
    movq    xmm3,  [r0 + r4]
    movq    xmm7,  [r3 + r4]
    punpcklqdq   xmm3,  xmm7

    lea     r0,   [r0 + r1 * 4]
    lea     r3,   [r3 + r1 * 4]
    movq    xmm4,  [r0]
    movq    xmm7,  [r3]
    punpcklqdq   xmm4,  xmm7
    movq    xmm5,  [r0 + r1]
    movq    xmm7,  [r3 + r1]
    punpcklqdq   xmm5,  xmm7
    movq    xmm6,  [r0 + r1*2]
    movq    xmm7,  [r3 + r1*2]
    punpcklqdq   xmm6,  xmm7

    movdqa  [r7],   xmm0
    movq    xmm7,  [r0 + r4]
    movq    xmm0,  [r3 + r4]
    punpcklqdq   xmm7,  xmm0
    movdqa  xmm0,   [r7]

    SSE2_TransTwo8x8B  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r7]
    ;pOut: m5, m3, m4, m8, m6, m2, m7, m1
   
    movdqa  [r2],    xmm4
    movdqa  [r2 + 10h],  xmm2
    movdqa  [r2 + 20h],  xmm3
    movdqa  [r2 + 30h],  xmm7
    movdqa  [r2 + 40h],  xmm5
    movdqa  [r2 + 50h],  xmm1
    movdqa  [r2 + 60h],  xmm6
    movdqa  [r2 + 70h],  xmm0

    mov     r7,   r5
    pop     r5
    pop     r4
    pop     r3
    ret


;*******************************************************************************************
;
;   void DeblockLumaTransposeV2H_sse2(uint8_t * pPixY, int32_t iStride, uint8_t * pSrc);
;
;*******************************************************************************************

WELS_EXTERN   DeblockLumaTransposeV2H_sse2

ALIGN  16

DeblockLumaTransposeV2H_sse2:
    push     r3
    push     r4 

%assign  push_num 2
    LOAD_3_PARA

    SIGN_EXTENTION   r1, r1d 

    mov      r4,    r7
    mov      r3,    r7 
    and      r3,    0Fh
    sub      r7,    r3 
    sub      r7,    10h

    movdqa   xmm0,   [r2]
    movdqa   xmm1,   [r2 + 10h]
    movdqa   xmm2,   [r2 + 20h]
    movdqa   xmm3,   [r2 + 30h]
    movdqa   xmm4,   [r2 + 40h]
    movdqa   xmm5,   [r2 + 50h]
    movdqa   xmm6,   [r2 + 60h]
    movdqa   xmm7,   [r2 + 70h]

    SSE2_TransTwo8x8B  xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7, [r7]
    ;pOut: m5, m3, m4, m8, m6, m2, m7, m1

    lea      r2,   [r1 * 3]

    movq     [r0],  xmm4
    movq     [r0 + r1],  xmm2
    movq     [r0 + r1*2],  xmm3
    movq     [r0 + r2],  xmm7

    lea      r0,   [r0 + r1*4]
    movq     [r0],  xmm5
    movq     [r0 + r1],  xmm1
    movq     [r0 + r1*2],  xmm6
    movq     [r0 + r2],  xmm0

    psrldq    xmm4,   8
    psrldq    xmm2,   8
    psrldq    xmm3,   8
    psrldq    xmm7,   8
    psrldq    xmm5,   8
    psrldq    xmm1,   8
    psrldq    xmm6,   8
    psrldq    xmm0,   8

    lea       r0,  [r0 + r1*4]
    movq     [r0],  xmm4
    movq     [r0 + r1],  xmm2
    movq     [r0 + r1*2],  xmm3
    movq     [r0 + r2],  xmm7

    lea      r0,   [r0 + r1*4]
    movq     [r0],  xmm5
    movq     [r0 + r1],  xmm1
    movq     [r0 + r1*2],  xmm6
    movq     [r0 + r2],  xmm0


    mov      r7,   r4
    pop      r4
    pop      r3
    ret

