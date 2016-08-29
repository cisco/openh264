#include <gtest/gtest.h>
#include <stdlib.h>
#include "codec_api.h"
#include "codec_app_def.h"
#include "svc_enc_slice_segment.h"
#include "test_stdint.h"
#include "utils/FileInputStream.h"
//TODO: consider using BaseEncoderTest class from #include "../BaseEncoderTest.h"

class EncoderInterfaceTest : public ::testing::Test {
#define MB_SIZE (16)
#define MAX_WIDTH (3840)
#define MAX_HEIGHT (2160)
#define VALID_SIZE(iSize) (((iSize)>16)?(iSize):16)
#define MEM_VARY_SIZE (512)
#define IMAGE_VARY_SIZE (512)
#define TEST_FRAMES (30)

#define NAL_HEADER_BYTES (4)
#define NAL_TYPE (0x0F)
#define SPS_NAL_TYPE (7)
#define PPS_NAL_TYPE (8)
#define SUBSETSPS_NAL_TYPE (15)
#define GET_NAL_TYPE(pNalStart) (*(pNalStart+NAL_HEADER_BYTES) & NAL_TYPE)
#define IS_PARASET(iNalType) ((iNalType==SPS_NAL_TYPE) || (iNalType==PPS_NAL_TYPE) || (iNalType==SUBSETSPS_NAL_TYPE))

 public:
  virtual void SetUp() {
    int rv = WelsCreateSVCEncoder (&pPtrEnc);
    ASSERT_EQ (0, rv);
    ASSERT_TRUE (pPtrEnc != NULL);

    unsigned int uiTraceLevel = WELS_LOG_ERROR;
    pPtrEnc->SetOption (ENCODER_OPTION_TRACE_LEVEL, &uiTraceLevel);

    pParamExt = new SEncParamExt();
    ASSERT_TRUE (pParamExt != NULL);

    pSrcPic = new SSourcePicture;
    ASSERT_TRUE (pSrcPic != NULL);

    pOption = new SEncParamExt();
    ASSERT_TRUE (pOption != NULL);

    pYUV = NULL;
    m_iWidth = MAX_WIDTH;
    m_iHeight = MAX_HEIGHT;
    m_iPicResSize =  m_iWidth * m_iHeight * 3 >> 1;
    pYUV = new unsigned char [m_iPicResSize];
    ASSERT_TRUE (pYUV != NULL);
  }

  virtual void TearDown() {
    delete pParamExt;
    delete pOption;
    delete pSrcPic;
    delete []pYUV;
    if (pPtrEnc) {
      WelsDestroySVCEncoder (pPtrEnc);
      pPtrEnc = NULL;
    }
  }

  void InitializeParamExt();
  void SetRandomParams();
  void TemporalLayerSettingTest();
  void MemoryCheckTest();
  void EncodeOneFrame (SEncParamBase* pEncParamBase);
  void PrepareOneSrcFrame();
  void EncodeOneIDRandP (ISVCEncoder* pPtrEnc);
  void ChangeResolutionAndCheckStatistics (const SEncParamBase& sEncParamBase, SEncoderStatistics* pEncoderStatistics);

 public:
  ISVCEncoder* pPtrEnc;
  SEncParamExt* pParamExt;
  SSourcePicture* pSrcPic;
  SEncParamExt* pOption;
  unsigned char* pYUV;
  SFrameBSInfo sFbi;

  int m_iWidth;
  int m_iHeight;
  int m_iPicResSize;
};

void EncoderInterfaceTest::PrepareOneSrcFrame() {
  pSrcPic->iColorFormat = videoFormatI420;
  pSrcPic->uiTimeStamp = 0;
  pSrcPic->iPicWidth = pParamExt->iPicWidth;
  pSrcPic->iPicHeight = pParamExt->iPicHeight;
  pSrcPic->iPicWidth = VALID_SIZE (pParamExt->iPicWidth);
  pSrcPic->iPicHeight = VALID_SIZE (pParamExt->iPicHeight);
  m_iWidth = pSrcPic->iPicWidth;
  m_iHeight = pSrcPic->iPicHeight;
  m_iPicResSize =  m_iWidth * m_iHeight * 3 >> 1;

  pYUV[0] = rand() % 256;
  for (int i = 1; i < m_iPicResSize; i++) {
    if ((i % 256) == 0)
      pYUV[i] = rand() % 256;
    else
      pYUV[i] = (pYUV[i - 1] + (rand() % 3) - 1) & 0xff;
  }
  pSrcPic->iStride[0] = m_iWidth;
  pSrcPic->iStride[1] = pSrcPic->iStride[2] = pSrcPic->iStride[0] >> 1;

  pSrcPic->pData[0] = pYUV;
  pSrcPic->pData[1] = pSrcPic->pData[0] + (m_iWidth * m_iHeight);
  pSrcPic->pData[2] = pSrcPic->pData[1] + (m_iWidth * m_iHeight >> 2);

  memset (&sFbi, 0, sizeof (SFrameBSInfo));
}

void EncoderInterfaceTest::EncodeOneFrame (SEncParamBase* pEncParamBase) {
  int iResult;
  iResult = pPtrEnc->Initialize (pEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  PrepareOneSrcFrame();
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));

  iResult = pPtrEnc->GetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_BASE, pEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

void EncoderInterfaceTest::EncodeOneIDRandP (ISVCEncoder* pPtrEnc) {
  int iResult;
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));

  pSrcPic->uiTimeStamp += 30;
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

void EncoderInterfaceTest::InitializeParamExt() {
  pParamExt->iPicWidth = 1280;
  pParamExt->iPicHeight = 720;
  pParamExt->iTargetBitrate = 50000;
  pParamExt->iTemporalLayerNum = 3;
  pParamExt->iSpatialLayerNum = 1;
  pParamExt->iNumRefFrame = AUTO_REF_PIC_COUNT;
  pParamExt->sSpatialLayers[0].iVideoHeight = pParamExt->iPicHeight;
  pParamExt->sSpatialLayers[0].iVideoWidth = pParamExt->iPicWidth;
  pParamExt->sSpatialLayers[0].iSpatialBitrate = 50000;
}

