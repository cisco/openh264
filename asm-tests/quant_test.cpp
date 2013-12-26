#include<gtest/gtest.h>
#include<stdint.h>
#include<stdlib.h>
#include "mem_align.h"
#include "encode_mb_aux.h"
#include "decode_mb_aux.h"
using namespace WelsDec;
using namespace WelsSVCEnc;
TEST(Quant, WelsQuant4x4_sse2)
{
	int16_t *pDct_sse2, *pDct_c;
	int16_t *ff, *mf;
	pDct_sse2 = (int16_t*)WelsMalloc(16*sizeof(int16_t), "pDct_sse2");
	pDct_c = (int16_t*)WelsMalloc(16*sizeof(int16_t), "pDct_c");
	ff = (int16_t*)WelsMalloc(16*sizeof(int16_t), "ff");
	mf = (int16_t*)WelsMalloc(16*sizeof(int16_t), "mf");
	const int bits = 14;
	const int range = (1 << bits) - 1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 16; i++)
		pDct_sse2[i] = pDct_c[i] = (rand() & range) - offset;
	for(int i = 0; i < 16; i++) {
		ff[i] = (rand() & range) ;
		mf[i] = (rand() & range) ;
	}
	WelsQuant4x4_sse2(pDct_sse2, ff, mf);
	WelsQuant4x4_c(pDct_c, ff, mf);
	for(int i = 0; i < 16; i++) ASSERT_EQ(pDct_c[i], pDct_sse2[i]);
	WelsFree(pDct_sse2, "pDct_sse2");
	WelsFree(pDct_c, "pDct_c");
	WelsFree(ff, "ff");
	WelsFree(mf, "mf");
}

