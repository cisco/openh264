#include <gtest/gtest.h>
#include <math.h>
#include <string.h>
#include "cpu.h"
#include "cpu_core.h"
#include "util.h"
#include "macros.h"
#include "IWelsVP.h"
#include "AdaptiveQuantization.h"


using namespace WelsVP;

static void FillWithRandomData (uint8_t* p, int32_t Len) {
  for (int32_t i = 0; i < Len; i++) {
    p[i] = rand() % 256;
  }
}

void SampleVariance16x16_ref (uint8_t* pRefY, int32_t iRefStride, uint8_t* pSrcY, int32_t iSrcStride,
                              SMotionTextureUnit* pMotionTexture) {
  uint32_t uiCurSquare = 0,  uiSquare = 0;
  uint16_t uiCurSum = 0,  uiSum = 0;

  for (int32_t y = 0; y < MB_WIDTH_LUMA; y++) {
    for (int32_t x = 0; x < MB_WIDTH_LUMA; x++) {
      uint32_t uiDiff = WELS_ABS (pRefY[x] - pSrcY[x]);
      uiSum += uiDiff;
      uiSquare += uiDiff * uiDiff;

      uiCurSum += pSrcY[x];
      uiCurSquare += pSrcY[x] * pSrcY[x];
    }
    pRefY += iRefStride;
    pSrcY += iSrcStride;
  }

  uiSum = uiSum >> 8;
  pMotionTexture->uiMotionIndex = (uiSquare >> 8) - (uiSum * uiSum);

  uiCurSum = uiCurSum >> 8;
  pMotionTexture->uiTextureIndex = (uiCurSquare >> 8) - (uiCurSum * uiCurSum);
}

#define GENERATE_AQTEST(method, flag) \
TEST (AdaptiveQuantization, method) {\
  uint32_t uiCPUFlags = WelsCPUFeatureDetect(NULL); \
  if ((uiCPUFlags & flag) == 0 && flag != 0) \
    return; \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pRefY,32*16,16)\
  ENFORCE_STACK_ALIGN_1D (uint8_t, pSrcY,48*16,16)\
  SMotionTextureUnit pMotionTexture[2];\
  FillWithRandomData (pRefY,32*16);\
  FillWithRandomData (pSrcY,48*16);\
  SampleVariance16x16_ref (pRefY,32,pSrcY,48,&pMotionTexture[0]);\
  method(pRefY,32,pSrcY,48,&pMotionTexture[1]);\
  ASSERT_EQ(pMotionTexture[0].uiMotionIndex,pMotionTexture[1].uiMotionIndex);\
  ASSERT_EQ(pMotionTexture[0].uiMotionIndex,pMotionTexture[1].uiMotionIndex);\
  memset (pRefY,0,32*16);\
  memset (pSrcY,255,48*16);\
  SampleVariance16x16_ref (pRefY,32,pSrcY,48,&pMotionTexture[0]);\
  method(pRefY,32,pSrcY,48,&pMotionTexture[1]);\
  ASSERT_EQ(pMotionTexture[0].uiMotionIndex,pMotionTexture[1].uiMotionIndex);\
  ASSERT_EQ(pMotionTexture[0].uiMotionIndex,pMotionTexture[1].uiMotionIndex);\
}

GENERATE_AQTEST (SampleVariance16x16_c, 0)
#if defined(X86_ASM)
GENERATE_AQTEST (SampleVariance16x16_sse2, WELS_CPU_SSE2)
#endif

#if defined(HAVE_NEON)
GENERATE_AQTEST (SampleVariance16x16_neon, WELS_CPU_NEON)
#endif

#if defined(HAVE_NEON_AARCH64)
GENERATE_AQTEST (SampleVariance16x16_AArch64_neon, WELS_CPU_NEON)
#endif

