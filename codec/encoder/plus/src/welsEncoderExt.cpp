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

#include <assert.h>
#include "welsEncoderExt.h"
#include "welsCodecTrace.h"
#include "typedefs.h"
#include "wels_const.h"
#include "utils.h"
#include "macros.h"
#include "version.h"
#include "crt_util_safe_x.h"	// Safe CRT routines like util for cross platforms
#include "ref_list_mgr_svc.h"
#include "codec_ver.h"

#include <time.h>
#include <measure_time.h>
#if defined(_WIN32) /*&& defined(_DEBUG)*/

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#else
#include <sys/time.h>
#endif

namespace WelsEnc {

/*
 *	CWelsH264SVCEncoder class implementation
 */
CWelsH264SVCEncoder::CWelsH264SVCEncoder()
  :	m_pEncContext (NULL),
    m_pWelsTrace (NULL),
    m_iMaxPicWidth (0),
    m_iMaxPicHeight (0),
    m_iCspInternal (0),
    m_bInitialFlag (false) {
#ifdef REC_FRAME_COUNT
  int32_t m_uiCountFrameNum = 0;
#endif//REC_FRAME_COUNT

#ifdef OUTPUT_BIT_STREAM
  char strStreamFileName[1024] = { 0 };  //for .264
  int32_t iBufferUsed = 0;
  int32_t iBufferLeft = 1023;
  int32_t iCurUsed;

  char strLenFileName[1024] = { 0 }; //for .len
  int32_t iBufferUsedSize = 0;
  int32_t iBufferLeftSize = 1023;
  int32_t iCurUsedSize;
#endif//OUTPUT_BIT_STREAM

#ifdef OUTPUT_BIT_STREAM
  SWelsTime tTime;

  WelsGetTimeOfDay (&tTime);

  iCurUsed      = WelsSnprintf (strStreamFileName, iBufferLeft, "enc_bs_0x%p_", (void*)this);
  iCurUsedSize  = WelsSnprintf (strLenFileName, iBufferLeftSize, "enc_size_0x%p_", (void*)this);


  iBufferUsed += iCurUsed;
  iBufferLeft -= iCurUsed;
  if (iBufferLeft > 0) {
    iCurUsed = WelsStrftime (&strStreamFileName[iBufferUsed], iBufferLeft, "%y%m%d%H%M%S", &tTime);
    iBufferUsed += iCurUsed;
    iBufferLeft -= iCurUsed;
  }

  iBufferUsedSize += iCurUsedSize;
  iBufferLeftSize -= iCurUsedSize;
  if (iBufferLeftSize > 0) {
    iCurUsedSize = WelsStrftime (&strLenFileName[iBufferUsedSize], iBufferLeftSize, "%y%m%d%H%M%S", &tTime);
    iBufferUsedSize += iCurUsedSize;
    iBufferLeftSize -= iCurUsedSize;
  }

  if (iBufferLeft > 0) {
    iCurUsed = WelsSnprintf (&strStreamFileName[iBufferUsed], iBufferLeft, ".%03.3u.264",
                             WelsGetMillisecond (&tTime));
    iBufferUsed += iCurUsed;
    iBufferLeft -= iCurUsed;
  }

  if (iBufferLeftSize > 0) {
    iCurUsedSize = WelsSnprintf (&strLenFileName[iBufferUsedSize], iBufferLeftSize, ".%03.3u.len",
                                 WelsGetMillisecond (&tTime));
    iBufferUsedSize += iCurUsedSize;
    iBufferLeftSize -= iCurUsedSize;
  }

  m_pFileBs     = WelsFopen (strStreamFileName, "wb");
  m_pFileBsSize = WelsFopen (strLenFileName, "wb");

  m_bSwitch	= false;
  m_iSwitchTimes	= 0;
#endif//OUTPUT_BIT_STREAM

  InitEncoder();
}

CWelsH264SVCEncoder::~CWelsH264SVCEncoder() {
  if (m_pWelsTrace) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsH264SVCEncoder::~CWelsH264SVCEncoder()");
  }

#ifdef REC_FRAME_COUNT
  m_uiCountFrameNum = 0;
#endif//REC_FRAME_COUNT

#ifdef OUTPUT_BIT_STREAM
  if (m_pFileBs) {
    WelsFclose (m_pFileBs);
    m_pFileBs = NULL;
  }
  if (m_pFileBsSize) {
    WelsFclose (m_pFileBsSize);
    m_pFileBsSize = NULL;
  }
  m_bSwitch	= false;
  m_iSwitchTimes	= 0;
#endif//OUTPUT_BIT_STREAM

  Uninitialize();

  if (m_pWelsTrace) {
    delete m_pWelsTrace;
    m_pWelsTrace = NULL;
  }
}

void CWelsH264SVCEncoder::InitEncoder (void) {

  m_pWelsTrace	= new welsCodecTrace();
  if (m_pWelsTrace == NULL) {
    return;
  }
  m_pWelsTrace->SetCodecInstance (this);
}

/* Interfaces override from ISVCEncoder */

int CWelsH264SVCEncoder::GetDefaultParams (SEncParamExt* argv) {
  SWelsSvcCodingParam::FillDefault (*argv);
  return cmResultSuccess;
}

/*
 *	SVC Encoder Initialization
 */
int CWelsH264SVCEncoder::Initialize (const SEncParamBase* argv) {
  if (m_pWelsTrace == NULL) {
    return cmMallocMemeError;
  }

  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsH264SVCEncoder::InitEncoder(), openh264 codec version = %s",
           VERSION_NUMBER);

  if (NULL == argv) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), invalid argv= 0x%p",
             argv);
    return cmInitParaError;
  }

  SWelsSvcCodingParam	sConfig;
  // Convert SEncParamBase into WelsSVCParamConfig here..
  if (sConfig.ParamBaseTranscode (*argv)) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), parameter_translation failed.");
    TraceParamInfo (&sConfig);
    Uninitialize();
    return cmInitParaError;
  }

  return InitializeInternal (&sConfig);
}

int CWelsH264SVCEncoder::InitializeExt (const SEncParamExt* argv) {
  if (m_pWelsTrace == NULL) {
    return cmMallocMemeError;
  }

  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsH264SVCEncoder::InitEncoder(), openh264 codec version = %s",
           VERSION_NUMBER);

  if (NULL == argv) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::InitializeExt(), invalid argv= 0x%p",
             argv);
    return cmInitParaError;
  }

  SWelsSvcCodingParam	sConfig;
  // Convert SEncParamExt into WelsSVCParamConfig here..
  if (sConfig.ParamTranscode (*argv)) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::InitializeExt(), parameter_translation failed.");
    TraceParamInfo (&sConfig);
    Uninitialize();
    return cmInitParaError;
  }

  return InitializeInternal (&sConfig);
}

