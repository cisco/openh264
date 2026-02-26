#include <gtest/gtest.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "cpu_core.h"
#include "cpu.h"
#include "macros.h"
#include "ls_defines.h"
#include "svc_motion_estimate.h"

using namespace WelsEnc;
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


void InitializeHashforFeature_ref (uint32_t* pTimesOfFeatureValue, uint16_t* pBuf, const int32_t kiListSize,
                                   uint16_t** pLocationOfFeature, uint16_t** pFeatureValuePointerList) {
  //assign location pointer
  uint16_t* pBufPos  = pBuf;
  for (int32_t i = 0 ; i < kiListSize; ++i) {
    pLocationOfFeature[i] =
      pFeatureValuePointerList[i] = pBufPos;
    pBufPos      += (pTimesOfFeatureValue[i] << 1);
  }
}
void FillQpelLocationByFeatureValue_ref (uint16_t* pFeatureOfBlock, const int32_t kiWidth, const int32_t kiHeight,
    uint16_t** pFeatureValuePointerList) {
  //assign each pixel's position
  uint16_t* pSrcPointer  =  pFeatureOfBlock;
  int32_t iQpelY = 0;
  for (int32_t y = 0; y < kiHeight; y++) {
    for (int32_t x = 0; x < kiWidth; x++) {
      uint16_t uiFeature = pSrcPointer[x];
      pFeatureValuePointerList[uiFeature][0] = x << 2;
      pFeatureValuePointerList[uiFeature][1] = iQpelY;
      pFeatureValuePointerList[uiFeature] += 2;
    }
    iQpelY += 4;
    pSrcPointer += kiWidth;
  }
}

#define GENERATE_SumOfSingleBlock(anchor, method, flag) \
TEST (SVC_ME_FunTest, method) {\
  uint32_t uiCPUFlags = WelsCPUFeatureDetect(NULL); \
  if ((uiCPUFlags & flag) == 0 && flag != 0) \
    return; \
  ENFORCE_STACK_ALIGN_1D (uint8_t,  uiRefBuf,   16*320, 16);\
  int32_t iRes[2];\
  for (int32_t k = 0; k < SVC_ME_TEST_NUM; k++) {\
    FillWithRandomData (uiRefBuf,16*320);\
    iRes[0] = anchor (uiRefBuf,320);\
    iRes[1] = method (uiRefBuf,320);\
    ASSERT_EQ (iRes[0], iRes[1]);\
  }\
}

GENERATE_SumOfSingleBlock (SumOf8x8SingleBlock_ref, SumOf8x8SingleBlock_c, 0)
GENERATE_SumOfSingleBlock (SumOf16x16SingleBlock_ref, SumOf16x16SingleBlock_c, 0)

#ifdef X86_ASM
GENERATE_SumOfSingleBlock (SumOf8x8SingleBlock_ref, SumOf8x8SingleBlock_sse2, WELS_CPU_SSE2)
GENERATE_SumOfSingleBlock (SumOf16x16SingleBlock_ref, SumOf16x16SingleBlock_sse2, WELS_CPU_SSE2)
#endif

#ifdef HAVE_NEON
GENERATE_SumOfSingleBlock (SumOf8x8SingleBlock_ref, SumOf8x8SingleBlock_neon, WELS_CPU_NEON)
GENERATE_SumOfSingleBlock (SumOf16x16SingleBlock_ref, SumOf16x16SingleBlock_neon, WELS_CPU_NEON)
#endif

#if defined(HAVE_NEON_AARCH64) && defined(__aarch64__)
GENERATE_SumOfSingleBlock (SumOf8x8SingleBlock_ref, SumOf8x8SingleBlock_AArch64_neon, WELS_CPU_NEON)
GENERATE_SumOfSingleBlock (SumOf16x16SingleBlock_ref, SumOf16x16SingleBlock_AArch64_neon, WELS_CPU_NEON)
#endif

#ifdef HAVE_LSX
GENERATE_SumOfSingleBlock (SumOf8x8SingleBlock_ref, SumOf8x8SingleBlock_lsx, WELS_CPU_LSX)
#endif

