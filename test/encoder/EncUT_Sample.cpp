#include <gtest/gtest.h>
#include <math.h>

#include "cpu_core.h"
#include "cpu.h"
#include "sample.h"
#include "sad_common.h"
#include "get_intra_predictor.h"

using namespace WelsEnc;

#define GENERATE_Intra16x16_UT(func, ref, ASM, CPUFLAGS) \
TEST (IntraSadSatdFuncTest, func) { \
  const int32_t iLineSizeDec = 32; \
  const int32_t iLineSizeEnc = 32; \
  int32_t tmpa, tmpb; \
  int32_t iBestMode_c, iBestMode_a, iLambda = 50; \
  if (ASM) {\
    int32_t iCpuCores = 0; \
    uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
    if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
      return; \
  } \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pDec, iLineSizeDec << 5, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pEnc, iLineSizeEnc << 5, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pDst, 512, 16); \
  for (int i = 0; i < (iLineSizeDec << 5); i++) \
    pDec[i] = rand() % 256; \
  for (int i = 0; i < (iLineSizeEnc << 5); i++) \
    pEnc[i] = rand() % 256; \
  for (int i = 0; i < 512; i++) \
    pDst[i] = rand() % 256; \
  tmpa = ref (pDec + 128, iLineSizeDec, pEnc, iLineSizeEnc, &iBestMode_c, iLambda, pDst); \
  tmpb = func (pDec + 128, iLineSizeDec, pEnc, iLineSizeEnc, &iBestMode_a, iLambda, pDst); \
  ASSERT_EQ (tmpa, tmpb); \
  ASSERT_EQ (iBestMode_c, iBestMode_a); \
}

#define GENERATE_Intra4x4_UT(func, ASM, CPUFLAGS) \
TEST (IntraSadSatdFuncTest, func) { \
  const int32_t iLineSizeDec = 32; \
  const int32_t iLineSizeEnc = 32; \
  int32_t tmpa, tmpb; \
  int32_t iBestMode_c, iBestMode_a, iLambda = 50; \
  int32_t lambda[2] = {iLambda << 2, iLambda}; \
  int32_t iPredMode = rand() % 3; \
  if (ASM) {\
    int32_t iCpuCores = 0; \
    uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
    if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
     return; \
  } \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pDec, iLineSizeDec << 5, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pEnc, iLineSizeEnc << 5, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pDst, 512, 16); \
  for (int i = 0; i < (iLineSizeDec << 5); i++) \
    pDec[i] = rand() % 256; \
  for (int i = 0; i < (iLineSizeEnc << 5); i++) \
    pEnc[i] = rand() % 256; \
  for (int i = 0; i < 512; i++) \
    pDst[i] = rand() % 256; \
  tmpa = WelsSampleSatdIntra4x4Combined3_c (pDec + 128, iLineSizeDec, pEnc, iLineSizeEnc, pDst, &iBestMode_c, \
         lambda[iPredMode == 2], lambda[iPredMode == 1], lambda[iPredMode == 0]); \
  tmpb = func (pDec + 128, iLineSizeDec, pEnc, iLineSizeEnc, pDst, &iBestMode_a, \
                                      lambda[iPredMode == 2], lambda[iPredMode == 1], lambda[iPredMode == 0]); \
  ASSERT_EQ (tmpa, tmpb); \
  ASSERT_EQ (iBestMode_c, iBestMode_a); \
}

