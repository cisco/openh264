#include <cstdio>
#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/HashFunctions.h"
#include "BaseDecoderTest.h"
#include "BaseEncoderTest.h"

static void UpdateHashFromFrame(const SFrameBSInfo& info, SHA_CTX* ctx) {
  for (int i = 0; i < info.iLayerNum; ++i) {
    const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
    int layerSize = 0;
    for (int j = 0; j < layerInfo.iNalCount; ++j) {
      layerSize += layerInfo.iNalLengthInByte[j];
    }
    SHA1_Update(ctx, layerInfo.pBsBuf, layerSize);
  }
}

static void WritePlaneToFile(FILE* file, const uint8_t* plane,
    int width, int height, int stride) {
  for (int i = 0; i < height; i++) {
    fwrite(plane, 1, width, file);
    plane += stride;
  }
}

struct DecodeEncodeFileParam {
  const char* fileName;
  const char* hashStr;
  int width;
  int height;
  float frameRate;
};

class DecodeEncodeTest : public ::testing::TestWithParam<DecodeEncodeFileParam>,
    public BaseDecoderTest, public BaseDecoderTest::Callback,
    public BaseEncoderTest , public BaseEncoderTest::Callback {
 public:
  DecodeEncodeTest() : tmpFileName_(NULL), tmpFile_(NULL) {}

  virtual void SetUp() {
    BaseDecoderTest::SetUp();
    if (HasFatalFailure()) {
      return;
    }
    BaseEncoderTest::SetUp();
    if (HasFatalFailure()) {
      return;
    }

    tmpFileName_ = tmpnam(NULL);
    tmpFile_ = fopen(tmpFileName_, "wb");
    ASSERT_TRUE(tmpFile_ != NULL);

    SHA1_Init(&ctx_);
  }

  virtual void TearDown() {
    BaseDecoderTest::TearDown();
    BaseEncoderTest::TearDown();
  }

  virtual void onDecodeFrame(const Frame& frame) {
    const Plane& y = frame.y;
    const Plane& u = frame.u;
    const Plane& v = frame.v;
    WritePlaneToFile(tmpFile_, y.data, y.width, y.height, y.stride);
    WritePlaneToFile(tmpFile_, u.data, u.width, u.height, u.stride);
    WritePlaneToFile(tmpFile_, v.data, v.width, v.height, v.stride);
  }

  virtual void onEncodeFrame(const SFrameBSInfo& frameInfo) {
    UpdateHashFromFrame(frameInfo, &ctx_);
  }

 protected:
  SHA_CTX ctx_;
  const char* tmpFileName_;
  FILE* tmpFile_;
};

TEST_P(DecodeEncodeTest, CompareOutput) {
  DecodeEncodeFileParam p = GetParam();

  DecodeFile(p.fileName, this);
  if (HasFatalFailure()) {
    return;
  }

  // force flushing the file
  fclose(tmpFile_);

  EncodeFile(tmpFileName_, p.width, p.height, p.frameRate, this);
  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1_Final(digest, &ctx_);
  if (!HasFatalFailure()) {
    ASSERT_TRUE(CompareHash(digest, p.hashStr));
  }

  remove(tmpFileName_);
}

static const DecodeEncodeFileParam kFileParamArray[] = {
  {"res/test_vd_1d.264", "41c672107cfe9e8e8a67b5d08cbd701f6c982ccd", 320, 192, 12.0f},
  {"res/test_vd_rc.264", "d546ea7c671b42503f8a46ba50bef2a3eaca4c5a", 320, 192, 12.0f},
};

INSTANTIATE_TEST_CASE_P(DecodeEncodeFile, DecodeEncodeTest,
    ::testing::ValuesIn(kFileParamArray));
