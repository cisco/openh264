#include <stdlib.h>
#include "gtest/gtest.h"

#include "au_set.h"
#include "param_svc.h"
#include "parameter_sets.h"
#include "wels_const.h"

using namespace WelsEnc;

class ParameterSetStrategyTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    pMa = NULL;
    m_pSpsArray = NULL;
    m_pSubsetArray = NULL;

    pMa = new CMemoryAlign (0);
    m_pSpsArray = (SWelsSPS*)pMa->WelsMalloc (MAX_SPS_COUNT * sizeof (SWelsSPS), "m_pSpsArray");
    ASSERT_TRUE (NULL != m_pSpsArray);
    m_pSubsetArray = (SSubsetSps*)pMa->WelsMalloc (MAX_SPS_COUNT * sizeof (SSubsetSps), "m_pSubsetArray");
    ASSERT_TRUE (NULL != m_pSubsetArray);

    m_pSpsArrayPointer = &m_pSpsArray[0];
    m_pSubsetArrayPointer = &m_pSubsetArray[0];

  }
  virtual void TearDown() {
    pMa->WelsFree (m_pSpsArray, "m_pSpsArray");
    pMa->WelsFree (m_pSubsetArray, "m_pSubsetArray");
    delete pMa;
  }
  void GenerateParam (SWelsSvcCodingParam* pParam);
 public:
  CMemoryAlign* pMa;
  SWelsSPS*     m_pSpsArray;
  SSubsetSps*   m_pSubsetArray;

  SWelsSPS*     m_pSpsArrayPointer;
  SSubsetSps*   m_pSubsetArrayPointer;

};

void ParameterSetStrategyTest::GenerateParam (SWelsSvcCodingParam* pParam) {
  SEncParamBase sEncParamBase;
  //TODO: consider randomize it
  sEncParamBase.iUsageType = CAMERA_VIDEO_REAL_TIME;
  sEncParamBase.iPicWidth = 1280;
  sEncParamBase.iPicHeight = 720;
  sEncParamBase.iTargetBitrate = 1000000;
  sEncParamBase.iRCMode = RC_BITRATE_MODE;
  sEncParamBase.fMaxFrameRate = 30.0f;
  pParam->ParamBaseTranscode (sEncParamBase);
}

TEST_F (ParameterSetStrategyTest, FindExistingSps) {
  int iDlayerIndex = 0;
  int iDlayerCount = 0;
  bool bUseSubsetSps = false;
  int iFoundId = -1;
  int iRet = 0;
  SSpatialLayerConfig* pDlayerParam;

  //init parameter
  SWelsSvcCodingParam sParam1;
  GenerateParam (&sParam1);

  //prepare first SPS
  int iCurSpsId = 0;
  int iCurSpsInUse = 1;
  m_pSpsArrayPointer = &m_pSpsArray[iCurSpsId];

  pDlayerParam = & (sParam1.sSpatialLayers[iDlayerIndex]);
  iRet = WelsInitSps (m_pSpsArrayPointer, pDlayerParam, &sParam1.sDependencyLayers[iDlayerIndex], sParam1.uiIntraPeriod,
                      sParam1.iMaxNumRefFrame,
                      iCurSpsId, sParam1.bEnableFrameCroppingFlag, sParam1.iRCMode != RC_OFF_MODE, iDlayerCount, false);

  // try finding #0
  iFoundId = FindExistingSps (&sParam1, bUseSubsetSps, iDlayerIndex, iDlayerCount, iCurSpsInUse,
                              m_pSpsArray, m_pSubsetArray, false);
  EXPECT_EQ (iFoundId, iCurSpsId);

  // try not finding
  SWelsSvcCodingParam sParam2 = sParam1;
  sParam2.iMaxNumRefFrame ++;
  iFoundId = FindExistingSps (&sParam2, bUseSubsetSps, iDlayerIndex, iDlayerCount, iCurSpsInUse,
                              m_pSpsArray, m_pSubsetArray, false);
  EXPECT_EQ (iFoundId, INVALID_ID);

  // add new sps
  iCurSpsId = 1;
  m_pSpsArrayPointer = &m_pSpsArray[iCurSpsId];
  pDlayerParam = & (sParam2.sSpatialLayers[iDlayerIndex]);
  iRet = WelsInitSps (m_pSpsArrayPointer, pDlayerParam, &sParam2.sDependencyLayers[iDlayerIndex], sParam2.uiIntraPeriod,
                      sParam2.iMaxNumRefFrame,
                      iCurSpsId, sParam2.bEnableFrameCroppingFlag, sParam2.iRCMode != RC_OFF_MODE, iDlayerCount, false);
  iCurSpsInUse = 2;

  // try finding #1
  iFoundId = FindExistingSps (&sParam2, bUseSubsetSps, iDlayerIndex, iDlayerCount, iCurSpsInUse,
                              m_pSpsArray, m_pSubsetArray, false);
  EXPECT_EQ (iFoundId, iCurSpsId);

  // try finding #0
  iFoundId = FindExistingSps (&sParam1, bUseSubsetSps, iDlayerIndex, iDlayerCount, iCurSpsInUse,
                              m_pSpsArray, m_pSubsetArray, false);
  EXPECT_EQ (iFoundId, 0);

  // try not finding
  if (sParam2.sDependencyLayers[0].iActualWidth > 1) {

    sParam2.sDependencyLayers[0].iActualWidth--;
  } else {
    sParam2.sDependencyLayers[0].iActualWidth++;
  }

  iFoundId = FindExistingSps (&sParam2, bUseSubsetSps, iDlayerIndex, iDlayerCount, iCurSpsInUse,
                              m_pSpsArray, m_pSubsetArray, false);
  EXPECT_EQ (iFoundId, INVALID_ID);
  (void) iRet; // Not using iRet at the moment
}

