/*!
 * \copy
 *     Copyright (c)  2008-2013, Cisco Systems
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
 *  manage_dec_ref.cpp
 *
 *  Abstract
 *      Implementation for managing reference picture
 *
 *  History
 *      07/21/2008 Created
 *
 *****************************************************************************/

#include "manage_dec_ref.h"
#include "error_concealment.h"
#include "error_code.h"

namespace WelsDec {

static PPicture WelsDelShortFromList (PRefPic pRefPic, int32_t iFrameNum);
static PPicture WelsDelLongFromList (PRefPic pRefPic, uint32_t uiLongTermFrameIdx);
static PPicture WelsDelShortFromListSetUnref (PRefPic pRefPic, int32_t iFrameNum);
static PPicture WelsDelLongFromListSetUnref (PRefPic pRefPic, uint32_t uiLongTermFrameIdx);

static int32_t MMCO (PWelsDecoderContext pCtx, PRefPicMarking pRefPicMarking);
static int32_t MMCOProcess (PWelsDecoderContext pCtx, uint32_t uiMmcoType,
                            int32_t iShortFrameNum, uint32_t uiLongTermPicNum, int32_t iLongTermFrameIdx, int32_t iMaxLongTermFrameIdx);
static int32_t SlidingWindow (PWelsDecoderContext pCtx);

static int32_t AddShortTermToList (PRefPic pRefPic, PPicture pPic);
static int32_t AddLongTermToList (PRefPic pRefPic, PPicture pPic, int32_t iLongTermFrameIdx);
static int32_t MarkAsLongTerm (PRefPic pRefPic, int32_t iFrameNum, int32_t iLongTermFrameIdx);
#ifdef LONG_TERM_REF
int32_t GetLTRFrameIndex (PRefPic pRefPic, int32_t iAncLTRFrameNum);
#endif
static int32_t RemainOneBufferInDpbForEC (PWelsDecoderContext pCtx);

static void SetUnRef (PPicture pRef) {
  if (NULL != pRef) {
    pRef->bUsedAsRef = false;
    pRef->bIsLongRef = false;
    pRef->iFrameNum = -1;
    //pRef->iFramePoc = 0;
    pRef->iLongTermFrameIdx = -1;
    pRef->uiQualityId = -1;
    pRef->uiTemporalId = -1;
    pRef->uiSpatialId = -1;
    pRef->iSpsId = -1;
    pRef->bIsComplete = false;
  }
}

//reset pRefList when
// 1.sps arrived that is new sequence starting
// 2.IDR NAL i.e. 1st layer in IDR AU

void WelsResetRefPic (PWelsDecoderContext pCtx) {
  int32_t i = 0;
  PRefPic pRefPic = &pCtx->sRefPic;
  pCtx->sRefPic.uiLongRefCount[LIST_0] = pCtx->sRefPic.uiShortRefCount[LIST_0] = 0;

  pRefPic->uiRefCount[LIST_0] = 0;

  for (i = 0; i < MAX_DPB_COUNT; i++) {
    if (pRefPic->pShortRefList[LIST_0][i] != NULL) {
      SetUnRef (pRefPic->pShortRefList[LIST_0][i]);
      pRefPic->pShortRefList[LIST_0][i] = NULL;
    }
  }
  pRefPic->uiShortRefCount[LIST_0] = 0;

  for (i = 0; i < MAX_DPB_COUNT; i++) {
    if (pRefPic->pLongRefList[LIST_0][i] != NULL) {
      SetUnRef (pRefPic->pLongRefList[LIST_0][i]);
      pRefPic->pLongRefList[LIST_0][i] = NULL;
    }
  }
  pRefPic->uiLongRefCount[LIST_0] = 0;
}

/**
 * fills the pRefPic.pRefList.
 */
int32_t WelsInitRefList (PWelsDecoderContext pCtx, int32_t iPoc) {
  int32_t i, iCount = 0;

  if ((pCtx->sRefPic.uiShortRefCount[LIST_0] + pCtx->sRefPic.uiLongRefCount[LIST_0] <= 0) && (pCtx->eSliceType != I_SLICE
      && pCtx->eSliceType != SI_SLICE)) {
    if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) { //IDR lost!, recover it for future decoding with data all set to 0
      PPicture pRef = PrefetchPic (pCtx->pPicBuff[0]);
      if (pRef != NULL) {
        // IDR lost, set new
        pRef->bIsComplete = false; // Set complete flag to false for lost IDR ref picture
        pRef->iSpsId = pCtx->pSps->iSpsId;
        pRef->iPpsId = pCtx->pPps->iPpsId;
        pCtx->iErrorCode |= dsDataErrorConcealed;
        bool bCopyPrevious = ((ERROR_CON_FRAME_COPY_CROSS_IDR == pCtx->eErrorConMethod)
                              || (ERROR_CON_SLICE_COPY_CROSS_IDR == pCtx->eErrorConMethod)
                              || (ERROR_CON_SLICE_COPY_CROSS_IDR_FREEZE_RES_CHANGE == pCtx->eErrorConMethod)
                              || (ERROR_CON_SLICE_MV_COPY_CROSS_IDR == pCtx->eErrorConMethod)
                              || (ERROR_CON_SLICE_MV_COPY_CROSS_IDR_FREEZE_RES_CHANGE == pCtx->eErrorConMethod))
                             && (NULL != pCtx->pPreviousDecodedPictureInDpb);
        bCopyPrevious = bCopyPrevious && (pRef->iWidthInPixel == pCtx->pPreviousDecodedPictureInDpb->iWidthInPixel)
                        && (pRef->iHeightInPixel == pCtx->pPreviousDecodedPictureInDpb->iHeightInPixel);

        if (!bCopyPrevious) {
          memset (pRef->pData[0], 128, pRef->iLinesize[0] * pRef->iHeightInPixel);
          memset (pRef->pData[1], 128, pRef->iLinesize[1] * pRef->iHeightInPixel / 2);
          memset (pRef->pData[2], 128, pRef->iLinesize[2] * pRef->iHeightInPixel / 2);
        } else if (pRef == pCtx->pPreviousDecodedPictureInDpb) {
          WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "WelsInitRefList()::EC memcpy overlap.");
        } else {
          memcpy (pRef->pData[0], pCtx->pPreviousDecodedPictureInDpb->pData[0], pRef->iLinesize[0] * pRef->iHeightInPixel);
          memcpy (pRef->pData[1], pCtx->pPreviousDecodedPictureInDpb->pData[1], pRef->iLinesize[1] * pRef->iHeightInPixel / 2);
          memcpy (pRef->pData[2], pCtx->pPreviousDecodedPictureInDpb->pData[2], pRef->iLinesize[2] * pRef->iHeightInPixel / 2);
        }
        pRef->iFrameNum = 0;
        pRef->iFramePoc = 0;
        pRef->uiTemporalId = pRef->uiQualityId = 0;
        ExpandReferencingPicture (pRef->pData, pRef->iWidthInPixel, pRef->iHeightInPixel, pRef->iLinesize,
                                  pCtx->sExpandPicFunc.pfExpandLumaPicture, pCtx->sExpandPicFunc.pfExpandChromaPicture);
        AddShortTermToList (&pCtx->sRefPic, pRef);
      } else {
        WelsLog (& (pCtx->sLogCtx), WELS_LOG_ERROR, "WelsInitRefList()::PrefetchPic for EC errors.");
        pCtx->iErrorCode |= dsOutOfMemory;
        return ERR_INFO_REF_COUNT_OVERFLOW;
      }
    }
  }

  PPicture* ppShoreRefList = pCtx->sRefPic.pShortRefList[LIST_0];
  PPicture* ppLongRefList  = pCtx->sRefPic.pLongRefList[LIST_0];
  memset (pCtx->sRefPic.pRefList[LIST_0], 0, MAX_DPB_COUNT * sizeof (PPicture));
  //short
  for (i = 0; i < pCtx->sRefPic.uiShortRefCount[LIST_0]; ++i) {
    pCtx->sRefPic.pRefList[LIST_0][iCount++ ] = ppShoreRefList[i];
  }

  //long
  for (i = 0; i < pCtx->sRefPic.uiLongRefCount[LIST_0] ; ++i) {
    pCtx->sRefPic.pRefList[LIST_0][iCount++  ] = ppLongRefList[i];
  }
  pCtx->sRefPic.uiRefCount[LIST_0] = iCount;

  return ERR_NONE;
}

