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

TEST_P (EncodeDecodeTestAPI, SetOptionECFlag_ERROR_CON_DISABLE) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  m_LTR_Recover_Request.iLayerId = 0;
  m_LTR_Marking_Feedback.iLayerId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.bEnableLongTermReference = true;
  param_.iLTRRefNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  if (decoder_ != NULL) {
    decoder_->Uninitialize();
  }
  SDecodingParam decParam;
  memset (&decParam, 0, sizeof (SDecodingParam));
  decParam.uiTargetDqLayer = UCHAR_MAX;
  decParam.eEcActiveIdc = ERROR_CON_DISABLE;
  decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
  rv = decoder_->Initialize (&decParam);
  ASSERT_EQ (0, rv);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
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
  int iSkipedBytes;
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
    iSkipedBytes = SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx,
                                    bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    if (iSkipedBytes && bVCLLoss) {
      ASSERT_TRUE (dstBufInfo_.iBufferStatus == 0);
    }
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, SetOptionECFlag_ERROR_CON_SLICE_COPY) {
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

  InitialEncDec (p.width, p.height);
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
  int iSkipedBytes;
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
    iSkipedBytes = SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx,
                                    bVCLLoss);
    uint32_t uiEcIdc = rand() % 2 == 1 ? ERROR_CON_DISABLE : ERROR_CON_SLICE_COPY;
    decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
  (void) iSkipedBytes;
}

static const EncodeDecodeFileParamBase kFileParamArray[] = {
  {300, 160, 96, 6.0f, 2, 1, "000000000000001010101010101010101010101001101010100000010101000011"},
  {300, 140, 96, 6.0f, 4, 1, "000000000000001010101010101010101010101001101010100000010101000011"},
  {300, 140, 96, 6.0f, 4, 1, "000000000000001010111010101011101010101001101010100000010101110011010101"},
};

INSTANTIATE_TEST_CASE_P (EncodeDecodeTestAPIBase, EncodeDecodeTestAPI,
                         ::testing::ValuesIn (kFileParamArray));

TEST_P (EncodeDecodeTestAPI, SetOptionECIDC_GeneralSliceChange) {
  uint32_t uiEcIdc;
  uint32_t uiGet;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY

  uiEcIdc = 0;
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);

  //Start for enc/dec

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int iIdx = 0;
  bool bVCLLoss = false;
  int iPacketNum;
  int len;
  int iTotalSliceSize;

  //enc/dec pictures
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    //decoding after each encoding frame
    len = 0;
    iPacketNum = 0;
    iTotalSliceSize = 0;
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    while (iPacketNum < info.sLayerInfo[0].iNalCount) {
      encToDecSliceData (0, iPacketNum, info, len);
      uiEcIdc = (ERROR_CON_IDC) (rand() % 2);
      decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
      decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
      EXPECT_EQ (uiGet, uiEcIdc);

      bVCLLoss = rand() & 1; //loss or not
      if (!bVCLLoss) { //not loss
        rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf + iTotalSliceSize,
                                     info.sLayerInfo[0].pNalLengthInByte[iPacketNum], pData, &dstBufInfo_);
        if (uiEcIdc == ERROR_CON_DISABLE)
          EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
      }
      //EC_IDC should not change till now
      decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
      EXPECT_EQ (uiGet, uiEcIdc);
      //Set again inside
      uiEcIdc = (ERROR_CON_IDC) (rand() % 2);
      decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
      decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
      EXPECT_EQ (uiGet, uiEcIdc);

      rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
      //EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
      if (uiEcIdc == ERROR_CON_DISABLE && rv != 0)
        EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);

      //deal with next slice
      iTotalSliceSize += len;
      iPacketNum++;
    } //while slice
    iIdx++;
  } //while frame
}

//This case contain 1 slice per picture
//coding order:                 0   1   2   3   4   5   6   7
//frame type:                   IDR P   P   P   P   P   P   P
//EC_IDC:                       0   0   0   2   0   0   1   1
//loss:                         N   Y   N   N   N   Y   Y   N

