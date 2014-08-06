#include <gtest/gtest.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "cpu_core.h"
#include "cpu.h"
#include "macros.h"
#include "svc_motion_estimate.h"

using namespace WelsSVCEnc;
#define SVC_ME_TEST_NUM 10
static void FillWithRandomData (uint8_t* p, int32_t Len) {
  for (int32_t i = 0; i < Len; i++) {
    p[i] = rand() % 256;
  }
}

//preprocess related
int32_t SumOf8x8SingleBlock_ref (uint8_t* pRef, const int32_t kiRefStride) {
  int32_t iSum = 0, i;
  for (i = 0; i < 8; i++) {
    iSum +=  pRef[0]    + pRef[1]  + pRef[2]  + pRef[3];
    iSum +=  pRef[4]    + pRef[5]  + pRef[6]  + pRef[7];
    pRef += kiRefStride;
  }
  return iSum;
}
int32_t SumOf16x16SingleBlock_ref (uint8_t* pRef, const int32_t kiRefStride) {
  int32_t iSum = 0, i;
  for (i = 0; i < 16; i++) {
    iSum +=  pRef[0]    + pRef[1]  + pRef[2]  + pRef[3];
    iSum +=  pRef[4]    + pRef[5]  + pRef[6]  + pRef[7];
    iSum    +=  pRef[8]    + pRef[9]  + pRef[10]  + pRef[11];
    iSum    +=  pRef[12]  + pRef[13]  + pRef[14]  + pRef[15];
    pRef += kiRefStride;
  }
  return iSum;
}

void SumOf8x8BlockOfFrame_ref (uint8_t* pRefPicture, const int32_t kiWidth, const int32_t kiHeight,
                               const int32_t kiRefStride,
                               uint16_t* pFeatureOfBlock, uint32_t pTimesOfFeatureValue[]) {
  int32_t x, y;
  uint8_t* pRef;
  uint16_t* pBuffer;
  int32_t iSum;
  for (y = 0; y < kiHeight; y++) {
    pRef = pRefPicture  + kiRefStride * y;
    pBuffer  = pFeatureOfBlock + kiWidth * y;
    for (x = 0; x < kiWidth; x++) {
      iSum = SumOf8x8SingleBlock_c (pRef + x, kiRefStride);

      pBuffer[x] = iSum;
      pTimesOfFeatureValue[iSum]++;
    }
  }
}

void SumOf16x16BlockOfFrame_ref (uint8_t* pRefPicture, const int32_t kiWidth, const int32_t kiHeight,
                                 const int32_t kiRefStride,
                                 uint16_t* pFeatureOfBlock, uint32_t pTimesOfFeatureValue[]) {
  //TODO: this is similar to SumOf8x8BlockOfFrame_c expect the calling of single block func, refactor-able?
  int32_t x, y;
  uint8_t* pRef;
  uint16_t* pBuffer;
  int32_t iSum;
  for (y = 0; y < kiHeight; y++) {
    pRef = pRefPicture  + kiRefStride * y;
    pBuffer  = pFeatureOfBlock + kiWidth * y;
    for (x = 0; x < kiWidth; x++) {
      iSum = SumOf16x16SingleBlock_c (pRef + x, kiRefStride);

      pBuffer[x] = iSum;
      pTimesOfFeatureValue[iSum]++;
    }
  }
}

#define GENERATE_SumOfSingleBlock(anchor, method) \
TEST (SVC_ME_FunTest, method) {\
  ENFORCE_STACK_ALIGN_1D (uint8_t,  uiRefBuf,   16*320, 16);\
  int32_t iRes[2];\
  for (int32_t k = 0; k < SVC_ME_TEST_NUM; k++) {\
    FillWithRandomData (uiRefBuf,16*320);\
    iRes[0] = anchor (uiRefBuf,320);\
    iRes[1] = method (uiRefBuf,320);\
    ASSERT_EQ (iRes[0], iRes[1]);\
  }\
}

