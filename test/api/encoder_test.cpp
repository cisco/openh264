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
    {"c4be9cea9fd012da2efa1247a934d3041cb8e210"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    {"0f4638fadc3c688424e3c4708f0c48aed91cf508"}, CAMERA_VIDEO_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    {"22f317206212ec53c195726a07c157cb54163143"}, CAMERA_VIDEO_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"f6f802ea68ecbc313de6327275a7dd92607cb9f8"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_RASTER_SLICE, false, 1, false, false, false // One slice per MB row
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"5f958ec330026009d5fd47d0acc2f3dd9b38acc0"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    // Allow for different output depending on whether averaging is done
    // vertically or horizontally first when downsampling.
    { "a9b51d700974501807199bc0612054de86a23278", "214059cd68f10da94d0c240624e6364071388d74" },
    CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 2, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"bc95b57efa84595a8e41bfb6e97e5086f4743134"}, CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    // Allow for different output depending on whether averaging is done
    // vertically or horizontally first when downsampling.
    { "3b435a98682957cf58b3aa819e6fc6727e0f783d", "d59eb6f7657a82008d60d44f7de95445a35483d7" },
    CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SINGLE_SLICE, false, 4, false, false, false
  },

  // the following values may be adjusted for times since we start tuning the strategy
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"70db2fe49cabaa7a3cef50c1d0b3bc177f655717"}, SCREEN_CONTENT_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    {"34de3a5d4dcbeb956722ba42c2dca1b3840dd12a"}, SCREEN_CONTENT_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    {"49b1e0077eb9698731242890104c67ea170d533c"}, SCREEN_CONTENT_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"138b205e6c5ceacda38c94e975bb8d861fc12b88"}, SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, false
  },
  //for different strategy
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"5db43d830d7d31549b5ff3d085b5c4a803b23cf4"}, SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, true, true, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"2bbea8b822405788aa37755bfabfc111bbf807ac"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, true //turn on cabac
  },

  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"e2231611b47abee0ebfe861e0d077b2000e633aa"}, CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, true
  },

  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"822071b3d149557b758299178d4146dab0e7022d"}, CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_FIXEDSLCNUM_SLICE, false, 1, false, false, true
  },

};

INSTANTIATE_TEST_CASE_P (EncodeFile, EncoderOutputTest,
                         ::testing::ValuesIn (kFileParamArray));
