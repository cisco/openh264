#include<gtest/gtest.h>
#include<stdlib.h>
#include "codec_api.h"
#include "welsEncoderExt.h"

using namespace WelsSVCEnc;

TEST (EncoderExtTest, SetOption) {
  CWelsH264SVCEncoder* pPtrEnc = new CWelsH264SVCEncoder();
  SEncParamExt* pParamExt = new SEncParamExt();

  pParamExt->iInputCsp = 23;
  pParamExt->iPicWidth = 1280;
  pParamExt->iPicHeight = 720;
  pParamExt->iTargetBitrate = 60000;
  pParamExt->sSpatialLayers[0].iVideoHeight = pParamExt->iPicHeight;
  pParamExt->sSpatialLayers[0].iVideoWidth = pParamExt->iPicWidth;
  pParamExt->sSpatialLayers[0].iSpatialBitrate = 50000;
  pParamExt->iTemporalLayerNum = 1;
  pParamExt->iSpatialLayerNum = 1;

  int32_t iResult = pPtrEnc->InitializeExt (pParamExt);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));

  SSourcePicture* pSrcPic = new SSourcePicture;
  pSrcPic->iColorFormat = videoFormatI420;
  pSrcPic->uiTimeStamp = 0;
  uint32_t iSourceWidth, iSourceHeight, kiPicResSize;

  iSourceWidth = pSrcPic->iPicWidth = pParamExt->iPicWidth;
  iSourceHeight = pSrcPic->iPicHeight = pParamExt->iPicHeight;
  kiPicResSize = iSourceWidth * iSourceHeight * 3 >> 1;

  uint8_t* pYUV = NULL;
  pYUV = new uint8_t [kiPicResSize];
  if (pYUV == NULL){
    delete pPtrEnc;
    delete pParamExt;
    delete pSrcPic;
    return;
  }

  for(int32_t i=0;i<kiPicResSize;i++)
    pYUV[i]=rand()%256;

  pSrcPic->iStride[0] = iSourceWidth;
  pSrcPic->iStride[1] = pSrcPic->iStride[2] = pSrcPic->iStride[0] >> 1;

  pSrcPic->pData[0] = pYUV;
  pSrcPic->pData[1] = pSrcPic->pData[0] + (iSourceWidth * iSourceHeight);
  pSrcPic->pData[2] = pSrcPic->pData[1] + (iSourceWidth * iSourceHeight >> 2);

  SFrameBSInfo sFbi;
  memset (&sFbi, 0, sizeof (SFrameBSInfo));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int32_t> (videoFrameTypeInvalid));

  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int32_t> (videoFrameTypeIDR));

  pSrcPic->uiTimeStamp = 30;
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int32_t> (videoFrameTypeP));

  SEncParamExt* pOption = new SEncParamExt();
  memcpy (pOption, pParamExt, sizeof (SEncParamExt));
  pOption ->iTemporalLayerNum = 4;

  ENCODER_OPTION eOptionId = ENCODER_OPTION_SVC_ENCODE_PARAM_EXT;
  iResult = pPtrEnc->SetOption (eOptionId, pOption);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));

  pSrcPic->uiTimeStamp = 60;
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int32_t> (videoFrameTypeIDR));
 
  pOption ->iTemporalLayerNum = 2;
  iResult = pPtrEnc->SetOption (eOptionId, pOption);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));
  pSrcPic->uiTimeStamp = 90;
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int32_t> (videoFrameTypeP));

  pOption ->iTemporalLayerNum = 4;
  iResult = pPtrEnc->SetOption (eOptionId, pOption);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));
  pSrcPic->uiTimeStamp = 120;
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int32_t> (videoFrameTypeP));

  pPtrEnc->Uninitialize();
  delete pPtrEnc;
  delete pParamExt;
  delete pOption;
  delete pSrcPic;
  delete []pYUV;
 }
