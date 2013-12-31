#include <gtest/gtest.h>

#include "encode_mb_aux.h"
#include "mem_align.h"
using namespace WelsSVCEnc;
using WelsDec::WelsMalloc;
using WelsDec::WelsFree;


TEST(score_test, WelsScan4x4DcAc_sse2) {
	int16_t *pDct = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pDct");
	int16_t *pLevel = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevel");
	int16_t *pLevelRef = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevelRef");
	srand(time(NULL));
	const int bits = 12;
	const int range = (1 << bits) -1;
	const int offset = 1 << (bits - 1);
	for(int i = 0; i < 16; i++) {
		pDct[i] = (rand() & range) - offset;
	}
	WelsScan4x4DcAc_c(pLevelRef, pDct);
	WelsScan4x4DcAc_sse2(pLevel, pDct);
	for(int i = 0; i < 16; i ++) ASSERT_EQ(pLevel[i], pLevelRef[i]);
	WelsFree(pDct, "pDct");
	WelsFree(pLevel, "pLevel");
	WelsFree(pLevelRef, "pLevelRef");
}
TEST(score_test, WelsScan4x4DcAc_ssse3) {
	int16_t *pDct = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pDct");
	int16_t *pLevel = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevel");
	int16_t *pLevelRef = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevelRef");
	srand(time(NULL));
	const int bits = 12;
	const int range = (1 << bits) -1;
	const int offset = 1 << (bits - 1);
	for(int i = 0; i < 16; i++) {
		pDct[i] = (rand() & range) - offset;
	}
	WelsScan4x4DcAc_c(pLevelRef, pDct);
	WelsScan4x4DcAc_ssse3(pLevel, pDct);
	for(int i = 0; i < 16; i ++) ASSERT_EQ(pLevel[i], pLevelRef[i]);
	WelsFree(pDct, "pDct");
	WelsFree(pLevel, "pLevel");
	WelsFree(pLevelRef, "pLevelRef");
}
TEST(score_test, WelsScan4x4Ac_sse2) {
	int16_t *pDct = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pDct");
	int16_t *pLevel = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevel");
	int16_t *pLevelRef = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevelRef");
	srand(time(NULL));
	const int bits = 12;
	const int range = (1 << bits) -1;
	const int offset = 1 << (bits - 1);
	for(int i = 0; i < 16; i++) {
		pDct[i] = (rand() & range) - offset;
	}
	WelsScan4x4Ac_c(pLevelRef, pDct);
	WelsScan4x4Ac_sse2(pLevel, pDct);
	for(int i = 0; i < 16; i ++) ASSERT_EQ(pLevel[i], pLevelRef[i]);
	WelsFree(pDct, "pDct");
	WelsFree(pLevel, "pLevel");
	WelsFree(pLevelRef, "pLevelRef");
}
TEST(score_test, WelsGetNoneZeroCount_sse2) {
	int16_t *pDct = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pDct");
	int16_t *pLevel = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevel");
	int16_t *pLevelRef = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevelRef");
	srand(time(NULL));
	const int bits = 12;
	const int range = (1 << bits) -1;
	const int offset = 1 << (bits - 1);
	for(int i = 0; i < 16; i++) {
		pDct[i] = (rand() & range) - offset;
	}
	int rv = WelsGetNoneZeroCount_sse2(pDct);
	int rv_ref = WelsGetNoneZeroCount_c(pDct);
	ASSERT_EQ(rv, rv_ref);
	WelsFree(pDct, "pDct");
	WelsFree(pLevel, "pLevel");
	WelsFree(pLevelRef, "pLevelRef");
}
TEST(score_test, WelsCalculateSingleCtr4x4_sse2) {
	int16_t *pDct = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pDct");
	int16_t *pLevel = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevel");
	int16_t *pLevelRef = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLevelRef");
	srand(time(NULL));
	const int bits = 12;
	const int range = (1 << bits) -1;
	const int offset = 1 << (bits - 1);
	for(int i = 0; i < 16; i++) {
		pDct[i] = (rand() & range) - offset;
	}
	int rv = WelsCalculateSingleCtr4x4_sse2(pDct);
	int rv_ref = WelsCalculateSingleCtr4x4_c(pDct);
	ASSERT_EQ(rv, rv_ref);
	WelsFree(pDct, "pDct");
	WelsFree(pLevel, "pLevel");
	WelsFree(pLevelRef, "pLevelRef");
}