TEST_F (EncoderInterfaceTest, EncoderOptionSetTest) {
  int iResult, iValue, iReturn;
  float fFrameRate, fReturn;
  int uiTraceLevel = WELS_LOG_QUIET;

  pPtrEnc->SetOption (ENCODER_OPTION_TRACE_LEVEL, &uiTraceLevel);

  InitializeParamExt();
  iResult = pPtrEnc->InitializeExt (pParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  PrepareOneSrcFrame();

  ENCODER_OPTION eOptionId = ENCODER_OPTION_DATAFORMAT;
  iValue = rand() % 256;
  iResult = pPtrEnc->SetOption (eOptionId, &iValue);

  if (iValue == 0)
    EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));
  else
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  iResult = pPtrEnc->GetOption (eOptionId, &iReturn);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (iValue, iReturn);

  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  pSrcPic->uiTimeStamp += 30;

  eOptionId = ENCODER_OPTION_IDR_INTERVAL;
  iValue = rand() % 256 - 5;
  iResult = pPtrEnc->SetOption (eOptionId, &iValue);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  iResult = pPtrEnc->GetOption (eOptionId, &iReturn);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  if (iValue < -1 || iValue == 0)
    iValue = 1;
  EXPECT_EQ (iValue, iReturn);

  PrepareOneSrcFrame();
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  pSrcPic->uiTimeStamp += 30;

  eOptionId = ENCODER_OPTION_FRAME_RATE;
  fFrameRate = static_cast<float> (rand() % 100 - 5);
  iResult = pPtrEnc->SetOption (eOptionId, &fFrameRate);

  if (fFrameRate <= 0)
    EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));
  else {
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

    iResult = pPtrEnc->GetOption (eOptionId, &fReturn);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    EXPECT_EQ (WELS_CLIP3 (fFrameRate, 1, 60), fReturn);
  }
  PrepareOneSrcFrame();
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  pSrcPic->uiTimeStamp += 30;

  eOptionId = ENCODER_OPTION_BITRATE;
  SBitrateInfo sInfo, sReturn;
  sInfo.iBitrate = rand() % 100000 - 100;
  sInfo.iLayer = SPATIAL_LAYER_0;
  iResult = pPtrEnc->SetOption (eOptionId, &sInfo);
  pPtrEnc->GetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, pParamExt);
  if (sInfo.iBitrate <= 0)
    EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));
  else if (pParamExt->sSpatialLayers[sInfo.iLayer].iSpatialBitrate >
           pParamExt->sSpatialLayers[sInfo.iLayer].iMaxSpatialBitrate) {
    EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));
  } else {
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    sReturn.iLayer = SPATIAL_LAYER_0;
    iResult = pPtrEnc->GetOption (eOptionId, &sReturn);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    EXPECT_EQ (WELS_CLIP3 (sInfo.iBitrate, 1, 2147483647), sReturn.iBitrate);
  }
  PrepareOneSrcFrame();
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  pSrcPic->uiTimeStamp += 30;

  eOptionId = ENCODER_OPTION_MAX_BITRATE;
  sInfo.iBitrate = rand() % 100000 - 100;
  sInfo.iLayer = SPATIAL_LAYER_0;
  iResult = pPtrEnc->SetOption (eOptionId, &sInfo);
  pPtrEnc->GetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, pParamExt);
  if (sInfo.iBitrate <= 0 || pParamExt->sSpatialLayers[sInfo.iLayer].iSpatialBitrate <= (int) (fFrameRate + 0.5f))
    EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));
  else if (pParamExt->sSpatialLayers[sInfo.iLayer].iSpatialBitrate >
           pParamExt->sSpatialLayers[sInfo.iLayer].iMaxSpatialBitrate) {
    EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));
  } else {
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    sReturn.iLayer = SPATIAL_LAYER_0;
    iResult = pPtrEnc->GetOption (eOptionId, &sReturn);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    EXPECT_EQ (WELS_CLIP3 (sInfo.iBitrate, 1, 2147483647), sReturn.iBitrate);
  }
  PrepareOneSrcFrame();
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  pSrcPic->uiTimeStamp += 30;

  eOptionId = ENCODER_OPTION_RC_MODE;
  iValue = (rand() % 4) - 1;
  iResult = pPtrEnc->SetOption (eOptionId, &iValue);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  PrepareOneSrcFrame();
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  pSrcPic->uiTimeStamp += 30;

  uiTraceLevel = WELS_LOG_ERROR;
  pPtrEnc->SetOption (ENCODER_OPTION_TRACE_LEVEL, &uiTraceLevel);
}

TEST_F (EncoderInterfaceTest, TemporalLayerSettingTest) {

  pParamExt->iPicWidth = 1280;
  pParamExt->iPicHeight = 720;
  m_iWidth = pParamExt->iPicWidth;
  m_iHeight = pParamExt->iPicHeight;
  pParamExt->iTargetBitrate = 60000;
  pParamExt->sSpatialLayers[0].iVideoHeight = pParamExt->iPicHeight;
  pParamExt->sSpatialLayers[0].iVideoWidth = pParamExt->iPicWidth;
  pParamExt->sSpatialLayers[0].iSpatialBitrate = 50000;
  pParamExt->iTemporalLayerNum = 1;
  pParamExt->iSpatialLayerNum = 1;
  pParamExt->iNumRefFrame = AUTO_REF_PIC_COUNT;

  for (int i = 0; i < 2; i++) {
    pParamExt->iUsageType = ((i == 0) ? SCREEN_CONTENT_REAL_TIME : CAMERA_VIDEO_REAL_TIME);
    int iResult = pPtrEnc->InitializeExt (pParamExt);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

    PrepareOneSrcFrame();

    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));

    pSrcPic->uiTimeStamp = 30;
    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeP));

    ENCODER_OPTION eOptionId = ENCODER_OPTION_SVC_ENCODE_PARAM_EXT;
    iResult = pPtrEnc->GetOption (eOptionId, pOption);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pOption ->iTemporalLayerNum = 4;

    iResult = pPtrEnc->SetOption (eOptionId, pOption);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

    pSrcPic->uiTimeStamp = 60;
    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));

    pOption ->iTemporalLayerNum = 2;
    iResult = pPtrEnc->SetOption (eOptionId, pOption);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp = 90;
    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeP));

    pOption ->iTemporalLayerNum = 4;
    iResult = pPtrEnc->SetOption (eOptionId, pOption);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp = 120;
    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeP));

    //change backgrouddetection and qp
    pOption ->bEnableBackgroundDetection = !pOption ->bEnableBackgroundDetection;
    iResult = pPtrEnc->SetOption (eOptionId, pOption);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp = 150;
    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

    pOption ->bEnableAdaptiveQuant = !pOption ->bEnableAdaptiveQuant;
    iResult = pPtrEnc->SetOption (eOptionId, pOption);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp = 180;
    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

    iResult = pPtrEnc->Uninitialize();
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  }

  InitializeParamExt();
  pParamExt->iUsageType = (rand() % 2) ? SCREEN_CONTENT_REAL_TIME : CAMERA_VIDEO_REAL_TIME;
  pParamExt->iRCMode = (rand() % 2) ? RC_BITRATE_MODE : RC_QUALITY_MODE;

  int iResult = pPtrEnc->InitializeExt (pParamExt);
  const int kiFrameNumber = TEST_FRAMES;

  m_iWidth = pParamExt->iPicWidth;
  m_iHeight = pParamExt->iPicHeight;
  m_iPicResSize =  m_iWidth * m_iHeight * 3 >> 1;
  delete []pYUV;
  pYUV = new unsigned char [m_iPicResSize];
  ASSERT_TRUE (pYUV != NULL);
  PrepareOneSrcFrame();

  ENCODER_OPTION eOptionId = ENCODER_OPTION_SVC_ENCODE_PARAM_EXT;
  memcpy (pOption, pParamExt, sizeof (SEncParamExt));

  for (int i = 0; i < kiFrameNumber; i ++) {
    if ((i % 7) == 0) {
      if (pOption->iTemporalLayerNum < 4) {
        pOption->iTemporalLayerNum++;
      } else {
        pOption->iTemporalLayerNum--;
      }
      iResult = pPtrEnc->SetOption (eOptionId, pOption);
      EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
      pSrcPic->uiTimeStamp += 30;
    }

    int iStartX = rand() % (m_iPicResSize >> 1);
    int iEndX = (iStartX + (rand() % MEM_VARY_SIZE)) % m_iPicResSize;
    for (int j = iStartX; j < iEndX; j++)
      pYUV[j] = rand() % 256;

    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp += 30;
  }

  iResult = pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

}

