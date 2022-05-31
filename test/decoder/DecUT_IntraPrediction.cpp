#include <gtest/gtest.h>
#include "cpu.h"
#include "cpu_core.h"
#include "get_intra_predictor.h"
#include "typedefs.h"
#include "ls_defines.h"
#include "macros.h"

using namespace WelsDec;
#define GENERATE_4x4_UT(pred, ref, ASM, CPUFLAGS) \
  TEST(DecoderIntraPredictionTest, pred) { \
  const int32_t kiStride = 32; \
  int32_t iRunTimes = 1000; \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pPredBuffer, 12 * kiStride, 4); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pRefBuffer, 12 * kiStride, 4); \
if (ASM) {\
  int32_t  iNumberofCPUCore = 1; \
  uint32_t uiCPUFlags = WelsCPUFeatureDetect( &iNumberofCPUCore); \
  if ((uiCPUFlags & CPUFLAGS) == 0) {\
    return; \
  } \
}\
  while(iRunTimes--) {\
  for (int i = 0; i < 12; i++) {\
    pRefBuffer[kiStride * 3 + i] = pPredBuffer[kiStride * 3 + i] = rand() & 255; \
    pRefBuffer[i * kiStride + 3] = pPredBuffer[i * kiStride + 3] = rand() & 255; \
  } \
  pred (&pPredBuffer[kiStride * 4 + 4], kiStride); \
  ref (&pRefBuffer[kiStride * 4 + 4], kiStride); \
  bool ok = true; \
  for (int i = 0; i < 4; i++) \
    for (int j = 0; j < 4; j++) \
      if (pPredBuffer[(i+4) * kiStride + j + 4] != pRefBuffer[(i+4) * kiStride + j + 4]) { \
        ok = false; \
        break; \
      } \
  EXPECT_EQ(ok, true);\
  } \
  }

#define PREDV(size) \
void LumaI##size##x##size##PredV(uint8_t *pPred, const int32_t kiStride) {\
  int i; \
  for (i = 0; i < size; i++) {\
    memcpy(pPred + i * kiStride, pPred - kiStride, size * sizeof(uint8_t)); \
  } \
}

#define PREDH(size) \
void LumaI##size##x##size##PredH(uint8_t *pPred, const int32_t kiStride) {\
  for (int i = 0; i < size; i++) { \
    memset(pPred + i * kiStride, pPred[i * kiStride - 1], size * sizeof(uint8_t));\
  }\
}

#define PREDDC(size, log) \
void LumaI##size##x##size##PredDC(uint8_t *pPred, const int32_t kiStride) {\
  int iSum = size; \
  for (int i = 0; i < size; i ++) \
    iSum += pPred[-1 + i * kiStride] + pPred[i - kiStride]; \
  uint8_t uiMean = iSum >>(log+1);\
  for (int i = 0; i < size; i ++) \
    memset(pPred + i * kiStride, uiMean, size * sizeof(uint8_t)); \
}

#define PREDDCLeft(size, log) \
void LumaI##size##x##size##PredDCLeft(uint8_t *pPred, const int32_t kiStride) {\
  int iSum = size/2; \
  for (int i = 0; i < size; i ++) \
    iSum += pPred[-1 + i * kiStride]; \
  uint8_t uiMean = iSum >>(log);\
  for (int i = 0; i < size; i ++) \
    memset(pPred + i * kiStride, uiMean, size * sizeof(uint8_t)); \
}

#define PREDDCTop(size, log) \
void LumaI##size##x##size##PredDCTop(uint8_t *pPred, const int32_t kiStride) {\
  int iSum = size/2; \
  for (int i = 0; i < size; i ++) \
    iSum += pPred[i - kiStride]; \
  uint8_t uiMean = iSum >>(log);\
  for (int i = 0; i < size; i ++) \
    memset(pPred + i * kiStride, uiMean, size * sizeof(uint8_t)); \
}

#define PREDDCNone(size, log) \
void LumaI##size##x##size##PredDCNone(uint8_t *pPred, const int32_t kiStride) {\
  uint8_t uiMean = 128;\
  for (int i = 0; i < size; i ++) \
    memset(pPred + i * kiStride, uiMean, size * sizeof(uint8_t)); \
}


/*down pLeft*/
void WelsI4x4LumaPredDDL_ref (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2 = kiStride << 1;
  const int32_t kiStride3 = kiStride + kiStride2;
  /*get pTop*/
  uint8_t* ptop          = &pPred[-kiStride];
  const uint8_t kuiT0    = *ptop;
  const uint8_t kuiT1    = * (ptop + 1);
  const uint8_t kuiT2    = * (ptop + 2);
  const uint8_t kuiT3    = * (ptop + 3);
  const uint8_t kuiT4    = * (ptop + 4);
  const uint8_t kuiT5    = * (ptop + 5);
  const uint8_t kuiT6    = * (ptop + 6);
  const uint8_t kuiT7    = * (ptop + 7);
  const uint8_t kuiDDL0 = (2 + kuiT0 + kuiT2 + (kuiT1 << 1)) >> 2;  // kDDL0
  const uint8_t kuiDDL1 = (2 + kuiT1 + kuiT3 + (kuiT2 << 1)) >> 2;  // kDDL1
  const uint8_t kuiDDL2 = (2 + kuiT2 + kuiT4 + (kuiT3 << 1)) >> 2;  // kDDL2
  const uint8_t kuiDDL3 = (2 + kuiT3 + kuiT5 + (kuiT4 << 1)) >> 2;  // kDDL3
  const uint8_t kuiDDL4 = (2 + kuiT4 + kuiT6 + (kuiT5 << 1)) >> 2;  // kDDL4
  const uint8_t kuiDDL5 = (2 + kuiT5 + kuiT7 + (kuiT6 << 1)) >> 2;  // kDDL5
  const uint8_t kuiDDL6 = (2 + kuiT6 + kuiT7 + (kuiT7 << 1)) >> 2;  // kDDL6
  const uint8_t kuiList[8] = { kuiDDL0, kuiDDL1, kuiDDL2, kuiDDL3, kuiDDL4, kuiDDL5, kuiDDL6, 0 };

  ST32 (pPred            , LD32 (kuiList));
  ST32 (pPred + kiStride , LD32 (kuiList + 1));
  ST32 (pPred + kiStride2, LD32 (kuiList + 2));
  ST32 (pPred + kiStride3, LD32 (kuiList + 3));
}

