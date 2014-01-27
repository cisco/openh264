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
 * \file	encoder_ext.c
 *
 * \brief	core encoder for SVC
 *
 * \date	7/24/2009 Created
 *
 *************************************************************************************
 */

#include "encoder.h"
#include "cpu.h"
#include "utils.h"
#include "svc_enc_golomb.h"
#include "au_set.h"
#include "picture_handle.h"
#include "svc_base_layer_md.h"
#include "svc_encode_slice.h"
#include "decode_mb_aux.h"
#include "deblocking.h"
#include "ref_list_mgr_svc.h"
#include "ls_defines.h"
#include "crt_util_safe_x.h"	// Safe CRT routines like utils for cross platforms
#include "array_stack_align.h"
#if defined(MT_ENABLED)
#include "slice_multi_threading.h"
#endif//MT_ENABLED
#if defined(DYNAMIC_SLICE_ASSIGN) || defined(MT_DEBUG)
#include "measure_time.h"
#endif//DYNAMIC_SLICE_ASSIGN

namespace WelsSVCEnc {


int32_t WelsCodeOnePicPartition (sWelsEncCtx* pCtx,
                                 SLayerBSInfo* pLbi,
                                 int32_t* pNalIdxInLayer,
                                 int32_t* pLayerSize,
                                 int32_t iFirstMbInPartition,	// first mb inclusive in partition
                                 int32_t iEndMbInPartition,	// end mb exclusive in partition
                                 int32_t iStartSliceIdx
                                );


/*!
 * \brief	validate checking in parameter configuration
 * \pParam	pParam		SWelsSvcCodingParam*
 * \return	successful - 0; otherwise none 0 for failed
 */
int32_t ParamValidation (SWelsSvcCodingParam* pCfg) {
  float fMaxFrameRate = 0.0f;
  const float fEpsn = 0.000001f;
  int32_t i = 0;
  int32_t iLastSpatialWidth	= 0;
  int32_t	iLastSpatialHeight	= 0;
  float fLastFrameRateIn	= 0.0f;
  float fLastFrameRateOut	= 0.0f;
  SDLayerParam* pLastSpatialParam = NULL;

  assert (pCfg != NULL);

  for (i = 0; i < pCfg->iNumDependencyLayer; ++ i) {
    SDLayerParam* fDlp = &pCfg->sDependencyLayers[i];
    if (fDlp->fOutputFrameRate > fDlp->fInputFrameRate || (fDlp->fInputFrameRate >= -fEpsn
        && fDlp->fInputFrameRate <= fEpsn)
        || (fDlp->fOutputFrameRate >= -fEpsn && fDlp->fOutputFrameRate <= fEpsn)) {
#if defined (_DEBUG)
      fprintf (stderr, "Invalid settings in input frame rate(%.6f) or output frame rate(%.6f) of layer #%d config file..\n",
               fDlp->fInputFrameRate, fDlp->fOutputFrameRate, i);
#endif
      return 1;
    }
    if (UINT_MAX == GetLogFactor (fDlp->fOutputFrameRate, fDlp->fInputFrameRate)) {
#if defined (_DEBUG)
      fprintf (stderr,
               "Invalid settings in input frame rate(%.6f) and output frame rate(%.6f) of layer #%d config file: iResult of output frame rate divided by input frame rate should be power of 2(i.e,in/pOut=2^n)..\n",
               fDlp->fInputFrameRate, fDlp->fOutputFrameRate, i);
#endif
      return 1;
    }
  }

  for (i = 0; i < pCfg->iNumDependencyLayer; ++ i) {
    SDLayerParam* fDlp = &pCfg->sDependencyLayers[i];
    if (fDlp->fInputFrameRate > fMaxFrameRate)
      fMaxFrameRate	= fDlp->fInputFrameRate;
  }

  if (fMaxFrameRate > fEpsn && (fMaxFrameRate - pCfg->fMaxFrameRate > fEpsn
                                || fMaxFrameRate - pCfg->fMaxFrameRate < -fEpsn)) {
    pCfg->fMaxFrameRate	= fMaxFrameRate;
  }

  for (i = 0; i < pCfg->iNumDependencyLayer; ++ i) {
    SDLayerParam* fDlp = &pCfg->sDependencyLayers[i];

    pLastSpatialParam	= fDlp;
    iLastSpatialWidth	= fDlp->iFrameWidth;
    iLastSpatialHeight	= fDlp->iFrameHeight;
    fLastFrameRateIn	= fDlp->fInputFrameRate;
    fLastFrameRateOut	= fDlp->fOutputFrameRate;
  }

  return 0;
}

int32_t ParamValidationExt (void* pParam) {
  SWelsSvcCodingParam* pCodingParam = (SWelsSvcCodingParam*)pParam;
  int8_t i = 0;
  int32_t iIdx = 0;

  assert (pCodingParam != NULL);
  if (NULL == pCodingParam)
    return 1;

  if (pCodingParam->iNumDependencyLayer < 1 || pCodingParam->iNumDependencyLayer > MAX_DEPENDENCY_LAYER) {
#if defined (_DEBUG)
    fprintf (stderr, "ParamValidationExt(), monitor invalid pCodingParam->iNumDependencyLayer: %d!\n",
             pCodingParam->iNumDependencyLayer);
#endif//#if _DEBUG

    return 1;
  }

  if (pCodingParam->iNumTemporalLayer < 1 || pCodingParam->iNumTemporalLayer > MAX_TEMPORAL_LEVEL) {
#if defined (_DEBUG)
    fprintf (stderr, "ParamValidationExt(), monitor invalid pCodingParam->iNumTemporalLayer: %d!\n",
             pCodingParam->iNumTemporalLayer);
#endif//#if _DEBUG
    return 1;
  }

  if (pCodingParam->uiGopSize < 1 || pCodingParam->uiGopSize > MAX_GOP_SIZE) {
#if defined (_DEBUG)
    fprintf (stderr, "ParamValidationExt(), monitor invalid pCodingParam->uiGopSize: %d!\n", pCodingParam->uiGopSize);
#endif//#if _DEBUG
    return 1;
  }


  if (pCodingParam->uiIntraPeriod && pCodingParam->uiIntraPeriod < pCodingParam->uiGopSize) {
#if defined (_DEBUG)
    fprintf (stderr,
             "ParamValidationExt(), uiIntraPeriod(%d) should be not less than that of uiGopSize(%d) or -1 specified!\n",
             pCodingParam->uiIntraPeriod, pCodingParam->uiGopSize);
#endif//#if _DEBUG
    return 1;
  }

  if (pCodingParam->uiIntraPeriod && (pCodingParam->uiIntraPeriod & (pCodingParam->uiGopSize - 1)) != 0) {
#if defined (_DEBUG)
    fprintf (stderr, "ParamValidationExt(), uiIntraPeriod(%d) should be multiple of uiGopSize(%d) or -1 specified!\n",
             pCodingParam->uiIntraPeriod, pCodingParam->uiGopSize);
#endif//#if _DEBUG
    return 1;
  }


#ifdef MT_ENABLED
  //about iMultipleThreadIdc, bDeblockingParallelFlag, iLoopFilterDisableIdc, & uiSliceMode
  // (1) Single Thread
  //	if (THREAD==1)//single thread
  //		no parallel_deblocking: bDeblockingParallelFlag = 0;
  // (2) Multi Thread: see uiSliceMode decision
  if (pCodingParam->iMultipleThreadIdc == 1) {
    //now is single thread. no parallel deblocking, set flag=0
    pCodingParam->bDeblockingParallelFlag = false;
  } else {
    pCodingParam->bDeblockingParallelFlag = true;
  }
#else
  pCodingParam->bDeblockingParallelFlag	= false;
#endif//MT_ENABLED

  for (i = 0; i < pCodingParam->iNumDependencyLayer; ++ i) {
    SDLayerParam* fDlp = &pCodingParam->sDependencyLayers[i];
    const int32_t kiPicWidth = fDlp->iFrameWidth;
    const int32_t kiPicHeight = fDlp->iFrameHeight;
    int32_t iMbWidth		= 0;
    int32_t iMbHeight		= 0;
    int32_t iMbNumInFrame		= 0;
    int32_t iMaxSliceNum		= MAX_SLICES_NUM;
    if (kiPicWidth <= 0 || kiPicHeight <= 0) {
#if defined (_DEBUG)
      fprintf (stderr, "ParamValidationExt(), invalid %d x %d in dependency layer settings!\n", kiPicWidth, kiPicHeight);
#endif//#if _DEBUG
      return 1;
    }
    if ((kiPicWidth & 0x0F) != 0 || (kiPicHeight & 0x0F) != 0) {
#if defined (_DEBUG)
      fprintf (stderr,
               "ParamValidationExt(), in layer #%d iWidth x iHeight(%d x %d) both should be multiple of 16, can not support with arbitrary size currently!\n",
               i, kiPicWidth, kiPicHeight);
#endif//#if _DEBUG
      return 1;
    }

    if (fDlp->sMso.uiSliceMode >= SM_RESERVED) {
#if defined (_DEBUG)
      fprintf (stderr, "ParamValidationExt(), invalid uiSliceMode (%d) settings!\n", fDlp->sMso.uiSliceMode);
#endif//#if _DEBUG
      return 1;
    }

    //check pSlice settings under multi-pSlice
    if (kiPicWidth <= 16 && kiPicHeight <= 16) {
      //only have one MB, set to single_slice
      fDlp->sMso.uiSliceMode = SM_SINGLE_SLICE;
    }
    switch (fDlp->sMso.uiSliceMode) {
    case SM_SINGLE_SLICE:
      fDlp->sMso.sSliceArgument.iSliceNum = 1;
      fDlp->sMso.sSliceArgument.uiSliceSizeConstraint = 0;
      fDlp->sMso.sSliceArgument.iSliceNum = 0;
      for (iIdx = 0; iIdx < MAX_SLICES_NUM; iIdx++) {
        fDlp->sMso.sSliceArgument.uiSliceMbNum[iIdx] = 0;
      }
      break;
    case SM_FIXEDSLCNUM_SLICE: {
      fDlp->sMso.sSliceArgument.uiSliceSizeConstraint = 0;

      iMbWidth	= (kiPicWidth + 15) >> 4;
      iMbHeight	= (kiPicHeight + 15) >> 4;
      iMbNumInFrame = iMbWidth * iMbHeight;
      iMaxSliceNum = MAX_SLICES_NUM;
      if (fDlp->sMso.sSliceArgument.iSliceNum <= 0
          || fDlp->sMso.sSliceArgument.iSliceNum > iMaxSliceNum) {
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), invalid uiSliceNum (%d) settings!\n", fDlp->sMso.sSliceArgument.iSliceNum);
#endif//#if _DEBUG
        return 1;
      }
      if (fDlp->sMso.sSliceArgument.iSliceNum == 1) {
#if defined (_DEBUG)
        fprintf (stderr,
                 "ParamValidationExt(), uiSliceNum(%d) you set for SM_FIXEDSLCNUM_SLICE, now turn to SM_SINGLE_SLICE type!\n",
                 fDlp->sMso.sSliceArgument.iSliceNum);
#endif//#if _DEBUG
        fDlp->sMso.uiSliceMode	= SM_SINGLE_SLICE;
        break;
      }
      if (pCodingParam->bEnableRc) {	// multiple slices verify with gom
        //check uiSliceNum
        GomValidCheckSliceNum (iMbWidth, iMbHeight, (int32_t*)&fDlp->sMso.sSliceArgument.iSliceNum);
        assert (fDlp->sMso.sSliceArgument.iSliceNum > 1);
        //set uiSliceMbNum with current uiSliceNum
        GomValidCheckSliceMbNum (iMbWidth, iMbHeight, &fDlp->sMso.sSliceArgument);
      } else if (!CheckFixedSliceNumMultiSliceSetting (iMbNumInFrame,
                 &fDlp->sMso.sSliceArgument)) {	// verify interleave mode settings
        //check uiSliceMbNum with current uiSliceNum
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), invalid uiSliceMbNum (%d) settings!\n",
                 fDlp->sMso.sSliceArgument.uiSliceMbNum[0]);
#endif//#if _DEBUG
        return 1;
      }
      // considering the coding efficient and performance, iCountMbNum constraint by MIN_NUM_MB_PER_SLICE condition of multi-pSlice mode settting
      if (iMbNumInFrame <= MIN_NUM_MB_PER_SLICE) {
        fDlp->sMso.uiSliceMode	= SM_SINGLE_SLICE;
        fDlp->sMso.sSliceArgument.iSliceNum	= 1;
        break;
      }
    }
    break;
    case SM_RASTER_SLICE: {
      fDlp->sMso.sSliceArgument.uiSliceSizeConstraint = 0;

      iMbWidth	= (kiPicWidth + 15) >> 4;
      iMbHeight	= (kiPicHeight + 15) >> 4;
      iMbNumInFrame = iMbWidth * iMbHeight;
      iMaxSliceNum = MAX_SLICES_NUM;
      if (fDlp->sMso.sSliceArgument.uiSliceMbNum[0] <= 0) {
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), invalid uiSliceMbNum (%d) settings!\n",
                 fDlp->sMso.sSliceArgument.uiSliceMbNum[0]);
#endif//#if _DEBUG
        return 1;
      }

      if (!CheckRasterMultiSliceSetting (iMbNumInFrame, &fDlp->sMso.sSliceArgument)) {	// verify interleave mode settings
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), invalid uiSliceMbNum (%d) settings!\n",
                 fDlp->sMso.sSliceArgument.uiSliceMbNum[0]);
#endif//#if _DEBUG
        return 1;
      }
      if (fDlp->sMso.sSliceArgument.iSliceNum <= 0
          || fDlp->sMso.sSliceArgument.iSliceNum > iMaxSliceNum) {	// verify interleave mode settings
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), invalid uiSliceNum (%d) in SM_RASTER_SLICE settings!\n",
                 fDlp->sMso.sSliceArgument.iSliceNum);
#endif//#if _DEBUG
        return 1;
      }
      if (fDlp->sMso.sSliceArgument.iSliceNum == 1) {
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), pSlice setting for SM_RASTER_SLICE now turn to SM_SINGLE_SLICE!\n");
#endif//#if _DEBUG
        fDlp->sMso.uiSliceMode	= SM_SINGLE_SLICE;
        break;
      }
#ifdef MT_ENABLED
      if (pCodingParam->bEnableRc && fDlp->sMso.sSliceArgument.iSliceNum > 1) {
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), WARNING: GOM based RC do not support SM_RASTER_SLICE!\n");
#endif//#if _DEBUG
      }
#endif
      // considering the coding efficient and performance, iCountMbNum constraint by MIN_NUM_MB_PER_SLICE condition of multi-pSlice mode settting
      if (iMbNumInFrame <= MIN_NUM_MB_PER_SLICE) {
        fDlp->sMso.uiSliceMode	= SM_SINGLE_SLICE;
        fDlp->sMso.sSliceArgument.iSliceNum	= 1;
        break;
      }
    }
    break;
    case SM_ROWMB_SLICE: {
      fDlp->sMso.sSliceArgument.uiSliceSizeConstraint = 0;

      iMbWidth	= (kiPicWidth + 15) >> 4;
      iMbHeight	= (kiPicHeight + 15) >> 4;
      iMaxSliceNum = MAX_SLICES_NUM;
      if (iMbHeight > iMaxSliceNum) {
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), invalid uiSliceNum (%d) settings more than MAX!\n", iMbHeight);
#endif//#if _DEBUG
        return 1;
      }
      fDlp->sMso.sSliceArgument.iSliceNum	= iMbHeight;

      if (fDlp->sMso.sSliceArgument.iSliceNum <= 0) {
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), invalid uiSliceNum (%d) settings!\n", fDlp->sMso.sSliceArgument.iSliceNum);
#endif//#if _DEBUG
        return 1;
      }
      if (!CheckRowMbMultiSliceSetting (iMbWidth, &fDlp->sMso.sSliceArgument)) {	// verify interleave mode settings
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), invalid uiSliceMbNum (%d) settings!\n",
                 fDlp->sMso.sSliceArgument.uiSliceMbNum[0]);
#endif//#if _DEBUG
        return 1;
      }
    }
    break;
    case SM_DYN_SLICE: {
      iMbWidth	= (kiPicWidth + 15) >> 4;
      iMbHeight	= (kiPicHeight + 15) >> 4;
      if (fDlp->sMso.sSliceArgument.uiSliceSizeConstraint <= 0) {
#if defined (_DEBUG)
        fprintf (stderr, "ParamValidationExt(), invalid iSliceSize (%d) settings!\n",
                 fDlp->sMso.sSliceArgument.uiSliceSizeConstraint);
#endif//#if _DEBUG
        return 1;
      }
      // considering the coding efficient and performance, iCountMbNum constraint by MIN_NUM_MB_PER_SLICE condition of multi-pSlice mode settting
      if (iMbWidth * iMbHeight <= MIN_NUM_MB_PER_SLICE) {
        fDlp->sMso.uiSliceMode	= SM_SINGLE_SLICE;
        fDlp->sMso.sSliceArgument.iSliceNum	= 1;
        break;
      }
    }
    break;
    default: {

#if defined (_DEBUG)
      fprintf (stderr, "ParamValidationExt(), invalid uiSliceMode (%d) settings!\n",
               pCodingParam->sDependencyLayers[0].sMso.uiSliceMode);
#endif//#if _DEBUG
      return 1;

    }
    break;
    }
  }

  return ParamValidation (pCodingParam);
}


void WelsEncoderApplyFrameRate(SWelsSvcCodingParam* pParam)
{
  SDLayerParam* pLayerParam;
  const float kfEpsn = 0.000001f;
  const int32_t kiNumLayer = pParam->iNumDependencyLayer;
  int32_t i;
  const float kfMaxFrameRate = pParam->fMaxFrameRate;
  float fRatio;
  float fTargetOutputFrameRate;

  //set input frame rate to each layer
  for (i=0;i<kiNumLayer;i++) {
    pLayerParam = &(pParam->sDependencyLayers[i]);

    fRatio = pLayerParam->fOutputFrameRate / pLayerParam->fInputFrameRate;
    if ( (kfMaxFrameRate - pLayerParam->fInputFrameRate) > kfEpsn
        || (kfMaxFrameRate - pLayerParam->fInputFrameRate) < -kfEpsn ) {
      pLayerParam->fInputFrameRate = kfMaxFrameRate;
      fTargetOutputFrameRate = kfMaxFrameRate*fRatio;
      pLayerParam->fOutputFrameRate = (fTargetOutputFrameRate>=6)?fTargetOutputFrameRate:(pLayerParam->fInputFrameRate);
      //TODO:{Sijia} from design, there is no sense to have temporal layer when under 6fps even with such setting?
    }
  }
}


void WelsEncoderApplyBitRate(SWelsSvcCodingParam* pParam)
{
  //TODO (Sijia):  this is a temporary solution which keep the ratio between layers
  //but it is also possible to fulfill the bitrate of lower layer first

  SDLayerParam* pLayerParam;
  const int32_t iNumLayers = pParam->iNumDependencyLayer;
  int32_t i, iOrigTotalBitrate=0;
  //read old BR
  for (i=0;i<iNumLayers;i++) {
    iOrigTotalBitrate += pParam->sDependencyLayers[i].iSpatialBitrate;
  }
  //write new BR
  float fRatio = 0.0;
  for (i=0;i<iNumLayers;i++) {
    pLayerParam = &(pParam->sDependencyLayers[i]);
    fRatio = pLayerParam->iSpatialBitrate/(static_cast<float>(iOrigTotalBitrate));
    pLayerParam->iSpatialBitrate = static_cast<int32_t>(pParam->iTargetBitrate*fRatio);
  }
}
/*!
 * \brief	acquire count number of layers and NALs based on configurable paramters dependency
 * \pParam	pCtx				sWelsEncCtx*
 * \pParam	pParam			SWelsSvcCodingParam*
 * \pParam	pCountLayers	pointer of count number of layers indeed
 * \pParam	iCountNals		pointer of count number of nals indeed
 * \return	0 - successful; otherwise failed
 */
static inline int32_t AcquireLayersNals (sWelsEncCtx** ppCtx, SWelsSvcCodingParam* pParam, int32_t* pCountLayers,
    int32_t* pCountNals) {
  int32_t iCountNumLayers		= 0;
  int32_t iCountNumNals			= 0;
  int32_t iNumDependencyLayers	= 0;
  int32_t iDIndex 				= 0;
#if defined(MT_ENABLED) && defined(PACKING_ONE_SLICE_PER_LAYER)
  int32_t iNumLayersPack = 0;
#endif//MT_ENABLED && PACKING_ONE_SLICE_PER_LAYER

  if (NULL == pParam || NULL == ppCtx || NULL == *ppCtx)
    return 1;

  iNumDependencyLayers	= pParam->iNumDependencyLayer;

  do {
    SDLayerParam* pDLayer = &pParam->sDependencyLayers[iDIndex];
//		pDLayer->ptr_cfg = pParam;
    int32_t iOrgNumNals = iCountNumNals;

    //Note: Sep. 2010
    //Review this part and suggest no change, since the memory over-use
    //(1) counts little to the overall performance
    //(2) should not be critial even under mobile case
    if (SM_DYN_SLICE == pDLayer->sMso.uiSliceMode) {
      iCountNumNals += MAX_SLICES_NUM;
      // plus prefix NALs
      if (iDIndex == 0)
        iCountNumNals += MAX_SLICES_NUM;
      // MAX_SLICES_NUM < MAX_LAYER_NUM_OF_FRAME ensured at svc_enc_slice_segment.h
#if defined(MT_ENABLED) && defined(PACKING_ONE_SLICE_PER_LAYER)
      assert (MAX_SLICES_NUM < MAX_LAYER_NUM_OF_FRAME);
      // iNumLayersPack += MAX_SLICES_NUM; // do not count it for dynamic slicing mode
#else//!MT_ENABLED || !PACKING_ONE_SLICE_PER_LAYER
      assert (iCountNumNals - iOrgNumNals <= MAX_NAL_UNITS_IN_LAYER);
#endif//MT_ENABLED && PACKING_ONE_SLICE_PER_LAYER
    } else { /*if ( SM_SINGLE_SLICE != pDLayer->sMso.uiSliceMode )*/
      const int32_t kiNumOfSlice = GetInitialSliceNum ((pDLayer->iFrameWidth + 0x0f) >> 4,
                                   (pDLayer->iFrameHeight + 0x0f) >> 4,
                                   &pDLayer->sMso);

      // NEED check iCountNals value in case multiple slices is used
      iCountNumNals += kiNumOfSlice; // for pSlice VCL NALs
      // plus prefix NALs
      if (iDIndex == 0)
        iCountNumNals += kiNumOfSlice;
#if defined(MT_ENABLED) && defined(PACKING_ONE_SLICE_PER_LAYER)
      assert (num_of_slice <= MAX_SLICES_NUM && MAX_SLICES_NUM < MAX_LAYER_NUM_OF_FRAME);
      iNumLayersPack += num_of_slice;
#else//!MT_ENABLED || !PACKING_ONE_SLICE_PER_LAYER
      assert (iCountNumNals - iOrgNumNals <= MAX_NAL_UNITS_IN_LAYER);
#endif//MT_ENALBED && PACKING_ONE_SLICE_PER_LAYER
      if (kiNumOfSlice > MAX_SLICES_NUM) {
        WelsLog (*ppCtx, WELS_LOG_ERROR,
                 "AcquireLayersNals(), num_of_slice(%d) > MAX_SLICES_NUM(%d) per (iDid= %d, qid= %d) settings!\n",
                 kiNumOfSlice, MAX_SLICES_NUM, iDIndex, 0);
        return 1;
      }
    }
#if !defined(MT_ENABLED) || !defined(PACKING_ONE_SLICE_PER_LAYER)
    if (iCountNumNals - iOrgNumNals > MAX_NAL_UNITS_IN_LAYER) {
      WelsLog (*ppCtx, WELS_LOG_ERROR,
               "AcquireLayersNals(), num_of_nals(%d) > MAX_NAL_UNITS_IN_LAYER(%d) per (iDid= %d, qid= %d) settings!\n",
               (iCountNumNals - iOrgNumNals), MAX_NAL_UNITS_IN_LAYER, iDIndex, 0);
      return 1;
    }
#endif//!MT_ENABLED) || !PACKING_ONE_SLICE_PER_LAYER

    iCountNumLayers ++;

    ++ iDIndex;
  } while (iDIndex < iNumDependencyLayers);

  iCountNumNals += 1 + iNumDependencyLayers + (iCountNumLayers << 1) +
                   iCountNumLayers;	// plus iCountNumLayers for reserved application
#if defined(MT_ENABLED) && defined(PACKING_ONE_SLICE_PER_LAYER)
  iNumLayersPack += 1 + iNumDependencyLayers + (iCountNumLayers << 1);
#endif//MT_ENABLED && PACKING_ONE_SLICE_PER_LAYER

  // to check number of layers / nals / slices dependencies, 12/8/2010
#if !defined(MT_ENABLED)
  if (iCountNumLayers > MAX_LAYER_NUM_OF_FRAME) {
    WelsLog (*ppCtx, WELS_LOG_ERROR, "AcquireLayersNals(), iCountNumLayers(%d) > MAX_LAYER_NUM_OF_FRAME(%d)!",
             iCountNumLayers, MAX_LAYER_NUM_OF_FRAME);
    return 1;
  }
#else//MT_ENABLED
#if defined(PACKING_ONE_SLICE_PER_LAYER)
  if (iNumLayersPack > MAX_LAYER_NUM_OF_FRAME) {
    WelsLog (*ppCtx, WELS_LOG_ERROR, "AcquireLayersNals(), num_layers_pack_overall(%d) > MAX_LAYER_NUM_OF_FRAME(%d)!",
             iNumLayersPack, MAX_LAYER_NUM_OF_FRAME);
    return 1;
  }
#else//!PACKING_ONE_SLICE_PER_LAYER
  if (iCountNumLayers > MAX_LAYER_NUM_OF_FRAME) {
    WelsLog (*ppCtx, WELS_LOG_ERROR, "AcquireLayersNals(), iCountNumLayers(%d) > MAX_LAYER_NUM_OF_FRAME(%d)!",
             iCountNumLayers, MAX_LAYER_NUM_OF_FRAME);
    return 1;
  }
#endif//PACKING_ONE_SLICE_PER_LAYER
#endif//!MT_ENABLED

  if (NULL != pCountLayers)
    *pCountLayers	= iCountNumLayers;
  if (NULL != pCountNals)
    *pCountNals 	= iCountNumNals;
  return 0;
}

/*!
 * \brief	alloc spatial layers pictures (I420 based source pictures)
 */
