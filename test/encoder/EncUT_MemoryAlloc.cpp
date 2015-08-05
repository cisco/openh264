#include "gtest/gtest.h"
#include "memory_align.h"

using namespace WelsCommon;

//Tests of WelsGetCacheLineSize Begin
TEST (MemoryAlignTest, GetCacheLineSize_LoopWithin16K) {
  const unsigned int kuiTestBoundary16K = 16 * 1024;
  unsigned int uiTargetAlign = 1;
  while (uiTargetAlign < kuiTestBoundary16K) {
    CMemoryAlign cTestMa (uiTargetAlign);
    ASSERT_EQ ((uiTargetAlign & 0x0F) ? 16 : uiTargetAlign, cTestMa.WelsGetCacheLineSize());
    ++ uiTargetAlign;
  }
}

TEST (MemoryAlignTest, GetCacheLineSize_Zero) {
  CMemoryAlign cTestMa (0);
  const uint32_t kuiSixteen = 16;
  ASSERT_EQ (kuiSixteen, cTestMa.WelsGetCacheLineSize());
}
TEST (MemoryAlignTest, GetCacheLineSize_MaxUINT) {
  CMemoryAlign cTestMa (0xFFFFFFFF);
  const uint32_t kuiSixteen = 16;
  ASSERT_EQ (kuiSixteen, cTestMa.WelsGetCacheLineSize());
}
//Tests of WelsGetCacheLineSize End
//Tests of WelsMallocAndFree Begin
TEST (MemoryAlignTest, WelsMallocAndFreeOnceFunctionVerify) {
  const uint32_t kuiTargetAlignSize[4] = {32, 16, 64, 8};
  const uint32_t kuiZero = 0;
  for (int i = 0; i < 4; i++) {
    const uint32_t kuiTestAlignSize = kuiTargetAlignSize[i];
    const uint32_t kuiTestDataSize  = abs (rand());

    CMemoryAlign cTestMa (kuiTestAlignSize);
    const uint32_t uiSize = kuiTestDataSize;
    const char strUnitTestTag[100] = "pUnitTestData";
    const uint32_t kuiUsedCacheLineSize = ((kuiTestAlignSize == 0)
                                           || (kuiTestAlignSize & 0x0F)) ? (16) : (kuiTestAlignSize);
    const uint32_t kuiExtraAlignSize    = kuiUsedCacheLineSize - 1;
    const uint32_t kuiExpectedSize      = sizeof (void**) + sizeof (int32_t) + kuiExtraAlignSize + uiSize;
    uint8_t* pUnitTestData = static_cast<uint8_t*> (cTestMa.WelsMalloc (uiSize, strUnitTestTag));
    if (pUnitTestData != NULL) {
      ASSERT_TRUE ((((uintptr_t) (pUnitTestData)) & kuiExtraAlignSize) == 0);
      EXPECT_EQ (kuiExpectedSize, cTestMa.WelsGetMemoryUsage());
      cTestMa.WelsFree (pUnitTestData, strUnitTestTag);
      EXPECT_EQ (kuiZero, cTestMa.WelsGetMemoryUsage());
    } else {
      EXPECT_EQ (NULL, pUnitTestData);
      EXPECT_EQ (kuiZero, cTestMa.WelsGetMemoryUsage());
      cTestMa.WelsFree (pUnitTestData, strUnitTestTag);
      EXPECT_EQ (kuiZero, cTestMa.WelsGetMemoryUsage());
    }
  }
}
