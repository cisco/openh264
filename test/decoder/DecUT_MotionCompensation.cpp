#include <gtest/gtest.h>
#include "codec_def.h"
#include "mc.h"
#include "mem_align.h"
#include "cpu_core.h"
#include "cpu.h"
using namespace WelsDec;

#define MC_BUFF_SRC_STRIDE 32
#define MC_BUFF_DST_STRIDE 32
#define MC_BUFF_HEIGHT 30

/**********************MC Unit Test Anchor Code Begin******************************/
bool bQpelNeeded[4][4] = {
  { false, true, false, true },
  { true,  true,  true, true },
  { false, true, false, true },
  { true,  true,  true, true }
};
int32_t iHpelRef0Array[4][4] = {
  { 0, 1, 1, 1 },
  { 0, 1, 1, 1 },
  { 2, 3, 3, 3 },
  { 0, 1, 1, 1 }
};
int32_t iHpelRef1Array[4][4] = {
  { 0, 0, 0, 0 },
  { 2, 2, 3, 2 },
  { 2, 2, 3, 2 },
  { 2, 2, 3, 2 }
};
#define FILTER6TAP(pPixBuff, x, iStride) ((pPixBuff)[x-2*iStride] + (pPixBuff)[x+3*iStride] - 5*((pPixBuff)[x-iStride] + (pPixBuff)[x+2*iStride]) + 20*((pPixBuff)[x] + (pPixBuff)[x+iStride]))
static inline uint8_t Clip255 (int32_t x) {
  return ((x & ~255) ? (-x) >> 31 & 255 : x);
}

void MCCopyAnchor (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                   int32_t iHeight) {
  for (int32_t y = 0; y < iHeight; y++) {
    memcpy (pDst, pSrc, iWidth * sizeof (uint8_t));
    pSrc += iSrcStride;
    pDst += iDstStride;
  }
}

void MCHalfPelFilterAnchor (uint8_t* pDstH, uint8_t* pDstV, uint8_t* pDstHV, uint8_t* pSrc,
                            int32_t iStride, int32_t iWidth, int32_t iHeight, int16_t* pBuf) {
  for (int32_t y = 0; y < iHeight; y++) {
    for (int32_t x = 0; x < iWidth; x++)
      pDstH[x] = Clip255 ((FILTER6TAP (pSrc, x, 1) + 16) >> 5);
    for (int32_t x = -2; x < iWidth + 3; x++) {
      int32_t v = FILTER6TAP (pSrc, x, iStride);
      pDstV[x] = Clip255 ((v + 16) >> 5);
      pBuf[x + 2] = v;
    }
    for (int32_t x = 0; x < iWidth; x++)
      pDstHV[x] = Clip255 ((FILTER6TAP (pBuf + 2, x, 1) + 512) >> 10);
    pDstH += iStride;
    pDstV += iStride;
    pDstHV += iStride;
    pSrc += iStride;
  }
}

void PixelAvgAnchor (uint8_t* pDst,  int32_t iDstStride,
                     uint8_t* pSrc1, int32_t iSrc1Stride,
                     uint8_t* pSrc2, int32_t iSrc2Stride, int32_t iWidth, int32_t iHeight) {
  for (int32_t y = 0; y < iHeight; y++) {
    for (int32_t x = 0; x < iWidth; x++)
      pDst[x] = (pSrc1[x] + pSrc2[x] + 1) >> 1;
    pDst  += iDstStride;
    pSrc1 += iSrc1Stride;
    pSrc2 += iSrc2Stride;
  }
}

void MCLumaAnchor (uint8_t* pDst,    int32_t iDstStride, uint8_t* pSrc[4], int32_t iSrcStride,
                   int32_t iMvX, int32_t iMvY, int32_t iWidth, int32_t iHeight) {
  int32_t iMvXIdx = iMvX & 3;
  int32_t iMvYIdx = iMvY & 3;
  int32_t iOffset = (iMvY >> 2) * iSrcStride + (iMvX >> 2);
  uint8_t* pSrc1 = pSrc[iHpelRef0Array[iMvYIdx][iMvXIdx]] + iOffset + ((iMvYIdx) == 3) * iSrcStride;

  if (bQpelNeeded[iMvYIdx][iMvXIdx]) {
    uint8_t* pSrc2 = pSrc[iHpelRef1Array[iMvYIdx][iMvXIdx]] + iOffset + ((iMvXIdx) == 3);
    PixelAvgAnchor (pDst, iDstStride, pSrc1, iSrcStride, pSrc2, iSrcStride, iWidth, iHeight);
  } else {
    MCCopyAnchor (pSrc1, iSrcStride, pDst, iDstStride, iWidth, iHeight);
  }
}

