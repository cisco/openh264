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