int32_t WelsReorderRefList (PWelsDecoderContext pCtx) {
  PRefPicListReorderSyn pRefPicListReorderSyn = pCtx->pCurDqLayer->pRefPicListReordering;
  PNalUnitHeaderExt pNalHeaderExt = &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt;
  PSliceHeader pSliceHeader = &pCtx->pCurDqLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader;
  PPicture pPic = NULL;
  PPicture* ppRefList = pCtx->sRefPic.pRefList[LIST_0];
  int32_t iMaxRefIdx = pCtx->pSps->iNumRefFrames;
  int32_t iRefCount = pCtx->sRefPic.uiRefCount[LIST_0];
  int32_t iPredFrameNum = pSliceHeader->iFrameNum;
  int32_t iMaxPicNum = 1 << pSliceHeader->pSps->uiLog2MaxFrameNum;
  int32_t iAbsDiffPicNum = -1;
  int32_t iReorderingIndex = 0;
  int32_t i = 0;

  if (pCtx->eSliceType == I_SLICE || pCtx->eSliceType == SI_SLICE) {
    return ERR_NONE;
  }

  if (iRefCount <= 0) {
    pCtx->iErrorCode = dsNoParamSets; //No any reference for decoding, SHOULD request IDR
    return ERR_INFO_REFERENCE_PIC_LOST;
  }

  if (pRefPicListReorderSyn->bRefPicListReorderingFlag[LIST_0]) {
    while ((iReorderingIndex < iMaxRefIdx)
           && (pRefPicListReorderSyn->sReorderingSyn[LIST_0][iReorderingIndex].uiReorderingOfPicNumsIdc != 3)) {
      uint16_t uiReorderingOfPicNumsIdc =
        pRefPicListReorderSyn->sReorderingSyn[LIST_0][iReorderingIndex].uiReorderingOfPicNumsIdc;
      if (uiReorderingOfPicNumsIdc < 2) {
        iAbsDiffPicNum = pRefPicListReorderSyn->sReorderingSyn[LIST_0][iReorderingIndex].uiAbsDiffPicNumMinus1 + 1;

        if (uiReorderingOfPicNumsIdc == 0) {
          iPredFrameNum -= iAbsDiffPicNum;
        } else {
          iPredFrameNum += iAbsDiffPicNum;
        }
        iPredFrameNum &= iMaxPicNum - 1;

        for (i = iMaxRefIdx - 1; i >= 0; i--) {
          if (ppRefList[i] != NULL && ppRefList[i]->iFrameNum == iPredFrameNum && !ppRefList[i]->bIsLongRef) {
            if ((pNalHeaderExt->uiQualityId == ppRefList[i]->uiQualityId)
                && (pSliceHeader->iSpsId != ppRefList[i]->iSpsId)) {   //check;
              WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "WelsReorderRefList()::::BASE LAYER::::iSpsId:%d, ref_sps_id:%d",
                       pSliceHeader->iSpsId, ppRefList[i]->iSpsId);
              pCtx->iErrorCode = dsNoParamSets; //cross-IDR reference frame selection, SHOULD request IDR.--
              return ERR_INFO_REFERENCE_PIC_LOST;
            } else {
              break;
            }
          }
        }

      } else if (uiReorderingOfPicNumsIdc == 2) {
        for (i = iMaxRefIdx - 1; i >= 0; i--) {
          if (ppRefList[i] != NULL && ppRefList[i]->bIsLongRef
              && ppRefList[i]->iLongTermFrameIdx ==
              pRefPicListReorderSyn->sReorderingSyn[LIST_0][iReorderingIndex].uiLongTermPicNum) {
            if ((pNalHeaderExt->uiQualityId == ppRefList[i]->uiQualityId)
                && (pSliceHeader->iSpsId != ppRefList[i]->iSpsId)) {    //check;
              WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "WelsReorderRefList()::::BASE LAYER::::iSpsId:%d, ref_sps_id:%d",
                       pSliceHeader->iSpsId, ppRefList[i]->iSpsId);
              pCtx->iErrorCode = dsNoParamSets; //cross-IDR reference frame selection, SHOULD request IDR.--
              return ERR_INFO_REFERENCE_PIC_LOST;
            } else {
              break;
            }
          }
        }
      }
      if (i < 0) {
        return ERR_INFO_REFERENCE_PIC_LOST;
      }
      pPic = ppRefList[i];
      if (i > iReorderingIndex) {
        memmove (&ppRefList[1 + iReorderingIndex], &ppRefList[iReorderingIndex],
                 (i - iReorderingIndex)*sizeof (PPicture)); //confirmed_safe_unsafe_usage
      } else if (i < iReorderingIndex) {
        memmove (&ppRefList[1 + iReorderingIndex], &ppRefList[iReorderingIndex],
                 (iMaxRefIdx - iReorderingIndex)*sizeof (PPicture));
      }
      ppRefList[iReorderingIndex] = pPic;
      iReorderingIndex++;
    }
  }
  return ERR_NONE;
}

