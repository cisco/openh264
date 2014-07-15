#include <gtest/gtest.h>
#include <stdlib.h>
#include "codec_api.h"
#include "codec_app_def.h"
//TODO: consider using BaseEncoderTest class from #include "../BaseEncoderTest.h"

class EncoderInterfaceTest : public ::testing::Test {
#define MB_SIZE (16)
#define MAX_WIDTH (3840)
#define MAX_HEIGHT (2160)
#define VALID_SIZE(iSize) (((iSize)>1)?(iSize):1)

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

  void TemporalLayerSettingTest();
  void MemoryCheckTest();
  void EncodeOneFrame (SEncParamBase* pEncParamBase);
  void PrepareOneSrcFrame();
  void EncodeOneIDRandP (ISVCEncoder* pPtrEnc);

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

  for (int i = 0; i < m_iPicResSize; i++)
    pYUV[i] = rand() % 256;

  pSrcPic->iStride[0] = m_iWidth;
  pSrcPic->iStride[1] = pSrcPic->iStride[2] = pSrcPic->iStride[0] >> 1;

  pSrcPic->pData[0] = pYUV;
  pSrcPic->pData[1] = pSrcPic->pData[0] + (m_iWidth * m_iHeight);
  pSrcPic->pData[2] = pSrcPic->pData[1] + (m_iWidth * m_iHeight >> 2);

  //clean the output
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

  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_NE (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));
}

void EncoderInterfaceTest::TemporalLayerSettingTest() {

  pParamExt->iPicWidth = m_iWidth;
  pParamExt->iPicHeight = m_iHeight;
  pParamExt->iTargetBitrate = 60000;
  pParamExt->sSpatialLayers[0].iVideoHeight = pParamExt->iPicHeight;
  pParamExt->sSpatialLayers[0].iVideoWidth = pParamExt->iPicWidth;
  pParamExt->sSpatialLayers[0].iSpatialBitrate = 50000;
  pParamExt->iTemporalLayerNum = 1;
  pParamExt->iSpatialLayerNum = 1;

  for(int i = 0; i < 2; i++){
    pParamExt->iUsageType = (( i == 0 ) ? SCREEN_CONTENT_REAL_TIME : CAMERA_VIDEO_REAL_TIME);
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

    memcpy (pOption, pParamExt, sizeof (SEncParamExt));
    pOption ->iTemporalLayerNum = 4;

    ENCODER_OPTION eOptionId = ENCODER_OPTION_SVC_ENCODE_PARAM_EXT;
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

    pPtrEnc->Uninitialize();
  }
}

TEST_F (EncoderInterfaceTest, TestTemporalLayerSetting) {
  TemporalLayerSettingTest();
}

