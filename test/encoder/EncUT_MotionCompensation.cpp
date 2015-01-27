#include <gtest/gtest.h>
#include "codec_def.h"
#include "mc.h"
#include "cpu.h"
using namespace WelsEnc;

static void McLumaFunc (SMcFunc* pFuncs, const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                        int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight) {
  uint8_t uiMvpIdx = ((iMvY & 0x03) << 2) + (iMvX & 0x03);
  ASSERT_EQ (iWidth, 16);
  pFuncs->pfLumaQuarpelMc[uiMvpIdx] (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}

#define InitMcFunc WelsInitMcFuncs

#define LUMA_FUNC(funcs, src, srcstride, dst, dststride, mvx, mvy, width, height) \
  McLumaFunc (funcs, src, srcstride, dst, dststride, mvx, mvy, width, height)

#define CHROMA_FUNC sMcFunc.pfChromaMc

#include "mc_test_common.h"

DEF_MCCOPYTEST (Enc, 16, 8)
DEF_MCCOPYTEST (Enc, 16, 16)

DEF_LUMA_MCTEST (Enc, 16, 8)
DEF_LUMA_MCTEST (Enc, 16, 16)

DEF_CHROMA_MCTEST (Enc, 4, 2)
DEF_CHROMA_MCTEST (Enc, 4, 4)
DEF_CHROMA_MCTEST (Enc, 4, 8)
DEF_CHROMA_MCTEST (Enc, 8, 4)
DEF_CHROMA_MCTEST (Enc, 8, 8)

TEST (EncMcAvg, PixelAvg) {
  SMcFunc sMcFunc;
  for (int32_t k = 0; k < 2; k++) {
    for (int32_t w = 0; w < 2; w++) {
      int32_t width = 8 << w;
      int32_t height = 16;
      uint32_t uiCpuFlag = k == 0 ? 0 : WelsCPUFeatureDetect (NULL);
      WelsInitMcFuncs (&sMcFunc, uiCpuFlag);
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
      sMcFunc.pfSampleAveraging[w] (uDstTest[0], MC_BUFF_DST_STRIDE, uSrc1[0], MC_BUFF_SRC_STRIDE, uSrc2[0],
                                    MC_BUFF_SRC_STRIDE, height);
      for (int32_t j = 0; j < height; j++) {
        for (int32_t i = 0; i < width; i++) {
          ASSERT_EQ (uDstAnchor[j][i], uDstTest[j][i]);
        }
      }
    }
  }
}

TEST (EncMcHalfpel, LumaHalfpel) {
  SMcFunc sMcFunc;
  for (int32_t k = 0; k < 2; k++) {
    for (int32_t w = 0; w < 2; w++) {
      int32_t width = 8 << w;
      int32_t height = 16;
      uint8_t uAnchor[4][MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE];
      uint8_t uSrcTest[MC_BUFF_HEIGHT][MC_BUFF_SRC_STRIDE];
      ENFORCE_STACK_ALIGN_2D (uint8_t, uDstTest, MC_BUFF_HEIGHT, MC_BUFF_DST_STRIDE, 16);
      uint8_t* uAnchors[4];
      int16_t pBuf[MC_BUFF_DST_STRIDE];
      uAnchors[0] = &uAnchor[0][4][4];
      uAnchors[1] = &uAnchor[1][4][4];
      uAnchors[2] = &uAnchor[2][4][4];
      uAnchors[3] = &uAnchor[3][4][4];

      memset (uAnchor, 0, 4 * sizeof (uint8_t)*MC_BUFF_HEIGHT * MC_BUFF_DST_STRIDE);
      memset (uDstTest, 0, sizeof (uint8_t)*MC_BUFF_HEIGHT * MC_BUFF_DST_STRIDE);
      for (int32_t j = 0; j < MC_BUFF_HEIGHT; j++) {
        for (int32_t i = 0; i < MC_BUFF_SRC_STRIDE; i++) {
          uAnchor[0][j][i] = uSrcTest[j][i] = rand() % 256;
        }
      }

      uint32_t uiCpuFlag = k == 0 ? 0 : WelsCPUFeatureDetect (NULL);
      WelsInitMcFuncs (&sMcFunc, uiCpuFlag);

      MCHalfPelFilterAnchor (uAnchors[1], uAnchors[2], uAnchors[3], uAnchors[0], MC_BUFF_SRC_STRIDE, width, height, pBuf + 4);
      sMcFunc.pfLumaHalfpelHor (&uSrcTest[4][4], MC_BUFF_SRC_STRIDE, uDstTest[0], MC_BUFF_DST_STRIDE, width + 1, height);
      for (int32_t j = 0; j < height; j++) {
        for (int32_t i = 0; i < width; i++) {
          ASSERT_EQ (uAnchor[1][4 + j][4 + i], uDstTest[j][i]);
        }
      }
      sMcFunc.pfLumaHalfpelVer (&uSrcTest[4][4], MC_BUFF_SRC_STRIDE, uDstTest[0], MC_BUFF_DST_STRIDE, width, height + 1);
      for (int32_t j = 0; j < height; j++) {
        for (int32_t i = 0; i < width; i++) {
          ASSERT_EQ (uAnchor[2][4 + j][4 + i], uDstTest[j][i]);
        }
      }
      sMcFunc.pfLumaHalfpelCen (&uSrcTest[4][4], MC_BUFF_SRC_STRIDE, uDstTest[0], MC_BUFF_DST_STRIDE, width + 1, height + 1);
      for (int32_t j = 0; j < height; j++) {
        for (int32_t i = 0; i < width; i++) {
          ASSERT_EQ (uAnchor[3][4 + j][4 + i], uDstTest[j][i]);
        }
      }
    }
  }
}