int32_t AllocSpatialPictures (sWelsEncCtx** ppCtx, SWelsSvcCodingParam* pParam) {
  CMemoryAlign* pMa						= (*ppCtx)->pMemAlign;
  const int32_t kiDlayerCount					= pParam->iNumDependencyLayer;
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
      WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pPic), FreeMemorySvc (ppCtx); *ppCtx = NULL)
      (*ppCtx)->pSpatialPic[iDlayerIndex][i] = pPic;
      ++ i;
    } while (i < kuiRefNumInTemporal);

    (*ppCtx)->uiSpatialLayersInTemporal[iDlayerIndex] = kuiLayerInTemporal;
    (*ppCtx)->uiSpatialPicNum[iDlayerIndex] = kuiRefNumInTemporal;
    ++ iDlayerIndex;
  } while (iDlayerIndex < kiDlayerCount);

  return 0;
}

void FreeSpatialPictures (sWelsEncCtx* pCtx) {
  CMemoryAlign* pMa	= pCtx->pMemAlign;
  int32_t j = 0;
  while (j < pCtx->pSvcParam->iNumDependencyLayer) {
    uint8_t i = 0;
    uint8_t uiRefNumInTemporal = pCtx->uiSpatialPicNum[j];

    while (i < uiRefNumInTemporal) {
      if (NULL != pCtx->pSpatialPic[j][i]) {
        FreePicture (pMa, &pCtx->pSpatialPic[j][i]);
      }
      ++ i;
    }
    pCtx->uiSpatialLayersInTemporal[j]	= 0;
    ++ j;
  }

}

static  void  InitMbInfo (sWelsEncCtx* pEnc, SMB*   pList, SDqLayer* pLayer, const int32_t kiDlayerId,
                          const int32_t kiMaxMbNum) {
  int32_t  iMbWidth		= pLayer->iMbWidth;
  int32_t  iMbHeight		= pLayer->iMbHeight;
  int32_t  iIdx;
  int32_t  iMbNum			= iMbWidth * iMbHeight;
  SSliceCtx* pSliceCtx = pLayer->pSliceEncCtx;
  uint32_t uiNeighborAvail;
  const int32_t kiOffset	= (kiDlayerId & 0x01) * kiMaxMbNum;
  SMVUnitXY (*pLayerMvUnitBlock4x4)[MB_BLOCK4x4_NUM]	= (SMVUnitXY (*)[MB_BLOCK4x4_NUM]) (
        &pEnc->pMvUnitBlock4x4[MB_BLOCK4x4_NUM * kiOffset]);
  int8_t (*pLayerRefIndexBlock8x8)[MB_BLOCK8x8_NUM]		= (int8_t (*)[MB_BLOCK8x8_NUM]) (
        &pEnc->pRefIndexBlock4x4[MB_BLOCK8x8_NUM * kiOffset]);

  for (iIdx = 0; iIdx < iMbNum; iIdx++) {
    BOOL_T     bLeft;
    BOOL_T     bTop;
    BOOL_T     bLeftTop;
    BOOL_T     bRightTop;
    int32_t  iLeftXY, iTopXY, iLeftTopXY, iRightTopXY;
    uint8_t  uiSliceIdc;

    pList[iIdx].iMbX = pEnc->pStrideTab->pMbIndexX[kiDlayerId][iIdx];
    pList[iIdx].iMbY = pEnc->pStrideTab->pMbIndexY[kiDlayerId][iIdx];
    pList[iIdx].iMbXY = iIdx;

    uiSliceIdc = WelsMbToSliceIdc (pSliceCtx, iIdx);
    iLeftXY = iIdx - 1;
    iTopXY = iIdx - iMbWidth;
    iLeftTopXY = iTopXY - 1;
    iRightTopXY = iTopXY + 1;

    bLeft = (pList[iIdx].iMbX > 0) && (uiSliceIdc == WelsMbToSliceIdc (pSliceCtx, iLeftXY));
    bTop = (pList[iIdx].iMbY > 0) && (uiSliceIdc == WelsMbToSliceIdc (pSliceCtx, iTopXY));
    bLeftTop = (pList[iIdx].iMbX > 0) && (pList[iIdx].iMbY > 0) && (uiSliceIdc ==
               WelsMbToSliceIdc (pSliceCtx, iLeftTopXY));
    bRightTop = (pList[iIdx].iMbX < (iMbWidth - 1)) && (pList[iIdx].iMbY > 0) && (uiSliceIdc ==
                WelsMbToSliceIdc (pSliceCtx, iRightTopXY));

    uiNeighborAvail = 0;
    if (bLeft) {
      uiNeighborAvail |= LEFT_MB_POS;
    }
    if (bTop) {
      uiNeighborAvail |= TOP_MB_POS;
    }
    if (bLeftTop) {
      uiNeighborAvail |= TOPLEFT_MB_POS;
    }
    if (bRightTop) {
      uiNeighborAvail |= TOPRIGHT_MB_POS;
    }
    pList[iIdx].uiSliceIdc		= uiSliceIdc;	// merge from svc_hd_opt_b for multiple slices coding
    pList[iIdx].uiNeighborAvail	= uiNeighborAvail;
    uiNeighborAvail = 0;
    if (pList[iIdx].iMbX >= BASE_MV_MB_NMB)
      uiNeighborAvail |= LEFT_MB_POS;
    if (pList[iIdx].iMbX <= (iMbWidth - 1 - BASE_MV_MB_NMB))
      uiNeighborAvail |= RIGHT_MB_POS;
    if (pList[iIdx].iMbY >= BASE_MV_MB_NMB)
      uiNeighborAvail |= TOP_MB_POS;
    if (pList[iIdx].iMbY <= (iMbHeight - 1 - BASE_MV_MB_NMB))
      uiNeighborAvail |= BOTTOM_MB_POS;

    pList[iIdx].sMv					= pLayerMvUnitBlock4x4[iIdx];
    pList[iIdx].pRefIndex			= pLayerRefIndexBlock8x8[iIdx];
    pList[iIdx].pSadCost				= &pEnc->pSadCostMb[iIdx];
    pList[iIdx].pIntra4x4PredMode	= &pEnc->pIntra4x4PredModeBlocks[iIdx * INTRA_4x4_MODE_NUM];
    pList[iIdx].pNonZeroCount		= &pEnc->pNonZeroCountBlocks[iIdx * MB_LUMA_CHROMA_BLOCK4x4_NUM];
  }
}


int32_t   InitMbListD (sWelsEncCtx** ppCtx) {
  int32_t		iNumDlayer = (*ppCtx)->pSvcParam->iNumDependencyLayer;
  int32_t		iMbSize[MAX_DEPENDENCY_LAYER] = { 0 };
  int32_t		iOverallMbNum = 0;
  int32_t		iMbWidth = 0;
  int32_t		iMbHeight = 0;
  int32_t		i;

  if (iNumDlayer > MAX_DEPENDENCY_LAYER)
    return 1;

  for (i = 0; i < iNumDlayer; i++) {
    iMbWidth = ((*ppCtx)->pSvcParam->sDependencyLayers[i].iFrameWidth + 15) >> 4;
    iMbHeight = ((*ppCtx)->pSvcParam->sDependencyLayers[i].iFrameHeight + 15) >> 4;
    iMbSize[i] = iMbWidth  * iMbHeight;
    iOverallMbNum += iMbSize[i];
  }

  (*ppCtx)->ppMbListD = static_cast<SMB**> ((*ppCtx)->pMemAlign->WelsMalloc (iNumDlayer * sizeof (SMB*), "ppMbListD"));
  (*ppCtx)->ppMbListD[0] = NULL;
  WELS_VERIFY_RETURN_PROC_IF (1, (*ppCtx)->ppMbListD == NULL, FreeMemorySvc (ppCtx));
  (*ppCtx)->ppMbListD[0] = static_cast<SMB*> ((*ppCtx)->pMemAlign->WelsMallocz (iOverallMbNum * sizeof (SMB),
                           "ppMbListD[0]"));
  WELS_VERIFY_RETURN_PROC_IF (1, (*ppCtx)->ppMbListD[0] == NULL, FreeMemorySvc (ppCtx));
  (*ppCtx)->ppDqLayerList[0]->sMbDataP = (*ppCtx)->ppMbListD[0];
  InitMbInfo (*ppCtx, (*ppCtx)->ppMbListD[0], (*ppCtx)->ppDqLayerList[0], 0, iMbSize[iNumDlayer - 1]);
  for (i = 1; i < iNumDlayer; i++) {
    (*ppCtx)->ppMbListD[i] = (*ppCtx)->ppMbListD[i - 1] + iMbSize[i - 1];
    (*ppCtx)->ppDqLayerList[i]->sMbDataP = (*ppCtx)->ppMbListD[i];
    InitMbInfo (*ppCtx, (*ppCtx)->ppMbListD[i], (*ppCtx)->ppDqLayerList[i], i, iMbSize[iNumDlayer - 1]);
  }

  return 0;
}

int32_t AllocMbCacheAligned (SMbCache* pMbCache, CMemoryAlign* pMa) {
  pMbCache->pCoeffLevel = (int16_t*)pMa->WelsMalloc (MB_COEFF_LIST_SIZE * sizeof (int16_t), "pMbCache->pCoeffLevel");
  WELS_VERIFY_RETURN_IF (1, (NULL == pMbCache->pCoeffLevel));
  pMbCache->pMemPredMb = (uint8_t*)pMa->WelsMalloc (2 * 256 * sizeof (uint8_t), "pMbCache->pMemPredMb");
  WELS_VERIFY_RETURN_IF (1, (NULL == pMbCache->pMemPredMb));
  pMbCache->pSkipMb = (uint8_t*)pMa->WelsMalloc (384 * sizeof (uint8_t), "pMbCache->pSkipMb");
  WELS_VERIFY_RETURN_IF (1, (NULL == pMbCache->pSkipMb));
  pMbCache->pMemPredBlk4 = (uint8_t*)pMa->WelsMalloc (2 * 16 * sizeof (uint8_t), "pMbCache->pMemPredBlk4");
  WELS_VERIFY_RETURN_IF (1, (NULL == pMbCache->pMemPredBlk4));
  pMbCache->pBufferInterPredMe = (uint8_t*)pMa->WelsMalloc (4 * 640 * sizeof (uint8_t), "pMbCache->pBufferInterPredMe");
  WELS_VERIFY_RETURN_IF (1, (NULL == pMbCache->pBufferInterPredMe));
  pMbCache->pPrevIntra4x4PredModeFlag = (bool_t*)pMa->WelsMalloc (16 * sizeof (bool_t),
                                        "pMbCache->pPrevIntra4x4PredModeFlag");
  WELS_VERIFY_RETURN_IF (1, (NULL == pMbCache->pPrevIntra4x4PredModeFlag));
  pMbCache->pRemIntra4x4PredModeFlag	= (int8_t*)pMa->WelsMalloc (16 * sizeof (int8_t),
                                        "pMbCache->pRemIntra4x4PredModeFlag");
  WELS_VERIFY_RETURN_IF (1, (NULL == pMbCache->pRemIntra4x4PredModeFlag));
  pMbCache->pDct = (SDCTCoeff*)pMa->WelsMalloc (sizeof (SDCTCoeff), "pMbCache->pDct");
  WELS_VERIFY_RETURN_IF (1, (NULL == pMbCache->pDct));
  return 0;
}

void FreeMbCache (SMbCache* pMbCache, CMemoryAlign* pMa) {
  if (NULL != pMbCache->pCoeffLevel) {
    pMa->WelsFree (pMbCache->pCoeffLevel, "pMbCache->pCoeffLevel");
    pMbCache->pCoeffLevel = NULL;
  }
  if (NULL != pMbCache->pMemPredMb) {
    pMa->WelsFree (pMbCache->pMemPredMb, "pMbCache->pMemPredMb");
    pMbCache->pMemPredMb = NULL;
  }
  if (NULL != pMbCache->pSkipMb) {
    pMa->WelsFree (pMbCache->pSkipMb, "pMbCache->pSkipMb");
    pMbCache->pSkipMb = NULL;
  }
  if (NULL != pMbCache->pMemPredBlk4) {
    pMa->WelsFree (pMbCache->pMemPredBlk4, "pMbCache->pMemPredBlk4");
    pMbCache->pMemPredBlk4 = NULL;
  }
  if (NULL != pMbCache->pBufferInterPredMe) {
    pMa->WelsFree (pMbCache->pBufferInterPredMe, "pMbCache->pBufferInterPredMe");
    pMbCache->pBufferInterPredMe = NULL;
  }
  if (NULL != pMbCache->pPrevIntra4x4PredModeFlag) {
    pMa->WelsFree (pMbCache->pPrevIntra4x4PredModeFlag, "pMbCache->pPrevIntra4x4PredModeFlag");
    pMbCache->pPrevIntra4x4PredModeFlag = NULL;
  }
  if (NULL != pMbCache->pRemIntra4x4PredModeFlag) {
    pMa->WelsFree (pMbCache->pRemIntra4x4PredModeFlag, "pMbCache->pRemIntra4x4PredModeFlag");
    pMbCache->pRemIntra4x4PredModeFlag = NULL;
  }
  if (NULL != pMbCache->pDct) {
    pMa->WelsFree (pMbCache->pDct, "pMbCache->pDct");
    pMbCache->pDct = NULL;
  }
}


/*!
 * \brief	initialize ppDqLayerList and slicepEncCtx_list due to count number of layers available
 * \pParam	pCtx			sWelsEncCtx*
 * \return	0 - successful; otherwise failed
 */
static inline int32_t InitDqLayers (sWelsEncCtx** ppCtx) {
  SWelsSvcCodingParam* pParam	= NULL;
  SWelsSPS* pSps						= NULL;
  SSubsetSps* pSubsetSps			= NULL;
  SWelsPPS* pPps						= NULL;
  CMemoryAlign* pMa				= NULL;
  SStrideTables* pStrideTab		= NULL;
  int32_t iDlayerCount					= 0;
  int32_t iDlayerIndex					= 0;
  uint32_t iSpsId					= 0;
  uint32_t iPpsId					= 0;
  uint32_t iNumRef				= 0;
  int32_t iResult					= 0;

  if (NULL == ppCtx || NULL == *ppCtx)
    return 1;

  pMa		= (*ppCtx)->pMemAlign;
  pParam	= (*ppCtx)->pSvcParam;
  iDlayerCount	= pParam->iNumDependencyLayer;
  iNumRef	= pParam->iNumRefFrame;
//	highest_layers_in_temporal = 1 + WELS_MAX(pParam->iDecompStages, 1);
  pStrideTab	= (*ppCtx)->pStrideTab;

  iDlayerIndex			= 0;
  while (iDlayerIndex < iDlayerCount) {
    SRefList* pRefList			= NULL;
    uint32_t i					= 0;
    const int32_t kiWidth			= pParam->sDependencyLayers[iDlayerIndex].iFrameWidth;
    const int32_t kiHeight		= pParam->sDependencyLayers[iDlayerIndex].iFrameHeight;
    int32_t iPicWidth			= WELS_ALIGN (kiWidth, MB_WIDTH_LUMA) + (PADDING_LENGTH << 1);	// with iWidth of horizon
    int32_t iPicChromaWidth	= iPicWidth >> 1;

    iPicWidth	= WELS_ALIGN (iPicWidth,
                            32);	// 32(or 16 for chroma below) to match original imp. here instead of iCacheLineSize
    iPicChromaWidth	= WELS_ALIGN (iPicChromaWidth, 16);

    WelsGetEncBlockStrideOffset ((*ppCtx)->pStrideTab->pStrideEncBlockOffset[iDlayerIndex], iPicWidth, iPicChromaWidth);

    // pRef list
    pRefList		= (SRefList*)pMa->WelsMallocz (sizeof (SRefList), "pRefList");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pRefList), FreeMemorySvc (ppCtx))

    do {
      pRefList->pRef[i]	= AllocPicture (pMa, kiWidth, kiHeight, true);	// to use actual size of current layer
      WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pRefList->pRef[i]), FreeMemorySvc (ppCtx))
      ++ i;
    } while (i < 1 + iNumRef);

    pRefList->pNextBuffer = pRefList->pRef[0];
    (*ppCtx)->ppRefPicListExt[iDlayerIndex]	= pRefList;
    ++ iDlayerIndex;
  }

  // for I420 based source spatial pictures
  if (AllocSpatialPictures (ppCtx, pParam)) {
    FreeMemorySvc (ppCtx);
    return 1;
  }

  iDlayerIndex	= 0;
  while (iDlayerIndex < iDlayerCount) {
    SDqLayer* pDqLayer		= NULL;
    SDLayerParam* pDlayer	= &pParam->sDependencyLayers[iDlayerIndex];
    const int32_t kiMbW		= (pDlayer->iFrameWidth + 0x0f) >> 4;
    const int32_t kiMbH		= (pDlayer->iFrameHeight + 0x0f) >> 4;
    int32_t iMaxSliceNum	= 1;
    const int32_t kiSliceNum = GetInitialSliceNum (kiMbW, kiMbH, &pDlayer->sMso);
    if (iMaxSliceNum < kiSliceNum)
      iMaxSliceNum = kiSliceNum;

    // pDq layers list
    pDqLayer = (SDqLayer*)pMa->WelsMallocz (sizeof (SDqLayer), "pDqLayer");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pDqLayer), FreeMemorySvc (ppCtx))

    // for dynamic slicing mode
    if (SM_DYN_SLICE == pDlayer->sMso.uiSliceMode) {
      const int32_t iSize			= pParam->iCountThreadsNum * sizeof (int32_t);

      pDqLayer->pNumSliceCodedOfPartition		= (int32_t*)pMa->WelsMallocz (iSize, "pNumSliceCodedOfPartition");
      pDqLayer->pLastCodedMbIdxOfPartition	= (int32_t*)pMa->WelsMallocz (iSize, "pLastCodedMbIdxOfPartition");
      pDqLayer->pLastMbIdxOfPartition			= (int32_t*)pMa->WelsMallocz (iSize, "pLastMbIdxOfPartition");

      WELS_VERIFY_RETURN_PROC_IF (1,
                                  (NULL == pDqLayer->pNumSliceCodedOfPartition ||
                                   NULL == pDqLayer->pLastCodedMbIdxOfPartition ||
                                   NULL == pDqLayer->pLastMbIdxOfPartition),
                                  FreeMemorySvc (ppCtx))
    }

    pDqLayer->iMbWidth					= kiMbW;
    pDqLayer->iMbHeight					= kiMbH;
#ifndef MT_ENABLED
    if (SM_DYN_SLICE == pDlayer->sMso.uiSliceMode) { //wmalloc pSliceInLayer
      SSlice* pSlice			= NULL;
      int32_t iSliceIdx		= 0;
      //wmalloc AVERSLICENUM_CONSTANT of pDqLayer->sLayerInfo.pSliceInLayer,
      //wmalloc AVERSLICENUM_CONSTANT num of pSlice as initialization
      //only set value for the first pSlice
      pDqLayer->sLayerInfo.pSliceInLayer	= (SSlice*)pMa->WelsMallocz (sizeof (SSlice) * iMaxSliceNum, "pSliceInLayer");

      WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pDqLayer->sLayerInfo.pSliceInLayer), FreeMemorySvc (ppCtx)) {
        pSlice = &pDqLayer->sLayerInfo.pSliceInLayer[0];
        pSlice->uiSliceIdx = 0;
        pSlice->pSliceBsa = & (*ppCtx)->pOut->sBsWrite;
      }

      while (iSliceIdx < iMaxSliceNum) {
        pSlice = &pDqLayer->sLayerInfo.pSliceInLayer[iSliceIdx];
        if (AllocMbCacheAligned (&pSlice->sMbCacheInfo, pMa)) {
          FreeMemorySvc (ppCtx);
          return 1;
        }
        ++ iSliceIdx;
      }
    } else
#endif//!MT_ENABLED
    {
      int32_t iSliceIdx		= 0;
      pDqLayer->sLayerInfo.pSliceInLayer	= (SSlice*)pMa->WelsMallocz (sizeof (SSlice) * iMaxSliceNum, "pSliceInLayer");

      WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pDqLayer->sLayerInfo.pSliceInLayer), FreeMemorySvc (ppCtx))
      if (iMaxSliceNum > 1) {
        while (iSliceIdx < iMaxSliceNum) {
          SSlice* pSlice = &pDqLayer->sLayerInfo.pSliceInLayer[iSliceIdx];
          pSlice->uiSliceIdx = iSliceIdx;
#ifdef MT_ENABLED
          if (pParam->iMultipleThreadIdc > 1)
            pSlice->pSliceBsa = & (*ppCtx)->pSliceBs[iSliceIdx].sBsWrite;
          else
            pSlice->pSliceBsa = & (*ppCtx)->pOut->sBsWrite;
#else
          pSlice->pSliceBsa = & (*ppCtx)->pOut->sBsWrite;
#endif//MT_ENABLED
          if (AllocMbCacheAligned (&pSlice->sMbCacheInfo, pMa)) {
            FreeMemorySvc (ppCtx);
            return 1;
          }
          ++ iSliceIdx;
        }
      }
      // fix issue in case single pSlice coding might be inclusive exist in variant spatial layer setting, also introducing multi-pSlice modes
      else {	// only one pSlice
        SSlice* pSlice = &pDqLayer->sLayerInfo.pSliceInLayer[0];
        pSlice->uiSliceIdx	= 0;
        pSlice->pSliceBsa	= & (*ppCtx)->pOut->sBsWrite;
        if (AllocMbCacheAligned (&pSlice->sMbCacheInfo, pMa)) {
          FreeMemorySvc (ppCtx);
          return 1;
        }
      }
    }

    //deblocking parameters initialization
    //target-layer deblocking
    pDqLayer->iLoopFilterDisableIdc	                = pParam->iLoopFilterDisableIdc;
    pDqLayer->iLoopFilterAlphaC0Offset				= (pParam->iLoopFilterAlphaC0Offset) << 1;
    pDqLayer->iLoopFilterBetaOffset					= (pParam->iLoopFilterBetaOffset) << 1;
    //inter-layer deblocking
    pDqLayer->uiDisableInterLayerDeblockingFilterIdc	= pParam->iInterLayerLoopFilterDisableIdc;
    pDqLayer->iInterLayerSliceAlphaC0Offset				= (pParam->iInterLayerLoopFilterAlphaC0Offset) << 1;
    pDqLayer->iInterLayerSliceBetaOffset				= (pParam->iInterLayerLoopFilterBetaOffset) << 1;
    //parallel deblocking
    pDqLayer->bDeblockingParallelFlag                  = pParam->bDeblockingParallelFlag;

    //deblocking parameter adjustment
    if (SM_SINGLE_SLICE == pDlayer->sMso.uiSliceMode) {
      //iLoopFilterDisableIdc: will be 0 or 1 under single_slice
      if (2 == pParam->iLoopFilterDisableIdc) {
        pDqLayer->iLoopFilterDisableIdc	= 0;
      }
      //bDeblockingParallelFlag
      pDqLayer->bDeblockingParallelFlag = false;
    } else {
      //multi-pSlice
#ifdef MT_ENABLED
      if (0 == pDqLayer->iLoopFilterDisableIdc) {
        pDqLayer->bDeblockingParallelFlag	= false;
      }
#endif
    }

    (*ppCtx)->ppDqLayerList[iDlayerIndex]	= pDqLayer;

    ++ iDlayerIndex;
  }

  // for dynamically malloc for parameter sets memory instead of maximal items for standard to reduce size, 3/18/2010
  if (& (*ppCtx)->pSvcParam->bMgsT0OnlyStrategy) {
    (*ppCtx)->pPPSArray	= (SWelsPPS*)pMa->WelsMalloc ((1 + iDlayerCount) * sizeof (SWelsPPS), "pPPSArray");
  } else {
    (*ppCtx)->pPPSArray	= (SWelsPPS*)pMa->WelsMalloc (iDlayerCount * sizeof (SWelsPPS), "pPPSArray");
  }
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pPPSArray), FreeMemorySvc (ppCtx))

  (*ppCtx)->pSpsArray	= (SWelsSPS*)pMa->WelsMalloc (sizeof (SWelsSPS), "pSpsArray");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pSpsArray), FreeMemorySvc (ppCtx))
  if (iDlayerCount > 1) {
    (*ppCtx)->pSubsetArray	= (SSubsetSps*)pMa->WelsMalloc ((iDlayerCount - 1) * sizeof (SSubsetSps), "pSubsetArray");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pSubsetArray), FreeMemorySvc (ppCtx))
  }

  (*ppCtx)->pDqIdcMap	= (SDqIdc*)pMa->WelsMallocz (iDlayerCount * sizeof (SDqIdc), "pDqIdcMap");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pDqIdcMap), FreeMemorySvc (ppCtx))

  iDlayerIndex	= 0;
  while (iDlayerIndex < iDlayerCount) {
    SDqIdc* pDqIdc		= & (*ppCtx)->pDqIdcMap[iDlayerIndex];
    const bool_t bUseSubsetSps			= (iDlayerIndex > BASE_DEPENDENCY_ID);
    SDLayerParam* pDlayerParam	= &pParam->sDependencyLayers[iDlayerIndex];

    pDqIdc->uiSpatialId	= iDlayerIndex;
    pPps	= & (*ppCtx)->pPPSArray[iPpsId];
    if (!bUseSubsetSps) {
      pSps	= & (*ppCtx)->pSpsArray[iSpsId];
    } else {
      pSubsetSps	= & (*ppCtx)->pSubsetArray[iSpsId];
      pSps			= &pSubsetSps->pSps;
    }

    // Need port pSps/pPps initialization due to spatial scalability changed
    if (!bUseSubsetSps) {
      WelsInitSps (pSps, pDlayerParam, pParam->uiIntraPeriod, pParam->iNumRefFrame, iSpsId,
                   pParam->bEnableFrameCroppingFlag, pParam->bEnableRc);

      if (iDlayerCount > 1) {
        pSps->bConstraintSet0Flag = true;
        pSps->bConstraintSet1Flag = true;
        pSps->bConstraintSet2Flag = true;
      }
    } else {
      WelsInitSubsetSps (pSubsetSps, pDlayerParam, pParam->uiIntraPeriod, pParam->iNumRefFrame, iSpsId,
                         pParam->bEnableFrameCroppingFlag, pParam->bEnableRc);
    }

    // initialize pPps
    WelsInitPps (pPps, pSps, pSubsetSps, iPpsId, true, bUseSubsetSps);

    // Not using FMO in SVC coding so far, come back if need FMO
    {
      iResult = InitSlicePEncCtx (& (*ppCtx)->pSliceCtxList[iDlayerIndex],
                                  (*ppCtx)->pMemAlign,
                                  false,
                                  pSps->iMbWidth,
                                  pSps->iMbHeight,
                                  & (pDlayerParam->sMso),
                                  pPps);
      if (iResult) {
        WelsLog (*ppCtx, WELS_LOG_WARNING, "InitDqLayers(), InitSlicePEncCtx failed(%d)!", iResult);
        FreeMemorySvc (ppCtx);
        return 1;
      }
      (*ppCtx)->ppDqLayerList[iDlayerIndex]->pSliceEncCtx	= & (*ppCtx)->pSliceCtxList[iDlayerIndex];
    }
    pDqIdc->iSpsId	= iSpsId;
    pDqIdc->iPpsId	= iPpsId;

    (*ppCtx)->sPSOVector.bPpsIdMappingIntoSubsetsps[iPpsId] = bUseSubsetSps;

    if (bUseSubsetSps)
      ++ iSpsId;
    ++ iPpsId;
    ++ (*ppCtx)->iSpsNum;
    ++ (*ppCtx)->iPpsNum;

    ++ iDlayerIndex;
  }
  return 0;
}

