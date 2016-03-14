#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"
#include "BaseDecoderTest.h"
#include "BaseEncoderTest.h"
#include "wels_common_defs.h"
#include "utils/HashFunctions.h"
#include <string>
#include <vector>
#include "encode_decode_api_test.h"
using namespace WelsCommon;

static void TestOutPutTrace (void* ctx, int level, const char* string) {
  STraceUnit* pTraceUnit = (STraceUnit*) ctx;
  EXPECT_LE (level, pTraceUnit->iTarLevel);
}

TEST_P (EncodeDecodeTestAPI, DecoderVclNal) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum, p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  InitialEncDec (p.width, p.height);

  int iIdx = 0;
  while (iIdx <= p.numframes) {

    EncodeOneFrame (0);

    //decoding after each encoding frame
    int vclNal, len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_TRUE (rv == cmResultSuccess);
    rv = decoder_->GetOption (DECODER_OPTION_VCL_NAL, &vclNal);
    EXPECT_EQ (vclNal, FEEDBACK_UNKNOWN_NAL); //no reconstruction, unknown return
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_TRUE (rv == cmResultSuccess);
    rv = decoder_->GetOption (DECODER_OPTION_VCL_NAL, &vclNal);
    EXPECT_EQ (vclNal, FEEDBACK_VCL_NAL);
    iIdx++;
  } //while
  //ignore last frame
}

TEST_P (EncodeDecodeTestAPI, GetOptionFramenum) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  InitialEncDec (p.width, p.height);

  int32_t iEncFrameNum = -1;
  int32_t iDecFrameNum;
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (0);
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_FRAME_NUM, &iDecFrameNum);
    EXPECT_EQ (iDecFrameNum, -1);
    iEncFrameNum++;
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_FRAME_NUM, &iDecFrameNum);
    EXPECT_EQ (iEncFrameNum, iDecFrameNum);
    iIdx++;
  } //while
  //ignore last frame
}

TEST_P (EncodeDecodeTestAPI, GetOptionIDR) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  //init for encoder
  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  InitialEncDec (p.width, p.height);

  int32_t iEncCurIdrPicId = 0;
  int32_t iDecCurIdrPicId;
  int32_t iIDRPeriod = 1;
  int32_t iSpsPpsIdAddition = 0;
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    iSpsPpsIdAddition = rand() %
                        2; //the current strategy supports more than 2 modes, but the switch between the modes>2 is not allowed
    iIDRPeriod = (rand() % 150) + 1;
    encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
    encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);

    EncodeOneFrame (0);

    if (info.eFrameType == videoFrameTypeIDR) {
      iEncCurIdrPicId = iEncCurIdrPicId + 1;
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_IDR_PIC_ID, &iDecCurIdrPicId);
    EXPECT_EQ (iDecCurIdrPicId, iEncCurIdrPicId);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_IDR_PIC_ID, &iDecCurIdrPicId);
    EXPECT_EQ (iDecCurIdrPicId, iEncCurIdrPicId);
    iIdx++;
  } //while
  //ignore last frame
}

TEST_P (EncodeDecodeTestAPI, InOutTimeStamp) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  int iSkipedBytes;
  unsigned long long uiEncTimeStamp = 100;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    uint32_t uiEcIdc = ERROR_CON_SLICE_COPY_CROSS_IDR_FREEZE_RES_CHANGE;
    decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
    dstBufInfo_.uiInBsTimeStamp = uiEncTimeStamp;
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    dstBufInfo_.uiInBsTimeStamp = uiEncTimeStamp;
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    if (dstBufInfo_.iBufferStatus == 1) {
      EXPECT_EQ (uiEncTimeStamp, dstBufInfo_.uiOutYuvTimeStamp);
    }
    iIdx++;
    uiEncTimeStamp++;
  }
  (void) iSkipedBytes;
}

