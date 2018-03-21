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


void EncodeDecodeTestBase::prepareParam (int iLayers, int iSlices, int width, int height, float framerate,
    SEncParamExt* pParam) {
  pParam->iUsageType = CAMERA_VIDEO_REAL_TIME;
  pParam->iPicWidth = width;
  pParam->iPicHeight = height;
  pParam->fMaxFrameRate = framerate;
  pParam->iRCMode = RC_OFF_MODE; //rc off
  pParam->iMultipleThreadIdc = 1; //single thread
  pParam->iSpatialLayerNum = iLayers;
  pParam->iNumRefFrame = AUTO_REF_PIC_COUNT;
  for (int i = 0; i < iLayers; i++) {
    pParam->sSpatialLayers[i].iVideoWidth = width >> (iLayers - i - 1);
    pParam->sSpatialLayers[i].iVideoHeight = height >> (iLayers - i - 1);
    pParam->sSpatialLayers[i].fFrameRate = framerate;
    pParam->sSpatialLayers[i].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
    pParam->sSpatialLayers[i].sSliceArgument.uiSliceNum = iSlices;
  }
}

bool EncodeDecodeTestBase::prepareEncDecParam (const EncodeDecodeFileParamBase EncDecFileParam) {
  //for encoder
  //I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = EncDecFileParam.width * EncDecFileParam.height * 3 / 2;
  buf_.SetLength (frameSize);
  if (buf_.Length() != (size_t)frameSize) {
    printf ("buf_.Length() failed! frameSize = %d\n", frameSize);
    return false;
  }
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
  return true;
}

void EncodeDecodeTestBase::encToDecData (const SFrameBSInfo& info, int& len) {
  len = 0;
  for (int i = 0; i < info.iLayerNum; ++i) {
    const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
    for (int j = 0; j < layerInfo.iNalCount; ++j) {
      len += layerInfo.pNalLengthInByte[j];
    }
  }
}

void EncodeDecodeTestBase::encToDecSliceData (const int iLayerNum, const int iSliceNum, const SFrameBSInfo& info,
    int& len) {
  ASSERT_TRUE (iLayerNum < MAX_LAYER_NUM_OF_FRAME);
  len = 0;
  const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayerNum];
  if (iSliceNum < layerInfo.iNalCount)
    len = layerInfo.pNalLengthInByte[iSliceNum];
}

void EncodeDecodeTestAPIBase::prepareParam0 (int iLayers, int iSlices, int width, int height, float framerate,
    SEncParamExt* pParam) {
  memset (pParam, 0, sizeof (SEncParamExt));
  EncodeDecodeTestBase::prepareParam (iLayers, iSlices, width, height, framerate, pParam);
}

void EncodeDecodeTestAPIBase::prepareParamDefault (int iLayers, int iSlices, int width, int height, float framerate,
    SEncParamExt* pParam) {
  memset (pParam, 0, sizeof (SEncParamExt));
  encoder_->GetDefaultParams (pParam);
  EncodeDecodeTestBase::prepareParam (iLayers, iSlices, width, height, framerate, pParam);
}


void EncodeDecodeTestAPIBase::EncodeOneFrame (int iCheckTypeIndex) {
  int frameSize = EncPic.iPicWidth * EncPic.iPicHeight * 3 / 2;
  int lumaSize = EncPic.iPicWidth * EncPic.iPicHeight;
  memset (buf_.data(), iRandValue, lumaSize);
  memset (buf_.data() + lumaSize, rand() % 256, (frameSize - lumaSize));
  int rv = encoder_->EncodeFrame (&EncPic, &info);
  if (0 == iCheckTypeIndex)
    ASSERT_TRUE (rv == cmResultSuccess) << rv;
  else if (1 == iCheckTypeIndex)
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnknownReason);
}

bool EncodeDecodeTestAPIBase::EncDecOneFrame (const int iWidth, const int iHeight, const int iFrame, FILE* pfEnc) {
  int iLen = 0, rv;
  if (!InitialEncDec (iWidth, iHeight))
    return false;
  EncodeOneFrame (iFrame);

  //extract target layer data
  encToDecData (info, iLen);
  //call decoder
  unsigned char* pData[3] = { NULL };
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrameNoDelay (info.sLayerInfo[0].pBsBuf, iLen, pData, &dstBufInfo_);
  EXPECT_TRUE (rv == cmResultSuccess) << " rv = " << rv << " iFrameIdx = " << iFrame;
  if (NULL != pfEnc) {
    fwrite (info.sLayerInfo[0].pBsBuf, iLen, 1, pfEnc);
  }
  return true;
}

