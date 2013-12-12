#include <gtest/gtest.h>
#if defined (WIN32)
#include <windows.h>
#include <tchar.h>
#else
#include <string.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "codec_def.h"
#include "codec_app_def.h"
#include "codec_api.h"

class CodecTest : public ::testing::Test {
 public:
  CodecTest() : decoder_(NULL) {}

  ~CodecTest() {
    if (decoder_) DestroyDecoder(decoder_);
  }

  void SetUp() {
    long rv = CreateDecoder(&decoder_);
    ASSERT_EQ(0, rv);
    ASSERT_TRUE(decoder_);
  }

protected:
  ISVCDecoder *decoder_;
};

TEST_F(CodecTest, JustInit) {
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
