#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"
#include "BaseDecoderTest.h"
#include "BaseEncoderTest.h"
#include "wels_common_defs.h"
#include <string>
#include <vector>
#include "encode_decode_api_test.h"
using namespace WelsCommon;

void EncodeDecodeTestAPIBase::InitialEncDec (int iWidth, int iHeight) {
  // for encoder
  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = iWidth * iHeight * 3 / 2;

  buf_.SetLength (frameSize);
  ASSERT_TRUE (buf_.Length() == (size_t)frameSize);

  memset (&EncPic, 0, sizeof (SSourcePicture));
  EncPic.iPicWidth    = iWidth;
  EncPic.iPicHeight   =  iHeight;
  EncPic.iColorFormat = videoFormatI420;
  EncPic.iStride[0]   = EncPic.iPicWidth;
  EncPic.iStride[1]   = EncPic.iStride[2] = EncPic.iPicWidth >> 1;
  EncPic.pData[0]     = buf_.data();
  EncPic.pData[1]     = EncPic.pData[0] + iWidth * iHeight;
  EncPic.pData[2]     = EncPic.pData[1] + (iWidth * iHeight >> 2);

  //for decoder
  memset (&info, 0, sizeof (SFrameBSInfo));

  //set a fixed random value
  iRandValue = rand() % 256;
}

void EncodeDecodeTestAPIBase::RandomParamExtCombination() {

  param_.iPicWidth  = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 2, MAX_WIDTH);
  param_.iPicHeight = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1) + 1) << 1, 2, MAX_HEIGHT);

  param_.fMaxFrameRate      = rand() % FRAME_RATE_RANGE + 0.5f;
  param_.iUsageType         = static_cast<EUsageType> (rand() % 2);
  param_.iTemporalLayerNum  = rand() % TEMPORAL_LAYER_NUM_RANGE;
  param_.iSpatialLayerNum   = rand() % SPATIAL_LAYER_NUM_RANGE;

  param_.uiIntraPeriod      = rand() - 1;
  param_.iNumRefFrame       = AUTO_REF_PIC_COUNT;
  param_.iMultipleThreadIdc = rand();

  int iValue   = rand() % 7;
  switch (iValue) {
  case 0:
    param_.eSpsPpsIdStrategy  = CONSTANT_ID;
    break;
  case 0x01:
    param_.eSpsPpsIdStrategy  = INCREASING_ID;
    break;
  case 0x02:
    param_.eSpsPpsIdStrategy  = SPS_LISTING;
    break;
  case 0x03:
    param_.eSpsPpsIdStrategy  = SPS_LISTING_AND_PPS_INCREASING;
    break;
  case 0x06:
    param_.eSpsPpsIdStrategy  = SPS_PPS_LISTING;
    break;
  default:
    param_.eSpsPpsIdStrategy  = CONSTANT_ID;
    break;
  }
  param_.bPrefixNalAddingCtrl      = (rand() % 2 == 0) ? false : true;
  param_.bEnableSSEI               = (rand() % 2 == 0) ? false : true;
  param_.iPaddingFlag              = rand() % 2;

  //LTR
  param_.bEnableLongTermReference  = (rand() % 2 == 0) ? false : true;
  param_.bIsLosslessLink           = (rand() % 2 == 0) ? false : true;
  param_.iLTRRefNum                = rand();
  param_.iLtrMarkPeriod            = rand();

  //loop filter
  param_.iLoopFilterDisableIdc     = rand() % 7;
  param_.iLoopFilterAlphaC0Offset  = rand();
  param_.iLoopFilterBetaOffset     = rand();

  param_.bEnableDenoise             = (rand() % 2 == 0) ? false : true;
  param_.bEnableBackgroundDetection = (rand() % 2 == 0) ? false : true;
  param_.bEnableAdaptiveQuant       = (rand() % 2 == 0) ? false : true;
  param_.bEnableFrameCroppingFlag   = (rand() % 2 == 0) ? false : true;
  param_.bEnableSceneChangeDetect   = (rand() % 2 == 0) ? false : true;

  //for rc
  param_.iRCMode            = static_cast<RC_MODES> (rand() % RC_MODE_RANGE - 1);
  param_.iMaxBitrate        = rand() % BIT_RATE_RANGE;
  param_.iTargetBitrate     = rand() % BIT_RATE_RANGE;
  param_.iMaxQp             = rand() % QP_RANGE;
  param_.iMinQp             = rand() % QP_RANGE;
  param_.uiMaxNalSize       = rand();
  param_.bEnableFrameSkip   = (rand() % 2 == 0) ? false : true;

  for (int iSpatialIdx = 0; iSpatialIdx < param_.iSpatialLayerNum; iSpatialIdx++) {
    if (iSpatialIdx < MAX_SPATIAL_LAYER_NUM) {
      SSpatialLayerConfig* pSpatialLayer = &param_.sSpatialLayers[iSpatialIdx];

      //to do: profile and level id
      //pSpatialLayer->uiProfileIdc        = 0;
      //pSpatialLayer->uiLevelIdc          = 0;
      pSpatialLayer->iVideoWidth         = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 2, MAX_WIDTH);
      pSpatialLayer->iVideoHeight        = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1) + 1) << 1, 2, MAX_HEIGHT);
      pSpatialLayer->fFrameRate          = rand() % FRAME_RATE_RANGE + 0.5f;
      pSpatialLayer->iMaxSpatialBitrate  = rand() % BIT_RATE_RANGE;
      pSpatialLayer->iSpatialBitrate     = rand() % BIT_RATE_RANGE;


      pSpatialLayer->sSliceArgument.uiSliceMode = static_cast<SliceModeEnum> (rand() % SLICE_MODE_NUM);
      if (pSpatialLayer->sSliceArgument.uiSliceMode != SM_SIZELIMITED_SLICE) {
        param_.uiMaxNalSize       = 0;
      }
      pSpatialLayer->sSliceArgument.uiSliceNum = rand();
      pSpatialLayer->sSliceArgument.uiSliceSizeConstraint = rand();
    }
  }
}

