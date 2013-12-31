#include <gtest/gtest.h>
#include <stdint.h>
#include "mem_align.h"
using WelsDec::WelsMalloc;
using WelsDec::WelsFree;

extern "C" int32_t CavlcParamCal_sse2(int16_t*coffLevel, uint8_t* run, int16_t *Level, int32_t* total_coeffs , int32_t endIdx);
int32_t CavlcParamCal_c (int16_t* pCoffLevel, uint8_t* pRun, int16_t* pLevel, int32_t* pTotalCoeff ,
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

TEST(coeff_test, CavlcParamCal_sse2) {
    int16_t *pCoffLevel = (int16_t*)WelsMalloc(16*sizeof(int16_t), "pCoffLevel");
    uint8_t *pRun = (uint8_t*) WelsMalloc(16*sizeof(uint8_t), "pRun");
    int16_t *pLevel = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevel");
    uint8_t *pRunRef = (uint8_t*) WelsMalloc(16*sizeof(uint8_t), "pRunRef");
    int16_t *pLevelRef = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevelRef");
    int32_t iTotalCoeff, iTotalCoeffRef;
    int iLastIndex;
    int nzc;
    srand(time(NULL));
    nzc = rand() % 17 ; // nzc is in range [0, 16], inclusive
    if (nzc == 16)
       	iLastIndex = nzc;
    else
	iLastIndex = nzc + (rand() % (16 - nzc));
    memset(pCoffLevel, 0, 16*sizeof(int16_t));
    memset(pLevel, 0, 16*sizeof(int16_t));
    memset(pLevelRef, 0, 16*sizeof(int16_t));
    memset(pRun, 0, 16*sizeof(uint8_t));
    memset(pRunRef, 0, 16*sizeof(uint8_t));
    const int bits = 13;
    const int range = (1 << bits) - 1;
    const int offset = 1 << (bits - 1);
    for(int i = 0; i < nzc; i++) {
	pCoffLevel[i] = (rand() & range) - offset;
    }
    int rvRef = CavlcParamCal_c(pCoffLevel, pRunRef, pLevelRef, &iTotalCoeffRef, iLastIndex);
    int rv = CavlcParamCal_sse2(pCoffLevel, pRun, pLevel, &iTotalCoeff, iLastIndex);
    ASSERT_EQ(rv, rvRef);
    ASSERT_EQ(iTotalCoeff, iTotalCoeffRef);
    for(int i = 0; i < 16; i++) {
	ASSERT_EQ(pRun[i], pRunRef[i]);
	ASSERT_EQ(pLevel[i], pLevelRef[i]);
    }
    WelsFree(pCoffLevel, "pCoffLevel");
    WelsFree(pRun, "pRun");
    WelsFree(pRunRef, "pRunRef");
    WelsFree(pLevel, "pLevel");
    WelsFree(pLevelRef, "pLevelRef");
}
