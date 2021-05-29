#include <gtest/gtest.h>
#include "decode_mb_aux.h"
#include "wels_common_basis.h"
#include "macros.h"
#include "cpu.h"

using namespace WelsEnc;


TEST (DecodeMbAuxTest, TestIhdm_4x4_dc) {
  short W[16], T[16], Y[16];
  for (int i = 0; i < 16; i++)
    W[i] = rand() % 256 + 1;

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

  WelsIHadamard4x4Dc (W);
  for (int i = 0; i < 16; i++)
    EXPECT_EQ (Y[i], W[i]);
}

TEST (DecodeMbAuxTest, TestDequant_4x4_luma_dc) {
  short T[16], W[16];

  for (int qp = 0; qp < 12; qp++) {
    for (int i = 0; i < 16; i++) {
      T[i] = rand() % 256 + 1;
      W[i] = T[i];
    }
    WelsDequantLumaDc4x4 (W, qp);
    for (int i = 0; i < 16; i++) {
      T[i] = (((T[i] * g_kuiDequantCoeff[qp % 6][0] + (1 << (1 -  qp / 6)))) >> (2 - qp / 6));
      EXPECT_EQ (T[i], W[i]);
    }
  }
}

TEST (DecodeMbAuxTest, TestDequant_ihdm_4x4_c) {
  short W[16], T[16], Y[16];
  const unsigned short mf = rand() % 16 + 1;
  for (int i = 0; i < 16; i++)
    W[i] = rand() % 256 + 1;

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

  Y[0] = (T[0] + T[1] + T[2] + T[3]) * mf;
  Y[1] = (T[0] + T[1] - T[2] - T[3]) * mf;
  Y[2] = (T[0] - T[1] - T[2] + T[3]) * mf;
  Y[3] = (T[0] - T[1] + T[2] - T[3]) * mf;

  Y[4] = (T[4] + T[5] + T[6] + T[7]) * mf;
  Y[5] = (T[4] + T[5] - T[6] - T[7]) * mf;
  Y[6] = (T[4] - T[5] - T[6] + T[7]) * mf;
  Y[7] = (T[4] - T[5] + T[6] - T[7]) * mf;

  Y[8] = (T[8] + T[9] + T[10] + T[11]) * mf;
  Y[9] = (T[8] + T[9] - T[10] - T[11]) * mf;
  Y[10] = (T[8] - T[9] - T[10] + T[11]) * mf;
  Y[11] = (T[8] - T[9] + T[10] - T[11]) * mf;

  Y[12] = (T[12] + T[13] + T[14] + T[15]) * mf;
  Y[13] = (T[12] + T[13] - T[14] - T[15]) * mf;
  Y[14] = (T[12] - T[13] - T[14] + T[15]) * mf;
  Y[15] = (T[12] - T[13] + T[14] - T[15]) * mf;

  WelsDequantIHadamard4x4_c (W, mf);
  for (int i = 0; i < 16; i++)
    EXPECT_EQ (Y[i], W[i]);
}