int32_t AllocStrideTables (sWelsEncCtx** ppCtx, const int32_t kiNumSpatialLayers) {
  CMemoryAlign* pMa				= (*ppCtx)->pMemAlign;
  SWelsSvcCodingParam* pParam	= (*ppCtx)->pSvcParam;
  SStrideTables* pPtr				= NULL;
  int16_t* pTmpRow	= NULL, *pRowX = NULL, *pRowY = NULL, *p = NULL;
  uint8_t* pBase		= NULL;
  uint8_t* pBaseDec = NULL, *pBaseEnc = NULL, *pBaseMbX = NULL, *pBaseMbY = NULL;
  struct {
    int32_t iMbWidth;
    int32_t iCountMbNum;				// count number of SMB in each spatial
    int32_t iSizeAllMbAlignCache;	// cache line size aligned in each spatial
  } sMbSizeMap[MAX_DEPENDENCY_LAYER] = {0};
  int32_t iLineSizeY[MAX_DEPENDENCY_LAYER][2] = {0};
  int32_t iLineSizeUV[MAX_DEPENDENCY_LAYER][2] = {0};
  int32_t iMapSpatialIdx[MAX_DEPENDENCY_LAYER][2] = {0};
  int32_t iSizeDec		= 0;
  int32_t iSizeEnc		= 0;
  int32_t iCountLayersNeedCs[2]	= {0};
  const int32_t kiUnit1Size = 24 * sizeof (int32_t);
  int32_t iUnit2Size		= 0;
  int32_t iNeedAllocSize	= 0;
  int32_t iRowSize		= 0;
  int16_t iMaxMbWidth	= 0;
  int16_t iMaxMbHeight	= 0;
  int32_t i				= 0;
  int32_t iSpatialIdx		= 0;
  int32_t iTemporalIdx	= 0;
  int32_t iCntTid			= 0;

  if (kiNumSpatialLayers <= 0 || kiNumSpatialLayers > MAX_DEPENDENCY_LAYER)
    return 1;

  pPtr = (SStrideTables*)pMa->WelsMalloc (sizeof (SStrideTables), "SStrideTables");
  if (NULL == pPtr)
    return 1;
  (*ppCtx)->pStrideTab = pPtr;

  iCntTid	= pParam->iNumTemporalLayer > 1 ? 2 : 1;

  iSpatialIdx = 0;
  while (iSpatialIdx < kiNumSpatialLayers) {
    const int32_t kiTmpWidth = (pParam->sDependencyLayers[iSpatialIdx].iFrameWidth + 15) >> 4;
    const int32_t kiTmpHeight = (pParam->sDependencyLayers[iSpatialIdx].iFrameHeight + 15) >> 4;
    int32_t iNumMb = kiTmpWidth * kiTmpHeight;

    sMbSizeMap[iSpatialIdx].iMbWidth		= kiTmpWidth;
    sMbSizeMap[iSpatialIdx].iCountMbNum	= iNumMb;

    iNumMb *= sizeof (int16_t);
    sMbSizeMap[iSpatialIdx].iSizeAllMbAlignCache = iNumMb;
    iUnit2Size += iNumMb;

    ++ iSpatialIdx;
  }

  // Adaptive size_cs, size_fdec by implementation dependency
  iTemporalIdx = 0;
  while (iTemporalIdx < iCntTid) {
    const bool_t kbBaseTemporalFlag	= (iTemporalIdx == 0);

    iSpatialIdx = 0;
    while (iSpatialIdx < kiNumSpatialLayers) {
      SDLayerParam* fDlp					= &pParam->sDependencyLayers[iSpatialIdx];

      const int32_t kiWidthPad = WELS_ALIGN (fDlp->iFrameWidth, 16) + (PADDING_LENGTH << 1);
      iLineSizeY[iSpatialIdx][kbBaseTemporalFlag]	= WELS_ALIGN (kiWidthPad, 32);
      iLineSizeUV[iSpatialIdx][kbBaseTemporalFlag] = WELS_ALIGN ((kiWidthPad >> 1), 16);

      iMapSpatialIdx[iCountLayersNeedCs[kbBaseTemporalFlag]][kbBaseTemporalFlag] = iSpatialIdx;
      ++ iCountLayersNeedCs[kbBaseTemporalFlag];
      ++ iSpatialIdx;
    }
    ++ iTemporalIdx;
  }
  iSizeDec = kiUnit1Size * (iCountLayersNeedCs[0] + iCountLayersNeedCs[1]);
  iSizeEnc = kiUnit1Size * kiNumSpatialLayers;

  iNeedAllocSize = iSizeDec + iSizeEnc + (iUnit2Size << 1);

  pBase = (uint8_t*)pMa->WelsMalloc (iNeedAllocSize, "pBase");
  if (NULL == pBase) {
    return 1;
  }

  pBaseDec = pBase;		// iCountLayersNeedCs
  pBaseEnc = pBaseDec + iSizeDec;		// iNumSpatialLayers
  pBaseMbX = pBaseEnc + iSizeEnc;	// iNumSpatialLayers
  pBaseMbY = pBaseMbX + iUnit2Size;	// iNumSpatialLayers

  iTemporalIdx = 0;
  while (iTemporalIdx < iCntTid) {
    const bool_t kbBaseTemporalFlag	= (iTemporalIdx == 0);

    iSpatialIdx = 0;
    while (iSpatialIdx < iCountLayersNeedCs[kbBaseTemporalFlag]) {
      const int32_t kiActualSpatialIdx = iMapSpatialIdx[iSpatialIdx][kbBaseTemporalFlag];
      const int32_t kiLumaWidth	= iLineSizeY[kiActualSpatialIdx][kbBaseTemporalFlag];
      const int32_t kiChromaWidth	= iLineSizeUV[kiActualSpatialIdx][kbBaseTemporalFlag];

      WelsGetEncBlockStrideOffset ((int32_t*)pBaseDec, kiLumaWidth, kiChromaWidth);

      pPtr->pStrideDecBlockOffset[kiActualSpatialIdx][kbBaseTemporalFlag]	= (int32_t*)pBaseDec;
      pBaseDec += kiUnit1Size;

      ++ iSpatialIdx;
    }
    ++ iTemporalIdx;
  }
  iTemporalIdx = 0;
  while (iTemporalIdx < iCntTid) {
    const bool_t kbBaseTemporalFlag	= (iTemporalIdx == 0);

    iSpatialIdx = 0;
    while (iSpatialIdx < kiNumSpatialLayers) {
      int32_t iMatchIndex = 0;
      bool_t bInMap = false;
      bool_t bMatchFlag = false;

      i = 0;
      while (i < iCountLayersNeedCs[kbBaseTemporalFlag]) {
        const int32_t kiActualIdx = iMapSpatialIdx[i][kbBaseTemporalFlag];
        if (kiActualIdx == iSpatialIdx) {
          bInMap	= true;
          break;
        }
        if (!bMatchFlag) {
          iMatchIndex	= kiActualIdx;
          bMatchFlag	= true;
        }
        ++ i;
      }

      if (bInMap) {
        ++ iSpatialIdx;
        continue;
      }

      // not in spatial map and assign match one to it
      pPtr->pStrideDecBlockOffset[iSpatialIdx][kbBaseTemporalFlag]	=
        pPtr->pStrideDecBlockOffset[iMatchIndex][kbBaseTemporalFlag];

      ++ iSpatialIdx;
    }
    ++ iTemporalIdx;
  }

  iSpatialIdx = 0;
  while (iSpatialIdx < kiNumSpatialLayers) {
    const int32_t kiAllocMbSize = sMbSizeMap[iSpatialIdx].iSizeAllMbAlignCache;

    pPtr->pStrideEncBlockOffset[iSpatialIdx]	= (int32_t*)pBaseEnc;

    pPtr->pMbIndexX[iSpatialIdx]				= (int16_t*)pBaseMbX;
    pPtr->pMbIndexY[iSpatialIdx]				= (int16_t*)pBaseMbY;

    pBaseEnc += kiUnit1Size;
    pBaseMbX += kiAllocMbSize;
    pBaseMbY += kiAllocMbSize;

    ++ iSpatialIdx;
  }

  while (iSpatialIdx < MAX_DEPENDENCY_LAYER) {
    pPtr->pStrideDecBlockOffset[iSpatialIdx][0]	= NULL;
    pPtr->pStrideDecBlockOffset[iSpatialIdx][1]	= NULL;
    pPtr->pStrideEncBlockOffset[iSpatialIdx]		= NULL;
    pPtr->pMbIndexX[iSpatialIdx]					= NULL;
    pPtr->pMbIndexY[iSpatialIdx]					= NULL;

    ++ iSpatialIdx;
  }

  // initialize pMbIndexX and pMbIndexY tables as below

  iMaxMbWidth	= sMbSizeMap[kiNumSpatialLayers - 1].iMbWidth;
  iMaxMbWidth	= WELS_ALIGN (iMaxMbWidth, 4);	// 4 loops for int16_t required introduced as below
  iRowSize		= iMaxMbWidth * sizeof (int16_t);

  pTmpRow = (int16_t*)pMa->WelsMalloc (iRowSize, "pTmpRow");
  if (NULL == pTmpRow) {
    return 1;
  }
  pRowX = pTmpRow;
  pRowY = pRowX;
  // initialize pRowX & pRowY
  i = 0;
  p = pRowX;
  while (i < iMaxMbWidth) {
    *p		= i;
    * (p + 1)	= 1 + i;
    * (p + 2)	= 2 + i;
    * (p + 3)	= 3 + i;

    p += 4;
    i += 4;
  }

  iSpatialIdx = kiNumSpatialLayers;
  while (--iSpatialIdx >= 0) {
    int16_t* pMbIndexX = pPtr->pMbIndexX[iSpatialIdx];
    const int32_t kiMbWidth	= sMbSizeMap[iSpatialIdx].iMbWidth;
    const int32_t kiMbHeight	= sMbSizeMap[iSpatialIdx].iCountMbNum / kiMbWidth;
    const int32_t kiLineSize	= kiMbWidth * sizeof (int16_t);

    i = 0;
    while (i < kiMbHeight) {
      memcpy (pMbIndexX, pRowX, kiLineSize);	// confirmed_safe_unsafe_usage

      pMbIndexX += kiMbWidth;
      ++ i;
    }
  }

  memset (pRowY, 0, iRowSize);
  iMaxMbHeight	= sMbSizeMap[kiNumSpatialLayers - 1].iCountMbNum / sMbSizeMap[kiNumSpatialLayers - 1].iMbWidth;
  i = 0;
  for (;;) {
    ENFORCE_STACK_ALIGN_1D (int16_t, t, 4, 16)

    int32_t t32 = 0;
    int16_t j = 0;

    for (iSpatialIdx = kiNumSpatialLayers - 1; iSpatialIdx >= 0; -- iSpatialIdx) {
      const int32_t kiMbWidth	= sMbSizeMap[iSpatialIdx].iMbWidth;
      const int32_t kiMbHeight = sMbSizeMap[iSpatialIdx].iCountMbNum / kiMbWidth;
      const int32_t kiLineSize	= kiMbWidth * sizeof (int16_t);
      int16_t* pMbIndexY = pPtr->pMbIndexY[iSpatialIdx] + i * kiMbWidth;

      if (i < kiMbHeight) {
        memcpy (pMbIndexY, pRowY, kiLineSize);	// confirmed_safe_unsafe_usage
      }
    }
    ++ i;
    if (i >= iMaxMbHeight)
      break;

    t32 = i | (i << 16);
    ST32 (t  , t32);
    ST32 (t + 2, t32);

    p = pRowY;
    while (j < iMaxMbWidth) {
      ST64 (p, LD64 (t));

      p += 4;
      j += 4;
    }
  }

  pMa->WelsFree (pTmpRow, "pTmpRow");
  pTmpRow = NULL;

  return 0;
}

/*!
 * \brief	request specific memory for SVC
 * \pParam	pEncCtx		sWelsEncCtx*
 * \return	successful - 0; otherwise none 0 for failed
 */
int32_t RequestMemorySvc (sWelsEncCtx** ppCtx) {
  SWelsSvcCodingParam* pParam	= (*ppCtx)->pSvcParam;
  CMemoryAlign* pMa				= (*ppCtx)->pMemAlign;
  SDLayerParam* pFinalSpatial	= NULL;
  int32_t iCountBsLen			= 0;
  int32_t iCountNals				= 0;
  int32_t iMaxPicWidth			= 0;
  int32_t iMaxPicHeight			= 0;
  int32_t iCountMaxMbNum		= 0;
  int32_t iIndex					= 0;
  int32_t iCountLayers			= 0;
  int32_t iResult					= 0;
  float	fCompressRatioThr		= .5f;
  const int32_t kiNumDependencyLayers	= pParam->iNumDependencyLayer;
  const uint32_t kuiMvdInterTableSize	= (kiNumDependencyLayers == 1 ? (1 + (648 << 1)) : (1 + (972 << 1)));
  const uint32_t kuiMvdCacheAlginedSize	= kuiMvdInterTableSize * sizeof (uint16_t);
  int32_t iVclLayersBsSizeCount		= 0;
  int32_t iNonVclLayersBsSizeCount	= 0;
#if defined(MT_ENABLED)
  int32_t iTargetSpatialBsSize			= 0;
#endif//MT_ENABLED

  if (kiNumDependencyLayers < 1 || kiNumDependencyLayers > MAX_DEPENDENCY_LAYER) {
    WelsLog (*ppCtx, WELS_LOG_WARNING, "RequestMemorySvc() failed due to invalid iNumDependencyLayers(%d)!\n",
             kiNumDependencyLayers);
    FreeMemorySvc (ppCtx);
    return 1;
  }

  if (pParam->uiGopSize == 0 || (pParam->uiIntraPeriod && ((pParam->uiIntraPeriod % pParam->uiGopSize) != 0))) {
    WelsLog (*ppCtx, WELS_LOG_WARNING,
             "RequestMemorySvc() failed due to invalid uiIntraPeriod(%d) (=multipler of uiGopSize(%d)!",
             pParam->uiIntraPeriod, pParam->uiGopSize);
    FreeMemorySvc (ppCtx);
    return 1;
  }

  pFinalSpatial	= &pParam->sDependencyLayers[kiNumDependencyLayers - 1];
  iMaxPicWidth	= pFinalSpatial->iFrameWidth;
  iMaxPicHeight	= pFinalSpatial->iFrameHeight;
  iCountMaxMbNum = ((15 + iMaxPicWidth) >> 4) * ((15 + iMaxPicHeight) >> 4);

  iResult = AcquireLayersNals (ppCtx, pParam, &iCountLayers, &iCountNals);
  if (iResult) {
    WelsLog (*ppCtx, WELS_LOG_WARNING, "RequestMemorySvc(), AcquireLayersNals failed(%d)!", iResult);
    FreeMemorySvc (ppCtx);
    return 1;
  }

  iNonVclLayersBsSizeCount = SSEI_BUFFER_SIZE + pParam->iNumDependencyLayer * SPS_BUFFER_SIZE +
                             (1 + pParam->iNumDependencyLayer) * PPS_BUFFER_SIZE;

  int32_t iLayerBsSize = 0;
  iIndex = 0;
  while (iIndex < pParam->iNumDependencyLayer) {
    SDLayerParam* fDlp = &pParam->sDependencyLayers[iIndex];

    fCompressRatioThr	= COMPRESS_RATIO_DECIDED_BY_RESOLUTION (fDlp->iFrameWidth, fDlp->iFrameHeight);

    iLayerBsSize = WELS_ROUND (((3 * fDlp->iFrameWidth * fDlp->iFrameHeight) >> 1) * fCompressRatioThr);
    iLayerBsSize	= WELS_ALIGN (iLayerBsSize, 4);			// 4 bytes alinged
    iVclLayersBsSizeCount += iLayerBsSize;
    ++ iIndex;
  }
#if defined(MT_ENABLED)
  iTargetSpatialBsSize = iLayerBsSize;
#endif//MT_ENABLED
  iCountBsLen = iNonVclLayersBsSizeCount + iVclLayersBsSizeCount;

  pParam->iNumRefFrame	= WELS_CLIP3 (pParam->iNumRefFrame, MIN_REF_PIC_COUNT, MAX_REFERENCE_PICTURE_COUNT_NUM);

  // Output
  (*ppCtx)->pOut = (SWelsEncoderOutput*)pMa->WelsMalloc (sizeof (SWelsEncoderOutput), "SWelsEncoderOutput");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pOut), FreeMemorySvc (ppCtx))
  (*ppCtx)->pOut->pBsBuffer		= (uint8_t*)pMa->WelsMalloc (iCountBsLen, "pOut->pBsBuffer");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pOut->pBsBuffer), FreeMemorySvc (ppCtx))
  (*ppCtx)->pOut->uiSize			= iCountBsLen;
  (*ppCtx)->pOut->sNalList		= (SWelsNalRaw*)pMa->WelsMalloc (iCountNals * sizeof (SWelsNalRaw), "pOut->sNalList");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pOut->sNalList), FreeMemorySvc (ppCtx))
  (*ppCtx)->pOut->iCountNals		= iCountNals;
  (*ppCtx)->pOut->iNalIndex		= 0;

#ifdef MT_ENABLED
  if (pParam->iMultipleThreadIdc > 1) {
    (*ppCtx)->pFrameBs			= (uint8_t*)pMa->WelsMalloc (iCountBsLen + (iTargetSpatialBsSize * ((*ppCtx)->iMaxSliceCount - 1)),
                              "pFrameBs");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pFrameBs), FreeMemorySvc (ppCtx))
    (*ppCtx)->iFrameBsSize		= iCountBsLen * (*ppCtx)->iMaxSliceCount;
  } else
#endif//MT_ENABLED
  {
    (*ppCtx)->pFrameBs			= (uint8_t*)pMa->WelsMalloc (iCountBsLen, "pFrameBs");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pFrameBs), FreeMemorySvc (ppCtx))
    (*ppCtx)->iFrameBsSize		= iCountBsLen;
  }
  (*ppCtx)->iPosBsBuffer		= 0;

#ifdef MT_ENABLED
  // for pSlice bs buffers
  if (pParam->iMultipleThreadIdc > 1 && RequestMtResource (ppCtx, pParam, iCountBsLen, iTargetSpatialBsSize)) {
    WelsLog (*ppCtx, WELS_LOG_WARNING, "RequestMemorySvc(), RequestMtResource failed!");
    FreeMemorySvc (ppCtx);
    return 1;
  }
#endif

  (*ppCtx)->pIntra4x4PredModeBlocks = static_cast<int8_t*>
                                      (pMa->WelsMallocz (iCountMaxMbNum * INTRA_4x4_MODE_NUM, "pIntra4x4PredModeBlocks"));
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pIntra4x4PredModeBlocks), FreeMemorySvc (ppCtx))

  (*ppCtx)->pNonZeroCountBlocks = static_cast<int8_t*>
                                  (pMa->WelsMallocz (iCountMaxMbNum * MB_LUMA_CHROMA_BLOCK4x4_NUM, "pNonZeroCountBlocks"));
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pNonZeroCountBlocks), FreeMemorySvc (ppCtx))

  (*ppCtx)->pMvUnitBlock4x4 = static_cast<SMVUnitXY*>
                              (pMa->WelsMallocz (iCountMaxMbNum * 2 * MB_BLOCK4x4_NUM * sizeof (SMVUnitXY), "pMvUnitBlock4x4"));
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pMvUnitBlock4x4), FreeMemorySvc (ppCtx))

  (*ppCtx)->pRefIndexBlock4x4 = static_cast<int8_t*>
                                (pMa->WelsMallocz (iCountMaxMbNum * 2 * MB_BLOCK8x8_NUM * sizeof (int8_t), "pRefIndexBlock4x4"));
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pRefIndexBlock4x4), FreeMemorySvc (ppCtx))

  (*ppCtx)->pSadCostMb	= static_cast<int32_t*>
                          (pMa->WelsMallocz (iCountMaxMbNum * sizeof (int32_t), "pSadCostMb"));
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pSadCostMb), FreeMemorySvc (ppCtx))

  (*ppCtx)->bEncCurFrmAsIdrFlag = true;  // make sure first frame is IDR
  (*ppCtx)->iGlobalQp				= 26;	// global qp in default

  (*ppCtx)->pLtr = (SLTRState*)pMa->WelsMalloc (kiNumDependencyLayers * sizeof (SLTRState), "SLTRState");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pLtr), FreeMemorySvc (ppCtx))
  int32_t i = 0;
  for (i = 0; i < kiNumDependencyLayers; i++) {
    ResetLtrState (& (*ppCtx)->pLtr[i]);
  }

  (*ppCtx)->ppRefPicListExt	= (SRefList**)pMa->WelsMalloc (kiNumDependencyLayers * sizeof (SRefList*), "ppRefPicListExt");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->ppRefPicListExt), FreeMemorySvc (ppCtx))

  // pSlice context list
  (*ppCtx)->pSliceCtxList	= (SSliceCtx*)pMa->WelsMallocz (kiNumDependencyLayers * sizeof (SSliceCtx), "pSliceCtxList");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pSliceCtxList), FreeMemorySvc (ppCtx))

  (*ppCtx)->ppDqLayerList	= (SDqLayer**)pMa->WelsMalloc (kiNumDependencyLayers * sizeof (SDqLayer*), "ppDqLayerList");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->ppDqLayerList), FreeMemorySvc (ppCtx))

  // stride tables
  if (AllocStrideTables (ppCtx, kiNumDependencyLayers)) {
    WelsLog (*ppCtx, WELS_LOG_WARNING, "RequestMemorySvc(), AllocStrideTables failed!");
    FreeMemorySvc (ppCtx);
    return 1;
  }

  //Rate control module memory allocation
  // only malloc once for RC pData, 12/14/2009
  (*ppCtx)->pWelsSvcRc = (SWelsSvcRc*)pMa->WelsMallocz (kiNumDependencyLayers * sizeof (SWelsSvcRc), "pWelsSvcRc");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pWelsSvcRc), FreeMemorySvc (ppCtx))
  //End of Rate control module memory allocation

  //pVaa memory allocation
  (*ppCtx)->pVaa	= (SVAAFrameInfo*)pMa->WelsMallocz (sizeof (SVAAFrameInfo), "pVaa");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa), FreeMemorySvc (ppCtx))

  if ((*ppCtx)->pSvcParam->bEnableAdaptiveQuant) { //malloc mem
    (*ppCtx)->pVaa->sAdaptiveQuantParam.pMotionTextureUnit   = static_cast<SMotionTextureUnit*>
        (pMa->WelsMallocz (iCountMaxMbNum * sizeof (SMotionTextureUnit), "pVaa->sAdaptiveQuantParam.pMotionTextureUnit"));
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->sAdaptiveQuantParam.pMotionTextureUnit), FreeMemorySvc (ppCtx))
    (*ppCtx)->pVaa->sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp   = static_cast<int8_t*>
        (pMa->WelsMallocz (iCountMaxMbNum * sizeof (int8_t), "pVaa->sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp"));
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp),
                                FreeMemorySvc (ppCtx))
  }

  (*ppCtx)->pVaa->pVaaBackgroundMbFlag = (int8_t*)pMa->WelsMallocz (iCountMaxMbNum * sizeof (int8_t),
                                         "pVaa->vaa_skip_mb_flag");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->pVaaBackgroundMbFlag), FreeMemorySvc (ppCtx))

  (*ppCtx)->pVaa->sVaaCalcInfo.pSad8x8 = static_cast<int32_t (*)[4]>
                                         (pMa->WelsMallocz (iCountMaxMbNum * 4 * sizeof (int32_t), "pVaa->sVaaCalcInfo.sad8x8"));
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->sVaaCalcInfo.pSad8x8), FreeMemorySvc (ppCtx))
  (*ppCtx)->pVaa->sVaaCalcInfo.pSsd16x16 = static_cast<int32_t*>
      (pMa->WelsMallocz (iCountMaxMbNum * sizeof (int32_t), "pVaa->sVaaCalcInfo.pSsd16x16"));
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->sVaaCalcInfo.pSsd16x16), FreeMemorySvc (ppCtx))
  (*ppCtx)->pVaa->sVaaCalcInfo.pSum16x16 = static_cast<int32_t*>
      (pMa->WelsMallocz (iCountMaxMbNum * sizeof (int32_t), "pVaa->sVaaCalcInfo.pSum16x16"));
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->sVaaCalcInfo.pSum16x16), FreeMemorySvc (ppCtx))
  (*ppCtx)->pVaa->sVaaCalcInfo.pSumOfSquare16x16 = static_cast<int32_t*>
      (pMa->WelsMallocz (iCountMaxMbNum * sizeof (int32_t), "pVaa->sVaaCalcInfo.pSumOfSquare16x16"));
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->sVaaCalcInfo.pSumOfSquare16x16), FreeMemorySvc (ppCtx))

  if ((*ppCtx)->pSvcParam->bEnableBackgroundDetection) { //BGD control
    (*ppCtx)->pVaa->sVaaCalcInfo.pSumOfDiff8x8 = static_cast<int32_t (*)[4]>
        (pMa->WelsMallocz (iCountMaxMbNum * 4 * sizeof (int32_t), "pVaa->sVaaCalcInfo.sd_16x16"));
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->sVaaCalcInfo.pSumOfDiff8x8), FreeMemorySvc (ppCtx))
    (*ppCtx)->pVaa->sVaaCalcInfo.pMad8x8 = static_cast<uint8_t (*)[4]>
                                           (pMa->WelsMallocz (iCountMaxMbNum * 4 * sizeof (uint8_t), "pVaa->sVaaCalcInfo.mad_16x16"));
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->sVaaCalcInfo.pMad8x8), FreeMemorySvc (ppCtx))
  }

  //End of pVaa memory allocation

  iResult = InitDqLayers (ppCtx);
  if (iResult) {
    WelsLog (*ppCtx, WELS_LOG_WARNING, "RequestMemorySvc(), InitDqLayers failed(%d)!", iResult);
    FreeMemorySvc (ppCtx);
    return iResult;
  }

  if (InitMbListD (ppCtx)) {
    WelsLog (*ppCtx, WELS_LOG_WARNING, "RequestMemorySvc(), InitMbListD failed!");
    FreeMemorySvc (ppCtx);
    return 1;
  }

  (*ppCtx)->pMvdCostTableInter = (uint16_t*)pMa->WelsMallocz (52 * kuiMvdCacheAlginedSize, "pMvdCostTableInter");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pMvdCostTableInter), FreeMemorySvc (ppCtx))
  MvdCostInit ((*ppCtx)->pMvdCostTableInter, kuiMvdInterTableSize);  //should put to a better place?

  if ((*ppCtx)->ppRefPicListExt[0] != NULL && (*ppCtx)->ppRefPicListExt[0]->pRef[0] != NULL)
    (*ppCtx)->pDecPic				= (*ppCtx)->ppRefPicListExt[0]->pRef[0];
  else
    (*ppCtx)->pDecPic				= NULL;	// error here

  (*ppCtx)->pSps				= & (*ppCtx)->pSpsArray[0];
  (*ppCtx)->pPps				= & (*ppCtx)->pPPSArray[0];

  return 0;
}


