#include <gtest/gtest.h>
#include "codec_def.h"
#include "macros.h"
#include "mc.h"
#include "cpu.h"
using namespace WelsCommon;

#define MC_BUFF_SRC_STRIDE 32
#define MC_BUFF_DST_STRIDE 32
#define MC_BUFF_HEIGHT 30

/**********************MC Unit Test Anchor Code Begin******************************/
static bool bQpelNeeded[4][4] = {
  { false, true, false, true },
  { true,  true,  true, true },
  { false, true, false, true },
  { true,  true,  true, true }
};
static int32_t iHpelRef0Array[4][4] = {
  { 0, 1, 1, 1 },
  { 0, 1, 1, 1 },
  { 2, 3, 3, 3 },
  { 0, 1, 1, 1 }
};
static int32_t iHpelRef1Array[4][4] = {
  { 0, 0, 0, 0 },
  { 2, 2, 3, 2 },
  { 2, 2, 3, 2 },
  { 2, 2, 3, 2 }
};
#define FILTER6TAP(pPixBuff, x, iStride) ((pPixBuff)[x-2*iStride] + (pPixBuff)[x+3*iStride] - 5*((pPixBuff)[x-iStride] + (pPixBuff)[x+2*iStride]) + 20*((pPixBuff)[x] + (pPixBuff)[x+iStride]))
static inline uint8_t Clip255 (int32_t x) {
  return ((x & ~255) ? (-x) >> 31 & 255 : x);
}

static void MCCopyAnchor (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                          int32_t iHeight) {
  for (int32_t y = 0; y < iHeight; y++) {
    memcpy (pDst, pSrc, iWidth * sizeof (uint8_t));
    pSrc += iSrcStride;
    pDst += iDstStride;
  }
}