#define GENERATE_Intra8x8_UT(func, ref, ASM, CPUFLAGS) \
TEST (IntraSadSatdFuncTest, func) { \
  const int32_t iLineSizeDec = 32; \
  const int32_t iLineSizeEnc = 32; \
  int32_t tmpa, tmpb; \
  int32_t iBestMode_c, iBestMode_a, iLambda = 50; \
  if (ASM) {\
    int32_t iCpuCores = 0; \
    uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores); \
    if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
        return; \
  } \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pDecCb, iLineSizeDec << 5, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pEncCb, iLineSizeEnc << 5, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pDecCr, iLineSizeDec << 5, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pEncCr, iLineSizeEnc << 5, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pDstChma, 512, 16); \
  for (int i = 0; i < (iLineSizeDec << 5); i++) { \
    pDecCb[i] = rand() % 256; \
    pDecCr[i] = rand() % 256; \
  } \
  for (int i = 0; i < (iLineSizeEnc << 5); i++) { \
    pEncCb[i] = rand() % 256; \
    pEncCr[i] = rand() % 256; \
  } \
  for (int i = 0; i < 512; i++) \
    pDstChma[i] = rand() % 256; \
  tmpa = ref (pDecCb + 128, iLineSizeDec, pEncCb, iLineSizeEnc, &iBestMode_c, iLambda, \
         pDstChma, pDecCr + 128, pEncCr); \
  tmpb = func (pDecCb + 128, iLineSizeDec, pEncCb, iLineSizeEnc, &iBestMode_a, iLambda, \
         pDstChma, pDecCr + 128, pEncCr); \
  ASSERT_EQ (tmpa, tmpb); \
  ASSERT_EQ (iBestMode_c, iBestMode_a); \
}

#ifdef X86_ASM
GENERATE_Intra16x16_UT (WelsIntra16x16Combined3Sad_ssse3, WelsSampleSadIntra16x16Combined3_c, 1, WELS_CPU_SSSE3)
GENERATE_Intra16x16_UT (WelsIntra16x16Combined3Satd_sse41, WelsSampleSatdIntra16x16Combined3_c, 1, WELS_CPU_SSE41)
GENERATE_Intra8x8_UT (WelsIntraChroma8x8Combined3Satd_sse41, WelsSampleSatdIntra8x8Combined3_c, 1, WELS_CPU_SSE41)
GENERATE_Intra4x4_UT (WelsSampleSatdThree4x4_sse2, 1, WELS_CPU_SSE2)
#endif

#ifdef HAVE_NEON
GENERATE_Intra16x16_UT (WelsIntra16x16Combined3Sad_neon, WelsSampleSadIntra16x16Combined3_c, 1, WELS_CPU_NEON)
GENERATE_Intra16x16_UT (WelsIntra16x16Combined3Satd_neon, WelsSampleSatdIntra16x16Combined3_c, 1, WELS_CPU_NEON)
GENERATE_Intra8x8_UT (WelsIntra8x8Combined3Satd_neon, WelsSampleSatdIntra8x8Combined3_c, 1, WELS_CPU_NEON)
GENERATE_Intra8x8_UT (WelsIntra8x8Combined3Sad_neon, WelsSampleSadIntra8x8Combined3_c, 1, WELS_CPU_NEON)
GENERATE_Intra4x4_UT (WelsIntra4x4Combined3Satd_neon, 1, WELS_CPU_NEON)
#endif

#ifdef HAVE_NEON_AARCH64
GENERATE_Intra16x16_UT (WelsIntra16x16Combined3Sad_AArch64_neon, WelsSampleSadIntra16x16Combined3_c, 1, WELS_CPU_NEON)
GENERATE_Intra16x16_UT (WelsIntra16x16Combined3Satd_AArch64_neon, WelsSampleSatdIntra16x16Combined3_c, 1, WELS_CPU_NEON)
GENERATE_Intra8x8_UT (WelsIntra8x8Combined3Satd_AArch64_neon, WelsSampleSatdIntra8x8Combined3_c, 1, WELS_CPU_NEON)
GENERATE_Intra8x8_UT (WelsIntra8x8Combined3Sad_AArch64_neon, WelsSampleSadIntra8x8Combined3_c, 1, WELS_CPU_NEON)
GENERATE_Intra4x4_UT (WelsIntra4x4Combined3Satd_AArch64_neon, 1, WELS_CPU_NEON)
#endif

#define ASSERT_MEMORY_FAIL2X(A, B)     \
  if (NULL == B) {                     \
    pMemAlign->WelsFree(A, "Sad_SrcA");\
    ASSERT_TRUE(0);                    \
  }

