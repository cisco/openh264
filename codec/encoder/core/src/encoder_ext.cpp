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
#include "svc_mode_decision.h"
#include "decode_mb_aux.h"
#include "deblocking.h"
#include "ref_list_mgr_svc.h"
#include "ls_defines.h"
#include "crt_util_safe_x.h"	// Safe CRT routines like utils for cross platforms
#include "slice_multi_threading.h"
#include "measure_time.h"

namespace WelsEnc {


int32_t WelsCodeOnePicPartition (sWelsEncCtx* pCtx,
                                 SFrameBSInfo* pFrameBsInfo,
                                 SLayerBSInfo* pLayerBsInfo,
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
int32_t ParamValidation (SLogContext* pLogCtx, SWelsSvcCodingParam* pCfg) {
  const float fEpsn = 0.000001f;
  int32_t i = 0;

  assert (pCfg != NULL);

  if ((pCfg->iUsageType != CAMERA_VIDEO_REAL_TIME) && (pCfg->iUsageType != SCREEN_CONTENT_REAL_TIME)) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidation(),Invalid usage type = %d", pCfg->iUsageType);
    return ENC_RETURN_UNSUPPORTED_PARA;
  }
  if (pCfg->iUsageType == SCREEN_CONTENT_REAL_TIME) {
    if (pCfg->iSpatialLayerNum > 1) {
      WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidation(),Invalid the number of Spatial layer(%d)for screen content",
               pCfg->iSpatialLayerNum);
      return ENC_RETURN_UNSUPPORTED_PARA;
    }
    if (pCfg->bEnableAdaptiveQuant) {
      WelsLog (pLogCtx, WELS_LOG_WARNING,
               "ParamValidation(), AdaptiveQuant(%d) is not supported yet for screen content, auto turned off",
               pCfg->bEnableAdaptiveQuant);
      pCfg->bEnableAdaptiveQuant = false;
    }
    if (pCfg->bEnableSceneChangeDetect == false) {
      pCfg->bEnableSceneChangeDetect = true;
      WelsLog (pLogCtx, WELS_LOG_WARNING,
               "ParamValidation(), screen change detection should be turned on,change bEnableSceneChangeDetect as true");
    }

  }
  if (pCfg->iSpatialLayerNum > 1) {
    int32_t iFinalWidth = pCfg->sSpatialLayers[pCfg->iSpatialLayerNum - 1].iVideoWidth;
    int32_t iFinalHeight = pCfg->sSpatialLayers[pCfg->iSpatialLayerNum - 1].iVideoHeight;
    for (i = 0; i < (pCfg->iSpatialLayerNum - 1); i++) {
      SSpatialLayerConfig* fDlp = &pCfg->sSpatialLayers[i];
      if ((fDlp->iVideoWidth > iFinalWidth) || (fDlp->iVideoHeight > iFinalHeight)) {
        WelsLog (pLogCtx, WELS_LOG_ERROR,
                 "ParamValidation,Invalid resolution layer(%d) resolution(%d x %d) shoudl be less than the highest spatial layer resolution(%d x %d) ",
                 i, fDlp->iVideoWidth, fDlp->iVideoHeight, iFinalWidth, iFinalHeight);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }
    }
  }
  for (i = 0; i < pCfg->iSpatialLayerNum; ++ i) {
    SSpatialLayerInternal* fDlp = &pCfg->sDependencyLayers[i];
    if (fDlp->fOutputFrameRate > fDlp->fInputFrameRate || (fDlp->fInputFrameRate >= -fEpsn
        && fDlp->fInputFrameRate <= fEpsn)
        || (fDlp->fOutputFrameRate >= -fEpsn && fDlp->fOutputFrameRate <= fEpsn)) {
      WelsLog (pLogCtx, WELS_LOG_ERROR,
               "Invalid settings in input frame rate(%.6f) or output frame rate(%.6f) of layer #%d config file..",
               fDlp->fInputFrameRate, fDlp->fOutputFrameRate, i);
      return ENC_RETURN_INVALIDINPUT;
    }
    if (UINT_MAX == GetLogFactor (fDlp->fOutputFrameRate, fDlp->fInputFrameRate)) {
      WelsLog (pLogCtx, WELS_LOG_WARNING,
               "AUTO CORRECT: Invalid settings in input frame rate(%.6f) and output frame rate(%.6f) of layer #%d config file: iResult of output frame rate divided by input frame rate should be power of 2(i.e,in/pOut=2^n). \n Auto correcting Output Framerate to Input Framerate %f!\n",
               fDlp->fInputFrameRate, fDlp->fOutputFrameRate, i, fDlp->fInputFrameRate);
      fDlp->fOutputFrameRate = fDlp->fInputFrameRate;
    }
  }

  if ((pCfg->iRCMode != RC_OFF_MODE) && (pCfg->iRCMode != RC_QUALITY_MODE) && (pCfg->iRCMode != RC_BUFFERBASED_MODE)
      && (pCfg->iRCMode != RC_BITRATE_MODE)) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidation(),Invalid iRCMode = %d", pCfg->iRCMode);
    return ENC_RETURN_UNSUPPORTED_PARA;
  }
  //bitrate setting validation
  if (pCfg->iRCMode != RC_OFF_MODE) {
    int32_t  iTotalBitrate = 0;
    if (pCfg->iTargetBitrate <= 0) {
      WelsLog (pLogCtx, WELS_LOG_ERROR, "Invalid bitrate settings in total configure, bitrate= %d", pCfg->iTargetBitrate);
      return ENC_RETURN_INVALIDINPUT;
    }
    for (i = 0; i < pCfg->iSpatialLayerNum; ++ i) {
      SSpatialLayerConfig* pSpatialLayer = &pCfg->sSpatialLayers[i];
      iTotalBitrate += pSpatialLayer->iSpatialBitrate;
      if (pSpatialLayer->iSpatialBitrate <= 0) {
        WelsLog (pLogCtx, WELS_LOG_ERROR, "Invalid bitrate settings in layer %d, bitrate= %d", i,
                 pSpatialLayer->iSpatialBitrate);
        return ENC_RETURN_INVALIDINPUT;
      }
      if (pSpatialLayer->iMaxSpatialBitrate < pSpatialLayer->iSpatialBitrate * 1.1f) {
        WelsLog (pLogCtx, WELS_LOG_WARNING,
          "MaxSpatialBitrate (%d) should set be larger than 1.1 times of SpatialBitrate (%d)",
          pSpatialLayer->iMaxSpatialBitrate, pSpatialLayer->iSpatialBitrate);
 //       pSpatialLayer->iSpatialBitrate = (int32_t) (pSpatialLayer->iMaxSpatialBitrate/1.1f);
      }
    }
    if (iTotalBitrate > pCfg->iTargetBitrate) {
      WelsLog (pLogCtx, WELS_LOG_ERROR,
               "Invalid settings in bitrate. the sum of each layer bitrate(%d) is larger than total bitrate setting(%d)",
               iTotalBitrate, pCfg->iTargetBitrate);
      return ENC_RETURN_INVALIDINPUT;
    }
    if ((pCfg->iRCMode == RC_QUALITY_MODE) || (pCfg->iRCMode == RC_BITRATE_MODE))
      if (!pCfg->bEnableFrameSkip)
        WelsLog (pLogCtx, WELS_LOG_WARNING,
                 "bEnableFrameSkip = %d,bitrate can't be controlled for RC_QUALITY_MODE and RC_BITRATE_MODE without enabling skip frame.",
                 pCfg->bEnableFrameSkip);
  }
  return WelsCheckRefFrameLimitation (pLogCtx, pCfg);
}


int32_t ParamValidationExt (SLogContext* pLogCtx, SWelsSvcCodingParam* pCodingParam) {
  int8_t i = 0;
  int32_t iIdx = 0;

  assert (pCodingParam != NULL);
  if (NULL == pCodingParam)
    return ENC_RETURN_INVALIDINPUT;

  if ((pCodingParam->iUsageType != CAMERA_VIDEO_REAL_TIME) && (pCodingParam->iUsageType != SCREEN_CONTENT_REAL_TIME)) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(),Invalid usage type = %d", pCodingParam->iUsageType);
    return ENC_RETURN_UNSUPPORTED_PARA;
  }
  if ((pCodingParam->iUsageType == SCREEN_CONTENT_REAL_TIME) && (!pCodingParam->bIsLosslessLink
      && pCodingParam->bEnableLongTermReference)) {
    WelsLog (pLogCtx, WELS_LOG_WARNING,
             "ParamValidationExt(), setting lossy link for LTR under screen, which is not supported yet! Auto disabled LTR!");
    pCodingParam->bEnableLongTermReference = false;
  }
  if (pCodingParam->iSpatialLayerNum < 1 || pCodingParam->iSpatialLayerNum > MAX_DEPENDENCY_LAYER) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), monitor invalid pCodingParam->iSpatialLayerNum: %d!",
             pCodingParam->iSpatialLayerNum);
    return ENC_RETURN_UNSUPPORTED_PARA;
  }

  if (pCodingParam->iTemporalLayerNum < 1 || pCodingParam->iTemporalLayerNum > MAX_TEMPORAL_LEVEL) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), monitor invalid pCodingParam->iTemporalLayerNum: %d!",
             pCodingParam->iTemporalLayerNum);
    return ENC_RETURN_UNSUPPORTED_PARA;
  }

  if (pCodingParam->uiGopSize < 1 || pCodingParam->uiGopSize > MAX_GOP_SIZE) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), monitor invalid pCodingParam->uiGopSize: %d!",
             pCodingParam->uiGopSize);
    return ENC_RETURN_UNSUPPORTED_PARA;
  }


  if (pCodingParam->uiIntraPeriod && pCodingParam->uiIntraPeriod < pCodingParam->uiGopSize) {
    WelsLog (pLogCtx, WELS_LOG_ERROR,
             "ParamValidationExt(), uiIntraPeriod(%d) should be not less than that of uiGopSize(%d) or -1 specified!",
             pCodingParam->uiIntraPeriod, pCodingParam->uiGopSize);
    return ENC_RETURN_UNSUPPORTED_PARA;
  }

  if (pCodingParam->uiIntraPeriod && (pCodingParam->uiIntraPeriod & (pCodingParam->uiGopSize - 1)) != 0) {
    WelsLog (pLogCtx, WELS_LOG_ERROR,
             "ParamValidationExt(), uiIntraPeriod(%d) should be multiple of uiGopSize(%d) or -1 specified!",
             pCodingParam->uiIntraPeriod, pCodingParam->uiGopSize);
    return ENC_RETURN_UNSUPPORTED_PARA;
  }


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

  for (i = 0; i < pCodingParam->iSpatialLayerNum; ++ i) {
    SSpatialLayerConfig* pSpatialLayer = &pCodingParam->sSpatialLayers[i];
    const int32_t kiPicWidth = pSpatialLayer->iVideoWidth;
    const int32_t kiPicHeight = pSpatialLayer->iVideoHeight;
    uint32_t iMbWidth		= 0;
    uint32_t iMbHeight		= 0;
    int32_t iMbNumInFrame		= 0;
    uint32_t iMaxSliceNum		= MAX_SLICES_NUM;
    if ((kiPicWidth <= 0) || (kiPicHeight <= 0) || (kiPicWidth > MAX_WIDTH) || (kiPicHeight > MAX_HEIGHT)) {
      WelsLog (pLogCtx, WELS_LOG_ERROR,
               "ParamValidationExt(),width(1-%d),height(1-%d)invalid %d x %d in dependency layer settings!", MAX_WIDTH, MAX_HEIGHT,
               kiPicWidth, kiPicHeight);
      return ENC_RETURN_UNSUPPORTED_PARA;
    }
    if ((kiPicWidth & 0x0F) != 0 || (kiPicHeight & 0x0F) != 0) {
      WelsLog (pLogCtx, WELS_LOG_ERROR,
               "ParamValidationExt(), in layer #%d iWidth x iHeight(%d x %d) both should be multiple of 16, can not support with arbitrary size currently!",
               i, kiPicWidth, kiPicHeight);
      return ENC_RETURN_UNSUPPORTED_PARA;
    }

    if (pSpatialLayer->sSliceCfg.uiSliceMode >= SM_RESERVED) {
      WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceMode (%d) settings!",
               pSpatialLayer->sSliceCfg.uiSliceMode);
      return ENC_RETURN_UNSUPPORTED_PARA;
    }
    if ((pCodingParam->uiMaxNalSize != 0) && (pSpatialLayer->sSliceCfg.uiSliceMode != SM_DYN_SLICE)) {
      WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceMode (%d) settings!,MaxNalSize = %d",
               pSpatialLayer->sSliceCfg.uiSliceMode, pCodingParam->uiMaxNalSize);
      return ENC_RETURN_UNSUPPORTED_PARA;
    }
    //check pSlice settings under multi-pSlice
    if (kiPicWidth <= 16 && kiPicHeight <= 16) {
      //only have one MB, set to single_slice
      pSpatialLayer->sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
    }
    switch (pSpatialLayer->sSliceCfg.uiSliceMode) {
    case SM_SINGLE_SLICE:
      pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum = 1;
      pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 0;
      for (iIdx = 0; iIdx < MAX_SLICES_NUM; iIdx++) {
        pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceMbNum[iIdx] = 0;
      }
      break;
    case SM_FIXEDSLCNUM_SLICE: {
      pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 0;

      iMbWidth	= (kiPicWidth + 15) >> 4;
      iMbHeight	= (kiPicHeight + 15) >> 4;
      iMbNumInFrame = iMbWidth * iMbHeight;
      iMaxSliceNum = MAX_SLICES_NUM;
      if (pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum <= 0
          || pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum > iMaxSliceNum) {
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceNum (%d) settings!",
                 pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }
      if (pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum == 1) {
        WelsLog (pLogCtx, WELS_LOG_DEBUG,
                 "ParamValidationExt(), uiSliceNum(%d) you set for SM_FIXEDSLCNUM_SLICE, now turn to SM_SINGLE_SLICE type!",
                 pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum);
        pSpatialLayer->sSliceCfg.uiSliceMode	= SM_SINGLE_SLICE;
        break;
      }
      if (pCodingParam->iRCMode != RC_OFF_MODE) {	// multiple slices verify with gom
        //check uiSliceNum
        GomValidCheckSliceNum (iMbWidth, iMbHeight, &pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum);
        assert (pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum > 1);
        //set uiSliceMbNum with current uiSliceNum
        GomValidCheckSliceMbNum (iMbWidth, iMbHeight, &pSpatialLayer->sSliceCfg.sSliceArgument);
      } else if (!CheckFixedSliceNumMultiSliceSetting (iMbNumInFrame,
                 &pSpatialLayer->sSliceCfg.sSliceArgument)) {	// verify interleave mode settings
        //check uiSliceMbNum with current uiSliceNum
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceMbNum (%d) settings!",
                 pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceMbNum[0]);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }
      // considering the coding efficient and performance, iCountMbNum constraint by MIN_NUM_MB_PER_SLICE condition of multi-pSlice mode settting
      if (iMbNumInFrame <= MIN_NUM_MB_PER_SLICE) {
        pSpatialLayer->sSliceCfg.uiSliceMode	= SM_SINGLE_SLICE;
        pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum	= 1;
        break;
      }
    }
    break;
    case SM_AUTO_SLICE: {
      pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 0;
    }
    break;
    case SM_RASTER_SLICE: {
      pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 0;

      iMbWidth	= (kiPicWidth + 15) >> 4;
      iMbHeight	= (kiPicHeight + 15) >> 4;
      iMbNumInFrame = iMbWidth * iMbHeight;
      iMaxSliceNum = MAX_SLICES_NUM;
      if (pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceMbNum[0] <= 0) {
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceMbNum (%d) settings!",
                 pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceMbNum[0]);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }

      if (!CheckRasterMultiSliceSetting (iMbNumInFrame,
                                         &pSpatialLayer->sSliceCfg.sSliceArgument)) {	// verify interleave mode settings
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceMbNum (%d) settings!",
                 pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceMbNum[0]);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }
      if (pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum <= 0
          || pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum > iMaxSliceNum) {	// verify interleave mode settings
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceNum (%d) in SM_RASTER_SLICE settings!",
                 pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }
      if (pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum == 1) {
        WelsLog (pLogCtx, WELS_LOG_ERROR,
                 "ParamValidationExt(), pSlice setting for SM_RASTER_SLICE now turn to SM_SINGLE_SLICE!");
        pSpatialLayer->sSliceCfg.uiSliceMode	= SM_SINGLE_SLICE;
        break;
      }
      if ((pCodingParam->iRCMode != RC_OFF_MODE) && pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum > 1) {
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), WARNING: GOM based RC do not support SM_RASTER_SLICE!");
      }
      // considering the coding efficient and performance, iCountMbNum constraint by MIN_NUM_MB_PER_SLICE condition of multi-pSlice mode settting
      if (iMbNumInFrame <= MIN_NUM_MB_PER_SLICE) {
        pSpatialLayer->sSliceCfg.uiSliceMode	= SM_SINGLE_SLICE;
        pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum	= 1;
        break;
      }
    }
    break;
    case SM_ROWMB_SLICE: {
      pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 0;

      iMbWidth	= (kiPicWidth + 15) >> 4;
      iMbHeight	= (kiPicHeight + 15) >> 4;
      iMaxSliceNum = MAX_SLICES_NUM;
      if (iMbHeight > iMaxSliceNum) {
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceNum (%d) settings more than MAX!", iMbHeight);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }
      pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum	= iMbHeight;

      if (pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum <= 0) {
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceNum (%d) settings!",
                 pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }
      if (!CheckRowMbMultiSliceSetting (iMbWidth,
                                        &pSpatialLayer->sSliceCfg.sSliceArgument)) {	// verify interleave mode settings
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceMbNum (%d) settings!",
                 pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceMbNum[0]);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }
    }
    break;
    case SM_DYN_SLICE: {
      iMbWidth	= (kiPicWidth + 15) >> 4;
      iMbHeight	= (kiPicHeight + 15) >> 4;
      if (pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint <= 0) {
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid iSliceSize (%d) settings!",
                 pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }

      if (pCodingParam->uiMaxNalSize <= NAL_HEADER_ADD_0X30BYTES) {
        WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiMaxNalSize (%d) settings!",
                 pCodingParam->uiMaxNalSize);
        return ENC_RETURN_UNSUPPORTED_PARA;
      }

      if (pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint > (pCodingParam->uiMaxNalSize -
          NAL_HEADER_ADD_0X30BYTES)) {
        WelsLog (pLogCtx, WELS_LOG_WARNING,
                 "ParamValidationExt(), slice mode = SM_DYN_SLICE, uiSliceSizeConstraint = %d ,uiMaxNalsize = %d!",
                 pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint, pCodingParam->uiMaxNalSize);
        pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint =  pCodingParam->uiMaxNalSize - NAL_HEADER_ADD_0X30BYTES;
      }

    }
    break;
    default: {
      WelsLog (pLogCtx, WELS_LOG_ERROR, "ParamValidationExt(), invalid uiSliceMode (%d) settings!",
               pCodingParam->sSpatialLayers[0].sSliceCfg.uiSliceMode);
      return ENC_RETURN_UNSUPPORTED_PARA;

    }
    break;
    }
  }

  return ParamValidation (pLogCtx, pCodingParam);
}


