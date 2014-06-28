#include<gtest/gtest.h>
#include<math.h>

#include "cpu_core.h"
#include "cpu.h"
#include "sample.h"
#include "sad_common.h"
#include "get_intra_predictor.h"

using namespace WelsSVCEnc;
#ifdef X86_ASM
TEST (IntraSadSatdFuncTest, WelsIntra16x16Combined3Sad_ssse3) {
  const int32_t iLineSizeDec = 32;
  const int32_t iLineSizeEnc = 32;
  int32_t tmpa, tmpb;
  int32_t iBestMode_c, iBestMode_a, iLambda = 50;
  CMemoryAlign cMemoryAlign (0);
  int32_t iCpuCores = 0;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSSE3))
    return;
  uint8_t* pDec = (uint8_t*)cMemoryAlign.WelsMalloc (iLineSizeDec << 5, "pDec");
  uint8_t* pEnc = (uint8_t*)cMemoryAlign.WelsMalloc (iLineSizeEnc << 5, "pEnc");
  uint8_t* pDst = (uint8_t*)cMemoryAlign.WelsMalloc (512, "pDst");
  for (int i = 0; i < (iLineSizeDec << 5); i++)
    pDec[i] = rand() % 256;
  for (int i = 0; i < (iLineSizeEnc << 5); i++)
    pEnc[i] = rand() % 256;

  for (int i = 0; i < 512; i++)
    pDst[i] = rand() % 256;
  tmpa = WelsSampleSadIntra16x16Combined3_c (pDec + 128, iLineSizeDec, pEnc, iLineSizeEnc, &iBestMode_c, iLambda, pDst);
  tmpb = WelsIntra16x16Combined3Sad_ssse3 (pDec + 128, iLineSizeDec, pEnc, iLineSizeEnc, &iBestMode_a, iLambda, pDst);

  ASSERT_EQ (tmpa, tmpb);
  ASSERT_EQ (iBestMode_c, iBestMode_a);

  cMemoryAlign.WelsFree (pDec, "pDec");
  cMemoryAlign.WelsFree (pEnc, "pEnc");
  cMemoryAlign.WelsFree (pDst, "pDst");
}

TEST (IntraSadSatdFuncTest, WelsIntra16x16Combined3Satd_sse41) {
  const int32_t iLineSizeDec = 32;
  const int32_t iLineSizeEnc = 32;
  int32_t tmpa, tmpb;
  int32_t iBestMode_c, iBestMode_a, iLambda = 50;
  CMemoryAlign cMemoryAlign (0);
  int32_t iCpuCores = 0;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  uint8_t* pDec = (uint8_t*)cMemoryAlign.WelsMalloc (iLineSizeDec << 5, "pDec");
  uint8_t* pEnc = (uint8_t*)cMemoryAlign.WelsMalloc (iLineSizeEnc << 5, "pEnc");
  uint8_t* pDst = (uint8_t*)cMemoryAlign.WelsMalloc (512, "pDst");
  for (int i = 0; i < (iLineSizeDec << 5); i++)
    pDec[i] = rand() % 256;
  for (int i = 0; i < (iLineSizeEnc << 5); i++)
    pEnc[i] = rand() % 256;
  for (int i = 0; i < 512; i++)
    pDst[i] = rand() % 256;
  tmpa = WelsSampleSatdIntra16x16Combined3_c (pDec + 128, iLineSizeDec, pEnc, iLineSizeEnc, &iBestMode_c, iLambda, pDst);
  tmpb = WelsIntra16x16Combined3Satd_sse41 (pDec + 128, iLineSizeDec, pEnc, iLineSizeEnc, &iBestMode_a, iLambda, pDst);
  ASSERT_EQ (tmpa, tmpb);
  ASSERT_EQ (iBestMode_c, iBestMode_a);
  cMemoryAlign.WelsFree (pDec, "pDec");
  cMemoryAlign.WelsFree (pEnc, "pEnc");
  cMemoryAlign.WelsFree (pDst, "pDst");
}

