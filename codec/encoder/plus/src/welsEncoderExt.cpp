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

#include "crt_util_safe_x.h"	// Safe CRT routines like util for cross platforms
#include "ref_list_mgr_svc.h"

#include <time.h>
#if defined(_WIN32) /*&& defined(_DEBUG)*/

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

namespace WelsSVCEnc {

/*
 *	CWelsH264SVCEncoder class implementation
 */
CWelsH264SVCEncoder::CWelsH264SVCEncoder()
  :	m_pEncContext (NULL),
    m_pWelsTrace (NULL),
    m_pSrcPicList (NULL),
    m_iSrcListSize (0),
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

  WelsGetTimeOfDay(&tTime);

  iCurUsed      = WelsSnprintf (strStreamFileName, iBufferLeft, "enc_bs_0x%p_", (void*)this);
  iCurUsedSize  = WelsSnprintf (strLenFileName, iBufferLeftSize, "enc_size_0x%p_", (void*)this);


  if (iCurUsed > 0) {
    iBufferUsed += iCurUsed;
    iBufferLeft -= iCurUsed;
  }
  if (iBufferLeft > 0) {
    iCurUsed = WelsStrftime (&strStreamFileName[iBufferUsed], iBufferLeft, "%y%m%d%H%M%S", &tTime);
    iBufferUsed += iCurUsed;
    iBufferLeft -= iCurUsed;
  }

  if (iCurUsedSize > 0) {
    iBufferUsedSize += iCurUsedSize;
    iBufferLeftSize -= iCurUsedSize;
  }
  if (iBufferLeftSize > 0) {
    iCurUsedSize = WelsStrftime (&strLenFileName[iBufferUsedSize], iBufferLeftSize, "%y%m%d%H%M%S", &tTime);
    iBufferUsedSize += iCurUsedSize;
    iBufferLeftSize -= iCurUsedSize;
  }

  if (iBufferLeft > 0) {
    iCurUsed = WelsSnprintf (&strStreamFileName[iBufferUsed], iBufferLeft, ".%03.3u.264",
                             WelsGetMillisecond(&tTime));
    if (iCurUsed > 0) {
      iBufferUsed += iCurUsed;
      iBufferLeft -= iCurUsed;
    }
  }

  if (iBufferLeftSize > 0) {
    iCurUsedSize = WelsSnprintf (&strLenFileName[iBufferUsedSize], iBufferLeftSize, ".%03.3u.len",
                                 WelsGetMillisecond(&tTime));
    if (iCurUsedSize > 0) {
      iBufferUsedSize += iCurUsedSize;
      iBufferLeftSize -= iCurUsedSize;
    }
  }

  m_pFileBs     = WelsFopen (strStreamFileName, "wb");
  m_pFileBsSize = WelsFopen (strLenFileName, "wb");

  m_bSwitch	= false;
  m_iSwitchTimes	= 0;
#endif//OUTPUT_BIT_STREAM

  InitEncoder();
}

CWelsH264SVCEncoder::~CWelsH264SVCEncoder() {
  WelsLog (NULL, WELS_LOG_INFO, "CWelsH264SVCEncoder::~CWelsH264SVCEncoder()\n");

  if (m_pWelsTrace != NULL) {
    delete m_pWelsTrace;
    m_pWelsTrace = NULL;
  }
#ifdef REC_FRAME_COUNT
  WelsLog (m_pEncContext, WELS_LOG_INFO,
           "CWelsH264SVCEncoder::~CWelsH264SVCEncoder(), m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n", m_uiCountFrameNum,
           m_iCspInternal);
#endif

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
}

void CWelsH264SVCEncoder::InitEncoder (void) {

#ifdef REC_FRAME_COUNT
  WelsLog (m_pEncContext, WELS_LOG_INFO,
           "CWelsH264SVCEncoder::InitEncoder, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n", m_uiCountFrameNum, m_iCspInternal);
#endif

  m_pWelsTrace	= new welsCodecTrace();
  if (m_pWelsTrace != NULL) {
    const int32_t iWelsTraceExistingFlag = m_pWelsTrace->WelsTraceModuleIsExist();
    if (iWelsTraceExistingFlag) {
      m_pWelsTrace->SetTraceLevel (WELS_LOG_ERROR);
      WelsSetLogCallback (welsCodecTrace::CODEC_TRACE);
    }
  }

  // initialization
  WelsSetLogLevel (WELS_LOG_ERROR);	// no output, WELS_LOG_QUIET
}

/* Interfaces override from ISVCEncoder */

int CWelsH264SVCEncoder::GetDefaultParams (SEncParamExt* argv) {
  SWelsSvcCodingParam::FillDefault(*argv, true);
  return cmResultSuccess;
}

/*
 *	SVC Encoder Initialization
 */
int CWelsH264SVCEncoder::Initialize (const SEncParamBase* argv) {

  if (NULL == argv) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), invalid argv= 0x%p\n",
             argv);
    return cmInitParaError;
  }

  SWelsSvcCodingParam	sConfig (true);
  // Convert SEncParamBase into WelsSVCParamConfig here..
  if (sConfig.ParamBaseTranscode (*argv, true)) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), parameter_translation failed.\n");
    Uninitialize();
    return cmInitParaError;
  }

  return Initialize2 (&sConfig);
}