/*!
 * \brief	free memory	in SVC core encoder
 * \pParam	pEncCtx		sWelsEncCtx*
 * \return	none
 */
void FreeMemorySvc (sWelsEncCtx** ppCtx) {
  if (NULL != *ppCtx) {
    sWelsEncCtx* pCtx	= *ppCtx;
    CMemoryAlign* pMa			= pCtx->pMemAlign;
    SWelsSvcCodingParam* pParam = pCtx->pSvcParam;
    int32_t ilayer				= 0;

    // SStrideTables
    if (NULL != pCtx->pStrideTab) {
      if (NULL != pCtx->pStrideTab->pStrideDecBlockOffset[0][1]) {
        pMa->WelsFree (pCtx->pStrideTab->pStrideDecBlockOffset[0][1], "pBase");
        pCtx->pStrideTab->pStrideDecBlockOffset[0][1] = NULL;
      }
      pMa->WelsFree (pCtx->pStrideTab, "SStrideTables");
      pCtx->pStrideTab = NULL;
    }
    // pDq idc map
    if (NULL != pCtx->pDqIdcMap) {
      pMa->WelsFree (pCtx->pDqIdcMap, "pDqIdcMap");
      pCtx->pDqIdcMap = NULL;
    }

    if (NULL != pCtx->pOut) {
      // bs pBuffer
      if (NULL != pCtx->pOut->pBsBuffer) {
        pMa->WelsFree (pCtx->pOut->pBsBuffer, "pOut->pBsBuffer");
        pCtx->pOut->pBsBuffer = NULL;
      }
      // NALs list
      if (NULL != pCtx->pOut->sNalList) {
        pMa->WelsFree (pCtx->pOut->sNalList, "pOut->sNalList");
        pCtx->pOut->sNalList = NULL;
      }
      pMa->WelsFree (pCtx->pOut, "SWelsEncoderOutput");
      pCtx->pOut = NULL;
    }

#ifdef MT_ENABLED
    if (pParam != NULL && pParam->iMultipleThreadIdc > 1)
      ReleaseMtResource (ppCtx);
#endif//MT_ENABLED

    // frame bitstream pBuffer
    if (NULL != pCtx->pFrameBs) {
      pMa->WelsFree (pCtx->pFrameBs, "pFrameBs");
      pCtx->pFrameBs = NULL;
    }

    // pSpsArray
    if (NULL != pCtx->pSpsArray) {
      pMa->WelsFree (pCtx->pSpsArray, "pSpsArray");
      pCtx->pSpsArray = NULL;
    }
    // pPPSArray
    if (NULL != pCtx->pPPSArray) {
      pMa->WelsFree (pCtx->pPPSArray, "pPPSArray");
      pCtx->pPPSArray = NULL;
    }
    // subset_sps_array
    if (NULL != pCtx->pSubsetArray) {
      pMa->WelsFree (pCtx->pSubsetArray, "pSubsetArray");
      pCtx->pSubsetArray = NULL;
    }

    if (NULL != pCtx->pIntra4x4PredModeBlocks) {
      pMa->WelsFree (pCtx->pIntra4x4PredModeBlocks, "pIntra4x4PredModeBlocks");
      pCtx->pIntra4x4PredModeBlocks = NULL;
    }

    if (NULL != pCtx->pNonZeroCountBlocks) {
      pMa->WelsFree (pCtx->pNonZeroCountBlocks, "pNonZeroCountBlocks");
      pCtx->pNonZeroCountBlocks = NULL;
    }

    if (NULL != pCtx->pMvUnitBlock4x4) {
      pMa->WelsFree (pCtx->pMvUnitBlock4x4, "pMvUnitBlock4x4");
      pCtx->pMvUnitBlock4x4	= NULL;
    }

    if (NULL != pCtx->pRefIndexBlock4x4) {
      pMa->WelsFree (pCtx->pRefIndexBlock4x4, "pRefIndexBlock4x4");
      pCtx->pRefIndexBlock4x4	= NULL;
    }

    if (NULL != pCtx->ppMbListD) {
      if (NULL != pCtx->ppMbListD[0]) {
        pMa->WelsFree (pCtx->ppMbListD[0], "ppMbListD[0]");
        (*ppCtx)->ppMbListD[0] = NULL;
      }
      pMa->WelsFree (pCtx->ppMbListD, "ppMbListD");
      pCtx->ppMbListD = NULL;
    }

    if (NULL != pCtx->pSadCostMb) {
      pMa->WelsFree (pCtx->pSadCostMb, "pSadCostMb");
      pCtx->pSadCostMb = NULL;
    }

    // SLTRState
    if (NULL != pCtx->pLtr) {
      pMa->WelsFree (pCtx->pLtr, "SLTRState");
      pCtx->pLtr = NULL;
    }

    // pDq layers list
    ilayer = 0;
    if (NULL != pCtx->ppDqLayerList && pParam != NULL) {
      while (ilayer < pParam->iNumDependencyLayer) {
        SDqLayer* pDq	= pCtx->ppDqLayerList[ilayer];
        SDLayerParam* pDlp = &pCtx->pSvcParam->sDependencyLayers[ilayer];
        const BOOL_T kbIsDynamicSlicing = (SM_DYN_SLICE == pDlp->sMso.uiSliceMode);

        // pDq layers
        if (NULL != pDq) {
          if (NULL != pDq->sLayerInfo.pSliceInLayer) {
            int32_t iSliceIdx = 0;
            int32_t iSliceNum = GetInitialSliceNum (pDq->iMbWidth, pDq->iMbHeight, &pDlp->sMso);
            if (iSliceNum < 1)
              iSliceNum = 1;
            while (iSliceIdx < iSliceNum) {
              SSlice* pSlice = &pDq->sLayerInfo.pSliceInLayer[iSliceIdx];
              FreeMbCache (&pSlice->sMbCacheInfo, pMa);
              ++ iSliceIdx;
            }
            pMa->WelsFree (pDq->sLayerInfo.pSliceInLayer, "pSliceInLayer");
            pDq->sLayerInfo.pSliceInLayer = NULL;
          }
          if (kbIsDynamicSlicing) {
            pMa->WelsFree (pDq->pNumSliceCodedOfPartition, "pNumSliceCodedOfPartition");
            pDq->pNumSliceCodedOfPartition	= NULL;
            pMa->WelsFree (pDq->pLastCodedMbIdxOfPartition, "pLastCodedMbIdxOfPartition");
            pDq->pLastCodedMbIdxOfPartition	= NULL;
            pMa->WelsFree (pDq->pLastMbIdxOfPartition, "pLastMbIdxOfPartition");
            pDq->pLastMbIdxOfPartition = NULL;
          }

          pMa->WelsFree (pDq, "pDq");
          pDq = NULL;
          pCtx->ppDqLayerList[ilayer] = NULL;
        }
        ++ ilayer;
      }
      pMa->WelsFree (pCtx->ppDqLayerList, "ppDqLayerList");
      pCtx->ppDqLayerList = NULL;
    }
    FreeSpatialPictures (pCtx);

    // reference picture list extension
    if (NULL != pCtx->ppRefPicListExt && pParam != NULL) {
      ilayer = 0;
      while (ilayer < pParam->iNumDependencyLayer) {
        SRefList* pRefList		= pCtx->ppRefPicListExt[ilayer];
        if (NULL != pRefList) {
          int32_t iRef = 0;
          do {
            if (pRefList->pRef[iRef] != NULL) {
              FreePicture (pMa, &pRefList->pRef[iRef]);
            }
            ++ iRef;
          } while (iRef < 1 + pParam->iNumRefFrame);

          pMa->WelsFree (pCtx->ppRefPicListExt[ilayer], "ppRefPicListExt[]");
          pCtx->ppRefPicListExt[ilayer] = NULL;
        }
        ++ ilayer;
      }

      pMa->WelsFree (pCtx->ppRefPicListExt, "ppRefPicListExt");
      pCtx->ppRefPicListExt = NULL;
    }

    // pSlice context list
    if (NULL != pCtx->pSliceCtxList && pParam != NULL) {
      ilayer = 0;
      while (ilayer < pParam->iNumDependencyLayer) {
        SSliceCtx* pSliceCtx	= &pCtx->pSliceCtxList[ilayer];
        if (NULL != pSliceCtx)
          UninitSlicePEncCtx (pSliceCtx, pMa);
        ++ ilayer;
      }
      pMa->WelsFree (pCtx->pSliceCtxList, "pSliceCtxList");
      pCtx->pSliceCtxList = NULL;
    }

    // VAA
    if (NULL != pCtx->pVaa) {
      if (pCtx->pSvcParam->bEnableAdaptiveQuant) { //free mem
        pMa->WelsFree (pCtx->pVaa->sAdaptiveQuantParam.pMotionTextureUnit, "pVaa->sAdaptiveQuantParam.pMotionTextureUnit");
        pCtx->pVaa->sAdaptiveQuantParam.pMotionTextureUnit = NULL;
        pMa->WelsFree (pCtx->pVaa->sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp,
                       "pVaa->sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp");
        pCtx->pVaa->sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp = NULL;
      }

      pMa->WelsFree (pCtx->pVaa->pVaaBackgroundMbFlag, "pVaa->pVaaBackgroundMbFlag");
      pCtx->pVaa->pVaaBackgroundMbFlag	= NULL;
      pMa->WelsFree (pCtx->pVaa->sVaaCalcInfo.pSad8x8, "pVaa->sVaaCalcInfo.sad8x8");
      pCtx->pVaa->sVaaCalcInfo.pSad8x8		= NULL;
      pMa->WelsFree (pCtx->pVaa->sVaaCalcInfo.pSsd16x16, "pVaa->sVaaCalcInfo.pSsd16x16");
      pCtx->pVaa->sVaaCalcInfo.pSsd16x16	= NULL;
      pMa->WelsFree (pCtx->pVaa->sVaaCalcInfo.pSum16x16, "pVaa->sVaaCalcInfo.pSum16x16");
      pCtx->pVaa->sVaaCalcInfo.pSum16x16	= NULL;
      pMa->WelsFree (pCtx->pVaa->sVaaCalcInfo.pSumOfSquare16x16, "pVaa->sVaaCalcInfo.pSumOfSquare16x16");
      pCtx->pVaa->sVaaCalcInfo.pSumOfSquare16x16		= NULL;

      if (pCtx->pSvcParam->bEnableBackgroundDetection) { //BGD control
        pMa->WelsFree (pCtx->pVaa->sVaaCalcInfo.pSumOfDiff8x8, "pVaa->sVaaCalcInfo.pSumOfDiff8x8");
        pCtx->pVaa->sVaaCalcInfo.pSumOfDiff8x8	= NULL;
        pMa->WelsFree (pCtx->pVaa->sVaaCalcInfo.pMad8x8, "pVaa->sVaaCalcInfo.pMad8x8");
        pCtx->pVaa->sVaaCalcInfo.pMad8x8	= NULL;
      }

      pMa->WelsFree (pCtx->pVaa, "pVaa");
      pCtx->pVaa = NULL;
    }

    WelsRcFreeMemory (pCtx);
    // rate control module memory free
    if (NULL != pCtx->pWelsSvcRc) {
      pMa->WelsFree (pCtx->pWelsSvcRc, "pWelsSvcRc");
      pCtx->pWelsSvcRc = NULL;
    }

    /* MVD cost tables for Inter */
    if (NULL != pCtx->pMvdCostTableInter) {
      pMa->WelsFree (pCtx->pMvdCostTableInter, "pMvdCostTableInter");
      pCtx->pMvdCostTableInter = NULL;
    }

#ifdef ENABLE_TRACE_FILE
    if (NULL != pCtx->pFileLog) {
      fclose (pCtx->pFileLog);
      pCtx->pFileLog	= NULL;
    }
    pCtx->uiSizeLog	= 0;
#endif//ENABLE_TRACE_FILE

    FreeCodingParam (&pCtx->pSvcParam, pMa);
    if (NULL != pCtx->pFuncList) {
      pMa->WelsFree (pCtx->pFuncList, "SWelsFuncPtrList");
      pCtx->pFuncList = NULL;
    }

#if defined(MEMORY_MONITOR)
    assert (pMa->WelsGetMemoryUsage() == 0);	// ensure all memory free well
#endif//MEMORY_MONITOR

    if ((*ppCtx)->pMemAlign != NULL) {
      WelsLog (NULL, WELS_LOG_INFO, "FreeMemorySvc(), verify memory usage (%d bytes) after free..\n",
               (*ppCtx)->pMemAlign->WelsGetMemoryUsage());
      delete (*ppCtx)->pMemAlign;
      (*ppCtx)->pMemAlign = NULL;
    }

    free (*ppCtx);
    *ppCtx = NULL;
  }
}

int32_t InitSliceSettings (SWelsSvcCodingParam* pCodingParam, const int32_t kiCpuCores, int16_t* pMaxSliceCount) {
  int32_t iSpatialIdx = 0, iSpatialNum = pCodingParam->iNumDependencyLayer;
  int16_t iMaxSliceCount = 0;

  do {
    SDLayerParam* pDlp				= &pCodingParam->sDependencyLayers[iSpatialIdx];
    SMulSliceOption* pMso			= &pDlp->sMso;
    SSliceArgument* pSlcArg			= &pMso->sSliceArgument;
    const int32_t kiMbWidth			= (pDlp->iFrameWidth + 15) >> 4;
    const int32_t kiMbHeight			= (pDlp->iFrameHeight + 15) >> 4;
    const int32_t kiMbNumInFrame	= kiMbWidth * kiMbHeight;
#if defined(MT_ENABLED)
#if defined(DYNAMIC_SLICE_ASSIGN)
    int32_t iSliceNum				= (SM_FIXEDSLCNUM_SLICE == pMso->uiSliceMode
                               || SM_DYN_SLICE == pMso->uiSliceMode) ? kiCpuCores :
                              pSlcArg->iSliceNum; // uiSliceNum per input has been validated at ParamValidationExt()
#else//!DYNAMIC_SLICE_ASSIGN
    int32_t iSliceNum				= (SM_DYN_SLICE == pMso->uiSliceMode) ? kiCpuCores :
                              pSlcArg->uiSliceNum; // uiSliceNum per input has been validated at ParamValidationExt()
#endif//DYNAMIC_SLICE_ASSIGN
#else//!MT_ENABLED
    int16_t iSliceNum				= pSlcArg->iSliceNum; // uiSliceNum per input has been validated at ParamValidationExt()
#endif//MT_ENABLED

    // NOTE: Per design, in case MT/DYNAMIC_SLICE_ASSIGN enabled, for SM_FIXEDSLCNUM_SLICE mode,
    // uiSliceNum of current spatial layer settings equals to uiCpuCores number; SM_DYN_SLICE mode,
    // uiSliceNum intials as uiCpuCores also, stay tuned dynamically slicing in future
    pSlcArg->iSliceNum	= iSliceNum;	// used fixed one

    switch (pMso->uiSliceMode) {
    case SM_DYN_SLICE:
      iMaxSliceCount	= AVERSLICENUM_CONSTRAINT;
//#ifndef MT_ENABLED
      break;	// go through for MT_ENABLED & SM_DYN_SLICE?
//#endif//MT_ENABLED
    case SM_FIXEDSLCNUM_SLICE:
      if (iSliceNum > iMaxSliceCount)
        iMaxSliceCount = iSliceNum;
      // need perform check due uiSliceNum might change, although has been initialized somewhere outside
      if (pCodingParam->bEnableRc) {
        GomValidCheckSliceMbNum (kiMbWidth, kiMbHeight, pSlcArg);
      } else {
        CheckFixedSliceNumMultiSliceSetting (kiMbNumInFrame, pSlcArg);
      }
      break;
    case SM_SINGLE_SLICE:
      if (iSliceNum > iMaxSliceCount)
        iMaxSliceCount = iSliceNum;
      break;
    case SM_RASTER_SLICE:
      if (iSliceNum > iMaxSliceCount)
        iMaxSliceCount = iSliceNum;
      break;
    case SM_ROWMB_SLICE:
      if (iSliceNum > iMaxSliceCount)
        iMaxSliceCount = iSliceNum;
      break;
    default:
      break;
    }

    ++ iSpatialIdx;
  } while (iSpatialIdx < iSpatialNum);

#ifdef MT_ENABLED
  pCodingParam->iCountThreadsNum				= WELS_MIN (kiCpuCores, iMaxSliceCount);
  pCodingParam->iMultipleThreadIdc	= pCodingParam->iCountThreadsNum;
#else
  pCodingParam->iMultipleThreadIdc	= 1;
  pCodingParam->iCountThreadsNum				= 1;
#endif//MT_ENABLED

#ifndef WELS_TESTBED	// for product release and non-SGE testing

  if (kiCpuCores < 2) {	// single CPU core, make no sense for MT parallelization
    pCodingParam->iMultipleThreadIdc	= 1;
    pCodingParam->iCountThreadsNum				= 1;
  }
#endif

  *pMaxSliceCount					= iMaxSliceCount;

  return 0;
}

/*!
 * \brief	log output for cpu features/capabilities
 */
void OutputCpuFeaturesLog (uint32_t uiCpuFeatureFlags, uint32_t uiCpuCores, int32_t iCacheLineSize) {
  // welstracer output
  WelsLog (NULL, WELS_LOG_INFO, "WELS CPU features/capacities (0x%x) detected: \t"	\
           "HTT:      %c, "	\
           "MMX:      %c, "	\
           "MMXEX:    %c, "	\
           "SSE:      %c, "	\
           "SSE2:     %c, "	\
           "SSE3:     %c, "	\
           "SSSE3:    %c, "	\
           "SSE4.1:   %c, "	\
           "SSE4.2:   %c, "	\
           "AVX:      %c, "	\
           "FMA:      %c, "	\
           "X87-FPU:  %c, "	\
           "3DNOW:    %c, "	\
           "3DNOWEX:  %c, "	\
           "ALTIVEC:  %c, "	\
           "CMOV:     %c, "	\
           "MOVBE:    %c, "	\
           "AES:      %c, "	\
           "NUMBER OF LOGIC PROCESSORS ON CHIP: %d, "	\
           "CPU CACHE LINE SIZE (BYTES):        %d\n",
           uiCpuFeatureFlags,
           (uiCpuFeatureFlags & WELS_CPU_HTT) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_MMX) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_MMXEXT) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSE) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSE2) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSE3) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSSE3) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSE41) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSE42) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_AVX) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_FMA) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_FPU) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_3DNOW) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_3DNOWEXT) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_ALTIVEC) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_CMOV) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_MOVBE) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_AES) ? 'Y' : 'N',
           uiCpuCores,
           iCacheLineSize);

#ifdef _DEBUG	// output at console & _debug
  fprintf (stderr, "WELS CPU features/capacities (0x%x) detected: \n"	\
           "HTT:      %c, "	\
           "MMX:      %c, "	\
           "MMXEX:    %c, "	\
           "SSE:      %c, "	\
           "SSE2:     %c, "	\
           "SSE3:     %c, "	\
           "SSSE3:    %c, "	\
           "SSE4.1:   %c, "	\
           "SSE4.2:   %c, "	\
           "AVX:      %c, "	\
           "FMA:      %c, "	\
           "X87-FPU:  %c, "	\
           "3DNOW:    %c, "	\
           "3DNOWEX:  %c, "	\
           "ALTIVEC:  %c, "	\
           "CMOV:     %c, "	\
           "MOVBE:    %c, "	\
           "AES:      %c, "	\
           "NUMBER OF LOGIC PROCESSORS ON CHIP: %d, "	\
           "CPU CACHE LINE SIZE (BYTES):        %d\n",
           uiCpuFeatureFlags,
           (uiCpuFeatureFlags & WELS_CPU_HTT) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_MMX) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_MMXEXT) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSE) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSE2) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSE3) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSSE3) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSE41) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_SSE42) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_AVX) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_FMA) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_FPU) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_3DNOW) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_3DNOWEXT) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_ALTIVEC) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_CMOV) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_MOVBE) ? 'Y' : 'N',
           (uiCpuFeatureFlags & WELS_CPU_AES) ? 'Y' : 'N',
           uiCpuCores,
           iCacheLineSize);
#endif//_DEBUG
}

/*!
 * \brief	initialize Wels avc encoder core library
 * \pParam	ppCtx		sWelsEncCtx**
 * \pParam	pParam		SWelsSvcCodingParam*
 * \return	successful - 0; otherwise none 0 for failed
 */
int32_t WelsInitEncoderExt (sWelsEncCtx** ppCtx, SWelsSvcCodingParam* pCodingParam) {
  sWelsEncCtx* pCtx		= NULL;
  int32_t	iRet					= 0;
  uint32_t uiCpuFeatureFlags		= 0;	// CPU features
  int32_t uiCpuCores				=
    1;	// number of logic processors on physical processor package, one logic processor means HTT not supported
  int32_t iCacheLineSize			= 16;	// on chip cache line size in byte
  int16_t iSliceNum				= 1;	// number of slices used

  if (NULL == ppCtx || NULL == pCodingParam) {
    WelsLog (NULL, WELS_LOG_ERROR, "WelsInitEncoderExt(), NULL == ppCtx(0x%p) or NULL == pCodingParam(0x%p).\n",
             (void*)ppCtx, (void*)pCodingParam);
    return 1;
  }

  iRet	=	ParamValidationExt (pCodingParam);
  if (iRet != 0) {
    WelsLog (NULL, WELS_LOG_ERROR, "WelsInitEncoderExt(), ParamValidationExt failed return %d.\n", iRet);
    return iRet;
  }

  // for cpu features detection, Only detect once??
#ifdef X86_ASM
  uiCpuFeatureFlags	= WelsCPUFeatureDetect (&uiCpuCores);	// detect cpu capacity features
  if (uiCpuFeatureFlags & WELS_CPU_CACHELINE_128)
    iCacheLineSize = 128;
  else if (uiCpuFeatureFlags & WELS_CPU_CACHELINE_64)
    iCacheLineSize = 64;
  else if (uiCpuFeatureFlags & WELS_CPU_CACHELINE_32)
    iCacheLineSize	= 32;
  else if (uiCpuFeatureFlags & WELS_CPU_CACHELINE_16)
    iCacheLineSize	= 16;
  OutputCpuFeaturesLog (uiCpuFeatureFlags, uiCpuCores, iCacheLineSize);
#else
  iCacheLineSize	= 16;	// 16 bytes aligned in default
#endif//X86_ASM

#ifndef WELS_TESTBED

#if defined(MT_ENABLED) && defined(DYNAMIC_DETECT_CPU_CORES)
  if (pCodingParam->iMultipleThreadIdc > 0)
    uiCpuCores = pCodingParam->iMultipleThreadIdc;
  else {
    if (uiCpuFeatureFlags ==
        0)	// cpuid not supported, use high level system API as followed to detect number of pysical/logic processor
      uiCpuCores = DynamicDetectCpuCores();
    // So far so many cpu cores up to MAX_THREADS_NUM mean for server platforms,
    // for client application here it is constrained by maximal to MAX_THREADS_NUM
    if (uiCpuCores > MAX_THREADS_NUM)	// MAX_THREADS_NUM
      uiCpuCores	= MAX_THREADS_NUM;	// MAX_THREADS_NUM
    else if (uiCpuCores < 1)	// just for safe
      uiCpuCores	= 1;
  }
#endif//MT_ENABLED && DYNAMIC_DETECT_CPU_CORES

#else//WELS_TESTBED

  uiCpuCores	= pCodingParam->iMultipleThreadIdc;	// assigned uiCpuCores from iMultipleThreadIdc from SGE testing

#endif//WELS_TESTBED

  uiCpuCores	= WELS_CLIP3 (uiCpuCores, 1, MAX_THREADS_NUM);

  if (InitSliceSettings (pCodingParam, uiCpuCores, &iSliceNum)) {
    WelsLog (NULL, WELS_LOG_ERROR, "WelsInitEncoderExt(), InitSliceSettings failed.\n");
    return 1;
  }

  *ppCtx	= NULL;

  pCtx	= static_cast<sWelsEncCtx*> (malloc (sizeof (sWelsEncCtx)));

  WELS_VERIFY_RETURN_IF (1, (NULL == pCtx))
  memset (pCtx, 0, sizeof (sWelsEncCtx));

  pCtx->pMemAlign = new CMemoryAlign (iCacheLineSize);
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pCtx->pMemAlign), FreeMemorySvc (&pCtx))

  // for logs
#ifdef ENABLE_TRACE_FILE
  if (wlog == WelsLogDefault) {
    str_t fname[MAX_FNAME_LEN] = {0};

    WelsSnprintf (fname, MAX_FNAME_LEN, "%swels_svc_encoder_trace.txt",
                  pCodingParam->sTracePath);		// confirmed_safe_unsafe_usage


    pCtx->pFileLog	= WelsFopen (fname, "wt+");
    pCtx->uiSizeLog	= 0;
  }
#endif//ENABLE_TRACE_FILE

  pCodingParam->DetermineTemporalSettings();
  iRet = AllocCodingParam (&pCtx->pSvcParam, pCtx->pMemAlign, pCodingParam->iNumDependencyLayer);
  if (iRet != 0) {
    FreeMemorySvc (&pCtx);
    return iRet;
  }
  memcpy (pCtx->pSvcParam, pCodingParam, sizeof (SWelsSvcCodingParam));	// confirmed_safe_unsafe_usage

  pCtx->pFuncList = (SWelsFuncPtrList*)pCtx->pMemAlign->WelsMalloc (sizeof (SWelsFuncPtrList), "SWelsFuncPtrList");
  if (NULL == pCtx->pFuncList) {
    FreeMemorySvc (&pCtx);
    return 1;
  }
  InitFunctionPointers (pCtx->pFuncList, pCtx->pSvcParam, uiCpuFeatureFlags);

  pCtx->iActiveThreadsNum	= pCodingParam->iCountThreadsNum;
  pCtx->iMaxSliceCount	= iSliceNum;
  iRet = RequestMemorySvc (&pCtx);
  if (iRet != 0) {
    WelsLog (pCtx, WELS_LOG_ERROR, "WelsInitEncoderExt(), RequestMemorySvc failed return %d.\n", iRet);
    FreeMemorySvc (&pCtx);
    return iRet;
  }

#ifdef MT_ENABLED
  if (pCodingParam->iMultipleThreadIdc > 1)
    iRet = CreateSliceThreads (pCtx);
#endif

  WelsRcInitModule (pCtx,  pCtx->pSvcParam->bEnableRc ? WELS_RC_GOM : WELS_RC_DISABLE);

  pCtx->pVpp = new CWelsPreProcess ((void*)pCtx);
  if (pCtx->pVpp == NULL) {
    WelsLog (pCtx, WELS_LOG_ERROR, "WelsInitEncoderExt(), pOut of memory in case new CWelsPreProcess().\n");
    FreeMemorySvc (&pCtx);
    return iRet;
  }

