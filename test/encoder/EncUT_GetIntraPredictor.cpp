#include <gtest/gtest.h>
#include <stdlib.h>

#include "get_intra_predictor.h"
#include "ls_defines.h"
#include "macros.h"

using namespace WelsEnc;

TEST (GetIntraPredictorTest, TestGetI4x4LumaPredV) {
  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[64];
  for (int i = 0; i < 64; i++)
    pRef[i] = rand() % 256;

  const int32_t kkiStride = 0;
  WelsI4x4LumaPredV_c (pPred, pRef, kkiStride);

  for (int i = 0; i < 4; i++)
    EXPECT_EQ (LD32 (&pPred[4 * i]), LD32 (&pRef[-kkiStride]));

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI4x4LumaPredH) {
  const int32_t kiStride = rand() % 256 + 16;
  const uint32_t kiStride2 = (kiStride << 1) - 1;
  const uint32_t kiStride3 = kiStride + kiStride2;

  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[kiStride3 + 2];

  for (int i = 0; i < (static_cast<int32_t> (kiStride3 + 2)); i++)
    pRef[i] = rand() % 256;

  pRef++;

  const uint8_t kuiH1 = pRef[-1];
  const uint8_t kuiH2 = pRef[kiStride - 1];
  const uint8_t kuiH3 = pRef[kiStride2];
  const uint8_t kuiH4 = pRef[kiStride3];
  const uint8_t kuiV1[4] = {kuiH1, kuiH1, kuiH1, kuiH1};
  const uint8_t kuiV2[4] = {kuiH2, kuiH2, kuiH2, kuiH2};
  const uint8_t kuiV3[4] = {kuiH3, kuiH3, kuiH3, kuiH3};
  const uint8_t kuiV4[4] = {kuiH4, kuiH4, kuiH4, kuiH4};

  ENFORCE_STACK_ALIGN_1D (uint8_t, uiV, 16, 16) // TobeCont'd about assign opt as follows
  ST32 (&uiV[0], LD32 (kuiV1));
  ST32 (&uiV[4], LD32 (kuiV2));
  ST32 (&uiV[8], LD32 (kuiV3));
  ST32 (&uiV[12], LD32 (kuiV4));

  WelsI4x4LumaPredH_c (pPred, pRef, kiStride);

  for (int i = 0; i < 4; i++)
    EXPECT_EQ (LD32 (&pPred[4 * i]), LD32 (&uiV[4 * i]));

  pRef--;

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI4x4LumaPredDDL) {
  const int32_t kiStride = 0;

  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[64];
  for (int i = 0; i < 64; i++)
    pRef[i] = rand() % 256;

  const uint8_t kuiT0   = pRef[-kiStride];
  const uint8_t kuiT1   = pRef[1 - kiStride];
  const uint8_t kuiT2   = pRef[2 - kiStride];
  const uint8_t kuiT3   = pRef[3 - kiStride];
  const uint8_t kuiT4   = pRef[4 - kiStride];
  const uint8_t kuiT5   = pRef[5 - kiStride];
  const uint8_t kuiT6   = pRef[6 - kiStride];
  const uint8_t kuiT7   = pRef[7 - kiStride];
  const uint8_t kuiDDL0 = (2 + kuiT0 + kuiT2 + (kuiT1 << 1)) >> 2;
  const uint8_t kuiDDL1 = (2 + kuiT1 + kuiT3 + (kuiT2 << 1)) >> 2;
  const uint8_t kuiDDL2 = (2 + kuiT2 + kuiT4 + (kuiT3 << 1)) >> 2;
  const uint8_t kuiDDL3 = (2 + kuiT3 + kuiT5 + (kuiT4 << 1)) >> 2;
  const uint8_t kuiDDL4 = (2 + kuiT4 + kuiT6 + (kuiT5 << 1)) >> 2;
  const uint8_t kuiDDL5 = (2 + kuiT5 + kuiT7 + (kuiT6 << 1)) >> 2;
  const uint8_t kuiDDL6 = (2 + kuiT6 + kuiT7 + (kuiT7 << 1)) >> 2;
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiV, 16, 16) // TobeCont'd about assign opt as follows
  uiV[0] = kuiDDL0;
  uiV[1] = uiV[4] = kuiDDL1;
  uiV[2] = uiV[5] = uiV[8] = kuiDDL2;
  uiV[3] = uiV[6] = uiV[9] = uiV[12] = kuiDDL3;
  uiV[7] = uiV[10] = uiV[13] = kuiDDL4;
  uiV[11] = uiV[14] = kuiDDL5;
  uiV[15] = kuiDDL6;

  WelsI4x4LumaPredDDL_c (pPred, pRef, kiStride);

  for (int i = 0; i < 4; i++)
    EXPECT_EQ (LD32 (&pPred[4 * i]), LD32 (&uiV[4 * i]));

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI4x4LumaPredDDLTop) {
  const int32_t kiStride = 0;

  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[64];
  for (int i = 0; i < 64; i++)
    pRef[i] = rand() % 256;

  const uint8_t kuiT0   = pRef[-kiStride];
  const uint8_t kuiT1   = pRef[1 - kiStride];
  const uint8_t kuiT2   = pRef[2 - kiStride];
  const uint8_t kuiT3   = pRef[3 - kiStride];
  const uint8_t kuiDLT0 = (2 + kuiT0 + kuiT2 + (kuiT1 << 1)) >> 2;
  const uint8_t kuiDLT1 = (2 + kuiT1 + kuiT3 + (kuiT2 << 1)) >> 2;
  const uint8_t kuiDLT2 = (2 + kuiT2 + kuiT3 + (kuiT3 << 1)) >> 2;
  const uint8_t kuiDLT3 = (2 + (kuiT3 << 2)) >> 2;
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiV, 16, 16) // TobeCont'd about assign opt as follows
  memset (&uiV[6], kuiDLT3, 10 * sizeof (uint8_t));
  uiV[0] = kuiDLT0;
  uiV[1] = uiV[4] = kuiDLT1;
  uiV[2] = uiV[5] = uiV[8] = kuiDLT2;
  uiV[3] = kuiDLT3;

  WelsI4x4LumaPredDDLTop_c (pPred, pRef, kiStride);

  for (int i = 0; i < 4; i++)
    EXPECT_EQ (LD32 (&pPred[4 * i]), LD32 (&uiV[4 * i]));

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI4x4LumaPredDDR) {
  const int32_t kiStride = rand() % 256 + 16;
  const int32_t kiStride2 = kiStride << 1;
  const int32_t kiStride3 = kiStride + kiStride2;

  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[kiStride3 + kiStride + 1];

  for (int i = 0; i < kiStride3 + kiStride + 1; i++)
    pRef[i] = rand() % 256;

  pRef += kiStride + 1;

  const uint8_t kuiLT   = pRef[-kiStride - 1];
  const uint8_t kuiL0   = pRef[-1];
  const uint8_t kuiL1   = pRef[kiStride - 1];
  const uint8_t kuiL2   = pRef[kiStride2 - 1];
  const uint8_t kuiL3   = pRef[kiStride3 - 1];
  const uint8_t kuiT0   = pRef[-kiStride];
  const uint8_t kuiT1   = pRef[1 - kiStride];
  const uint8_t kuiT2   = pRef[2 - kiStride];
  const uint8_t kuiT3   = pRef[3 - kiStride];
  const uint16_t kuiTL0 = 1 + kuiLT + kuiL0;
  const uint16_t kuiLT0 = 1 + kuiLT + kuiT0;
  const uint16_t kuiT01 = 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12 = 1 + kuiT1 + kuiT2;
  const uint16_t kuiT23 = 1 + kuiT2 + kuiT3;
  const uint16_t kuiL01 = 1 + kuiL0 + kuiL1;
  const uint16_t kuiL12 = 1 + kuiL1 + kuiL2;
  const uint16_t kuiL23 = 1 + kuiL2 + kuiL3;
  const uint8_t kuiDDR0 = (kuiTL0 + kuiLT0) >> 2;
  const uint8_t kuiDDR1 = (kuiLT0 + kuiT01) >> 2;
  const uint8_t kuiDDR2 = (kuiT01 + kuiT12) >> 2;
  const uint8_t kuiDDR3 = (kuiT12 + kuiT23) >> 2;
  const uint8_t kuiDDR4 = (kuiTL0 + kuiL01) >> 2;
  const uint8_t kuiDDR5 = (kuiL01 + kuiL12) >> 2;
  const uint8_t kuiDDR6 = (kuiL12 + kuiL23) >> 2;
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiV, 16, 16) // TobeCont'd about assign opt as follows
  uiV[0] = uiV[5] = uiV[10] = uiV[15] = kuiDDR0;
  uiV[1] = uiV[6] = uiV[11] = kuiDDR1;
  uiV[2] = uiV[7] = kuiDDR2;
  uiV[3] = kuiDDR3;
  uiV[4] = uiV[9] = uiV[14] = kuiDDR4;
  uiV[8] = uiV[13] = kuiDDR5;
  uiV[12] = kuiDDR6;

  WelsI4x4LumaPredDDR_c (pPred, pRef, kiStride);

  for (int i = 0; i < 4; i++)
    EXPECT_EQ (LD32 (&pPred[4 * i]), LD32 (&uiV[4 * i]));

  pRef -= kiStride + 1;

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI4x4LumaPredVL) {
  const int32_t kiStride = 0;

  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[64];
  for (int i = 0; i < 64; i++)
    pRef[i] = rand() % 256;

  const uint8_t kuiT0  = pRef[-kiStride];
  const uint8_t kuiT1  = pRef[1 - kiStride];
  const uint8_t kuiT2  = pRef[2 - kiStride];
  const uint8_t kuiT3  = pRef[3 - kiStride];
  const uint8_t kuiT4  = pRef[4 - kiStride];
  const uint8_t kuiT5  = pRef[5 - kiStride];
  const uint8_t kuiT6  = pRef[6 - kiStride];
  const uint8_t kuiVL0 = (1 + kuiT0 + kuiT1) >> 1;
  const uint8_t kuiVL1 = (1 + kuiT1 + kuiT2) >> 1;
  const uint8_t kuiVL2 = (1 + kuiT2 + kuiT3) >> 1;
  const uint8_t kuiVL3 = (1 + kuiT3 + kuiT4) >> 1;
  const uint8_t kuiVL4 = (1 + kuiT4 + kuiT5) >> 1;
  const uint8_t kuiVL5 = (2 + kuiT0 + (kuiT1 << 1) + kuiT2) >> 2;
  const uint8_t kuiVL6 = (2 + kuiT1 + (kuiT2 << 1) + kuiT3) >> 2;
  const uint8_t kuiVL7 = (2 + kuiT2 + (kuiT3 << 1) + kuiT4) >> 2;
  const uint8_t kuiVL8 = (2 + kuiT3 + (kuiT4 << 1) + kuiT5) >> 2;
  const uint8_t kuiVL9 = (2 + kuiT4 + (kuiT5 << 1) + kuiT6) >> 2;
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiV, 16, 16) // TobeCont'd about assign opt as follows
  uiV[0] = kuiVL0;
  uiV[1] = uiV[8] = kuiVL1;
  uiV[2] = uiV[9] = kuiVL2;
  uiV[3] = uiV[10] = kuiVL3;
  uiV[4] = kuiVL5;
  uiV[5] = uiV[12] = kuiVL6;
  uiV[6] = uiV[13] = kuiVL7;
  uiV[7] = uiV[14] = kuiVL8;
  uiV[11] = kuiVL4;
  uiV[15] = kuiVL9;

  WelsI4x4LumaPredVL_c (pPred, pRef, kiStride);

  for (int i = 0; i < 4; i++)
    EXPECT_EQ (LD32 (&pPred[4 * i]), LD32 (&uiV[4 * i]));

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI4x4LumaPredVLTop) {
  const int32_t kiStride = 0;

  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[64];
  for (int i = 0; i < 64; i++)
    pRef[i] = rand() % 256;

  pRef++;

  uint8_t* pTopLeft = &pRef[-kiStride - 1]; // top-left

  const uint8_t kuiT0   = * (pTopLeft + 1);
  const uint8_t kuiT1   = * (pTopLeft + 2);
  const uint8_t kuiT2   = * (pTopLeft + 3);
  const uint8_t kuiT3   = * (pTopLeft + 4);
  const uint8_t kuiVLT0 = (1 + kuiT0 + kuiT1) >> 1;
  const uint8_t kuiVLT1 = (1 + kuiT1 + kuiT2) >> 1;
  const uint8_t kuiVLT2 = (1 + kuiT2 + kuiT3) >> 1;
  const uint8_t kuiVLT3 = (1 + (kuiT3 << 1)) >> 1;
  const uint8_t kuiVLT4 = (2 + kuiT0 + (kuiT1 << 1) + kuiT2) >> 2;
  const uint8_t kuiVLT5 = (2 + kuiT1 + (kuiT2 << 1) + kuiT3) >> 2;
  const uint8_t kuiVLT6 = (2 + kuiT2 + (kuiT3 << 1) + kuiT3) >> 2;
  const uint8_t kuiVLT7 = (2 + (kuiT3 << 2)) >> 2;
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiV, 16, 16) // TobeCont'd about assign opt as follows
  uiV[0] = kuiVLT0;
  uiV[1] = uiV[8] = kuiVLT1;
  uiV[2] = uiV[9] = kuiVLT2;
  uiV[3] = uiV[10] = uiV[11] = kuiVLT3;
  uiV[4] = kuiVLT4;
  uiV[5] = uiV[12] = kuiVLT5;
  uiV[6] = uiV[13] = kuiVLT6;
  uiV[7] = uiV[14] = uiV[15] = kuiVLT7;

  WelsI4x4LumaPredVLTop_c (pPred, pRef, kiStride);

  for (int i = 0; i < 4; i++)
    EXPECT_EQ (LD32 (&pPred[4 * i]), LD32 (&uiV[4 * i]));

  pRef--;

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI4x4LumaPredVR) {
  const int32_t kiStride = rand() % 256 + 16;
  const int32_t kiStride2 = kiStride << 1;

  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[kiStride2 + kiStride + 1];

  for (int i = 0; i < kiStride2 + kiStride + 1; i++)
    pRef[i] = rand() % 256;

  pRef += kiStride + 1;

  const uint8_t kuiLT  = pRef[-kiStride - 1]; // top-left
  const uint8_t kuiL0  = pRef[-1];
  const uint8_t kuiL1  = pRef[kiStride - 1];
  const uint8_t kuiL2  = pRef[kiStride2 - 1];
  const uint8_t kuiT0  = pRef[-kiStride];
  const uint8_t kuiT1  = pRef[1 - kiStride];
  const uint8_t kuiT2  = pRef[2 - kiStride];
  const uint8_t kuiT3  = pRef[3 - kiStride];
  const uint8_t kuiVR0 = (1 + kuiLT + kuiT0) >> 1;
  const uint8_t kuiVR1 = (1 + kuiT0 + kuiT1) >> 1;
  const uint8_t kuiVR2 = (1 + kuiT1 + kuiT2) >> 1;
  const uint8_t kuiVR3 = (1 + kuiT2 + kuiT3) >> 1;
  const uint8_t kuiVR4 = (2 + kuiL0 + (kuiLT << 1) + kuiT0) >> 2;
  const uint8_t kuiVR5 = (2 + kuiLT + (kuiT0 << 1) + kuiT1) >> 2;
  const uint8_t kuiVR6 = (2 + kuiT0 + (kuiT1 << 1) + kuiT2) >> 2;
  const uint8_t kuiVR7 = (2 + kuiT1 + (kuiT2 << 1) + kuiT3) >> 2;
  const uint8_t kuiVR8 = (2 + kuiLT + (kuiL0 << 1) + kuiL1) >> 2;
  const uint8_t kuiVR9 = (2 + kuiL0 + (kuiL1 << 1) + kuiL2) >> 2;
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiV, 16, 16) // TobeCont'd about assign opt as follows
  uiV[0] = uiV[9] = kuiVR0;
  uiV[1] = uiV[10] = kuiVR1;
  uiV[2] = uiV[11] = kuiVR2;
  uiV[3] = kuiVR3;
  uiV[4] = uiV[13] = kuiVR4;
  uiV[5] = uiV[14] = kuiVR5;
  uiV[6] = uiV[15] = kuiVR6;
  uiV[7] = kuiVR7;
  uiV[8] = kuiVR8;
  uiV[12] = kuiVR9;

  WelsI4x4LumaPredVR_c (pPred, pRef, kiStride);

  for (int i = 0; i < 4; i++)
    EXPECT_EQ (LD32 (&pPred[4 * i]), LD32 (&uiV[4 * i]));

  pRef -= kiStride + 1;

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI4x4LumaPredHU) {
  const int32_t kiStride = rand() % 256 + 16;
  const int32_t kiStride2 = kiStride << 1;
  const int32_t kiStride3 = kiStride + kiStride2;

  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[kiStride3 + 1];

  for (int i = 0; i < kiStride3 + 1; i++)
    pRef[i] = rand() % 256;

  pRef++;

  const uint8_t kuiL0   = pRef[-1];
  const uint8_t kuiL1   = pRef[kiStride - 1];
  const uint8_t kuiL2   = pRef[kiStride2 - 1];
  const uint8_t kuiL3   = pRef[kiStride3 - 1];
  const uint16_t kuiL01 = (1 + kuiL0 + kuiL1);
  const uint16_t kuiL12 = (1 + kuiL1 + kuiL2);
  const uint16_t kuiL23 = (1 + kuiL2 + kuiL3);
  const uint8_t kuiHU0  = kuiL01 >> 1;
  const uint8_t kuiHU1  = (kuiL01 + kuiL12) >> 2;
  const uint8_t kuiHU2  = kuiL12 >> 1;
  const uint8_t kuiHU3  = (kuiL12 + kuiL23) >> 2;
  const uint8_t kuiHU4  = kuiL23 >> 1;
  const uint8_t kuiHU5  = (1 + kuiL23 + (kuiL3 << 1)) >> 2;
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiV, 16, 16) // TobeCont'd about assign opt as follows
  uiV[0] = kuiHU0;
  uiV[1] = kuiHU1;
  uiV[2] = uiV[4] = kuiHU2;
  uiV[3] = uiV[5] = kuiHU3;
  uiV[6] = uiV[8] = kuiHU4;
  uiV[7] = uiV[9] = kuiHU5;
  memset (&uiV[10], kuiL3, 6 * sizeof (uint8_t));

  WelsI4x4LumaPredHU_c (pPred, pRef, kiStride);

  for (int i = 0; i < 4; i++)
    EXPECT_EQ (LD32 (&pPred[4 * i]), LD32 (&uiV[4 * i]));

  pRef--;

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI4x4LumaPredHD) {
  const int32_t kiStride = rand() % 256 + 16;
  const int32_t kiStride2 = kiStride << 1;
  const int32_t kiStride3 = kiStride + kiStride2;

  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[kiStride3 + kiStride + 1];

  for (int i = 0; i < kiStride3 + kiStride + 1; i++)
    pRef[i] = rand() % 256;

  pRef += kiStride + 1;

  const uint8_t kuiLT  = pRef[-kiStride - 1]; // top-left
  const uint8_t kuiL0  = pRef[-1];
  const uint8_t kuiL1  = pRef[kiStride - 1];
  const uint8_t kuiL2  = pRef[kiStride2 - 1];
  const uint8_t kuiL3  = pRef[kiStride3 - 1];
  const uint8_t kuiT0  = pRef[-kiStride];
  const uint8_t kuiT1  = pRef[1 - kiStride];
  const uint8_t kuiT2  = pRef[2 - kiStride];
  const uint8_t kuiHD0 = (1 + kuiLT + kuiL0) >> 1;
  const uint8_t kuiHD1 = (2 + kuiL0 + (kuiLT << 1) + kuiT0) >> 2;
  const uint8_t kuiHD2 = (2 + kuiLT + (kuiT0 << 1) + kuiT1) >> 2;
  const uint8_t kuiHD3 = (2 + kuiT0 + (kuiT1 << 1) + kuiT2) >> 2;
  const uint8_t kuiHD4 = (1 + kuiL0 + kuiL1) >> 1;
  const uint8_t kuiHD5 = (2 + kuiLT + (kuiL0 << 1) + kuiL1) >> 2;
  const uint8_t kuiHD6 = (1 + kuiL1 + kuiL2) >> 1;
  const uint8_t kuiHD7 = (2 + kuiL0 + (kuiL1 << 1) + kuiL2) >> 2;
  const uint8_t kuiHD8 = (1 + kuiL2 + kuiL3) >> 1;
  const uint8_t kuiHD9 = (2 + kuiL1 + (kuiL2 << 1) + kuiL3) >> 2;
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiV, 16, 16) // TobeCont'd about assign opt as follows
  uiV[0] = uiV[6] = kuiHD0;
  uiV[1] = uiV[7] = kuiHD1;
  uiV[2] = kuiHD2;
  uiV[3] = kuiHD3;
  uiV[4] = uiV[10] = kuiHD4;
  uiV[5] = uiV[11] = kuiHD5;
  uiV[8] = uiV[14] = kuiHD6;
  uiV[9] = uiV[15] = kuiHD7;
  uiV[12] = kuiHD8;
  uiV[13] = kuiHD9;

  WelsI4x4LumaPredHD_c (pPred, pRef, kiStride);

  for (int i = 0; i < 4; i++)
    EXPECT_EQ (LD32 (&pPred[4 * i]), LD32 (&uiV[4 * i]));

  pRef -= kiStride + 1;

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetIChromaPredV) {
  uint8_t* pPred = new uint8_t[64];
  uint8_t* pRef  = new uint8_t[64];
  for (int i = 0; i < 64; i++)
    pRef[i] = rand() % 256 + 1;

  const int32_t kiStride = 0;
  WelsIChromaPredV_c (pPred, pRef, kiStride);

  for (int i = 0; i < 8; i++)
    EXPECT_EQ (LD32 (&pPred[8 * i]), LD32 (&pRef[-kiStride]));

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI16x16LumaPredPlane) {
  const int32_t kiStride = rand() % 16 + 16;

  int32_t i, j;

  uint8_t* pPred = new uint8_t[16 * kiStride];
  uint8_t* pRef  = new uint8_t[16 * kiStride + 1];
  for (i = 0; i < 16 * kiStride + 1; i++)
    pRef[i] = rand() % 256 + 1;

  pRef += kiStride + 1;

  int32_t iA = 0, iB = 0, iC = 0, iH = 0, iV = 0;
  uint8_t* pTop = &pRef[-kiStride];
  uint8_t* pLeft = &pRef[-1];
  int32_t iPredStride = 16;

  for (i = 0; i < 8; i++) {
    iH += (i + 1) * (pTop[8 + i] - pTop[6 - i]);
    iV += (i + 1) * (pLeft[ (8 + i) * kiStride] - pLeft[ (6 - i) * kiStride]);
  }

  iA = (pLeft[15 * kiStride] + pTop[15]) << 4;
  iB = (5 * iH + 32) >> 6;
  iC = (5 * iV + 32) >> 6;

  WelsI16x16LumaPredPlane_c (pPred, pRef, kiStride);
  for (i = 0; i < 16; i++) {
    for (j = 0; j < 16; j++) {
      EXPECT_EQ (pPred[j], (uint8_t)WelsClip1 ((iA + iB * (j - 7) + iC * (i - 7) + 16) >> 5));
    }
    pPred += iPredStride;
  }

  pRef -= kiStride + 1;
  pPred -= (iPredStride * 16);
  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI16x16LumaPredDc) {
  const int32_t kiStride = rand() % 16 + 16;

  int i;

  uint8_t* pPred = new uint8_t[256];
  uint8_t* pRef  = new uint8_t[16 * kiStride];
  for (i = 0; i < 16 * kiStride; i++)
    pRef[i] = rand() % 256 + 1;

  pRef += kiStride;

  int32_t iTmp = (kiStride << 4) - kiStride;
  int32_t iSum = 0;
  i = 15;
  uint8_t uiMean = 0;

  do {
    iSum += pRef[-1 + iTmp] + pRef[-kiStride + i];
    iTmp -= kiStride;
  } while (i-- > 0);
  uiMean = (16 + iSum) >> 5;

  WelsI16x16LumaPredDc_c (pPred, pRef, kiStride);
  for (i = 0; i < 256; i++)
    EXPECT_EQ (pPred[i], uiMean);

  pRef -= kiStride;

  delete []pRef;
  delete []pPred;
}

TEST (GetIntraPredictorTest, TestGetI16x16LumaPredDcTop) {
  const int32_t kiStride = rand() % 16 + 16;

  int i;

  uint8_t* pPred = new uint8_t[256];
  uint8_t* pRef  = new uint8_t[16 * kiStride];
  for (i = 0; i < 16 * kiStride; i++)
    pRef[i] = rand() % 256 + 1;

  pRef += kiStride;

  int32_t iSum = 0;
  i = 15;
  uint8_t uiMean = 0;
  do {
    iSum += pRef[-kiStride + i];
  } while (i-- > 0);
  uiMean = (8 + iSum) >> 4;

  WelsI16x16LumaPredDcTop_c (pPred, pRef, kiStride);
  for (i = 0; i < 256; i++)
    EXPECT_EQ (pPred[i], uiMean);

  pRef -= kiStride;

  delete []pRef;
  delete []pPred;
}
