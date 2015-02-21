#include <gtest/gtest.h>
#include <math.h>
#include "svc_enc_golomb.h"
#include "macros.h"

using namespace WelsEnc;

const double g_kdLog2Factor = 1.0 / log (2.0);

TEST (UeExpGolombTest, TestBsSizeUeLt256) {
  uint32_t uiInVal = 0;
  for (; uiInVal < 256; ++ uiInVal) {
    const uint32_t uiActVal = BsSizeUE (uiInVal);
    const int32_t m = static_cast<int32_t> (log ((uiInVal + 1) * 1.0) * g_kdLog2Factor + 1e-6);
    const uint32_t uiExpVal = (m << 1) + 1;
    EXPECT_EQ (uiActVal, uiExpVal);
  }
}

TEST (UeExpGolombTest, TestBsSizeUeRangeFrom256To65534) {
  uint32_t uiInVal = 0x100;
  for (; uiInVal < 0xFFFF; ++ uiInVal) {
    const uint32_t uiActVal = BsSizeUE (uiInVal);
    const int32_t m = static_cast<int32_t> (log ((uiInVal + 1) * 1.0) * g_kdLog2Factor + 1e-6);
    const uint32_t uiExpVal = (m << 1) + 1;
    EXPECT_EQ (uiActVal, uiExpVal);
  }
}

TEST (UeExpGolombTest, TestBsSizeUeRangeFrom65535ToPlus256) {
  uint32_t uiInVal = 0xFFFF;
  const uint32_t uiCountBase = 256;
  const uint32_t uiInValEnd = uiInVal + uiCountBase;
  for (; uiInVal < uiInValEnd; ++ uiInVal) {
    const uint32_t uiActVal = BsSizeUE (uiInVal);
    // float precision issue in case use math::log
    const int32_t m = WELS_LOG2 (1 + uiInVal);
    const uint32_t uiExpVal = (m << 1) + 1;
    EXPECT_EQ (uiActVal, uiExpVal);
  }
}
