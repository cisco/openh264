#include <gtest/gtest.h>
#include "cpu.h"
#include "cpu_core.h"
#include "deblocking_common.h"
#include "macros.h"

#define WRAP_LUMA_FUNC(func) \
void func ## _wrap (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc) { \
  func (pPixY, iStride, iAlpha, iBeta); \
}
#define WRAP_CHROMA_FUNC(func) \
void func ## _wrap (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc) { \
  func (pPixCb, pPixCr, iStride, iAlpha, iBeta); \
}

#define GENERATE_LUMA_UT(name, func, ref, CPUFLAGS, HORIZ) \
TEST(DeblockTest, name) { \
  int32_t iRunTimes = 1000; \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pTestBuffer, 16*17, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pRefBuffer, 16*17, 16); \
  int32_t iNumberofCPUCore = 1; \
  uint32_t uiCPUFlags = WelsCPUFeatureDetect(&iNumberofCPUCore); \
  if ((uiCPUFlags & CPUFLAGS) == 0) \
    return; \
  while (iRunTimes--) { \
    int iAlpha = rand() & 255; \
    int iBeta = rand() & 255; \
    ENFORCE_STACK_ALIGN_1D (int8_t, iTc, 4, 16); \
    for (int i = 0; i < 4; i++) \
      iTc[i] = -1 + (rand() % 28); \
    if (iRunTimes == 1) { /* special case to test all pixels */ \
        iAlpha = iBeta = 255; \
        for (int i = 0; i < 4; i++) \
          iTc[i] = 27; \
    } \
    for (int i = 0; i < 16*17; i++) \
      pRefBuffer[i] = pTestBuffer[i] = rand() & 255; \
    uint8_t* pRefStart = HORIZ ? pRefBuffer + 16 : pRefBuffer + 16*8; \
    uint8_t* pTestStart = HORIZ ? pTestBuffer + 16 : pTestBuffer + 16*8; \
    func (pTestStart, 16, iAlpha, iBeta, iTc); \
    ref (pRefStart, 16, iAlpha, iBeta, iTc); \
    bool ok = true; \
    for (int i = 0; i < 16*17; i++) { \
      if (pTestBuffer[i] != pRefBuffer[i]) { \
        ok = false; \
        break; \
      } \
    } \
    EXPECT_EQ(ok, true); \
  } \
}

#define GENERATE_CHROMA_UT(name, func, ref, CPUFLAGS, HORIZ) \
TEST(DeblockTest, name) { \
  int32_t iRunTimes = 1000; \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pTestBuffer, 8 + 8*8*2, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pRefBuffer, 8 + 8*8*2, 16); \
  int32_t iNumberofCPUCore = 1; \
  uint32_t uiCPUFlags = WelsCPUFeatureDetect(&iNumberofCPUCore); \
  if ((uiCPUFlags & CPUFLAGS) == 0) \
    return; \
  while (iRunTimes--) { \
    int iAlpha = rand() & 255; \
    int iBeta = rand() & 255; \
    ENFORCE_STACK_ALIGN_1D (int8_t, iTc, 4, 16); \
    for (int i = 0; i < 4; i++) \
      iTc[i] = -1 + (rand() % 28); \
    if (iRunTimes == 1) { /* special case to test all pixels */ \
        iAlpha = iBeta = 255; \
        for (int i = 0; i < 4; i++) \
          iTc[i] = 27; \
    } \
    for (int i = 0; i < 8 + 8*8*2; i++) \
      pRefBuffer[i] = pTestBuffer[i] = rand() & 255; \
    uint8_t* pCbRefStart = HORIZ ? pRefBuffer + 8 : pRefBuffer + 8*4; \
    uint8_t* pCbTestStart = HORIZ ? pTestBuffer + 8 : pTestBuffer + 8*4; \
    uint8_t* pCrRefStart = pCbRefStart + 8*8; \
    uint8_t* pCrTestStart = pCbTestStart + 8*8; \
    func (pCbTestStart, pCrTestStart, 8, iAlpha, iBeta, iTc); \
    ref (pCbRefStart, pCrRefStart, 8, iAlpha, iBeta, iTc); \
    bool ok = true; \
    for (int i = 0; i < 8 + 8*8*2; i++) { \
      if (pTestBuffer[i] != pRefBuffer[i]) { \
        ok = false; \
        break; \
      } \
    } \
    EXPECT_EQ(ok, true); \
  } \
}