TEST_F (EncodeDecodeTestAPI, SetOptionECIDC_SpecificFrameChange) {
  uint32_t uiEcIdc;
  uint32_t uiGet;
  EncodeDecodeFileParamBase p = kFileParamArray[0];
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);

  param_.iSpatialLayerNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (ERROR_CON_IDC) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);

  //Start for enc/dec
  int iIdx = 0;
  int len = 0;
  unsigned char* pData[3] = { NULL };
  InitialEncDec (p.width, p.height);
  //Frame 0: IDR, EC_IDC=DISABLE, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (ERROR_CON_IDC) ERROR_CON_DISABLE);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_EQ (rv, 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1);
  iIdx++;

  //Frame 1: P, EC_IDC=DISABLE, loss = 1
  EncodeOneFrame (1);
  iIdx++;

  //Frame 2: P, EC_IDC=DISABLE, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv != 0); //construction error due to data loss
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output due to EC DISABLE
  iIdx++;

  //set EC=SLICE_COPY
  uiEcIdc = (uint32_t) (ERROR_CON_SLICE_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 3: P, EC_IDC=SLICE_COPY, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv != 0); //construction error due to data loss
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1);
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 4: P, EC_IDC=DISABLE, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  //Ref picture is ECed, so current status is ECed, when EC disable, NO output
  EXPECT_TRUE (rv != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  iIdx++;

  //Frame 5: P, EC_IDC=DISABLE, loss = 1
  EncodeOneFrame (1);
  iIdx++;

  //set EC=FRAME_COPY
  uiEcIdc = (uint32_t) (ERROR_CON_FRAME_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 6: P, EC_IDC=FRAME_COPY, loss = 1
  EncodeOneFrame (1);
  EXPECT_EQ (uiGet, uiEcIdc);
  iIdx++;

  //Frame 7: P, EC_IDC=FRAME_COPY, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_FRAME_COPY);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv == 0); // Now the parse process is Error_None, and the reconstruction will has error return
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv != 0); //not sure if previous data drop would be detected in construction
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1);
  iIdx++;
}

//This case contain 2 slices per picture for IDR loss
//coding order:                 0   1   2   3   4
//frame type                    IDR P   P   P   P
//EC_IDC                        2   2   0   1   0
//loss (2 slice: 1,2):          2   0   0   1   0