void WelsEncoderApplyFrameRate (SWelsSvcCodingParam* pParam) {
  SSpatialLayerInternal* pLayerParam;
  const float kfEpsn = 0.000001f;
  const int32_t kiNumLayer = pParam->iSpatialLayerNum;
  int32_t i;
  const float kfMaxFrameRate = pParam->fMaxFrameRate;
  float fRatio;
  float fTargetOutputFrameRate;

  //set input frame rate to each layer
  for (i = 0; i < kiNumLayer; i++) {
    pLayerParam = & (pParam->sDependencyLayers[i]);

    fRatio = pLayerParam->fOutputFrameRate / pLayerParam->fInputFrameRate;
    if ((kfMaxFrameRate - pLayerParam->fInputFrameRate) > kfEpsn
        || (kfMaxFrameRate - pLayerParam->fInputFrameRate) < -kfEpsn) {
      pLayerParam->fInputFrameRate = kfMaxFrameRate;
      fTargetOutputFrameRate = kfMaxFrameRate * fRatio;
      pLayerParam->fOutputFrameRate = (fTargetOutputFrameRate >= 6) ? fTargetOutputFrameRate : (pLayerParam->fInputFrameRate);
      //TODO:{Sijia} from design, there is no sense to have temporal layer when under 6fps even with such setting?
    }
  }
}


