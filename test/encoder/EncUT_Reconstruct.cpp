#include <gtest/gtest.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "cpu_core.h"
#include "cpu.h"
#include "macros.h"
#include "encode_mb_aux.h"
#include "decode_mb_aux.h"
#include "wels_func_ptr_def.h"

using namespace WelsEnc;
#define RECONTEST_NUM 1000
static void FillWithRandomData (uint8_t* p, int32_t Len) {
  for (int32_t i = 0; i < Len; i++) {
    p[i] = rand() % 256;
  }
}

TEST (ReconstructionFunTest, WelsIDctRecI16x16Dc) {
  ENFORCE_STACK_ALIGN_2D (uint8_t, pRec, 2, 16 * 16, 16)
  ENFORCE_STACK_ALIGN_1D (uint8_t, pPred, 32 * 16, 16)
  ENFORCE_STACK_ALIGN_1D (int16_t, pDct, 16, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitReconstructionFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    FillWithRandomData (pPred, 32 * 16);
    FillWithRandomData ((uint8_t*)pDct, 16 * 2);
    for (int32_t i = 0 ; i < 16; i++) {
      pDct[i] = WELS_CLIP3 (pDct[i], -4080, 4080);
    }
    WelsIDctRecI16x16Dc_c (pRec[0], 16, pPred, 32, pDct);
    sFuncPtrList.pfIDctI16x16Dc (pRec[1], 16, pPred, 32, pDct);

    for (int32_t j = 0 ; j < 16; j++) {
      for (int32_t i = 0 ; i < 16; i++) {
        ASSERT_EQ (pRec[0][i + j * 16], pRec[1][i + j * 16]);
      }
    }
  }

}

TEST (ReconstructionFunTest, WelsGetNoneZeroCount) {
  ENFORCE_STACK_ALIGN_1D (int16_t, pInput, 64, 16)
  int32_t iZeroCount[2];
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    FillWithRandomData ((uint8_t*)pInput, 128);
    iZeroCount[0] = WelsGetNoneZeroCount_c (pInput);
    iZeroCount[1] = sFuncPtrList.pfGetNoneZeroCount (pInput);
    ASSERT_EQ (iZeroCount[0], iZeroCount[1]);
  }

}

TEST (ReconstructionFunTest, WelsHadamardT4Dc) {
  ENFORCE_STACK_ALIGN_1D (int16_t, pDct, 16 * 16, 16)
  ENFORCE_STACK_ALIGN_2D (int16_t, pLumaDc, 2, 16, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    FillWithRandomData ((uint8_t*)pDct, 16 * 16 * 2);
    for (int32_t j = 0 ; j < 16; j++) {
      for (int32_t i = 0 ; i < 16; i++) {
        pDct[i + j * 16] = WELS_CLIP3 (pDct[i + j * 16], -4080, 4080);
      }
    }
    WelsHadamardT4Dc_c (pLumaDc[0], pDct);
    sFuncPtrList.pfTransformHadamard4x4Dc (pLumaDc[1], pDct);
    for (int32_t i = 0 ; i < 16; i++) {
      ASSERT_EQ (pLumaDc[0][i], pLumaDc[1][i]);
    }
  }


}

TEST (ReconstructionFunTest, WelsDctT4) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pInput1, 16 * 4, 16)
  ENFORCE_STACK_ALIGN_1D (uint8_t, pInput2, 32 * 4, 16)
  ENFORCE_STACK_ALIGN_2D (int16_t, pOut, 2, 16, 16)

  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    FillWithRandomData (pInput1, 16 * 4);
    FillWithRandomData (pInput2, 32 * 4);
    WelsDctT4_c (pOut[0], pInput1, 16, pInput2, 32);
    sFuncPtrList.pfDctT4 (pOut[1], pInput1, 16, pInput2, 32);
    for (int32_t i = 0 ; i < 16; i++) {
      ASSERT_EQ (pOut[0][i], pOut[1][i]);
    }
  }


  memset (pInput1, 255, 16 * 4);
  memset (pInput2, 0, 32 * 4);
  WelsDctT4_c (pOut[0], pInput1, 16, pInput2, 32);
  sFuncPtrList.pfDctT4 (pOut[1], pInput1, 16, pInput2, 32);
  for (int32_t i = 0 ; i < 16; i++) {
    ASSERT_EQ (pOut[0][i], pOut[1][i]);
  }

  memset (pInput1, 0, 16 * 4);
  memset (pInput2, 255, 32 * 4);
  WelsDctT4_c (pOut[0], pInput1, 16, pInput2, 32);
  sFuncPtrList.pfDctT4 (pOut[1], pInput1, 16, pInput2, 32);
  for (int32_t i = 0 ; i < 16; i++) {
    ASSERT_EQ (pOut[0][i], pOut[1][i]);
  }
}

