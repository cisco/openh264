#include <gtest/gtest.h>
#include "macros.h"
#include "decode_mb_aux.h"
#include "deblocking.h"
#include "cpu.h"
using namespace WelsDec;

namespace {

void IdctResAddPred_ref (uint8_t* pPred, const int32_t kiStride, int16_t* pRs) {
  int16_t iSrc[16];

  uint8_t* pDst             = pPred;
  const int32_t kiStride2   = kiStride << 1;
  const int32_t kiStride3   = kiStride + kiStride2;
  int32_t i;

  for (i = 0; i < 4; i++) {
    const int32_t kiY  = i << 2;
    const int32_t kiT0 = pRs[kiY] + pRs[kiY + 2];
    const int32_t kiT1 = pRs[kiY] - pRs[kiY + 2];
    const int32_t kiT2 = (pRs[kiY + 1] >> 1) - pRs[kiY + 3];
    const int32_t kiT3 = pRs[kiY + 1] + (pRs[kiY + 3] >> 1);

    iSrc[kiY] = kiT0 + kiT3;
    iSrc[kiY + 1] = kiT1 + kiT2;
    iSrc[kiY + 2] = kiT1 - kiT2;
    iSrc[kiY + 3] = kiT0 - kiT3;
  }

  for (i = 0; i < 4; i++) {
    int32_t kT1 = iSrc[i]     +  iSrc[i + 8];
    int32_t kT2 = iSrc[i + 4] + (iSrc[i + 12] >> 1);
    int32_t kT3 = (32 + kT1 + kT2) >> 6;
    int32_t kT4 = (32 + kT1 - kT2) >> 6;

    pDst[i] = WelsClip1 (kT3 + pPred[i]);
    pDst[i + kiStride3] = WelsClip1 (kT4 + pPred[i + kiStride3]);

    kT1 =  iSrc[i]           - iSrc[i + 8];
    kT2 = (iSrc[i + 4] >> 1) - iSrc[i + 12];
    pDst[i + kiStride] = WelsClip1 (((32 + kT1 + kT2) >> 6) + pDst[i + kiStride]);
    pDst[i + kiStride2] = WelsClip1 (((32 + kT1 - kT2) >> 6) + pDst[i + kiStride2]);
  }
}

void SetNonZeroCount_ref (int8_t* pNonZeroCount) {
  int32_t i;

  for (i = 0; i < 24; i++) {
    pNonZeroCount[i] = !!pNonZeroCount[i];
  }
}

void IdctResAddPred8x8_ref (uint8_t* pPred, const int32_t kiStride, int16_t* pRs) {
  int16_t p[8], b[8];
  int16_t a[4];

  int16_t iTmp[64];
  int16_t iRes[64];

  // Horizontal
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      p[j] = pRs[j + (i << 3)];
    }
    a[0] = p[0] + p[4];
    a[1] = p[0] - p[4];
    a[2] = p[6] - (p[2] >> 1);
    a[3] = p[2] + (p[6] >> 1);

    b[0] =  a[0] + a[3];
    b[2] =  a[1] - a[2];
    b[4] =  a[1] + a[2];
    b[6] =  a[0] - a[3];

    a[0] = -p[3] + p[5] - p[7] - (p[7] >> 1);
    a[1] =  p[1] + p[7] - p[3] - (p[3] >> 1);
    a[2] = -p[1] + p[7] + p[5] + (p[5] >> 1);
    a[3] =  p[3] + p[5] + p[1] + (p[1] >> 1);

    b[1] =  a[0] + (a[3] >> 2);
    b[3] =  a[1] + (a[2] >> 2);
    b[5] =  a[2] - (a[1] >> 2);
    b[7] =  a[3] - (a[0] >> 2);

