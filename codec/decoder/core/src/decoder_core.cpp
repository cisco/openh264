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
 *	decoder_core.c:	Wels decoder framework core implementation
 */

#include "decoder_core.h"
#include "error_code.h"
#include "memmgr_nal_unit.h"
#include "au_parser.h"
#include "decode_slice.h"
#include "manage_dec_ref.h"
#include "expand_pic.h"
#include "decoder.h"
#include "decode_mb_aux.h"
#include "mem_align.h"

namespace WelsDec {

static inline int32_t DecodeFrameConstruction (PWelsDecoderContext pCtx, uint8_t** ppDst, int32_t* pDstLen,
    int32_t* pWidth, int32_t* pHeight, SBufferInfo* pDstInfo) {
  PDqLayer pCurDq = pCtx->pCurDqLayer;
  PPicture pPic = pCtx->pDec;

  const int32_t kiWidth = pCurDq->iMbWidth << 4;
  const int32_t kiHeight = pCurDq->iMbHeight << 4;

  const int32_t kiTotalNumMbInCurLayer = pCurDq->iMbWidth * pCurDq->iMbHeight;

  if (pPic->iTotalNumMbRec != kiTotalNumMbInCurLayer) {
    WelsLog (pCtx, WELS_LOG_WARNING,
             "DecodeFrameConstruction():::iTotalNumMbRec:%d, total_num_mb_sps:%d, cur_layer_mb_width:%d, cur_layer_mb_height:%d \n",
             pPic->iTotalNumMbRec, kiTotalNumMbInCurLayer, pCurDq->iMbWidth, pCurDq->iMbHeight);
    return -1;
  }
#ifdef NO_WAITING_AU
  pPic->iTotalNumMbRec = 0;
#endif

  if (I_SLICE == pCurDq->sLayerInfo.sSliceInLayer.eSliceType) {
    memcpy (& (pCtx->sFrameCrop), & (pCurDq->sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pSps->sFrameCrop),
            sizeof (SPosOffset)); //confirmed_safe_unsafe_usage
#ifdef LONG_TERM_REF
    pCtx->bParamSetsLostFlag      = false;
#else
    pCtx->bReferenceLostAtT0Flag = false;	// need initialize it due I_SLICE, 6/4/2010
#endif //LONG_TERM_REF
    WelsLog (pCtx, WELS_LOG_INFO,
             "DecodeFrameConstruction()::::output good I frame, %d x %d, crop_left:%d, crop_right:%d, crop_top:%d, crop_bottom:%d.\n",
             kiWidth, kiHeight, pCtx->sFrameCrop.iLeftOffset, pCtx->sFrameCrop.iRightOffset, pCtx->sFrameCrop.iTopOffset,
             pCtx->sFrameCrop.iBottomOffset);
  }

  //////output:::normal path
  ppDst[0]      = pPic->pData[0];
  ppDst[1]      = pPic->pData[1];
  ppDst[2]      = pPic->pData[2];
  *pDstLen     = pPic->iLinesize[0];
  * (pDstLen + 1) = pPic->iLinesize[1];
  *pWidth      = kiWidth;
  *pHeight     = kiHeight;

  pDstInfo->UsrData.sSystemBuffer.iFormat = videoFormatI420;

  pDstInfo->UsrData.sSystemBuffer.iWidth = kiWidth - (pCtx->sFrameCrop.iLeftOffset + pCtx->sFrameCrop.iRightOffset) * 2;
  pDstInfo->UsrData.sSystemBuffer.iHeight = kiHeight - (pCtx->sFrameCrop.iTopOffset + pCtx->sFrameCrop.iBottomOffset) * 2;
  pDstInfo->UsrData.sSystemBuffer.iStride[0] = pPic->iLinesize[0];
  pDstInfo->UsrData.sSystemBuffer.iStride[1] = pPic->iLinesize[1];
  ppDst[0] = ppDst[0] + pCtx->sFrameCrop.iTopOffset * 2 * pPic->iLinesize[0] + pCtx->sFrameCrop.iLeftOffset * 2;
  ppDst[1] = ppDst[1] + pCtx->sFrameCrop.iTopOffset  * pPic->iLinesize[1] + pCtx->sFrameCrop.iLeftOffset;
  ppDst[2] = ppDst[2] + pCtx->sFrameCrop.iTopOffset  * pPic->iLinesize[1] + pCtx->sFrameCrop.iLeftOffset;
  pDstInfo->iBufferStatus = 1;

  return 0;
}

inline bool    CheckSliceNeedReconstruct (int16_t iCurDid, int16_t iCurQid, bool bStoreRefBasePicFlag,
    uint8_t uiDidMax, uint8_t uiLayerDqId, uint8_t uiTargetDqId) {
  return ((iCurDid == uiDidMax) && (iCurQid == BASE_QUALITY_ID) && (bStoreRefBasePicFlag))   // store base
         || (uiLayerDqId == uiTargetDqId); // target layer
}

inline uint8_t GetTargetDqId (uint8_t uiTargetDqId,  SDecodingParam* psParam) {
  uint8_t  uiRequiredDqId = psParam ? psParam->uiTargetDqLayer : (uint8_t)255;

  return WELS_MIN (uiTargetDqId, uiRequiredDqId);
}


inline void    HandleReferenceLostL0 (PWelsDecoderContext pCtx, PNalUnit pCurNal) {
  if (0 == pCurNal->sNalHeaderExt.uiTemporalId) {
    pCtx->bReferenceLostAtT0Flag = true;
  }
#ifndef LONG_TERM_REF
  if (pCtx->bReferenceLostAtT0Flag) {
    ResetParameterSetsState (pCtx);
  }
#endif
  pCtx->iErrorCode |= dsBitstreamError;
}

inline void    HandleReferenceLost (PWelsDecoderContext pCtx, PNalUnit pCurNal) {
  if ((0 == pCurNal->sNalHeaderExt.uiTemporalId) || (1 == pCurNal->sNalHeaderExt.uiTemporalId)) {
    pCtx->bReferenceLostAtT0Flag = true;
  }
#ifndef LONG_TERM_REF
  if (pCtx->bReferenceLostAtT0Flag) {
    ResetParameterSetsState (pCtx);
  }
#endif
  pCtx->iErrorCode |= dsRefLost;
}

inline int32_t  WelsDecodeConstructSlice (PWelsDecoderContext pCtx, PNalUnit pCurNal) {
  int32_t  iRet = WelsTargetSliceConstruction (pCtx);

  if (iRet) {
    HandleReferenceLostL0 (pCtx, pCurNal);
  }

  return iRet;
}

/*
 *	Predeclared function routines ..
 */
int32_t ParseRefPicListReordering (PBitStringAux pBs, PSliceHeader pSh) {
  int32_t iList = 0;
  const ESliceType keSt = pSh->eSliceType;
  PRefPicListReorderSyn pRefPicListReordering = &pSh->pRefPicListReordering;
  PSps pSps = pSh->pSps;
  if (keSt == I_SLICE || keSt == SI_SLICE)
    return ERR_NONE;

  // Common syntaxs for P or B slices: list0, list1 followed if B slices used.
  do {
    pRefPicListReordering->bRefPicListReorderingFlag[iList]	= !!BsGetOneBit (pBs);

    if (pRefPicListReordering->bRefPicListReorderingFlag[iList]) {
      int32_t iIdx = 0;
      do {
        const uint32_t kuiIdc = BsGetUe (pBs);

        //Fixed the referrence list reordering crash issue.(fault kIdc value > 3 case)---
        if ((iIdx >= MAX_REF_PIC_COUNT) || (kuiIdc > 3)) {
          return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_REF_REORDERING);
        }
        pRefPicListReordering->sReorderingSyn[iList][iIdx].uiReorderingOfPicNumsIdc	= kuiIdc;
        if (kuiIdc == 3)
          break;

        if (iIdx >= pSh->uiRefCount[iList] || iIdx >= MAX_REF_PIC_COUNT)
          return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_REF_REORDERING);

        if (kuiIdc == 0 || kuiIdc == 1) {
          // abs_diff_pic_num_minus1 should be in range 0 to MaxPicNum-1, MaxPicNum is derived as
          // 2^(4+log2_max_frame_num_minus4)
          uint32_t uiTmp = BsGetUe (pBs);
          WELS_CHECK_SE_UPPER_ERROR_NOLOG (uiTmp, (1 << pSps->uiLog2MaxFrameNum), "abs_diff_pic_num_minus1",
                                           GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_REF_REORDERING));
          pRefPicListReordering->sReorderingSyn[iList][iIdx].uiAbsDiffPicNumMinus1 = uiTmp;	// uiAbsDiffPicNumMinus1
        } else if (kuiIdc == 2) {
          pRefPicListReordering->sReorderingSyn[iList][iIdx].uiLongTermPicNum = BsGetUe (pBs);
        }

        ++ iIdx;
      } while (true);
    }
    if (keSt != B_SLICE)
      break;
    ++ iList;
  } while (iList < LIST_A);

  return ERR_NONE;
}

int32_t ParseDecRefPicMarking (PWelsDecoderContext pCtx, PBitStringAux pBs, PSliceHeader pSh, PSps pSps,
                               const bool kbIdrFlag) {
  PRefPicMarking const kpRefMarking = &pSh->sRefMarking;
  PRefPic pRefPic = &pCtx->sRefPic;
  if (kbIdrFlag) {
    kpRefMarking->bNoOutputOfPriorPicsFlag	= !!BsGetOneBit (pBs);
    kpRefMarking->bLongTermRefFlag			= !!BsGetOneBit (pBs);
  } else {
    kpRefMarking->bAdaptiveRefPicMarkingModeFlag	= !!BsGetOneBit (pBs);
    if (kpRefMarking->bAdaptiveRefPicMarkingModeFlag) {
      int32_t iIdx = 0;
      do {
        const int32_t kiMmco = BsGetUe (pBs);

        kpRefMarking->sMmcoRef[iIdx].uiMmcoType = kiMmco;
        if (kiMmco == MMCO_END)
          break;

        if (kiMmco == MMCO_SHORT2UNUSED || kiMmco == MMCO_SHORT2LONG) {
          kpRefMarking->sMmcoRef[iIdx].iDiffOfPicNum = 1 + BsGetUe (pBs);
          kpRefMarking->sMmcoRef[iIdx].iShortFrameNum = (pSh->iFrameNum - kpRefMarking->sMmcoRef[iIdx].iDiffOfPicNum) & ((
                1 << pSps->uiLog2MaxFrameNum) - 1);
        } else if (kiMmco == MMCO_LONG2UNUSED)
          kpRefMarking->sMmcoRef[iIdx].uiLongTermPicNum = BsGetUe (pBs);

        if (kiMmco == MMCO_SHORT2LONG || kiMmco == MMCO_LONG) {
          kpRefMarking->sMmcoRef[iIdx].iLongTermFrameIdx = BsGetUe (pBs);
        } else if (kiMmco == MMCO_SET_MAX_LONG)
          kpRefMarking->sMmcoRef[iIdx].iMaxLongTermFrameIdx = -1 + BsGetUe (pBs);
        ++ iIdx;

      } while (iIdx < MAX_MMCO_COUNT);
    }
  }

  return ERR_NONE;
}

bool FillDefaultSliceHeaderExt (PSliceHeaderExt pShExt, PNalUnitHeaderExt pNalExt) {
  if (pShExt == NULL || pNalExt == NULL)
    return false;

  if (pNalExt->iNoInterLayerPredFlag || pNalExt->uiQualityId > 0)
    pShExt->bBasePredWeightTableFlag	= false;
  else
    pShExt->bBasePredWeightTableFlag	= true;
  pShExt->uiRefLayerDqId = (uint8_t) - 1;
  pShExt->uiDisableInterLayerDeblockingFilterIdc	= 0;
  pShExt->iInterLayerSliceAlphaC0Offset			= 0;
  pShExt->iInterLayerSliceBetaOffset				= 0;
  pShExt->bConstrainedIntraResamplingFlag			= false;
  pShExt->uiRefLayerChromaPhaseXPlus1Flag			= 0;
  pShExt->uiRefLayerChromaPhaseYPlus1				= 1;
  //memset(&pShExt->sScaledRefLayer, 0, sizeof(SPosOffset));

  pShExt->iScaledRefLayerPicWidthInSampleLuma	= pShExt->sSliceHeader.iMbWidth << 4;
  pShExt->iScaledRefLayerPicHeightInSampleLuma	= pShExt->sSliceHeader.iMbHeight << 4;

  pShExt->bSliceSkipFlag	= false;
  pShExt->bAdaptiveBaseModeFlag	= false;
  pShExt->bDefaultBaseModeFlag	= false;
  pShExt->bAdaptiveMotionPredFlag	= false;
  pShExt->bDefaultMotionPredFlag	= false;
  pShExt->bAdaptiveResidualPredFlag	= false;
  pShExt->bDefaultResidualPredFlag	= false;
  pShExt->bTCoeffLevelPredFlag		= false;
  pShExt->uiScanIdxStart				= 0;
  pShExt->uiScanIdxEnd				= 15;

  return true;
}