TEST (ReconstructionFunTest, WelsDctFourT4) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pInput1, 16 * 8, 16)
  ENFORCE_STACK_ALIGN_1D (uint8_t, pInput2, 32 * 8, 16)
  ENFORCE_STACK_ALIGN_2D (int16_t, pOut, 2, 64, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    FillWithRandomData (pInput1, 16 * 8);
    FillWithRandomData (pInput2, 32 * 8);
    WelsDctFourT4_c (pOut[0], pInput1, 16, pInput2, 32);
    sFuncPtrList.pfDctFourT4 (pOut[1], pInput1, 16, pInput2, 32);
    for (int32_t i = 0 ; i < 64; i++) {
      ASSERT_EQ (pOut[0][i], pOut[1][i]);
    }
  }

}

TEST (ReconstructionFunTest, WelsIDctT4Rec) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pDct, 2, 16, 16)
  ENFORCE_STACK_ALIGN_1D (uint8_t, pPred, 32 * 4, 16)
  ENFORCE_STACK_ALIGN_2D (uint8_t, pRec, 2, 16 * 4, 16)
  ENFORCE_STACK_ALIGN_1D (uint8_t, pInput1, 16 * 4, 16)
  ENFORCE_STACK_ALIGN_1D (uint8_t, pInput2, 32 * 4, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitReconstructionFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    FillWithRandomData (pPred, 32 * 4);
    FillWithRandomData (pInput1, 16 * 4);
    FillWithRandomData (pInput2, 32 * 4);
    WelsDctT4_c (pDct[0], pInput1, 16, pInput2, 32);
    sFuncPtrList.pfDctT4 (pDct[1], pInput1, 16, pInput2, 32);
    WelsIDctT4Rec_c (pRec[0], 16, pPred, 32, pDct[0]);
    sFuncPtrList.pfIDctT4 (pRec[1], 16, pPred, 32, pDct[1]);

    for (int32_t j = 0 ; j < 4; j++) {
      for (int32_t i = 0 ; i < 4; i++) {
        ASSERT_EQ (pRec[0][i + j * 16], pRec[1][i + j * 16]);
      }
    }
  }


  memset (pPred, 255, 32 * 4);
  memset (pInput1, 255, 16 * 4);
  memset (pInput2, 0, 32 * 4);
  WelsDctT4_c (pDct[0], pInput1, 16, pInput2, 32);
  sFuncPtrList.pfDctT4 (pDct[1], pInput1, 16, pInput2, 32);
  WelsIDctT4Rec_c (pRec[0], 16, pPred, 32, pDct[0]);
  sFuncPtrList.pfIDctT4 (pRec[1], 16, pPred, 32, pDct[1]);

  for (int32_t j = 0 ; j < 4; j++) {
    for (int32_t i = 0 ; i < 4; i++) {
      ASSERT_EQ (pRec[0][i + j * 16], pRec[1][i + j * 16]);
    }
  }

  memset (pPred, 255, 32 * 4);
  memset (pInput1, 0, 16 * 4);
  memset (pInput2, 255, 32 * 4);
  WelsDctT4_c (pDct[0], pInput1, 16, pInput2, 32);
  sFuncPtrList.pfDctT4 (pDct[1], pInput1, 16, pInput2, 32);
  WelsIDctT4Rec_c (pRec[0], 16, pPred, 32, pDct[0]);
  sFuncPtrList.pfIDctT4 (pRec[1], 16, pPred, 32, pDct[1]);

  for (int32_t j = 0 ; j < 4; j++) {
    for (int32_t i = 0 ; i < 4; i++) {
      ASSERT_EQ (pRec[0][i + j * 16], pRec[1][i + j * 16]);
    }
  }

}


