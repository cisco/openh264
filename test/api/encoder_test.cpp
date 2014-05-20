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
  EUsageType usageType;
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
  EncodeFile(p.fileName, p.usageType ,p.width, p.height, p.frameRate, p.slices, p.denoise, p.layers, this);

  //will remove this after screen content algorithms are ready,
  //because the bitstream output will vary when the different algorithms are added.
  if(p.usageType == SCREEN_CONTENT_REAL_TIME)
    return;
  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1Result(&ctx_, digest);
  if (!HasFatalFailure()) {
    CompareHash(digest, p.hashStr);
  }
}

static const EncodeFileParam kFileParamArray[] = {
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "5fa8c8551133b7d7586f498121028d0e05a28e1d", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1
  },
  {
      "res/CiscoVT2people_160x96_6fps.yuv",
      "c619645a7d46f8fade40d2b0e5ae01adc2e5c3ff", CAMERA_VIDEO_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1
  },
  {
      "res/Static_152_100.yuv",
      "68cde1b5f790213baab1a10d4a19a3618c138405", CAMERA_VIDEO_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "d0d0a087451c2813e9b0fd61bc5b25a4e82519ac", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_ROWMB_SLICE, false, 1 // One slice per MB row
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "d3760e61e38af978d5b59232d8402448812d1540", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "a74ae382356098fb5cce216a97f2c0cef00a0a9d", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 2
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "", SCREEN_CONTENT_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1
  },
  {
     "res/CiscoVT2people_160x96_6fps.yuv",
      "", SCREEN_CONTENT_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1
  },
  {
      "res/Static_152_100.yuv",
      "", SCREEN_CONTENT_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1
  }
};

INSTANTIATE_TEST_CASE_P(EncodeFile, EncoderOutputTest,
    ::testing::ValuesIn(kFileParamArray));