bool EncodeDecodeTestAPIBase::TestOneSimulcastAVC (SEncParamExt* pParam, ISVCDecoder** decoder, unsigned char** pBsBuf,
    int iSpatialLayerNum,
    int iEncFrameNum,
    int iSaveFileIdx) {
  int aLen[MAX_SPATIAL_LAYER_NUM] = {0, 0, 0, 0};

  FILE* fEnc[MAX_SPATIAL_LAYER_NUM];
  if (iSaveFileIdx == 1) {
    fEnc[0] = fopen ("enc00.264", "wb");
    fEnc[1] = fopen ("enc01.264", "wb");
    fEnc[2] = fopen ("enc02.264", "wb");
    fEnc[3] = fopen ("enc03.264", "wb");
  } else if (iSaveFileIdx > 1) {
    fEnc[0] = fopen ("enc10.264", "wb");
    fEnc[1] = fopen ("enc11.264", "wb");
    fEnc[2] = fopen ("enc12.264", "wb");
    fEnc[3] = fopen ("enc13.264", "wb");
  }

  int rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, pParam);
  if (rv != cmResultSuccess) {
    printf ("SetOption Failed, rv = %d\n", rv);
    return false;
  }

  int iIdx;
  //begin testing
  for (int iFrame = 0; iFrame < iEncFrameNum; iFrame++) {
    int iResult;
    int iLayerLen = 0;
    unsigned char* pData[3] = { NULL };

    if (!InitialEncDec (pParam->iPicWidth, pParam->iPicHeight))
      return false;
    EncodeOneFrame (0);

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

      if (iSaveFileIdx > 0) {
        fwrite (pBsBuf[iIdx], aLen[iIdx], 1, fEnc[iIdx]);
      }

      iResult = decoder[iIdx]->DecodeFrame2 (pBsBuf[iIdx], aLen[iIdx], pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << ", LayerIdx=" << iIdx;

      iResult = decoder[iIdx]->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << ", LayerIdx=" << iIdx;
      EXPECT_EQ (dstBufInfo_.iBufferStatus, 1) << "LayerIdx=" << iIdx;
    }
  }

  if (iSaveFileIdx > 0) {
    fclose (fEnc[0]);
    fclose (fEnc[1]);
    fclose (fEnc[2]);
    fclose (fEnc[3]);
  }
  return true;
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
  } else if (((hr & dsRefLost) && (1 == temple_id)) || ((dsErrorFree != hr) && (0 == temple_id))) {
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
                         long hr, bool m_P2PmodeFlag) {
  long bKLost = IsKeyFrameLost (pDecoder, p_LTR_Recover_Request, hr);
  if (m_P2PmodeFlag) {
    if (bKLost == IDR_RECOVERY_REQUEST) {
      pEncoder->ForceIntraFrame (true);
    } else if (bKLost == LTR_RECOVERY_REQUEST) {
      p_LTR_Recover_Request->uiFeedbackType = LTR_RECOVERY_REQUEST;
      pDecoder->GetOption (DECODER_OPTION_FRAME_NUM, &p_LTR_Recover_Request->iCurrentFrameNum);
      pDecoder->GetOption (DECODER_OPTION_IDR_PIC_ID, &p_LTR_Recover_Request->uiIDRPicId);
      pEncoder->SetOption (ENCODER_LTR_RECOVERY_REQUEST, p_LTR_Recover_Request);
    }
  } else {
    if (bKLost == IDR_RECOVERY_REQUEST || bKLost == LTR_RECOVERY_REQUEST) {
      p_LTR_Recover_Request->uiFeedbackType = IDR_RECOVERY_REQUEST;
      pEncoder->ForceIntraFrame (true);
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
      tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrcPtr + iPrefix)) & 0x1f); // eNalUnitType
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
  int iLossCharLen = (int) strlen (pLossChars);
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
        tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrc + iBufPos + ilastprefixlen)) & 0x1f); // eNalUnitType
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
        tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrc + iBufPos + ilastprefixlen)) & 0x1f); // eNalUnitType
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
    tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrc + iBufPos + ilastprefixlen)) & 0x1f); // eNalUnitType
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