#if defined(MEMORY_MONITOR)
  WelsLog (pCtx, WELS_LOG_INFO, "WelsInitEncoderExt() exit, overall memory usage: %llu bytes\n",
           static_cast<unsigned long long> (sizeof (sWelsEncCtx) /* requested size from malloc() or new operator */
               + pCtx->pMemAlign->WelsGetMemoryUsage())	/* requested size from CMemoryAlign::WelsMalloc() */
          );
#endif//MEMORY_MONITOR

  *ppCtx	= pCtx;

  WelsLog (pCtx, WELS_LOG_DEBUG, "WelsInitEncoderExt(), pCtx= 0x%p.\n", (void*)pCtx);

  return 0;
}
/*
 *
 * status information output
 */
#if defined(STAT_OUTPUT)
void StatOverallEncodingExt (sWelsEncCtx* pCtx) {
  int8_t i = 0;
  int8_t j = 0;
  for (i = 0; i < pCtx->pSvcParam->iNumDependencyLayer; i++) {
    fprintf (stdout, "\nDependency layer : %d\n", i);
    fprintf (stdout, "Quality layer : %d\n", j);
    {
      const int32_t iCount = pCtx->sStatData[i][j].sSliceData.iSliceCount[I_SLICE] +
                             pCtx->sStatData[i][j].sSliceData.iSliceCount[P_SLICE] +
                             pCtx->sStatData[i][j].sSliceData.iSliceCount[B_SLICE];
#if defined(MB_TYPES_CHECK)
      if (iCount > 0) {
        int32_t iCountNumIMb = pCtx->sStatData[i][j].sSliceData.iMbCount[I_SLICE][Intra4x4] +
                               pCtx->sStatData[i][j].sSliceData.iMbCount[I_SLICE][Intra16x16] + pCtx->sStatData[i][j].sSliceData.iMbCount[I_SLICE][7];
        int32_t iCountNumPMb	=	pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Intra4x4] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Intra16x16] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][7] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter16x16] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter16x8] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter8x16] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter8x8] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][10] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][PSkip];
        int32_t count_p_mbL0 = 	pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter16x16] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter16x8] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter8x16] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter8x8] +
                                pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][10];

        int32_t iMbCount = iCountNumIMb + iCountNumPMb;
        if (iMbCount > 0) {
          fprintf (stderr,
                   "SVC: overall Slices	MBs: %d Avg\nI4x4: %.3f%% I16x16: %.3f%% IBL: %.3f%%\nP16x16: %.3f%% P16x8: %.3f%% P8x16: %.3f%% P8x8: %.3f%% SUBP8x8: %.3f%% PSKIP: %.3f%%\nILP(All): %.3f%% ILP(PL0): %.3f%% BLSKIP(PL0): %.3f%% RP(PL0): %.3f%%\n",
                   iMbCount,
                   (100.0f * (pCtx->sStatData[i][j].sSliceData.iMbCount[I_SLICE][Intra4x4] +
                              pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Intra4x4]) / iMbCount),
                   (100.0f * (pCtx->sStatData[i][j].sSliceData.iMbCount[I_SLICE][Intra16x16] +
                              pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Intra16x16]) / iMbCount),
                   (100.0f * (pCtx->sStatData[i][j].sSliceData.iMbCount[I_SLICE][7] +
                              pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][7]) / iMbCount),
                   (100.0f * pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter16x16] / iMbCount),
                   (100.0f * pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter16x8] / iMbCount),
                   (100.0f * pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter8x16] / iMbCount),
                   (100.0f * pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][Inter8x8] / iMbCount),
                   (100.0f * pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][10] / iMbCount),
                   (100.0f * pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][PSkip] / iMbCount),
                   (100.0f * pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][11] / iMbCount),
                   (100.0f * pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][11] / count_p_mbL0),
                   (100.0f * pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][8] / count_p_mbL0),
                   (100.0f * pCtx->sStatData[i][j].sSliceData.iMbCount[P_SLICE][9] / count_p_mbL0)
                  );
        }
      }
#endif //#if defined(MB_TYPES_CHECK)

      if (iCount > 0) {
        fprintf (stdout, "SVC: overall PSNR Y: %2.3f U: %2.3f V: %2.3f kb/s: %.1f fps: %.3f\n\n",
                 (pCtx->sStatData[i][j].sQualityStat.rYPsnr[I_SLICE] + pCtx->sStatData[i][j].sQualityStat.rYPsnr[P_SLICE] +
                  pCtx->sStatData[i][j].sQualityStat.rYPsnr[B_SLICE]) / (float) (iCount),
                 (pCtx->sStatData[i][j].sQualityStat.rUPsnr[I_SLICE] + pCtx->sStatData[i][j].sQualityStat.rUPsnr[P_SLICE] +
                  pCtx->sStatData[i][j].sQualityStat.rUPsnr[B_SLICE]) / (float) (iCount),
                 (pCtx->sStatData[i][j].sQualityStat.rVPsnr[I_SLICE] + pCtx->sStatData[i][j].sQualityStat.rVPsnr[P_SLICE] +
                  pCtx->sStatData[i][j].sQualityStat.rVPsnr[B_SLICE]) / (float) (iCount),
                 1.0f * pCtx->pSvcParam->sDependencyLayers[i].fOutputFrameRate * (pCtx->sStatData[i][j].sSliceData.iSliceSize[I_SLICE] +
                     pCtx->sStatData[i][j].sSliceData.iSliceSize[P_SLICE] + pCtx->sStatData[i][j].sSliceData.iSliceSize[B_SLICE]) / (float) (
                   iCount + pCtx->pWelsSvcRc[i].iSkipFrameNum) / 1000,
                 1.0f * pCtx->pSvcParam->sDependencyLayers[i].fOutputFrameRate);

      }

    }

  }
}
#endif
/*!
 * \brief	uninitialize Wels encoder core library
 * \pParam	pEncCtx		sWelsEncCtx*
 * \return	none
 */
void WelsUninitEncoderExt (sWelsEncCtx** ppCtx) {
  if (NULL == ppCtx || NULL == *ppCtx)
    return;

  WelsLog (*ppCtx, WELS_LOG_INFO, "WelsUninitEncoderExt(), pCtx= %p, iThreadCount= %d, iMultipleThreadIdc= %d.\n",
           (void*) (*ppCtx), (*ppCtx)->pSvcParam->iCountThreadsNum, (*ppCtx)->pSvcParam->iMultipleThreadIdc);

#if defined(STAT_OUTPUT)
  StatOverallEncodingExt (*ppCtx);
#endif

#if defined(MT_ENABLED)
  if ((*ppCtx)->pSvcParam->iMultipleThreadIdc > 1 && (*ppCtx)->pSliceThreading != NULL) {
    const int32_t iThreadCount = (*ppCtx)->pSvcParam->iCountThreadsNum;
    int32_t iThreadIdx = 0;

#if defined(_WIN32)
    if ((*ppCtx)->pSliceThreading->pExitEncodeEvent != NULL) {
      do {
        if ((*ppCtx)->pSliceThreading->pThreadHandles[iThreadIdx] != NULL)	// iThreadIdx is already created successfully
          WelsEventSignal (& (*ppCtx)->pSliceThreading->pExitEncodeEvent[iThreadIdx]);
        ++ iThreadIdx;
      } while (iThreadIdx < iThreadCount);

      WelsMultipleEventsWaitAllBlocking (iThreadCount, & (*ppCtx)->pSliceThreading->pFinSliceCodingEvent[0]);

    }
#elif defined(__GNUC__)
    while (iThreadIdx < iThreadCount) {
      int res = 0;
      if ((*ppCtx)->pSliceThreading->pThreadHandles[iThreadIdx]) {
        res = WelsThreadCancel ((*ppCtx)->pSliceThreading->pThreadHandles[iThreadIdx]);
        WelsLog (*ppCtx, WELS_LOG_INFO, "WelsUninitEncoderExt(), WelsThreadCancel(pThreadHandles%d) return %d..\n", iThreadIdx,
                 res);
        res = WelsThreadJoin ((*ppCtx)->pSliceThreading->pThreadHandles[iThreadIdx]);	// waiting thread exit
        WelsLog (*ppCtx, WELS_LOG_INFO, "WelsUninitEncoderExt(), pthread_join(pThreadHandles%d) return %d..\n", iThreadIdx,
                 res);
        (*ppCtx)->pSliceThreading->pThreadHandles[iThreadIdx] = 0;
      }
#if defined(DYNAMIC_SLICE_ASSIGN) && defined(TRY_SLICING_BALANCE)
      if ((*ppCtx)->pSliceThreading->pUpdateMbListThrdHandles[iThreadIdx]) {
        res = WelsThreadCancel ((*ppCtx)->pSliceThreading->pUpdateMbListThrdHandles[iThreadIdx]);
        WelsLog (*ppCtx, WELS_LOG_INFO, "WelsUninitEncoderExt(), WelsThreadCancel(pUpdateMbListThrdHandles%d) return %d..\n",
                 iThreadIdx, res);
        res = WelsThreadJoin ((*ppCtx)->pSliceThreading->pUpdateMbListThrdHandles[iThreadIdx]);	// waiting thread exit
        WelsLog (*ppCtx, WELS_LOG_INFO, "WelsUninitEncoderExt(), pthread_join(pUpdateMbListThrdHandles%d) return %d..\n",
                 iThreadIdx, res);
        (*ppCtx)->pSliceThreading->pUpdateMbListThrdHandles[iThreadIdx] = 0;
      }
#endif//DYNAMIC_SLICE_ASSIGN && TRY_SLICING_BALANCE
      ++ iThreadIdx;
    }
#endif//WIN32
  }
#endif//MT_ENABLED

  if ((*ppCtx)->pVpp) {
    delete (*ppCtx)->pVpp;
    (*ppCtx)->pVpp = NULL;
  }
  FreeMemorySvc (ppCtx);
  *ppCtx = NULL;
}

/*!
 * \brief	get temporal level due to configuration and coding context
 */
static inline int32_t GetTemporalLevel (SDLayerParam* fDlp, const int32_t kiFrameNum, const int32_t kiGopSize) {
  const int32_t kiCodingIdx	= kiFrameNum & (kiGopSize - 1);

  return fDlp->uiCodingIdx2TemporalId[kiCodingIdx];
}

void DynslcUpdateMbNeighbourInfoListForAllSlices (SSliceCtx* pSliceCtx, SMB* pMbList) {
  const int32_t kiMbWidth			= pSliceCtx->iMbWidth;
  const int32_t kiEndMbInSlice	= pSliceCtx->iMbNumInFrame - 1;
  int32_t  iIdx					= 0;

  do {
    SMB* pMb = &pMbList[iIdx];
    uint32_t uiNeighborAvailFlag	= 0;
    const int32_t kiMbXY				= pMb->iMbXY;
    const int32_t kiMbX				= pMb->iMbX;
    const int32_t kiMbY				= pMb->iMbY;
    BOOL_T     bLeft;
    BOOL_T     bTop;
    BOOL_T     bLeftTop;
    BOOL_T     bRightTop;
    int32_t  uiSliceIdc;
    int32_t   iLeftXY, iTopXY, iLeftTopXY, iRightTopXY;

    uiSliceIdc = WelsMbToSliceIdc (pSliceCtx, kiMbXY);
    pMb->uiSliceIdc	= uiSliceIdc;
    iLeftXY = kiMbXY - 1;
    iTopXY = kiMbXY - kiMbWidth;
    iLeftTopXY = iTopXY - 1;
    iRightTopXY = iTopXY + 1;

    bLeft = (kiMbX > 0) && (uiSliceIdc == WelsMbToSliceIdc (pSliceCtx, iLeftXY));
    bTop = (kiMbY > 0) && (uiSliceIdc == WelsMbToSliceIdc (pSliceCtx, iTopXY));
    bLeftTop = (kiMbX > 0) && (kiMbY > 0) && (uiSliceIdc == WelsMbToSliceIdc (pSliceCtx, iLeftTopXY));
    bRightTop = (kiMbX < (kiMbWidth - 1)) && (kiMbY > 0) && (uiSliceIdc == WelsMbToSliceIdc (pSliceCtx, iRightTopXY));

    if (bLeft) {
      uiNeighborAvailFlag |= LEFT_MB_POS;
    }
    if (bTop) {
      uiNeighborAvailFlag |= TOP_MB_POS;
    }
    if (bLeftTop) {
      uiNeighborAvailFlag |= TOPLEFT_MB_POS;
    }
    if (bRightTop) {
      uiNeighborAvailFlag |= TOPRIGHT_MB_POS;
    }
    pMb->uiNeighborAvail	= (uint8_t)uiNeighborAvailFlag;

    ++ iIdx;
  } while (iIdx <= kiEndMbInSlice);
}

/*
 * TUNE back if number of picture partition decision algorithm based on past if available
 */
int32_t PicPartitionNumDecision (sWelsEncCtx* pCtx) {
  int32_t iPartitionNum	= 1;
#ifdef MT_ENABLED
  if (pCtx->pSvcParam->iMultipleThreadIdc > 1) {
    iPartitionNum	= pCtx->pSvcParam->iCountThreadsNum;
#if !defined(FIXED_PARTITION_ASSIGN)
    if (P_SLICE == pCtx->eSliceType)
      iPartitionNum	= 1;
#endif//!FIXED_PARTITION_ASSIGN
  }
  return iPartitionNum;
#else
  return iPartitionNum;
#endif//MT_ENABLED
}

#if defined(MT_ENABLED)
void WelsInitCurrentQBLayerMltslc (sWelsEncCtx* pCtx) {
  //pData init
  SDqLayer*		pCurDq				= pCtx->pCurDqLayer;
  SSliceCtx*	pSliceCtx			= (pCurDq->pSliceEncCtx);

  //mb_neighbor
  DynslcUpdateMbNeighbourInfoListForAllSlices (pSliceCtx, pCurDq->sMbDataP);
}

void UpdateSlicepEncCtxWithPartition (SSliceCtx* pSliceCtx, int32_t iPartitionNum) {
  const int32_t kiMbNumInFrame	= pSliceCtx->iMbNumInFrame;
  int32_t iCountMbNumPerPartition	= kiMbNumInFrame;
  int32_t iAssignableMbLeft		= kiMbNumInFrame;
  int32_t iFirstMbIdx			= 0;
  int32_t i/*, j*/;

  if (iPartitionNum <= 0)
    iPartitionNum	= 1;
  else if (iPartitionNum > AVERSLICENUM_CONSTRAINT)
    iPartitionNum	= AVERSLICENUM_CONSTRAINT;	// AVERSLICENUM_CONSTRAINT might be variable, however not fixed by MACRO
  iCountMbNumPerPartition	/= iPartitionNum;
  pSliceCtx->iSliceNumInFrame	= iPartitionNum;
  i = 0;
  while (i < iPartitionNum) {
    if (i + 1 == iPartitionNum) {
      pSliceCtx->pCountMbNumInSlice[i]	= iAssignableMbLeft;
    } else {
      pSliceCtx->pCountMbNumInSlice[i]	= iCountMbNumPerPartition;
    }
    pSliceCtx->pFirstMbInSlice[i]	=	iFirstMbIdx;

    memset (pSliceCtx->pOverallMbMap + iFirstMbIdx, (uint8_t)i, pSliceCtx->pCountMbNumInSlice[i]*sizeof (uint8_t));

    // for next partition(or pSlice)
    iFirstMbIdx	+= pSliceCtx->pCountMbNumInSlice[i];
    iAssignableMbLeft -= pSliceCtx->pCountMbNumInSlice[i];
    ++ i;
  }
}

void WelsInitCurrentDlayerMltslc (sWelsEncCtx* pCtx, int32_t iPartitionNum) {
  SDqLayer* pCurDq				= pCtx->pCurDqLayer;
  SSliceCtx* pSliceCtx		= pCurDq->pSliceEncCtx;

  UpdateSlicepEncCtxWithPartition (pSliceCtx, iPartitionNum);

  if (I_SLICE == pCtx->eSliceType) { //check if uiSliceSizeConstraint too small
#define byte_complexIMBat26 (60)
    uint8_t		iCurDid = pCtx->uiDependencyId;
    uint32_t	uiFrmByte = 0;

    if (pCtx->pSvcParam->bEnableRc) {
      //RC case
      uiFrmByte = (
                    ((uint32_t) (pCtx->pSvcParam->sDependencyLayers[iCurDid].iSpatialBitrate)
                     / (uint32_t) (pCtx->pSvcParam->sDependencyLayers[iCurDid].fInputFrameRate)) >> 3);
    } else {
      //fixed QP case
      const int32_t iTtlMbNumInFrame = pSliceCtx->iMbNumInFrame;
      int32_t iQDeltaTo26 = (26 - pCtx->pSvcParam->sDependencyLayers[iCurDid].iDLayerQp);

      uiFrmByte = (iTtlMbNumInFrame * byte_complexIMBat26);
      if (iQDeltaTo26 > 0) {
        //smaller QP than 26
        uiFrmByte = (uint32_t) (uiFrmByte * ((float)iQDeltaTo26 / 4));
      } else if (iQDeltaTo26 < 0) {
        //larger QP than 26
        iQDeltaTo26 = ((-iQDeltaTo26) >> 2);   //delta mod 4
        uiFrmByte = (uiFrmByte >> (iQDeltaTo26));   //if delta 4, byte /2
      }
    }

    //MINPACKETSIZE_CONSTRAINT
    if (pSliceCtx->uiSliceSizeConstraint
        <
        (uint32_t) (uiFrmByte//suppose 16 byte per mb at average
                    / (pSliceCtx->iMaxSliceNumConstraint))
       ) {

      WelsLog (pCtx,
               WELS_LOG_WARNING,
               "Set-SliceConstraint(%d) too small for current resolution (MB# %d) under QP/BR!\n",
               pSliceCtx->uiSliceSizeConstraint,
               pSliceCtx->iMbNumInFrame
              );
    }
  }

  WelsInitCurrentQBLayerMltslc (pCtx);
}
#else
void WelsInitCurrentQBLayerMltslc (sWelsEncCtx* pCtx) {
  //pData init
  SDqLayer*		pCurDq				= pCtx->pCurDqLayer;
  SSliceCtx*	pSliceCtx			= (pCurDq->pSliceEncCtx);
  SSlice* 			pSlice				= &pCurDq->sLayerInfo.pSliceInLayer[0];
  int32_t			iTtlMbNumInFrame = pSliceCtx->iMbNumInFrame;

  //pSliceCtx
  memset (pSliceCtx->pOverallMbMap,		0, iTtlMbNumInFrame * sizeof (uint8_t));
  memset (pSliceCtx->pCountMbNumInSlice,	0, pSliceCtx->iSliceNumInFrame * sizeof (int32_t));
  memset (pSliceCtx->pFirstMbInSlice,		0, pSliceCtx->iSliceNumInFrame * sizeof (int16_t));
  pSliceCtx->iSliceNumInFrame				= 1;//
  pSliceCtx->pCountMbNumInSlice[0]			= iTtlMbNumInFrame;

  //mb_neighbor
  DynslcUpdateMbNeighbourInfoListForAllSlices (pSliceCtx, pCurDq->sMbDataP);

  //pSlice init
  pSlice->uiSliceIdx				= 0;
  pSlice->pSliceBsa				= &pCtx->pOut->sBsWrite;
  pSlice->bDynamicSlicingSliceSizeCtrlFlag			= false;
  pSlice->uiAssumeLog2BytePerMb	= (pCtx->eSliceType == P_SLICE) ? 0 : 1;
}

void WelsInitCurrentDlayerMltslc (sWelsEncCtx* pCtx, int32_t iPartitionNum) {
  SDqLayer* pCurDq = pCtx->pCurDqLayer;
  SSliceCtx* pSliceCtx = (pCurDq->pSliceEncCtx);
  int32_t iTtlMbNumInFrame = pCurDq->iMbHeight * pCurDq->iMbWidth;

  pSliceCtx->iMbNumInFrame
    = pSliceCtx->pCountMbNumInSlice[0] = iTtlMbNumInFrame;

  if (I_SLICE == pCtx->eSliceType) { //check if uiSliceSizeConstraint too small
#define byte_complexIMBat26 (60)
    uint8_t		iCurDid = pCtx->uiDependencyId;
    uint32_t	uiFrmByte = 0;

    if (pCtx->pSvcParam->bEnableRc) {
      //RC case
      uiFrmByte = (
                    ((uint32_t) (pCtx->pSvcParam->sDependencyLayers[iCurDid].iSpatialBitrate)
                     / (uint32_t) (pCtx->pSvcParam->sDependencyLayers[iCurDid].fInputFrameRate)) >> 3);
    } else {
      //fixed QP case
      int32_t iQDeltaTo26 = (26 - pCtx->pSvcParam->sDependencyLayers[iCurDid].iDLayerQp);

      uiFrmByte = (iTtlMbNumInFrame * byte_complexIMBat26);
      if (iQDeltaTo26 > 0) {
        //smaller QP than 26
        uiFrmByte = (uint32_t) (uiFrmByte * ((float)iQDeltaTo26 / 4));
      } else if (iQDeltaTo26 < 0) {
        //larger QP than 26
        iQDeltaTo26 = ((-iQDeltaTo26) >> 2);   //delta mod 4
        uiFrmByte = (uiFrmByte >> (iQDeltaTo26));   //if delta 4, byte /2
      }
    }

    //MINPACKETSIZE_CONSTRAINT
    if (pSliceCtx->uiSliceSizeConstraint
        <
        (uint32_t) (uiFrmByte//suppose 16 byte per mb at average
                    / (pSliceCtx->iMaxSliceNumConstraint))
       ) {

      WelsLog (pCtx,
               WELS_LOG_WARNING,
               "Set-SliceConstraint(%d) too small for current resolution (MB# %d) under QP/BR!\n",
               pSliceCtx->uiSliceSizeConstraint,
               pSliceCtx->iMbNumInFrame
              );
    }
  }

  WelsInitCurrentQBLayerMltslc (pCtx);
}
#endif

/*!
 * \brief	initialize current layer
 */
void WelsInitCurrentLayer (sWelsEncCtx* pCtx,
                           const int32_t kiWidth,
                           const int32_t kiHeight) {
  SWelsSvcCodingParam* pParam	= pCtx->pSvcParam;
  SPicture* pEncPic					= pCtx->pEncPic;
  SPicture* pDecPic					= pCtx->pDecPic;
  SDqLayer* pCurDq				= pCtx->pCurDqLayer;
  SSlice* pBaseSlice				= &pCurDq->sLayerInfo.pSliceInLayer[0];
  SSlice* pSlice					= NULL;
  const uint8_t kiCurDid			= pCtx->uiDependencyId;
  const bool_t kbUseSubsetSpsFlag = (kiCurDid > BASE_DEPENDENCY_ID);
  SDLayerParam* fDlp				= &pParam->sDependencyLayers[kiCurDid];
  SNalUnitHeaderExt* pNalHdExt	= &pCurDq->sLayerInfo.sNalHeaderExt;
  SNalUnitHeader* pNalHd			= &pNalHdExt->sNalHeader;
  SDqIdc* pDqIdc						= &pCtx->pDqIdcMap[kiCurDid];
  int32_t iIdx						= 0;
  int32_t iSliceCount				= 0;

  if (NULL == pCurDq)
    return;

  pCurDq->pDecPic	= pDecPic;

  if (fDlp->sMso.uiSliceMode == SM_DYN_SLICE)	// need get extra slices for update
    iSliceCount = GetInitialSliceNum (pCurDq->iMbWidth, pCurDq->iMbHeight, &fDlp->sMso);
  else
    iSliceCount = GetCurrentSliceNum (pCurDq->pSliceEncCtx);
  assert (iSliceCount > 0);

  pBaseSlice->sSliceHeaderExt.sSliceHeader.iPpsId	= pDqIdc->iPpsId;
  pCurDq->sLayerInfo.pPpsP							=
    pBaseSlice->sSliceHeaderExt.sSliceHeader.pPps		= &pCtx->pPPSArray[pBaseSlice->sSliceHeaderExt.sSliceHeader.iPpsId];
  pBaseSlice->sSliceHeaderExt.sSliceHeader.iSpsId	= pDqIdc->iSpsId;
  if (kbUseSubsetSpsFlag) {
    pCurDq->sLayerInfo.pSubsetSpsP					= &pCtx->pSubsetArray[pDqIdc->iSpsId];
    pCurDq->sLayerInfo.pSpsP						=
      pBaseSlice->sSliceHeaderExt.sSliceHeader.pSps	= &pCurDq->sLayerInfo.pSubsetSpsP->pSps;
  } else {
    pCurDq->sLayerInfo.pSubsetSpsP					= NULL;
    pCurDq->sLayerInfo.pSpsP						=
      pBaseSlice->sSliceHeaderExt.sSliceHeader.pSps	= &pCtx->pSpsArray[pBaseSlice->sSliceHeaderExt.sSliceHeader.iSpsId];
  }

  pSlice = pBaseSlice;
  iIdx = 1;
  while (iIdx < iSliceCount) {
    ++ pSlice;
    pSlice->sSliceHeaderExt.sSliceHeader.iPpsId	= pBaseSlice->sSliceHeaderExt.sSliceHeader.iPpsId;
    pSlice->sSliceHeaderExt.sSliceHeader.pPps	= pBaseSlice->sSliceHeaderExt.sSliceHeader.pPps;
    pSlice->sSliceHeaderExt.sSliceHeader.iSpsId	= pBaseSlice->sSliceHeaderExt.sSliceHeader.iSpsId;
    pSlice->sSliceHeaderExt.sSliceHeader.pSps	= pBaseSlice->sSliceHeaderExt.sSliceHeader.pSps;
    ++ iIdx;
  }

  memset (pNalHdExt, 0, sizeof (SNalUnitHeaderExt));
  pNalHd->uiNalRefIdc					= pCtx->eNalPriority;
  pNalHd->eNalUnitType				= pCtx->eNalType;

  pNalHdExt->uiDependencyId			= kiCurDid;
  pNalHdExt->bDiscardableFlag		= (pCtx->bNeedPrefixNalFlag) ? (pNalHd->uiNalRefIdc == NRI_PRI_LOWEST) : false;
  pNalHdExt->bIdrFlag				= (pCtx->iFrameNum == 0) && ((pCtx->eNalType == NAL_UNIT_CODED_SLICE_IDR)
                              || (pCtx->eSliceType == I_SLICE));
  pNalHdExt->uiTemporalId				= pCtx->uiTemporalId;

  pBaseSlice->bSliceHeaderExtFlag	= (NAL_UNIT_CODED_SLICE_EXT == pNalHd->eNalUnitType);

  pSlice = pBaseSlice;
  iIdx = 1;
  while (iIdx < iSliceCount) {
    ++ pSlice;
    pSlice->bSliceHeaderExtFlag			= pBaseSlice->bSliceHeaderExtFlag;
    ++ iIdx;
  }

  // pEncPic pData
  pCurDq->pEncData[0]		= pEncPic->pData[0];
  pCurDq->pEncData[1]		= pEncPic->pData[1];
  pCurDq->pEncData[2]		= pEncPic->pData[2];
  pCurDq->iEncStride[0]	= pEncPic->iLineSize[0];
  pCurDq->iEncStride[1]	= pEncPic->iLineSize[1];
  pCurDq->iEncStride[2]	= pEncPic->iLineSize[2];
  // cs pData
  pCurDq->pCsData[0]		= pDecPic->pData[0];
  pCurDq->pCsData[1]		= pDecPic->pData[1];
  pCurDq->pCsData[2]		= pDecPic->pData[2];
  pCurDq->iCsStride[0]	= pDecPic->iLineSize[0];
  pCurDq->iCsStride[1]	= pDecPic->iLineSize[1];
  pCurDq->iCsStride[2]	= pDecPic->iLineSize[2];

  if (pCurDq->pRefLayer != NULL) {
    pCurDq->bBaseLayerAvailableFlag	= true;
  } else {
    pCurDq->bBaseLayerAvailableFlag	= false;
  }
}

