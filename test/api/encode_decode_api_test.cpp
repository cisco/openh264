#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"
#include "BaseDecoderTest.h"
#include "BaseEncoderTest.h"
#include "wels_common_defs.h"
#include <string>
#include <vector>
using namespace WelsCommon;

typedef struct SLost_Sim {
  WelsCommon::EWelsNalUnitType eNalType;
  bool isLost;
} SLostSim;


struct EncodeDecodeFileParamBase {
  int numframes;
  int width;
  int height;
  float frameRate;
  int slicenum;
  bool bLostPara;
  const char* pLossSequence;
};

static void welsStderrTraceOrigin (void* ctx, int level, const char* string) {
  fprintf (stderr, "%s\n", string);
}

typedef struct STrace_Unit {
  int iTarLevel;
} STraceUnit;

static void TestOutPutTrace (void* ctx, int level, const char* string) {
  STraceUnit* pTraceUnit = (STraceUnit*) ctx;
  EXPECT_LE (level, pTraceUnit->iTarLevel);
}

class EncodeDecodeTestBase : public ::testing::TestWithParam<EncodeDecodeFileParamBase>,
  public BaseEncoderTest, public BaseDecoderTest {
 public:
  virtual void SetUp() {
    BaseEncoderTest::SetUp();
    BaseDecoderTest::SetUp();
    pFunc = welsStderrTraceOrigin;
    pTraceInfo = NULL;
    encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK, &pFunc);
    encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);
    decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK, &pFunc);
    decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);
  }

  virtual void TearDown() {
    BaseEncoderTest::TearDown();
    BaseDecoderTest::TearDown();
  }

  virtual void prepareParam (int width, int height, float framerate) {
    memset (&param_, 0, sizeof (SEncParamExt));
    param_.iUsageType = CAMERA_VIDEO_REAL_TIME;
    param_.iPicWidth = width;
    param_.iPicHeight = height;
    param_.fMaxFrameRate = framerate;
    param_.iRCMode = RC_OFF_MODE; //rc off
    param_.iMultipleThreadIdc = 1; //single thread
    param_.sSpatialLayers[0].iVideoWidth = width;
    param_.sSpatialLayers[0].iVideoHeight = height;
    param_.sSpatialLayers[0].fFrameRate = framerate;
    param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
  }

  virtual void encToDecData (const SFrameBSInfo& info, int& len) {
    len = 0;
    for (int i = 0; i < info.iLayerNum; ++i) {
      const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
      for (int j = 0; j < layerInfo.iNalCount; ++j) {
        len += layerInfo.pNalLengthInByte[j];
      }
    }
  }

  virtual void encToDecSliceData (const int iLayerNum, const int iSliceNum, const SFrameBSInfo& info, int& len) {
    ASSERT_TRUE (iLayerNum < MAX_LAYER_NUM_OF_FRAME);
    len = 0;
    const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayerNum];
    if (iSliceNum < layerInfo.iNalCount)
      len = layerInfo.pNalLengthInByte[iSliceNum];
  }

 protected:
  SEncParamExt param_;
  BufferedData buf_;
  SBufferInfo dstBufInfo_;
  std::vector<SLostSim> m_SLostSim;
  WelsTraceCallback pFunc;
  STraceUnit sTrace;
  STraceUnit* pTraceInfo;
};

class EncodeDecodeTestAPI : public EncodeDecodeTestBase {
 public:
  void SetUp() {
    EncodeDecodeTestBase::SetUp();
  }

  void TearDown() {
    EncodeDecodeTestBase::TearDown();
  }

  void prepareParam (int width, int height, float framerate) {
    EncodeDecodeTestBase::prepareParam (width, height, framerate);
  }
};

static const EncodeDecodeFileParamBase kFileParamArray[] = {
  {300, 160, 96, 6.0f, 2, 1, "000000000000001010101010101010101010101001101010100000010101000011"},
  {300, 140, 96, 6.0f, 4, 1, "000000000000001010101010101010101010101001101010100000010101000011"},
  {300, 140, 96, 6.0f, 4, 1, "000000000000001010111010101011101010101001101010100000010101110011010101"},
};

