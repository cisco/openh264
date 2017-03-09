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


TEST_P (EncodeDecodeTestAPI, GetOptionLTR_ALLIDR) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  ASSERT_TRUE (InitialEncDec (p.width, p.height));

  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_SPS_PPS_ID_STRATEGY, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = (int32_t) pow (2.0f, (param_.iTemporalLayerNum - 1)) * ((rand() % 5) + 1);
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (0);
    ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    encoder_->ForceIntraFrame (true);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionLTR_ALLLTR) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.iLayerId = 0;
  m_LTR_Marking_Feedback.iLayerId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
  int frameSize = p.width * p.height * 3 / 2;
  buf_.SetLength (frameSize);
  ASSERT_TRUE (buf_.Length() == (size_t)frameSize);
  SFrameBSInfo info;
  memset (&info, 0, sizeof (SFrameBSInfo));

  SSourcePicture pic;
  memset (&pic, 0, sizeof (SSourcePicture));
  pic.iPicWidth = p.width;
  pic.iPicHeight = p.height;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = pic.iPicWidth;
  pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
  pic.pData[0] = buf_.data();
  pic.pData[1] = pic.pData[0] + p.width * p.height;
  pic.pData[2] = pic.pData[1] + (p.width * p.height >> 2);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_SPS_PPS_ID_STRATEGY, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnknownReason);
    m_LTR_Recover_Request.uiFeedbackType = LTR_RECOVERY_REQUEST;
    m_LTR_Recover_Request.iCurrentFrameNum = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    m_LTR_Recover_Request.uiIDRPicId = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    m_LTR_Recover_Request.iLastCorrectFrameNum = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    encoder_->SetOption (ENCODER_LTR_RECOVERY_REQUEST, &m_LTR_Recover_Request);
    m_LTR_Marking_Feedback.uiFeedbackType = rand() % 2 == 1 ? LTR_MARKING_SUCCESS : LTR_MARKING_FAILED;
    m_LTR_Marking_Feedback.uiIDRPicId = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    m_LTR_Marking_Feedback.iLTRFrameNum = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    encoder_->SetOption (ENCODER_LTR_MARKING_FEEDBACK, &m_LTR_Marking_Feedback);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionLTR_Engine) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  m_LTR_Recover_Request.iLayerId = 0;
  m_LTR_Marking_Feedback.iLayerId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  ASSERT_TRUE (InitialEncDec (p.width, p.height));

  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_SPS_PPS_ID_STRATEGY, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  int iLossIdx = 0;
  bool bVCLLoss = false;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx, bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

