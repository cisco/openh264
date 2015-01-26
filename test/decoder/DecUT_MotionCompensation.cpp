#include <gtest/gtest.h>
#include "codec_def.h"
#include "mc.h"
#include "cpu.h"
using namespace WelsDec;

#define LUMA_FUNC(funcs, src, srcstride, dst, dststride, mvx, mvy, width, height) \
  sMcFunc.pMcLumaFunc (src, srcstride, dst, dststride, mvx, mvy, width, height)

#define CHROMA_FUNC sMcFunc.pMcChromaFunc

#define PREFIX

#define DEF_MCCOPYTESTS(pfx)  \
DEF_MCCOPYTEST (pfx, 2, 2, 1)   \
DEF_MCCOPYTEST (pfx, 2, 4, 1)   \
DEF_MCCOPYTEST (pfx, 4, 2, 0)   \
DEF_MCCOPYTEST (pfx, 4, 4, 0)   \
DEF_MCCOPYTEST (pfx, 4, 8, 0)   \
DEF_MCCOPYTEST (pfx, 8, 4, 0)   \
DEF_MCCOPYTEST (pfx, 8, 8, 0)   \
DEF_MCCOPYTEST (pfx, 16, 8, 0)  \
DEF_MCCOPYTEST (pfx, 8, 16, 0)  \
DEF_MCCOPYTEST (pfx, 16, 16, 0)

#define DEF_LUMA_MCTEST(pfx,a,b) \
DEF_LUMA_MCTEST_SUBCASE(pfx,a,b,4,4)  \
DEF_LUMA_MCTEST_SUBCASE(pfx,a,b,4,8)  \
DEF_LUMA_MCTEST_SUBCASE(pfx,a,b,8,4)  \
DEF_LUMA_MCTEST_SUBCASE(pfx,a,b,8,8)  \
DEF_LUMA_MCTEST_SUBCASE(pfx,a,b,16,8) \
DEF_LUMA_MCTEST_SUBCASE(pfx,a,b,8,16) \
DEF_LUMA_MCTEST_SUBCASE(pfx,a,b,16,16)

#define DEF_CHROMA_MCTEST(pfx,a,b) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,2,2) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,2,4) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,4,2) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,4,4) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,4,8) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,8,4) \
DEF_CHROMA_MCTEST_SUBCASE(pfx,a,b,8,8)

#include "mc_test_common.h"