TEST_F (EncoderInterfaceTest, MemoryCheckTest) {

  InitializeParamExt();
  pParamExt->iPicWidth = 1280;
  pParamExt->iPicHeight = 720;

  int iResult = pPtrEnc->InitializeExt (pParamExt);
  const int kiFrameNumber = TEST_FRAMES;

  m_iWidth = pParamExt->iPicWidth;
  m_iHeight = pParamExt->iPicHeight;
  m_iPicResSize =  m_iWidth * m_iHeight * 3 >> 1;
  delete []pYUV;
  pYUV = new unsigned char [m_iPicResSize];
  PrepareOneSrcFrame();

  for (int i = 0; i < kiFrameNumber; i ++) {
    int iStartX = rand() % (m_iPicResSize >> 1);
    int iEndX = (iStartX + (rand() % MEM_VARY_SIZE)) % m_iPicResSize;
    for (int j = iStartX; j < iEndX; j++)
      pYUV[j] = rand() % 256;

    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp += 30;
  }

  pParamExt->iPicWidth += (rand() << 1) % IMAGE_VARY_SIZE;
  pParamExt->iPicHeight += (rand() << 1) % IMAGE_VARY_SIZE;
  m_iWidth = pParamExt->iPicWidth;
  m_iHeight = pParamExt->iPicHeight;
  m_iPicResSize =  m_iWidth * m_iHeight * 3 >> 1;
  delete []pYUV;
  pYUV = new unsigned char [m_iPicResSize];

  iResult = pPtrEnc->InitializeExt (pParamExt);
  PrepareOneSrcFrame();

  ENCODER_OPTION eOptionId = ENCODER_OPTION_SVC_ENCODE_PARAM_EXT;
  memcpy (pOption, pParamExt, sizeof (SEncParamExt));
  pOption ->iPicWidth = m_iWidth;
  pOption ->iPicHeight = m_iHeight;
  iResult = pPtrEnc->SetOption (eOptionId, pOption);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  for (int i = 0; i < kiFrameNumber; i ++) {
    int iStartX = rand() % (m_iPicResSize >> 1);
    int iEndX = (iStartX + (rand() % MEM_VARY_SIZE)) % m_iPicResSize;
    for (int j = iStartX; j < iEndX; j++)
      pYUV[j] = rand() % 256;

    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp += 30;
  }

  pOption ->iLTRRefNum += rand() % 8 + 1;
  iResult = pPtrEnc->SetOption (eOptionId, pOption);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  for (int i = 0; i < kiFrameNumber; i ++) {
    int iStartX = rand() % (m_iPicResSize >> 1);
    int iEndX = (iStartX + (rand() % MEM_VARY_SIZE)) % m_iPicResSize;
    for (int j = iStartX; j < iEndX; j++)
      pYUV[j] = rand() % 256;

    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp += 30;
  }

  iResult = pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

void GetValidEncParamBase (SEncParamBase* pEncParamBase) {
  pEncParamBase->iUsageType = CAMERA_VIDEO_REAL_TIME;
  pEncParamBase->iPicWidth = 2 + ((rand() % ((MAX_WIDTH >> 1) - 1)) << 1);
  pEncParamBase->iPicHeight = 2 + ((rand() % ((MAX_HEIGHT >> 1) - 1)) << 1);
  pEncParamBase->iPicWidth = VALID_SIZE (pEncParamBase->iPicWidth);
  pEncParamBase->iPicHeight = VALID_SIZE (pEncParamBase->iPicHeight);
  pEncParamBase->iTargetBitrate = rand() + 1; //!=0
  // Force a bitrate of at least w*h/50, otherwise we will only get skipped frames
  pEncParamBase->iTargetBitrate = WELS_CLIP3 (pEncParamBase->iTargetBitrate,
                                  pEncParamBase->iPicWidth * pEncParamBase->iPicHeight / 50, 100000000);
  int32_t iLevelMaxBitrate = WelsCommon::g_ksLevelLimits[LEVEL_5_0 - 1].uiMaxBR * CpbBrNalFactor;
  if (pEncParamBase->iTargetBitrate > iLevelMaxBitrate)
    pEncParamBase->iTargetBitrate = iLevelMaxBitrate;
  pEncParamBase->iRCMode = RC_BITRATE_MODE; //-1, 0, 1, 2
  pEncParamBase->fMaxFrameRate = rand() + 0.5f; //!=0
}

TEST_F (EncoderInterfaceTest, BasicInitializeTest) {
  SEncParamBase sEncParamBase;
  GetValidEncParamBase (&sEncParamBase);

  int iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess)) << "iUsageType = " << sEncParamBase.iUsageType <<
      ", iPicWidth = " << sEncParamBase.iPicWidth << ", iPicHeight = " << sEncParamBase.iPicHeight << ", iTargetBitrate = " <<
      sEncParamBase.iTargetBitrate << ", fMaxFrameRate = " << sEncParamBase.fMaxFrameRate;

  PrepareOneSrcFrame();

  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));

  iResult = pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

