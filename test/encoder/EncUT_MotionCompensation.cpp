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

#define PREFIX Enc

#define DEF_MCCOPYTESTS(pfx)    \
DEF_MCCOPYTEST (pfx, 16, 8, 0)  \
DEF_MCCOPYTEST (pfx, 16, 16, 0)

#define DEF_LUMA_MCTEST(pfx,a,b) \
DEF_LUMA_MCTEST_SUBCASE(pfx,a,b,16,8) \
DEF_LUMA_MCTEST_SUBCASE(pfx,a,b,16,16)

#define DEF_CHROMA_MCTEST(pfx,a,b) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,4,2) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,4,4) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,4,8) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,8,4) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,8,8)

#include "mc_test_common.h"