TEST_P (EncodeDecodeTestAPI, DecoderVclNal) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  //init for encoder
  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = p.width * p.height * 3 / 2;

  buf_.SetLength (frameSize);
  ASSERT_TRUE (buf_.Length() == (size_t)frameSize);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
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
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    ASSERT_TRUE (rv == cmResultSuccess);
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
  prepareParam (p.width, p.height, p.frameRate);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  //init for encoder
  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = p.width * p.height * 3 / 2;
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
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
  int32_t iEncFrameNum = -1;
  int32_t iDecFrameNum;
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    ASSERT_TRUE (rv == cmResultSuccess);
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
  prepareParam (p.width, p.height, p.frameRate);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  //init for encoder
  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = p.width * p.height * 3 / 2;
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
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
  int32_t iEncCurIdrPicId = 0;
  int32_t iDecCurIdrPicId;
  int32_t iIDRPeriod = 1;
  int32_t iSpsPpsIdAddition = 0;
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
    iSpsPpsIdAddition = rand() % 2;
    iIDRPeriod = (rand() % 150) + 1;
    encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
    encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
    rv = encoder_->EncodeFrame (&pic, &info);
    ASSERT_TRUE (rv == cmResultSuccess);
    if (info.eFrameType == videoFrameTypeIDR) {
      iEncCurIdrPicId = (iSpsPpsIdAddition == 0) ? 0 : (iEncCurIdrPicId + 1);
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


long IsKeyFrameLost (ISVCDecoder* pDecoder, SLTRRecoverRequest* p_LTR_Recover_Request, long hr) {
  long bLost = NO_RECOVERY_REQUSET;
  int tempInt = -1;
  int temple_id = -1;
  bool m_P2PmodeFlag = true;
  pDecoder->GetOption (DECODER_OPTION_TEMPORAL_ID, &temple_id);
  if (hr == dsErrorFree) {
    if (m_P2PmodeFlag && temple_id == 0) {
      pDecoder->GetOption (DECODER_OPTION_IDR_PIC_ID, &tempInt);
      // idr_pic_id change ,reset last correct position
      if (p_LTR_Recover_Request->uiIDRPicId != (unsigned int) tempInt) {
        p_LTR_Recover_Request->uiIDRPicId = tempInt;
        p_LTR_Recover_Request->iLastCorrectFrameNum = -1;
      }
      pDecoder->GetOption (DECODER_OPTION_FRAME_NUM, &tempInt);
      if (tempInt >= 0) {
        p_LTR_Recover_Request->iLastCorrectFrameNum = tempInt;
      }
    }
    bLost = NO_RECOVERY_REQUSET;
  } else if (hr & dsNoParamSets) {
    bLost = IDR_RECOVERY_REQUEST;
  } else if (((hr & dsRefLost) && (1 == temple_id)) || ((dsErrorFree != hr) && (0 == temple_id)))	{
    bLost = LTR_RECOVERY_REQUEST;
  } else {
    bLost = NO_RECOVERY_REQUSET;
  }
  return bLost;
}

bool IsLTRMarking (ISVCDecoder* pDecoder) {
  int bLTR_marking_flag = 0;
  pDecoder->GetOption (DECODER_OPTION_LTR_MARKING_FLAG, &bLTR_marking_flag);
  return (bLTR_marking_flag) ? (true) : (false);
}

void LTRRecoveryRequest (ISVCDecoder* pDecoder, ISVCEncoder* pEncoder, SLTRRecoverRequest* p_LTR_Recover_Request,
                         long hr) {

  long bKLost = IsKeyFrameLost (pDecoder, p_LTR_Recover_Request, hr);
  bool m_P2PmodeFlag = true;
  if (m_P2PmodeFlag) {
    if (bKLost == IDR_RECOVERY_REQUEST) {
      pEncoder->ForceIntraFrame (true);
    } else if (bKLost == LTR_RECOVERY_REQUEST)	{
      p_LTR_Recover_Request->uiFeedbackType = LTR_RECOVERY_REQUEST;
      pDecoder->GetOption (DECODER_OPTION_FRAME_NUM, &p_LTR_Recover_Request->iCurrentFrameNum);
      pDecoder->GetOption (DECODER_OPTION_IDR_PIC_ID, &p_LTR_Recover_Request->uiIDRPicId);
      pEncoder->SetOption (ENCODER_LTR_RECOVERY_REQUEST, p_LTR_Recover_Request);
    }
  } else {
    if (bKLost == IDR_RECOVERY_REQUEST || bKLost == LTR_RECOVERY_REQUEST)	{
      p_LTR_Recover_Request->uiFeedbackType = IDR_RECOVERY_REQUEST;
      pDecoder->GetOption (DECODER_OPTION_FRAME_NUM, &p_LTR_Recover_Request->iCurrentFrameNum);
      pDecoder->GetOption (DECODER_OPTION_IDR_PIC_ID, &p_LTR_Recover_Request->uiIDRPicId);
      pEncoder->SetOption (ENCODER_LTR_RECOVERY_REQUEST, p_LTR_Recover_Request);
    }
  }
}

void LTRMarkFeedback (ISVCDecoder* pDecoder, ISVCEncoder* pEncoder, SLTRMarkingFeedback* p_LTR_Marking_Feedback,
                      long hr) {
  if (IsLTRMarking (pDecoder) == true) {
    p_LTR_Marking_Feedback->uiFeedbackType = (hr == dsErrorFree) ? (LTR_MARKING_SUCCESS) : (LTR_MARKING_FAILED);
    pDecoder->GetOption (DECODER_OPTION_IDR_PIC_ID, &p_LTR_Marking_Feedback->uiIDRPicId);
    pDecoder->GetOption (DECODER_OPTION_LTR_MARKED_FRAME_NUM, &p_LTR_Marking_Feedback->iLTRFrameNum);
    pEncoder->SetOption (ENCODER_LTR_MARKING_FEEDBACK, p_LTR_Marking_Feedback);
  }
}

bool ToRemainDidNal (const unsigned char* pSrc, EWelsNalUnitType eNalType, int iTarDid) {
  uint8_t uiCurByte = *pSrc;
  if (IS_NEW_INTRODUCED_SVC_NAL (eNalType)) {
    int iDid = (uiCurByte & 0x70) >> 4;
    return iDid == iTarDid;
  } else if ((IS_VCL_NAL_AVC_BASE (eNalType)) && iTarDid != 0) {
    return false;
  } else {
    return true;
  }
}

void ExtractDidNal (SFrameBSInfo* pBsInfo, int& iSrcLen, std::vector<SLostSim>* p_SLostSim, int iTarDid) {
  unsigned char* pDst = new unsigned char[iSrcLen];
  const unsigned char* pSrc = pBsInfo->sLayerInfo[0].pBsBuf;
  int iDstLen = 0;
  bool bLost;
  SLostSim tmpSLostSim;
  p_SLostSim->clear();
  int iPrefix;
  unsigned char* pSrcPtr = pBsInfo->sLayerInfo[0].pBsBuf;
  for (int j = 0; j < pBsInfo->iLayerNum; j++) {
    for (int k = 0; k < pBsInfo->sLayerInfo[j].iNalCount; k++) {
      if (pSrcPtr[0] == 0 && pSrcPtr[1] == 0 && pSrcPtr[2] == 0 && pSrcPtr[3] == 1) {
        iPrefix = 4;
      } else if (pSrcPtr[0] == 0 && pSrcPtr[1] == 0 && pSrcPtr[2] == 1) {
        iPrefix = 3;
      } else {
        iPrefix = 0;
      }
      tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrcPtr + iPrefix)) & 0x1f);	// eNalUnitType
      bLost = (ToRemainDidNal ((pSrcPtr + iPrefix + 2), tmpSLostSim.eNalType, iTarDid)) ? false : true;
      tmpSLostSim.isLost = bLost;
      p_SLostSim->push_back (tmpSLostSim);
      if (!bLost) {
        memcpy (pDst + iDstLen, pSrcPtr, pBsInfo->sLayerInfo[j].pNalLengthInByte[k]);
        iDstLen += (pBsInfo->sLayerInfo[j].pNalLengthInByte[k]);
      }
      pSrcPtr += pBsInfo->sLayerInfo[j].pNalLengthInByte[k];
    }
  }
  memset ((void*)pSrc, 0, iSrcLen);
  memcpy ((void*)pSrc, pDst, iDstLen);
  iSrcLen = iDstLen;
  delete [] pDst;
}