int CWelsH264SVCEncoder::InitializeInternal (SWelsSvcCodingParam* pCfg) {
  if (NULL == pCfg) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), invalid argv= 0x%p.",
             pCfg);
    return cmInitParaError;
  }

  if (m_bInitialFlag) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_WARNING,
             "CWelsH264SVCEncoder::Initialize(), reinitialize, m_bInitialFlag= %d.",
             m_bInitialFlag);
    Uninitialize();
  }
  // Check valid parameters
  const int32_t iNumOfLayers = pCfg->iSpatialLayerNum;
  if (iNumOfLayers < 1 || iNumOfLayers > MAX_DEPENDENCY_LAYER) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid iSpatialLayerNum= %d, valid at range of [1, %d].", iNumOfLayers,
             MAX_DEPENDENCY_LAYER);
    Uninitialize();
    return cmInitParaError;
  }
  if (pCfg->iTemporalLayerNum < 1)
    pCfg->iTemporalLayerNum	= 1;
  if (pCfg->iTemporalLayerNum > MAX_TEMPORAL_LEVEL) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid iTemporalLayerNum= %d, valid at range of [1, %d].",
             pCfg->iTemporalLayerNum, MAX_TEMPORAL_LEVEL);
    Uninitialize();
    return cmInitParaError;
  }

  //	assert( cfg.uiGopSize >= 1 && ( cfg.uiIntraPeriod && (cfg.uiIntraPeriod % cfg.uiGopSize) == 0) );

  if (pCfg->uiGopSize < 1 || pCfg->uiGopSize > MAX_GOP_SIZE) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiGopSize= %d, valid at range of [1, %d].", pCfg->uiGopSize,
             MAX_GOP_SIZE);
    Uninitialize();
    return cmInitParaError;
  }

  if (!WELS_POWER2_IF (pCfg->uiGopSize)) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiGopSize= %d, valid at range of [1, %d] and yield to power of 2.",
             pCfg->uiGopSize, MAX_GOP_SIZE);
    Uninitialize();
    return cmInitParaError;
  }

  if (pCfg->uiIntraPeriod && pCfg->uiIntraPeriod < pCfg->uiGopSize) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiIntraPeriod= %d, valid in case it equals to 0 for unlimited intra period or exceeds specified uiGopSize= %d.",
             pCfg->uiIntraPeriod, pCfg->uiGopSize);
    Uninitialize();
    return cmInitParaError;
  }

  if ((pCfg->uiIntraPeriod && (pCfg->uiIntraPeriod & (pCfg->uiGopSize - 1)) != 0)) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiIntraPeriod= %d, valid in case it equals to 0 for unlimited intra period or exceeds specified uiGopSize= %d also multiple of it.",
             pCfg->uiIntraPeriod, pCfg->uiGopSize);
    Uninitialize();
    return cmInitParaError;
  }
  if (pCfg->iUsageType == SCREEN_CONTENT_REAL_TIME) {
    if (pCfg->bEnableLongTermReference) {
      pCfg->iLTRRefNum = LONG_TERM_REF_NUM_SCREEN;
      if (pCfg->iNumRefFrame == AUTO_REF_PIC_COUNT)
        pCfg->iNumRefFrame = WELS_MAX (1, WELS_LOG2 (pCfg->uiGopSize)) + pCfg->iLTRRefNum;
    } else {
      pCfg->iLTRRefNum = 0;
      if (pCfg->iNumRefFrame == AUTO_REF_PIC_COUNT)
        pCfg->iNumRefFrame = WELS_MAX (1, pCfg->uiGopSize >> 1);
    }
  } else {
    pCfg->iLTRRefNum = pCfg->bEnableLongTermReference ? LONG_TERM_REF_NUM : 0;
    if (pCfg->iNumRefFrame == AUTO_REF_PIC_COUNT) {
      pCfg->iNumRefFrame		= ((pCfg->uiGopSize >> 1) > 1) ? ((pCfg->uiGopSize >> 1) + pCfg->iLTRRefNum) :
                              (MIN_REF_PIC_COUNT + pCfg->iLTRRefNum);
      pCfg->iNumRefFrame		= WELS_CLIP3 (pCfg->iNumRefFrame, MIN_REF_PIC_COUNT, MAX_REFERENCE_PICTURE_COUNT_NUM_CAMERA);
    }
  }

  if (pCfg->iLtrMarkPeriod == 0) {
    pCfg->iLtrMarkPeriod = 30;
  }

  const int32_t kiDecStages = WELS_LOG2 (pCfg->uiGopSize);
  pCfg->iTemporalLayerNum	= (int8_t) (1 + kiDecStages);
  pCfg->iLoopFilterAlphaC0Offset	= WELS_CLIP3 (pCfg->iLoopFilterAlphaC0Offset, -6, 6);
  pCfg->iLoopFilterBetaOffset		= WELS_CLIP3 (pCfg->iLoopFilterBetaOffset, -6, 6);

  // decide property list size between INIT_TYPE_PARAMETER_BASED/INIT_TYPE_CONFIG_BASED
  m_iMaxPicWidth	= pCfg->iPicWidth;
  m_iMaxPicHeight	= pCfg->iPicHeight;

  TraceParamInfo (pCfg);
  if (WelsInitEncoderExt (&m_pEncContext, pCfg, &m_pWelsTrace->m_sLogCtx, NULL)) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), WelsInitEncoderExt failed.");
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_DEBUG,
             "Problematic Input Base Param: iUsageType=%d, Resolution=%dx%d, FR=%f, TLayerNum=%d, DLayerNum=%d",
             pCfg->iUsageType, pCfg->iPicWidth, pCfg->iPicHeight, pCfg->fMaxFrameRate, pCfg->iTemporalLayerNum,
             pCfg->iSpatialLayerNum);
    Uninitialize();
    return cmInitParaError;
  }

  m_bInitialFlag  = true;

  return cmResultSuccess;
}

/*
 *	SVC Encoder Uninitialization
 */
int32_t CWelsH264SVCEncoder::Uninitialize() {
  if (!m_bInitialFlag) {
    return 0;
  }

  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsH264SVCEncoder::Uninitialize(), openh264 codec version = %s.",
           VERSION_NUMBER);

  if (NULL != m_pEncContext) {
    WelsUninitEncoderExt (&m_pEncContext);
    m_pEncContext	= NULL;
  }

  m_bInitialFlag = false;

  return 0;
}


/*
 *	SVC core encoding
 */
int CWelsH264SVCEncoder::EncodeFrame (const SSourcePicture* kpSrcPic, SFrameBSInfo* pBsInfo) {
  if (! (kpSrcPic && m_bInitialFlag && pBsInfo)) {
    return cmInitParaError;
  }
  if (kpSrcPic->iColorFormat != videoFormatI420)
    return cmInitParaError;

  const int32_t kiEncoderReturn = EncodeFrameInternal (kpSrcPic, pBsInfo);

  if (kiEncoderReturn != cmResultSuccess)
    return kiEncoderReturn;

#ifdef REC_FRAME_COUNT
  ++ m_uiCountFrameNum;
  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
           "CWelsH264SVCEncoder::EncodeFrame(), m_uiCountFrameNum= %d,", m_uiCountFrameNum);
#endif//REC_FRAME_COUNT

#ifdef DUMP_SRC_PICTURE
  DumpSrcPicture (pSrc);
#endif // DUMP_SRC_PICTURE
  return kiEncoderReturn;
}