#define ENFORCE_NEW_ALIGN_1D(_tp, _nm, _nbuff, _sz, _al) \
_tp *_nbuff = new _tp[(_sz)+(_al)-1]; \
_tp *_nm = _nbuff + ((_al)-1) - (((uintptr_t)(_nbuff + ((_al)-1)) & ((_al)-1))/sizeof(_tp));

#define GENERATE_SumOfFrame(anchor, method, kiWidth, kiHeight, flag) \
TEST (SVC_ME_FunTest, method##_##kiWidth##x##kiHeight) {\
uint32_t uiCPUFlags = WelsCPUFeatureDetect(NULL); \
if ((uiCPUFlags & flag) == 0 && flag != 0) \
  return; \
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

#define GENERATE_InitializeHashforFeature(anchor, method, kiWidth, kiHeight, flag) \
TEST (SVC_ME_FunTest, method##_##kiWidth##x##kiHeight) {\
uint32_t uiCPUFlags = WelsCPUFeatureDetect(NULL); \
if ((uiCPUFlags & flag) == 0 && flag != 0) \
  return; \
ENFORCE_NEW_ALIGN_1D (uint8_t, pRefPicture, pRefPictureBuff, ((kiHeight+16)*((((kiWidth+15)>>4)<<4)+16)), 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t, pFeatureOfBlock, pFeatureOfBlockBuff, (kiWidth*kiHeight), 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t, pLocation1, pLocationBuff1, (kiWidth*kiHeight)*2, 16) \
ENFORCE_NEW_ALIGN_1D (uint32_t, pTimesOfFeatureValue, pTimesOfFeatureValueBuff, 65536, 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t*, pLocationFeature0, pLocationFeature0Buff, 65536, 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t*, pLocationFeature1, pLocationFeature1Buff, 65536, 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t*, pFeaturePointValueList0, pFeaturePointValueList0Buff, 65536, 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t*, pFeaturePointValueList1, pFeaturePointValueList1Buff, 65536, 16) \
for (int32_t k = 0; k < SVC_ME_TEST_NUM; k++) { \
  FillWithRandomData (pRefPicture,(kiHeight+16)*((((kiWidth+15)>>4)<<4)+16)); \
  memset(pTimesOfFeatureValue, 0, 65536*sizeof(uint32_t)); \
  memset(pLocationFeature0, 0, 65536*sizeof(uint16_t*)); \
  memset(pFeaturePointValueList0, 0, 65536*sizeof(uint16_t*)); \
  memset(pLocationFeature1, 0, 65536*sizeof(uint16_t*)); \
  memset(pFeaturePointValueList1, 0, 65536*sizeof(uint16_t*)); \
  SumOf8x8BlockOfFrame_c (pRefPicture,kiWidth,kiHeight,((((kiWidth+15)>>4)<<4)+16),pFeatureOfBlock,pTimesOfFeatureValue); \
  int32_t iActSize = 65536;\
  anchor ( pTimesOfFeatureValue, pLocation1, iActSize, pLocationFeature0, pFeaturePointValueList0);\
  method ( pTimesOfFeatureValue, pLocation1, iActSize, pLocationFeature1, pFeaturePointValueList1); \
  for(int32_t j =0; j<65536; j++) { \
    EXPECT_EQ (pLocationFeature0[j], pLocationFeature1[j]); \
    EXPECT_EQ (pFeaturePointValueList0[j], pFeaturePointValueList1[j]); \
  } \
} \
delete[] pRefPictureBuff; \
delete[] pFeatureOfBlockBuff; \
delete[] pLocationBuff1; \
delete[] pTimesOfFeatureValueBuff; \
delete[] pLocationFeature0Buff; \
delete[] pFeaturePointValueList0Buff; \
delete[] pLocationFeature1Buff; \
delete[] pFeaturePointValueList1Buff; \
}


#define GENERATE_FillQpelLocationByFeatureValue(anchor, method, kiWidth, kiHeight, flag) \
TEST (SVC_ME_FunTest, method##_##kiWidth##x##kiHeight) {\
uint32_t uiCPUFlags = WelsCPUFeatureDetect(NULL); \
if ((uiCPUFlags & flag) == 0 && flag != 0) \
  return; \
ENFORCE_NEW_ALIGN_1D (uint8_t, pRefPicture, pRefPictureBuff, ((kiHeight+16)*((((kiWidth+15)>>4)<<4)+16)), 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t, pFeatureOfBlock, pFeatureOfBlockBuff, (kiWidth*kiHeight), 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t, pLocation1, pLocationBuff1, (kiWidth*kiHeight)*2, 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t, pLocation2, pLocationBuff2, (kiWidth*kiHeight)*2, 16) \
ENFORCE_NEW_ALIGN_1D (uint32_t, pTimesOfFeatureValue, pTimesOfFeatureValueBuff, 65536, 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t*, pLocationFeature0, pLocationFeature0Buff, 65536, 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t*, pLocationFeature1, pLocationFeature1Buff, 65536, 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t*, pFeaturePointValueList0, pFeaturePointValueList0Buff, 65536, 16) \
ENFORCE_NEW_ALIGN_1D (uint16_t*, pFeaturePointValueList1, pFeaturePointValueList1Buff, 65536, 16) \
for (int32_t k = 0; k < SVC_ME_TEST_NUM; k++) { \
  FillWithRandomData (pRefPicture,(kiHeight+16)*((((kiWidth+15)>>4)<<4)+16)); \
  memset(pTimesOfFeatureValue, 0, 65536*sizeof(uint32_t)); \
  memset(pLocationFeature0, 0, 65536*sizeof(uint16_t*)); \
  memset(pFeaturePointValueList0, 0, 65536*sizeof(uint16_t*)); \
  memset(pLocationFeature1, 0, 65536*sizeof(uint16_t*)); \
  memset(pFeaturePointValueList1, 0, 65536*sizeof(uint16_t*)); \
  SumOf8x8BlockOfFrame_c (pRefPicture,kiWidth,kiHeight,((((kiWidth+15)>>4)<<4)+16),pFeatureOfBlock,pTimesOfFeatureValue); \
  int32_t iActSize = 65536; \
  InitializeHashforFeature_c ( pTimesOfFeatureValue, pLocation1, iActSize, pLocationFeature0, pFeaturePointValueList0); \
  InitializeHashforFeature_c( pTimesOfFeatureValue, pLocation2, iActSize, pLocationFeature1, pFeaturePointValueList1); \
  anchor(pFeatureOfBlock, kiWidth, kiHeight, pFeaturePointValueList0); \
  method(pFeatureOfBlock, kiWidth, kiHeight, pFeaturePointValueList1); \
  for(int32_t j =0; j<kiWidth*kiHeight*2; j++) { \
    EXPECT_EQ (pLocation1[j], pLocation2[j]); \
  } \
} \
delete[] pRefPictureBuff; \
delete[] pFeatureOfBlockBuff; \
delete[] pLocationBuff1; \
delete[] pLocationBuff2; \
delete[] pTimesOfFeatureValueBuff; \
delete[] pLocationFeature0Buff; \
delete[] pFeaturePointValueList0Buff; \
delete[] pLocationFeature1Buff; \
delete[] pFeaturePointValueList1Buff; \
}

GENERATE_InitializeHashforFeature (InitializeHashforFeature_ref, InitializeHashforFeature_c, 10, 10, 0)
GENERATE_FillQpelLocationByFeatureValue (FillQpelLocationByFeatureValue_ref, FillQpelLocationByFeatureValue_c, 16, 16, 0)
GENERATE_InitializeHashforFeature (InitializeHashforFeature_ref, InitializeHashforFeature_c, 640, 320, 0)
GENERATE_FillQpelLocationByFeatureValue (FillQpelLocationByFeatureValue_ref, FillQpelLocationByFeatureValue_c, 640, 320, 0)
#ifdef X86_ASM
GENERATE_InitializeHashforFeature (InitializeHashforFeature_ref, InitializeHashforFeature_sse2, 10, 10, WELS_CPU_SSE2)
GENERATE_FillQpelLocationByFeatureValue (FillQpelLocationByFeatureValue_ref, FillQpelLocationByFeatureValue_sse2, 16,
    16, WELS_CPU_SSE2)
GENERATE_InitializeHashforFeature (InitializeHashforFeature_ref, InitializeHashforFeature_sse2, 640, 320, WELS_CPU_SSE2)
GENERATE_FillQpelLocationByFeatureValue (FillQpelLocationByFeatureValue_ref, FillQpelLocationByFeatureValue_sse2, 640,
    320, WELS_CPU_SSE2)
#endif

GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_c, 1, 1, 0)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_c, 1, 1, 0)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_c, 1, 320, 0)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_c, 1, 320, 0)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_c, 640, 320, 0)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_c, 640, 320, 0)

