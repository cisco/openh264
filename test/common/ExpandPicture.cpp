#include <gtest/gtest.h>
#include "codec_def.h"
#include "expand_pic.h"
#include "memory_align.h"
#include "cpu.h"
#include "cpu_core.h"
#include "macros.h"
#include "wels_const_common.h"

#define EXPAND_PIC_TEST_NUM 10
#define H264_PADDING_LENGTH_LUMA (PADDING_LENGTH)
#define H264_PADDING_LENGTH_CHROMA (PADDING_LENGTH>>1)

using namespace WelsCommon;

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

bool CompareImage (uint8_t* pSrc0, uint8_t* pSrc1, int32_t iSize) {
  for (int32_t n = 0; n < iSize; n++) {
    if (pSrc0[n] !=  pSrc1[n]) {
      return false;
    }

  }
  return true;
}

TEST (ExpandPicture, ExpandPictureLuma) {
  SExpandPicFunc sExpandPicFunc;
  int32_t iCpuCores = 1;
  uint32_t uiCpuFlag = 0;
  for (int32_t k = 0; k < 2; k++) {
    if (k == 0) {
      uiCpuFlag = 0;
    } else {
      uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores);
    }
    InitExpandPictureFunc (&sExpandPicFunc, uiCpuFlag);
    for (int32_t iTestIdx = 0; iTestIdx < EXPAND_PIC_TEST_NUM; iTestIdx++) {
      int32_t iPicWidth = 16 + (rand() % 200) * 16;
      int32_t iPicHeight = 16 + (rand() % 100) * 16;

      int32_t iStride = iPicWidth + H264_PADDING_LENGTH_LUMA * 2;
      int32_t iBuffHeight = iPicHeight + H264_PADDING_LENGTH_LUMA * 2;
      int32_t iBuffSize =  iBuffHeight * iStride * sizeof (uint8_t);
      uint8_t* pAnchorDstBuff = static_cast<uint8_t*> (WelsMallocz (iBuffSize, "pAnchorDstBuff"));
      uint8_t* pAnchorDst = pAnchorDstBuff + H264_PADDING_LENGTH_LUMA * iStride + H264_PADDING_LENGTH_LUMA;

      uint8_t* pTestDstBuff = static_cast<uint8_t*> (WelsMallocz (iBuffSize, "pTestDstBuff"));
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
  SExpandPicFunc sExpandPicFunc;
  int32_t iCpuCores = 1;
  uint32_t uiCpuFlag = 0;
  for (int32_t k = 0; k < 2; k++) {
    if (k == 0) {
      uiCpuFlag = 0;
    } else {
      uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores);
    }
    InitExpandPictureFunc (&sExpandPicFunc, uiCpuFlag);

    for (int32_t iTestIdx = 0; iTestIdx < EXPAND_PIC_TEST_NUM; iTestIdx++) {
      int32_t iPicWidth = (8 + (rand() % 200) * 8);
      if (uiCpuFlag & WELS_CPU_SSE2) {
        iPicWidth = WELS_MAX (iPicWidth, 16);
      }
      int32_t iPicHeight = (8 + (rand() % 100) * 8);

      int32_t iStride = (iPicWidth + H264_PADDING_LENGTH_CHROMA * 2 + 8) >> 4 << 4;
      int32_t iBuffHeight = iPicHeight + H264_PADDING_LENGTH_CHROMA * 2;
      int32_t iBuffSize =  iBuffHeight * iStride * sizeof (uint8_t);
      uint8_t* pAnchorDstBuff = static_cast<uint8_t*> (WelsMallocz (iBuffSize, "pAnchorDstBuff"));
      uint8_t* pAnchorDst = pAnchorDstBuff + H264_PADDING_LENGTH_CHROMA * iStride + H264_PADDING_LENGTH_CHROMA;

      uint8_t* pTestDstBuff = static_cast<uint8_t*> (WelsMallocz (iBuffSize, "pTestDstBuff"));
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
  SExpandPicFunc sExpandPicFunc;
  int32_t iCpuCores = 1;
  uint32_t uiCpuFlag = 0;
  for (int32_t k = 0; k < 2; k++) {
    if (k == 0) {
      uiCpuFlag = 0;
    } else {
      uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores);
    }
    InitExpandPictureFunc (&sExpandPicFunc, uiCpuFlag);
    uint8_t* pPicAnchorBuffer = NULL;
    uint8_t* pPicTestBuffer = NULL;
    uint8_t* pPicAnchor[3] = {NULL, NULL, NULL};
    uint8_t* pPicTest[3] = {NULL, NULL, NULL};
    int32_t iStride[3];
    for (int32_t iTestIdx = 0; iTestIdx < EXPAND_PIC_TEST_NUM; iTestIdx++) {
      int32_t iPicWidth = (16 + (rand() % 200) * 16);
      int32_t iPicHeight = (16 + (rand() % 100) * 16);
      if (uiCpuFlag & WELS_CPU_SSE2) {
        iPicWidth = WELS_ALIGN (iPicWidth, 32);
      }
      iStride[0]                  = WELS_ALIGN (iPicWidth, MB_WIDTH_LUMA)   + (PADDING_LENGTH << 1);      // with width of horizon
      int32_t iPicHeightExt       = WELS_ALIGN (iPicHeight, MB_HEIGHT_LUMA) + (PADDING_LENGTH << 1);      // with height of vertical
      iStride[1]                  = iStride[0] >> 1;
      int32_t iPicChromaHeightExt = iPicHeightExt >> 1;
      iStride[2]                  = iStride[1];
      int32_t iLumaSize           = iStride[0] * iPicHeightExt;
      int32_t iChromaSize         = iStride[1] * iPicChromaHeightExt;

      pPicAnchorBuffer = static_cast<uint8_t*> (WelsMallocz (iLumaSize + (iChromaSize << 1), "pPicAnchor"));
      pPicAnchor[0]     = pPicAnchorBuffer + (1 + iStride[0]) * PADDING_LENGTH;
      pPicAnchor[1]     = pPicAnchorBuffer + iLumaSize + (((1 + iStride[1]) * PADDING_LENGTH) >> 1);
      pPicAnchor[2]     = pPicAnchorBuffer + iLumaSize + iChromaSize + (((1 + iStride[2]) * PADDING_LENGTH) >> 1);

      pPicTestBuffer = static_cast<uint8_t*> (WelsMallocz (iLumaSize + (iChromaSize << 1), "pPicTest"));
      pPicTest[0]       = pPicTestBuffer + (1 + iStride[0]) * PADDING_LENGTH;
      pPicTest[1]       = pPicTestBuffer + iLumaSize + (((1 + iStride[1]) * PADDING_LENGTH) >> 1);
      pPicTest[2]       = pPicTestBuffer + iLumaSize + iChromaSize + (((1 + iStride[2]) * PADDING_LENGTH) >> 1);


      // Generate Src
      for (int32_t j = 0; j < iPicHeight; j++) {
        for (int32_t i = 0; i < iPicWidth; i++) {
          pPicAnchor[0][i + j * iStride[0]] =  pPicTest[0][i + j * iStride[0]] = rand() % 256;
        }
      }
      for (int32_t j = 0; j < iPicHeight / 2; j++) {
        for (int32_t i = 0; i < iPicWidth / 2; i++) {
          pPicAnchor[1][i + j * iStride[1]] =  pPicTest[1][i + j * iStride[1]] = rand() % 256;
          pPicAnchor[2][i + j * iStride[2]] =  pPicTest[2][i + j * iStride[2]] = rand() % 256;
        }
      }
      H264ExpandPictureLumaAnchor_c (pPicAnchor[0], iStride[0], iPicWidth, iPicHeight);
      H264ExpandPictureChromaAnchor_c (pPicAnchor[1], iStride[1], iPicWidth / 2, iPicHeight / 2);
      H264ExpandPictureChromaAnchor_c (pPicAnchor[2], iStride[2], iPicWidth / 2, iPicHeight / 2);
      ExpandReferencingPicture (pPicTest, iPicWidth, iPicHeight, iStride,
                                sExpandPicFunc.pfExpandLumaPicture, sExpandPicFunc.pfExpandChromaPicture);
      EXPECT_EQ (CompareImage (pPicAnchorBuffer, pPicTestBuffer, (iLumaSize + (iChromaSize << 1))), true);


      WELS_SAFE_FREE (pPicAnchorBuffer, "pPicAnchor");
      WELS_SAFE_FREE (pPicTestBuffer, "pPicTest");

    }
  }
}