#define ASSERT_MEMORY_FAIL3X(A, B, C)   \
  if (NULL == C) {                      \
    pMemAlign->WelsFree(A, "Sad_SrcA"); \
    pMemAlign->WelsFree(B, "Sad_SrcB"); \
    ASSERT_TRUE(0);                     \
  }

#define PIXEL_STRIDE 32

class SadSatdCFuncTest : public testing::Test {
 public:
  virtual void SetUp() {
    pMemAlign = new CMemoryAlign (0);

    m_iStrideA = rand() % 256 + PIXEL_STRIDE;
    m_iStrideB = rand() % 256 + PIXEL_STRIDE;
    m_pPixSrcA = (uint8_t*)pMemAlign->WelsMalloc (m_iStrideA << 5, "Sad_m_pPixSrcA");
    ASSERT_TRUE (NULL != m_pPixSrcA);
    m_pPixSrcB = (uint8_t*)pMemAlign->WelsMalloc (m_iStrideB << 5, "Sad_m_pPixSrcB");
    ASSERT_MEMORY_FAIL2X (m_pPixSrcA, m_pPixSrcB)
    m_pSad = (int32_t*)pMemAlign->WelsMalloc (4 * sizeof (int32_t), "m_pSad");
    ASSERT_MEMORY_FAIL3X (m_pPixSrcA, m_pPixSrcB, m_pSad)
  }
  virtual void TearDown() {
    pMemAlign->WelsFree (m_pPixSrcA, "Sad_m_pPixSrcA");
    pMemAlign->WelsFree (m_pPixSrcB, "Sad_m_pPixSrcB");
    pMemAlign->WelsFree (m_pSad, "m_pSad");
    delete pMemAlign;
  }
 public:
  uint8_t* m_pPixSrcA;
  uint8_t* m_pPixSrcB;
  int32_t m_iStrideA;
  int32_t m_iStrideB;
  int32_t* m_pSad;

  CMemoryAlign* pMemAlign;
};