TEST_F (EncoderInterfaceTest, BaseParamSettingTest) {
  SEncParamBase sEncParamBase;
  GetValidEncParamBase (&sEncParamBase);

  int iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess)) << "iUsageType = " << sEncParamBase.iUsageType <<
      ", iPicWidth = " << sEncParamBase.iPicWidth << ", iPicHeight = " << sEncParamBase.iPicHeight << ", iTargetBitrate = " <<
      sEncParamBase.iTargetBitrate << ", fMaxFrameRate = " << sEncParamBase.fMaxFrameRate;

  PrepareOneSrcFrame();

  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));

  GetValidEncParamBase (&sEncParamBase);
  iResult = pPtrEnc->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_BASE, &sEncParamBase);

  PrepareOneSrcFrame();

  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  iResult = pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

TEST_F (EncoderInterfaceTest, BasicInitializeTestFalse) {
  int iResult;
  SEncParamBase sEncParamBase;
  int uiTraceLevel = WELS_LOG_QUIET;

  pPtrEnc->SetOption (ENCODER_OPTION_TRACE_LEVEL, &uiTraceLevel);
  //iUsageType
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iUsageType = static_cast<EUsageType> (2);
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  //iPicWidth
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iPicWidth = 0;
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iPicWidth = -1;
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  //TODO: add checking max in interface and then enable this checking
  //GetValidEncParamBase(&sEncParamBase);
  //sEncParamBase.iPicWidth = rand()+(MAX_WIDTH+1);
  //iResult = pPtrEnc->Initialize (&sEncParamBase);
  //EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  //iPicHeight
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iPicHeight = 0;
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iPicHeight = -1;
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  //iPicWidth * iPicHeight <= 36864 * 16 * 16, from Level 5.2 constraint
  //Initialize test: FALSE
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iPicWidth = 5000;
  sEncParamBase.iPicHeight = 5000;
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));
  //Initialize test: TRUE, with SetOption test following
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iPicWidth = 36864;
  sEncParamBase.iPicHeight = 256;
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  sEncParamBase.iPicWidth = 256;
  sEncParamBase.iPicHeight = 36864;
  iResult = pPtrEnc->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_BASE, &sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  sEncParamBase.iPicWidth = 5000;
  sEncParamBase.iPicHeight = 5000;
  iResult = pPtrEnc->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_BASE, &sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));
  pPtrEnc->Uninitialize();

  //iTargetBitrate
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iTargetBitrate = 0;
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iTargetBitrate = -1;
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  uiTraceLevel = WELS_LOG_ERROR;
  pPtrEnc->SetOption (ENCODER_OPTION_TRACE_LEVEL, &uiTraceLevel);
}

TEST_F (EncoderInterfaceTest, BasicInitializeTestAutoAdjustment) {
  SEncParamBase sEncParamBase;

  // large fMaxFrameRate
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.fMaxFrameRate = 50000;
  EncodeOneFrame (&sEncParamBase);
  EXPECT_LE (sEncParamBase.fMaxFrameRate, 60.0);
  EXPECT_GE (sEncParamBase.fMaxFrameRate, 1.0);

  // fMaxFrameRate = 0
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.fMaxFrameRate = 0;
  EncodeOneFrame (&sEncParamBase);
  EXPECT_LE (sEncParamBase.fMaxFrameRate, 60.0);
  EXPECT_GE (sEncParamBase.fMaxFrameRate, 1.0);

  // fMaxFrameRate = -1
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.fMaxFrameRate = -1;
  EncodeOneFrame (&sEncParamBase);
  EXPECT_LE (sEncParamBase.fMaxFrameRate, 60.0);
  EXPECT_GE (sEncParamBase.fMaxFrameRate, 1.0);
}

TEST_F (EncoderInterfaceTest, ForceIntraFrame) {
  SEncParamBase sEncParamBase;
  GetValidEncParamBase (&sEncParamBase);

  int iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess)) << "iUsageType = " << sEncParamBase.iUsageType <<
      ", iPicWidth = " << sEncParamBase.iPicWidth << ", iPicHeight = " << sEncParamBase.iPicHeight << ", iTargetBitrate = " <<
      sEncParamBase.iTargetBitrate << ", fMaxFrameRate = " << sEncParamBase.fMaxFrameRate;

  PrepareOneSrcFrame();

  bool bIDR = true;
  pPtrEnc->ForceIntraFrame (bIDR);
  EncodeOneIDRandP (pPtrEnc);

  //call next frame to be IDR
  pPtrEnc->ForceIntraFrame (bIDR);
  int iCount = 0;
  do {
    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp += 30;
  } while ((sFbi.eFrameType == static_cast<int> (videoFrameTypeSkip)) && (iCount ++ < 100));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));

  pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