void EncodeDecodeTestAPIBase::ValidateParamExtCombination() {

  bool bDynSliceModeFlag   = false;
  unsigned int uiGOPSize   = 0;
  unsigned int uiSliceNum  = 0;
  int iTotalBitRate        = 0;
  int iMinQP               = 0;

  param_.iPicWidth          = WELS_CLIP3 (param_.iPicWidth,  2, MAX_WIDTH);
  param_.iPicHeight         = WELS_CLIP3 (param_.iPicHeight, 2, MAX_HEIGHT);
  param_.fMaxFrameRate      = WELS_CLIP3 (param_.fMaxFrameRate, MIN_FRAME_RATE, MAX_FRAME_RATE);
  param_.iTemporalLayerNum  = WELS_CLIP3 (param_.iTemporalLayerNum, 1, MAX_TEMPORAL_LAYER_NUM);

  if (CAMERA_VIDEO_REAL_TIME == param_.iUsageType)
    param_.iSpatialLayerNum   = WELS_CLIP3 (param_.iSpatialLayerNum, 1, MAX_SPATIAL_LAYER_NUM);
  else
    param_.iSpatialLayerNum   = 1;

  //IntraPeriod
  uiGOPSize = 1 << (param_.iTemporalLayerNum - 1);
  param_.uiIntraPeriod -= param_.uiIntraPeriod % uiGOPSize;

  //RefNum
  int32_t iRefUpperBound    = (param_.iUsageType == CAMERA_VIDEO_REAL_TIME) ?
                              MAX_REFERENCE_PICTURE_COUNT_NUM_CAMERA : MAX_REFERENCE_PICTURE_COUNT_NUM_SCREEN;
  param_.iNumRefFrame       = WELS_CLIP3 (param_.iNumRefFrame, MIN_REF_PIC_COUNT, iRefUpperBound);

  //to do: will add more validate logic for thread number
  param_.iMultipleThreadIdc = 1;

  //LTR
  //iLTRRefNum: not supported to set it arbitrary yet
  if (true == param_.bEnableLongTermReference) {
    param_.iLTRRefNum     = (SCREEN_CONTENT_REAL_TIME == param_.iUsageType) ? LONG_TERM_REF_NUM_SCREEN : LONG_TERM_REF_NUM;
    param_.iLtrMarkPeriod = (0 == param_.iLtrMarkPeriod) ? 1 : param_.iLtrMarkPeriod;

  } else {
    param_.iLTRRefNum = 0;
  }

  //loop filter
  param_.iLoopFilterDisableIdc    = param_.iLoopFilterDisableIdc    % LOOP_FILTER_IDC_NUM;
  param_.iLoopFilterAlphaC0Offset = param_.iLoopFilterAlphaC0Offset % (2 * LOOF_FILTER_OFFSET_RANGE + 1) -
                                    LOOF_FILTER_OFFSET_RANGE;
  param_.iLoopFilterBetaOffset    = param_.iLoopFilterBetaOffset    % (2 * LOOF_FILTER_OFFSET_RANGE + 1) -
                                    LOOF_FILTER_OFFSET_RANGE;

  for (int iSpatialIdx = 0; iSpatialIdx < param_.iSpatialLayerNum; iSpatialIdx++) {
    SSpatialLayerConfig* pSpatialLayer = &param_.sSpatialLayers[iSpatialIdx];
    pSpatialLayer->iVideoWidth  = param_.iPicWidth >> (param_.iSpatialLayerNum - 1 - iSpatialIdx);
    pSpatialLayer->iVideoHeight = param_.iPicHeight >> (param_.iSpatialLayerNum - 1 - iSpatialIdx);
    pSpatialLayer->fFrameRate   = param_.fMaxFrameRate;

    pSpatialLayer->iMaxSpatialBitrate  = WELS_CLIP3 (pSpatialLayer->iMaxSpatialBitrate, 1, BIT_RATE_RANGE);
    pSpatialLayer->iSpatialBitrate     = WELS_CLIP3 (pSpatialLayer->iSpatialBitrate, 1, pSpatialLayer->iMaxSpatialBitrate);
    iTotalBitRate += pSpatialLayer->iSpatialBitrate;

    uiSliceNum  = pSpatialLayer->sSliceArgument.uiSliceNum;
    pSpatialLayer->sSliceArgument.uiSliceNum = WELS_CLIP3 (uiSliceNum, 1, MAX_SLICES_NUM);
    pSpatialLayer->sSliceArgument.uiSliceSizeConstraint = 0;


    //for SM_FIXEDSLCNUM_SLICE
    // to do will add this when GOM bug fixed
    if (SM_FIXEDSLCNUM_SLICE == pSpatialLayer->sSliceArgument.uiSliceMode) {
      pSpatialLayer->sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
    }

    //for slice mode = SM_SIZELIMITED_SLICE
    if (SM_SIZELIMITED_SLICE == pSpatialLayer->sSliceArgument.uiSliceMode) {
      bDynSliceModeFlag = true;
    }

    //for slice mode = SM_RASTER_SLICE
    if (SM_RASTER_SLICE == pSpatialLayer->sSliceArgument.uiSliceMode) {
      if (0 != pSpatialLayer->sSliceArgument.uiSliceMbNum[0]) {
        SliceParamValidationForMode2 (iSpatialIdx);
      } else {
        SliceParamValidationForMode3 (iSpatialIdx);
      }
    }
  }

  //for RC
  if ((RC_QUALITY_MODE == param_.iRCMode) || (RC_BITRATE_MODE == param_.iRCMode)) {
    param_.bEnableFrameSkip = true;
  }
  if (param_.iTargetBitrate < iTotalBitRate) {
    param_.iTargetBitrate = iTotalBitRate;
  }
  if (param_.iMaxBitrate < param_.iTargetBitrate) {
    param_.iMaxBitrate = param_.iTargetBitrate;
  }
  param_.iMaxQp       = WELS_CLIP3 (param_.iMaxQp, MIN_QP, MAX_QP);
  param_.iMinQp       = WELS_CLIP3 (param_.iMinQp, MIN_QP, MAX_QP);
  iMinQP              = (param_.iMaxQp < param_.iMinQp) ? param_.iMaxQp : param_.iMinQp;
  param_.iMaxQp       = (param_.iMaxQp > param_.iMinQp) ? param_.iMaxQp : param_.iMinQp;
  param_.iMinQp       = iMinQP;
  param_.uiMaxNalSize = 0;

  //for slice mode = SM_SIZELIMITED_SLICE
  if (true == bDynSliceModeFlag) {
    SliceParamValidationForMode4();
  }

}


void EncodeDecodeTestAPIBase::SliceParamValidationForMode2 (int iSpatialIdx) {

  unsigned int uiMbWidth          = 0;
  unsigned int uiMbHeight         = 0;
  unsigned int uiMbNumInFrame     = 0;
  unsigned int uiCountMb          = 0;
  unsigned int uiSliceIdx         = 0;
  unsigned int uiActualSliceCount = 0;

  uiMbWidth      = (param_.iPicWidth  + 15) >> 4;
  uiMbHeight     = (param_.iPicHeight + 15) >> 4;
  uiMbNumInFrame = uiMbWidth * uiMbHeight;

  uiSliceIdx = 0;
  while (uiSliceIdx < MAX_SLICES_NUM) {
    param_.sSpatialLayers[iSpatialIdx].sSliceArgument.uiSliceMbNum[uiSliceIdx] = rand() % uiMbNumInFrame;
    uiCountMb           += param_.sSpatialLayers[iSpatialIdx].sSliceArgument.uiSliceMbNum[uiSliceIdx];
    uiActualSliceCount   =  uiSliceIdx + 1;

    if (uiCountMb >= uiMbNumInFrame) {
      break;
    }

    ++ uiSliceIdx;
  }

  if (uiCountMb >= uiMbNumInFrame) {
    param_.sSpatialLayers[iSpatialIdx].sSliceArgument.uiSliceMbNum[uiActualSliceCount - 1] -=
      (uiCountMb - uiMbNumInFrame);

  } else {
    param_.sSpatialLayers[iSpatialIdx].sSliceArgument.uiSliceMbNum[uiActualSliceCount - 1 ] +=
      (uiMbNumInFrame - uiCountMb);
  }
  param_.sSpatialLayers[iSpatialIdx].sSliceArgument.uiSliceNum = uiActualSliceCount;

}
void EncodeDecodeTestAPIBase::SliceParamValidationForMode3 (int iSpatialIdx) {

  unsigned int uiMbHeight         = 0;

  uiMbHeight = (param_.iPicHeight + 15) >> 4;

  //change slice mode to SM_SINGLE_SLICE
  if (uiMbHeight >  MAX_SLICES_NUM) {
    param_.sSpatialLayers[iSpatialIdx].sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
  }

}

void EncodeDecodeTestAPIBase::SliceParamValidationForMode4() {
  //slice mode of all spatial layer should be set as SM_SIZELIMITED_SLICE
  for (int iSpatialIdx = 0; iSpatialIdx < param_.iSpatialLayerNum; iSpatialIdx++) {
    param_.sSpatialLayers[iSpatialIdx].sSliceArgument.uiSliceSizeConstraint = 600;
    param_.sSpatialLayers[iSpatialIdx].sSliceArgument.uiSliceMode = SM_SIZELIMITED_SLICE;
  }
  param_.uiMaxNalSize = 1500;
}

