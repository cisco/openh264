#include <fstream>
#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"
#include "BaseEncoderTest.h"

static int InitWithParam (ISVCEncoder* encoder, SEncParamExt* pEncParamExt) {

  SliceModeEnum eSliceMode = pEncParamExt->sSpatialLayers[0].sSliceCfg.uiSliceMode;
  bool bBaseParamFlag      = (SM_SINGLE_SLICE == eSliceMode             && !pEncParamExt->bEnableDenoise
                              && pEncParamExt->iSpatialLayerNum == 1     && !pEncParamExt->bIsLosslessLink
                              && !pEncParamExt->bEnableLongTermReference && !pEncParamExt->iEntropyCodingModeFlag) ? true : false;
  if (bBaseParamFlag) {
    SEncParamBase param;
    memset (&param, 0, sizeof (SEncParamBase));

    param.iUsageType     = pEncParamExt->iUsageType;
    param.fMaxFrameRate  = pEncParamExt->fMaxFrameRate;
    param.iPicWidth      = pEncParamExt->iPicWidth;
    param.iPicHeight     = pEncParamExt->iPicHeight;
    param.iTargetBitrate = 5000000;

    return encoder->Initialize (&param);
  } else {
    SEncParamExt param;
    encoder->GetDefaultParams (&param);

    param.iUsageType       = pEncParamExt->iUsageType;
    param.fMaxFrameRate    = pEncParamExt->fMaxFrameRate;
    param.iPicWidth        = pEncParamExt->iPicWidth;
    param.iPicHeight       = pEncParamExt->iPicHeight;
    param.iTargetBitrate   = 5000000;
    param.bEnableDenoise   = pEncParamExt->bEnableDenoise;
    param.iSpatialLayerNum = pEncParamExt->iSpatialLayerNum;
    param.bIsLosslessLink  = pEncParamExt->bIsLosslessLink;
    param.bEnableLongTermReference = pEncParamExt->bEnableLongTermReference;
    param.iEntropyCodingModeFlag   = pEncParamExt->iEntropyCodingModeFlag ? 1 : 0;
    if (eSliceMode != SM_SINGLE_SLICE && eSliceMode != SM_DYN_SLICE) //SM_DYN_SLICE don't support multi-thread now
      param.iMultipleThreadIdc = 2;

    for (int i = 0; i < param.iSpatialLayerNum; i++) {
      param.sSpatialLayers[i].iVideoWidth     = pEncParamExt->iPicWidth  >> (param.iSpatialLayerNum - 1 - i);
      param.sSpatialLayers[i].iVideoHeight    = pEncParamExt->iPicHeight >> (param.iSpatialLayerNum - 1 - i);
      param.sSpatialLayers[i].fFrameRate      = pEncParamExt->fMaxFrameRate;
      param.sSpatialLayers[i].iSpatialBitrate = param.iTargetBitrate;

      param.sSpatialLayers[i].sSliceCfg.uiSliceMode = eSliceMode;
      if (eSliceMode == SM_DYN_SLICE) {
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

  unsigned int uiTraceLevel = WELS_LOG_ERROR;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &uiTraceLevel);
}

void BaseEncoderTest::TearDown() {
  if (encoder_) {
    encoder_->Uninitialize();
    WelsDestroySVCEncoder (encoder_);
  }
}

void BaseEncoderTest::EncodeStream (InputStream* in, SEncParamExt* pEncParamExt, Callback* cbk) {

  ASSERT_TRUE (NULL != pEncParamExt);

  int rv = InitWithParam (encoder_, pEncParamExt);
  ASSERT_TRUE (rv == cmResultSuccess);

  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = pEncParamExt->iPicWidth * pEncParamExt->iPicHeight * 3 / 2;

  BufferedData buf;
  buf.SetLength (frameSize);
  ASSERT_TRUE (buf.Length() == (size_t)frameSize);

  SFrameBSInfo info;
  memset (&info, 0, sizeof (SFrameBSInfo));

  SSourcePicture pic;
  memset (&pic, 0, sizeof (SSourcePicture));
  pic.iPicWidth    = pEncParamExt->iPicWidth;
  pic.iPicHeight   = pEncParamExt->iPicHeight;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0]   = pic.iPicWidth;
  pic.iStride[1]   = pic.iStride[2] = pic.iPicWidth >> 1;
  pic.pData[0]     = buf.data();
  pic.pData[1]     = pic.pData[0] + pEncParamExt->iPicWidth * pEncParamExt->iPicHeight;
  pic.pData[2]     = pic.pData[1] + (pEncParamExt->iPicWidth * pEncParamExt->iPicHeight >> 2);
  while (in->read (buf.data(), frameSize) == frameSize) {
    rv = encoder_->EncodeFrame (&pic, &info);
    ASSERT_TRUE (rv == cmResultSuccess);
    if (info.eFrameType != videoFrameTypeSkip && cbk != NULL) {
      cbk->onEncodeFrame (info);
    }
  }
}

void BaseEncoderTest::EncodeFile (const char* fileName, SEncParamExt* pEncParamExt, Callback* cbk) {
  FileInputStream fileStream;
  ASSERT_TRUE (fileStream.Open (fileName));
  ASSERT_TRUE (NULL != pEncParamExt);
  EncodeStream (&fileStream, pEncParamExt, cbk);
}