TEST_F (EncoderInterfaceTest, ForceIntraFrameWithTemporal) {
  SEncParamExt sEncParamExt;
  pPtrEnc->GetDefaultParams (&sEncParamExt);
  sEncParamExt.iUsageType = CAMERA_VIDEO_REAL_TIME;
  sEncParamExt.iPicWidth = MB_SIZE + abs ((rand() * 2) % (MAX_WIDTH - MB_SIZE));
  sEncParamExt.iPicHeight = MB_SIZE + abs ((rand() * 2) % (MAX_HEIGHT - MB_SIZE));
  sEncParamExt.iTargetBitrate = rand() + 1; //!=0
  // Force a bitrate of at least w*h/50, otherwise we will only get skipped frames
  sEncParamExt.iTargetBitrate = WELS_CLIP3 (sEncParamExt.iTargetBitrate,
                                sEncParamExt.iPicWidth * sEncParamExt.iPicHeight / 50, 100000000);
  int32_t iLevelMaxBitrate = WelsCommon::g_ksLevelLimits[LEVEL_5_0 - 1].uiMaxBR * CpbBrNalFactor;
  if (sEncParamExt.iTargetBitrate > iLevelMaxBitrate)
    sEncParamExt.iTargetBitrate = iLevelMaxBitrate;
  sEncParamExt.iRCMode = RC_BITRATE_MODE; //-1, 0, 1, 2
  sEncParamExt.fMaxFrameRate = rand() + 0.5f; //!=0
  sEncParamExt.sSpatialLayers[0].iVideoWidth = sEncParamExt.iPicWidth;
  sEncParamExt.sSpatialLayers[0].iVideoHeight = sEncParamExt.iPicHeight;
  sEncParamExt.sSpatialLayers[0].iSpatialBitrate = sEncParamExt.iTargetBitrate;
  int iTargetTemporalLayerNum = rand() % MAX_TEMPORAL_LAYER_NUM;
  sEncParamExt.iTemporalLayerNum = (iTargetTemporalLayerNum > 2) ? iTargetTemporalLayerNum : 2;

  int iResult = pPtrEnc->InitializeExt (&sEncParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess)) << "iUsageType = " << sEncParamExt.iUsageType <<
      ", iPicWidth = " << sEncParamExt.iPicWidth << ", iPicHeight = " << sEncParamExt.iPicHeight << ", iTargetBitrate = " <<
      sEncParamExt.iTargetBitrate << ", fMaxFrameRate = " << sEncParamExt.fMaxFrameRate;

  PrepareOneSrcFrame();

  bool bIDR = true;
  EncodeOneIDRandP (pPtrEnc);

  //call next frame to be IDR
  pPtrEnc->ForceIntraFrame (bIDR);
  int iCount = 0;
  do {
    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp += 30;
  } while ((sFbi.eFrameType == static_cast<int> (videoFrameTypeSkip)) && (iCount ++ < 100));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));

  pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

TEST_F (EncoderInterfaceTest, EncodeParameterSets) {
  SEncParamBase sEncParamBase;
  GetValidEncParamBase (&sEncParamBase);

  int iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess)) << "iUsageType = " << sEncParamBase.iUsageType <<
      ", iPicWidth = " << sEncParamBase.iPicWidth << ", iPicHeight = " << sEncParamBase.iPicHeight << ", iTargetBitrate = " <<
      sEncParamBase.iTargetBitrate << ", fMaxFrameRate = " << sEncParamBase.fMaxFrameRate;
  PrepareOneSrcFrame();
  EncodeOneIDRandP (pPtrEnc);

  //try EncodeParameterSets
  pPtrEnc->EncodeParameterSets (&sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeInvalid));

  //check the result
  int iNalType = 0;
  SLayerBSInfo* pLayerBsInfo;
  for (int i = 0; i < sFbi.iLayerNum; i++) {
    pLayerBsInfo = & (sFbi.sLayerInfo[i]);
    EXPECT_EQ (pLayerBsInfo->uiLayerType , static_cast<int> (NON_VIDEO_CODING_LAYER));

    iNalType = GET_NAL_TYPE (pLayerBsInfo->pBsBuf);
    EXPECT_EQ (true, IS_PARASET (iNalType));
    for (int j = 0; j < (pLayerBsInfo->iNalCount - 1); j++) {
      iNalType = GET_NAL_TYPE (pLayerBsInfo->pBsBuf + pLayerBsInfo->pNalLengthInByte[j]);
      EXPECT_EQ (true, IS_PARASET (iNalType));
    }
  }

  //try another P to make sure no impact on succeeding frames
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_NE (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));

  iResult = pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

TEST_F (EncoderInterfaceTest, BasicReturnTypeTest) {
  //TODO
}

void EncoderInterfaceTest::ChangeResolutionAndCheckStatistics (const SEncParamBase& sEncParamBase,
    SEncoderStatistics* pEncoderStatistics) {
  unsigned int uiExistingFrameCount = pEncoderStatistics->uiInputFrameCount;
  unsigned int uiExistingIDR = pEncoderStatistics->uiIDRSentNum;
  unsigned int uiExistingResolutionChange = pEncoderStatistics->uiResolutionChangeTimes;

  // 1, get the existing param
  int iResult = pPtrEnc->GetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, pParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  // 2, change setting
  unsigned int uiKnownResolutionChangeTimes = uiExistingResolutionChange;
  bool bCheckIDR = false;
  if (pParamExt->iPicWidth != sEncParamBase.iPicWidth || pParamExt->iPicHeight != sEncParamBase.iPicHeight) {
    uiKnownResolutionChangeTimes += 1;
    bCheckIDR = true;
  }
  pParamExt->iPicWidth = pParamExt->sSpatialLayers[0].iVideoWidth = sEncParamBase.iPicWidth;
  pParamExt->iPicHeight = pParamExt->sSpatialLayers[0].iVideoHeight = sEncParamBase.iPicHeight;
  pPtrEnc->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, pParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  // 3, code one frame
  PrepareOneSrcFrame();
  pSrcPic->uiTimeStamp = 30 * pEncoderStatistics->uiInputFrameCount;
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  iResult = pPtrEnc->GetOption (ENCODER_OPTION_GET_STATISTICS, pEncoderStatistics);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  EXPECT_EQ (pEncoderStatistics->uiInputFrameCount, uiExistingFrameCount + 1);
  EXPECT_EQ (pEncoderStatistics->uiResolutionChangeTimes, uiKnownResolutionChangeTimes);
  if (bCheckIDR) {
    EXPECT_EQ (pEncoderStatistics->uiIDRSentNum, uiExistingIDR + 1);
  }

  EXPECT_EQ (pEncoderStatistics->uiWidth, static_cast<unsigned int> (sEncParamBase.iPicWidth));
  EXPECT_EQ (pEncoderStatistics->uiHeight, static_cast<unsigned int> (sEncParamBase.iPicHeight));
}