TEST_F (EncodeDecodeTestAPI, SetOptionEncParamExt) {
  int iSpatialLayerNum = 4;
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_HEIGHT);
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);
  int iSliceNum        = 1;
  encoder_->GetDefaultParams (&param_);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &param_);

  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  int iTraceLevel = WELS_LOG_QUIET;
  rv = encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  ASSERT_TRUE (rv == cmResultSuccess);

  for (int i = 0; i < iEncFrameNum; i++) {
    int iResult;
    int len = 0;
    unsigned char* pData[3] = { NULL };

    RandomParamExtCombination();
    iResult = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &param_);
    //ValidateParamExtCombination();
    //ASSERT_TRUE (iResult == cmResultSuccess);
    //to do
    // currently, there are still some error cases even though under condition cmResultSuccess == iResult
    // so need to enhance the validation check for any random value of each variable in ParamExt

    if (cmResultSuccess == iResult) {
      InitialEncDec (param_.iPicWidth, param_.iPicHeight);
      EncodeOneFrame (0);
      encToDecData (info, len);
      pData[0] = pData[1] = pData[2] = 0;
      memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

      iResult = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
      ASSERT_TRUE (iResult == cmResultSuccess);
      iResult = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
      ASSERT_TRUE (iResult == cmResultSuccess);
      EXPECT_EQ (dstBufInfo_.iBufferStatus, 1);
    }
  }

  iTraceLevel = WELS_LOG_ERROR;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
}

struct EncodeDecodeParamBase {
  int width;
  int height;
  float frameRate;
  int iTarBitrate;
};

//#define DEBUG_FILE_SAVE_INCREASING_ID
TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_INCREASING_ID) {

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  SEncParamExt   sParam3;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.bSimulcastAVC = 1;
  sParam1.eSpsPpsIdStrategy = INCREASING_ID;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam2.iPicWidth) == GET_MB_WIDTH (sParam1.iPicWidth)) {
    sParam2.iPicWidth = GetRandWidth();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
  sParam2.bSimulcastAVC = 1;
  sParam2.eSpsPpsIdStrategy = INCREASING_ID;
  //prepare param3
  memcpy (&sParam3, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam3.iPicHeight) == GET_MB_WIDTH (sParam1.iPicHeight)) {
    sParam3.iPicHeight = GetRandHeight();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam3.iPicWidth, sParam3.iPicHeight, fFrameRate, &sParam3);
  sParam3.bSimulcastAVC = 1;
  sParam3.eSpsPpsIdStrategy = INCREASING_ID;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE_INCREASING_ID
  fEnc = fopen ("enc_INCREASING_ID.264", "wb");
#endif

  // Test part#1
  // step#1: pParam1
  //int TraceLevel = WELS_LOG_INFO;
  //encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &TraceLevel);
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // step#2: pParam2
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption: rv = " << rv << " at " << sParam2.iPicWidth << "x" <<
                                      sParam2.iPicHeight;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // step#3: back to pParam1
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // step#4: back to pParam2
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv << sParam2.iPicWidth << sParam2.iPicHeight;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

#ifdef DEBUG_FILE_SAVE_INCREASING_ID
  fclose (fEnc);
#endif
  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

  // Test part#2
  // step#1: pParam1
  rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt Failed: rv = " << rv;
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam3);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam3: rv = " << rv;

#ifdef DEBUG_FILE_SAVE_INCREASING_ID
  fEnc = fopen ("enc_INCREASING_ID2.264", "wb");
#endif
  iEncFrameNum = 0;
  EncDecOneFrame (sParam3.iPicWidth, sParam3.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

#ifdef DEBUG_FILE_SAVE_INCREASING_ID
  fclose (fEnc);
#endif
  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
}

//#define DEBUG_FILE_SAVE2
TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_LISTING_AND_PPS_INCREASING1) {

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  SEncParamExt   sParam3;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam2.iPicWidth) == GET_MB_WIDTH (sParam1.iPicWidth)) {
    sParam2.iPicWidth = GetRandWidth();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
  sParam2.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;
  //prepare param3
  memcpy (&sParam3, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam3.iPicHeight) == GET_MB_WIDTH (sParam1.iPicHeight)) {
    sParam3.iPicHeight = GetRandHeight();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam3.iPicWidth, sParam3.iPicHeight, fFrameRate, &sParam3);
  sParam3.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE2
  fEnc = fopen ("enc_SPS_LISTING_AND_PPS_INCREASING1.264", "wb");
#endif

  // Test part#1
  // step#1: pParam1
  //int TraceLevel = WELS_LOG_INFO;
  //encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &TraceLevel);
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // step#2: pParam2
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption: rv = " << rv << " at " << sParam2.iPicWidth << "x" <<
                                      sParam2.iPicHeight;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // step#3: back to pParam1
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // step#4: back to pParam2
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv << sParam2.iPicWidth << sParam2.iPicHeight;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

#ifdef DEBUG_FILE_SAVE2
  fclose (fEnc);
#endif
  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

  // Test part#2
  // step#1: pParam1
  rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt Failed: rv = " << rv;
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam3);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam3: rv = " << rv;

#ifdef DEBUG_FILE_SAVE2
  fEnc = fopen ("enc_SPS_LISTING_AND_PPS_INCREASING11.264", "wb");
#endif
  iEncFrameNum = 0;
  EncDecOneFrame (sParam3.iPicWidth, sParam3.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

#ifdef DEBUG_FILE_SAVE2
  fclose (fEnc);
#endif
  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
}

//#define DEBUG_FILE_SAVE5
TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_LISTING_AND_PPS_INCREASING2) {
  //usage 3: 2 Params with different num_ref, encode IDR0, P1, IDR2;
  //the bs will show two SPS and different PPS

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;
  sParam1.iTemporalLayerNum = 1;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
  sParam2.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;
  sParam2.iTemporalLayerNum = 3;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE5
  fEnc = fopen ("enc_SPS_LISTING_AND_PPS_INCREASING2.264", "wb");
#endif

  // step#1: pParam1
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;

  // step#2: pParam2
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;

  // step#3: set back to pParam1, with a smaller num_ref, it still uses the previous SPS
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam1: rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR, PPS increases
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

#ifdef DEBUG_FILE_SAVE5
  fclose (fEnc);
#endif
}

TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_LISTING_AND_PPS_INCREASING3) {

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE2
  fEnc = fopen ("enc_SPS_LISTING_AND_PPS_INCREASING3.264", "wb");
#endif

  // step#1: pParam1
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt Failed: rv = " << rv;

  int max_count = 65; // make it more then twice as MAX_SPS_COUNT
  std::vector<int> vWidthTable;
  vWidthTable.push_back (GET_MB_WIDTH (sParam1.iPicWidth));

  std::vector<int>::iterator vWidthTableIt;
  for (int times = 0; times < max_count; times++) {
    //prepare param2
    memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
    do {
      sParam2.iPicWidth = GetRandWidth();
      vWidthTableIt = std::find (vWidthTable.begin(), vWidthTable.end(), GET_MB_WIDTH (sParam2.iPicWidth));
    } while (vWidthTableIt != vWidthTable.end());
    vWidthTable.push_back (GET_MB_WIDTH (sParam2.iPicWidth));
    prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
    sParam2.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;

    rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
    ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv << ", sParam2.iPicWidth=" <<
                                        sParam2.iPicWidth;
  } // end of setting loop

  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

#ifdef DEBUG_FILE_SAVE2
  fclose (fEnc);
#endif
  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
}

//#define DEBUG_FILE_SAVE6
TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_PPS_LISTING1) {
  //usage 1: 1 resolution Params, encode IDR0, P1, IDR2;
  //the bs will show same SPS and different PPS
  // PPS: pic_parameter_set_id                                     1 (  0)
  // PPS: seq_parameter_set_id                                     1 (  0)
  // PPS: pic_parameter_set_id                                   010 (  1)
  // PPS: seq_parameter_set_id                                     1 (  0)
  // SH: slice_type                                              011 (  2)
  // SH: pic_parameter_set_id                                      1 (  0)
  // SH: slice_type                                                1 (  0)
  // SH: pic_parameter_set_id                                      1 (  0)
  // SH: slice_type                                              011 (  2)
  // SH: pic_parameter_set_id                                    010 (  1)
  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_PPS_LISTING;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE6
  fEnc = fopen ("encLIST1.264", "wb");
#endif

  // step#1: pParam1
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

#ifdef DEBUG_FILE_SAVE6
  fclose (fEnc);
#endif
}

TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_PPS_LISTING2) {
  //usage 2: 2 resolution Params, encode IDR0, IDR1, IDR2;
  //the bs will show two SPS and different PPS
  // === SPS LIST ===
  //SPS: seq_parameter_set_id                                     1 (  0) -- PARAM1
  //SPS: seq_parameter_set_id                                   010 (  1) -- PARAM2
  // === PPS LIST ===
  //PPS: pic_parameter_set_id                                     1 (  0)
  //PPS: seq_parameter_set_id                                     1 (  0)
  //PPS: pic_parameter_set_id                                   010 (  1)
  //PPS: seq_parameter_set_id                                   010 (  1)
  //PPS: pic_parameter_set_id                                   011 (  2) -- PPS2 - SPS0
  //PPS: seq_parameter_set_id                                     1 (  0)
  //PPS: pic_parameter_set_id                                 00100 (  3) -- PPS3 - SPS1
  //PPS: seq_parameter_set_id                                   010 (  1)
  //PPS: pic_parameter_set_id                                 00101 (  4) -- PPS4 - SPS0
  //PPS: seq_parameter_set_id                                     1 (  0)
  // === VCL LAYER ===
  //SH: slice_type                                              011 (  2) -- PARAM2
  //SH: pic_parameter_set_id                                    010 (  1) -- PPS1 - SPS1 - PARAM2
  //SH: slice_type                                              011 (  2) -- PARAM1
  //SH: pic_parameter_set_id                                    011 (  2) -- PPS2 - SPS0 - PARAM1
  //SH: slice_type                                             011 (  2) -- PARAM1
  //SH: pic_parameter_set_id                                 00101 (  4) -- PPS4 - SPS0 - PARAM1

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_PPS_LISTING;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam2.iPicWidth) == GET_MB_WIDTH (sParam1.iPicWidth)) {
    sParam2.iPicWidth = GetRandWidth();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
  sParam2.eSpsPpsIdStrategy = SPS_PPS_LISTING;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE5
  fEnc = fopen ("encLIST2.264", "wb");
#endif

  // step#1: pParam1
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;

  // step#2: pParam2
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // step#3: back to pParam1, SHOULD NOT encounter ERROR
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

#ifdef DEBUG_FILE_SAVE5
  fclose (fEnc);
#endif
}

TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_PPS_LISTING3) {

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  SEncParamExt   sParam3;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_PPS_LISTING;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam2.iPicWidth) == GET_MB_WIDTH (sParam1.iPicWidth)) {
    sParam2.iPicWidth = GetRandWidth();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
  sParam2.eSpsPpsIdStrategy = SPS_PPS_LISTING;
  //prepare param3
  memcpy (&sParam3, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam3.iPicWidth) == GET_MB_WIDTH (sParam1.iPicWidth) ||
         GET_MB_WIDTH (sParam3.iPicWidth) == GET_MB_WIDTH (sParam2.iPicWidth)) {
    sParam3.iPicWidth = GetRandWidth();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam3.iPicWidth, sParam3.iPicHeight, fFrameRate, &sParam3);
  sParam3.eSpsPpsIdStrategy = SPS_PPS_LISTING;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE5
  fEnc = fopen ("enc_LISTING3.264", "wb");
#endif

  // step#1: ordinary encoding
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // step#2: set strategy for success
  int32_t iNewStra = SPS_PPS_LISTING;
  rv = encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iNewStra);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv << " iNewStra=" << iNewStra;

  // step#3: setting new strategy, SHOULD encounter ERROR
  unsigned int TraceLevel = WELS_LOG_QUIET;
  rv = encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &TraceLevel);
  ASSERT_TRUE (rv == cmResultSuccess);
  iNewStra = CONSTANT_ID;
  rv = encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iNewStra);
  ASSERT_TRUE (rv != cmResultSuccess);

  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // step#4: pParam3, SHOULD encounter ERROR
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam3);
  ASSERT_TRUE (rv != cmResultSuccess) << "SetOption: rv = " << rv << " at " << sParam3.iPicWidth << "x" <<
                                      sParam3.iPicHeight;

  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

#ifdef DEBUG_FILE_SAVE5
  fclose (fEnc);
#endif
}


TEST_F (EncodeDecodeTestAPI, SimulcastSVC) {
  int iSpatialLayerNum = WelsClip3 ((rand() % MAX_SPATIAL_LAYER_NUM), 2, MAX_SPATIAL_LAYER_NUM);
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 16, MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, 16, MAX_HEIGHT);
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);
  int iSliceNum        = 1;
  encoder_->GetDefaultParams (&param_);
  prepareParam (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &param_);

  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  int aLen[MAX_SPATIAL_LAYER_NUM] = {0};
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

#ifdef DEBUG_FILE_SAVE2
  FILE* fEnc[MAX_SPATIAL_LAYER_NUM] = { NULL };
  fEnc[0] = fopen ("enc0.264", "wb");
  fEnc[1] = fopen ("enc1.264", "wb");
  fEnc[2] = fopen ("enc2.264", "wb");
  fEnc[3] = fopen ("enc3.264", "wb");
#endif

  // init decoders
  int iIdx = 0;
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);
    aLen[iIdx] = 0;

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

  for (int iFrame = 0; iFrame < iEncFrameNum; iFrame++) {
    int iResult;
    int iLayerLen = 0;
    unsigned char* pData[3] = { NULL };

    InitialEncDec (param_.iPicWidth, param_.iPicHeight);
    EncodeOneFrame (0);

    iLayerLen = 0;
    for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
      aLen[iIdx] = 0;
    }
    for (int iLayer = 0; iLayer < info.iLayerNum; ++iLayer) {
      iLayerLen = 0;
      const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayer];
      for (int iNal = 0; iNal < layerInfo.iNalCount; ++iNal) {
        iLayerLen += layerInfo.pNalLengthInByte[iNal];
      }

      if (layerInfo.uiLayerType == NON_VIDEO_CODING_LAYER) {
        // under SimulcastSVC, need to copy non-VCL to all layers
        for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
          memcpy ((pBsBuf[iIdx] + aLen[iIdx]), layerInfo.pBsBuf, iLayerLen * sizeof (unsigned char));
          aLen[iIdx] += iLayerLen;
        }
      } else {
        iIdx = layerInfo.uiSpatialId;
        EXPECT_TRUE (iIdx < iSpatialLayerNum);
        memcpy ((pBsBuf[iIdx] + aLen[iIdx]), layerInfo.pBsBuf, iLayerLen * sizeof (unsigned char));
        aLen[iIdx] += iLayerLen;
      }
    }

    for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
      pData[0] = pData[1] = pData[2] = 0;
      memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

#ifdef DEBUG_FILE_SAVE2
      fwrite (pBsBuf[iIdx], aLen[iIdx], 1, fEnc[iIdx]);
#endif
      iResult = decoder[iIdx]->DecodeFrame2 (pBsBuf[iIdx], aLen[iIdx], pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;
      iResult = decoder[iIdx]->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;
      EXPECT_EQ (dstBufInfo_.iBufferStatus, 1) << "LayerIdx=" << iIdx;
    }
  }

  // free all
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    if (pBsBuf[iIdx]) {
      free (pBsBuf[iIdx]);
    }

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }

#ifdef DEBUG_FILE_SAVE2
    fclose (fEnc[iIdx]);
#endif
  }

}