void WelsEncoderApplyBitRate (SLogContext* pLogCtx, SWelsSvcCodingParam* pParam, int iLayer) {
  //TODO (Sijia):  this is a temporary solution which keep the ratio between layers
  //but it is also possible to fulfill the bitrate of lower layer first

  SSpatialLayerConfig* pLayerParam;
  const int32_t iNumLayers = pParam->iSpatialLayerNum;
  int32_t i, iOrigTotalBitrate = 0;
  if (iLayer == SPATIAL_LAYER_ALL) {
    //read old BR
    for (i = 0; i < iNumLayers; i++) {
      iOrigTotalBitrate += pParam->sSpatialLayers[i].iSpatialBitrate;
    }
    //write new BR
    float fRatio = 0.0;
    for (i = 0; i < iNumLayers; i++) {
      pLayerParam = & (pParam->sSpatialLayers[i]);
      fRatio = pLayerParam->iSpatialBitrate / (static_cast<float> (iOrigTotalBitrate));
      pLayerParam->iSpatialBitrate = static_cast<int32_t> (pParam->iTargetBitrate * fRatio);
    }
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

  if (NULL == pParam || NULL == ppCtx || NULL == *ppCtx)
    return 1;

  iNumDependencyLayers	= pParam->iSpatialLayerNum;

  do {
    SSpatialLayerConfig* pDLayer = &pParam->sSpatialLayers[iDIndex];
//		pDLayer->ptr_cfg = pParam;
    int32_t iOrgNumNals = iCountNumNals;

    //Note: Sep. 2010
    //Review this part and suggest no change, since the memory over-use
    //(1) counts little to the overall performance
    //(2) should not be critial even under mobile case
    if (SM_DYN_SLICE == pDLayer->sSliceCfg.uiSliceMode) {
      iCountNumNals += MAX_SLICES_NUM;
      // plus prefix NALs
      if (iDIndex == 0)
        iCountNumNals += MAX_SLICES_NUM;
      // MAX_SLICES_NUM < MAX_LAYER_NUM_OF_FRAME ensured at svc_enc_slice_segment.h
      assert (iCountNumNals - iOrgNumNals <= MAX_NAL_UNITS_IN_LAYER);
    } else { /*if ( SM_SINGLE_SLICE != pDLayer->sSliceCfg.uiSliceMode )*/
      const int32_t kiNumOfSlice = GetInitialSliceNum ((pDLayer->iVideoWidth + 0x0f) >> 4,
                                   (pDLayer->iVideoHeight + 0x0f) >> 4,
                                   &pDLayer->sSliceCfg);

      // NEED check iCountNals value in case multiple slices is used
      iCountNumNals += kiNumOfSlice; // for pSlice VCL NALs
      // plus prefix NALs
      if (iDIndex == 0)
        iCountNumNals += kiNumOfSlice;
      assert (iCountNumNals - iOrgNumNals <= MAX_NAL_UNITS_IN_LAYER);
      if (kiNumOfSlice > MAX_SLICES_NUM) {
        WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_ERROR,
                 "AcquireLayersNals(), num_of_slice(%d) > MAX_SLICES_NUM(%d) per (iDid= %d, qid= %d) settings!",
                 kiNumOfSlice, MAX_SLICES_NUM, iDIndex, 0);
        return 1;
      }
    }

    if (iCountNumNals - iOrgNumNals > MAX_NAL_UNITS_IN_LAYER) {
      WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_ERROR,
               "AcquireLayersNals(), num_of_nals(%d) > MAX_NAL_UNITS_IN_LAYER(%d) per (iDid= %d, qid= %d) settings!",
               (iCountNumNals - iOrgNumNals), MAX_NAL_UNITS_IN_LAYER, iDIndex, 0);
      return 1;
    }

    iCountNumLayers ++;

    ++ iDIndex;
  } while (iDIndex < iNumDependencyLayers);

  iCountNumNals += 1 + iNumDependencyLayers + (iCountNumLayers << 1) +
                   iCountNumLayers;	// plus iCountNumLayers for reserved application

  // to check number of layers / nals / slices dependencies, 12/8/2010
  if (iCountNumLayers > MAX_LAYER_NUM_OF_FRAME) {
    WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_ERROR, "AcquireLayersNals(), iCountNumLayers(%d) > MAX_LAYER_NUM_OF_FRAME(%d)!",
             iCountNumLayers, MAX_LAYER_NUM_OF_FRAME);
    return 1;
  }

  if (NULL != pCountLayers)
    *pCountLayers = iCountNumLayers;
  if (NULL != pCountNals)
    *pCountNals	= iCountNumNals;
  return 0;
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
    bool     bLeft;
    bool     bTop;
    bool     bLeftTop;
    bool     bRightTop;
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
  int32_t		iNumDlayer = (*ppCtx)->pSvcParam->iSpatialLayerNum;
  int32_t		iMbSize[MAX_DEPENDENCY_LAYER] = { 0 };
  int32_t		iOverallMbNum = 0;
  int32_t		iMbWidth = 0;
  int32_t		iMbHeight = 0;
  int32_t		i;

  if (iNumDlayer > MAX_DEPENDENCY_LAYER)
    return 1;

  for (i = 0; i < iNumDlayer; i++) {
    iMbWidth = ((*ppCtx)->pSvcParam->sSpatialLayers[i].iVideoWidth + 15) >> 4;
    iMbHeight = ((*ppCtx)->pSvcParam->sSpatialLayers[i].iVideoHeight + 15) >> 4;
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
  pMbCache->pPrevIntra4x4PredModeFlag = (bool*)pMa->WelsMalloc (16 * sizeof (bool),
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
  iDlayerCount	= pParam->iSpatialLayerNum;
  iNumRef	= pParam->iMaxNumRefFrame;

  const int32_t kiFeatureStrategyIndex = FME_DEFAULT_FEATURE_INDEX;
  const int32_t kiMe16x16 = ME_DIA_CROSS;
  const int32_t kiMe8x8 = ME_DIA_CROSS_FME;
  const int32_t kiNeedFeatureStorage = (pParam->iUsageType != SCREEN_CONTENT_REAL_TIME) ? 0 :
                                       ((kiFeatureStrategyIndex << 16) + ((kiMe16x16 & 0x00FF) << 8) + (kiMe8x8 & 0x00FF));

  iDlayerIndex			= 0;
  while (iDlayerIndex < iDlayerCount) {
    SRefList* pRefList			= NULL;
    uint32_t i					= 0;
    const int32_t kiWidth			= pParam->sSpatialLayers[iDlayerIndex].iVideoWidth;
    const int32_t kiHeight		= pParam->sSpatialLayers[iDlayerIndex].iVideoHeight;
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
      pRefList->pRef[i]	= AllocPicture (pMa, kiWidth, kiHeight, true,
                                        (iDlayerIndex == iDlayerCount - 1) ? kiNeedFeatureStorage : 0);	// to use actual size of current layer
      WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pRefList->pRef[i]), FreeMemorySvc (ppCtx))
      ++ i;
    } while (i < 1 + iNumRef);

    pRefList->pNextBuffer = pRefList->pRef[0];
    (*ppCtx)->ppRefPicListExt[iDlayerIndex]	= pRefList;
    ++ iDlayerIndex;
  }

  iDlayerIndex	= 0;
  while (iDlayerIndex < iDlayerCount) {
    SDqLayer* pDqLayer		= NULL;
    SSpatialLayerConfig* pDlayer	= &pParam->sSpatialLayers[iDlayerIndex];
    const int32_t kiMbW		= (pDlayer->iVideoWidth + 0x0f) >> 4;
    const int32_t kiMbH		= (pDlayer->iVideoHeight + 0x0f) >> 4;
    int32_t iMaxSliceNum	= 1;
    const int32_t kiSliceNum = GetInitialSliceNum (kiMbW, kiMbH, &pDlayer->sSliceCfg);
    if (iMaxSliceNum < kiSliceNum)
      iMaxSliceNum = kiSliceNum;

    // pDq layers list
    pDqLayer = (SDqLayer*)pMa->WelsMallocz (sizeof (SDqLayer), "pDqLayer");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pDqLayer), FreeMemorySvc (ppCtx))

    // for dynamic slicing mode
    if (SM_DYN_SLICE == pDlayer->sSliceCfg.uiSliceMode) {
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
    {
      int32_t iSliceIdx		= 0;
      pDqLayer->sLayerInfo.pSliceInLayer	= (SSlice*)pMa->WelsMallocz (sizeof (SSlice) * iMaxSliceNum, "pSliceInLayer");

      WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pDqLayer->sLayerInfo.pSliceInLayer), FreeMemorySvc (ppCtx))
      if (iMaxSliceNum > 1) {
        while (iSliceIdx < iMaxSliceNum) {
          SSlice* pSlice = &pDqLayer->sLayerInfo.pSliceInLayer[iSliceIdx];
          pSlice->uiSliceIdx = iSliceIdx;
          if (pParam->iMultipleThreadIdc > 1)
            pSlice->pSliceBsa = & (*ppCtx)->pSliceBs[iSliceIdx].sBsWrite;
          else
            pSlice->pSliceBsa = & (*ppCtx)->pOut->sBsWrite;
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
    //parallel deblocking
    pDqLayer->bDeblockingParallelFlag                  = pParam->bDeblockingParallelFlag;

    //deblocking parameter adjustment
    if (SM_SINGLE_SLICE == pDlayer->sSliceCfg.uiSliceMode) {
      //iLoopFilterDisableIdc: will be 0 or 1 under single_slice
      if (2 == pParam->iLoopFilterDisableIdc) {
        pDqLayer->iLoopFilterDisableIdc	= 0;
      }
      //bDeblockingParallelFlag
      pDqLayer->bDeblockingParallelFlag = false;
    } else {
      //multi-pSlice
      if (0 == pDqLayer->iLoopFilterDisableIdc) {
        pDqLayer->bDeblockingParallelFlag	= false;
      }
    }

    //
    if (kiNeedFeatureStorage && iDlayerIndex == iDlayerCount - 1) {
      pDqLayer->pFeatureSearchPreparation	= static_cast<SFeatureSearchPreparation*> (pMa->WelsMallocz (sizeof (
                                              SFeatureSearchPreparation), "pFeatureSearchPreparation"));
      WELS_VERIFY_RETURN_PROC_IF (1, NULL == pDqLayer->pFeatureSearchPreparation, FreeMemorySvc (ppCtx));
      int32_t iReturn = RequestFeatureSearchPreparation (pMa, pDlayer->iVideoWidth, pDlayer->iVideoHeight,
                        kiNeedFeatureStorage,
                        pDqLayer->pFeatureSearchPreparation);
      WELS_VERIFY_RETURN_PROC_IF (1, ENC_RETURN_SUCCESS != iReturn, FreeMemorySvc (ppCtx));
    } else {
      pDqLayer->pFeatureSearchPreparation = NULL;
    }

    (*ppCtx)->ppDqLayerList[iDlayerIndex]	= pDqLayer;

    ++ iDlayerIndex;
  }

  // for dynamically malloc for parameter sets memory instead of maximal items for standard to reduce size, 3/18/2010
  (*ppCtx)->pPPSArray	= (SWelsPPS*)pMa->WelsMalloc (iDlayerCount * sizeof (SWelsPPS), "pPPSArray");
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
    const bool bUseSubsetSps			= (iDlayerIndex > BASE_DEPENDENCY_ID);
    SSpatialLayerConfig* pDlayerParam	= &pParam->sSpatialLayers[iDlayerIndex];

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
      WelsInitSps (pSps, pDlayerParam, &pParam->sDependencyLayers[iDlayerIndex], pParam->uiIntraPeriod,
                   pParam->iMaxNumRefFrame,
                   iSpsId, pParam->bEnableFrameCroppingFlag, pParam->iRCMode != RC_OFF_MODE);

      if (iDlayerCount > 1) {
        pSps->bConstraintSet0Flag = true;
        pSps->bConstraintSet1Flag = true;
        pSps->bConstraintSet2Flag = true;
      }
    } else {
      WelsInitSubsetSps (pSubsetSps, pDlayerParam, &pParam->sDependencyLayers[iDlayerIndex], pParam->uiIntraPeriod,
                         pParam->iMaxNumRefFrame,
                         iSpsId, pParam->bEnableFrameCroppingFlag, pParam->iRCMode != RC_OFF_MODE);
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
                                  & (pDlayerParam->sSliceCfg),
                                  pPps);
      if (iResult) {
        WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_WARNING, "InitDqLayers(), InitSlicePEncCtx failed(%d)!", iResult);
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
  } sMbSizeMap[MAX_DEPENDENCY_LAYER] = {{ 0 }};
  int32_t iLineSizeY[MAX_DEPENDENCY_LAYER][2] = {{ 0 }};
  int32_t iLineSizeUV[MAX_DEPENDENCY_LAYER][2] = {{ 0 }};
  int32_t iMapSpatialIdx[MAX_DEPENDENCY_LAYER][2] = {{ 0 }};
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

  iCntTid	= pParam->iTemporalLayerNum > 1 ? 2 : 1;

  iSpatialIdx = 0;
  while (iSpatialIdx < kiNumSpatialLayers) {
    const int32_t kiTmpWidth = (pParam->sSpatialLayers[iSpatialIdx].iVideoWidth + 15) >> 4;
    const int32_t kiTmpHeight = (pParam->sSpatialLayers[iSpatialIdx].iVideoHeight + 15) >> 4;
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
    const bool kbBaseTemporalFlag	= (iTemporalIdx == 0);

    iSpatialIdx = 0;
    while (iSpatialIdx < kiNumSpatialLayers) {
      SSpatialLayerConfig* fDlp					= &pParam->sSpatialLayers[iSpatialIdx];

      const int32_t kiWidthPad = WELS_ALIGN (fDlp->iVideoWidth, 16) + (PADDING_LENGTH << 1);
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
    const bool kbBaseTemporalFlag	= (iTemporalIdx == 0);

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
    const bool kbBaseTemporalFlag	= (iTemporalIdx == 0);

    iSpatialIdx = 0;
    while (iSpatialIdx < kiNumSpatialLayers) {
      int32_t iMatchIndex = 0;
      bool bInMap = false;
      bool bMatchFlag = false;

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
int32_t RequestMemoryVaaScreen (SVAAFrameInfo* pVaa,  CMemoryAlign* pMa,  const int32_t iNumRef,
                                const int32_t iCountMax8x8BNum) {
  SVAAFrameInfoExt* pVaaExt = static_cast<SVAAFrameInfoExt*> (pVaa);

  pVaaExt->pVaaBlockStaticIdc[0] = (static_cast<uint8_t*> (pMa->WelsMallocz (iNumRef * iCountMax8x8BNum * sizeof (
                                      uint8_t), "pVaa->pVaaBlockStaticIdc[0]")));
  if (NULL == pVaaExt->pVaaBlockStaticIdc[0]) {
    return 1;
  }

  for (int32_t idx = 1; idx < iNumRef; idx++) {
    pVaaExt->pVaaBlockStaticIdc[idx] = pVaaExt->pVaaBlockStaticIdc[idx - 1] + iCountMax8x8BNum;
  }
  return 0;
}
void ReleaseMemoryVaaScreen (SVAAFrameInfo* pVaa,  CMemoryAlign* pMa, const int32_t iNumRef) {
  SVAAFrameInfoExt* pVaaExt = static_cast<SVAAFrameInfoExt*> (pVaa);
  if (pVaaExt && pMa && pVaaExt->pVaaBlockStaticIdc[0]) {
    pMa->WelsFree (pVaaExt->pVaaBlockStaticIdc[0], "pVaa->pVaaBlockStaticIdc[0]");

    for (int32_t idx = 0; idx < iNumRef; idx++) {
      pVaaExt->pVaaBlockStaticIdc[idx] = NULL;
    }
  }
}
/*!
 * \brief	request specific memory for SVC
 * \pParam	pEncCtx		sWelsEncCtx*
 * \return	successful - 0; otherwise none 0 for failed
 */
void GetMvMvdRange (SWelsSvcCodingParam* pParam, int32_t& iMvRange, int32_t& iMvdRange) {
  ELevelIdc iMinLevelIdc = LEVEL_5_2;
  int32_t iMinMv = 0;
  int32_t iMaxMv = 0;
  int32_t iFixMvRange = pParam->iUsageType ? EXPANDED_MV_RANGE : CAMERA_STARTMV_RANGE;
  int32_t iFixMvdRange = (pParam->iUsageType ? EXPANDED_MVD_RANGE : ((pParam->iSpatialLayerNum == 1) ? CAMERA_MVD_RANGE :
                          CAMERA_HIGHLAYER_MVD_RANGE));
  for (int32_t iLayer = 0; iLayer < pParam->iSpatialLayerNum; iLayer++) {
    if (pParam->sSpatialLayers[iLayer].uiLevelIdc < iMinLevelIdc)
      iMinLevelIdc = pParam->sSpatialLayers[iLayer].uiLevelIdc;
  }

  iMinMv = (g_ksLevelLimits[iMinLevelIdc - 1].iMinVmv) >> 2;
  iMaxMv = (g_ksLevelLimits[iMinLevelIdc - 1].iMaxVmv) >> 2;

  iMvRange = WELS_MIN (WELS_ABS (iMinMv), iMaxMv);

  iMvRange = WELS_MIN (iMvRange, iFixMvRange);

  iMvdRange = (iMvRange + 1) << 1;

  iMvdRange = WELS_MIN (iMvdRange, iFixMvdRange);
}
int32_t RequestMemorySvc (sWelsEncCtx** ppCtx) {
  SWelsSvcCodingParam* pParam	= (*ppCtx)->pSvcParam;
  CMemoryAlign* pMa				= (*ppCtx)->pMemAlign;
  SSpatialLayerConfig* pFinalSpatial	= NULL;
  int32_t iCountBsLen			= 0;
  int32_t iCountNals				= 0;
  int32_t iMaxPicWidth			= 0;
  int32_t iMaxPicHeight			= 0;
  int32_t iCountMaxMbNum		= 0;
  int32_t iIndex					= 0;
  int32_t iCountLayers			= 0;
  int32_t iResult					= 0;
  float	fCompressRatioThr		= .5f;
  const int32_t kiNumDependencyLayers	= pParam->iSpatialLayerNum;
  int32_t iMvdRange = 0;
  GetMvMvdRange (pParam, (*ppCtx)->iMvRange, iMvdRange);
  const uint32_t kuiMvdInterTableSize	= (iMvdRange << 2); //intepel*4=qpel
  const uint32_t kuiMvdInterTableStride	=  1 + (kuiMvdInterTableSize << 1);//qpel_mv_range*2=(+/-);
  const uint32_t kuiMvdCacheAlignedSize	= kuiMvdInterTableStride * sizeof (uint16_t);
  int32_t iVclLayersBsSizeCount		= 0;
  int32_t iNonVclLayersBsSizeCount	= 0;
  int32_t iTargetSpatialBsSize			= 0;

  if (kiNumDependencyLayers < 1 || kiNumDependencyLayers > MAX_DEPENDENCY_LAYER) {
    WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_WARNING, "RequestMemorySvc() failed due to invalid iNumDependencyLayers(%d)!",
             kiNumDependencyLayers);
    FreeMemorySvc (ppCtx);
    return 1;
  }

  if (pParam->uiGopSize == 0 || (pParam->uiIntraPeriod && ((pParam->uiIntraPeriod % pParam->uiGopSize) != 0))) {
    WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_WARNING,
             "RequestMemorySvc() failed due to invalid uiIntraPeriod(%d) (=multipler of uiGopSize(%d)!",
             pParam->uiIntraPeriod, pParam->uiGopSize);
    FreeMemorySvc (ppCtx);
    return 1;
  }

  pFinalSpatial	= &pParam->sSpatialLayers[kiNumDependencyLayers - 1];
  iMaxPicWidth	= pFinalSpatial->iVideoWidth;
  iMaxPicHeight	= pFinalSpatial->iVideoHeight;
  iCountMaxMbNum = ((15 + iMaxPicWidth) >> 4) * ((15 + iMaxPicHeight) >> 4);

  iResult = AcquireLayersNals (ppCtx, pParam, &iCountLayers, &iCountNals);
  if (iResult) {
    WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_WARNING, "RequestMemorySvc(), AcquireLayersNals failed(%d)!", iResult);
    FreeMemorySvc (ppCtx);
    return 1;
  }

  iNonVclLayersBsSizeCount = SSEI_BUFFER_SIZE + pParam->iSpatialLayerNum * SPS_BUFFER_SIZE +
                             (1 + pParam->iSpatialLayerNum) * PPS_BUFFER_SIZE;

  int32_t iLayerBsSize = 0;
  iIndex = 0;
  while (iIndex < pParam->iSpatialLayerNum) {
    SSpatialLayerConfig* fDlp = &pParam->sSpatialLayers[iIndex];

    fCompressRatioThr	= COMPRESS_RATIO_THR;

    iLayerBsSize = WELS_ROUND (((3 * fDlp->iVideoWidth * fDlp->iVideoHeight) >> 1) * fCompressRatioThr) +
                   MAX_MACROBLOCK_SIZE_IN_BYTE;
    iLayerBsSize	= WELS_ALIGN (iLayerBsSize, 4);			// 4 bytes alinged
    iVclLayersBsSizeCount += iLayerBsSize;
    ++ iIndex;
  }
  iTargetSpatialBsSize = iLayerBsSize;
  iCountBsLen = iNonVclLayersBsSizeCount + iVclLayersBsSizeCount;

  pParam->iNumRefFrame	= WELS_CLIP3 (pParam->iNumRefFrame, MIN_REF_PIC_COUNT, (pParam->iUsageType==CAMERA_VIDEO_REAL_TIME?MAX_REFERENCE_PICTURE_COUNT_NUM_CAMERA:MAX_REFERENCE_PICTURE_COUNT_NUM_SCREEN));

  // Output
  (*ppCtx)->pOut = (SWelsEncoderOutput*)pMa->WelsMalloc (sizeof (SWelsEncoderOutput), "SWelsEncoderOutput");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pOut), FreeMemorySvc (ppCtx))
  (*ppCtx)->pOut->pBsBuffer    = (uint8_t*)pMa->WelsMalloc (iCountBsLen, "pOut->pBsBuffer");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pOut->pBsBuffer), FreeMemorySvc (ppCtx))
  (*ppCtx)->pOut->uiSize      = iCountBsLen;
  (*ppCtx)->pOut->sNalList		= (SWelsNalRaw*)pMa->WelsMalloc (iCountNals * sizeof (SWelsNalRaw), "pOut->sNalList");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pOut->sNalList), FreeMemorySvc (ppCtx))
  (*ppCtx)->pOut->pNalLen     = (int32_t*)pMa->WelsMallocz (iCountNals * sizeof (int32_t), "pOut->pNalLen");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pOut->pNalLen), FreeMemorySvc (ppCtx))
  (*ppCtx)->pOut->iCountNals		= iCountNals;
  (*ppCtx)->pOut->iNalIndex		= 0;

  if (pParam->iMultipleThreadIdc > 1) {
    const int32_t iTotalLength = iCountBsLen + (iTargetSpatialBsSize * ((*ppCtx)->iMaxSliceCount - 1));
    (*ppCtx)->pFrameBs			= (uint8_t*)pMa->WelsMalloc (iTotalLength, "pFrameBs");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pFrameBs), FreeMemorySvc (ppCtx))
    (*ppCtx)->iFrameBsSize = iTotalLength;
  } else {
    (*ppCtx)->pFrameBs			= (uint8_t*)pMa->WelsMalloc (iCountBsLen, "pFrameBs");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pFrameBs), FreeMemorySvc (ppCtx))
    (*ppCtx)->iFrameBsSize		= iCountBsLen;
  }
  (*ppCtx)->iPosBsBuffer		= 0;

  // for pSlice bs buffers
  if (pParam->iMultipleThreadIdc > 1 && RequestMtResource (ppCtx, pParam, iCountBsLen, iTargetSpatialBsSize)) {
    WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_WARNING, "RequestMemorySvc(), RequestMtResource failed!");
    FreeMemorySvc (ppCtx);
    return 1;
  }

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
    WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_WARNING, "RequestMemorySvc(), AllocStrideTables failed!");
    FreeMemorySvc (ppCtx);
    return 1;
  }

  //Rate control module memory allocation
  // only malloc once for RC pData, 12/14/2009
  (*ppCtx)->pWelsSvcRc = (SWelsSvcRc*)pMa->WelsMallocz (kiNumDependencyLayers * sizeof (SWelsSvcRc), "pWelsSvcRc");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pWelsSvcRc), FreeMemorySvc (ppCtx))
  //End of Rate control module memory allocation

  //pVaa memory allocation
  if (pParam->iUsageType == SCREEN_CONTENT_REAL_TIME) {
    (*ppCtx)->pVaa	= (SVAAFrameInfoExt*)pMa->WelsMallocz (sizeof (SVAAFrameInfoExt), "pVaa");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa), FreeMemorySvc (ppCtx))
    if (RequestMemoryVaaScreen ((*ppCtx)->pVaa, pMa, (*ppCtx)->pSvcParam->iMaxNumRefFrame, iCountMaxMbNum << 2)) {
      WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_WARNING, "RequestMemorySvc(), RequestMemoryVaaScreen failed!");
      FreeMemorySvc (ppCtx);
      return 1;
    }
  } else {
    (*ppCtx)->pVaa	= (SVAAFrameInfo*)pMa->WelsMallocz (sizeof (SVAAFrameInfo), "pVaa");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa), FreeMemorySvc (ppCtx))
  }

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
                                         "pVaa->pVaaBackgroundMbFlag");
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
        (pMa->WelsMallocz (iCountMaxMbNum * 4 * sizeof (int32_t), "pVaa->sVaaCalcInfo.pSumOfDiff8x8"));
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->sVaaCalcInfo.pSumOfDiff8x8), FreeMemorySvc (ppCtx))
    (*ppCtx)->pVaa->sVaaCalcInfo.pMad8x8 = static_cast<uint8_t (*)[4]>
                                           (pMa->WelsMallocz (iCountMaxMbNum * 4 * sizeof (uint8_t), "pVaa->sVaaCalcInfo.pMad8x8"));
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pVaa->sVaaCalcInfo.pMad8x8), FreeMemorySvc (ppCtx))
  }

  //End of pVaa memory allocation

  iResult = InitDqLayers (ppCtx);
  if (iResult) {
    WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_WARNING, "RequestMemorySvc(), InitDqLayers failed(%d)!", iResult);
    FreeMemorySvc (ppCtx);
    return iResult;
  }

  if (InitMbListD (ppCtx)) {
    WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_WARNING, "RequestMemorySvc(), InitMbListD failed!");
    FreeMemorySvc (ppCtx);
    return 1;
  }

  (*ppCtx)->iMvdCostTableSize = kuiMvdInterTableSize;
  (*ppCtx)->iMvdCostTableStride = kuiMvdInterTableStride;
  (*ppCtx)->pMvdCostTable = (uint16_t*)pMa->WelsMallocz (52 * kuiMvdCacheAlignedSize, "pMvdCostTable");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == (*ppCtx)->pMvdCostTable), FreeMemorySvc (ppCtx))
  MvdCostInit ((*ppCtx)->pMvdCostTable, kuiMvdInterTableStride);  //should put to a better place?

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
      // NALs len
      if (NULL != pCtx->pOut->pNalLen) {
        pMa->WelsFree (pCtx->pOut->pNalLen, "pOut->pNalLen");
        pCtx->pOut->pNalLen = NULL;
      }
      pMa->WelsFree (pCtx->pOut, "SWelsEncoderOutput");
      pCtx->pOut = NULL;
    }

    if (pParam != NULL && pParam->iMultipleThreadIdc > 1)
      ReleaseMtResource (ppCtx);

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
      while (ilayer < pParam->iSpatialLayerNum) {
        SDqLayer* pDq	= pCtx->ppDqLayerList[ilayer];
        SSpatialLayerConfig* pDlp = &pCtx->pSvcParam->sSpatialLayers[ilayer];

        const bool kbIsDynamicSlicing = (SM_DYN_SLICE == pDlp->sSliceCfg.uiSliceMode);

        // pDq layers
        if (NULL != pDq) {
          if (NULL != pDq->sLayerInfo.pSliceInLayer) {
            int32_t iSliceIdx = 0;
            int32_t iSliceNum = GetInitialSliceNum (pDq->iMbWidth, pDq->iMbHeight, &pDlp->sSliceCfg);
            if (pDlp->sSliceCfg.uiSliceMode == SM_DYN_SLICE && pCtx->iActiveThreadsNum == 1) {
              if (iSliceNum < pDq->pSliceEncCtx->iMaxSliceNumConstraint) {
                iSliceNum = pDq->pSliceEncCtx->iMaxSliceNumConstraint;
              }
            }
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

          if (pDq->pFeatureSearchPreparation) {
            ReleaseFeatureSearchPreparation (pMa, pDq->pFeatureSearchPreparation->pFeatureOfBlock);
            pMa->WelsFree (pDq->pFeatureSearchPreparation, "pFeatureSearchPreparation");
            pDq->pFeatureSearchPreparation = NULL;
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
    // reference picture list extension
    if (NULL != pCtx->ppRefPicListExt && pParam != NULL) {
      ilayer = 0;
      while (ilayer < pParam->iSpatialLayerNum) {
        SRefList* pRefList		= pCtx->ppRefPicListExt[ilayer];
        if (NULL != pRefList) {
          int32_t iRef = 0;
          do {
            if (pRefList->pRef[iRef] != NULL) {
              FreePicture (pMa, &pRefList->pRef[iRef]);
            }
            ++ iRef;
          } while (iRef < 1 + pParam->iMaxNumRefFrame);

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
      while (ilayer < pParam->iSpatialLayerNum) {
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
      if (pCtx->pSvcParam->iUsageType == SCREEN_CONTENT_REAL_TIME)
        ReleaseMemoryVaaScreen (pCtx->pVaa, pMa, pCtx->pSvcParam->iMaxNumRefFrame);
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
    if (NULL != pCtx->pMvdCostTable) {
      pMa->WelsFree (pCtx->pMvdCostTable, "pMvdCostTable");
      pCtx->pMvdCostTable = NULL;
    }

    FreeCodingParam (&pCtx->pSvcParam, pMa);
    if (NULL != pCtx->pFuncList) {
      pMa->WelsFree (pCtx->pFuncList, "SWelsFuncPtrList");
      pCtx->pFuncList = NULL;
    }

#if defined(MEMORY_MONITOR)
    assert (pMa->WelsGetMemoryUsage() == 0);	// ensure all memory free well
#endif//MEMORY_MONITOR

    if ((*ppCtx)->pMemAlign != NULL) {
      WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_INFO, "FreeMemorySvc(), verify memory usage (%d bytes) after free..",
               (*ppCtx)->pMemAlign->WelsGetMemoryUsage());
      delete (*ppCtx)->pMemAlign;
      (*ppCtx)->pMemAlign = NULL;
    }

    free (*ppCtx);
    *ppCtx = NULL;
  }
}

int32_t InitSliceSettings (SLogContext* pLogCtx, SWelsSvcCodingParam* pCodingParam, const int32_t kiCpuCores,
                           int16_t* pMaxSliceCount) {
  int32_t iSpatialIdx = 0, iSpatialNum = pCodingParam->iSpatialLayerNum;
  uint16_t iMaxSliceCount = 0;

  do {
    SSpatialLayerConfig* pDlp				= &pCodingParam->sSpatialLayers[iSpatialIdx];
    SSliceConfig* pMso			= &pDlp->sSliceCfg;
    SSliceArgument* pSlcArg			= &pMso->sSliceArgument;
    const int32_t kiMbWidth			= (pDlp->iVideoWidth + 15) >> 4;
    const int32_t kiMbHeight			= (pDlp->iVideoHeight + 15) >> 4;
    const int32_t kiMbNumInFrame	= kiMbWidth * kiMbHeight;
    int32_t iSliceNum				= (SM_AUTO_SLICE == pMso->uiSliceMode) ? kiCpuCores : pSlcArg->uiSliceNum;
    // NOTE: Per design, in case MT/DYNAMIC_SLICE_ASSIGN enabled, for SM_FIXEDSLCNUM_SLICE mode,
    // uiSliceNum of current spatial layer settings equals to uiCpuCores number; SM_DYN_SLICE mode,
    // uiSliceNum intials as uiCpuCores also, stay tuned dynamically slicing in future
    pSlcArg->uiSliceNum	= iSliceNum;	// used fixed one

    switch (pMso->uiSliceMode) {
    case SM_DYN_SLICE:
      iMaxSliceCount	= AVERSLICENUM_CONSTRAINT;
      break;	// go through for SM_DYN_SLICE?
    case SM_FIXEDSLCNUM_SLICE:
      if (iSliceNum > iMaxSliceCount)
        iMaxSliceCount = iSliceNum;
      // need perform check due uiSliceNum might change, although has been initialized somewhere outside
      if (pCodingParam->iRCMode != RC_OFF_MODE) {
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
    case SM_AUTO_SLICE:
      iMaxSliceCount = MAX_SLICES_NUM;
      pDlp->sSliceCfg.sSliceArgument.uiSliceNum = kiCpuCores;
      if (pDlp->sSliceCfg.sSliceArgument.uiSliceNum > iMaxSliceCount) {
        pDlp->sSliceCfg.sSliceArgument.uiSliceNum = iMaxSliceCount;
      }
      if (pDlp->sSliceCfg.sSliceArgument.uiSliceNum == 1) {
        WelsLog (pLogCtx, WELS_LOG_DEBUG,
                 "InitSliceSettings(), uiSliceNum(%d) you set for SM_AUTO_SLICE, now turn to SM_SINGLE_SLICE type!",
                 pDlp->sSliceCfg.sSliceArgument.uiSliceNum);
        pDlp->sSliceCfg.uiSliceMode	= SM_SINGLE_SLICE;
        break;
      }
      if (pCodingParam->iRCMode != RC_OFF_MODE) {	// multiple slices verify with gom
        //check uiSliceNum
        GomValidCheckSliceNum (kiMbWidth, kiMbHeight, &pDlp->sSliceCfg.sSliceArgument.uiSliceNum);
        assert (pDlp->sSliceCfg.sSliceArgument.uiSliceNum > 1);
        //set uiSliceMbNum with current uiSliceNum
        GomValidCheckSliceMbNum (kiMbWidth, kiMbHeight, &pDlp->sSliceCfg.sSliceArgument);
      } else if (!CheckFixedSliceNumMultiSliceSetting (kiMbNumInFrame,
                 &pDlp->sSliceCfg.sSliceArgument)) {	// verify interleave mode settings
        //check uiSliceMbNum with current uiSliceNum
        WelsLog (pLogCtx, WELS_LOG_ERROR,
                 "InitSliceSettings(), invalid uiSliceMbNum (%d) settings!,now turn to SM_SINGLE_SLICE type",
                 pDlp->sSliceCfg.sSliceArgument.uiSliceMbNum[0]);
        pDlp->sSliceCfg.uiSliceMode	= SM_SINGLE_SLICE;
        pDlp->sSliceCfg.sSliceArgument.uiSliceNum	= 1;
      }
      // considering the coding efficient and performance, iCountMbNum constraint by MIN_NUM_MB_PER_SLICE condition of multi-pSlice mode settting
      if (kiMbNumInFrame <= MIN_NUM_MB_PER_SLICE) {
        pDlp->sSliceCfg.uiSliceMode	= SM_SINGLE_SLICE;
        pDlp->sSliceCfg.sSliceArgument.uiSliceNum	= 1;
        break;
      }
      break;
    default:
      break;
    }

    ++ iSpatialIdx;
  } while (iSpatialIdx < iSpatialNum);

  pCodingParam->iCountThreadsNum				= WELS_MIN (kiCpuCores, iMaxSliceCount);
  pCodingParam->iMultipleThreadIdc	= pCodingParam->iCountThreadsNum;
  if (pCodingParam->iLoopFilterDisableIdc == 0
      && pCodingParam->iMultipleThreadIdc != 1) // Loop filter requested to be enabled, with threading enabled
    pCodingParam->iLoopFilterDisableIdc =
      2; // Disable loop filter on slice boundaries since that's not allowed with multithreading
  *pMaxSliceCount					= iMaxSliceCount;

  return 0;
}

/*!
 * \brief	log output for cpu features/capabilities
 */
void OutputCpuFeaturesLog (SLogContext* pLogCtx, uint32_t uiCpuFeatureFlags, uint32_t uiCpuCores,
                           int32_t iCacheLineSize) {
  // welstracer output
  WelsLog (pLogCtx, WELS_LOG_INFO, "WELS CPU features/capacities (0x%x) detected: \t"	\
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
           "CPU CACHE LINE SIZE (BYTES):        %d",
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
}

int32_t GetMultipleThreadIdc (SLogContext* pLogCtx, SWelsSvcCodingParam* pCodingParam, int16_t& iSliceNum,
                              int32_t& iCacheLineSize, uint32_t& uiCpuFeatureFlags) {
  // for cpu features detection, Only detect once??
  int32_t uiCpuCores				=
    0;	// number of logic processors on physical processor package, zero logic processors means HTT not supported
  uiCpuFeatureFlags	= WelsCPUFeatureDetect (&uiCpuCores);	// detect cpu capacity features

#ifdef X86_ASM
  if (uiCpuFeatureFlags & WELS_CPU_CACHELINE_128)
    iCacheLineSize = 128;
  else if (uiCpuFeatureFlags & WELS_CPU_CACHELINE_64)
    iCacheLineSize = 64;
  else if (uiCpuFeatureFlags & WELS_CPU_CACHELINE_32)
    iCacheLineSize	= 32;
  else if (uiCpuFeatureFlags & WELS_CPU_CACHELINE_16)
    iCacheLineSize	= 16;
  OutputCpuFeaturesLog (pLogCtx, uiCpuFeatureFlags, uiCpuCores, iCacheLineSize);
#else
  iCacheLineSize	= 16;	// 16 bytes aligned in default
#endif//X86_ASM

#if defined(DYNAMIC_DETECT_CPU_CORES)
  if (pCodingParam->iMultipleThreadIdc > 0)
    uiCpuCores = pCodingParam->iMultipleThreadIdc;
  else {
    if (uiCpuCores ==
        0)	// cpuid not supported or doesn't expose the number of cores, use high level system API as followed to detect number of pysical/logic processor
      uiCpuCores = DynamicDetectCpuCores();
    // So far so many cpu cores up to MAX_THREADS_NUM mean for server platforms,
    // for client application here it is constrained by maximal to MAX_THREADS_NUM
    if (uiCpuCores > MAX_THREADS_NUM)	// MAX_THREADS_NUM
      uiCpuCores	= MAX_THREADS_NUM;	// MAX_THREADS_NUM
    else if (uiCpuCores < 1)	// just for safe
      uiCpuCores	= 1;
  }
#endif//DYNAMIC_DETECT_CPU_CORES

  uiCpuCores	= WELS_CLIP3 (uiCpuCores, 1, MAX_THREADS_NUM);

  if (InitSliceSettings (pLogCtx, pCodingParam, uiCpuCores, &iSliceNum)) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "GetMultipleThreadIdc(), InitSliceSettings failed.");
    return 1;
  }
  return 0;
}

/*!
 * \brief	initialize Wels avc encoder core library
 * \pParam	ppCtx		sWelsEncCtx**
 * \pParam	pParam		SWelsSvcCodingParam*
 * \return	successful - 0; otherwise none 0 for failed
 */
int32_t WelsInitEncoderExt (sWelsEncCtx** ppCtx, SWelsSvcCodingParam* pCodingParam, SLogContext* pLogCtx) {
  sWelsEncCtx* pCtx		= NULL;
  int32_t	iRet					= 0;
  int16_t iSliceNum				= 1;	// number of slices used
  int32_t iCacheLineSize			= 16;	// on chip cache line size in byte
  uint32_t uiCpuFeatureFlags = 0;
  if (NULL == ppCtx || NULL == pCodingParam) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "WelsInitEncoderExt(), NULL == ppCtx(0x%p) or NULL == pCodingParam(0x%p).",
             (void*)ppCtx, (void*)pCodingParam);
    return 1;
  }

  iRet	=	ParamValidationExt (pLogCtx, pCodingParam);
  if (iRet != 0) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "WelsInitEncoderExt(), ParamValidationExt failed return %d.", iRet);
    return iRet;
  }
  iRet	=	pCodingParam->DetermineTemporalSettings();
  if (iRet != ENC_RETURN_SUCCESS) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "WelsInitEncoderExt(), DetermineTemporalSettings failed return %d (check in/out frame rate and temporal layer setting!)", iRet);
    return iRet;
  }
  iRet = GetMultipleThreadIdc (pLogCtx, pCodingParam, iSliceNum, iCacheLineSize, uiCpuFeatureFlags);
  if (iRet != 0) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "WelsInitEncoderExt(), GetMultipleThreadIdc failed return %d.", iRet);
    return iRet;
  }


  *ppCtx	= NULL;

  pCtx	= static_cast<sWelsEncCtx*> (malloc (sizeof (sWelsEncCtx)));

  WELS_VERIFY_RETURN_IF (1, (NULL == pCtx))
  memset (pCtx, 0, sizeof (sWelsEncCtx));

  pCtx->sLogCtx = *pLogCtx;

  pCtx->pMemAlign = new CMemoryAlign (iCacheLineSize);
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pCtx->pMemAlign), FreeMemorySvc (&pCtx))

  iRet = AllocCodingParam (&pCtx->pSvcParam, pCtx->pMemAlign);
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
    WelsLog (pLogCtx, WELS_LOG_ERROR, "WelsInitEncoderExt(), RequestMemorySvc failed return %d.", iRet);
    FreeMemorySvc (&pCtx);
    return iRet;
  }

  if (pCodingParam->iMultipleThreadIdc > 1)
    iRet = CreateSliceThreads (pCtx);

  WelsRcInitModule (pCtx,  pCtx->pSvcParam->iRCMode);

  pCtx->pVpp = new CWelsPreProcess (pCtx);
  if (pCtx->pVpp == NULL) {
    iRet = 1;
    WelsLog (pLogCtx, WELS_LOG_ERROR, "WelsInitEncoderExt(), pOut of memory in case new CWelsPreProcess().");
    FreeMemorySvc (&pCtx);
    return iRet;
  }
  if ((iRet = pCtx->pVpp->AllocSpatialPictures (pCtx, pCtx->pSvcParam)) != 0) {
    WelsLog (pLogCtx, WELS_LOG_ERROR, "WelsInitEncoderExt(), pVPP alloc spatial pictures failed");
    FreeMemorySvc (&pCtx);
    return iRet;
  }

