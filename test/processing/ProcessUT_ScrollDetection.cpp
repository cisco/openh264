#include <gtest/gtest.h>
#include <math.h>
#include <string.h>
#include "cpu.h"
#include "cpu_core.h"
#include "IWelsVP.h"
#include "ScrollDetection.h"
#include "ScrollDetectionFuncs.h"
#include "utils/DataGenerator.h"

using namespace WelsVP;

#define ASSERT_MEMORY_FAIL2X(A, B)     \
  if (NULL == B) {                     \
  delete []A;\
  ASSERT_TRUE(0);                    \
  }

TEST (ScrollDetectionTest, TestScroll) {
  unsigned char* pSrc, *pRef;
  int iWidthSets[4] = {640, 1024, 1280, 1980};
  int iHeightSets[4] = {360, 768, 720, 1080};
  int iStride = 0;

  for (int i = 0; i < 4; i++) {
    int iWidth = iWidthSets[i];
    int iHeight = iHeightSets[i];
    iStride = iWidth + 16;
    pSrc = new unsigned char[iHeight * iStride];
    ASSERT_TRUE (NULL != pSrc);
    pRef = new unsigned char[iHeight * iStride];
    ASSERT_MEMORY_FAIL2X (pSrc, pRef)
    RandomPixelDataGenerator (pRef, iWidth, iHeight, iStride);

    int iMvRange = iHeight / 3;
    int iScrollMv = rand() % (iMvRange << 1) - iMvRange;
    unsigned char* pSrcTmp = pSrc;
    unsigned char* pRefTmp = pRef;

    for (int j = 0; j < iHeight; j++) {
      if ((j + iScrollMv) >= 0 && (j + iScrollMv) < iHeight)
        for (int i = 0; i < iWidth; i++) {
          memcpy (pSrcTmp , &pRefTmp[ (j + iScrollMv)*iStride], iWidth * sizeof (unsigned char));
        }
      else {
        for (int i = 0; i < iWidth; i++)
          pSrcTmp[i] = rand() % 256;
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
    int iCoreNum = 1;
    unsigned int uiCPUFlag = WelsCPUFeatureDetect (&iCoreNum);

    CScrollDetection* pTest = new CScrollDetection (uiCPUFlag);
    int iMethodIdx = METHOD_SCROLL_DETECTION;

    pTest->Set (iMethodIdx, (&sScrollDetectionResult));
    int ret = pTest->Process (iMethodIdx, &sSrcMap, &sRefMap);
    EXPECT_EQ (ret, 0);
    pTest->Get (iMethodIdx, (&sScrollDetectionResult));

    EXPECT_EQ (sScrollDetectionResult.bScrollDetectFlag, true);
    EXPECT_EQ (sScrollDetectionResult.iScrollMvY, iScrollMv);

    delete pTest;
    delete []pSrc;
    delete []pRef;
  }
}