int32_t WelsMarkAsRef (PWelsDecoderContext pCtx) {
  PRefPic pRefPic = &pCtx->sRefPic;
  PRefPicMarking pRefPicMarking = pCtx->pCurDqLayer->pRefPicMarking;
  PAccessUnit pCurAU = pCtx->pAccessUnitList;
  bool bIsIDRAU = false;
  uint32_t j;

  int32_t iRet = ERR_NONE;

  pCtx->pDec->uiQualityId = pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt.uiQualityId;
  pCtx->pDec->uiTemporalId = pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt.uiTemporalId;
  pCtx->pDec->iSpsId = pCtx->pSps->iSpsId;
  pCtx->pDec->iPpsId = pCtx->pPps->iPpsId;

  for (j = pCurAU->uiStartPos; j <= pCurAU->uiEndPos; j++) {
    if (pCurAU->pNalUnitsList[j]->sNalHeaderExt.sNalUnitHeader.eNalUnitType == NAL_UNIT_CODED_SLICE_IDR
        || pCurAU->pNalUnitsList[j]->sNalHeaderExt.bIdrFlag) {
      bIsIDRAU = true;
      break;
    }
  }
  if (bIsIDRAU) {
    if (pRefPicMarking->bLongTermRefFlag) {
      pCtx->sRefPic.iMaxLongTermFrameIdx = 0;
      AddLongTermToList (pRefPic, pCtx->pDec, 0);
    } else {
      pCtx->sRefPic.iMaxLongTermFrameIdx = -1;
    }
  } else {
    if (pRefPicMarking->bAdaptiveRefPicMarkingModeFlag) {
      iRet = MMCO (pCtx, pRefPicMarking);
      if (iRet != ERR_NONE) {
        if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
          iRet = RemainOneBufferInDpbForEC (pCtx);
          WELS_VERIFY_RETURN_IF (iRet, iRet);
        } else {
          return iRet;
        }
      }

      if (pCtx->bLastHasMmco5) {
        pCtx->pDec->iFrameNum = 0;
        pCtx->pDec->iFramePoc = 0;
      }

    } else {
      iRet = SlidingWindow (pCtx);
      if (iRet != ERR_NONE) {
        if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
          iRet = RemainOneBufferInDpbForEC (pCtx);
          WELS_VERIFY_RETURN_IF (iRet, iRet);
        } else {
          return iRet;
        }
      }
    }
  }

  if (!pCtx->pDec->bIsLongRef) {
    if (pRefPic->uiLongRefCount[LIST_0] + pRefPic->uiShortRefCount[LIST_0] >= WELS_MAX (1, pCtx->pSps->iNumRefFrames)) {
      if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
        iRet = RemainOneBufferInDpbForEC (pCtx);
        WELS_VERIFY_RETURN_IF (iRet, iRet);
      } else {
        return ERR_INFO_INVALID_MMCO_REF_NUM_OVERFLOW;
      }
    }
    iRet = AddShortTermToList (pRefPic, pCtx->pDec);
  }

  return iRet;
}