TEST_F (EncoderInterfaceTest, GetStatistics) {
  SEncParamBase sEncParamBase;
  GetValidEncParamBase (&sEncParamBase);

  int iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess)) << "iUsageType = " << sEncParamBase.iUsageType <<
      ", iPicWidth = " << sEncParamBase.iPicWidth << ", iPicHeight = " << sEncParamBase.iPicHeight << ", iTargetBitrate = " <<
      sEncParamBase.iTargetBitrate << ", fMaxFrameRate = " << sEncParamBase.fMaxFrameRate;

  PrepareOneSrcFrame();
  EncodeOneIDRandP (pPtrEnc);

  SEncoderStatistics sEncoderStatistics;
  iResult = pPtrEnc->GetOption (ENCODER_OPTION_GET_STATISTICS, &sEncoderStatistics);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (sEncoderStatistics.uiInputFrameCount, static_cast<unsigned int> (2));
  EXPECT_EQ (sEncoderStatistics.uiIDRSentNum, static_cast<unsigned int> (1));
  EXPECT_EQ (sEncoderStatistics.uiResolutionChangeTimes, static_cast<unsigned int> (0));

  EXPECT_EQ (sEncoderStatistics.uiWidth, static_cast<unsigned int> (sEncParamBase.iPicWidth));
  EXPECT_EQ (sEncoderStatistics.uiHeight, static_cast<unsigned int> (sEncParamBase.iPicHeight));

  // try param change
  GetValidEncParamBase (&sEncParamBase);
  ChangeResolutionAndCheckStatistics (sEncParamBase, &sEncoderStatistics);

  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iPicWidth = (sEncParamBase.iPicWidth % 16) + 1; //try 1~16
  sEncParamBase.iPicHeight = (sEncParamBase.iPicHeight % 16) + 1; //try 1~16
  ChangeResolutionAndCheckStatistics (sEncParamBase, &sEncoderStatistics);

  // try timestamp and frame rate
  pSrcPic->uiTimeStamp = 1000;
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  iResult = pPtrEnc->GetOption (ENCODER_OPTION_GET_STATISTICS, &sEncoderStatistics);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (static_cast<unsigned int> (sEncoderStatistics.fAverageFrameRate), sEncoderStatistics.uiInputFrameCount);

  // 4, change log interval
  int32_t iInterval = 0;
  iResult = pPtrEnc->GetOption (ENCODER_OPTION_STATISTICS_LOG_INTERVAL, &iInterval);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (iInterval, 5000);

  int32_t iInterval2 = 2000;
  iResult = pPtrEnc->SetOption (ENCODER_OPTION_STATISTICS_LOG_INTERVAL, &iInterval2);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  iResult = pPtrEnc->GetOption (ENCODER_OPTION_STATISTICS_LOG_INTERVAL, &iInterval);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (iInterval, iInterval2);

  iInterval2 = 0;
  iResult = pPtrEnc->SetOption (ENCODER_OPTION_STATISTICS_LOG_INTERVAL, &iInterval2);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  iResult = pPtrEnc->GetOption (ENCODER_OPTION_STATISTICS_LOG_INTERVAL, &iInterval);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (iInterval, iInterval2);

  // finish
  pPtrEnc->Uninitialize();
}

TEST_F (EncoderInterfaceTest, FrameSizeCheck) {
  SEncParamBase sEncParamBase;
  GetValidEncParamBase (&sEncParamBase);

  int iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess)) << "iUsageType = " << sEncParamBase.iUsageType <<
      ", iPicWidth = " << sEncParamBase.iPicWidth << ", iPicHeight = " << sEncParamBase.iPicHeight << ", iTargetBitrate = " <<
      sEncParamBase.iTargetBitrate << ", fMaxFrameRate = " << sEncParamBase.fMaxFrameRate;

  PrepareOneSrcFrame();
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);

  int length = 0;
  for (int i = 0; i < sFbi.iLayerNum; ++i) {
    for (int j = 0; j < sFbi.sLayerInfo[i].iNalCount; ++j) {
      length += sFbi.sLayerInfo[i].pNalLengthInByte[j];
    }
  }
  EXPECT_EQ (length, sFbi.iFrameSizeInBytes);

  // finish
  pPtrEnc->Uninitialize();
}

TEST_F (EncoderInterfaceTest, SkipFrameCheck) {
  SEncParamExt sEncParamExt;
  int iResult = pPtrEnc->GetDefaultParams (&sEncParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  //actual encode param: iUsageType = 0, spatial_layer_num = 1, temporal_layer_num = 3, frame_rate = 28.248587, target_bitrate = 573000, denoise =0, background_detection = 1, adaptive_quant = 1, enable_crop_pic = 1, scenechange_detection = 0, enable_long_term_reference = 0, ltr_mark_period = 30, ltr_ref_num = 0, enable_multiple_slice = 1, padding = 0, rc_mode = 1, enable_frame_skip = 1, max_bitrate = 573000, max_nalu_size = 0s
  //actual encode param: spatial 0: width=360, height=640, frame_rate=28.248587, spatial_bitrate=573000, max_layer_bitrate =895839855, max_nalu_size=0, slice_mode=1,this=0x7c62ee48
  sEncParamExt.iUsageType = CAMERA_VIDEO_REAL_TIME;
  sEncParamExt.iPicWidth = 360;
  sEncParamExt.iPicHeight = 640;
  sEncParamExt.iTargetBitrate = 573000;
  sEncParamExt.iRCMode = RC_BITRATE_MODE;
  sEncParamExt.fMaxFrameRate = 28.248587f;

  sEncParamExt.iTemporalLayerNum = 3;
  sEncParamExt.iSpatialLayerNum = 1;
  sEncParamExt.bEnableLongTermReference = 0;
  sEncParamExt.bEnableSceneChangeDetect = 0;
  sEncParamExt.bEnableFrameSkip = 1;
  sEncParamExt.iMaxBitrate = 895839855;
  sEncParamExt.uiMaxNalSize = 0;

  sEncParamExt.sSpatialLayers[0].uiLevelIdc = LEVEL_5_0;
  sEncParamExt.sSpatialLayers[0].iVideoWidth = 360;
  sEncParamExt.sSpatialLayers[0].iVideoHeight = 640;
  sEncParamExt.sSpatialLayers[0].fFrameRate = 28.248587f;
  sEncParamExt.sSpatialLayers[0].iSpatialBitrate = 573000;
  sEncParamExt.sSpatialLayers[0].iMaxSpatialBitrate = 895839855;
  sEncParamExt.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;

  pParamExt->iPicWidth = sEncParamExt.sSpatialLayers[0].iVideoWidth;
  pParamExt->iPicHeight = sEncParamExt.sSpatialLayers[0].iVideoHeight;

  iResult = pPtrEnc->InitializeExt (&sEncParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess)) << "iUsageType = " << sEncParamExt.iUsageType <<
      ", iPicWidth = " << sEncParamExt.iPicWidth << ", iPicHeight = " << sEncParamExt.iPicHeight << ", iTargetBitrate = " <<
      sEncParamExt.iTargetBitrate << ", fMaxFrameRate = " << sEncParamExt.fMaxFrameRate;

  int iInterval = 300;
  iResult = pPtrEnc->SetOption (ENCODER_OPTION_STATISTICS_LOG_INTERVAL, &iInterval);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  SEncoderStatistics sEncoderStatistics;
  for (int i = 0; i < 50; i++) {
    PrepareOneSrcFrame();
    pSrcPic->uiTimeStamp = i * 33;
    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);

    if (i == 25) {
      sEncParamExt.fMaxFrameRate = 28;
      sEncParamExt.sSpatialLayers[0].fFrameRate = 28;
      sEncParamExt.sSpatialLayers[0].iSpatialBitrate = 500000;
      iResult = pPtrEnc->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sEncParamExt);
      EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    }

  }
  iResult = pPtrEnc->GetOption (ENCODER_OPTION_GET_STATISTICS, &sEncoderStatistics);
  EXPECT_TRUE ((sEncoderStatistics.uiInputFrameCount - sEncoderStatistics.uiSkippedFrameCount) > 2)
      << "uiInputFrameCount = " << sEncoderStatistics.uiInputFrameCount << ", uiSkippedFrameCount = " <<
      sEncoderStatistics.uiSkippedFrameCount;
  // finish
  pPtrEnc->Uninitialize();
}

