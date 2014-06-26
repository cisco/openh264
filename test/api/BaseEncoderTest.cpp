#include <fstream>
#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"
#include "BaseEncoderTest.h"

static int InitWithParam (ISVCEncoder* encoder, EUsageType usageType, int width,
                          int height, float frameRate, SliceModeEnum sliceMode, bool denoise, int layers) {
  if (SM_SINGLE_SLICE == sliceMode && !denoise && layers == 1) {
    SEncParamBase param;
    memset (&param, 0, sizeof (SEncParamBase));

    param.iUsageType = usageType;
    param.fMaxFrameRate = frameRate;
    param.iPicWidth = width;
    param.iPicHeight = height;
    param.iTargetBitrate = 5000000;
    param.iInputCsp = videoFormatI420;

    return encoder->Initialize (&param);
  } else {
    SEncParamExt param;
    encoder->GetDefaultParams (&param);

    param.iUsageType = usageType;
    param.fMaxFrameRate = frameRate;
    param.iPicWidth = width;
    param.iPicHeight = height;
    param.iTargetBitrate = 5000000;
    param.iInputCsp = videoFormatI420;
    param.bEnableDenoise = denoise;
    param.iSpatialLayerNum = layers;

    if (sliceMode != SM_SINGLE_SLICE && sliceMode != SM_DYN_SLICE) //SM_DYN_SLICE don't support multi-thread now
      param.iMultipleThreadIdc = 2;

    for (int i = 0; i < param.iSpatialLayerNum; i++) {
      param.sSpatialLayers[i].iVideoWidth = width >> (param.iSpatialLayerNum - 1 - i);
      param.sSpatialLayers[i].iVideoHeight = height >> (param.iSpatialLayerNum - 1 - i);
      param.sSpatialLayers[i].fFrameRate = frameRate;
      param.sSpatialLayers[i].iSpatialBitrate = param.iTargetBitrate;

      param.sSpatialLayers[i].sSliceCfg.uiSliceMode = sliceMode;
      if (sliceMode == SM_DYN_SLICE) {
        param.sSpatialLayers[i].sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 600;
        param.uiMaxNalSize = 1500;
      }
    }
    param.iTargetBitrate *= param.iSpatialLayerNum;

    return encoder->InitializeExt (&param);
  }
}

BaseEncoderTest::BaseEncoderTest() : encoder_ (NULL) {}

void BaseEncoderTest::SetUp() {
  int rv = WelsCreateSVCEncoder (&encoder_);
  ASSERT_EQ (0, rv);
  ASSERT_TRUE (encoder_ != NULL);
}

void BaseEncoderTest::TearDown() {
  if (encoder_) {
    encoder_->Uninitialize();
    WelsDestroySVCEncoder (encoder_);
  }
}

void BaseEncoderTest::EncodeStream (InputStream* in, EUsageType usageType, int width, int height,
                                    float frameRate, SliceModeEnum slices, bool denoise, int layers, Callback* cbk) {
  int rv = InitWithParam (encoder_, usageType, width, height, frameRate, slices, denoise, layers);
  ASSERT_TRUE (rv == cmResultSuccess);

  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = width * height * 3 / 2;

  BufferedData buf;
  buf.SetLength (frameSize);
  ASSERT_TRUE (buf.Length() == (size_t)frameSize);

  SFrameBSInfo info;
  memset (&info, 0, sizeof (SFrameBSInfo));

  SSourcePicture pic;
  memset (&pic, 0, sizeof (SSourcePicture));
  pic.iPicWidth = width;
  pic.iPicHeight = height;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = pic.iPicWidth;
  pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
  pic.pData[0] = buf.data();
  pic.pData[1] = pic.pData[0] + width * height;
  pic.pData[2] = pic.pData[1] + (width * height >> 2);
  while (in->read (buf.data(), frameSize) == frameSize) {
    rv = encoder_->EncodeFrame (&pic, &info);
    ASSERT_TRUE (rv == cmResultSuccess);
    if (info.eFrameType != videoFrameTypeSkip && cbk != NULL) {
      cbk->onEncodeFrame (info);
    }
  }
}

void BaseEncoderTest::EncodeFile (const char* fileName, EUsageType usageType, int width, int height,
                                  float frameRate, SliceModeEnum slices, bool denoise, int layers, Callback* cbk) {
  FileInputStream fileStream;
  ASSERT_TRUE (fileStream.Open (fileName));
  EncodeStream (&fileStream, usageType, width, height, frameRate, slices, denoise, layers, cbk);
}