TEST_F (EncodeDecodeTestAPI, SimulcastAVC) {
//#define DEBUG_FILE_SAVE3
  int iSpatialLayerNum = WelsClip3 ((rand() % MAX_SPATIAL_LAYER_NUM), 2, MAX_SPATIAL_LAYER_NUM);
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 16, MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, 16, MAX_HEIGHT);
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);
  int iSliceNum        = 1;
  encoder_->GetDefaultParams (&param_);
  prepareParam (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &param_);

  //set flag of bSimulcastAVC
  param_.bSimulcastAVC = true;

  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  int aLen[MAX_SPATIAL_LAYER_NUM] = {0};
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

#ifdef DEBUG_FILE_SAVE3
  FILE* fEnc[MAX_SPATIAL_LAYER_NUM];
  fEnc[0] = fopen ("enc0.264", "wb");
  fEnc[1] = fopen ("enc1.264", "wb");
  fEnc[2] = fopen ("enc2.264", "wb");
  fEnc[3] = fopen ("enc3.264", "wb");
#endif

  int iIdx = 0;

  //create decoder
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);
    aLen[iIdx] = 0;

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

  iEncFrameNum = 10;
  for (int iFrame = 0; iFrame < iEncFrameNum; iFrame++) {
    int iResult;
    int iLayerLen = 0;
    unsigned char* pData[3] = { NULL };

    InitialEncDec (param_.iPicWidth, param_.iPicHeight);
    EncodeOneFrame (0);

    // init
    for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
      aLen[iIdx] = 0;
    }
    for (int iLayer = 0; iLayer < info.iLayerNum; ++iLayer) {
      iLayerLen = 0;
      const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayer];
      const int kiFirstNalType = ((* (layerInfo.pBsBuf + 4)) & 0x1f);
      ASSERT_TRUE ((kiFirstNalType == NAL_SPS) || (kiFirstNalType == NAL_PPS) || (kiFirstNalType == NAL_SLICE)
                   || (kiFirstNalType == NAL_SLICE_IDR) || (kiFirstNalType == NAL_SEI));
      for (int iNal = 0; iNal < layerInfo.iNalCount; ++iNal) {
        iLayerLen += layerInfo.pNalLengthInByte[iNal];
      }

      iIdx = layerInfo.uiSpatialId;
      EXPECT_TRUE (iIdx < iSpatialLayerNum);
      memcpy ((pBsBuf[iIdx] + aLen[iIdx]), layerInfo.pBsBuf, iLayerLen * sizeof (unsigned char));
      aLen[iIdx] += iLayerLen;
    }

    for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
      pData[0] = pData[1] = pData[2] = 0;
      memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

#ifdef DEBUG_FILE_SAVE3
      fwrite (pBsBuf[iIdx], aLen[iIdx], 1, fEnc[iIdx]);
#endif
      iResult = decoder[iIdx]->DecodeFrame2 (pBsBuf[iIdx], aLen[iIdx], pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;

      iResult = decoder[iIdx]->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;
      EXPECT_EQ (dstBufInfo_.iBufferStatus, 1) << "LayerIdx=" << iIdx;
    }
  }

  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    free (pBsBuf[iIdx]);

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }
  }
#ifdef DEBUG_FILE_SAVE_SimulcastAVCDiffFps
  for (int i = 0; i <)MAX_SPATIAL_LAYER_NUM;
  i++) {
    fclose (fEnc[i]);
  }
#endif
}

TEST_F (EncodeDecodeTestAPI, SimulcastAVC_SPS_PPS_LISTING) {
  int iSpatialLayerNum = WelsClip3 ((rand() % MAX_SPATIAL_LAYER_NUM), 2, MAX_SPATIAL_LAYER_NUM);;
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_HEIGHT);
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);
  int iSliceNum        = 1;
  iWidth = VALID_SIZE (iWidth);
  iHeight = VALID_SIZE (iHeight);
  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  //set flag of SPS_PPS_LISTING
  sParam1.eSpsPpsIdStrategy = SPS_PPS_LISTING;//SPS_LISTING;//
  //set flag of bSimulcastAVC
  sParam1.bSimulcastAVC = true;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  sParam2.sSpatialLayers[0].iVideoWidth = (sParam1.sSpatialLayers[0].iVideoWidth / 2);
  sParam2.sSpatialLayers[0].iVideoHeight = (sParam1.sSpatialLayers[0].iVideoHeight / 2);

  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "Init Failed sParam1: rv = " << rv;;
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

  int iIdx = 0;

  //create decoder
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

  TestOneSimulcastAVC (&sParam1, decoder, pBsBuf, iSpatialLayerNum, iEncFrameNum, 0);
  TestOneSimulcastAVC (&sParam2, decoder, pBsBuf, iSpatialLayerNum, iEncFrameNum, 0);

  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    free (pBsBuf[iIdx]);

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }

  }
}

struct EncodeOptionParam {
  bool bTestNalSize;
  bool bAllRandom;
  bool bTestDecoder;
  int iNumframes;
  int iWidth;
  int iHeight;
  int iQp;
  SliceModeEnum eSliceMode;
  int uiMaxNalLen;
  float fFramerate;
  int iThreads;
  const char* sFileSave;
};

static const EncodeOptionParam kOptionParamArray[] = {
  {true, true, false, 30, 600, 460, 1, SM_SIZELIMITED_SLICE, 450, 15.0, 1, ""},
  {true, true, false, 30, 340, 96, 24, SM_SIZELIMITED_SLICE, 1000, 30.0, 1, ""},
  {true, true, false, 30, 140, 196, 51, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""},
  {true, true, false, 30, 110, 296, 50, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""},
  {true, true, false, 30, 104, 416, 44, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""},
  {true, true, false, 30, 16, 16, 2, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""}, //5
  {true, false, true, 30, 600, 460, 1, SM_SIZELIMITED_SLICE, 450, 15.0, 1, ""},
  {true, false, true, 30, 340, 96, 24, SM_SIZELIMITED_SLICE, 1000, 30.0, 1, ""},
  {true, false, true, 30, 140, 196, 51, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""},
  {true, false, true, 30, 110, 296, 50, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""},
  {true, false, true, 30, 104, 416, 44, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""}, //10
  {true, false, true, 30, 16, 16, 2, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""},
  {true, true, true, 30, 600, 460, 1, SM_SIZELIMITED_SLICE, 450, 15.0, 1, ""},
  {true, true, true, 30, 340, 96, 24, SM_SIZELIMITED_SLICE, 1000, 30.0, 1, ""},
  {true, true, true, 30, 140, 196, 51, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""},
  {true, true, true, 30, 110, 296, 50, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""}, //15
  {true, true, true, 30, 104, 416, 44, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""},
  {true, true, true, 30, 16, 16, 2, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""},
  {false, true, false, 30, 32, 16, 2, SM_SIZELIMITED_SLICE, 500, 7.5, 1, ""},
  {false, true, false, 30, 600, 460, 1, SM_SIZELIMITED_SLICE, 450, 15.0, 4, ""},
  {false, true, false, 30, 340, 96, 24, SM_SIZELIMITED_SLICE, 1000, 30.0, 2, ""}, //20
  {false, true, false, 30, 140, 196, 51, SM_SIZELIMITED_SLICE, 500, 7.5, 3, ""},
  {false, true, false, 30, 110, 296, 50, SM_SIZELIMITED_SLICE, 500, 7.5, 2, ""},
  {false, true, false, 30, 104, 416, 44, SM_SIZELIMITED_SLICE, 500, 7.5, 2, ""},
  {false, true, false, 30, 16, 16, 2, SM_SIZELIMITED_SLICE, 500, 7.5, 3, ""},
  {false, true, false, 30, 32, 16, 2, SM_SIZELIMITED_SLICE, 500, 7.5, 3, ""}, //25
  {false, false, true, 30, 600, 460, 1, SM_FIXEDSLCNUM_SLICE, 0, 15.0, 4, ""},
  {false, false, true, 30, 600, 460, 1, SM_FIXEDSLCNUM_SLICE, 0, 15.0, 8, ""},
  //for large size tests
  {true, false, true, 2, 4096, 2304, 1, SM_RESERVED, 0, 7.5, 1, ""}, // large picture size, //28
  {true, false, true, 2, 2304, 4096, 1, SM_RESERVED, 0, 15.0, 1, ""}, //29
  {true, false, true, 2, 3072, 3072, 1, SM_RESERVED, 0, 15.0, 1, ""}, //30
  //{true, false, true, 2, 3072, 3072, 1, SM_RESERVED, 0, 15.0, 4, ""}, //14760
  {false, false, true, 2, 1072, 8576, 1, SM_RESERVED, 0, 15.0, 1, ""},
  {false, false, true, 2, 8576, 1072, 24, SM_SINGLE_SLICE, 0, 15.0, 1, ""}, //14754
  //{false, false, false, 2, 8576, 1088, 24, SM_SINGLE_SLICE, 0, 15.0, 1, ""}, //14755
  //{false, false, false, 2, 1088, 8576, 24, SM_SINGLE_SLICE, 0, 15.0, 1, ""}, //14755
  {false, false, true, 2, 8688, 1072, 24, SM_SINGLE_SLICE, 0, 15.0, 1, ""}, //Annex A: PicWidthInMbs <= sqrt(36864*8) = 543; 543*16=8688; 36864/543=67; 67*16=1072
  {false, false, true, 2, 1072, 8688, 24, SM_SINGLE_SLICE, 0, 15.0, 1, ""}, //Annex A: FrameHeightInMbs <= sqrt(36864*8) = 543; 543*16=8688; 36864/543=67; 67*16=1072
  //{false, false, true, 2, 589824, 16, 24, SM_RESERVED, 0, 15.0, 1, ""},
  //{false, false, true, 2, 589824, 16, 24, SM_RESERVED, 0, 15.0, 1, ""},
};

