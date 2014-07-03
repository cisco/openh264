#include<gtest/gtest.h>
#include<stdlib.h>
#include "codec_api.h"
#include "welsEncoderExt.h"

using namespace WelsSVCEnc;

class EncoderInterfaceTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    pPtrEnc = new CWelsH264SVCEncoder();
    pParamExt = new SEncParamExt();
    pSrcPic = new SSourcePicture;
    pOption = new SEncParamExt();
    pYUV = NULL;

    m_iWidth = 1280;
    m_iHeight = 720;
    m_iPicResSize =  m_iWidth * m_iHeight * 3 >> 1;
    pYUV = new uint8_t [m_iPicResSize];
  }
  void TemporalLayerSettingTest();
  virtual void TearDown() {
    delete pPtrEnc;
    delete pParamExt;
    delete pOption;
    delete pSrcPic;
    delete []pYUV;
  }
 public:
  CWelsH264SVCEncoder* pPtrEnc;
  SEncParamExt* pParamExt;
  SSourcePicture* pSrcPic;
  SEncParamExt* pOption;
  uint8_t* pYUV;

  int32_t m_iWidth;
  int32_t m_iHeight;
  int32_t m_iPicResSize;
};

void EncoderInterfaceTest::TemporalLayerSettingTest() {

  pParamExt->iInputCsp = 23;
  pParamExt->iPicWidth = m_iWidth;
  pParamExt->iPicHeight = m_iHeight;
  pParamExt->iTargetBitrate = 60000;
  pParamExt->sSpatialLayers[0].iVideoHeight = pParamExt->iPicHeight;
  pParamExt->sSpatialLayers[0].iVideoWidth = pParamExt->iPicWidth;
  pParamExt->sSpatialLayers[0].iSpatialBitrate = 50000;
  pParamExt->iTemporalLayerNum = 1;
  pParamExt->iSpatialLayerNum = 1;

  int32_t iResult = pPtrEnc->InitializeExt (pParamExt);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));

  pSrcPic->iColorFormat = videoFormatI420;
  pSrcPic->uiTimeStamp = 0;
  pSrcPic->iPicWidth = pParamExt->iPicWidth;
  pSrcPic->iPicHeight = pParamExt->iPicHeight;

  for (int32_t i = 0; i < m_iPicResSize; i++)
    pYUV[i] = rand() % 256;

  pSrcPic->iStride[0] = m_iWidth;
  pSrcPic->iStride[1] = pSrcPic->iStride[2] = pSrcPic->iStride[0] >> 1;

  pSrcPic->pData[0] = pYUV;
  pSrcPic->pData[1] = pSrcPic->pData[0] + (m_iWidth * m_iHeight);
  pSrcPic->pData[2] = pSrcPic->pData[1] + (m_iWidth * m_iHeight >> 2);

  SFrameBSInfo sFbi;
  memset (&sFbi, 0, sizeof (SFrameBSInfo));

  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int32_t> (videoFrameTypeIDR));

  pSrcPic->uiTimeStamp = 30;
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int32_t> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int32_t> (videoFrameTypeP));

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

}

TEST_F (EncoderInterfaceTest, TestTemporalLayerSetting) {
  TemporalLayerSettingTest();
}