TEST_F (SadSatdCFuncTest, WelsSampleSad4x4_c) {
  for (int i = 0; i < (m_iStrideA << 2); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 2); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++)
      iSumSad += abs (pPixA[j] - pPixB[j]);
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  EXPECT_EQ (WelsSampleSad4x4_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSad8x4_c) {
  for (int i = 0; i < (m_iStrideA << 2); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 2); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 8; j++)
      iSumSad += abs (pPixA[j] - pPixB[j]);
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  EXPECT_EQ (WelsSampleSad8x4_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSad4x8_c) {
  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 4; j++)
      iSumSad += abs (pPixA[j] - pPixB[j]);
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  EXPECT_EQ (WelsSampleSad4x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSad8x8_c) {
  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++)
      iSumSad += abs (pPixA[j] - pPixB[j]);

    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  EXPECT_EQ (WelsSampleSad8x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSad16x8_c) {
  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 16; j++)
      iSumSad += abs (pPixA[j] - pPixB[j]);

    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  EXPECT_EQ (WelsSampleSad16x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSad8x16_c) {
  for (int i = 0; i < (m_iStrideA << 4); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 4); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 8; j++)
      iSumSad += abs (pPixA[j] - pPixB[j]);

    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  EXPECT_EQ (WelsSampleSad8x16_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSad16x16_c) {
  for (int i = 0; i < (m_iStrideA << 4); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 4); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++)
      iSumSad += abs (pPixA[j] - pPixB[j]);

    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  EXPECT_EQ (WelsSampleSad16x16_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSatd4x4_c) {
  for (int i = 0; i < (m_iStrideA << 2); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 2); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB;

  int32_t W[16], T[16], Y[16], k = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++)
      W[k++] = pPixA[j] - pPixB[j];
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }

  T[0] = W[0] + W[4] + W[8] + W[12];
  T[1] = W[1] + W[5] + W[9] + W[13];
  T[2] = W[2] + W[6] + W[10] + W[14];
  T[3] = W[3] + W[7] + W[11] + W[15];

  T[4] = W[0] + W[4] - W[8] - W[12];
  T[5] = W[1] + W[5] - W[9] - W[13];
  T[6] = W[2] + W[6] - W[10] - W[14];
  T[7] = W[3] + W[7] - W[11] - W[15];

  T[8] = W[0] - W[4] - W[8] + W[12];
  T[9] = W[1] - W[5] - W[9] + W[13];
  T[10] = W[2] - W[6] - W[10] + W[14];
  T[11] = W[3] - W[7] - W[11] + W[15];

  T[12] = W[0] - W[4] + W[8] - W[12];
  T[13] = W[1] - W[5] + W[9] - W[13];
  T[14] = W[2] - W[6] + W[10] - W[14];
  T[15] = W[3] - W[7] + W[11] - W[15];

  Y[0] = T[0] + T[1] + T[2] + T[3];
  Y[1] = T[0] + T[1] - T[2] - T[3];
  Y[2] = T[0] - T[1] - T[2] + T[3];
  Y[3] = T[0] - T[1] + T[2] - T[3];

  Y[4] = T[4] + T[5] + T[6] + T[7];
  Y[5] = T[4] + T[5] - T[6] - T[7];
  Y[6] = T[4] - T[5] - T[6] + T[7];
  Y[7] = T[4] - T[5] + T[6] - T[7];

  Y[8] = T[8] + T[9] + T[10] + T[11];
  Y[9] = T[8] + T[9] - T[10] - T[11];
  Y[10] = T[8] - T[9] - T[10] + T[11];
  Y[11] = T[8] - T[9] + T[10] - T[11];

  Y[12] = T[12] + T[13] + T[14] + T[15];
  Y[13] = T[12] + T[13] - T[14] - T[15];
  Y[14] = T[12] - T[13] - T[14] + T[15];
  Y[15] = T[12] - T[13] + T[14] - T[15];

  int32_t iSumSatd = 0;
  for (int i = 0; i < 16; i++)
    iSumSatd += abs (Y[i]);

  EXPECT_EQ (WelsSampleSatd4x4_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), (iSumSatd + 1) >> 1);
}

TEST_F (SadSatdCFuncTest, WelsSampleSadFour16x16_c) {
  for (int i = 0; i < (m_iStrideA << 5); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 5); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB + m_iStrideB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      iSumSad += abs (pPixA[j] - pPixB[j - 1]);
      iSumSad += abs (pPixA[j] - pPixB[j + 1]);
      iSumSad += abs (pPixA[j] - pPixB[j - m_iStrideB]);
      iSumSad += abs (pPixA[j] - pPixB[j + m_iStrideB]);
    }
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  WelsSampleSadFour16x16_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSadFour16x8_c) {
  for (int i = 0; i < (m_iStrideA << 5); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 5); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB + m_iStrideB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 16; j++) {
      iSumSad += abs (pPixA[j] - pPixB[j - 1]);
      iSumSad += abs (pPixA[j] - pPixB[j + 1]);
      iSumSad += abs (pPixA[j] - pPixB[j - m_iStrideB]);
      iSumSad += abs (pPixA[j] - pPixB[j + m_iStrideB]);
    }
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }

  WelsSampleSadFour16x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSadFour8x16_c) {
  for (int i = 0; i < (m_iStrideA << 5); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 5); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB + m_iStrideB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 8; j++) {
      iSumSad += abs (pPixA[j] - pPixB[j - 1]);
      iSumSad += abs (pPixA[j] - pPixB[j + 1]);
      iSumSad += abs (pPixA[j] - pPixB[j - m_iStrideB]);
      iSumSad += abs (pPixA[j] - pPixB[j + m_iStrideB]);
    }
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }

  WelsSampleSadFour8x16_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSadFour8x8_c) {
  for (int i = 0; i < (m_iStrideA << 4); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 4); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB + m_iStrideB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      iSumSad += abs (pPixA[j] - pPixB[j - 1]);
      iSumSad += abs (pPixA[j] - pPixB[j + 1]);
      iSumSad += abs (pPixA[j] - pPixB[j - m_iStrideB]);
      iSumSad += abs (pPixA[j] - pPixB[j + m_iStrideB]);
    }
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  WelsSampleSadFour8x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSadFour4x4_c) {
  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB + m_iStrideB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      iSumSad += abs (pPixA[j] - pPixB[j - 1]);
      iSumSad += abs (pPixA[j] - pPixB[j + 1]);
      iSumSad += abs (pPixA[j] - pPixB[j - m_iStrideB]);
      iSumSad += abs (pPixA[j] - pPixB[j + m_iStrideB]);
    }
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  WelsSampleSadFour4x4_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSadFour8x4_c) {
  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB + m_iStrideB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 8; j++) {
      iSumSad += abs (pPixA[j] - pPixB[j - 1]);
      iSumSad += abs (pPixA[j] - pPixB[j + 1]);
      iSumSad += abs (pPixA[j] - pPixB[j - m_iStrideB]);
      iSumSad += abs (pPixA[j] - pPixB[j + m_iStrideB]);
    }
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  WelsSampleSadFour8x4_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}