int CWelsH264SVCEncoder ::EncodeFrameInternal (const SSourcePicture*  pSrcPic, SFrameBSInfo* pBsInfo) {
  const int64_t kiBeforeFrameUs = WelsTime();
  const int32_t kiEncoderReturn = WelsEncoderEncodeExt (m_pEncContext, pBsInfo, pSrcPic);
  const int64_t kiCurrentFrameMs = (WelsTime() - kiBeforeFrameUs) / 1000;

  if (kiEncoderReturn == ENC_RETURN_MEMALLOCERR) {
    WelsUninitEncoderExt (&m_pEncContext);
    return cmMallocMemeError;
  } else if ((kiEncoderReturn != ENC_RETURN_SUCCESS) && (kiEncoderReturn == ENC_RETURN_CORRECTED)) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "unexpected return(%d) from EncodeFrameInternal()!",
             kiEncoderReturn);
    return cmUnkonwReason;
  }

  UpdateStatistics (pSrcPic->uiTimeStamp, pBsInfo->eFrameType, pBsInfo->iFrameSizeInBytes, kiCurrentFrameMs);

  ///////////////////for test
#ifdef OUTPUT_BIT_STREAM
  if (pBsInfo->eFrameType != videoFrameTypeInvalid && pBsInfo->eFrameType != videoFrameTypeSkip) {
    SLayerBSInfo* pLayer = NULL;
    int32_t i = 0, j = 0, iCurLayerBits = 0, total_bits = 0;

    if (m_bSwitch) {
      if (m_pFileBs) {
        WelsFclose (m_pFileBs);
        m_pFileBs = NULL;
      }
      if (m_pFileBsSize) {
        WelsFclose (m_pFileBsSize);
        m_pFileBsSize = NULL;
      }
      char strStreamFileName[128] = {0};
      WelsSnprintf (strStreamFileName, 128, "adj%d_w%d.264", m_iSwitchTimes,
                    m_pEncContext->pSvcParam->iPicWidth);
      m_pFileBs = WelsFopen (strStreamFileName, "wb");
      WelsSnprintf (strStreamFileName, 128, "adj%d_w%d_size.iLen", m_iSwitchTimes,
                    m_pEncContext->pSvcParam->iPicWidth);
      m_pFileBsSize = WelsFopen (strStreamFileName, "wb");


      m_bSwitch = false;
    }

    for (i = 0; i < pBsInfo->iLayerNum; i++) {
      pLayer = &pBsInfo->sLayerInfo[i];

      iCurLayerBits = 0;
      for (j = 0; j < pLayer->iNalCount; j++) {
        iCurLayerBits += pLayer->pNalLengthInByte[j];
      }
      total_bits += iCurLayerBits;
      if (m_pFileBs != NULL)
        WelsFwrite (pLayer->pBsBuf, 1, iCurLayerBits, m_pFileBs);
    }

    if (m_pFileBsSize != NULL)
      WelsFwrite (&total_bits, sizeof (int32_t), 1, m_pFileBsSize);
  }
#endif //OUTPUT_BIT_STREAM
#ifdef DUMP_SRC_PICTURE
  DumpSrcPicture (pSrcPicList[0]->pData[0]);
#endif // DUMP_SRC_PICTURE

  return cmResultSuccess;

}

int CWelsH264SVCEncoder::EncodeParameterSets (SFrameBSInfo* pBsInfo) {
  return WelsEncoderEncodeParameterSets (m_pEncContext, pBsInfo);
}

/*
 *	Force key frame
 */
int CWelsH264SVCEncoder::ForceIntraFrame (bool bIDR) {
  if (! (m_pEncContext && m_bInitialFlag)) {
    return 1;
  }
  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
           "CWelsH264SVCEncoder::ForceIntraFrame(), bIDR= %d", bIDR);

  ForceCodingIDR (m_pEncContext);

  m_pEncContext->sEncoderStatistics.uiIDRReqNum++;

  return 0;
}
void CWelsH264SVCEncoder::TraceParamInfo (SEncParamExt* pParam) {
  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
           "iUsageType = %d,iPicWidth= %d;iPicHeight= %d;iTargetBitrate= %d;iMaxBitrate= %d;iRCMode= %d;iPaddingFlag= %d;iTemporalLayerNum= %d;iSpatialLayerNum= %d;fFrameRate= %.6ff;uiIntraPeriod= %d;\
             eSpsPpsIdStrategy = %d;bPrefixNalAddingCtrl = %d;bEnableDenoise= %d;bEnableBackgroundDetection= %d;bEnableAdaptiveQuant= %d;bEnableFrameSkip= %d;bEnableLongTermReference= %d;iLtrMarkPeriod= %d;\
             iComplexityMode = %d;iNumRefFrame = %d;iEntropyCodingModeFlag = %d;uiMaxNalSize = %d;iLTRRefNum = %d;iMultipleThreadIdc = %d;iLoopFilterDisableIdc = %d (offset(alpha/beta): %d,%d)",
           pParam->iUsageType,
           pParam->iPicWidth,
           pParam->iPicHeight,
           pParam->iTargetBitrate,
           pParam->iMaxBitrate,
           pParam->iRCMode,
           pParam->iPaddingFlag,
           pParam->iTemporalLayerNum,
           pParam->iSpatialLayerNum,
           pParam->fMaxFrameRate,
           pParam->uiIntraPeriod,
           pParam->eSpsPpsIdStrategy,
           pParam->bPrefixNalAddingCtrl,
           pParam->bEnableDenoise,
           pParam->bEnableBackgroundDetection,
           pParam->bEnableAdaptiveQuant,
           pParam->bEnableFrameSkip,
           pParam->bEnableLongTermReference,
           pParam->iLtrMarkPeriod,
           pParam->iComplexityMode,
           pParam->iNumRefFrame,
           pParam->iEntropyCodingModeFlag,
           pParam->uiMaxNalSize,
           pParam->iLTRRefNum,
           pParam->iMultipleThreadIdc,
           pParam->iLoopFilterDisableIdc,
           pParam->iLoopFilterAlphaC0Offset,
           pParam->iLoopFilterBetaOffset
          );
  int32_t i = 0;
  int32_t iSpatialLayers = (pParam->iSpatialLayerNum < MAX_SPATIAL_LAYER_NUM) ? (pParam->iSpatialLayerNum) :
                           MAX_SPATIAL_LAYER_NUM;
  while (i < iSpatialLayers) {
    SSpatialLayerConfig* pSpatialCfg = &pParam->sSpatialLayers[i];
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "sSpatialLayers[%d]: .iVideoWidth= %d; .iVideoHeight= %d; .fFrameRate= %.6ff; .iSpatialBitrate= %d; .iMaxSpatialBitrate= %d; .sSliceCfg.uiSliceMode= %d; .sSliceCfg.sSliceArgument.iSliceNum= %d; .sSliceCfg.sSliceArgument.uiSliceSizeConstraint= %d;\
               uiProfileIdc = %d;uiLevelIdc = %d",
             i, pSpatialCfg->iVideoWidth,
             pSpatialCfg->iVideoHeight,
             pSpatialCfg->fFrameRate,
             pSpatialCfg->iSpatialBitrate,
             pSpatialCfg->iMaxSpatialBitrate,
             pSpatialCfg->sSliceCfg.uiSliceMode,
             pSpatialCfg->sSliceCfg.sSliceArgument.uiSliceNum,
             pSpatialCfg->sSliceCfg.sSliceArgument.uiSliceSizeConstraint,
             pSpatialCfg->uiProfileIdc,
             pSpatialCfg->uiLevelIdc
            );
    ++ i;
  }
}

