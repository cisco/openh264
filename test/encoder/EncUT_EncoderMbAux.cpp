#include <gtest/gtest.h>
#include "cpu.h"
#include "ls_defines.h"
#include "encode_mb_aux.h"
#include "wels_common_basis.h"
#include <algorithm>
#include <cstddef>

using namespace WelsEnc;

#define ALLOC_MEMORY(type, name, num) type* name = (type*)cMemoryAlign.WelsMalloc(num*sizeof(type), #name);
#define FREE_MEMORY(name) cMemoryAlign.WelsFree(name, #name);
TEST (EncodeMbAuxTest, TestScan_4x4_ac_c) {
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, iLevel, 16);
  ALLOC_MEMORY (int16_t, iDctA, 16);
  ALLOC_MEMORY (int16_t, iDctB, 16);
  for (int i = 0; i < 16; i++) {
    iDctA[i] = rand() % 256 + 1;
    iDctB[i] = iDctA[i];
  }
  WelsScan4x4Ac_c (iLevel, iDctA);
  EXPECT_EQ (iLevel[0], iDctB[1]);
  EXPECT_EQ (iLevel[1], iDctB[4]);
  EXPECT_EQ (iLevel[2], iDctB[8]);
  EXPECT_EQ (iLevel[3], iDctB[5]);
  EXPECT_EQ (iLevel[4], iDctB[2]);
  EXPECT_EQ (iLevel[5], iDctB[3]);
  EXPECT_EQ (iLevel[6], iDctB[6]);
  EXPECT_EQ (iLevel[7], iDctB[9]);
  EXPECT_EQ (iLevel[8], iDctB[12]);
  EXPECT_EQ (iLevel[9], iDctB[13]);
  EXPECT_EQ (iLevel[10], iDctB[10]);
  EXPECT_EQ (iLevel[11], iDctB[7]);
  EXPECT_EQ (iLevel[12], iDctB[11]);
  EXPECT_EQ (iLevel[13], iDctB[14]);
  EXPECT_EQ (iLevel[14], iDctB[15]);
  EXPECT_EQ (iLevel[15], 0);
  FREE_MEMORY (iLevel);
  FREE_MEMORY (iDctA);
  FREE_MEMORY (iDctB);
}