static int32_t MMCO (PWelsDecoderContext pCtx, PRefPicMarking pRefPicMarking) {
  PSps pSps = pCtx->pCurDqLayer->sLayerInfo.pSps;
  int32_t i = 0;
  int32_t iRet = ERR_NONE;
  for (i = 0; pRefPicMarking->sMmcoRef[i].uiMmcoType != MMCO_END; i++) {
    uint32_t uiMmcoType = pRefPicMarking->sMmcoRef[i].uiMmcoType;
    int32_t iShortFrameNum = (pCtx->iFrameNum - pRefPicMarking->sMmcoRef[i].iDiffOfPicNum) & ((
                               1 << pSps->uiLog2MaxFrameNum) - 1);
    uint32_t uiLongTermPicNum = pRefPicMarking->sMmcoRef[i].uiLongTermPicNum;
    int32_t iLongTermFrameIdx = pRefPicMarking->sMmcoRef[i].iLongTermFrameIdx;
    int32_t iMaxLongTermFrameIdx = pRefPicMarking->sMmcoRef[i].iMaxLongTermFrameIdx;
    if (uiMmcoType > MMCO_LONG) {
      return ERR_INFO_INVALID_MMCO_OPCODE_BASE;
    }
    iRet = MMCOProcess (pCtx, uiMmcoType, iShortFrameNum, uiLongTermPicNum, iLongTermFrameIdx, iMaxLongTermFrameIdx);
    if (iRet != ERR_NONE) {
      return iRet;
    }
  }

  return ERR_NONE;
}
static int32_t MMCOProcess (PWelsDecoderContext pCtx, uint32_t uiMmcoType,
                            int32_t iShortFrameNum, uint32_t uiLongTermPicNum , int32_t iLongTermFrameIdx, int32_t iMaxLongTermFrameIdx) {
  PRefPic pRefPic = &pCtx->sRefPic;
  PPicture pPic = NULL;
  int32_t i = 0;
  int32_t iRet = ERR_NONE;

  switch (uiMmcoType) {
  case MMCO_SHORT2UNUSED:
    pPic = WelsDelShortFromListSetUnref (pRefPic, iShortFrameNum);
    if (pPic == NULL) {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "MMCO_SHORT2UNUSED: delete a empty entry from short term list");
    }
    break;
  case MMCO_LONG2UNUSED:
    pPic = WelsDelLongFromListSetUnref (pRefPic, uiLongTermPicNum);
    if (pPic == NULL) {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "MMCO_LONG2UNUSED: delete a empty entry from long term list");
    }
    break;
  case MMCO_SHORT2LONG:
    if (iLongTermFrameIdx > pRefPic->iMaxLongTermFrameIdx) {
      return ERR_INFO_INVALID_MMCO_LONG_TERM_IDX_EXCEED_MAX;
    }
    pPic = WelsDelShortFromList (pRefPic, iShortFrameNum);
    if (pPic == NULL) {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "MMCO_LONG2LONG: delete a empty entry from short term list");
      break;
    }
    WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx);
