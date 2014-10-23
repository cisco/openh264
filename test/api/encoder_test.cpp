#include <gtest/gtest.h>
#include "utils/HashFunctions.h"
#include "BaseEncoderTest.h"
#include <string>
static void UpdateHashFromFrame (const SFrameBSInfo& info, SHA1Context* ctx) {
  for (int i = 0; i < info.iLayerNum; ++i) {
    const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
    int layerSize = 0;
    for (int j = 0; j < layerInfo.iNalCount; ++j) {
      layerSize += layerInfo.pNalLengthInByte[j];
    }
    SHA1Input (ctx, layerInfo.pBsBuf, layerSize);
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

TEST_F (EncoderInitTest, JustInit) {}

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
  bool isLossless;
  bool enableLtr;
};

class EncoderOutputTest : public ::testing::WithParamInterface<EncodeFileParam>,
  public EncoderInitTest , public BaseEncoderTest::Callback {
 public:
  virtual void SetUp() {
    EncoderInitTest::SetUp();
    if (HasFatalFailure()) {
      return;
    }
    SHA1Reset (&ctx_);
  }
  virtual void onEncodeFrame (const SFrameBSInfo& frameInfo) {
    UpdateHashFromFrame (frameInfo, &ctx_);
  }
 protected:
  SHA1Context ctx_;
};


TEST_P (EncoderOutputTest, CompareOutput) {
  EncodeFileParam p = GetParam();
#if defined(ANDROID_NDK)
  std::string filename = std::string ("/sdcard/") + p.fileName;
  EncodeFile (filename.c_str(), p.usageType , p.width, p.height, p.frameRate, p.slices, p.denoise, p.layers, p.isLossless,
              p.enableLtr, this);
#else
  EncodeFile (p.fileName, p.usageType , p.width, p.height, p.frameRate, p.slices, p.denoise, p.layers, p.isLossless,
              p.enableLtr, this);
#endif
  //will remove this after screen content algorithms are ready,
  //because the bitstream output will vary when the different algorithms are added.
  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1Result (&ctx_, digest);
  if (!HasFatalFailure()) {
    CompareHash (digest, p.hashStr);
  }
}
static const EncodeFileParam kFileParamArray[] = {
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "0a36b75e423fc6b49f6adf7eee12c039a096f538", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    "73981e6ea5b62f7338212c538a7cc755e7c9c030", CAMERA_VIDEO_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false
  },
  {
    "res/Static_152_100.yuv",
    "02bbff550ee0630e44e46e14dc459d3686f2a360", CAMERA_VIDEO_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "c8b759bcec7ffa048f1d3ded594b8815bed0aead", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_ROWMB_SLICE, false, 1, false, false // One slice per MB row
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "e64ba75456c821ca35a949eda89f85bff8ee69fa", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "684e6d141ada776892bdb01ee93efe475983ed36", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 2, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "2bc06262d87fa0897ad4c336cc4047d5a67f7203", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "68c3220e49b7a57d563faf7c99a870ab34a23400", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SINGLE_SLICE, false, 4, false, false
  },
  // the following values may be adjusted for times since we start tuning the strategy
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "3ce65d9c326657b845cd00b22ce76128c29f8347", SCREEN_CONTENT_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    "2b57e1cc7a4db6258116c302eada3bf870ee94a1", SCREEN_CONTENT_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false
  },
  {
    "res/Static_152_100.yuv",
    "bad065da4564d0580a1722d91463fa0f9fd947c8", SCREEN_CONTENT_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "8ee6cd375b58e9877f6145fb72da844e65162b14", SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, false, false
  },
  //for different strategy
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "868f327765dc8e705ad6a9a942bfc7e32c03c791", SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, true, true
  },
};

INSTANTIATE_TEST_CASE_P (EncodeFile, EncoderOutputTest,
                         ::testing::ValuesIn (kFileParamArray));
