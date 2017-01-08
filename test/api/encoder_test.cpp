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
  const char* pkcHashStr[2];
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
    pEnxParamExt->sSpatialLayers[i].sSliceArgument.uiSliceMode = pEncFileParam->eSliceMode;
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
    CompareHashAnyOf (digest, p.pkcHashStr, sizeof p.pkcHashStr / sizeof *p.pkcHashStr);
  }
}
static const EncodeFileParam kFileParamArray[] = {
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"4370a847a8d680a29176489f3194b6e7bd2fc468"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    {"3ba799ac78194154d990836db529325c0c42f62a"}, CAMERA_VIDEO_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    {"ba5a3f7cce2fd2e692bb07a25e288edaee3c220d"}, CAMERA_VIDEO_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"b608de962bd4eb127f96fe91cb4a15435aff35c8"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_RASTER_SLICE, false, 1, false, false, false // One slice per MB row
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"d151b7e8755b4dceece593db0f0eb5806174e1fd"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    // Allow for different output depending on whether averaging is done
    // vertically or horizontally first when downsampling.
    { "e5dc8db66abf235b146a844a1c890e9e1035daae", "1ddc203a75454e7fe29340d037577aad763d84d4" },
    CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 2, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"d6a42dfac92eb8e9a29f59b2da73e6af41f6d803"}, CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    // Allow for different output depending on whether averaging is done
    // vertically or horizontally first when downsampling.
    { "aef963ffd561d43d6390a43a66792d420a6856cc", "f229eeb69065129d898a11c1bd40e79cd25c2af7" },
    CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SINGLE_SLICE, false, 4, false, false, false
  },

  // the following values may be adjusted for times since we start tuning the strategy
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"10e4a1c5825d086e8b7d707db53d0642a26caa29"}, SCREEN_CONTENT_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    {"93b1e3eea46e74801d4fbfc99c2e4339986be6db"}, SCREEN_CONTENT_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    {"9ecf846b0555c4b4a29b0d3ecef47691b6330d18"}, SCREEN_CONTENT_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"d625df1540b074ee4b13034c728b9a02f8bb1371"}, SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, false
  },
  //for different strategy
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"d1c2ee11318521a911718d883d720e9e83bf533f"}, SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, true, true, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"a8eec17dc4ae43cf01eeb32f072661d7ad810691"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, true //turn on cabac
  },

  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"a511da916ba97967e18540953d2c0b1287b50aa3"}, CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, true
  },

  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"752aa204be96efbf5f838e35164fa5341a464420"}, CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_FIXEDSLCNUM_SLICE, false, 1, false, false, true
  },

};

INSTANTIATE_TEST_CASE_P (EncodeFile, EncoderOutputTest,
                         ::testing::ValuesIn (kFileParamArray));
