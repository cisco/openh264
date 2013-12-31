#include<gtest/gtest.h>
#include<stdlib.h>
#include "mem_align.h"
#include "encode_mb_aux.h"
#include "decode_mb_aux.h"
using namespace WelsSVCEnc;
using WelsDec::WelsMalloc;
using WelsDec::WelsFree;

TEST(Dct_encoder, WelsDctT4_mmx) {
	int iStride = 64;
	uint8_t *pix1 = (uint8_t*)WelsMalloc(4*iStride*sizeof(uint8_t), "pix1");
	uint8_t *pix2 = (uint8_t*)WelsMalloc(4*iStride*sizeof(uint8_t), "pix2");
	int16_t pDct[16], pDct_ref[16];
	srand(time(NULL));
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			pix1[i*iStride+j] = rand() & 255;
			pix2[i*iStride+j] = rand() & 255;
		}
	}
	WelsDctT4_c(pDct_ref, pix1, iStride, pix2, iStride);
	WelsDctT4_mmx(pDct, pix1, iStride, pix2, iStride);
	for(int i = 0; i < 16; i++) {
			ASSERT_EQ(pDct[i], pDct_ref[i]);
	}
	WelsFree(pix1, "pix1");
	WelsFree(pix2, "pix2");
}

TEST(Dct_encoder, WelsDctFourT4_sse2) {
	int iStride = 64;
	uint8_t *pix1 = (uint8_t*)WelsMalloc(8*iStride*sizeof(uint8_t), "pix1");
	uint8_t *pix2 = (uint8_t*)WelsMalloc(8*iStride*sizeof(uint8_t), "pix2");
	//int16_t pDct[16], pDct_ref[16];
	int16_t *pDct = (int16_t*)WelsMalloc(64*sizeof(int16_t), "pDct");
	int16_t *pDct_ref = (int16_t*)WelsMalloc(64*sizeof(int16_t), "pDct_ref");
	srand(time(NULL));
	for(int i = 0; i < 8; i++) {
		for(int j = 0; j < 8; j++) {
			pix1[i*iStride+j] = rand() & 255;
			pix2[i*iStride+j] = rand() & 255;
		}
	}
	WelsDctFourT4_c(pDct_ref, pix1, iStride, pix2, iStride);
	WelsDctFourT4_sse2(pDct, pix1, iStride, pix2, iStride);
	for(int i = 0; i < 64; i++) {
			ASSERT_EQ(pDct[i], pDct_ref[i]);
	}
	WelsFree(pix1, "pix1");
	WelsFree(pix2, "pix2");
	WelsFree(pDct, "pDct");
	WelsFree(pDct_ref, "pDct_ref");
}
TEST(Dct_encoder, WelsIDctT4Rec_mmx) {
	const int iStride = 64;
	uint8_t *pPred = (uint8_t*) WelsMalloc(4*iStride*sizeof(uint8_t), "pPred");
	int16_t *pRS = (int16_t*) WelsMalloc(4*iStride*sizeof(int16_t), "pRS");
	uint8_t *pRec = (uint8_t*) WelsMalloc(4*iStride*sizeof(uint8_t), "pRec");
	uint8_t *pRecRef = (uint8_t*) WelsMalloc(4*iStride*sizeof(uint8_t), "pRecRef");
	const int bits = 12;
	const int range = (1 << bits) - 1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++) {
			pPred[i*iStride+j] = rand() & 255;
			pRS[i*4+j] = (rand() & range) - offset;
		}
	WelsIDctT4Rec_mmx(pRec, iStride, pPred, iStride, pRS);
	WelsIDctT4Rec_c(pRecRef, iStride, pPred, iStride, pRS);
	for(int i = 0; i < 4; i++)
		for(int j = 0; j < 4; j++)
			ASSERT_EQ(pRec[i*iStride+j], pRecRef[i*iStride+j]);
	WelsFree(pRecRef, "pRecRef");
	WelsFree(pRec, "pRec");
	WelsFree(pRS, "pRS");
	WelsFree(pPred, "pPred");
}
TEST(Dct_encoder, WelsIDctFourT4Rec_sse2) {
	const int iStride = 64;
	uint8_t *pPred = (uint8_t*) WelsMalloc(8*iStride*sizeof(uint8_t), "pPred");
	int16_t *pRS = (int16_t*) WelsMalloc(64*sizeof(int16_t), "pRS");
	uint8_t *pRec = (uint8_t*) WelsMalloc(8*iStride*sizeof(uint8_t), "pRec");
	uint8_t *pRecRef = (uint8_t*) WelsMalloc(8*iStride*sizeof(uint8_t), "pRecRef");
	const int bits = 12;
	const int range = (1 << bits) - 1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 8; i++)
		for(int j = 0; j < 8; j++) {
			pPred[i*iStride+j] = rand() & 255;
			pRS[i*8+j] = (rand() & range) - offset;
		}
	WelsIDctFourT4Rec_sse2(pRec, iStride, pPred, iStride, pRS);
	WelsIDctFourT4Rec_c(pRecRef, iStride, pPred, iStride, pRS);
	for(int i = 0; i < 8; i++)
		for(int j = 0; j < 8; j++)
			ASSERT_EQ(pRec[i*iStride+j], pRecRef[i*iStride+j]);
	WelsFree(pRecRef, "pRecRef");
	WelsFree(pRec, "pRec");
	WelsFree(pRS, "pRS");
	WelsFree(pPred, "pPred");
}
TEST(Dct_encoder, WelsIDctRecI16x16Dc_sse2) {
	const int iStride = 64;
	uint8_t *pPred = (uint8_t*) WelsMalloc(16*iStride*sizeof(uint8_t), "pPred");
	int16_t *pRS = (int16_t*) WelsMalloc(256*sizeof(int16_t), "pRS");
	uint8_t *pRec = (uint8_t*) WelsMalloc(16*iStride*sizeof(uint8_t), "pRec");
	uint8_t *pRecRef = (uint8_t*) WelsMalloc(16*iStride*sizeof(uint8_t), "pRecRef");
	const int bits = 12;
	const int range = (1 << bits) - 1;
	const int offset = 1 << (bits - 1);
	srand(time(NULL));
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 16; j++) {
			pPred[i*iStride+j] = rand() & 255;
			pRS[i*16+j] = (rand() & range) - offset;
		}
	WelsIDctRecI16x16Dc_sse2(pRec, iStride, pPred, iStride, pRS);
	WelsIDctRecI16x16Dc_c(pRecRef, iStride, pPred, iStride, pRS);
	for(int i = 0; i < 16; i++)
		for(int j = 0; j < 16; j++)
			ASSERT_EQ(pRec[i*iStride+j], pRecRef[i*iStride+j]);
	WelsFree(pRecRef, "pRecRef");
	WelsFree(pRec, "pRec");
	WelsFree(pRS, "pRS");
	WelsFree(pPred, "pPred");
}

// this ut is always failed, and I found that the original asm code fails too, so I need to check it out
TEST(Dct_encoder, WelsHadamardT4Dc_sse2) {
	int16_t *pDct = (int16_t*) WelsMalloc(256*sizeof(int16_t), "pDct");
	int16_t *pLumaDc = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLumaDc");
	int16_t *pLumaDcRef = (int16_t*) WelsMalloc(16*sizeof(int16_t), "pLumaDc");
	srand(100);
	const int bits = 9;
	const int range = (1 << bits) -1;
	const int offset = 1 << (bits - 1);
	for(int i = 0; i < 256; i++) {
		pDct[i] = (rand() & range) - offset;
	}
	WelsHadamardT4Dc_c(pLumaDcRef, pDct);
	WelsHadamardT4Dc_sse2(pLumaDc, pDct);
	for(int i = 0; i < 16; i++)
		ASSERT_EQ(pLumaDc[i], pLumaDcRef[i]);
	WelsFree(pDct, "pDct");
	WelsFree(pLumaDc, "pLumaDc");
	WelsFree(pLumaDcRef, "pLumaDcRef");
}
