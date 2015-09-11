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
    "03d992976d9b9c3dd7f9803883d3f7b7614b47bf", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    "f7ccdadc446f87927d63f9a1fe78c72344a920cb", CAMERA_VIDEO_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    "c25d6d8c45de417cb42d861ee71a42d0ebfb6da9", CAMERA_VIDEO_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "21dbfaaf4f09af735298434c1f97cf95a464165b", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_ROWMB_SLICE, false, 1, false, false, false // One slice per MB row
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "a37f67d6050fe113ccfb65e562a6c32086a29eb3", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "4f12d2b08af761b8bb61a5b25f96ad6a321a43fa", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 2, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "9006ccba6d9d90815bd42ba850b344919fa399f4", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "2b5965c752e1f722592c3ce9a1eb82445c9dbaa3", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SINGLE_SLICE, false, 4, false, false, false
  },
  // the following values may be adjusted for times since we start tuning the strategy
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "a731f20ec06d5591142f80e0d0e3edff88167d19", SCREEN_CONTENT_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    "3a472cf6fbfa474c73fabaeb6e68c07112a2f5d2", SCREEN_CONTENT_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    "43e0ea83028ea51eb41f29633af50fff7137024a", SCREEN_CONTENT_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "aa0aaca96475c65d66e90e513618fa814b098dff", SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, false, false, false
  },
  //for different strategy
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "5622722316eed2482bf78520eda7e448312e340e", SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, true, true, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "cd9716c92c595f5492a46adf33f07bedf98db408", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, true //turn on cabac
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "dfd4666f9b90d5d77647454e2a06d546adac6a7c", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_DYN_SLICE, false, 1, false, false, true
  },
};

INSTANTIATE_TEST_CASE_P (EncodeFile, EncoderOutputTest,
                         ::testing::ValuesIn (kFileParamArray));