    iTmp[0 + (i << 3)] = b[0] + b[7];
    iTmp[1 + (i << 3)] = b[2] - b[5];
    iTmp[2 + (i << 3)] = b[4] + b[3];
    iTmp[3 + (i << 3)] = b[6] + b[1];
    iTmp[4 + (i << 3)] = b[6] - b[1];
    iTmp[5 + (i << 3)] = b[4] - b[3];
    iTmp[6 + (i << 3)] = b[2] + b[5];
    iTmp[7 + (i << 3)] = b[0] - b[7];
  }

  //Vertical
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      p[j] = iTmp[i + (j << 3)];
    }

    a[0] =  p[0] + p[4];
    a[1] =  p[0] - p[4];
    a[2] =  p[6] - (p[2] >> 1);
    a[3] =  p[2] + (p[6] >> 1);

    b[0] = a[0] + a[3];
    b[2] = a[1] - a[2];
    b[4] = a[1] + a[2];
    b[6] = a[0] - a[3];

    a[0] = -p[3] + p[5] - p[7] - (p[7] >> 1);
    a[1] =  p[1] + p[7] - p[3] - (p[3] >> 1);
    a[2] = -p[1] + p[7] + p[5] + (p[5] >> 1);
    a[3] =  p[3] + p[5] + p[1] + (p[1] >> 1);


    b[1] =  a[0] + (a[3] >> 2);
    b[7] =  a[3] - (a[0] >> 2);
    b[3] =  a[1] + (a[2] >> 2);
    b[5] =  a[2] - (a[1] >> 2);

    iRes[ (0 << 3) + i] = b[0] + b[7];
    iRes[ (1 << 3) + i] = b[2] - b[5];
    iRes[ (2 << 3) + i] = b[4] + b[3];
    iRes[ (3 << 3) + i] = b[6] + b[1];
    iRes[ (4 << 3) + i] = b[6] - b[1];
    iRes[ (5 << 3) + i] = b[4] - b[3];
    iRes[ (6 << 3) + i] = b[2] + b[5];
    iRes[ (7 << 3) + i] = b[0] - b[7];
  }

  uint8_t* pDst = pPred;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      pDst[i * kiStride + j] = WelsClip1 (((32 + iRes[ (i << 3) + j]) >> 6) + pDst[i * kiStride + j]);
    }
  }

}

#if defined(X86_ASM)
#if defined(HAVE_AVX2)
void IdctFourResAddPred_ref (uint8_t* pPred, int32_t iStride, int16_t* pRs) {
  IdctResAddPred_ref (pPred + 0 * iStride + 0, iStride, pRs + 0 * 16);
  IdctResAddPred_ref (pPred + 0 * iStride + 4, iStride, pRs + 1 * 16);
  IdctResAddPred_ref (pPred + 4 * iStride + 0, iStride, pRs + 2 * 16);
  IdctResAddPred_ref (pPred + 4 * iStride + 4, iStride, pRs + 3 * 16);
}
#endif
#endif

} // anon ns

#define GENERATE_IDCTRESADDPRED(pred, flag) \
TEST(DecoderDecodeMbAux, pred) {\
  const int32_t kiStride = 32;\
  const int iBits = 12;\
  const int iMask = (1 << iBits) - 1;\
  const int iOffset = 1 << (iBits - 1);\
  ENFORCE_STACK_ALIGN_1D (int16_t, iRS, 16, 16);\
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiPred, 16 * kiStride, 16);\
  int16_t iRefRS[16];\
  uint8_t uiRefPred[16*kiStride];\
  int32_t iRunTimes = 1000;\
  uint32_t uiCPUFlags = WelsCPUFeatureDetect(NULL); \
  if ((uiCPUFlags & flag) == 0 && flag != 0) \
    return; \
  while(iRunTimes--) {\
    for(int i = 0; i < 4; i++)\
      for(int j = 0; j < 4; j++)\
        iRefRS[i*4+j] = iRS[i*4+j] = (rand() & iMask) - iOffset;\
    for(int i = 0; i < 4; i++)\
      for(int j = 0; j < 4; j++)\
        uiRefPred[i * kiStride + j] = uiPred[i * kiStride + j] = rand() & 255;\
    pred(uiPred, kiStride, iRS);\
    IdctResAddPred_ref(uiRefPred, kiStride, iRefRS);\
    bool ok = true;\
    for(int i = 0; i < 4; i++)\
      for(int j = 0; j < 4; j++)\
        if (uiRefPred[i * kiStride + j] != uiPred[i * kiStride + j]) {\
          ok = false;\
          goto next;\
        }\
    next:\
    EXPECT_EQ(ok, true);\
  }\
}