/*
 * WelsInitMemory
 * Memory request for new introduced data
 * Especially for:
 * rbsp_au_buffer, cur_dq_layer_ptr and ref_dq_layer_ptr in MB info cache.
 * return:
 *	0 - success; otherwise returned error_no defined in error_no.h.
*/
int32_t WelsInitMemory (PWelsDecoderContext pCtx) {
  if (pCtx == NULL) {
    return ERR_INFO_INVALID_PTR;
  }

  if (MemInitNalList (&pCtx->pAccessUnitList, MAX_NAL_UNIT_NUM_IN_AU) != 0)
    return ERR_INFO_OUT_OF_MEMORY;

  if ((pCtx->sRawData.pHead = static_cast<uint8_t*> (WelsMalloc (MAX_ACCESS_UNIT_CAPACITY,
                              "pCtx->sRawData->pHead"))) == NULL) {
    return ERR_INFO_OUT_OF_MEMORY;
  }
  pCtx->sRawData.pStartPos               =
    pCtx->sRawData.pCurPos                 = pCtx->sRawData.pHead;
  pCtx->sRawData.pEnd                     = pCtx->sRawData.pHead + MAX_ACCESS_UNIT_CAPACITY;

  pCtx->uiTargetDqId			= (uint8_t) - 1;
  pCtx->bEndOfStreamFlag	= false;
  pCtx->iImgWidthInPixel	= 0;
  pCtx->iImgHeightInPixel	= 0;

  return ERR_NONE;
}

/*
 * WelsFreeMemory
 * Free memory introduced in WelsInitMemory at destruction of decoder.
 *
 */
void WelsFreeMemory (PWelsDecoderContext pCtx) {
  if (pCtx == NULL)
    return;

  if (NULL != pCtx->pParam) {
    WelsFree (pCtx->pParam, "pCtx->pParam");

    pCtx->pParam = NULL;
  }

  MemFreeNalList (&pCtx->pAccessUnitList);

  if (pCtx->sRawData.pHead) {
    WelsFree (pCtx->sRawData.pHead, "pCtx->sRawData->pHead");
  }
  pCtx->sRawData.pHead                = NULL;
  pCtx->sRawData.pEnd                 = NULL;
  pCtx->sRawData.pStartPos	        = NULL;
  pCtx->sRawData.pCurPos             = NULL;
}

/*
 *	DecodeNalHeaderExt
 *	Trigger condition: NAL_UNIT_TYPE = NAL_UNIT_PREFIX or NAL_UNIT_CODED_SLICE_EXT
 *	Parameter:
 *	pNal:	target NALUnit ptr
 *	pSrc:	NAL Unit bitstream
 */
void DecodeNalHeaderExt (PNalUnit pNal, uint8_t* pSrc) {
  PNalUnitHeaderExt pHeaderExt = &pNal->sNalHeaderExt;

  uint8_t uiCurByte = *pSrc;
  pHeaderExt->bIdrFlag				 = !! (uiCurByte & 0x40);
  pHeaderExt->uiPriorityId			 = uiCurByte & 0x3F;

  uiCurByte = * (++pSrc);
  pHeaderExt->iNoInterLayerPredFlag = uiCurByte >> 7;
  pHeaderExt->uiDependencyId			 = (uiCurByte & 0x70) >> 4;
  pHeaderExt->uiQualityId				 = uiCurByte & 0x0F;
  uiCurByte = * (++pSrc);
  pHeaderExt->uiTemporalId			 = uiCurByte >> 5;
  pHeaderExt->bUseRefBasePicFlag	     = !! (uiCurByte & 0x10);
  pHeaderExt->bDiscardableFlag		 = !! (uiCurByte & 0x08);
  pHeaderExt->bOutputFlag				 = !! (uiCurByte & 0x04);
  pHeaderExt->uiReservedThree2Bits	 = uiCurByte & 0x03;
  pHeaderExt->uiLayerDqId				 = (pHeaderExt->uiDependencyId << 4) | pHeaderExt->uiQualityId;
}


#ifdef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
int32_t CheckPpsId (PWelsDecoderContext pCtx, PPps* ppPps, uint32_t uiPpsId) {
  PPps pPpsList = pCtx->sPpsBuffer;
  int32_t iPpsNum = pCtx->iPpsTotalNum;
  int32_t i = 0;

  if (iPpsNum <= 0) {
    pCtx->iErrorCode |= dsNoParamSets;

    WelsLog (pCtx, WELS_LOG_WARNING, "CheckPpsId():::::PPS list is empty...NO PPS!!!\n");
    return dsNoParamSets;
  }

  while (i < iPpsNum) {
    if (uiPpsId == pPpsList[i].iPpsId) {
      *ppPps = &pPpsList[i];
      break;
    } else {
      ++i;
    }
  }

  if (i == iPpsNum) {
    pCtx->iErrorCode |= dsNoParamSets;

    WelsLog (pCtx, WELS_LOG_WARNING, "CheckPpsId()::::::CAN NOT find the matching from the PPS List.  iPpsId:%d\n",
             uiPpsId);
    return dsNoParamSets;
  }

  return 0;
}

int32_t CheckSpsId (PWelsDecoderContext pCtx, PSubsetSps* ppSubsetSps, PSps* ppSps, int32_t iSpsId,
                    bool bExtensionFlag) {
  PSps pSpsList = pCtx->sSpsBuffer;
  PSubsetSps pSubspsList = pCtx->sSubsetSpsBuffer;

  int32_t iSpsNum    = pCtx->iSpsTotalNum;
  int32_t iSubspsNum = pCtx->iSubspsTotalNum;
  int32_t i = 0;

  if (bExtensionFlag) {
    if (iSubspsNum <= 0) {
      pCtx->iErrorCode |= dsNoParamSets;

      WelsLog (pCtx, WELS_LOG_WARNING, "CheckSpsId()::::SUBSPS list is empty....NO SUBSPS\n");
      return dsNoParamSets;
    }
    while (i < iSubspsNum) {
      if (iSpsId == pSubspsList[i].sSps.iSpsId) {
        *ppSubsetSps = &pSubspsList[i];
        *ppSps       = &pSubspsList[i].sSps;
        break;
      } else {
        ++i;
      }
    }
    if (i == iSubspsNum) {
      pCtx->iErrorCode |= dsNoParamSets;

      WelsLog (pCtx, WELS_LOG_WARNING, "CheckSpsId()::::::CAN NOT find the matching from the SUBSPS List.  iSpsId:%d\n",
               iSpsId);
      return dsNoParamSets;
    }
  } else {
    if (iSpsNum <= 0) {
      pCtx->iErrorCode |= dsNoParamSets;

      WelsLog (pCtx, WELS_LOG_WARNING, "CheckSpsId()::::SPS list is empty....NO SPS\n");
      return dsNoParamSets;
    }
    while (i < iSpsNum) {
      if (iSpsId == pSpsList[i].iSpsId) {
        *ppSubsetSps = NULL;
        *ppSps       = &pSpsList[i];
        break;
      } else {
        ++i;
      }
    }
    if (i == iSpsNum) {
      pCtx->iErrorCode |= dsNoParamSets;

      WelsLog (pCtx, WELS_LOG_WARNING, "CheckSpsId()::::::CAN NOT find the matching from the SPS List.  iSpsId:%d\n", iSpsId);
      return dsNoParamSets;
    }
  }

  return 0;
}

#endif

#define SLICE_HEADER_IDR_PIC_ID_MAX 65535
#define SLICE_HEADER_REDUNDANT_PIC_CNT_MAX 127
#define SLICE_HEADER_ALPHAC0_BETA_OFFSET_MIN -12
#define SLICE_HEADER_ALPHAC0_BETA_OFFSET_MAX 12
#define SLICE_HEADER_INTER_LAYER_ALPHAC0_BETA_OFFSET_MIN -12
#define SLICE_HEADER_INTER_LAYER_ALPHAC0_BETA_OFFSET_MAX 12
/*
 *	decode_slice_header_avc
 *	Parse slice header of bitstream in avc for storing data structure
 */
int32_t ParseSliceHeaderSyntaxs (PWelsDecoderContext pCtx, PBitStringAux pBs, const bool kbExtensionFlag) {
  PNalUnit const kpCurNal				= pCtx->pAccessUnitList->pNalUnitsList[pCtx->pAccessUnitList->uiAvailUnitsNum - 1];

  PNalUnitHeaderExt pNalHeaderExt	= NULL;
  PSliceHeader pSliceHead			= NULL;
  PSliceHeaderExt pSliceHeadExt	= NULL;
  PSubsetSps pSubsetSps				= NULL;
  PSps pSps							= NULL;
  PPps pPps							= NULL;
  ENalUnitType eNalType				= static_cast<ENalUnitType> (0);
  int32_t iPpsId						= 0;
  int32_t iRet						= ERR_NONE;
  uint8_t uiSliceType				= 0;
  uint8_t uiQualityId					= BASE_QUALITY_ID;
  uint32_t uiTmp;
  bool	bIdrFlag					= false;
  bool	bSgChangeCycleInvolved	= false;	// involved slice group change cycle ?

  if (kpCurNal == NULL) {
    return ERR_INFO_OUT_OF_MEMORY;
  }

  pNalHeaderExt	= &kpCurNal->sNalHeaderExt;
  pSliceHead		= &kpCurNal->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader;
  eNalType		= pNalHeaderExt->sNalUnitHeader.eNalUnitType;

  pSliceHeadExt	= &kpCurNal->sNalData.sVclNal.sSliceHeaderExt;

  if (pSliceHeadExt) {
    SRefBasePicMarking sBaseMarking;
    const bool kbStoreRefBaseFlag = pSliceHeadExt->bStoreRefBasePicFlag;
    memcpy (&sBaseMarking, &pSliceHeadExt->sRefBasePicMarking, sizeof (SRefBasePicMarking)); //confirmed_safe_unsafe_usage
    memset (pSliceHeadExt, 0, sizeof (SSliceHeaderExt));
    pSliceHeadExt->bStoreRefBasePicFlag	= kbStoreRefBaseFlag;
    memcpy (&pSliceHeadExt->sRefBasePicMarking, &sBaseMarking, sizeof (SRefBasePicMarking)); //confirmed_safe_unsafe_usage
  }

  kpCurNal->sNalData.sVclNal.bSliceHeaderExtFlag	= kbExtensionFlag;

  // first_mb_in_slice
  pSliceHead->iFirstMbInSlice	= BsGetUe (pBs);

  uiSliceType = BsGetUe (pBs);
  if (uiSliceType > 9) {
    WelsLog (pCtx, WELS_LOG_WARNING, "slice type too large (%d) at first_mb(%d)\n", uiSliceType,
             pSliceHead->iFirstMbInSlice);
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_SLICE_TYPE);
  }
  if (uiSliceType > 4)
    uiSliceType -= 5;

  if ((eNalType == NAL_UNIT_CODED_SLICE_IDR) && (uiSliceType != 2)) {
    WelsLog (pCtx, WELS_LOG_WARNING, "Invalid slice type(%d) in IDR picture. \n", uiSliceType);
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_SLICE_TYPE);
  }

  if (kbExtensionFlag) {
    if (uiSliceType > 2) {
      WelsLog (pCtx, WELS_LOG_WARNING, "Invalid slice type(%d).\n", uiSliceType);
      return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_SLICE_TYPE);
    }
  }

  pSliceHead->eSliceType	= static_cast <ESliceType> (uiSliceType);

  iPpsId = BsGetUe (pBs);

  if (iPpsId >= MAX_PPS_COUNT) {
    WelsLog (pCtx, WELS_LOG_WARNING, "iPpsId out of range\n");
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_PPS_ID_OVERFLOW);
  }

#ifdef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
  if (CheckPpsId (pCtx, &pPps, iPpsId)) {
    return dsNoParamSets;
  }
#else
  pPps    = &pCtx->sPpsBuffer[iPpsId];
#endif //MOSAIC_AVOID_BASED_ON_SPS_PPS_ID

  if (pPps->uiNumSliceGroups == 0) {
    WelsLog (pCtx, WELS_LOG_WARNING, "non existing PPS referenced\n");
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_NO_PARAM_SETS);
  }

  if (pPps->iSpsId >= MAX_SPS_COUNT) {
    WelsLog (pCtx, WELS_LOG_WARNING, "iSpsId out of range\n");
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_SPS_ID_OVERFLOW);
  }


#ifdef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
  if (CheckSpsId (pCtx, &pSubsetSps, &pSps, pPps->iSpsId, kExtensionFlag)) {
    return dsNoParamSets;
  }
#else
  if (kbExtensionFlag) {
    pSubsetSps	= &pCtx->sSubsetSpsBuffer[pPps->iSpsId];
    pSps		= &pSubsetSps->sSps;
  } else {
    pSps		= &pCtx->sSpsBuffer[pPps->iSpsId];
  }
  pCtx->pSps			= pSps;