/*down pLeft*/
void WelsI4x4LumaPredDDLTop_ref (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2 = kiStride << 1;
  const int32_t kiStride3 = kiStride + kiStride2;
  /*get pTop*/
  uint8_t* ptop         = &pPred[-kiStride];
  const uint8_t kuiT0   = *ptop;
  const uint8_t kuiT1   = * (ptop + 1);
  const uint8_t kuiT2   = * (ptop + 2);
  const uint8_t kuiT3   = * (ptop + 3);
  const uint16_t kuiT01 = 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12 = 1 + kuiT1 + kuiT2;
  const uint16_t kuiT23 = 1 + kuiT2 + kuiT3;
  const uint16_t kuiT33 = 1 + (kuiT3 << 1);
  const uint8_t kuiDLT0 = (kuiT01 + kuiT12) >> 2;   // kDLT0
  const uint8_t kuiDLT1 = (kuiT12 + kuiT23) >> 2;   // kDLT1
  const uint8_t kuiDLT2 = (kuiT23 + kuiT33) >> 2;   // kDLT2
  const uint8_t kuiDLT3 = kuiT33 >> 1;          // kDLT3
  const uint8_t kuiList[8] = { kuiDLT0, kuiDLT1, kuiDLT2, kuiDLT3, kuiDLT3, kuiDLT3, kuiDLT3 , kuiDLT3 };

  ST32 (pPred,             LD32 (kuiList));
  ST32 (pPred + kiStride,  LD32 (kuiList + 1));
  ST32 (pPred + kiStride2, LD32 (kuiList + 2));
  ST32 (pPred + kiStride3, LD32 (kuiList + 3));
}


/*down right*/
void WelsI4x4LumaPredDDR_ref (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2 = kiStride << 1;
  const int32_t kiStride3 = kiStride + kiStride2;
  uint8_t* ptopleft       = &pPred[- (kiStride + 1)];
  uint8_t* pleft          = &pPred[-1];
  const uint8_t kuiLT     = *ptopleft;
  /*get pLeft and pTop*/
  const uint8_t kuiL0   = *pleft;
  const uint8_t kuiL1   = * (pleft + kiStride);
  const uint8_t kuiL2   = * (pleft + kiStride2);
  const uint8_t kuiL3   = * (pleft + kiStride3);
  const uint8_t kuiT0   = * (ptopleft + 1);
  const uint8_t kuiT1   = * (ptopleft + 2);
  const uint8_t kuiT2   = * (ptopleft + 3);
  const uint8_t kuiT3   = * (ptopleft + 4);
  const uint16_t kuiTL0 = 1 + kuiLT + kuiL0;
  const uint16_t kuiLT0 = 1 + kuiLT + kuiT0;
  const uint16_t kuiT01 = 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12 = 1 + kuiT1 + kuiT2;
  const uint16_t kuiT23 = 1 + kuiT2 + kuiT3;
  const uint16_t kuiL01 = 1 + kuiL0 + kuiL1;
  const uint16_t kuiL12 = 1 + kuiL1 + kuiL2;
  const uint16_t kuiL23 = 1 + kuiL2 + kuiL3;
  const uint8_t kuiDDR0 = (kuiTL0 + kuiLT0) >> 2;   // kuiDDR0
  const uint8_t kuiDDR1 = (kuiLT0 + kuiT01) >> 2;   // kuiDDR1
  const uint8_t kuiDDR2 = (kuiT01 + kuiT12) >> 2;   // kuiDDR2
  const uint8_t kuiDDR3 = (kuiT12 + kuiT23) >> 2;   // kuiDDR3
  const uint8_t kuiDDR4 = (kuiTL0 + kuiL01) >> 2;   // kuiDDR4
  const uint8_t kuiDDR5 = (kuiL01 + kuiL12) >> 2;   // kuiDDR5
  const uint8_t kuiDDR6 = (kuiL12 + kuiL23) >> 2;   // kuiDDR6
  const uint8_t kuiList[8] = { kuiDDR6, kuiDDR5, kuiDDR4, kuiDDR0, kuiDDR1, kuiDDR2, kuiDDR3, 0 };

  ST32 (pPred            , LD32 (kuiList + 3));
  ST32 (pPred + kiStride , LD32 (kuiList + 2));
  ST32 (pPred + kiStride2, LD32 (kuiList + 1));
  ST32 (pPred + kiStride3, LD32 (kuiList));
}