#ifdef LONG_TERM_REF
    pCtx->bCurAuContainLtrMarkSeFlag = true;
    pCtx->iFrameNumOfAuMarkedLtr      = iShortFrameNum;
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO, "ex_mark_avc():::MMCO_SHORT2LONG:::LTR marking....iFrameNum: %d",
             pCtx->iFrameNumOfAuMarkedLtr);
#endif

    MarkAsLongTerm (pRefPic, iShortFrameNum, iLongTermFrameIdx);
    break;
  case MMCO_SET_MAX_LONG:
    pRefPic->iMaxLongTermFrameIdx = iMaxLongTermFrameIdx;
    for (i = 0 ; i < pRefPic->uiLongRefCount[LIST_0]; i++) {
      if (pRefPic->pLongRefList[LIST_0][i]->iLongTermFrameIdx > pRefPic->iMaxLongTermFrameIdx) {
        WelsDelLongFromListSetUnref (pRefPic, pRefPic->pLongRefList[LIST_0][i]->iLongTermFrameIdx);
      }
    }
    break;
  case MMCO_RESET:
    WelsResetRefPic (pCtx);
    pCtx->bLastHasMmco5 = true;
    break;
  case MMCO_LONG:
    if (iLongTermFrameIdx > pRefPic->iMaxLongTermFrameIdx) {
      return ERR_INFO_INVALID_MMCO_LONG_TERM_IDX_EXCEED_MAX;
    }
    WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx);
    if (pRefPic->uiLongRefCount[LIST_0] + pRefPic->uiShortRefCount[LIST_0] >= WELS_MAX (1, pCtx->pSps->iNumRefFrames)) {
      return ERR_INFO_INVALID_MMCO_REF_NUM_OVERFLOW;
    }
