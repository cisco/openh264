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
  const char* pkcFileName;
  const char* pkcHashStr;
  EUsageType eUsageType;
  int iWidth;
  int iHeight;
  float fFrameRate;
  SliceModeEnum eSliceMode;
  bool bDenoise;
  int  iLayerNum;
  bool bLossless;
  bool bEnableLtr;
  bool bCabac;
// unsigned short iMultipleThreadIdc;
};

void EncFileParamToParamExt (EncodeFileParam* pEncFileParam, SEncParamExt* pEnxParamExt) {
  ASSERT_TRUE (NULL != pEncFileParam && NULL != pEnxParamExt);
  pEnxParamExt->iUsageType       = pEncFileParam->eUsageType;
  pEnxParamExt->iPicWidth        = pEncFileParam->iWidth;
  pEnxParamExt->iPicHeight       = pEncFileParam->iHeight;
  pEnxParamExt->fMaxFrameRate    = pEncFileParam->fFrameRate;
  pEnxParamExt->iSpatialLayerNum = pEncFileParam->iLayerNum;

  pEnxParamExt->bEnableDenoise   = pEncFileParam->bDenoise;
  pEnxParamExt->bIsLosslessLink  = pEncFileParam->bLossless;
  pEnxParamExt->bEnableLongTermReference = pEncFileParam->bEnableLtr;
  pEnxParamExt->iEntropyCodingModeFlag   = pEncFileParam->bCabac ? 1 : 0;

  for (int i = 0; i < pEnxParamExt->iSpatialLayerNum; i++) {
    pEnxParamExt->sSpatialLayers[i].sSliceCfg.uiSliceMode = pEncFileParam->eSliceMode;
  }

}

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
  SEncParamExt EnxParamExt;

  EncFileParamToParamExt (&p, &EnxParamExt);

#if defined(ANDROID_NDK)
  std::string filename = std::string ("/sdcard/") + p.pkcFileName;
  EncodeFile (p.pkcFileName, &EnxParamExt, this);
#else
  EncodeFile (p.pkcFileName, &EnxParamExt, this);
#endif
  //will remove this after screen content algorithms are ready,
  //because the bitstream output will vary when the different algorithms are added.
  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1Result (&ctx_, digest);
  if (!HasFatalFailure()) {
    CompareHash (digest, p.pkcHashStr);
  }
}
static const EncodeFileParam kFileParamArray[] = {
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "bfe882345e17346a76438dad967ecd8a15052e0b", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    "9879c0e65e0f2d166c450c371c95a1e36eadfa4e", CAMERA_VIDEO_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    "f2a4089dbf7278f9e7204351d6ce93e913d6ea1f", CAMERA_VIDEO_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "3ea419b2ea341fe548dbe65be0e955ffea59483a", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_ROWMB_SLICE, false, 1, false, false, false // One slice per MB row
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "7614af78a3cb16df8187c1db5492f82f3440a63b", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "833a7ae798720f72d6a0b46e5cd1f046c0fdd758", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 2, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "6cc7d08b2a80fc2836396808f919f9b5d4ee1d97", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "40bc1ffec7febdb725ae99954bfa4cb64f070891", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SINGLE_SLICE, false, 4, false, false, false
  },
  // the following values may be adjusted for times since we start tuning the strategy
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "6ce04eca6d8a6127ae1a6ee3e0d67977f806f5e2", SCREEN_CONTENT_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    "2a9942d8d67509712828a5268483141bb8c9b30f", SCREEN_CONTENT_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    "2041510ebc2810596495d694672ada66f6ec3a0e", SCREEN_CONTENT_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "116663b5a00f000ab40e95b6202563e3cb4ce488", SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, false, false, false
  },
  //for different strategy
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "6b981e68a6e0b03a8b769fdd39ec5838380bc4d2", SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, true, true, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "aae07cf34b065871921f742cdf25a4bf4d623935", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, true //turn on cabac
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "bb8ee13f829fa593b77760afdeb1215d0a577ee1", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, false, false, true
  },
};

INSTANTIATE_TEST_CASE_P (EncodeFile, EncoderOutputTest,
                         ::testing::ValuesIn (kFileParamArray));