#ifdef X86_ASM
TEST (EncodeMbAuxTest, TestScan_4x4_ac_sse2) {
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, iLevelA, 16);
  ALLOC_MEMORY (int16_t, iLevelB, 16);
  ALLOC_MEMORY (int16_t, iDct, 16);
  for (int i = 0; i < 16; i++) {
    iDct[i] = rand() % 256 + 1;
  }
  WelsScan4x4Ac_c (iLevelA, iDct);
  WelsScan4x4Ac_sse2 (iLevelB, iDct);
  for (int j = 0; j < 16; j++)
    EXPECT_EQ (iLevelA[j], iLevelB[j]);
  FREE_MEMORY (iLevelA);
  FREE_MEMORY (iLevelB);
  FREE_MEMORY (iDct);
}
TEST (EncodeMbAuxTest, WelsScan4x4DcAc_sse2) {
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, iLevelA, 32);
  ALLOC_MEMORY (int16_t, iLevelB, 32);
  ALLOC_MEMORY (int16_t, iDct, 32);
  for (int i = 0; i < 32; i++)
    iDct[i] = (rand() & 32767) - 16384;
  WelsScan4x4DcAc_sse2 (iLevelA, iDct);
  WelsScan4x4DcAc_c (iLevelB, iDct);
  for (int i = 0; i < 16; i++)
    EXPECT_EQ (iLevelA[i], iLevelB[i]);
  FREE_MEMORY (iLevelA);
  FREE_MEMORY (iLevelB);
  FREE_MEMORY (iDct);
}
#endif
TEST (EncodeMbAuxTest, TestScan_4x4_dcc) {
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, iLevel, 16);
  ALLOC_MEMORY (int16_t, iDctA, 16);
  ALLOC_MEMORY (int16_t, iDctB, 16);
  for (int i = 0; i < 16; i++)
    iDctA[i] = iDctB[i] = rand() % 256 + 1;
  WelsScan4x4Dc (iLevel, iDctA);
  EXPECT_EQ (iLevel[0], iDctB[0]);
  EXPECT_EQ (iLevel[1], iDctB[1]);
  EXPECT_EQ (iLevel[2], iDctB[4]);
  EXPECT_EQ (iLevel[3], iDctB[8]);
  EXPECT_EQ (iLevel[4], iDctB[5]);
  EXPECT_EQ (iLevel[5], iDctB[2]);
  EXPECT_EQ (iLevel[6], iDctB[3]);
  EXPECT_EQ (iLevel[7], iDctB[6]);
  EXPECT_EQ (iLevel[8], iDctB[9]);
  EXPECT_EQ (iLevel[9], iDctB[12]);
  EXPECT_EQ (iLevel[10], iDctB[13]);
  EXPECT_EQ (iLevel[11], iDctB[10]);
  EXPECT_EQ (iLevel[12], iDctB[7]);
  EXPECT_EQ (iLevel[13], iDctB[11]);
  EXPECT_EQ (iLevel[14], iDctB[14]);
  EXPECT_EQ (iLevel[15], iDctB[15]);
  FREE_MEMORY (iLevel);
  FREE_MEMORY (iDctA);
  FREE_MEMORY (iDctB);
}
static inline void PixelSubWH (int16_t* iDiff, int iSize, uint8_t* pPix1, int iStride1, uint8_t* pPix2, int iStride2) {
  int y, x;
  for (y = 0; y < iSize; y++) {
    for (x = 0; x < iSize; x++)
      iDiff[x + y * iSize] = pPix1[x] - pPix2[x];
    pPix1 += iStride1;
    pPix2 += iStride2;
  }
}

#define FENC_STRIDE 16
#define FDEC_STRIDE 32
static void Sub4x4DctAnchor (int16_t iDct[4][4], uint8_t* pPix1, uint8_t* pPix2) {
  int16_t iDiff[4][4];
  int16_t tmp[4][4];
  int i;
  PixelSubWH ((int16_t*)iDiff, 4, pPix1, FENC_STRIDE, pPix2, FDEC_STRIDE);
  for (i = 0; i < 4; i++) {
    const int a03 = iDiff[i][0] + iDiff[i][3];
    const int a12 = iDiff[i][1] + iDiff[i][2];
    const int s03 = iDiff[i][0] - iDiff[i][3];
    const int s12 = iDiff[i][1] - iDiff[i][2];
    tmp[0][i] =   a03 +   a12;
    tmp[1][i] = 2 * s03 +   s12;
    tmp[2][i] =   a03 -   a12;
    tmp[3][i] =   s03 - 2 * s12;
  }
  for (i = 0; i < 4; i++) {
    const int a03 = tmp[i][0] + tmp[i][3];
    const int a12 = tmp[i][1] + tmp[i][2];
    const int s03 = tmp[i][0] - tmp[i][3];
    const int s12 = tmp[i][1] - tmp[i][2];
    iDct[i][0] =   a03 +   a12;
    iDct[i][1] = 2 * s03 +   s12;
    iDct[i][2] =   a03 -   a12;
    iDct[i][3] =   s03 - 2 * s12;
  }
}