#ifdef LONG_TERM_REF
    pCtx->bCurAuContainLtrMarkSeFlag = true;
    pCtx->iFrameNumOfAuMarkedLtr      = pCtx->iFrameNum;
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO, "ex_mark_avc():::MMCO_LONG:::LTR marking....iFrameNum: %d",
             pCtx->iFrameNum);
#endif
    iRet = AddLongTermToList (pRefPic, pCtx->pDec, iLongTermFrameIdx);
    break;
  default :
    break;
  }

  return iRet;
}

static int32_t SlidingWindow (PWelsDecoderContext pCtx) {
  PRefPic pRefPic = &pCtx->sRefPic;
  PPicture pPic = NULL;
  int32_t i = 0;

  if (pCtx->sRefPic.uiShortRefCount[LIST_0] + pCtx->sRefPic.uiLongRefCount[LIST_0] >= pCtx->pSps->iNumRefFrames) {
    if (pCtx->sRefPic.uiShortRefCount[LIST_0] == 0) {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_ERROR, "No reference picture in short term list when sliding window");
      return ERR_INFO_INVALID_MMCO_REF_NUM_NOT_ENOUGH;
    }
    for (i = pRefPic->uiShortRefCount[LIST_0] - 1; i >= 0; i--) {
      pPic = WelsDelShortFromList (pRefPic, pRefPic->pShortRefList[LIST_0][i]->iFrameNum);
      if (pPic) {
        SetUnRef (pPic);
        break;
      } else {
        return ERR_INFO_INVALID_MMCO_REF_NUM_OVERFLOW;
      }
    }
  }
  return ERR_NONE;
}