int SimulateNALLoss (const unsigned char* pSrc,  int& iSrcLen, std::vector<SLostSim>* p_SLostSim,
                     const char* pLossChars, bool bLossPara, int& iLossIdx, bool& bVCLLoss) {
  unsigned char* pDst = new unsigned char[iSrcLen];
  int iLossCharLen = strlen (pLossChars);
  int iSkipedBytes = 0;
  int iDstLen = 0;
  int iBufPos = 0;
  int ilastprefixlen = 0;
  int i = 0;
  bool bLost;
  bVCLLoss = false;
  SLostSim tmpSLostSim;
  p_SLostSim->clear();
  for (i = 0; i < iSrcLen;) {
    if (pSrc[i] == 0 && pSrc[i + 1] == 0 && pSrc[i + 2] == 0 && pSrc[i + 3] == 1) {
      if (i - iBufPos) {
        tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrc + iBufPos + ilastprefixlen)) & 0x1f);	// eNalUnitType
        bLost = iLossIdx < iLossCharLen ? (pLossChars[iLossIdx] == '1') : (rand() % 2 == 1);
        bLost = (!bLossPara) && (IS_PARAM_SETS_NALS (tmpSLostSim.eNalType)) ? false : bLost;
        iLossIdx++;
        tmpSLostSim.isLost = bLost;
        p_SLostSim->push_back (tmpSLostSim);
        if (!bLost) {
          memcpy (pDst + iDstLen, pSrc + iBufPos, i - iBufPos);
          iDstLen += (i - iBufPos);
        } else {
          bVCLLoss = (IS_VCL_NAL (tmpSLostSim.eNalType, 1)) ? true : bVCLLoss;
          iSkipedBytes += (i - iBufPos);
        }
      }
      ilastprefixlen = 4;
      iBufPos = i;
      i = i + 4;
    } else if (pSrc[i] == 0 && pSrc[i + 1] == 0 && pSrc[i + 2] == 1) {
      if (i - iBufPos) {
        tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrc + iBufPos + ilastprefixlen)) & 0x1f);	// eNalUnitType
        bLost = iLossIdx < iLossCharLen ? (pLossChars[iLossIdx] == '1') : (rand() % 2 == 1);
        bLost = (!bLossPara) && (IS_PARAM_SETS_NALS (tmpSLostSim.eNalType)) ? false : bLost;
        iLossIdx++;
        tmpSLostSim.isLost = bLost;
        p_SLostSim->push_back (tmpSLostSim);
        if (!bLost) {
          memcpy (pDst + iDstLen, pSrc + iBufPos, i - iBufPos);
          iDstLen += (i - iBufPos);
        } else {
          bVCLLoss = (IS_VCL_NAL (tmpSLostSim.eNalType, 1)) ? true : bVCLLoss;
          iSkipedBytes += (i - iBufPos);
        }
      }
      ilastprefixlen = 3;
      iBufPos = i;
      i = i + 3;
    } else {
      i++;
    }
  }
  if (i - iBufPos) {
    tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrc + iBufPos + ilastprefixlen)) & 0x1f);	// eNalUnitType
    bLost = iLossIdx < iLossCharLen ? (pLossChars[iLossIdx] == '1') : (rand() % 2 == 1);
    bLost = (!bLossPara) && (IS_PARAM_SETS_NALS (tmpSLostSim.eNalType)) ? false : bLost;
    iLossIdx++;
    tmpSLostSim.isLost = bLost;
    p_SLostSim->push_back (tmpSLostSim);
    if (!bLost) {
      memcpy (pDst + iDstLen, pSrc + iBufPos, i - iBufPos);
      iDstLen += (i - iBufPos);
    } else {
      bVCLLoss = (IS_VCL_NAL (tmpSLostSim.eNalType, 1)) ? true : bVCLLoss;
      iSkipedBytes += (i - iBufPos);
    }
  }
  memset ((void*)pSrc, 0, iSrcLen);
  memcpy ((void*)pSrc, pDst, iDstLen);
  iSrcLen = iDstLen;
  delete [] pDst;
  return iSkipedBytes;
}