TEST_P (EncodeDecodeTestAPI, GetOptionTid_AVC_NOPREFIX) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.bPrefixNalAddingCtrl = false;
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
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
    int iTid = -1;
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    if (iTid != -1) {
      ASSERT_EQ (iTid, 0);
    }
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    std::vector<SLostSim>::iterator iter = m_SLostSim.begin();
    bool bHasVCL = false;
    for (unsigned int k = 0; k < m_SLostSim.size(); k++) {
      if (IS_VCL_NAL (iter->eNalType, 0) && iter->isLost == false) {
        bHasVCL = true;
        break;
      }
      iter++;
    }
    (void) bHasVCL;
    if (iTid != -1) {
      ASSERT_EQ (iTid, 0);
    }
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionTid_AVC_WITH_PREFIX_NOLOSS) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.bPrefixNalAddingCtrl = true;
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
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
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    ExtractDidNal (&info, len, &m_SLostSim, 0);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    int iTid = -1;
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, -1);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, info.sLayerInfo[0].uiTemporalId);
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionTid_SVC_L1_NOLOSS) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (2, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 2;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
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
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    ExtractDidNal (&info, len, &m_SLostSim, 1);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    int iTid = -1;
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, -1);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, info.sLayerInfo[0].uiTemporalId);
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}



TEST_P (EncodeDecodeTestAPI, SetOption_Trace) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = 1;

  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  pFunc = TestOutPutTrace;
  pTraceInfo = &sTrace;
  sTrace.iTarLevel = iTraceLevel;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK, &pFunc);
  encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);
  decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK, &pFunc);
  decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);


  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
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
    iTraceLevel = rand() % 33;
    sTrace.iTarLevel = iTraceLevel;
    encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    ExtractDidNal (&info, len, &m_SLostSim, 0);
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

TEST_P (EncodeDecodeTestAPI, SetOption_Trace_NULL) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = 1;
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);

  int32_t iTraceLevel = WELS_LOG_QUIET;
  pFunc = NULL;
  pTraceInfo = NULL;
  encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK, &pFunc);
  encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);
  decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK, &pFunc);
  decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
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
    iTraceLevel = rand() % 33;
    encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    ExtractDidNal (&info, len, &m_SLostSim, 0);
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




class DecodeCrashTestAPI : public ::testing::TestWithParam<EncodeDecodeFileParamBase>, public EncodeDecodeTestBase {
 public:
  uint8_t iRandValue;
 public:
  void SetUp() {
    EncodeDecodeTestBase::SetUp();
    ucBuf_ = NULL;
    ucBuf_ = new unsigned char [1000000];
    ASSERT_TRUE (ucBuf_ != NULL);
  }

  void TearDown() {
    EncodeDecodeTestBase::TearDown();
    if (NULL != ucBuf_) {
      delete[] ucBuf_;
      ucBuf_ = NULL;
    }
    ASSERT_TRUE (ucBuf_ == NULL);
  }

  void prepareParam (int iLayerNum, int iSliceNum, int width, int height, float framerate, SEncParamExt* pParam) {
    memset (pParam, 0, sizeof (SEncParamExt));
    EncodeDecodeTestBase::prepareParam (iLayerNum, iSliceNum,  width, height, framerate, pParam);
  }

  void prepareEncDecParam (const EncodeDecodeFileParamBase EncDecFileParam);
  void EncodeOneFrame() {
    int frameSize = EncPic.iPicWidth * EncPic.iPicHeight * 3 / 2;
    memset (buf_.data(), iRandValue, (frameSize >> 2));
    memset (buf_.data() + (frameSize >> 2), rand() % 256, (frameSize - (frameSize >> 2)));
    int rv = encoder_->EncodeFrame (&EncPic, &info);
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnknownReason);
  }
 protected:
  unsigned char* ucBuf_;
};

void DecodeCrashTestAPI::prepareEncDecParam (const EncodeDecodeFileParamBase EncDecFileParam) {
  // for encoder
  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = EncDecFileParam.width * EncDecFileParam.height * 3 / 2;

  buf_.SetLength (frameSize);
  ASSERT_TRUE (buf_.Length() == (size_t)frameSize);

  memset (&EncPic, 0, sizeof (SSourcePicture));
  EncPic.iPicWidth = EncDecFileParam.width;
  EncPic.iPicHeight = EncDecFileParam.height;
  EncPic.iColorFormat = videoFormatI420;
  EncPic.iStride[0] = EncPic.iPicWidth;
  EncPic.iStride[1] = EncPic.iStride[2] = EncPic.iPicWidth >> 1;
  EncPic.pData[0] = buf_.data();
  EncPic.pData[1] = EncPic.pData[0] + EncDecFileParam.width * EncDecFileParam.height;
  EncPic.pData[2] = EncPic.pData[1] + (EncDecFileParam.width * EncDecFileParam.height >> 2);

  //for decoder
  memset (&info, 0, sizeof (SFrameBSInfo));

  //set a fixed random value
  iRandValue = rand() % 256;
}