void CWelsH264SVCEncoder::UpdateStatistics (const int64_t kiCurrentFrameTs, EVideoFrameType eFrameType,
    const int32_t kiCurrentFrameSize, const int64_t kiCurrentFrameMs) {
  SEncoderStatistics* pStatistics = & (m_pEncContext->sEncoderStatistics);

  int32_t iMaxDid = m_pEncContext->pSvcParam->iSpatialLayerNum - 1;
  if ((0 != pStatistics->uiWidth && 0 != pStatistics->uiHeight)
      && (pStatistics->uiWidth != (unsigned int) m_pEncContext->pSvcParam->sDependencyLayers[iMaxDid].iActualWidth
          || pStatistics->uiHeight != (unsigned int) m_pEncContext->pSvcParam->sDependencyLayers[iMaxDid].iActualHeight)) {
    pStatistics->uiResolutionChangeTimes ++;
  }
  pStatistics->uiWidth = m_pEncContext->pSvcParam->sDependencyLayers[iMaxDid].iActualWidth;
  pStatistics->uiHeight = m_pEncContext->pSvcParam->sDependencyLayers[iMaxDid].iActualHeight;

  int32_t iProcessedFrameCount = pStatistics->uiInputFrameCount - pStatistics->uiSkippedFrameCount;
  const bool kbCurrentFrameSkipped = (videoFrameTypeSkip == eFrameType);
  if (!kbCurrentFrameSkipped && (iProcessedFrameCount + 1) != 0) {
    pStatistics->fAverageFrameSpeedInMs = (iProcessedFrameCount * pStatistics->fAverageFrameSpeedInMs +
                                           kiCurrentFrameMs) / (iProcessedFrameCount + 1);
  }
  pStatistics->uiInputFrameCount ++;
  pStatistics->uiSkippedFrameCount += (kbCurrentFrameSkipped ? 1 : 0);

  // rate control related
  if (0 != m_pEncContext->uiStartTimestamp) {
    if (kiCurrentFrameTs > m_pEncContext->uiStartTimestamp + 800) {
      pStatistics->fAverageFrameRate = (static_cast<float> (pStatistics->uiInputFrameCount) * 1000 /
                                        (kiCurrentFrameTs - m_pEncContext->uiStartTimestamp));
    }
  } else {
    m_pEncContext->uiStartTimestamp = kiCurrentFrameTs;
  }
  pStatistics->fLatestFrameRate = m_pEncContext->pWelsSvcRc->fLatestFrameRate; //TODO: finish the calculation in RC
  pStatistics->uiBitRate = m_pEncContext->pWelsSvcRc->iActualBitRate; //TODO: finish the calculation in RC
  pStatistics->uiAverageFrameQP = m_pEncContext->pWelsSvcRc->iAverageFrameQp;

  if (videoFrameTypeIDR == eFrameType || videoFrameTypeI == eFrameType) {
    pStatistics->uiIDRSentNum ++;
  }
  if (m_pEncContext->pLtr->bLTRMarkingFlag) {
    pStatistics->uiLTRSentNum ++;
  }

  m_pEncContext->iTotalEncodedBits += kiCurrentFrameSize;

  if (m_pEncContext->iStatisticsLogInterval > 0) {
    int64_t iTimeDiff = kiCurrentFrameTs - m_pEncContext->iLastStatisticsLogTs;
    if ((iTimeDiff > m_pEncContext->iStatisticsLogInterval) || (0 == pStatistics->uiInputFrameCount % 300)) {
      if (iTimeDiff) {
        pStatistics->fLatestFrameRate = static_cast<float> ((pStatistics->uiInputFrameCount -
                                        m_pEncContext->iLastStatisticsFrameCount) * 1000 /
                                        iTimeDiff);
        pStatistics->uiBitRate = static_cast<unsigned int> ((m_pEncContext->iTotalEncodedBits -
                                 m_pEncContext->iLastStatisticsBits) * 1000 / iTimeDiff);
      }

      // update variables
      m_pEncContext->iLastStatisticsLogTs = kiCurrentFrameTs;
      m_pEncContext->iLastStatisticsBits = m_pEncContext->iTotalEncodedBits;
      m_pEncContext->iLastStatisticsFrameCount = pStatistics->uiInputFrameCount;

      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "EncoderStatistics: %dx%d, SpeedInMs: %f, fAverageFrameRate=%f, \
               LastFrameRate=%f, LatestBitRate=%d, LastFrameQP=%d, uiInputFrameCount=%d, uiSkippedFrameCount=%d, \
               uiResolutionChangeTimes=%d, uIDRReqNum=%d, uIDRSentNum=%d, uLTRSentNum=NA",
               pStatistics->uiWidth, pStatistics->uiHeight,
               pStatistics->fAverageFrameSpeedInMs, pStatistics->fAverageFrameRate,
               pStatistics->fLatestFrameRate, pStatistics->uiBitRate, pStatistics->uiAverageFrameQP,
               pStatistics->uiInputFrameCount, pStatistics->uiSkippedFrameCount,
               pStatistics->uiResolutionChangeTimes, pStatistics->uiIDRReqNum, pStatistics->uiIDRSentNum);
      //TODO: the following statistics will be calculated and added later
      //pStatistics->uiLTRSentNum
    }
  }

}