WRAP_LUMA_FUNC (DeblockLumaEq4V_c)
WRAP_LUMA_FUNC (DeblockLumaEq4H_c)
WRAP_CHROMA_FUNC (DeblockChromaEq4V_c)
WRAP_CHROMA_FUNC (DeblockChromaEq4H_c)

#if defined(X86_ASM)
WRAP_LUMA_FUNC (DeblockLumaEq4V_ssse3)
WRAP_LUMA_FUNC (DeblockLumaEq4H_ssse3)
WRAP_CHROMA_FUNC (DeblockChromaEq4V_ssse3)
WRAP_CHROMA_FUNC (DeblockChromaEq4H_ssse3)

GENERATE_LUMA_UT (LumaLt4V_ssse3, DeblockLumaLt4V_ssse3, DeblockLumaLt4V_c, WELS_CPU_SSSE3, 0)
GENERATE_LUMA_UT (LumaLt4H_ssse3, DeblockLumaLt4H_ssse3, DeblockLumaLt4H_c, WELS_CPU_SSSE3, 1)
GENERATE_LUMA_UT (LumaEq4V_ssse3, DeblockLumaEq4V_ssse3_wrap, DeblockLumaEq4V_c_wrap, WELS_CPU_SSSE3, 0)
GENERATE_LUMA_UT (LumaEq4H_ssse3, DeblockLumaEq4H_ssse3_wrap, DeblockLumaEq4H_c_wrap, WELS_CPU_SSSE3, 1)

GENERATE_CHROMA_UT (ChromaLt4V_ssse3, DeblockChromaLt4V_ssse3, DeblockChromaLt4V_c, WELS_CPU_SSSE3, 0)
GENERATE_CHROMA_UT (ChromaLt4H_ssse3, DeblockChromaLt4H_ssse3, DeblockChromaLt4H_c, WELS_CPU_SSSE3, 1)
GENERATE_CHROMA_UT (ChromaEq4V_ssse3, DeblockChromaEq4V_ssse3_wrap, DeblockChromaEq4V_c_wrap, WELS_CPU_SSSE3, 0)
GENERATE_CHROMA_UT (ChromaEq4H_ssse3, DeblockChromaEq4H_ssse3_wrap, DeblockChromaEq4H_c_wrap, WELS_CPU_SSSE3, 1)
#endif

#if defined(HAVE_NEON)
WRAP_LUMA_FUNC (DeblockLumaEq4V_neon)
WRAP_LUMA_FUNC (DeblockLumaEq4H_neon)
WRAP_CHROMA_FUNC (DeblockChromaEq4V_neon)
WRAP_CHROMA_FUNC (DeblockChromaEq4H_neon)

GENERATE_LUMA_UT (LumaLt4V_neon, DeblockLumaLt4V_neon, DeblockLumaLt4V_c, WELS_CPU_NEON, 0)
GENERATE_LUMA_UT (LumaLt4H_neon, DeblockLumaLt4H_neon, DeblockLumaLt4H_c, WELS_CPU_NEON, 1)
GENERATE_LUMA_UT (LumaEq4V_neon, DeblockLumaEq4V_neon_wrap, DeblockLumaEq4V_c_wrap, WELS_CPU_NEON, 0)
GENERATE_LUMA_UT (LumaEq4H_neon, DeblockLumaEq4H_neon_wrap, DeblockLumaEq4H_c_wrap, WELS_CPU_NEON, 1)

