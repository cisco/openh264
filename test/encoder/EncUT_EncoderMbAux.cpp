#include <gtest/gtest.h>
#include "ls_defines.h"
#include "encode_mb_aux.h"
#include "wels_common_basis.h"

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
TEST (EncodeMbAuxTest, WelsDctT4_c) {
  int16_t iDctRef[4][4];
  uint8_t uiPix1[16 * FENC_STRIDE], uiPix2[16 * FDEC_STRIDE];
  int16_t iDct[16];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      uiPix1[i * FENC_STRIDE + j] = uiPix2[i * FDEC_STRIDE + j] = rand() & 255;
  Sub4x4DctAnchor (iDctRef, uiPix1, uiPix2);
  WelsDctT4_c (iDct, uiPix1, FENC_STRIDE, uiPix2, FDEC_STRIDE);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      EXPECT_EQ (iDctRef[j][i], iDct[i * 4 + j]);
}
TEST (EncodeMbAuxTest, WelsDctFourT4_c) {
  int16_t iDctRef[4][4][4];
  uint8_t uiPix1[16 * FENC_STRIDE], uiPix2[16 * FDEC_STRIDE];
  int16_t iDct[16 * 4];
  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8; j++)
      uiPix1[i * FENC_STRIDE + j] = uiPix2[i * FDEC_STRIDE + j] = rand() & 255;
  Sub8x8DctAnchor (iDctRef, uiPix1, uiPix2);
  WelsDctFourT4_c (iDct, uiPix1, FENC_STRIDE, uiPix2, FDEC_STRIDE);
  for (int k = 0; k < 4; k++)
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        EXPECT_EQ (iDctRef[k][j][i], iDct[k * 16 + i * 4 + j]);
}

#ifdef X86_ASM
TEST (EncodeMbAuxTest, WelsDctT4_mmx) {
  int16_t iDctC[16], iDctM[16];
  uint8_t uiPix1[16 * FENC_STRIDE], uiPix2[16 * FDEC_STRIDE];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      uiPix1[i * FENC_STRIDE + j] = uiPix2[i * FDEC_STRIDE + j] = rand() & 255;
  WelsDctT4_c (iDctC, uiPix1, FENC_STRIDE, uiPix2, FDEC_STRIDE);
  WelsDctT4_mmx (iDctM, uiPix1, FENC_STRIDE, uiPix2, FDEC_STRIDE);
  for (int i = 0; i < 16; i++)
    EXPECT_EQ (iDctC[i], iDctM[i]);
}

TEST (EncodeMbAuxTest, WelsDctFourT4_sse2) {
  CMemoryAlign cMemoryAlign (0);
  ALLOC_MEMORY (uint8_t, uiPix1, 16 * FENC_STRIDE);
  ALLOC_MEMORY (uint8_t, uiPix2, 16 * FDEC_STRIDE);
  ALLOC_MEMORY (int16_t, iDctC, 16 * 4);
  ALLOC_MEMORY (int16_t, iDctS, 16 * 4);
  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8; j++)
      uiPix1[i * FENC_STRIDE + j] = uiPix2[i * FDEC_STRIDE + j] = rand() & 255;
  WelsDctFourT4_c (iDctC, uiPix1, FENC_STRIDE, uiPix2, FDEC_STRIDE);
  WelsDctFourT4_sse2 (iDctS, uiPix1, FENC_STRIDE, uiPix2, FDEC_STRIDE);
  for (int i = 0; i < 64; i++)
    EXPECT_EQ (iDctC[i], iDctS[i]);
  FREE_MEMORY (uiPix1);
  FREE_MEMORY (uiPix2);
  FREE_MEMORY (iDctC);
  FREE_MEMORY (iDctS);
}

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
TEST (EncodeMbAuxTest, WelsGetNoneZeroCount_c) {
  ENFORCE_STACK_ALIGN_1D (int16_t, pLevel, 16, 16);
  int32_t result = 0;
  for (int i = 0; i < 16; i++) {
    pLevel[i] = (rand() & 0x07) - 4;
    if (pLevel[i]) result ++;
  }
  int32_t nnz = WelsGetNoneZeroCount_c (pLevel);
  EXPECT_EQ (nnz, result);
}
#ifdef X86_ASM
TEST (EncodeMbAuxTest, WelsGetNoneZeroCount_sse2) {
  ENFORCE_STACK_ALIGN_1D (int16_t, pLevel, 16, 16);
  int32_t result = 0;
  for (int i = 0; i < 16; i++) {
    pLevel[i] = (rand() & 0x07) - 4;
    if (pLevel[i]) result ++;
  }
  int32_t nnz = WelsGetNoneZeroCount_sse2 (pLevel);
  EXPECT_EQ (nnz, result);
}
#endif
#define WELS_ABS_LC(a) ((sign ^ (int32_t)(a)) - sign)
#define NEW_QUANT(pDct, ff, mf) (((ff)+ WELS_ABS_LC(pDct))*(mf)) >>16
#define WELS_NEW_QUANT(pDct,ff,mf) WELS_ABS_LC(NEW_QUANT(pDct, ff, mf))
void WelsQuantFour4x4MaxAnchor (int16_t* pDct, int16_t* ff,  int16_t* mf, int16_t* max) {
  int32_t i, j, k, sign;
  int16_t max_abs;
  for (k = 0; k < 4; k++) {
    max_abs = 0;
    for (i = 0; i < 16; i++) {
      j = i & 0x07;
      sign = WELS_SIGN (pDct[i]);
      pDct[i] = NEW_QUANT (pDct[i], ff[j], mf[j]);
      if (max_abs < pDct[i]) max_abs = pDct[i];
      pDct[i] = WELS_ABS_LC (pDct[i]);
    }
    pDct += 16;
    max[k] = max_abs;
  }
}
TEST (EncodeMbAuxTest, WelsQuantFour4x4Max_c) {
  int16_t ff[8], mf[8];
  int16_t iDctA[64], iMaxA[16];
  int16_t iDctC[64], iMaxC[16];
  for (int i = 0; i < 8; i++) {
    ff[i] = rand() & 32767;
    mf[i] = rand() & 32767;
  }
  for (int i = 0; i < 64; i++)
    iDctA[i] = iDctC[i] = (rand() & 65535) - 32767;
  WelsQuantFour4x4MaxAnchor (iDctA, ff, mf, iMaxA);
  WelsQuantFour4x4Max_c (iDctC, ff, mf, iMaxC);
  for (int i = 0; i < 64; i++)
    EXPECT_EQ (iDctA[i], iDctC[i]);
  for (int i = 0; i < 4; i++)
    EXPECT_EQ (iMaxA[i], iMaxC[i]);
}
#ifdef X86_ASM
TEST (EncodeMbAuxTest, WelsQuantFour4x4Max_sse2) {
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
  WelsQuantFour4x4Max_c (iDctC, ff, mf, iMaxC);
  WelsQuantFour4x4Max_sse2 (iDctS, ff, mf, iMaxS);
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
