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
 *  manage_ref_pic.cpp
 *
 *  Abstract
 *      Implementation for managing reference picture
 *
 *  History
 *      07/21/2008 Created
 *
 *****************************************************************************/
#include <string.h>

#include "manage_dec_ref.h"
#include "error_code.h"
#include "utils.h"
#include "as264_common.h" // for LTR macro can be delete later

namespace WelsDec {

static void_t SetUnRef (PPicture pRef) {
  if (NULL != pRef) {
    pRef->bUsedAsRef = false;
    pRef->bIsLongRef = false;
    pRef->iFrameNum = -1;
    pRef->iFramePoc = 0;
    pRef->iLongTermFrameIdx = -1;
    pRef->bRefBaseFlag = 0;
    pRef->uiQualityId = -1;
    pRef->uiTemporalId = -1;
    pRef->uiSpatialId = -1;
    pRef->iSpsId = -1;
  }
}

//reset pRefList when
// 1.sps arrived that is new sequence starting
// 2.IDR NAL i.e. 1st layer in IDR AU

void_t WelsResetRefPic (PWelsDecoderContext pCtx) {
  int32_t i = 0;
  PRefPic pRefPic = &pCtx->sRefPic;
  pCtx->sRefPic.uiLongRefCount[0] = pCtx->sRefPic.uiShortRefCount[0] = 0;

  pRefPic->uiRefCount[LIST_0]	= 0;

  for (i = 0; i < MAX_SHORT_REF_COUNT; i++)	{
    if (pRefPic->pShortRefList[LIST_0][i] != NULL) {
      SetUnRef (pRefPic->pShortRefList[LIST_0][i]);
      pRefPic->pShortRefList[LIST_0][i] = NULL;
    }
  }
  pRefPic->uiShortRefCount[LIST_0] = 0;

  for (i = 0; i < MAX_LONG_REF_COUNT; i++) {
    if (pRefPic->pLongRefList[LIST_0][i] != NULL)	{
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
  int32_t i, j, iCount = 0;
  const bool_t kbUseRefBasePicFlag = pCtx->pCurDqLayer->bUseRefBasePicFlag;
  PPicture* ppShoreRefList = pCtx->sRefPic.pShortRefList[LIST_0];
  PPicture* ppLongRefList  = pCtx->sRefPic.pLongRefList[LIST_0];
  memset (pCtx->sRefPic.pRefList[LIST_0], 0, MAX_REF_PIC_COUNT * sizeof (PPicture));
  //short
  for (i = 0; i < pCtx->sRefPic.uiShortRefCount[LIST_0]; ++i) {
    if (kbUseRefBasePicFlag == ppShoreRefList[i]->bRefBaseFlag) {
      pCtx->sRefPic.pRefList[LIST_0][iCount++ ] = ppShoreRefList[i];
    } else {
      for (j = 0; j < pCtx->sRefPic.uiShortRefCount[LIST_0]; ++j) {
        if (ppShoreRefList[j]->iFrameNum == ppShoreRefList[i]->iFrameNum
            && ppShoreRefList[j]->bRefBaseFlag == kbUseRefBasePicFlag) {
          break;
        }
      }
      if (j == pCtx->sRefPic.uiShortRefCount[LIST_0]) {
        pCtx->sRefPic.pRefList[LIST_0][iCount++] = ppShoreRefList[i];
      }
    }
  }

  //long
  j = 0;
  for (i = 0; i < pCtx->sRefPic.uiLongRefCount[LIST_0] ; ++i) {
    if (kbUseRefBasePicFlag == ppLongRefList[i]->bRefBaseFlag) {
      pCtx->sRefPic.pRefList[LIST_0][iCount++  ] = ppLongRefList[i];
    } else {
      for (j = 0; j < pCtx->sRefPic.uiLongRefCount[LIST_0]; ++j) {
        if (ppLongRefList[j]->iLongTermFrameIdx == ppLongRefList[i]->iLongTermFrameIdx
            && ppLongRefList[j]->bRefBaseFlag == kbUseRefBasePicFlag) {
          break;
        }
      }
      if (j == pCtx->sRefPic.uiLongRefCount[LIST_0]) {
        pCtx->sRefPic.pRefList[LIST_0][iCount++] = ppLongRefList[i];
      }
    }
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
  int32_t iRefCount = pCtx->sRefPic.uiRefCount[LIST_0];
  int32_t iPredFrameNum = pSliceHeader->iFrameNum;
  int32_t iMaxPicNum = 1 << pSliceHeader->pSps->uiLog2MaxFrameNum;
  int32_t iAbsDiffPicNum = -1;
  int32_t iReorderingIndex = 0;
  int32_t i = 0;

  if (pCtx->eSliceType == I_SLICE || pCtx->eSliceType == SI_SLICE)	{
    return ERR_NONE;
  }

  if (iRefCount <= 0) {
    pCtx->iErrorCode = dsNoParamSets; //No any reference for decoding, SHOULD request IDR
    return ERR_INFO_REFERENCE_PIC_LOST;
  }

  if (pRefPicListReorderSyn->bRefPicListReorderingFlag[LIST_0]) {
    while (pRefPicListReorderSyn->sReorderingSyn[LIST_0][iReorderingIndex].uiReorderingOfPicNumsIdc != 3) {
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

        for (i = iRefCount - 1; i >= iReorderingIndex; i--) {
          if (ppRefList[i]->iFrameNum == iPredFrameNum && !ppRefList[i]->bIsLongRef) {
            if ((pNalHeaderExt->uiQualityId == ppRefList[i]->uiQualityId)
                && (pSliceHeader->iSpsId != ppRefList[i]->iSpsId)) {   //check;
              WelsLog (pCtx, WELS_LOG_WARNING, "WelsReorderRefList()::::BASE LAYER::::iSpsId:%d, ref_sps_id:%d\n",
                       pSliceHeader->iSpsId, ppRefList[i]->iSpsId);
              pCtx->iErrorCode = dsNoParamSets;	//cross-IDR reference frame selection, SHOULD request IDR.--
              return ERR_INFO_REFERENCE_PIC_LOST;
            } else {
              break;
            }
          }
        }

      } else if (uiReorderingOfPicNumsIdc == 2) {
        for (i = iRefCount - 1; i >= iReorderingIndex; i--) {
          if (ppRefList[i]->bIsLongRef
              && ppRefList[i]->iLongTermFrameIdx ==
              pRefPicListReorderSyn->sReorderingSyn[LIST_0][iReorderingIndex].uiLongTermPicNum) {
            if ((pNalHeaderExt->uiQualityId == ppRefList[i]->uiQualityId)
                && (pSliceHeader->iSpsId != ppRefList[i]->iSpsId)) {    //check;
              WelsLog (pCtx, WELS_LOG_WARNING, "WelsReorderRefList()::::BASE LAYER::::iSpsId:%d, ref_sps_id:%d\n",
                       pSliceHeader->iSpsId, ppRefList[i]->iSpsId);
              pCtx->iErrorCode = dsNoParamSets;	//cross-IDR reference frame selection, SHOULD request IDR.--
              return ERR_INFO_REFERENCE_PIC_LOST;
            } else {
              break;
            }
          }
        }
      }
      if (i < 0)	{
        return ERR_INFO_REFERENCE_PIC_LOST;
      }
      pPic = ppRefList[i];
      memmove (&ppRefList[1 + iReorderingIndex], &ppRefList[iReorderingIndex],
               (i - iReorderingIndex)*sizeof (PPicture)); //confirmed_safe_unsafe_usage
      ppRefList[iReorderingIndex] = pPic;
      iReorderingIndex++;
    }
  }
  return ERR_NONE;
}

int32_t WelsMarkAsRef (PWelsDecoderContext pCtx, const bool_t kbRefBaseMarkingFlag) {
  PRefPic pRefPic = &pCtx->sRefPic;
  PRefPicMarking pRefPicMarking = pCtx->pCurDqLayer->pRefPicMarking;
  PRefBasePicMarking pRefPicBaseMarking = pCtx->pCurDqLayer->pRefPicBaseMarking;
  PAccessUnit pCurAU = pCtx->pAccessUnitList;
  bool_t bIsIDRAU = FALSE;
  uint32_t j;

  int32_t iRet = ERR_NONE;
  if (pCtx->pCurDqLayer->bStoreRefBasePicFlag && (pCtx->pSps->iNumRefFrames < 2)) {
    return ERR_INFO_INVALID_MMCO_REF_NUM_NOT_ENOUGH;
  }

  pCtx->pDec->bUsedAsRef = TRUE;
  pCtx->pDec->uiQualityId = pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt.uiQualityId;
  pCtx->pDec->uiTemporalId = pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt.uiTemporalId;
  pCtx->pDec->bRefBaseFlag = kbRefBaseMarkingFlag;

  for (j = pCurAU->uiStartPos; j <= pCurAU->uiEndPos; j++) {
    if (pCurAU->pNalUnitsList[j]->sNalHeaderExt.sNalUnitHeader.eNalUnitType == NAL_UNIT_CODED_SLICE_IDR
        ||	pCurAU->pNalUnitsList[j]->sNalHeaderExt.bIdrFlag) {
      bIsIDRAU = TRUE;
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
    if (pRefPicBaseMarking->bAdaptiveRefBasePicMarkingModeFlag) {
      iRet = MMCOBase (pCtx, pRefPicBaseMarking);
    }

    if (iRet != ERR_NONE) {
      return iRet;
    }

    if (pRefPicMarking->bAdaptiveRefPicMarkingModeFlag) {
      iRet = MMCO (pCtx, pRefPicMarking);
      if (pCtx->bLastHasMmco5) {
        pCtx->pDec->iFrameNum = 0;
        pCtx->pDec->iFramePoc = 0;
      }
      if (pRefPic->uiLongRefCount[LIST_0] + pRefPic->uiShortRefCount[LIST_0] > pCtx->pSps->iNumRefFrames) {
        return ERR_INFO_INVALID_MMCO_REF_NUM_OVERFLOW;
      }
    } else {
      iRet = SlidingWindow (pCtx);
    }
  }

  if (!pCtx->pDec->bIsLongRef) {
    AddShortTermToList (pRefPic, pCtx->pDec);
  }

  return iRet;
}

static int32_t MMCOBase (PWelsDecoderContext pCtx, PRefBasePicMarking pRefPicBaseMarking) {
  PSps pSps = pCtx->pCurDqLayer->sLayerInfo.pSps;
  int32_t i = 0;
  int32_t iRet = ERR_NONE;

  for (i = 0 ; pRefPicBaseMarking->mmco_base[i].uiMmcoType != MMCO_END; i++) {
    uint32_t uiMmcoType = pRefPicBaseMarking->mmco_base[i].uiMmcoType;
    int32_t iShortFrameNum = (pCtx->iFrameNum - pRefPicBaseMarking->mmco_base[i].uiDiffOfPicNums) & ((
                               1 << pSps->uiLog2MaxFrameNum) - 1);
    uint32_t uiLongTermPicNum = pRefPicBaseMarking->mmco_base[i].uiLongTermPicNum;
    if (uiMmcoType > MMCO_LONG2UNUSED)	{
      return ERR_INFO_INVALID_MMCO_OPCODE_BASE;
    }
    iRet = MMCOProcess (pCtx, uiMmcoType, TRUE, iShortFrameNum, uiLongTermPicNum, 0, 0);

    if (iRet != ERR_NONE) {
      return iRet;
    }
  }

  return ERR_NONE;
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
    if (uiMmcoType > MMCO_LONG)	{
      return ERR_INFO_INVALID_MMCO_OPCODE_BASE;
    }
    iRet = MMCOProcess (pCtx, uiMmcoType, FALSE, iShortFrameNum, uiLongTermPicNum, iLongTermFrameIdx, iMaxLongTermFrameIdx);
    if (iRet != ERR_NONE) {
      return iRet;
    }
  }

  return ERR_NONE;
}
static int32_t MMCOProcess (PWelsDecoderContext pCtx, uint32_t uiMmcoType, bool_t bRefBasePic,
                            int32_t iShortFrameNum, uint32_t uiLongTermPicNum , int32_t iLongTermFrameIdx, int32_t iMaxLongTermFrameIdx) {
  PRefPic pRefPic = &pCtx->sRefPic;
  PPicture pPic = NULL;
  int32_t i = 0;
  int32_t iRet = ERR_NONE;

  switch (uiMmcoType) {
  case MMCO_SHORT2UNUSED:
    pPic = WelsDelShortFromListSetUnref (pRefPic, iShortFrameNum, (ERemoveFlag) bRefBasePic);
    break;
  case MMCO_LONG2UNUSED:
    pPic = WelsDelLongFromListSetUnref (pRefPic, uiLongTermPicNum, (ERemoveFlag) bRefBasePic);
    break;
  case MMCO_SHORT2LONG:
    if (iLongTermFrameIdx > pRefPic->iMaxLongTermFrameIdx) {
      return ERR_INFO_INVALID_MMCO_LONG_TERM_IDX_EXCEED_MAX;
    }
    pPic = WelsDelShortFromList (pRefPic, iShortFrameNum, REMOVE_TARGET);
    WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx, REMOVE_TARGET);

    WelsDelShortFromList (pRefPic, iShortFrameNum, REMOVE_BASE);
    WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx, REMOVE_BASE);
#ifdef LONG_TERM_REF
    pCtx->bCurAuContainLtrMarkSeFlag = true;
    pCtx->iFrameNumOfAuMarkedLtr      = iShortFrameNum;
    WelsLog (pCtx, WELS_LOG_INFO, "ex_mark_avc():::MMCO_SHORT2LONG:::LTR marking....iFrameNum: %d\n",
             pCtx->iFrameNumOfAuMarkedLtr);
#endif

    MarkAsLongTerm (pRefPic, iShortFrameNum, iLongTermFrameIdx);
    break;
  case MMCO_SET_MAX_LONG:
    pRefPic->iMaxLongTermFrameIdx = iMaxLongTermFrameIdx;
    for (i = 0 ; i < pRefPic->uiLongRefCount[LIST_0]; i++) {
      if (pRefPic->pLongRefList[LIST_0][i]->iLongTermFrameIdx > pRefPic->iMaxLongTermFrameIdx) {
        WelsDelLongFromListSetUnref (pRefPic, pRefPic->pLongRefList[LIST_0][i]->iLongTermFrameIdx, REMOVE_BASE_FIRST);
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
#ifdef LONG_TERM_REF
    pCtx->bCurAuContainLtrMarkSeFlag = true;
    pCtx->iFrameNumOfAuMarkedLtr      = pCtx->iFrameNum;
    WelsLog (pCtx, WELS_LOG_INFO, "ex_mark_avc():::MMCO_LONG:::LTR marking....iFrameNum: %d\n", pCtx->iFrameNum);
#endif
    WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx, REMOVE_TARGET);
    WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx, REMOVE_BASE);
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
    for (i = pRefPic->uiShortRefCount[LIST_0] - 1; i >= 0; i--) {
      pPic = WelsDelShortFromList (pRefPic, pRefPic->pShortRefList[LIST_0][i]->iFrameNum, REMOVE_BASE_FIRST);
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

static PPicture WelsDelShortFromList (PRefPic pRefPic, int32_t iFrameNum, ERemoveFlag eRemoveFlag) {
  int32_t i = 0;
  int32_t iMoveSize = 0;
  PPicture pPic = NULL;

  for (i = 0; i < pRefPic->uiShortRefCount[LIST_0]; i++) {
    if (pRefPic->pShortRefList[LIST_0][i]->iFrameNum == iFrameNum) {
      if ((eRemoveFlag == REMOVE_TARGET && !pRefPic->pShortRefList[LIST_0][i]->bRefBaseFlag)
          || (eRemoveFlag == REMOVE_BASE && pRefPic->pShortRefList[LIST_0][i]->bRefBaseFlag)
          || (eRemoveFlag == REMOVE_BASE_FIRST)) {
        iMoveSize = pRefPic->uiShortRefCount[LIST_0] - i - 1;
        pRefPic->pShortRefList[LIST_0][i]->bUsedAsRef = false;
        pPic = pRefPic->pShortRefList[LIST_0][i];
        pRefPic->pShortRefList[LIST_0][i] = NULL;
        if (iMoveSize > 0) {
          memmove (&pRefPic->pShortRefList[LIST_0][i], &pRefPic->pShortRefList[LIST_0][i + 1],
                   iMoveSize * sizeof (PPicture)); //confirmed_safe_unsafe_usage
        }
        pRefPic->uiShortRefCount[LIST_0]--;
        pRefPic->pShortRefList[LIST_0][pRefPic->uiShortRefCount[0]] = NULL;
        break;
      }
    }
  }

  return pPic;
}

static PPicture WelsDelShortFromListSetUnref (PRefPic pRefPic, int32_t iFrameNum, ERemoveFlag eRemoveFlag) {
  PPicture pPic = WelsDelShortFromList (pRefPic, iFrameNum, eRemoveFlag);
  if (pPic) {
    SetUnRef (pPic);
  }
  return pPic;
}

static PPicture WelsDelLongFromList (PRefPic pRefPic, uint32_t uiLongTermFrameIdx, ERemoveFlag eRemoveFlag) {
  PPicture pPic = NULL;
  int32_t i = 0;
  for (i = 0; i < pRefPic->uiLongRefCount[LIST_0]; i++) {
    pPic = pRefPic->pLongRefList[LIST_0][i];
    if (pPic->iLongTermFrameIdx == (int32_t)uiLongTermFrameIdx) {
      if (((eRemoveFlag == REMOVE_TARGET) && ! (pPic->bRefBaseFlag)) || ((eRemoveFlag == REMOVE_BASE)
          && pPic->bRefBaseFlag)) {
        int32_t iMoveSize = pRefPic->uiLongRefCount[LIST_0] - i - 1;
        pPic->bUsedAsRef = FALSE;
        pPic->bIsLongRef = FALSE;
        if (iMoveSize > 0) {
          memmove (&pRefPic->pLongRefList[LIST_0][i], &pRefPic->pLongRefList[LIST_0][i + 1],
                   iMoveSize * sizeof (PPicture)); //confirmed_safe_unsafe_usage
        }
        pRefPic->uiLongRefCount[LIST_0]--;
        pRefPic->pLongRefList[LIST_0][pRefPic->uiLongRefCount[LIST_0]] = NULL;
        return pPic;
      }
    }
  }
  return NULL;
}

static PPicture WelsDelLongFromListSetUnref (PRefPic pRefPic, uint32_t uiLongTermFrameIdx, ERemoveFlag eRemoveFlag) {
  PPicture pPic = WelsDelLongFromList (pRefPic, uiLongTermFrameIdx, eRemoveFlag);
  if (pPic) {
    SetUnRef (pPic);
  }
  return pPic;
}

static int32_t AddShortTermToList (PRefPic pRefPic, PPicture pPic) {
  pPic->bUsedAsRef = TRUE;
  pPic->bIsLongRef = FALSE;
  pPic->iLongTermFrameIdx = -1;
  if (pRefPic->uiShortRefCount[LIST_0] > 0)	{
    memmove (&pRefPic->pShortRefList[LIST_0][1], &pRefPic->pShortRefList[LIST_0][0],
             pRefPic->uiShortRefCount[LIST_0]*sizeof (PPicture));//confirmed_safe_unsafe_usage
  }
  pRefPic->pShortRefList[LIST_0][0] = pPic;
  pRefPic->uiShortRefCount[LIST_0]++;
  return ERR_NONE;
}

static int32_t AddLongTermToList (PRefPic pRefPic, PPicture pPic, int32_t iLongTermFrameIdx) {
  int32_t i = 0;

  pPic->bUsedAsRef = TRUE;
  pPic->bIsLongRef = TRUE;
  pPic->iLongTermFrameIdx = iLongTermFrameIdx;
  if (pRefPic->uiLongRefCount[LIST_0] == 0) {
    pRefPic->pLongRefList[LIST_0][pRefPic->uiLongRefCount[LIST_0]] = pPic;
  } else if (pRefPic->uiLongRefCount[LIST_0] > 0) {
    for (i = 0; i < pRefPic->uiLongRefCount[LIST_0]; i++) {
      if (pRefPic->pLongRefList[LIST_0][i]->iLongTermFrameIdx > pPic->iLongTermFrameIdx)	{
        break;
      }
    }
    memmove (&pRefPic->pLongRefList[LIST_0][i + 1], &pRefPic->pLongRefList[LIST_0][i],
             (pRefPic->uiLongRefCount[LIST_0] - i)*sizeof (PPicture)); //confirmed_safe_unsafe_usage
    pRefPic->pLongRefList[LIST_0][i] = pPic;
  } else {
    return ERR_INFO_REF_COUNT_OVERFLOW;
  }


  pRefPic->uiLongRefCount[LIST_0]++;
  return ERR_NONE;
}

static int32_t AssignLongTermIdx (PRefPic pRefPic, int32_t iFrameNum, int32_t iLongTermFrameIdx) {
  PPicture pPic = NULL;
  int32_t iRet = ERR_NONE;
  WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx, REMOVE_TARGET);
  WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx, REMOVE_BASE);

  pPic = WelsDelShortFromList (pRefPic, iFrameNum, REMOVE_TARGET);
  if (pPic) {
    iRet = AddLongTermToList (pRefPic, pPic, iLongTermFrameIdx);
  } else {
    return ERR_INFO_INVALID_REF_MARKING;
  }

  pPic = NULL;
  pPic = WelsDelShortFromList (pRefPic, iFrameNum, REMOVE_BASE);
  if (pPic) {
    iRet = AddLongTermToList (pRefPic, pPic, iLongTermFrameIdx);
  }

  return iRet;
}

static int32_t MarkAsLongTerm (PRefPic pRefPic, int32_t iFrameNum, int32_t iLongTermFrameIdx) {
  PPicture pPic = NULL;
  int32_t i = 0;
  int32_t iRet = ERR_NONE;
  WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx, REMOVE_TARGET);
  WelsDelLongFromListSetUnref (pRefPic, iLongTermFrameIdx, REMOVE_BASE);

  for (i = 0; i < pRefPic->uiRefCount[LIST_0]; i++)	{
    pPic = pRefPic->pRefList[LIST_0][i];
    if (pPic->iFrameNum == iFrameNum && !pPic->bIsLongRef) {
      iRet = AddLongTermToList (pRefPic, pPic, iLongTermFrameIdx);
    }
  }

  return iRet;
}

} // namespace WelsDec