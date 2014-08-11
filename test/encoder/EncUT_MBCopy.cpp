#include <gtest/gtest.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "cpu_core.h"
#include "cpu.h"
#include "macros.h"
#include "encode_mb_aux.h"
#include "wels_func_ptr_def.h"
#include "copy_mb.h"

using namespace WelsEnc;
#define MBCOPYTEST_NUM 1000
static void FillWithRandomData (uint8_t* p, int32_t Len) {
  for (int32_t i = 0; i < Len; i++) {
    p[i] = rand() % 256;
  }
}


TEST (MBCopyFunTest, pfCopy8x8Aligned) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pSrcAlign, 16 * 64 + 1, 16)
  ENFORCE_STACK_ALIGN_2D (uint8_t, pDstAlign, 2, 16 * 32 + 16, 16)

  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < MBCOPYTEST_NUM; k++) {
    memset (pDstAlign[0], 0, 16 * 32 + 1);
    memset (pDstAlign[1], 0, 16 * 32 + 1);
    FillWithRandomData ((uint8_t*)pSrcAlign, 16 * 64 + 1);
    WelsCopy8x8_c (pDstAlign[0], 32, pSrcAlign, 64);
    sFuncPtrList.pfCopy8x8Aligned (pDstAlign[1], 32, pSrcAlign, 64);

    for (int32_t i = 0; i < 16 * 32 + 1; i++) {
      ASSERT_EQ (pDstAlign[0][i], pDstAlign[1][i]);
    }

  }

}

TEST (MBCopyFunTest, pfCopy8x16Aligned) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pSrcAlign, 16 * 64 + 1, 16)
  ENFORCE_STACK_ALIGN_2D (uint8_t, pDstAlign, 2, 16 * 32 + 16, 16)

  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < MBCOPYTEST_NUM; k++) {
    memset (pDstAlign[0], 0, 16 * 32 + 1);
    memset (pDstAlign[1], 0, 16 * 32 + 1);
    FillWithRandomData ((uint8_t*)pSrcAlign, 16 * 64 + 1);
    WelsCopy8x16_c (pDstAlign[0], 32, pSrcAlign, 64);
    sFuncPtrList.pfCopy8x16Aligned (pDstAlign[1], 32, pSrcAlign, 64);

    for (int32_t i = 0; i < 16 * 32 + 1; i++) {
      ASSERT_EQ (pDstAlign[0][i], pDstAlign[1][i]);
    }

  }

}

TEST (MBCopyFunTest, pfCopy16x16Aligned) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pSrcAlign, 16 * 64 + 1, 16)
  ENFORCE_STACK_ALIGN_2D (uint8_t, pDstAlign, 2, 16 * 32 + 16, 16)

  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < MBCOPYTEST_NUM; k++) {
    memset (pDstAlign[0], 0, 16 * 32 + 1);
    memset (pDstAlign[1], 0, 16 * 32 + 1);
    FillWithRandomData ((uint8_t*)pSrcAlign, 16 * 64 + 1);
    WelsCopy16x16_c (pDstAlign[0], 32, pSrcAlign, 64);
    sFuncPtrList.pfCopy16x16Aligned (pDstAlign[1], 32, pSrcAlign, 64);

    for (int32_t i = 0; i < 16 * 32 + 1; i++) {
      ASSERT_EQ (pDstAlign[0][i], pDstAlign[1][i]);
    }

  }

}

TEST (MBCopyFunTest, pfCopy16x8NotAligned) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pSrcAlign, 16 * 64 + 1, 16)
  ENFORCE_STACK_ALIGN_2D (uint8_t, pDstAlign, 2, 16 * 32 + 16, 16)

  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < MBCOPYTEST_NUM; k++) {
    memset (pDstAlign[0], 0, 16 * 32 + 1);
    memset (pDstAlign[1], 0, 16 * 32 + 1);
    FillWithRandomData ((uint8_t*)pSrcAlign, 16 * 64 + 1);
    WelsCopy16x8_c (pDstAlign[0], 32, pSrcAlign + 1, 64);
    sFuncPtrList.pfCopy16x8NotAligned (pDstAlign[1], 32, pSrcAlign + 1, 64);

    for (int32_t i = 0; i < 16 * 32 + 1; i++) {
      ASSERT_EQ (pDstAlign[0][i], pDstAlign[1][i]);
    }

  }

}

TEST (MBCopyFunTest, pfCopy16x16NotAligned) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pSrcAlign, 16 * 64 + 1, 16)
  ENFORCE_STACK_ALIGN_2D (uint8_t, pDstAlign, 2, 16 * 32 + 16, 16)

  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < MBCOPYTEST_NUM; k++) {
    memset (pDstAlign[0], 0, 16 * 32 + 1);
    memset (pDstAlign[1], 0, 16 * 32 + 1);
    FillWithRandomData ((uint8_t*)pSrcAlign, 16 * 64 + 1);
    WelsCopy16x16_c (pDstAlign[0], 32, pSrcAlign + 1, 64);
    sFuncPtrList.pfCopy16x16NotAligned (pDstAlign[1], 32, pSrcAlign + 1, 64);

    for (int32_t i = 0; i < 16 * 32 + 1; i++) {
      ASSERT_EQ (pDstAlign[0][i], pDstAlign[1][i]);
    }

  }

}