TEST_F (EncodeDecodeTestAPI, SetOptionECIDC_SpecificSliceChange_IDRLoss) {
  uint32_t uiEcIdc = 2; //default set as SLICE_COPY
  uint32_t uiGet;
  EncodeDecodeFileParamBase p = kFileParamArray[0];
  prepareParamDefault (1, 2,  p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  //Start for enc/dec
  int iIdx = 0;
  int len = 0;
  unsigned char* pData[3] = { NULL };
  int iTotalSliceSize = 0;

  InitialEncDec (p.width, p.height);

  //Frame 0: IDR, EC_IDC=2, loss = 2
  EncodeOneFrame (1);
  iTotalSliceSize = 0;
  encToDecSliceData (0, 0, info, len); //SPS
  iTotalSliceSize = len;
  encToDecSliceData (0, 1, info, len); //PPS
  iTotalSliceSize += len;
  encToDecSliceData (1, 0, info, len); //first slice
  iTotalSliceSize += len;
  //second slice loss
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, iTotalSliceSize, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_EQ (rv, 0); // Reconstruct first slice OK
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //slice incomplete, no output
  iIdx++;

  //Frame 1: P, EC_IDC=2, loss = 0
  //will clean SPS/PPS status
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice together
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE ((rv & 32) != 0); //parse correct, but reconstruct ECed
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //ECed output for frame 0
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //ECed status, reconstruction current frame 1
  EXPECT_TRUE ((rv & 32) != 0); //decoder ECed status
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //ECed output for frame 1
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 2: P, EC_IDC=0, loss = 0
  /////will clean SPS/PPS status
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice together
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  // Ref picture is ECed, so reconstructed picture is ECed
  EXPECT_TRUE ((rv & 32) != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  iIdx++;

  //set EC=SLICE_COPY
  uiEcIdc = (uint32_t) (ERROR_CON_FRAME_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 3: P, EC_IDC=2, loss = 1
  EncodeOneFrame (1);
  encToDecSliceData (0, 0, info, iTotalSliceSize); //slice 1 lost
  encToDecSliceData (0, 1, info, len); //slice 2
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_FRAME_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf + iTotalSliceSize, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE ((rv & 32) != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //slice loss
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 4: P, EC_IDC=0, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); // EC_IDC=0, previous picture slice lost, no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); // No ref picture, no output
  iIdx++;

}



//This case contain 2 slices per picture for no IDR loss
//coding order:                 0   1   2   3   4   5
//frame type                    IDR P   P   P   P   IDR
//EC_IDC                        0   2   0   2   0   ^2^
//loss (2 slice: 1,2):          0   0   1   2   0   0

TEST_F (EncodeDecodeTestAPI, SetOptionECIDC_SpecificSliceChange_IDRNoLoss) {
  uint32_t uiEcIdc;
  uint32_t uiGet;
  EncodeDecodeFileParamBase p = kFileParamArray[0];
  prepareParamDefault (1, 2,  p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  //Start for enc/dec
  int iIdx = 0;
  int len = 0;
  unsigned char* pData[3] = { NULL };
  int iTotalSliceSize = 0;

  InitialEncDec (p.width, p.height);

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 0: IDR, EC_IDC=0, loss = 0
  //Expected result: all OK, 2nd Output
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //output frame 0
  iIdx++;

  //Frame 1: P, EC_IDC=0, loss = 0
  //Expected result: all OK, 2nd Output
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice together
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //ECed output for frame 0
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction current frame 1
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //ECed output for frame 1
  iIdx++;

  //Frame 2: P, EC_IDC=0, loss = 1
  //Expected result: all OK, no Output
  EncodeOneFrame (1);
  encToDecSliceData (0, 0, info, iTotalSliceSize); // slice 1 lost
  encToDecSliceData (0, 1, info, len); // slice 2 only
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf + iTotalSliceSize, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  iIdx++;

  //set EC=SLICE_COPY
  uiEcIdc = (uint32_t) (ERROR_CON_SLICE_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 3: P, EC_IDC=2, loss = 2
  //Expected result: neither OK, 1st Output
  EncodeOneFrame (1);
  encToDecSliceData (0, 0, info, len); //slice 1 only
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE ((rv & 32) != 0); //parse OK but frame 2 ECed
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //slice loss but ECed output Frame 2
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE ((rv & 32) != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //slice loss
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 4: P, EC_IDC=0, loss = 0
  //Expected result: depends on DecodeFrame2 result. If OK, output; else ,no output
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv != 0); //previous slice not outputted, will return error due to incomplete frame
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //output previous pic
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction,
  // previous frame NOT output, no ref
  EXPECT_TRUE (rv != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //output previous pic
  iIdx++;

  //Frame 5: IDR, EC_IDC=2->0, loss = 0
  //Expected result: depends on DecodeFrame2 result. If OK, output; else ,no output
  int32_t iIDRPeriod = 1;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  EncodeOneFrame (1);
  EXPECT_TRUE (info.eFrameType == videoFrameTypeIDR);
  encToDecSliceData (0, 0, info, len); //SPS
  iTotalSliceSize = len;
  encToDecSliceData (0, 1, info, len); //PPS
  iTotalSliceSize += len;
  encToDecSliceData (1, 0, info, len); //slice 1
  iTotalSliceSize += len;
  //set EC=SLICE_COPY for slice 1
  uiEcIdc = (uint32_t) (ERROR_CON_SLICE_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, iTotalSliceSize, pData, &dstBufInfo_);
  EXPECT_TRUE (rv == 0); // IDR status return error_free
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //frame incomplete
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction,
  EXPECT_TRUE (rv == 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //output previous pic
  //set EC=DISABLE for slice 2
  encToDecSliceData (1, 1, info, len); //slice 1
  uiEcIdc = (int) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf + iTotalSliceSize, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //Parse correct under no EC
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //frame incomplete
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction,
  EXPECT_EQ (rv, 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //output previous pic
  iIdx++;

}

static const EncodeDecodeFileParamBase kSVCSwitch[] = {
  {300, 160, 96, 6.0f, 2, 1, "120012130101012311201221323"},
};

TEST_F (EncodeDecodeTestAPI, Engine_SVC_Switch_I) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  m_LTR_Recover_Request.iLayerId = 0;
  m_LTR_Marking_Feedback.iLayerId = 0;
  EncodeDecodeFileParamBase p = kSVCSwitch[0];
  p.width = p.width << 2;
  p.height = p.height << 2;
  prepareParamDefault (4, p.slicenum, p.width, p.height, p.frameRate, &param_);
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 4;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
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
  int iTarDid = 0;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    if (info.eFrameType == videoFrameTypeIDR) {
      iTarDid = rand() % 4;
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

    ExtractDidNal (&info, len, &m_SLostSim, iTarDid);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_EQ (rv, 0);
    int iTid = -1;
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, -1);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_EQ (rv, 0);
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, info.sLayerInfo[0].uiTemporalId);
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_F (EncodeDecodeTestAPI, Engine_SVC_Switch_P) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  m_LTR_Recover_Request.iLayerId = 0;
  m_LTR_Marking_Feedback.iLayerId = 0;
  EncodeDecodeFileParamBase p = kSVCSwitch[0];
  int iTarDid = 0;
  int iLastDid = 0;
  p.width = p.width << 2;
  p.height = p.height << 2;
  prepareParamDefault (4, p.slicenum, p.width, p.height, p.frameRate, &param_);
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 4;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
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
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    if (iIdx < (int) strlen (p.pLossSequence)) {
      switch (p.pLossSequence[iIdx]) {
      case '0':
        iTarDid = 0;
        break;
      case '1':
        iTarDid = 1;
        break;
      case '2':
        iTarDid = 2;
        break;
      case '3':
        iTarDid = 3;
        break;
      default :
        iTarDid = rand() % 4;
        break;
      }
    } else {
      iTarDid = rand() % 4;
    }

    ExtractDidNal (&info, len, &m_SLostSim, iTarDid);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    int iTid = -1;
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, -1);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, false);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    if (info.eFrameType == videoFrameTypeP && iIdx > 0 && iLastDid != iTarDid) {
      ASSERT_NE (rv, 0);
    } else if (info.eFrameType == videoFrameTypeIDR) {
      ASSERT_EQ (rv, 0);
    }
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, info.sLayerInfo[0].uiTemporalId);
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, false);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
    iLastDid = iTarDid;
  }
}