/*vertical pLeft*/
void WelsI4x4LumaPredVL_ref (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2   = kiStride << 1;
  const int32_t kiStride3   = kiStride + kiStride2;
  uint8_t* ptopleft         = &pPred[- (kiStride + 1)];
  /*get pTop*/
  const uint8_t kuiT0       = * (ptopleft + 1);
  const uint8_t kuiT1       = * (ptopleft + 2);
  const uint8_t kuiT2       = * (ptopleft + 3);
  const uint8_t kuiT3       = * (ptopleft + 4);
  const uint8_t kuiT4       = * (ptopleft + 5);
  const uint8_t kuiT5       = * (ptopleft + 6);
  const uint8_t kuiT6       = * (ptopleft + 7);
  const uint16_t kuiT01     = 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12     = 1 + kuiT1 + kuiT2;
  const uint16_t kuiT23     = 1 + kuiT2 + kuiT3;
  const uint16_t kuiT34     = 1 + kuiT3 + kuiT4;
  const uint16_t kuiT45     = 1 + kuiT4 + kuiT5;
  const uint16_t kuiT56     = 1 + kuiT5 + kuiT6;
  const uint8_t kuiVL0      = kuiT01 >> 1;              // kuiVL0
  const uint8_t kuiVL1      = kuiT12 >> 1;              // kuiVL1
  const uint8_t kuiVL2      = kuiT23 >> 1;              // kuiVL2
  const uint8_t kuiVL3      = kuiT34 >> 1;              // kuiVL3
  const uint8_t kuiVL4      = kuiT45 >> 1;              // kuiVL4
  const uint8_t kuiVL5      = (kuiT01 + kuiT12) >> 2;   // kuiVL5
  const uint8_t kuiVL6      = (kuiT12 + kuiT23) >> 2;   // kuiVL6
  const uint8_t kuiVL7      = (kuiT23 + kuiT34) >> 2;   // kuiVL7
  const uint8_t kuiVL8      = (kuiT34 + kuiT45) >> 2;   // kuiVL8
  const uint8_t kuiVL9      = (kuiT45 + kuiT56) >> 2;   // kuiVL9
  const uint8_t kuiList[10] = { kuiVL0, kuiVL1, kuiVL2, kuiVL3, kuiVL4, kuiVL5, kuiVL6, kuiVL7, kuiVL8, kuiVL9 };

  ST32 (pPred,             LD32 (kuiList));
  ST32 (pPred + kiStride,  LD32 (kuiList + 5));
  ST32 (pPred + kiStride2, LD32 (kuiList + 1));
  ST32 (pPred + kiStride3, LD32 (kuiList + 6));
}

/*vertical pLeft*/
void WelsI4x4LumaPredVLTop_ref (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2   = kiStride << 1;
  const int32_t kiStride3   = kiStride + kiStride2;
  uint8_t* ptopleft         = &pPred[- (kiStride + 1)];
  /*get pTop*/
  const uint8_t kuiT0       = * (ptopleft + 1);
  const uint8_t kuiT1       = * (ptopleft + 2);
  const uint8_t kuiT2       = * (ptopleft + 3);
  const uint8_t kuiT3       = * (ptopleft + 4);
  const uint16_t kuiT01     = 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12     = 1 + kuiT1 + kuiT2;
  const uint16_t kuiT23     = 1 + kuiT2 + kuiT3;
  const uint16_t kuiT33     = 1 + (kuiT3 << 1);
  const uint8_t kuiVL0      = kuiT01 >> 1;
  const uint8_t kuiVL1      = kuiT12 >> 1;
  const uint8_t kuiVL2      = kuiT23 >> 1;
  const uint8_t kuiVL3      = kuiT33 >> 1;
  const uint8_t kuiVL4      = (kuiT01 + kuiT12) >> 2;
  const uint8_t kuiVL5      = (kuiT12 + kuiT23) >> 2;
  const uint8_t kuiVL6      = (kuiT23 + kuiT33) >> 2;
  const uint8_t kuiVL7      = kuiVL3;
  const uint8_t kuiList[10] = { kuiVL0, kuiVL1, kuiVL2, kuiVL3, kuiVL3, kuiVL4, kuiVL5, kuiVL6, kuiVL7, kuiVL7 };

  ST32 (pPred            , LD32 (kuiList));
  ST32 (pPred + kiStride , LD32 (kuiList + 5));
  ST32 (pPred + kiStride2, LD32 (kuiList + 1));
  ST32 (pPred + kiStride3, LD32 (kuiList + 6));
}