TEST_F (SadSatdCFuncTest, WelsSampleSadFour4x8_c) {
  for (int i = 0; i < (m_iStrideA << 4); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 4); i++)
    m_pPixSrcB[i] = rand() % 256;
  uint8_t* pPixA = m_pPixSrcA;
  uint8_t* pPixB = m_pPixSrcB + m_iStrideB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 4; j++) {
      iSumSad += abs (pPixA[j] - pPixB[j - 1]);
      iSumSad += abs (pPixA[j] - pPixB[j + 1]);
      iSumSad += abs (pPixA[j] - pPixB[j - m_iStrideB]);
      iSumSad += abs (pPixA[j] - pPixB[j + m_iStrideB]);
    }
    pPixA += m_iStrideA;
    pPixB += m_iStrideB;
  }
  WelsSampleSadFour4x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}

class SadSatdAssemblyFuncTest : public testing::Test {
 public:
  virtual void SetUp() {
    int32_t iCpuCores = 0;
    m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
    pMemAlign = new CMemoryAlign (16);
    m_iStrideA = m_iStrideB = PIXEL_STRIDE;
    m_pPixSrcA = (uint8_t*)pMemAlign->WelsMalloc (m_iStrideA << 5, "Sad_m_pPixSrcA");
    ASSERT_TRUE (NULL != m_pPixSrcA);
    m_pPixSrcB = (uint8_t*)pMemAlign->WelsMalloc (m_iStrideB << 5, "Sad_m_pPixSrcB");
    ASSERT_MEMORY_FAIL2X (m_pPixSrcA, m_pPixSrcB)
    m_pSad = (int32_t*)pMemAlign->WelsMalloc (4 * sizeof (int32_t), "m_pSad");
    ASSERT_MEMORY_FAIL3X (m_pPixSrcA, m_pPixSrcB, m_pSad)
  }
  virtual void TearDown() {
    pMemAlign->WelsFree (m_pPixSrcA, "Sad_m_pPixSrcA");
    pMemAlign->WelsFree (m_pPixSrcB, "Sad_m_pPixSrcB");
    pMemAlign->WelsFree (m_pSad, "m_pSad");
    delete pMemAlign;
  }
 public:
  uint32_t m_uiCpuFeatureFlag;
  uint8_t* m_pPixSrcA;
  uint8_t* m_pPixSrcB;
  int32_t m_iStrideA;
  int32_t m_iStrideB;
  int32_t* m_pSad;

  CMemoryAlign* pMemAlign;
};

#define GENERATE_Sad4x4_UT(func, ref, CPUFLAGS) \
TEST_F (SadSatdAssemblyFuncTest, func) { \
  if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
    return; \
  for (int i = 0; i < (m_iStrideA << 2); i++) \
    m_pPixSrcA[i] = rand() % 256; \
  for (int i = 0; i < (m_iStrideB << 2); i++) \
    m_pPixSrcB[i] = rand() % 256; \
  EXPECT_EQ (ref (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), func (m_pPixSrcA, \
             m_iStrideA, m_pPixSrcB, m_iStrideB)); \
}