static void MCHalfPelFilterAnchor (uint8_t* pDstH, uint8_t* pDstV, uint8_t* pDstHV, uint8_t* pSrc,
                                   int32_t iStride, int32_t iWidth, int32_t iHeight, int16_t* pBuf) {
  for (int32_t y = 0; y < iHeight; y++) {
    for (int32_t x = 0; x < iWidth; x++)
      pDstH[x] = Clip255 ((FILTER6TAP (pSrc, x, 1) + 16) >> 5);
    for (int32_t x = -2; x < iWidth + 3; x++) {
      int32_t v = FILTER6TAP (pSrc, x, iStride);
      if (x >= 0 && x < iWidth)
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

static void PixelAvgAnchor (uint8_t* pDst,  int32_t iDstStride,
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

static void MCLumaAnchor (uint8_t* pDst, int32_t iDstStride, uint8_t* pSrc[4], int32_t iSrcStride,
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

static void MCChromaAnchor (uint8_t* pDstU, uint8_t* pDstV, int32_t iDstStride, uint8_t* pSrc, int32_t iSrcStride,
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
#define DEF_MCCOPYTEST(iW,iH) \
TEST(McCopy_c,iW##x##iH) \
{                             \
    SMcFunc sMcFunc;      \
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

DEF_MCCOPYTEST (2, 2)
DEF_MCCOPYTEST (2, 4)
DEF_MCCOPYTEST (4, 2)
DEF_MCCOPYTEST (4, 4)
DEF_MCCOPYTEST (4, 8)
DEF_MCCOPYTEST (8, 4)
DEF_MCCOPYTEST (8, 8)
DEF_MCCOPYTEST (16, 8)
DEF_MCCOPYTEST (8, 16)
DEF_MCCOPYTEST (16, 16)

#define DEF_LUMA_MCTEST(iW,iH) \
TEST(McHorVer,iW##x##iH)  \
{                       \
    for (int32_t a = 0; a < 4; a++) { \
    for (int32_t b = 0; b < 4; b++) { \
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
    }\
    }\
}


DEF_LUMA_MCTEST (4, 4)
DEF_LUMA_MCTEST (4, 8)
DEF_LUMA_MCTEST (8, 4)
DEF_LUMA_MCTEST (8, 8)
DEF_LUMA_MCTEST (16, 8)
DEF_LUMA_MCTEST (8, 16)
DEF_LUMA_MCTEST (16, 16)

#define DEF_CHROMA_MCTEST(iW,iH) \
TEST(McChroma,iW##x##iH)  \
{                       \
    for (int32_t a = 0; a < 8; a++) { \
    for (int32_t b = 0; b < 8; b++) { \
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
    }\
    }\
}

DEF_CHROMA_MCTEST (2, 2)
DEF_CHROMA_MCTEST (2, 4)
DEF_CHROMA_MCTEST (4, 2)
DEF_CHROMA_MCTEST (4, 4)
DEF_CHROMA_MCTEST (4, 8)
DEF_CHROMA_MCTEST (8, 4)
DEF_CHROMA_MCTEST (8, 8)

TEST (EncMcAvg, PixelAvg) {
  SMcFunc sMcFunc;
  for (int32_t k = 0; k < 2; k++) {
    for (int32_t w = 0; w < 2; w++) {
      int32_t width = 8 << w;
      int32_t height = 16;
      uint32_t uiCpuFlag = k == 0 ? 0 : WelsCPUFeatureDetect (NULL);
      InitMcFunc (&sMcFunc, uiCpuFlag);
      uint8_t uSrc1[MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE];
      uint8_t uSrc2[MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE];
      ENFORCE_STACK_ALIGN_2D (uint8_t, uDstAnchor, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16);
      ENFORCE_STACK_ALIGN_2D (uint8_t, uDstTest, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16);
      for (int32_t j = 0; j < MC_BUFF_HEIGHT; j++) {
        for (int32_t i = 0; i < MC_BUFF_SRC_STRIDE; i++) {
          uSrc1[j][i] = rand() % 256;
          uSrc2[j][i] = rand() % 256;
        }
      }
      PixelAvgAnchor (uDstAnchor[0], MC_BUFF_DST_STRIDE, uSrc1[0], MC_BUFF_SRC_STRIDE, uSrc2[0], MC_BUFF_SRC_STRIDE, width,
                      height);
      sMcFunc.pfSampleAveraging (uDstTest[0], MC_BUFF_DST_STRIDE, uSrc1[0], MC_BUFF_SRC_STRIDE, uSrc2[0],
                                 MC_BUFF_SRC_STRIDE, width, height);
      for (int32_t j = 0; j < height; j++) {
        for (int32_t i = 0; i < width; i++) {
          ASSERT_EQ (uDstAnchor[j][i], uDstTest[j][i]);
        }
      }
    }
  }
}

#define DEF_HALFPEL_MCTEST(iW,iH) \
TEST (EncMcHalfpel, iW##x##iH) { \
    SMcFunc sMcFunc; \
    for (int32_t k = 0; k < 2; k++) { \
        for (int32_t w = 0; w < 2; w++) { \
            int32_t width = iW ; \
            int32_t height = iH; \
            uint8_t uAnchor[4][MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE]; \
            uint8_t uSrcTest[MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE]; \
            ENFORCE_STACK_ALIGN_2D (uint8_t, uDstTest, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16); \
            uint8_t* uAnchors[4]; \
            int16_t pBuf[MC_BUFF_DST_STRIDE]; \
            uAnchors[0] = &uAnchor[0][4][4]; \
            uAnchors[1] = &uAnchor[1][4][4]; \
            uAnchors[2] = &uAnchor[2][4][4]; \
            uAnchors[3] = &uAnchor[3][4][4]; \
             \
            memset (uAnchor, 0, 4 * sizeof (uint8_t)*MC_BUFF_HEIGHT * MC_BUFF_SRC_STRIDE); \
            memset (uDstTest, 0, sizeof (uint8_t)*MC_BUFF_HEIGHT * MC_BUFF_DST_STRIDE); \
            for (int32_t j = 0; j < MC_BUFF_HEIGHT; j++) { \
                for (int32_t i = 0; i < MC_BUFF_SRC_STRIDE; i++) { \
                    uAnchor[0][j][i] = uSrcTest[j][i] = rand() % 256; \
                } \
            } \
             \
            uint32_t uiCpuFlag = k == 0 ? 0 : WelsCPUFeatureDetect (NULL); \
            InitMcFunc (&sMcFunc, uiCpuFlag); \
             \
            MCHalfPelFilterAnchor (uAnchors[1], uAnchors[2], uAnchors[3], uAnchors[0], MC_BUFF_SRC_STRIDE, width + 1, height + 1, pBuf + 4); \
            sMcFunc.pfLumaHalfpelHor (&uSrcTest[4][4], MC_BUFF_SRC_STRIDE, uDstTest[0], MC_BUFF_DST_STRIDE, width + 1, height); \
            for (int32_t j = 0; j < height; j++) { \
                for (int32_t i = 0; i < width + 1; i++) { \
                    ASSERT_EQ (uAnchor[1][4 + j][4 + i], uDstTest[j][i]); \
                } \
            } \
            sMcFunc.pfLumaHalfpelVer (&uSrcTest[4][4], MC_BUFF_SRC_STRIDE, uDstTest[0], MC_BUFF_DST_STRIDE, width, height + 1); \
            for (int32_t j = 0; j < height + 1; j++) { \
                for (int32_t i = 0; i < width; i++) { \
                    ASSERT_EQ (uAnchor[2][4 + j][4 + i], uDstTest[j][i]); \
                } \
            } \
            sMcFunc.pfLumaHalfpelCen (&uSrcTest[4][4], MC_BUFF_SRC_STRIDE, uDstTest[0], MC_BUFF_DST_STRIDE, width + 1, height + 1); \
            for (int32_t j = 0; j < height + 1; j++) { \
                for (int32_t i = 0; i < width + 1; i++) { \
                    ASSERT_EQ (uAnchor[3][4 + j][4 + i], uDstTest[j][i]); \
                } \
            } \
        } \
    } \
}

DEF_HALFPEL_MCTEST(4,4)
DEF_HALFPEL_MCTEST(4,8)
DEF_HALFPEL_MCTEST(8,4)
DEF_HALFPEL_MCTEST(8,8)
DEF_HALFPEL_MCTEST(8,16)
DEF_HALFPEL_MCTEST(16,8)
DEF_HALFPEL_MCTEST(16,16)