/*vertical right*/
void WelsI4x4LumaPredVR_ref (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2   = kiStride << 1;
  const int32_t kiStride3   = kiStride + kiStride2;
  const uint8_t kuiLT       = pPred[-kiStride - 1];
  /*get pLeft and pTop*/
  const uint8_t kuiL0       = pPred[          - 1];
  const uint8_t kuiL1       = pPred[kiStride  - 1];
  const uint8_t kuiL2       = pPred[kiStride2 - 1];
  const uint8_t kuiT0       = pPred[ -kiStride];
  const uint8_t kuiT1       = pPred[1 - kiStride];
  const uint8_t kuiT2       = pPred[2 - kiStride];
  const uint8_t kuiT3       = pPred[3 - kiStride];
  const uint8_t kuiVR0      = (1 + kuiLT + kuiT0) >> 1;                 // kuiVR0
  const uint8_t kuiVR1      = (1 + kuiT0 + kuiT1) >> 1;                 // kuiVR1
  const uint8_t kuiVR2      = (1 + kuiT1 + kuiT2) >> 1;                 // kuiVR2
  const uint8_t kuiVR3      = (1 + kuiT2 + kuiT3) >> 1;                 // kuiVR3
  const uint8_t kuiVR4      = (2 + kuiL0 + (kuiLT << 1) + kuiT0) >> 2;  // kuiVR4
  const uint8_t kuiVR5      = (2 + kuiLT + (kuiT0 << 1) + kuiT1) >> 2;  // kuiVR5
  const uint8_t kuiVR6      = (2 + kuiT0 + (kuiT1 << 1) + kuiT2) >> 2;  // kuiVR6
  const uint8_t kuiVR7      = (2 + kuiT1 + (kuiT2 << 1) + kuiT3) >> 2;  // kuiVR7
  const uint8_t kuiVR8      = (2 + kuiLT + (kuiL0 << 1) + kuiL1) >> 2;  // kuiVR8
  const uint8_t kuiVR9      = (2 + kuiL0 + (kuiL1 << 1) + kuiL2) >> 2;  // kuiVR9
  const uint8_t kuiList[10] = { kuiVR8, kuiVR0, kuiVR1, kuiVR2, kuiVR3, kuiVR9, kuiVR4, kuiVR5, kuiVR6, kuiVR7 };

  ST32 (pPred            , LD32 (kuiList + 1));
  ST32 (pPred + kiStride , LD32 (kuiList + 6));
  ST32 (pPred + kiStride2, LD32 (kuiList));
  ST32 (pPred + kiStride3, LD32 (kuiList + 5));
}

/*horizontal up*/
void WelsI4x4LumaPredHU_ref (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2   = kiStride << 1;
  const int32_t kiStride3   = kiStride + kiStride2;
  /*get pLeft*/
  const uint8_t kuiL0       = pPred[          - 1];
  const uint8_t kuiL1       = pPred[kiStride  - 1];
  const uint8_t kuiL2       = pPred[kiStride2 - 1];
  const uint8_t kuiL3       = pPred[kiStride3 - 1];
  const uint16_t kuiL01     = 1 + kuiL0 + kuiL1;
  const uint16_t kuiL12     = 1 + kuiL1 + kuiL2;
  const uint16_t kuiL23     = 1 + kuiL2 + kuiL3;
  const uint8_t kuiHU0      = kuiL01 >> 1;
  const uint8_t kuiHU1      = (kuiL01 + kuiL12) >> 2;
  const uint8_t kuiHU2      = kuiL12 >> 1;
  const uint8_t kuiHU3      = (kuiL12 + kuiL23) >> 2;
  const uint8_t kuiHU4      = kuiL23 >> 1;
  const uint8_t kuiHU5      = (1 + kuiL23 + (kuiL3 << 1)) >> 2;
  const uint8_t kuiList[10] = { kuiHU0, kuiHU1, kuiHU2, kuiHU3, kuiHU4, kuiHU5, kuiL3, kuiL3, kuiL3, kuiL3 };

  ST32 (pPred            , LD32 (kuiList));
  ST32 (pPred + kiStride , LD32 (kuiList + 2));
  ST32 (pPred + kiStride2, LD32 (kuiList + 4));
  ST32 (pPred + kiStride3, LD32 (kuiList + 6));
}

/*horizontal down*/
void WelsI4x4LumaPredHD_ref (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiStride2   = kiStride << 1;
  const int32_t kiStride3   = kiStride + kiStride2;
  const uint8_t kuiLT       = pPred[- (kiStride + 1)];
  /*get pLeft and pTop*/
  const uint8_t kuiL0       = pPred[-1            ];
  const uint8_t kuiL1       = pPred[-1 + kiStride ];
  const uint8_t kuiL2       = pPred[-1 + kiStride2];
  const uint8_t kuiL3       = pPred[-1 + kiStride3];
  const uint8_t kuiT0       = pPred[-kiStride     ];
  const uint8_t kuiT1       = pPred[-kiStride + 1 ];
  const uint8_t kuiT2       = pPred[-kiStride + 2 ];
  const uint16_t kuiTL0     = 1 + kuiLT + kuiL0;
  const uint16_t kuiLT0     = 1 + kuiLT + kuiT0;
  const uint16_t kuiT01     = 1 + kuiT0 + kuiT1;
  const uint16_t kuiT12     = 1 + kuiT1 + kuiT2;
  const uint16_t kuiL01     = 1 + kuiL0 + kuiL1;
  const uint16_t kuiL12     = 1 + kuiL1 + kuiL2;
  const uint16_t kuiL23     = 1 + kuiL2 + kuiL3;
  const uint8_t kuiHD0      = kuiTL0 >> 1;
  const uint8_t kuiHD1      = (kuiTL0 + kuiLT0) >> 2;
  const uint8_t kuiHD2      = (kuiLT0 + kuiT01) >> 2;
  const uint8_t kuiHD3      = (kuiT01 + kuiT12) >> 2;
  const uint8_t kuiHD4      = kuiL01 >> 1;
  const uint8_t kuiHD5      = (kuiTL0 + kuiL01) >> 2;
  const uint8_t kuiHD6      = kuiL12 >> 1;
  const uint8_t kuiHD7      = (kuiL01 + kuiL12) >> 2;
  const uint8_t kuiHD8      = kuiL23 >> 1;
  const uint8_t kuiHD9      = (kuiL12 + kuiL23) >> 2;
  const uint8_t kuiList[10] = { kuiHD8, kuiHD9, kuiHD6, kuiHD7, kuiHD4, kuiHD5, kuiHD0, kuiHD1, kuiHD2, kuiHD3 };

  ST32 (pPred            , LD32 (kuiList + 6));
  ST32 (pPred + kiStride , LD32 (kuiList + 4));
  ST32 (pPred + kiStride2, LD32 (kuiList + 2));
  ST32 (pPred + kiStride3, LD32 (kuiList));
}
// Unit test for Luma 4x4 cases
PREDV (4)
GENERATE_4x4_UT (WelsI4x4LumaPredV_c, LumaI4x4PredV, 0, 0)