void PreprocessSliceCoding (sWelsEncCtx* pCtx) {
  SDqLayer* pCurLayer		= pCtx->pCurDqLayer;
  const bool_t kbBaseAvail	= pCurLayer->bBaseLayerAvailableFlag;

  /* function pointers conditional assignment under sWelsEncCtx, layer_mb_enc_rec (in stack) is exclusive */

  if (P_SLICE == pCtx->eSliceType) {
    if (kbBaseAvail) {
      if (pCtx->pSvcParam->iNumDependencyLayer == (pCurLayer->sLayerInfo.sNalHeaderExt.uiDependencyId + 1)) { //
        pCtx->pFuncList->pfMotionSearch = WelsMotionEstimateSearchSad;
        pCtx->pFuncList->pfFirstIntraMode = WelsMdFirstIntraMode;
        pCtx->pFuncList->pfIntraFineMd = WelsMdIntraFinePartitionVaa;
        pCtx->pFuncList->pfInterFineMd = WelsMdInterFinePartitionVaa;
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3 = pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Sad;
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3 =
          pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Sad;
        pCtx->pFuncList->sSampleDealingFuncs.pfMdCost = pCtx->pFuncList->sSampleDealingFuncs.pfSampleSad;
      } else {
        pCtx->pFuncList->pfMotionSearch  = WelsMotionEstimateSearchSatd;
        pCtx->pFuncList->pfFirstIntraMode = WelsMdFirstIntraMode;
        pCtx->pFuncList->pfIntraFineMd = WelsMdIntraFinePartition;
        pCtx->pFuncList->pfInterFineMd = WelsMdInterFinePartition;
        pCtx->pFuncList->sSampleDealingFuncs.pfMdCost = pCtx->pFuncList->sSampleDealingFuncs.pfSampleSatd;
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3 =
          pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Satd;
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3 = pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Satd;
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3 = pCtx->pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3Satd;
      }
      pCtx->pFuncList->sSampleDealingFuncs.pfMeCost = pCtx->pFuncList->sSampleDealingFuncs.pfSampleSatd;
    } else {
      //case 3: pBase layer MD + encoding
      if (pCurLayer->sLayerInfo.sNalHeaderExt.uiDependencyId + 1 == pCtx->pSvcParam->iNumDependencyLayer) {
        pCtx->pFuncList->pfMotionSearch  = WelsMotionEstimateSearchSad;
        pCtx->pFuncList->pfFirstIntraMode = WelsMdFirstIntraMode;
        pCtx->pFuncList->pfIntraFineMd = WelsMdIntraFinePartitionVaa;
        pCtx->pFuncList->pfInterFineMd = WelsMdInterFinePartitionVaa;
        pCtx->pFuncList->sSampleDealingFuncs.pfMdCost = pCtx->pFuncList->sSampleDealingFuncs.pfSampleSad;
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3 =
          pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Sad;
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3 = pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Sad;
      } else {
        pCtx->pFuncList->pfMotionSearch  = WelsMotionEstimateSearchSatd;
        pCtx->pFuncList->pfFirstIntraMode = WelsMdFirstIntraMode;
        pCtx->pFuncList->pfIntraFineMd = WelsMdIntraFinePartition;
        pCtx->pFuncList->pfInterFineMd = WelsMdInterFinePartition;
        pCtx->pFuncList->sSampleDealingFuncs.pfMdCost = pCtx->pFuncList->sSampleDealingFuncs.pfSampleSatd;
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3 =
          pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Satd;
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3 = pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Satd;
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3 = pCtx->pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3Satd;
      }
      pCtx->pFuncList->sSampleDealingFuncs.pfMeCost = pCtx->pFuncList->sSampleDealingFuncs.pfSampleSatd;
    }
  } else if (I_SLICE == pCtx->eSliceType) {
    if (pCurLayer->sLayerInfo.sNalHeaderExt.uiDependencyId + 1 == pCtx->pSvcParam->iNumDependencyLayer) {
      pCtx->pFuncList->sSampleDealingFuncs.pfMdCost = pCtx->pFuncList->sSampleDealingFuncs.pfSampleSad;
      pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3 =
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Sad;
      pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3 = pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Sad;
      pCtx->pFuncList->pfIntraFineMd = WelsMdIntraFinePartitionVaa;
    } else {
      pCtx->pFuncList->sSampleDealingFuncs.pfMdCost = pCtx->pFuncList->sSampleDealingFuncs.pfSampleSatd;
      pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3 =
        pCtx->pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Satd;
      pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3 = pCtx->pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Satd;
      pCtx->pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3 = pCtx->pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3Satd;
      pCtx->pFuncList->pfIntraFineMd = WelsMdIntraFinePartition;
    }
  }
}

/*!
 * \brief	swap pDq layers between current pDq layer and reference pDq layer
 */

static inline void WelsSwapDqLayers (sWelsEncCtx* pCtx) {
  // swap and assign reference
  const int32_t kiDid			= pCtx->uiDependencyId;
  const int32_t kiNextDqIdx   = 1 + kiDid;

  SDqLayer* pTmpLayer			= pCtx->ppDqLayerList[kiNextDqIdx];
  SDqLayer* pRefLayer			= pCtx->pCurDqLayer;
  pCtx->pCurDqLayer				= pTmpLayer;
  pCtx->pCurDqLayer->pRefLayer	= pRefLayer;
}

/*!
 * \brief	prefetch reference picture after WelsBuildRefList
 */
static inline void PrefetchReferencePicture (sWelsEncCtx* pCtx, const EFrameType keFrameType) {
  SSlice* pSliceBase = &pCtx->pCurDqLayer->sLayerInfo.pSliceInLayer[0];
  const int32_t kiSliceCount = GetCurrentSliceNum (pCtx->pCurDqLayer->pSliceEncCtx);
  int32_t iIdx = 0;
  uint8_t uiRefIdx = -1;

  assert (kiSliceCount > 0);
  if (keFrameType != WELS_FRAME_TYPE_IDR) {
    assert (pCtx->iNumRef0 > 0);
    pCtx->pRefPic	= pCtx->pRefList0[0];	// always get item 0 due to reordering done
    pCtx->pCurDqLayer->pRefPic	= pCtx->pRefPic;
    uiRefIdx	= 0;	// reordered reference iIndex
  } else {	// safe for IDR coding
    pCtx->pRefPic					= NULL;
    pCtx->pCurDqLayer->pRefPic	= NULL;
  }

  iIdx = 0;
  while (iIdx < kiSliceCount) {
    pSliceBase->sSliceHeaderExt.sSliceHeader.uiRefIndex	= uiRefIdx;
    ++ pSliceBase;
    ++ iIdx;
  }
}


void ParasetIdAdditionIdAdjust (SParaSetOffsetVariable* sParaSetOffsetVariable, const int32_t kiCurEncoderParaSetId,
                                const uint32_t kuiMaxIdInBs) { //paraset_type = 0: SPS; =1: PPS
  //SPS_ID in avc_sps and pSubsetSps will be different using this
  //SPS_ID case example:
  //1st enter:		next_spsid_in_bs == 0; spsid == 0; delta==0;				//actual spsid_in_bs == 0
  //1st finish:		next_spsid_in_bs == 1;
  //2nd enter:	next_spsid_in_bs == 1; spsid == 0; delta==1;				//actual spsid_in_bs == 1
  //2nd finish:		next_spsid_in_bs == 2;
  //31st enter:	next_spsid_in_bs == 31; spsid == 0~2; delta==31~29;	//actual spsid_in_bs == 31
  //31st finish:	next_spsid_in_bs == 0;
  //31st enter:	next_spsid_in_bs == 0; spsid == 0~2; delta==-2~0;		//actual spsid_in_bs == 0
  //31st finish:	next_spsid_in_bs == 1;

  const int32_t kiEncId			= kiCurEncoderParaSetId;
  const uint32_t kuiPrevIdInBs	= sParaSetOffsetVariable->iParaSetIdDelta[kiEncId] + kiEncId;//mark current_id
  const bool_t* kpUsedIdPointer   = &sParaSetOffsetVariable->bUsedParaSetIdInBs[0];
  uint32_t uiNextIdInBs			= sParaSetOffsetVariable->uiNextParaSetIdToUseInBs;

#if _DEBUG
  if (0 != sParaSetOffsetVariable->iParaSetIdDelta[kiEncId])
    assert (sParaSetOffsetVariable->bUsedParaSetIdInBs[kuiPrevIdInBs]);   //sure the prev-used one was marked activated correctly
#endif
  //update current layer's pCodingParam
  sParaSetOffsetVariable->iParaSetIdDelta[kiEncId]	= uiNextIdInBs -
      kiEncId;  //for current parameter set, change its id_delta
  //write pso pData for next update:
  sParaSetOffsetVariable->bUsedParaSetIdInBs[kuiPrevIdInBs] = false;	//
  sParaSetOffsetVariable->bUsedParaSetIdInBs[uiNextIdInBs] = true;		//   update current used_id

  //prepare for next update:
  //   find the next avaibable iId
  do {
    ++uiNextIdInBs;
    if (uiNextIdInBs >= kuiMaxIdInBs) {
      uiNextIdInBs = 0;//ensure the SPS_ID wound not exceed MAX_SPS_COUNT
    }
  } while (kpUsedIdPointer[uiNextIdInBs]);

  //   update next_id
  sParaSetOffsetVariable->uiNextParaSetIdToUseInBs = uiNextIdInBs;

#if _DEBUG
  assert (!sParaSetOffsetVariable->bUsedParaSetIdInBs[uiNextIdInBs]);   //sure the next-to-use one is marked activated correctly
#endif

}

/*!
 * \brief	write all parameter sets introduced in SVC extension
 * \return	size in bytes of bitstream wrote
 */
int32_t WelsWriteParameterSets (sWelsEncCtx* pCtx, int32_t* pNalLen, int32_t* pNumNal) {
  int32_t iSize	= 0;
  int32_t iNal	= 0;
  int32_t	iIdx	= 0;
  int32_t iId	= 0;
  int32_t iCountNal	= 0;

  if (NULL == pCtx || NULL == pNalLen || NULL == pNumNal)
    return 0;

  /* write all SPS */
  iIdx = 0;
  while (iIdx < pCtx->iSpsNum) {
    SDqIdc* pDqIdc		= &pCtx->pDqIdcMap[iIdx];
    const int32_t kiDid	= pDqIdc->uiSpatialId;
    const bool_t kbUsingSubsetSps = (kiDid > BASE_DEPENDENCY_ID);

    iNal	= pCtx->pOut->iNalIndex;

    if (pCtx->pSvcParam->bEnableSpsPpsIdAddition) {
#if _DEBUG
      pCtx->sPSOVector.bEnableSpsPpsIdAddition = 1;
      assert (kiDid < MAX_DEPENDENCY_LAYER);
      assert (iIdx < MAX_DQ_LAYER_NUM);
#endif

      ParasetIdAdditionIdAdjust (& (pCtx->sPSOVector.sParaSetOffsetVariable[kbUsingSubsetSps ? PARA_SET_TYPE_SUBSETSPS :
                                    PARA_SET_TYPE_AVCSPS]),
                                 (kbUsingSubsetSps) ? (pCtx->pSubsetArray[iIdx - 1].pSps.uiSpsId) : (pCtx->pSpsArray[0].uiSpsId),
                                 MAX_SPS_COUNT);
    } else {
      memset (& (pCtx->sPSOVector), 0, sizeof (pCtx->sPSOVector));
    }

    if (kbUsingSubsetSps) {
      iId	= iIdx - 1;

      /* generate Subset SPS */
      WelsLoadNal (pCtx->pOut, NAL_UNIT_SUBSET_SPS, NRI_PRI_HIGHEST);

      WelsWriteSubsetSpsSyntax (&pCtx->pSubsetArray[iId], &pCtx->pOut->sBsWrite,
                                & (pCtx->sPSOVector.sParaSetOffsetVariable[PARA_SET_TYPE_SUBSETSPS].iParaSetIdDelta[0]));
      WelsUnloadNal (pCtx->pOut);
    } else {
      iId	= 0;

      /* generate sequence parameters set */
      WelsLoadNal (pCtx->pOut, NAL_UNIT_SPS, NRI_PRI_HIGHEST);
      WelsWriteSpsNal (&pCtx->pSpsArray[0], &pCtx->pOut->sBsWrite,
                       & (pCtx->sPSOVector.sParaSetOffsetVariable[PARA_SET_TYPE_AVCSPS].iParaSetIdDelta[0]));
      WelsUnloadNal (pCtx->pOut);
    }

    pNalLen[iCountNal] = WelsEncodeNal (&pCtx->pOut->sNalList[iNal], pCtx->pFrameBs + pCtx->iPosBsBuffer,
                                        &pNalLen[iCountNal]);

    pCtx->iPosBsBuffer	+= pNalLen[iCountNal];
    iSize				+= pNalLen[iCountNal];

    ++ iIdx;
    ++ iCountNal;
  }

  /* write all PPS */
  iIdx = 0;
  while (iIdx < pCtx->iPpsNum) {
    if (pCtx->pSvcParam->bEnableSpsPpsIdAddition) {
      //para_set_type = 2: PPS, use MAX_PPS_COUNT
      ParasetIdAdditionIdAdjust (&pCtx->sPSOVector.sParaSetOffsetVariable[PARA_SET_TYPE_PPS], pCtx->pPPSArray[iIdx].iPpsId,
                                 MAX_PPS_COUNT);
    }

    iNal	= pCtx->pOut->iNalIndex;
    /* generate picture parameter set */
    WelsLoadNal (pCtx->pOut, NAL_UNIT_PPS, NRI_PRI_HIGHEST);
    WelsWritePpsSyntax (&pCtx->pPPSArray[iIdx], &pCtx->pOut->sBsWrite, & (pCtx->sPSOVector));
    WelsUnloadNal (pCtx->pOut);

    pNalLen[iCountNal] = WelsEncodeNal (&pCtx->pOut->sNalList[iNal], pCtx->pFrameBs + pCtx->iPosBsBuffer,
                                        &pNalLen[iCountNal]);

    pCtx->iPosBsBuffer	+= pNalLen[iCountNal];
    iSize				+= pNalLen[iCountNal];

    ++ iIdx;
    ++ iCountNal;
  }

  *pNumNal = iCountNal;

  return iSize;
}

static inline int32_t AddPrefixNal (sWelsEncCtx* pCtx,
                                    SLayerBSInfo* pLayerBsInfo,
                                    int32_t* pNalLen,
                                    int32_t* pNalIdxInLayer,
                                    const EWelsNalUnitType keNalType,
                                    const EWelsNalRefIdc keNalRefIdc) {
  int32_t iPayloadSize = 0;

  if (keNalRefIdc != NRI_PRI_LOWEST) {
    WelsLoadNal (pCtx->pOut, NAL_UNIT_PREFIX, keNalRefIdc);

    WelsWriteSVCPrefixNal (&pCtx->pOut->sBsWrite, keNalRefIdc, (NAL_UNIT_CODED_SLICE_IDR == keNalType));

    WelsUnloadNal (pCtx->pOut);

    iPayloadSize	= WelsEncodeNalExt (&pCtx->pOut->sNalList[pCtx->pOut->iNalIndex - 1],
                                      &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt,
                                      pCtx->pFrameBs + pCtx->iPosBsBuffer,
                                      &pNalLen[*pNalIdxInLayer]);

    pCtx->iPosBsBuffer							+= iPayloadSize;
    pLayerBsInfo->iNalLengthInByte[*pNalIdxInLayer]	= iPayloadSize;

    (*pNalIdxInLayer) ++;
  } else { // No Prefix NAL Unit RBSP syntax here, but need add NAL Unit Header extension
    WelsLoadNal (pCtx->pOut, NAL_UNIT_PREFIX, keNalRefIdc);
    // No need write any syntax of prefix NAL Unit RBSP here
    WelsUnloadNal (pCtx->pOut);

    iPayloadSize = WelsEncodeNalExt (&pCtx->pOut->sNalList[pCtx->pOut->iNalIndex - 1],
                                     &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt,
                                     pCtx->pFrameBs + pCtx->iPosBsBuffer,
                                     &pNalLen[*pNalIdxInLayer]);

    pCtx->iPosBsBuffer							+= iPayloadSize;
    pLayerBsInfo->iNalLengthInByte[*pNalIdxInLayer]	= iPayloadSize;

    (*pNalIdxInLayer) ++;
  }

  return iPayloadSize;
}

int32_t WritePadding (sWelsEncCtx* pCtx, int32_t iLen) {
  int32_t i = 0;
  int32_t iNal	= 0;
  SBitStringAux*	pBs = NULL;
  int32_t iNalLen;
  int32_t iSize = 0;

  iNal	= pCtx->pOut->iNalIndex;
  pBs	=	&pCtx->pOut->sBsWrite;	// SBitStringAux instance for non VCL NALs decoding

  if ((pBs->pBufEnd - pBs->pBufPtr) < iLen || iNal >= pCtx->pOut->iCountNals) {
#if GOM_TRACE_FLAG
    WelsLog (pCtx, WELS_LOG_ERROR,
             "[RC] paddingcal pBuffer overflow, bufferlen=%lld, paddinglen=%d, iNalIdx= %d, iCountNals= %d\n",
             static_cast<long long int> (pBs->pBufEnd - pBs->pBufPtr), iLen, iNal, pCtx->pOut->iCountNals);
#endif
    return 0;
  }

  WelsLoadNal (pCtx->pOut, NAL_UNIT_FILLER_DATA, NRI_PRI_LOWEST);

  for (i = 0; i < iLen; i++) {
    BsWriteBits (pBs, 8, 0xff);
  }

  BsRbspTrailingBits (pBs);

  BsFlush (pBs);

  WelsUnloadNal (pCtx->pOut);
  iNalLen = WelsEncodeNal (&pCtx->pOut->sNalList[iNal], pCtx->pFrameBs + pCtx->iPosBsBuffer, &iNalLen);

  pCtx->iPosBsBuffer	+= iNalLen;
  iSize				+= iNalLen;

  return iSize;
}

/*
 * post process of dynamic slicing bs writing in case PACKING_ONE_SLICE_PER_LAYER
 * include: count bs size of over all the slices in layer,
 * return: count number of slices in layer
 */
#if defined(MT_ENABLED) && defined(PACKING_ONE_SLICE_PER_LAYER)
int32_t PostProcDynamicSlicingBsWriting (sWelsEncCtx* pCtx, SLayerBSInfo* pLayerBsInfo, int32_t* pLayerSize,
    const int32_t kiPartitionCnt) {
  SDqLayer* pCurDq		= pCtx->pCurDqLayer;
  int32_t iPartitionIdx	= 0;
  int32_t iCheckingIdx	= 0;
  int32_t iSwappingIdx	= -1;
  int32_t iSliceCount		= 0;
  int32_t iLayerSize		= 0;

  // count number of slices in layer and layer size
  while (iPartitionIdx < kiPartitionCnt) {
    const int32_t coded_slice_cnt = pCurDq->pNumSliceCodedOfPartition[iPartitionIdx];
    iLayerSize += pCtx->pSliceThreading->pCountBsSizeInPartition[iPartitionIdx];
    iSliceCount += coded_slice_cnt;
    ++ iPartitionIdx;
  }
  *pLayerSize	= iLayerSize;

  // reordering pLayerBs pointers, but do not ensure raster scan order of picture
  // just maintain discontinuous items,i.e,
  // input:
  // partition 1: uiSliceIdx: 0 2 4 6
  // partition 2: uiSliceIdx: 1 3 5 7 9 11 13
  // output:
  // uiSliceIdx: 0 1 2 3 4 5 6 7 8 9 10
  iCheckingIdx = 0;
  while (true) {
    bool_t bMatchFlag = false;
    iPartitionIdx = 0;
    while (iPartitionIdx < kiPartitionCnt) {
      const int32_t coded_slice_cnt = pCurDq->pNumSliceCodedOfPartition[iPartitionIdx];
      // iCheckingIdx need convert to iIndex of iPartitionIdx based to avoid linear searching
      // belong this partition and not exceed the number of slices coded in partition
      if (iPartitionIdx == (iCheckingIdx % kiPartitionCnt)
          && iCheckingIdx / kiPartitionCnt < coded_slice_cnt) {
        if (iSwappingIdx >= 0) {
          // memory swapping
          memmove (pLayerBsInfo + iSwappingIdx, LayerBsInfo + iCheckingIdx, sizeof (SLayerBSInfo));	// confirmed_safe_unsafe_usage
          ++ iSwappingIdx;	// record iSwappingIdx
        }
        ++ iCheckingIdx;
        bMatchFlag = true;
        break;
      }
      ++ iPartitionIdx;
    }
    if (!bMatchFlag) {
      if (iSwappingIdx < 0)
        iSwappingIdx = iCheckingIdx;
      ++ iCheckingIdx;
    }
    if (iSwappingIdx >= iSliceCount)
      break;
  }

  return iSliceCount;
}
#endif//MT_ENABLED && PACKING_ONE_SLICE_PER_LAYER

/*
 * Force coding IDR as follows
 */
int32_t ForceCodingIDR (sWelsEncCtx* pCtx) {
  if (NULL == pCtx)
    return 1;

  pCtx->bEncCurFrmAsIdrFlag = true;
  pCtx->iCodingIndex	= 0;

  return 0;
}

int32_t WelsEncoderEncodeParameterSets (sWelsEncCtx* pCtx, void* pDst) {
  SFrameBSInfo* pFbi          = (SFrameBSInfo*)pDst;
  SLayerBSInfo* pLayerBsInfo  = &pFbi->sLayerInfo[0];
  int32_t iNalLen[128]        = {0};
  int32_t iCountNal           = 0;

  pLayerBsInfo->pBsBuf = pCtx->pFrameBs;
  InitBits (&pCtx->pOut->sBsWrite, pCtx->pOut->pBsBuffer, pCtx->pOut->uiSize);

  WelsWriteParameterSets (pCtx, &iNalLen[0], &iCountNal);

  pLayerBsInfo->uiPriorityId  = 0;
  pLayerBsInfo->uiSpatialId   = 0;
  pLayerBsInfo->uiTemporalId  = 0;
  pLayerBsInfo->uiQualityId   = 0;
  pLayerBsInfo->uiLayerType   = NON_VIDEO_CODING_LAYER;
  pLayerBsInfo->iNalCount     = iCountNal;
  for (int32_t iNalIndex      = 0; iNalIndex < iCountNal; ++ iNalIndex) {
    pLayerBsInfo->iNalLengthInByte[iNalIndex] = iNalLen[iNalIndex];
  }

  pCtx->eLastNalPriority      = NRI_PRI_HIGHEST;
  pFbi->iLayerNum             = 1;

#if defined(X86_ASM)
  WelsEmms();
#endif //X86_ASM

  return 0;
}

/*!
 * \brief	core svc encoding process
 *
 * \pParam	pCtx			sWelsEncCtx*, encoder context
 * \pParam	pDst			FrameBSInfo*
 * \pParam	pSrc			SSourcePicture* for need_ds = true or SSourcePicture** for need_ds = false
 * \pParam	iConfiguredLayerNum	=1 in case need_ds = true or >1 in case need_ds = false
 * \pParam	need_ds		Indicate whether need down sampling desired
 *						[NO in picture list case, YES in console aplication based]
 * \return	EFrameType (WELS_FRAME_TYPE_IDR/WELS_FRAME_TYPE_I/WELS_FRAME_TYPE_P)
 */
