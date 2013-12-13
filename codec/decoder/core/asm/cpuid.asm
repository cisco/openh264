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
;*	cpu_mmx.asm
;*
;*  Abstract
;*		verify cpuid feature support and cpuid detection
;*
;*  History
;*      04/29/2009	Created
;*
;*************************************************************************/

bits 32

;******************************************************************************************
; Macros
;******************************************************************************************

%macro WELS_EXTERN 1
	%ifdef PREFIX
		global _%1
		%define %1 _%1
	%else
		global %1
	%endif
%endmacro

;******************************************************************************************
; Code
;******************************************************************************************

SECTION .text

; refer to "The IA-32 Intel(R) Architecture Software Developers Manual, Volume 2A A-M"
; section CPUID - CPU Identification

WELS_EXTERN WelsCPUIdVerify
ALIGN 16
;******************************************************************************************
;   int32_t WelsCPUIdVerify()
;******************************************************************************************
WelsCPUIdVerify:
    pushfd					; decrease the SP by 4 and load EFLAGS register onto stack, pushfd 32 bit and pushf for 16 bit
	pushfd					; need push 2 EFLAGS, one for processing and the another one for storing purpose
    pop     ecx				; get EFLAGS to bit manipulation
    mov     eax, ecx		; store into ecx followed
    xor     eax, 00200000h	; get ID flag (bit 21) of EFLAGS to directly indicate cpuid support or not
	xor		eax, ecx		; get the ID flag bitwise, eax - 0: not support; otherwise: support
    popfd					; store back EFLAGS and keep unchanged for system
    ret

WELS_EXTERN WelsCPUId
ALIGN 16
;****************************************************************************************************
;   void WelsCPUId( int32_t index, int32_t *uiFeatureA, int32_t *uiFeatureB, int32_t *uiFeatureC, int32_t *uiFeatureD )
;****************************************************************************************************
WelsCPUId:
	push	ebx
	push	edi

	mov     eax, [esp+12]	; operating index
    cpuid					; cpuid

	; processing various information return
	mov     edi, [esp+16]
    mov     [edi], eax
    mov     edi, [esp+20]
    mov     [edi], ebx
    mov     edi, [esp+24]
    mov     [edi], ecx
    mov     edi, [esp+28]
    mov     [edi], edx

	pop		edi
    pop     ebx
	ret

WELS_EXTERN WelsCPUSupportAVX
; need call after cpuid=1 and eax, ecx flag got then
ALIGN 16
;****************************************************************************************************
;   int32_t WelsCPUSupportAVX( uint32_t eax, uint32_t ecx )
;****************************************************************************************************
WelsCPUSupportAVX:
	mov eax, [esp+4]
	mov ecx, [esp+8]

	; refer to detection of AVX addressed in INTEL AVX manual document
	and ecx, 018000000H
	cmp ecx, 018000000H		; check both OSXSAVE and AVX feature flags
	jne avx_not_supported
	; processor supports AVX instructions and XGETBV is enabled by OS
	mov ecx, 0				; specify 0 for XFEATURE_ENABLED_MASK register
	XGETBV					; result in EDX:EAX
	and eax, 06H
	cmp eax, 06H			; check OS has enabled both XMM and YMM state support
	jne avx_not_supported
	mov eax, 1
	ret
avx_not_supported:
	mov eax, 0
	ret

WELS_EXTERN WelsCPUSupportFMA
; need call after cpuid=1 and eax, ecx flag got then
ALIGN 16
;****************************************************************************************************
;   int32_t WelsCPUSupportFMA( uint32_t eax, uint32_t ecx )
;****************************************************************************************************
WelsCPUSupportFMA:
	mov eax, [esp+4]
	mov ecx, [esp+8]

	; refer to detection of FMA addressed in INTEL AVX manual document
	and ecx, 018001000H
	cmp ecx, 018001000H		; check OSXSAVE, AVX, FMA feature flags
	jne fma_not_supported
	; processor supports AVX,FMA instructions and XGETBV is enabled by OS
	mov ecx, 0				; specify 0 for XFEATURE_ENABLED_MASK register
	XGETBV					; result in EDX:EAX
	and eax, 06H
	cmp eax, 06H			; check OS has enabled both XMM and YMM state support
	jne fma_not_supported
	mov eax, 1
	ret
fma_not_supported:
	mov eax, 0
	ret

WELS_EXTERN WelsEmms
ALIGN 16
;******************************************************************************************
;   void WelsEmms()
;******************************************************************************************
WelsEmms:
	emms	; empty mmx technology states
	ret