TEST (IntraSadSatdFuncTest, WelsSampleSatdThree4x4_sse2) {
  const int32_t iLineSizeDec = 32;
  const int32_t iLineSizeEnc = 32;
  int32_t tmpa, tmpb;
  int32_t iBestMode_c, iBestMode_a, iLambda = 50;
  int32_t lambda[2]						= {iLambda << 2, iLambda};
  int32_t iPredMode = rand() % 3;
  CMemoryAlign cMemoryAlign (0);
  int32_t iCpuCores = 0;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  uint8_t* pDec = (uint8_t*)cMemoryAlign.WelsMalloc (iLineSizeDec << 5, "pDec");
  uint8_t* pEnc = (uint8_t*)cMemoryAlign.WelsMalloc (iLineSizeEnc << 5, "pEnc");
  uint8_t* pDst = (uint8_t*)cMemoryAlign.WelsMalloc (512, "pDst");
  WelsInitFillingPredFuncs (WELS_CPU_SSE2);
  for (int i = 0; i < (iLineSizeDec << 5); i++)
    pDec[i] = rand() % 256;
  for (int i = 0; i < (iLineSizeEnc << 5); i++)
    pEnc[i] = rand() % 256;
  for (int i = 0; i < 512; i++)
    pDst[i] = rand() % 256;
  tmpa = WelsSampleSatdIntra4x4Combined3_c (pDec + 128, iLineSizeDec, pEnc, iLineSizeEnc, pDst, &iBestMode_c,
         lambda[iPredMode == 2], lambda[iPredMode == 1], lambda[iPredMode == 0]);
  tmpb = WelsSampleSatdThree4x4_sse2 (pDec + 128, iLineSizeDec, pEnc, iLineSizeEnc, pDst, &iBestMode_a,
                                      lambda[iPredMode == 2], lambda[iPredMode == 1], lambda[iPredMode == 0]);
  ASSERT_EQ (tmpa, tmpb);
  ASSERT_EQ (iBestMode_c, iBestMode_a);
  cMemoryAlign.WelsFree (pDec, "pDec");
  cMemoryAlign.WelsFree (pEnc, "pEnc");
  cMemoryAlign.WelsFree (pDst, "pDst");
}

TEST (IntraSadSatdFuncTest, WelsIntraChroma8x8Combined3Satd_sse41) {
  const int32_t iLineSizeDec = 32;
  const int32_t iLineSizeEnc = 32;
  int32_t tmpa, tmpb;
  int32_t iBestMode_c, iBestMode_a, iLambda = 50;
  CMemoryAlign cMemoryAlign (0);
  int32_t iCpuCores = 0;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  uint8_t* pDecCb = (uint8_t*)cMemoryAlign.WelsMalloc (iLineSizeDec << 5, "pDecCb");
  uint8_t* pEncCb = (uint8_t*)cMemoryAlign.WelsMalloc (iLineSizeEnc << 5, "pEncCb");
  uint8_t* pDecCr = (uint8_t*)cMemoryAlign.WelsMalloc (iLineSizeDec << 5, "pDecCr");
  uint8_t* pEncCr = (uint8_t*)cMemoryAlign.WelsMalloc (iLineSizeEnc << 5, "pEncCr");
  uint8_t* pDstChma = (uint8_t*)cMemoryAlign.WelsMalloc (512, "pDstChma");
  for (int i = 0; i < (iLineSizeDec << 5); i++) {
    pDecCb[i] = rand() % 256;
    pDecCr[i] = rand() % 256;
  }
  for (int i = 0; i < (iLineSizeEnc << 5); i++) {
    pEncCb[i] = rand() % 256;
    pEncCr[i] = rand() % 256;
  }
  for (int i = 0; i < 512; i++)
    pDstChma[i] = rand() % 256;
  tmpa = WelsSampleSatdIntra8x8Combined3_c (pDecCb + 128, iLineSizeDec, pEncCb, iLineSizeEnc, &iBestMode_c, iLambda,
         pDstChma, pDecCr + 128, pEncCr);
  tmpb = WelsIntraChroma8x8Combined3Satd_sse41 (pDecCb + 128, iLineSizeDec, pEncCb, iLineSizeEnc, &iBestMode_a, iLambda,
         pDstChma, pDecCr + 128, pEncCr);
  ASSERT_EQ (tmpa, tmpb);
  ASSERT_EQ (iBestMode_c, iBestMode_a);
  cMemoryAlign.WelsFree (pDecCb, "pDecCb");
  cMemoryAlign.WelsFree (pEncCb, "pEncCb");
  cMemoryAlign.WelsFree (pDecCr, "pDecCr");
  cMemoryAlign.WelsFree (pEncCr, "pEncCr");
  cMemoryAlign.WelsFree (pDstChma, "pDstChma");
}
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