TEST(Quant, WelsQuant4x4Dc_sse2)
{
	int16_t *pDct_sse2, *pDct_c;
	int16_t *ff, *mf;
	pDct_sse2 = (int16_t*)WelsMalloc(16*sizeof(int16_t), "pDct_sse2");
	pDct_c = (int16_t*)WelsMalloc(16*sizeof(int16_t), "pDct_c");
	ff = (int16_t*)WelsMalloc(16*sizeof(int16_t), "ff");
	mf = (int16_t*)WelsMalloc(16*sizeof(int16_t), "mf");
	const int bits = 14;
	const int range = (1 << bits) - 1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 16; i++)
		pDct_sse2[i] = pDct_c[i] = (rand() & range) - offset;
	for(int i = 0; i < 16; i++) {
		ff[i] = (rand() & range) ;
		mf[i] = (rand() & range) ;
	}
	WelsQuant4x4Dc_sse2(pDct_sse2, *ff, *mf);
	WelsQuant4x4Dc_c(pDct_c, *ff, *mf);
	for(int i = 0; i < 16; i++) ASSERT_EQ(pDct_c[i], pDct_sse2[i]);
	WelsFree(pDct_sse2, "pDct_sse2");
	WelsFree(pDct_c, "pDct_c");
	WelsFree(ff, "ff");
	WelsFree(mf, "mf");
}
TEST(Quant, WelsQuantFour4x4_sse2)
{
	int16_t *pDct_sse2, *pDct_c;
	int16_t *ff, *mf;
	pDct_sse2 = (int16_t*)WelsMalloc(64*sizeof(int16_t), "pDct_sse2");
	pDct_c = (int16_t*)WelsMalloc(64*sizeof(int16_t), "pDct_c");
	ff = (int16_t*)WelsMalloc(16*sizeof(int16_t), "ff");
	mf = (int16_t*)WelsMalloc(16*sizeof(int16_t), "mf");
	const int bits = 14;
	const int range = (1 << bits) - 1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 64; i++)
		pDct_sse2[i] = pDct_c[i] = (rand() & range) - offset;
	for(int i = 0; i < 16; i++) {
		ff[i] = (rand() & range) ;
		mf[i] = (rand() & range) ;
	}
	WelsQuantFour4x4_sse2(pDct_sse2, ff, mf);
	WelsQuantFour4x4_c(pDct_c, ff, mf);
	for(int i = 0; i < 64; i++) ASSERT_EQ(pDct_c[i], pDct_sse2[i]);
	WelsFree(pDct_sse2, "pDct_sse2");
	WelsFree(pDct_c, "pDct_c");
	WelsFree(ff, "ff");
	WelsFree(mf, "mf");
}
TEST(Quant, WelsQuantFour4x4Max_sse2)
{
	int16_t *pDct_sse2, *pDct_c;
	int16_t *ff, *mf;
	int16_t *max_sse2, *max_c;
	pDct_sse2 = (int16_t*)WelsMalloc(64*sizeof(int16_t), "pDct_sse2");
	pDct_c = (int16_t*)WelsMalloc(64*sizeof(int16_t), "pDct_c");
	ff = (int16_t*)WelsMalloc(16*sizeof(int16_t), "ff");
	mf = (int16_t*)WelsMalloc(16*sizeof(int16_t), "mf");
	max_sse2 = (int16_t*)WelsMalloc(16*sizeof(int16_t), "max_sse2");
	max_c = (int16_t*)WelsMalloc(16*sizeof(int16_t), "max_c");
	const int bits = 14;
	const int range = (1 << bits) - 1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 64; i++)
		pDct_sse2[i] = pDct_c[i] = (rand() & range) - offset;
	for(int i = 0; i < 16; i++) {
		ff[i] = (rand() & range) ;
		mf[i] = (rand() & range) ;
	}
	WelsQuantFour4x4Max_sse2(pDct_sse2, ff, mf, max_sse2);
	WelsQuantFour4x4Max_c(pDct_c, ff, mf, max_c);
	for(int i = 0; i < 64; i++) ASSERT_EQ(pDct_c[i], pDct_sse2[i]);
	for(int i = 0; i < 4; i++) ASSERT_EQ(max_c[i] , max_sse2[i]);
	WelsFree(pDct_sse2, "pDct_sse2");
	WelsFree(pDct_c, "pDct_c");
	WelsFree(ff, "ff");
	WelsFree(mf, "mf");
	WelsFree(max_c, "max_c");
	WelsFree(max_sse2, "max_sse2");
}
TEST(Quant, WelsHadamardQuant2x2_mmx)
{
	int16_t pRs_c[64], pDct_c[4], pBlock_c[4];
	int16_t pRs_mmx[64], pDct_mmx[4], pBlock_mmx[4];
	int16_t ff, mf;
	const int bits = 11;
	const int range = (1 << bits) - 1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 64; i++)
		pRs_c[i] = pRs_mmx[i] = (rand() & range) - offset;
	ff = rand() & range;
	mf = rand() & range;
	int32_t c_rv = WelsHadamardQuant2x2_c(pRs_c, ff, mf, pDct_c, pBlock_c);
	int32_t mmx_rv = WelsHadamardQuant2x2_mmx(pRs_mmx, ff, mf, pDct_mmx, pBlock_mmx);
	ASSERT_EQ(c_rv, mmx_rv);
	for(int i = 0; i < 64; i++)
		ASSERT_EQ(pRs_c[i], pRs_mmx[i]);
	for(int i = 0; i < 4; i++) {
		ASSERT_EQ(pDct_c[i], pDct_mmx[i]);
		ASSERT_EQ(pBlock_c[i], pBlock_mmx[i]);
	}
}
TEST(Quant, WelsHadamardQuant2x2Skip_mmx)
{
	int16_t *pDct_sse2, *pDct_c;
	int16_t *ff, *mf;
	pDct_sse2 = (int16_t*)WelsMalloc(64*sizeof(int16_t), "pDct_sse2");
	pDct_c = (int16_t*)WelsMalloc(64*sizeof(int16_t), "pDct_c");
	ff = (int16_t*)WelsMalloc(16*sizeof(int16_t), "ff");
	mf = (int16_t*)WelsMalloc(16*sizeof(int16_t), "mf");
	const int bits = 14;
	const int range = (1 << bits) - 1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 64; i++)
		pDct_sse2[i] = pDct_c[i] = (rand() & range) - offset;
	for(int i = 0; i < 16; i++) {
		ff[i] = (rand() & range) ;
		mf[i] = (rand() & range) ;
	}
	int32_t mmx_rv = WelsHadamardQuant2x2Skip_mmx(pDct_sse2, *ff, *mf);
	int32_t c_rv = WelsHadamardQuant2x2Skip_c(pDct_c, *ff, *mf);
	ASSERT_EQ(mmx_rv != 0, c_rv != 0);
	WelsFree(pDct_sse2, "pDct_sse2");
	WelsFree(pDct_c, "pDct_c");
	WelsFree(ff, "ff");
	WelsFree(mf, "mf");
}


