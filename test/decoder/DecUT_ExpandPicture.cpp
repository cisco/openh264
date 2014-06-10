#include<gtest/gtest.h>
#include "codec_def.h"
#include "expand_pic.h"
#include "mem_align.h"
#include "decoder_context.h"
#include "cpu.h"
using namespace WelsDec;
#define EXPAND_PIC_TEST_NUM 10
namespace WelsDec {
extern PPicture AllocPicture (PWelsDecoderContext pCtx, const int32_t kPicWidth, const int32_t kPicHeight);
extern void FreePicture (PPicture pPic);
}
#define H264_PADDING_LENGTH_LUMA (PADDING_LENGTH)
#define H264_PADDING_LENGTH_CHROMA (PADDING_LENGTH>>1)

void H264ExpandPictureLumaAnchor_c (uint8_t* pDst, int32_t iStride, int32_t iPicWidth, int32_t iPicHeight) {
  uint8_t* pTmp = pDst;
  uint8_t* pDstLastLine = pTmp + (iPicHeight - 1) * iStride;
  uint8_t pTL = pTmp[0];
  uint8_t pTR = pTmp[iPicWidth - 1];
  uint8_t pBL = pDstLastLine[0];
  uint8_t pBR = pDstLastLine[iPicWidth - 1];
  int32_t i = 0;

  do {
    const int32_t kStrides = (1 + i) * iStride;
    uint8_t* pTop = pTmp - kStrides;
    uint8_t* pBottom = pDstLastLine + kStrides;

    // pad pTop and pBottom
    memcpy (pTop, pTmp, iPicWidth);
    memcpy (pBottom, pDstLastLine, iPicWidth);

    // pad corners
    memset (pTop - H264_PADDING_LENGTH_LUMA, pTL, H264_PADDING_LENGTH_LUMA); //pTop left
    memset (pTop + iPicWidth, pTR, H264_PADDING_LENGTH_LUMA); //pTop right
    memset (pBottom - H264_PADDING_LENGTH_LUMA, pBL, H264_PADDING_LENGTH_LUMA); //pBottom left
    memset (pBottom + iPicWidth, pBR, H264_PADDING_LENGTH_LUMA); //pBottom right
    ++ i;
  } while (i < H264_PADDING_LENGTH_LUMA);

  // pad left and right
  i = 0;
  do {
    memset (pTmp - H264_PADDING_LENGTH_LUMA, pTmp[0], H264_PADDING_LENGTH_LUMA);
    memset (pTmp + iPicWidth, pTmp[iPicWidth - 1], H264_PADDING_LENGTH_LUMA);
    pTmp += iStride;
    ++ i;
  } while (i < iPicHeight);
}

void H264ExpandPictureChromaAnchor_c (uint8_t* pDst, int32_t iStride, int32_t iPicWidth, int32_t iPicHeight) {
  uint8_t* pTmp = pDst;
  uint8_t* pDstLastLine = pTmp + (iPicHeight - 1) * iStride;
  uint8_t pTL = pTmp[0];
  uint8_t pTR = pTmp[iPicWidth - 1];
  uint8_t pBL = pDstLastLine[0];
  uint8_t pBR = pDstLastLine[iPicWidth - 1];
  int32_t i = 0;

  do {
    const int32_t kStrides = (1 + i) * iStride;
    uint8_t* pTop = pTmp - kStrides;
    uint8_t* pBottom = pDstLastLine + kStrides;

    // pad pTop and pBottom
    memcpy (pTop, pTmp, iPicWidth);
    memcpy (pBottom, pDstLastLine, iPicWidth);

    // pad corners
    memset (pTop - H264_PADDING_LENGTH_CHROMA, pTL, H264_PADDING_LENGTH_CHROMA); //pTop left
    memset (pTop + iPicWidth, pTR, H264_PADDING_LENGTH_CHROMA); //pTop right
    memset (pBottom - H264_PADDING_LENGTH_CHROMA, pBL, H264_PADDING_LENGTH_CHROMA); //pBottom left
    memset (pBottom + iPicWidth, pBR, H264_PADDING_LENGTH_CHROMA); //pBottom right

    ++ i;
  } while (i < H264_PADDING_LENGTH_CHROMA);

  // pad left and right
  i = 0;
  do {
    memset (pTmp - H264_PADDING_LENGTH_CHROMA, pTmp[0], H264_PADDING_LENGTH_CHROMA);
    memset (pTmp + iPicWidth, pTmp[iPicWidth - 1], H264_PADDING_LENGTH_CHROMA);

    pTmp += iStride;
    ++ i;
  } while (i < iPicHeight);
}