static void Sub8x8DctAnchor (int16_t iDct[4][4][4], uint8_t* pPix1, uint8_t* pPix2) {
  Sub4x4DctAnchor (iDct[0], &pPix1[0], &pPix2[0]);
  Sub4x4DctAnchor (iDct[1], &pPix1[4], &pPix2[4]);
  Sub4x4DctAnchor (iDct[2], &pPix1[4 * FENC_STRIDE + 0], &pPix2[4 * FDEC_STRIDE + 0]);
  Sub4x4DctAnchor (iDct[3], &pPix1[4 * FENC_STRIDE + 4], &pPix2[4 * FDEC_STRIDE + 4]);
}
static void TestDctT4 (PDctFunc func) {
  int16_t iDctRef[4][4];
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (uint8_t, uiPix1, 16 * FENC_STRIDE);
  ALLOC_MEMORY (uint8_t, uiPix2, 16 * FDEC_STRIDE);
  ALLOC_MEMORY (int16_t, iDct, 16);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      uiPix1[i * FENC_STRIDE + j] = rand() & 255;
      uiPix2[i * FDEC_STRIDE + j] = rand() & 255;
    }
  }
  Sub4x4DctAnchor (iDctRef, uiPix1, uiPix2);
  func (iDct, uiPix1, FENC_STRIDE, uiPix2, FDEC_STRIDE);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      EXPECT_EQ (iDctRef[j][i], iDct[i * 4 + j]);
  FREE_MEMORY (uiPix1);
  FREE_MEMORY (uiPix2);
  FREE_MEMORY (iDct);
}
static void TestDctFourT4 (PDctFunc func) {
  int16_t iDctRef[4][4][4];
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (uint8_t, uiPix1, 16 * FENC_STRIDE);
  ALLOC_MEMORY (uint8_t, uiPix2, 16 * FDEC_STRIDE);
  ALLOC_MEMORY (int16_t, iDct, 16 * 4);
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      uiPix1[i * FENC_STRIDE + j] = rand() & 255;
      uiPix2[i * FDEC_STRIDE + j] = rand() & 255;
    }
  }
  Sub8x8DctAnchor (iDctRef, uiPix1, uiPix2);
  func (iDct, uiPix1, FENC_STRIDE, uiPix2, FDEC_STRIDE);
  for (int k = 0; k < 4; k++)
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        EXPECT_EQ (iDctRef[k][j][i], iDct[k * 16 + i * 4 + j]);
  FREE_MEMORY (uiPix1);
  FREE_MEMORY (uiPix2);
  FREE_MEMORY (iDct);
}
TEST (EncodeMbAuxTest, WelsDctT4_c) {
  TestDctT4 (WelsDctT4_c);
}
TEST (EncodeMbAuxTest, WelsDctFourT4_c) {
  TestDctFourT4 (WelsDctFourT4_c);
}

#ifdef X86_ASM
TEST (EncodeMbAuxTest, WelsDctT4_mmx) {
  TestDctT4 (WelsDctT4_mmx);
}

TEST (EncodeMbAuxTest, WelsDctT4_sse2) {
  TestDctT4 (WelsDctT4_sse2);
}

TEST (EncodeMbAuxTest, WelsDctFourT4_sse2) {
  TestDctFourT4 (WelsDctFourT4_sse2);
}

#ifdef HAVE_AVX2
TEST (EncodeMbAuxTest, WelsDctT4_avx2) {
  if (WelsCPUFeatureDetect (0) & WELS_CPU_AVX2)
    TestDctT4 (WelsDctT4_avx2);
}

TEST (EncodeMbAuxTest, WelsDctFourT4_avx2) {
  if (WelsCPUFeatureDetect (0) & WELS_CPU_AVX2)
    TestDctFourT4 (WelsDctFourT4_avx2);
}
#endif //HAVE_AVX2

#ifndef X86_32_PICASM
TEST (EncodeMbAuxTest, WelsCalculateSingleCtr4x4_sse2) {
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, iDctC, 16);
  ALLOC_MEMORY (int16_t, iDctS, 16);
  for (int i = 0; i < 16; i++)
    iDctC[i] = iDctS[i] = (rand() & 65535) - 32768;
  WelsCalculateSingleCtr4x4_c (iDctC);
  WelsCalculateSingleCtr4x4_sse2 (iDctS);
  for (int i = 0; i < 16; i++)
    EXPECT_EQ (iDctC[i], iDctS[i]);
  FREE_MEMORY (iDctC);
  FREE_MEMORY (iDctS);
}
#endif //#ifndef X86_32_PICASM
#endif

