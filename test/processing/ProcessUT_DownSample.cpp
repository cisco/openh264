#include <gtest/gtest.h>
#include "cpu.h"
#include "cpu_core.h"
#include "util.h"
#include "macros.h"
#include "IWelsVP.h"
#include "downsample.h"

using namespace WelsVP;

void DyadicBilinearDownsampler_ref (uint8_t* pDst, const int32_t kiDstStride,
                                    uint8_t* pSrc, const int32_t kiSrcStride,
                                    const int32_t kiSrcWidth, const int32_t kiSrcHeight) {
  uint8_t* pDstLine = pDst;
  uint8_t* pSrcLine = pSrc;
  const int32_t kiSrcStridex2 = kiSrcStride << 1;
  const int32_t kiDstWidth    = kiSrcWidth  >> 1;
  const int32_t kiDstHeight   = kiSrcHeight >> 1;

  for (int32_t j = 0; j < kiDstHeight; j ++) {
    for (int32_t i = 0; i < kiDstWidth; i ++) {
      const int32_t kiSrcX = i << 1;
      const int32_t kiTempRow1 = (pSrcLine[kiSrcX] + pSrcLine[kiSrcX + 1] + 1) >> 1;
      const int32_t kiTempRow2 = (pSrcLine[kiSrcX + kiSrcStride] + pSrcLine[kiSrcX + kiSrcStride + 1] + 1) >> 1;

      pDstLine[i] = (uint8_t) ((kiTempRow1 + kiTempRow2 + 1) >> 1);
    }
    pDstLine += kiDstStride;
    pSrcLine += kiSrcStridex2;
  }
}

void DyadicBilinearDownsampler2_ref (uint8_t* pDst, const int32_t kiDstStride,
                                     const uint8_t* pSrc, const int32_t kiSrcStride,
                                     const int32_t kiSrcWidth, const int32_t kiSrcHeight) {
  uint8_t* pDstLine = pDst;
  const uint8_t* pSrcLine1 = pSrc;
  const uint8_t* pSrcLine2 = pSrc + kiSrcStride;
  const int32_t kiDstWidth  = kiSrcWidth >> 1;
  const int32_t kiDstHeight = kiSrcHeight >> 1;

  for (int32_t j = 0; j < kiDstHeight; j++) {
    for (int32_t i = 0; i < kiDstWidth; i++) {
      const int32_t kiTempCol1 = (pSrcLine1[2 * i + 0] + pSrcLine2[2 * i + 0] + 1) >> 1;
      const int32_t kiTempCol2 = (pSrcLine1[2 * i + 1] + pSrcLine2[2 * i + 1] + 1) >> 1;
      pDstLine[i] = (uint8_t) ((kiTempCol1 + kiTempCol2 + 1) >> 1);
    }
    pDstLine += kiDstStride;
    pSrcLine1 += 2 * kiSrcStride;
    pSrcLine2 += 2 * kiSrcStride;
  }
}