#endif //MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
  pSliceHead->iPpsId = iPpsId;
  pSliceHead->iSpsId = pPps->iSpsId;
  pSliceHead->pPps   = pPps;
  pSliceHead->pSps   = pSps;

  pSliceHeadExt->pSubsetSps = pSubsetSps;

  bIdrFlag = (!kbExtensionFlag && eNalType == NAL_UNIT_CODED_SLICE_IDR) || (kbExtensionFlag && pNalHeaderExt->bIdrFlag);

  if (pSps->uiLog2MaxFrameNum == 0) {
    WelsLog (pCtx, WELS_LOG_WARNING, "non existing SPS referenced\n");
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_NO_PARAM_SETS);
  }
  // check first_mb_in_slice
  WELS_CHECK_SE_UPPER_ERROR ((uint32_t) (pSliceHead->iFirstMbInSlice), pSps->uiTotalMbCount, "first_mb_in_slice",
                             GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_FIRST_MB_IN_SLICE));
  pSliceHead->iFrameNum = BsGetBits (pBs, pSps->uiLog2MaxFrameNum);

  pSliceHead->bFieldPicFlag		= false;
  pSliceHead->bBottomFiledFlag	= false;
  if (!pSps->bFrameMbsOnlyFlag) {
    WelsLog (pCtx, WELS_LOG_WARNING, "ParseSliceHeaderSyntaxs(): frame_mbs_only_flag = %d not supported. \n",
             pSps->bFrameMbsOnlyFlag);
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_MBAFF);
  }
  pSliceHead->iMbWidth	= pSps->iMbWidth;
  pSliceHead->iMbHeight	= pSps->iMbHeight / (1 + pSliceHead->bFieldPicFlag);

  if (bIdrFlag) {
    if (pSliceHead->iFrameNum != 0) {
      WelsLog (pCtx, WELS_LOG_WARNING, "ParseSliceHeaderSyntaxs(), invaild frame number: %d due to IDR frame introduced!\n",
               pSliceHead->iFrameNum);
      return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_FRAME_NUM);
    }
    uiTmp = BsGetUe (pBs);
    // standard 7.4.3 idr_pic_id should be in range 0 to 65535, inclusive.
    WELS_CHECK_SE_UPPER_ERROR (uiTmp, SLICE_HEADER_IDR_PIC_ID_MAX, "idr_pic_id", GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER,
                               ERR_INFO_INVALID_IDR_PIC_ID));
    pSliceHead->uiIdrPicId	= uiTmp; /* uiIdrPicId */
#ifdef LONG_TERM_REF
    pCtx->uiCurIdrPicId      = pSliceHead->uiIdrPicId;
#endif
  }

  pSliceHead->iDeltaPicOrderCntBottom	= 0;
  pSliceHead->iDeltaPicOrderCnt[0]		=
    pSliceHead->iDeltaPicOrderCnt[1]		= 0;
  if (pSps->uiPocType == 0) {
    pSliceHead->iPicOrderCntLsb	= BsGetBits (pBs, pSps->iLog2MaxPocLsb);
    if (pPps->bPicOrderPresentFlag && !pSliceHead->bFieldPicFlag) {
      pSliceHead->iDeltaPicOrderCntBottom	= BsGetSe (pBs);
    }
  } else if (pSps->uiPocType == 1 && !pSps->bDeltaPicOrderAlwaysZeroFlag) {
    pSliceHead->iDeltaPicOrderCnt[0]	= BsGetSe (pBs);
    if (pPps->bPicOrderPresentFlag && !pSliceHead->bFieldPicFlag)
      pSliceHead->iDeltaPicOrderCnt[1] = BsGetSe (pBs);
  }

  pSliceHead->iRedundantPicCnt	= 0;
  if (pPps->bRedundantPicCntPresentFlag) {
    uiTmp = BsGetUe (pBs);
    // standard section 7.4.3, redundant_pic_cnt should be in range 0 to 127, inclusive.
    WELS_CHECK_SE_UPPER_ERROR (uiTmp, SLICE_HEADER_REDUNDANT_PIC_CNT_MAX, "redundant_pic_cnt",
                               GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_REDUNDANT_PIC_CNT));
    pSliceHead->iRedundantPicCnt = uiTmp;
  }

  //set defaults, might be overriden a few line later
  pSliceHead->uiRefCount[0]	= pPps->uiNumRefIdxL0Active;
  pSliceHead->uiRefCount[1]	= pPps->uiNumRefIdxL1Active;
  if (kbExtensionFlag) {
    uiQualityId = pNalHeaderExt->uiQualityId;
  } else if (uiSliceType == P_SLICE || uiSliceType == SP_SLICE || uiSliceType == B_SLICE) {
    const bool kbBipredFlag = (B_SLICE == uiSliceType);
    if (kbBipredFlag) {
      WelsLog (pCtx, WELS_LOG_WARNING, "ParseSliceHeaderSyntaxs(): kbBipredFlag = 1 not supported.\n");
      return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_BIPRED);
    }
    pSliceHead->bNumRefIdxActiveOverrideFlag	= !!BsGetOneBit (pBs);
    if (pSliceHead->bNumRefIdxActiveOverrideFlag) {
      pSliceHead->uiRefCount[0]	= 1 + BsGetUe (pBs);
    }
  }

  if (pSliceHead->uiRefCount[0] > MAX_REF_PIC_COUNT || pSliceHead->uiRefCount[1] > MAX_REF_PIC_COUNT) {
    WelsLog (pCtx, WELS_LOG_WARNING, "reference overflow\n");
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_REF_COUNT_OVERFLOW);
  }

  if (BASE_QUALITY_ID == uiQualityId) {
    iRet = ParseRefPicListReordering (pBs, pSliceHead);
    if (iRet != ERR_NONE) {
      WelsLog (pCtx, WELS_LOG_WARNING, "invalid ref pPic list reordering syntaxs!\n");
      return iRet;
    }

    if (kbExtensionFlag) {
      if (pNalHeaderExt->iNoInterLayerPredFlag || pNalHeaderExt->uiQualityId > 0)
        pSliceHeadExt->bBasePredWeightTableFlag	= false;
      else
        pSliceHeadExt->bBasePredWeightTableFlag	= true;
    }

    if (kpCurNal->sNalHeaderExt.sNalUnitHeader.uiNalRefIdc != 0) {
      iRet = ParseDecRefPicMarking (pCtx, pBs, pSliceHead, pSps, bIdrFlag);
      if (iRet != ERR_NONE) {
        return iRet;
      }

      if (kbExtensionFlag && !pSubsetSps->sSpsSvcExt.bSliceHeaderRestrictionFlag) {
        pSliceHeadExt->bStoreRefBasePicFlag	= !!BsGetOneBit (pBs);
        if ((pNalHeaderExt->bUseRefBasePicFlag || pSliceHeadExt->bStoreRefBasePicFlag) && !bIdrFlag) {
          WelsLog (pCtx, WELS_LOG_WARNING,
                   "ParseSliceHeaderSyntaxs(): bUseRefBasePicFlag or bStoreRefBasePicFlag = 1 not supported.\n");
          return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_ILP);
        }
      }
    }
  }

  if (pPps->bEntropyCodingModeFlag) {
    WelsLog (pCtx, WELS_LOG_WARNING, "ParseSliceHeaderSyntaxs(): CABAC in Enhancement layer not supported.\n");
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_CABAC_EL);
  }

  pSliceHead->iSliceQpDelta	= BsGetSe (pBs);
  pSliceHead->iSliceQp		= pPps->iPicInitQp + pSliceHead->iSliceQpDelta;
  if (pSliceHead->iSliceQp < 0 || pSliceHead->iSliceQp > 51) {
    WelsLog (pCtx, WELS_LOG_WARNING, "QP %d out of range\n", pSliceHead->iSliceQp);
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_QP);
  }

  //FIXME qscale / qp ... stuff
  if (!kbExtensionFlag) {
    if (uiSliceType == SP_SLICE || uiSliceType == SI_SLICE) {
      WelsLog (pCtx, WELS_LOG_WARNING, "SP/SI not supported\n");
      return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_SPSI);
    }
  }

  pSliceHead->uiDisableDeblockingFilterIdc	= 0;
  pSliceHead->iSliceAlphaC0Offset			= 0;
  pSliceHead->iSliceBetaOffset				= 0;
  if (pPps->bDeblockingFilterControlPresentFlag) {
    pSliceHead->uiDisableDeblockingFilterIdc	= BsGetUe (pBs);
    //refer to JVT-X201wcm1.doc G.7.4.3.4--2010.4.20
    if (pSliceHead->uiDisableDeblockingFilterIdc > 6) {
      WelsLog (pCtx, WELS_LOG_WARNING, "disable_deblock_filter_idc (%d) out of range [0, 6]\n",
               pSliceHead->uiDisableDeblockingFilterIdc);
      return ERR_INFO_INVALID_DBLOCKING_IDC;
    }
    if (pSliceHead->uiDisableDeblockingFilterIdc != 1) {
      pSliceHead->iSliceAlphaC0Offset	= BsGetSe (pBs) * 2;	// slice_alpha_c0_offset_div2
      WELS_CHECK_SE_BOTH_ERROR (pSliceHead->iSliceAlphaC0Offset, SLICE_HEADER_ALPHAC0_BETA_OFFSET_MIN,
                                SLICE_HEADER_ALPHAC0_BETA_OFFSET_MAX, "slice_alpha_c0_offset_div2 * 2", GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER,
                                    ERR_INFO_INVALID_SLICE_ALPHA_C0_OFFSET_DIV2));
      pSliceHead->iSliceBetaOffset		= BsGetSe (pBs) * 2;	// iSliceBetaOffset
      WELS_CHECK_SE_BOTH_ERROR (pSliceHead->iSliceBetaOffset, SLICE_HEADER_ALPHAC0_BETA_OFFSET_MIN,
                                SLICE_HEADER_ALPHAC0_BETA_OFFSET_MAX, "slice_beta_offset_div2 * 2", GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER,
                                    ERR_INFO_INVALID_SLICE_BETA_OFFSET_DIV2));
    }
  }

  bSgChangeCycleInvolved	= (pPps->uiNumSliceGroups > 1 && pPps->uiSliceGroupMapType >= 3
                             && pPps->uiSliceGroupMapType <= 5);
  if (kbExtensionFlag && bSgChangeCycleInvolved)
    bSgChangeCycleInvolved = (bSgChangeCycleInvolved && (uiQualityId == BASE_QUALITY_ID));
  if (bSgChangeCycleInvolved) {
    if (pPps->uiSliceGroupChangeRate > 0) {
      const int32_t kiNumBits = (int32_t)WELS_CEIL (log (static_cast<double> (1 + pPps->uiPicSizeInMapUnits /
                                pPps->uiSliceGroupChangeRate)));
      pSliceHead->iSliceGroupChangeCycle	= BsGetBits (pBs, kiNumBits);	// For FMO extra types
    } else
      pSliceHead->iSliceGroupChangeCycle	= 0;
  }

  if (!kbExtensionFlag) {
    FillDefaultSliceHeaderExt (pSliceHeadExt, pNalHeaderExt);
  } else {
    /* Extra syntax elements newly introduced */
    pSliceHeadExt->pSubsetSps	= pSubsetSps;

    if (!pNalHeaderExt->iNoInterLayerPredFlag && BASE_QUALITY_ID == uiQualityId) {
      //the following should be deleted for CODE_CLEAN
      pSliceHeadExt->uiRefLayerDqId	= BsGetUe (pBs);
      if (pSubsetSps->sSpsSvcExt.bInterLayerDeblockingFilterCtrlPresentFlag) {
        pSliceHeadExt->uiDisableInterLayerDeblockingFilterIdc	= BsGetUe (pBs);
        //refer to JVT-X201wcm1.doc G.7.4.3.4--2010.4.20
        if (pSliceHeadExt->uiDisableInterLayerDeblockingFilterIdc > 6) {
          WelsLog (pCtx, WELS_LOG_WARNING, "disable_inter_layer_deblock_filter_idc (%d) out of range [0, 6]\n",
                   pSliceHeadExt->uiDisableInterLayerDeblockingFilterIdc);
          return ERR_INFO_INVALID_DBLOCKING_IDC;
        }
        if (pSliceHeadExt->uiDisableInterLayerDeblockingFilterIdc != 1) {
          pSliceHeadExt->iInterLayerSliceAlphaC0Offset	= BsGetSe (pBs) << 1;
          WELS_CHECK_SE_BOTH_ERROR (pSliceHeadExt->iInterLayerSliceAlphaC0Offset,
                                    SLICE_HEADER_INTER_LAYER_ALPHAC0_BETA_OFFSET_MIN, SLICE_HEADER_INTER_LAYER_ALPHAC0_BETA_OFFSET_MAX,
                                    "inter_layer_alpha_c0_offset_div2 * 2", GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER,
                                        ERR_INFO_INVALID_SLICE_ALPHA_C0_OFFSET_DIV2));
          pSliceHeadExt->iInterLayerSliceBetaOffset		= BsGetSe (pBs) << 1;
          WELS_CHECK_SE_BOTH_ERROR (pSliceHeadExt->iInterLayerSliceBetaOffset, SLICE_HEADER_INTER_LAYER_ALPHAC0_BETA_OFFSET_MIN,
                                    SLICE_HEADER_INTER_LAYER_ALPHAC0_BETA_OFFSET_MAX, "inter_layer_slice_beta_offset_div2 * 2",
                                    GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_INVALID_SLICE_BETA_OFFSET_DIV2));
        }
      }

      pSliceHeadExt->uiRefLayerChromaPhaseXPlus1Flag	= pSubsetSps->sSpsSvcExt.uiSeqRefLayerChromaPhaseXPlus1Flag;
      pSliceHeadExt->uiRefLayerChromaPhaseYPlus1		= pSubsetSps->sSpsSvcExt.uiSeqRefLayerChromaPhaseYPlus1;

      pSliceHeadExt->bConstrainedIntraResamplingFlag	= !!BsGetOneBit (pBs);

      {
        SPosOffset pos;
        pos.iLeftOffset	= pSubsetSps->sSpsSvcExt.sSeqScaledRefLayer.iLeftOffset;
        pos.iTopOffset	= pSubsetSps->sSpsSvcExt.sSeqScaledRefLayer.iTopOffset * (2 - pSps->bFrameMbsOnlyFlag);
        pos.iRightOffset = pSubsetSps->sSpsSvcExt.sSeqScaledRefLayer.iRightOffset;
        pos.iBottomOffset = pSubsetSps->sSpsSvcExt.sSeqScaledRefLayer.iBottomOffset * (2 - pSps->bFrameMbsOnlyFlag);
        //memcpy(&pSliceHeadExt->sScaledRefLayer, &pos, sizeof(SPosOffset));//confirmed_safe_unsafe_usage
        pSliceHeadExt->iScaledRefLayerPicWidthInSampleLuma	= (pSliceHead->iMbWidth << 4) - (pos.iLeftOffset + pos.iRightOffset);
        pSliceHeadExt->iScaledRefLayerPicHeightInSampleLuma	= (pSliceHead->iMbHeight << 4) -
            (pos.iTopOffset + pos.iBottomOffset) / (1 + pSliceHead->bFieldPicFlag);
      }
    } else if (uiQualityId > BASE_QUALITY_ID) {
      WelsLog (pCtx, WELS_LOG_WARNING, "MGS not supported.\n");
      return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_MGS);
    } else {
      pSliceHeadExt->uiRefLayerDqId	= (uint8_t) - 1;
    }

    pSliceHeadExt->bSliceSkipFlag	= false;
    pSliceHeadExt->bAdaptiveBaseModeFlag	= false;
    pSliceHeadExt->bDefaultBaseModeFlag	= false;
    pSliceHeadExt->bAdaptiveMotionPredFlag	= false;
    pSliceHeadExt->bDefaultMotionPredFlag	= false;
    pSliceHeadExt->bAdaptiveResidualPredFlag	= false;
    pSliceHeadExt->bDefaultResidualPredFlag	= false;
    if (pNalHeaderExt->iNoInterLayerPredFlag)
      pSliceHeadExt->bTCoeffLevelPredFlag	= false;
    else
      pSliceHeadExt->bTCoeffLevelPredFlag	= pSubsetSps->sSpsSvcExt.bSeqTCoeffLevelPredFlag;

    if (!pNalHeaderExt->iNoInterLayerPredFlag) {
      pSliceHeadExt->bSliceSkipFlag	= !!BsGetOneBit (pBs);
      if (pSliceHeadExt->bSliceSkipFlag) {
        pSliceHeadExt->uiNumMbsInSlice	= 1 + BsGetUe (pBs);
      } else {
        pSliceHeadExt->bAdaptiveBaseModeFlag	= !!BsGetOneBit (pBs);
        if (!pSliceHeadExt->bAdaptiveBaseModeFlag) {
          pSliceHeadExt->bDefaultBaseModeFlag	= !!BsGetOneBit (pBs);
        }
        if (!pSliceHeadExt->bDefaultBaseModeFlag) {
          pSliceHeadExt->bAdaptiveMotionPredFlag	= !!BsGetOneBit (pBs);
          if (!pSliceHeadExt->bAdaptiveMotionPredFlag)
            pSliceHeadExt->bDefaultMotionPredFlag	= !!BsGetOneBit (pBs);
        }

        pSliceHeadExt->bAdaptiveResidualPredFlag	= !!BsGetOneBit (pBs);
        if (!pSliceHeadExt->bAdaptiveResidualPredFlag) {
          pSliceHeadExt->bDefaultResidualPredFlag = !!BsGetOneBit (pBs);
        }
      }
      if (pSubsetSps->sSpsSvcExt.bAdaptiveTCoeffLevelPredFlag)
        pSliceHeadExt->bTCoeffLevelPredFlag	= !!BsGetOneBit (pBs);
    }

    if (!pSubsetSps->sSpsSvcExt.bSliceHeaderRestrictionFlag) {
      pSliceHeadExt->uiScanIdxStart	= BsGetBits (pBs, 4);
      pSliceHeadExt->uiScanIdxEnd	= BsGetBits (pBs, 4);
      if (pSliceHeadExt->uiScanIdxStart != 0 || pSliceHeadExt->uiScanIdxEnd != 15) {
        WelsLog (pCtx, WELS_LOG_WARNING, "uiScanIdxStart (%d) != 0 and uiScanIdxEnd (%d) !=15 not supported here\n",
                 pSliceHeadExt->uiScanIdxStart, pSliceHeadExt->uiScanIdxEnd);
        return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_MGS);
      }
    } else {
      pSliceHeadExt->uiScanIdxStart	= 0;
      pSliceHeadExt->uiScanIdxEnd	= 15;
    }
  }

  return ERR_NONE;
}

