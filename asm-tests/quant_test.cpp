#include<gtest/gtest.h>
#include<stdint.h>
#include<stdlib.h>
#include "mem_align.h"
#include "encode_mb_aux.h"
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

