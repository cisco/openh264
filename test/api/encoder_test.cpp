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
  int deblocking;
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
  EncodeFile(p.fileName, p.width, p.height, p.frameRate, p.slices, p.denoise, p.deblocking, p.layers, this);

  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1Result(&ctx_, digest);
  if (!HasFatalFailure()) {
    CompareHash(digest, p.hashStr);
  }
}

static const EncodeFileParam kFileParamArray[] = {
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "06441376891cbc237a36e59b62131cd94ff9cb19", 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, 1
  },
  {
      "res/CiscoVT2people_160x96_6fps.yuv",
      "4f3759fc44125b27a179ebff158dbba9e431bd0b", 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, 1
  },
  {
      "res/Static_152_100.yuv",
      "af5c6a41b567ce1b2cb6fd427f4379473d3b829f", 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, 1
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "be0079b022b18fdce04570db24e4327ca26a0ecb", 320, 192, 12.0f, SM_ROWMB_SLICE, false, 1, 1 // One slice per MB row
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "f4649601ca15f9693671d7e161e9daa8efd3a7a1", 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1, 1
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "e4bcd744d2e6f885f6c0dbe5d52818ba64d986d9", 320, 192, 12.0f, SM_SINGLE_SLICE, false, 0, 1
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "ba81a0f1a14214e6d3c7f1608991b3ac97789370", 320, 192, 12.0f, SM_SINGLE_SLICE, false, 0, 2
  },
};

INSTANTIATE_TEST_CASE_P(EncodeFile, EncoderOutputTest,
    ::testing::ValuesIn(kFileParamArray));