int32_t WelsEncoderEncodeExt (sWelsEncCtx* pCtx, void* pDst, const SSourcePicture** ppSrcList,
                              const int32_t iConfiguredLayerNum) {
  SFrameBSInfo* pFbi					= (SFrameBSInfo*)pDst;
  SLayerBSInfo* pLayerBsInfo					= &pFbi->sLayerInfo[0];
  SWelsSvcCodingParam* pSvcParam	= pCtx->pSvcParam;
  SSpatialPicIndex* pSpatialIndexMap = &pCtx->sSpatialIndexMap[0];
#if defined(ENABLE_FRAME_DUMP) || defined(ENABLE_PSNR_CALC)
  SPicture* fsnr						= NULL;
#endif//ENABLE_FRAME_DUMP || ENABLE_PSNR_CALC
  SPicture* pEncPic						= NULL;	// to be decided later
#if defined(MT_ENABLED) && (defined(DYNAMIC_SLICE_ASSIGN) || defined(MT_DEBUG))
  int32_t did_list[MAX_DEPENDENCY_LAYER]	= {0};
#endif//MT_ENABLED && DYNAMIC_SLICE_ASSIGN
  int32_t iLayerNum					= 0;
  int32_t iLayerSize					= 0;
  int32_t iSpatialNum					= 0; // available count number of spatial layers due to frame size changed in this given frame
  int32_t iSpatialIdx					= 0; // iIndex of spatial layers due to frame size changed in this given frame
  int32_t iFrameSize					= 0;
  int32_t iNalLen[128]				= {0};
  int32_t iNalIdxInLayer			= 0;
  int32_t iCountNal					= 0;
  EFrameType eFrameType				= WELS_FRAME_TYPE_AUTO;
  int32_t iCurWidth					= 0;
  int32_t iCurHeight					= 0;
  EWelsNalUnitType eNalType			= NAL_UNIT_UNSPEC_0;
  EWelsNalRefIdc eNalRefIdc			= NRI_PRI_LOWEST;
  int8_t iCurDid						= 0;
  int8_t iCurTid						= 0;
  bool_t bAvcBased					= false;
#if defined(ENABLE_PSNR_CALC)
  real32_t snr_y = .0f, snr_u = .0f, snr_v = .0f;
#endif//ENABLE_PSNR_CALC

#if defined(_DEBUG)
  int32_t i = 0, j = 0, k = 0;
#endif//_DEBUG

  pFbi->iLayerNum	= 0;	// for initialization

  // perform csc/denoise/downsample/padding, generate spatial layers
  iSpatialNum = pCtx->pVpp->WelsPreprocessStep1 (pCtx, ppSrcList, iConfiguredLayerNum);
  if (iSpatialNum < 1) {	// skip due to temporal layer settings (different frame rate)
    ++ pCtx->iCodingIndex;
    return WELS_FRAME_TYPE_SKIP;
  }

  eFrameType = DecideFrameType (pCtx, iSpatialNum);
  if (eFrameType == WELS_FRAME_TYPE_SKIP)
    return eFrameType;

  InitFrameCoding (pCtx, eFrameType);

  iCurTid	= GetTemporalLevel (&pSvcParam->sDependencyLayers[pSpatialIndexMap->iDid], pCtx->iCodingIndex,
                              pSvcParam->uiGopSize);
  pCtx->uiTemporalId	= iCurTid;

  pLayerBsInfo->pBsBuf	= pCtx->pFrameBs ;

  if (eFrameType == WELS_FRAME_TYPE_IDR) {
    ++ pCtx->sPSOVector.uiIdrPicId;
    //if ( pSvcParam->bEnableSSEI )

    // write parameter sets bitstream here
    WelsWriteParameterSets (pCtx, &iNalLen[0], &iCountNal);

    pLayerBsInfo->uiPriorityId	= 0;
    pLayerBsInfo->uiSpatialId		= 0;
    pLayerBsInfo->uiTemporalId	= 0;
    pLayerBsInfo->uiQualityId		= 0;
    pLayerBsInfo->uiLayerType		= NON_VIDEO_CODING_LAYER;
    pLayerBsInfo->iNalCount		= iCountNal;
    for (int32_t iNalIndex	= 0; iNalIndex < iCountNal; ++ iNalIndex) {
      pLayerBsInfo->iNalLengthInByte[iNalIndex]	= iNalLen[iNalIndex];
    }

    ++ pLayerBsInfo;
    pLayerBsInfo->pBsBuf			= pCtx->pFrameBs + pCtx->iPosBsBuffer;
    ++ iLayerNum;
  }

  pCtx->pCurDqLayer				= pCtx->ppDqLayerList[pSpatialIndexMap->iDid];
  pCtx->pCurDqLayer->pRefLayer	= NULL;

  while (iSpatialIdx < iSpatialNum) {
    const int32_t d_idx			= (pSpatialIndexMap + iSpatialIdx)->iDid;	// get iDid
    SDLayerParam* param_d		= &pSvcParam->sDependencyLayers[d_idx];

    pCtx->uiDependencyId	= iCurDid = (int8_t)d_idx;
    pCtx->pVpp->WelsPreprocessStep3 (pCtx, d_idx);

    pCtx->pEncPic	 = pEncPic = (pSpatialIndexMap + iSpatialIdx)->pSrc;
    pCtx->pEncPic->iPictureType	= pCtx->eSliceType;
    pCtx->pEncPic->iFramePoc		= pCtx->iPOC;

    iCurWidth	= param_d->iFrameWidth;
    iCurHeight	= param_d->iFrameHeight;

#if defined(MT_ENABLED) && (defined(DYNAMIC_SLICE_ASSIGN) || defined(MT_DEBUG))
    did_list[iSpatialIdx]	= iCurDid;
#endif//MT_ENABLED && DYNAMIC_SLICE_ASSIGN

    // Encoding this picture might mulitiple sQualityStat layers potentially be encoded as followed

    switch (param_d->sMso.uiSliceMode) {
    case SM_FIXEDSLCNUM_SLICE: {
#if defined(MT_ENABLED) && defined(DYNAMIC_SLICE_ASSIGN)
      if ((iCurDid > 0) && (pSvcParam->iMultipleThreadIdc > 1) &&
          (pSvcParam->sDependencyLayers[iCurDid].sMso.uiSliceMode == SM_FIXEDSLCNUM_SLICE
           && pSvcParam->iMultipleThreadIdc >= pSvcParam->sDependencyLayers[iCurDid].sMso.sSliceArgument.iSliceNum)
         )
        AdjustEnhanceLayer (pCtx, iCurDid);
#endif//MT_ENABLED && DYNAMIC_SLICE_ASSIGN
      break;
    }
    case SM_DYN_SLICE: {
      int32_t iPicIPartitionNum = PicPartitionNumDecision (pCtx);
      // MT compatibility
      pCtx->iActiveThreadsNum	=
        iPicIPartitionNum;	// we try to active number of threads, equal to number of picture partitions
      WelsInitCurrentDlayerMltslc (pCtx, iPicIPartitionNum);
      break;
    }
    default: {
      break;
    }
    }

    /* coding each spatial layer, only one sQualityStat layer within spatial support */
    int32_t iSliceCount	= 1;
    if (iLayerNum >= MAX_LAYER_NUM_OF_FRAME) {	// check available layer_bs_info writing as follows
      WelsLog (pCtx, WELS_LOG_ERROR, "WelsEncoderEncodeExt(), iLayerNum(%d) overflow(max:%d)!", iLayerNum,
               MAX_LAYER_NUM_OF_FRAME);
      return -1;
    }

    iNalIdxInLayer	= 0;
    bAvcBased	= (iCurDid == BASE_DEPENDENCY_ID);
    pCtx->bNeedPrefixNalFlag	= (bAvcBased &&
                                 (pSvcParam->bPrefixNalAddingCtrl ||
                                  (pSvcParam->iNumDependencyLayer > 1)));

    if (eFrameType == WELS_FRAME_TYPE_P) {
      eNalType	= bAvcBased ? NAL_UNIT_CODED_SLICE : NAL_UNIT_CODED_SLICE_EXT;
    } else if (eFrameType == WELS_FRAME_TYPE_IDR) {
      eNalType	= bAvcBased ? NAL_UNIT_CODED_SLICE_IDR : NAL_UNIT_CODED_SLICE_EXT;
    }
    if (iCurTid == 0 || pCtx->eSliceType == I_SLICE)
      eNalRefIdc	= NRI_PRI_HIGHEST;
    else if (iCurTid == pSvcParam->iDecompStages)
      eNalRefIdc	= NRI_PRI_LOWEST;
    else if (1 + iCurTid == pSvcParam->iDecompStages)
      eNalRefIdc	= NRI_PRI_LOW;
    else	// more details for other temporal layers?
      eNalRefIdc	= NRI_PRI_HIGHEST;
    pCtx->eNalType		= eNalType;
    pCtx->eNalPriority	= eNalRefIdc;

    pCtx->pDecPic					= pCtx->ppRefPicListExt[iCurDid]->pNextBuffer;
#if defined(ENABLE_FRAME_DUMP) || defined(ENABLE_PSNR_CALC)
    fsnr					= pCtx->pDecPic;
#endif//#if defined(ENABLE_FRAME_DUMP) || defined(ENABLE_PSNR_CALC)
    pCtx->pDecPic->iPictureType	= pCtx->eSliceType;
    pCtx->pDecPic->iFramePoc		= pCtx->iPOC;

    WelsInitCurrentLayer (pCtx, iCurWidth, iCurHeight);

    WelsMarkPic (pCtx);
    if (!WelsBuildRefList (pCtx, pCtx->iPOC)) {
      // Force coding IDR as followed
      ForceCodingIDR (pCtx);
      WelsLog (pCtx, WELS_LOG_WARNING, "WelsEncoderEncodeExt(), WelsBuildRefList failed for P frames, pCtx->iNumRef0= %d.\n",
               pCtx->iNumRef0);
      return -1;
    }
#ifdef LONG_TERM_REF_DUMP
    dump_ref (pCtx);
#endif
    WelsUpdateRefSyntax (pCtx,  pCtx->iPOC,
                         eFrameType);	//get reordering syntax used for writing slice header and transmit to encoder.
    PrefetchReferencePicture (pCtx, eFrameType);	// update reference picture for current pDq layer

    pCtx->pFuncList->pfRc.pfWelsRcPictureInit (pCtx);
    PreprocessSliceCoding (pCtx);	// MUST be called after pfWelsRcPictureInit() and WelsInitCurrentLayer()

    iLayerSize	= 0;
    if (SM_SINGLE_SLICE == param_d->sMso.uiSliceMode) {	// only one slice within a sQualityStat layer
      int32_t iSliceSize = 0;

      if (pCtx->bNeedPrefixNalFlag) {
        iLayerSize += AddPrefixNal (pCtx, pLayerBsInfo, &iNalLen[0], &iNalIdxInLayer, eNalType, eNalRefIdc);
      }

      WelsLoadNal (pCtx->pOut, eNalType, eNalRefIdc);

      WelsCodeOneSlice (pCtx, 0, eNalType);

      WelsUnloadNal (pCtx->pOut);

      iSliceSize = WelsEncodeNalExt (&pCtx->pOut->sNalList[pCtx->pOut->iNalIndex - 1],
                                     &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt,
                                     pCtx->pFrameBs + pCtx->iPosBsBuffer,
                                     &iNalLen[iNalIdxInLayer]);
      iLayerSize += iSliceSize;
      pCtx->iPosBsBuffer	+= iSliceSize;
      pLayerBsInfo->uiLayerType		= VIDEO_CODING_LAYER;
      pLayerBsInfo->uiSpatialId		= iCurDid;
      pLayerBsInfo->uiTemporalId	= iCurTid;
      pLayerBsInfo->uiQualityId		= 0;
      pLayerBsInfo->uiPriorityId	= 0;
      pLayerBsInfo->iNalLengthInByte[iNalIdxInLayer]	= iSliceSize;
      pLayerBsInfo->iNalCount		= ++ iNalIdxInLayer;
    }
    // for dynamic slicing single threading..
#ifndef MT_ENABLED
    else if (SM_DYN_SLICE == param_d->sMso.uiSliceMode)
#else	// MT_ENABLED
    else if ((SM_DYN_SLICE == param_d->sMso.uiSliceMode) && (pSvcParam->iMultipleThreadIdc <= 1))
#endif//MT_ENABLED
    {
      const int32_t kiLastMbInFrame = pCtx->pCurDqLayer->pSliceEncCtx->iMbNumInFrame;
      WelsCodeOnePicPartition (pCtx, pLayerBsInfo, &iNalIdxInLayer, &iLayerSize, 0, kiLastMbInFrame, 0);
    } else {
      //other multi-slice uiSliceMode
#if defined(MT_ENABLED)
      int err = 0;
      // THREAD_FULLY_FIRE_MODE/THREAD_PICK_UP_MODE for any mode of non-SM_DYN_SLICE
      if ((SM_DYN_SLICE != param_d->sMso.uiSliceMode) && (pSvcParam->iMultipleThreadIdc > 1)) {
        iSliceCount	= GetCurrentSliceNum (pCtx->pCurDqLayer->pSliceEncCtx);
        if (iLayerNum +
#if defined(PACKING_ONE_SLICE_PER_LAYER)
            iSliceCount
#else
            1
#endif//PACKING_ONE_SLICE_PER_LAYER
            >= MAX_LAYER_NUM_OF_FRAME) {	// check available layer_bs_info for further writing as followed
          WelsLog (pCtx, WELS_LOG_ERROR,
                   "WelsEncoderEncodeExt(), iLayerNum(%d) overflow(max:%d) at iDid= %d uiSliceMode= %d, iSliceCount= %d!",
                   iLayerNum, MAX_LAYER_NUM_OF_FRAME, iCurDid, param_d->sMso.uiSliceMode, iSliceCount);
          return -1;
        }
        if (iSliceCount <= 1) {
          WelsLog (pCtx, WELS_LOG_ERROR,
                   "WelsEncoderEncodeExt(), iSliceCount(%d) from GetCurrentSliceNum() is untrusted due stack/heap crupted!\n",
                   iSliceCount);
          return -1;
        }

        if (pSvcParam->iCountThreadsNum >= iSliceCount) {	//THREAD_FULLY_FIRE_MODE
#if defined(PACKING_ONE_SLICE_PER_LAYER)
          int32_t iSliceIdx = 1;
          int32_t iOrgSlicePos[MAX_SLICES_NUM] = {0};
          iOrgSlicePos[0] = pCtx->iPosBsBuffer;
          while (uiSliceIdx < iSliceCount) {
            iOrgSlicePos[uiSliceIdx] = pCtx->pSliceBs[uiSliceIdx].uiBsPos;
            ++ uiSliceIdx;
          }
#elif defined(MT_DEBUG)
          int64_t t_bs_append = 0;
#endif//PACKING_ONE_SLICE_PER_LAYER

          pCtx->iActiveThreadsNum	= iSliceCount;
          // to fire slice coding threads
          err = FiredSliceThreads (&pCtx->pSliceThreading->pThreadPEncCtx[0], &pCtx->pSliceThreading->pReadySliceCodingEvent[0],
                                   pLayerBsInfo, iSliceCount, pCtx->pCurDqLayer->pSliceEncCtx, FALSE);
          if (err) {
            WelsLog (pCtx, WELS_LOG_ERROR,
                     "[MT] WelsEncoderEncodeExt(), FiredSliceThreads return(%d) failed and exit encoding frame, iCountThreadsNum= %d, iSliceCount= %d, uiSliceMode= %d, iMultipleThreadIdc= %d!!\n",
                     err, pSvcParam->iCountThreadsNum, iSliceCount, param_d->sMso.uiSliceMode, pSvcParam->iMultipleThreadIdc);
            return -1;
          }

          WelsMultipleEventsWaitAllBlocking (iSliceCount, &pCtx->pSliceThreading->pSliceCodedEvent[0]);


          // all slices are finished coding here
          // append exclusive slice 0 bs to pFrameBs
#if defined(PACKING_ONE_SLICE_PER_LAYER)
          iLayerSize = pCtx->iPosBsBuffer - iOrgSlicePos[0];
          uiSliceIdx = 1;
          while (uiSliceIdx < iSliceCount) {
            iLayerSize += pCtx->pSliceBs[uiSliceIdx].uiBsPos - iOrgSlicePos[uiSliceIdx];
            ++ uiSliceIdx;
          }
          iLayerNum += iSliceCount;	// each slice stickly output as layer info for performance improvement directly
          pLayerBsInfo += iSliceCount;
#else
#if defined(MT_DEBUG)
          t_bs_append = WelsTime();
#endif//MT_DEBUG
          iLayerSize = AppendSliceToFrameBs (pCtx, pLayerBsInfo, iSliceCount);
#if defined(MT_DEBUG)
          t_bs_append = WelsTime() - t_bs_append;
          if (pCtx->pSliceThreading->pFSliceDiff) {
            fprintf (pCtx->pSliceThreading->pFSliceDiff,
#if defined(_WIN32)
                     "%6I64d us consumed at AppendSliceToFrameBs() for coding_idx: %d iDid: %d qid: %d\n",
#else
                     "%6lld us consumed at AppendSliceToFrameBs() for coding_idx: %d iDid: %d qid: %d\n",
#endif//WIN32
                     t_bs_append, pCtx->iCodingIndex, iCurDid, 0);
          }
#endif//MT_DEBUG
#endif//PACKING_ONE_SLICE_PER_LAYER
        } else {	//THREAD_PICK_UP_MODE
          int32_t iNumThreadsRunning = 0;
          int32_t iNumThreadsScheduled = 0;
          int32_t iIndexOfSliceToBeCoded = 0;
#if defined(PACKING_ONE_SLICE_PER_LAYER)
          int32_t iSliceIdx = 1;
          int32_t iOrgSlicePos[MAX_SLICES_NUM] = {0};
          iOrgSlicePos[0] = pCtx->iPosBsBuffer;
          while (uiSliceIdx < iSliceCount) {
            iOrgSlicePos[uiSliceIdx] = pCtx->pSliceBs[uiSliceIdx].uiBsPos;
            ++ uiSliceIdx;
          }
#endif//PACKING_ONE_SLICE_PER_LAYER

          pCtx->iActiveThreadsNum	= pSvcParam->iCountThreadsNum;
          iNumThreadsScheduled	= pCtx->iActiveThreadsNum;
          iNumThreadsRunning		= iNumThreadsScheduled;
          // to fire slice coding threads
          err = FiredSliceThreads (&pCtx->pSliceThreading->pThreadPEncCtx[0], &pCtx->pSliceThreading->pReadySliceCodingEvent[0],
                                   pLayerBsInfo, iNumThreadsRunning, pCtx->pCurDqLayer->pSliceEncCtx, FALSE);
          if (err) {
            WelsLog (pCtx, WELS_LOG_ERROR,
                     "[MT] WelsEncoderEncodeExt(), FiredSliceThreads return(%d) failed and exit encoding frame, iCountThreadsNum= %d, iSliceCount= %d, uiSliceMode= %d, iMultipleThreadIdc= %d!!\n",
                     err, pSvcParam->iCountThreadsNum, iSliceCount, param_d->sMso.uiSliceMode, pSvcParam->iMultipleThreadIdc);
            return -1;
          }

          iIndexOfSliceToBeCoded = iNumThreadsRunning;
          while (1) {
            if (iIndexOfSliceToBeCoded >= iSliceCount && iNumThreadsRunning <= 0)
              break;
#ifdef _WIN32
            WELS_THREAD_ERROR_CODE lwait	= 0;
            int32_t iEventId				= -1;

            lwait = WelsMultipleEventsWaitSingleBlocking (iNumThreadsScheduled,
                    &pCtx->pSliceThreading->pSliceCodedEvent[0],
                    2);	// 2 ms for one tick
            iEventId = (int32_t) (lwait - WELS_THREAD_ERROR_WAIT_OBJECT_0);
            if (iEventId >= 0 && iEventId < iNumThreadsScheduled) {
              if (iIndexOfSliceToBeCoded < iSliceCount) {
                // pick up succeeding slice for threading
                // thread_id equal to iEventId per implementation here
                pCtx->pSliceThreading->pThreadPEncCtx[iEventId].iSliceIndex	= iIndexOfSliceToBeCoded;
#ifdef PACKING_ONE_SLICE_PER_LAYER
                pCtx->pSliceThreading->pThreadPEncCtx[iEventId].pLayerBs	= pLayerBsInfo + iIndexOfSliceToBeCoded;
#endif//PACKING_ONE_SLICE_PER_LAYER
                WelsEventSignal (&pCtx->pSliceThreading->pReadySliceCodingEvent[iEventId]);

                ++ iIndexOfSliceToBeCoded;
              } else {	// no other slices left for coding
                -- iNumThreadsRunning;
              }
            } else {
              WelsSleep (1);
            }
#else//__GNUC__
            // TODO for pthread platforms
            // alternate implementation using blocking due non-blocking with timeout mode not support at wels thread lib, tune back if available
            WelsMultipleEventsWaitAllBlocking (iNumThreadsRunning, &pCtx->pSliceThreading->pSliceCodedEvent[0]);
            if (iIndexOfSliceToBeCoded < iSliceCount) {
              int32_t iThreadIdx = 0;
              // pick up succeeding slices for threading if left
              while (iThreadIdx < iNumThreadsScheduled) {
                if (iIndexOfSliceToBeCoded >= iSliceCount)
                  break;
                pCtx->pSliceThreading->pThreadPEncCtx[iThreadIdx].iSliceIndex = iIndexOfSliceToBeCoded;
#ifdef PACKING_ONE_SLICE_PER_LAYER
                pCtx->pSliceThreading->pThreadPEncCtx[iThreadIdx].pLayerBs = pLayerBsInfo + iIndexOfSliceToBeCoded;
#endif//PACKING_ONE_SLICE_PER_LAYER
                WelsEventSignal (pCtx->pSliceThreading->pReadySliceCodingEvent[iThreadIdx]);

                ++ iIndexOfSliceToBeCoded;
                ++ iThreadIdx;
              }
              // update iNumThreadsRunning
              iNumThreadsRunning		= iThreadIdx;
            } else {
              iNumThreadsRunning = 0;
            }
#endif//_WIN32
          }//while(1)

// all slices are finished coding here
          // append exclusive slice 0 bs to pFrameBs
#if defined(PACKING_ONE_SLICE_PER_LAYER)
          iLayerSize = pCtx->iPosBsBuffer - iOrgSlicePos[0];
          uiSliceIdx = 1;
          while (uiSliceIdx < iSliceCount) {
            iLayerSize += pCtx->pSliceBs[uiSliceIdx].uiBsPos - iOrgSlicePos[uiSliceIdx];
            ++ uiSliceIdx;
          }
          iLayerNum += iSliceCount;	// each slice stickly output as layer info for performance improvement directly
          pLayerBsInfo += iSliceCount;
#else
          iLayerSize = AppendSliceToFrameBs (pCtx, pLayerBsInfo, iSliceCount);
#endif//PACKING_ONE_SLICE_PER_LAYER
        }
      }
      // THREAD_FULLY_FIRE_MODE && SM_DYN_SLICE
      else if ((SM_DYN_SLICE == param_d->sMso.uiSliceMode) && (pSvcParam->iMultipleThreadIdc > 1)) {
        const int32_t kiPartitionCnt	= pCtx->iActiveThreadsNum; //pSvcParam->iCountThreadsNum;
#if defined(PACKING_ONE_SLICE_PER_LAYER)
        ResetCountBsSizeInPartitions (pCtx->pSliceThreading->pCountBsSizeInPartition, kiPartitionCnt);
        pCtx->pCurDqLayer->pSliceEncCtx->iMaxSliceNumConstraint = WELS_MIN (MAX_SLICES_NUM,
            DynamicMaxSliceNumConstraint (MAX_LAYER_NUM_OF_FRAME, iLayerNum, 1 + /*( num_qlayer - 1) +*/ (((iCurDid == 0)
                                          && (pSvcParam->uiGopSize > 1)) ? 1 : 0)));
#endif//PACKING_ONE_SLICE_PER_LAYER

        // to fire slice coding threads
        err = FiredSliceThreads (&pCtx->pSliceThreading->pThreadPEncCtx[0], &pCtx->pSliceThreading->pReadySliceCodingEvent[0],
                                 pLayerBsInfo, kiPartitionCnt, pCtx->pCurDqLayer->pSliceEncCtx, TRUE);
        if (err) {
          WelsLog (pCtx, WELS_LOG_ERROR,
                   "[MT] WelsEncoderEncodeExt(), FiredSliceThreads return(%d) failed and exit encoding frame, iCountThreadsNum= %d, iSliceCount= %d, uiSliceMode= %d, iMultipleThreadIdc= %d!!\n",
                   err, pSvcParam->iCountThreadsNum, iSliceCount, param_d->sMso.uiSliceMode, pSvcParam->iMultipleThreadIdc);
          return -1;
        }

        WelsMultipleEventsWaitAllBlocking (kiPartitionCnt, &pCtx->pSliceThreading->pSliceCodedEvent[0]);

#if defined(PACKING_ONE_SLICE_PER_LAYER)
        iSliceCount = PostProcDynamicSlicingBsWriting (pCtx, pLayerBsInfo, &iLayerSize, kiPartitionCnt);
        assert (iLayerNum + iSliceCount < MAX_LAYER_NUM_OF_FRAME);
        pLayerBsInfo += iSliceCount;
        iLayerNum += iSliceCount;
#else
        iLayerSize = AppendSliceToFrameBs (pCtx, pLayerBsInfo, kiPartitionCnt);
#endif//PACKING_ONE_SLICE_PER_LAYER
      } else	// for non-dynamic-slicing mode single threading branch..
#endif//MT_ENABLED
      {
        const bool_t bNeedPrefix	= pCtx->bNeedPrefixNalFlag;
        int32_t iSliceIdx			= 0;

        iSliceCount	= GetCurrentSliceNum (pCtx->pCurDqLayer->pSliceEncCtx);
        while (iSliceIdx < iSliceCount) {
          int32_t iSliceSize	= 0;

          if (bNeedPrefix) {
            iLayerSize += AddPrefixNal (pCtx, pLayerBsInfo, &iNalLen[0], &iNalIdxInLayer, eNalType, eNalRefIdc);
          }

          WelsLoadNal (pCtx->pOut, eNalType, eNalRefIdc);
          WelsCodeOneSlice (pCtx, iSliceIdx, eNalType);
          WelsUnloadNal (pCtx->pOut);

          iSliceSize = WelsEncodeNalExt (&pCtx->pOut->sNalList[pCtx->pOut->iNalIndex - 1],
                                         &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt,
                                         pCtx->pFrameBs + pCtx->iPosBsBuffer,
                                         &iNalLen[iNalIdxInLayer]);
          pCtx->iPosBsBuffer	+= iSliceSize;
          iLayerSize	+= iSliceSize;
          pLayerBsInfo->iNalLengthInByte[iNalIdxInLayer]	= iSliceSize;

#if defined(SLICE_INFO_OUTPUT)
          fprintf (stderr,
                   "@slice=%-6d sliceType:%c idc:%d size:%-6d\n",
                   iSliceIdx,
                   (pCtx->eSliceType == P_SLICE ? 'P' : 'I'),
                   eNalRefIdc,
                   iSliceSize);
#endif//SLICE_INFO_OUTPUT
          ++ iNalIdxInLayer;
          ++ iSliceIdx;
        }

        pLayerBsInfo->uiLayerType		= VIDEO_CODING_LAYER;
        pLayerBsInfo->uiSpatialId		= iCurDid;
        pLayerBsInfo->uiTemporalId	= iCurTid;
        pLayerBsInfo->uiQualityId		= 0;
        pLayerBsInfo->uiPriorityId	= 0;
        pLayerBsInfo->iNalCount		= iNalIdxInLayer;
      }
    }

    // deblocking filter
    if (
#if defined(MT_ENABLED)
      (!pCtx->pCurDqLayer->bDeblockingParallelFlag) &&
#endif//MT_ENABLED
#if !defined(ENABLE_FRAME_DUMP)
      ((eNalRefIdc != NRI_PRI_LOWEST) && (param_d->iHighestTemporalId == 0 || iCurTid < param_d->iHighestTemporalId)) &&
#endif//!ENABLE_FRAME_DUMP
      true
    ) {
      PerformDeblockingFilter (pCtx);
    }

    // reference picture list update
    if (eNalRefIdc != NRI_PRI_LOWEST) {
      if (!WelsUpdateRefList (pCtx)) {
        // Force coding IDR as followed
        ForceCodingIDR (pCtx);
        WelsLog (pCtx, WELS_LOG_WARNING, "WelsEncoderEncodeExt(), WelsUpdateRefList failed.\n");
        return -1;
      }
    }

    iFrameSize += iLayerSize;

    pCtx->pFuncList->pfRc.pfWelsRcPictureInfoUpdate (pCtx, iLayerSize);

#ifdef ENABLE_FRAME_DUMP
    // Dump reconstruction picture for each sQualityStat layer
    if (iCurDid + 1 < pSvcParam->iNumDependencyLayer)
      DumpDependencyRec (fsnr, &param_d->sRecFileName[0], iCurDid);
#endif//ENABLE_FRAME_DUMP

#if defined(ENABLE_PSNR_CALC)
    snr_y	= WelsCalcPsnr (fsnr->pData[0],
                          fsnr->iLineSize[0],
                          pEncPic->pData[0],
                          pEncPic->iLineSize[0],
                          iCurWidth,
                          iCurHeight);
    snr_u	= WelsCalcPsnr (fsnr->pData[1],
                          fsnr->iLineSize[1],
                          pEncPic->pData[1],
                          pEncPic->iLineSize[1],
                          (iCurWidth >> 1),
                          (iCurHeight >> 1));
    snr_v	= WelsCalcPsnr (fsnr->pData[2],
                          fsnr->iLineSize[2],
                          pEncPic->pData[2],
                          pEncPic->iLineSize[2],
                          (iCurWidth >> 1),
                          (iCurHeight >> 1));
#endif//ENABLE_PSNR_CALC

#if defined(LAYER_INFO_OUTPUT)
    fprintf (stderr, "%2s %5d: %-5d %2s   T%1d D%1d Q%-2d  QP%3d   Y%2.2f  U%2.2f  V%2.2f  %8d bits\n",
             (iSpatialIdx == 0) ? "#AU" : "   ",
             pCtx->iPOC,
             pCtx->iFrameNum,
             (uiFrameType == WELS_FRAME_TYPE_I || uiFrameType == WELS_FRAME_TYPE_IDR) ? "I" : "P",
             iCurTid,
             iCurDid,
             0,
             pCtx->pWelsSvcRc[pCtx->uiDependencyId].iAverageFrameQp,
             snr_y,
             snr_u,
             snr_v,
             (iLayerSize << 3));
#endif//LAYER_INFO_OUTPUT

#if defined(STAT_OUTPUT)

#if defined(ENABLE_PSNR_CALC)
    {
      pCtx->sStatData[iCurDid][0].sQualityStat.rYPsnr[pCtx->eSliceType]	+= snr_y;
      pCtx->sStatData[iCurDid][0].sQualityStat.rUPsnr[pCtx->eSliceType]	+= snr_u;
      pCtx->sStatData[iCurDid][0].sQualityStat.rVPsnr[pCtx->eSliceType]	+= snr_v;
    }
#endif//ENABLE_PSNR_CALC

#if defined(MB_TYPES_CHECK) //091025, frame output
    if (pCtx->eSliceType == P_SLICE) {
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][Intra4x4] += pCtx->sPerInfo.iMbCount[P_SLICE][Intra4x4];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][Intra16x16] += pCtx->sPerInfo.iMbCount[P_SLICE][Intra16x16];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][Inter16x16] += pCtx->sPerInfo.iMbCount[P_SLICE][Inter16x16];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][Inter16x8] += pCtx->sPerInfo.iMbCount[P_SLICE][Inter16x8];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][Inter8x16] += pCtx->sPerInfo.iMbCount[P_SLICE][Inter8x16];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][Inter8x8] += pCtx->sPerInfo.iMbCount[P_SLICE][Inter8x8];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][PSkip] += pCtx->sPerInfo.iMbCount[P_SLICE][PSkip];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][8] += pCtx->sPerInfo.iMbCount[P_SLICE][8];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][9] += pCtx->sPerInfo.iMbCount[P_SLICE][9];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][10] += pCtx->sPerInfo.iMbCount[P_SLICE][10];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[P_SLICE][11] += pCtx->sPerInfo.iMbCount[P_SLICE][11];
    } else if (pCtx->eSliceType == I_SLICE) {
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[I_SLICE][Intra4x4] += pCtx->sPerInfo.iMbCount[I_SLICE][Intra4x4];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[I_SLICE][Intra16x16] += pCtx->sPerInfo.iMbCount[I_SLICE][Intra16x16];
      pCtx->sStatData[iCurDid][0].sSliceData.iMbCount[I_SLICE][7] += pCtx->sPerInfo.iMbCount[I_SLICE][7];
    }

    memset (pCtx->sPerInfo.iMbCount[P_SLICE], 0, 18 * sizeof (int32_t));
    memset (pCtx->sPerInfo.iMbCount[I_SLICE], 0, 18 * sizeof (int32_t));