/************************************************************************
* InDataFormat, IDRInterval, SVC Encode Param, Frame Rate, Bitrate,..
************************************************************************/
int CWelsH264SVCEncoder::SetOption (ENCODER_OPTION eOptionId, void* pOption) {
  if (NULL == pOption) {
    return cmInitParaError;
  }

  if ((NULL == m_pEncContext || false == m_bInitialFlag) && eOptionId != ENCODER_OPTION_TRACE_LEVEL
      && eOptionId != ENCODER_OPTION_TRACE_CALLBACK && eOptionId != ENCODER_OPTION_TRACE_CALLBACK_CONTEXT) {
    return cmInitExpected;
  }

  switch (eOptionId) {
  case ENCODER_OPTION_INTER_SPATIAL_PRED: {	// Inter spatial layer prediction flag
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "ENCODER_OPTION_INTER_SPATIAL_PRED, this feature not supported at present.");
  }
  break;
  case ENCODER_OPTION_DATAFORMAT: {	// Input color space
    int32_t iValue = * ((int32_t*)pOption);
    int32_t iColorspace = iValue;
    if (iColorspace == 0) {
      return cmInitParaError;
    }

    m_iCspInternal = iColorspace;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_DATAFORMAT, m_iCspInternal= 0x%x", m_iCspInternal);
  }
  break;
  case ENCODER_OPTION_IDR_INTERVAL: {	// IDR Interval
    int32_t iValue	= * ((int32_t*)pOption);
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_IDR_INTERVAL iValue= %d", iValue);
    if (iValue < -1 || iValue == 0)
      iValue = 1;
    if (iValue == (int32_t)m_pEncContext->pSvcParam->uiIntraPeriod) {
      return cmResultSuccess;
    }
    m_pEncContext->pSvcParam->uiIntraPeriod	= (uint32_t)iValue;
  }
  break;
  case ENCODER_OPTION_SVC_ENCODE_PARAM_BASE: {	// SVC Encoding Parameter
    SEncParamBase		sEncodingParam;
    SWelsSvcCodingParam	sConfig;
    int32_t iTargetWidth = 0;
    int32_t iTargetHeight = 0;

    memcpy (&sEncodingParam, pOption, sizeof (SEncParamBase));	// confirmed_safe_unsafe_usage
    if (sConfig.ParamBaseTranscode (sEncodingParam)) {
      return cmInitParaError;
    }
    /* New configuration available here */
    iTargetWidth	= sConfig.iPicWidth;
    iTargetHeight	= sConfig.iPicHeight;
    if (m_iMaxPicWidth != iTargetWidth
        || m_iMaxPicHeight != iTargetHeight) {
      m_iMaxPicWidth	= iTargetWidth;
      m_iMaxPicHeight	= iTargetHeight;
    }
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_BASE iUsageType = %d,iPicWidth= %d;iPicHeight= %d;iTargetBitrate= %d;fMaxFrameRate=  %.6ff;iRCMode= %d",
             sEncodingParam.iUsageType,
             sEncodingParam.iPicWidth,
             sEncodingParam.iPicHeight,
             sEncodingParam.iTargetBitrate,
             sEncodingParam.fMaxFrameRate,
             sEncodingParam.iRCMode);
    if (WelsEncoderParamAdjust (&m_pEncContext, &sConfig)) {
      return cmInitParaError;
    }
  }
  break;

  case ENCODER_OPTION_SVC_ENCODE_PARAM_EXT: {	// SVC Encoding Parameter
    SEncParamExt		sEncodingParam;
    SWelsSvcCodingParam	sConfig;
    int32_t iTargetWidth = 0;
    int32_t iTargetHeight = 0;

    memcpy (&sEncodingParam, pOption, sizeof (SEncParamExt));	// confirmed_safe_unsafe_usage
    TraceParamInfo (&sEncodingParam);
#ifdef OUTPUT_BIT_STREAM
    if (sEncodingParam.sSpatialLayers[sEncodingParam.iSpatialLayerNum - 1].iVideoWidth !=
        m_pEncContext->pSvcParam->sDependencyLayers[m_pEncContext->pSvcParam->iSpatialLayerNum - 1].iActualWidth) {
      ++ m_iSwitchTimes;
      m_bSwitch = true;
    }
#endif//OUTPUT_BIT_STREAM
    if (sEncodingParam.iSpatialLayerNum < 1
        || sEncodingParam.iSpatialLayerNum > MAX_SPATIAL_LAYER_NUM) {	// verify number of spatial layer
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, iSpatialLayerNum(%d) failed!",
               sEncodingParam.iSpatialLayerNum);
      return cmInitParaError;
    }

    if (sConfig.ParamTranscode (sEncodingParam)) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, ParamTranscode failed!");
      return cmInitParaError;
    }
    if (sConfig.iSpatialLayerNum < 1) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, iSpatialLayerNum(%d) failed!",
               sConfig.iSpatialLayerNum);
      return cmInitParaError;
    }
    if (sConfig.DetermineTemporalSettings()) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, DetermineTemporalSettings failed!");
      return cmInitParaError;
    }

    /* New configuration available here */
    iTargetWidth	= sConfig.iPicWidth;
    iTargetHeight	= sConfig.iPicHeight;
    if (m_iMaxPicWidth != iTargetWidth
        || m_iMaxPicHeight != iTargetHeight) {
      m_iMaxPicWidth	= iTargetWidth;
      m_iMaxPicHeight	= iTargetHeight;
    }
    /* Check every field whether there is new request for memory block changed or else, Oct. 24, 2008 */
    if (WelsEncoderParamAdjust (&m_pEncContext, &sConfig)) {
      return cmInitParaError;
    }
  }
  break;
  case ENCODER_OPTION_FRAME_RATE: {	// Maximal input frame rate
    float iValue	= * ((float*)pOption);
    if (iValue <= 0) {
      return cmInitParaError;
    }
    //adjust to valid range
    m_pEncContext->pSvcParam->fMaxFrameRate = WELS_CLIP3 (iValue, MIN_FRAME_RATE, MAX_FRAME_RATE);
    WelsEncoderApplyFrameRate (m_pEncContext->pSvcParam);
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_FRAME_RATE,m_pEncContext->pSvcParam->fMaxFrameRate= %f",
             m_pEncContext->pSvcParam->fMaxFrameRate);
  }
  break;
  case ENCODER_OPTION_BITRATE: {	// Target bit-rate
    SBitrateInfo* pInfo = (static_cast<SBitrateInfo*> (pOption));
    int32_t iBitrate = pInfo->iBitrate;
    if (iBitrate <= 0) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE,iBitrate = %d",
               iBitrate);
      return cmInitParaError;
    }
    iBitrate	= WELS_CLIP3 (iBitrate, MIN_BIT_RATE, MAX_BIT_RATE);
    switch (pInfo->iLayer) {
    case SPATIAL_LAYER_ALL:
      m_pEncContext->pSvcParam->iTargetBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_0:
      m_pEncContext->pSvcParam->sSpatialLayers[0].iSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_1:
      m_pEncContext->pSvcParam->sSpatialLayers[1].iSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_2:
      m_pEncContext->pSvcParam->sSpatialLayers[2].iSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_3:
      m_pEncContext->pSvcParam->sSpatialLayers[3].iSpatialBitrate = iBitrate;
      break;
    default:
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE,iLayer = %d",
               pInfo->iLayer);
      return cmInitParaError;
      break;
    }
    //adjust to valid range
    if (WelsEncoderApplyBitRate (&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam, pInfo->iLayer)) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE layerId= %d,iSpatialBitrate = %d", pInfo->iLayer, iBitrate);
      return cmInitParaError;
    } else {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE layerId= %d,iSpatialBitrate = %d", pInfo->iLayer, iBitrate);

    }

  }
  break;
  case ENCODER_OPTION_MAX_BITRATE: {	// Target bit-rate
    SBitrateInfo* pInfo = (static_cast<SBitrateInfo*> (pOption));
    int32_t iBitrate = pInfo->iBitrate;
    if (iBitrate <= 0) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_MAX_BITRATE,iBitrate = %d",
               iBitrate);
      return cmInitParaError;
    }
    iBitrate	= WELS_CLIP3 (iBitrate, MIN_BIT_RATE, MAX_BIT_RATE);
    switch (pInfo->iLayer) {
    case SPATIAL_LAYER_ALL:
      m_pEncContext->pSvcParam->iMaxBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_0:
      m_pEncContext->pSvcParam->sSpatialLayers[0].iMaxSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_1:
      m_pEncContext->pSvcParam->sSpatialLayers[1].iMaxSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_2:
      m_pEncContext->pSvcParam->sSpatialLayers[2].iMaxSpatialBitrate = iBitrate;
      break;
    case SPATIAL_LAYER_3:
      m_pEncContext->pSvcParam->sSpatialLayers[3].iMaxSpatialBitrate = iBitrate;
      break;
    default:
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_MAX_BITRATE,iLayer = %d",
               pInfo->iLayer);
      return cmInitParaError;
      break;
    }
    //adjust to valid range
    if (WelsEncoderApplyBitRate (&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam, pInfo->iLayer)) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE layerId= %d,iMaxSpatialBitrate = %d", pInfo->iLayer, iBitrate);
      return cmInitParaError;
    } else {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE layerId= %d,iMaxSpatialBitrate = %d", pInfo->iLayer, iBitrate);

    }
  }
  break;
  case ENCODER_OPTION_RC_MODE: {	// 0:quality mode;1:bit-rate mode;2:bitrate limited mode
    int32_t iValue = * ((int32_t*)pOption);
    m_pEncContext->pSvcParam->iRCMode	= (RC_MODES) iValue;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_RC_MODE iRCMode= %d ",
             iValue);
  }
  break;
  case ENCODER_PADDING_PADDING: {	// 0:disable padding;1:padding
    int32_t iValue = * ((int32_t*)pOption);
    m_pEncContext->pSvcParam->iPaddingFlag	= iValue;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_PADDING_PADDING iPaddingFlag= %d ",
             iValue);
  }
  break;
  case ENCODER_LTR_RECOVERY_REQUEST: {
    SLTRRecoverRequest* pLTR_Recover_Request = (SLTRRecoverRequest*) (pOption);
    FilterLTRRecoveryRequest (m_pEncContext, pLTR_Recover_Request);
  }
  break;
  case ENCODER_LTR_MARKING_FEEDBACK: {
    SLTRMarkingFeedback* fb = (SLTRMarkingFeedback*) (pOption);
    FilterLTRMarkingFeedback (m_pEncContext, fb);
  }
  break;
  case ENCODER_LTR_MARKING_PERIOD: {
    uint32_t iValue = * ((uint32_t*) (pOption));
    m_pEncContext->pSvcParam->iLtrMarkPeriod = iValue;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_LTR_MARKING_PERIOD iLtrMarkPeriod= %d ",
             iValue);
  }
  break;
  case ENCODER_OPTION_LTR: {
    SLTRConfig* pLTRValue = ((SLTRConfig*) (pOption));
    if (WelsEncoderApplyLTR (&m_pWelsTrace->m_sLogCtx, &m_pEncContext, pLTRValue)) {
      return cmInitParaError;
    }
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_LTR,expected bEnableLongTermReference = %d,expeced iLTRRefNum = %d,actual bEnableLongTermReference = %d,actual iLTRRefNum = %d",
             pLTRValue->bEnableLongTermReference, pLTRValue->iLTRRefNum, m_pEncContext->pSvcParam->bEnableLongTermReference,
             m_pEncContext->pSvcParam->iLTRRefNum);
  }
  break;
  case ENCODER_OPTION_ENABLE_SSEI: {
    bool iValue = * ((bool*)pOption);
    m_pEncContext->pSvcParam->bEnableSSEI = iValue;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             " CWelsH264SVCEncoder::SetOption enable SSEI = %d -- this is not supported yet",
             m_pEncContext->pSvcParam->bEnableSSEI);
  }
  break;
  case ENCODER_OPTION_ENABLE_PREFIX_NAL_ADDING: {
    bool iValue = * ((bool*)pOption);
    m_pEncContext->pSvcParam->bPrefixNalAddingCtrl = iValue;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, " CWelsH264SVCEncoder::SetOption bPrefixNalAddingCtrl = %d ",
             m_pEncContext->pSvcParam->bPrefixNalAddingCtrl);
  }
  break;
  case ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION: {
    int32_t iValue = * (static_cast<int32_t*>(pOption));
    EParameterSetStrategy eNewStrategy = CONSTANT_ID;
    switch (iValue) {
      case 0:
        eNewStrategy = CONSTANT_ID;
        break;
      case 0x01:
        eNewStrategy = INCREASING_ID;
        break;
      case 0x02:
        eNewStrategy = SPS_LISTING;
        break;
      case 0x03:
        eNewStrategy = SPS_LISTING_AND_PPS_INCREASING;
        break;
      case 0x06:
        eNewStrategy = SPS_PPS_LISTING;
        break;
      default:
        WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
                 " CWelsH264SVCEncoder::SetOption eSpsPpsIdStrategy(%d) not in valid range, unchanged! existing=%d",
                 iValue, m_pEncContext->pSvcParam->eSpsPpsIdStrategy);
        break;
    }

    if (((eNewStrategy & SPS_LISTING) || (m_pEncContext->pSvcParam->eSpsPpsIdStrategy & SPS_LISTING))
        && m_pEncContext->pSvcParam->eSpsPpsIdStrategy != eNewStrategy) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               " CWelsH264SVCEncoder::SetOption eSpsPpsIdStrategy changing in the middle of call is NOT allowed for eSpsPpsIdStrategy>INCREASING_ID: existing setting is %d and the new one is %d",
               m_pEncContext->pSvcParam->eSpsPpsIdStrategy, iValue);
      return cmInitParaError;
    }
    m_pEncContext->pSvcParam->eSpsPpsIdStrategy = eNewStrategy;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, " CWelsH264SVCEncoder::SetOption eSpsPpsIdStrategy = %d ",
             m_pEncContext->pSvcParam->eSpsPpsIdStrategy);
  }
  break;
  case ENCODER_OPTION_CURRENT_PATH: {
    if (m_pEncContext->pSvcParam != NULL) {
      char* path = static_cast<char*> (pOption);
      m_pEncContext->pSvcParam->pCurPath = path;
    }
  }
  break;
  case ENCODER_OPTION_DUMP_FILE: {
#ifdef ENABLE_FRAME_DUMP
    if (m_pEncContext->pSvcParam != NULL) {
      SDumpLayer* pDump = (static_cast<SDumpLayer*> (pOption));
      WelsStrncpy (m_pEncContext->pSvcParam->sDependencyLayers[pDump->iLayer].sRecFileName,
                   sizeof (m_pEncContext->pSvcParam->sDependencyLayers[pDump->iLayer].sRecFileName), pDump->pFileName);
    }
#endif
  }
  break;
  case ENCODER_OPTION_TRACE_LEVEL: {
    if (m_pWelsTrace) {
      uint32_t level = * ((uint32_t*)pOption);
      m_pWelsTrace->SetTraceLevel (level);
    }
  }
  break;
  case ENCODER_OPTION_TRACE_CALLBACK: {
    if (m_pWelsTrace) {
      WelsTraceCallback callback = * ((WelsTraceCallback*)pOption);
      m_pWelsTrace->SetTraceCallback (callback);
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsH264SVCEncoder::SetOption(), openh264 codec version = %s.",
               VERSION_NUMBER);
    }
  }
  break;
  case ENCODER_OPTION_TRACE_CALLBACK_CONTEXT: {
    if (m_pWelsTrace) {
      void* ctx = * ((void**)pOption);
      m_pWelsTrace->SetTraceCallbackContext (ctx);
    }
  }
  break;
  case ENCODER_OPTION_PROFILE: {
    SProfileInfo* pProfileInfo = (static_cast<SProfileInfo*> (pOption));
    if ((pProfileInfo->iLayer < SPATIAL_LAYER_0) || (pProfileInfo->iLayer > SPATIAL_LAYER_3)) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_PROFILE,iLayer = %d(rang0-3)", pProfileInfo->iLayer);
      return cmInitParaError;
    }
    CheckProfileSetting (&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam, pProfileInfo->iLayer,
                         pProfileInfo->uiProfileIdc);
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_PROFILE,layerId = %d,expected profile = %d,actual profile = %d",
             pProfileInfo->iLayer, pProfileInfo->uiProfileIdc,
             m_pEncContext->pSvcParam->sSpatialLayers[pProfileInfo->iLayer].uiProfileIdc);
  }
  break;
  case ENCODER_OPTION_LEVEL: {
    SLevelInfo* pLevelInfo = (static_cast<SLevelInfo*> (pOption));
    if ((pLevelInfo->iLayer < SPATIAL_LAYER_0) || (pLevelInfo->iLayer > SPATIAL_LAYER_3)) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_PROFILE,iLayer = %d(rang0-3)", pLevelInfo->iLayer);
      return cmInitParaError;
    }
    CheckLevelSetting (&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam, pLevelInfo->iLayer, pLevelInfo->uiLevelIdc);
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_LEVEL,layerId = %d,expected level = %d,actual level = %d",
             pLevelInfo->iLayer, pLevelInfo->uiLevelIdc, m_pEncContext->pSvcParam->sSpatialLayers[pLevelInfo->iLayer].uiLevelIdc);
  }
  break;
  case ENCODER_OPTION_NUMBER_REF: {
    int32_t iValue = * ((int32_t*)pOption);
    CheckReferenceNumSetting (&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam, iValue);
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_NUMBER_REF,expected refNum = %d,actual refnum = %d", iValue,
             m_pEncContext->pSvcParam->iNumRefFrame);
  }
  break;
  case ENCODER_OPTION_DELIVERY_STATUS: {
    SDeliveryStatus* pValue = (static_cast<SDeliveryStatus*> (pOption));
    m_pEncContext->bDeliveryFlag = pValue->bDeliveryFlag;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_DELIVERY_STATUS,bDeliveryFlag = %d", pValue->bDeliveryFlag);
  }
  break;
  case ENCODER_OPTION_COMPLEXITY: {
    int32_t iValue = * (static_cast<int32_t*> (pOption));
    m_pEncContext->pSvcParam->iComplexityMode = (ECOMPLEXITY_MODE)iValue;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_COMPLEXITY,iComplexityMode = %d", iValue);
  }
  break;
  case ENCODER_OPTION_GET_STATISTICS: {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_WARNING,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_GET_STATISTICS: this option is get-only!");
  }
  break;
  case ENCODER_OPTION_STATISTICS_LOG_INTERVAL: {
    int32_t iValue = * (static_cast<int32_t*> (pOption));
    m_pEncContext->iStatisticsLogInterval = iValue;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_STATISTICS_LOG_INTERVAL,iStatisticsLogInterval = %d", iValue);
  }
  break;
  case ENCODER_OPTION_IS_LOSSLESS_LINK: {
    bool bValue = * (static_cast<bool*> (pOption));
    m_pEncContext->pSvcParam->bIsLosslessLink = bValue;
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_IS_LOSSLESS_LINK,bIsLosslessLink = %d", bValue);
  }
  break;
  case ENCODER_OPTION_BITS_VARY_PERCENTAGE: {
    int32_t iValue = * (static_cast<int32_t*> (pOption));
    m_pEncContext->pSvcParam->iBitsVaryPercentage = WELS_CLIP3 (iValue, 0, 100);
    WelsEncoderApplyBitVaryRang (&m_pWelsTrace->m_sLogCtx, m_pEncContext->pSvcParam,
                                 m_pEncContext->pSvcParam->iBitsVaryPercentage);
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITS_VARY_PERCENTAGE,iBitsVaryPercentage = %d", iValue);
  }
  break;

  default:
    return cmInitParaError;
  }

  return 0;
}

