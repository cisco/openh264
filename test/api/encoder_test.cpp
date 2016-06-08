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
    CompareHash (digest, p.pkcHashStr);
  }
}
static const EncodeFileParam kFileParamArray[] = {
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "ee97bab09041362fbe8aed16f69a55f16b106f92", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    "37b4f5b7b77b362dee9a74ac4b2bc043537f2dd0", CAMERA_VIDEO_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    "b220be0163974e36fbd6662236d4e05566a21546", CAMERA_VIDEO_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "21dbfaaf4f09af735298434c1f97cf95a464165b", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_RASTER_SLICE, false, 1, false, false, false // One slice per MB row
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "409ced068a5a313cad7c654d27ab7a2edaed4630", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
// X86_ASM downsampling routines average vertically first, as opposed to
// horizontally first, which results in different output.
#ifdef X86_ASM
    "a5341d588b769809c1f1d983e5a0fcef7362f3ad",
#else
    "73156dfc1dc45924349b5b79f8debcac13d7231d",
#endif
    CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 2, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "84f97cff898055aaf96a303960abb76e12bf24c1", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
// X86_ASM downsampling routines average vertically first, as opposed to
// horizontally first, which results in different output.
#ifdef X86_ASM
    "ec9d776a7d92cf0f6640065aee8af2450af0e993",
#else
    "3943145545a2bd27a642b2045d4e3dbae55c6870",
#endif
    CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SINGLE_SLICE, false, 4, false, false, false
  },
  // the following values may be adjusted for times since we start tuning the strategy
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "c384a0dafc46573d02a38d8323304c5e1309d9d0", SCREEN_CONTENT_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    "637d50652f9bb8750359c4b418f30a039908d56d", SCREEN_CONTENT_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    "77a140c6bd80a6479ee13aecd2a8dac0a17cf03d", SCREEN_CONTENT_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "8027935ed347671cb2a181d09cd4380ce2c0da79", SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, false
  },
  //for different strategy
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "0e1af98bd978e602cc15329ba70a2c4c2afe8016", SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, true, true, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    "04ad01bb3872f7dae055c1ec661218f41a020dac", CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, true //turn on cabac
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "8942c8811fd0cb086d709734bf9fd5b184772f5e", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, true
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    "a27539982433279faa9975c96eaec28df770223e", CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_FIXEDSLCNUM_SLICE, false, 1, false, false, true
  },
};

INSTANTIATE_TEST_CASE_P (EncodeFile, EncoderOutputTest,
                         ::testing::ValuesIn (kFileParamArray));