#if defined(MEMORY_MONITOR)
  WelsLog (pLogCtx, WELS_LOG_INFO, "WelsInitEncoderExt() exit, overall memory usage: %llu bytes",
           static_cast<unsigned long long> (sizeof (sWelsEncCtx) /* requested size from malloc() or new operator */
               + pCtx->pMemAlign->WelsGetMemoryUsage())  /* requested size from CMemoryAlign::WelsMalloc() */
          );
#endif//MEMORY_MONITOR

  *ppCtx	= pCtx;

  WelsLog (pLogCtx, WELS_LOG_DEBUG, "WelsInitEncoderExt(), pCtx= 0x%p.", (void*)pCtx);

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
  for (i = 0; i < pCtx->pSvcParam->iSpatialLayerNum; i++) {
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

  WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_INFO,
           "WelsUninitEncoderExt(), pCtx= %p, iThreadCount= %d, iMultipleThreadIdc= %d.",
           (void*) (*ppCtx), (*ppCtx)->pSvcParam->iCountThreadsNum, (*ppCtx)->pSvcParam->iMultipleThreadIdc);

#if defined(STAT_OUTPUT)
  StatOverallEncodingExt (*ppCtx);
#endif

  if ((*ppCtx)->pSvcParam->iMultipleThreadIdc > 1 && (*ppCtx)->pSliceThreading != NULL) {
    const int32_t iThreadCount = (*ppCtx)->pSvcParam->iCountThreadsNum;
    int32_t iThreadIdx = 0;

    if ((*ppCtx)->pSliceThreading->pExitEncodeEvent != NULL) {
      while (iThreadIdx < iThreadCount) {
        int res = 0;
        if ((*ppCtx)->pSliceThreading->pThreadHandles[iThreadIdx]) {
          WelsEventSignal (& (*ppCtx)->pSliceThreading->pExitEncodeEvent[iThreadIdx]);
          WelsEventSignal (& (*ppCtx)->pSliceThreading->pThreadMasterEvent[iThreadIdx]);
          res = WelsThreadJoin ((*ppCtx)->pSliceThreading->pThreadHandles[iThreadIdx]);	// waiting thread exit
          WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_INFO, "WelsUninitEncoderExt(), pthread_join(pThreadHandles%d) return %d..",
                   iThreadIdx,
                   res);
          (*ppCtx)->pSliceThreading->pThreadHandles[iThreadIdx] = 0;
        }
        ++ iThreadIdx;
      }
    }
  }

  if ((*ppCtx)->pVpp) {
    (*ppCtx)->pVpp->FreeSpatialPictures (*ppCtx);
    delete (*ppCtx)->pVpp;
    (*ppCtx)->pVpp = NULL;
  }
  FreeMemorySvc (ppCtx);
  *ppCtx = NULL;
}

/*!
 * \brief	get temporal level due to configuration and coding context
 */
int32_t GetTemporalLevel (SSpatialLayerInternal* fDlp, const int32_t kiFrameNum, const int32_t kiGopSize) {
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
    bool     bLeft;
    bool     bTop;
    bool     bLeftTop;
    bool     bRightTop;
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
  if (pCtx->pSvcParam->iMultipleThreadIdc > 1) {
    iPartitionNum	= pCtx->pSvcParam->iCountThreadsNum;
  }
  return iPartitionNum;
}

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

    if (pCtx->pSvcParam->iRCMode != RC_OFF_MODE) {
      //RC case
      uiFrmByte = (
                    ((uint32_t) (pCtx->pSvcParam->sSpatialLayers[iCurDid].iSpatialBitrate)
                     / (uint32_t) (pCtx->pSvcParam->sDependencyLayers[iCurDid].fInputFrameRate)) >> 3);
    } else {
      //fixed QP case
      const int32_t iTtlMbNumInFrame = pSliceCtx->iMbNumInFrame;
      int32_t iQDeltaTo26 = (26 - pCtx->pSvcParam->sSpatialLayers[iCurDid].iDLayerQp);

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

      WelsLog (& (pCtx->sLogCtx),
               WELS_LOG_WARNING,
               "Set-SliceConstraint(%d) too small for current resolution (MB# %d) under QP/BR!",
               pSliceCtx->uiSliceSizeConstraint,
               pSliceCtx->iMbNumInFrame
              );
    }
  }

  WelsInitCurrentQBLayerMltslc (pCtx);
}

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
  const bool kbUseSubsetSpsFlag = (kiCurDid > BASE_DEPENDENCY_ID);
  SSpatialLayerConfig* fDlp				= &pParam->sSpatialLayers[kiCurDid];
  SNalUnitHeaderExt* pNalHdExt	= &pCurDq->sLayerInfo.sNalHeaderExt;
  SNalUnitHeader* pNalHd			= &pNalHdExt->sNalHeader;
  SDqIdc* pDqIdc						= &pCtx->pDqIdcMap[kiCurDid];
  int32_t iIdx						= 0;
  int32_t iSliceCount				= 0;

  if (NULL == pCurDq)
    return;

  pCurDq->pDecPic	= pDecPic;

  if (fDlp->sSliceCfg.uiSliceMode == SM_DYN_SLICE)	// need get extra slices for update
    iSliceCount = GetInitialSliceNum (pCurDq->iMbWidth, pCurDq->iMbHeight, &fDlp->sSliceCfg);
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

static inline void SetFastCodingFunc (SWelsFuncPtrList* pFuncList) {
  pFuncList->pfIntraFineMd = WelsMdIntraFinePartitionVaa;
  pFuncList->sSampleDealingFuncs.pfMdCost = pFuncList->sSampleDealingFuncs.pfSampleSad;
  pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3 = pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Sad;
  pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3 = pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Sad;
}
static inline void SetNormalCodingFunc (SWelsFuncPtrList* pFuncList) {
  pFuncList->pfIntraFineMd = WelsMdIntraFinePartition;
  pFuncList->sSampleDealingFuncs.pfMdCost = pFuncList->sSampleDealingFuncs.pfSampleSatd;
  pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3 =
    pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Satd;
  pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3 =
    pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Satd;
  pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3 =
    pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3Satd;
}
bool SetMeMethod (const uint8_t uiMethod, PSearchMethodFunc& pSearchMethodFunc) {
  switch (uiMethod) {
  case  ME_DIA:
    pSearchMethodFunc  = WelsDiamondSearch;
    break;
  case  ME_CROSS:
    pSearchMethodFunc = WelsMotionCrossSearch;
    break;
  case  ME_DIA_CROSS:
    pSearchMethodFunc = WelsDiamondCrossSearch;
    break;
  case  ME_DIA_CROSS_FME:
    pSearchMethodFunc = WelsDiamondCrossFeatureSearch;
    break;
  case ME_FULL:
    pSearchMethodFunc = WelsDiamondSearch;
    return false;
  default:
    pSearchMethodFunc = WelsDiamondSearch;
    return false;
  }
  return true;
}