void copy (uint8_t* pDst, int32_t iDStride, uint8_t* pSrc, int32_t iSStride, int32_t iWidth, int32_t iHeight) {
  for (int i = 0; i < iHeight; i++)
    memcpy (pDst + i * iDStride, pSrc + i * iSStride, iWidth);
}

#define GENERATE_UT_FOR_COPY(width, height, function) \
TEST(EncodeMbAuxTest, function) { \
  const int iSStride = 64;  \
  const int iDStride = 64;  \
  ENFORCE_STACK_ALIGN_1D (uint8_t, ref_src, iSStride*height, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, ref_dst, iDStride*height, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, dst, iDStride*height, 16); \
  for(int i = 0; i < height; i++) \
    for(int j = 0; j < width; j++) \
      ref_src[i*iSStride+j] = rand() & 255; \
  function(dst, iDStride, ref_src, iSStride); \
  copy(ref_dst, iDStride, ref_src, iSStride, width, height); \
  for(int i = 0; i < height; i++) \
    for(int j = 0; j < width; j++) \
      EXPECT_EQ(ref_dst[i*iDStride+j], dst[i*iDStride+j]); \
}

GENERATE_UT_FOR_COPY (4, 4, WelsCopy4x4_c);
GENERATE_UT_FOR_COPY (8, 4, WelsCopy8x4_c);
GENERATE_UT_FOR_COPY (4, 8, WelsCopy4x8_c);
GENERATE_UT_FOR_COPY (8, 8, WelsCopy8x8_c);
GENERATE_UT_FOR_COPY (8, 16, WelsCopy8x16_c);
GENERATE_UT_FOR_COPY (16, 8, WelsCopy16x8_c);
GENERATE_UT_FOR_COPY (16, 16, WelsCopy16x16_c);
#ifdef X86_ASM
GENERATE_UT_FOR_COPY (16, 8, WelsCopy16x8NotAligned_sse2);
GENERATE_UT_FOR_COPY (16, 16, WelsCopy16x16NotAligned_sse2);
GENERATE_UT_FOR_COPY (16, 16, WelsCopy16x16_sse2);
#endif

namespace {

void TestGetNoneZeroCount (PGetNoneZeroCountFunc func) {
  ENFORCE_STACK_ALIGN_1D (int16_t, pLevel, 16, 16);
  const int num_test_runs = 1000;
  for (int run = 0; run < num_test_runs; run++) {
    const bool all_zero = run == 0;
    const bool all_nonzero = run == 1;
    int result = 0;
    for (int i = 0; i < 16; i++) {
      const int r = rand();
      if (all_zero)
        pLevel[i] = 0;
      else if (all_nonzero)
        pLevel[i] = r % 0xFFFF - 0x8000 ? r % 0xFFFF - 0x8000 : 0x7FFF;
      else
        pLevel[i] = (r >> 16 & 1) * ((r & 0xFFFF) - 0x8000);
      result += pLevel[i] != 0;
    }
    const int32_t nnz = func (pLevel);
    EXPECT_EQ (nnz, result);
  }
}

} // anon ns.