TEST_P (EncodeDecodeTestAPI, GetOptionLTR_ALLIDR) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
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
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    ASSERT_EQ (rv, cmResultSuccess);
    ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    encoder_->ForceIntraFrame (true);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionLTR_ALLLTR) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
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
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
    m_LTR_Recover_Request.uiFeedbackType = LTR_RECOVERY_REQUEST;
    m_LTR_Recover_Request.iCurrentFrameNum = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    m_LTR_Recover_Request.uiIDRPicId = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
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
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;
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
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx, bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, SetOptionECFlag_ERROR_CON_DISABLE) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;
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
  decParam.eOutputColorFormat  = videoFormatI420;
  decParam.uiTargetDqLayer = UCHAR_MAX;
  decParam.eEcActiveIdc = ERROR_CON_DISABLE;
  decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
  rv = decoder_->Initialize (&decParam);
  ASSERT_EQ (0, rv);
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
  int iSkipedBytes;
  bool bVCLLoss = false;
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    iSkipedBytes = SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx,
                                    bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
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
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;
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
  int iSkipedBytes;
  bool bVCLLoss = false;
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
  (void) iSkipedBytes;
}

TEST_P (EncodeDecodeTestAPI, GetOptionTid_AVC_NOPREFIX) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
  param_.bPrefixNalAddingCtrl = false;
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;
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
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    std::vector<SLostSim>::iterator iter = m_SLostSim.begin();
    bool bHasVCL = false;
    for (int k = 0; k < m_SLostSim.size(); k++) {
      if (IS_VCL_NAL (iter->eNalType, 0) && iter->isLost == false) {
        bHasVCL = true;
        break;
      }
      iter++;
    }
    if (iTid != -1) {
      ASSERT_EQ (iTid, 0);
    }
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionTid_AVC_WITH_PREFIX_NOLOSS) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
  param_.bPrefixNalAddingCtrl = true;
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 1;
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;
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
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, info.sLayerInfo[0].uiTemporalId);
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionTid_SVC_L1_NOLOSS) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width / 2, p.height / 2, p.frameRate);
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 2;
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;
  param_.sSpatialLayers[1].iVideoWidth = p.width;
  param_.sSpatialLayers[1].iVideoHeight = p.height;
  param_.sSpatialLayers[1].fFrameRate = p.frameRate;
  param_.sSpatialLayers[1].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[1].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;

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
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, info.sLayerInfo[0].uiTemporalId);
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}