#define GENERATE_Sad8x8_UT(func, ref, CPUFLAGS) \
TEST_F (SadSatdAssemblyFuncTest, func) { \
  if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
    return; \
  for (int i = 0; i < (m_iStrideA << 3); i++) \
    m_pPixSrcA[i] = rand() % 256; \
  for (int i = 0; i < (m_iStrideB << 3); i++) \
    m_pPixSrcB[i] = rand() % 256; \
  EXPECT_EQ (ref (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), func (m_pPixSrcA, \
             m_iStrideA, m_pPixSrcB, m_iStrideB)); \
}

#define GENERATE_Sad8x16_UT(func, ref, CPUFLAGS) \
TEST_F (SadSatdAssemblyFuncTest, func) { \
  if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
    return; \
  for (int i = 0; i < (m_iStrideA << 4); i++) \
    m_pPixSrcA[i] = rand() % 256; \
  for (int i = 0; i < (m_iStrideB << 4); i++) \
    m_pPixSrcB[i] = rand() % 256; \
  EXPECT_EQ (ref (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), func (m_pPixSrcA, \
             m_iStrideA, m_pPixSrcB, m_iStrideB)); \
}

#define GENERATE_Sad16x8_UT(func, ref, CPUFLAGS) \
TEST_F (SadSatdAssemblyFuncTest, func) { \
  if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
    return; \
  for (int i = 0; i < (m_iStrideA << 3); i++) \
    m_pPixSrcA[i] = rand() % 256; \
  for (int i = 0; i < (m_iStrideB << 3); i++) \
    m_pPixSrcB[i] = rand() % 256; \
  EXPECT_EQ (ref (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), func (m_pPixSrcA, \
             m_iStrideA, m_pPixSrcB, m_iStrideB)); \
}

#define GENERATE_Sad16x16_UT(func, ref, CPUFLAGS) \
TEST_F (SadSatdAssemblyFuncTest, func) { \
  if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
    return; \
  for (int i = 0; i < (m_iStrideA << 4); i++) \
    m_pPixSrcA[i] = rand() % 256; \
  for (int i = 0; i < (m_iStrideB << 4); i++) \
    m_pPixSrcB[i] = rand() % 256; \
  EXPECT_EQ (ref (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), func (m_pPixSrcA, \
             m_iStrideA, m_pPixSrcB, m_iStrideB)); \
}

#ifdef X86_ASM
GENERATE_Sad4x4_UT (WelsSampleSad4x4_mmx, WelsSampleSad4x4_c, WELS_CPU_MMXEXT)
GENERATE_Sad8x8_UT (WelsSampleSad8x8_sse21, WelsSampleSad8x8_c, WELS_CPU_SSE2)
GENERATE_Sad8x16_UT (WelsSampleSad8x16_sse2, WelsSampleSad8x16_c, WELS_CPU_SSE2)
GENERATE_Sad16x8_UT (WelsSampleSad16x8_sse2, WelsSampleSad16x8_c, WELS_CPU_SSE2)
GENERATE_Sad16x16_UT (WelsSampleSad16x16_sse2, WelsSampleSad16x16_c, WELS_CPU_SSE2)

GENERATE_Sad4x4_UT (WelsSampleSatd4x4_sse2, WelsSampleSatd4x4_c, WELS_CPU_SSE2)
GENERATE_Sad8x8_UT (WelsSampleSatd8x8_sse2, WelsSampleSatd8x8_c, WELS_CPU_SSE2)
GENERATE_Sad8x16_UT (WelsSampleSatd8x16_sse2, WelsSampleSatd8x16_c, WELS_CPU_SSE2)
GENERATE_Sad16x8_UT (WelsSampleSatd16x8_sse2, WelsSampleSatd16x8_c, WELS_CPU_SSE2)
GENERATE_Sad16x16_UT (WelsSampleSatd16x16_sse2, WelsSampleSatd16x16_c, WELS_CPU_SSE2)