TEST (ReconstructionFunTest, WelsIDctFourT4Rec) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pDct, 2, 64, 16)
  ENFORCE_STACK_ALIGN_1D (uint8_t, pPred, 32 * 8, 16)
  ENFORCE_STACK_ALIGN_2D (uint8_t, pRec, 2, 16 * 8, 16)
  ENFORCE_STACK_ALIGN_1D (uint8_t, pInput1, 16 * 8, 16)
  ENFORCE_STACK_ALIGN_1D (uint8_t, pInput2, 32 * 8, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitReconstructionFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    FillWithRandomData (pInput1, 16 * 8);
    FillWithRandomData (pInput2, 32 * 8);
    FillWithRandomData (pPred, 32 * 8);
    WelsDctFourT4_c (pDct[0], pInput1, 16, pInput2, 32);
    sFuncPtrList.pfDctFourT4 (pDct[1], pInput1, 16, pInput2, 32);
    WelsIDctFourT4Rec_c (pRec[0], 16, pPred, 32, pDct[0]);
    sFuncPtrList.pfIDctFourT4 (pRec[1], 16, pPred, 32, pDct[1]);
    for (int32_t j = 0 ; j < 8; j++) {
      for (int32_t i = 0 ; i < 8; i++) {
        ASSERT_EQ (pRec[0][i + j * 16], pRec[1][i + j * 16]);
      }
    }
  }

}


TEST (ReconstructionFunTest, WelsDequant4x4) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pInput, 2, 16, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitReconstructionFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    uint8_t uiQp = rand() % 52;
    FillWithRandomData ((uint8_t*)pInput[0], 32);
    for (int32_t i = 0 ; i < 16; i++) {
      pInput[0][i] = WELS_CLIP3 (pInput[0][i], -32000, 32000);
    }
    memcpy ((uint8_t*)pInput[1], (uint8_t*)pInput[0], 32);

    const int16_t* pMF = g_kiQuantMF[uiQp];
    const int16_t* pFF = g_iQuantIntraFF[uiQp];
    WelsQuant4x4_c (pInput[0], pFF, pMF);
    sFuncPtrList.pfQuantization4x4 (pInput[1], pFF, pMF);

    WelsDequant4x4_c (pInput[0], g_kuiDequantCoeff[uiQp]);
    sFuncPtrList.pfDequantization4x4 (pInput[1], g_kuiDequantCoeff[uiQp]);
    for (int32_t i = 0 ; i < 16; i++) {
      ASSERT_EQ (pInput[0][i], pInput[1][i]);
    }
  }

}

TEST (ReconstructionFunTest, WelsDequantIHadamard4x4) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pInput, 2, 16, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitReconstructionFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    uint8_t uiQp = rand() % 52;
    FillWithRandomData ((uint8_t*)pInput[0], 32);
    for (int32_t i = 0 ; i < 16; i++) {
      pInput[0][i] = WELS_CLIP3 (pInput[0][i], -32000, 32000);
    }
    memcpy ((uint8_t*)pInput[1], (uint8_t*)pInput[0], 32);

    const int16_t* pMF = g_kiQuantMF[uiQp];
    const int16_t* pFF = g_iQuantIntraFF[uiQp];
    WelsQuant4x4_c (pInput[0], pFF, pMF);
    sFuncPtrList.pfQuantization4x4 (pInput[1], pFF, pMF);

    WelsDequantIHadamard4x4_c (pInput[0], g_kuiDequantCoeff[uiQp][0]);
    sFuncPtrList.pfDequantizationIHadamard4x4 (pInput[1], g_kuiDequantCoeff[uiQp][0]);
    for (int32_t i = 0 ; i < 16; i++) {
      ASSERT_EQ (pInput[0][i], pInput[1][i]);
    }
  }

}