static PPicture WelsDelShortFromList (PRefPic pRefPic, int32_t iFrameNum) {
  int32_t i = 0;
  int32_t iMoveSize = 0;
  PPicture pPic = NULL;

  for (i = 0; i < pRefPic->uiShortRefCount[LIST_0]; i++) {
    if (pRefPic->pShortRefList[LIST_0][i]->iFrameNum == iFrameNum) {
      iMoveSize = pRefPic->uiShortRefCount[LIST_0] - i - 1;
      pRefPic->pShortRefList[LIST_0][i]->bUsedAsRef = false;
      pPic = pRefPic->pShortRefList[LIST_0][i];
      pRefPic->pShortRefList[LIST_0][i] = NULL;
      if (iMoveSize > 0) {
        memmove (&pRefPic->pShortRefList[LIST_0][i], &pRefPic->pShortRefList[LIST_0][i + 1],
                 iMoveSize * sizeof (PPicture)); //confirmed_safe_unsafe_usage
      }
      pRefPic->uiShortRefCount[LIST_0]--;
      pRefPic->pShortRefList[LIST_0][pRefPic->uiShortRefCount[LIST_0]] = NULL;
      break;
    }
  }

  return pPic;
}

static PPicture WelsDelShortFromListSetUnref (PRefPic pRefPic, int32_t iFrameNum) {
  PPicture pPic = WelsDelShortFromList (pRefPic, iFrameNum);
  if (pPic) {
    SetUnRef (pPic);
  }
  return pPic;
}

static PPicture WelsDelLongFromList (PRefPic pRefPic, uint32_t uiLongTermFrameIdx) {
  PPicture pPic = NULL;
  int32_t i = 0;
  for (i = 0; i < pRefPic->uiLongRefCount[LIST_0]; i++) {
    pPic = pRefPic->pLongRefList[LIST_0][i];
    if (pPic->iLongTermFrameIdx == (int32_t)uiLongTermFrameIdx) {
      int32_t iMoveSize = pRefPic->uiLongRefCount[LIST_0] - i - 1;
      pPic->bUsedAsRef = false;
      pPic->bIsLongRef = false;
      if (iMoveSize > 0) {
        memmove (&pRefPic->pLongRefList[LIST_0][i], &pRefPic->pLongRefList[LIST_0][i + 1],
                 iMoveSize * sizeof (PPicture)); //confirmed_safe_unsafe_usage
      }
      pRefPic->uiLongRefCount[LIST_0]--;
      pRefPic->pLongRefList[LIST_0][pRefPic->uiLongRefCount[LIST_0]] = NULL;
      return pPic;
    }
  }
  return NULL;
}

static PPicture WelsDelLongFromListSetUnref (PRefPic pRefPic, uint32_t uiLongTermFrameIdx) {
  PPicture pPic = WelsDelLongFromList (pRefPic, uiLongTermFrameIdx);
  if (pPic) {
    SetUnRef (pPic);
  }
  return pPic;
}

static int32_t AddShortTermToList (PRefPic pRefPic, PPicture pPic) {
  pPic->bUsedAsRef = true;
  pPic->bIsLongRef = false;
  pPic->iLongTermFrameIdx = -1;
  if (pRefPic->uiShortRefCount[LIST_0] > 0) {
    // Check the duplicate frame_num in short ref list
    for (int32_t iPos = 0; iPos < pRefPic->uiShortRefCount[LIST_0]; iPos++) {
      if (pPic->iFrameNum == pRefPic->pShortRefList[LIST_0][iPos]->iFrameNum) {
        // Replace the previous ref pic with the new one with the same frame_num
        pRefPic->pShortRefList[LIST_0][iPos] = pPic;
        return ERR_INFO_DUPLICATE_FRAME_NUM;
      }
    }

    memmove (&pRefPic->pShortRefList[LIST_0][1], &pRefPic->pShortRefList[LIST_0][0],
             pRefPic->uiShortRefCount[LIST_0]*sizeof (PPicture));//confirmed_safe_unsafe_usage
  }
  pRefPic->pShortRefList[LIST_0][0] = pPic;
  pRefPic->uiShortRefCount[LIST_0]++;
  return ERR_NONE;
}