#define GENERATE_IDCTFOURRESADDPRED(pred, flag) \
TEST(DecoderDecodeMbAux, pred) {\
  const int32_t kiStride = 32;\
  const int iBits = 12;\
  const int iMask = (1 << iBits) - 1;\
  const int iOffset = 1 << (iBits - 1);\
  ENFORCE_STACK_ALIGN_1D (int16_t, iRS, 4 * 16, 16);\
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiPred, 4 * 16 * kiStride, 16);\
  int16_t iRefRS[4 * 16];\
  uint8_t uiRefPred[4 * 16 * kiStride];\
  int8_t iNzc[6] = { 0 };\
  int32_t iRunTimes = 1000;\
  uint32_t uiCPUFlags = WelsCPUFeatureDetect(0); \
  if ((uiCPUFlags & flag) == 0 && flag != 0) \
    return; \
  while (iRunTimes--) {\
    for (int i = 0; i < 4; i++)\
      for (int j = 0; j < 16; j++)\
        iNzc[i / 2 * 4 + i % 2] += !!(iRefRS[16 * i + j] = iRS[16 * i + j] = (rand() & iMask) - iOffset);\
    for (int i = 0; i < 8; i++)\
      for (int j = 0; j < 8; j++)\
        uiRefPred[i * kiStride + j] = uiPred[i * kiStride + j] = rand() & 255;\
    pred (uiPred, kiStride, iRS, iNzc);\
    IdctFourResAddPred_ref (uiRefPred, kiStride, iRefRS);\
    bool ok = true;\
    for (int i = 0; i < 8; i++)\
      for (int j = 0; j < 8; j++)\
        if (uiRefPred[i * kiStride + j] != uiPred[i * kiStride + j]) {\
          ok = false;\
          goto next;\
        }\
    next:\
    EXPECT_EQ(ok, true);\
  }\
}

GENERATE_IDCTRESADDPRED (IdctResAddPred_c, 0)
#if defined(X86_ASM)
GENERATE_IDCTRESADDPRED (IdctResAddPred_mmx, WELS_CPU_MMXEXT)
GENERATE_IDCTRESADDPRED (IdctResAddPred_sse2, WELS_CPU_SSE2)
#if defined(HAVE_AVX2)
GENERATE_IDCTRESADDPRED (IdctResAddPred_avx2, WELS_CPU_AVX2)
GENERATE_IDCTFOURRESADDPRED (IdctFourResAddPred_avx2, WELS_CPU_AVX2)
#endif
#endif

#if defined(HAVE_NEON)
GENERATE_IDCTRESADDPRED (IdctResAddPred_neon, WELS_CPU_NEON)
#endif

#if defined(HAVE_NEON_AARCH64)
GENERATE_IDCTRESADDPRED (IdctResAddPred_AArch64_neon, WELS_CPU_NEON)
#endif

#if defined(HAVE_MMI)
GENERATE_IDCTRESADDPRED (IdctResAddPred_mmi, WELS_CPU_MMI)
#endif

#if defined(HAVE_LSX)
GENERATE_IDCTRESADDPRED (IdctResAddPred_lsx, WELS_CPU_LSX)
#endif