class EncodeTestAPI : public ::testing::TestWithParam<EncodeOptionParam>, public ::EncodeDecodeTestAPIBase {
 public:
  void SetUp() {
    EncodeDecodeTestAPIBase::SetUp();
  }

  void TearDown() {
    EncodeDecodeTestAPIBase::TearDown();
  }
  void EncodeOneFrameRandom (int iCheckTypeIndex, bool bAllRandom) {
    int frameSize = EncPic.iPicWidth * EncPic.iPicHeight * 3 / 2;
    uint8_t* ptr = buf_.data();
    uint8_t uiVal = rand() % 256;
    for (int i = 0; i < frameSize; i++) {
      ptr[i] = bAllRandom ? (rand() % 256) : uiVal;
    }
    int rv = encoder_->EncodeFrame (&EncPic, &info);
    if (0 == iCheckTypeIndex)
      ASSERT_TRUE (rv == cmResultSuccess) << "rv=" << rv;
    else if (1 == iCheckTypeIndex)
      ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnknownReason) << "rv=" << rv;
  }
};

INSTANTIATE_TEST_CASE_P (EncodeDecodeTestAPIBase, EncodeTestAPI,
                         ::testing::ValuesIn (kOptionParamArray));

TEST_P (EncodeTestAPI, SetEncOptionSize) {
  EncodeOptionParam p = GetParam();
  memset (&param_, 0, sizeof (SEncParamExt));
  encoder_->GetDefaultParams (&param_);
  param_.uiMaxNalSize = p.uiMaxNalLen;
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 1;
  param_.iUsageType = CAMERA_VIDEO_REAL_TIME;
  param_.iPicWidth = p.iWidth;
  param_.iPicHeight = p.iHeight;
  param_.fMaxFrameRate = p.fFramerate;
  param_.iRCMode = RC_OFF_MODE; //rc off
  param_.iMultipleThreadIdc = p.iThreads;
  param_.iNumRefFrame = AUTO_REF_PIC_COUNT;
  param_.sSpatialLayers[0].iVideoWidth = p.iWidth;
  param_.sSpatialLayers[0].iVideoHeight = p.iHeight;
  param_.sSpatialLayers[0].fFrameRate = p.fFramerate;

  int iSliceModeTestNum = 1;
  if (SM_RESERVED == p.eSliceMode) {
    iSliceModeTestNum = SLICE_MODE_NUM;
  }

  for (int iSliceIdx = 0; iSliceIdx < iSliceModeTestNum; iSliceIdx++) {
    if (1 == iSliceModeTestNum) {
      param_.sSpatialLayers[0].sSliceArgument.uiSliceMode = p.eSliceMode;
    } else {
      param_.sSpatialLayers[0].sSliceArgument.uiSliceMode = static_cast<SliceModeEnum> (iSliceIdx);
    }

    FILE* pFile = NULL;
    if (p.sFileSave != NULL && strlen (p.sFileSave) > 0) {
      pFile = fopen (p.sFileSave, "wb");
    }

    if (SM_FIXEDSLCNUM_SLICE == param_.sSpatialLayers[0].sSliceArgument.uiSliceMode) {
      param_.sSpatialLayers[0].sSliceArgument.uiSliceNum = 8;
    } else if (SM_RASTER_SLICE == param_.sSpatialLayers[0].sSliceArgument.uiSliceMode) {
      param_.sSpatialLayers[0].sSliceArgument.uiSliceMbNum[0] =
        param_.sSpatialLayers[0].sSliceArgument.uiSliceMbNum[1] =
          param_.sSpatialLayers[0].sSliceArgument.uiSliceMbNum[2] =
            param_.sSpatialLayers[0].sSliceArgument.uiSliceMbNum[3] = ((p.iWidth * p.iHeight) >> 10);
    } else if (SM_SIZELIMITED_SLICE == param_.sSpatialLayers[0].sSliceArgument.uiSliceMode && 450 > p.uiMaxNalLen) {
      param_.uiMaxNalSize = 450;
      param_.sSpatialLayers[0].sSliceArgument.uiSliceSizeConstraint = 450;
    }

    int32_t iTraceLevel = WELS_LOG_QUIET;
    encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

    encoder_->Uninitialize();
    int rv = encoder_->InitializeExt (&param_);
    ASSERT_TRUE (rv == cmResultSuccess);
    InitialEncDec (p.iWidth, p.iHeight);


    int32_t iSpsPpsIdAddition = 1;
    encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
    int32_t iIDRPeriod = (int32_t) pow (2.0f, (param_.iTemporalLayerNum - 1)) * ((rand() % 5) + 1);
    encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
    int iIdx = 0;
    int iLen;
    unsigned char* pData[3] = { NULL };

    //FIXME: remove this after the multi-thread case is correctly handled in encoder
    if (p.iThreads > 1 && SM_SIZELIMITED_SLICE == p.eSliceMode) {
      p.bAllRandom = false;
    }

    while (iIdx <= p.iNumframes) {
      EncodeOneFrameRandom (0, p.bAllRandom);
      encToDecData (info, iLen);
      if (pFile) {
        fwrite (info.sLayerInfo[0].pBsBuf, iLen, 1, pFile);
        fflush (pFile);
      }
      memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
      if (iLen && p.bTestDecoder) {
        rv = decoder_->DecodeFrameNoDelay (info.sLayerInfo[0].pBsBuf, iLen, pData, &dstBufInfo_);
        ASSERT_EQ (rv, 0);
        ASSERT_EQ (dstBufInfo_.iBufferStatus, 1);
      }
      int iLayer = 0;
      while (iLayer < info.iLayerNum) {
        SLayerBSInfo* pLayerBsInfo = &info.sLayerInfo[iLayer];
        if (pLayerBsInfo != NULL) {
          int iNalIdx = WELS_MAX (pLayerBsInfo->iNalCount - 2, 0); // ignore last slice under single slice mode
          do {
            if (SM_SIZELIMITED_SLICE == p.eSliceMode
                && p.bTestNalSize) { // ignore the case that 2 MBs in one picture, and the multithreads case, enable them when code is ready
              ASSERT_GE (((int)param_.uiMaxNalSize), pLayerBsInfo->pNalLengthInByte[iNalIdx]);
            }
            -- iNalIdx;
          } while (iNalIdx >= 0);
        }
        ++ iLayer;
      }
      iIdx++;
    }
    if (pFile) {
      fclose (pFile);
    }
  }
}