void GeneralBilinearFastDownsampler_ref (uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth,
    const int32_t kiDstHeight,
    uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight) {
  const uint32_t kuiScaleBitWidth = 16, kuiScaleBitHeight = 15;
  const uint32_t kuiScaleWidth = (1 << kuiScaleBitWidth), kuiScaleHeight = (1 << kuiScaleBitHeight);
  int32_t fScalex = WELS_ROUND ((float)kiSrcWidth / (float)kiDstWidth * kuiScaleWidth);
  int32_t fScaley = WELS_ROUND ((float)kiSrcHeight / (float)kiDstHeight * kuiScaleHeight);
  uint32_t x;
  int32_t iYInverse, iXInverse;

  uint8_t* pByDst = pDst;
  uint8_t* pByLineDst = pDst;

  iYInverse = 1 << (kuiScaleBitHeight - 1);
  for (int32_t i = 0; i < kiDstHeight - 1; i++) {
    int32_t iYy = iYInverse >> kuiScaleBitHeight;
    int32_t fv = iYInverse & (kuiScaleHeight - 1);

    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;

    pByDst = pByLineDst;
    iXInverse = 1 << (kuiScaleBitWidth - 1);
    for (int32_t j = 0; j < kiDstWidth - 1; j++) {
      int32_t iXx = iXInverse >> kuiScaleBitWidth;
      int32_t iFu = iXInverse & (kuiScaleWidth - 1);

      uint8_t* pByCurrent = pBySrc + iXx;
      uint8_t a, b, c, d;

      a = *pByCurrent;
      b = * (pByCurrent + 1);
      c = * (pByCurrent + kiSrcStride);
      d = * (pByCurrent + kiSrcStride + 1);

      x  = (((uint32_t) (kuiScaleWidth - 1 - iFu)) * (kuiScaleHeight - 1 - fv) >> kuiScaleBitWidth) * a;
      x += (((uint32_t) (iFu)) * (kuiScaleHeight - 1 - fv) >> kuiScaleBitWidth) * b;
      x += (((uint32_t) (kuiScaleWidth - 1 - iFu)) * (fv) >> kuiScaleBitWidth) * c;
      x += (((uint32_t) (iFu)) * (fv) >> kuiScaleBitWidth) * d;
      x >>= (kuiScaleBitHeight - 1);
      x += 1;
      x >>= 1;
      //x = (((__int64)(SCALE_BIG - 1 - iFu))*(SCALE_BIG - 1 - fv)*a + ((__int64)iFu)*(SCALE_BIG - 1 -fv)*b + ((__int64)(SCALE_BIG - 1 -iFu))*fv*c +
      // ((__int64)iFu)*fv*d + (1 << (2*SCALE_BIT_BIG-1)) ) >> (2*SCALE_BIT_BIG);
      x = WELS_CLAMP (x, 0, 255);
      *pByDst++ = (uint8_t)x;

      iXInverse += fScalex;
    }
    *pByDst = * (pBySrc + (iXInverse >> kuiScaleBitWidth));
    pByLineDst += kiDstStride;
    iYInverse += fScaley;
  }

  // last row special
  {
    int32_t iYy = iYInverse >> kuiScaleBitHeight;
    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;

    pByDst = pByLineDst;
    iXInverse = 1 << (kuiScaleBitWidth - 1);
    for (int32_t j = 0; j < kiDstWidth; j++) {
      int32_t iXx = iXInverse >> kuiScaleBitWidth;
      *pByDst++ = * (pBySrc + iXx);

      iXInverse += fScalex;
    }
  }
}

void GeneralBilinearAccurateDownsampler_ref (uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth,
    const int32_t kiDstHeight,
    uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight) {
  const int32_t kiScaleBit = 15;
  const int32_t kiScale = (1 << kiScaleBit);
  int32_t iScalex = WELS_ROUND ((float)kiSrcWidth / (float)kiDstWidth * kiScale);
  int32_t iScaley = WELS_ROUND ((float)kiSrcHeight / (float)kiDstHeight * kiScale);
  int64_t x;
  int32_t iYInverse, iXInverse;

  uint8_t* pByDst = pDst;
  uint8_t* pByLineDst = pDst;

  iYInverse = 1 << (kiScaleBit - 1);
  for (int32_t i = 0; i < kiDstHeight - 1; i++) {
    int32_t iYy = iYInverse >> kiScaleBit;
    int32_t iFv = iYInverse & (kiScale - 1);

    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;

    pByDst = pByLineDst;
    iXInverse = 1 << (kiScaleBit - 1);
    for (int32_t j = 0; j < kiDstWidth - 1; j++) {
      int32_t iXx = iXInverse >> kiScaleBit;
      int32_t iFu = iXInverse & (kiScale - 1);

      uint8_t* pByCurrent = pBySrc + iXx;
      uint8_t a, b, c, d;

      a = *pByCurrent;
      b = * (pByCurrent + 1);
      c = * (pByCurrent + kiSrcStride);
      d = * (pByCurrent + kiSrcStride + 1);

      x = (((int64_t) (kiScale - 1 - iFu)) * (kiScale - 1 - iFv) * a + ((int64_t)iFu) * (kiScale - 1 - iFv) * b + ((int64_t) (
             kiScale - 1 - iFu)) * iFv * c +
           ((int64_t)iFu) * iFv * d + (int64_t) (1 << (2 * kiScaleBit - 1))) >> (2 * kiScaleBit);
      x = WELS_CLAMP (x, 0, 255);
      *pByDst++ = (uint8_t)x;

      iXInverse += iScalex;
    }
    *pByDst = * (pBySrc + (iXInverse >> kiScaleBit));
    pByLineDst += kiDstStride;
    iYInverse += iScaley;
  }

  // last row special
  {
    int32_t iYy = iYInverse >> kiScaleBit;
    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;

    pByDst = pByLineDst;
    iXInverse = 1 << (kiScaleBit - 1);
    for (int32_t j = 0; j < kiDstWidth; j++) {
      int32_t iXx = iXInverse >> kiScaleBit;
      *pByDst++ = * (pBySrc + iXx);

      iXInverse += iScalex;
    }
  }
}

