#include <gtest/gtest.h>
#include "utils/HashFunctions.h"
#include "BaseEncoderTest.h"

static void UpdateHashFromFrame(const SFrameBSInfo& info, SHA1Context* ctx) {
  for (int i = 0; i < info.iLayerNum; ++i) {
    const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
    int layerSize = 0;
    for (int j = 0; j < layerInfo.iNalCount; ++j) {
      layerSize += layerInfo.iNalLengthInByte[j];
    }
    SHA1Input(ctx, layerInfo.pBsBuf, layerSize);
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
  SliceModeEnum slices;
  bool denoise;
  int layers;
};

class EncoderOutputTest : public ::testing::WithParamInterface<EncodeFileParam>,
    public EncoderInitTest , public BaseEncoderTest::Callback {
 public:
  virtual void SetUp() {
    EncoderInitTest::SetUp();
    if (HasFatalFailure()) {
      return;
    }
    SHA1Reset(&ctx_);
  }
  virtual void onEncodeFrame(const SFrameBSInfo& frameInfo) {
    UpdateHashFromFrame(frameInfo, &ctx_);
  }
 protected:
  SHA1Context ctx_;
};


TEST_P(EncoderOutputTest, CompareOutput) {
  EncodeFileParam p = GetParam();
  EncodeFile(p.fileName, p.width, p.height, p.frameRate, p.slices, p.denoise, p.layers, this);

  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1Result(&ctx_, digest);
  if (!HasFatalFailure()) {
    CompareHash(digest, p.hashStr);
  }
}

static const EncodeFileParam kFileParamArray[] = {
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "8d4c87f48e8a679c1ccbf5fe1ee040fed4776b30", 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1
  },
  {
      "res/CiscoVT2people_160x96_6fps.yuv",
      "75334dc69d95b8ac2e0a52977bba0179df4f151f", 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1
  },
  {
      "res/Static_152_100.yuv",
      "3467201e18a934e7f8c50f3c8f3e05f4334ad12c", 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "a57c7cc8a00ffe8d8ca5527a13af1683a41b5150", 320, 192, 12.0f, SM_ROWMB_SLICE, false, 1 // One slice per MB row
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "bc9b203c1b031299df7981201c2af393994d876f", 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "ba81a0f1a14214e6d3c7f1608991b3ac97789370", 320, 192, 12.0f, SM_SINGLE_SLICE, false, 2
  },
};

INSTANTIATE_TEST_CASE_P(EncodeFile, EncoderOutputTest,
    ::testing::ValuesIn(kFileParamArray));