int CWelsH264SVCEncoder::InitializeExt (const SEncParamExt* argv) {

  if (NULL == argv) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), invalid argv= 0x%p\n",
             argv);
    return cmInitParaError;
  }

  SWelsSvcCodingParam	sConfig (true);
  // Convert SEncParamExt into WelsSVCParamConfig here..
  if (sConfig.ParamTranscode (*argv)) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), parameter_translation failed.\n");
    Uninitialize();
    return cmInitParaError;
  }

  return Initialize2 (&sConfig);
}

int CWelsH264SVCEncoder::Initialize2 (SWelsSvcCodingParam* pCfg) {
  if (NULL == pCfg) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), invalid argv= 0x%p.\n",
             pCfg);
    return cmInitParaError;
  }

  if (m_bInitialFlag) {
    WelsLog (m_pEncContext, WELS_LOG_WARNING, "CWelsH264SVCEncoder::Initialize(), reinitialize, m_bInitialFlag= %d.\n",
             m_bInitialFlag);
    Uninitialize();
  }

#ifdef REC_FRAME_COUNT
  SWelsSvcCodingParam &sEncodingParam = *pCfg;
  WelsLog (m_pEncContext, WELS_LOG_INFO, "CWelsH264SVCEncoder::Initialize, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n",
           m_uiCountFrameNum, m_iCspInternal);
  WelsLog (m_pEncContext, WELS_LOG_INFO,
           "coding_param->iPicWidth= %d;coding_param->iPicHeight= %d;coding_param->iTargetBitrate= %d;coding_param->iRCMode= %d;coding_param->iTemporalLayerNum= %d;coding_param->iSpatialLayerNum= %d;coding_param->fFrameRate= %.6ff;coding_param->iInputCsp= %d;coding_param->uiIntraPeriod= %d;coding_param->bEnableSpsPpsIdAddition = %d;coding_param->bPrefixNalAddingCtrl = %d;coding_param->bEnableDenoise= %d;coding_param->bEnableBackgroundDetection= %d;coding_param->bEnableAdaptiveQuant= %d;coding_param->bEnableFrameSkip= %d;coding_param->bEnableCropPic= %d;coding_param->bEnableLongTermReference= %d;coding_param->iLtrMarkPeriod= %d;\n",
           sEncodingParam.iPicWidth,
           sEncodingParam.iPicHeight,
           sEncodingParam.iTargetBitrate,
           sEncodingParam.iRCMode,
           sEncodingParam.iTemporalLayerNum,
           sEncodingParam.iSpatialLayerNum,
           sEncodingParam.fMaxFrameRate,
           sEncodingParam.iInputCsp,
           sEncodingParam.uiIntraPeriod,
           sEncodingParam.bEnableSpsPpsIdAddition,
           sEncodingParam.bPrefixNalAddingCtrl,
           sEncodingParam.bEnableDenoise,
           sEncodingParam.bEnableBackgroundDetection,
           sEncodingParam.bEnableAdaptiveQuant,
           sEncodingParam.bEnableFrameSkip,
           sEncodingParam.bEnableCropPic,
           sEncodingParam.bEnableLongTermReference,
           sEncodingParam.iLtrMarkPeriod);
  int32_t i = 0;
  while (i < sEncodingParam.iSpatialLayerNum) {
    SSpatialLayerConfig* spatial_cfg = &sEncodingParam.sSpatialLayers[i];
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "coding_param->sSpatialLayers[%d]: .iVideoWidth= %d; .iVideoHeight= %d; .fFrameRate= %.6ff; .iSpatialBitrate= %d; .sSliceCfg.uiSliceMode= %d; .sSliceCfg.sSliceArgument.uiSliceNum= %d; .sSliceCfg.sSliceArgument.uiSliceSizeConstraint= %d;\n",
             i, spatial_cfg->iVideoWidth,
             spatial_cfg->iVideoHeight,
             spatial_cfg->fFrameRate,
             spatial_cfg->iSpatialBitrate,
             spatial_cfg->sSliceCfg.uiSliceMode,
             spatial_cfg->sSliceCfg.sSliceArgument.uiSliceNum,
             spatial_cfg->sSliceCfg.sSliceArgument.uiSliceSizeConstraint
            );
    ++ i;
  }
