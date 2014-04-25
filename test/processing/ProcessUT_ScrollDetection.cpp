#include <gtest/gtest.h>
#include <math.h>
#include <string.h>
#include "cpu.h"
#include "cpu_core.h"
#include "typedef.h"
#include "IWelsVP.h"
#include "ScrollDetection.h"
#include "ScrollDetectionFuncs.h"
#include "utils/DataGenerator.h"

using namespace nsWelsVP;

#define ASSERT_MEMORY_FAIL2X(A, B)     \
  if (NULL == B) {                     \
  delete []A;\
  ASSERT_TRUE(0);                    \
  }

TEST(ScrollDetectionTest,TestScroll)
{
  uint8_t* pSrc, *pRef;
  int32_t iWidthSets[4] = {640,1024,1280,1980};
  int32_t iHeightSets[4] = {360,768,720,1080};
  int32_t iStride = 0;
  int32_t iIdx = 0;

  for(int32_t i=0; i<4; i++){
    int32_t iWidth = iWidthSets[i];
    int32_t iHeight = iHeightSets[i];
    iStride = iWidth + 16;
    pSrc = new uint8_t[iHeight*iStride];
    ASSERT_TRUE(NULL != pSrc);
    pRef = new uint8_t[iHeight*iStride];
    ASSERT_MEMORY_FAIL2X(pSrc, pRef)
    RandomPixelDataGenerator(pRef, iWidth, iHeight, iStride, iIdx );

    int32_t iMvRange = iHeight/3;
    int32_t iScrollMv = rand()%(iMvRange<<1) - iMvRange;
    uint8_t* pSrcTmp = pSrc;
    uint8_t* pRefTmp = pRef;

    for (int32_t j=0;j<iHeight;j++) {
      if ((j+iScrollMv)>=0 && (j+iScrollMv)<iHeight)
        for (int32_t i=0;i<iWidth;i++) {
            memcpy(pSrcTmp , &pRefTmp[(j+iScrollMv)*iStride], iWidth*sizeof(uint8_t));
      } else {
        for (int32_t i=0;i<iWidth;i++)
          pSrcTmp[i] = rand()%256;
      }
      pSrcTmp += iStride;
    }


    SPixMap sSrcMap = { { 0 } };
    SPixMap sRefMap = { { 0 } };

    sSrcMap.pPixel[0] = pSrc;
    sRefMap.pPixel[0] = pRef;
    sSrcMap.iStride[0] = sRefMap.iStride[0] = iStride;
    sSrcMap.sRect.iRectWidth = sRefMap.sRect.iRectWidth = iWidth;
    sSrcMap.sRect.iRectHeight = sRefMap.sRect.iRectHeight = iHeight;

    SScrollDetectionParam sScrollDetectionResult;
    WelsMemset (&sScrollDetectionResult, 0, sizeof (sScrollDetectionResult));
    int32_t iCoreNum = 1;
    uint32_t uiCPUFlag = WelsCPUFeatureDetect (&iCoreNum);

    CScrollDetection *pTest =new CScrollDetection(uiCPUFlag);
    int32_t iMethodIdx = METHOD_SCROLL_DETECTION;

    pTest->Set(iMethodIdx, (&sScrollDetectionResult));
    int32_t ret = pTest->Process(iMethodIdx,&sSrcMap, &sRefMap);
    EXPECT_EQ(ret,0);
    pTest->Get(iMethodIdx, (&sScrollDetectionResult));

    EXPECT_EQ(sScrollDetectionResult.bScrollDetectFlag,1);
    EXPECT_EQ(sScrollDetectionResult.iScrollMvY,iScrollMv);

    delete pTest;
    delete []pSrc;
    delete []pRef;
  }
}