TEST_P (EncodeDecodeTestAPI, SetOption_Trace) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
  param_.iSpatialLayerNum = 1;
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;

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
  pFunc = TestOutPutTrace;
  pTraceInfo = &sTrace;
  sTrace.iTarLevel = iTraceLevel;
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
    memset (buf_.data(), rand() % 256, frameSize);
    iTraceLevel = rand() % 33;
    sTrace.iTarLevel = iTraceLevel;
    encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    ExtractDidNal (&info, len, &m_SLostSim, 0);
    SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx, bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, SetOption_Trace_NULL) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
  param_.iSpatialLayerNum = 1;
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;

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
    memset (buf_.data(), rand() % 256, frameSize);
    iTraceLevel = rand() % 33;
    encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    ExtractDidNal (&info, len, &m_SLostSim, 0);
    SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx, bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

INSTANTIATE_TEST_CASE_P (EncodeDecodeTestBase, EncodeDecodeTestAPI,
                         ::testing::ValuesIn (kFileParamArray));

TEST_P (EncodeDecodeTestAPI, SetOptionECIDC_GeneralSliceChange) {
  uint32_t uiEcIdc;
  uint32_t uiGet;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParam (p.width, p.height, p.frameRate);
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;
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
  int iIdx = 0;
  bool bVCLLoss = false;
  int iPacketNum;
  int len;
  int iTotalSliceSize;

  //enc/dec pictures
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
  prepareParam (p.width, p.height, p.frameRate);
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = p.slicenum;
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
  int iIdx = 0;
  int len = 0;
  unsigned char* pData[3] = { NULL };

  //Frame 0: IDR, EC_IDC=DISABLE, loss = 0
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  iIdx++;

  //Frame 2: P, EC_IDC=DISABLE, loss = 0
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  encToDecData (info, len);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  if (rv == 0) //TODO: should depend on if ref-frame is OK.
    EXPECT_EQ (dstBufInfo_.iBufferStatus, 1);
  else
    EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  iIdx++;

  //Frame 5: P, EC_IDC=DISABLE, loss = 1
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  iIdx++;

  //set EC=FRAME_COPY
  uiEcIdc = (uint32_t) (ERROR_CON_FRAME_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 6: P, EC_IDC=FRAME_COPY, loss = 1
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  EXPECT_EQ (uiGet, uiEcIdc);
  iIdx++;

  //Frame 7: P, EC_IDC=FRAME_COPY, loss = 0
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  encToDecData (info, len);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_FRAME_COPY);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv != 0); //parse correct, but previous decoding error, ECed
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
  uint32_t uiEcIdc;
  uint32_t uiGet;
  EncodeDecodeFileParamBase p = kFileParamArray[0];
  prepareParam (p.width, p.height, p.frameRate);
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = 2;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  //Start for enc/dec
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
  int iIdx = 0;
  int len = 0;
  unsigned char* pData[3] = { NULL };
  int iTotalSliceSize = 0;

  //Frame 0: IDR, EC_IDC=2, loss = 2
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //slice incomplete, no output
  iIdx++;

  //Frame 1: P, EC_IDC=2, loss = 0
  //will clean SPS/PPS status
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  encToDecData (info, len); //all slice together
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv & 32); //parse correct, but reconstruct ECed
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //ECed output for frame 0
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //ECed status, reconstruction current frame 1
  EXPECT_TRUE (rv & 32); //decoder ECed status
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //ECed output for frame 1
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 2: P, EC_IDC=0, loss = 0
  /////will clean SPS/PPS status
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  encToDecData (info, len); //all slice together
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  if (rv == 0) //TODO: should depend on if ref-frame is OK.
    EXPECT_EQ (dstBufInfo_.iBufferStatus, 1);
  else
    EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  iIdx++;

  //set EC=SLICE_COPY
  uiEcIdc = (uint32_t) (ERROR_CON_FRAME_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 3: P, EC_IDC=2, loss = 1
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  encToDecSliceData (0, 0, info, iTotalSliceSize); //slice 1 lost
  encToDecSliceData (0, 1, info, len); //slice 2
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_FRAME_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf + iTotalSliceSize, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv & 32);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv & 32);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //slice loss
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 4: P, EC_IDC=0, loss = 0
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  encToDecData (info, len); //all slice
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //output previous pic
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //output previous pic
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
  prepareParam (p.width, p.height, p.frameRate);
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = 2;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  //Start for enc/dec
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
  int iIdx = 0;
  int len = 0;
  unsigned char* pData[3] = { NULL };
  int iTotalSliceSize = 0;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 0: IDR, EC_IDC=0, loss = 0
  //Expected result: all OK, 2nd Output
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  encToDecSliceData (0, 0, info, len); //slice 1 only
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv & 32); //ECed
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //slice loss but ECed output Frame 2
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv & 32);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //slice loss
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 4: P, EC_IDC=0, loss = 0
  //Expected result: depends on DecodeFrame2 result. If OK, output; else ,no output
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  encToDecData (info, len); //all slice
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv != 0); //previous slice not outputted, will return error due to incomplete frame
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //output previous pic
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction,
  //not sure if current frame can be correctly decoded
  if (rv == 0)
    EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //output previous pic
  else
    EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //output previous pic
  iIdx++;

  //Frame 5: IDR, EC_IDC=2->0, loss = 0
  //Expected result: depends on DecodeFrame2 result. If OK, output; else ,no output
  int32_t iIDRPeriod = 1;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  memset (buf_.data(), rand() % 256, frameSize);
  rv = encoder_->EncodeFrame (&pic, &info);
  ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
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
  EXPECT_TRUE (rv != 0); //TODO: should be correct, now ECed status will return error
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //frame incomplete
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction,
  EXPECT_TRUE (rv != 0); //TODO: as above
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
  EXPECT_EQ (rv, 0); //Parse correct under no EC
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //output previous pic
  iIdx++;

}