#endif//REC_FRAME_COUNT

  const int32_t iColorspace = pCfg->iInputCsp;
  if (0 == iColorspace) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), invalid iInputCsp= %d.\n", iColorspace);
    Uninitialize();
    return cmInitParaError;
  }

  // Check valid parameters
  const int32_t iNumOfLayers = pCfg->iSpatialLayerNum;
  if (iNumOfLayers < 1 || iNumOfLayers > MAX_DEPENDENCY_LAYER) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid iSpatialLayerNum= %d, valid at range of [1, %d].\n", iNumOfLayers,
             MAX_DEPENDENCY_LAYER);
    Uninitialize();
    return cmInitParaError;
  }
  if (pCfg->iTemporalLayerNum < 1)
    pCfg->iTemporalLayerNum	= 1;
  if (pCfg->iTemporalLayerNum > MAX_TEMPORAL_LEVEL) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid iTemporalLayerNum= %d, valid at range of [1, %d].\n",
             pCfg->iTemporalLayerNum, MAX_TEMPORAL_LEVEL);
    Uninitialize();
    return cmInitParaError;
  }

  //	assert( cfg.uiGopSize >= 1 && ( cfg.uiIntraPeriod && (cfg.uiIntraPeriod % cfg.uiGopSize) == 0) );

  if (pCfg->uiGopSize < 1 || pCfg->uiGopSize > MAX_GOP_SIZE) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiGopSize= %d, valid at range of [1, %d].\n", pCfg->uiGopSize,
             MAX_GOP_SIZE);
    Uninitialize();
    return cmInitParaError;
  }

  if (!WELS_POWER2_IF (pCfg->uiGopSize)) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiGopSize= %d, valid at range of [1, %d] and yield to power of 2.\n",
             pCfg->uiGopSize, MAX_GOP_SIZE);
    Uninitialize();
    return cmInitParaError;
  }

  if (pCfg->uiIntraPeriod && pCfg->uiIntraPeriod < pCfg->uiGopSize) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiIntraPeriod= %d, valid in case it equals to 0 for unlimited intra period or exceeds specified uiGopSize= %d.\n",
             pCfg->uiIntraPeriod, pCfg->uiGopSize);
    Uninitialize();
    return cmInitParaError;
  }

  if ((pCfg->uiIntraPeriod && (pCfg->uiIntraPeriod & (pCfg->uiGopSize - 1)) != 0)) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), invalid uiIntraPeriod= %d, valid in case it equals to 0 for unlimited intra period or exceeds specified uiGopSize= %d also multiple of it.\n",
             pCfg->uiIntraPeriod, pCfg->uiGopSize);
    Uninitialize();
    return cmInitParaError;
  }

  // Fine tune num_ref_num
  if (pCfg->bEnableLongTermReference) {
    pCfg->iLTRRefNum = LONG_TERM_REF_NUM;
  } else {
    pCfg->iLTRRefNum = 0;
  }
  pCfg->iNumRefFrame = ((pCfg->uiGopSize >> 1) > 1) ? ((pCfg->uiGopSize >> 1) + pCfg->iLTRRefNum) :
                       (MIN_REF_PIC_COUNT + pCfg->iLTRRefNum);

  pCfg->iNumRefFrame = WELS_CLIP3 (pCfg->iNumRefFrame, MIN_REF_PIC_COUNT, MAX_REFERENCE_PICTURE_COUNT_NUM);

  if (pCfg->iLtrMarkPeriod == 0) {
    pCfg->iLtrMarkPeriod = 30;
  }

  const int32_t kiDecStages = WELS_LOG2 (pCfg->uiGopSize);
  pCfg->iInputCsp			= iColorspace;
  pCfg->iTemporalLayerNum	= (int8_t) (1 + kiDecStages);
  pCfg->iLoopFilterAlphaC0Offset	= WELS_CLIP3 (pCfg->iLoopFilterAlphaC0Offset, -6, 6);
  pCfg->iLoopFilterBetaOffset		= WELS_CLIP3 (pCfg->iLoopFilterBetaOffset, -6, 6);

//	m_pSrcPicList	= (SSourcePicture **)WelsMalloc( pCfg->iSpatialLayerNum * sizeof(SSourcePicture *), "m_pSrcPicList" );
  // prefer use new/delete pair due encoder intialization stage not start yet for CacheLineSize not detection here (16 or 64 not matched)
  m_pSrcPicList	= new SSourcePicture* [iNumOfLayers];

  if (NULL == m_pSrcPicList) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR,
             "CWelsH264SVCEncoder::Initialize(), pOut of memory due m_pSrcPicList memory request.\n");
    Uninitialize();
    return cmMallocMemeError;
  }

  // decide property list size between INIT_TYPE_PARAMETER_BASED/INIT_TYPE_CONFIG_BASED
  m_iMaxPicWidth	= pCfg->iPicWidth;
  m_iMaxPicHeight	= pCfg->iPicHeight;
  m_iSrcListSize  = iNumOfLayers;

  for (int32_t i = 0; i < m_iSrcListSize; ++ i) {
//		m_pSrcPicList[i]	= (SSourcePicture *)WelsMalloc( sizeof(SSourcePicture), "m_pSrcPicList[]" );
    // prefer use new/delete pair due encoder intialization stage not start yet for CacheLineSize not detection here (16 or 64 not matched)
    m_pSrcPicList[i]	= new SSourcePicture;

    if (NULL == m_pSrcPicList[i]) {
      WelsLog (m_pEncContext, WELS_LOG_ERROR,
               "CWelsH264SVCEncoder::Initialize(), pOut of memory due m_pSrcPicList[%d] memory request.\n", i);
      Uninitialize();
      m_iSrcListSize = 0;
      return cmMallocMemeError;
    }
    if(InitPic (m_pSrcPicList[i], iColorspace, m_iMaxPicWidth, m_iMaxPicHeight))
    {
       WelsLog (m_pEncContext, WELS_LOG_ERROR,
                 "CWelsH264SVCEncoder::Initialize(), InitPic Failed iColorspace= 0x%x\n", iColorspace);
       Uninitialize();
       return cmInitParaError;
    }
    }

  if (WelsInitEncoderExt (&m_pEncContext, pCfg)) {
    WelsLog (m_pEncContext, WELS_LOG_ERROR, "CWelsH264SVCEncoder::Initialize(), WelsInitEncoderExt failed.\n");
    Uninitialize();
    return cmInitParaError;
  }

  m_iCspInternal	= iColorspace;
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

  WelsLog (m_pEncContext, WELS_LOG_INFO, "CWelsH264SVCEncoder::Uninitialize()..\n");