void EncoderInterfaceTest::MemoryCheckTest() {
  #define MEM_VARY_SIZE 1024
  #define TEST_FRAMES 500

  pParamExt->iPicWidth = 1280;
  pParamExt->iPicHeight = 720;
  pParamExt->iTargetBitrate = 60000;
  pParamExt->sSpatialLayers[0].iVideoHeight = pParamExt->iPicHeight;
  pParamExt->sSpatialLayers[0].iVideoWidth = pParamExt->iPicWidth;
  pParamExt->sSpatialLayers[0].iSpatialBitrate = 50000;
  pParamExt->iTemporalLayerNum = 3;
  pParamExt->iSpatialLayerNum = 1;

  int iResult = pPtrEnc->InitializeExt (pParamExt);
  const int kiFrameNumber = TEST_FRAMES;

  m_iWidth = pParamExt->iPicWidth;
  m_iHeight = pParamExt->iPicHeight;
  m_iPicResSize =  m_iWidth * m_iHeight * 3 >> 1;
  delete []pYUV;
  pYUV = new unsigned char [m_iPicResSize];
  PrepareOneSrcFrame();

  for(int i = 0; i < kiFrameNumber; i ++){
    int iStartX = rand() % (m_iPicResSize >> 1);
    int iEndX = (iStartX + (rand() % MEM_VARY_SIZE)) % m_iPicResSize;
    for (int j = iStartX; j < iEndX; j++)
      pYUV[j] = rand() % 256;

    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp += 30;
  }

  pParamExt->iPicWidth += (rand() << 1) % MAX_WIDTH;
  if (pParamExt->iPicWidth > MAX_WIDTH)
    pParamExt->iPicWidth = MAX_WIDTH;
  pParamExt->iPicHeight += (rand() << 1) % MAX_HEIGHT;
  if (pParamExt->iPicWidth > MAX_HEIGHT)
    pParamExt->iPicHeight = MAX_HEIGHT;
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

  for(int i = 0; i < kiFrameNumber; i ++){
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

  for(int i = 0; i < kiFrameNumber; i ++){
    int iStartX = rand() % (m_iPicResSize >> 1);
    int iEndX = (iStartX + (rand() % MEM_VARY_SIZE)) % m_iPicResSize;
    for (int j = iStartX; j < iEndX; j++)
      pYUV[j] = rand() % 256;

    iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
    EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
    pSrcPic->uiTimeStamp += 30;
  }
  pPtrEnc->Uninitialize();
}

TEST_F (EncoderInterfaceTest, MemoryCheck) {
  MemoryCheckTest();
}

void GetValidEncParamBase (SEncParamBase* pEncParamBase) {
  pEncParamBase->iUsageType = CAMERA_VIDEO_REAL_TIME;
  pEncParamBase->iPicWidth = ((rand() * 2) % (MAX_WIDTH));
  pEncParamBase->iPicHeight = ((rand() * 2) % (MAX_HEIGHT));
  pEncParamBase->iPicWidth = VALID_SIZE(pEncParamBase->iPicWidth);
  pEncParamBase->iPicHeight = VALID_SIZE(pEncParamBase->iPicHeight);
  pEncParamBase->iTargetBitrate = rand() + 1; //!=0
  pEncParamBase->iRCMode = RC_BITRATE_MODE; //-1, 0, 1, 2
  pEncParamBase->fMaxFrameRate = rand() + 0.5f; //!=0
}

TEST_F (EncoderInterfaceTest, BasicInitializeTest) {
  SEncParamBase sEncParamBase;
  GetValidEncParamBase (&sEncParamBase);

  int iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  if (iResult != cmResultSuccess) {
    fprintf (stderr, "Unexpected ParamBase? \
             iUsageType=%d, Pic=%dx%d, TargetBitrate=%d, iRCMode=%d, fMaxFrameRate=%.1f\n",
             sEncParamBase.iUsageType, sEncParamBase.iPicWidth, sEncParamBase.iPicHeight,
             sEncParamBase.iTargetBitrate, sEncParamBase.iRCMode, sEncParamBase.fMaxFrameRate);
  }

  PrepareOneSrcFrame();

  iResult = pPtrEnc->EncodeFrame (pSrcPic, &sFbi);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  EXPECT_EQ (sFbi.eFrameType, static_cast<int> (videoFrameTypeIDR));

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

  //TODO: add checking max in interface and then enable this checking
  //GetValidEncParamBase(&sEncParamBase);
  //sEncParamBase.iPicWidth = rand()+(MAX_HEIGHT+1);
  //iResult = pPtrEnc->Initialize (&sEncParamBase);
  //EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  //iTargetBitrate
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iTargetBitrate = 0;
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iTargetBitrate = -1;
  iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmInitParaError));

  //iUsageType
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.iRCMode = static_cast<RC_MODES> (3);
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
  EXPECT_LE (sEncParamBase.fMaxFrameRate, 30.0);
  EXPECT_GE (sEncParamBase.fMaxFrameRate, 1.0);

  // fMaxFrameRate = 0
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.fMaxFrameRate = 0;
  EncodeOneFrame (&sEncParamBase);
  EXPECT_LE (sEncParamBase.fMaxFrameRate, 30.0);
  EXPECT_GE (sEncParamBase.fMaxFrameRate, 1.0);

  // fMaxFrameRate = -1
  GetValidEncParamBase (&sEncParamBase);
  sEncParamBase.fMaxFrameRate = -1;
  EncodeOneFrame (&sEncParamBase);
  EXPECT_LE (sEncParamBase.fMaxFrameRate, 30.0);
  EXPECT_GE (sEncParamBase.fMaxFrameRate, 1.0);
}

