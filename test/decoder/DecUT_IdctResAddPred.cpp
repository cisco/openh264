#include <gtest/gtest.h>
#include <time.h>
#include "macros.h"
#include "../codec/decoder/core/inc/decode_mb_aux.h"
using namespace WelsDec;
void IdctResAddPred_ref (uint8_t* pPred, const int32_t kiStride, int16_t* pRs) {
  int16_t iSrc[16];

  uint8_t* pDst			= pPred;
  const int32_t kiStride2	= kiStride << 1;
  const int32_t kiStride3	= kiStride + kiStride2;
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
    int32_t kT1	= iSrc[i]	+ iSrc[i + 8];
    int32_t kT2	= iSrc[i + 4] + (iSrc[i + 12] >> 1);
    int32_t kT3	= (32 + kT1 + kT2) >> 6;
    int32_t kT4	= (32 + kT1 - kT2) >> 6;

    pDst[i] = WelsClip1 (kT3 + pPred[i]);
    pDst[i + kiStride3] = WelsClip1 (kT4 + pPred[i + kiStride3]);

    kT1	= iSrc[i] - iSrc[i + 8];
    kT2	= (iSrc[i + 4] >> 1) - iSrc[i + 12];
    pDst[i + kiStride] = WelsClip1 (((32 + kT1 + kT2) >> 6) + pDst[i + kiStride]);
    pDst[i + kiStride2] = WelsClip1 (((32 + kT1 - kT2) >> 6) + pDst[i + kiStride2]);
  }
}

#define GENERATE_IDCTRESADDPRED(pred) \
TEST(DecoderDecodeMbAux, pred) {\
  const int32_t kiStride = 32;\
  const int iBits = 12;\
  const int iMask = (1 << iBits) - 1;\
  const int iOffset = 1 << (iBits - 1);\
  int16_t iRS[16];\
  uint8_t uiPred[16*kiStride];\
  int16_t iRefRS[16];\
  uint8_t uiRefPred[16*kiStride];\
  int32_t iRunTimes = 1000;\
  srand((unsigned int)time(NULL));\
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

GENERATE_IDCTRESADDPRED (IdctResAddPred_c)
#if defined(X86_ASM)
GENERATE_IDCTRESADDPRED (IdctResAddPred_mmx)
#endif

#if defined(HAVE_NEON)
GENERATE_IDCTRESADDPRED (IdctResAddPred_neon)
#endif