/*
 *	Copy relative syntax elements of NALUnitHeaderExt, sRefPicBaseMarking and bStoreRefBasePicFlag in prefix nal unit.
 *	pSrc:	mark as decoded prefix NAL
 *	ppDst:	succeeded VCL NAL based AVC (I/P Slice)
 */
bool PrefetchNalHeaderExtSyntax (PWelsDecoderContext pCtx, PNalUnit const kppDst, PNalUnit const kpSrc) {
  PNalUnitHeaderExt pNalHdrExtD	= NULL, pNalHdrExtS = NULL;
  PSliceHeaderExt pShExtD = NULL;
  PPrefixNalUnit pPrefixS = NULL;
  PSps pSps = NULL;
  int32_t iIdx = 0;

  if (kppDst == NULL || kpSrc == NULL)
    return false;

  pNalHdrExtD	= &kppDst->sNalHeaderExt;
  pNalHdrExtS	= &kpSrc->sNalHeaderExt;
  pShExtD		= &kppDst->sNalData.sVclNal.sSliceHeaderExt;
  pPrefixS		= &kpSrc->sNalData.sPrefixNal;
  pSps			= &pCtx->sSpsBuffer[pCtx->sPpsBuffer[pShExtD->sSliceHeader.iPpsId].iSpsId];

  pNalHdrExtD->uiDependencyId	    = pNalHdrExtS->uiDependencyId;
  pNalHdrExtD->uiQualityId		= pNalHdrExtS->uiQualityId;
  pNalHdrExtD->uiTemporalId		= pNalHdrExtS->uiTemporalId;
  pNalHdrExtD->uiPriorityId		= pNalHdrExtS->uiPriorityId;
  pNalHdrExtD->bIdrFlag			= pNalHdrExtS->bIdrFlag;
  pNalHdrExtD->iNoInterLayerPredFlag	= pNalHdrExtS->iNoInterLayerPredFlag;
  pNalHdrExtD->bDiscardableFlag			= pNalHdrExtS->bDiscardableFlag;
  pNalHdrExtD->bOutputFlag				= pNalHdrExtS->bOutputFlag;
  pNalHdrExtD->bUseRefBasePicFlag	= pNalHdrExtS->bUseRefBasePicFlag;
  pNalHdrExtD->uiLayerDqId				= pNalHdrExtS->uiLayerDqId;

  pShExtD->bStoreRefBasePicFlag		= pPrefixS->bStoreRefBasePicFlag;
  memcpy (&pShExtD->sRefBasePicMarking, &pPrefixS->sRefPicBaseMarking,
          sizeof (SRefBasePicMarking)); //confirmed_safe_unsafe_usage
  if (pShExtD->sRefBasePicMarking.bAdaptiveRefBasePicMarkingModeFlag) {
    PRefBasePicMarking pRefBasePicMarking = &pShExtD->sRefBasePicMarking;
    iIdx = 0;
    do {
      if (pRefBasePicMarking->mmco_base[iIdx].uiMmcoType == MMCO_END)
        break;
      if (pRefBasePicMarking->mmco_base[iIdx].uiMmcoType == MMCO_SHORT2UNUSED)
        pRefBasePicMarking->mmco_base[iIdx].iShortFrameNum = (pShExtD->sSliceHeader.iFrameNum -
            pRefBasePicMarking->mmco_base[iIdx].uiDiffOfPicNums) & ((1 << pSps->uiLog2MaxFrameNum) - 1);
      ++ iIdx;
    } while (iIdx < MAX_MMCO_COUNT);
  }

  return true;
}



int32_t UpdateAccessUnit (PWelsDecoderContext pCtx) {
  PAccessUnit pCurAu	= pCtx->pAccessUnitList;
  int32_t iIdx         = pCurAu->uiEndPos;

  // Conversed iterator
  pCtx->uiTargetDqId = pCurAu->pNalUnitsList[iIdx]->sNalHeaderExt.uiLayerDqId;
  pCurAu->uiActualUnitsNum  = iIdx + 1;
  pCurAu->bCompletedAuFlag = true;

  // Added for mosaic avoidance, 11/19/2009
#ifdef LONG_TERM_REF
  if (pCtx->bParamSetsLostFlag)
#else
  if (pCtx->bReferenceLostAtT0Flag)
#endif
  {
    uint32_t uiActualIdx = 0;
    while (uiActualIdx < pCurAu->uiActualUnitsNum) {
      PNalUnit nal = pCurAu->pNalUnitsList[uiActualIdx];

      if (nal->sNalHeaderExt.sNalUnitHeader.eNalUnitType == NAL_UNIT_CODED_SLICE_IDR || nal->sNalHeaderExt.bIdrFlag) {
        break;
      }
      ++ uiActualIdx;
    }
    if (uiActualIdx ==
        pCurAu->uiActualUnitsNum) {	// no found IDR nal within incoming AU, need exit to avoid mosaic issue, 11/19/2009
      WelsLog (pCtx, WELS_LOG_WARNING, "UpdateAccessUnit():::::Key frame lost.....CAN NOT find IDR from current AU.\n");
#ifdef LONG_TERM_REF
      pCtx->iErrorCode |= dsNoParamSets;
      return dsNoParamSets;
#else
      pCtx->iErrorCode |= dsRefLost;
      return ERR_INFO_REFERENCE_PIC_LOST;
#endif
    }
  }

  return ERR_NONE;
}