void MCChromaAnchor (uint8_t* pDstU, uint8_t* pDstV, int32_t iDstStride, uint8_t* pSrc, int32_t iSrcStride,
                     int32_t iMvX, int32_t iMvY, int32_t iWidth, int32_t iHeight) {
  uint8_t* pSrcTmp;
  pSrc += (iMvY >> 3) * iSrcStride + (iMvX >> 3) * 2;
  pSrcTmp = &pSrc[iSrcStride];

  int32_t iMvXIdx = iMvX & 0x07;
  int32_t iMvYIdx = iMvY & 0x07;
  int32_t iBiPara0 = (8 - iMvXIdx) * (8 - iMvYIdx);
  int32_t iBiPara1 = iMvXIdx    * (8 - iMvYIdx);
  int32_t iBiPara2 = (8 - iMvXIdx) * iMvYIdx;
  int32_t iBiPara3 = iMvXIdx    * iMvYIdx;
  for (int32_t y = 0; y < iHeight; y++) {
    for (int32_t x = 0; x < iWidth; x++) {
      pDstU[x] = (iBiPara0 * pSrc[2 * x]  + iBiPara1 * pSrc[2 * x + 2] +
                  iBiPara2 * pSrcTmp[2 * x] + iBiPara3 * pSrcTmp[2 * x + 2] + 32) >> 6;
      pDstV[x] = (iBiPara0 * pSrc[2 * x + 1]  + iBiPara1 * pSrc[2 * x + 3] +
                  iBiPara2 * pSrcTmp[2 * x + 1] + iBiPara3 * pSrcTmp[2 * x + 3] + 32) >> 6;
    }
    pSrc   = pSrcTmp;
    pSrcTmp += iSrcStride;
    pDstU += iDstStride;
    pDstV += iDstStride;
  }
}

/**********************MC Unit Test OPENH264 Code Begin******************************/
#define DEF_MCCOPYTEST(iW,iH, forceC) \
TEST(McCopy_c,iW##x##iH) \
{                             \
    SMcFunc sMcFunc;      \
    int32_t iCpuCores = 1; \
    uint32_t uiCpuFlag;\
    for(int32_t k =0; k<2; k++)\
    {\
      if(k==0||forceC!=0)\
      {\
        uiCpuFlag = 0;\
      }else \
      {\
        uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores); \
      }\
      InitMcFunc(&sMcFunc, uiCpuFlag); \
      uint8_t uSrcAnchor[MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE]; \
      uint8_t uSrcTest[MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE];    \
      ENFORCE_STACK_ALIGN_2D(uint8_t, uDstAnchor, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16); \
      ENFORCE_STACK_ALIGN_2D(uint8_t, uDstTest, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16); \
      for(int32_t j=0;j<MC_BUFF_HEIGHT;j++)                    \
      {                                                         \
        for(int32_t i=0;i<MC_BUFF_SRC_STRIDE;i++)                  \
        {                                                       \
          uSrcAnchor[j][i] = uSrcTest[j][i] = rand()%256;      \
        }                                                         \
      }                                                              \
      memset(uDstAnchor,0,sizeof(uint8_t)*MC_BUFF_HEIGHT*MC_BUFF_DST_STRIDE);\
      memset(uDstTest,0,sizeof(uint8_t)*MC_BUFF_HEIGHT*MC_BUFF_DST_STRIDE);  \
      MCCopyAnchor(uSrcAnchor[0],MC_BUFF_SRC_STRIDE,uDstAnchor[0],MC_BUFF_DST_STRIDE,iW,iH);   \
      sMcFunc.pMcLumaFunc(uSrcTest[0],MC_BUFF_SRC_STRIDE,uDstTest[0],MC_BUFF_DST_STRIDE,0,0,iW,iH); \
      for(int32_t j=0;j<MC_BUFF_HEIGHT;j++)   \
      {                                                                             \
        for(int32_t i=0;i<MC_BUFF_DST_STRIDE;i++)                                  \
        {                                                                           \
          ASSERT_EQ(uDstAnchor[j][i],uDstTest[j][i]);                              \
        }                                                                             \
      }                                                                                 \
    }\
}