int CWelsH264SVCEncoder::GetOption (ENCODER_OPTION eOptionId, void* pOption) {
  if (NULL == pOption) {
    return cmInitParaError;
  }
  if (NULL == m_pEncContext || false == m_bInitialFlag) {
    return cmInitExpected;
  }

  switch (eOptionId) {
  case ENCODER_OPTION_INTER_SPATIAL_PRED: {	// Inter spatial layer prediction flag
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "ENCODER_OPTION_INTER_SPATIAL_PRED, this feature not supported at present.");
  }
  break;
  case ENCODER_OPTION_DATAFORMAT: {	// Input color space
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_DATAFORMAT, m_iCspInternal= 0x%x", m_iCspInternal);
    * ((int32_t*)pOption)	= m_iCspInternal;
  }
  break;
  case ENCODER_OPTION_IDR_INTERVAL: {	// IDR Interval
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_IDR_INTERVAL, uiIntraPeriod= %d",
             m_pEncContext->pSvcParam->uiIntraPeriod);
    * ((int32_t*)pOption) = m_pEncContext->pSvcParam->uiIntraPeriod;
  }
  break;
  case ENCODER_OPTION_SVC_ENCODE_PARAM_EXT: {	// SVC Encoding Parameter
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT");
    memcpy (pOption, m_pEncContext->pSvcParam, sizeof (SEncParamExt));	// confirmed_safe_unsafe_usage
  }
  break;
  case ENCODER_OPTION_SVC_ENCODE_PARAM_BASE: {	// SVC Encoding Parameter
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_BASE");
    m_pEncContext->pSvcParam->GetBaseParams ((SEncParamBase*) pOption);
  }
  break;

  case ENCODER_OPTION_FRAME_RATE: {	// Maximal input frame rate
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_FRAME_RATE, fMaxFrameRate = %.6ff",
             m_pEncContext->pSvcParam->fMaxFrameRate);
    * ((float*)pOption)	= m_pEncContext->pSvcParam->fMaxFrameRate;
  }
  break;
  case ENCODER_OPTION_BITRATE: {	// Target bit-rate

    SBitrateInfo* pInfo = (static_cast<SBitrateInfo*> (pOption));
    if ((pInfo->iLayer != SPATIAL_LAYER_ALL) && (pInfo->iLayer != SPATIAL_LAYER_0) && (pInfo->iLayer != SPATIAL_LAYER_1)
        && (pInfo->iLayer != SPATIAL_LAYER_2) && (pInfo->iLayer != SPATIAL_LAYER_3))
      return cmInitParaError;
    if (pInfo->iLayer == SPATIAL_LAYER_ALL) {
      pInfo->iBitrate = m_pEncContext->pSvcParam->iTargetBitrate;
    } else {
      pInfo->iBitrate = m_pEncContext->pSvcParam->sSpatialLayers[pInfo->iLayer].iSpatialBitrate;
    }
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_BITRATE, layerId =%d,iBitrate = %d",
             pInfo->iLayer, pInfo->iBitrate);
  }
  break;
  case ENCODER_OPTION_MAX_BITRATE: {	// Target bit-rate
    SBitrateInfo* pInfo = (static_cast<SBitrateInfo*> (pOption));
    if ((pInfo->iLayer != SPATIAL_LAYER_ALL) && (pInfo->iLayer != SPATIAL_LAYER_0) && (pInfo->iLayer != SPATIAL_LAYER_1)
        && (pInfo->iLayer != SPATIAL_LAYER_2) && (pInfo->iLayer != SPATIAL_LAYER_3))
      return cmInitParaError;
    if (pInfo->iLayer == SPATIAL_LAYER_ALL) {
      pInfo->iBitrate = m_pEncContext->pSvcParam->iMaxBitrate;
    } else {
      pInfo->iBitrate = m_pEncContext->pSvcParam->sSpatialLayers[pInfo->iLayer].iMaxSpatialBitrate;
    }
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_MAX_BITRATE,, layerId =%d,iBitrate = %d",
             pInfo->iLayer, pInfo->iBitrate);
  }
  break;
  case ENCODER_OPTION_GET_STATISTICS: {
    SEncoderStatistics* pStatistics = (static_cast<SEncoderStatistics*> (pOption));
    pStatistics->uiWidth = m_pEncContext->sEncoderStatistics.uiWidth;
    pStatistics->uiHeight = m_pEncContext->sEncoderStatistics.uiHeight;
    pStatistics->fAverageFrameSpeedInMs = m_pEncContext->sEncoderStatistics.fAverageFrameSpeedInMs;

    // rate control related
    pStatistics->fAverageFrameRate = m_pEncContext->sEncoderStatistics.fAverageFrameRate;
    pStatistics->fLatestFrameRate = m_pEncContext->sEncoderStatistics.fLatestFrameRate;
    pStatistics->uiBitRate = m_pEncContext->sEncoderStatistics.uiBitRate;

    pStatistics->uiInputFrameCount = m_pEncContext->sEncoderStatistics.uiInputFrameCount;
    pStatistics->uiSkippedFrameCount = m_pEncContext->sEncoderStatistics.uiSkippedFrameCount;

    pStatistics->uiResolutionChangeTimes = m_pEncContext->sEncoderStatistics.uiResolutionChangeTimes;
    pStatistics->uiIDRReqNum = m_pEncContext->sEncoderStatistics.uiIDRReqNum;
    pStatistics->uiIDRSentNum = m_pEncContext->sEncoderStatistics.uiIDRSentNum;
    pStatistics->uiLTRSentNum = m_pEncContext->sEncoderStatistics.uiLTRSentNum;
  }
  break;
  case ENCODER_OPTION_STATISTICS_LOG_INTERVAL: {
    * ((int32_t*)pOption)	= m_pEncContext->iStatisticsLogInterval;
  }
  break;
  case ENCODER_OPTION_COMPLEXITY: {
    * ((int32_t*)pOption) =  m_pEncContext->pSvcParam->iComplexityMode;
  }
  break;
  default:
    return cmInitParaError;
  }

  return 0;
}