#define GENERATE_DyadicBilinearDownsampler_UT_with_ref(func, ASM, CPUFLAGS, ref_func) \
TEST (DownSampleTest, func) { \
  if (ASM) {\
    int32_t iCpuCores = 0; \
    uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
    if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
    return; \
  } \
  ENFORCE_STACK_ALIGN_1D (uint8_t, dst_c, 50000, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, src_c, 50000, 16); \
  int dst_stride_c; \
  int src_stride_c; \
  int src_width_c; \
  int src_height_c; \
  ENFORCE_STACK_ALIGN_1D (uint8_t, dst_a, 50000, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, src_a, 50000, 16); \
  int dst_stride_a; \
  int src_stride_a; \
  int src_width_a; \
  int src_height_a; \
  dst_stride_c = dst_stride_a = 560; \
  src_stride_c = src_stride_a = 560; \
  src_width_c = src_width_a = 512; \
  src_height_c = src_height_a = 80; \
  for (int j = 0; j < 50000; j++) { \
    dst_c[j] = dst_a[j] = rand() % 256; \
    src_c[j] = src_a[j] = rand() % 256; \
  } \
  ref_func (dst_c, dst_stride_c, src_c, src_stride_c, src_width_c, src_height_c); \
  func (dst_a, dst_stride_a, src_a, src_stride_a, src_width_a, src_height_a); \
  for (int j = 0; j < (src_height_c >> 1); j++) { \
    for (int m = 0; m < (src_width_c >> 1); m++) { \
      ASSERT_EQ (dst_c[m + j * dst_stride_c], dst_a[m + j * dst_stride_a]); \
    } \
  } \
}

#define GENERATE_DyadicBilinearDownsampler_UT(func, ASM, CPUFLAGS) \
  GENERATE_DyadicBilinearDownsampler_UT_with_ref(func, ASM, CPUFLAGS, DyadicBilinearDownsampler_ref)
#define GENERATE_DyadicBilinearDownsampler2_UT(func, ASM, CPUFLAGS) \
  GENERATE_DyadicBilinearDownsampler_UT_with_ref(func, ASM, CPUFLAGS, DyadicBilinearDownsampler2_ref)

#define GENERATE_DyadicBilinearOneThirdDownsampler_UT(func, ASM, CPUFLAGS) \
TEST (DownSampleTest, func) { \
  if (ASM) {\
    int32_t iCpuCores = 0; \
    uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
    if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
    return; \
  } \
  ENFORCE_STACK_ALIGN_1D (uint8_t, dst_c, 50000, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, src_c, 50000, 16); \
  int dst_stride_c; \
  int src_stride_c; \
  int src_width_c; \
  int src_height_c; \
  ENFORCE_STACK_ALIGN_1D (uint8_t, dst_a, 50000, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, src_a, 50000, 16); \
  int dst_stride_a; \
  int src_stride_a; \
  int src_width_a; \
  int src_height_a; \
  dst_stride_c = dst_stride_a = 560; \
  src_stride_c = src_stride_a = 560; \
  src_width_c = src_width_a = 480; \
  src_height_c = src_height_a = 30; \
  for (int j = 0; j < 50000; j++) { \
    dst_c[j] = dst_a[j] = rand() % 256; \
    src_c[j] = src_a[j] = rand() % 256; \
  } \
  DyadicBilinearOneThirdDownsampler_c (dst_c, dst_stride_c, src_c, src_stride_c, src_width_c, src_height_c/3); \
  func (dst_a, dst_stride_a, src_a, src_stride_a, src_width_a, src_height_a/3); \
  for (int j = 0; j < (src_height_c /3 ); j++) { \
    for (int m = 0; m < (src_width_c /3); m++) { \
      ASSERT_EQ (dst_c[m + j * dst_stride_c], dst_a[m + j * dst_stride_a]); \
    } \
  } \
}