GENERATE_SumOfSingleBlock (SumOf8x8SingleBlock_ref, SumOf8x8SingleBlock_c)
GENERATE_SumOfSingleBlock (SumOf16x16SingleBlock_ref, SumOf16x16SingleBlock_c)

#ifdef HAVE_NEON
GENERATE_SumOfSingleBlock (SumOf8x8SingleBlock_ref, SumOf8x8SingleBlock_neon)
GENERATE_SumOfSingleBlock (SumOf16x16SingleBlock_ref, SumOf16x16SingleBlock_neon)
#endif

#ifdef HAVE_NEON_AARCH64
GENERATE_SumOfSingleBlock (SumOf8x8SingleBlock_ref, SumOf8x8SingleBlock_AArch64_neon)
GENERATE_SumOfSingleBlock (SumOf16x16SingleBlock_ref, SumOf16x16SingleBlock_AArch64_neon)
#endif


#define ENFORCE_NEW_ALIGN_1D(_tp, _nm, _nbuff, _sz, _al) \
_tp *_nbuff = new _tp[(_sz)+(_al)-1]; \
_tp *_nm = _nbuff + ((_al)-1) - (((uintptr_t)(_nbuff + ((_al)-1)) & ((_al)-1))/sizeof(_tp));

#define GENERATE_SumOfFrame(anchor, method, kiWidth, kiHeight) \
TEST (SVC_ME_FunTest, method##_##kiWidth##x##kiHeight) {\
ENFORCE_NEW_ALIGN_1D (uint8_t, pRefPicture, pRefPictureBuff, ((kiHeight+16)*((((kiWidth+15)>>4)<<4)+16)), 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t, pFeatureOfBlock1, pFeatureOfBlockBuff1, (kiWidth*kiHeight), 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t, pFeatureOfBlock2, pFeatureOfBlockBuff2, (kiWidth*kiHeight), 16) \
uint32_t pTimesOfFeatureValue[2][65536]; \
for (int32_t k = 0; k < SVC_ME_TEST_NUM; k++) {\
  FillWithRandomData (pRefPicture,(kiHeight+16)*((((kiWidth+15)>>4)<<4)+16));\
  memset(pTimesOfFeatureValue[0], 0, 65536*sizeof(uint32_t)); \
  memset(pTimesOfFeatureValue[1], 0, 65536*sizeof(uint32_t)); \
  anchor (pRefPicture,kiWidth,kiHeight,((((kiWidth+15)>>4)<<4)+16),pFeatureOfBlock1,pTimesOfFeatureValue[0]); \
  method (pRefPicture,kiWidth,kiHeight,((((kiWidth+15)>>4)<<4)+16),pFeatureOfBlock2,pTimesOfFeatureValue[1]); \
  for(int32_t j=0;j<kiWidth*kiHeight;j++){\
      ASSERT_EQ (pFeatureOfBlock1[j], pFeatureOfBlock2[j]);\
  }\
  for(int32_t  j=0;j<65536;j++){\
      ASSERT_EQ (pTimesOfFeatureValue[0][j], pTimesOfFeatureValue[1][j]);\
  }\
}\
delete[] pRefPictureBuff; \
delete[] pFeatureOfBlockBuff1; \
delete[] pFeatureOfBlockBuff2; \
}

GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_c, 1, 1)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_c, 1, 1)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_c, 1, 320)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_c, 1, 320)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_c, 640, 320)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_c, 640, 320)

#ifdef HAVE_NEON
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_neon, 1, 1)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_neon, 1, 1)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_neon, 1, 320)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_neon, 1, 320)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_neon, 640, 320)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_neon, 640, 320)
#endif

#ifdef HAVE_NEON_AARCH64
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_AArch64_neon, 1, 1)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_AArch64_neon, 1, 1)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_AArch64_neon, 1, 320)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_AArch64_neon, 1, 320)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_AArch64_neon, 640, 320)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_AArch64_neon, 640, 320)
#endif
