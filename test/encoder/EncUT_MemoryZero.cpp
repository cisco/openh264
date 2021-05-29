#include <gtest/gtest.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "cpu_core.h"
#include "cpu.h"
#include "macros.h"
#include "wels_func_ptr_def.h"
#include "../../codec/encoder/core/src/encoder.cpp"

using namespace WelsEnc;
#define MEMORYZEROTEST_NUM 1000

TEST (SetMemZeroFunTest, WelsSetMemZero) {
  int32_t iLen = 64;
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores);
  /* Functionality utilization of CPU instructions dependency */
  sFuncPtrList.pfSetMemZeroSize8           = WelsSetMemZero_c; // confirmed_safe_unsafe_usage
  sFuncPtrList.pfSetMemZeroSize64Aligned16 = WelsSetMemZero_c; // confirmed_safe_unsafe_usage
  sFuncPtrList.pfSetMemZeroSize64          = WelsSetMemZero_c; // confirmed_safe_unsafe_usage
#if defined(X86_ASM)
  if (uiCpuFlag & WELS_CPU_MMXEXT) {
    sFuncPtrList.pfSetMemZeroSize8           = WelsSetMemZeroSize8_mmx;  // confirmed_safe_unsafe_usage
    sFuncPtrList.pfSetMemZeroSize64Aligned16 = WelsSetMemZeroSize64_mmx; // confirmed_safe_unsafe_usage
    sFuncPtrList.pfSetMemZeroSize64          = WelsSetMemZeroSize64_mmx; // confirmed_safe_unsafe_usage
  }
  if (uiCpuFlag & WELS_CPU_SSE2) {
    sFuncPtrList.pfSetMemZeroSize64Aligned16 = WelsSetMemZeroAligned64_sse2; // confirmed_safe_unsafe_usage
  }
#else
  (void) uiCpuFlag; // Avoid warnings if no assembly is enabled
#endif//X86_ASM

#if defined(HAVE_NEON)
  if (uiCpuFlag & WELS_CPU_NEON) {
    sFuncPtrList.pfSetMemZeroSize8           = WelsSetMemZero_neon;
    sFuncPtrList.pfSetMemZeroSize64Aligned16 = WelsSetMemZero_neon;
    sFuncPtrList.pfSetMemZeroSize64          = WelsSetMemZero_neon;
  }
#endif

#if defined(HAVE_NEON_AARCH64)
  if (uiCpuFlag & WELS_CPU_NEON) {
    sFuncPtrList.pfSetMemZeroSize8           = WelsSetMemZero_AArch64_neon;
    sFuncPtrList.pfSetMemZeroSize64Aligned16 = WelsSetMemZero_AArch64_neon;
    sFuncPtrList.pfSetMemZeroSize64          = WelsSetMemZero_AArch64_neon;
  }
#endif

  ENFORCE_STACK_ALIGN_2D (uint8_t, pInputAlign, 2, 64 * 101, 16)

  for (int32_t k = 0; k < MEMORYZEROTEST_NUM; k++) {
    memset (pInputAlign[0], 255, 64 * 101);
    memset (pInputAlign[1], 255, 64 * 101);
    iLen = 64 * (1 + (rand() % 100));
    WelsSetMemZero_c (pInputAlign[0], iLen);
    sFuncPtrList.pfSetMemZeroSize64Aligned16 (pInputAlign[1], iLen);
    for (int32_t i = 0 ; i < 64 * 101; i++) {
      ASSERT_EQ (pInputAlign[0][i], pInputAlign[1][i]);
    }
  }

  for (int32_t k = 0; k < MEMORYZEROTEST_NUM; k++) {
    memset (pInputAlign[0], 255, 64 * 101);
    memset (pInputAlign[1], 255, 64 * 101);
    iLen = 64 * (1 + (rand() % 100));
    WelsSetMemZero_c (pInputAlign[0] + 1, iLen);
    sFuncPtrList.pfSetMemZeroSize64 (pInputAlign[1] + 1, iLen);
    for (int32_t i = 0 ; i < 64 * 101; i++) {
      ASSERT_EQ (pInputAlign[0][i], pInputAlign[1][i]);
    }
  }

  memset (pInputAlign[0], 255, 64 * 101);
  memset (pInputAlign[1], 255, 64 * 101);
  iLen = 32;
  WelsSetMemZero_c (pInputAlign[0] + 1, iLen);
  sFuncPtrList.pfSetMemZeroSize8 (pInputAlign[1] + 1, iLen);
  for (int32_t i = 0 ; i < 64 * 101; i++) {
    ASSERT_EQ (pInputAlign[0][i], pInputAlign[1][i]);
  }

  memset (pInputAlign[0], 255, 64 * 101);
  memset (pInputAlign[1], 255, 64 * 101);
  iLen = 24;
  WelsSetMemZero_c (pInputAlign[0] + 1, iLen);
  sFuncPtrList.pfSetMemZeroSize8 (pInputAlign[1] + 1, iLen);
  for (int32_t i = 0 ; i < 64 * 101; i++) {
    ASSERT_EQ (pInputAlign[0][i], pInputAlign[1][i]);
  }
}