GENERATE_CHROMA_UT (ChromaLt4V_neon, DeblockChromaLt4V_neon, DeblockChromaLt4V_c, WELS_CPU_NEON, 0)
GENERATE_CHROMA_UT (ChromaLt4H_neon, DeblockChromaLt4H_neon, DeblockChromaLt4H_c, WELS_CPU_NEON, 1)
GENERATE_CHROMA_UT (ChromaEq4V_neon, DeblockChromaEq4V_neon_wrap, DeblockChromaEq4V_c_wrap, WELS_CPU_NEON, 0)
GENERATE_CHROMA_UT (ChromaEq4H_neon, DeblockChromaEq4H_neon_wrap, DeblockChromaEq4H_c_wrap, WELS_CPU_NEON, 1)
#endif

#if defined(HAVE_NEON_AARCH64)
WRAP_LUMA_FUNC (DeblockLumaEq4V_AArch64_neon)
WRAP_LUMA_FUNC (DeblockLumaEq4H_AArch64_neon)
WRAP_CHROMA_FUNC (DeblockChromaEq4V_AArch64_neon)
WRAP_CHROMA_FUNC (DeblockChromaEq4H_AArch64_neon)

GENERATE_LUMA_UT (LumaLt4V_AArch64_neon, DeblockLumaLt4V_AArch64_neon, DeblockLumaLt4V_c, WELS_CPU_NEON, 0)
GENERATE_LUMA_UT (LumaLt4H_AArch64_neon, DeblockLumaLt4H_AArch64_neon, DeblockLumaLt4H_c, WELS_CPU_NEON, 1)
GENERATE_LUMA_UT (LumaEq4V_AArch64_neon, DeblockLumaEq4V_AArch64_neon_wrap, DeblockLumaEq4V_c_wrap, WELS_CPU_NEON, 0)
GENERATE_LUMA_UT (LumaEq4H_AArch64_neon, DeblockLumaEq4H_AArch64_neon_wrap, DeblockLumaEq4H_c_wrap, WELS_CPU_NEON, 1)

GENERATE_CHROMA_UT (ChromaLt4V_AArch64_neon, DeblockChromaLt4V_AArch64_neon, DeblockChromaLt4V_c, WELS_CPU_NEON, 0)
GENERATE_CHROMA_UT (ChromaLt4H_AArch64_neon, DeblockChromaLt4H_AArch64_neon, DeblockChromaLt4H_c, WELS_CPU_NEON, 1)
GENERATE_CHROMA_UT (ChromaEq4V_AArch64_neon, DeblockChromaEq4V_AArch64_neon_wrap, DeblockChromaEq4V_c_wrap,
                    WELS_CPU_NEON, 0)
GENERATE_CHROMA_UT (ChromaEq4H_AArch64_neon, DeblockChromaEq4H_AArch64_neon_wrap, DeblockChromaEq4H_c_wrap,
                    WELS_CPU_NEON, 1)
#endif

#if defined(HAVE_MMI)
WRAP_LUMA_FUNC (DeblockLumaEq4V_mmi)
WRAP_LUMA_FUNC (DeblockLumaEq4H_mmi)
WRAP_CHROMA_FUNC (DeblockChromaEq4V_mmi)
WRAP_CHROMA_FUNC (DeblockChromaEq4H_mmi)

GENERATE_LUMA_UT (LumaLt4V_mmi, DeblockLumaLt4V_mmi, DeblockLumaLt4V_c, WELS_CPU_MMI, 0)
GENERATE_LUMA_UT (LumaLt4H_mmi, DeblockLumaLt4H_mmi, DeblockLumaLt4H_c, WELS_CPU_MMI, 1)
GENERATE_LUMA_UT (LumaEq4V_mmi, DeblockLumaEq4V_mmi_wrap, DeblockLumaEq4V_c_wrap, WELS_CPU_MMI, 0)
GENERATE_LUMA_UT (LumaEq4H_mmi, DeblockLumaEq4H_mmi_wrap, DeblockLumaEq4H_c_wrap, WELS_CPU_MMI, 1)