TEST_F (EncodeDecodeTestAPI, SimulcastAVCDiffFps) {
//#define DEBUG_FILE_SAVE_SimulcastAVCDiffFps
  int iSpatialLayerNum = WelsClip3 ((rand() % MAX_SPATIAL_LAYER_NUM), 2, MAX_SPATIAL_LAYER_NUM);
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_HEIGHT);
  iWidth = VALID_SIZE (iWidth);
  iHeight = VALID_SIZE (iHeight);

  float fFrameRate = 30;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);
  int iSliceNum        = 1;
  encoder_->GetDefaultParams (&param_);
  prepareParam (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &param_);

  //set flag of bSimulcastAVC
  param_.bSimulcastAVC = true;
  param_.iTemporalLayerNum = (rand() % 2) ? 3 : 4;

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  int aLen[MAX_SPATIAL_LAYER_NUM] = {0};
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

#ifdef DEBUG_FILE_SAVE_SimulcastAVCDiffFps
  FILE* fEnc[MAX_SPATIAL_LAYER_NUM];
  fEnc[0] = fopen ("enc0.264", "wb");
  fEnc[1] = fopen ("enc1.264", "wb");
  fEnc[2] = fopen ("enc2.264", "wb");
  fEnc[3] = fopen ("enc3.264", "wb");
#endif

  int iIdx = 0;

  //create decoder
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);
    aLen[iIdx] = 0;

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

#define PATTERN_NUM (18)
  const int32_t iTemporalPattern[PATTERN_NUM][MAX_SPATIAL_LAYER_NUM] = { {2, 1, 1, 1}, {2, 2, 2, 1}, {4, 1, 1, 1}, {4, 2, 1, 1},
    {1, 2, 1, 1}, {1, 1, 2, 1}, {1, 4, 1, 1}, {2, 4, 2, 1}, {1, 4, 2, 1}, {1, 4, 4, 1},
    {1, 2, 2, 1}, {2, 1, 2, 1}, {1, 2, 4, 1},
    {1, 1, 1, 2}, {1, 2, 2, 2}, {1, 2, 2, 4}, {1, 2, 4, 2}, {1, 4, 4, 4},
  };
  for (int iPatternIdx = 0; iPatternIdx < PATTERN_NUM; iPatternIdx++) {
    for (int i = 0; i < iSpatialLayerNum; i++) {
      param_.sSpatialLayers[i].fFrameRate = (fFrameRate / iTemporalPattern[iPatternIdx][i]);
    }

    int rv = encoder_->InitializeExt (&param_);
    ASSERT_TRUE (rv == cmResultSuccess);

    iEncFrameNum = 10;
    int iInsertIdr = rand() % iEncFrameNum;
    for (int iFrame = 0; iFrame < iEncFrameNum; iFrame++) {
      int iResult;
      int iLayerLen = 0;
      unsigned char* pData[3] = { NULL };

      InitialEncDec (param_.iPicWidth, param_.iPicHeight);
      EncodeOneFrame (0);

      if (iInsertIdr == iFrame) {
        encoder_->ForceIntraFrame (true);
      }

      // init aLen for the current frame
      for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
        aLen[iIdx] = 0;
      }
      for (int iLayer = 0; iLayer < info.iLayerNum; ++iLayer) {
        iLayerLen = 0;
        const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayer];
        const int kiFirstNalType = ((* (layerInfo.pBsBuf + 4)) & 0x1f);
        ASSERT_TRUE ((kiFirstNalType == NAL_SPS) || (kiFirstNalType == NAL_PPS) || (kiFirstNalType == NAL_SLICE)
                     || (kiFirstNalType == NAL_SLICE_IDR) || (kiFirstNalType == NAL_SEI));
        for (int iNal = 0; iNal < layerInfo.iNalCount; ++iNal) {
          iLayerLen += layerInfo.pNalLengthInByte[iNal];
        }

        iIdx = layerInfo.uiSpatialId;
        EXPECT_TRUE (iIdx < iSpatialLayerNum);
        memcpy ((pBsBuf[iIdx] + aLen[iIdx]), layerInfo.pBsBuf, iLayerLen * sizeof (unsigned char));
        aLen[iIdx] += iLayerLen;
      }

      for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
        pData[0] = pData[1] = pData[2] = 0;
        memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

        if (aLen[iIdx] > 0) {
#ifdef DEBUG_FILE_SAVE_SimulcastAVCDiffFps
          fwrite (pBsBuf[iIdx], aLen[iIdx], 1, fEnc[iIdx]);
#endif
          iResult = decoder[iIdx]->DecodeFrame2 (pBsBuf[iIdx], aLen[iIdx], pData, &dstBufInfo_);
          EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;

          iResult = decoder[iIdx]->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
          EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx << iPatternIdx;
          EXPECT_EQ (dstBufInfo_.iBufferStatus, 1) << "LayerIdx=" << iIdx;
        }
      }
    }
  }

  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    free (pBsBuf[iIdx]);

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }
  }
#ifdef DEBUG_FILE_SAVE_SimulcastAVCDiffFps
  for (int i = 0; i < MAX_SPATIAL_LAYER_NUM; i++) {
    fclose (fEnc[i]);
  }
#endif
}

TEST_F (EncodeDecodeTestAPI, DiffSlicingInDlayer) {
  int iSpatialLayerNum = 3;
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, (64 << 2), MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, (64 << 2),
                                2240);//TODO: use MAX_HEIGHT after the limit is removed
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);

  // prepare params
  SEncParamExt   sParam;
  encoder_->GetDefaultParams (&sParam);
  prepareParamDefault (iSpatialLayerNum, 1, iWidth, iHeight, fFrameRate, &sParam);
  sParam.iMultipleThreadIdc = (rand() % 4) + 1;
  sParam.bSimulcastAVC = 1;
  sParam.sSpatialLayers[0].iVideoWidth = (iWidth >> 2);
  sParam.sSpatialLayers[0].iVideoHeight = (iHeight >> 2);
  sParam.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_RASTER_SLICE;
  sParam.sSpatialLayers[1].sSliceArgument.uiSliceMbNum[0] = 0;

  sParam.sSpatialLayers[1].iVideoWidth = (iWidth >> 1);
  sParam.sSpatialLayers[1].iVideoHeight = (iHeight >> 1);
  sParam.sSpatialLayers[1].sSliceArgument.uiSliceMode = SM_RASTER_SLICE;
  sParam.sSpatialLayers[1].sSliceArgument.uiSliceMbNum[0] = 30;
  sParam.sSpatialLayers[1].sSliceArgument.uiSliceMbNum[1] = 32;

  sParam.sSpatialLayers[2].iVideoWidth = iWidth;
  sParam.sSpatialLayers[2].iVideoHeight = iHeight;
  sParam.sSpatialLayers[2].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  sParam.sSpatialLayers[2].sSliceArgument.uiSliceNum = (rand() % 30) + 1;


  int rv = encoder_->InitializeExt (&sParam);
  ASSERT_TRUE (rv == cmResultSuccess) << "Init Failed sParam: rv = " << rv;;

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

  int iIdx = 0;

  //create decoder
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

  TestOneSimulcastAVC (&sParam, decoder, pBsBuf, iSpatialLayerNum, iEncFrameNum, 0);

  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    free (pBsBuf[iIdx]);

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }

  }
}