TEST_F (ParameterSetStrategyTest, TestVSTPParameters) {
  int32_t iRet = 0;
  bool FalseLocal = false;	// EXPECT_EQ does not like "true" or "false" as its first arg

  // this test verifies that the client's "video signal type present" parameter values end up in SWelsSPS 

  //init client parameters
  SEncParamExt sParamExt;
  SWelsSvcCodingParam::FillDefault(sParamExt);
  sParamExt.iUsageType = CAMERA_VIDEO_REAL_TIME;
  sParamExt.iPicWidth = 1280;
  sParamExt.iPicHeight = 720;
  sParamExt.iTargetBitrate = 1000000;
  sParamExt.iRCMode = RC_BITRATE_MODE;
  sParamExt.fMaxFrameRate = 30.0f;

  // VSTP parameters should be their default values (see SWelsSvcCodingParam::FillDefault())
  for  ( int i = 0; i < MAX_SPATIAL_LAYER_NUM; i++ )
  {
    //         expected    actual
    EXPECT_EQ( FalseLocal, sParamExt.sSpatialLayers[i].bVideoSignalTypePresent);
    EXPECT_EQ( VF_UNDEF,   sParamExt.sSpatialLayers[i].uiVideoFormat);//5
    EXPECT_EQ( FalseLocal, sParamExt.sSpatialLayers[i].bFullRange);
    EXPECT_EQ( FalseLocal, sParamExt.sSpatialLayers[i].bColorDescriptionPresent);
    EXPECT_EQ( CP_UNDEF,   sParamExt.sSpatialLayers[i].uiColorPrimaries);//2
    EXPECT_EQ( TRC_UNDEF,  sParamExt.sSpatialLayers[i].uiTransferCharacteristics);//2
    EXPECT_EQ( CM_UNDEF,   sParamExt.sSpatialLayers[i].uiColorMatrix);//2
  }

  // set non-default VSTP values
  sParamExt.iSpatialLayerNum = 2;

  sParamExt.sSpatialLayers[0].bVideoSignalTypePresent   = true;
  sParamExt.sSpatialLayers[0].uiVideoFormat             = VF_NTSC;//2
  sParamExt.sSpatialLayers[0].bFullRange                = true;
  sParamExt.sSpatialLayers[0].bColorDescriptionPresent  = true;
  sParamExt.sSpatialLayers[0].uiColorPrimaries          = CP_BT709;//1
  sParamExt.sSpatialLayers[0].uiTransferCharacteristics = TRC_BT709;//1
  sParamExt.sSpatialLayers[0].uiColorMatrix             = CM_BT709;//1

  sParamExt.sSpatialLayers[1].bVideoSignalTypePresent   = true;
  sParamExt.sSpatialLayers[1].uiVideoFormat             = VF_PAL;//1
  sParamExt.sSpatialLayers[1].bFullRange                = true;
  sParamExt.sSpatialLayers[1].bColorDescriptionPresent  = true;
  sParamExt.sSpatialLayers[1].uiColorPrimaries          = CP_SMPTE170M;//6
  sParamExt.sSpatialLayers[1].uiTransferCharacteristics = TRC_SMPTE170M;//6
  sParamExt.sSpatialLayers[1].uiColorMatrix             = CM_SMPTE170M;//6

  // transcode parameters from client
  SWelsSvcCodingParam sSvcCodingParam;
  iRet = sSvcCodingParam.ParamTranscode(sParamExt);
  EXPECT_EQ (iRet, 0);

  // transcoded VSTP parameters should match the client values
  for  ( int i = 0; i < sParamExt.iSpatialLayerNum; i++ )
  {
    EXPECT_EQ( sParamExt.sSpatialLayers[i].bVideoSignalTypePresent,   sSvcCodingParam.sSpatialLayers[i].bVideoSignalTypePresent);
    EXPECT_EQ( sParamExt.sSpatialLayers[i].uiVideoFormat,             sSvcCodingParam.sSpatialLayers[i].uiVideoFormat);
    EXPECT_EQ( sParamExt.sSpatialLayers[i].bFullRange,                sSvcCodingParam.sSpatialLayers[i].bFullRange);
    EXPECT_EQ( sParamExt.sSpatialLayers[i].bColorDescriptionPresent,  sSvcCodingParam.sSpatialLayers[i].bColorDescriptionPresent);
    EXPECT_EQ( sParamExt.sSpatialLayers[i].uiColorPrimaries,          sSvcCodingParam.sSpatialLayers[i].uiColorPrimaries);
    EXPECT_EQ( sParamExt.sSpatialLayers[i].uiTransferCharacteristics, sSvcCodingParam.sSpatialLayers[i].uiTransferCharacteristics);
    EXPECT_EQ( sParamExt.sSpatialLayers[i].uiColorMatrix,             sSvcCodingParam.sSpatialLayers[i].uiColorMatrix);
  }

  // use transcoded parameters to initialize an SWelsSPS
  m_pSpsArrayPointer = &m_pSpsArray[0];
  SSpatialLayerConfig* pDlayerParam = &(sSvcCodingParam.sSpatialLayers[0]);
  iRet = WelsInitSps (
    m_pSpsArrayPointer,
    pDlayerParam,
    &sSvcCodingParam.sDependencyLayers[0],
    sSvcCodingParam.uiIntraPeriod,
    sSvcCodingParam.iMaxNumRefFrame,
    0, //SpsId
    sSvcCodingParam.bEnableFrameCroppingFlag,
    sSvcCodingParam.iRCMode != RC_OFF_MODE,
    0, //DlayerCount
    false
    );
  EXPECT_EQ (iRet, 0);

  // SPS VSTP parameters should match the transcoded values
  EXPECT_EQ( sSvcCodingParam.sSpatialLayers[0].bVideoSignalTypePresent,   m_pSpsArrayPointer->bVideoSignalTypePresent);
  EXPECT_EQ( sSvcCodingParam.sSpatialLayers[0].uiVideoFormat,             m_pSpsArrayPointer->uiVideoFormat);
  EXPECT_EQ( sSvcCodingParam.sSpatialLayers[0].bFullRange,                m_pSpsArrayPointer->bFullRange);
  EXPECT_EQ( sSvcCodingParam.sSpatialLayers[0].bColorDescriptionPresent,  m_pSpsArrayPointer->bColorDescriptionPresent);
  EXPECT_EQ( sSvcCodingParam.sSpatialLayers[0].uiColorPrimaries,          m_pSpsArrayPointer->uiColorPrimaries);
  EXPECT_EQ( sSvcCodingParam.sSpatialLayers[0].uiTransferCharacteristics, m_pSpsArrayPointer->uiTransferCharacteristics);
  EXPECT_EQ( sSvcCodingParam.sSpatialLayers[0].uiColorMatrix,             m_pSpsArrayPointer->uiColorMatrix);

  // TODO: verify that WriteVUI (au_set.cpp) writes the SPS VSTP values to the output file (verified using FFmpeg)

  (void) iRet; // Not using iRet at the moment
}