#define GENERATE_DyadicBilinearQuarterDownsampler_UT(func, ASM, CPUFLAGS) \
TEST (DownSampleTest, func) { \
  if (ASM) {\
    int32_t iCpuCores = 0; \
    uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
    if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
    return; \
  } \
  ENFORCE_STACK_ALIGN_1D (uint8_t, dst_c, 50000, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, src_c, 50000, 16); \
  int dst_stride_c; \
  int src_stride_c; \
  int src_width_c; \
  int src_height_c; \
  ENFORCE_STACK_ALIGN_1D (uint8_t, dst_a, 50000, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, src_a, 50000, 16); \
  int dst_stride_a; \
  int src_stride_a; \
  int src_width_a; \
  int src_height_a; \
  dst_stride_c = dst_stride_a = 560; \
  src_stride_c = src_stride_a = 560; \
  src_width_c = src_width_a = 640; \
  src_height_c = src_height_a = 80; \
  for (int j = 0; j < 50000; j++) { \
    dst_c[j] = dst_a[j] = rand() % 256; \
    src_c[j] = src_a[j] = rand() % 256; \
  } \
  DyadicBilinearQuarterDownsampler_c (dst_c, dst_stride_c, src_c, src_stride_c, src_width_c, src_height_c); \
  func (dst_a, dst_stride_a, src_a, src_stride_a, src_width_a, src_height_a); \
  for (int j = 0; j < (src_height_c >> 2); j++) { \
    for (int m = 0; m < (src_width_c >> 2); m++) { \
      ASSERT_EQ (dst_c[m + j * dst_stride_c], dst_a[m + j * dst_stride_a]); \
    } \
  } \
}
#define GENERATE_GeneralBilinearDownsampler_UT(func, ref, ASM, CPUFLAGS) \
TEST (DownSampleTest, func) { \
  if (ASM) {\
    int32_t iCpuCores = 0; \
    uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
    if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
    return; \
  } \
  ENFORCE_STACK_ALIGN_1D (uint8_t, dst_c, 70000, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, src_c, 70000, 16); \
  int dst_stride_c; \
  int dst_width_c; \
  int dst_height_c; \
  int src_stride_c; \
  int src_width_c; \
  int src_height_c; \
  ENFORCE_STACK_ALIGN_1D (uint8_t, dst_a, 70000, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, src_a, 70000, 16); \
  int dst_stride_a; \
  int dst_width_a; \
  int dst_height_a; \
  int src_stride_a; \
  int src_width_a; \
  int src_height_a; \
  for (int i = 0; i < 5; i++) { \
    dst_stride_c = dst_stride_a = 320; \
    src_stride_c = src_stride_a = 320; \
    src_width_c = src_width_a = 320; \
    src_height_c = src_height_a = 180; \
    dst_width_c = dst_width_a = (src_width_c >> (i + 1)) + rand() % (src_width_c >> (i + 1)); \
    dst_height_c = dst_height_a = (src_height_c >> (i + 1)) + rand() % (src_height_c >> (i + 1)); \
    for (int j = 0; j < 70000; j++) { \
      dst_c[j] = dst_a[j] = rand() % 256; \
      src_c[j] = src_a[j] = rand() % 256; \
    } \
    ref (dst_c, dst_stride_c, dst_width_c, dst_height_c, src_c, src_stride_c, src_width_c, src_height_c); \
    func (dst_a, dst_stride_a, dst_width_a, dst_height_a, src_a, src_stride_a, src_width_a, src_height_a); \
    for (int j = 0; j < dst_height_c; j++) { \
      for (int m = 0; m < dst_width_c ; m++) { \
        ASSERT_EQ (dst_c[m + j * dst_stride_c], dst_a[m + j * dst_stride_a]); \
      } \
    } \
  } \
}


GENERATE_DyadicBilinearDownsampler_UT (DyadicBilinearDownsampler_c, 0, 0)
GENERATE_GeneralBilinearDownsampler_UT (GeneralBilinearFastDownsampler_c, GeneralBilinearFastDownsampler_ref, 0, 0)
GENERATE_GeneralBilinearDownsampler_UT (GeneralBilinearAccurateDownsampler_c, GeneralBilinearAccurateDownsampler_ref, 0,
                                        0)

#if defined(X86_ASM)
GENERATE_DyadicBilinearDownsampler_UT (DyadicBilinearDownsamplerWidthx32_sse, 1, WELS_CPU_SSE)
GENERATE_DyadicBilinearDownsampler_UT (DyadicBilinearDownsamplerWidthx16_sse, 1, WELS_CPU_SSE)
GENERATE_DyadicBilinearDownsampler_UT (DyadicBilinearDownsamplerWidthx8_sse, 1, WELS_CPU_SSE)