void CWelsH264SVCEncoder::DumpSrcPicture (const uint8_t* pSrc) {
#ifdef DUMP_SRC_PICTURE
  FILE* pFile = NULL;
  char strFileName[256] = {0};
  const int32_t iDataLength = m_iMaxPicWidth * m_iMaxPicHeight;

  WelsStrncpy (strFileName, 256, "pic_in_");	// confirmed_safe_unsafe_usage

  if (m_iMaxPicWidth == 640) {
    WelsStrcat (strFileName, 256, "360p.");	// confirmed_safe_unsafe_usage
  } else if (m_iMaxPicWidth == 320) {
    WelsStrcat (strFileName, 256, "180p.");	// confirmed_safe_unsafe_usage
  } else if (m_iMaxPicWidth == 160) {
    WelsStrcat (strFileName, 256, "90p.");	// confirmed_safe_unsafe_usage
  }

  switch (m_iCspInternal) {
  case videoFormatI420:
  case videoFormatYV12:
    WelsStrcat (strFileName, 256, "yuv");	// confirmed_safe_unsafe_usage
    pFile = WelsFopen (strFileName, "ab+");
    //				WelsLog( &m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "WELS_CSP_I420, m_iCspInternal= 0x%x", m_iCspInternal);
    if (NULL != pFile) {
      fwrite (pSrc, sizeof (uint8_t), (iDataLength * 3) >> 1, pFile);
      fflush (pFile);
      fclose (pFile);
    }
    break;
  case videoFormatRGB:
    WelsStrcat (strFileName, 256, "rgb");	// confirmed_safe_unsafe_usage
    pFile = WelsFopen (strFileName, "ab+");
    if (NULL != pFile) {
      fwrite (pSrc, sizeof (uint8_t), iDataLength * 3, pFile);
      fflush (pFile);
      fclose (pFile);
    }
  case videoFormatBGR:
    WelsStrcat (strFileName, 256, "bgr");	// confirmed_safe_unsafe_usage
    pFile = WelsFopen (strFileName, "ab+");
    //				WelsLog( &m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "WELS_CSP_BGR, m_iCspInternal= 0x%x", m_iCspInternal);
    if (NULL != pFile) {
      fwrite (pSrc, sizeof (uint8_t), iDataLength * 3, pFile);
      fflush (pFile);
      fclose (pFile);
    }
    break;
  case videoFormatYUY2:
    WelsStrcat (strFileName, 256, "yuy2");	// confirmed_safe_unsafe_usage
    pFile = WelsFopen (strFileName, "ab+");
    if (NULL != pFile) {
      fwrite (pSrc, sizeof (uint8_t), (CALC_BI_STRIDE (m_iMaxPicWidth,  16)) * m_iMaxPicHeight, pFile);
      fflush (pFile);
      fclose (pFile);
    }
    break;
  default:
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "Exclusive case, m_iCspInternal= 0x%x", m_iCspInternal);
    break;
  }
#endif//DUMP_SRC_PICTURE
  return;
}
}

using namespace WelsEnc;

int32_t WelsCreateSVCEncoder (ISVCEncoder** ppEncoder) {
  if ((*ppEncoder = new CWelsH264SVCEncoder()) != NULL) {
    return 0;
  }

  return 1;
}

void WelsDestroySVCEncoder (ISVCEncoder* pEncoder) {
  CWelsH264SVCEncoder* pSVCEncoder = (CWelsH264SVCEncoder*)pEncoder;

  if (pSVCEncoder) {
    delete pSVCEncoder;
    pSVCEncoder = NULL;
  }
}

OpenH264Version WelsGetCodecVersion() {
  return g_stCodecVersion;
}

void WelsGetCodecVersionEx (OpenH264Version* pVersion) {
  *pVersion = g_stCodecVersion;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