int32_t InitialDqLayersContext (PWelsDecoderContext pCtx, const int32_t kiMaxWidth, const int32_t kiMaxHeight) {
  const int32_t kiPicStride		= ((kiMaxWidth + 15) & 0xfffff0) + (PADDING_LENGTH << 1);
  const int32_t kiPicLines		= ((kiMaxHeight + 15) & 0xfffff0);

  int32_t i = 0;

  WELS_VERIFY_RETURN_IF (ERR_INFO_INVALID_PARAM, (NULL == pCtx || kiMaxWidth <= 0 || kiMaxHeight <= 0))
  pCtx->sMb.iMbWidth		= (kiMaxWidth + 15) >> 4;
  pCtx->sMb.iMbHeight		= (kiMaxHeight + 15) >> 4;

  if (pCtx->bInitialDqLayersMem && kiMaxWidth <= pCtx->iPicWidthReq
      && kiMaxHeight <= pCtx->iPicHeightReq)	// have same dimension memory, skipped
    return ERR_NONE;


  UninitialDqLayersContext (pCtx);

  do {
    PDqLayer pDq = (PDqLayer)WelsMalloc (sizeof (SDqLayer), "PDqLayer");

    int32_t iPlaneIdx = 0;

    if (pDq == NULL)
      return ERR_INFO_OUT_OF_MEMORY;

    memset (pDq, 0, sizeof (SDqLayer));

      do {
        const int32_t kiHshift	= iPlaneIdx ? 1 : 0;
        const int32_t kiVshift	= kiHshift;
        const int32_t kiStride	= WELS_ALIGN ((kiPicStride >> kiHshift), (16 << (1 - kiHshift)));
        const int32_t kiLine	= (kiPicLines + (PADDING_LENGTH << 1)) >> kiVshift;
        const int32_t kiSize	= kiStride * kiLine;

        pCtx->pCsListXchg[i][iPlaneIdx]	= (uint8_t*)WelsMalloc (kiSize * sizeof (uint8_t), "pCtx->pCsListXchg[][]");

        WELS_VERIFY_RETURN_IF (ERR_INFO_OUT_OF_MEMORY, (NULL == pCtx->pCsListXchg[i][iPlaneIdx]))
        pCtx->iCsStride[iPlaneIdx]	= kiStride;


        pCtx->pRsListXchg[i][iPlaneIdx]	= (int16_t*)WelsMalloc (kiSize * sizeof (int16_t), "pCtx->pRsListXchg[][]");

        WELS_VERIFY_RETURN_IF (ERR_INFO_OUT_OF_MEMORY , (NULL == pCtx->pRsListXchg[i][iPlaneIdx]))
        pCtx->iRsStride[iPlaneIdx]	= kiStride;

        ++ iPlaneIdx;
      } while (iPlaneIdx < 3);


      pCtx->sMb.pMbType[i] = (int8_t*)WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int8_t),
                             "pCtx->sMb.pMbType[]");
      pCtx->sMb.pMv[i][0] = (int16_t (*)[16][2])WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (
                              int16_t) * MV_A * MB_BLOCK4x4_NUM, "pCtx->sMb.pMv[][]");
      pCtx->sMb.pRefIndex[i][0] = (int8_t (*)[MB_BLOCK4x4_NUM])WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (
                                    int8_t) * MB_BLOCK4x4_NUM, "pCtx->sMb.pRefIndex[][]");
      pCtx->sMb.pLumaQp[i] = (int8_t*)WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int8_t),
                             "pCtx->sMb.pLumaQp[]");
      pCtx->sMb.pChromaQp[i] = (int8_t*)WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int8_t),
                               "pCtx->sMb.pChromaQp[]");
      pCtx->sMb.pNzc[i] = (int8_t (*)[24])WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int8_t) * 24,
                          "pCtx->sMb.pNzc[]");
      pCtx->sMb.pNzcRs[i] = (int8_t (*)[24])WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int8_t) * 24,
                            "pCtx->sMb.pNzcRs[]");
      pCtx->sMb.pScaledTCoeff[i] = (int16_t (*)[MB_COEFF_LIST_SIZE])WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight *
                                   sizeof (int16_t) * MB_COEFF_LIST_SIZE, "pCtx->sMb.pScaledTCoeff[]");
      pCtx->sMb.pIntraPredMode[i] = (int8_t (*)[8])WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int8_t) * 8,
                                    "pCtx->sMb.pIntraPredMode[]");
      pCtx->sMb.pIntra4x4FinalMode[i] = (int8_t (*)[MB_BLOCK4x4_NUM])WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight *
                                        sizeof (int8_t) * MB_BLOCK4x4_NUM, "pCtx->sMb.pIntra4x4FinalMode[]");
      pCtx->sMb.pChromaPredMode[i] = (int8_t*)WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int8_t),
                                     "pCtx->sMb.pChromaPredMode[]");
      pCtx->sMb.pCbp[i] = (int8_t*)WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int8_t),
                          "pCtx->sMb.pCbp[]");
      pCtx->sMb.pSubMbType[i] = (int8_t (*)[MB_PARTITION_SIZE])WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (
                                  int8_t) * MB_PARTITION_SIZE, "pCtx->sMb.pSubMbType[]");
      pCtx->sMb.pSliceIdc[i] = (int32_t*) WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int32_t),
                               "pCtx->sMb.pSliceIdc[]");	// using int32_t for slice_idc, 4/21/2010
      if (pCtx->sMb.pSliceIdc[i] != NULL)
        memset (pCtx->sMb.pSliceIdc[i], 0xff, (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int32_t)));
      pCtx->sMb.pResidualPredFlag[i] = (int8_t*) WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int8_t),
                                       "pCtx->sMb.pResidualPredFlag[]");
      //pCtx->sMb.pMotionPredFlag[i] = (uint8_t *) WelsMalloc(pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof(uint8_t), "pCtx->sMb.pMotionPredFlag[]");
      pCtx->sMb.pInterPredictionDoneFlag[i] = (int8_t*) WelsMalloc (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (
          int8_t), "pCtx->sMb.pInterPredictionDoneFlag[]");

      // check memory block valid due above allocated..
      WELS_VERIFY_RETURN_IF (ERR_INFO_OUT_OF_MEMORY,
                             ((NULL == pCtx->sMb.pMbType[i]) ||
                              (NULL == pCtx->sMb.pMv[i][0]) ||
                              (NULL == pCtx->sMb.pRefIndex[i][0]) ||
                              (NULL == pCtx->sMb.pLumaQp[i]) ||
                              (NULL == pCtx->sMb.pChromaQp[i]) ||
                              (NULL == pCtx->sMb.pNzc[i]) ||
                              (NULL == pCtx->sMb.pNzcRs[i]) ||
                              (NULL == pCtx->sMb.pScaledTCoeff[i]) ||
                              (NULL == pCtx->sMb.pIntraPredMode[i]) ||
                              (NULL == pCtx->sMb.pIntra4x4FinalMode[i]) ||
                              (NULL == pCtx->sMb.pChromaPredMode[i]) ||
                              (NULL == pCtx->sMb.pCbp[i]) ||
                              (NULL == pCtx->sMb.pSubMbType[i]) ||
                              (NULL == pCtx->sMb.pSliceIdc[i]) ||
                              (NULL == pCtx->sMb.pResidualPredFlag[i]) ||
                              (NULL == pCtx->sMb.pInterPredictionDoneFlag[i])
                             )
                            )

    pCtx->pDqLayersList[i] = pDq;
    ++ i;
  } while (i < LAYER_NUM_EXCHANGEABLE);


  pCtx->bInitialDqLayersMem	= true;
  pCtx->iPicWidthReq			= kiMaxWidth;
  pCtx->iPicHeightReq			= kiMaxHeight;

  return ERR_NONE;
}

void UninitialDqLayersContext (PWelsDecoderContext pCtx) {
  int32_t i = 0;
  int32_t j = 0;

  do {
    PDqLayer pDq = pCtx->pDqLayersList[i];
    if (pDq == NULL) {
      ++ i;
      continue;
    }

    if (pCtx->pCsListXchg[i]) {	// cs picture
      j = 0;
      do {
        if (NULL != pCtx->pCsListXchg[i][j]) {
          WelsFree (pCtx->pCsListXchg[i][j], "pCtx->pCsListXchg[][]");

          pCtx->pCsListXchg[i][j] = NULL;
        }
        pCtx->iCsStride[j]	= 0;
        ++ j;
      } while (j < 3);

      pDq->pCsData[i]		= NULL;	// for safe
      pDq->iCsStride[i]	= 0;
    }
    if (pCtx->pRsListXchg[i]) {
      j = 0;
      do {
        if (NULL != pCtx->pRsListXchg[i][j]) {
          WelsFree (pCtx->pRsListXchg[i][j], "pCtx->pRsListXchg[][]");

          pCtx->pRsListXchg[i][j]	= NULL;
        }
        pCtx->iRsStride[j]	= 0;
        ++ j;
      } while (j < 3);
    }

    if (pCtx->sMb.pMbType[i]) {
      WelsFree (pCtx->sMb.pMbType[i], "pCtx->sMb.pMbType[]");

      pCtx->sMb.pMbType[i] = NULL;
    }

    if (pCtx->sMb.pMv[i][0]) {
      WelsFree (pCtx->sMb.pMv[i][0], "pCtx->sMb.pMv[][]");

      pCtx->sMb.pMv[i][0] = NULL;
    }

    if (pCtx->sMb.pRefIndex[i][0]) {
      WelsFree (pCtx->sMb.pRefIndex[i][0], "pCtx->sMb.pRefIndex[][]");

      pCtx->sMb.pRefIndex[i][0] = NULL;
    }

    if (pCtx->sMb.pLumaQp[i]) {
      WelsFree (pCtx->sMb.pLumaQp[i], "pCtx->sMb.pLumaQp[]");

      pCtx->sMb.pLumaQp[i] = NULL;
    }

    if (pCtx->sMb.pChromaQp[i]) {
      WelsFree (pCtx->sMb.pChromaQp[i], "pCtx->sMb.pChromaQp[]");

      pCtx->sMb.pChromaQp[i] = NULL;
    }

    if (pCtx->sMb.pNzc[i]) {
      WelsFree (pCtx->sMb.pNzc[i], "pCtx->sMb.pNzc[]");

      pCtx->sMb.pNzc[i] = NULL;
    }

    if (pCtx->sMb.pNzcRs[i]) {
      WelsFree (pCtx->sMb.pNzcRs[i], "pCtx->sMb.pNzcRs[]");

      pCtx->sMb.pNzcRs[i] = NULL;
    }

    if (pCtx->sMb.pScaledTCoeff[i]) {
      WelsFree (pCtx->sMb.pScaledTCoeff[i], "pCtx->sMb.pScaledTCoeff[]");

      pCtx->sMb.pScaledTCoeff[i] = NULL;
    }

    if (pCtx->sMb.pIntraPredMode[i]) {
      WelsFree (pCtx->sMb.pIntraPredMode[i], "pCtx->sMb.pIntraPredMode[]");

      pCtx->sMb.pIntraPredMode[i] = NULL;
    }

    if (pCtx->sMb.pIntra4x4FinalMode[i]) {
      WelsFree (pCtx->sMb.pIntra4x4FinalMode[i], "pCtx->sMb.pIntra4x4FinalMode[]");

      pCtx->sMb.pIntra4x4FinalMode[i] = NULL;
    }

    if (pCtx->sMb.pChromaPredMode[i]) {
      WelsFree (pCtx->sMb.pChromaPredMode[i], "pCtx->sMb.pChromaPredMode[]");

      pCtx->sMb.pChromaPredMode[i] = NULL;
    }

    if (pCtx->sMb.pCbp[i]) {
      WelsFree (pCtx->sMb.pCbp[i], "pCtx->sMb.pCbp[]");

      pCtx->sMb.pCbp[i] = NULL;
    }

    //      if (pCtx->sMb.pMotionPredFlag[i])
    //{
    //	WelsFree( pCtx->sMb.pMotionPredFlag[i], "pCtx->sMb.pMotionPredFlag[]" );

    //	pCtx->sMb.pMotionPredFlag[i] = NULL;
    //}

    if (pCtx->sMb.pSubMbType[i]) {
      WelsFree (pCtx->sMb.pSubMbType[i], "pCtx->sMb.pSubMbType[]");

      pCtx->sMb.pSubMbType[i] = NULL;
    }

    if (pCtx->sMb.pSliceIdc[i]) {
      WelsFree (pCtx->sMb.pSliceIdc[i], "pCtx->sMb.pSliceIdc[]");

      pCtx->sMb.pSliceIdc[i] = NULL;
    }

    if (pCtx->sMb.pResidualPredFlag[i]) {
      WelsFree (pCtx->sMb.pResidualPredFlag[i], "pCtx->sMb.pResidualPredFlag[]");

      pCtx->sMb.pResidualPredFlag[i] = NULL;
    }

    if (pCtx->sMb.pInterPredictionDoneFlag[i]) {
      WelsFree (pCtx->sMb.pInterPredictionDoneFlag[i], "pCtx->sMb.pInterPredictionDoneFlag[]");

      pCtx->sMb.pInterPredictionDoneFlag[i] = NULL;
    }
    WelsFree (pDq, "pDq");

    pDq = NULL;
    pCtx->pDqLayersList[i] = NULL;

    ++ i;
  } while (i < LAYER_NUM_EXCHANGEABLE);

  pCtx->iPicWidthReq			= 0;
  pCtx->iPicHeightReq			= 0;
  pCtx->bInitialDqLayersMem	= false;
}

void ResetCurrentAccessUnit (PWelsDecoderContext pCtx) {
  PAccessUnit pCurAu = pCtx->pAccessUnitList;

  pCurAu->uiEndPos		= 0;
  pCurAu->bCompletedAuFlag	= false;
  if (pCurAu->uiActualUnitsNum > 0) {
    uint32_t iIdx = 0;
    const uint32_t kuiActualNum = pCurAu->uiActualUnitsNum;
    // a more simpler method to do nal units list management prefered here
    const uint32_t kuiAvailNum	= pCurAu->uiAvailUnitsNum;
    const uint32_t kuiLeftNum	= kuiAvailNum - kuiActualNum;

    // Swapping active nal unit nodes of succeeding AU with leading of list
    while (iIdx < kuiLeftNum) {
      PNalUnit t = pCurAu->pNalUnitsList[kuiActualNum + iIdx];
      pCurAu->pNalUnitsList[kuiActualNum + iIdx] = pCurAu->pNalUnitsList[iIdx];
      pCurAu->pNalUnitsList[iIdx] = t;
      ++ iIdx;
    }
    pCurAu->uiActualUnitsNum = pCurAu->uiAvailUnitsNum	= kuiLeftNum;
  }
}

/*!
 * \brief	Force reset current Acess Unit Nal list in case error parsing/decoding in current AU
 * \author
 * \history	11/16/2009
 */
void ForceResetCurrentAccessUnit (PAccessUnit pAu) {
  uint32_t uiSucAuIdx	= pAu->uiEndPos + 1;
  uint32_t uiCurAuIdx	= 0;

  // swap the succeeding AU's nal units to the front
  while (uiSucAuIdx < pAu->uiAvailUnitsNum) {
    PNalUnit t = pAu->pNalUnitsList[uiSucAuIdx];
    pAu->pNalUnitsList[uiSucAuIdx]	= pAu->pNalUnitsList[uiCurAuIdx];
    pAu->pNalUnitsList[uiCurAuIdx]	= t;
    ++ uiSucAuIdx;
    ++ uiCurAuIdx;
  }

  // Update avail/actual units num accordingly for next AU parsing
  if (pAu->uiAvailUnitsNum > pAu->uiEndPos)
    pAu->uiAvailUnitsNum	-= (pAu->uiEndPos + 1);
  else
    pAu->uiAvailUnitsNum	= 0;
  pAu->uiActualUnitsNum	= 0;
  pAu->uiEndPos		= 0;
  pAu->bCompletedAuFlag	= false;
}

