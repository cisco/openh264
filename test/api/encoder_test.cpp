#include <gtest/gtest.h>
#include "utils/HashFunctions.h"
#include "BaseEncoderTest.h"

static void UpdateHashFromFrame(const SFrameBSInfo& info, SHA1Context* ctx) {
  for (int i = 0; i < info.iLayerNum; ++i) {
    const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
    int layerSize = 0;
    for (int j = 0; j < layerInfo.iNalCount; ++j) {
      layerSize += layerInfo.pNalLengthInByte[j];
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
      "86ee1314eabf5a6d6083c5260d36e2d908f36ea5", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1
  },
  {
      "res/CiscoVT2people_160x96_6fps.yuv",
      "20f3e5cbefe70e5a8793e60672fd70e136bc082a", CAMERA_VIDEO_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1
  },
  {
      "res/Static_152_100.yuv",
      "035f2e5b35947ab43cad7c5ca4cd8a3b658306ba", CAMERA_VIDEO_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "c8b759bcec7ffa048f1d3ded594b8815bed0aead", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_ROWMB_SLICE, false, 1 // One slice per MB row
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "6f49cc4e746522d7d91599c9a883072e838ae918", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1
  },
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "4da4526a763497cfd777e1a211c67df5e9d48b29", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 2
  },
  {
      "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
      "6df1ece77c0de63cdf8ab52ccef3a7d139022717", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1
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