#ifdef X86_ASM
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_sse2, 6, 6, WELS_CPU_SSE2)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_sse2, 6, 6, WELS_CPU_SSE2)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_sse2, 6, 320, WELS_CPU_SSE2)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_sse2, 6, 320, WELS_CPU_SSE2)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_sse2, 640, 320, WELS_CPU_SSE2)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_sse2, 640, 320, WELS_CPU_SSE2)

GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_sse4, 8, 2, WELS_CPU_SSE41)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_sse4, 16, 2, WELS_CPU_SSE41)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_sse4, 8, 320, WELS_CPU_SSE41)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_sse4, 16, 320, WELS_CPU_SSE41)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_sse4, 640, 320, WELS_CPU_SSE41)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_sse4, 640, 320, WELS_CPU_SSE41)
#endif

#ifdef HAVE_NEON
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_neon, 1, 1, WELS_CPU_NEON)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_neon, 1, 1, WELS_CPU_NEON)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_neon, 1, 320, WELS_CPU_NEON)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_neon, 1, 320, WELS_CPU_NEON)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_neon, 640, 320, WELS_CPU_NEON)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_neon, 640, 320, WELS_CPU_NEON)
GENERATE_InitializeHashforFeature (InitializeHashforFeature_ref, InitializeHashforFeature_neon, 10, 10, WELS_CPU_NEON)
GENERATE_FillQpelLocationByFeatureValue (FillQpelLocationByFeatureValue_ref, FillQpelLocationByFeatureValue_neon, 16,
    16, WELS_CPU_NEON)