//clear current corrupted NAL from pNalUnitsList
void ForceClearCurrentNal (PAccessUnit pAu) {
  if (pAu->uiAvailUnitsNum > 0)
    -- pAu->uiAvailUnitsNum;
}


void CheckAvailNalUnitsListContinuity (PWelsDecoderContext pCtx, int32_t iStartIdx, int32_t iEndIdx) {
  PAccessUnit pCurAu = pCtx->pAccessUnitList;

  uint8_t uiLastNuDependencyId, uiLastNuLayerDqId;
  uint8_t uiCurNuDependencyId, uiCurNuQualityId, uiCurNuLayerDqId, uiCurNuRefLayerDqId;

  int32_t iCurNalUnitIdx = 0;

  //check the continuity of pNalUnitsList forwards (from pIdxNoInterLayerPred to end_postion)
  uiLastNuDependencyId = pCurAu->pNalUnitsList[iStartIdx]->sNalHeaderExt.uiDependencyId;//starting nal unit
  uiLastNuLayerDqId   = pCurAu->pNalUnitsList[iStartIdx]->sNalHeaderExt.uiLayerDqId;//starting nal unit
  iCurNalUnitIdx = iStartIdx + 1;//current nal unit
  while (iCurNalUnitIdx <= iEndIdx) {
    uiCurNuDependencyId   = pCurAu->pNalUnitsList[iCurNalUnitIdx]->sNalHeaderExt.uiDependencyId;
    uiCurNuQualityId      = pCurAu->pNalUnitsList[iCurNalUnitIdx]->sNalHeaderExt.uiQualityId;
    uiCurNuLayerDqId     = pCurAu->pNalUnitsList[iCurNalUnitIdx]->sNalHeaderExt.uiLayerDqId;
    uiCurNuRefLayerDqId = pCurAu->pNalUnitsList[iCurNalUnitIdx]->sNalData.sVclNal.sSliceHeaderExt.uiRefLayerDqId;

    if (uiCurNuDependencyId == uiLastNuDependencyId) {
      uiLastNuLayerDqId = uiCurNuLayerDqId;
      ++ iCurNalUnitIdx;
    } else { //uiCurNuDependencyId != uiLastNuDependencyId, new dependency arrive
      if (uiCurNuQualityId == 0) {
        uiLastNuDependencyId = uiCurNuDependencyId;
        if (uiCurNuRefLayerDqId == uiLastNuLayerDqId) {
          uiLastNuLayerDqId = uiCurNuLayerDqId;
          ++ iCurNalUnitIdx;
        } else { //cur_nu_layer_id != next_nu_ref_layer_dq_id, the chain is broken at this point
          break;
        }
      } else { //new dependency arrive, but no base quality layer, so we must stop in this point
        break;
      }
    }
  }

  -- iCurNalUnitIdx;
  pCurAu->uiEndPos = iCurNalUnitIdx;
  pCtx->uiTargetDqId = pCurAu->pNalUnitsList[iCurNalUnitIdx]->sNalHeaderExt.uiLayerDqId;
}

//main purpose: to support multi-slice and to include all slice which have the same uiDependencyId, uiQualityId and frame_num
//for single slice, pIdxNoInterLayerPred SHOULD NOT be modified
void RefineIdxNoInterLayerPred (PAccessUnit pCurAu, int32_t* pIdxNoInterLayerPred) {
  int32_t iLastNalDependId  = pCurAu->pNalUnitsList[*pIdxNoInterLayerPred]->sNalHeaderExt.uiDependencyId;
  int32_t iLastNalQualityId = pCurAu->pNalUnitsList[*pIdxNoInterLayerPred]->sNalHeaderExt.uiQualityId;
  uint8_t uiLastNalTId       = pCurAu->pNalUnitsList[*pIdxNoInterLayerPred]->sNalHeaderExt.uiTemporalId;
  int32_t iLastNalFrameNum  =
    pCurAu->pNalUnitsList[*pIdxNoInterLayerPred]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.iFrameNum;
  int32_t iLastNalPoc        =
    pCurAu->pNalUnitsList[*pIdxNoInterLayerPred]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.iPicOrderCntLsb;
  int32_t iLastNalFirstMb   =
    pCurAu->pNalUnitsList[*pIdxNoInterLayerPred]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.iFirstMbInSlice;
  int32_t iCurNalDependId, iCurNalQualityId, iCurNalTId, iCurNalFrameNum, iCurNalPoc, iCurNalFirstMb, iCurIdx,
          iFinalIdxNoInterLayerPred;

  bool  bMultiSliceFind = false;

  iFinalIdxNoInterLayerPred = 0;
  iCurIdx = *pIdxNoInterLayerPred - 1;
  while (iCurIdx >= 0) {
    if (pCurAu->pNalUnitsList[iCurIdx]->sNalHeaderExt.iNoInterLayerPredFlag) {
      iCurNalDependId  = pCurAu->pNalUnitsList[iCurIdx]->sNalHeaderExt.uiDependencyId;
      iCurNalQualityId = pCurAu->pNalUnitsList[iCurIdx]->sNalHeaderExt.uiQualityId;
      iCurNalTId       = pCurAu->pNalUnitsList[iCurIdx]->sNalHeaderExt.uiTemporalId;
      iCurNalFrameNum  = pCurAu->pNalUnitsList[iCurIdx]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.iFrameNum;
      iCurNalPoc        = pCurAu->pNalUnitsList[iCurIdx]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.iPicOrderCntLsb;
      iCurNalFirstMb   = pCurAu->pNalUnitsList[iCurIdx]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.iFirstMbInSlice;

      if (iCurNalDependId == iLastNalDependId  &&
          iCurNalQualityId == iLastNalQualityId &&
          iCurNalTId       == uiLastNalTId       &&
          iCurNalFrameNum  == iLastNalFrameNum  &&
          iCurNalPoc        == iLastNalPoc        &&
          iCurNalFirstMb   != iLastNalFirstMb) {
        bMultiSliceFind = true;
        iFinalIdxNoInterLayerPred = iCurIdx;
        --iCurIdx;
        continue;
      } else {
        break;
      }
    }
    --iCurIdx;
  }

  if (bMultiSliceFind && *pIdxNoInterLayerPred != iFinalIdxNoInterLayerPred) {
    *pIdxNoInterLayerPred = iFinalIdxNoInterLayerPred;
  }
}

bool CheckPocOfCurValidNalUnits (PAccessUnit pCurAu, int32_t pIdxNoInterLayerPred) {
  int32_t iEndIdx    = pCurAu->uiEndPos;
  int32_t iCurAuPoc =
    pCurAu->pNalUnitsList[pIdxNoInterLayerPred]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.iPicOrderCntLsb;
  int32_t iTmpPoc, i;
  for (i = pIdxNoInterLayerPred + 1; i < iEndIdx; i++) {
    iTmpPoc = pCurAu->pNalUnitsList[i]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.iPicOrderCntLsb;
    if (iTmpPoc != iCurAuPoc) {
      return false;
    }
  }

  return true;
}

bool CheckIntegrityNalUnitsList (PWelsDecoderContext pCtx) {
  PAccessUnit pCurAu = pCtx->pAccessUnitList;
  const int32_t kiEndPos = pCurAu->uiEndPos;
  int32_t iIdxNoInterLayerPred = 0;
  int32_t iCurNalUnitIdx = kiEndPos;

  ESliceType eSliceType = static_cast<ESliceType> (0);//EC 2009.11.12

  if (!pCurAu->bCompletedAuFlag)
    return false;

  eSliceType = pCurAu->pNalUnitsList[iCurNalUnitIdx]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.eSliceType;

  if (I_SLICE == eSliceType) {
    pCurAu->uiStartPos = 0;
    //step1: search the pNalUnit whose iNoInterLayerPredFlag equal to 1 backwards (from uiEndPos to 0)
    iIdxNoInterLayerPred = kiEndPos;
    while (iIdxNoInterLayerPred >= 0) {
      if (pCurAu->pNalUnitsList[iIdxNoInterLayerPred]->sNalHeaderExt.iNoInterLayerPredFlag) {
        break;
      }
      --iIdxNoInterLayerPred;
    }
    if (iIdxNoInterLayerPred < 0) {
      //can not find the Nal Unit whose no_inter_pred_falg equal to 1, MUST STOP decode
      return false;
    }

    //step2: support multi-slice, to include all base layer slice
    RefineIdxNoInterLayerPred (pCurAu, &iIdxNoInterLayerPred);
    pCurAu->uiStartPos = iIdxNoInterLayerPred;
    CheckAvailNalUnitsListContinuity (pCtx, iIdxNoInterLayerPred, kiEndPos);

    if (!CheckPocOfCurValidNalUnits (pCurAu, iIdxNoInterLayerPred)) {
      return false;
    }

    pCtx->iCurSeqIntervalTargetDependId = pCurAu->pNalUnitsList[pCurAu->uiEndPos]->sNalHeaderExt.uiDependencyId;
    pCtx->iCurSeqIntervalMaxPicWidth  =
      pCurAu->pNalUnitsList[pCurAu->uiEndPos]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.iMbWidth << 4;
    pCtx->iCurSeqIntervalMaxPicHeight =
      pCurAu->pNalUnitsList[pCurAu->uiEndPos]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.iMbHeight << 4;
  } else { //P_SLICE
    //step 1: search uiDependencyId equal to pCtx->cur_seq_interval_target_dependency_id
    bool bGetDependId = false;
    int32_t iIdxDependId = 0;

    iIdxDependId = kiEndPos;
    while (iIdxDependId >= 0) {
      if (pCtx->iCurSeqIntervalTargetDependId == pCurAu->pNalUnitsList[iIdxDependId]->sNalHeaderExt.uiDependencyId) {
        bGetDependId = true;
        break;
      } else {
        --iIdxDependId;
      }
    }

    //step 2: switch according to whether or not find the index of pNalUnit whose uiDependencyId equal to iCurSeqIntervalTargetDependId
    if (bGetDependId) { //get the index of pNalUnit whose uiDependencyId equal to iCurSeqIntervalTargetDependId
      bool bGetNoInterPredFront = false;
      //step 2a: search iNoInterLayerPredFlag [0....iIdxDependId]
      iIdxNoInterLayerPred = iIdxDependId;
      while (iIdxNoInterLayerPred >= 0) {
        if (pCurAu->pNalUnitsList[iIdxNoInterLayerPred]->sNalHeaderExt.iNoInterLayerPredFlag) {
          bGetNoInterPredFront = true;
          break;
        }
        --iIdxNoInterLayerPred;
      }
      //step 2b: switch, whether or not find the NAL unit whose no_inter_pred_flag equal to 1 among [0....iIdxDependId]
      if (bGetNoInterPredFront) { //YES
        RefineIdxNoInterLayerPred (pCurAu, &iIdxNoInterLayerPred);
        pCurAu->uiStartPos = iIdxNoInterLayerPred;
        CheckAvailNalUnitsListContinuity (pCtx, iIdxNoInterLayerPred, iIdxDependId);

        if (!CheckPocOfCurValidNalUnits (pCurAu, iIdxNoInterLayerPred)) {
          return false;
        }
      } else { //NO, should find the NAL unit whose no_inter_pred_flag equal to 1 among [iIdxDependId....uiEndPos]
        iIdxNoInterLayerPred = iIdxDependId;
        while (iIdxNoInterLayerPred <= kiEndPos) {
          if (pCurAu->pNalUnitsList[iIdxNoInterLayerPred]->sNalHeaderExt.iNoInterLayerPredFlag) {
            break;
          }
          ++iIdxNoInterLayerPred;
        }

        if (iIdxNoInterLayerPred > kiEndPos) {
          return false; //cann't find the index of pNalUnit whose no_inter_pred_flag = 1
        }

        RefineIdxNoInterLayerPred (pCurAu, &iIdxNoInterLayerPred);
        pCurAu->uiStartPos = iIdxNoInterLayerPred;
        CheckAvailNalUnitsListContinuity (pCtx, iIdxNoInterLayerPred, kiEndPos);

        if (!CheckPocOfCurValidNalUnits (pCurAu, iIdxNoInterLayerPred)) {
          return false;
        }
      }
    } else { //without the index of pNalUnit, should process this AU as common case
      iIdxNoInterLayerPred = kiEndPos;
      while (iIdxNoInterLayerPred >= 0) {
        if (pCurAu->pNalUnitsList[iIdxNoInterLayerPred]->sNalHeaderExt.iNoInterLayerPredFlag) {
          break;
        }
        --iIdxNoInterLayerPred;
      }
      if (iIdxNoInterLayerPred < 0) {
        return false; //cann't find the index of pNalUnit whose iNoInterLayerPredFlag = 1
      }

      RefineIdxNoInterLayerPred (pCurAu, &iIdxNoInterLayerPred);
      pCurAu->uiStartPos = iIdxNoInterLayerPred;
      CheckAvailNalUnitsListContinuity (pCtx, iIdxNoInterLayerPred, kiEndPos);

      if (!CheckPocOfCurValidNalUnits (pCurAu, iIdxNoInterLayerPred)) {
        return false;
      }
    }
  }

  return true;
}