TEST_F (EncoderInterfaceTest, DiffResolutionCheck) {
  SEncParamExt sEncParamExt;
  int iResult = pPtrEnc->GetDefaultParams (&sEncParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  //actual encode param: iUsageType = 0, spatial_layer_num = 1, temporal_layer_num = 3, frame_rate = 28.248587, target_bitrate = 573000, denoise =0, background_detection = 1, adaptive_quant = 1, enable_crop_pic = 1, scenechange_detection = 0, enable_long_term_reference = 0, ltr_mark_period = 30, ltr_ref_num = 0, enable_multiple_slice = 1, padding = 0, rc_mode = 1, enable_frame_skip = 1, max_bitrate = 573000, max_nalu_size = 0s
  //actual encode param: spatial 0: width=360, height=640, frame_rate=28.248587, spatial_bitrate=573000, max_layer_bitrate =895839855, max_nalu_size=0, slice_mode=1,this=0x7c62ee48
  sEncParamExt.iUsageType = CAMERA_VIDEO_REAL_TIME;
  sEncParamExt.iPicWidth = 360;
  sEncParamExt.iPicHeight = 640;
  sEncParamExt.iTargetBitrate = 1000000;
  sEncParamExt.iRCMode = RC_BITRATE_MODE;
  sEncParamExt.fMaxFrameRate = 24;

  sEncParamExt.iTemporalLayerNum = 3;
  sEncParamExt.iSpatialLayerNum = 3;
  sEncParamExt.bEnableLongTermReference = 0;
  sEncParamExt.bEnableSceneChangeDetect = 0;
  sEncParamExt.bEnableFrameSkip = 1;
  sEncParamExt.uiMaxNalSize = 0;

  sEncParamExt.sSpatialLayers[0].uiLevelIdc = LEVEL_UNKNOWN;
  sEncParamExt.sSpatialLayers[0].iVideoWidth = 90;
  sEncParamExt.sSpatialLayers[0].iVideoHeight = 160;
  sEncParamExt.sSpatialLayers[0].fFrameRate = 6;
  sEncParamExt.sSpatialLayers[0].iSpatialBitrate = 80000;
  sEncParamExt.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;

  sEncParamExt.sSpatialLayers[1].uiLevelIdc = LEVEL_UNKNOWN;
  sEncParamExt.sSpatialLayers[1].iVideoWidth = 180;
  sEncParamExt.sSpatialLayers[1].iVideoHeight = 320;
  sEncParamExt.sSpatialLayers[1].fFrameRate = 12;
  sEncParamExt.sSpatialLayers[1].iSpatialBitrate = 200000;
  sEncParamExt.sSpatialLayers[1].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;

  sEncParamExt.sSpatialLayers[2].uiLevelIdc = LEVEL_UNKNOWN;
  sEncParamExt.sSpatialLayers[2].iVideoWidth = 360;
  sEncParamExt.sSpatialLayers[2].iVideoHeight = 640;
  sEncParamExt.sSpatialLayers[2].fFrameRate = 24;
  sEncParamExt.sSpatialLayers[2].iSpatialBitrate = 600000;
  sEncParamExt.sSpatialLayers[2].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;

  pParamExt->iPicWidth = sEncParamExt.iPicWidth;
  pParamExt->iPicHeight = sEncParamExt.iPicWidth;

  iResult = pPtrEnc->InitializeExt (&sEncParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess)) << "iUsageType = " << sEncParamExt.iUsageType <<
      ", iPicWidth = " << sEncParamExt.iPicWidth << ", iPicHeight = " << sEncParamExt.iPicHeight << ", iTargetBitrate = " <<
      sEncParamExt.iTargetBitrate << ", fMaxFrameRate = " << sEncParamExt.fMaxFrameRate;

  PrepareOneSrcFrame();
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  pSrcPic->uiTimeStamp += 30;

  //correct setting
  sEncParamExt.iSpatialLayerNum = 2;
  sEncParamExt.sSpatialLayers[1].uiLevelIdc = LEVEL_UNKNOWN;
  sEncParamExt.sSpatialLayers[1].iVideoWidth = 360;
  sEncParamExt.sSpatialLayers[1].iVideoHeight = 640;
  sEncParamExt.sSpatialLayers[1].fFrameRate = 24;
  sEncParamExt.sSpatialLayers[1].iSpatialBitrate = 600000;
  iResult = pPtrEnc->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sEncParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

  PrepareOneSrcFrame();
  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  pSrcPic->uiTimeStamp += 30;

  //incorrect setting
  int uiTraceLevel = WELS_LOG_QUIET;
  pPtrEnc->SetOption (ENCODER_OPTION_TRACE_LEVEL, &uiTraceLevel);

  sEncParamExt.sSpatialLayers[1].iVideoWidth = 90;
  sEncParamExt.sSpatialLayers[1].iVideoHeight = 160;
  sEncParamExt.sSpatialLayers[1].fFrameRate = 6;
  sEncParamExt.sSpatialLayers[1].iSpatialBitrate = 80000;

  sEncParamExt.sSpatialLayers[0].iVideoWidth = 360;
  sEncParamExt.sSpatialLayers[0].iVideoHeight = 640;
  sEncParamExt.sSpatialLayers[0].fFrameRate = 24;
  sEncParamExt.sSpatialLayers[0].iSpatialBitrate = 600000;
  iResult = pPtrEnc->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sEncParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  //incorrect setting
  sEncParamExt.iSpatialLayerNum = 3;
  sEncParamExt.sSpatialLayers[1].iVideoWidth = 90;
  sEncParamExt.sSpatialLayers[1].iVideoHeight = 160;
  sEncParamExt.sSpatialLayers[1].fFrameRate = 6;
  sEncParamExt.sSpatialLayers[1].iSpatialBitrate = 80000;

  sEncParamExt.sSpatialLayers[0].iVideoWidth = 180;
  sEncParamExt.sSpatialLayers[0].iVideoHeight = 320;
  sEncParamExt.sSpatialLayers[0].fFrameRate = 12;
  sEncParamExt.sSpatialLayers[0].iSpatialBitrate = 200000;

  sEncParamExt.sSpatialLayers[2].iVideoWidth = 360;
  sEncParamExt.sSpatialLayers[2].iVideoHeight = 640;
  sEncParamExt.sSpatialLayers[2].fFrameRate = 24;
  sEncParamExt.sSpatialLayers[2].iSpatialBitrate = 600000;
  iResult = pPtrEnc->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sEncParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  // finish
  pPtrEnc->Uninitialize();
}