void PreprocessSliceCoding (sWelsEncCtx* pCtx) {
  SDqLayer* pCurLayer		= pCtx->pCurDqLayer;
  //const bool kbBaseAvail	= pCurLayer->bBaseLayerAvailableFlag;
  const bool kbHighestSpatialLayer	=
    (pCtx->pSvcParam->iSpatialLayerNum == (pCurLayer->sLayerInfo.sNalHeaderExt.uiDependencyId + 1));
  SWelsFuncPtrList* pFuncList = pCtx->pFuncList;
  SLogContext* pLogCtx = & (pCtx->sLogCtx);
  /* function pointers conditional assignment under sWelsEncCtx, layer_mb_enc_rec (in stack) is exclusive */
  if ((pCtx->pSvcParam->iUsageType == CAMERA_VIDEO_REAL_TIME && kbHighestSpatialLayer) ||
      (pCtx->pSvcParam->iUsageType == SCREEN_CONTENT_REAL_TIME && P_SLICE == pCtx->eSliceType
       && kbHighestSpatialLayer) //TODO: here is for sync with the origin code, consider the design again with more tests
     ) {
    SetFastCodingFunc (pFuncList);
  } else {
    SetNormalCodingFunc (pFuncList);
  }

  if (P_SLICE == pCtx->eSliceType) {
    for (int i = 0; i < BLOCK_STATIC_IDC_ALL; i++) {
      pFuncList->pfMotionSearch[i] = WelsMotionEstimateSearch;
    }
    pFuncList->pfSearchMethod[BLOCK_16x16]  =
      pFuncList->pfSearchMethod[BLOCK_16x8] =
        pFuncList->pfSearchMethod[BLOCK_8x16] =
          pFuncList->pfSearchMethod[BLOCK_8x8] =
            pFuncList->pfSearchMethod[BLOCK_4x4] = WelsDiamondSearch;
    pFuncList->pfFirstIntraMode = WelsMdFirstIntraMode;
    pFuncList->sSampleDealingFuncs.pfMeCost = pCtx->pFuncList->sSampleDealingFuncs.pfSampleSatd;
    pFuncList->pfSetScrollingMv = SetScrollingMvToMdNull;

    if (kbHighestSpatialLayer) {
      pFuncList->pfCalculateSatd = NotCalculateSatdCost;
      pFuncList->pfInterFineMd = WelsMdInterFinePartitionVaa;
    } else {
      pFuncList->pfCalculateSatd = CalculateSatdCost;
      pFuncList->pfInterFineMd = WelsMdInterFinePartition;
    }
  }

  //to init at each frame will be needed when dealing with hybrid content (camera+screen)
  if (pCtx->pSvcParam->iUsageType == SCREEN_CONTENT_REAL_TIME) {
    if (P_SLICE == pCtx->eSliceType) {
      //MD related func pointers
      pFuncList->pfInterFineMd = WelsMdInterFinePartitionVaaOnScreen;

      //ME related func pointers
      SVAAFrameInfoExt* pVaaExt		= static_cast<SVAAFrameInfoExt*> (pCtx->pVaa);
      if (pVaaExt->sScrollDetectInfo.bScrollDetectFlag
          && (pVaaExt->sScrollDetectInfo.iScrollMvX | pVaaExt->sScrollDetectInfo.iScrollMvY)) {
        pFuncList->pfSetScrollingMv = SetScrollingMvToMd;
      } else {
        pFuncList->pfSetScrollingMv = SetScrollingMvToMdNull;
      }

      pFuncList->pfMotionSearch[NO_STATIC] = WelsMotionEstimateSearch;
      pFuncList->pfMotionSearch[COLLOCATED_STATIC] = WelsMotionEstimateSearchStatic;
      pFuncList->pfMotionSearch[SCROLLED_STATIC] = WelsMotionEstimateSearchScrolled;
      //ME16x16
      if (!SetMeMethod (ME_DIA_CROSS, pFuncList->pfSearchMethod[BLOCK_16x16])) {
        WelsLog (pLogCtx, WELS_LOG_WARNING, "SetMeMethod(BLOCK_16x16) ME_DIA_CROSS unsuccessful, switched to default search");
      }
      //ME8x8
      SFeatureSearchPreparation* pFeatureSearchPreparation = pCurLayer->pFeatureSearchPreparation;
      if (pFeatureSearchPreparation) {
        pFeatureSearchPreparation->iHighFreMbCount = 0;

        //calculate bFMESwitchFlag
        SVAAFrameInfoExt* pVaaExt		= static_cast<SVAAFrameInfoExt*> (pCtx->pVaa);
        const int32_t kiMbSize = pCurLayer->iMbHeight * pCurLayer->iMbWidth;
        pFeatureSearchPreparation->bFMESwitchFlag = CalcFMESwitchFlag (pFeatureSearchPreparation->uiFMEGoodFrameCount,
            pFeatureSearchPreparation->iHighFreMbCount * 100 / kiMbSize, pCtx->pVaa->sVaaCalcInfo.iFrameSad / kiMbSize,
            pVaaExt->sScrollDetectInfo.bScrollDetectFlag);

        //PerformFMEPreprocess
        SScreenBlockFeatureStorage* pScreenBlockFeatureStorage = pCurLayer->pRefPic->pScreenBlockFeatureStorage;
        pFeatureSearchPreparation->pRefBlockFeature = pScreenBlockFeatureStorage;
        if (pFeatureSearchPreparation->bFMESwitchFlag
            && !pScreenBlockFeatureStorage->bRefBlockFeatureCalculated) {
          SPicture* pRef = (pCtx->pSvcParam->bEnableLongTermReference ? pCurLayer->pRefOri[0] : pCurLayer->pRefPic);
          PerformFMEPreprocess (pFuncList, pRef, pFeatureSearchPreparation->pFeatureOfBlock,
                                pScreenBlockFeatureStorage);
        }

        //assign ME pointer
        if (pFeatureSearchPreparation->bFMESwitchFlag && pScreenBlockFeatureStorage->bRefBlockFeatureCalculated
            && (!pScreenBlockFeatureStorage->iIs16x16)) {
          if (!SetMeMethod (ME_DIA_CROSS_FME, pFuncList->pfSearchMethod[BLOCK_8x8])) {
            WelsLog (pLogCtx, WELS_LOG_WARNING,
                     "SetMeMethod(BLOCK_8x8) ME_DIA_CROSS_FME unsuccessful, switched to default search");
          }
        }

        //assign UpdateFMESwitch pointer
        if (pFeatureSearchPreparation->bFMESwitchFlag) {
          pFuncList->pfUpdateFMESwitch = UpdateFMESwitch;
        } else {
          pFuncList->pfUpdateFMESwitch = UpdateFMESwitchNull;
        }
      }//if (pFeatureSearchPreparation)
    } else {
      //reset some status when at I_SLICE
      pCurLayer->pFeatureSearchPreparation->bFMESwitchFlag = true;
      pCurLayer->pFeatureSearchPreparation->uiFMEGoodFrameCount = FMESWITCH_DEFAULT_GOODFRAME_NUM;
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
static inline void PrefetchReferencePicture (sWelsEncCtx* pCtx, const EVideoFrameType keFrameType) {
  SSlice* pSliceBase = &pCtx->pCurDqLayer->sLayerInfo.pSliceInLayer[0];
  const int32_t kiSliceCount = GetCurrentSliceNum (pCtx->pCurDqLayer->pSliceEncCtx);
  int32_t iIdx = 0;
  uint8_t uiRefIdx = -1;

  assert (kiSliceCount > 0);
  if (keFrameType != videoFrameTypeIDR) {
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
  uint32_t uiNextIdInBs			= sParaSetOffsetVariable->uiNextParaSetIdToUseInBs;

  //update current layer's pCodingParam
  sParaSetOffsetVariable->iParaSetIdDelta[kiEncId]	= uiNextIdInBs -
      kiEncId;  //for current parameter set, change its id_delta
  //write pso pData for next update:
  sParaSetOffsetVariable->bUsedParaSetIdInBs[uiNextIdInBs] = true;		//   update current used_id

  //prepare for next update:
  //   find the next avaibable iId
  ++uiNextIdInBs;
  if (uiNextIdInBs >= kuiMaxIdInBs) {
    uiNextIdInBs = 0;//ensure the SPS_ID wound not exceed MAX_SPS_COUNT
  }
  //   update next_id
  sParaSetOffsetVariable->uiNextParaSetIdToUseInBs = uiNextIdInBs;
}

/*!
 * \brief	write all parameter sets introduced in SVC extension
 * \return	writing results, success or error
 */
int32_t WelsWriteParameterSets (sWelsEncCtx* pCtx, int32_t* pNalLen, int32_t* pNumNal) {
  int32_t iSize	= 0;
  int32_t iNal	= 0;
  int32_t	iIdx	= 0;
  int32_t iId	= 0;
  int32_t iCountNal	= 0;
  int32_t iNalLength	= 0;
  int32_t iReturn = ENC_RETURN_SUCCESS;

  if (NULL == pCtx || NULL == pNalLen || NULL == pNumNal)
    return ENC_RETURN_UNEXPECTED;

  /* write all SPS */
  iIdx = 0;
  while (iIdx < pCtx->iSpsNum) {
    SDqIdc* pDqIdc		= &pCtx->pDqIdcMap[iIdx];
    const int32_t kiDid	= pDqIdc->uiSpatialId;
    const bool kbUsingSubsetSps = (kiDid > BASE_DEPENDENCY_ID);

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

    iReturn = WelsEncodeNal (&pCtx->pOut->sNalList[iNal], NULL,
                             pCtx->iFrameBsSize - pCtx->iPosBsBuffer,//available buffer to be written, so need to substract the used length
                             pCtx->pFrameBs + pCtx->iPosBsBuffer,
                             &iNalLength);
    WELS_VERIFY_RETURN_IFNEQ (iReturn, ENC_RETURN_SUCCESS)
    pNalLen[iCountNal] = iNalLength;

    pCtx->iPosBsBuffer	+= iNalLength;
    iSize				+= iNalLength;

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

    iReturn = WelsEncodeNal (&pCtx->pOut->sNalList[iNal], NULL,
                             pCtx->iFrameBsSize - pCtx->iPosBsBuffer,
                             pCtx->pFrameBs + pCtx->iPosBsBuffer,
                             &iNalLength);
    WELS_VERIFY_RETURN_IFNEQ (iReturn, ENC_RETURN_SUCCESS)
    pNalLen[iCountNal] = iNalLength;
    pCtx->iPosBsBuffer	+= iNalLength;
    iSize				+= iNalLength;

    ++ iIdx;
    ++ iCountNal;
  }

  *pNumNal = iCountNal;

  return ENC_RETURN_SUCCESS;
}

static inline int32_t AddPrefixNal (sWelsEncCtx* pCtx,
                                    SLayerBSInfo* pLayerBsInfo,
                                    int32_t* pNalLen,
                                    int32_t* pNalIdxInLayer,
                                    const EWelsNalUnitType keNalType,
                                    const EWelsNalRefIdc keNalRefIdc,
                                    int32_t& iPayloadSize) {
  int32_t iReturn = ENC_RETURN_SUCCESS;
  iPayloadSize = 0;

  if (keNalRefIdc != NRI_PRI_LOWEST) {
    WelsLoadNal (pCtx->pOut, NAL_UNIT_PREFIX, keNalRefIdc);

    WelsWriteSVCPrefixNal (&pCtx->pOut->sBsWrite, keNalRefIdc, (NAL_UNIT_CODED_SLICE_IDR == keNalType));

    WelsUnloadNal (pCtx->pOut);

    iReturn = WelsEncodeNal (&pCtx->pOut->sNalList[pCtx->pOut->iNalIndex - 1],
                             &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt,
                             pCtx->iFrameBsSize - pCtx->iPosBsBuffer,
                             pCtx->pFrameBs + pCtx->iPosBsBuffer,
                             &pNalLen[*pNalIdxInLayer]);
    WELS_VERIFY_RETURN_IFNEQ (iReturn, ENC_RETURN_SUCCESS)
    iPayloadSize = pNalLen[*pNalIdxInLayer];

    pCtx->iPosBsBuffer							+= iPayloadSize;

    (*pNalIdxInLayer) ++;
  } else { // No Prefix NAL Unit RBSP syntax here, but need add NAL Unit Header extension
    WelsLoadNal (pCtx->pOut, NAL_UNIT_PREFIX, keNalRefIdc);
    // No need write any syntax of prefix NAL Unit RBSP here
    WelsUnloadNal (pCtx->pOut);

    iReturn = WelsEncodeNal (&pCtx->pOut->sNalList[pCtx->pOut->iNalIndex - 1],
                             &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt,
                             pCtx->iFrameBsSize - pCtx->iPosBsBuffer,
                             pCtx->pFrameBs + pCtx->iPosBsBuffer,
                             &pNalLen[*pNalIdxInLayer]);
    WELS_VERIFY_RETURN_IFNEQ (iReturn, ENC_RETURN_SUCCESS)
    iPayloadSize = pNalLen[*pNalIdxInLayer];

    pCtx->iPosBsBuffer							+= iPayloadSize;

    (*pNalIdxInLayer) ++;
  }

  return ENC_RETURN_SUCCESS;
}

int32_t WritePadding (sWelsEncCtx* pCtx, int32_t iLen, int32_t& iSize) {
  int32_t i = 0;
  int32_t iNal	= 0;
  SBitStringAux*	pBs = NULL;
  int32_t iNalLen;

  iSize = 0;
  iNal	= pCtx->pOut->iNalIndex;
  pBs	=	&pCtx->pOut->sBsWrite;	// SBitStringAux instance for non VCL NALs decoding

  if ((pBs->pBufEnd - pBs->pBufPtr) < iLen || iNal >= pCtx->pOut->iCountNals) {
#if GOM_TRACE_FLAG
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_ERROR,
             "[RC] paddingcal pBuffer overflow, bufferlen=%lld, paddinglen=%d, iNalIdx= %d, iCountNals= %d",
             static_cast<long long int> (pBs->pBufEnd - pBs->pBufPtr), iLen, iNal, pCtx->pOut->iCountNals);
#endif
    return ENC_RETURN_MEMOVERFLOWFOUND;
  }

  WelsLoadNal (pCtx->pOut, NAL_UNIT_FILLER_DATA, NRI_PRI_LOWEST);

  for (i = 0; i < iLen; i++) {
    BsWriteBits (pBs, 8, 0xff);
  }

  BsRbspTrailingBits (pBs);

  WelsUnloadNal (pCtx->pOut);
  int32_t iReturn = WelsEncodeNal (&pCtx->pOut->sNalList[iNal], NULL,
                                   pCtx->iFrameBsSize - pCtx->iPosBsBuffer,
                                   pCtx->pFrameBs + pCtx->iPosBsBuffer,
                                   &iNalLen);
  WELS_VERIFY_RETURN_IFNEQ (iReturn, ENC_RETURN_SUCCESS)

  pCtx->iPosBsBuffer	+= iNalLen;
  iSize				+= iNalLen;

  return ENC_RETURN_SUCCESS;
}

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
  int32_t iCountNal           = 0;

  pLayerBsInfo->pBsBuf = pCtx->pFrameBs;
  pLayerBsInfo->pNalLengthInByte = pCtx->pOut->pNalLen;
  InitBits (&pCtx->pOut->sBsWrite, pCtx->pOut->pBsBuffer, pCtx->pOut->uiSize);

  pCtx->iPosBsBuffer = 0;
  int32_t iReturn = WelsWriteParameterSets (pCtx, &pLayerBsInfo->pNalLengthInByte[0], &iCountNal);
  WELS_VERIFY_RETURN_IFNEQ (iReturn, ENC_RETURN_SUCCESS)

  pLayerBsInfo->uiSpatialId   = 0;
  pLayerBsInfo->uiTemporalId  = 0;
  pLayerBsInfo->uiQualityId   = 0;
  pLayerBsInfo->uiLayerType   = NON_VIDEO_CODING_LAYER;
  pLayerBsInfo->iNalCount     = iCountNal;

  pCtx->eLastNalPriority      = NRI_PRI_HIGHEST;
  pFbi->iLayerNum             = 1;
  pFbi->eFrameType            = videoFrameTypeInvalid;

  WelsEmms();

  return ENC_RETURN_SUCCESS;
}

int32_t GetSubSequenceId (sWelsEncCtx* pCtx, EVideoFrameType eFrameType) {
  int32_t iSubSeqId = 0;
  if (eFrameType == videoFrameTypeIDR)
    iSubSeqId = 0;
  else if (eFrameType == videoFrameTypeI)
    iSubSeqId = 1;
  else if (eFrameType == videoFrameTypeP) {
    if (pCtx->bCurFrameMarkedAsSceneLtr)
      iSubSeqId = 2;
    else
      iSubSeqId = 3 + pCtx->uiTemporalId; //T0:3 T1:4 T2:5 T3:6
  } else
    iSubSeqId = 3 + MAX_TEMPORAL_LAYER_NUM;
  return iSubSeqId;
}

//loop each layer to check if have skip frame when RC and frame skip enable (maxbr>0)
bool CheckFrameSkipBasedMaxbr (sWelsEncCtx* pCtx, int32_t iSpatialNum, EVideoFrameType eFrameType,
                               const uint32_t uiTimeStamp) {
  SSpatialPicIndex* pSpatialIndexMap = &pCtx->sSpatialIndexMap[0];
  bool bSkipMustFlag = false;
  if (pCtx->pSvcParam->bEnableFrameSkip) {
    if ((RC_QUALITY_MODE == pCtx->pSvcParam->iRCMode) || (RC_BITRATE_MODE == pCtx->pSvcParam->iRCMode)) {
      for (int32_t i = 0; i < iSpatialNum; i++) {
        if (0 == pCtx->pSvcParam->sSpatialLayers[i].iMaxSpatialBitrate) {
          break;
        }
        pCtx->uiDependencyId = (uint8_t) (pSpatialIndexMap + i)->iDid;
        pCtx->pFuncList->pfRc.pfWelsRcPicDelayJudge (pCtx, eFrameType, uiTimeStamp);
        if (true == pCtx->pWelsSvcRc[pCtx->uiDependencyId].bSkipFlag) {
          bSkipMustFlag = true;
          break;
        }
      }
    }
  }
  return bSkipMustFlag;
}

/*!
 * \brief	core svc encoding process
 *
 * \pParam	pCtx			sWelsEncCtx*, encoder context
 * \pParam	pFbi			FrameBSInfo*
 * \pParam	pSrcPic			Source Picture
 * \return	EFrameType (videoFrameTypeIDR/videoFrameTypeI/videoFrameTypeP)
 */
int32_t WelsEncoderEncodeExt (sWelsEncCtx* pCtx, SFrameBSInfo* pFbi, const SSourcePicture* pSrcPic) {
  if (pCtx == NULL) {
    return ENC_RETURN_MEMALLOCERR;
  }
  SLayerBSInfo* pLayerBsInfo					= &pFbi->sLayerInfo[0];
  SWelsSvcCodingParam* pSvcParam	= pCtx->pSvcParam;
  SSpatialPicIndex* pSpatialIndexMap = &pCtx->sSpatialIndexMap[0];
#if defined(ENABLE_FRAME_DUMP) || defined(ENABLE_PSNR_CALC)
  SPicture* fsnr						= NULL;
#endif//ENABLE_FRAME_DUMP || ENABLE_PSNR_CALC
  SPicture* pEncPic						= NULL;	// to be decided later
  int32_t iDidList[MAX_DEPENDENCY_LAYER]	= {0};
  int32_t iLayerNum					= 0;
  int32_t iLayerSize					= 0;
  int32_t iSpatialNum					= 0; // available count number of spatial layers due to frame size changed in this given frame
  int32_t iSpatialIdx					= 0; // iIndex of spatial layers due to frame size changed in this given frame
  int32_t iFrameSize					= 0;
  int32_t iNalIdxInLayer			= 0;
  int32_t iCountNal					= 0;
  EVideoFrameType eFrameType				= videoFrameTypeInvalid;
  int32_t iCurWidth					= 0;
  int32_t iCurHeight					= 0;
  EWelsNalUnitType eNalType			= NAL_UNIT_UNSPEC_0;
  EWelsNalRefIdc eNalRefIdc			= NRI_PRI_LOWEST;
  int8_t iCurDid						= 0;
  int8_t iCurTid						= 0;
  bool bAvcBased					= false;
  SLogContext* pLogCtx = & (pCtx->sLogCtx);
#if defined(ENABLE_PSNR_CALC)
  float fSnrY = .0f, fSnrU = .0f, fSnrV = .0f;
#endif//ENABLE_PSNR_CALC

#if defined(_DEBUG)
  int32_t i = 0, j = 0, k = 0;
#endif//_DEBUG

  pCtx->iEncoderError						= ENC_RETURN_SUCCESS;
  pCtx->bCurFrameMarkedAsSceneLtr   = false;
  pFbi->iLayerNum	= 0;	// for initialization
  pFbi->uiTimeStamp = pSrcPic->uiTimeStamp;
  // perform csc/denoise/downsample/padding, generate spatial layers
  iSpatialNum = pCtx->pVpp->BuildSpatialPicList (pCtx, pSrcPic);
  if (iSpatialNum < 1) {	// skip due to temporal layer settings (different frame rate)
    ++ pCtx->iCodingIndex;
    pFbi->eFrameType = videoFrameTypeSkip;
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_DEBUG,"[Rc] Frame timestamp = %8"PRId64", skip one frame", (int64_t)pSrcPic->uiTimeStamp);
    return ENC_RETURN_SUCCESS;
  }

  eFrameType = DecideFrameType (pCtx, iSpatialNum);
  if (eFrameType == videoFrameTypeSkip) {
    pFbi->eFrameType = eFrameType;
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_DEBUG,"[Rc] Frame timestamp = %8"PRId64", skip one frame", (int64_t)pSrcPic->uiTimeStamp);
    return ENC_RETURN_SUCCESS;
  }

  //loop each layer to check if have skip frame when RC and frame skip enable
  if (CheckFrameSkipBasedMaxbr (pCtx, iSpatialNum, eFrameType, (uint32_t)pSrcPic->uiTimeStamp)) {
    pFbi->eFrameType = videoFrameTypeSkip;
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_DEBUG,"[Rc] Frame timestamp = %8"PRId64", skip one frame", (int64_t)pSrcPic->uiTimeStamp);
    return ENC_RETURN_SUCCESS;
  }

  InitFrameCoding (pCtx, eFrameType);

  iCurTid	= GetTemporalLevel (&pSvcParam->sDependencyLayers[pSpatialIndexMap->iDid], pCtx->iCodingIndex,
                              pSvcParam->uiGopSize);
  pCtx->uiTemporalId	= iCurTid;

  pLayerBsInfo->pBsBuf	= pCtx->pFrameBs ;
  pLayerBsInfo->pNalLengthInByte = pCtx->pOut->pNalLen;

  if (eFrameType == videoFrameTypeIDR) {
    ++ pCtx->sPSOVector.uiIdrPicId;
    //if ( pSvcParam->bEnableSSEI )

    // write parameter sets bitstream here
    pCtx->iEncoderError = WelsWriteParameterSets (pCtx, &pLayerBsInfo->pNalLengthInByte[0], &iCountNal);
    WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)

    pLayerBsInfo->uiSpatialId		= 0;
    pLayerBsInfo->uiTemporalId	= 0;
    pLayerBsInfo->uiQualityId		= 0;
    pLayerBsInfo->uiLayerType		= NON_VIDEO_CODING_LAYER;
    pLayerBsInfo->iNalCount		= iCountNal;

    ++ pLayerBsInfo;
    pLayerBsInfo->pBsBuf			= pCtx->pFrameBs + pCtx->iPosBsBuffer;
    pLayerBsInfo->pNalLengthInByte = (pLayerBsInfo - 1)->pNalLengthInByte + iCountNal;
    ++ iLayerNum;
  }

  pCtx->pCurDqLayer				= pCtx->ppDqLayerList[pSpatialIndexMap->iDid];
  pCtx->pCurDqLayer->pRefLayer	= NULL;

  while (iSpatialIdx < iSpatialNum) {
    const int32_t iDidIdx			= (pSpatialIndexMap + iSpatialIdx)->iDid;	// get iDid
    SSpatialLayerConfig* pParam		= &pSvcParam->sSpatialLayers[iDidIdx];

    pCtx->uiDependencyId	= iCurDid = (int8_t)iDidIdx;
    pCtx->pVpp->AnalyzeSpatialPic (pCtx, iDidIdx);

    pCtx->pEncPic	 = pEncPic = (pSpatialIndexMap + iSpatialIdx)->pSrc;
    pCtx->pEncPic->iPictureType	= pCtx->eSliceType;
    pCtx->pEncPic->iFramePoc		= pCtx->iPOC;

    iCurWidth	= pParam->iVideoWidth;
    iCurHeight	= pParam->iVideoHeight;

    iDidList[iSpatialIdx]	= iCurDid;

    // Encoding this picture might mulitiple sQualityStat layers potentially be encoded as followed

    switch (pParam->sSliceCfg.uiSliceMode) {
    case SM_FIXEDSLCNUM_SLICE:
    case SM_AUTO_SLICE: {
      if ((iCurDid > 0) && (pSvcParam->iMultipleThreadIdc > 1) &&
          (pSvcParam->sSpatialLayers[iCurDid].sSliceCfg.uiSliceMode == SM_FIXEDSLCNUM_SLICE
           && pSvcParam->iMultipleThreadIdc >= pSvcParam->sSpatialLayers[iCurDid].sSliceCfg.sSliceArgument.uiSliceNum)
         )
        AdjustEnhanceLayer (pCtx, iCurDid);
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
      WelsLog (pLogCtx, WELS_LOG_ERROR, "WelsEncoderEncodeExt(), iLayerNum(%d) overflow(max:%d)!", iLayerNum,
               MAX_LAYER_NUM_OF_FRAME);
      return ENC_RETURN_UNSUPPORTED_PARA;
    }

    iNalIdxInLayer	= 0;
    bAvcBased	= (iCurDid == BASE_DEPENDENCY_ID);
    pCtx->bNeedPrefixNalFlag	= (bAvcBased &&
                                 (pSvcParam->bPrefixNalAddingCtrl ||
                                  (pSvcParam->iSpatialLayerNum > 1)));

    if (eFrameType == videoFrameTypeP) {
      eNalType	= bAvcBased ? NAL_UNIT_CODED_SLICE : NAL_UNIT_CODED_SLICE_EXT;
    } else if (eFrameType == videoFrameTypeIDR) {
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

    pCtx->pFuncList->pMarkPic (pCtx);
    if (!pCtx->pFuncList->pBuildRefList (pCtx, pCtx->iPOC, 0)) {
      pCtx->pVpp->UpdateSpatialPictures (pCtx, pSvcParam, iCurTid, iDidIdx);
      // Force coding IDR as followed
      ForceCodingIDR (pCtx);
      WelsLog (pLogCtx, WELS_LOG_WARNING,
               "WelsEncoderEncodeExt(), WelsBuildRefList failed for P frames, pCtx->iNumRef0= %d. ForceCodingIDR!",
               pCtx->iNumRef0);
      pFbi->eFrameType = videoFrameTypeIDR;
      pCtx->iEncoderError = ENC_RETURN_CORRECTED;
      return ENC_RETURN_CORRECTED;
    }
    if (pCtx->eSliceType != I_SLICE) {
      pCtx->pFuncList->pAfterBuildRefList (pCtx);
    }
#ifdef LONG_TERM_REF_DUMP
    DumpRef (pCtx);
#endif
    if ((pSvcParam->iUsageType == SCREEN_CONTENT_REAL_TIME) && (pSvcParam->iRCMode != RC_OFF_MODE))
      pCtx->pVpp->AnalyzePictureComplexity (pCtx, pCtx->pEncPic, ((pCtx->eSliceType == P_SLICE)
                                            && (pCtx->iNumRef0 > 0)) ? pCtx->pRefList0[0] : NULL,
                                            iCurDid, pSvcParam->bEnableBackgroundDetection);

    WelsUpdateRefSyntax (pCtx,  pCtx->iPOC,
                         eFrameType);	//get reordering syntax used for writing slice header and transmit to encoder.
    PrefetchReferencePicture (pCtx, eFrameType);	// update reference picture for current pDq layer

    pCtx->pFuncList->pfRc.pfWelsRcPictureInit (pCtx);
    PreprocessSliceCoding (pCtx);	// MUST be called after pfWelsRcPictureInit() and WelsInitCurrentLayer()

    //TODO Complexity Calculation here for screen content
    iLayerSize	= 0;

    if (SM_SINGLE_SLICE == pParam->sSliceCfg.uiSliceMode) {	// only one slice within a sQualityStat layer
      int32_t iSliceSize = 0;
      int32_t iPayloadSize	= 0;

      if (pCtx->bNeedPrefixNalFlag) {
        pCtx->iEncoderError = AddPrefixNal (pCtx, pLayerBsInfo, &pLayerBsInfo->pNalLengthInByte[0], &iNalIdxInLayer, eNalType,
                                            eNalRefIdc,
                                            iPayloadSize);
        WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)
        iLayerSize += iPayloadSize;
      }

      WelsLoadNal (pCtx->pOut, eNalType, eNalRefIdc);

      pCtx->iEncoderError = WelsCodeOneSlice (pCtx, 0, eNalType);
      WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)

      WelsUnloadNal (pCtx->pOut);

      pCtx->iEncoderError = WelsEncodeNal (&pCtx->pOut->sNalList[pCtx->pOut->iNalIndex - 1],
                                           &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt,
                                           pCtx->iFrameBsSize - pCtx->iPosBsBuffer,
                                           pCtx->pFrameBs + pCtx->iPosBsBuffer,
                                           &pLayerBsInfo->pNalLengthInByte[iNalIdxInLayer]);
      WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)
      iSliceSize = pLayerBsInfo->pNalLengthInByte[iNalIdxInLayer];

      iLayerSize += iSliceSize;
      pCtx->iPosBsBuffer	+= iSliceSize;
      pLayerBsInfo->uiLayerType		= VIDEO_CODING_LAYER;
      pLayerBsInfo->uiSpatialId		= iCurDid;
      pLayerBsInfo->uiTemporalId	= iCurTid;
      pLayerBsInfo->uiQualityId		= 0;
      pLayerBsInfo->iNalCount		= ++ iNalIdxInLayer;
    }
    // for dynamic slicing single threading..
    else if ((SM_DYN_SLICE == pParam->sSliceCfg.uiSliceMode) && (pSvcParam->iMultipleThreadIdc <= 1)) {
      const int32_t kiLastMbInFrame = pCtx->pCurDqLayer->pSliceEncCtx->iMbNumInFrame;
      pCtx->iEncoderError = WelsCodeOnePicPartition (pCtx, pFbi, pLayerBsInfo, &iNalIdxInLayer, &iLayerSize, 0,
                            kiLastMbInFrame, 0);
      WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)
    } else {
      //other multi-slice uiSliceMode
      int32_t iRet = 0;
      // THREAD_FULLY_FIRE_MODE/THREAD_PICK_UP_MODE for any mode of non-SM_DYN_SLICE
      if ((SM_DYN_SLICE != pParam->sSliceCfg.uiSliceMode) && (pSvcParam->iMultipleThreadIdc > 1)) {
        iSliceCount	= GetCurrentSliceNum (pCtx->pCurDqLayer->pSliceEncCtx);
        if (iLayerNum + 1 >= MAX_LAYER_NUM_OF_FRAME) {	// check available layer_bs_info for further writing as followed
          WelsLog (pLogCtx, WELS_LOG_ERROR,
                   "WelsEncoderEncodeExt(), iLayerNum(%d) overflow(max:%d) at iDid= %d uiSliceMode= %d, iSliceCount= %d!",
                   iLayerNum, MAX_LAYER_NUM_OF_FRAME, iCurDid, pParam->sSliceCfg.uiSliceMode, iSliceCount);
          return ENC_RETURN_UNSUPPORTED_PARA;
        }
        if (iSliceCount <= 1) {
          WelsLog (pLogCtx, WELS_LOG_ERROR,
                   "WelsEncoderEncodeExt(), iSliceCount(%d) from GetCurrentSliceNum() is untrusted due stack/heap crupted!",
                   iSliceCount);
          return ENC_RETURN_UNEXPECTED;
        }

        if (pSvcParam->iCountThreadsNum >= iSliceCount) {	//THREAD_FULLY_FIRE_MODE
#if defined(MT_DEBUG)
          int64_t t_bs_append = 0;
#endif

          pCtx->iActiveThreadsNum	= iSliceCount;
          // to fire slice coding threads
          iRet = FiredSliceThreads (pCtx, &pCtx->pSliceThreading->pThreadPEncCtx[0],
                                    &pCtx->pSliceThreading->pReadySliceCodingEvent[0],
                                    &pCtx->pSliceThreading->pThreadMasterEvent[0],
                                    pLayerBsInfo, iSliceCount, pCtx->pCurDqLayer->pSliceEncCtx, false);
          if (iRet) {
            WelsLog (pLogCtx, WELS_LOG_ERROR,
                     "[MT] WelsEncoderEncodeExt(), FiredSliceThreads return(%d) failed and exit encoding frame, iCountThreadsNum= %d, iSliceCount= %d, uiSliceMode= %d, iMultipleThreadIdc= %d!!",
                     iRet, pSvcParam->iCountThreadsNum, iSliceCount, pParam->sSliceCfg.uiSliceMode, pSvcParam->iMultipleThreadIdc);
            return ENC_RETURN_UNEXPECTED;
          }

          WelsMultipleEventsWaitAllBlocking (iSliceCount, &pCtx->pSliceThreading->pSliceCodedEvent[0],
                                             &pCtx->pSliceThreading->pSliceCodedMasterEvent);


          // all slices are finished coding here
          WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)

          // append exclusive slice 0 bs to pFrameBs
#if defined(MT_DEBUG)
          t_bs_append = WelsTime();
#endif//MT_DEBUG
          iLayerSize = AppendSliceToFrameBs (pCtx, pLayerBsInfo, iSliceCount);
#if defined(MT_DEBUG)
          t_bs_append = WelsTime() - t_bs_append;
          if (pCtx->pSliceThreading->pFSliceDiff) {
            fprintf (pCtx->pSliceThreading->pFSliceDiff,
                     "%6"PRId64" us consumed at AppendSliceToFrameBs() for coding_idx: %d iDid: %d qid: %d\n",
                     t_bs_append, pCtx->iCodingIndex, iCurDid, 0);
          }
#endif//MT_DEBUG
        } else {	//THREAD_PICK_UP_MODE
          int32_t iNumThreadsRunning = 0;
          int32_t iNumThreadsScheduled = 0;
          int32_t iIndexOfSliceToBeCoded = 0;

          pCtx->iActiveThreadsNum	= pSvcParam->iCountThreadsNum;
          iNumThreadsScheduled	= pCtx->iActiveThreadsNum;
          iNumThreadsRunning		= iNumThreadsScheduled;
          // to fire slice coding threads
          iRet = FiredSliceThreads (pCtx, &pCtx->pSliceThreading->pThreadPEncCtx[0],
                                    &pCtx->pSliceThreading->pReadySliceCodingEvent[0],
                                    &pCtx->pSliceThreading->pThreadMasterEvent[0],
                                    pLayerBsInfo, iNumThreadsRunning, pCtx->pCurDqLayer->pSliceEncCtx, false);
          if (iRet) {
            WelsLog (pLogCtx, WELS_LOG_ERROR,
                     "[MT] WelsEncoderEncodeExt(), FiredSliceThreads return(%d) failed and exit encoding frame, iCountThreadsNum= %d, iSliceCount= %d, uiSliceMode= %d, iMultipleThreadIdc= %d!!",
                     iRet, pSvcParam->iCountThreadsNum, iSliceCount, pParam->sSliceCfg.uiSliceMode, pSvcParam->iMultipleThreadIdc);
            return ENC_RETURN_UNEXPECTED;
          }

          iIndexOfSliceToBeCoded = iNumThreadsRunning;
          while (1) {
            if (iIndexOfSliceToBeCoded >= iSliceCount && iNumThreadsRunning <= 0)
              break;
            WELS_THREAD_ERROR_CODE lwait	= 0;
            int32_t iEventId				= -1;

            lwait = WelsMultipleEventsWaitSingleBlocking (iNumThreadsScheduled,
                    &pCtx->pSliceThreading->pSliceCodedEvent[0],
                    &pCtx->pSliceThreading->pSliceCodedMasterEvent);
            iEventId = (int32_t) (lwait - WELS_THREAD_ERROR_WAIT_OBJECT_0);
            if (iEventId >= 0 && iEventId < iNumThreadsScheduled) {
              if (iIndexOfSliceToBeCoded < iSliceCount) {
                // pick up succeeding slice for threading
                // thread_id equal to iEventId per implementation here
                pCtx->pSliceThreading->pThreadPEncCtx[iEventId].iSliceIndex	= iIndexOfSliceToBeCoded;
                WelsEventSignal (&pCtx->pSliceThreading->pReadySliceCodingEvent[iEventId]);
                WelsEventSignal (&pCtx->pSliceThreading->pThreadMasterEvent[iEventId]);

                ++ iIndexOfSliceToBeCoded;
              } else {	// no other slices left for coding
                -- iNumThreadsRunning;
              }
            }
          }//while(1)

          // all slices are finished coding here
          // append exclusive slice 0 bs to pFrameBs
          iLayerSize = AppendSliceToFrameBs (pCtx, pLayerBsInfo, iSliceCount);
        }
      }
      // THREAD_FULLY_FIRE_MODE && SM_DYN_SLICE
      else if ((SM_DYN_SLICE == pParam->sSliceCfg.uiSliceMode) && (pSvcParam->iMultipleThreadIdc > 1)) {
        const int32_t kiPartitionCnt	= pCtx->iActiveThreadsNum; //pSvcParam->iCountThreadsNum;

        // to fire slice coding threads
        iRet = FiredSliceThreads (pCtx, &pCtx->pSliceThreading->pThreadPEncCtx[0],
                                  &pCtx->pSliceThreading->pReadySliceCodingEvent[0],
                                  &pCtx->pSliceThreading->pThreadMasterEvent[0],
                                  pLayerBsInfo, kiPartitionCnt, pCtx->pCurDqLayer->pSliceEncCtx, true);
        if (iRet) {
          WelsLog (pLogCtx, WELS_LOG_ERROR,
                   "[MT] WelsEncoderEncodeExt(), FiredSliceThreads return(%d) failed and exit encoding frame, iCountThreadsNum= %d, iSliceCount= %d, uiSliceMode= %d, iMultipleThreadIdc= %d!!",
                   iRet, pSvcParam->iCountThreadsNum, iSliceCount, pParam->sSliceCfg.uiSliceMode, pSvcParam->iMultipleThreadIdc);
          return ENC_RETURN_UNEXPECTED;
        }

        WelsMultipleEventsWaitAllBlocking (kiPartitionCnt, &pCtx->pSliceThreading->pSliceCodedEvent[0],
                                           &pCtx->pSliceThreading->pSliceCodedMasterEvent);
        WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)

        iLayerSize = AppendSliceToFrameBs (pCtx, pLayerBsInfo, kiPartitionCnt);
      } else {	// for non-dynamic-slicing mode single threading branch..
        const bool bNeedPrefix	= pCtx->bNeedPrefixNalFlag;
        int32_t iSliceIdx			= 0;

        iSliceCount	= GetCurrentSliceNum (pCtx->pCurDqLayer->pSliceEncCtx);
        while (iSliceIdx < iSliceCount) {
          int32_t iSliceSize	= 0;
          int32_t iPayloadSize	= 0;
          if (bNeedPrefix) {
            pCtx->iEncoderError = AddPrefixNal (pCtx, pLayerBsInfo, &pLayerBsInfo->pNalLengthInByte[0], &iNalIdxInLayer, eNalType,
                                                eNalRefIdc,
                                                iPayloadSize);
            WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)
            iLayerSize += iPayloadSize;
          }

          WelsLoadNal (pCtx->pOut, eNalType, eNalRefIdc);
          pCtx->iEncoderError = WelsCodeOneSlice (pCtx, iSliceIdx, eNalType);
          WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)

          WelsUnloadNal (pCtx->pOut);

          pCtx->iEncoderError = WelsEncodeNal (&pCtx->pOut->sNalList[pCtx->pOut->iNalIndex - 1],
                                               &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt,
                                               pCtx->iFrameBsSize - pCtx->iPosBsBuffer,
                                               pCtx->pFrameBs + pCtx->iPosBsBuffer, &pLayerBsInfo->pNalLengthInByte[iNalIdxInLayer]);
          WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)
          iSliceSize = pLayerBsInfo->pNalLengthInByte[iNalIdxInLayer];

          pCtx->iPosBsBuffer	+= iSliceSize;
          iLayerSize	+= iSliceSize;

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
        pLayerBsInfo->iNalCount		= iNalIdxInLayer;
      }
    }

    // deblocking filter
    if (
      (!pCtx->pCurDqLayer->bDeblockingParallelFlag) &&
#if !defined(ENABLE_FRAME_DUMP)
      ((eNalRefIdc != NRI_PRI_LOWEST) && (pSvcParam->sDependencyLayers[iDidIdx].iHighestTemporalId == 0
                                          || iCurTid < pSvcParam->sDependencyLayers[iDidIdx].iHighestTemporalId)) &&
#endif//!ENABLE_FRAME_DUMP
      true
    ) {
      PerformDeblockingFilter (pCtx);
    }

    pCtx->pFuncList->pfRc.pfWelsRcPictureInfoUpdate (pCtx, iLayerSize);
    RcTraceFrameBits (pCtx,pSrcPic->uiTimeStamp);
    pCtx->pDecPic->iFrameAverageQp = pCtx->pWelsSvcRc->iAverageFrameQp;

    //update scc related
    pCtx->pFuncList->pfUpdateFMESwitch (pCtx->pCurDqLayer);

    // reference picture list update
    if (eNalRefIdc != NRI_PRI_LOWEST) {
      if (!pCtx->pFuncList->pUpdateRefList (pCtx)) {
        pCtx->pVpp->UpdateSpatialPictures (pCtx, pSvcParam, iCurTid, iDidIdx);
        // Force coding IDR as followed
        ForceCodingIDR (pCtx);
        WelsLog (pLogCtx, WELS_LOG_WARNING, "WelsEncoderEncodeExt(), WelsUpdateRefList failed. ForceCodingIDR!");
        //the above is to set the next frame to be IDR
        pFbi->eFrameType = eFrameType;
        return ENC_RETURN_CORRECTED;
      }
    }

    iFrameSize += iLayerSize;
    //check MinCr
    {
      int32_t iImageSize = (pParam->iVideoWidth * pParam->iVideoHeight * 3) >> 1;
      int32_t iMinCr = g_ksLevelLimits[pParam->uiLevelIdc - 1].uiMinCR;
      if (iFrameSize > (iImageSize / iMinCr))
        WelsLog (pLogCtx, WELS_LOG_WARNING,
                 "WelsEncoderEncodeExt()MinCr Checking,codec bitstream size is larger than Level limitation");
    }