TEST (EncodeMbAuxTest, WelsGetNoneZeroCount_c) {
  TestGetNoneZeroCount (WelsGetNoneZeroCount_c);
}
#ifdef X86_ASM
#ifndef X86_32_PICASM
TEST (EncodeMbAuxTest, WelsGetNoneZeroCount_sse2) {
  TestGetNoneZeroCount (WelsGetNoneZeroCount_sse2);
}
#endif
TEST (EncodeMbAuxTest, WelsGetNoneZeroCount_sse42) {
  if (WelsCPUFeatureDetect (0) & WELS_CPU_SSE42)
    TestGetNoneZeroCount (WelsGetNoneZeroCount_sse42);
}
#endif
#define WELS_ABS_LC(a) ((sign ^ (int32_t)(a)) - sign)
#define NEW_QUANT(pDct, ff, mf) (((ff)+ WELS_ABS_LC(pDct))*(mf)) >>16
#define WELS_NEW_QUANT(pDct,ff,mf) WELS_ABS_LC(NEW_QUANT(pDct, ff, mf))
namespace {
int16_t WelsQuant4x4MaxAnchor (int16_t* pDct, int16_t* ff, int16_t* mf) {
  int16_t max_abs = 0;
  for (int i = 0; i < 16; i++) {
    const int j = i & 0x07;
    const int32_t sign = WELS_SIGN (pDct[i]);
    pDct[i] = NEW_QUANT (pDct[i], ff[j], mf[j]);
    max_abs = std::max(max_abs, pDct[i]);
    pDct[i] = WELS_ABS_LC (pDct[i]);
  }
  return max_abs;
}
void WelsQuant4x4DcAnchor (int16_t* pDct, int16_t iFF, int16_t iMF) {
  for (int i = 0; i < 16; i++) {
    const int32_t sign = WELS_SIGN (pDct[i]);
    pDct[i] = WELS_NEW_QUANT (pDct[i], iFF, iMF);
  }
}
void WelsQuantFour4x4Anchor (int16_t* pDct, int16_t* ff,  int16_t* mf) {
  for (int i = 0; i < 4; i++)
    WelsQuant4x4MaxAnchor (pDct + 16 * i, ff, mf);
}
void WelsQuantFour4x4MaxAnchor (int16_t* pDct, int16_t* ff,  int16_t* mf, int16_t* max) {
  for (int i = 0; i < 4; i++)
    max[i] = WelsQuant4x4MaxAnchor (pDct + 16 * i, ff, mf);
}
void TestWelsQuant4x4 (PQuantizationFunc func) {
  const std::size_t f_size = 8;
  const std::size_t dct_size = 16;
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, ff, f_size);
  ALLOC_MEMORY (int16_t, mf, f_size);
  ALLOC_MEMORY (int16_t, iDctC, dct_size);
  ALLOC_MEMORY (int16_t, iDctS, dct_size);
  for (std::size_t i = 0; i < f_size; i++) {
    ff[i] = rand() & 32767;
    mf[i] = rand() & 32767;
  }
  for (std::size_t i = 0; i < dct_size; i++)
    iDctC[i] = iDctS[i] = (rand() & 65535) - 32768;
  WelsQuant4x4MaxAnchor (iDctC, ff, mf);
  func (iDctS, ff, mf);
  for (std::size_t i = 0; i < dct_size; i++)
    EXPECT_EQ (iDctC[i], iDctS[i]);
  FREE_MEMORY (ff);
  FREE_MEMORY (mf);
  FREE_MEMORY (iDctC);
  FREE_MEMORY (iDctS);
}
void TestWelsQuant4x4Dc (PQuantizationDcFunc func) {
  const std::size_t dct_size = 16;
  const int16_t ff = rand() & 32767;
  const int16_t mf = rand() & 32767;
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, iDctC, dct_size);
  ALLOC_MEMORY (int16_t, iDctS, dct_size);
  for (std::size_t i = 0; i < dct_size; i++)
    iDctC[i] = iDctS[i] = (rand() & 65535) - 32768;
  WelsQuant4x4DcAnchor (iDctC, ff, mf);
  func (iDctS, ff, mf);
  for (std::size_t i = 0; i < dct_size; i++)
    EXPECT_EQ (iDctC[i], iDctS[i]);
  FREE_MEMORY (iDctC);
  FREE_MEMORY (iDctS);
}
void TestWelsQuantFour4x4 (PQuantizationFunc func) {
  const std::size_t f_size = 8;
  const std::size_t dct_size = 4 * 16;
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, ff, f_size);
  ALLOC_MEMORY (int16_t, mf, f_size);
  ALLOC_MEMORY (int16_t, iDctC, dct_size);
  ALLOC_MEMORY (int16_t, iDctS, dct_size);
  for (std::size_t i = 0; i < f_size; i++) {
    ff[i] = rand() & 32767;
    mf[i] = rand() & 32767;
  }
  for (std::size_t i = 0; i < dct_size; i++)
    iDctC[i] = iDctS[i] = (rand() & 65535) - 32768;
  WelsQuantFour4x4Anchor (iDctC, ff, mf);
  func (iDctS, ff, mf);
  for (std::size_t i = 0; i < dct_size; i++)
    EXPECT_EQ (iDctC[i], iDctS[i]);
  FREE_MEMORY (ff);
  FREE_MEMORY (mf);
  FREE_MEMORY (iDctC);
  FREE_MEMORY (iDctS);
}
void TestWelsQuantFour4x4Max (PQuantizationMaxFunc func) {
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, ff, 8);
  ALLOC_MEMORY (int16_t, mf, 8);
  ALLOC_MEMORY (int16_t, iDctC, 64);
  ALLOC_MEMORY (int16_t, iDctS, 64);
  ALLOC_MEMORY (int16_t, iMaxC, 16);
  ALLOC_MEMORY (int16_t, iMaxS, 16);
  for (int i = 0; i < 8; i++) {
    ff[i] = rand() & 32767;
    mf[i] = rand() & 32767;
  }
  for (int i = 0; i < 64; i++)
    iDctC[i] = iDctS[i] = (rand() & 65535) - 32767;
  WelsQuantFour4x4MaxAnchor (iDctC, ff, mf, iMaxC);
  func (iDctS, ff, mf, iMaxS);
  for (int i = 0; i < 64; i++)
    EXPECT_EQ (iDctC[i], iDctS[i]);
  for (int i = 0; i < 4; i++)
    EXPECT_EQ (iMaxC[i], iMaxS[i]);
  FREE_MEMORY (ff);
  FREE_MEMORY (mf);
  FREE_MEMORY (iDctC);
  FREE_MEMORY (iDctS);
  FREE_MEMORY (iMaxC);
  FREE_MEMORY (iMaxS);
}
} // anon ns
TEST (EncodeMbAuxTest, WelsQuant4x4_c) {
  TestWelsQuant4x4 (WelsQuant4x4_c);
}
TEST (EncodeMbAuxTest, WelsQuant4x4Dc_c) {
  TestWelsQuant4x4Dc (WelsQuant4x4Dc_c);
}
TEST (EncodeMbAuxTest, WelsQuantFour4x4_c) {
  TestWelsQuantFour4x4 (WelsQuantFour4x4_c);
}
TEST (EncodeMbAuxTest, WelsQuantFour4x4Max_c) {
  TestWelsQuantFour4x4Max (WelsQuantFour4x4Max_c);
}
#ifdef X86_ASM
TEST (EncodeMbAuxTest, WelsQuant4x4_sse2) {
  TestWelsQuant4x4 (WelsQuant4x4_sse2);
}
TEST (EncodeMbAuxTest, WelsQuant4x4Dc_sse2) {
  TestWelsQuant4x4Dc (WelsQuant4x4Dc_sse2);
}
TEST (EncodeMbAuxTest, WelsQuantFour4x4_sse2) {
  TestWelsQuantFour4x4 (WelsQuantFour4x4_sse2);
}
TEST (EncodeMbAuxTest, WelsQuantFour4x4Max_sse2) {
  TestWelsQuantFour4x4Max (WelsQuantFour4x4Max_sse2);
}
#ifdef HAVE_AVX2
TEST (EncodeMbAuxTest, WelsQuant4x4_avx2) {
  if (WelsCPUFeatureDetect (0) & WELS_CPU_AVX2)
    TestWelsQuant4x4 (WelsQuant4x4_avx2);
}
TEST (EncodeMbAuxTest, WelsQuant4x4Dc_avx2) {
  if (WelsCPUFeatureDetect (0) & WELS_CPU_AVX2)
    TestWelsQuant4x4Dc (WelsQuant4x4Dc_avx2);
}
TEST (EncodeMbAuxTest, WelsQuantFour4x4_avx2) {
  if (WelsCPUFeatureDetect (0) & WELS_CPU_AVX2)
    TestWelsQuantFour4x4 (WelsQuantFour4x4_avx2);
}
TEST (EncodeMbAuxTest, WelsQuantFour4x4Max_avx2) {
  if (WelsCPUFeatureDetect (0) & WELS_CPU_AVX2)
    TestWelsQuantFour4x4Max (WelsQuantFour4x4Max_avx2);
}
#endif //HAVE_AVX2
#endif
int32_t WelsHadamardQuant2x2SkipAnchor (int16_t* rs, int16_t ff,  int16_t mf) {
  int16_t pDct[4], s[4];
  int16_t threshold = ((1 << 16) - 1) / mf - ff;
  s[0] = rs[0]  + rs[32];
  s[1] = rs[0]  - rs[32];
  s[2] = rs[16] + rs[48];
  s[3] = rs[16] - rs[48];
  pDct[0] = s[0] + s[2];
  pDct[1] = s[0] - s[2];
  pDct[2] = s[1] + s[3];
  pDct[3] = s[1] - s[3];
  return ((WELS_ABS (pDct[0]) > threshold) || (WELS_ABS (pDct[1]) > threshold) || (WELS_ABS (pDct[2]) > threshold)
          || (WELS_ABS (pDct[3]) > threshold));
}

