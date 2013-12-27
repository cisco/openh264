#include<gtest/gtest.h>
#include<stdint.h>
#include<stdlib.h>
#include "mem_align.h"
#include "get_intra_predictor.h"
using namespace WelsDec;

#define TEST_PREDICTION(width, height, ref_func, func) \
TEST(IntraPrediction, func) {\
	int32_t iStride = 64; \
	uint8_t *_buf = (uint8_t*) WelsMalloc(32*iStride*sizeof(uint8_t), "PredBuffer"); \
	uint8_t *_buf_ref = (uint8_t*) WelsMalloc(32*iStride*sizeof(uint8_t), "PredBuffer"); \
	uint8_t *buf = &_buf[iStride+16]; \
	uint8_t *buf_ref = &_buf_ref[iStride+16]; \
	srand(time(NULL)); \
	for(int i = 0; i < 17; i ++) { \
		buf_ref[-1-iStride+i] =  buf[-1-iStride+i] = rand() & 255; \
		buf_ref[-1 + iStride*i] = buf[-1 + iStride*i] = rand() & 255; \
	} \
	func(buf, iStride); \
	ref_func(buf_ref, iStride); \
	for(int i = 0; i < width; i++) \
		for(int j = 0; j < height; j++) \
			ASSERT_EQ(buf[i*iStride+j], buf_ref[i*iStride+j]); \
	WelsFree(_buf, "PredBuffer"); \
	WelsFree(_buf_ref, "PredBuffer"); \
}

TEST_PREDICTION(4, 4, WelsI4x4LumaPredH_c, WelsI4x4LumaPredH_sse2);
TEST_PREDICTION(16, 16, WelsI16x16LumaPredPlane_c, WelsI16x16LumaPredPlane_sse2);
TEST_PREDICTION(16, 16, WelsI16x16LumaPredH_c, WelsI16x16LumaPredH_sse2);
TEST_PREDICTION(16, 16, WelsI16x16LumaPredV_c, WelsI16x16LumaPredV_sse2);
TEST_PREDICTION(8, 8, WelsIChromaPredPlane_c, WelsIChromaPredPlane_sse2);
TEST_PREDICTION(4, 4, WelsI4x4LumaPredDDR_c, WelsI4x4LumaPredDDR_mmx);
TEST_PREDICTION(4, 4, WelsI4x4LumaPredDc_c, WelsI4x4LumaPredDc_sse2);
TEST_PREDICTION(8, 8, WelsIChromaPredH_c, WelsIChromaPredH_mmx);
TEST_PREDICTION(8, 8, WelsIChromaPredV_c, WelsIChromaPredV_mmx);
TEST_PREDICTION(4, 4, WelsI4x4LumaPredHD_c, WelsI4x4LumaPredHD_mmx);
TEST_PREDICTION(4, 4, WelsI4x4LumaPredHU_c, WelsI4x4LumaPredHU_mmx);
TEST_PREDICTION(4, 4, WelsI4x4LumaPredVR_c, WelsI4x4LumaPredVR_mmx);
TEST_PREDICTION(4, 4, WelsI4x4LumaPredDDL_c, WelsI4x4LumaPredDDL_mmx);
TEST_PREDICTION(4, 4, WelsI4x4LumaPredVL_c, WelsI4x4LumaPredVL_mmx);
TEST_PREDICTION(8, 8, WelsIChromaPredDc_c, WelsIChromaPredDc_sse2);
TEST_PREDICTION(16, 16, WelsI16x16LumaPredDc_c, WelsI16x16LumaPredDc_sse2);
TEST_PREDICTION(16, 16, WelsI16x16LumaPredDcTop_c, WelsI16x16LumaPredDcTop_sse2);
TEST_PREDICTION(16, 16, WelsI16x16LumaPredDcNA_c, WelsI16x16LumaPredDcNA_sse2);
TEST_PREDICTION(8, 8, WelsIChromaPredDcLeft_c, WelsIChromaPredDcLeft_mmx);
TEST_PREDICTION(8, 8, WelsIChromaPredDcNA_c, WelsIChromaPredDcNA_mmx);
TEST_PREDICTION(8, 8, WelsIChromaPredDcTop_c, WelsIChromaPredDcTop_sse2);