struct EncodeDecodeParamBase {
  int width;
  int height;
  float frameRate;
  int iTarBitrate;
};

#define NUM_OF_POSSIBLE_RESOLUTION (9)
static const EncodeDecodeParamBase kParamArray[] = {
  {160, 90, 6.0f, 250000},
  {90, 160, 6.0f, 250000},
  {320, 180, 12.0f, 500000},
  {180, 320, 12.0f, 500000},
  {480, 270, 12.0f, 600000},
  {270, 480, 12.0f, 600000},
  {640, 360, 24.0f, 800000},
  {360, 640, 24.0f, 800000},
  {1280, 720, 24.0f, 1000000},
};

//#define DEBUG_FILE_SAVE_CRA
TEST_F (DecodeCrashTestAPI, DecoderCrashTest) {
  uint32_t uiGet;
  encoder_->Uninitialize();

  //do tests until crash
  unsigned int uiLoopRound = 0;
  unsigned char* pucBuf = ucBuf_;
  int iDecAuSize;
#ifdef DEBUG_FILE_SAVE_CRA
  //open file to save tested BS
  FILE* fDataFile = fopen ("test_crash.264", "wb");
  FILE* fLenFile = fopen ("test_crash_len.log", "w");
  int iFileSize = 0;
#endif

  //set eCurStrategy for one test
  EParameterSetStrategy eCurStrategy = CONSTANT_ID;
  switch (rand() % 7) {
  case 1:
    eCurStrategy = INCREASING_ID;
    break;
  case 2:
    eCurStrategy = SPS_LISTING;
    break;
  case 3:
    eCurStrategy = SPS_LISTING_AND_PPS_INCREASING;
    break;
  case 6:
    eCurStrategy = SPS_PPS_LISTING;
    break;
  default:
    //using the initial value
    break;
  }

  do {
    int iTotalFrameNum = (rand() % 100) + 1;
    int iSeed = rand() % NUM_OF_POSSIBLE_RESOLUTION;
    EncodeDecodeParamBase p = kParamArray[iSeed];
#ifdef DEBUG_FILE_SAVE_CRA
    printf ("using param set %d in loop %d\n", iSeed, uiLoopRound);
#endif
    //Initialize Encoder
    prepareParam (1, 1, p.width, p.height, p.frameRate, &param_);
    param_.iRCMode = RC_TIMESTAMP_MODE;
    param_.iTargetBitrate = p.iTarBitrate;
    param_.uiIntraPeriod = 0;
    param_.eSpsPpsIdStrategy = eCurStrategy;
    param_.bEnableBackgroundDetection = true;
    param_.bEnableSceneChangeDetect = (rand() % 3) ? true : false;
    param_.bPrefixNalAddingCtrl = (rand() % 2) ? true : false;
    param_.iEntropyCodingModeFlag = 0;
    param_.bEnableFrameSkip = true;
    param_.iMultipleThreadIdc = 0;
    param_.sSpatialLayers[0].iSpatialBitrate = p.iTarBitrate;
    param_.sSpatialLayers[0].iMaxSpatialBitrate = p.iTarBitrate << 1;
    param_.sSpatialLayers[0].sSliceArgument.uiSliceMode = (rand() % 2) ? SM_SIZELIMITED_SLICE : SM_SINGLE_SLICE;
    if (param_.sSpatialLayers[0].sSliceArgument.uiSliceMode == SM_SIZELIMITED_SLICE) {
      param_.sSpatialLayers[0].sSliceArgument.uiSliceSizeConstraint = 1400;
      param_.uiMaxNalSize = 1400;
    } else {
      param_.sSpatialLayers[0].sSliceArgument.uiSliceSizeConstraint = 0;
      param_.uiMaxNalSize = 0;
    }

    int rv = encoder_->InitializeExt (&param_);
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
    EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY
    int32_t iTraceLevel = WELS_LOG_QUIET;
    encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

    //Start for enc/dec
    int iIdx = 0;
    unsigned char* pData[3] = { NULL };

    EncodeDecodeFileParamBase pInput; //to conform with old functions
    pInput.width =  p.width;
    pInput.height = p.height;
    pInput.frameRate = p.frameRate;
    prepareEncDecParam (pInput);
    while (iIdx++ < iTotalFrameNum) { // loop in frame
      EncodeOneFrame();
#ifdef DEBUG_FILE_SAVE_CRA
      //reset file if file size large
      if ((info.eFrameType == videoFrameTypeIDR) && (iFileSize >= (1 << 25))) {
        fclose (fDataFile);
        fDataFile = fopen ("test_crash.264", "wb");
        iFileSize = 0;
        decoder_->Uninitialize();

        SDecodingParam decParam;
        memset (&decParam, 0, sizeof (SDecodingParam));
        decParam.uiTargetDqLayer = UCHAR_MAX;
        decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
        decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

        rv = decoder_->Initialize (&decParam);
        ASSERT_EQ (0, rv);
      }
#endif
      if (info.eFrameType == videoFrameTypeSkip)
        continue;
      //deal with packets
      unsigned char* pBsBuf;
      iDecAuSize = 0;
      pucBuf = ucBuf_; //init buf start pos for decoder usage
      for (int iLayerNum = 0; iLayerNum < info.iLayerNum; iLayerNum++) {
        SLayerBSInfo* pLayerBsInfo = &info.sLayerInfo[iLayerNum];
        pBsBuf = info.sLayerInfo[iLayerNum].pBsBuf;
        int iTotalNalCnt = pLayerBsInfo->iNalCount;
        for (int iNalCnt = 0; iNalCnt < iTotalNalCnt; iNalCnt++) {  //loop in NAL
          int iPacketSize = pLayerBsInfo->pNalLengthInByte[iNalCnt];
          //packet loss
          int iLossRateRange = (uiLoopRound % 100) + 1; //1-100
          int iLossRate = (rand() % iLossRateRange);
          bool bPacketLost = (rand() % 101) > (100 -
                                               iLossRate);   // [0, (100-iLossRate)] indicates NO LOSS, (100-iLossRate, 100] indicates LOSS
          if (!bPacketLost) { //no loss
            memcpy (pucBuf, pBsBuf, iPacketSize);
            pucBuf += iPacketSize;
            iDecAuSize += iPacketSize;
          }
#ifdef DEBUG_FILE_SAVE_CRA
          else {
            printf ("lost packet size=%d at frame-type=%d at loss rate %d (%d)\n", iPacketSize, info.eFrameType, iLossRate,
                    iLossRateRange);
          }
#endif
          //update bs info
          pBsBuf += iPacketSize;
        } //nal
      } //layer

#ifdef DEBUG_FILE_SAVE_CRA
      //save to file
      fwrite (ucBuf_, 1, iDecAuSize, fDataFile);
      fflush (fDataFile);
      iFileSize += iDecAuSize;

      //save to len file
      unsigned long ulTmp[4];
      ulTmp[0] = ulTmp[1] = ulTmp[2] = iIdx;
      ulTmp[3] = iDecAuSize;
      fwrite (ulTmp, sizeof (unsigned long), 4, fLenFile); // index, timeStamp, data size
      fflush (fLenFile);
#endif

      //decode
      pData[0] = pData[1] = pData[2] = 0;
      memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

      rv = decoder_->DecodeFrame2 (ucBuf_, iDecAuSize, pData, &dstBufInfo_);
      rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
      //guarantee decoder EC status
      decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
      EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
    } //frame
    uiLoopRound ++;
    if (uiLoopRound >= (1 << 30))
      uiLoopRound = 0;
#ifdef DEBUG_FILE_SAVE_CRA
    if (uiLoopRound % 100 == 0)
      printf ("run %d times.\n", uiLoopRound);
  } while (1); //while (iLoopRound<100);
  fclose (fDataFile);
  fclose (fLenFile);
#else
  }
  while (uiLoopRound < 10);