#ifdef REC_FRAME_COUNT
  WelsLog (m_pEncContext, WELS_LOG_INFO,
           "CWelsH264SVCEncoder::Uninitialize, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n", m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT

  if (NULL != m_pEncContext) {
    if (NULL != m_pSrcPicList) {
      for (int32_t i = 0; i < m_iSrcListSize; i++) {
        SSourcePicture* pic = m_pSrcPicList[i];
        if (NULL != pic) {
//					WelsFree( pic, "m_pSrcPicList[]" );
          // prefer use new/delete pair due encoder intialization stage not start yet for CacheLineSize not detection here (16 or 64 not matched)
          delete pic;

          pic = NULL;
        }
      }
//			WelsFree( m_pSrcPicList, "m_pSrcPicList" );
      // prefer use new/delete pair due encoder intialization stage not start yet for CacheLineSize not detection here (16 or 64 not matched)
      delete [] m_pSrcPicList;

      m_pSrcPicList = NULL;
      m_iSrcListSize = 0;
    }

    WelsUninitEncoderExt (&m_pEncContext);
    m_pEncContext	= NULL;
  }

  m_bInitialFlag = false;

  return 0;
}


int32_t CWelsH264SVCEncoder::RawData2SrcPic (const uint8_t* pSrc) {
  assert (m_iSrcListSize > 0);

  int32_t y_length = m_iMaxPicWidth * m_iMaxPicHeight;
  m_pSrcPicList[0]->pData[0] = const_cast<uint8_t*> (pSrc);

  switch (m_iCspInternal & (~videoFormatVFlip)) {
  case videoFormatYVYU:
  case videoFormatUYVY:
  case videoFormatYUY2:
  case videoFormatRGB:
  case videoFormatBGR:
  case videoFormatBGRA:
  case videoFormatRGBA:
  case videoFormatARGB:
  case videoFormatABGR:
    m_pSrcPicList[0]->pData[1] = m_pSrcPicList[0]->pData[2] = NULL;
    break;
  case videoFormatI420:
  case videoFormatYV12:
    m_pSrcPicList[0]->pData[1] = m_pSrcPicList[0]->pData[0] + y_length;
    m_pSrcPicList[0]->pData[2] = m_pSrcPicList[0]->pData[1] + (y_length >> 2);
    break;
  default:
    return 1;
  }

  return 0;
}


/*
 *	SVC core encoding
 */