TEST (ReconstructionFunTest, WelsQuant4x4) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pInput, 2, 16, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    uint8_t uiQp = rand() % 52;
    FillWithRandomData ((uint8_t*)pInput[0], 32);
    for (int32_t i = 0 ; i < 16; i++) {
      pInput[0][i] = WELS_CLIP3 (pInput[0][i], -32000, 32000);
    }
    memcpy ((uint8_t*)pInput[1], (uint8_t*)pInput[0], 32);

    const int16_t* pMF = g_kiQuantMF[uiQp];
    const int16_t* pFF = g_iQuantIntraFF[uiQp];
    WelsQuant4x4_c (pInput[0], pFF, pMF);
    sFuncPtrList.pfQuantization4x4 (pInput[1], pFF, pMF);
    for (int32_t i = 0 ; i < 16; i++) {
      ASSERT_EQ (pInput[0][i], pInput[1][i]);
    }
  }

}


TEST (ReconstructionFunTest, WelsQuant4x4Dc) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pInput, 2, 16, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    uint8_t uiQp = rand() % 52;
    FillWithRandomData ((uint8_t*)pInput[0], 32);
    for (int32_t i = 0 ; i < 16; i++) {
      pInput[0][i] = WELS_CLIP3 (pInput[0][i], -32000, 32000);
    }
    memcpy ((uint8_t*)pInput[1], (uint8_t*)pInput[0], 32);

    const int16_t* pMF = g_kiQuantMF[uiQp];
    const int16_t* pFF = g_iQuantIntraFF[uiQp];
    WelsQuant4x4Dc_c (pInput[0], pFF[0], pMF[0]);
    sFuncPtrList.pfQuantizationDc4x4 (pInput[1], pFF[0], pMF[0]);
    for (int32_t i = 0 ; i < 16; i++) {
      ASSERT_EQ (pInput[0][i], pInput[1][i]);
    }
  }

}

TEST (ReconstructionFunTest, WelsQuantFour4x4) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pInput, 2, 64, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    uint8_t uiQp = rand() % 52;
    FillWithRandomData ((uint8_t*)pInput[0], 128);
    for (int32_t i = 0 ; i < 64; i++) {
      pInput[0][i] = WELS_CLIP3 (pInput[0][i], -32000, 32000);
    }
    memcpy ((uint8_t*)pInput[1], (uint8_t*)pInput[0], 128);

    const int16_t* pMF = g_kiQuantMF[uiQp];
    const int16_t* pFF = g_iQuantIntraFF[uiQp];
    WelsQuantFour4x4_c (pInput[0], pFF, pMF);
    sFuncPtrList.pfQuantizationFour4x4 (pInput[1], pFF, pMF);
    for (int32_t i = 0 ; i < 64; i++) {
      ASSERT_EQ (pInput[0][i], pInput[1][i]);
    }
  }

}

TEST (ReconstructionFunTest, WelsQuantFour4x4Max) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pInput, 2, 64, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  int16_t pMax[2][4];
  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    uint8_t uiQp = rand() % 52;
    FillWithRandomData ((uint8_t*)pInput[0], 128);
    for (int32_t i = 0 ; i < 64; i++) {
      pInput[0][i] = WELS_CLIP3 (pInput[0][i], -32000, 32000);
    }
    memcpy ((uint8_t*)pInput[1], (uint8_t*)pInput[0], 128);

    const int16_t* pMF = g_kiQuantMF[uiQp];
    const int16_t* pFF = g_iQuantIntraFF[uiQp];
    WelsQuantFour4x4Max_c (pInput[0], pFF, pMF, pMax[0]);
    sFuncPtrList.pfQuantizationFour4x4Max (pInput[1], pFF, pMF, pMax[1]);
    for (int32_t i = 0 ; i < 64; i++) {
      ASSERT_EQ (pInput[0][i], pInput[1][i]);
      ASSERT_EQ (pMax[0][i >> 4], pMax[1][i >> 4]);
    }
  }
}

