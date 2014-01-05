#include <gtest/gtest.h>
#include <stdint.h>
#include <emmintrin.h>
#include "macros.h"
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
__align16(int8_t, sse2_b_1[16]) = { 
	-1, -1, -1, -1, -1, -1, -1, 0, 
	-1, -1, -1, -1, -1, -1, -1, -1 
};
__align16(uint8_t, byte_1pos_table[256*8]) = {
	 0,0,0,0,0,0,0,0, 
	 0,0,0,0,0,0,0,1, 
	 1,0,0,0,0,0,0,1, 
	 1,0,0,0,0,0,0,2, 
	 2,0,0,0,0,0,0,1, 
	 2,0,0,0,0,0,0,2, 
	 2,1,0,0,0,0,0,2, 
	 2,1,0,0,0,0,0,3, 
	 3,0,0,0,0,0,0,1, 
	 3,0,0,0,0,0,0,2, 
	 3,1,0,0,0,0,0,2, 
	 3,1,0,0,0,0,0,3, 
	 3,2,0,0,0,0,0,2, 
	 3,2,0,0,0,0,0,3, 
	 3,2,1,0,0,0,0,3, 
	 3,2,1,0,0,0,0,4, 
	 4,0,0,0,0,0,0,1, 
	 4,0,0,0,0,0,0,2, 
	 4,1,0,0,0,0,0,2, 
	 4,1,0,0,0,0,0,3, 
	 4,2,0,0,0,0,0,2, 
	 4,2,0,0,0,0,0,3, 
	 4,2,1,0,0,0,0,3, 
	 4,2,1,0,0,0,0,4, 
	 4,3,0,0,0,0,0,2, 
	 4,3,0,0,0,0,0,3, 
	 4,3,1,0,0,0,0,3, 
	 4,3,1,0,0,0,0,4, 
	 4,3,2,0,0,0,0,3, 
	 4,3,2,0,0,0,0,4, 
	 4,3,2,1,0,0,0,4, 
	 4,3,2,1,0,0,0,5, 
	 5,0,0,0,0,0,0,1, 
	 5,0,0,0,0,0,0,2, 
	 5,1,0,0,0,0,0,2, 
	 5,1,0,0,0,0,0,3, 
	 5,2,0,0,0,0,0,2, 
	 5,2,0,0,0,0,0,3, 
	 5,2,1,0,0,0,0,3, 
	 5,2,1,0,0,0,0,4, 
	 5,3,0,0,0,0,0,2, 
	 5,3,0,0,0,0,0,3, 
	 5,3,1,0,0,0,0,3, 
	 5,3,1,0,0,0,0,4, 
	 5,3,2,0,0,0,0,3, 
	 5,3,2,0,0,0,0,4, 
	 5,3,2,1,0,0,0,4, 
	 5,3,2,1,0,0,0,5, 
	 5,4,0,0,0,0,0,2, 
	 5,4,0,0,0,0,0,3, 
	 5,4,1,0,0,0,0,3, 
	 5,4,1,0,0,0,0,4, 
	 5,4,2,0,0,0,0,3, 
	 5,4,2,0,0,0,0,4, 
	 5,4,2,1,0,0,0,4, 
	 5,4,2,1,0,0,0,5, 
	 5,4,3,0,0,0,0,3, 
	 5,4,3,0,0,0,0,4, 
	 5,4,3,1,0,0,0,4, 
	 5,4,3,1,0,0,0,5, 
	 5,4,3,2,0,0,0,4, 
	 5,4,3,2,0,0,0,5, 
	 5,4,3,2,1,0,0,5, 
	 5,4,3,2,1,0,0,6, 
	 6,0,0,0,0,0,0,1, 
	 6,0,0,0,0,0,0,2, 
	 6,1,0,0,0,0,0,2, 
	 6,1,0,0,0,0,0,3, 
	 6,2,0,0,0,0,0,2, 
	 6,2,0,0,0,0,0,3, 
	 6,2,1,0,0,0,0,3, 
	 6,2,1,0,0,0,0,4, 
	 6,3,0,0,0,0,0,2, 
	 6,3,0,0,0,0,0,3, 
	 6,3,1,0,0,0,0,3, 
	 6,3,1,0,0,0,0,4, 
	 6,3,2,0,0,0,0,3, 
	 6,3,2,0,0,0,0,4, 
	 6,3,2,1,0,0,0,4, 
	 6,3,2,1,0,0,0,5, 
	 6,4,0,0,0,0,0,2, 
	 6,4,0,0,0,0,0,3, 
	 6,4,1,0,0,0,0,3, 
	 6,4,1,0,0,0,0,4, 
	 6,4,2,0,0,0,0,3, 
	 6,4,2,0,0,0,0,4, 
	 6,4,2,1,0,0,0,4, 
	 6,4,2,1,0,0,0,5, 
	 6,4,3,0,0,0,0,3, 
	 6,4,3,0,0,0,0,4, 
	 6,4,3,1,0,0,0,4, 
	 6,4,3,1,0,0,0,5, 
	 6,4,3,2,0,0,0,4, 
	 6,4,3,2,0,0,0,5, 
	 6,4,3,2,1,0,0,5, 
	 6,4,3,2,1,0,0,6, 
	 6,5,0,0,0,0,0,2, 
	 6,5,0,0,0,0,0,3, 
	 6,5,1,0,0,0,0,3, 
	 6,5,1,0,0,0,0,4, 
	 6,5,2,0,0,0,0,3, 
	 6,5,2,0,0,0,0,4, 
	 6,5,2,1,0,0,0,4, 
	 6,5,2,1,0,0,0,5, 
	 6,5,3,0,0,0,0,3, 
	 6,5,3,0,0,0,0,4, 
	 6,5,3,1,0,0,0,4, 
	 6,5,3,1,0,0,0,5, 
	 6,5,3,2,0,0,0,4, 
	 6,5,3,2,0,0,0,5, 
	 6,5,3,2,1,0,0,5, 
	 6,5,3,2,1,0,0,6, 
	 6,5,4,0,0,0,0,3, 
	 6,5,4,0,0,0,0,4, 
	 6,5,4,1,0,0,0,4, 
	 6,5,4,1,0,0,0,5, 
	 6,5,4,2,0,0,0,4, 
	 6,5,4,2,0,0,0,5, 
	 6,5,4,2,1,0,0,5, 
	 6,5,4,2,1,0,0,6, 
	 6,5,4,3,0,0,0,4, 
	 6,5,4,3,0,0,0,5, 
	 6,5,4,3,1,0,0,5, 
	 6,5,4,3,1,0,0,6, 
	 6,5,4,3,2,0,0,5, 
	 6,5,4,3,2,0,0,6, 
	 6,5,4,3,2,1,0,6, 
	 6,5,4,3,2,1,0,7, 
	 7,0,0,0,0,0,0,1, 
	 7,0,0,0,0,0,0,2, 
	 7,1,0,0,0,0,0,2, 
	 7,1,0,0,0,0,0,3, 
	 7,2,0,0,0,0,0,2, 
	 7,2,0,0,0,0,0,3, 
	 7,2,1,0,0,0,0,3, 
	 7,2,1,0,0,0,0,4, 
	 7,3,0,0,0,0,0,2, 
	 7,3,0,0,0,0,0,3, 
	 7,3,1,0,0,0,0,3, 
	 7,3,1,0,0,0,0,4, 
	 7,3,2,0,0,0,0,3, 
	 7,3,2,0,0,0,0,4, 
	 7,3,2,1,0,0,0,4, 
	 7,3,2,1,0,0,0,5, 
	 7,4,0,0,0,0,0,2, 
	 7,4,0,0,0,0,0,3, 
	 7,4,1,0,0,0,0,3, 
	 7,4,1,0,0,0,0,4, 
	 7,4,2,0,0,0,0,3, 
	 7,4,2,0,0,0,0,4, 
	 7,4,2,1,0,0,0,4, 
	 7,4,2,1,0,0,0,5, 
	 7,4,3,0,0,0,0,3, 
	 7,4,3,0,0,0,0,4, 
	 7,4,3,1,0,0,0,4, 
	 7,4,3,1,0,0,0,5, 
	 7,4,3,2,0,0,0,4, 
	 7,4,3,2,0,0,0,5, 
	 7,4,3,2,1,0,0,5, 
	 7,4,3,2,1,0,0,6, 
	 7,5,0,0,0,0,0,2, 
	 7,5,0,0,0,0,0,3, 
	 7,5,1,0,0,0,0,3, 
	 7,5,1,0,0,0,0,4, 
	 7,5,2,0,0,0,0,3, 
	 7,5,2,0,0,0,0,4, 
	 7,5,2,1,0,0,0,4, 
	 7,5,2,1,0,0,0,5, 
	 7,5,3,0,0,0,0,3, 
	 7,5,3,0,0,0,0,4, 
	 7,5,3,1,0,0,0,4, 
	 7,5,3,1,0,0,0,5, 
	 7,5,3,2,0,0,0,4, 
	 7,5,3,2,0,0,0,5, 
	 7,5,3,2,1,0,0,5, 
	 7,5,3,2,1,0,0,6, 
	 7,5,4,0,0,0,0,3, 
	 7,5,4,0,0,0,0,4, 
	 7,5,4,1,0,0,0,4, 
	 7,5,4,1,0,0,0,5, 
	 7,5,4,2,0,0,0,4, 
	 7,5,4,2,0,0,0,5, 
	 7,5,4,2,1,0,0,5, 
	 7,5,4,2,1,0,0,6, 
	 7,5,4,3,0,0,0,4, 
	 7,5,4,3,0,0,0,5, 
	 7,5,4,3,1,0,0,5, 
	 7,5,4,3,1,0,0,6, 
	 7,5,4,3,2,0,0,5, 
	 7,5,4,3,2,0,0,6, 
	 7,5,4,3,2,1,0,6, 
	 7,5,4,3,2,1,0,7, 
	 7,6,0,0,0,0,0,2, 
	 7,6,0,0,0,0,0,3, 
	 7,6,1,0,0,0,0,3, 
	 7,6,1,0,0,0,0,4, 
	 7,6,2,0,0,0,0,3, 
	 7,6,2,0,0,0,0,4, 
	 7,6,2,1,0,0,0,4, 
	 7,6,2,1,0,0,0,5, 
	 7,6,3,0,0,0,0,3, 
	 7,6,3,0,0,0,0,4, 
	 7,6,3,1,0,0,0,4, 
	 7,6,3,1,0,0,0,5, 
	 7,6,3,2,0,0,0,4, 
	 7,6,3,2,0,0,0,5, 
	 7,6,3,2,1,0,0,5, 
	 7,6,3,2,1,0,0,6, 
	 7,6,4,0,0,0,0,3, 
	 7,6,4,0,0,0,0,4, 
	 7,6,4,1,0,0,0,4, 
	 7,6,4,1,0,0,0,5, 
	 7,6,4,2,0,0,0,4, 
	 7,6,4,2,0,0,0,5, 
	 7,6,4,2,1,0,0,5, 
	 7,6,4,2,1,0,0,6, 
	 7,6,4,3,0,0,0,4, 
	 7,6,4,3,0,0,0,5, 
	 7,6,4,3,1,0,0,5, 
	 7,6,4,3,1,0,0,6, 
	 7,6,4,3,2,0,0,5, 
	 7,6,4,3,2,0,0,6, 
	 7,6,4,3,2,1,0,6, 
	 7,6,4,3,2,1,0,7, 
	 7,6,5,0,0,0,0,3, 
	 7,6,5,0,0,0,0,4, 
	 7,6,5,1,0,0,0,4, 
	 7,6,5,1,0,0,0,5, 
	 7,6,5,2,0,0,0,4, 
	 7,6,5,2,0,0,0,5, 
	 7,6,5,2,1,0,0,5, 
	 7,6,5,2,1,0,0,6, 
	 7,6,5,3,0,0,0,4, 
	 7,6,5,3,0,0,0,5, 
	 7,6,5,3,1,0,0,5, 
	 7,6,5,3,1,0,0,6, 
	 7,6,5,3,2,0,0,5, 
	 7,6,5,3,2,0,0,6, 
	 7,6,5,3,2,1,0,6, 
	 7,6,5,3,2,1,0,7, 
	 7,6,5,4,0,0,0,4, 
	 7,6,5,4,0,0,0,5, 
	 7,6,5,4,1,0,0,5, 
	 7,6,5,4,1,0,0,6, 
	 7,6,5,4,2,0,0,5, 
	 7,6,5,4,2,0,0,6, 
	 7,6,5,4,2,1,0,6, 
	 7,6,5,4,2,1,0,7, 
	 7,6,5,4,3,0,0,5, 
	 7,6,5,4,3,0,0,6, 
	 7,6,5,4,3,1,0,6, 
	 7,6,5,4,3,1,0,7, 
	 7,6,5,4,3,2,0,6, 
	 7,6,5,4,3,2,0,7, 
	 7,6,5,4,3,2,1,7, 
	 7,6,5,4,3,2,1,8 
};
__align16(int8_t, sse2_b8[8]) = {
	8,8,8,8,8,8,8,8
};
int32_t CavlcParamCal_sse2_intrincs(int16_t *coffLevel, uint8_t* run, int16_t *Level, int32_t *total_coeffs, int32_t endIdx) {
	__m128i r0, r1, r2, r3, r4, r5, r6, r7;
	int eax, ebx, ecx, edx, esi;
	if(endIdx == 3) {
		r1 = _mm_setzero_si128();
		r0 = _mm_loadl_epi64((__m128i const*)coffLevel);
	}
	else {
		r0 = _mm_load_si128((__m128i const*)coffLevel);
		r1 = _mm_load_si128((__m128i const*)(coffLevel + 8));
	}
	//r2 = r0;
	r0 = _mm_packs_epi16(r0, r1);
	r4 = r0;
	r3 = _mm_setzero_si128();
	r0 = _mm_cmpgt_epi8(r0, r3);
	r3 = _mm_cmpgt_epi8(r3, r4);
	r0 = _mm_or_si128(r0, r3);
	edx = _mm_movemask_epi8(r0);
	if(edx == 0) {
		*total_coeffs = 0;
		return 0;
	}
	r6 = _mm_load_si128((__m128i const*)sse2_b_1);
	r7 = _mm_set1_epi8(-1);
	ebx = edx >> 8;
	ebx = ebx << 3; // ebx = ebx * 8;
	r0 = _mm_loadl_epi64((__m128i const*)&byte_1pos_table[ebx]);
	ecx = _mm_extract_epi16(r0, 3);
	ecx >>= 8;
	r0 = _mm_and_si128(r0, r6);
	int tmp = ecx;
	if (tmp > 0) {
		do {
		esi = byte_1pos_table[ebx];
		esi += 8;
		esi = coffLevel[esi];
		*Level++ = esi;
		ebx ++;
		tmp --;
		} while(tmp > 0);
		if(ecx == 8) {
			Level[-1] = coffLevel[8];
		}
	}
	edx &= 0xff;
	ebx = edx << 3;
	r1 = _mm_loadl_epi64((__m128i const*)&byte_1pos_table[ebx]);
	esi = _mm_extract_epi16(r1, 3);
	esi >>= 8;
	r1 = _mm_and_si128(r1, r6);
	tmp = esi;
	if(tmp > 0) {
		do {
			edx = byte_1pos_table[ebx];
			edx = coffLevel[edx];
			*Level++ = edx;
			ebx ++;
			tmp --;
		}while(tmp > 0);
		if(esi == 8) {
			Level[-1] = coffLevel[0];
		}
	}
	*total_coeffs = esi + ecx;
	r5 = _mm_loadl_epi64((__m128i const*)sse2_b8);
	r0 = _mm_add_epi8(r0, r5);
	r2 = _mm_setzero_si128();
	r3 = _mm_setzero_si128();
	eax = 8;
	eax = eax - ecx;
	r2 = _mm_insert_epi16(r2, ecx << 3, 0);
	r3 = _mm_insert_epi16(r3, eax << 3, 0);
	r0 = _mm_sll_epi64(r0, r3);
	r0 = _mm_srl_epi64(r0, r3);
	r4 = r1;
	r1 = _mm_sll_epi64(r1, r2);
	r4 = _mm_srl_epi64(r4, r3);
	r1 = _mm_unpacklo_epi64(r1, r4);
	r0 = _mm_or_si128(r0, r1);
	eax = _mm_extract_epi16(r0, 0) & 0xff;
	eax ++;
	eax -= (esi + ecx);
	r1 = r0;
	r1 = _mm_add_epi8(r1, r7);
	r0 = _mm_srli_si128(r0, 1);
	r1 = _mm_sub_epi8(r1, r0);
	_mm_store_si128((__m128i *)run, r1);
	return eax;
}
TEST(coeff_test, CavlcParamCal_sse2) {
    int16_t *pCoffLevel = (int16_t*)WelsMalloc(16*sizeof(int16_t), "pCoffLevel");
    uint8_t *pRun = (uint8_t*) WelsMalloc(16*sizeof(uint8_t), "pRun");
    int16_t *pLevel = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevel");
    uint8_t *pRunRef = (uint8_t*) WelsMalloc(16*sizeof(uint8_t), "pRunRef");
    int16_t *pLevelRef = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevelRef");
    int32_t iTotalCoeff = 0, iTotalCoeffRef = 0;
    int iLastIndex;
    int nzc;
    srand(time(NULL));
    nzc = rand() % 16 ; // nzc is in range [0, 16], inclusive
    if (nzc == 16)
       	iLastIndex = nzc;
    else
	iLastIndex = nzc + (rand() % (16 - nzc));
    memset(pCoffLevel, 0, 16*sizeof(int16_t));
    memset(pLevel, 0, 16*sizeof(int16_t));
    memset(pLevelRef, 0, 16*sizeof(int16_t));
    memset(pRun, 0, 16*sizeof(uint8_t));
    memset(pRunRef, 0, 16*sizeof(uint8_t));
    const int bits = 2;
    const int range = (1 << bits) - 1;
    const int offset = 1 << (bits - 1);
    for(int i = 0; i < nzc; i++) {
	pCoffLevel[i] = (rand() & range) - offset;
    }
    int rvRef = CavlcParamCal_c(pCoffLevel, pRunRef, pLevelRef, &iTotalCoeffRef, iLastIndex);
    int rv = CavlcParamCal_sse2_intrincs(pCoffLevel, pRun, pLevel, &iTotalCoeff, iLastIndex);
    ASSERT_EQ(iTotalCoeff, iTotalCoeffRef);
    for(int i = 0; i < iTotalCoeff; i++) {
	ASSERT_EQ(pLevel[i], pLevelRef[i]);
    }
    for(int i = 0; i < iTotalCoeff-1; i++) {
	ASSERT_EQ(pRun[i], pRunRef[i]);
    }
    ASSERT_EQ(rv, rvRef);
    WelsFree(pCoffLevel, "pCoffLevel");
    WelsFree(pRun, "pRun");
    WelsFree(pRunRef, "pRunRef");
    WelsFree(pLevel, "pLevel");
    WelsFree(pLevelRef, "pLevelRef");
}