#endif

}

const uint32_t kiTotalLayer = 3; //DO NOT CHANGE!
const uint32_t kiSliceNum = 2; //DO NOT CHANGE!
const uint32_t kiWidth = 160; //DO NOT CHANGE!
const uint32_t kiHeight = 96; //DO NOT CHANGE!
const uint32_t kiFrameRate = 12; //DO NOT CHANGE!
const uint32_t kiFrameNum = 100; //DO NOT CHANGE!
const char* pHashStr[] = { //DO NOT CHANGE!
  "9c4e6146b29bac5d5d4be3c5bbab9c072dcb3f3f",
  "f350001c333902029800bd291fbed915a4bdf19a",
  "eb9d853b7daec03052c4850027ac94adc84c3a7e"
};

class DecodeParseAPI : public ::testing::TestWithParam<EncodeDecodeFileParamBase>, public EncodeDecodeTestBase {
 public:
  void SetUp() {
    SHA1Reset (&ctx_);
    EncodeDecodeTestBase::SetUp();

    if (decoder_)
      decoder_->Uninitialize();
    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.bParseOnly = true;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    int rv = decoder_->Initialize (&decParam);
    ASSERT_EQ (0, rv);
    memset (&BsInfo_, 0, sizeof (SParserBsInfo));
    fYuv_ = fopen ("./res/CiscoVT2people_160x96_6fps.yuv", "rb");
    ASSERT_TRUE (fYuv_ != NULL);
    iWidth_ = kiWidth;
    iHeight_ = kiHeight;
  }
  void TearDown() {
    EncodeDecodeTestBase::TearDown();
    fclose (fYuv_);
  }

