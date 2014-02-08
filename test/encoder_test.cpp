#include <gtest/gtest.h>
#include "utils/HashFunctions.h"
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

class EncoderInitTest : public ::testing::Test, public BaseEncoderTest {
 public:
  virtual void SetUp() {
    BaseEncoderTest::SetUp();
  }
  virtual void TearDown() {
    BaseEncoderTest::TearDown();
  }
};

TEST_F(EncoderInitTest, JustInit) {}

struct EncodeFileParam {
  const char* fileName;
  const char* hashStr;
  int width;
  int height;
  float frameRate;
};

class EncoderOutputTest : public ::testing::WithParamInterface<EncodeFileParam>,
    public EncoderInitTest , public BaseEncoderTest::Callback {
 public:
  virtual void SetUp() {
    EncoderInitTest::SetUp();
    if (HasFatalFailure()) {
      return;
    }
    SHA1_Init(&ctx_);
  }
  virtual void onEncodeFrame(const SFrameBSInfo& frameInfo) {
    UpdateHashFromFrame(frameInfo, &ctx_);
  }
 protected:
  SHA_CTX ctx_;
};


TEST_P(EncoderOutputTest, CompareOutput) {
  EncodeFileParam p = GetParam();
  EncodeFile(p.fileName, p.width, p.height, p.frameRate, this);

  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1_Final(digest, &ctx_);
  if (!HasFatalFailure()) {
    ASSERT_TRUE(CompareHash(digest, p.hashStr));
  }
}

static const EncodeFileParam kFileParamArray[] = {
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "4df5751a59eb02153e086ade9b3ecfcb8845c30b", 320, 192, 12.0f
  },
  {
      "res/CiscoVT2people_160x96_6fps.yuv",
      "6eb53b6bfdb95dfca0575bd3efe81aa58163951c", 160, 96, 6.0f
  },
  {
      "res/Static_152_100.yuv",
      "c5af55647a5ead570bd5d96651e05ea699762b8e", 152, 100, 6.0f
  },
};

INSTANTIATE_TEST_CASE_P(EncodeFile, EncoderOutputTest,
    ::testing::ValuesIn(kFileParamArray));