bool CompareBuff (uint8_t* pSrc0, uint8_t* pSrc1, int32_t iStride, int32_t iWidth, int32_t iHeight) {
  for (int32_t j = 0; j < iHeight; j++) {
    for (int32_t i = 0; i < iWidth; i++) {
      if (pSrc0[i + j * iStride] !=  pSrc1[i + j * iStride]) {
        return false;
      }
    }
  }
  return true;
}

TEST (ExpandPicture, ExpandPictureLuma) {
  SExpandPicFunc	    sExpandPicFunc;
  int32_t iCpuCores = 1;
  uint32_t uiCpuFlag = 0;
  for(int32_t k =0; k<2; k++) {
    if(k==0) {
      uiCpuFlag = 0;
    }else {
      uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores);
    }
    InitExpandPictureFunc (&sExpandPicFunc, uiCpuFlag);
    srand ((unsigned int)time (0));
    for (int32_t iTestIdx = 0; iTestIdx < EXPAND_PIC_TEST_NUM; iTestIdx++) {
      int32_t iPicWidth = 16 + (rand() % 200) * 16;
      int32_t iPicHeight = 16 + (rand() % 100) * 16;

      int32_t iStride = iPicWidth + H264_PADDING_LENGTH_LUMA * 2;
      int32_t iBuffHeight = iPicHeight + H264_PADDING_LENGTH_LUMA * 2;
      int32_t iBuffSize =  iBuffHeight * iStride * sizeof (uint8_t);
      uint8_t* pAnchorDstBuff = static_cast<uint8_t*> (WelsMalloc (iBuffSize, "pAnchorDstBuff"));
      uint8_t* pAnchorDst = pAnchorDstBuff + H264_PADDING_LENGTH_LUMA * iStride + H264_PADDING_LENGTH_LUMA;

      uint8_t* pTestDstBuff = static_cast<uint8_t*> (WelsMalloc (iBuffSize, "pTestDstBuff"));
      uint8_t* pTestDst = pTestDstBuff + H264_PADDING_LENGTH_LUMA * iStride + H264_PADDING_LENGTH_LUMA;

      // Generate Src
      for (int32_t j = 0; j < iPicHeight; j++) {
        for (int32_t i = 0; i < iPicWidth; i++) {
          pAnchorDst[i + j * iStride] = pTestDst[i + j * iStride] = rand() % 256;
        }
      }
      H264ExpandPictureLumaAnchor_c (pAnchorDst, iStride, iPicWidth, iPicHeight);
      sExpandPicFunc.pfExpandLumaPicture (pTestDst, iStride, iPicWidth, iPicHeight);
      EXPECT_EQ (CompareBuff (pAnchorDstBuff, pTestDstBuff, iStride, iPicWidth + H264_PADDING_LENGTH_LUMA * 2,
                              iPicHeight + H264_PADDING_LENGTH_LUMA * 2), true);

      WELS_SAFE_FREE (pAnchorDstBuff, "pAnchorDstBuff");
      WELS_SAFE_FREE (pTestDstBuff, "pTestDstBuff");
    }
  }

}

TEST (ExpandPicture, ExpandPictureChroma) {
  SExpandPicFunc	    sExpandPicFunc;
  int32_t iCpuCores = 1;
  uint32_t uiCpuFlag = 0;
  for(int32_t k =0; k<2; k++) {
    if(k==0) {
      uiCpuFlag = 0;
    }else {
      uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores);
    }
    InitExpandPictureFunc (&sExpandPicFunc, uiCpuFlag);
    srand ((unsigned int)time (0));

    for (int32_t iTestIdx = 0; iTestIdx < EXPAND_PIC_TEST_NUM; iTestIdx++) {
      int32_t iPicWidth = (8 + (rand() % 200) * 8);
      int32_t iPicHeight = (8 + (rand() % 100) * 8);

      int32_t iStride = (iPicWidth + H264_PADDING_LENGTH_CHROMA * 2 + 8) >> 4 << 4;
      int32_t iBuffHeight = iPicHeight + H264_PADDING_LENGTH_CHROMA * 2;
      int32_t iBuffSize =  iBuffHeight * iStride * sizeof (uint8_t);
      uint8_t* pAnchorDstBuff = static_cast<uint8_t*> (WelsMalloc (iBuffSize, "pAnchorDstBuff"));
      uint8_t* pAnchorDst = pAnchorDstBuff + H264_PADDING_LENGTH_CHROMA * iStride + H264_PADDING_LENGTH_CHROMA;

      uint8_t* pTestDstBuff = static_cast<uint8_t*> (WelsMalloc (iBuffSize, "pTestDstBuff"));
      uint8_t* pTestDst = pTestDstBuff + H264_PADDING_LENGTH_CHROMA * iStride + H264_PADDING_LENGTH_CHROMA;

      // Generate Src
      for (int32_t j = 0; j < iPicHeight; j++) {
        for (int32_t i = 0; i < iPicWidth; i++) {
          pAnchorDst[i + j * iStride] =  pTestDst[i + j * iStride] = rand() % 256;
        }
      }
      H264ExpandPictureChromaAnchor_c (pAnchorDst, iStride, iPicWidth, iPicHeight);
      sExpandPicFunc.pfExpandChromaPicture[0] (pTestDst, iStride, iPicWidth, iPicHeight);
      EXPECT_EQ (CompareBuff (pAnchorDstBuff, pTestDstBuff, iStride, iPicWidth + H264_PADDING_LENGTH_CHROMA * 2,
                              iPicHeight + H264_PADDING_LENGTH_CHROMA * 2), true);

      WELS_SAFE_FREE (pAnchorDstBuff, "pAnchorDstBuff");
      WELS_SAFE_FREE (pTestDstBuff, "pTestDstBuff");
    }
  }

}