  void prepareEncDecParam (const EncodeDecodeFileParamBase p) {
    EncodeDecodeTestBase::prepareEncDecParam (p);
    unsigned char* pTmpPtr = BsInfo_.pDstBuff; //store for restore
    memset (&BsInfo_, 0, sizeof (SParserBsInfo));
    BsInfo_.pDstBuff = pTmpPtr;
  }

  void EncodeOneFrame (int iIdx) {
    int iFrameSize = iWidth_ * iHeight_ * 3 / 2;
    int iSize = (int) fread (buf_.data(), sizeof (char), iFrameSize, fYuv_);
    if (feof (fYuv_) || iSize != iFrameSize) {
      rewind (fYuv_);
      iSize = (int) fread (buf_.data(), sizeof (char), iFrameSize, fYuv_);
      ASSERT_TRUE (iSize == iFrameSize);
    }
    int rv = encoder_->EncodeFrame (&EncPic, &info);
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnknownReason);
  }

  void prepareParam (int iLayerNum, int iSliceNum, int width, int height, float framerate, SEncParamExt* pParam) {
    memset (pParam, 0, sizeof (SEncParamExt));
    EncodeDecodeTestBase::prepareParam (iLayerNum, iSliceNum,  width, height, framerate, pParam);
  }

 protected:
  SParserBsInfo BsInfo_;
  FILE* fYuv_;
  int iWidth_;
  int iHeight_;
  SHA1Context ctx_;
};