#ifdef ENABLE_FRAME_DUMP
    if (iCurDid + 1 < pSvcParam->iSpatialLayerNum) {
      DumpDependencyRec (fsnr, &pSvcParam->sDependencyLayers[iCurDid].sRecFileName[0], iCurDid,
                         pCtx->bDependencyRecFlag[iCurDid], pCtx->pCurDqLayer);
      pCtx->bDependencyRecFlag[iCurDid] = true;
    }
#endif//ENABLE_FRAME_DUMP

#if defined(ENABLE_PSNR_CALC)
    fSnrY	= WelsCalcPsnr (fsnr->pData[0],
                          fsnr->iLineSize[0],
                          pEncPic->pData[0],
                          pEncPic->iLineSize[0],
                          iCurWidth,
                          iCurHeight);
    fSnrU	= WelsCalcPsnr (fsnr->pData[1],
                          fsnr->iLineSize[1],
                          pEncPic->pData[1],
                          pEncPic->iLineSize[1],
                          (iCurWidth >> 1),
                          (iCurHeight >> 1));
    fSnrV	= WelsCalcPsnr (fsnr->pData[2],
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
             (uiFrameType == videoFrameTypeI || uiFrameType == videoFrameTypeIDR) ? "I" : "P",
             iCurTid,
             iCurDid,
             0,
             pCtx->pWelsSvcRc[pCtx->uiDependencyId].iAverageFrameQp,
             fSnrY,
             fSnrU,
             fSnrV,
             (iLayerSize << 3));