PREDH (4)
GENERATE_4x4_UT (WelsI4x4LumaPredH_c, LumaI4x4PredH, 0, 0)

PREDDC (4, 2)
GENERATE_4x4_UT (WelsI4x4LumaPredDc_c, LumaI4x4PredDC, 0, 0)

PREDDCLeft (4, 2)
GENERATE_4x4_UT (WelsI4x4LumaPredDcLeft_c, LumaI4x4PredDCLeft, 0, 0)

PREDDCTop (4, 2)
GENERATE_4x4_UT (WelsI4x4LumaPredDcTop_c, LumaI4x4PredDCTop, 0, 0)

PREDDCNone (4, 2)
GENERATE_4x4_UT (WelsI4x4LumaPredDcNA_c, LumaI4x4PredDCNone, 0, 0)
GENERATE_4x4_UT (WelsI4x4LumaPredDDL_c, WelsI4x4LumaPredDDL_ref, 0, 0)
GENERATE_4x4_UT (WelsI4x4LumaPredDDLTop_c, WelsI4x4LumaPredDDLTop_ref, 0, 0)
GENERATE_4x4_UT (WelsI4x4LumaPredDDR_c, WelsI4x4LumaPredDDR_ref, 0, 0)
GENERATE_4x4_UT (WelsI4x4LumaPredVR_c, WelsI4x4LumaPredVR_ref, 0, 0)
GENERATE_4x4_UT (WelsI4x4LumaPredVL_c, WelsI4x4LumaPredVL_ref, 0, 0)
GENERATE_4x4_UT (WelsI4x4LumaPredVLTop_c, WelsI4x4LumaPredVLTop_ref, 0, 0)
GENERATE_4x4_UT (WelsI4x4LumaPredHU_c, WelsI4x4LumaPredHU_ref, 0, 0)
GENERATE_4x4_UT (WelsI4x4LumaPredHD_c, WelsI4x4LumaPredHD_ref, 0, 0)

#define GENERATE_8x8_UT(pred, ref, ASM, CPUFLAGS) \
TEST(DecoderIntraPredictionTest, pred) {\
  const int32_t kiStride = 32; \
  int iRunTimes = 1000; \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pRefBuffer, 18 * kiStride, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pPredBuffer, 18 * kiStride, 16); \
  if (ASM) { \
    int32_t iTmp = 1; \
    uint32_t uiCPUFlags = WelsCPUFeatureDetect(&iTmp); \
    if ((uiCPUFlags & CPUFLAGS) == 0) {\
      return; \
    } \
  } \
  while(iRunTimes--) {\
    for (int i = 0; i < 17; i ++) {\
      pRefBuffer[kiStride + i] = pPredBuffer[kiStride + i] = rand() & 255; \
      pRefBuffer[(i+1) * kiStride - 1] = pPredBuffer[(i+1) * kiStride - 1] = rand() & 255; \
    }\
    pred(&pPredBuffer[2*kiStride], kiStride); \
    ref(&pRefBuffer[2*kiStride], kiStride); \
    bool ok = true; \
    for (int i = 0; i < 8; i ++)\
      for(int j = 0; j < 8; j ++)\
        if (pPredBuffer[(i+2) * kiStride + j] != pRefBuffer[(i+2) * kiStride + j]) {\
          ok = false; \
          break; \
        } \
    EXPECT_EQ(ok, true); \
  } \
}

void WelsIChromaPredPlane_ref (uint8_t* pPred, const int32_t kiStride) {
  int32_t a = 0, b = 0, c = 0, H = 0, V = 0;
  int32_t i, j;
  uint8_t* pTop = &pPred[-kiStride];
  uint8_t* pLeft = &pPred[-1];

  for (i = 0 ; i < 4 ; i ++) {
    H += (i + 1) * (pTop[4 + i] - pTop[2 - i]);
    V += (i + 1) * (pLeft[ (4 + i) * kiStride] - pLeft[ (2 - i) * kiStride]);
  }

  a = (pLeft[7 * kiStride] + pTop[7]) << 4;
  b = (17 * H + 16) >> 5;
  c = (17 * V + 16) >> 5;

  for (i = 0 ; i < 8 ; i ++) {
    for (j = 0 ; j < 8 ; j ++) {
      int32_t iTmp = (a + b * (j - 3) + c * (i - 3) + 16) >> 5;
      pPred[j] = (iTmp < 0) ? 0 : ((iTmp > 255) ? 255 : iTmp);
    }
    pPred += kiStride;
  }
}