TEST (ExpandPicture, ExpandPicForMotion) {
  SExpandPicFunc	    sExpandPicFunc;
  int32_t iCpuCores = 1;
  uint32_t uiCpuFlag = 0;
  for(int32_t k =0; k<2; k++) {
    if(k==0) {
      uiCpuFlag = 0;
    }else {
      uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores);
    }
    InitExpandPictureFunc (&sExpandPicFunc, uiCpuFlag);
    srand ((unsigned int)time (0));
    SWelsDecoderContext sCtx;
    PPicture pPicAnchor = NULL;
    PPicture pPicTest = NULL;
    for (int32_t iTestIdx = 0; iTestIdx < EXPAND_PIC_TEST_NUM; iTestIdx++) {
      int32_t iPicWidth = (16 + (rand() % 200) * 16);
      int32_t iPicHeight = (16 + (rand() % 100) * 16);

      pPicAnchor = AllocPicture (&sCtx, iPicWidth, iPicHeight);
      pPicTest = AllocPicture (&sCtx, iPicWidth, iPicHeight);
      sCtx.pDec = pPicTest;

      int32_t iStride = pPicAnchor->iLinesize[0];
      int32_t iStrideC;
      iStrideC = pPicAnchor->iLinesize[1];
      // Generate Src
      for (int32_t j = 0; j < iPicHeight; j++) {
        for (int32_t i = 0; i < iPicWidth; i++) {
          pPicAnchor->pData[0][i + j * iStride] =  pPicTest->pData[0][i + j * iStride] = rand() % 256;
        }
      }
      for (int32_t j = 0; j < iPicHeight / 2; j++) {
        for (int32_t i = 0; i < iPicWidth / 2; i++) {
          pPicAnchor->pData[1][i + j * iStrideC] =  pPicTest->pData[1][i + j * iStrideC] = rand() % 256;
          pPicAnchor->pData[2][i + j * iStrideC] =  pPicTest->pData[2][i + j * iStrideC] = rand() % 256;
        }
      }

      H264ExpandPictureLumaAnchor_c (pPicAnchor->pData[0], iStride, iPicWidth, iPicHeight);
      H264ExpandPictureChromaAnchor_c (pPicAnchor->pData[1], iStrideC, iPicWidth / 2, iPicHeight / 2);
      H264ExpandPictureChromaAnchor_c (pPicAnchor->pData[2], iStrideC, iPicWidth / 2, iPicHeight / 2);
      ExpandReferencingPicture (sCtx.pDec->pData, sCtx.pDec->iWidthInPixel, sCtx.pDec->iHeightInPixel, sCtx.pDec->iLinesize,
                                sExpandPicFunc.pfExpandLumaPicture, sExpandPicFunc.pfExpandChromaPicture);

      EXPECT_EQ (CompareBuff (pPicAnchor->pBuffer[0], pPicTest->pBuffer[0], iStride, iPicWidth + PADDING_LENGTH * 2,
                              iPicHeight + PADDING_LENGTH * 2), true);
      EXPECT_EQ (CompareBuff (pPicAnchor->pBuffer[1], pPicTest->pBuffer[1], iStrideC, iPicWidth / 2 + PADDING_LENGTH,
                              iPicHeight / 2 + PADDING_LENGTH), true);
      EXPECT_EQ (CompareBuff (pPicAnchor->pBuffer[2], pPicTest->pBuffer[2], iStrideC, iPicWidth / 2 + PADDING_LENGTH,
                              iPicHeight / 2 + PADDING_LENGTH), true);

      FreePicture (pPicAnchor);
      FreePicture (pPicTest);
    }
  }
}

