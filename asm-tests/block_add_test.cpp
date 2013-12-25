#include <gtest/gtest.h>
#include <stdlib.h>
#include <stdint.h>
extern "C" void WelsResBlockZero16x16_sse2(int16_t* pBlock, int32_t iStride);
TEST(block_add, WelsResBlockZero16x16_sse2) {
  const int32_t iStride = 64;
  uint8_t *_pb = (uint8_t*)malloc((16*iStride)*sizeof(int16_t)+15);
  ASSERT_TRUE(_pb);
  uint8_t *__pb = (uint8_t*)((((unsigned long)(_pb)) + 15) & ~15);
  int16_t *pb = (int16_t*)__pb;
  WelsResBlockZero16x16_sse2(pb, iStride);
  for(int i = 0; i < 16; i++)
    for(int j = 0; j < 16; j++) {
      ASSERT_EQ(pb[i*iStride+j], 0);
    }
  free(_pb);
}

extern  "C" void WelsResBlockZero8x8_sse2(int16_t *pBlock, int32_t iStride);
TEST(block_add, WelsResBlockZero8x8_sse2) {
  const int32_t iStride = 64;
  int16_t *_pb = (int16_t*)malloc(16*iStride+15);
  int16_t *pb =(int16_t*) (((((unsigned long)(_pb)) + 15) >> 4) << 4);
  WelsResBlockZero8x8_sse2(pb, iStride);
  for(int i = 0; i < 8; i++)
    for(int j = 0; j < 8; j++) {
      ASSERT_EQ(pb[i*iStride+j], 0);
    }
  free(_pb);
}