#ifdef X86_ASM
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

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSad4x4_mmx) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_MMXEXT))
    return;

  for (int i = 0; i < (m_iStrideA << 2); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 2); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSad4x4_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSad4x4_mmx (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}


TEST_F (SadSatdAssemblyFuncTest, WelsSampleSad8x8_sse21) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;

  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSad8x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSad8x8_sse21 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSad8x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  for (int i = 0; i < (m_iStrideA << 4); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 4); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSad8x16_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSad8x16_sse2 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSad16x8_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSad16x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSad16x8_sse2 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSad16x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  for (int i = 0; i < (m_iStrideA << 4); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 4); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSad16x16_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSad16x16_sse2 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSatd4x4_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  for (int i = 0; i < (m_iStrideA << 2); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 2); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSatd4x4_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSatd4x4_sse2 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSatd8x8_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSatd8x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSatd8x8_sse2 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSatd8x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  for (int i = 0; i < (m_iStrideA << 4); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 4); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSatd8x16_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSatd8x16_sse2 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSatd16x8_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSatd16x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSatd16x8_sse2 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSatd16x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  for (int i = 0; i < (m_iStrideA << 4); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 4); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSatd16x16_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSatd16x16_sse2 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSatd4x4_sse41) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  for (int i = 0; i < (m_iStrideA << 2); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 2); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSatd4x4_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSatd4x4_sse41 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSatd8x8_sse41) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSatd8x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSatd8x8_sse41 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSatd8x16_sse41) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  for (int i = 0; i < (m_iStrideA << 4); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 4); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSatd8x16_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSatd8x16_sse41 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSatd16x8_sse41) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  for (int i = 0; i < (m_iStrideA << 3); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 3); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSatd16x8_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB), WelsSampleSatd16x8_sse41 (m_pPixSrcA,
             m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSatd16x16_sse41) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  for (int i = 0; i < (m_iStrideA << 4); i++)
    m_pPixSrcA[i] = rand() % 256;
  for (int i = 0; i < (m_iStrideB << 4); i++)
    m_pPixSrcB[i] = rand() % 256;

  EXPECT_EQ (WelsSampleSatd16x16_c (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB),
             WelsSampleSatd16x16_sse41 (m_pPixSrcA, m_iStrideA, m_pPixSrcB, m_iStrideB));
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSadFour16x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;

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

  WelsSampleSadFour16x16_sse2 (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSadFour16x8_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;

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

  WelsSampleSadFour16x8_sse2 (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSadFour8x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;

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

  WelsSampleSadFour8x16_sse2 (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}

TEST_F (SadSatdAssemblyFuncTest, WelsSampleSadFour8x8_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;

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

  WelsSampleSadFour8x8_sse2 (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}


TEST_F (SadSatdAssemblyFuncTest, WelsSampleSadFour4x4_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;

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

  WelsSampleSadFour4x4_sse2 (m_pPixSrcA, m_iStrideA, m_pPixSrcB + m_iStrideB, m_iStrideB, m_pSad);
  EXPECT_EQ (m_pSad[0] + m_pSad[1] + m_pSad[2] + m_pSad[3], iSumSad);
}
#endif