TEST (ReconstructionFunTest, WelsDeQuantFour4x4) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pInput, 2, 64, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);
  WelsInitReconstructionFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    uint8_t uiQp = rand() % 52;
    FillWithRandomData ((uint8_t*)pInput[0], 128);
    for (int32_t i = 0 ; i < 64; i++) {
      pInput[0][i] = WELS_CLIP3 (pInput[0][i], -32000, 32000);
    }
    memcpy ((uint8_t*)pInput[1], (uint8_t*)pInput[0], 128);

    const int16_t* pMF = g_kiQuantMF[uiQp];
    const int16_t* pFF = g_iQuantIntraFF[uiQp];
    WelsQuantFour4x4_c (pInput[0], pFF, pMF);
    sFuncPtrList.pfQuantizationFour4x4 (pInput[1], pFF, pMF);

    WelsDequantFour4x4_c (pInput[0], g_kuiDequantCoeff[uiQp]);
    sFuncPtrList.pfDequantizationFour4x4 (pInput[1], g_kuiDequantCoeff[uiQp]);
    for (int32_t i = 0 ; i < 64; i++) {
      ASSERT_EQ (pInput[0][i], pInput[1][i]);
    }
  }
}

TEST (ReconstructionFunTest, WelsHadamardQuant2x2Skip) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pInput, 2, 64, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    uint8_t uiQp = rand() % 52;
    FillWithRandomData ((uint8_t*)pInput[0], 128);
    for (int32_t i = 0 ; i < 64; i++) {
      pInput[0][i] = WELS_CLIP3 (pInput[0][i], -4080, 4080);
    }
    memcpy ((uint8_t*)pInput[1], (uint8_t*)pInput[0], 128);

    const int16_t* pMF = g_kiQuantMF[uiQp];
    const int16_t* pFF = g_iQuantIntraFF[uiQp];
    int32_t iSkip_c = WelsHadamardQuant2x2Skip_c (pInput[0], pFF[0], pMF[0]);
    int32_t iSkip_test = sFuncPtrList.pfQuantizationHadamard2x2Skip (pInput[1], pFF[0], pMF[0]);

    ASSERT_EQ ((iSkip_test != 0), (iSkip_c != 0));
  }

}

TEST (ReconstructionFunTest, WelsHadamardQuant2x2) {
  ENFORCE_STACK_ALIGN_2D (int16_t, pInput, 2, 64, 16)
  ENFORCE_STACK_ALIGN_2D (int16_t, pDct, 2, 4, 16)
  ENFORCE_STACK_ALIGN_2D (int16_t, pBlock, 2, 4, 16)
  int32_t iCpuCores = 0;
  SWelsFuncPtrList sFuncPtrList;
  uint32_t m_uiCpuFeatureFlag = WelsCPUFeatureDetect (&iCpuCores);
  WelsInitEncodingFuncs (&sFuncPtrList, m_uiCpuFeatureFlag);

  for (int32_t k = 0; k < RECONTEST_NUM; k++) {
    uint8_t uiQp = rand() % 52;
    FillWithRandomData ((uint8_t*)pInput[0], 128);
    for (int32_t i = 0 ; i < 64; i++) {
      pInput[0][i] = WELS_CLIP3 (pInput[0][i], -4080, 4080);
    }
    memcpy ((uint8_t*)pInput[1], (uint8_t*)pInput[0], 128);

    const int16_t* pMF = g_kiQuantMF[uiQp];
    const int16_t* pFF = g_iQuantIntraFF[uiQp];
    int32_t iSkip_c = WelsHadamardQuant2x2_c (pInput[0], pFF[0], pMF[0], pDct[0], pBlock[0]);
    int32_t iSkip_test = sFuncPtrList.pfQuantizationHadamard2x2 (pInput[1], pFF[0], pMF[0], pDct[1], pBlock[1]);

    ASSERT_EQ ((iSkip_test != 0), (iSkip_c != 0));
    for (int32_t i = 0 ; i < 64; i++) {
      ASSERT_EQ (pInput[0][i], pInput[1][i]);
    }
    for (int32_t i = 0 ; i < 4; i++) {
      ASSERT_EQ (pDct[0][i], pDct[1][i]);
      ASSERT_EQ (pBlock[0][i], pBlock[1][i]);
    }
  }

}

