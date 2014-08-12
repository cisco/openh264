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
 * \file	au_set.c
 *
 * \brief	Interfaces introduced in Access Unit level based writer
 *
 * \date	05/18/2009 Created
 *
 *************************************************************************************
 */

#include "au_set.h"
#include "svc_enc_golomb.h"
#include "macros.h"

#include "wels_common_defs.h"

using namespace WelsCommon;

namespace WelsEnc {

static inline int32_t WelsCheckLevelLimitation (const SWelsSPS* kpSps, const SLevelLimits* kpLevelLimit,
    float fFrameRate, int32_t iTargetBitRate) {
  uint32_t uiPicWidthInMBs = kpSps->iMbWidth;
  uint32_t uiPicHeightInMBs = kpSps->iMbHeight;
  uint32_t uiPicInMBs = uiPicWidthInMBs * uiPicHeightInMBs;
  uint32_t uiNumRefFrames = kpSps->iNumRefFrames;

  if (kpLevelLimit->uiMaxMBPS < (uint32_t) (uiPicInMBs * fFrameRate))
    return 0;
  if (kpLevelLimit->uiMaxFS < uiPicInMBs)
    return 0;
  if ((kpLevelLimit->uiMaxFS << 3) < (uiPicWidthInMBs * uiPicWidthInMBs))
    return 0;
  if ((kpLevelLimit->uiMaxFS << 3) < (uiPicHeightInMBs * uiPicHeightInMBs))
    return 0;
  if (kpLevelLimit->uiMaxDPBMbs < uiNumRefFrames * uiPicInMBs)
    return 0;
  if (iTargetBitRate
      && ((int32_t) kpLevelLimit->uiMaxBR  * 1200) < iTargetBitRate)    //RC enabled, considering bitrate constraint
    return 0;
  //add more checks here if needed in future

  return 1;

}

int32_t WelsCheckRefFrameLimitation (SLogContext* pLogCtx, SWelsSvcCodingParam* pParam) {
  int32_t i = 0;
  int32_t iRefFrame = 1;
  //get the number of reference frame according to level limitation.
  for (i = 0; i < pParam->iSpatialLayerNum; ++ i) {
    SSpatialLayerConfig* pSpatialLayer = &pParam->sSpatialLayers[i];
    uint32_t uiPicInMBs = ((pSpatialLayer->iVideoHeight + 15) >> 4) * ((pSpatialLayer->iVideoWidth + 15) >> 4);
    if (pSpatialLayer->uiLevelIdc == LEVEL_UNKNOWN) {
      pSpatialLayer->uiLevelIdc = LEVEL_5_0;
      WelsLog (pLogCtx, WELS_LOG_WARNING, "change level to level5.0\n");
    }
    iRefFrame = g_ksLevelLimits[pSpatialLayer->uiLevelIdc - 1].uiMaxDPBMbs / uiPicInMBs;
    if (iRefFrame < pParam->iMaxNumRefFrame)
      pParam->iMaxNumRefFrame = iRefFrame;
    if (pParam->iMaxNumRefFrame < 1) {
      pParam->iMaxNumRefFrame = 1;
      WelsLog (pLogCtx, WELS_LOG_ERROR, "error Level setting (%d)\n", pSpatialLayer->uiLevelIdc);
      return ENC_RETURN_UNSUPPORTED_PARA;
    }
  }

  return ENC_RETURN_SUCCESS;
}
static inline int32_t WelsGetLevelIdc (const SWelsSPS* kpSps, float fFrameRate, int32_t iTargetBitRate) {
  int32_t iOrder;
  for (iOrder = 0; iOrder < LEVEL_NUMBER; iOrder++) {
    if (WelsCheckLevelLimitation (kpSps, & (g_ksLevelLimits[iOrder]), fFrameRate, iTargetBitRate)) {
      return (int32_t) (g_ksLevelLimits[iOrder].uiLevelIdc);
    }
  }
  return 51; //final decision: select the biggest level
}


/*!
 *************************************************************************************
 * \brief	to set Sequence Parameter Set (SPS)
 *
 * \param 	pSps 	SWelsSPS to be wrote, update iSpsId dependency
 * \param	pBitStringAux		bitstream writer auxiliary
 *
 * \return	0 - successed
 *	    	1 - failed
 *
 * \note	Call it in case EWelsNalUnitType is SPS.
 *************************************************************************************
 */
int32_t WelsWriteSpsSyntax (SWelsSPS* pSps, SBitStringAux* pBitStringAux, int32_t* pSpsIdDelta) {
  SBitStringAux* pLocalBitStringAux = pBitStringAux;

  assert (pSps != NULL && pBitStringAux != NULL);

  BsWriteBits (pLocalBitStringAux, 8, pSps->uiProfileIdc);

  BsWriteOneBit (pLocalBitStringAux, pSps->bConstraintSet0Flag);	// bConstraintSet0Flag
  BsWriteOneBit (pLocalBitStringAux, pSps->bConstraintSet1Flag);	// bConstraintSet1Flag
  BsWriteOneBit (pLocalBitStringAux, pSps->bConstraintSet2Flag);	// bConstraintSet2Flag
  BsWriteOneBit (pLocalBitStringAux, pSps->bConstraintSet3Flag);	// bConstraintSet3Flag
  BsWriteBits (pLocalBitStringAux, 4, 0);							// reserved_zero_4bits, equal to 0
  BsWriteBits (pLocalBitStringAux, 8, pSps->iLevelIdc);				// iLevelIdc
  BsWriteUE (pLocalBitStringAux, pSps->uiSpsId + pSpsIdDelta[pSps->uiSpsId]);					     // seq_parameter_set_id

  if (PRO_SCALABLE_BASELINE == pSps->uiProfileIdc || PRO_SCALABLE_HIGH == pSps->uiProfileIdc ||
      PRO_HIGH == pSps->uiProfileIdc || PRO_HIGH10 == pSps->uiProfileIdc ||
      PRO_HIGH422 == pSps->uiProfileIdc || PRO_HIGH444 == pSps->uiProfileIdc ||
      PRO_CAVLC444 == pSps->uiProfileIdc || 44 == pSps->uiProfileIdc) {
    BsWriteUE (pLocalBitStringAux, 1);  //uiChromaFormatIdc, now should be 1
    BsWriteUE (pLocalBitStringAux, 0); //uiBitDepthLuma
    BsWriteUE (pLocalBitStringAux, 0); //uiBitDepthChroma
    BsWriteOneBit (pLocalBitStringAux, 0); //qpprime_y_zero_transform_bypass_flag
    BsWriteOneBit (pLocalBitStringAux, 0); //seq_scaling_matrix_present_flag
  }

  BsWriteUE (pLocalBitStringAux, pSps->uiLog2MaxFrameNum - 4);	// log2_max_frame_num_minus4
  BsWriteUE (pLocalBitStringAux, 0/*pSps->uiPocType*/);		     // pic_order_cnt_type
  BsWriteUE (pLocalBitStringAux, pSps->iLog2MaxPocLsb - 4);	// log2_max_pic_order_cnt_lsb_minus4

  BsWriteUE (pLocalBitStringAux, pSps->iNumRefFrames);		// max_num_ref_frames
  BsWriteOneBit (pLocalBitStringAux, true/*pSps->bGapsInFrameNumValueAllowedFlag*/);	// bGapsInFrameNumValueAllowedFlag
  BsWriteUE (pLocalBitStringAux, pSps->iMbWidth - 1);		// pic_width_in_mbs_minus1
  BsWriteUE (pLocalBitStringAux, pSps->iMbHeight - 1);		// pic_height_in_map_units_minus1
  BsWriteOneBit (pLocalBitStringAux, true/*pSps->bFrameMbsOnlyFlag*/);	// bFrameMbsOnlyFlag

  BsWriteOneBit (pLocalBitStringAux, 0/*pSps->bDirect8x8InferenceFlag*/);	// direct_8x8_inference_flag
  BsWriteOneBit (pLocalBitStringAux, pSps->bFrameCroppingFlag);	// bFrameCroppingFlag
  if (pSps->bFrameCroppingFlag) {
    BsWriteUE (pLocalBitStringAux, pSps->sFrameCrop.iCropLeft);	// frame_crop_left_offset
    BsWriteUE (pLocalBitStringAux, pSps->sFrameCrop.iCropRight);	// frame_crop_right_offset
    BsWriteUE (pLocalBitStringAux, pSps->sFrameCrop.iCropTop);	// frame_crop_top_offset
    BsWriteUE (pLocalBitStringAux, pSps->sFrameCrop.iCropBottom);	// frame_crop_bottom_offset
  }

  BsWriteOneBit (pLocalBitStringAux, 0/*pSps->bVuiParamPresentFlag*/);	// vui_parameters_present_flag

  return 0;
}


int32_t WelsWriteSpsNal (SWelsSPS* pSps, SBitStringAux* pBitStringAux, int32_t* pSpsIdDelta) {
  WelsWriteSpsSyntax (pSps, pBitStringAux, pSpsIdDelta);

  BsRbspTrailingBits (pBitStringAux);

  return 0;
}

/*!
 *************************************************************************************
 * \brief	to write SubSet Sequence Parameter Set
 *
 * \param 	sub_sps		subset pSps parsed
 * \param	pBitStringAux		bitstream writer auxiliary
 *
 * \return	0 - successed
 *		    1 - failed
 *
 * \note	Call it in case EWelsNalUnitType is SubSet SPS.
 *************************************************************************************
 */

int32_t WelsWriteSubsetSpsSyntax (SSubsetSps* pSubsetSps, SBitStringAux* pBitStringAux , int32_t* pSpsIdDelta) {
  SWelsSPS* pSps = &pSubsetSps->pSps;

  WelsWriteSpsSyntax (pSps, pBitStringAux, pSpsIdDelta);

  if (pSps->uiProfileIdc == PRO_SCALABLE_BASELINE || pSps->uiProfileIdc == PRO_SCALABLE_HIGH) {
    SSpsSvcExt* pSubsetSpsExt = &pSubsetSps->sSpsSvcExt;

    BsWriteOneBit (pBitStringAux, true/*pSubsetSpsExt->bInterLayerDeblockingFilterCtrlPresentFlag*/);
    BsWriteBits (pBitStringAux, 2, pSubsetSpsExt->iExtendedSpatialScalability);
    BsWriteOneBit (pBitStringAux, 0/*pSubsetSpsExt->uiChromaPhaseXPlus1Flag*/);
    BsWriteBits (pBitStringAux, 2, 1/*pSubsetSpsExt->uiChromaPhaseYPlus1*/);
    if (pSubsetSpsExt->iExtendedSpatialScalability == 1) {
      BsWriteOneBit (pBitStringAux, 0/*pSubsetSpsExt->uiSeqRefLayerChromaPhaseXPlus1Flag*/);
      BsWriteBits (pBitStringAux, 2, 1/*pSubsetSpsExt->uiSeqRefLayerChromaPhaseYPlus1*/);
      BsWriteSE (pBitStringAux, 0/*pSubsetSpsExt->sSeqScaledRefLayer.left_offset*/);
      BsWriteSE (pBitStringAux, 0/*pSubsetSpsExt->sSeqScaledRefLayer.top_offset*/);
      BsWriteSE (pBitStringAux, 0/*pSubsetSpsExt->sSeqScaledRefLayer.right_offset*/);
      BsWriteSE (pBitStringAux, 0/*pSubsetSpsExt->sSeqScaledRefLayer.bottom_offset*/);
    }
    BsWriteOneBit (pBitStringAux, pSubsetSpsExt->bSeqTcoeffLevelPredFlag);
    if (pSubsetSpsExt->bSeqTcoeffLevelPredFlag) {
      BsWriteOneBit (pBitStringAux, pSubsetSpsExt->bAdaptiveTcoeffLevelPredFlag);
    }
    BsWriteOneBit (pBitStringAux, pSubsetSpsExt->bSliceHeaderRestrictionFlag);

    BsWriteOneBit (pBitStringAux, false/*pSubsetSps->bSvcVuiParamPresentFlag*/);
  }
  BsWriteOneBit (pBitStringAux, false/*pSubsetSps->bAdditionalExtension2Flag*/);

  BsRbspTrailingBits (pBitStringAux);

  return 0;
}

/*!
 *************************************************************************************
 * \brief	to write Picture Parameter Set (PPS)
 *
 * \param 	pPps     	pPps
 * \param	pBitStringAux		bitstream writer auxiliary
 *
 * \return	0 - successed
 *	    	1 - failed
 *
 * \note	Call it in case EWelsNalUnitType is PPS.
 *************************************************************************************
 */
int32_t WelsWritePpsSyntax (SWelsPPS* pPps, SBitStringAux* pBitStringAux, SParaSetOffset* sPSOVector) {
  SBitStringAux* pLocalBitStringAux = pBitStringAux;

  bool bUsedSubset    =  sPSOVector->bPpsIdMappingIntoSubsetsps[pPps->iPpsId];
  int32_t iParameterSetType = (bUsedSubset ? PARA_SET_TYPE_SUBSETSPS : PARA_SET_TYPE_AVCSPS);

  BsWriteUE (pLocalBitStringAux, pPps->iPpsId +
             sPSOVector->sParaSetOffsetVariable[PARA_SET_TYPE_PPS].iParaSetIdDelta[pPps->iPpsId]);
  BsWriteUE (pLocalBitStringAux, pPps->iSpsId +
             sPSOVector->sParaSetOffsetVariable[iParameterSetType].iParaSetIdDelta[pPps->iSpsId]);

#if _DEBUG
  //SParaSetOffset use, 110421
  if (sPSOVector->bEnableSpsPpsIdAddition) {
    const int32_t kiTmpSpsIdInBs = pPps->iSpsId +
                                   sPSOVector->sParaSetOffsetVariable[iParameterSetType].iParaSetIdDelta[pPps->iSpsId];
    const int32_t tmp_pps_id_in_bs = pPps->iPpsId +
                                     sPSOVector->sParaSetOffsetVariable[PARA_SET_TYPE_PPS].iParaSetIdDelta[pPps->iPpsId];
    assert (MAX_SPS_COUNT > kiTmpSpsIdInBs);
    assert (MAX_PPS_COUNT > tmp_pps_id_in_bs);
    assert (sPSOVector->sParaSetOffsetVariable[iParameterSetType].bUsedParaSetIdInBs[kiTmpSpsIdInBs]);
  }
#endif

  BsWriteOneBit (pLocalBitStringAux, false/*pPps->entropy_coding_mode_flag*/);
  BsWriteOneBit (pLocalBitStringAux, false/*pPps->bPicOrderPresentFlag*/);

#ifdef DISABLE_FMO_FEATURE
  BsWriteUE (pLocalBitStringAux, 0/*pPps->uiNumSliceGroups - 1*/);
#else
  BsWriteUE (pLocalBitStringAux, pPps->uiNumSliceGroups - 1);
  if (pPps->uiNumSliceGroups > 1) {
    uint32_t i, uiNumBits;

    BsWriteUE (pLocalBitStringAux, pPps->uiSliceGroupMapType);

    switch (pPps->uiSliceGroupMapType) {
    case 0:
      for (i = 0; i < pPps->uiNumSliceGroups; i ++) {
        BsWriteUE (pLocalBitStringAux, pPps->uiRunLength[i] - 1);
      }
      break;
    case 2:
      for (i = 0; i < pPps->uiNumSliceGroups; i ++) {
        BsWriteUE (pLocalBitStringAux, pPps->uiTopLeft[i]);
        BsWriteUE (pLocalBitStringAux, pPps->uiBottomRight[i]);
      }
      break;
    case 3:
    case 4:
    case 5:
      BsWriteOneBit (pLocalBitStringAux, pPps->bSliceGroupChangeDirectionFlag);
      BsWriteUE (pLocalBitStringAux, pPps->uiSliceGroupChangeRate - 1);
      break;
    case 6:
      BsWriteUE (pLocalBitStringAux, pPps->uiPicSizeInMapUnits - 1);
      uiNumBits = 0;///////////////////WELS_CEILLOG2(pPps->uiPicSizeInMapUnits);
      for (i = 0; i < pPps->uiPicSizeInMapUnits; i ++) {
        BsWriteBits (pLocalBitStringAux, uiNumBits, pPps->uiSliceGroupId[i]);
      }
      break;
    default:
      break;
    }
  }
#endif//!DISABLE_FMO_FEATURE

  BsWriteUE (pLocalBitStringAux, 0/*pPps->uiNumRefIdxL0Active - 1*/);
  BsWriteUE (pLocalBitStringAux, 0/*pPps->uiNumRefIdxL1Active - 1*/);


  BsWriteOneBit (pLocalBitStringAux, false/*pPps->bWeightedPredFlag*/);
  BsWriteBits (pLocalBitStringAux, 2, 0/*pPps->uiWeightedBiPredIdc*/);

  BsWriteSE (pLocalBitStringAux, pPps->iPicInitQp - 26);
  BsWriteSE (pLocalBitStringAux, pPps->iPicInitQs - 26);

  BsWriteSE (pLocalBitStringAux, pPps->uiChromaQpIndexOffset);
  BsWriteOneBit (pLocalBitStringAux, pPps->bDeblockingFilterControlPresentFlag);
  BsWriteOneBit (pLocalBitStringAux, false/*pPps->bConstainedIntraPredFlag*/);
  BsWriteOneBit (pLocalBitStringAux, false/*pPps->bRedundantPicCntPresentFlag*/);

  BsRbspTrailingBits (pLocalBitStringAux);

  return 0;
}

static inline bool WelsGetPaddingOffset (int32_t iActualWidth, int32_t iActualHeight,  int32_t iWidth,
    int32_t iHeight, SCropOffset& pOffset) {
  if ((iWidth < iActualWidth) || (iHeight < iActualHeight))
    return false;

  // make actual size even
  iActualWidth -= (iActualWidth & 1);
  iActualHeight -= (iActualHeight & 1);

  pOffset.iCropLeft = 0;
  pOffset.iCropRight = (iWidth - iActualWidth) / 2;
  pOffset.iCropTop = 0;
  pOffset.iCropBottom = (iHeight - iActualHeight) / 2;

  return (iWidth > iActualWidth) || (iHeight > iActualHeight);
}
int32_t WelsInitSps (SWelsSPS* pSps, SSpatialLayerConfig* pLayerParam, SSpatialLayerInternal* pLayerParamInternal,
                     const uint32_t kuiIntraPeriod, const int32_t kiNumRefFrame,
                     const uint32_t kuiSpsId, const bool kbEnableFrameCropping, bool bEnableRc) {
  memset (pSps, 0, sizeof (SWelsSPS));

  pSps->uiSpsId		= kuiSpsId;
  pSps->iMbWidth	= (pLayerParam->iVideoWidth + 15) >> 4;
  pSps->iMbHeight	= (pLayerParam->iVideoHeight + 15) >> 4;

  if (0 == kuiIntraPeriod) {
    //max value of both iFrameNum and POC are 2^16-1, in our encoder, iPOC=2*iFrameNum, so max of iFrameNum should be 2^15-1.--
    pSps->uiLog2MaxFrameNum = 15;//16;
  } else {
    pSps->uiLog2MaxFrameNum	= 4;
    while ((uint32_t) (1 << pSps->uiLog2MaxFrameNum) <= kuiIntraPeriod) {
      ++ pSps->uiLog2MaxFrameNum;
    }
  }
  pSps->iLog2MaxPocLsb	= 1 + pSps->uiLog2MaxFrameNum;

  pSps->iNumRefFrames	= kiNumRefFrame;	/* min pRef size when fifo pRef operation*/

  if (kbEnableFrameCropping) {
    // TODO: get frame_crop_left_offset, frame_crop_right_offset, frame_crop_top_offset, frame_crop_bottom_offset
    pSps->bFrameCroppingFlag = WelsGetPaddingOffset (pLayerParamInternal->iActualWidth, pLayerParamInternal->iActualHeight,
                               pLayerParam->iVideoWidth, pLayerParam->iVideoHeight, pSps->sFrameCrop);
  } else {
    pSps->bFrameCroppingFlag	= false;
  }

  pSps->uiProfileIdc	= pLayerParam->uiProfileIdc ? pLayerParam->uiProfileIdc : PRO_BASELINE;

  if (bEnableRc)  //fixed QP condition
    pSps->iLevelIdc	= WelsGetLevelIdc (pSps, pLayerParamInternal->fOutputFrameRate, pLayerParam->iSpatialBitrate);
  else
    pSps->iLevelIdc  = WelsGetLevelIdc (pSps, pLayerParamInternal->fOutputFrameRate,
                                        0); // Set tar_br = 0 to remove the bitrate constraint; a better way is to set actual tar_br as 0

  //for Scalable Baseline, Scalable High, and Scalable High Intra profiles.If level_idc is equal to 9, the indicated level is level 1b.
  //for the Baseline, Constrained Baseline, Main, and Extended profiles,If level_idc is equal to 11 and constraint_set3_flag is equal to 1, the indicated level is level 1b.
  if ((pSps->iLevelIdc == 9) &&
      ((pSps->uiProfileIdc == PRO_BASELINE) || (pSps->uiProfileIdc == PRO_MAIN) || (pSps->uiProfileIdc == PRO_EXTENDED))) {
    pSps->iLevelIdc = 11;
    pSps->bConstraintSet3Flag = true;
  }
  return 0;
}


int32_t WelsInitSubsetSps (SSubsetSps* pSubsetSps, SSpatialLayerConfig* pLayerParam,
                           SSpatialLayerInternal* pLayerParamInternal,
                           const uint32_t kuiIntraPeriod, const int32_t kiNumRefFrame,
                           const uint32_t kuiSpsId, const bool kbEnableFrameCropping, bool bEnableRc) {
  SWelsSPS* pSps = &pSubsetSps->pSps;

  memset (pSubsetSps, 0, sizeof (SSubsetSps));

  WelsInitSps (pSps, pLayerParam, pLayerParamInternal, kuiIntraPeriod, kiNumRefFrame, kuiSpsId, kbEnableFrameCropping,
               bEnableRc);

  pSps->uiProfileIdc	= (pLayerParam->uiProfileIdc >= PRO_SCALABLE_BASELINE) ? pLayerParam->uiProfileIdc :
                        PRO_SCALABLE_BASELINE;

  pSubsetSps->sSpsSvcExt.iExtendedSpatialScalability	= 0;	/* ESS is 0 in default */
  pSubsetSps->sSpsSvcExt.bAdaptiveTcoeffLevelPredFlag	= false;
  pSubsetSps->sSpsSvcExt.bSeqTcoeffLevelPredFlag	= false;
  pSubsetSps->sSpsSvcExt.bSliceHeaderRestrictionFlag = true;

  return 0;
}

int32_t WelsInitPps (SWelsPPS* pPps,
                     SWelsSPS* pSps,
                     SSubsetSps* pSubsetSps,
                     const uint32_t kuiPpsId,
                     const bool kbDeblockingFilterPresentFlag,
                     const bool kbUsingSubsetSps) {
  SWelsSPS* pUsedSps = NULL;
  if (pPps == NULL || (pSps == NULL && pSubsetSps == NULL))
    return 1;
  if (!kbUsingSubsetSps) {
    assert (pSps != NULL);
    if (NULL == pSps)
      return 1;
    pUsedSps	= pSps;
  } else {
    assert (pSubsetSps != NULL);
    if (NULL == pSubsetSps)
      return 1;
    pUsedSps	= &pSubsetSps->pSps;
  }

  /* fill picture parameter set syntax */
  pPps->iPpsId		= kuiPpsId;
  pPps->iSpsId		= pUsedSps->uiSpsId;
#if !defined(DISABLE_FMO_FEATURE)
  pPps->uiNumSliceGroups =  1;	//param->qos_param.sliceGroupCount;
  if (pPps->uiNumSliceGroups > 1) {
    pPps->uiSliceGroupMapType = 0;	//param->qos_param.sliceGroupType;
    if (pPps->uiSliceGroupMapType == 0) {
      uint32_t uiGroup = 0;
      while (uiGroup < pPps->uiNumSliceGroups) {
        pPps->uiRunLength[uiGroup]	= 25;
        ++ uiGroup;
      }
    } else if (pPps->uiSliceGroupMapType == 2) {
      memset (&pPps->uiTopLeft[0], 0, MAX_SLICEGROUP_IDS * sizeof (pPps->uiTopLeft[0]));
      memset (&pPps->uiBottomRight[0], 0, MAX_SLICEGROUP_IDS * sizeof (pPps->uiBottomRight[0]));
    } else if (pPps->uiSliceGroupMapType >= 3 &&
               pPps->uiSliceGroupMapType <= 5) {
      pPps->bSliceGroupChangeDirectionFlag = false;
      pPps->uiSliceGroupChangeRate = 0;
    } else if (pPps->uiSliceGroupMapType == 6) {
      pPps->uiPicSizeInMapUnits = 1;
      memset (&pPps->uiSliceGroupId[0], 0, MAX_SLICEGROUP_IDS * sizeof (pPps->uiSliceGroupId[0]));
    }
  }
#endif//!DISABLE_FMO_FEATURE

  pPps->iPicInitQp							= 26;
  pPps->iPicInitQs							= 26;

  pPps->uiChromaQpIndexOffset					= 0;
  pPps->bDeblockingFilterControlPresentFlag	= kbDeblockingFilterPresentFlag;

  return 0;
}
} // namespace WelsEnc