void WelsIChromaPredDc_ref (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiL1        = kiStride - 1;
  const int32_t kiL2        = kiL1 + kiStride;
  const int32_t kiL3        = kiL2 + kiStride;
  const int32_t kiL4        = kiL3 + kiStride;
  const int32_t kiL5        = kiL4 + kiStride;
  const int32_t kiL6        = kiL5 + kiStride;
  const int32_t kiL7        = kiL6 + kiStride;
  /*caculate the kMean value*/
  const uint8_t kuiM1       = (pPred[-kiStride] + pPred[1 - kiStride] + pPred[2 - kiStride] + pPred[3 - kiStride] +
                           pPred[-1] + pPred[kiL1] + pPred[kiL2] + pPred[kiL3] + 4) >> 3 ;
  const uint32_t kuiSum2    = pPred[4 - kiStride] + pPred[5 - kiStride] + pPred[6 - kiStride] + pPred[7 - kiStride];
  const uint32_t kuiSum3    = pPred[kiL4] + pPred[kiL5] + pPred[kiL6] + pPred[kiL7];
  const uint8_t kuiM2       = (kuiSum2 + 2) >> 2;
  const uint8_t kuiM3       = (kuiSum3 + 2) >> 2;
  const uint8_t kuiM4       = (kuiSum2 + kuiSum3 + 4) >> 3;
  const uint8_t kuiMUP[8]   = {kuiM1, kuiM1, kuiM1, kuiM1, kuiM2, kuiM2, kuiM2, kuiM2};
  const uint8_t kuiMDown[8] = {kuiM3, kuiM3, kuiM3, kuiM3, kuiM4, kuiM4, kuiM4, kuiM4};
  const uint64_t kuiUP64    = LD64 (kuiMUP);
  const uint64_t kuiDN64    = LD64 (kuiMDown);

  ST64 (pPred           , kuiUP64);
  ST64 (pPred + kiL1 + 1, kuiUP64);
  ST64 (pPred + kiL2 + 1, kuiUP64);
  ST64 (pPred + kiL3 + 1, kuiUP64);
  ST64 (pPred + kiL4 + 1, kuiDN64);
  ST64 (pPred + kiL5 + 1, kuiDN64);
  ST64 (pPred + kiL6 + 1, kuiDN64);
  ST64 (pPred + kiL7 + 1, kuiDN64);
}

void WelsIChromaPredDcLeft_ref (uint8_t* pPred, const int32_t kiStride) {
  const int32_t kiL1    =   -1 + kiStride;
  const int32_t kiL2    = kiL1 + kiStride;
  const int32_t kiL3    = kiL2 + kiStride;
  const int32_t kiL4    = kiL3 + kiStride;
  const int32_t kiL5    = kiL4 + kiStride;
  const int32_t kiL6    = kiL5 + kiStride;
  const int32_t kiL7    = kiL6 + kiStride;
  /*caculate the kMean value*/
  const uint8_t kuiMUP   = (pPred[-1] + pPred[kiL1] + pPred[kiL2] + pPred[kiL3] + 2) >> 2 ;
  const uint8_t kuiMDown = (pPred[kiL4] + pPred[kiL5] + pPred[kiL6] + pPred[kiL7] + 2) >> 2;
  const uint64_t kuiUP64 = 0x0101010101010101ULL * kuiMUP;
  const uint64_t kuiDN64 = 0x0101010101010101ULL * kuiMDown;

  ST64 (pPred           , kuiUP64);
  ST64 (pPred + kiL1 + 1, kuiUP64);
  ST64 (pPred + kiL2 + 1, kuiUP64);
  ST64 (pPred + kiL3 + 1, kuiUP64);
  ST64 (pPred + kiL4 + 1, kuiDN64);
  ST64 (pPred + kiL5 + 1, kuiDN64);
  ST64 (pPred + kiL6 + 1, kuiDN64);
  ST64 (pPred + kiL7 + 1, kuiDN64);
}

void WelsIChromaPredDcTop_ref (uint8_t* pPred, const int32_t kiStride) {
  int32_t iTmp = (kiStride << 3) - kiStride;
  /*caculate the kMean value*/
  const uint8_t kuiM1 = (pPred[-kiStride] + pPred[1 - kiStride] + pPred[2 - kiStride] + pPred[3 - kiStride] + 2) >> 2;
  const uint8_t kuiM2 = (pPred[4 - kiStride] + pPred[5 - kiStride] + pPred[6 - kiStride] + pPred[7 - kiStride] + 2) >> 2;
  const uint8_t kuiM[8] = {kuiM1, kuiM1, kuiM1, kuiM1, kuiM2, kuiM2, kuiM2, kuiM2};

  uint8_t i = 7;

  do {
    ST64 (pPred + iTmp, LD64 (kuiM));

    iTmp -= kiStride;
  } while (i-- > 0);
}
PREDV (8)
PREDH (8)
PREDDCNone (8, 3)
GENERATE_8x8_UT (WelsIChromaPredDcNA_c, LumaI8x8PredDCNone, 0, 0)
GENERATE_8x8_UT (WelsIChromaPredPlane_c, WelsIChromaPredPlane_ref, 0, 0)
GENERATE_8x8_UT (WelsIChromaPredDc_c, WelsIChromaPredDc_ref, 0, 0)
GENERATE_8x8_UT (WelsIChromaPredDcTop_c, WelsIChromaPredDcTop_ref, 0, 0)
GENERATE_8x8_UT (WelsIChromaPredDcLeft_c, WelsIChromaPredDcLeft_ref, 0, 0)
GENERATE_8x8_UT (WelsIChromaPredH_c, LumaI8x8PredH, 0, 0)
GENERATE_8x8_UT (WelsIChromaPredV_c, LumaI8x8PredV, 0, 0)
#define GENERATE_16x16_UT(pred, ref, ASM, CPUFLAGS) \
TEST(DecoderIntraPredictionTest, pred) {\
  const int32_t kiStride = 32; \
  int32_t iRunTimes = 1000; \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pRefBuffer, 18 * kiStride, 16); \
  ENFORCE_STACK_ALIGN_1D (uint8_t, pPredBuffer, 18 * kiStride, 16); \
  if (ASM) { \
    int32_t iTmp = 1; \
    uint32_t uiCPUFlags = WelsCPUFeatureDetect( &iTmp); \
    if ((uiCPUFlags & CPUFLAGS) == 0) {\
      return ; \
    } \
  }\
  while(iRunTimes--) {\
    for (int i = 0; i < 17; i ++) {\
      pRefBuffer[kiStride + i] = pPredBuffer[kiStride + i] = rand() & 255; \
      pRefBuffer[(i+1) * kiStride - 1] = pPredBuffer[(i+1) * kiStride - 1] = rand() & 255; \
    }\
    pred(&pPredBuffer[2*kiStride], kiStride); \
    ref(&pRefBuffer[2*kiStride], kiStride); \
    bool ok = true; \
    for (int i = 0; i < 16; i ++)\
      for(int j = 0; j < 16; j ++)\
        if (pPredBuffer[(i+2) * kiStride + j] != pRefBuffer[(i+2) * kiStride + j]) {\
          ok = false; \
          break; \
        } \
    EXPECT_EQ(ok, true); \
  } \
}
void WelsI16x16LumaPredPlane_ref (uint8_t* pPred, const int32_t kiStride) {
  int32_t a = 0, b = 0, c = 0, H = 0, V = 0;
  int32_t i, j;
  uint8_t* pTop = &pPred[-kiStride];
  uint8_t* pLeft = &pPred[-1];

  for (i = 0 ; i < 8 ; i ++) {
    H += (i + 1) * (pTop[8 + i] - pTop[6 - i]);
    V += (i + 1) * (pLeft[ (8 + i) * kiStride] - pLeft[ (6 - i) * kiStride]);
  }

  a = (pLeft[15 * kiStride] + pTop[15]) << 4;
  b = (5 * H + 32) >> 6;
  c = (5 * V + 32) >> 6;

  for (i = 0 ; i < 16 ; i ++) {
    for (j = 0 ; j < 16 ; j ++) {
      int32_t iTmp = (a + b * (j - 7) + c * (i - 7) + 16) >> 5;
      pPred[j] = (iTmp < 0) ? 0 : ((iTmp > 255) ? 255 : iTmp);
    }
    pPred += kiStride;
  }
}

