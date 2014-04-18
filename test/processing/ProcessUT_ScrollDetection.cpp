#include <gtest/gtest.h>
#include <math.h>
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
//  uint8_t* src_a = new uint8_t[stride_pix_a<<3];
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

    int32_t iScrollMv = rand()%128;
    uint8_t* pSrcTmp = pSrc;
    uint8_t* pRefTmp = pRef;

    for (int32_t j=0;j<iHeight;j++) {
      if ((j+iScrollMv)>=0 && (j+iScrollMv)<iHeight)
        for (int32_t i=0;i<iWidth;i++) {
          pSrcTmp[i] = pRefTmp[(j+iScrollMv)*iStride+i];
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

    int32_t iStartX, iStartY;
    const int32_t kiPicBorderWidth= sSrcMap.sRect.iRectHeight>>4;
    const int32_t kiRegionWidth = (int) (sSrcMap.sRect.iRectWidth-(kiPicBorderWidth<<1))/3;
    const int32_t kiRegionHeight = (sSrcMap.sRect.iRectHeight*7)>>3;
    const int32_t kiHieghtStride = (int) sSrcMap.sRect.iRectHeight*5/24;
    SScrollDetectionParam m_sScrollDetectionParam;

    for (int32_t i=0; i< 9;i++){
    iStartX = kiPicBorderWidth+(i%3)*kiRegionWidth;
    iStartY = -sSrcMap.sRect.iRectHeight*7/48+ (int)(i/3)*(kiHieghtStride);
    iWidth = kiRegionWidth;
    iHeight = kiRegionHeight;

    iWidth /= 2;
    iStartX += iWidth/2;

    m_sScrollDetectionParam.iScrollMvX = 0;
    m_sScrollDetectionParam.iScrollMvY = 0;
    m_sScrollDetectionParam.bScrollDetectFlag = false;

    ScrollDetectionCore(&sSrcMap, &sRefMap, iWidth, iHeight, iStartX, iStartY, m_sScrollDetectionParam);

    if (m_sScrollDetectionParam.bScrollDetectFlag && m_sScrollDetectionParam.iScrollMvY)
      break;
   }

    EXPECT_EQ(m_sScrollDetectionParam.bScrollDetectFlag,1);
    EXPECT_EQ(m_sScrollDetectionParam.iScrollMvY,iScrollMv);

    delete []pSrc;
    delete []pRef;
  }
}