TEST (EncodeMbAuxTest, WelsHadamardQuant2x2Skip_c) {
  int16_t iRS[64];
  int16_t ff, mf;
  for (int i = 0; i < 64; i++)
    iRS[i] = (rand() & 32767) - 16384;
  ff = rand() & 32767;
  mf = rand() & 32767;
  EXPECT_EQ (WelsHadamardQuant2x2Skip_c (iRS, ff, mf), WelsHadamardQuant2x2SkipAnchor (iRS, ff, mf));
}

int32_t WelsHadamardQuant2x2Anchor (int16_t* rs, const int16_t ff, int16_t mf, int16_t* pDct, int16_t* block) {
  int16_t s[4];
  int32_t sign, i, dc_nzc = 0;

  s[0] = rs[0]  + rs[32];
  s[1] = rs[0]  - rs[32];
  s[2] = rs[16] + rs[48];
  s[3] = rs[16] - rs[48];

  rs[0] = 0;
  rs[16] = 0;
  rs[32] = 0;
  rs[48] = 0;

  pDct[0] = s[0] + s[2];
  pDct[1] = s[0] - s[2];
  pDct[2] = s[1] + s[3];
  pDct[3] = s[1] - s[3];

  sign = WELS_SIGN (pDct[0]);
  pDct[0] = WELS_NEW_QUANT (pDct[0], ff, mf);
  sign = WELS_SIGN (pDct[1]);
  pDct[1] = WELS_NEW_QUANT (pDct[1], ff, mf);
  sign = WELS_SIGN (pDct[2]);
  pDct[2] = WELS_NEW_QUANT (pDct[2], ff, mf);
  sign = WELS_SIGN (pDct[3]);
  pDct[3] = WELS_NEW_QUANT (pDct[3], ff, mf);
  ST64 (block, LD64 (pDct));
  for (i = 0; i < 4; i++)
    dc_nzc += (block[i] != 0);
  return dc_nzc;
}