PREDV (16)
PREDH (16)
PREDDC (16, 4)
PREDDCTop (16, 4)
PREDDCLeft (16, 4)
PREDDCNone (16, 4)

GENERATE_16x16_UT (WelsI16x16LumaPredDcNA_c, LumaI16x16PredDCNone, 0, 0)
GENERATE_16x16_UT (WelsI16x16LumaPredPlane_c, WelsI16x16LumaPredPlane_ref, 0, 0)
GENERATE_16x16_UT (WelsI16x16LumaPredDcLeft_c, LumaI16x16PredDCLeft, 0, 0)
GENERATE_16x16_UT (WelsI16x16LumaPredDcTop_c, LumaI16x16PredDCTop, 0, 0)
GENERATE_16x16_UT (WelsI16x16LumaPredDc_c, LumaI16x16PredDC, 0, 0)
GENERATE_16x16_UT (WelsI16x16LumaPredH_c, LumaI16x16PredH, 0, 0)
GENERATE_16x16_UT (WelsI16x16LumaPredV_c, LumaI16x16PredV, 0, 0)
#if defined(X86_ASM)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredH_sse2, LumaI4x4PredH, 1, WELS_CPU_SSE2)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredDDR_mmx, WelsI4x4LumaPredDDR_ref, 1, WELS_CPU_MMX)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredHD_mmx, WelsI4x4LumaPredHD_ref, 1, WELS_CPU_MMX)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredHU_mmx, WelsI4x4LumaPredHU_ref, 1, WELS_CPU_MMX)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredVR_mmx, WelsI4x4LumaPredVR_ref, 1, WELS_CPU_MMX)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredDDL_mmx, WelsI4x4LumaPredDDL_ref, 1, WELS_CPU_MMX)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredVL_mmx, WelsI4x4LumaPredVL_ref, 1, WELS_CPU_MMX)
GENERATE_8x8_UT (WelsDecoderIChromaPredDcTop_sse2, WelsIChromaPredDcTop_ref, 1, WELS_CPU_SSE2)
GENERATE_8x8_UT (WelsDecoderIChromaPredDc_sse2, WelsIChromaPredDc_ref, 1, WELS_CPU_SSE2)
GENERATE_8x8_UT (WelsDecoderIChromaPredPlane_sse2, WelsIChromaPredPlane_ref, 1, WELS_CPU_SSE2)
GENERATE_8x8_UT (WelsDecoderIChromaPredH_mmx, LumaI8x8PredH, 1, WELS_CPU_MMX)
GENERATE_8x8_UT (WelsDecoderIChromaPredV_mmx, LumaI8x8PredV, 1, WELS_CPU_MMX)
GENERATE_8x8_UT (WelsDecoderIChromaPredDcLeft_mmx, WelsIChromaPredDcLeft_ref, 1, WELS_CPU_MMX)
GENERATE_8x8_UT (WelsDecoderIChromaPredDcNA_mmx, LumaI8x8PredDCNone, 1, WELS_CPU_MMX)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredPlane_sse2, WelsI16x16LumaPredPlane_ref, 1, WELS_CPU_SSE2)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredH_sse2, LumaI16x16PredH, 1, WELS_CPU_SSE2)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredV_sse2, LumaI16x16PredV, 1, WELS_CPU_SSE2)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredDc_sse2, LumaI16x16PredDC, 1, WELS_CPU_SSE2)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredDcTop_sse2, LumaI16x16PredDCTop, 1, WELS_CPU_SSE2)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredDcNA_sse2, LumaI16x16PredDCNone, 1, WELS_CPU_SSE2)
#endif

