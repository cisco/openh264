#include <fstream>
#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/BufferedData.h"
#include "BaseEncoderTest.h"

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

BaseEncoderTest::BaseEncoderTest() : encoder_(NULL) {}

void BaseEncoderTest::SetUp() {
  int rv = CreateSVCEncoder(&encoder_);
  ASSERT_EQ(0, rv);
  ASSERT_TRUE(encoder_ != NULL);
}

void BaseEncoderTest::TearDown() {
  if (encoder_) {
    encoder_->Uninitialize();
    DestroySVCEncoder(encoder_);
  }
}

void BaseEncoderTest::EncodeFile(const char* fileName, int width, int height,
    float frameRate, Callback* cbk) {
  std::ifstream file(fileName, std::ios::in | std::ios::binary);
  ASSERT_TRUE(file.is_open());

  int rv = InitWithParam(encoder_, width, height, frameRate);
  ASSERT_TRUE(rv == cmResultSuccess);

  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = width * height * 3 / 2;

  BufferedData buf;
  buf.SetLength(frameSize);
  ASSERT_TRUE(buf.Length() == frameSize);
  char* data = reinterpret_cast<char*>(buf.data());

  SFrameBSInfo info;
  memset(&info, 0, sizeof(SFrameBSInfo));

  while (file.read(data, frameSize), file.gcount() == frameSize) {
    rv = encoder_->EncodeFrame(buf.data(), &info);
    ASSERT_TRUE(rv != videoFrameTypeInvalid);
    if (rv != videoFrameTypeSkip && cbk != NULL) {
      cbk->onEncodeFrame(info);
    }
  }
}