int CWelsH264SVCEncoder::EncodeFrame (const SSourcePicture* kpSrcPic, SFrameBSInfo* pBsInfo) {
  if (! (kpSrcPic && m_pEncContext && m_bInitialFlag)) {
    return cmInitParaError;
  }

  const int32_t kiEncoderReturn = EncodeFrameInternal(kpSrcPic, pBsInfo);

  if(kiEncoderReturn != cmResultSuccess)
    return kiEncoderReturn;

#ifdef REC_FRAME_COUNT
  ++ m_uiCountFrameNum;
  WelsLog (m_pEncContext, WELS_LOG_INFO,
           "CWelsH264SVCEncoder::EncodeFrame(), m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n", m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT

#ifdef DUMP_SRC_PICTURE
  DumpSrcPicture (pSrc);
#endif // DUMP_SRC_PICTURE
  return kiEncoderReturn;
}


int CWelsH264SVCEncoder::EncodeFrameInternal(const SSourcePicture*  pSrcPic, SFrameBSInfo* pBsInfo) {
  if (!(pSrcPic && m_pEncContext && m_bInitialFlag) ){
    return cmInitParaError;
  }

  const int32_t kiEncoderReturn = WelsEncoderEncodeExt (m_pEncContext, pBsInfo, pSrcPic);

  if(kiEncoderReturn == ENC_RETURN_MEMALLOCERR) {
    WelsUninitEncoderExt (&m_pEncContext);
    return cmMallocMemeError;
  }
  else if((kiEncoderReturn != ENC_RETURN_SUCCESS)&&(kiEncoderReturn == ENC_RETURN_CORRECTED)){
    WelsLog (m_pEncContext, WELS_LOG_ERROR, "unexpected return(%d) from EncodeFrameInternal()!\n", kiEncoderReturn);
    return cmUnkonwReason;
  }
  ///////////////////for test
#ifdef OUTPUT_BIT_STREAM
  if (pBsInfo->eOutputFrameType != videoFrameTypeInvalid && pBsInfo->eOutputFrameType != videoFrameTypeSkip) {
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
      int32_t iLen = WelsSnprintf (strStreamFileName, 128, "adj%d_w%d.264", m_iSwitchTimes,
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
        iCurLayerBits += pLayer->iNalLengthInByte[j];
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
 * return: 0 - success; otherwise - failed;
 */
int CWelsH264SVCEncoder::PauseFrame (const SSourcePicture* kpSrcPic, SFrameBSInfo* pBsInfo) {
  int32_t  iReturn = 1;

  ForceIntraFrame (true);

  if (EncodeFrameInternal (kpSrcPic, pBsInfo) != videoFrameTypeInvalid) {
	iReturn = 0;
  }


  // to avoid pause frame bitstream and
  // normal bitstream use different video channel.
  ForceIntraFrame (true);

  return (int)iReturn;
}


/*
 *	Force key frame
 */
int CWelsH264SVCEncoder::ForceIntraFrame (bool bIDR) {
  if (! (m_pEncContext && m_bInitialFlag)) {
    return 1;
  }

#ifdef REC_FRAME_COUNT
  WelsLog (m_pEncContext, WELS_LOG_INFO,
           "CWelsH264SVCEncoder::ForceIntraFrame(), bIDR= %d, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n", bIDR,
           m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT

  ForceCodingIDR (m_pEncContext);

  return 0;
}

/************************************************************************
* InDataFormat, IDRInterval, SVC Encode Param, Frame Rate, Bitrate,..
************************************************************************/
int CWelsH264SVCEncoder::SetOption (ENCODER_OPTION eOptionId, void* pOption) {
  if (NULL == pOption) {
    return cmInitParaError;
  }

  if (NULL == m_pEncContext || false == m_bInitialFlag) {
    return cmInitExpected;
  }

  switch (eOptionId) {
  case ENCODER_OPTION_INTER_SPATIAL_PRED: {	// Inter spatial layer prediction flag
    WelsLog (m_pEncContext, WELS_LOG_INFO, "ENCODER_OPTION_INTER_SPATIAL_PRED, this feature not supported at present.\n");
  }
  break;
  case ENCODER_OPTION_DATAFORMAT: {	// Input color space
    int32_t iValue = * ((int32_t*)pOption);
    int32_t iColorspace = iValue;
    if (iColorspace == 0) {
      return cmInitParaError;
    }

#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_DATAFORMAT, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x, iValue= %d\n",
             m_uiCountFrameNum, m_iCspInternal, iValue);
#endif//REC_FRAME_COUNT


    int32_t iPicIdx = m_iSrcListSize - 1;
    while (iPicIdx >= 0) {
      if (m_pSrcPicList[iPicIdx] == NULL) {
        -- iPicIdx;
        if (iPicIdx < 0) return cmInitParaError;
        continue;
      }

      if (m_pSrcPicList[iPicIdx]->iColorFormat == iColorspace) {
        -- iPicIdx;
        continue;
      }

      if(InitPic (m_pSrcPicList[iPicIdx], iColorspace, m_iMaxPicWidth, m_iMaxPicHeight))
      {
        WelsLog (m_pEncContext, WELS_LOG_INFO,
                 "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_DATAFORMAT, iColorspace= 0x%x\n",
                  iColorspace);
        return cmInitParaError;
      }
    }
    m_iCspInternal = iColorspace;
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_DATAFORMAT, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n",
             m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT
  }
  break;
  case ENCODER_OPTION_IDR_INTERVAL: {	// IDR Interval
    int32_t iValue	= * ((int32_t*)pOption);
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_IDR_INTERVAL, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x, iValue= %d\n",
             m_uiCountFrameNum, m_iCspInternal, iValue);
#endif//REC_FRAME_COUNT

    if (iValue < -1 || iValue == 0)
      iValue = 1;
    if (iValue == (int32_t)m_pEncContext->pSvcParam->uiIntraPeriod) {
      return cmResultSuccess;
    }


    m_pEncContext->pSvcParam->uiIntraPeriod	= (uint32_t)iValue;
  }
  break;
  case ENCODER_OPTION_SVC_ENCODE_PARAM_EXT: {	// SVC Encoding Parameter
    SEncParamExt		sEncodingParam;
    SWelsSvcCodingParam	sConfig (true);
    int32_t iInputColorspace = 0;
    int32_t iTargetWidth = 0;
    int32_t iTargetHeight = 0;

    memcpy (&sEncodingParam, pOption, sizeof (SEncParamExt));	// confirmed_safe_unsafe_usage
    WelsLog (m_pEncContext, WELS_LOG_INFO, "ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, sEncodingParam.iInputCsp= 0x%x\n",
             sEncodingParam.iInputCsp);
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "coding_param->iPicWidth= %d;coding_param->iPicHeight= %d;coding_param->iTargetBitrate= %d;coding_param->iRCMode= %d;coding_param->iPaddingFlag= %d;coding_param->iTemporalLayerNum= %d;coding_param->iSpatialLayerNum= %d;coding_param->fFrameRate= %.6ff;coding_param->iInputCsp= %d;coding_param->uiIntraPeriod= %d;coding_param->bEnableSpsPpsIdAddition = %d;coding_param->bPrefixNalAddingCtrl = %d;coding_param->bEnableDenoise= %d;coding_param->bEnableBackgroundDetection= %d;coding_param->bEnableAdaptiveQuant= %d;coding_param->bEnableAdaptiveQuant= %d;coding_param->bEnableLongTermReference= %d;coding_param->iLtrMarkPeriod= %d;\n",
             sEncodingParam.iPicWidth,
             sEncodingParam.iPicHeight,
             sEncodingParam.iTargetBitrate,
             sEncodingParam.iRCMode,
             sEncodingParam.iPaddingFlag,
             sEncodingParam.iTemporalLayerNum,
             sEncodingParam.iSpatialLayerNum,
             sEncodingParam.fMaxFrameRate,
             sEncodingParam.iInputCsp,
             sEncodingParam.uiIntraPeriod,
             sEncodingParam.bEnableSpsPpsIdAddition,
             sEncodingParam.bPrefixNalAddingCtrl,
             sEncodingParam.bEnableDenoise,
             sEncodingParam.bEnableBackgroundDetection,
             sEncodingParam.bEnableAdaptiveQuant,
             sEncodingParam.bEnableFrameSkip,
             sEncodingParam.bEnableLongTermReference,
             sEncodingParam.iLtrMarkPeriod);
    int32_t i = 0;
    while (i < sEncodingParam.iSpatialLayerNum) {
      SSpatialLayerConfig* pSpatialCfg = &sEncodingParam.sSpatialLayers[i];
      WelsLog (m_pEncContext, WELS_LOG_INFO,
               "coding_param->sSpatialLayers[%d]: .iVideoWidth= %d; .iVideoHeight= %d; .fFrameRate= %.6ff; .iSpatialBitrate= %d; .sSliceCfg.uiSliceMode= %d; .sSliceCfg.sSliceArgument.iSliceNum= %d; .sSliceCfg.sSliceArgument.uiSliceSizeConstraint= %d;\n",
               i, pSpatialCfg->iVideoWidth,
               pSpatialCfg->iVideoHeight,
               pSpatialCfg->fFrameRate,
               pSpatialCfg->iSpatialBitrate,
               pSpatialCfg->sSliceCfg.uiSliceMode,
               pSpatialCfg->sSliceCfg.sSliceArgument.uiSliceNum,
               pSpatialCfg->sSliceCfg.sSliceArgument.uiSliceSizeConstraint
              );
      ++ i;
    }
#ifdef OUTPUT_BIT_STREAM
    if (sEncodingParam.sSpatialLayers[sEncodingParam.iSpatialLayerNum - 1].iVideoWidth !=
        m_pEncContext->pSvcParam->sDependencyLayers[m_pEncContext->pSvcParam->iSpatialLayerNum - 1].iFrameWidth) {
      ++ m_iSwitchTimes;
      m_bSwitch = true;
    }
#endif//OUTPUT_BIT_STREAM
    if (sEncodingParam.iSpatialLayerNum < 1
        || sEncodingParam.iSpatialLayerNum > MAX_SPATIAL_LAYER_NUM) {	// verify number of spatial layer
      return cmInitParaError;
    }

    iInputColorspace	= sEncodingParam.iInputCsp;
    if (sConfig.ParamTranscode (sEncodingParam)) {
      return cmInitParaError;
    }
    if (sConfig.iSpatialLayerNum < 1) {
      return cmInitParaError;
    }
    iTargetWidth	= sConfig.iPicWidth;
    iTargetHeight	= sConfig.iPicHeight;
    if (m_pSrcPicList[0] == NULL) {
      return cmInitParaError;
    }
    if (m_iCspInternal != iInputColorspace || m_iMaxPicWidth != iTargetWidth
        || m_iMaxPicHeight != iTargetHeight) {	// for color space due to changed
      if(InitPic (m_pSrcPicList[0], iInputColorspace, iTargetWidth, iTargetHeight))
      {
        WelsLog (m_pEncContext, WELS_LOG_INFO,
                "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, iInputColorspace= 0x%x\n",
                 iInputColorspace);
          return cmInitParaError;
      }
      m_iMaxPicWidth	= iTargetWidth;
      m_iMaxPicHeight	= iTargetHeight;
      m_iCspInternal	= iInputColorspace;
    }
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n",
             m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT

    /* New configuration available here */
    sConfig.iInputCsp	= m_iCspInternal;	// I420 in default designed for presentation in encoder used internal
    sConfig.DetermineTemporalSettings();

    /* Check every field whether there is new request for memory block changed or else, Oct. 24, 2008 */
    WelsEncoderParamAdjust (&m_pEncContext, &sConfig);
  }
  break;
  case ENCODER_OPTION_FRAME_RATE: {	// Maximal input frame rate
    float iValue	= * ((float*)pOption);
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_FRAME_RATE, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x, iValue= %f\n",
             m_uiCountFrameNum, m_iCspInternal, iValue);
#endif//REC_FRAME_COUNT
    if (iValue<=0) {
      return cmInitParaError;
    }
    //adjust to valid range
    m_pEncContext->pSvcParam->fMaxFrameRate = WELS_CLIP3 (iValue, MIN_FRAME_RATE, MAX_FRAME_RATE);
    WelsEncoderApplyFrameRate (m_pEncContext->pSvcParam);
  }
  break;
  case ENCODER_OPTION_BITRATE: {	// Target bit-rate
    SBitrateInfo*pInfo = (static_cast<SBitrateInfo *>(pOption));
    int32_t iBitrate = pInfo->iBitrate;
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
      "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x, iValue= %d\n",
      m_uiCountFrameNum, m_iCspInternal, iValue);
#endif//REC_FRAME_COUNT
    if (iBitrate<=0) {
	  WelsLog (m_pEncContext, WELS_LOG_ERROR,"CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE,iBitrate = %d\n",iBitrate);
      return cmInitParaError;
    }
    iBitrate	= WELS_CLIP3 (iBitrate, MIN_BIT_RATE, MAX_BIT_RATE);
	switch(pInfo->iLayer){
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
	   WelsLog (m_pEncContext, WELS_LOG_ERROR,"CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE,iLayer = %d\n",pInfo->iLayer);
	return cmInitParaError;
	break;
	}
    //adjust to valid range
    WelsEncoderApplyBitRate (m_pEncContext->pSvcParam,pInfo->iLayer);
  }
  break;
  case ENCODER_OPTION_MAX_BITRATE: {	// Target bit-rate
    SBitrateInfo*pInfo = (static_cast<SBitrateInfo *>(pOption));
    int32_t iBitrate = pInfo->iBitrate;

#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
      "CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_BITRATE, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x, iValue= %d\n",
      m_uiCountFrameNum, m_iCspInternal, iValue);
#endif//REC_FRAME_COUNT
	if (iBitrate<=0) {
	  WelsLog (m_pEncContext, WELS_LOG_ERROR,"CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_MAX_BITRATE,iBitrate = %d\n",iBitrate);
	   return cmInitParaError;
	}
	iBitrate	= WELS_CLIP3 (iBitrate, MIN_BIT_RATE, MAX_BIT_RATE);
	switch(pInfo->iLayer){
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
	  WelsLog (m_pEncContext, WELS_LOG_ERROR,"CWelsH264SVCEncoder::SetOption():ENCODER_OPTION_MAX_BITRATE,iLayer = %d\n",pInfo->iLayer);
			return cmInitParaError;
	break;
	}
	  //adjust to valid range
	WelsEncoderApplyBitRate (m_pEncContext->pSvcParam,pInfo->iLayer);
  }
  break;
  case ENCODER_OPTION_RC_MODE: {	// 0:quality mode;1:bit-rate mode;2:bitrate limited mode
    int32_t iValue = * ((int32_t*)pOption);
    m_pEncContext->pSvcParam->iRCMode	= (RC_MODES) iValue;
  }
  break;
  case ENCODER_PADDING_PADDING: {	// 0:disable padding;1:padding
    int32_t iValue = * ((int32_t*)pOption);
    m_pEncContext->pSvcParam->iPaddingFlag	= iValue;
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
  case ENCOCER_LTR_MARKING_PERIOD: {
    uint32_t iValue = * ((uint32_t*) (pOption));
    m_pEncContext->pSvcParam->iLtrMarkPeriod = iValue;
  }
  break;
  case ENCODER_OPTION_LTR: {
    uint32_t iValue = * ((uint32_t*) (pOption));
    m_pEncContext->pSvcParam->bEnableLongTermReference = iValue ? true : false;
    WelsLog (m_pEncContext, WELS_LOG_WARNING, " CWelsH264SVCEncoder::SetOption enable LTR = %d",
             m_pEncContext->pSvcParam->bEnableLongTermReference);
  }
  break;
  case ENCODER_OPTION_ENABLE_SSEI: {
    bool iValue = * ((bool*)pOption);
    m_pEncContext->pSvcParam->bEnableSSEI = iValue;
    WelsLog (m_pEncContext, WELS_LOG_INFO, " CWelsH264SVCEncoder::SetOption enable SSEI = %d \n",
             m_pEncContext->pSvcParam->bEnableSSEI);
  }
  break;
  case ENCODER_OPTION_ENABLE_PREFIX_NAL_ADDING: {
    bool iValue = * ((bool*)pOption);
    m_pEncContext->pSvcParam->bPrefixNalAddingCtrl = iValue;
    WelsLog (m_pEncContext, WELS_LOG_INFO, " CWelsH264SVCEncoder::SetOption bPrefixNalAddingCtrl = %d \n",
             m_pEncContext->pSvcParam->bPrefixNalAddingCtrl);
  }
  break;
  case ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION: {
    bool iValue = * ((bool*)pOption);

    m_pEncContext->pSvcParam->bEnableSpsPpsIdAddition = iValue;
    WelsLog (m_pEncContext, WELS_LOG_INFO, " CWelsH264SVCEncoder::SetOption enable SPS/PPS ID = %d \n",
             m_pEncContext->pSvcParam->bEnableSpsPpsIdAddition);
  }
  break;
  case ENCODER_OPTION_CURRENT_PATH: {
    if (m_pEncContext->pSvcParam != NULL) {
      char* path = static_cast<char*> (pOption);
      m_pEncContext->pSvcParam->pCurPath = path;
    }
  }
  break;
  case ENCODER_OPTION_DUMP_FILE:{
#ifdef ENABLE_FRAME_DUMP
    if(m_pEncContext->pSvcParam!=NULL){
      SDumpLayer*pDump = (static_cast<SDumpLayer *>(pOption));
      WelsStrncpy(m_pEncContext->pSvcParam->sDependencyLayers[pDump->iLayer].sRecFileName, sizeof(m_pEncContext->pSvcParam->sDependencyLayers[pDump->iLayer].sRecFileName),pDump->pFileName);
    }
#endif
  }
  break;
  case ENCODER_OPTION_TRACE_LEVEL:{
    if(m_pWelsTrace){
	  uint32_t level = *((uint32_t*)pOption);
	  m_pWelsTrace->SetTraceLevel(level);
	}
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
    WelsLog (m_pEncContext, WELS_LOG_INFO, "ENCODER_OPTION_INTER_SPATIAL_PRED, this feature not supported at present.\n");
  }
  break;
  case ENCODER_OPTION_DATAFORMAT: {	// Input color space
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_DATAFORMAT, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n",
             m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT

    * ((int32_t*)pOption)	= m_iCspInternal;
  }
  break;
  case ENCODER_OPTION_IDR_INTERVAL: {	// IDR Interval
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_IDR_INTERVAL, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n",
             m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT
    * ((int32_t*)pOption) = m_pEncContext->pSvcParam->uiIntraPeriod;
  }
  break;
  case ENCODER_OPTION_SVC_ENCODE_PARAM_EXT: {	// SVC Encoding Parameter
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n",
             m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT
    memcpy (pOption, m_pEncContext->pSvcParam, sizeof (SEncParamExt));	// confirmed_safe_unsafe_usage
  }
  break;
  case ENCODER_OPTION_SVC_ENCODE_PARAM_BASE: {	// SVC Encoding Parameter
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_SVC_ENCODE_PARAM_BASE, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n",
             m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT
    m_pEncContext->pSvcParam->GetBaseParams((SEncParamBase*) pOption);
  }
  break;

  case ENCODER_OPTION_FRAME_RATE: {	// Maximal input frame rate
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_FRAME_RATE, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n",
             m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT
    * ((float*)pOption)	= m_pEncContext->pSvcParam->fMaxFrameRate;
  }
  break;
  case ENCODER_OPTION_BITRATE: {	// Target bit-rate
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,
             "CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_BITRATE, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n",
             m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT
    SBitrateInfo*pInfo = (static_cast<SBitrateInfo *>(pOption));
    if((pInfo->iLayer!=SPATIAL_LAYER_ALL)||(pInfo->iLayer!=SPATIAL_LAYER_0)||(pInfo->iLayer!=SPATIAL_LAYER_1)||(pInfo->iLayer!=SPATIAL_LAYER_2)||(pInfo->iLayer!=SPATIAL_LAYER_3))
        return cmInitParaError;
    if(pInfo->iLayer == SPATIAL_LAYER_ALL){
      pInfo->iBitrate = m_pEncContext->pSvcParam->iTargetBitrate;
    }else{
      pInfo->iBitrate = m_pEncContext->pSvcParam->sSpatialLayers[pInfo->iLayer].iSpatialBitrate;
	}
  }
  break;
  case ENCODER_OPTION_MAX_BITRATE: {	// Target bit-rate
#ifdef REC_FRAME_COUNT
    WelsLog (m_pEncContext, WELS_LOG_INFO,"CWelsH264SVCEncoder::GetOption():ENCODER_OPTION_MAX_BITRATE, m_uiCountFrameNum= %d, m_iCspInternal= 0x%x\n",
	          m_uiCountFrameNum, m_iCspInternal);
#endif//REC_FRAME_COUNT
    SBitrateInfo*pInfo = (static_cast<SBitrateInfo *>(pOption));
	if((pInfo->iLayer!=SPATIAL_LAYER_ALL)||(pInfo->iLayer!=SPATIAL_LAYER_0)||(pInfo->iLayer!=SPATIAL_LAYER_1)||(pInfo->iLayer!=SPATIAL_LAYER_2)||(pInfo->iLayer!=SPATIAL_LAYER_3))
	  return cmInitParaError;
	if(pInfo->iLayer == SPATIAL_LAYER_ALL){
      pInfo->iBitrate = m_pEncContext->pSvcParam->iMaxBitrate;
	}else{
	  pInfo->iBitrate = m_pEncContext->pSvcParam->sSpatialLayers[pInfo->iLayer].iMaxSpatialBitrate;
	}
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
    //				WelsLog( m_pEncContext, WELS_LOG_INFO, "WELS_CSP_I420, m_iCspInternal= 0x%x\n", m_iCspInternal);
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
    //				WelsLog( m_pEncContext, WELS_LOG_INFO, "WELS_CSP_BGR, m_iCspInternal= 0x%x\n", m_iCspInternal);
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
    WelsLog (m_pEncContext, WELS_LOG_INFO, "Exclusive case, m_iCspInternal= 0x%x\n", m_iCspInternal);
    break;
  }
#endif//DUMP_SRC_PICTURE
  return;
}
}

using namespace WelsSVCEnc;

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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