TEST_F (EncoderInterfaceTest, ForceIntraFrame) {
  SEncParamBase sEncParamBase;
  GetValidEncParamBase (&sEncParamBase);

  int iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  if (iResult != cmResultSuccess) {
    fprintf (stderr, "Unexpected ParamBase? \
             iUsageType=%d, Pic=%dx%d, TargetBitrate=%d, iRCMode=%d, fMaxFrameRate=%.1f\n",
             sEncParamBase.iUsageType, sEncParamBase.iPicWidth, sEncParamBase.iPicHeight,
             sEncParamBase.iTargetBitrate, sEncParamBase.iRCMode, sEncParamBase.fMaxFrameRate);
  }

  PrepareOneSrcFrame();

  bool bIDR = true;
  pPtrEnc->ForceIntraFrame (bIDR);
  EncodeOneIDRandP (pPtrEnc);

  //call next frame to be IDR
  pPtrEnc->ForceIntraFrame (bIDR);
  EncodeOneIDRandP (pPtrEnc);

  pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

TEST_F (EncoderInterfaceTest, ForceIntraFrameWithTemporal) {
  SEncParamExt sEncParamExt;
  pPtrEnc->GetDefaultParams (&sEncParamExt);
  sEncParamExt.iUsageType = CAMERA_VIDEO_REAL_TIME;
  sEncParamExt.iPicWidth = abs ((rand() * 2) + MB_SIZE) % (MAX_WIDTH + 1);
  sEncParamExt.iPicHeight = abs ((rand() * 2) + MB_SIZE) % (MAX_HEIGHT + 1);
  sEncParamExt.iTargetBitrate = rand() + 1; //!=0
  sEncParamExt.iRCMode = RC_BITRATE_MODE; //-1, 0, 1, 2
  sEncParamExt.fMaxFrameRate = rand() + 0.5f; //!=0
  sEncParamExt.sSpatialLayers[0].iVideoWidth = sEncParamExt.iPicWidth;
  sEncParamExt.sSpatialLayers[0].iVideoHeight = sEncParamExt.iPicHeight;
  sEncParamExt.sSpatialLayers[0].iSpatialBitrate = sEncParamExt.iTargetBitrate;

  int iTargetTemporalLayerNum = rand() % MAX_TEMPORAL_LAYER_NUM;
  sEncParamExt.iTemporalLayerNum = (iTargetTemporalLayerNum > 2) ? iTargetTemporalLayerNum : 2;

  int iResult = pPtrEnc->InitializeExt (&sEncParamExt);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  if (iResult != cmResultSuccess) {
    fprintf (stderr, "Unexpected ParamBase? \
             iUsageType=%d, Pic=%dx%d, TargetBitrate=%d, iRCMode=%d, fMaxFrameRate=%.1f\n",
             sEncParamExt.iUsageType, sEncParamExt.iPicWidth, sEncParamExt.iPicHeight,
             sEncParamExt.iTargetBitrate, sEncParamExt.iRCMode, sEncParamExt.fMaxFrameRate);
  }

  PrepareOneSrcFrame();

  bool bIDR = true;
  EncodeOneIDRandP (pPtrEnc);

  //call next frame to be IDR
  pPtrEnc->ForceIntraFrame (bIDR);
  EncodeOneIDRandP (pPtrEnc);

  pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

TEST_F (EncoderInterfaceTest, EncodeParameterSets) {
  SEncParamBase sEncParamBase;
  GetValidEncParamBase (&sEncParamBase);

  int iResult = pPtrEnc->Initialize (&sEncParamBase);
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
  if (iResult != cmResultSuccess) {
    fprintf (stderr, "Unexpected ParamBase? \
             iUsageType=%d, Pic=%dx%d, TargetBitrate=%d, iRCMode=%d, fMaxFrameRate=%.1f\n",
             sEncParamBase.iUsageType, sEncParamBase.iPicWidth, sEncParamBase.iPicHeight,
             sEncParamBase.iTargetBitrate, sEncParamBase.iRCMode, sEncParamBase.fMaxFrameRate);
  }
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

  pPtrEnc->Uninitialize();
  EXPECT_EQ (iResult, static_cast<int> (cmResultSuccess));
}

TEST_F (EncoderInterfaceTest, BasicReturnTypeTest) {
  //TODO
}