GENERATE_CHROMA_UT (ChromaLt4V_mmi, DeblockChromaLt4V_mmi, DeblockChromaLt4V_c, WELS_CPU_MMI, 0)
GENERATE_CHROMA_UT (ChromaLt4H_mmi, DeblockChromaLt4H_mmi, DeblockChromaLt4H_c, WELS_CPU_MMI, 1)
GENERATE_CHROMA_UT (ChromaEq4V_mmi, DeblockChromaEq4V_mmi_wrap, DeblockChromaEq4V_c_wrap, WELS_CPU_MMI, 0)
GENERATE_CHROMA_UT (ChromaEq4H_mmi, DeblockChromaEq4H_mmi_wrap, DeblockChromaEq4H_c_wrap, WELS_CPU_MMI, 1)
#endif//HAVE_MMI

#if defined(HAVE_MSA)
WRAP_LUMA_FUNC (DeblockLumaEq4V_msa)
WRAP_LUMA_FUNC (DeblockLumaEq4H_msa)
WRAP_CHROMA_FUNC (DeblockChromaEq4V_msa)
WRAP_CHROMA_FUNC (DeblockChromaEq4H_msa)

GENERATE_LUMA_UT (LumaLt4V_msa, DeblockLumaLt4V_msa, DeblockLumaLt4V_c, WELS_CPU_MSA, 0)
GENERATE_LUMA_UT (LumaLt4H_msa, DeblockLumaLt4H_msa, DeblockLumaLt4H_c, WELS_CPU_MSA, 1)
GENERATE_LUMA_UT (LumaEq4V_msa, DeblockLumaEq4V_msa_wrap, DeblockLumaEq4V_c_wrap, WELS_CPU_MSA, 0)
GENERATE_LUMA_UT (LumaEq4H_msa, DeblockLumaEq4H_msa_wrap, DeblockLumaEq4H_c_wrap, WELS_CPU_MSA, 1)

GENERATE_CHROMA_UT (ChromaLt4V_msa, DeblockChromaLt4V_msa, DeblockChromaLt4V_c, WELS_CPU_MSA, 0)
GENERATE_CHROMA_UT (ChromaLt4H_msa, DeblockChromaLt4H_msa, DeblockChromaLt4H_c, WELS_CPU_MSA, 1)
GENERATE_CHROMA_UT (ChromaEq4V_msa, DeblockChromaEq4V_msa_wrap, DeblockChromaEq4V_c_wrap, WELS_CPU_MSA, 0)
GENERATE_CHROMA_UT (ChromaEq4H_msa, DeblockChromaEq4H_msa_wrap, DeblockChromaEq4H_c_wrap, WELS_CPU_MSA, 1)
#endif//HAVE_MSA

#if defined(HAVE_LSX)
WRAP_LUMA_FUNC (DeblockLumaEq4V_lsx)
WRAP_LUMA_FUNC (DeblockLumaEq4H_lsx)
WRAP_CHROMA_FUNC (DeblockChromaEq4H_lsx)

GENERATE_LUMA_UT (LumaLt4V_lsx, DeblockLumaLt4V_lsx, DeblockLumaLt4V_c, WELS_CPU_LSX, 0)
GENERATE_LUMA_UT (LumaLt4H_lsx, DeblockLumaLt4H_lsx, DeblockLumaLt4H_c, WELS_CPU_LSX, 1)
GENERATE_LUMA_UT (LumaEq4V_lsx, DeblockLumaEq4V_lsx_wrap, DeblockLumaEq4V_c_wrap, WELS_CPU_LSX, 0)
GENERATE_LUMA_UT (LumaEq4H_lsx, DeblockLumaEq4H_lsx_wrap, DeblockLumaEq4H_c_wrap, WELS_CPU_LSX, 1)
GENERATE_CHROMA_UT (ChromaLt4V_lsx, DeblockChromaLt4V_lsx, DeblockChromaLt4V_c, WELS_CPU_LSX, 0)
GENERATE_CHROMA_UT (ChromaLt4H_lsx, DeblockChromaLt4H_lsx, DeblockChromaLt4H_c, WELS_CPU_LSX, 1)
GENERATE_CHROMA_UT (ChromaEq4H_lsx, DeblockChromaEq4H_lsx_wrap, DeblockChromaEq4H_c_wrap, WELS_CPU_LSX, 1)
#endif//HAVE_LSX