#endif//LAYER_INFO_OUTPUT

#if defined(STAT_OUTPUT)

#if defined(ENABLE_PSNR_CALC)
    {
      pCtx->sStatData[iCurDid][0].sQualityStat.rYPsnr[pCtx->eSliceType]	+= fSnrY;
      pCtx->sStatData[iCurDid][0].sQualityStat.rUPsnr[pCtx->eSliceType]	+= fSnrU;
      pCtx->sStatData[iCurDid][0].sQualityStat.rVPsnr[pCtx->eSliceType]	+= fSnrV;
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
      ++ pCtx->sStatData[iCurDid][0].sSliceData.iSliceCount[pCtx->eSliceType];	// for multiple slices coding
      pCtx->sStatData[iCurDid][0].sSliceData.iSliceSize[pCtx->eSliceType]	+= (iLayerSize << 3);	// bits
    }
#endif//STAT_OUTPUT

    iCountNal = pLayerBsInfo->iNalCount;
    ++ iLayerNum;
    ++ pLayerBsInfo;

    pLayerBsInfo->pBsBuf	= pCtx->pFrameBs + pCtx->iPosBsBuffer;
    pLayerBsInfo->pNalLengthInByte = (pLayerBsInfo - 1)->pNalLengthInByte + iCountNal;

    if (pSvcParam->iPaddingFlag && pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingSize > 0) {
      int32_t iPaddingNalSize = 0;
      pCtx->iEncoderError =  WritePadding (pCtx, pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingSize, iPaddingNalSize);
      WELS_VERIFY_RETURN_IFNEQ (pCtx->iEncoderError, ENC_RETURN_SUCCESS)

#if GOM_TRACE_FLAG
      WelsLog (pLogCtx, WELS_LOG_INFO, "[RC] encoding_qp%d Padding: %d", pCtx->uiDependencyId,
               pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingSize);
#endif
      if (iPaddingNalSize <= 0)
        return ENC_RETURN_UNEXPECTED;

      pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingBitrateStat += pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingSize;

      pCtx->pWelsSvcRc[pCtx->uiDependencyId].iPaddingSize = 0;

      pLayerBsInfo->uiSpatialId		= 0;
      pLayerBsInfo->uiTemporalId	= 0;
      pLayerBsInfo->uiQualityId		= 0;
      pLayerBsInfo->uiLayerType		= NON_VIDEO_CODING_LAYER;
      pLayerBsInfo->iNalCount		= 1;
      pLayerBsInfo->pNalLengthInByte[0] = iPaddingNalSize;
      ++ pLayerBsInfo;
      pLayerBsInfo->pBsBuf	= pCtx->pFrameBs + pCtx->iPosBsBuffer;
      pLayerBsInfo->pNalLengthInByte = (pLayerBsInfo - 1)->pNalLengthInByte + 1;
      ++ iLayerNum;
    }

    if ((pParam->sSliceCfg.uiSliceMode == SM_FIXEDSLCNUM_SLICE || pParam->sSliceCfg.uiSliceMode == SM_AUTO_SLICE)
        && pSvcParam->iMultipleThreadIdc > 1 &&
        pSvcParam->iMultipleThreadIdc >= pParam->sSliceCfg.sSliceArgument.uiSliceNum) {
      CalcSliceComplexRatio (pCtx->pSliceThreading->pSliceComplexRatio[iCurDid], pCtx->pCurDqLayer->pSliceEncCtx,
                             pCtx->pSliceThreading->pSliceConsumeTime[iCurDid]);
#if defined(MT_DEBUG)
      TrackSliceComplexities (pCtx, iCurDid);
#endif//#if defined(MT_DEBUG)
    }

    ++ iSpatialIdx;

    if (iCurDid + 1 < pSvcParam->iSpatialLayerNum) {
      WelsSwapDqLayers (pCtx);
    }

    if (pSvcParam->bEnableLongTermReference && (pCtx->pLtr[pCtx->uiDependencyId].bLTRMarkingFlag
        && (pCtx->pLtr[pCtx->uiDependencyId].iLTRMarkMode == LTR_DELAY_MARK))) {
      pCtx->bLongTermRefFlag[iDidIdx][0] = true;
    }

    if (pCtx->pVpp->UpdateSpatialPictures (pCtx, pSvcParam, iCurTid, iDidIdx) != 0) {
      ForceCodingIDR (pCtx);
      WelsLog (pLogCtx, WELS_LOG_WARNING, "WelsEncoderEncodeExt(), Logic Error Found in temporal level. ForceCodingIDR!");
      //the above is to set the next frame IDR
      pFbi->eFrameType = eFrameType;
      return ENC_RETURN_CORRECTED;
    }

    if (pSvcParam->bEnableLongTermReference && ((pCtx->pLtr[pCtx->uiDependencyId].bLTRMarkingFlag
        && (pCtx->pLtr[pCtx->uiDependencyId].iLTRMarkMode == LTR_DIRECT_MARK)) || eFrameType == videoFrameTypeIDR)) {
      pCtx->bLongTermRefFlag[iDidIdx][iCurTid] = true;
    }
  }

#if defined(MT_DEBUG)
  TrackSliceConsumeTime (pCtx, iDidList, iSpatialNum);
#endif//MT_DEBUG

  if (pSvcParam->iMultipleThreadIdc > 1 && iDidList[0] == BASE_DEPENDENCY_ID
      && ((pSvcParam->sSpatialLayers[0].sSliceCfg.uiSliceMode == SM_FIXEDSLCNUM_SLICE)
          || (pSvcParam->sSpatialLayers[0].sSliceCfg.uiSliceMode == SM_AUTO_SLICE))
      && pSvcParam->iMultipleThreadIdc >= pSvcParam->sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum
      && ((pSvcParam->sSpatialLayers[iDidList[iSpatialNum - 1]].sSliceCfg.uiSliceMode == SM_FIXEDSLCNUM_SLICE)
          || (pSvcParam->sSpatialLayers[iDidList[iSpatialNum - 1]].sSliceCfg.uiSliceMode == SM_AUTO_SLICE))
      && pSvcParam->iMultipleThreadIdc >= pSvcParam->sSpatialLayers[iDidList[iSpatialNum -
          1]].sSliceCfg.sSliceArgument.uiSliceNum) {
    AdjustBaseLayer (pCtx);
  }

#ifdef ENABLE_FRAME_DUMP
  DumpRecFrame (fsnr, &pSvcParam->sDependencyLayers[pSvcParam->iSpatialLayerNum - 1].sRecFileName[0],
                pSvcParam->iSpatialLayerNum - 1, pCtx->bRecFlag, pCtx->pCurDqLayer);	// pDecPic: final reconstruction output
  pCtx->bRecFlag = true;

#endif//ENABLE_FRAME_DUMP

  ++ pCtx->iCodingIndex;
  pCtx->eLastNalPriority	= eNalRefIdc;
  pFbi->iLayerNum			= iLayerNum;
  pFbi->iSubSeqId = GetSubSequenceId (pCtx, eFrameType);
  WelsEmms();

  pFbi->eFrameType = eFrameType;
  return ENC_RETURN_SUCCESS;
}

/*!
 * \brief	Wels SVC encoder parameters adjustment
 *			SVC adjustment results in new requirement in memory blocks adjustment
 */
int32_t WelsEncoderParamAdjust (sWelsEncCtx** ppCtx, SWelsSvcCodingParam* pNewParam) {
  SWelsSvcCodingParam* pOldParam		= NULL;
  int32_t iReturn = ENC_RETURN_SUCCESS;
  int8_t iIndexD = 0;
  bool bNeedReset = false;
  int16_t iSliceNum				= 1;	// number of slices used
  int32_t iCacheLineSize			= 16;	// on chip cache line size in byte
  uint32_t uiCpuFeatureFlags = 0;

  if (NULL == ppCtx || NULL == *ppCtx || NULL == pNewParam)	return 1;

  /* Check validation in new parameters */
  iReturn	= ParamValidationExt (& (*ppCtx)->sLogCtx, pNewParam);
  if (iReturn != ENC_RETURN_SUCCESS)	return iReturn;

  iReturn = GetMultipleThreadIdc (& (*ppCtx)->sLogCtx, pNewParam, iSliceNum, iCacheLineSize, uiCpuFeatureFlags);
  if (iReturn != ENC_RETURN_SUCCESS) {
    WelsLog (& (*ppCtx)->sLogCtx, WELS_LOG_ERROR, "WelsEncoderParamAdjust(), GetMultipleThreadIdc failed return %d.",
             iReturn);
    return iReturn;
  }


  pOldParam	= (*ppCtx)->pSvcParam;

  /* Decide whether need reset for IDR frame based on adjusting prarameters changed */
  /* Temporal levels, spatial settings and/ or quality settings changed need update parameter sets related. */
  bNeedReset	=	(pOldParam == NULL) ||
                (pOldParam->iSpatialLayerNum != pNewParam->iSpatialLayerNum) ||
                (pOldParam->iPicWidth != pNewParam->iPicWidth
                 || pOldParam->iPicHeight != pNewParam->iPicHeight) ||
                (pOldParam->SUsedPicRect.iWidth != pNewParam->SUsedPicRect.iWidth
                 || pOldParam->SUsedPicRect.iHeight != pNewParam->SUsedPicRect.iHeight) ||
                (pOldParam->bEnableLongTermReference != pNewParam->bEnableLongTermReference) ||
                (pOldParam->iLTRRefNum != pNewParam->iLTRRefNum) ||
                (pOldParam->iMultipleThreadIdc != pNewParam->iMultipleThreadIdc);
  if (pNewParam->iMaxNumRefFrame > pOldParam->iMaxNumRefFrame) {
    bNeedReset = true;
  }

  if (!bNeedReset) {	// Check its picture resolutions/quality settings respectively in each dependency layer
    iIndexD = 0;
    assert (pOldParam->iSpatialLayerNum == pNewParam->iSpatialLayerNum);
    do {
      const SSpatialLayerInternal* kpOldDlp	= &pOldParam->sDependencyLayers[iIndexD];
      const SSpatialLayerInternal* kpNewDlp	= &pNewParam->sDependencyLayers[iIndexD];
      float fT1 = .0f;
      float fT2 = .0f;

      // check frame size settings
      if (pOldParam->sSpatialLayers[iIndexD].iVideoWidth != pNewParam->sSpatialLayers[iIndexD].iVideoWidth ||
          pOldParam->sSpatialLayers[iIndexD].iVideoHeight != pNewParam->sSpatialLayers[iIndexD].iVideoHeight ||
          kpOldDlp->iActualWidth != kpNewDlp->iActualWidth ||
          kpOldDlp->iActualHeight != kpNewDlp->iActualHeight) {
        bNeedReset	= true;
        break;
      }

      if (pOldParam->sSpatialLayers[iIndexD].sSliceCfg.uiSliceMode != pNewParam->sSpatialLayers[iIndexD].sSliceCfg.uiSliceMode
          ||
          pOldParam->sSpatialLayers[iIndexD].sSliceCfg.sSliceArgument.uiSliceNum !=
          pNewParam->sSpatialLayers[iIndexD].sSliceCfg.sSliceArgument.uiSliceNum) {
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
      ++ iIndexD;
    } while (iIndexD < pOldParam->iSpatialLayerNum);
  }

  if (bNeedReset) {
    SParaSetOffsetVariable sTmpPsoVariable[PARA_SET_TYPE];
    uint16_t	          uiTmpIdrPicId;//this is for LTR!
    SLogContext sLogCtx = (*ppCtx)->sLogCtx;
    for (int32_t k = 0; k < PARA_SET_TYPE; k++)
      memset (((*ppCtx)->sPSOVector.sParaSetOffsetVariable[k].bUsedParaSetIdInBs), 0, MAX_PPS_COUNT * sizeof (bool));
    memcpy (sTmpPsoVariable, (*ppCtx)->sPSOVector.sParaSetOffsetVariable,
            (PARA_SET_TYPE)*sizeof (SParaSetOffsetVariable)); // confirmed_safe_unsafe_usage
    uiTmpIdrPicId = (*ppCtx)->sPSOVector.uiIdrPicId;

    WelsUninitEncoderExt (ppCtx);

    /* Update new parameters */
    if (WelsInitEncoderExt (ppCtx, pNewParam, &sLogCtx))
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
      (pNewParam->iUsageType==CAMERA_VIDEO_REAL_TIME?MAX_REFERENCE_PICTURE_COUNT_NUM_CAMERA:MAX_REFERENCE_PICTURE_COUNT_NUM_SCREEN));
    pNewParam->iLoopFilterDisableIdc					= WELS_CLIP3 (pNewParam->iLoopFilterDisableIdc, 0, 6);
    pNewParam->iLoopFilterAlphaC0Offset				= WELS_CLIP3 (pNewParam->iLoopFilterAlphaC0Offset, -6, 6);
    pNewParam->iLoopFilterBetaOffset					= WELS_CLIP3 (pNewParam->iLoopFilterBetaOffset, -6, 6);
    pNewParam->fMaxFrameRate							= WELS_CLIP3 (pNewParam->fMaxFrameRate, MIN_FRAME_RATE, MAX_FRAME_RATE);

    // we can not use direct struct based memcpy due some fields need keep unchanged as before
    pOldParam->fMaxFrameRate	= pNewParam->fMaxFrameRate;		// maximal frame rate [Hz / fps]
    pOldParam->iComplexityMode	= pNewParam->iComplexityMode;			// color space of input sequence
    pOldParam->uiIntraPeriod		= pNewParam->uiIntraPeriod;		// intra period (multiple of GOP size as desired)
    pOldParam->bEnableSpsPpsIdAddition = pNewParam->bEnableSpsPpsIdAddition;
    pOldParam->bPrefixNalAddingCtrl = pNewParam->bPrefixNalAddingCtrl;
    pOldParam->iNumRefFrame		= pNewParam->iNumRefFrame;		// number of reference frame used
    pOldParam->uiGopSize = pNewParam->uiGopSize;
    if (pOldParam->iTemporalLayerNum != pNewParam->iTemporalLayerNum) {
      pOldParam->iTemporalLayerNum = pNewParam->iTemporalLayerNum;
      (*ppCtx)->iCodingIndex = 0;
    }
    pOldParam->iDecompStages = pNewParam->iDecompStages;
    /* denoise control */
    pOldParam->bEnableDenoise	= pNewParam->bEnableDenoise;

    /* background detection control */
    pOldParam->bEnableBackgroundDetection		= pNewParam->bEnableBackgroundDetection;

    /* adaptive quantization control */
    pOldParam->bEnableAdaptiveQuant	= pNewParam->bEnableAdaptiveQuant;

    /* int32_t term reference control */
    pOldParam->bEnableLongTermReference	= pNewParam->bEnableLongTermReference;
    pOldParam->iLtrMarkPeriod	= pNewParam->iLtrMarkPeriod;

    // keep below values unchanged as before
    pOldParam->bEnableSSEI		= pNewParam->bEnableSSEI;
    pOldParam->bEnableFrameCroppingFlag	= pNewParam->bEnableFrameCroppingFlag;	// enable frame cropping flag

    /* Motion search */

    /* Deblocking loop filter */
    pOldParam->iLoopFilterDisableIdc	= pNewParam->iLoopFilterDisableIdc;	// 0: on, 1: off, 2: on except for slice boundaries
    pOldParam->iLoopFilterAlphaC0Offset	= pNewParam->iLoopFilterAlphaC0Offset;// AlphaOffset: valid range [-6, 6], default 0
    pOldParam->iLoopFilterBetaOffset		= pNewParam->iLoopFilterBetaOffset;	// BetaOffset:	valid range [-6, 6], default 0

    /* Rate Control */
    pOldParam->iRCMode	    	= pNewParam->iRCMode;
    pOldParam->iTargetBitrate	= pNewParam->iTargetBitrate;			// overall target bitrate introduced in RC module
    pOldParam->iPaddingFlag	    = pNewParam->iPaddingFlag;

    /* Layer definition */
    pOldParam->bPrefixNalAddingCtrl	= pNewParam->bPrefixNalAddingCtrl;

    // d
    iIndexD = 0;
    do {
      SSpatialLayerInternal* pOldDlpInternal	= &pOldParam->sDependencyLayers[iIndexD];
      SSpatialLayerInternal* pNewDlpInternal	= &pNewParam->sDependencyLayers[iIndexD];

      SSpatialLayerConfig* pOldDlp	= &pOldParam->sSpatialLayers[iIndexD];
      SSpatialLayerConfig* pNewDlp	= &pNewParam->sSpatialLayers[iIndexD];

      pOldDlpInternal->fInputFrameRate	= pNewDlpInternal->fInputFrameRate;	// input frame rate
      pOldDlpInternal->fOutputFrameRate	= pNewDlpInternal->fOutputFrameRate;	// output frame rate
      pOldDlp->iSpatialBitrate	= pNewDlp->iSpatialBitrate;

      pOldDlp->uiProfileIdc		= pNewDlp->uiProfileIdc;			// value of profile IDC (0 for auto-detection)

      /* Derived variants below */
      pOldDlpInternal->iTemporalResolution	= pNewDlpInternal->iTemporalResolution;
      pOldDlpInternal->iDecompositionStages	= pNewDlpInternal->iDecompositionStages;

      memcpy (pOldDlpInternal->uiCodingIdx2TemporalId, pNewDlpInternal->uiCodingIdx2TemporalId,
              sizeof (pOldDlpInternal->uiCodingIdx2TemporalId));	// confirmed_safe_unsafe_usage

      ++ iIndexD;
    } while (iIndexD < pOldParam->iSpatialLayerNum);
  }

  /* Any else initialization/reset for rate control here? */

  return 0;
}