DEF_MCCOPYTEST (2, 2, 1)
DEF_MCCOPYTEST (2, 4, 1)
DEF_MCCOPYTEST (4, 2, 0)
DEF_MCCOPYTEST (4, 4, 0)
DEF_MCCOPYTEST (4, 8, 0)
DEF_MCCOPYTEST (8, 4, 0)
DEF_MCCOPYTEST (8, 8, 0)
DEF_MCCOPYTEST (16, 8, 0)
DEF_MCCOPYTEST (8, 16, 0)
DEF_MCCOPYTEST (16, 16, 0)

#define DEF_LUMA_MCTEST_SUBCASE(a,b,iW,iH) \
TEST(McHorVer##a##b##_c,iW##x##iH)  \
{                       \
    SMcFunc sMcFunc;  \
    uint8_t uSrcAnchor[4][MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE]; \
    uint8_t uSrcTest[MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE];      \
    ENFORCE_STACK_ALIGN_2D(uint8_t, uDstAnchor, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16); \
    ENFORCE_STACK_ALIGN_2D(uint8_t, uDstTest, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16); \
    uint8_t* uSrcInputAnchor[4];                              \
    int16_t pBuf[MC_BUFF_DST_STRIDE]; \
    uSrcInputAnchor[0] = &uSrcAnchor[0][4][4]; \
    uSrcInputAnchor[1] = &uSrcAnchor[1][4][4]; \
    uSrcInputAnchor[2] = &uSrcAnchor[2][4][4]; \
    uSrcInputAnchor[3] = &uSrcAnchor[3][4][4]; \
    for(int32_t j=0;j<MC_BUFF_HEIGHT;j++)   \
    {\
      for(int32_t i=0;i<MC_BUFF_SRC_STRIDE;i++)   \
      {\
        uSrcAnchor[0][j][i] = uSrcTest[j][i] = rand()%256;  \
      }\
    }\
    int32_t iCpuCores = 1; \
    uint32_t uiCpuFlag;\
    for(int32_t k =0; k<2; k++)\
    {\
      if(k==0)\
      {\
        uiCpuFlag = 0;\
      }else \
      {\
        uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores); \
      }\
      InitMcFunc(&sMcFunc,uiCpuFlag);\
      memset(uDstAnchor,0,sizeof(uint8_t)*MC_BUFF_HEIGHT*MC_BUFF_DST_STRIDE); \
      memset(uDstTest,0,sizeof(uint8_t)*MC_BUFF_HEIGHT*MC_BUFF_DST_STRIDE); \
      MCHalfPelFilterAnchor(uSrcInputAnchor[1],uSrcInputAnchor[2],uSrcInputAnchor[3],uSrcInputAnchor[0],MC_BUFF_SRC_STRIDE,iW+1,iH+1,pBuf+4); \
      MCLumaAnchor(uDstAnchor[0],MC_BUFF_DST_STRIDE,uSrcInputAnchor,MC_BUFF_SRC_STRIDE,a,b,iW,iH); \
      sMcFunc.pMcLumaFunc(&uSrcTest[4][4],MC_BUFF_SRC_STRIDE,uDstTest[0],MC_BUFF_DST_STRIDE,a,b,iW,iH);\
      for(int32_t j=0;j<MC_BUFF_HEIGHT;j++)   \
      {                                                                             \
          for(int32_t i=0;i<MC_BUFF_DST_STRIDE;i++)                                  \
          {                                                                           \
              ASSERT_EQ(uDstAnchor[j][i],uDstTest[j][i]);                              \
          }                                                                             \
      }                                                                                \
    }\
}