#if defined(HAVE_NEON)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredV_neon, LumaI16x16PredV, 1, WELS_CPU_NEON)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredH_neon, LumaI16x16PredH, 1, WELS_CPU_NEON)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredDc_neon, LumaI16x16PredDC, 1, WELS_CPU_NEON)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredPlane_neon, WelsI16x16LumaPredPlane_ref, 1, WELS_CPU_NEON)

GENERATE_4x4_UT (WelsDecoderI4x4LumaPredV_neon, LumaI4x4PredV, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredH_neon, LumaI4x4PredH, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredDDL_neon, WelsI4x4LumaPredDDL_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredDDR_neon, WelsI4x4LumaPredDDR_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredVL_neon, WelsI4x4LumaPredVL_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredVR_neon, WelsI4x4LumaPredVR_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredHU_neon, WelsI4x4LumaPredHU_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredHD_neon, WelsI4x4LumaPredHD_ref, 1, WELS_CPU_NEON)

GENERATE_8x8_UT (WelsDecoderIChromaPredV_neon, LumaI8x8PredV, 1, WELS_CPU_NEON)
GENERATE_8x8_UT (WelsDecoderIChromaPredH_neon, LumaI8x8PredH, 1, WELS_CPU_NEON)
GENERATE_8x8_UT (WelsDecoderIChromaPredDc_neon, WelsIChromaPredDc_ref, 1, WELS_CPU_NEON)
GENERATE_8x8_UT (WelsDecoderIChromaPredPlane_neon, WelsIChromaPredPlane_ref, 1, WELS_CPU_NEON)
#endif

#if defined(HAVE_NEON_AARCH64)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredV_AArch64_neon, LumaI16x16PredV, 1, WELS_CPU_NEON)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredH_AArch64_neon, LumaI16x16PredH, 1, WELS_CPU_NEON)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredDc_AArch64_neon, LumaI16x16PredDC, 1, WELS_CPU_NEON)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredDcTop_AArch64_neon, LumaI16x16PredDCTop, 1, WELS_CPU_NEON)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredDcLeft_AArch64_neon, LumaI16x16PredDCLeft, 1, WELS_CPU_NEON)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredPlane_AArch64_neon, WelsI16x16LumaPredPlane_ref, 1, WELS_CPU_NEON)

GENERATE_4x4_UT (WelsDecoderI4x4LumaPredH_AArch64_neon, LumaI4x4PredH, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredDDL_AArch64_neon, WelsI4x4LumaPredDDL_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredDDLTop_AArch64_neon, WelsI4x4LumaPredDDLTop_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredVL_AArch64_neon, WelsI4x4LumaPredVL_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredVLTop_AArch64_neon, WelsI4x4LumaPredVLTop_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredVR_AArch64_neon, WelsI4x4LumaPredVR_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredHU_AArch64_neon, WelsI4x4LumaPredHU_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredHD_AArch64_neon, WelsI4x4LumaPredHD_ref, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredDc_AArch64_neon, LumaI4x4PredDC, 1, WELS_CPU_NEON)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredDcTop_AArch64_neon, LumaI4x4PredDCTop, 1, WELS_CPU_NEON)

GENERATE_8x8_UT (WelsDecoderIChromaPredV_AArch64_neon, LumaI8x8PredV, 1, WELS_CPU_NEON)
GENERATE_8x8_UT (WelsDecoderIChromaPredH_AArch64_neon, LumaI8x8PredH, 1, WELS_CPU_NEON)
GENERATE_8x8_UT (WelsDecoderIChromaPredDc_AArch64_neon, WelsIChromaPredDc_ref, 1, WELS_CPU_NEON)
GENERATE_8x8_UT (WelsDecoderIChromaPredPlane_AArch64_neon, WelsIChromaPredPlane_ref, 1, WELS_CPU_NEON)
GENERATE_8x8_UT (WelsDecoderIChromaPredDcTop_AArch64_neon, WelsIChromaPredDcTop_ref, 1, WELS_CPU_NEON)
#endif

#if defined(HAVE_MMI)
GENERATE_4x4_UT (WelsDecoderI4x4LumaPredH_mmi, LumaI4x4PredH, 1, WELS_CPU_MMI)
GENERATE_8x8_UT (WelsDecoderIChromaPredDcTop_mmi, WelsIChromaPredDcTop_ref, 1, WELS_CPU_MMI)
GENERATE_8x8_UT (WelsDecoderIChromaPredDc_mmi, WelsIChromaPredDc_ref, 1, WELS_CPU_MMI)
GENERATE_8x8_UT (WelsDecoderIChromaPredPlane_mmi, WelsIChromaPredPlane_ref, 1, WELS_CPU_MMI)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredPlane_mmi, WelsI16x16LumaPredPlane_ref, 1, WELS_CPU_MMI)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredH_mmi, LumaI16x16PredH, 1, WELS_CPU_MMI)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredV_mmi, LumaI16x16PredV, 1, WELS_CPU_MMI)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredDc_mmi, LumaI16x16PredDC, 1, WELS_CPU_MMI)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredDcTop_mmi, LumaI16x16PredDCTop, 1, WELS_CPU_MMI)
GENERATE_16x16_UT (WelsDecoderI16x16LumaPredDcNA_mmi, LumaI16x16PredDCNone, 1, WELS_CPU_MMI)
#endif