#define GENERATE_SETNONZEROCOUNT(method, flag) \
TEST(DecoderDecodeMbAux, method) \
{\
    uint32_t uiCPUFlags = WelsCPUFeatureDetect(NULL); \
    if ((uiCPUFlags & flag) == 0 && flag != 0) \
        return; \
    int8_t iNonZeroCount[2][24];\
    for(int32_t i = 0; i < 24; i++) {\
        iNonZeroCount[0][i] = iNonZeroCount[1][i] = (rand() % 25);\
    }\
    method(iNonZeroCount[0]);\
    SetNonZeroCount_ref(iNonZeroCount[1]);\
    for(int32_t i =0; i<24; i++) {\
        ASSERT_EQ (iNonZeroCount[0][i], iNonZeroCount[1][i]);\
    }\
    for(int32_t i =0; i<24; i++) {\
        iNonZeroCount[0][i] = iNonZeroCount[1][i] = 0;\
    }\
    method(iNonZeroCount[0]);\
    SetNonZeroCount_ref(iNonZeroCount[1]);\
    for(int32_t i =0; i<24; i++) {\
        ASSERT_EQ (iNonZeroCount[0][i], iNonZeroCount[1][i]);\
    }\
    for(int32_t i =0; i<24; i++) {\
        iNonZeroCount[0][i] = iNonZeroCount[1][i] = 16;\
    }\
    method(iNonZeroCount[0]);\
    SetNonZeroCount_ref(iNonZeroCount[1]);\
    for(int32_t i =0; i<24; i++) {\
        ASSERT_EQ (iNonZeroCount[0][i], iNonZeroCount[1][i]);\
    }\
}

GENERATE_SETNONZEROCOUNT (WelsNonZeroCount_c, 0)

#if defined(X86_ASM)
GENERATE_SETNONZEROCOUNT (WelsNonZeroCount_sse2, WELS_CPU_SSE2)
#endif

#if defined(HAVE_NEON)
GENERATE_SETNONZEROCOUNT (WelsNonZeroCount_neon, WELS_CPU_NEON)
#endif

#if defined(HAVE_NEON_AARCH64)
GENERATE_SETNONZEROCOUNT (WelsNonZeroCount_AArch64_neon, WELS_CPU_NEON)
#endif

#if defined(HAVE_MSA)
GENERATE_SETNONZEROCOUNT (WelsNonZeroCount_msa, WELS_CPU_MSA)
#endif

#define GENERATE_IDCTRESADDPRED8x8(pred, flag) \
TEST(DecoderDecodeMbAux, pred) {\
  const int32_t kiStride = 32;\
  const int iBits = 12;\
  const int iMask = (1 << iBits) - 1;\
  const int iOffset = 1 << (iBits - 1);\
  ENFORCE_STACK_ALIGN_1D (int16_t, iRS, 64, 16);\
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiPred, 64 * kiStride, 16);\
  int16_t iRefRS[64];\
  uint8_t uiRefPred[64*kiStride];\
  int32_t iRunTimes = 1000;\
  uint32_t uiCPUFlags = WelsCPUFeatureDetect(NULL); \
  if ((uiCPUFlags & flag) == 0 && flag != 0) \
    return; \
  while(iRunTimes--) {\
    for(int i = 0; i < 8; i++)\
      for(int j = 0; j < 8; j++)\
        iRefRS[i*8+j] = iRS[i*8+j] = (rand() & iMask) - iOffset;\
    for(int i = 0; i < 8; i++)\
      for(int j = 0; j < 8; j++)\
        uiRefPred[i * kiStride + j] = uiPred[i * kiStride + j] = rand() & 255;\
    pred(uiPred, kiStride, iRS);\
    IdctResAddPred8x8_ref(uiRefPred, kiStride, iRefRS);\
    bool ok = true;\
    for(int i = 0; i < 8; i++)\
      for(int j = 0; j < 8; j++)\
        if (uiRefPred[i * kiStride + j] != uiPred[i * kiStride + j]) {\
          ok = false;\
          goto next;\
        }\
    next:\
    EXPECT_EQ(ok, true);\
  }\
}

GENERATE_IDCTRESADDPRED8x8 (IdctResAddPred8x8_c, 0);

#if defined(HAVE_LSX)
GENERATE_IDCTRESADDPRED8x8 (IdctResAddPred8x8_lsx, WELS_CPU_LSX)
#endif