//#define DEBUG_FILE_SAVE
TEST_F (DecodeParseAPI, ParseOnly_General) {
  EncodeDecodeFileParamBase p;
  p.width = iWidth_;
  p.height = iHeight_;
  p.frameRate = kiFrameRate;
  p.numframes = kiFrameNum;
  prepareParam (kiTotalLayer, kiSliceNum, p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = kiTotalLayer;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == 0);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  uint32_t uiTargetLayerId = rand() % kiTotalLayer; //run only once
#ifdef DEBUG_FILE_SAVE
  FILE* fDec = fopen ("output.264", "wb");
  FILE* fEnc = fopen ("enc.264", "wb");
  FILE* fExtract = fopen ("extract.264", "wb");
#endif
  if (uiTargetLayerId < kiTotalLayer) { //should always be true
    //Start for enc
    int iLen = 0;
    prepareEncDecParam (p);
    int iFrame = 0;

    while (iFrame < p.numframes) {
      //encode
      EncodeOneFrame (iFrame);
      //extract target layer data
      encToDecData (info, iLen);
#ifdef DEBUG_FILE_SAVE
      fwrite (info.sLayerInfo[0].pBsBuf, iLen, 1, fEnc);
#endif
      ExtractDidNal (&info, iLen, &m_SLostSim, uiTargetLayerId);
#ifdef DEBUG_FILE_SAVE
      fwrite (info.sLayerInfo[0].pBsBuf, iLen, 1, fExtract);
#endif
      //parseonly
      //BsInfo_.pDstBuff = new unsigned char [1000000];
      rv = decoder_->DecodeParser (info.sLayerInfo[0].pBsBuf, iLen, &BsInfo_);
      EXPECT_TRUE (rv == 0);
      EXPECT_TRUE (BsInfo_.iNalNum == 0);
      rv = decoder_->DecodeParser (NULL, 0, &BsInfo_);
      EXPECT_TRUE (rv == 0);
      EXPECT_TRUE (BsInfo_.iNalNum != 0);
      //get final output bs
      iLen = 0;
      int i = 0;
      while (i < BsInfo_.iNalNum) {
        iLen += BsInfo_.iNalLenInByte[i];
        i++;
      }
#ifdef DEBUG_FILE_SAVE
      fwrite (BsInfo_.pDstBuff, iLen, 1, fDec);
#endif
      SHA1Input (&ctx_, BsInfo_.pDstBuff, iLen);
      iFrame++;
    }
    //calculate final SHA1 value
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1Result (&ctx_, digest);
    if (!HasFatalFailure()) {
      CompareHash (digest, pHashStr[uiTargetLayerId]);
    }
  } //while
#ifdef DEBUG_FILE_SAVE
  fclose (fEnc);
  fclose (fExtract);
  fclose (fDec);
#endif
}

//This case is for one layer only, for incomplete frame input
//First slice is loss for random one picture with 2 slices per pic
TEST_F (DecodeParseAPI, ParseOnly_SpecSliceLoss) {
  int32_t iLayerNum = 1;
  int32_t iSliceNum = 2;
  EncodeDecodeFileParamBase p;
  p.width = iWidth_;
  p.height = iHeight_;
  p.frameRate = kiFrameRate;
  p.numframes = 5;
  prepareParam (iLayerNum, iSliceNum, p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = iLayerNum;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == 0);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  int32_t iMissedPicNum = rand() % (p.numframes - 1) + 1; //IDR no loss
  //Start for enc
  int iLen = 0;
  uint32_t uiGet;
  prepareEncDecParam (p);
  int iFrame = 0;

  while (iFrame < p.numframes) {
    //encode
    EncodeOneFrame (iFrame);
    //parseonly
    if (iFrame == iMissedPicNum) { //make current frame partly missing
      //Frame: P, first slice loss
      int32_t iTotalSliceSize = 0;
      encToDecSliceData (0, 0, info, iTotalSliceSize); //slice 1 lost
      encToDecSliceData (0, 1, info, iLen); //slice 2
      decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
      EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
      rv = decoder_->DecodeParser (info.sLayerInfo[0].pBsBuf + iTotalSliceSize, iLen, &BsInfo_);
      EXPECT_TRUE (rv == 0);
      EXPECT_TRUE (BsInfo_.iNalNum == 0);
      rv = decoder_->DecodeParser (NULL, 0, &BsInfo_);
      EXPECT_TRUE (rv != 0);
    } else { //normal frame, complete
      encToDecData (info, iLen);
      rv = decoder_->DecodeParser (info.sLayerInfo[0].pBsBuf, iLen, &BsInfo_);
      EXPECT_TRUE (rv == 0); //parse correct
      EXPECT_TRUE (BsInfo_.iNalNum == 0);
      rv = decoder_->DecodeParser (NULL, 0, &BsInfo_);
      if (iFrame < iMissedPicNum) { //correct frames, all OK with output
        EXPECT_TRUE (rv == 0);
        EXPECT_TRUE (BsInfo_.iNalNum != 0);
      } else { //(iFrame > iMissedPicNum), should output nothing as error
        EXPECT_TRUE (rv != 0);
        EXPECT_TRUE (BsInfo_.iNalNum == 0);
      }
    }
    iFrame++;
  } //while
}