TEST (DecodeMbAuxTest, TestDequant_4x4_c) {
  short W[16], T[16];
  unsigned short mf[16];
  for (int i = 0; i < 16; i++) {
    W[i] = rand() % 256 + 1;
    T[i] = W[i];
  }

  for (int i = 0; i < 8; i++)
    mf[i] = rand() % 16 + 1;
  WelsDequant4x4_c (W, mf);
  for (int i = 0; i < 16; i++)
    EXPECT_EQ (T[i]*mf[i % 8], W[i]);
}
TEST (DecodeMbAuxTest, TestDequant_4_4x4_c) {
  short W[64], T[64];
  unsigned short mf[16];
  for (int i = 0; i < 64; i++) {
    W[i] = rand() % 256 + 1;
    T[i] = W[i];
  }
  for (int i = 0; i < 8; i++)
    mf[i] = rand() % 16 + 1;
  WelsDequantFour4x4_c (W, mf);
  for (int i = 0; i < 64; i++)
    EXPECT_EQ (T[i]*mf[i % 8], W[i]);
}
void WelsDequantHadamard2x2DcAnchor (int16_t* pDct, int16_t iMF) {
  const int16_t iSumU = pDct[0] + pDct[2];
  const int16_t iDelU =   pDct[0] -  pDct[2];
  const int16_t iSumD = pDct[1] + pDct[3];
  const int16_t iDelD =   pDct[1] -  pDct[3];
  pDct[0] = ((iSumU + iSumD) * iMF) >> 1;
  pDct[1] = ((iSumU - iSumD) * iMF) >> 1;
  pDct[2] = ((iDelU + iDelD) * iMF) >> 1;
  pDct[3] = ((iDelU - iDelD) * iMF) >> 1;
}
TEST (DecodeMbAuxTest, WelsDequantIHadamard2x2Dc) {
  int16_t iDct[4], iRefDct[4];
  int16_t iMF;
  iMF = rand() & 127;
  for (int i = 0; i < 4; i++)
    iDct[i] = iRefDct[i] = (rand() & 65535) - 32768;
  WelsDequantHadamard2x2DcAnchor (iRefDct, iMF);
  WelsDequantIHadamard2x2Dc (iDct, iMF);
  bool ok = true;
  for (int i = 0; i < 4; i++) {
    if (iDct[i] != iRefDct[i]) {
      ok = false;
      break;
    }
  }
  EXPECT_TRUE (ok);
}
#define FDEC_STRIDE 32
template<typename clip_t>
void WelsIDctT4Anchor (uint8_t* p_dst, int16_t dct[16]) {
  int16_t tmp[16];
  int32_t iStridex2 = (FDEC_STRIDE << 1);
  int32_t iStridex3 = iStridex2 + FDEC_STRIDE;
  uint8_t uiDst = 0;
  int i;
  for (i = 0; i < 4; i++) {
    tmp[i << 2]     = dct[i << 2] + dct[ (i << 2) + 1]      + dct[ (i << 2) + 2] + (dct[ (i << 2) + 3] >> 1);
    tmp[ (i << 2) + 1] = dct[i << 2] + (dct[ (i << 2) + 1] >> 1) - dct[ (i << 2) + 2] - dct[ (i << 2) + 3];
    tmp[ (i << 2) + 2] = dct[i << 2] - (dct[ (i << 2) + 1] >> 1) - dct[ (i << 2) + 2] + dct[ (i << 2) + 3];
    tmp[ (i << 2) + 3] = dct[i << 2] - dct[ (i << 2) + 1]      + dct[ (i << 2) + 2] - (dct[ (i << 2) + 3] >> 1);
  }
  for (i = 0; i < 4; i++) {
    uiDst = p_dst[i];
    p_dst[i]             = WelsClip1 (uiDst + (clip_t (tmp[i] + tmp[4 + i] +     tmp[8 + i] + (tmp[12 + i] >> 1) + 32) >> 6));
    uiDst = p_dst[i + FDEC_STRIDE];
    p_dst[i + FDEC_STRIDE] = WelsClip1 (uiDst + (clip_t (tmp[i] + (tmp[4 + i] >> 1) - tmp[8 + i] - tmp[12 + i] + 32)     >> 6));
    uiDst = p_dst[i + iStridex2];
    p_dst[i + iStridex2]   = WelsClip1 (uiDst + (clip_t (tmp[i] - (tmp[4 + i] >> 1) - tmp[8 + i] + tmp[12 + i] + 32)     >> 6));
    uiDst = p_dst[i + iStridex3];
    p_dst[i + iStridex3]   = WelsClip1 (uiDst + (clip_t (tmp[i] - tmp[4 + i] +     tmp[8 + i] - (tmp[12 + i] >> 1) + 32) >> 6));
  }
}
template<typename clip_t>
void TestIDctT4Rec (PIDctFunc func) {
  int16_t iRefDct[16];
  uint8_t iRefDst[16 * FDEC_STRIDE];
  ENFORCE_STACK_ALIGN_1D (int16_t, iDct, 16, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPred, 16 * FDEC_STRIDE, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iRec, 16 * FDEC_STRIDE, 16);
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      iRefDct[i * 4 + j] = iDct[i * 4 + j] = (rand() & 65535) - 32768;
      iPred[i * FDEC_STRIDE + j] = iRefDst[i * FDEC_STRIDE + j] = rand() & 255;
    }
  }
  WelsIDctT4Anchor<clip_t> (iRefDst, iRefDct);
  func (iRec, FDEC_STRIDE, iPred, FDEC_STRIDE, iDct);
  int ok = -1;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (iRec[i * FDEC_STRIDE + j] != iRefDst[i * FDEC_STRIDE + j]) {
        ok = i * 4 + j;
        break;
      }
    }
  }
  EXPECT_EQ (ok, -1);
}
TEST (DecodeMbAuxTest, WelsIDctT4Rec_c) {
  TestIDctT4Rec<int32_t> (WelsIDctT4Rec_c);
}
#if defined(X86_ASM)
TEST (DecodeMbAuxTest, WelsIDctT4Rec_mmx) {
  TestIDctT4Rec<int16_t> (WelsIDctT4Rec_mmx);
}
TEST (DecodeMbAuxTest, WelsIDctT4Rec_sse2) {
  TestIDctT4Rec<int16_t> (WelsIDctT4Rec_sse2);
}
#if defined(HAVE_AVX2)
TEST (DecodeMbAuxTest, WelsIDctT4Rec_avx2) {
  if (WelsCPUFeatureDetect (0) & WELS_CPU_AVX2)
    TestIDctT4Rec<int16_t> (WelsIDctT4Rec_avx2);
}
#endif
#endif
#if defined(HAVE_MMI)
TEST (DecodeMbAuxTest, WelsIDctT4Rec_mmi) {
  TestIDctT4Rec<int16_t> (WelsIDctT4Rec_mmi);
}
#endif
template<typename clip_t>
void WelsIDctT8Anchor (uint8_t* p_dst, int16_t dct[4][16]) {
  WelsIDctT4Anchor<clip_t> (&p_dst[0],                   dct[0]);
  WelsIDctT4Anchor<clip_t> (&p_dst[4],                   dct[1]);
  WelsIDctT4Anchor<clip_t> (&p_dst[4 * FDEC_STRIDE + 0], dct[2]);
  WelsIDctT4Anchor<clip_t> (&p_dst[4 * FDEC_STRIDE + 4], dct[3]);
}
template<typename clip_t>
void TestIDctFourT4Rec (PIDctFunc func) {
  int16_t iRefDct[4][16];
  uint8_t iRefDst[16 * FDEC_STRIDE];
  ENFORCE_STACK_ALIGN_1D (int16_t, iDct, 64, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPred, 16 * FDEC_STRIDE, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iRec, 16 * FDEC_STRIDE, 16);
  for (int k = 0; k < 4; k++)
    for (int i = 0; i < 16; i++)
      iRefDct[k][i] = iDct[k * 16 + i] = (rand() & 65535) - 32768;

  for (int i = 0; i < 8; i++)
    for (int j = 0; j < 8; j++)
      iPred[i * FDEC_STRIDE + j] = iRefDst[i * FDEC_STRIDE + j] = rand() & 255;

  WelsIDctT8Anchor<clip_t> (iRefDst, iRefDct);
  func (iRec, FDEC_STRIDE, iPred, FDEC_STRIDE, iDct);
  int ok = -1;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (iRec[i * FDEC_STRIDE + j] != iRefDst[i * FDEC_STRIDE + j]) {
        ok = i * 8 + j;
        break;
      }
    }
  }
  EXPECT_EQ (ok, -1);
}
TEST (DecodeMbAuxTest, WelsIDctFourT4Rec_c) {
  TestIDctFourT4Rec<int32_t> (WelsIDctFourT4Rec_c);
}
void WelsIDctRecI16x4DcAnchor (uint8_t* p_dst, int16_t dct[4]) {
  for (int i = 0; i < 4; i++, p_dst += FDEC_STRIDE) {
    p_dst[0] = WelsClip1 (p_dst[0] + ((dct[0] + 32) >> 6));
    p_dst[1] = WelsClip1 (p_dst[1] + ((dct[0] + 32) >> 6));
    p_dst[2] = WelsClip1 (p_dst[2] + ((dct[0] + 32) >> 6));
    p_dst[3] = WelsClip1 (p_dst[3] + ((dct[0] + 32) >> 6));

    p_dst[4] = WelsClip1 (p_dst[4] + ((dct[1] + 32) >> 6));
    p_dst[5] = WelsClip1 (p_dst[5] + ((dct[1] + 32) >> 6));
    p_dst[6] = WelsClip1 (p_dst[6] + ((dct[1] + 32) >> 6));
    p_dst[7] = WelsClip1 (p_dst[7] + ((dct[1] + 32) >> 6));

    p_dst[8]  = WelsClip1 (p_dst[8]  + ((dct[2] + 32) >> 6));
    p_dst[9]  = WelsClip1 (p_dst[9]  + ((dct[2] + 32) >> 6));
    p_dst[10] = WelsClip1 (p_dst[10] + ((dct[2] + 32) >> 6));
    p_dst[11] = WelsClip1 (p_dst[11] + ((dct[2] + 32) >> 6));

    p_dst[12] = WelsClip1 (p_dst[12] + ((dct[3] + 32) >> 6));
    p_dst[13] = WelsClip1 (p_dst[13] + ((dct[3] + 32) >> 6));
    p_dst[14] = WelsClip1 (p_dst[14] + ((dct[3] + 32) >> 6));
    p_dst[15] = WelsClip1 (p_dst[15] + ((dct[3] + 32) >> 6));
  }
}
void WelsIDctRecI16x16DcAnchor (uint8_t* p_dst, int16_t dct[4][4]) {
  for (int i = 0; i < 4; i++, p_dst += 4 * FDEC_STRIDE)
    WelsIDctRecI16x4DcAnchor (&p_dst[0], dct[i]);
}