TEST (EncodeMbAuxTest, WelsHadamardQuant2x2_c) {
  int16_t iRsC[64], iRsA[64];
  int16_t ff, mf;
  int16_t iBlockA[16], iBlockC[16], iDctA[4], iDctC[4];
  for (int i = 0; i < 64; i++)
    iRsA[i] = iRsC[i] = (rand() & 32767) - 16384;
  for (int i = 0; i < 4; i++)
    iDctA[i] = iDctC[i] = (rand() & 32767) - 16384;
  ff = rand() & 32767;
  mf = rand() & 32767;

  int32_t iRetA = WelsHadamardQuant2x2Anchor (iRsA, ff, mf, iDctA, iBlockA);
  int32_t iRetC = WelsHadamardQuant2x2_c (iRsC, ff, mf, iDctC,    iBlockC);
  EXPECT_EQ (iRetA, iRetC);
  for (int i = 0; i < 4; i++)
    EXPECT_EQ (iDctA[i], iDctC[i]);
}

void WelsHadamardT4DcAnchor (int16_t* pLumaDc, int16_t* pDct) {
  int32_t p[16], s[4];
  int32_t i, iIdx;
  for (i = 0 ; i < 16 ; i += 4) {
    iIdx = ((i & 0x08) << 4) + ((i & 0x04) << 3);
    s[0] = pDct[iIdx ]     + pDct[iIdx + 80];
    s[3] = pDct[iIdx ]     - pDct[iIdx + 80];
    s[1] = pDct[iIdx + 16] + pDct[iIdx + 64];
    s[2] = pDct[iIdx + 16] - pDct[iIdx + 64];
    p[i  ] = s[0] + s[1];
    p[i + 2] = s[0] - s[1];
    p[i + 1] = s[3] + s[2];
    p[i + 3] = s[3] - s[2];
  }
  for (i = 0 ; i < 4 ; i ++) {
    s[0] = p[i ]  + p[i + 12];
    s[3] = p[i ]  - p[i + 12];
    s[1] = p[i + 4] + p[i + 8];
    s[2] = p[i + 4] - p[i + 8];
    pLumaDc[i  ]  = WELS_CLIP3 ((s[0] + s[1] + 1) >> 1, -32768, 32767);
    pLumaDc[i + 8 ] = WELS_CLIP3 ((s[0] - s[1] + 1) >> 1, -32768, 32767);
    pLumaDc[i + 4 ] = WELS_CLIP3 ((s[3] + s[2] + 1) >> 1, -32768, 32767);
    pLumaDc[i + 12] = WELS_CLIP3 ((s[3] - s[2] + 1) >> 1, -32768, 32767);
  }
}
TEST (EncodeMbAuxTest, WelsHadamardT4Dc_c) {
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, iDct, 128 * 16);
  ALLOC_MEMORY (int16_t, iLumaDcR, 16);
  ALLOC_MEMORY (int16_t, iLumaDcC, 16);
  for (int i = 0; i < 128 * 16; i++)
    iDct[i] = (rand() & 32767) - 16384;
  WelsHadamardT4DcAnchor (iLumaDcR, iDct);
  WelsHadamardT4Dc_c (iLumaDcC, iDct);
  for (int i = 0; i < 16; i++)
    EXPECT_EQ (iLumaDcR[i], iLumaDcC[i]);
  FREE_MEMORY (iDct);
  FREE_MEMORY (iLumaDcR);
  FREE_MEMORY (iLumaDcC);
}
#ifdef X86_ASM
TEST (EncodeMbAuxTest, WelsHadamardT4Dc_sse2) {
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (int16_t, iDct, 128 * 16);
  ALLOC_MEMORY (int16_t, iLumaDcC, 16);
  ALLOC_MEMORY (int16_t, iLumaDcS, 16);
  for (int i = 0; i < 128 * 16; i++)
    iDct[i] = (rand() & 32767) - 16384;
  WelsHadamardT4Dc_c (iLumaDcC, iDct);
  WelsHadamardT4Dc_sse2 (iLumaDcS, iDct);
  for (int i = 0; i < 16; i++)
    EXPECT_EQ (iLumaDcC[i], iLumaDcS[i]);
  FREE_MEMORY (iDct);
  FREE_MEMORY (iLumaDcC);
  FREE_MEMORY (iLumaDcS);
}
#endif