void CheckOnlyOneLayerInAu (PWelsDecoderContext pCtx) {
  PAccessUnit pCurAu = pCtx->pAccessUnitList;

  int32_t iEndIdx = pCurAu->uiEndPos;
  int32_t iCurIdx = pCurAu->uiStartPos;
  uint8_t uiDId = pCurAu->pNalUnitsList[iCurIdx]->sNalHeaderExt.uiDependencyId;
  uint8_t uiQId = pCurAu->pNalUnitsList[iCurIdx]->sNalHeaderExt.uiQualityId;
  uint8_t uiTId = pCurAu->pNalUnitsList[iCurIdx]->sNalHeaderExt.uiTemporalId;

  uint8_t uiCurDId, uiCurQId, uiCurTId;

  pCtx->bOnlyOneLayerInCurAuFlag = true;

  if (iEndIdx == iCurIdx) { //only one NAL in pNalUnitsList
    return;
  }

  ++iCurIdx;
  while (iCurIdx <= iEndIdx) {
    uiCurDId = pCurAu->pNalUnitsList[iCurIdx]->sNalHeaderExt.uiDependencyId;
    uiCurQId = pCurAu->pNalUnitsList[iCurIdx]->sNalHeaderExt.uiQualityId;
    uiCurTId = pCurAu->pNalUnitsList[iCurIdx]->sNalHeaderExt.uiTemporalId;

    if (uiDId != uiCurDId || uiQId != uiCurQId || uiTId != uiCurTId) {
      pCtx->bOnlyOneLayerInCurAuFlag = false;
      return;
    }

    ++iCurIdx;
  }
}

int32_t WelsDecodeAccessUnitStart (PWelsDecoderContext pCtx) {
  // Roll back NAL units not being belong to current access unit list for proceeded access unit
  int32_t iRet = UpdateAccessUnit (pCtx);
  if (iRet != ERR_NONE)
    return iRet;

  pCtx->pAccessUnitList->uiStartPos = 0;
  if (!pCtx->bAvcBasedFlag && !CheckIntegrityNalUnitsList (pCtx)) {
    pCtx->iErrorCode |= dsBitstreamError;
    return dsBitstreamError;
  }

  //check current AU has only one layer or not
  //If YES, can use deblocking based on AVC
  if (!pCtx->bAvcBasedFlag) {
    CheckOnlyOneLayerInAu (pCtx);
  }

  return ERR_NONE;
}

void WelsDecodeAccessUnitEnd (PWelsDecoderContext pCtx) {
  // uninitialize context of current access unit and rbsp buffer clean
  ResetCurrentAccessUnit (pCtx);
}




/*
 * ConstructAccessUnit
 * construct an access unit for given input bitstream, maybe partial NAL Unit, one or more Units are involved to
 * joint a collective access unit.
 * parameter\
 *	buf:		bitstream data buffer
 *	bit_len:	size in bit length of data
 *	buf_len:	size in byte length of data
 *	coded_au:	mark an Access Unit decoding finished
 * return:
 *	0 - success; otherwise returned error_no defined in error_no.h
 */
int32_t ConstructAccessUnit (PWelsDecoderContext pCtx, uint8_t** ppDst, SBufferInfo* pDstInfo) {
  int32_t iErr;
  int32_t iWidth;
  int32_t iHeight;
  int32_t iStride[2] = { 0 };

  PAccessUnit pCurAu = pCtx->pAccessUnitList;

  pCtx->bAuReadyFlag = false;
  pCtx->bLastHasMmco5 = false;

  iErr = WelsDecodeAccessUnitStart (pCtx);
  GetVclNalTemporalId (pCtx);

  if (ERR_NONE != iErr) {
    ForceResetCurrentAccessUnit (pCtx->pAccessUnitList);
    pDstInfo->iBufferStatus = 0;
    return iErr;
  }

  pCtx->pSps = pCurAu->pNalUnitsList[pCurAu->uiStartPos]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.pSps;
  pCtx->pPps = pCurAu->pNalUnitsList[pCurAu->uiStartPos]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.pPps;

  //try to allocate or relocate DPB memory only when IDR arrival.
  if (NAL_UNIT_CODED_SLICE_IDR == pCurAu->pNalUnitsList[pCurAu->uiStartPos]->sNalHeaderExt.sNalUnitHeader.eNalUnitType ||
      pCurAu->pNalUnitsList[pCurAu->uiStartPos]->sNalHeaderExt.bIdrFlag) {
    WelsResetRefPic (pCtx); //clear ref pPic when IDR NAL
	iErr = SyncPictureResolutionExt (pCtx, pCtx->pSps->iMbWidth, pCtx->pSps->iMbHeight);

    if (ERR_NONE != iErr) {
      WelsLog (pCtx, WELS_LOG_WARNING, "sync picture resolution ext failed,  the error is %d", iErr);
      return iErr;
    }
  }


  iErr = DecodeCurrentAccessUnit (pCtx, ppDst, iStride, &iWidth, &iHeight, pDstInfo);

  WelsDecodeAccessUnitEnd (pCtx);

  if (ERR_NONE != iErr) {
    WelsLog (pCtx, WELS_LOG_INFO, "returned error from decoding:[0x%x]\n", iErr);

    pDstInfo->iBufferStatus = 0;
    return iErr;
  }

  return 0;
}

static inline void InitDqLayerInfo (PDqLayer pDqLayer, PLayerInfo pLayerInfo, PNalUnit pNalUnit, PPicture pPicDec) {
  PNalUnitHeaderExt pNalHdrExt    = &pNalUnit->sNalHeaderExt;
  PSliceHeaderExt pShExt			= &pNalUnit->sNalData.sVclNal.sSliceHeaderExt;
  PSliceHeader        pSh			= &pShExt->sSliceHeader;
  const uint8_t kuiQualityId		= pNalHdrExt->uiQualityId;

  memcpy (&pDqLayer->sLayerInfo, pLayerInfo, sizeof (SLayerInfo)); //confirmed_safe_unsafe_usage

  pDqLayer->pDec		= pPicDec;
  pDqLayer->iMbWidth	= pSh->iMbWidth;	// MB width of this picture
  pDqLayer->iMbHeight	= pSh->iMbHeight;// MB height of this picture

  pDqLayer->iSliceIdcBackup = (pSh->iFirstMbInSlice << 7) | (pNalHdrExt->uiDependencyId << 4) | (pNalHdrExt->uiQualityId);

  /* Common syntax elements across all slices of a DQLayer */
  pDqLayer->uiPpsId									= pLayerInfo->pPps->iPpsId;
  pDqLayer->uiDisableInterLayerDeblockingFilterIdc	= pShExt->uiDisableInterLayerDeblockingFilterIdc;
  pDqLayer->iInterLayerSliceAlphaC0Offset			    = pShExt->iInterLayerSliceAlphaC0Offset;
  pDqLayer->iInterLayerSliceBetaOffset				= pShExt->iInterLayerSliceBetaOffset;
  pDqLayer->iSliceGroupChangeCycle					= pSh->iSliceGroupChangeCycle;
  pDqLayer->bStoreRefBasePicFlag					    = pShExt->bStoreRefBasePicFlag;
  pDqLayer->bTCoeffLevelPredFlag					    = pShExt->bTCoeffLevelPredFlag;
  pDqLayer->bConstrainedIntraResamplingFlag			= pShExt->bConstrainedIntraResamplingFlag;
  pDqLayer->uiRefLayerDqId							= pShExt->uiRefLayerDqId;
  pDqLayer->uiRefLayerChromaPhaseXPlus1Flag		    = pShExt->uiRefLayerChromaPhaseXPlus1Flag;
  pDqLayer->uiRefLayerChromaPhaseYPlus1				= pShExt->uiRefLayerChromaPhaseYPlus1;
  //memcpy(&pDqLayer->sScaledRefLayer, &pShExt->sScaledRefLayer, sizeof(SPosOffset));//confirmed_safe_unsafe_usage

  if (kuiQualityId == BASE_QUALITY_ID) {
    pDqLayer->pRefPicListReordering		= &pSh->pRefPicListReordering;
    pDqLayer->pRefPicMarking		= &pSh->sRefMarking;
    pDqLayer->pRefPicBaseMarking	= &pShExt->sRefBasePicMarking;
  }

  pDqLayer->uiLayerDqId			= pNalHdrExt->uiLayerDqId;	// dq_id of current layer
  pDqLayer->bUseRefBasePicFlag	= pNalHdrExt->bUseRefBasePicFlag;
}

void WelsDqLayerDecodeStart (PWelsDecoderContext pCtx, PNalUnit pCurNal, PSps pSps, PPps pPps) {
  SNalUnitHeader* pNalHdr = &pCurNal->sNalHeaderExt.sNalUnitHeader;
  PSliceHeader pSh = &pCurNal->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader;

  pCtx->eSliceType			= pSh->eSliceType;
  pCtx->pSliceHeader			= pSh;

  pCtx->iFrameNum			= pSh->iFrameNum;

  if ((pNalHdr->eNalUnitType == NAL_UNIT_CODED_SLICE_IDR ||
       (pCurNal->sNalHeaderExt.uiQualityId == BASE_QUALITY_ID && pCurNal->sNalHeaderExt.bIdrFlag)) &&
      pSh->iFrameNum == 0) { //pSh->iFrameNum == 0 this condition can deleted??????
    WelsResetRefPic (pCtx);	// Reset decoded picture buffer lists due to an IDR frame incomes
  }
}

int32_t InitRefPicList (PWelsDecoderContext pCtx, const uint8_t kuiNRi, const bool kbFirstSlice, int32_t iPoc) {
  int32_t iRet = ERR_NONE;
  if (kbFirstSlice)
    iRet = WelsInitRefList (pCtx, iPoc);
  if ((pCtx->eSliceType != I_SLICE && pCtx->eSliceType != SI_SLICE) && kbFirstSlice) {
    iRet = WelsReorderRefList (pCtx);
  }

  return iRet;
}

void InitCurDqLayerData (PWelsDecoderContext pCtx, PDqLayer pCurDq) {
  if (NULL != pCtx && NULL != pCurDq) {
    pCurDq->pCsData[0]		= pCtx->pCsListXchg[0][0];
    pCurDq->pCsData[1]		= pCtx->pCsListXchg[0][1];
    pCurDq->pCsData[2]		= pCtx->pCsListXchg[0][2];
    pCurDq->iCsStride[0]	= pCtx->iCsStride[0];
    pCurDq->iCsStride[1]	= pCtx->iCsStride[1];
    pCurDq->iCsStride[2]	= pCtx->iCsStride[2];

    pCurDq->pMbType			= pCtx->sMb.pMbType[0];
    pCurDq->pSliceIdc		= pCtx->sMb.pSliceIdc[0];
    pCurDq->pMv[0]			= pCtx->sMb.pMv[0][0];
    pCurDq->pRefIndex[0]    = pCtx->sMb.pRefIndex[0][0];
    pCurDq->pLumaQp         = pCtx->sMb.pLumaQp[0];
    pCurDq->pChromaQp       = pCtx->sMb.pChromaQp[0];
    pCurDq->pNzc			= pCtx->sMb.pNzc[0];
    pCurDq->pNzcRs			= pCtx->sMb.pNzcRs[0];
    pCurDq->pScaledTCoeff   = pCtx->sMb.pScaledTCoeff[0];
    pCurDq->pIntraPredMode  = pCtx->sMb.pIntraPredMode[0];
    pCurDq->pIntra4x4FinalMode = pCtx->sMb.pIntra4x4FinalMode[0];
    pCurDq->pChromaPredMode = pCtx->sMb.pChromaPredMode[0];
    pCurDq->pCbp            = pCtx->sMb.pCbp[0];
    pCurDq->pSubMbType      = pCtx->sMb.pSubMbType[0];
    pCurDq->pInterPredictionDoneFlag = pCtx->sMb.pInterPredictionDoneFlag[0];
    pCurDq->pResidualPredFlag = pCtx->sMb.pResidualPredFlag[0];
  }
}

// added to reset state of parameter sets to waiting successive incoming IDR, 6/4/2010
// It will be called in case packets lost/ broken and decoded failed at temporal level 0
void ResetParameterSetsState (PWelsDecoderContext pCtx) {
  pCtx->bSpsExistAheadFlag	   = false;
  pCtx->bSubspsExistAheadFlag = false;
  pCtx->bPpsExistAheadFlag	   = false;
}

/*
 * DecodeCurrentAccessUnit
 * Decode current access unit when current AU is completed.
 */