GENERATE_InitializeHashforFeature (InitializeHashforFeature_ref, InitializeHashforFeature_neon, 640, 320, WELS_CPU_NEON)
GENERATE_FillQpelLocationByFeatureValue (FillQpelLocationByFeatureValue_ref, FillQpelLocationByFeatureValue_neon, 640,
    320, WELS_CPU_NEON)
#endif

#if defined(HAVE_NEON_AARCH64) && defined(__aarch64__)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_AArch64_neon, 1, 1, WELS_CPU_NEON)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_AArch64_neon, 1, 1, WELS_CPU_NEON)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_AArch64_neon, 1, 320, WELS_CPU_NEON)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_AArch64_neon, 1, 320, WELS_CPU_NEON)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_AArch64_neon, 640, 320, WELS_CPU_NEON)
GENERATE_SumOfFrame (SumOf16x16BlockOfFrame_ref, SumOf16x16BlockOfFrame_AArch64_neon, 640, 320, WELS_CPU_NEON)
GENERATE_InitializeHashforFeature (InitializeHashforFeature_ref, InitializeHashforFeature_AArch64_neon, 10, 10,
    WELS_CPU_NEON)
GENERATE_FillQpelLocationByFeatureValue (FillQpelLocationByFeatureValue_ref,
    FillQpelLocationByFeatureValue_AArch64_neon, 16, 16, WELS_CPU_NEON)
GENERATE_InitializeHashforFeature (InitializeHashforFeature_ref, InitializeHashforFeature_AArch64_neon, 640, 320,
    WELS_CPU_NEON)
GENERATE_FillQpelLocationByFeatureValue (FillQpelLocationByFeatureValue_ref,
    FillQpelLocationByFeatureValue_AArch64_neon, 640, 320, WELS_CPU_NEON)
#endif

#ifdef HAVE_LSX
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_lsx, 1, 1, WELS_CPU_LSX)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_lsx, 1, 320, WELS_CPU_LSX)
GENERATE_SumOfFrame (SumOf8x8BlockOfFrame_ref, SumOf8x8BlockOfFrame_lsx, 640, 320, WELS_CPU_LSX)
#endif
