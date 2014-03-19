/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef NO_DYNAMIC_VP
#if defined(_WIN32)
#include <windows.h>
#elif defined(MACOS)
#include "bundleloader.h"
#elif defined(__GNUC__)
#include <dlfcn.h>
#endif
#endif

#include "wels_preprocess.h"
#include "picture_handle.h"
#include "encoder_context.h"
#include "utils.h"

namespace WelsSVCEnc {

#define WelsSafeDelete(p) if(p){ delete (p); (p) = NULL; }


//***** entry API declaration ************************************************************************//
typedef EResult (* pfnCreateVpInterface) (void**, int);
typedef EResult (* pfnDestroyVpInterface) (void*, int);

int32_t WelsInitScaledPic (SWelsSvcCodingParam* pParam,  Scaled_Picture*  pScaledPic, CMemoryAlign* pMemoryAlign);
bool  JudgeNeedOfScaling (SWelsSvcCodingParam* pParam, Scaled_Picture* pScaledPic);
void    FreeScaledPic (Scaled_Picture*  pScaledPic, CMemoryAlign* pMemoryAlign);

//******* table definition ***********************************************************************//
const uint8_t g_kuiRefTemporalIdx[MAX_TEMPORAL_LEVEL][MAX_GOP_SIZE] = {
  {  0, }, // 0
  {  0,  0, }, // 1
  {  0,  0,  0,  1, }, // 2
  {  0,  0,  0,  2,  0,  1,  1,  2, }, // 3
};

const int32_t g_kiPixMapSizeInBits = sizeof (uint8_t) * 8;


inline  void   WelsUpdateSpatialIdxMap (sWelsEncCtx* pEncCtx, int32_t iPos, SPicture* pPic, int32_t iDidx) {
  pEncCtx->sSpatialIndexMap[iPos].pSrc = pPic;
  pEncCtx->sSpatialIndexMap[iPos].iDid = iDidx;
}


//***************************************************************************************************//
CWelsLib::CWelsLib (sWelsEncCtx* pEncCtx) {
  m_pInterface[0] = m_pInterface[1] = NULL;

#ifndef NO_DYNAMIC_VP
#if defined(_WIN32)
  const char WelsVPLib[] = "welsvp.dll";
  HMODULE shModule = LoadLibrary (WelsVPLib);
  if (!shModule)
    WelsLog (pEncCtx, WELS_LOG_ERROR, "welsvp load lib dynamic failed module=%x\n", shModule);

#elif defined(MACOS)
  const char WelsVPLib[] = "welsvp.bundle";
  char pCurPath[256];
  GetCurrentModulePath (pCurPath, 256);
  strlcat (pCurPath, WelsVPLib, 256);
  CFBundleRef shModule = LoadBundle (pCurPath);
  if (!shModule)
    WelsLog (pEncCtx, WELS_LOG_ERROR, "welsvp load lib dynamic failed module=%x\n", shModule);

#elif defined(__GNUC__)
  const char WelsVPLib[] = "./libwelsvp.so";
  void* shModule = NULL;
  shModule = dlopen (WelsVPLib, RTLD_LAZY);
  if (shModule == NULL)
    printf ("dlopen %s iRet=%p, err=%s\n", WelsVPLib, shModule, dlerror());
#endif

  m_pVpLib = (void*)shModule;
#endif
}

CWelsLib::~CWelsLib() {
#ifndef NO_DYNAMIC_VP
  if (m_pVpLib) {
#if defined(_WIN32)
    HMODULE shModule = (HMODULE)m_pVpLib;
    FreeLibrary (shModule);

#elif defined(MACOS)
    CFBundleRef shModule = (CFBundleRef)m_pVpLib;
    FreeBundle (shModule);

#elif defined(__GNUC__)
    void* shModule = m_pVpLib;
    dlclose (shModule);
#endif
    m_pVpLib = NULL;
  }
#endif
}

void* CWelsLib::QueryFunction (const char* pName) {
  void* pFunc = NULL;

#ifndef NO_DYNAMIC_VP
  if (m_pVpLib) {
#if defined(_WIN32)
    HMODULE shModule = (HMODULE)m_pVpLib;
    pFunc = (void*)GetProcAddress (shModule, pName);

#elif defined(MACOS)
    CFBundleRef shModule = (CFBundleRef)m_pVpLib;
    pFunc = (void*)GetProcessAddress (shModule, pName);

#elif defined(__GNUC__)
    void* shModule = m_pVpLib;
    pFunc = (void*)dlsym (shModule, pName);
    if (pFunc == NULL)
      printf ("dlsym %p iRet=%p, err=%s\n", shModule, pFunc, dlerror());
#endif
  }
#endif
  return pFunc;
}

int32_t CWelsLib::CreateIface (IWelsVP** ppInterfaceVp) {
  *ppInterfaceVp = NULL;
#ifndef NO_DYNAMIC_VP
  if (m_pVpLib) {

#endif
    pfnCreateVpInterface  pCreateVpInterface  = NULL;
    pfnDestroyVpInterface pDestroyVpInterface = NULL;

#ifndef NO_DYNAMIC_VP
    pCreateVpInterface  = (pfnCreateVpInterface)  QueryFunction ("CreateVpInterface");
    pDestroyVpInterface = (pfnDestroyVpInterface) QueryFunction ("DestroyVpInterface");
#else
    pCreateVpInterface  = CreateVpInterface;
    // TODO(ekr@rtfm.com): This cast corrects a signature difference... This is a potential real problem
    pDestroyVpInterface = (pfnDestroyVpInterface)DestroyVpInterface;
#endif

    m_pInterface[0] = (void*)pCreateVpInterface;
    m_pInterface[1] = (void*)pDestroyVpInterface;

    if (m_pInterface[0] && m_pInterface[1])
      pCreateVpInterface ((void**)ppInterfaceVp, WELSVP_INTERFACE_VERION);
#ifndef NO_DYNAMIC_VP
  } else {
  }
#endif

  return (*ppInterfaceVp) ? 0 : 1;
}

int32_t CWelsLib::DestroyIface (IWelsVP* pInterfaceVp) {
  if (pInterfaceVp) {
    pfnDestroyVpInterface pDestroyVpInterface = (pfnDestroyVpInterface) m_pInterface[1];
    if (pDestroyVpInterface) {
      pDestroyVpInterface (pInterfaceVp, WELSVP_INTERFACE_VERION);
    } else {
    }
  }

  return 0;
}

/***************************************************************************
*
*	implement of the interface
*
***************************************************************************/

CWelsPreProcess::CWelsPreProcess (sWelsEncCtx* pEncCtx) {
  m_pInterfaceVp = NULL;
  m_pEncLib = NULL;
  m_bInitDone = false;
  m_pEncCtx = pEncCtx;
  memset (&m_sScaledPicture, 0, sizeof (m_sScaledPicture));
  memset (m_pSpatialPic, 0, sizeof(m_pSpatialPic));
  memset (m_uiSpatialLayersInTemporal, 0, sizeof(m_uiSpatialLayersInTemporal));
  memset (m_uiSpatialPicNum, 0, sizeof(m_uiSpatialPicNum));
}

CWelsPreProcess::~CWelsPreProcess() {
  FreeScaledPic (&m_sScaledPicture,  m_pEncCtx->pMemAlign);
  WelsPreprocessDestroy();
}

int32_t CWelsPreProcess::WelsPreprocessCreate() {
  if (m_pEncLib == NULL && m_pInterfaceVp == NULL) {
    m_pEncLib  = new CWelsLib (m_pEncCtx);
    if (!m_pEncLib)
      goto exit;

    m_pEncLib->CreateIface (&m_pInterfaceVp);
    if (!m_pInterfaceVp)
      goto exit;
  } else
    goto exit;

  return 0;

exit:
  WelsPreprocessDestroy();
  return 1;
}

int32_t CWelsPreProcess::WelsPreprocessDestroy() {
  if (m_pEncLib) {
    m_pEncLib->DestroyIface (m_pInterfaceVp);
    m_pInterfaceVp = NULL;
    WelsSafeDelete (m_pEncLib);
  }

  return 0;
}

int32_t CWelsPreProcess::WelsPreprocessReset (sWelsEncCtx* pCtx) {
  int32_t iRet = -1;

  if (pCtx) {
    FreeScaledPic (&m_sScaledPicture, pCtx->pMemAlign);
    iRet = InitLastSpatialPictures (pCtx);
    iRet = WelsInitScaledPic (pCtx->pSvcParam, &m_sScaledPicture, pCtx->pMemAlign);
  }

  return iRet;
}

int32_t CWelsPreProcess::AllocSpatialPictures (sWelsEncCtx* pCtx, SWelsSvcCodingParam* pParam) {
  CMemoryAlign* pMa						= pCtx->pMemAlign;
  const int32_t kiDlayerCount					= pParam->iSpatialLayerNum;
  int32_t iDlayerIndex							= 0;

  // spatial pictures
  iDlayerIndex = 0;
  do {
    const int32_t kiPicWidth = pParam->sDependencyLayers[iDlayerIndex].iFrameWidth;
    const int32_t kiPicHeight   = pParam->sDependencyLayers[iDlayerIndex].iFrameHeight;
    const uint8_t kuiLayerInTemporal = 2 + WELS_MAX (pParam->sDependencyLayers[iDlayerIndex].iHighestTemporalId, 1);
    const uint8_t kuiRefNumInTemporal = kuiLayerInTemporal + pParam->iLTRRefNum;
    uint8_t i = 0;

    do {
      SPicture* pPic = AllocPicture (pMa, kiPicWidth, kiPicHeight, false);
      WELS_VERIFY_RETURN_IF(1, (NULL == pPic))
      m_pSpatialPic[iDlayerIndex][i] = pPic;
      ++ i;
    } while (i < kuiRefNumInTemporal);

    m_uiSpatialLayersInTemporal[iDlayerIndex] = kuiLayerInTemporal;
    m_uiSpatialPicNum[iDlayerIndex] = kuiRefNumInTemporal;
    ++ iDlayerIndex;
  } while (iDlayerIndex < kiDlayerCount);

  return 0;
}

void CWelsPreProcess::FreeSpatialPictures (sWelsEncCtx* pCtx) {
  CMemoryAlign* pMa	= pCtx->pMemAlign;
  int32_t j = 0;
  while (j < pCtx->pSvcParam->iSpatialLayerNum) {
    uint8_t i = 0;
    uint8_t uiRefNumInTemporal = m_uiSpatialPicNum[j];

    while (i < uiRefNumInTemporal) {
      if (NULL != m_pSpatialPic[j][i]) {
        FreePicture (pMa, &m_pSpatialPic[j][i]);
      }
      ++ i;
    }
    m_uiSpatialLayersInTemporal[j] = 0;
    ++ j;
  }
}

int32_t CWelsPreProcess::BuildSpatialPicList (sWelsEncCtx* pCtx, const SSourcePicture* kpSrcPic) {
  SWelsSvcCodingParam* pSvcParam = pCtx->pSvcParam;
  int32_t iSpatialNum = 0;

  if (!m_bInitDone) {
    if (WelsPreprocessCreate() != 0)
      return -1;
    if (WelsPreprocessReset (pCtx) != 0)
      return -1;

    m_bInitDone = true;
  }

  if (m_pInterfaceVp == NULL)
    return -1;

  pCtx->pVaa->bSceneChangeFlag = pCtx->pVaa->bIdrPeriodFlag = false;
  if (pSvcParam->uiIntraPeriod)
    pCtx->pVaa->bIdrPeriodFlag = (1 + pCtx->iFrameIndex >= (int32_t)pSvcParam->uiIntraPeriod) ? true : false;

  iSpatialNum = SingleLayerPreprocess (pCtx, kpSrcPic, &m_sScaledPicture);

  return iSpatialNum;
}

int32_t CWelsPreProcess::AnalyzeSpatialPic (sWelsEncCtx* pCtx, const int32_t kiDidx) {
  SWelsSvcCodingParam* pSvcParam = pCtx->pSvcParam;
  bool bNeededMbAq = (pSvcParam->bEnableAdaptiveQuant && (pCtx->eSliceType == P_SLICE));
  bool bCalculateBGD = (pCtx->eSliceType == P_SLICE && pSvcParam->bEnableBackgroundDetection);

  int32_t iCurTemporalIdx  = m_uiSpatialLayersInTemporal[kiDidx] - 1;

  int32_t iRefTemporalIdx = (int32_t)g_kuiRefTemporalIdx[pSvcParam->iDecompStages][pCtx->iCodingIndex &
                            (pSvcParam->uiGopSize - 1)];
  if (pCtx->uiTemporalId == 0 && pCtx->pLtr[pCtx->uiDependencyId].bReceivedT0LostFlag)
    iRefTemporalIdx = m_uiSpatialLayersInTemporal[kiDidx] + pCtx->pVaa->uiValidLongTermPicIdx;

  SPicture* pCurPic = m_pSpatialPic[kiDidx][iCurTemporalIdx];
  SPicture* pRefPic = m_pSpatialPic[kiDidx][iRefTemporalIdx];
  {
    SPicture* pLastPic = m_pLastSpatialPicture[kiDidx][0];
    bool bCalculateSQDiff = ((pLastPic->pData[0] == pRefPic->pData[0]) && bNeededMbAq);
    bool bCalculateVar = (pSvcParam->iRCMode == RC_MODE1 && pCtx->eSliceType == I_SLICE);

    VaaCalculation (pCtx->pVaa, pCurPic, pRefPic, bCalculateSQDiff, bCalculateVar, bCalculateBGD);
  }

  if (pSvcParam->bEnableBackgroundDetection) {
    BackgroundDetection (pCtx->pVaa, pCurPic, pRefPic, bCalculateBGD && pRefPic->iPictureType != I_SLICE);
  }

  if (bNeededMbAq) {
    SPicture* pCurPic = m_pLastSpatialPicture[kiDidx][1];
    SPicture* pRefPic = m_pLastSpatialPicture[kiDidx][0];

    AdaptiveQuantCalculation (pCtx->pVaa, pCurPic, pRefPic);
  }

  if (pSvcParam->bEnableRc) {
    AnalyzePictureComplexity (pCtx, pCurPic, pRefPic, kiDidx, bCalculateBGD);
  }

  WelsExchangeSpatialPictures (&m_pLastSpatialPicture[kiDidx][1], &m_pLastSpatialPicture[kiDidx][0]);

  return 0;
}

int32_t CWelsPreProcess::UpdateSpatialPictures(sWelsEncCtx* pCtx, SWelsSvcCodingParam* pParam,
    const int8_t iCurTid, const int32_t d_idx) {
  if (iCurTid < m_uiSpatialLayersInTemporal[d_idx] - 1 || pParam->iDecompStages == 0){
    if ((iCurTid >= MAX_TEMPORAL_LEVEL) || (m_uiSpatialLayersInTemporal[d_idx] - 1 > MAX_TEMPORAL_LEVEL)) {
      InitLastSpatialPictures(pCtx);
      return 1;
    }
    if (pParam->bEnableLongTermReference && pCtx->bLongTermRefFlag[d_idx][iCurTid]) {
      SPicture* tmp	= m_pSpatialPic[d_idx][m_uiSpatialLayersInTemporal[d_idx] + pCtx->pVaa->uiMarkLongTermPicIdx];
      m_pSpatialPic[d_idx][m_uiSpatialLayersInTemporal[d_idx] + pCtx->pVaa->uiMarkLongTermPicIdx] =
      m_pSpatialPic[d_idx][iCurTid];
      m_pSpatialPic[d_idx][iCurTid] = m_pSpatialPic[d_idx][m_uiSpatialLayersInTemporal[d_idx] - 1];
      m_pSpatialPic[d_idx][m_uiSpatialLayersInTemporal[d_idx] - 1] = tmp;
      pCtx->bLongTermRefFlag[d_idx][iCurTid] = false;
    } else {
      WelsExchangeSpatialPictures (&m_pSpatialPic[d_idx][m_uiSpatialLayersInTemporal[d_idx] - 1],
                                   &m_pSpatialPic[d_idx][iCurTid]);
    }
  }
  return 0;
}


/*
*	SingleLayerPreprocess: down sampling if applicable
*  @return:	exact number of spatial layers need to encoder indeed
*/
int32_t CWelsPreProcess::SingleLayerPreprocess (sWelsEncCtx* pCtx, const SSourcePicture* kpSrc,
    Scaled_Picture* pScaledPicture) {
  SWelsSvcCodingParam* pSvcParam    = pCtx->pSvcParam;
  int8_t  iDependencyId             = pSvcParam->iSpatialLayerNum - 1;
  int32_t iPicturePos               = m_uiSpatialLayersInTemporal[iDependencyId] - 1;

  SPicture* pSrcPic					= NULL;	// large
  SPicture* pDstPic					= NULL;	// small
  SDLayerParam* pDlayerParam					= NULL;
  int32_t iSpatialNum					= 0;
  int32_t iSrcWidth					= 0;
  int32_t iSrcHeight					= 0;
  int32_t iTargetWidth					= 0;
  int32_t iTargetHeight					= 0;
  int32_t iTemporalId = 0;
  int32_t iActualSpatialLayerNum      = 0;

  pDlayerParam = &pSvcParam->sDependencyLayers[iDependencyId];
  iTargetWidth	  = pDlayerParam->iFrameWidth;
  iTargetHeight  = pDlayerParam->iFrameHeight;
  iTemporalId    = pDlayerParam->uiCodingIdx2TemporalId[pCtx->iCodingIndex & (pSvcParam->uiGopSize - 1)];
  iSrcWidth   = pSvcParam->SUsedPicRect.iWidth;
  iSrcHeight  = pSvcParam->SUsedPicRect.iHeight;

  pSrcPic = pScaledPicture->pScaledInputPicture ? pScaledPicture->pScaledInputPicture :
            m_pSpatialPic[iDependencyId][iPicturePos];

  WelsMoveMemoryWrapper (pSvcParam, pSrcPic, kpSrc, iSrcWidth, iSrcHeight);

  if (pSvcParam->bEnableDenoise)
    BilateralDenoising (pSrcPic, iSrcWidth, iSrcHeight);

  // different scaling in between input picture and dst highest spatial picture.
  int32_t iShrinkWidth  = iSrcWidth;
  int32_t iShrinkHeight = iSrcHeight;
  pDstPic = pSrcPic;
  if (pScaledPicture->pScaledInputPicture) {
    // for highest downsampling
    pDstPic		= m_pSpatialPic[iDependencyId][iPicturePos];
    iShrinkWidth = pScaledPicture->iScaledWidth[iDependencyId];
    iShrinkHeight = pScaledPicture->iScaledHeight[iDependencyId];
  }
  DownsamplePadding (pSrcPic, pDstPic, iSrcWidth, iSrcHeight, iShrinkWidth, iShrinkHeight, iTargetWidth, iTargetHeight);

  if (pSvcParam->bEnableSceneChangeDetect && !pCtx->pVaa->bIdrPeriodFlag
      && !pCtx->bEncCurFrmAsIdrFlag
      && ! (pCtx->iCodingIndex & (pSvcParam->uiGopSize - 1))) {
    SPicture* pRefPic = pCtx->pLtr[iDependencyId].bReceivedT0LostFlag ?
                        m_pSpatialPic[iDependencyId][m_uiSpatialLayersInTemporal[iDependencyId] +
                            pCtx->pVaa->uiValidLongTermPicIdx] : m_pLastSpatialPicture[iDependencyId][0];

    pCtx->pVaa->bSceneChangeFlag = DetectSceneChange (pDstPic, pRefPic);
  }

  for (int32_t i = 0; i < pSvcParam->iSpatialLayerNum; i++) {
    if (pSvcParam->sDependencyLayers[i].uiCodingIdx2TemporalId[pCtx->iCodingIndex & (pSvcParam->uiGopSize - 1)]
        != INVALID_TEMPORAL_ID) {
      ++ iActualSpatialLayerNum;
    }
  }

  if (iTemporalId != INVALID_TEMPORAL_ID) {
    WelsUpdateSpatialIdxMap (pCtx, iActualSpatialLayerNum - 1, pDstPic, iDependencyId);
    ++ iSpatialNum;
    -- iActualSpatialLayerNum;
  }

  m_pLastSpatialPicture[iDependencyId][1]	= m_pSpatialPic[iDependencyId][iPicturePos];
  -- iDependencyId;

  // generate other spacial layer
  // pSrc is
  //	-- padded input pic, if downsample should be applied to generate highest layer, [if] block above
  //	-- highest layer, if no downsampling, [else] block above
  if (pSvcParam->iSpatialLayerNum > 1) {
    while (iDependencyId >= 0) {
      pDlayerParam			= &pSvcParam->sDependencyLayers[iDependencyId];
      iTargetWidth	= pDlayerParam->iFrameWidth;
      iTargetHeight	= pDlayerParam->iFrameHeight;
      iTemporalId = pDlayerParam->uiCodingIdx2TemporalId[pCtx->iCodingIndex & (pSvcParam->uiGopSize - 1)];
      iPicturePos		= m_uiSpatialLayersInTemporal[iDependencyId] - 1;

      // NOT work for CGS, FIXME
      // spatial layer is able to encode indeed
      if ((iTemporalId != INVALID_TEMPORAL_ID)) {
        // down sampling performed

        pDstPic	= m_pSpatialPic[iDependencyId][iPicturePos];	// small
        iShrinkWidth = pScaledPicture->iScaledWidth[iDependencyId];
        iShrinkHeight = pScaledPicture->iScaledHeight[iDependencyId];
        DownsamplePadding (pSrcPic, pDstPic, iSrcWidth, iSrcHeight, iShrinkWidth, iShrinkHeight, iTargetWidth, iTargetHeight);

        WelsUpdateSpatialIdxMap (pCtx, iActualSpatialLayerNum - 1, pDstPic, iDependencyId);

        -- iActualSpatialLayerNum;
        ++ iSpatialNum;

        m_pLastSpatialPicture[iDependencyId][1]	= m_pSpatialPic[iDependencyId][iPicturePos];
      }
      -- iDependencyId;
    }
  }

  return iSpatialNum;
}


/*!
 * \brief	Whether input picture need be scaled?
 */
bool JudgeNeedOfScaling (SWelsSvcCodingParam* pParam, Scaled_Picture* pScaledPicture) {
  const int32_t kiInputPicWidth	= pParam->SUsedPicRect.iWidth;
  const int32_t kiInputPicHeight = pParam->SUsedPicRect.iHeight;
  const int32_t kiDstPicWidth		= pParam->sDependencyLayers[pParam->iSpatialLayerNum - 1].iActualWidth;
  const int32_t kiDstPicHeight	= pParam->sDependencyLayers[pParam->iSpatialLayerNum - 1].iActualHeight;
  bool bNeedDownsampling = true;

  int32_t iSpatialIdx = pParam->iSpatialLayerNum - 1;

  if (kiDstPicWidth >= kiInputPicWidth && kiDstPicHeight >= kiInputPicHeight) {
    iSpatialIdx --;  // highest D layer do not need downsampling
    bNeedDownsampling = false;
  }

  for (; iSpatialIdx >= 0; iSpatialIdx --) {
    SDLayerParam* pCurLayer = &pParam->sDependencyLayers[iSpatialIdx];
    int32_t iCurDstWidth			= pCurLayer->iActualWidth;
    int32_t iCurDstHeight			= pCurLayer->iActualHeight;
    int32_t iInputWidthXDstHeight	= kiInputPicWidth * iCurDstHeight;
    int32_t iInputHeightXDstWidth	= kiInputPicHeight * iCurDstWidth;

    if (iInputWidthXDstHeight > iInputHeightXDstWidth) {
      pScaledPicture->iScaledWidth[iSpatialIdx] = iCurDstWidth;
      pScaledPicture->iScaledHeight[iSpatialIdx] = iInputHeightXDstWidth / kiInputPicWidth;
    } else {
      pScaledPicture->iScaledWidth[iSpatialIdx] = iInputWidthXDstHeight / kiInputPicHeight;
      pScaledPicture->iScaledHeight[iSpatialIdx] = iCurDstHeight;
    }
  }

  return bNeedDownsampling;
}

int32_t  WelsInitScaledPic (SWelsSvcCodingParam* pParam,  Scaled_Picture*  pScaledPicture, CMemoryAlign* pMemoryAlign) {
  bool bInputPicNeedScaling = JudgeNeedOfScaling (pParam, pScaledPicture);
  if (bInputPicNeedScaling) {
    pScaledPicture->pScaledInputPicture = AllocPicture (pMemoryAlign, pParam->SUsedPicRect.iWidth,
                                          pParam->SUsedPicRect.iHeight, false);
    if (pScaledPicture->pScaledInputPicture == NULL)
      return -1;
  }
  return 0;
}

void  FreeScaledPic (Scaled_Picture*  pScaledPicture, CMemoryAlign* pMemoryAlign) {
  if (pScaledPicture->pScaledInputPicture) {
    FreePicture (pMemoryAlign, &pScaledPicture->pScaledInputPicture);
    pScaledPicture->pScaledInputPicture = NULL;
  }
}

int32_t CWelsPreProcess::InitLastSpatialPictures (sWelsEncCtx* pCtx) {
  SWelsSvcCodingParam* pParam	= pCtx->pSvcParam;
  const int32_t kiDlayerCount			= pParam->iSpatialLayerNum;
  int32_t iDlayerIndex					= 0;

  for (; iDlayerIndex < kiDlayerCount; iDlayerIndex++) {
    const int32_t kiLayerInTemporal = m_uiSpatialLayersInTemporal[iDlayerIndex];
    m_pLastSpatialPicture[iDlayerIndex][0]	= m_pSpatialPic[iDlayerIndex][kiLayerInTemporal - 2];
    m_pLastSpatialPicture[iDlayerIndex][1]	= NULL;
  }
  for (; iDlayerIndex < MAX_DEPENDENCY_LAYER; iDlayerIndex++) {
    m_pLastSpatialPicture[iDlayerIndex][0]	= m_pLastSpatialPicture[iDlayerIndex][1] = NULL;
  }

  return 0;
}
//*********************************************************************************************************/

int32_t CWelsPreProcess::ColorspaceConvert (SWelsSvcCodingParam* pSvcParam, SPicture* pDstPic,
    const SSourcePicture* kpSrc, const int32_t kiWidth, const int32_t kiHeight) {
  return 1;
  //not support yet
}

void CWelsPreProcess::BilateralDenoising (SPicture* pSrc, const int32_t kiWidth, const int32_t kiHeight) {
  int32_t iMethodIdx = METHOD_DENOISE;
  SPixMap sSrcPixMap = {0};

  sSrcPixMap.pPixel[0] = pSrc->pData[0];
  sSrcPixMap.pPixel[1] = pSrc->pData[1];
  sSrcPixMap.pPixel[2] = pSrc->pData[2];
  sSrcPixMap.iSizeInBits = g_kiPixMapSizeInBits;
  sSrcPixMap.sRect.iRectWidth = kiWidth;
  sSrcPixMap.sRect.iRectHeight = kiHeight;
  sSrcPixMap.iStride[0] = pSrc->iLineSize[0];
  sSrcPixMap.iStride[1] = pSrc->iLineSize[1];
  sSrcPixMap.iStride[2] = pSrc->iLineSize[2];
  sSrcPixMap.eFormat = VIDEO_FORMAT_I420;

  m_pInterfaceVp->Process (iMethodIdx, &sSrcPixMap, NULL);
}

bool CWelsPreProcess::DetectSceneChange (SPicture* pCurPicture, SPicture* pRefPicture) {
  bool bSceneChangeFlag = false;
  int32_t iMethodIdx = METHOD_SCENE_CHANGE_DETECTION_VIDEO;
  SSceneChangeResult sSceneChangeDetectResult = { SIMILAR_SCENE };
  SPixMap sSrcPixMap = {0};
  SPixMap sRefPixMap = {0};

  sSrcPixMap.pPixel[0] = pCurPicture->pData[0];
  sSrcPixMap.iSizeInBits = g_kiPixMapSizeInBits;
  sSrcPixMap.iStride[0] = pCurPicture->iLineSize[0];
  sSrcPixMap.sRect.iRectWidth = pCurPicture->iWidthInPixel;
  sSrcPixMap.sRect.iRectHeight = pCurPicture->iHeightInPixel;
  sSrcPixMap.eFormat = VIDEO_FORMAT_I420;


  sRefPixMap.pPixel[0] = pRefPicture->pData[0];
  sRefPixMap.iSizeInBits = g_kiPixMapSizeInBits;
  sRefPixMap.iStride[0] = pRefPicture->iLineSize[0];
  sRefPixMap.sRect.iRectWidth = pRefPicture->iWidthInPixel;
  sRefPixMap.sRect.iRectHeight = pRefPicture->iHeightInPixel;
  sRefPixMap.eFormat = VIDEO_FORMAT_I420;

  int32_t iRet = m_pInterfaceVp->Process (iMethodIdx, &sSrcPixMap, &sRefPixMap);
  if (iRet == 0) {
    m_pInterfaceVp->Get (iMethodIdx, (void*)&sSceneChangeDetectResult);
    bSceneChangeFlag = (sSceneChangeDetectResult.eSceneChangeIdc == LARGE_CHANGED_SCENE) ? true : false;
  }

  return bSceneChangeFlag;
}

int32_t CWelsPreProcess::DownsamplePadding (SPicture* pSrc, SPicture* pDstPic,  int32_t iSrcWidth, int32_t iSrcHeight,
    int32_t iShrinkWidth, int32_t iShrinkHeight, int32_t iTargetWidth, int32_t iTargetHeight) {
  int32_t iRet = 0;
  SPixMap sSrcPixMap = {0};
  SPixMap sDstPicMap = {0};

  sSrcPixMap.pPixel[0]   = pSrc->pData[0];
  sSrcPixMap.pPixel[1]   = pSrc->pData[1];
  sSrcPixMap.pPixel[2]   = pSrc->pData[2];
  sSrcPixMap.iSizeInBits = g_kiPixMapSizeInBits;
  sSrcPixMap.sRect.iRectWidth  = iSrcWidth;
  sSrcPixMap.sRect.iRectHeight = iSrcHeight;
  sSrcPixMap.iStride[0]  = pSrc->iLineSize[0];
  sSrcPixMap.iStride[1]  = pSrc->iLineSize[1];
  sSrcPixMap.iStride[2]  = pSrc->iLineSize[2];
  sSrcPixMap.eFormat     = VIDEO_FORMAT_I420;

  if (iSrcWidth != iShrinkWidth || iSrcHeight != iShrinkHeight) {
    int32_t iMethodIdx = METHOD_DOWNSAMPLE;
    sDstPicMap.pPixel[0]   = pDstPic->pData[0];
    sDstPicMap.pPixel[1]   = pDstPic->pData[1];
    sDstPicMap.pPixel[2]   = pDstPic->pData[2];
    sDstPicMap.iSizeInBits = g_kiPixMapSizeInBits;
    sDstPicMap.sRect.iRectWidth  = iShrinkWidth;
    sDstPicMap.sRect.iRectHeight = iShrinkHeight;
    sDstPicMap.iStride[0]  = pDstPic->iLineSize[0];
    sDstPicMap.iStride[1]  = pDstPic->iLineSize[1];
    sDstPicMap.iStride[2]  = pDstPic->iLineSize[2];
    sDstPicMap.eFormat     = VIDEO_FORMAT_I420;

    iRet = m_pInterfaceVp->Process (iMethodIdx, &sSrcPixMap, &sDstPicMap);
  } else {
    memcpy (&sDstPicMap, &sSrcPixMap, sizeof (sDstPicMap));	// confirmed_safe_unsafe_usage
  }

  // get rid of odd line
  iShrinkWidth -= (iShrinkWidth & 1);
  iShrinkHeight -= (iShrinkHeight & 1);
  Padding ((uint8_t*)sDstPicMap.pPixel[0], (uint8_t*)sDstPicMap.pPixel[1], (uint8_t*)sDstPicMap.pPixel[2],
           sDstPicMap.iStride[0], sDstPicMap.iStride[1],	iShrinkWidth, iTargetWidth, iShrinkHeight, iTargetHeight);

  return iRet;
}

//*********************************************************************************************************/
void CWelsPreProcess::VaaCalculation (SVAAFrameInfo* pVaaInfo, SPicture* pCurPicture, SPicture* pRefPicture,
                                      bool bCalculateSQDiff, bool bCalculateVar, bool bCalculateBGD) {
  pVaaInfo->sVaaCalcInfo.pCurY = pCurPicture->pData[0];
  pVaaInfo->sVaaCalcInfo.pRefY = pRefPicture->pData[0];
  {
    int32_t iMethodIdx = METHOD_VAA_STATISTICS;
    SPixMap sCurPixMap = {0};
    SPixMap sRefPixMap = {0};
    SVAACalcParam calc_param = {0};

    sCurPixMap.pPixel[0] = pCurPicture->pData[0];
    sCurPixMap.iSizeInBits = g_kiPixMapSizeInBits;
    sCurPixMap.sRect.iRectWidth = pCurPicture->iWidthInPixel;
    sCurPixMap.sRect.iRectHeight = pCurPicture->iHeightInPixel;
    sCurPixMap.iStride[0] = pCurPicture->iLineSize[0];
    sCurPixMap.eFormat = VIDEO_FORMAT_I420;

    sRefPixMap.pPixel[0] = pRefPicture->pData[0];
    sRefPixMap.iSizeInBits = g_kiPixMapSizeInBits;
    sRefPixMap.sRect.iRectWidth = pRefPicture->iWidthInPixel;
    sRefPixMap.sRect.iRectHeight = pRefPicture->iHeightInPixel;
    sRefPixMap.iStride[0] = pRefPicture->iLineSize[0];
    sRefPixMap.eFormat = VIDEO_FORMAT_I420;

    calc_param.iCalcVar	= bCalculateVar;
    calc_param.iCalcBgd	= bCalculateBGD;
    calc_param.iCalcSsd	= bCalculateSQDiff;
    calc_param.pCalcResult = &pVaaInfo->sVaaCalcInfo;

    m_pInterfaceVp->Set (iMethodIdx, &calc_param);
    m_pInterfaceVp->Process (iMethodIdx, &sCurPixMap, &sRefPixMap);
  }
}

void CWelsPreProcess::BackgroundDetection (SVAAFrameInfo* pVaaInfo, SPicture* pCurPicture, SPicture* pRefPicture,
    bool bDetectFlag) {
  if (bDetectFlag) {
    pVaaInfo->iPicWidth     = pCurPicture->iWidthInPixel;
    pVaaInfo->iPicHeight    = pCurPicture->iHeightInPixel;

    pVaaInfo->iPicStride	= pCurPicture->iLineSize[0];
    pVaaInfo->iPicStrideUV	= pCurPicture->iLineSize[1];
    pVaaInfo->pCurY			= pCurPicture->pData[0];
    pVaaInfo->pRefY			= pRefPicture->pData[0];
    pVaaInfo->pCurU			= pCurPicture->pData[1];
    pVaaInfo->pRefU			= pRefPicture->pData[1];
    pVaaInfo->pCurV			= pCurPicture->pData[2];
    pVaaInfo->pRefV			= pRefPicture->pData[2];

    int32_t iMethodIdx = METHOD_BACKGROUND_DETECTION;
    SPixMap sSrcPixMap = {0};
    SPixMap sRefPixMap = {0};
    SBGDInterface BGDParam = {0};

    sSrcPixMap.pPixel[0] = pCurPicture->pData[0];
    sSrcPixMap.pPixel[1] = pCurPicture->pData[1];
    sSrcPixMap.pPixel[2] = pCurPicture->pData[2];
    sSrcPixMap.iSizeInBits = g_kiPixMapSizeInBits;
    sSrcPixMap.iStride[0] = pCurPicture->iLineSize[0];
    sSrcPixMap.iStride[1] = pCurPicture->iLineSize[1];
    sSrcPixMap.iStride[2] = pCurPicture->iLineSize[2];
    sSrcPixMap.sRect.iRectWidth = pCurPicture->iWidthInPixel;
    sSrcPixMap.sRect.iRectHeight = pCurPicture->iHeightInPixel;
    sSrcPixMap.eFormat = VIDEO_FORMAT_I420;

    sRefPixMap.pPixel[0] = pRefPicture->pData[0];
    sRefPixMap.pPixel[1] = pRefPicture->pData[1];
    sRefPixMap.pPixel[2] = pRefPicture->pData[2];
    sRefPixMap.iSizeInBits = g_kiPixMapSizeInBits;
    sRefPixMap.iStride[0] = pRefPicture->iLineSize[0];
    sRefPixMap.iStride[1] = pRefPicture->iLineSize[1];
    sRefPixMap.iStride[2] = pRefPicture->iLineSize[2];
    sRefPixMap.sRect.iRectWidth = pRefPicture->iWidthInPixel;
    sRefPixMap.sRect.iRectHeight = pRefPicture->iHeightInPixel;
    sRefPixMap.eFormat = VIDEO_FORMAT_I420;

    BGDParam.pBackgroundMbFlag = pVaaInfo->pVaaBackgroundMbFlag;
    BGDParam.pCalcRes = & (pVaaInfo->sVaaCalcInfo);
    m_pInterfaceVp->Set (iMethodIdx, (void*)&BGDParam);
    m_pInterfaceVp->Process (iMethodIdx, &sSrcPixMap, &sRefPixMap);
  } else {
    int32_t	iPicWidthInMb	= (pCurPicture->iWidthInPixel + 15) >> 4;
    int32_t	iPicHeightInMb = (pCurPicture->iHeightInPixel + 15) >> 4;
    memset (pVaaInfo->pVaaBackgroundMbFlag, 0, iPicWidthInMb * iPicHeightInMb);
  }
}

void CWelsPreProcess::AdaptiveQuantCalculation (SVAAFrameInfo* pVaaInfo, SPicture* pCurPicture, SPicture* pRefPicture) {
  pVaaInfo->sAdaptiveQuantParam.pCalcResult = & (pVaaInfo->sVaaCalcInfo);
  pVaaInfo->sAdaptiveQuantParam.dAverMotionTextureIndexToDeltaQp = 0;

  {
    int32_t iMethodIdx = METHOD_ADAPTIVE_QUANT;
    SPixMap pSrc = {0};
    SPixMap pRef = {0};
    int32_t iRet = 0;

    pSrc.pPixel[0] = pCurPicture->pData[0];
    pSrc.iSizeInBits = g_kiPixMapSizeInBits;
    pSrc.iStride[0] = pCurPicture->iLineSize[0];
    pSrc.sRect.iRectWidth = pCurPicture->iWidthInPixel;
    pSrc.sRect.iRectHeight = pCurPicture->iHeightInPixel;
    pSrc.eFormat = VIDEO_FORMAT_I420;

    pRef.pPixel[0] = pRefPicture->pData[0];
    pRef.iSizeInBits = g_kiPixMapSizeInBits;
    pRef.iStride[0] = pRefPicture->iLineSize[0];
    pRef.sRect.iRectWidth = pRefPicture->iWidthInPixel;
    pRef.sRect.iRectHeight = pRefPicture->iHeightInPixel;
    pRef.eFormat = VIDEO_FORMAT_I420;

    iRet = m_pInterfaceVp->Set (iMethodIdx, (void*) & (pVaaInfo->sAdaptiveQuantParam));
    iRet = m_pInterfaceVp->Process (iMethodIdx, &pSrc, &pRef);
    if (iRet == 0)
      m_pInterfaceVp->Get (iMethodIdx, (void*) & (pVaaInfo->sAdaptiveQuantParam));
  }
}

void CWelsPreProcess::SetRefMbType (sWelsEncCtx* pCtx, uint32_t** pRefMbTypeArray, int32_t iRefPicType) {
  const uint8_t uiTid	    = pCtx->uiTemporalId;
  const uint8_t uiDid       = pCtx->uiDependencyId;
  SRefList* pRefPicLlist    = pCtx->ppRefPicListExt[uiDid];
  SLTRState* pLtr	    = &pCtx->pLtr[uiDid];
  uint8_t i							= 0;

  if (pCtx->pSvcParam->bEnableLongTermReference && pLtr->bReceivedT0LostFlag && uiTid == 0) {
    for (i = 0; i < pRefPicLlist->uiLongRefCount; i++) {
      SPicture* pRef = pRefPicLlist->pLongRefList[i];
      if (pRef != NULL && pRef->uiRecieveConfirmed == 1/*RECIEVE_SUCCESS*/) {
        *pRefMbTypeArray = pRef->uiRefMbType;
        break;
      }
    }
  } else {
    for (i = 0; i < pRefPicLlist->uiShortRefCount; i++) {
      SPicture* pRef = pRefPicLlist->pShortRefList[i];
      if (pRef != NULL && pRef->bUsedAsRef && pRef->iFramePoc >= 0 && pRef->uiTemporalId <= uiTid) {
        *pRefMbTypeArray = pRef->uiRefMbType;
        break;
      }
    }
  }
}


void CWelsPreProcess::AnalyzePictureComplexity (sWelsEncCtx* pCtx, SPicture* pCurPicture, SPicture* pRefPicture,
    const int32_t kiDependencyId, const bool bCalculateBGD) {
  SWelsSvcCodingParam* pSvcParam = pCtx->pSvcParam;
  SVAAFrameInfo* pVaaInfo	 = pCtx->pVaa;

  SComplexityAnalysisParam* sComplexityAnalysisParam = & (pVaaInfo->sComplexityAnalysisParam);
  SWelsSvcRc* SWelsSvcRc = &pCtx->pWelsSvcRc[kiDependencyId];
  int32_t iComplexityAnalysisMode = 0;

  if (pSvcParam->iRCMode == RC_MODE0 && pCtx->eSliceType == P_SLICE) {
    iComplexityAnalysisMode = FRAME_SAD;
  } else if (pSvcParam->iRCMode == RC_MODE1 && pCtx->eSliceType == P_SLICE) {
    iComplexityAnalysisMode = GOM_SAD;
  } else if (pSvcParam->iRCMode == RC_MODE1 && pCtx->eSliceType == I_SLICE) {
    iComplexityAnalysisMode = GOM_VAR;
  } else {
    return;
  }

  sComplexityAnalysisParam->iComplexityAnalysisMode = iComplexityAnalysisMode;
  sComplexityAnalysisParam->pCalcResult = & (pVaaInfo->sVaaCalcInfo);
  sComplexityAnalysisParam->pBackgroundMbFlag = pVaaInfo->pVaaBackgroundMbFlag;
  SetRefMbType (pCtx, & (sComplexityAnalysisParam->uiRefMbType), pRefPicture->iPictureType);
  sComplexityAnalysisParam->iCalcBgd = bCalculateBGD;
  sComplexityAnalysisParam->iFrameComplexity = 0;

  memset (SWelsSvcRc->pGomForegroundBlockNum, 0, SWelsSvcRc->iGomSize * sizeof (int32_t));
  if (iComplexityAnalysisMode != FRAME_SAD)
    memset (SWelsSvcRc->pCurrentFrameGomSad, 0, SWelsSvcRc->iGomSize * sizeof (int32_t));

  sComplexityAnalysisParam->pGomComplexity = SWelsSvcRc->pCurrentFrameGomSad;
  sComplexityAnalysisParam->pGomForegroundBlockNum = SWelsSvcRc->pGomForegroundBlockNum;
  sComplexityAnalysisParam->iMbNumInGom = SWelsSvcRc->iNumberMbGom;

  {
    int32_t iMethodIdx = METHOD_COMPLEXITY_ANALYSIS;
    SPixMap sSrcPixMap = {0};
    SPixMap sRefPixMap = {0};
    int32_t iRet = 0;

    sSrcPixMap.pPixel[0] = pCurPicture->pData[0];
    sSrcPixMap.iSizeInBits = g_kiPixMapSizeInBits;
    sSrcPixMap.iStride[0] = pCurPicture->iLineSize[0];
    sSrcPixMap.sRect.iRectWidth = pCurPicture->iWidthInPixel;
    sSrcPixMap.sRect.iRectHeight = pCurPicture->iHeightInPixel;
    sSrcPixMap.eFormat = VIDEO_FORMAT_I420;

    sRefPixMap.pPixel[0] = pRefPicture->pData[0];
    sRefPixMap.iSizeInBits = g_kiPixMapSizeInBits;
    sRefPixMap.iStride[0] = pRefPicture->iLineSize[0];
    sRefPixMap.sRect.iRectWidth = pRefPicture->iWidthInPixel;
    sRefPixMap.sRect.iRectHeight = pRefPicture->iHeightInPixel;
    sRefPixMap.eFormat = VIDEO_FORMAT_I420;

    iRet = m_pInterfaceVp->Set (iMethodIdx, (void*)sComplexityAnalysisParam);
    iRet = m_pInterfaceVp->Process (iMethodIdx, &sSrcPixMap, &sRefPixMap);
    if (iRet == 0)
      m_pInterfaceVp->Get (iMethodIdx, (void*)sComplexityAnalysisParam);
  }
}

void  CWelsPreProcess::Padding (uint8_t* pSrcY, uint8_t* pSrcU, uint8_t* pSrcV, int32_t iStrideY, int32_t iStrideUV,
                                int32_t iActualWidth, int32_t iPaddingWidth, int32_t iActualHeight, int32_t iPaddingHeight) {
  int32_t i;

  if (iPaddingHeight > iActualHeight) {
    for (i = iActualHeight; i < iPaddingHeight; i++) {
      memset (pSrcY + i * iStrideY, 0, iActualWidth);

      if (! (i & 1)) {
        memset (pSrcU + i / 2 * iStrideUV, 0x80, iActualWidth / 2);
        memset (pSrcV + i / 2 * iStrideUV, 0x80, iActualWidth / 2);
      }
    }
  }

  if (iPaddingWidth > iActualWidth) {
    for (i = 0; i < iPaddingHeight; i++) {
      memset (pSrcY + i * iStrideY + iActualWidth, 0, iPaddingWidth - iActualWidth);
      if (! (i & 1)) {
        memset (pSrcU + i / 2 * iStrideUV + iActualWidth / 2, 0x80, (iPaddingWidth - iActualWidth) / 2);
        memset (pSrcV + i / 2 * iStrideUV + iActualWidth / 2, 0x80, (iPaddingWidth - iActualWidth) / 2);
      }
    }
  }
}


//TODO: may opti later
//TODO: not use this func?
void* WelsMemcpy (void* dst, const void* kpSrc, uint32_t uiSize) {
  return ::memcpy (dst, kpSrc, uiSize);
}
void* WelsMemset (void* p, int32_t val, uint32_t uiSize) {
  return ::memset (p, val, uiSize);
}

//i420_to_i420_c
void  WelsMoveMemory_c (uint8_t* pDstY, uint8_t* pDstU, uint8_t* pDstV,  int32_t iDstStrideY, int32_t iDstStrideUV,
                        uint8_t* pSrcY, uint8_t* pSrcU, uint8_t* pSrcV, int32_t iSrcStrideY, int32_t iSrcStrideUV, int32_t iWidth,
                        int32_t iHeight) {
  int32_t   iWidth2 = iWidth >> 1;
  int32_t   iHeight2 = iHeight >> 1;
  int32_t   j;

  for (j = iHeight; j; j--) {
    WelsMemcpy (pDstY, pSrcY, iWidth);
    pDstY += iDstStrideY;
    pSrcY += iSrcStrideY;
  }

  for (j = iHeight2; j; j--) {
    WelsMemcpy (pDstU, pSrcU, iWidth2);
    WelsMemcpy (pDstV, pSrcV, iWidth2);
    pDstU += iDstStrideUV;
    pDstV += iDstStrideUV;
    pSrcU += iSrcStrideUV;
    pSrcV += iSrcStrideUV;
  }
}

void  CWelsPreProcess::WelsMoveMemoryWrapper (SWelsSvcCodingParam* pSvcParam, SPicture* pDstPic,
    const SSourcePicture* kpSrc,
    const int32_t kiTargetWidth, const int32_t kiTargetHeight) {
  if (VIDEO_FORMAT_I420 != (kpSrc->iColorFormat & (~VIDEO_FORMAT_VFlip)))
    return;

  int32_t  iSrcWidth       = kpSrc->iPicWidth;
  int32_t  iSrcHeight      = kpSrc->iPicHeight;

  if (iSrcHeight > kiTargetHeight) 	iSrcHeight = kiTargetHeight;
  if (iSrcWidth > kiTargetWidth)		iSrcWidth  = kiTargetWidth;

  // copy from fr26 to fix the odd uiSize failed issue
  if (iSrcWidth & 0x1)		-- iSrcWidth;
  if (iSrcHeight & 0x1)		-- iSrcHeight;

  const int32_t kiSrcTopOffsetY = pSvcParam->SUsedPicRect.iTop;
  const int32_t kiSrcTopOffsetUV = (kiSrcTopOffsetY >> 1);
  const int32_t kiSrcLeftOffsetY = pSvcParam->SUsedPicRect.iLeft;
  const int32_t kiSrcLeftOffsetUV = (kiSrcLeftOffsetY >> 1);
  int32_t  iSrcOffset[3]       = {0, 0, 0};
  iSrcOffset[0] = kpSrc->iStride[0] * kiSrcTopOffsetY + kiSrcLeftOffsetY;
  iSrcOffset[1] = kpSrc->iStride[1] * kiSrcTopOffsetUV + kiSrcLeftOffsetUV ;
  iSrcOffset[2] = kpSrc->iStride[2] * kiSrcTopOffsetUV + kiSrcLeftOffsetUV;

  uint8_t* pSrcY = kpSrc->pData[0] + iSrcOffset[0];
  uint8_t* pSrcU = kpSrc->pData[1] + iSrcOffset[1];
  uint8_t* pSrcV = kpSrc->pData[2] + iSrcOffset[2];
  const int32_t kiSrcStrideY = kpSrc->iStride[0];
  const int32_t kiSrcStrideUV = kpSrc->iStride[1];

  uint8_t* pDstY = pDstPic->pData[0];
  uint8_t* pDstU = pDstPic->pData[1];
  uint8_t* pDstV = pDstPic->pData[2];
  const int32_t kiDstStrideY = pDstPic->iLineSize[0];
  const int32_t kiDstStrideUV = pDstPic->iLineSize[1];

#define MAX_WIDTH      (4096)
#define MAX_HEIGHT     (2304)//MAX_FS_LEVEL51 (36864); MAX_FS_LEVEL51*256/4096 = 2304
  if (pSrcY) {
    if (iSrcWidth <= 0 || iSrcWidth > MAX_WIDTH || iSrcHeight <= 0 || iSrcHeight > MAX_HEIGHT)
      return;
    if (kiSrcTopOffsetY >= iSrcHeight || kiSrcLeftOffsetY >= iSrcWidth || iSrcWidth > kiSrcStrideY)
      return;
  }
  if (pDstY) {
    if (kiTargetWidth <= 0 || kiTargetWidth > MAX_WIDTH || kiTargetHeight <= 0 || kiTargetHeight > MAX_HEIGHT)
      return;
    if (kiTargetWidth > kiDstStrideY)
      return;
  }

  if (pSrcY == NULL || pSrcU == NULL || pSrcV == NULL || pDstY == NULL || pDstU == NULL || pDstV == NULL
      || (iSrcWidth & 1) || (iSrcHeight & 1)) {
  } else {
    //i420_to_i420_c
    WelsMoveMemory_c (pDstY,  pDstU,  pDstV,  kiDstStrideY, kiDstStrideUV,
                      pSrcY,  pSrcU,  pSrcV, kiSrcStrideY, kiSrcStrideUV, iSrcWidth, iSrcHeight);

    //in VP Process
    if (kiTargetWidth > iSrcWidth || kiTargetHeight > iSrcHeight) {
      Padding(pDstY, pDstU, pDstV, kiDstStrideY, kiDstStrideUV, iSrcWidth, kiTargetWidth, iSrcHeight, kiTargetHeight);
    }
  }

}

//*********************************************************************************************************/
} // namespace WelsSVCEnc