static int32_t AddLongTermToList (PRefPic pRefPic, PPicture pPic, int32_t iLongTermFrameIdx) {
  int32_t i = 0;

  pPic->bUsedAsRef = true;
  pPic->bIsLongRef = true;
  pPic->iLongTermFrameIdx = iLongTermFrameIdx;
  if (pRefPic->uiLongRefCount[LIST_0] == 0) {
    pRefPic->pLongRefList[LIST_0][pRefPic->uiLongRefCount[LIST_0]] = pPic;
  } else {
    for (i = 0; i < pRefPic->uiLongRefCount[LIST_0]; i++) {
      if (pRefPic->pLongRefList[LIST_0][i]->iLongTermFrameIdx > pPic->iLongTermFrameIdx) {
        break;
      }
    }
    memmove (&pRefPic->pLongRefList[LIST_0][i + 1], &pRefPic->pLongRefList[LIST_0][i],
             (pRefPic->uiLongRefCount[LIST_0] - i)*sizeof (PPicture)); //confirmed_safe_unsafe_usage
    pRefPic->pLongRefList[LIST_0][i] = pPic;
  }

  pRefPic->uiLongRefCount[LIST_0]++;
  return ERR_NONE;
}

static int32_t MarkAsLongTerm (PRefPic pRefPic, int32_t iFrameNum, int32_t iLongTermFrameIdx) {
  PPicture pPic = NULL;
  int32_t i = 0;
  int32_t iRet = ERR_NONE;
  WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx);

  for (i = 0; i < pRefPic->uiRefCount[LIST_0]; i++) {
    pPic = pRefPic->pRefList[LIST_0][i];
    if (pPic->iFrameNum == iFrameNum && !pPic->bIsLongRef) {
      iRet = AddLongTermToList (pRefPic, pPic, iLongTermFrameIdx);
      break;
    }
  }

  return iRet;
}

#ifdef LONG_TERM_REF
int32_t GetLTRFrameIndex (PRefPic pRefPic, int32_t iAncLTRFrameNum) {
  int32_t iLTRFrameIndex = -1;
  PPicture pPic;
  for (int i = 0; i < pRefPic->uiLongRefCount[0]; ++i) {
    pPic = pRefPic->pLongRefList[LIST_0][i];
    if (pPic->iFrameNum == iAncLTRFrameNum) {
      return (pPic->iLongTermFrameIdx);
    }
  }
  return iLTRFrameIndex;
}
#endif

static int32_t RemainOneBufferInDpbForEC (PWelsDecoderContext pCtx) {
  int32_t iRet = ERR_NONE;
  PRefPic pRefPic = &pCtx->sRefPic;
  if (pRefPic->uiShortRefCount[0] + pRefPic->uiLongRefCount[0] < pCtx->pSps->iNumRefFrames)
    return iRet;

  if (pRefPic->uiShortRefCount[0] > 0) {
    iRet = SlidingWindow (pCtx);
  } else { //all LTR, remove the smallest long_term_frame_idx
    int32_t iLongTermFrameIdx = 0;
    int32_t iMaxLongTermFrameIdx = pRefPic->iMaxLongTermFrameIdx;
#ifdef LONG_TERM_REF
    int32_t iCurrLTRFrameIdx = GetLTRFrameIndex (pRefPic, pCtx->iFrameNumOfAuMarkedLtr);
#endif
    while ((pRefPic->uiLongRefCount[0] >= pCtx->pSps->iNumRefFrames) && (iLongTermFrameIdx <= iMaxLongTermFrameIdx)) {
#ifdef LONG_TERM_REF
      if (iLongTermFrameIdx == iCurrLTRFrameIdx) {
        iLongTermFrameIdx++;
        continue;
      }
#endif
      WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx);
      iLongTermFrameIdx++;
    }
  }
  if (pRefPic->uiShortRefCount[0] + pRefPic->uiLongRefCount[0] >=
      pCtx->pSps->iNumRefFrames) { //fail to remain one empty buffer in DPB
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "RemainOneBufferInDpbForEC(): empty one DPB failed for EC!");
    iRet = ERR_INFO_REF_COUNT_OVERFLOW;
  }

  return iRet;
}

} // namespace WelsDec