GENERATE_DyadicBilinearDownsampler2_UT (DyadicBilinearDownsamplerWidthx32_ssse3, 1, WELS_CPU_SSSE3)
GENERATE_DyadicBilinearDownsampler2_UT (DyadicBilinearDownsamplerWidthx16_ssse3, 1, WELS_CPU_SSSE3)

GENERATE_DyadicBilinearOneThirdDownsampler_UT (DyadicBilinearOneThirdDownsampler_ssse3, 1, WELS_CPU_SSSE3)
GENERATE_DyadicBilinearOneThirdDownsampler_UT (DyadicBilinearOneThirdDownsampler_sse4, 1, WELS_CPU_SSE41)

GENERATE_DyadicBilinearQuarterDownsampler_UT (DyadicBilinearQuarterDownsampler_sse, 1, WELS_CPU_SSE)
GENERATE_DyadicBilinearQuarterDownsampler_UT (DyadicBilinearQuarterDownsampler_ssse3, 1, WELS_CPU_SSSE3)
GENERATE_DyadicBilinearQuarterDownsampler_UT (DyadicBilinearQuarterDownsampler_sse4, 1, WELS_CPU_SSE41)

GENERATE_GeneralBilinearDownsampler_UT (GeneralBilinearFastDownsamplerWrap_sse2, GeneralBilinearFastDownsampler_ref, 1,
                                        WELS_CPU_SSE2)
GENERATE_GeneralBilinearDownsampler_UT (GeneralBilinearAccurateDownsamplerWrap_sse2,
                                        GeneralBilinearAccurateDownsampler_ref, 1, WELS_CPU_SSE2)
GENERATE_GeneralBilinearDownsampler_UT (GeneralBilinearFastDownsamplerWrap_ssse3, GeneralBilinearFastDownsampler_ref, 1,
                                        WELS_CPU_SSSE3)
GENERATE_GeneralBilinearDownsampler_UT (GeneralBilinearAccurateDownsamplerWrap_sse41,
                                        GeneralBilinearAccurateDownsampler_ref, 1, WELS_CPU_SSE41)
GENERATE_GeneralBilinearDownsampler_UT (GeneralBilinearFastDownsamplerWrap_avx2, GeneralBilinearFastDownsampler_ref, 1,
                                        WELS_CPU_AVX2)
GENERATE_GeneralBilinearDownsampler_UT (GeneralBilinearAccurateDownsamplerWrap_avx2,
                                        GeneralBilinearAccurateDownsampler_ref, 1, WELS_CPU_AVX2)
#endif

#if defined(HAVE_NEON)
GENERATE_DyadicBilinearDownsampler_UT (DyadicBilinearDownsamplerWidthx32_neon, 1, WELS_CPU_NEON)
GENERATE_DyadicBilinearDownsampler_UT (DyadicBilinearDownsampler_neon, 1, WELS_CPU_NEON)

GENERATE_DyadicBilinearOneThirdDownsampler_UT (DyadicBilinearOneThirdDownsampler_neon, 1, WELS_CPU_NEON)

GENERATE_DyadicBilinearQuarterDownsampler_UT (DyadicBilinearQuarterDownsampler_neon, 1, WELS_CPU_NEON)

GENERATE_GeneralBilinearDownsampler_UT (GeneralBilinearAccurateDownsamplerWrap_neon,
                                        GeneralBilinearAccurateDownsampler_ref, 1, WELS_CPU_NEON)
#endif

#if defined(HAVE_NEON_AARCH64)
GENERATE_DyadicBilinearDownsampler_UT (DyadicBilinearDownsamplerWidthx32_AArch64_neon, 1, WELS_CPU_NEON)
GENERATE_DyadicBilinearDownsampler_UT (DyadicBilinearDownsampler_AArch64_neon, 1, WELS_CPU_NEON)

GENERATE_DyadicBilinearOneThirdDownsampler_UT (DyadicBilinearOneThirdDownsampler_AArch64_neon, 1, WELS_CPU_NEON)

GENERATE_DyadicBilinearQuarterDownsampler_UT (DyadicBilinearQuarterDownsampler_AArch64_neon, 1, WELS_CPU_NEON)

GENERATE_GeneralBilinearDownsampler_UT (GeneralBilinearAccurateDownsamplerWrap_AArch64_neon,
                                        GeneralBilinearAccurateDownsampler_ref, 1, WELS_CPU_NEON)
#endif
