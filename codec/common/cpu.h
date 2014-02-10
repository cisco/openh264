/*!
 * \copy
 *     Copyright (c)  2009-2013, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * \file	cpu.h
 *
 * \brief	CPU feature compatibility detection
 *
 * \date	04/29/2009 Created
 *
 *************************************************************************************
 */
#if !defined(WELS_CPU_DETECTION_H__)
#define WELS_CPU_DETECTION_H__

#include "typedefs.h"
#include "cpu_core.h"


#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

#if defined(X86_ASM)
/*
 *	cpuid support verify routine
 *  return 0 if cpuid is not supported by cpu
 */
int32_t  WelsCPUIdVerify();

void WelsCPUId (uint32_t uiIndex, uint32_t* pFeatureA, uint32_t* pFeatureB, uint32_t* pFeatureC, uint32_t* pFeatureD);

int32_t WelsCPUSupportAVX (uint32_t eax, uint32_t ecx);
int32_t WelsCPUSupportFMA (uint32_t eax, uint32_t ecx);

void WelsEmms();

uint32_t WelsCPUFeatureDetect (int32_t* pNumberOfLogicProcessors);

/*
 *	clear FPU registers states for potential float based calculation if support
 */
void     WelsCPURestore (const uint32_t kuiCPU);

#ifdef  WIN64
void     WelsXmmRegStore(void * src);
void     WelsXmmRegLoad(void * src);
#endif

#endif

void     WelsXmmRegEmptyOp(void * pSrc);

#if defined(__cplusplus)
}
#endif//__cplusplus

typedef  void (*WelsXmmRegProtectFunc)(void * pSrc);


#if defined(WIN64) && defined(X86_ASM)
#define   XMMREG_PROTECT_DECLARE(name) \
  WelsXmmRegProtectFunc name##load;\
  WelsXmmRegProtectFunc name##store;\
  uint8_t               name##Buffer[160];

#define   XMMREG_PROTECT_INIT(name) \
  { \
    uint32_t uiCpuFlag = WelsCPUFeatureDetect(NULL);\
    if( uiCpuFlag & WELS_CPU_SSE2 ){\
      name##load = WelsXmmRegLoad;\
      name##store = WelsXmmRegStore; \
    } else { \
      name##load = WelsXmmRegEmptyOp; \
      name##store = WelsXmmRegEmptyOp; \
    } \
  }

#define   XMMREG_PROTECT_UNINIT(name) \

#define   XMMREG_PROTECT_STORE(name) \
  name##store(name##Buffer);

#define   XMMREG_PROTECT_LOAD(name) \
  name##load(name##Buffer);

#else

#define   XMMREG_PROTECT_DECLARE(name)
#define   XMMREG_PROTECT_INIT(name)
#define   XMMREG_PROTECT_UNINIT(name)
#define   XMMREG_PROTECT_STORE(name)
#define   XMMREG_PROTECT_LOAD(name)

#endif

#endif//WELS_CPU_DETECTION_H__
