/*!
 * \copy
 *     Copyright (c)  2009-2013, Cisco Systems
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
 *
 * \file	encoder.c
 *
 * \brief	core encoder
 *
 * \date	5/14/2009 Created
 *
 *************************************************************************************
 */
#include "encoder.h"
#include "cpu_core.h"

#include "decode_mb_aux.h"
#include "get_intra_predictor.h"

#include "deblocking.h"

#include "mc.h"
#include "sample.h"

#include "svc_base_layer_md.h"
#include "set_mb_syn_cavlc.h"
#include "crt_util_safe_x.h"	// Safe CRT routines like utils for cross_platforms
#ifdef MT_ENABLED
#include "slice_multi_threading.h"
#endif//MT_ENABLED

//  global   function  pointers  definition
namespace WelsSVCEnc {
/* Motion compensation */


/*!
 * \brief	initialize source picture body
 * \param	pSrc		SSourcePicture*
 * \param	csp		internal csp format
 * \param	iWidth	widht of picture in pixels
 * \param	iHeight	iHeight of picture in pixels
 * \return	successful - 0; otherwise none 0 for failed
 */
int32_t InitPic (const void* kpSrc, const int32_t kiColorspace, const int32_t kiWidth, const int32_t kiHeight) {
  SSourcePicture* pSrcPic = (SSourcePicture*)kpSrc;

  if (NULL == pSrcPic || kiWidth == 0 || kiHeight == 0)
    return 1;

  pSrcPic->iColorFormat	= kiColorspace;
  pSrcPic->iPicWidth		= kiWidth;
  pSrcPic->iPicHeight		= kiHeight;

  switch (kiColorspace & (~videoFormatVFlip)) {
  case videoFormatI420:
  case videoFormatYV12:
    pSrcPic->pData[0]	= NULL;
    pSrcPic->pData[1]	= NULL;
    pSrcPic->pData[2]	= NULL;
    pSrcPic->pData[3]	= NULL;
    pSrcPic->iStride[0]	= kiWidth;
    pSrcPic->iStride[2]	= pSrcPic->iStride[1] = kiWidth >> 1;
    pSrcPic->iStride[3]	= 0;
    break;
  case videoFormatYUY2:
  case videoFormatYVYU:
  case videoFormatUYVY:
    pSrcPic->pData[0]	= NULL;
    pSrcPic->pData[1]	= NULL;
    pSrcPic->pData[2]	= NULL;
    pSrcPic->pData[3]	= NULL;
    pSrcPic->iStride[0]	= CALC_BI_STRIDE (kiWidth,  16);
    pSrcPic->iStride[3]	= pSrcPic->iStride[2] = pSrcPic->iStride[1] = 0;
    break;
  case videoFormatRGB:
  case videoFormatBGR:
    pSrcPic->pData[0]	= NULL;
    pSrcPic->pData[1]	= NULL;
    pSrcPic->pData[2]	= NULL;
    pSrcPic->pData[3]	= NULL;
    pSrcPic->iStride[0]	= CALC_BI_STRIDE (kiWidth, 24);
    pSrcPic->iStride[3]	= pSrcPic->iStride[2] = pSrcPic->iStride[1] = 0;
    if (kiColorspace & videoFormatVFlip)
      pSrcPic->iColorFormat = kiColorspace & (~videoFormatVFlip);
    else
      pSrcPic->iColorFormat = kiColorspace | videoFormatVFlip;
    break;
  case videoFormatBGRA:
  case videoFormatRGBA:
  case videoFormatARGB:
  case videoFormatABGR:
    pSrcPic->pData[0]	= NULL;
    pSrcPic->pData[1]	= NULL;
    pSrcPic->pData[2]	= NULL;
    pSrcPic->pData[3]	= NULL;
    pSrcPic->iStride[0]	= kiWidth << 2;
    pSrcPic->iStride[3]	= pSrcPic->iStride[2] = pSrcPic->iStride[1] = 0;
    if (kiColorspace & videoFormatVFlip)
      pSrcPic->iColorFormat = kiColorspace & (~videoFormatVFlip);
    else
      pSrcPic->iColorFormat = kiColorspace | videoFormatVFlip;
    break;
  default:
    return 2;	// any else?
  }

  return 0;
}


void WelsInitBGDFunc (SWelsFuncPtrList* pFuncList, const bool_t kbEnableBackgroundDetection) {
  if (kbEnableBackgroundDetection) {
    pFuncList->pfInterMdBackgroundDecision = WelsMdInterJudgeBGDPskip;
    pFuncList->pfInterMdBackgroundInfoUpdate = WelsMdInterUpdateBGDInfo;
  } else {
    pFuncList->pfInterMdBackgroundDecision = WelsMdInterJudgeBGDPskipFalse;
    pFuncList->pfInterMdBackgroundInfoUpdate = WelsMdInterUpdateBGDInfoNULL;
  }
}

/*!
 * \brief	initialize function pointers that potentially used in Wels encoding
 * \param	pEncCtx		sWelsEncCtx*
 * \return	successful - 0; otherwise none 0 for failed
 */
int32_t InitFunctionPointers (SWelsFuncPtrList* pFuncList, SWelsSvcCodingParam* pParam, uint32_t uiCpuFlag) {
  int32_t iReturn = 0;

  /* Functionality utilization of CPU instructions dependency */
  pFuncList->pfSetMemZeroSize8	= WelsSetMemZero_c;		// confirmed_safe_unsafe_usage
  pFuncList->pfSetMemZeroSize64Aligned16	= WelsSetMemZero_c;	// confirmed_safe_unsafe_usage
  pFuncList->pfSetMemZeroSize64	= WelsSetMemZero_c;	// confirmed_safe_unsafe_usage
#if defined(X86_ASM)
  if (uiCpuFlag & WELS_CPU_MMXEXT) {
    pFuncList->pfSetMemZeroSize8	= WelsSetMemZeroSize8_mmx;		// confirmed_safe_unsafe_usage
    pFuncList->pfSetMemZeroSize64Aligned16	= WelsSetMemZeroSize64_mmx;	// confirmed_safe_unsafe_usage
    pFuncList->pfSetMemZeroSize64	= WelsSetMemZeroSize64_mmx;	// confirmed_safe_unsafe_usage
  }
  if (uiCpuFlag & WELS_CPU_SSE2) {
    pFuncList->pfSetMemZeroSize64Aligned16	= WelsSetMemZeroAligned64_sse2;	// confirmed_safe_unsafe_usage
  }
#endif//X86_ASM

  InitExpandPictureFunc (pFuncList, uiCpuFlag);

  /* Intra_Prediction_fn*/
  WelsInitFillingPredFuncs (uiCpuFlag);
  WelsInitIntraPredFuncs (pFuncList, uiCpuFlag);

  /* sad, satd, average */
  WelsInitSampleSadFunc (pFuncList, uiCpuFlag);

  //
  WelsInitBGDFunc (pFuncList, pParam->bEnableBackgroundDetection);
  // for pfGetVarianceFromIntraVaa function ptr adaptive by CPU features, 6/7/2010
  InitIntraAnalysisVaaInfo (pFuncList, uiCpuFlag);

  /* Motion compensation */
  /*init pixel average function*/
  /*get one column or row pixel when refinement*/
  WelsInitMcFuncs (pFuncList, uiCpuFlag);
  InitCoeffFunc (uiCpuFlag);

  WelsInitEncodingFuncs (pFuncList, uiCpuFlag);
  WelsInitReconstructionFuncs (pFuncList, uiCpuFlag);

  DeblockingInit (&pFuncList->pfDeblocking, uiCpuFlag);
  WelsBlockFuncInit (&pFuncList->pfSetNZCZero, uiCpuFlag);

  InitFillNeighborCacheInterFunc (pFuncList, pParam->bEnableBackgroundDetection);

  return iReturn;
}

/*!
 * \brief	initialize frame coding
 */
void InitFrameCoding (sWelsEncCtx* pEncCtx, const EFrameType keFrameType) {
  // for bitstream writing
  pEncCtx->iPosBsBuffer		= 0;	// reset bs pBuffer position
  pEncCtx->pOut->iNalIndex		= 0;	// reset NAL index

  InitBits (&pEncCtx->pOut->sBsWrite, pEncCtx->pOut->pBsBuffer, pEncCtx->pOut->uiSize);

  if (keFrameType == WELS_FRAME_TYPE_P) {
    if (pEncCtx->pSvcParam->uiIntraPeriod) {
      ++pEncCtx->iFrameIndex;
    }

    ++pEncCtx->uiFrameIdxRc;

    if (pEncCtx->iPOC < (1 << pEncCtx->pSps->iLog2MaxPocLsb) - 2)     // if iPOC type is no 0, this need be modification
      pEncCtx->iPOC			+= 2;	// for POC type 0
    else
      pEncCtx->iPOC = 0;

    if (pEncCtx->eLastNalPriority != 0) {
      if (pEncCtx->iFrameNum < (1 << pEncCtx->pSps->uiLog2MaxFrameNum) - 1)
        ++ pEncCtx->iFrameNum;
      else
        pEncCtx->iFrameNum	= 0;	// if iFrameNum overflow
    }
    pEncCtx->eNalType		= NAL_UNIT_CODED_SLICE;
    pEncCtx->eSliceType	= P_SLICE;
    pEncCtx->eNalPriority	= NRI_PRI_HIGH;
  } else if (keFrameType == WELS_FRAME_TYPE_IDR) {
    pEncCtx->iFrameNum		= 0;
    pEncCtx->iPOC			= 0;
    pEncCtx->bEncCurFrmAsIdrFlag = false;
    if (pEncCtx->pSvcParam->uiIntraPeriod) {
      pEncCtx->iFrameIndex = 0;
    }
    pEncCtx->uiFrameIdxRc = 0;

    pEncCtx->eNalType		= NAL_UNIT_CODED_SLICE_IDR;
    pEncCtx->eSliceType	= I_SLICE;
    pEncCtx->eNalPriority	= NRI_PRI_HIGHEST;

    pEncCtx->iCodingIndex	= 0;

    // reset_ref_list

    // rc_init_gop
  } else if (keFrameType == WELS_FRAME_TYPE_I) {
    if (pEncCtx->iPOC < (1 << pEncCtx->pSps->iLog2MaxPocLsb) - 2)     // if iPOC type is no 0, this need be modification
      pEncCtx->iPOC			+= 2;	// for POC type 0
    else
      pEncCtx->iPOC = 0;

    if (pEncCtx->eLastNalPriority != 0) {
      if (pEncCtx->iFrameNum < (1 << pEncCtx->pSps->uiLog2MaxFrameNum) - 1)
        ++ pEncCtx->iFrameNum;
      else
        pEncCtx->iFrameNum	= 0;	// if iFrameNum overflow
    }

    pEncCtx->eNalType		= NAL_UNIT_CODED_SLICE;
    pEncCtx->eSliceType	= I_SLICE;
    pEncCtx->eNalPriority	= NRI_PRI_HIGHEST;

    // rc_init_gop
  } else {	// B pictures are not supported now, any else?
    assert (0);
  }

#if defined(STAT_OUTPUT)
  memset (&pEncCtx->sPerInfo, 0, sizeof (SStatSliceInfo));
#endif//FRAME_INFO_OUTPUT

#if defined(MT_ENABLED) && defined(PACKING_ONE_SLICE_PER_LAYER)
  if (pEncCtx->pSvcParam->iMultipleThreadIdc > 1)
    reset_env_mt (pEncCtx);
#endif
}

EFrameType DecideFrameType (sWelsEncCtx* pEncCtx, const int8_t kiSpatialNum) {
  SWelsSvcCodingParam* pSvcParam	= pEncCtx->pSvcParam;
  EFrameType iFrameType = WELS_FRAME_TYPE_AUTO;
  bool_t bSceneChangeFlag = false;

  // perform scene change detection
  if ((!pSvcParam->bEnableSceneChangeDetect) || pEncCtx->pVaa->bIdrPeriodFlag ||
      (kiSpatialNum < pSvcParam->iNumDependencyLayer)
      || (pEncCtx->uiFrameIdxRc < (VGOP_SIZE << 1))) { // avoid too frequent I frame coding, rc control
    bSceneChangeFlag = false;
  } else {
    bSceneChangeFlag = pEncCtx->pVaa->bSceneChangeFlag;
  }

  //scene_changed_flag: RC enable && iSpatialNum == pSvcParam->iNumDependencyLayer
  //bIdrPeriodFlag: RC disable || iSpatialNum != pSvcParam->iNumDependencyLayer
  //pEncCtx->bEncCurFrmAsIdrFlag: 1. first frame should be IDR; 2. idr pause; 3. idr request
  iFrameType = (pEncCtx->pVaa->bIdrPeriodFlag || bSceneChangeFlag
                || pEncCtx->bEncCurFrmAsIdrFlag) ? WELS_FRAME_TYPE_IDR : WELS_FRAME_TYPE_P;

  if (WELS_FRAME_TYPE_P == iFrameType && pEncCtx->iSkipFrameFlag > 0) {  // for frame skip, 1/5/2010
    -- pEncCtx->iSkipFrameFlag;
    iFrameType = WELS_FRAME_TYPE_SKIP;
  } else if (WELS_FRAME_TYPE_IDR == iFrameType) {
    pEncCtx->iCodingIndex = 0;
  }

  return iFrameType;
}

/*!
 * \brief	Dump reconstruction for dependency layer
 */

extern "C" void DumpDependencyRec (SPicture* pCurPicture, const str_t* kpFileName, const int8_t kiDid) {
  WelsFileHandle* pDumpRecFile = NULL;
  static bool_t bDependencyRecFlag[MAX_DEPENDENCY_LAYER]	= {0};
  int32_t iWrittenSize											= 0;

  if (NULL == pCurPicture || NULL == kpFileName || kiDid >= MAX_DEPENDENCY_LAYER)
    return;

  if (bDependencyRecFlag[kiDid]) {
    if (strlen (kpFileName) > 0)	// confirmed_safe_unsafe_usage
      pDumpRecFile = WelsFopen (kpFileName, "ab");
    else {
      str_t sDependencyRecFileName[16] = {0};
      WelsSnprintf (sDependencyRecFileName, 16, "rec%d.yuv", kiDid);	// confirmed_safe_unsafe_usage
      pDumpRecFile	= WelsFopen (sDependencyRecFileName, "ab");
    }
    if (NULL != pDumpRecFile)
      WelsFseek (pDumpRecFile, 0, SEEK_END);
  } else {
    if (strlen (kpFileName) > 0) {	// confirmed_safe_unsafe_usage
      pDumpRecFile	= WelsFopen (kpFileName, "wb");
    } else {
      str_t sDependencyRecFileName[16] = {0};
      WelsSnprintf (sDependencyRecFileName, 16, "rec%d.yuv", kiDid);	// confirmed_safe_unsafe_usage
      pDumpRecFile	= WelsFopen (sDependencyRecFileName, "wb");
    }
    bDependencyRecFlag[kiDid]	= true;
  }

  if (NULL != pDumpRecFile) {
    int32_t i = 0;
    int32_t j = 0;
    const int32_t kiStrideY	= pCurPicture->iLineSize[0];
    const int32_t kiLumaWidth	= pCurPicture->iWidthInPixel;
    const int32_t kiLumaHeight	= pCurPicture->iHeightInPixel;
    const int32_t kiChromaWidth	= kiLumaWidth >> 1;
    const int32_t kiChromaHeight	= kiLumaHeight >> 1;

    for (j = 0; j < kiLumaHeight; ++ j) {
      iWrittenSize = WelsFwrite (&pCurPicture->pData[0][j * kiStrideY], 1, kiLumaWidth, pDumpRecFile);
      assert (iWrittenSize == kiLumaWidth);
      if (iWrittenSize < kiLumaWidth) {
        assert (0);	// make no sense for us if writing failed
        WelsFclose (pDumpRecFile);
        return;
      }
    }
    for (i = 1; i < I420_PLANES; ++ i) {
      const int32_t kiStrideUV = pCurPicture->iLineSize[i];
      for (j = 0; j < kiChromaHeight; ++ j) {
        iWrittenSize = WelsFwrite (&pCurPicture->pData[i][j * kiStrideUV], 1, kiChromaWidth, pDumpRecFile);
        assert (iWrittenSize == kiChromaWidth);
        if (iWrittenSize < kiChromaWidth) {
          assert (0);	// make no sense for us if writing failed
          WelsFclose (pDumpRecFile);
          return;
        }
      }
    }
    WelsFclose (pDumpRecFile);
    pDumpRecFile = NULL;
  }
}

/*!
 * \brief	Dump the reconstruction pictures
 */

void DumpRecFrame (SPicture* pCurPicture, const str_t* kpFileName) {
  WelsFileHandle* pDumpRecFile				= NULL;
  static bool_t bRecFlag	= false;
  int32_t iWrittenSize			= 0;

  if (NULL == pCurPicture || NULL == kpFileName)
    return;

  if (bRecFlag) {
    if (strlen (kpFileName) > 0) {	// confirmed_safe_unsafe_usage
      pDumpRecFile	= WelsFopen (kpFileName, "ab");
    } else {
      pDumpRecFile	= WelsFopen ("rec.yuv", "ab");
    }
    if (NULL != pDumpRecFile)
      WelsFseek (pDumpRecFile, 0, SEEK_END);
  } else {
    if (strlen (kpFileName) > 0) {	// confirmed_safe_unsafe_usage
      pDumpRecFile	= WelsFopen (kpFileName, "wb");
    } else {
      pDumpRecFile	= WelsFopen ("rec.yuv", "wb");
    }
    bRecFlag	= true;
  }

  if (NULL != pDumpRecFile) {
    int32_t i = 0;
    int32_t j = 0;
    const int32_t kiStrideY	= pCurPicture->iLineSize[0];
    const int32_t kiLumaWidth	= pCurPicture->iWidthInPixel;
    const int32_t kiLumaHeight	= pCurPicture->iHeightInPixel;
    const int32_t kiChromaWidth	= kiLumaWidth >> 1;
    const int32_t kiChromaHeight	= kiLumaHeight >> 1;

    for (j = 0; j < kiLumaHeight; ++ j) {
      iWrittenSize = WelsFwrite (&pCurPicture->pData[0][j * kiStrideY], 1, kiLumaWidth, pDumpRecFile);
      assert (iWrittenSize == kiLumaWidth);
      if (iWrittenSize < kiLumaWidth) {
        assert (0);	// make no sense for us if writing failed
        WelsFclose (pDumpRecFile);
        return;
      }
    }
    for (i = 1; i < I420_PLANES; ++ i) {
      const int32_t kiStrideUV = pCurPicture->iLineSize[i];
      for (j = 0; j < kiChromaHeight; ++ j) {
        iWrittenSize = WelsFwrite (&pCurPicture->pData[i][j * kiStrideUV], 1, kiChromaWidth, pDumpRecFile);
        assert (iWrittenSize == kiChromaWidth);
        if (iWrittenSize < kiChromaWidth) {
          assert (0);	// make no sense for us if writing failed
          WelsFclose (pDumpRecFile);
          return;
        }
      }
    }
    WelsFclose (pDumpRecFile);
    pDumpRecFile = NULL;
  }
}



/***********************************************************************************/
void WelsSetMemZero_c (void* pDst, int32_t iSize) {	// confirmed_safe_unsafe_usage
  memset (pDst, 0, iSize);
}
}