GENERATE_Sad4x4_UT (WelsSampleSatd4x4_sse41, WelsSampleSatd4x4_c, WELS_CPU_SSE41)
GENERATE_Sad8x8_UT (WelsSampleSatd8x8_sse41, WelsSampleSatd8x8_c, WELS_CPU_SSE41)
GENERATE_Sad8x16_UT (WelsSampleSatd8x16_sse41, WelsSampleSatd8x16_c, WELS_CPU_SSE41)
GENERATE_Sad16x8_UT (WelsSampleSatd16x8_sse41, WelsSampleSatd16x8_c, WELS_CPU_SSE41)
GENERATE_Sad16x16_UT (WelsSampleSatd16x16_sse41, WelsSampleSatd16x16_c, WELS_CPU_SSE41)

GENERATE_Sad8x8_UT (WelsSampleSatd8x8_avx2, WelsSampleSatd8x8_c, WELS_CPU_AVX2)
GENERATE_Sad8x16_UT (WelsSampleSatd8x16_avx2, WelsSampleSatd8x16_c, WELS_CPU_AVX2)
GENERATE_Sad16x8_UT (WelsSampleSatd16x8_avx2, WelsSampleSatd16x8_c, WELS_CPU_AVX2)
GENERATE_Sad16x16_UT (WelsSampleSatd16x16_avx2, WelsSampleSatd16x16_c, WELS_CPU_AVX2)
#endif

#ifdef HAVE_NEON
GENERATE_Sad4x4_UT (WelsSampleSad4x4_neon, WelsSampleSad4x4_c, WELS_CPU_NEON)
GENERATE_Sad8x8_UT (WelsSampleSad8x8_neon, WelsSampleSad8x8_c, WELS_CPU_NEON)
GENERATE_Sad8x16_UT (WelsSampleSad8x16_neon, WelsSampleSad8x16_c, WELS_CPU_NEON)
GENERATE_Sad16x8_UT (WelsSampleSad16x8_neon, WelsSampleSad16x8_c, WELS_CPU_NEON)
GENERATE_Sad16x16_UT (WelsSampleSad16x16_neon, WelsSampleSad16x16_c, WELS_CPU_NEON)

GENERATE_Sad4x4_UT (WelsSampleSatd4x4_neon, WelsSampleSatd4x4_c, WELS_CPU_NEON)
GENERATE_Sad8x8_UT (WelsSampleSatd8x8_neon, WelsSampleSatd8x8_c, WELS_CPU_NEON)
GENERATE_Sad8x16_UT (WelsSampleSatd8x16_neon, WelsSampleSatd8x16_c, WELS_CPU_NEON)
GENERATE_Sad16x8_UT (WelsSampleSatd16x8_neon, WelsSampleSatd16x8_c, WELS_CPU_NEON)
GENERATE_Sad16x16_UT (WelsSampleSatd16x16_neon, WelsSampleSatd16x16_c, WELS_CPU_NEON)
#endif

#ifdef HAVE_NEON_AARCH64
GENERATE_Sad4x4_UT (WelsSampleSad4x4_AArch64_neon, WelsSampleSad4x4_c, WELS_CPU_NEON)
GENERATE_Sad8x8_UT (WelsSampleSad8x8_AArch64_neon, WelsSampleSad8x8_c, WELS_CPU_NEON)
GENERATE_Sad8x16_UT (WelsSampleSad8x16_AArch64_neon, WelsSampleSad8x16_c, WELS_CPU_NEON)
GENERATE_Sad16x8_UT (WelsSampleSad16x8_AArch64_neon, WelsSampleSad16x8_c, WELS_CPU_NEON)
GENERATE_Sad16x16_UT (WelsSampleSad16x16_AArch64_neon, WelsSampleSad16x16_c, WELS_CPU_NEON)