TEST_F (EncodeDecodeTestAPI, DiffSlicingInDlayerMixed) {
  int iSpatialLayerNum = 2;
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, (64 << 2), MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, (64 << 2),
                                1120);//TODO: use MAX_HEIGHT after the limit is removed
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);

  // prepare params
  SEncParamExt   sParam;
  encoder_->GetDefaultParams (&sParam);
  prepareParamDefault (iSpatialLayerNum, 1, iWidth, iHeight, fFrameRate, &sParam);
  sParam.iMultipleThreadIdc = (rand() % 2) ? 4 : ((rand() % 4) + 1);
  sParam.bSimulcastAVC = 1;
  sParam.sSpatialLayers[0].iVideoWidth = (iWidth >> 2);
  sParam.sSpatialLayers[0].iVideoHeight = (iHeight >> 2);
  sParam.sSpatialLayers[0].sSliceArgument.uiSliceMode = (rand() % 2) ? SM_SIZELIMITED_SLICE : SM_FIXEDSLCNUM_SLICE;
  sParam.sSpatialLayers[0].sSliceArgument.uiSliceSizeConstraint = 1500;

  sParam.sSpatialLayers[1].iVideoWidth = iWidth;
  sParam.sSpatialLayers[1].iVideoHeight = iHeight;
  sParam.sSpatialLayers[1].sSliceArgument.uiSliceMode = (rand() % 2) ? SM_SIZELIMITED_SLICE : SM_FIXEDSLCNUM_SLICE;
  sParam.sSpatialLayers[1].sSliceArgument.uiSliceNum = 1;
  sParam.sSpatialLayers[1].sSliceArgument.uiSliceSizeConstraint = 1500;

  int iTraceLevel = WELS_LOG_QUIET;
  int rv = encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  rv = encoder_->InitializeExt (&sParam);
  ASSERT_TRUE (rv == cmResultSuccess) << "Init Failed sParam: rv = " << rv;;

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

  int iIdx = 0;

  //create decoder
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

  TestOneSimulcastAVC (&sParam, decoder, pBsBuf, iSpatialLayerNum, iEncFrameNum, 0);

  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    free (pBsBuf[iIdx]);

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }

  }
}

TEST_F (EncodeDecodeTestAPI, ThreadNumAndSliceNum) {
  int iSpatialLayerNum = 1;
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, (64 << 2), MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, (64 << 2),
                                2240);//TODO: use MAX_HEIGHT after the limit is removed
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);

  // prepare params
  SEncParamExt   sParam;
  encoder_->GetDefaultParams (&sParam);
  prepareParamDefault (iSpatialLayerNum, 1, iWidth, iHeight, fFrameRate, &sParam);
  sParam.iMultipleThreadIdc = (rand() % 3) + 2;
  sParam.bSimulcastAVC = 1;
  sParam.sSpatialLayers[0].iVideoWidth = iWidth;
  sParam.sSpatialLayers[0].iVideoHeight = iHeight;
  sParam.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  sParam.sSpatialLayers[0].sSliceArgument.uiSliceNum = (rand() % 2) ? (sParam.iMultipleThreadIdc + 1) :
      (sParam.iMultipleThreadIdc - 1);

  int rv = encoder_->InitializeExt (&sParam);
  ASSERT_TRUE (rv == cmResultSuccess) << "Init Failed sParam: rv = " << rv;;

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

  int iIdx = 0;

  //create decoder
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

  TestOneSimulcastAVC (&sParam, decoder, pBsBuf, iSpatialLayerNum, iEncFrameNum, 0);

  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    free (pBsBuf[iIdx]);

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }

  }
}


TEST_F (EncodeDecodeTestAPI, TriggerLoadBalancing) {
  int iSpatialLayerNum = 1;
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, (64 << 2), MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, (64 << 2),
                                2240);//TODO: use MAX_HEIGHT after the limit is removed
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);

  // prepare params
  SEncParamExt   sParam;
  encoder_->GetDefaultParams (&sParam);
  prepareParamDefault (iSpatialLayerNum, 1, iWidth, iHeight, fFrameRate, &sParam);
  sParam.iMultipleThreadIdc = 4;
  sParam.bSimulcastAVC = 1;
  sParam.sSpatialLayers[0].iVideoWidth = iWidth;
  sParam.sSpatialLayers[0].iVideoHeight = iHeight;
  sParam.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  //TODO: use this after the buffer problem is fixed. sParam.sSpatialLayers[0].sSliceArgument.uiSliceMode = (rand()%2) ? SM_FIXEDSLCNUM_SLICE : SM_SIZELIMITED_SLICE;
  sParam.sSpatialLayers[0].sSliceArgument.uiSliceNum = sParam.iMultipleThreadIdc;
  sParam.sSpatialLayers[0].sSliceArgument.uiSliceSizeConstraint = 1000;

  int iTraceLevel = WELS_LOG_QUIET;
  int rv = encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  rv = encoder_->InitializeExt (&sParam);
  ASSERT_TRUE (rv == cmResultSuccess) << "Init Failed sParam: rv = " << rv;;

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

  int iIdx = 0;
  int aLen[MAX_SPATIAL_LAYER_NUM] = {};

  //create decoder
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed pParam: rv = " << rv;

  //begin testing
  for (int iFrame = 0; iFrame < iEncFrameNum; iFrame++) {
    int iResult;
    int iLayerLen = 0;
    unsigned char* pData[3] = { NULL };

    InitialEncDec (sParam.iPicWidth, sParam.iPicHeight);
    int frameSize = EncPic.iPicWidth * EncPic.iPicHeight * 3 / 2;
    memset (buf_.data(), rand() % 256, (frameSize >> 2));
    memset (buf_.data() + (frameSize >> 2), rand() % 256, (frameSize - (frameSize >> 2)));

    int iStartStrip = 0;
    //during first half the complex strip is at top, then during the second half it is at bottom
    if (iFrame > iEncFrameNum / 2) {
      iStartStrip = EncPic.iPicHeight * EncPic.iPicWidth;
    }
    for (int k = 0; k < (EncPic.iPicHeight / 2); k++) {
      memset (buf_.data() + k * EncPic.iPicWidth + iStartStrip, rand() % 256, (EncPic.iPicWidth));
    }

    int rv = encoder_->EncodeFrame (&EncPic, &info);
    ASSERT_TRUE (rv == cmResultSuccess);

    // init
    for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
      aLen[iIdx] = 0;
    }
    for (int iLayer = 0; iLayer < info.iLayerNum; ++iLayer) {
      iLayerLen = 0;
      const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayer];
      for (int iNal = 0; iNal < layerInfo.iNalCount; ++iNal) {
        iLayerLen += layerInfo.pNalLengthInByte[iNal];
      }

      iIdx = layerInfo.uiSpatialId;
      EXPECT_TRUE (iIdx < iSpatialLayerNum) << "iIdx = " << iIdx << ", iSpatialLayerNum = " << iSpatialLayerNum;
      memcpy ((pBsBuf[iIdx] + aLen[iIdx]), layerInfo.pBsBuf, iLayerLen * sizeof (unsigned char));
      aLen[iIdx] += iLayerLen;
    }

    for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
      pData[0] = pData[1] = pData[2] = 0;
      memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

#ifdef DEBUG_FILE_SAVE4
      fwrite (pBsBuf[iIdx], aLen[iIdx], 1, fEnc[iIdx]);
#endif
      iResult = decoder[iIdx]->DecodeFrame2 (pBsBuf[iIdx], aLen[iIdx], pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << ", LayerIdx=" << iIdx;

      iResult = decoder[iIdx]->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << ", LayerIdx=" << iIdx;
      EXPECT_EQ (dstBufInfo_.iBufferStatus, 1) << "LayerIdx=" << iIdx;
    }
  }

  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    free (pBsBuf[iIdx]);

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }

  }
}