#define DEF_LUMA_MCTEST(a,b) \
DEF_LUMA_MCTEST_SUBCASE(a,b,4,4)  \
DEF_LUMA_MCTEST_SUBCASE(a,b,4,8)  \
DEF_LUMA_MCTEST_SUBCASE(a,b,8,4)  \
DEF_LUMA_MCTEST_SUBCASE(a,b,8,8)  \
DEF_LUMA_MCTEST_SUBCASE(a,b,16,8) \
DEF_LUMA_MCTEST_SUBCASE(a,b,8,16) \
DEF_LUMA_MCTEST_SUBCASE(a,b,16,16)

DEF_LUMA_MCTEST (0, 1)
DEF_LUMA_MCTEST (0, 2)
DEF_LUMA_MCTEST (0, 3)
DEF_LUMA_MCTEST (1, 0)
DEF_LUMA_MCTEST (1, 1)
DEF_LUMA_MCTEST (1, 2)
DEF_LUMA_MCTEST (1, 3)
DEF_LUMA_MCTEST (2, 0)
DEF_LUMA_MCTEST (2, 1)
DEF_LUMA_MCTEST (2, 2)
DEF_LUMA_MCTEST (2, 3)
DEF_LUMA_MCTEST (3, 0)
DEF_LUMA_MCTEST (3, 1)
DEF_LUMA_MCTEST (3, 2)
DEF_LUMA_MCTEST (3, 3)

#define DEF_CHROMA_MCTEST_SUBCASE(a,b,iW,iH) \
TEST(McChromaWithFragMv_##a##b##_c,iW##x##iH)  \
{                       \
    SMcFunc sMcFunc;  \
    uint8_t uSrcAnchor[MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE*2]; \
    uint8_t uSrcTest[MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE];      \
    ENFORCE_STACK_ALIGN_2D(uint8_t, uDstAnchor1, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16); \
    ENFORCE_STACK_ALIGN_2D(uint8_t, uDstAnchor2, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16); \
    ENFORCE_STACK_ALIGN_2D(uint8_t, uDstTest, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16); \
    for(int32_t j=0;j<MC_BUFF_HEIGHT;j++)   \
    {\
      for(int32_t i=0;i<MC_BUFF_SRC_STRIDE;i++)   \
      {\
        uSrcAnchor[j][i*2] = uSrcTest[j][i] = rand()%256;  \
      }\
    }\
    int32_t iCpuCores = 1; \
    uint32_t uiCpuFlag;\
    for(int32_t k =0; k<2; k++)\
    {\
      if(k==0)\
      {\
        uiCpuFlag = 0;\
      }else \
      {\
        uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores); \
      }\
      InitMcFunc(&sMcFunc,uiCpuFlag);\
      memset(uDstAnchor1,0,sizeof(uint8_t)*MC_BUFF_HEIGHT*MC_BUFF_DST_STRIDE); \
      memset(uDstAnchor2,0,sizeof(uint8_t)*MC_BUFF_HEIGHT*MC_BUFF_DST_STRIDE); \
      memset(uDstTest,0,sizeof(uint8_t)*MC_BUFF_HEIGHT*MC_BUFF_DST_STRIDE);     \
      MCChromaAnchor(uDstAnchor1[0],uDstAnchor2[0],MC_BUFF_DST_STRIDE,uSrcAnchor[0],MC_BUFF_SRC_STRIDE*2,a,b,iW,iH); \
      sMcFunc.pMcChromaFunc(uSrcTest[0],MC_BUFF_SRC_STRIDE,uDstTest[0],MC_BUFF_DST_STRIDE,a,b,iW,iH);\
      for(int32_t j=0;j<MC_BUFF_HEIGHT;j++)   \
      {                                                                             \
          for(int32_t i=0;i<MC_BUFF_DST_STRIDE;i++)                                  \
          {                                                                           \
              ASSERT_EQ(uDstAnchor1[j][i],uDstTest[j][i]);                             \
          }                                                                             \
      }                                                                                 \
    }\
}