GENERATE_Sad4x4_UT (WelsSampleSatd4x4_AArch64_neon, WelsSampleSatd4x4_c, WELS_CPU_NEON)
GENERATE_Sad8x8_UT (WelsSampleSatd8x8_AArch64_neon, WelsSampleSatd8x8_c, WELS_CPU_NEON)
GENERATE_Sad8x16_UT (WelsSampleSatd8x16_AArch64_neon, WelsSampleSatd8x16_c, WELS_CPU_NEON)
GENERATE_Sad16x8_UT (WelsSampleSatd16x8_AArch64_neon, WelsSampleSatd16x8_c, WELS_CPU_NEON)
GENERATE_Sad16x16_UT (WelsSampleSatd16x16_AArch64_neon, WelsSampleSatd16x16_c, WELS_CPU_NEON)
#endif

#define GENERATE_SadFour_UT(func, CPUFLAGS, width, height) \
TEST_F (SadSatdAssemblyFuncTest, func) { \
  if (0 == (m_uiCpuFeatureFlag & CPUFLAGS)) \
    return; \
  for (int i = 0; i < (m_iStrideA << 5); i++) \
    m_pPixSrcA[i] = rand() % 256; \
  for (int i = 0; i < (m_iStrideB << 5); i++) \
    m_pPixSrcB[i] = rand() % 256; \
  uint8_t* pPixA = m_pPixSrcA; \
  uint8_t* pPixB = m_pPixSrcB + m_iStrideB; \
  int32_t iSumSad = 0; \
  for (int i = 0; i < height; i++) { \
    for (int j = 0; j < width; j++) { \
      iSumSad += abs (pPixA[j] - pPixB[j - 1]); \
      iSumSad += abs (pPixA[j] - pPixB[j + 1]); \
      iSumSad += abs (pPixA[j] - pPixB[j - m_iStrideB]); \
      iSumSad += abs (pPixA[j] - pPixB[j + m_iStrideB]); \
    } \
    pPixA += m_iStrideA; \
    pPixB += m_iStrideB; \
  } \
  func (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad); \
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad); \
}

#ifdef X86_ASM
GENERATE_SadFour_UT (WelsSampleSadFour4x4_sse2, WELS_CPU_SSE2, 4, 4)
GENERATE_SadFour_UT (WelsSampleSadFour8x8_sse2, WELS_CPU_SSE2, 8, 8)
GENERATE_SadFour_UT (WelsSampleSadFour8x16_sse2, WELS_CPU_SSE2, 8, 16)
GENERATE_SadFour_UT (WelsSampleSadFour16x8_sse2, WELS_CPU_SSE2, 16, 8)
GENERATE_SadFour_UT (WelsSampleSadFour16x16_sse2, WELS_CPU_SSE2, 16, 16)
#endif

#ifdef HAVE_NEON
GENERATE_SadFour_UT (WelsSampleSadFour4x4_neon, WELS_CPU_NEON, 4, 4)
GENERATE_SadFour_UT (WelsSampleSadFour8x8_neon, WELS_CPU_NEON, 8, 8)
GENERATE_SadFour_UT (WelsSampleSadFour8x16_neon, WELS_CPU_NEON, 8, 16)
GENERATE_SadFour_UT (WelsSampleSadFour16x8_neon, WELS_CPU_NEON, 16, 8)
GENERATE_SadFour_UT (WelsSampleSadFour16x16_neon, WELS_CPU_NEON, 16, 16)
#endif

#ifdef HAVE_NEON_AARCH64
GENERATE_SadFour_UT (WelsSampleSadFour4x4_AArch64_neon, WELS_CPU_NEON, 4, 4)
GENERATE_SadFour_UT (WelsSampleSadFour8x8_AArch64_neon, WELS_CPU_NEON, 8, 8)
GENERATE_SadFour_UT (WelsSampleSadFour8x16_AArch64_neon, WELS_CPU_NEON, 8, 16)
GENERATE_SadFour_UT (WelsSampleSadFour16x8_AArch64_neon, WELS_CPU_NEON, 16, 8)
GENERATE_SadFour_UT (WelsSampleSadFour16x16_AArch64_neon, WELS_CPU_NEON, 16, 16)
#endif