#endif//MB_TYPES_CHECK
    {
      //no pCtx->pSvcParam->bMgsT0OnlyStrategy
      ++ pCtx->sStatData[iCurDid][0].sSliceData.iSliceCount[pCtx->eSliceType];	// for multiple slices coding
      pCtx->sStatData[iCurDid][0].sSliceData.iSliceSize[pCtx->eSliceType]	+= (iLayerSize << 3);	// bits
    }
#endif//STAT_OUTPUT

#if defined(MT_ENABLED) && defined(PACKING_ONE_SLICE_PER_LAYER)
    if (pSvcParam->iMultipleThreadIdc <= 1 || SM_SINGLE_SLICE == param_d->sMso.uiSliceMode)	// sigle thread actually used
#else
    if (1)
#endif//MT_ENABLED && PACKING_ONE_SLICE_PER_LAYER
    {
      ++ iLayerNum;
      ++ pLayerBsInfo;
    }


    pLayerBsInfo->pBsBuf	= pCtx->pFrameBs + pCtx->iPosBsBuffer;

    if (pSvcParam->iPaddingFlag && pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingSize > 0) {
      const int32_t kiPaddingNalSize = WritePadding (pCtx, pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingSize);

#if GOM_TRACE_FLAG
      WelsLog (pCtx, WELS_LOG_INFO, "[RC] encoding_qp%d Padding: %d\n", pCtx->uiDependencyId,
               pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingSize);
#endif
      if (kiPaddingNalSize <= 0)
        return -1;

      pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingBitrateStat += pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingSize;

      pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingSize = 0;

      pLayerBsInfo->uiPriorityId	= 0;
      pLayerBsInfo->uiSpatialId		= 0;
      pLayerBsInfo->uiTemporalId	= 0;
      pLayerBsInfo->uiQualityId		= 0;
      pLayerBsInfo->uiLayerType		= NON_VIDEO_CODING_LAYER;
      pLayerBsInfo->iNalCount		= 1;
      pLayerBsInfo->iNalLengthInByte[0] = kiPaddingNalSize;
      ++ pLayerBsInfo;
      pLayerBsInfo->pBsBuf	= pCtx->pFrameBs + pCtx->iPosBsBuffer;
      ++ iLayerNum;
    }

#if defined(MT_ENABLED) && defined(DYNAMIC_SLICE_ASSIGN) && defined(TRY_SLICING_BALANCE)
    if (param_d->sMso.uiSliceMode == SM_FIXEDSLCNUM_SLICE && pSvcParam->iMultipleThreadIdc > 1 &&
        pSvcParam->iMultipleThreadIdc >= param_d->sMso.sSliceArgument.iSliceNum) {
      CalcSliceComplexRatio (pCtx->pSliceThreading->pSliceComplexRatio[iCurDid], pCtx->pCurDqLayer->pSliceEncCtx,
                             pCtx->pSliceThreading->pSliceConsumeTime[iCurDid]);
#if defined(MT_DEBUG)
      TrackSliceComplexities (pCtx, iCurDid);
#endif//#if defined(MT_DEBUG)
    }
#endif//MT_ENABLED && DYNAMIC_SLICE_ASSIGN && TRY_SLICING_BALANCE

    ++ iSpatialIdx;

    if (iCurDid + 1 < pSvcParam->iNumDependencyLayer) {
      WelsSwapDqLayers (pCtx);
    }

    if (pSvcParam->bEnableLongTermReference && (pCtx->pLtr[pCtx->uiDependencyId].bLTRMarkingFlag
        && (pCtx->pLtr[pCtx->uiDependencyId].iLTRMarkMode == LTR_DELAY_MARK))) {
      pCtx->bLongTermRefFlag[d_idx][0] = true;
    }

    if (iCurTid < pCtx->uiSpatialLayersInTemporal[d_idx] - 1 || pSvcParam->iDecompStages == 0) {
      if ((iCurTid >= MAX_TEMPORAL_LEVEL) || (pCtx->uiSpatialLayersInTemporal[d_idx] - 1 >= MAX_TEMPORAL_LEVEL)) {
        ForceCodingIDR (pCtx);	// some logic error
        return -1;
      }

      if (pSvcParam->bEnableLongTermReference && pCtx->bLongTermRefFlag[d_idx][iCurTid]) {
        SPicture* tmp	= pCtx->pSpatialPic[d_idx][pCtx->uiSpatialLayersInTemporal[d_idx] + pCtx->pVaa->uiMarkLongTermPicIdx];
        pCtx->pSpatialPic[d_idx][pCtx->uiSpatialLayersInTemporal[d_idx] + pCtx->pVaa->uiMarkLongTermPicIdx] =
          pCtx->pSpatialPic[d_idx][iCurTid];
        pCtx->pSpatialPic[d_idx][iCurTid] = pCtx->pSpatialPic[d_idx][pCtx->uiSpatialLayersInTemporal[d_idx] - 1];
        pCtx->pSpatialPic[d_idx][pCtx->uiSpatialLayersInTemporal[d_idx] - 1] = tmp;
        pCtx->bLongTermRefFlag[d_idx][iCurTid] = false;
      } else {
        WelsExchangeSpatialPictures (&pCtx->pSpatialPic[d_idx][pCtx->uiSpatialLayersInTemporal[d_idx] - 1],
                                     &pCtx->pSpatialPic[d_idx][iCurTid]);
      }
    }

    if (pSvcParam->bEnableLongTermReference && ((pCtx->pLtr[pCtx->uiDependencyId].bLTRMarkingFlag
        && (pCtx->pLtr[pCtx->uiDependencyId].iLTRMarkMode == LTR_DIRECT_MARK)) || eFrameType == WELS_FRAME_TYPE_IDR)) {
      pCtx->bLongTermRefFlag[d_idx][iCurTid] = true;
    }
  }

#if defined(MT_ENABLED) && defined(MT_DEBUG)
  TrackSliceConsumeTime (pCtx, did_list, iSpatialNum);
#endif//MT_ENABLED && MT_DEBUG

#if defined(MT_ENABLED) && defined(DYNAMIC_SLICE_ASSIGN)
  if (pSvcParam->iMultipleThreadIdc > 1 && did_list[0] == BASE_DEPENDENCY_ID
      && pSvcParam->sDependencyLayers[0].sMso.uiSliceMode == SM_FIXEDSLCNUM_SLICE
      && pSvcParam->iMultipleThreadIdc >= pSvcParam->sDependencyLayers[0].sMso.sSliceArgument.iSliceNum
      && pSvcParam->sDependencyLayers[did_list[iSpatialNum - 1]].sMso.uiSliceMode == SM_FIXEDSLCNUM_SLICE
      && pSvcParam->iMultipleThreadIdc >= pSvcParam->sDependencyLayers[did_list[iSpatialNum -
          1]].sMso.sSliceArgument.iSliceNum) {
    AdjustBaseLayer (pCtx);
  }
#endif//DYNAMIC_SLICE_ASSIGN

#ifdef ENABLE_FRAME_DUMP
  DumpRecFrame (fsnr, &pSvcParam->sDependencyLayers[pSvcParam->iNumDependencyLayer -
                1].sRecFileName[0]);	// pDecPic: final reconstruction output
#endif//ENABLE_FRAME_DUMP

  ++ pCtx->iCodingIndex;
  pCtx->eLastNalPriority	= eNalRefIdc;
  pFbi->iLayerNum			= iLayerNum;

#if defined(X86_ASM)
  WelsEmms();
#endif //X86_ASM

  return eFrameType;
}

/*!
 * \brief	Wels SVC encoder parameters adjustment
 *			SVC adjustment results in new requirement in memory blocks adjustment
 */
int32_t WelsEncoderParamAdjust (sWelsEncCtx** ppCtx, SWelsSvcCodingParam* pNewParam) {
  SWelsSvcCodingParam* pOldParam		= NULL;
  int32_t iReturn = 0;
  int8_t iIndexD = 0;
  bool_t bNeedReset = false;

  if (NULL == ppCtx || NULL == *ppCtx || NULL == pNewParam)	return 1;

  /* Check validation in new parameters */
  iReturn	= ParamValidationExt (pNewParam);
  if (iReturn != 0)	return iReturn;

  pOldParam	= (*ppCtx)->pSvcParam;

  /* Decide whether need reset for IDR frame based on adjusting prarameters changed */
  /* Temporal levels, spatial settings and/ or quality settings changed need update parameter sets related. */
  bNeedReset	=	(pOldParam == NULL) ||
                (pOldParam->iNumTemporalLayer != pNewParam->iNumTemporalLayer) ||
                (pOldParam->uiGopSize != pNewParam->uiGopSize) ||
                (pOldParam->iNumDependencyLayer != pNewParam->iNumDependencyLayer) ||
                (pOldParam->iDecompStages != pNewParam->iDecompStages) ||
                (pOldParam->iActualPicWidth != pNewParam->iActualPicWidth
                 || pOldParam->iActualPicHeight != pNewParam->iActualPicHeight) ||
                (pOldParam->SUsedPicRect.iWidth != pNewParam->SUsedPicRect.iWidth
                 || pOldParam->SUsedPicRect.iHeight != pNewParam->SUsedPicRect.iHeight) ||
                (pOldParam->bEnableLongTermReference != pNewParam->bEnableLongTermReference);
  if (!bNeedReset) {	// Check its picture resolutions/quality settings respectively in each dependency layer
    iIndexD = 0;
    assert (pOldParam->iNumDependencyLayer == pNewParam->iNumDependencyLayer);
    do {
      const SDLayerParam* kpOldDlp	= &pOldParam->sDependencyLayers[iIndexD];
      const SDLayerParam* kpNewDlp	= &pNewParam->sDependencyLayers[iIndexD];
      float fT1 = .0f;
      float fT2 = .0f;

      // check frame size settings
      if (kpOldDlp->iFrameWidth != kpNewDlp->iFrameWidth ||
          kpOldDlp->iFrameHeight != kpNewDlp->iFrameHeight ||
          kpOldDlp->iActualWidth != kpNewDlp->iActualWidth ||
          kpOldDlp->iActualHeight != kpNewDlp->iActualHeight) {
        bNeedReset	= true;
        break;
      }

      if (kpOldDlp->sMso.uiSliceMode != kpNewDlp->sMso.uiSliceMode ||
          kpOldDlp->sMso.sSliceArgument.iSliceNum != kpNewDlp->sMso.sSliceArgument.iSliceNum) {
        bNeedReset	= true;
        break;
      }

      // check frame rate
      // we can not check whether corresponding fFrameRate is equal or not,
      // only need to check d_max/d_min and max_fr/d_max whether it is equal or not
      if (kpNewDlp->fInputFrameRate > EPSN && kpOldDlp->fInputFrameRate > EPSN)
        fT1 = kpNewDlp->fOutputFrameRate / kpNewDlp->fInputFrameRate - kpOldDlp->fOutputFrameRate / kpOldDlp->fInputFrameRate;
      if (kpNewDlp->fOutputFrameRate > EPSN && kpOldDlp->fOutputFrameRate > EPSN)
        fT2 = pNewParam->fMaxFrameRate / kpNewDlp->fOutputFrameRate - pOldParam->fMaxFrameRate / kpOldDlp->fOutputFrameRate;
      if (fT1 > EPSN || fT1 < -EPSN || fT2 > EPSN || fT2 < -EPSN) {
        bNeedReset = true;
        break;
      }

      if (kpOldDlp->iHighestTemporalId != kpNewDlp->iHighestTemporalId) {
        bNeedReset = true;
        break;
      }

      ++ iIndexD;
    } while (iIndexD < pOldParam->iNumDependencyLayer);
  }

  if (bNeedReset) {
    SParaSetOffsetVariable sTmpPsoVariable[PARA_SET_TYPE];
    uint16_t	          uiTmpIdrPicId;//this is for LTR!
    memcpy (sTmpPsoVariable, (*ppCtx)->sPSOVector.sParaSetOffsetVariable,
            (PARA_SET_TYPE)*sizeof (SParaSetOffsetVariable)); // confirmed_safe_unsafe_usage
    uiTmpIdrPicId = (*ppCtx)->sPSOVector.uiIdrPicId;

    WelsUninitEncoderExt (ppCtx);

    /* Update new parameters */
    if (WelsInitEncoderExt (ppCtx, pNewParam))
      return 1;

    // reset the scaled spatial picture size
    (*ppCtx)->pVpp->WelsPreprocessReset (*ppCtx);
    //if WelsInitEncoderExt succeed

    //for FLEXIBLE_PARASET_ID
    memcpy ((*ppCtx)->sPSOVector.sParaSetOffsetVariable, sTmpPsoVariable,
            (PARA_SET_TYPE)*sizeof (SParaSetOffsetVariable)); // confirmed_safe_unsafe_usage
    (*ppCtx)->sPSOVector.uiIdrPicId = uiTmpIdrPicId;
  } else {
    /* maybe adjustment introduced in bitrate or little settings adjustment and so on.. */
    pNewParam->iNumRefFrame								= WELS_CLIP3 (pNewParam->iNumRefFrame, MIN_REF_PIC_COUNT,
                                            MAX_REFERENCE_PICTURE_COUNT_NUM);
    pNewParam->iLoopFilterDisableIdc					= WELS_CLIP3 (pNewParam->iLoopFilterDisableIdc, 0, 6);
    pNewParam->iLoopFilterAlphaC0Offset				= WELS_CLIP3 (pNewParam->iLoopFilterAlphaC0Offset, -6, 6);
    pNewParam->iLoopFilterBetaOffset					= WELS_CLIP3 (pNewParam->iLoopFilterBetaOffset, -6, 6);
    pNewParam->iInterLayerLoopFilterDisableIdc		= WELS_CLIP3 (pNewParam->iInterLayerLoopFilterDisableIdc, 0, 6);
    pNewParam->iInterLayerLoopFilterAlphaC0Offset	= WELS_CLIP3 (pNewParam->iInterLayerLoopFilterAlphaC0Offset, -6, 6);
    pNewParam->iInterLayerLoopFilterBetaOffset		= WELS_CLIP3 (pNewParam->iInterLayerLoopFilterBetaOffset, -6, 6);
    pNewParam->fMaxFrameRate							= WELS_CLIP3 (pNewParam->fMaxFrameRate, MIN_FRAME_RATE, MAX_FRAME_RATE);

    // we can not use direct struct based memcpy due some fields need keep unchanged as before
    pOldParam->fMaxFrameRate	= pNewParam->fMaxFrameRate;		// maximal frame rate [Hz / fps]
    pOldParam->iInputCsp			= pNewParam->iInputCsp;			// color space of input sequence
    pOldParam->uiIntraPeriod		= pNewParam->uiIntraPeriod;		// intra period (multiple of GOP size as desired)
    pOldParam->bEnableSpsPpsIdAddition = pNewParam->bEnableSpsPpsIdAddition;
    pOldParam->bPrefixNalAddingCtrl = pNewParam->bPrefixNalAddingCtrl;
    pOldParam->iNumRefFrame		= pNewParam->iNumRefFrame;		// number of reference frame used

    /* denoise control */
    pOldParam->bEnableDenoise	= pNewParam->bEnableDenoise;

    /* background detection control */
    pOldParam->bEnableBackgroundDetection		= pNewParam->bEnableBackgroundDetection;

    /* adaptive quantization control */
    pOldParam->bEnableAdaptiveQuant	= pNewParam->bEnableAdaptiveQuant;

    /* int32_t term reference control */
    pOldParam->bEnableLongTermReference	= pNewParam->bEnableLongTermReference;
    pOldParam->uiLtrMarkPeriod	= pNewParam->uiLtrMarkPeriod;

    // keep below values unchanged as before
    pOldParam->bEnableSSEI		= pNewParam->bEnableSSEI;
    pOldParam->bEnableFrameCroppingFlag	= pNewParam->bEnableFrameCroppingFlag;	// enable frame cropping flag

    /* Motion search */

    /* Deblocking loop filter */
    pOldParam->iLoopFilterDisableIdc	= pNewParam->iLoopFilterDisableIdc;	// 0: on, 1: off, 2: on except for slice boundaries
    pOldParam->iLoopFilterAlphaC0Offset	= pNewParam->iLoopFilterAlphaC0Offset;// AlphaOffset: valid range [-6, 6], default 0
    pOldParam->iLoopFilterBetaOffset		= pNewParam->iLoopFilterBetaOffset;	// BetaOffset:	valid range [-6, 6], default 0
    pOldParam->iInterLayerLoopFilterDisableIdc	=
      pNewParam->iInterLayerLoopFilterDisableIdc; // Employed based upon inter-layer, same comment as above
    pOldParam->iInterLayerLoopFilterAlphaC0Offset	=
      pNewParam->iInterLayerLoopFilterAlphaC0Offset;	// InterLayerLoopFilterAlphaC0Offset
    pOldParam->iInterLayerLoopFilterBetaOffset		=
      pNewParam->iInterLayerLoopFilterBetaOffset;	// InterLayerLoopFilterBetaOffset

    /* Rate Control */
    pOldParam->bEnableRc			= pNewParam->bEnableRc;
    pOldParam->iRCMode	    	= pNewParam->iRCMode;
    pOldParam->iTargetBitrate	= pNewParam->iTargetBitrate;			// overall target bitrate introduced in RC module
    pOldParam->iPaddingFlag	    = pNewParam->iPaddingFlag;

    /* Layer definition */
    pOldParam->bPrefixNalAddingCtrl	= pNewParam->bPrefixNalAddingCtrl;

    // d
    iIndexD = 0;
    do {
      SDLayerParam* pOldDlp	= &pOldParam->sDependencyLayers[iIndexD];
      SDLayerParam* pNewDlp	= &pNewParam->sDependencyLayers[iIndexD];

      pOldDlp->fInputFrameRate	= pNewDlp->fInputFrameRate;	// input frame rate
      pOldDlp->fOutputFrameRate	= pNewDlp->fOutputFrameRate;	// output frame rate
      pOldDlp->iSpatialBitrate	= pNewDlp->iSpatialBitrate;

      pOldDlp->uiProfileIdc		= pNewDlp->uiProfileIdc;			// value of profile IDC (0 for auto-detection)

      /* Derived variants below */
      pOldDlp->iTemporalResolution	= pNewDlp->iTemporalResolution;
      pOldDlp->iDecompositionStages	= pNewDlp->iDecompositionStages;

      memcpy (pOldDlp->uiCodingIdx2TemporalId, pNewDlp->uiCodingIdx2TemporalId,
              sizeof (pOldDlp->uiCodingIdx2TemporalId));	// confirmed_safe_unsafe_usage

      ++ iIndexD;
    } while (iIndexD < pOldParam->iNumDependencyLayer);
  }

  /* Any else initialization/reset for rate control here? */

  return 0;
}


int32_t WelsCodeOnePicPartition (sWelsEncCtx* pCtx,
                                 SLayerBSInfo* pLayerBsInfo,
                                 int32_t* pNalIdxInLayer,
                                 int32_t* pLayerSize,
                                 int32_t iFirstMbInPartition,	// first mb inclusive in partition
                                 int32_t iEndMbInPartition,	// end mb exclusive in partition
                                 int32_t iStartSliceIdx
                                ) {

  SDqLayer* pCurLayer			= pCtx->pCurDqLayer;
  SSliceCtx* pSliceCtx		= pCurLayer->pSliceEncCtx;
  int32_t iNalLen[MAX_NAL_UNITS_IN_LAYER]			= {0};
  int32_t iNalIdxInLayer		= *pNalIdxInLayer;
  int32_t iSliceIdx				= iStartSliceIdx;
  const int32_t kiSliceStep		= pCtx->iActiveThreadsNum;
  const int32_t kiPartitionId		= iStartSliceIdx % kiSliceStep;
  int32_t iPartitionBsSize		= 0;
  int32_t iAnyMbLeftInPartition = iEndMbInPartition - iFirstMbInPartition;
  const EWelsNalUnitType keNalType	= pCtx->eNalType;
  const EWelsNalRefIdc keNalRefIdc	= pCtx->eNalPriority;
  const bool_t kbNeedPrefix		= pCtx->bNeedPrefixNalFlag;

  //init
  {
    pSliceCtx->pFirstMbInSlice[iSliceIdx]		= iFirstMbInPartition;
    pCurLayer->pNumSliceCodedOfPartition[kiPartitionId]	= 1;	// one slice per partition intialized, dynamic slicing inside
    pCurLayer->pLastMbIdxOfPartition[kiPartitionId]		= iEndMbInPartition - 1;
  }
  pCurLayer->pLastCodedMbIdxOfPartition[kiPartitionId] = 0;

  while (iAnyMbLeftInPartition > 0) {
    int32_t iSliceSize	= 0;

    if (iSliceIdx >= pSliceCtx->iMaxSliceNumConstraint) {	// insufficient memory in pSliceInLayer[]
      // TODO: need exception handler for not large enough of MAX_SLICES_NUM related memory usage
      // No idea about its solution due MAX_SLICES_NUM is fixed lenght in relevent pData structure
      return 1;
    }

    if (kbNeedPrefix) {
      iPartitionBsSize += AddPrefixNal (pCtx, pLayerBsInfo, &iNalLen[0], &iNalIdxInLayer, keNalType, keNalRefIdc);
    }

    WelsLoadNal (pCtx->pOut, keNalType, keNalRefIdc);
    WelsCodeOneSlice (pCtx, iSliceIdx, keNalType);
    WelsUnloadNal (pCtx->pOut);

    iSliceSize = WelsEncodeNalExt (&pCtx->pOut->sNalList[pCtx->pOut->iNalIndex - 1],
                                   &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt,
                                   pCtx->pFrameBs + pCtx->iPosBsBuffer,
                                   &iNalLen[iNalIdxInLayer]);
    pCtx->iPosBsBuffer	+= iSliceSize;
    iPartitionBsSize	+= iSliceSize;
    pLayerBsInfo->iNalLengthInByte[iNalIdxInLayer]	= iSliceSize;

#if defined(SLICE_INFO_OUTPUT)
    fprintf (stderr,
             "@slice=%-6d sliceType:%c idc:%d size:%-6d\n",
             iSliceIdx,
             (pCtx->eSliceType == P_SLICE ? 'P' : 'I'),
             eNalRefIdc,
             iSliceSize);
#endif//SLICE_INFO_OUTPUT

    ++ iNalIdxInLayer;
    iSliceIdx += kiSliceStep;	//if uiSliceIdx is not continuous
    iAnyMbLeftInPartition = iEndMbInPartition - (1 + pCurLayer->pLastCodedMbIdxOfPartition[kiPartitionId]);
  }

  *pLayerSize			= iPartitionBsSize;
  *pNalIdxInLayer	= iNalIdxInLayer;

  // slice based packing???
  pLayerBsInfo->uiLayerType		= VIDEO_CODING_LAYER;
  pLayerBsInfo->uiSpatialId		= pCtx->uiDependencyId;
  pLayerBsInfo->uiTemporalId	= pCtx->uiTemporalId;
  pLayerBsInfo->uiQualityId		= 0;
  pLayerBsInfo->uiPriorityId	= 0;
  pLayerBsInfo->iNalCount		= iNalIdxInLayer;

  return 0;
}
} // namespace WelsSVCEnc