#define DEF_CHROMA_MCTEST(a,b) \
DEF_CHROMA_MCTEST_SUBCASE(a,b,2,2) \
DEF_CHROMA_MCTEST_SUBCASE(a,b,2,4) \
DEF_CHROMA_MCTEST_SUBCASE(a,b,4,2) \
DEF_CHROMA_MCTEST_SUBCASE(a,b,4,4) \
DEF_CHROMA_MCTEST_SUBCASE(a,b,4,8) \
DEF_CHROMA_MCTEST_SUBCASE(a,b,8,4) \
DEF_CHROMA_MCTEST_SUBCASE(a,b,8,8)

DEF_CHROMA_MCTEST (0, 1)
DEF_CHROMA_MCTEST (0, 2)
DEF_CHROMA_MCTEST (0, 3)
DEF_CHROMA_MCTEST (0, 4)
DEF_CHROMA_MCTEST (0, 5)
DEF_CHROMA_MCTEST (0, 6)
DEF_CHROMA_MCTEST (0, 7)

DEF_CHROMA_MCTEST (1, 0)
DEF_CHROMA_MCTEST (1, 1)
DEF_CHROMA_MCTEST (1, 2)
DEF_CHROMA_MCTEST (1, 3)
DEF_CHROMA_MCTEST (1, 4)
DEF_CHROMA_MCTEST (1, 5)
DEF_CHROMA_MCTEST (1, 6)
DEF_CHROMA_MCTEST (1, 7)

DEF_CHROMA_MCTEST (2, 0)
DEF_CHROMA_MCTEST (2, 1)
DEF_CHROMA_MCTEST (2, 2)
DEF_CHROMA_MCTEST (2, 3)
DEF_CHROMA_MCTEST (2, 4)
DEF_CHROMA_MCTEST (2, 5)
DEF_CHROMA_MCTEST (2, 6)
DEF_CHROMA_MCTEST (2, 7)

DEF_CHROMA_MCTEST (3, 0)
DEF_CHROMA_MCTEST (3, 1)
DEF_CHROMA_MCTEST (3, 2)
DEF_CHROMA_MCTEST (3, 3)
DEF_CHROMA_MCTEST (3, 4)
DEF_CHROMA_MCTEST (3, 5)
DEF_CHROMA_MCTEST (3, 6)
DEF_CHROMA_MCTEST (3, 7)

DEF_CHROMA_MCTEST (4, 0)
DEF_CHROMA_MCTEST (4, 1)
DEF_CHROMA_MCTEST (4, 2)
DEF_CHROMA_MCTEST (4, 3)
DEF_CHROMA_MCTEST (4, 4)
DEF_CHROMA_MCTEST (4, 5)
DEF_CHROMA_MCTEST (4, 6)
DEF_CHROMA_MCTEST (4, 7)

DEF_CHROMA_MCTEST (5, 0)
DEF_CHROMA_MCTEST (5, 1)
DEF_CHROMA_MCTEST (5, 2)
DEF_CHROMA_MCTEST (5, 3)
DEF_CHROMA_MCTEST (5, 4)
DEF_CHROMA_MCTEST (5, 5)
DEF_CHROMA_MCTEST (5, 6)
DEF_CHROMA_MCTEST (5, 7)

DEF_CHROMA_MCTEST (6, 0)
DEF_CHROMA_MCTEST (6, 1)
DEF_CHROMA_MCTEST (6, 2)
DEF_CHROMA_MCTEST (6, 3)
DEF_CHROMA_MCTEST (6, 4)
DEF_CHROMA_MCTEST (6, 5)
DEF_CHROMA_MCTEST (6, 6)
DEF_CHROMA_MCTEST (6, 7)

DEF_CHROMA_MCTEST (7, 0)
DEF_CHROMA_MCTEST (7, 1)
DEF_CHROMA_MCTEST (7, 2)
DEF_CHROMA_MCTEST (7, 3)
DEF_CHROMA_MCTEST (7, 4)
DEF_CHROMA_MCTEST (7, 5)
DEF_CHROMA_MCTEST (7, 6)
DEF_CHROMA_MCTEST (7, 7)