TEST_F (EncoderInterfaceTest, NalSizeChecking) {
   // int uiTraceLevel = WELS_LOG_DETAIL;
   // pPtrEnc->SetOption (ENCODER_OPTION_TRACE_LEVEL, &uiTraceLevel);

    pParamExt->iPicWidth = 1280;
    pParamExt->iPicHeight = 720;
    pParamExt->iPicWidth += (rand() << 1) % IMAGE_VARY_SIZE;
    pParamExt->iPicHeight += (rand() << 1) % IMAGE_VARY_SIZE;
    pParamExt->fMaxFrameRate = 30;
    pParamExt->iTemporalLayerNum = rand()%3;
    pParamExt->iSpatialLayerNum = rand()%4;
    pParamExt->iNumRefFrame = AUTO_REF_PIC_COUNT;
    pParamExt->iSpatialLayerNum = WELS_CLIP3(pParamExt->iSpatialLayerNum,1,4);

    pParamExt->iMultipleThreadIdc = 1;//multi-thread can't control size. will be fixed.
    pParamExt->iEntropyCodingModeFlag = rand()%2;
    int iMaxNalSize =rand()%5000;
    iMaxNalSize =  WELS_CLIP3(iMaxNalSize,1000,5000);
    pParamExt->uiMaxNalSize = iMaxNalSize;
    for(int i = 0;i<pParamExt->iSpatialLayerNum;i++){
      pParamExt->sSpatialLayers[i].sSliceArgument.uiSliceMode = SM_SIZELIMITED_SLICE;
      pParamExt->sSpatialLayers[i].sSliceArgument.uiSliceSizeConstraint = iMaxNalSize;
      pParamExt->sSpatialLayers[i].iVideoHeight = pParamExt->iPicHeight;
      pParamExt->sSpatialLayers[i].iVideoWidth = pParamExt->iPicWidth;
      int bitrate = rand()%3000000;
      pParamExt->sSpatialLayers[i].iSpatialBitrate = WELS_CLIP3(bitrate,500000,3000000) ;
      pParamExt->iTargetBitrate+= pParamExt->sSpatialLayers[i].iSpatialBitrate;
      pParamExt->sSpatialLayers[i].fFrameRate = 30;
    }
    int iResult = pPtrEnc->InitializeExt (pParamExt);
    const int kiFrameNumber = TEST_FRAMES;

    m_iWidth = pParamExt->iPicWidth;
    m_iHeight = pParamExt->iPicHeight;
    m_iPicResSize =  m_iWidth * m_iHeight * 3 >> 1;
    if(pYUV)
       delete []pYUV;
    pYUV = new unsigned char [m_iPicResSize];
    ASSERT_TRUE (pYUV != NULL);
    FileInputStream fileStream;
    ASSERT_TRUE (fileStream.Open ("res/Cisco_Absolute_Power_1280x720_30fps.yuv"));
    PrepareOneSrcFrame();
    for (int i = 0; i < kiFrameNumber; i ++) {
        if(fileStream.read (pYUV, m_iPicResSize) != m_iPicResSize){
            break;
        }
        iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
        EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
        pSrcPic->uiTimeStamp += 30;

        for (int i = 0; i < sFbi.iLayerNum; ++i) {
            for (int j = 0; j < sFbi.sLayerInfo[i].iNalCount; ++j) {
                int length = sFbi.sLayerInfo[i].pNalLengthInByte[j];
                EXPECT_LE (length, iMaxNalSize);
            }
        }

    }
    pParamExt->iPicWidth = 1280;
    pParamExt->iPicHeight = 720;
    pParamExt->iPicWidth += (rand() << 1) % IMAGE_VARY_SIZE;
    pParamExt->iPicHeight += (rand() << 1) % IMAGE_VARY_SIZE;
    m_iWidth = pParamExt->iPicWidth;
    m_iHeight = pParamExt->iPicHeight;
    m_iPicResSize =  m_iWidth * m_iHeight * 3 >> 1;
    delete []pYUV;
    pYUV = new unsigned char [m_iPicResSize];
    ASSERT_TRUE (pYUV != NULL);
    iResult = pPtrEnc->InitializeExt (pParamExt);
    PrepareOneSrcFrame();

    ENCODER_OPTION eOptionId = ENCODER_OPTION_SVC_ENCODE_PARAM_EXT;
    memcpy (pOption, pParamExt, sizeof (SEncParamExt));
    pOption ->iPicWidth = m_iWidth;
    pOption ->iPicHeight = m_iHeight;
    iResult = pPtrEnc->SetOption (eOptionId, pOption);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));

    for (int i = 0; i < kiFrameNumber; i ++) {
        PrepareOneSrcFrame();
        iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
        EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
        pSrcPic->uiTimeStamp += 30;

        for (int i = 0; i < sFbi.iLayerNum; ++i) {
            for (int j = 0; j < sFbi.sLayerInfo[i].iNalCount; ++j) {
                int length = sFbi.sLayerInfo[i].pNalLengthInByte[j];
                EXPECT_LE (length, iMaxNalSize);
            }
        }
    }
    iResult = pPtrEnc->Uninitialize();
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}