TEST(Quant, WelsDequantIHadamard4x4_sse2) {
	int16_t * pRes_sse2, *pRes_c;
	int16_t mf;
	pRes_sse2 = (int16_t*)WelsMalloc(16*sizeof(int16_t), "pRes_sse2");
	pRes_c = (int16_t*)WelsMalloc(16*sizeof(int16_t), "pRes_c");
	const int bits = 12;
	const int range = (1 << bits) -1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 16; i++)
		pRes_sse2[i] = pRes_c[i] = (rand() & range) - offset;
	mf = rand() & range;
	WelsDequantIHadamard4x4_c(pRes_c, mf);
	WelsDequantIHadamard4x4_sse2(pRes_sse2, mf);
	for(int i = 0; i < 16; i++) ASSERT_EQ(pRes_c[i], pRes_sse2[i]);
	WelsFree(pRes_sse2, "pRes_sse2");
	WelsFree(pRes_c, "pRes_c");
}

TEST(Quant, WelsDequant4x4_sse2) {
	int16_t * pRes_sse2, *pRes_c;
	uint16_t *pMF;
	int16_t mf;
	pRes_sse2 = (int16_t*)WelsMalloc(16*sizeof(int16_t), "pRes_sse2");
	pRes_c = (int16_t*)WelsMalloc(16*sizeof(int16_t), "pRes_c");
	pMF = (uint16_t*)WelsMalloc(16*sizeof(int16_t), "pMF");
	const int bits = 12;
	const int range = (1 << bits) -1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 16; i++)
		pRes_sse2[i] = pRes_c[i] = (rand() & range) - offset;
	mf = rand() & range;
	for (int i = 0; i < 16; i++)
		pMF[i] = rand() & range;
	WelsDequant4x4_c(pRes_c, pMF);
	WelsDequant4x4_sse2(pRes_sse2, pMF);
	for(int i = 0; i < 16; i++) ASSERT_EQ(pRes_c[i], pRes_sse2[i]);
	WelsFree(pRes_sse2, "pRes_sse2");
	WelsFree(pRes_c, "pRes_c");
	WelsFree(pMF, "pMF");
}
TEST(Quant, WelsDequantFour4x4_sse2) {
	int16_t * pRes_sse2, *pRes_c;
	uint16_t *pMF;
	int16_t mf;
	pRes_sse2 = (int16_t*)WelsMalloc(64*sizeof(int16_t), "pRes_sse2");
	pRes_c = (int16_t*)WelsMalloc(64*sizeof(int16_t), "pRes_c");
	pMF = (uint16_t*)WelsMalloc(16*sizeof(int16_t), "pMF");
	const int bits = 12;
	const int range = (1 << bits) -1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 64; i++)
		pRes_sse2[i] = pRes_c[i] = (rand() & range) - offset;
	mf = rand() & range;
	for (int i = 0; i < 16; i++)
		pMF[i] = rand() & range;
	WelsDequantFour4x4_c(pRes_c, pMF);
	WelsDequantFour4x4_sse2(pRes_sse2, pMF);
	for(int i = 0; i < 64; i++) ASSERT_EQ(pRes_c[i], pRes_sse2[i]);
	WelsFree(pRes_sse2, "pRes_sse2");
	WelsFree(pRes_c, "pRes_c");
	WelsFree(pMF, "pMF");
}

