#include <gtest/gtest.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <fstream>

#include "codec_def.h"
#include "codec_app_def.h"
#include "codec_api.h"

#include "utils/BufferedData.h"
#include "utils/HashFunctions.h"

static int InitWithParam(ISVCEncoder* encoder, int width,
    int height, float frameRate) {
  SVCEncodingParam param;
  memset (&param, 0, sizeof(SVCEncodingParam));

  param.sSpatialLayers[0].iVideoWidth  = width;
  param.sSpatialLayers[0].iVideoHeight = height;
  param.sSpatialLayers[0].fFrameRate = frameRate;
  param.sSpatialLayers[0].iQualityLayerNum = 1;
  param.sSpatialLayers[0].iSpatialBitrate = 600000;

  SSliceConfig* sliceCfg = &param.sSpatialLayers[0].sSliceCfg;
  sliceCfg->sSliceArgument.uiSliceNum = 1;
  sliceCfg->sSliceArgument.uiSliceSizeConstraint = 1500;
  sliceCfg->sSliceArgument.uiSliceMbNum[0] = 960;

  param.fFrameRate = param.sSpatialLayers[0].fFrameRate;
  param.iPicWidth = param.sSpatialLayers[0].iVideoWidth;
  param.iPicHeight = param.sSpatialLayers[0].iVideoHeight;
  param.iTargetBitrate = 5000000;
  param.iTemporalLayerNum = 3;
  param.iSpatialLayerNum = 1;
  param.bEnableBackgroundDetection = true;
  param.bEnableLongTermReference = true;
  param.iLtrMarkPeriod = 30;
  param.iInputCsp = videoFormatI420;
  param.bEnableSpsPpsIdAddition = true;

  return encoder->Initialize(&param, INIT_TYPE_PARAMETER_BASED);
}

static void UpdateHashFromFrame(const SFrameBSInfo& info, SHA_CTX* ctx) {
  for (int i = 0; i < info.iLayerNum; ++i) {
    const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
    int layerSize = 0;
    for (int j = 0; j < layerInfo.iNalCount; ++j) {
      layerSize += layerInfo.iNalLengthInByte[j];
    }
    SHA1_Update(ctx, layerInfo.pBsBuf, layerSize);
  }
}

static void CompareFileToHash(ISVCEncoder* encoder,
    const char* fileName, const char* hashStr,
    int width, int height, float frameRate) {

  std::ifstream file(fileName, std::ios::in | std::ios::binary);
  ASSERT_TRUE(file.is_open());

  int rv = InitWithParam(encoder, width, height, frameRate);
  ASSERT_TRUE(rv == cmResultSuccess);

  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = width * height * 3 / 2;

  BufferedData buf;
  buf.SetLength(frameSize);
  ASSERT_TRUE(buf.Length() == frameSize);
  char* data = reinterpret_cast<char*>(buf.data());

  SFrameBSInfo info;
  memset(&info, 0, sizeof(SFrameBSInfo));

  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA_CTX ctx;
  SHA1_Init(&ctx);

  while (file.read(data, frameSize), file.gcount() == frameSize) {
    rv = encoder->EncodeFrame(buf.data(), &info);
    if (rv == videoFrameTypeInvalid) {
      SHA1_Final(digest, &ctx);
      FAIL() << "unable to encode frame";
    }
    if (rv != videoFrameTypeSkip) {
      UpdateHashFromFrame(info, &ctx);
    }
  }

  SHA1_Final(digest, &ctx);
  ASSERT_TRUE(CompareHash(digest, hashStr));
}

class EncoderInitTest : public ::testing::Test {
public:
  EncoderInitTest() : encoder_(NULL) {}

  virtual void SetUp() {
    int rv = CreateSVCEncoder(&encoder_);
    ASSERT_EQ(0, rv);
    ASSERT_TRUE(encoder_ != NULL);
  }

  virtual void TearDown() {
    if (encoder_ != NULL) {
      encoder_->Uninitialize();
      DestroySVCEncoder(encoder_);
    }
  }

protected:
  ISVCEncoder* encoder_;
};

TEST_F(EncoderInitTest, JustInit) {
}

struct EncodeFileParam {
  const char* fileName;
  const char* hashStr;
  int width;
  int height;
  float frameRate;
};

class EncoderOutputTest : public EncoderInitTest ,
    public ::testing::WithParamInterface<EncodeFileParam> {
};


TEST_P(EncoderOutputTest, CompareOutput) {
  EncodeFileParam p = GetParam();
  CompareFileToHash(encoder_, p.fileName, p.hashStr, p.width, p.height, p.frameRate);
}

static const EncodeFileParam kFileParamArray[] = {
  {
      "res/CiscoVT2people_320x192_12fps.yuv",
      "4df5751a59eb02153e086ade9b3ecfcb8845c30b", 320, 192, 12.0f
  },
  {
      "res/CiscoVT2people_160x96_6fps.yuv",
      "6eb53b6bfdb95dfca0575bd3efe81aa58163951c", 160, 96, 6.0f
  },
};

INSTANTIATE_TEST_CASE_P(EncodeFile, EncoderOutputTest,
    ::testing::ValuesIn(kFileParamArray));