TEST (DecodeMbAuxTest, WelsIDctRecI16x16Dc_c) {
  uint8_t iRefDst[16 * FDEC_STRIDE];
  int16_t iRefDct[4][4];
  ENFORCE_STACK_ALIGN_1D (int16_t, iDct, 16, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPred, 16 * FDEC_STRIDE, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iRec, 16 * FDEC_STRIDE, 16);
  for (int i = 0; i < 16; i++)
    for (int j = 0; j < 16; j++)
      iRefDst[i * FDEC_STRIDE + j] = iPred[i * FDEC_STRIDE + j] = rand() & 255;

  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      iRefDct[i][j] = iDct[i * 4 + j] = (rand() & 65535) - 32768;
  WelsIDctRecI16x16DcAnchor (iRefDst, iRefDct);
  WelsIDctRecI16x16Dc_c (iRec, FDEC_STRIDE, iPred, FDEC_STRIDE, iDct);
  int ok = -1;
  for (int i = 0; i < 16; i++) {
    for (int j = 0; j < 16; j++) {
      if (iRec[i * FDEC_STRIDE + j] != iRefDst[i * FDEC_STRIDE + j]) {
        ok = i * 16 + j;
        break;
      }
    }
  }
  EXPECT_EQ (ok, -1);
}
#if defined(X86_ASM)
TEST (DecodeMbAuxTest, WelsIDctFourT4Rec_sse2) {
  TestIDctFourT4Rec<int16_t> (WelsIDctFourT4Rec_sse2);
}
#if defined(HAVE_AVX2)
TEST (DecodeMbAuxTest, WelsIDctFourT4Rec_avx2) {
  if (WelsCPUFeatureDetect (0) & WELS_CPU_AVX2)
    TestIDctFourT4Rec<int16_t> (WelsIDctFourT4Rec_avx2);
}
#endif
TEST (DecodeMbAuxTest, WelsIDctRecI16x16Dc_sse2) {
  int32_t iCpuCores = 0;
  uint32_t uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);

  if (uiCpuFeatureFlag & WELS_CPU_SSE2) {
    uint8_t iRefDst[16 * FDEC_STRIDE];
    int16_t iRefDct[4][4];
    ENFORCE_STACK_ALIGN_1D (int16_t, iDct, 16, 16);
    ENFORCE_STACK_ALIGN_1D (uint8_t, iPred, 16 * FDEC_STRIDE, 16);
    ENFORCE_STACK_ALIGN_1D (uint8_t, iRec, 16 * FDEC_STRIDE, 16);
    for (int i = 0; i < 16; i++)
      for (int j = 0; j < 16; j++)
        iRefDst[i * FDEC_STRIDE + j] = iPred[i * FDEC_STRIDE + j] = rand() & 255;
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        iRefDct[i][j] = iDct[i * 4 + j] = (rand() & ((1 << 15) - 1)) - (1 <<
                                          14); //2^14 limit, (2^15+32) will cause overflow for SSE2.
    WelsIDctRecI16x16DcAnchor (iRefDst, iRefDct);
    WelsIDctRecI16x16Dc_sse2 (iRec, FDEC_STRIDE, iPred, FDEC_STRIDE, iDct);
    int ok = -1;
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        if (iRec[i * FDEC_STRIDE + j] != iRefDst[i * FDEC_STRIDE + j]) {
          ok = i * 16 + j;
          break;
        }
      }
    }
    EXPECT_EQ (ok, -1);
  }
}
#endif
#if defined(HAVE_MMI)
TEST (DecodeMbAuxTest, WelsIDctFourT4Rec_mmi) {
  TestIDctFourT4Rec<int16_t> (WelsIDctFourT4Rec_mmi);
}
TEST (DecodeMbAuxTest, WelsIDctRecI16x16Dc_mmi) {
  int32_t iCpuCores = 0;
  uint32_t uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);

  if (uiCpuFeatureFlag & WELS_CPU_MMI) {
    uint8_t iRefDst[16 * FDEC_STRIDE];
    int16_t iRefDct[4][4];
    ENFORCE_STACK_ALIGN_1D (int16_t, iDct, 16, 16);
    ENFORCE_STACK_ALIGN_1D (uint8_t, iPred, 16 * FDEC_STRIDE, 16);
    ENFORCE_STACK_ALIGN_1D (uint8_t, iRec, 16 * FDEC_STRIDE, 16);
    for (int i = 0; i < 16; i++)
      for (int j = 0; j < 16; j++)
        iRefDst[i * FDEC_STRIDE + j] = iPred[i * FDEC_STRIDE + j] = rand() & 255;
    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        iRefDct[i][j] = iDct[i * 4 + j] = (rand() & ((1 << 15) - 1)) - (1 <<
                                          14); //2^14 limit, (2^15+32) will cause overflow for SSE2.
    WelsIDctRecI16x16DcAnchor (iRefDst, iRefDct);
    WelsIDctRecI16x16Dc_mmi (iRec, FDEC_STRIDE, iPred, FDEC_STRIDE, iDct);
    int ok = -1;
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 16; j++) {
        if (iRec[i * FDEC_STRIDE + j] != iRefDst[i * FDEC_STRIDE + j]) {
          ok = i * 16 + j;
          break;
        }
      }
    }
    EXPECT_EQ (ok, -1);
  }
}
#endif