int32_t DecodeCurrentAccessUnit (PWelsDecoderContext pCtx, uint8_t** ppDst, int32_t* pDstLen, int32_t* pWidth,
                                 int32_t* pHeight, SBufferInfo* pDstInfo) {
  int32_t iRefCount[LIST_A];
  PNalUnit pNalCur = NULL;
  PAccessUnit pCurAu = pCtx->pAccessUnitList;

  int32_t iIdx = pCurAu->uiStartPos;
  int32_t iEndIdx = pCurAu->uiEndPos;

  int32_t iPpsId = 0;
  int32_t iRet = ERR_NONE;

  const uint8_t kuiTargetLayerDqId = GetTargetDqId (pCtx->uiTargetDqId, pCtx->pParam);
  const uint8_t kuiDependencyIdMax = (kuiTargetLayerDqId & 0x7F) >> 4;
  int16_t iLastIdD = -1, iLastIdQ = -1;
  int16_t iCurrIdD = 0, iCurrIdQ = 0;
  uint8_t uiNalRefIdc = 0;
  bool	bFreshSliceAvailable =
    true;	// Another fresh slice comingup for given dq layer, for multiple slices in case of header parts of slices sometimes loss over error-prone channels, 8/14/2008
  PPicture  pStoreBasePic = NULL;

  //update pCurDqLayer at the starting of AU decoding
  if (pCtx->bInitialDqLayersMem) {
    pCtx->pCurDqLayer				= pCtx->pDqLayersList[0];
  }

  InitCurDqLayerData (pCtx, pCtx->pCurDqLayer);

  pNalCur = pCurAu->pNalUnitsList[iIdx];
  while (iIdx <= iEndIdx) {
    PDqLayer dq_cur							= pCtx->pCurDqLayer;
    SLayerInfo pLayerInfo;
    PSliceHeaderExt pShExt					= NULL;
    PSliceHeader pSh							= NULL;

    if (pCtx->pDec == NULL) {
      pCtx->pDec = PrefetchPic (pCtx->pPicBuff[0]);

      if (NULL == pCtx->pDec) {
        WelsLog (pCtx, WELS_LOG_ERROR, "DecodeCurrentAccessUnit()::::::PrefetchPic ERROR, pSps->iNumRefFrames:%d.\n",
                 pCtx->pSps->iNumRefFrames);
        pCtx->iErrorCode |= dsOutOfMemory;
        return ERR_INFO_REF_COUNT_OVERFLOW;
      }
    }

#ifdef NO_WAITING_AU
    //For fixing the nal lossing issue
    if ((pCtx->pDec->iTotalNumMbRec != 0) &&
        (CheckAccessUnitBoundaryExt (&pCtx->sLastNalHdrExt, &pNalCur->sNalHeaderExt, &pCtx->sLastSliceHeader,
                                     &pNalCur->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader))) {
      pCtx->pDec->iTotalNumMbRec = 0;
    }
#else
    //initialize at the starting of AU.
    pCtx->pDec->iTotalNumMbRec = 0;
#endif
    if (pCtx->pDec->iTotalNumMbRec == 0) { //Picture start to decode
      for (int32_t i = 0; i < LAYER_NUM_EXCHANGEABLE; ++ i)
        memset (pCtx->sMb.pSliceIdc[i], 0xff, (pCtx->sMb.iMbWidth * pCtx->sMb.iMbHeight * sizeof (int32_t)));
    }
    GetI4LumaIChromaAddrTable (pCtx->iDecBlockOffsetArray, pCtx->pDec->iLinesize[0], pCtx->pDec->iLinesize[1]);

    if (pNalCur->sNalHeaderExt.uiLayerDqId > kuiTargetLayerDqId) { // confirmed pNalCur will never be NULL
      break;	// Per formance it need not to decode the remaining bits any more due to given uiLayerDqId required, 9/2/2009
    }

    memset (&pLayerInfo, 0, sizeof (SLayerInfo));

    /*
     *	Loop decoding for slices (even FMO and/ multiple slices) within a dq layer
     */
    while (iIdx <= iEndIdx) {
      bool         bReconstructSlice;
      iCurrIdQ	= pNalCur->sNalHeaderExt.uiQualityId;
      iCurrIdD	= pNalCur->sNalHeaderExt.uiDependencyId;
      pSh		= &pNalCur->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader;
      pShExt	= &pNalCur->sNalData.sVclNal.sSliceHeaderExt;

      bReconstructSlice = CheckSliceNeedReconstruct (iCurrIdD, iCurrIdQ, pShExt->bStoreRefBasePicFlag,
                          kuiDependencyIdMax, pNalCur->sNalHeaderExt.uiLayerDqId, kuiTargetLayerDqId);

      memcpy (&pLayerInfo.sNalHeaderExt, &pNalCur->sNalHeaderExt, sizeof (SNalUnitHeaderExt)); //confirmed_safe_unsafe_usage

      pCtx->pDec->iFrameNum = pSh->iFrameNum;

      memcpy (&pLayerInfo.sSliceInLayer.sSliceHeaderExt, pShExt, sizeof (SSliceHeaderExt)); //confirmed_safe_unsafe_usage
      pLayerInfo.sSliceInLayer.bSliceHeaderExtFlag	= pNalCur->sNalData.sVclNal.bSliceHeaderExtFlag;
      pLayerInfo.sSliceInLayer.eSliceType			= pSh->eSliceType;
      pLayerInfo.sSliceInLayer.iLastMbQp			= pSh->iSliceQp;
      dq_cur->pBitStringAux	= &pNalCur->sNalData.sVclNal.sSliceBitsRead;

      uiNalRefIdc	= pNalCur->sNalHeaderExt.sNalUnitHeader.uiNalRefIdc;

      iPpsId	= pSh->iPpsId;

      pLayerInfo.pPps = pSh->pPps;
      pLayerInfo.pSps = pSh->pSps;
      pLayerInfo.pSubsetSps = pShExt->pSubsetSps;

      pCtx->pFmo = &pCtx->sFmoList[iPpsId];
      if (!FmoParamUpdate (pCtx->pFmo, pLayerInfo.pSps, pLayerInfo.pPps, &pCtx->iActiveFmoNum)) {
        pCtx->iErrorCode |= dsBitstreamError;
        WelsLog (pCtx, WELS_LOG_WARNING, "DecodeCurrentAccessUnit(), FmoParamUpdate failed, eSliceType: %d.\n",
                 pSh->eSliceType);
        return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_FMO_INIT_FAIL);
      }

      bFreshSliceAvailable	= (iCurrIdD != iLastIdD
                               || iCurrIdQ != iLastIdQ);	// do not need condition of (first_mb == 0) due multiple slices might be disorder

      WelsDqLayerDecodeStart (pCtx, pNalCur, pLayerInfo.pSps, pLayerInfo.pPps);

      if (iCurrIdQ == BASE_QUALITY_ID) {
        ST64 (iRefCount, LD64 (pLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.uiRefCount));
      }

      if ((iLastIdD < 0) ||  //case 1: first layer
          (iLastIdD == iCurrIdD)) { //case 2: same uiDId
        InitDqLayerInfo (dq_cur, &pLayerInfo, pNalCur, pCtx->pDec);

        if (!dq_cur->sLayerInfo.pSps->bGapsInFrameNumValueAllowedFlag) {
          const bool kbIdrFlag = dq_cur->sLayerInfo.sNalHeaderExt.bIdrFlag
                                   || (dq_cur->sLayerInfo.sNalHeaderExt.sNalUnitHeader.eNalUnitType == NAL_UNIT_CODED_SLICE_IDR);
          // Subclause 8.2.5.2 Decoding process for gaps in frame_num
          if (!kbIdrFlag  &&
              pSh->iFrameNum != pCtx->iPrevFrameNum &&
              pSh->iFrameNum != ((pCtx->iPrevFrameNum + 1) & ((1 << dq_cur->sLayerInfo.pSps->uiLog2MaxFrameNum) - 1))) {
            WelsLog (pCtx, WELS_LOG_WARNING,
                     "referencing pictures lost due frame gaps exist, prev_frame_num: %d, curr_frame_num: %d\n", pCtx->iPrevFrameNum,
                     pSh->iFrameNum);

#ifdef LONG_TERM_REF
            pCtx->bParamSetsLostFlag = true;
#else
            pCtx->bReferenceLostAtT0Flag = true;
#endif
            ResetParameterSetsState (pCtx);

            pCtx->iErrorCode |= dsRefLost;
            return ERR_INFO_REFERENCE_PIC_LOST;
          }
        }

        if (iCurrIdD == kuiDependencyIdMax && iCurrIdQ == BASE_QUALITY_ID) {
          iRet = InitRefPicList (pCtx, uiNalRefIdc, bFreshSliceAvailable, pSh->iPicOrderCntLsb);
          if (iRet) {
            HandleReferenceLost (pCtx, pNalCur);
            WelsLog (pCtx, WELS_LOG_WARNING, "reference picture introduced by this frame is lost during transmission! uiTId: %d\n",
                     pNalCur->sNalHeaderExt.uiTemporalId);
            return iRet;
          }
        }

        iRet = WelsDecodeSlice (pCtx, bFreshSliceAvailable, pNalCur);

        //Output good store_base reconstruction when enhancement quality layer occurred error for MGS key picture case
        if (iRet != ERR_NONE) {
          WelsLog (pCtx, WELS_LOG_WARNING, "DecodeCurrentAccessUnit() failed (%d) in frame: %d uiDId: %d uiQId: %d\n",
                   iRet, pSh->iFrameNum, iCurrIdD, iCurrIdQ);
          HandleReferenceLostL0 (pCtx, pNalCur);
          return iRet;
        }
        if (bReconstructSlice)	{
          if (WelsDecodeConstructSlice (pCtx, pNalCur)) {
            return -1;
          }
        }
      }
#if defined (_DEBUG) &&  !defined (CODEC_FOR_TESTBED)
      fprintf (stderr, "cur_frame : %d	iCurrIdD : %d\n ",
               dq_cur->sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.iFrameNum, iCurrIdD);
#endif//#if !CODEC_FOR_TESTBED
      iLastIdD	= iCurrIdD;
      iLastIdQ	= iCurrIdQ;

      //pNalUnitsList overflow.
      ++ iIdx;
      if (iIdx <= iEndIdx) {
        pNalCur	= pCurAu->pNalUnitsList[iIdx];
      } else {
        pNalCur	= NULL;
      }

      if (pNalCur == NULL ||
          iLastIdD != pNalCur->sNalHeaderExt.uiDependencyId ||
          iLastIdQ != pNalCur->sNalHeaderExt.uiQualityId)
        break;
    }

    // A dq layer decoded here
#if defined (_DEBUG) &&  !defined (CODEC_FOR_TESTBED)
#undef fprintf
    fprintf (stderr, "POC: #%d, FRAME: #%d, D: %d, Q: %d, T: %d, P: %d,	%d\n",
             pSh->iPicOrderCntLsb, pSh->iFrameNum, iCurrIdD, iCurrIdQ, dq_cur->sLayerInfo.sNalHeaderExt.uiTemporalId,
             dq_cur->sLayerInfo.sNalHeaderExt.uiPriorityId, dq_cur->sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.iSliceQp);
#endif//#if !CODEC_FOR_TESTBED

    if (dq_cur->uiLayerDqId == kuiTargetLayerDqId) {
      if (DecodeFrameConstruction (pCtx, ppDst, pDstLen, pWidth, pHeight, pDstInfo)) {
#ifdef NO_WAITING_AU
        memcpy (&pCtx->sLastNalHdrExt, &pCurAu->pNalUnitsList[iIdx - 1]->sNalHeaderExt, sizeof (SNalUnitHeaderExt));
        memcpy (&pCtx->sLastSliceHeader, &pCurAu->pNalUnitsList[iIdx - 1]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader,
                sizeof (SSliceHeader));
        return ERR_NONE;
#else
        pCtx->iErrorCode |= dsBitstreamError;
        return -1;
#endif

      }
      if ((uiNalRefIdc > 0) && (iCurrIdQ || (!dq_cur->bStoreRefBasePicFlag))) {
        WelsMarkAsRef (pCtx, false);
        ExpandReferencingPicture (pCtx->pDec, pCtx->sExpandPicFunc.pExpandLumaPicture,
                                  pCtx->sExpandPicFunc.pExpandChromaPicture);
        pCtx->pDec = NULL;
      }
    }

    if ((iCurrIdD == kuiDependencyIdMax) && (iCurrIdQ == BASE_QUALITY_ID) && (dq_cur->bStoreRefBasePicFlag)) {
      pStoreBasePic = pCtx->pDec;

      if (uiNalRefIdc > 0) {
        WelsMarkAsRef (pCtx, true);
        ExpandReferencingPicture (pCtx->pDec, pCtx->sExpandPicFunc.pExpandLumaPicture,
                                  pCtx->sExpandPicFunc.pExpandChromaPicture);
        pCtx->pDec = NULL;
      }
    }
    // need update frame_num due current frame is well decoded
    pCtx->iPrevFrameNum	= pSh->iFrameNum;
    if (pCtx->bLastHasMmco5)
      pCtx->iPrevFrameNum = 0;
  }

  return ERR_NONE;
}

} // namespace WelsDec
