#include "cpu.h"
#include "macros.h"
#include "set_mb_syn_cavlc.h"
#include <gtest/gtest.h>
#include <cmath>
#include <cstddef>

using namespace WelsEnc;

namespace {

int32_t CavlcParamCal_ref (int16_t* pCoffLevel, uint8_t* pRun, int16_t* pLevel, int32_t* pTotalCoeff,
                           int32_t iLastIndex) {
  int32_t iTotalZeros = 0;
  int32_t iTotalCoeffs = 0;

  while (iLastIndex >= 0 && pCoffLevel[iLastIndex] == 0) {
    -- iLastIndex;
  }

  while (iLastIndex >= 0) {
    int32_t iCountZero = 0;
    pLevel[iTotalCoeffs] = pCoffLevel[iLastIndex--];

    while (iLastIndex >= 0 && pCoffLevel[iLastIndex] == 0) {
      ++ iCountZero;
      -- iLastIndex;
    }
    iTotalZeros += iCountZero;
    pRun[iTotalCoeffs++] = iCountZero;
  }
  *pTotalCoeff = iTotalCoeffs;
  return iTotalZeros;
}

void TestCavlcParamCalWithEndIdx (PCavlcParamCalFunc func, int endIdx, bool allZero, bool allNonZero) {
  ENFORCE_STACK_ALIGN_1D (int16_t, coeffLevel, 16, 16);
  ENFORCE_STACK_ALIGN_1D (int16_t, level, 16, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, run, 16, 16);
  uint8_t run_ref[16];
  int16_t level_ref[16];
  int32_t totalCoeffs = 0;
  int32_t totalCoeffs_ref = 0;
  for (int i = 0; i < 16; i++) {
    const int r = std::rand();
    if (allZero || (i > endIdx && endIdx > 7))
      coeffLevel[i] = 0;
    else if (allNonZero)
      coeffLevel[i] = r % 0xFFFF - 0x8000 ? r % 0xFFFF - 0x8000 : 0x7FFF;
    else
      coeffLevel[i] = (r >> 16 & 1) * ((r & 0xFFFF) - 0x8000);
  }
  const int32_t totalZeros_ref = CavlcParamCal_ref (coeffLevel, run_ref, level_ref, &totalCoeffs_ref, endIdx);
  const int32_t totalZeros = func (coeffLevel, run, level, &totalCoeffs, endIdx);
  ASSERT_EQ (totalCoeffs, totalCoeffs_ref);
  if (totalCoeffs > 0) {
    ASSERT_EQ (totalZeros, totalZeros_ref);
  }
  for (int i = 0; i < totalCoeffs_ref; i++)
    ASSERT_EQ (level[i], level_ref[i]);
  for (int i = 0; i < totalCoeffs_ref - 1; i++)
    ASSERT_EQ (run[i], run_ref[i]);
}

void TestCavlcParamCal (PCavlcParamCalFunc func) {
  const int endIdxes[] = { 3, 14, 15 };
  const int num_test_repetitions = 10000;
  for (std::size_t i = 0; i < sizeof endIdxes / sizeof * endIdxes; i++) {
    for (int count = 0; count < num_test_repetitions; count++)
      TestCavlcParamCalWithEndIdx (func, endIdxes[i], count == 0, count == 1);
  }
}

} // anon ns.

TEST (CavlcTest, CavlcParamCal_c) {
  TestCavlcParamCal (CavlcParamCal_c);
}

#ifdef X86_32_ASM
TEST (CavlcTest, CavlcParamCal_sse2) {
  TestCavlcParamCal (CavlcParamCal_sse2);
}
#endif

#ifdef X86_ASM
TEST (CavlcTest, CavlcParamCal_sse42) {
  if (WelsCPUFeatureDetect (0) & WELS_CPU_SSE42)
    TestCavlcParamCal (CavlcParamCal_sse42);
}
#endif
