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
 * \file	cpu.cpp
 *
 * \brief	CPU compatibility detection
 *
 * \date	04/29/2009 Created
 *
 *************************************************************************************
 */
#include <string.h>
#include <stdio.h>

#include "cpu.h"
#include "cpu_core.h"



#define    CPU_Vendor_AMD    "AuthenticAMD"
#define    CPU_Vendor_INTEL  "GenuineIntel"
#define    CPU_Vendor_CYRIX  "CyrixInstead"

#if defined(X86_ASM)

uint32_t WelsCPUFeatureDetect (int32_t* pNumberOfLogicProcessors) {
  uint32_t uiCPU = 0;
  uint32_t uiFeatureA = 0, uiFeatureB = 0, uiFeatureC = 0, uiFeatureD = 0;
  int32_t  CacheLineSize = 0;
  int8_t   chVendorName[16] = { 0 };
  uint32_t uiMaxCpuidLevel = 0;

  if (!WelsCPUIdVerify()) {
    /* cpuid is not supported in cpu */
    return 0;
  }

  WelsCPUId (0, &uiFeatureA, (uint32_t*)&chVendorName[0], (uint32_t*)&chVendorName[8], (uint32_t*)&chVendorName[4]);
  uiMaxCpuidLevel = uiFeatureA;
  if (uiMaxCpuidLevel == 0) {
    /* maximum input value for basic cpuid information */
    return 0;
  }

  WelsCPUId (1, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);
  if ((uiFeatureD & 0x00800000) == 0) {
    /* Basic MMX technology is not support in cpu, mean nothing for us so return here */
    return 0;
  }

  uiCPU = WELS_CPU_MMX;
  if (uiFeatureD & 0x02000000) {
    /* SSE technology is identical to AMD MMX extensions */
    uiCPU |= WELS_CPU_MMXEXT | WELS_CPU_SSE;
  }
  if (uiFeatureD & 0x04000000) {
    /* SSE2 support here */
    uiCPU |= WELS_CPU_SSE2;
  }
  if (uiFeatureD & 0x00000001) {
    /* x87 FPU on-chip checking */
    uiCPU |= WELS_CPU_FPU;
  }
  if (uiFeatureD & 0x00008000) {
    /* CMOV instruction checking */
    uiCPU |= WELS_CPU_CMOV;
  }
  if ((!strcmp ((const char*)chVendorName, CPU_Vendor_INTEL)) ||
      (!strcmp((const char*)chVendorName, CPU_Vendor_AMD)) ) {	// confirmed_safe_unsafe_usage
    if (uiFeatureD & 0x10000000) {
      /* Multi-Threading checking: contains of multiple logic processors */
      uiCPU |= WELS_CPU_HTT;
    }
  }

  if (uiFeatureC & 0x00000001) {
    /* SSE3 support here */
    uiCPU |= WELS_CPU_SSE3;
  }
  if (uiFeatureC & 0x00000200) {
    /* SSSE3 support here */
    uiCPU |= WELS_CPU_SSSE3;
  }
  if (uiFeatureC & 0x00080000) {
    /* SSE4.1 support here, 45nm Penryn processor */
    uiCPU |= WELS_CPU_SSE41;
  }
  if (uiFeatureC & 0x00100000) {
    /* SSE4.2 support here, next generation Nehalem processor */
    uiCPU |= WELS_CPU_SSE42;
  }
  if (WelsCPUSupportAVX (uiFeatureA, uiFeatureC)) {
    /* AVX supported */
    uiCPU |= WELS_CPU_AVX;
  }
  if (WelsCPUSupportFMA (uiFeatureA, uiFeatureC)) {
    /* AVX FMA supported */
    uiCPU |= WELS_CPU_FMA;
  }
  if (uiFeatureC & 0x02000000) {
    /* AES checking */
    uiCPU |= WELS_CPU_AES;
  }
  if (uiFeatureC & 0x00400000) {
    /* MOVBE checking */
    uiCPU |= WELS_CPU_MOVBE;
  }

  if( pNumberOfLogicProcessors != NULL ){
    if( uiCPU & WELS_CPU_HTT){
      *pNumberOfLogicProcessors = (uiFeatureB & 0x00ff0000) >> 16; // feature bits: 23-16 on returned EBX
    } else {
      *pNumberOfLogicProcessors = 0;
    }
    if( !strcmp((const char*)chVendorName, CPU_Vendor_INTEL) ){
      if( uiMaxCpuidLevel >= 4 ){
        uiFeatureC = 0;
        WelsCPUId(0x4, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);
        if( uiFeatureA != 0 ){
          *pNumberOfLogicProcessors = ((uiFeatureA&0xfc000000)>>26) + 1;
        }
      }
    }
  }

  WelsCPUId (0x80000000, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);

  if ((!strcmp ((const char*)chVendorName, CPU_Vendor_AMD))
      && (uiFeatureA >= 0x80000001)) {	// confirmed_safe_unsafe_usage
    WelsCPUId (0x80000001, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);
    if (uiFeatureD & 0x00400000) {
      uiCPU |= WELS_CPU_MMXEXT;
    }
    if (uiFeatureD & 0x80000000) {
      uiCPU |= WELS_CPU_3DNOW;
    }
  }

  if (!strcmp ((const char*)chVendorName, CPU_Vendor_INTEL)) {	// confirmed_safe_unsafe_usage
    int32_t  family, model;

    WelsCPUId (1, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);
    family = ((uiFeatureA >> 8) & 0xf) + ((uiFeatureA >> 20) & 0xff);
    model  = ((uiFeatureA >> 4) & 0xf) + ((uiFeatureA >> 12) & 0xf0);

    if ((family == 6) && (model == 9 || model == 13 || model == 14)) {
      uiCPU &= ~ (WELS_CPU_SSE2 | WELS_CPU_SSE3);
    }
  }

  // get cache line size
  if ((!strcmp ((const char*)chVendorName, CPU_Vendor_INTEL))
      || ! (strcmp ((const char*)chVendorName, CPU_Vendor_CYRIX))) {	// confirmed_safe_unsafe_usage
    WelsCPUId (1, &uiFeatureA, &uiFeatureB, &uiFeatureC, &uiFeatureD);

    CacheLineSize = (uiFeatureB & 0xff00) >>
                    5;	// ((clflush_line_size >> 8) << 3), CLFLUSH_line_size * 8 = CacheLineSize_in_byte

    if (CacheLineSize == 128) {
      uiCPU |= WELS_CPU_CACHELINE_128;
    } else if (CacheLineSize == 64) {
      uiCPU |= WELS_CPU_CACHELINE_64;
    } else if (CacheLineSize == 32) {
      uiCPU |= WELS_CPU_CACHELINE_32;
    } else if (CacheLineSize == 16) {
      uiCPU |= WELS_CPU_CACHELINE_16;
    }
  }

  return uiCPU;
}


void WelsCPURestore (const uint32_t kuiCPU) {
  if (kuiCPU & (WELS_CPU_MMX | WELS_CPU_MMXEXT | WELS_CPU_3DNOW | WELS_CPU_3DNOWEXT)) {
    WelsEmms();
  }
}

void WelsXmmRegEmptyOp(void * pSrc) {
}

#endif

#if defined(ARM_ASM)

uint32_t WelsCPUFeatureDetect (int32_t* pNumberOfLogicProcessors) {
#ifdef __linux__
  FILE *f = fopen("/proc/cpuinfo", "r");

  if (!f)
    return 0;

  char buf[200];
  int flags = 0;
  while (fgets(buf, sizeof(buf), f)) {
    if (!strncmp(buf, "Features", strlen("Features"))) {
      if (strstr(buf, " neon "))
        flags |= WELS_CPU_NEON;
      break;
    }
  }
  fclose(f);
  return flags;

#else /* !defined(__linux__) */
#ifdef HAVE_NEON
  return WELS_CPU_NEON;
#else
  return 0;
#endif
#endif /* !defined(__linux__) */
}
#endif