void WelsEncoderApplyLTR (SLogContext* pLogCtx, sWelsEncCtx** ppCtx, SLTRConfig* pLTRValue) {
  SWelsSvcCodingParam	sConfig;
  int32_t iNumRefFrame = 1;
  memcpy (&sConfig, (*ppCtx)->pSvcParam, sizeof (SWelsSvcCodingParam));
  sConfig.bEnableLongTermReference = pLTRValue->bEnableLongTermReference;
  sConfig.iLTRRefNum = pLTRValue->iLTRRefNum;
  int32_t uiGopSize			= 1 << (sConfig.iTemporalLayerNum - 1);
  if (sConfig.iUsageType == SCREEN_CONTENT_REAL_TIME) {
    if (sConfig.bEnableLongTermReference) {
      sConfig.iLTRRefNum = LONG_TERM_REF_NUM_SCREEN;//WELS_CLIP3 (sConfig.iLTRRefNum, 1, LONG_TERM_REF_NUM_SCREEN);
      iNumRefFrame = WELS_MAX (1, WELS_LOG2 (uiGopSize)) + sConfig.iLTRRefNum;
    } else {
      sConfig.iLTRRefNum = 0;
      iNumRefFrame = WELS_MAX (1, uiGopSize >> 1);
    }
  } else {
    if (sConfig.bEnableLongTermReference) {
      sConfig.iLTRRefNum = LONG_TERM_REF_NUM;//WELS_CLIP3 (sConfig.iLTRRefNum, 1, LONG_TERM_REF_NUM);
    } else {
      sConfig.iLTRRefNum = 0;
    }
    iNumRefFrame		= ((uiGopSize >> 1) > 1) ? ((uiGopSize >> 1) + sConfig.iLTRRefNum) : (MIN_REF_PIC_COUNT +
                      sConfig.iLTRRefNum);
    iNumRefFrame		= WELS_CLIP3 (iNumRefFrame, MIN_REF_PIC_COUNT, MAX_REFERENCE_PICTURE_COUNT_NUM_CAMERA);

  }
  if (sConfig.iNumRefFrame < iNumRefFrame)
    sConfig.iNumRefFrame = iNumRefFrame;
  if (sConfig.iNumRefFrame > sConfig.iMaxNumRefFrame)
    sConfig.iMaxNumRefFrame = sConfig.iNumRefFrame;

  WelsLog (pLogCtx, WELS_LOG_INFO, " CWelsH264SVCEncoder::SetOption enable LTR = %d,ltrnum = %d",
           sConfig.bEnableLongTermReference, sConfig.iLTRRefNum);
  WelsEncoderParamAdjust (ppCtx, &sConfig);
}
int32_t DynSliceRealloc (sWelsEncCtx* pCtx,
                         SFrameBSInfo* pFrameBsInfo,
                         SLayerBSInfo* pLayerBsInfo) {
  CMemoryAlign* pMA = pCtx->pMemAlign;
  SDqLayer* pCurLayer = pCtx->pCurDqLayer;

  int32_t iCountNals = pCtx->pOut->iCountNals;
  int32_t iMaxSliceNumOld = pCurLayer->pSliceEncCtx->iMaxSliceNumConstraint;
  int32_t iMaxSliceNum = iMaxSliceNumOld;
  iCountNals += iMaxSliceNum * (pCtx->pSvcParam->iSpatialLayerNum + pCtx->bNeedPrefixNalFlag);
  iMaxSliceNum *= SLICE_NUM_EXPAND_COEF;

  SWelsNalRaw* pNalList = (SWelsNalRaw*)pMA->WelsMalloc (iCountNals * sizeof (SWelsNalRaw), "pOut->sNalList");
  if (NULL == pNalList)
    return ENC_RETURN_MEMALLOCERR;
  memcpy (pNalList, pCtx->pOut->sNalList, sizeof (SWelsNalRaw) * pCtx->pOut->iCountNals);
  pMA->WelsFree (pCtx->pOut->sNalList, "pOut->sNalList");
  pCtx->pOut->sNalList = pNalList;

  int32_t* pNalLen = (int32_t*)pMA->WelsMalloc (iCountNals * sizeof (int32_t), "pOut->pNalLen");
  if (NULL == pNalLen)
    return ENC_RETURN_MEMALLOCERR;
  memcpy (pNalLen, pCtx->pOut->pNalLen, sizeof (int32_t) * pCtx->pOut->iCountNals);
  pMA->WelsFree (pCtx->pOut->pNalLen, "pOut->pNalLen");
  pCtx->pOut->pNalLen = pNalLen;

  pCtx->pOut->iCountNals = iCountNals;
  SLayerBSInfo* pLBI1, *pLBI2;
  pLBI1 = &pFrameBsInfo->sLayerInfo[0];
  pLBI1->pNalLengthInByte = pCtx->pOut->pNalLen;
  while (pLBI1 != pLayerBsInfo) {
    pLBI2 = pLBI1;
    ++ pLBI1;
    pLBI1->pNalLengthInByte = pLBI2->pNalLengthInByte + pLBI2->iNalCount;
  }

  SSlice* pSlice = (SSlice*)pMA->WelsMallocz (sizeof (SSlice) * iMaxSliceNum, "Slice");
  if (NULL == pSlice)
    return ENC_RETURN_MEMALLOCERR;
  memcpy (pSlice, pCurLayer->sLayerInfo.pSliceInLayer, sizeof (SSlice) * iMaxSliceNumOld);
  int32_t uiSliceIdx;
  uiSliceIdx = iMaxSliceNumOld;
  SSlice* pBaseSlice = &pCurLayer->sLayerInfo.pSliceInLayer[0];
  SSliceHeaderExt* pBaseSHExt = &pBaseSlice->sSliceHeaderExt;
  SSlice* pSliceIdx = &pSlice[uiSliceIdx];
  while (uiSliceIdx < iMaxSliceNum) {
    SSliceHeaderExt* pSHExt = &pSliceIdx->sSliceHeaderExt;
    pSliceIdx->uiSliceIdx = uiSliceIdx;
    if (pCtx->pSvcParam->iMultipleThreadIdc > 1)
      pSliceIdx->pSliceBsa = &pCtx->pSliceBs[uiSliceIdx].sBsWrite;
    else
      pSliceIdx->pSliceBsa = &pCtx->pOut->sBsWrite;
    if (AllocMbCacheAligned (&pSliceIdx->sMbCacheInfo, pMA))
      return ENC_RETURN_MEMALLOCERR;

    pSliceIdx->bSliceHeaderExtFlag = pBaseSlice->bSliceHeaderExtFlag;
    pSHExt->sSliceHeader.iPpsId = pBaseSHExt->sSliceHeader.iPpsId;
    pSHExt->sSliceHeader.pPps = pBaseSHExt->sSliceHeader.pPps;
    pSHExt->sSliceHeader.iSpsId = pBaseSHExt->sSliceHeader.iSpsId;
    pSHExt->sSliceHeader.pSps = pBaseSHExt->sSliceHeader.pSps;
    pSHExt->sSliceHeader.uiRefCount = pCtx->iNumRef0;
    memcpy (&pSHExt->sSliceHeader.sRefMarking, &pBaseSHExt->sSliceHeader.sRefMarking, sizeof (SRefPicMarking));
    memcpy (&pSHExt->sSliceHeader.sRefReordering, &pBaseSHExt->sSliceHeader.sRefReordering,
            sizeof (SRefPicListReorderSyntax));

    pSliceIdx++;
    uiSliceIdx++;
  }
  pMA->WelsFree (pCurLayer->sLayerInfo.pSliceInLayer, "Slice");
  pCurLayer->sLayerInfo.pSliceInLayer = pSlice;

  int16_t* pFirstMbInSlice = (int16_t*)pMA->WelsMalloc (iMaxSliceNum * sizeof (int16_t), "pSliceSeg->pFirstMbInSlice");
  if (NULL == pFirstMbInSlice)
    return ENC_RETURN_MEMALLOCERR;
  memset (pFirstMbInSlice, 0, sizeof (int16_t) * iMaxSliceNum);
  memcpy (pFirstMbInSlice, pCurLayer->pSliceEncCtx->pFirstMbInSlice, sizeof (int16_t) * iMaxSliceNumOld);
  pMA->WelsFree (pCurLayer->pSliceEncCtx->pFirstMbInSlice, "pSliceSeg->pFirstMbInSlice");
  pCurLayer->pSliceEncCtx->pFirstMbInSlice = pFirstMbInSlice;

  int32_t* pCountMbNumInSlice = (int32_t*)pMA->WelsMalloc (iMaxSliceNum * sizeof (int32_t),
                                "pSliceSeg->pCountMbNumInSlice");
  if (NULL == pCountMbNumInSlice)
    return ENC_RETURN_MEMALLOCERR;
  memcpy (pCountMbNumInSlice, pCurLayer->pSliceEncCtx->pCountMbNumInSlice, sizeof (int32_t) * iMaxSliceNumOld);
  uiSliceIdx = iMaxSliceNumOld;
  while (uiSliceIdx < iMaxSliceNum) {
    pCountMbNumInSlice[uiSliceIdx] = pCurLayer->pSliceEncCtx->iMbNumInFrame;
    uiSliceIdx++;
  }
  pMA->WelsFree (pCurLayer->pSliceEncCtx->pCountMbNumInSlice, "pSliceSeg->pCountMbNumInSlice");
  pCurLayer->pSliceEncCtx->pCountMbNumInSlice = pCountMbNumInSlice;

  SRCSlicing* pSlcingOverRc = (SRCSlicing*)pMA->WelsMalloc (iMaxSliceNum * sizeof (SRCSlicing), "SlicingOverRC");
  if (NULL == pSlcingOverRc)
    return ENC_RETURN_MEMALLOCERR;
  memcpy (pSlcingOverRc, pCtx->pWelsSvcRc->pSlicingOverRc, sizeof (SRCSlicing) * iMaxSliceNumOld);
  uiSliceIdx = iMaxSliceNumOld;
  SRCSlicing* pSORC = &pSlcingOverRc[uiSliceIdx];
  const int32_t kiBitsPerMb		= WELS_DIV_ROUND (pCtx->pWelsSvcRc->iTargetBits * INT_MULTIPLY,
                                pCtx->pWelsSvcRc->iNumberMbFrame);
  while (uiSliceIdx < iMaxSliceNum) {
    pSORC->iComplexityIndexSlice = 0;
    pSORC->iCalculatedQpSlice = pCtx->iGlobalQp;
    pSORC->iTotalQpSlice	= 0;
    pSORC->iTotalMbSlice	= 0;
    pSORC->iTargetBitsSlice = WELS_DIV_ROUND (kiBitsPerMb * pCurLayer->pSliceEncCtx->pCountMbNumInSlice[uiSliceIdx],
                              INT_MULTIPLY);
    pSORC->iFrameBitsSlice	= 0;
    pSORC->iGomBitsSlice	= 0;
    pSORC ++;
    uiSliceIdx ++;
  }
  pMA->WelsFree (pCtx->pWelsSvcRc->pSlicingOverRc, "SlicingOverRC");
  pCtx->pWelsSvcRc->pSlicingOverRc = pSlcingOverRc;

  if (pCtx->iMaxSliceCount < iMaxSliceNum)
    pCtx->iMaxSliceCount = iMaxSliceNum;
  pCurLayer->pSliceEncCtx->iMaxSliceNumConstraint = iMaxSliceNum;
  return ENC_RETURN_SUCCESS;
}
int32_t WelsCodeOnePicPartition (sWelsEncCtx* pCtx,
                                 SFrameBSInfo* pFrameBSInfo,
                                 SLayerBSInfo* pLayerBsInfo,
                                 int32_t* pNalIdxInLayer,
                                 int32_t* pLayerSize,
                                 int32_t iFirstMbInPartition,	// first mb inclusive in partition
                                 int32_t iEndMbInPartition,	// end mb exclusive in partition
                                 int32_t iStartSliceIdx
                                ) {

  SDqLayer* pCurLayer			= pCtx->pCurDqLayer;
  SSliceCtx* pSliceCtx		= pCurLayer->pSliceEncCtx;
  int32_t iNalIdxInLayer		= *pNalIdxInLayer;
  int32_t iSliceIdx				= iStartSliceIdx;
  const int32_t kiSliceStep		= pCtx->iActiveThreadsNum;
  const int32_t kiPartitionId		= iStartSliceIdx % kiSliceStep;
  int32_t iPartitionBsSize		= 0;
  int32_t iAnyMbLeftInPartition = iEndMbInPartition - iFirstMbInPartition;
  const EWelsNalUnitType keNalType	= pCtx->eNalType;
  const EWelsNalRefIdc keNalRefIdc	= pCtx->eNalPriority;
  const bool kbNeedPrefix		= pCtx->bNeedPrefixNalFlag;
  const int32_t kiSliceIdxStep = pCtx->iActiveThreadsNum;
  int32_t iReturn = ENC_RETURN_SUCCESS;

  //init
  {
    pSliceCtx->pFirstMbInSlice[iSliceIdx]		= iFirstMbInPartition;
    pCurLayer->pNumSliceCodedOfPartition[kiPartitionId]	= 1;	// one slice per partition intialized, dynamic slicing inside
    pCurLayer->pLastMbIdxOfPartition[kiPartitionId]		= iEndMbInPartition - 1;
  }
  pCurLayer->pLastCodedMbIdxOfPartition[kiPartitionId] = 0;

  while (iAnyMbLeftInPartition > 0) {
    int32_t iSliceSize	= 0;
    int32_t iPayloadSize	= 0;

    if (iSliceIdx >= (pSliceCtx->iMaxSliceNumConstraint - kiSliceIdxStep)) {	// insufficient memory in pSliceInLayer[]
      if (pCtx->iActiveThreadsNum == 1) {
        if (DynSliceRealloc (pCtx, pFrameBSInfo, pLayerBsInfo)) //only single thread support re-alloc now
          return ENC_RETURN_MEMALLOCERR;
      } else if (iSliceIdx >= pSliceCtx->iMaxSliceNumConstraint) {
        return ENC_RETURN_MEMALLOCERR;
      }
    }

    if (kbNeedPrefix) {
      iReturn = AddPrefixNal (pCtx, pLayerBsInfo, &pLayerBsInfo->pNalLengthInByte[0], &iNalIdxInLayer, keNalType, keNalRefIdc,
                              iPayloadSize);
      WELS_VERIFY_RETURN_IFNEQ (iReturn, ENC_RETURN_SUCCESS)
      iPartitionBsSize += iPayloadSize;
    }

    WelsLoadNal (pCtx->pOut, keNalType, keNalRefIdc);
    iReturn = WelsCodeOneSlice (pCtx, iSliceIdx, keNalType);
    WELS_VERIFY_RETURN_IFNEQ (iReturn, ENC_RETURN_SUCCESS)
    WelsUnloadNal (pCtx->pOut);

    iReturn = WelsEncodeNal (&pCtx->pOut->sNalList[pCtx->pOut->iNalIndex - 1],
                             &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt,
                             pCtx->iFrameBsSize - pCtx->iPosBsBuffer,
                             pCtx->pFrameBs + pCtx->iPosBsBuffer,
                             &pLayerBsInfo->pNalLengthInByte[iNalIdxInLayer]);
    WELS_VERIFY_RETURN_IFNEQ (iReturn, ENC_RETURN_SUCCESS)
    iSliceSize = pLayerBsInfo->pNalLengthInByte[iNalIdxInLayer];

    pCtx->iPosBsBuffer	+= iSliceSize;
    iPartitionBsSize	+= iSliceSize;

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
  pLayerBsInfo->iNalCount		= iNalIdxInLayer;

  return ENC_RETURN_SUCCESS;
}
} // namespace WelsEnc
