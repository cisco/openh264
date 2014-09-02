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

// ref_list_mgr_svc.c
#include "ref_list_mgr_svc.h"
#include "utils.h"
#include "picture_handle.h"
namespace WelsEnc {
/*
 *	set picture as unreferenced
 */
void SetUnref (SPicture* pRef) {
  if (NULL != pRef)	{
    pRef->iFramePoc		= -1;
    pRef->iFrameNum		= -1;
    pRef->uiTemporalId	=
      pRef->uiSpatialId		=
        pRef->iLongTermPicNum = -1;
    pRef->bIsLongRef	= false;
    pRef->uiRecieveConfirmed = RECIEVE_FAILED;
    pRef->iMarkFrameNum = -1;
    pRef->bUsedAsRef	= false;

    if (NULL != pRef->pScreenBlockFeatureStorage)
      pRef->pScreenBlockFeatureStorage->bRefBlockFeatureCalculated	= false;
  }
}

/*
*	reset LTR marking , recovery ,feedback state to default
*/
void ResetLtrState (SLTRState* pLtr) {
  pLtr->bReceivedT0LostFlag	= false;
  pLtr->iLastRecoverFrameNum = 0;
  pLtr->iLastCorFrameNumDec = -1;
  pLtr->iCurFrameNumInDec = -1;

  // LTR mark
  pLtr->iLTRMarkMode = LTR_DIRECT_MARK;
  pLtr->iLTRMarkSuccessNum = 0; //successful marked num
  pLtr->bLTRMarkingFlag = false;	//decide whether current frame marked as LTR
  pLtr->bLTRMarkEnable = false; //when LTR is confirmed and the interval is no smaller than the marking period
  pLtr->iCurLtrIdx = 0;
  memset (&pLtr->iLastLtrIdx , 0 , sizeof (pLtr->iLastLtrIdx)) ;
  pLtr->uiLtrMarkInterval = 0;

  // LTR mark feedback
  pLtr->uiLtrMarkState = NO_LTR_MARKING_FEEDBACK ;
  pLtr->iLtrMarkFbFrameNum = -1;
}

/*
 *	reset reference picture list
 */
void WelsResetRefList (sWelsEncCtx* pCtx) {
  SRefList* pRefList = pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  int32_t i;

  for (i = 0; i < MAX_SHORT_REF_COUNT + 1; i++)
    pRefList->pShortRefList[i] = NULL;
  for (i = 0; i < MAX_LONG_REF_COUNT + 1; i++)
    pRefList->pLongRefList[i] = NULL;
  for (i = 0; i < pCtx->pSvcParam->iNumRefFrame + 1; i++)
    SetUnref (pRefList->pRef[i]);

  pRefList->uiLongRefCount = 0;
  pRefList->uiShortRefCount = 0;
  pRefList->pNextBuffer = pRefList->pRef[0];
}

static inline void DeleteLTRFromLongList (sWelsEncCtx* pCtx, int32_t iIdx) {
  SRefList* pRefList = pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  int32_t k ;

  for (k = iIdx; k < pRefList->uiLongRefCount - 1; k++)	{
    pRefList->pLongRefList[k] = pRefList->pLongRefList[k + 1];
  }
  pRefList->pLongRefList[k] = NULL;
  pRefList->uiLongRefCount--;

}
static inline void DeleteSTRFromShortList (sWelsEncCtx* pCtx, int32_t iIdx) {
  SRefList* pRefList = pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  int32_t k ;

  for (k = iIdx; k < pRefList->uiShortRefCount - 1; k++)	{
    pRefList->pShortRefList[k] = pRefList->pShortRefList[k + 1];
  }
  pRefList->pShortRefList[k] = NULL;
  pRefList->uiShortRefCount--;

}
static void DeleteNonSceneLTR (sWelsEncCtx* pCtx) {
  SRefList* pRefList = pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  for (int32_t i = 0; i < pCtx->pSvcParam->iNumRefFrame; ++i) {
    SPicture* pRef = pRefList->pLongRefList[i];
    if (pRef != NULL &&  pRef->bUsedAsRef && pRef->bIsLongRef && (!pRef->bIsSceneLTR) &&
        (pCtx->uiTemporalId < pRef->uiTemporalId || pCtx->bCurFrameMarkedAsSceneLtr)) {
      SetUnref (pRef);
      DeleteLTRFromLongList (pCtx, i);
      i--;
    }
  }
}

static inline int32_t CompareFrameNum (int32_t iFrameNumA, int32_t iFrameNumB, int32_t iMaxFrameNumPlus1) {
  int64_t iNumA, iNumB, iDiffAB, iDiffMin;
  if (iFrameNumA > iMaxFrameNumPlus1 || iFrameNumB > iMaxFrameNumPlus1) {
    return -2;
  }
#define  WelsAbsDiffInt64(a,b) ( (a) > (b) )?( a - b ):( b - a )

  iDiffAB = WelsAbsDiffInt64 ((int64_t) (iFrameNumA), (int64_t) (iFrameNumB));

  iDiffMin = iDiffAB;
  if (iDiffMin == 0) {
    return FRAME_NUM_EQUAL;
  }

  iNumA = WelsAbsDiffInt64 ((int64_t) (iFrameNumA + iMaxFrameNumPlus1), (int64_t) (iFrameNumB));
  if (iNumA == 0) {
    return FRAME_NUM_EQUAL;
  } else if (iDiffMin > iNumA)	{
    return FRAME_NUM_BIGGER;
  }

  iNumB = WelsAbsDiffInt64 ((int64_t) (iFrameNumB + iMaxFrameNumPlus1), (int64_t) (iFrameNumA));
  if (iNumB == 0) {
    return FRAME_NUM_EQUAL;
  } else if (iDiffMin > iNumB)	{
    return FRAME_NUM_SMALLER;
  }

  return (iFrameNumA > iFrameNumB) ? (FRAME_NUM_BIGGER) : (FRAME_NUM_SMALLER);

}
/*
*	delete failed mark according LTR recovery pRequest
*/
static inline void DeleteInvalidLTR (sWelsEncCtx* pCtx) {
  SRefList* pRefList		= pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  SPicture** pLongRefList = pRefList->pLongRefList;
  SLTRState* pLtr = &pCtx->pLtr[pCtx->uiDependencyId];
  int32_t iMaxFrameNumPlus1 = (1 << pCtx->pSps->uiLog2MaxFrameNum);
  int32_t i;
  SLogContext* pLogCtx = & (pCtx->sLogCtx);

  for (i = 0; i < LONG_TERM_REF_NUM; i++) {
    if (pLongRefList[i] != NULL)	{
      if (CompareFrameNum (pLongRefList[i]->iFrameNum , pLtr->iLastCorFrameNumDec, iMaxFrameNumPlus1) == FRAME_NUM_BIGGER
          && (CompareFrameNum (pLongRefList[i]->iFrameNum , pLtr->iCurFrameNumInDec,
                               iMaxFrameNumPlus1) & (FRAME_NUM_EQUAL | FRAME_NUM_SMALLER))) {
        WelsLog (pLogCtx, WELS_LOG_WARNING, "LTR ,invalid LTR delete ,long_term_idx = %d , iFrameNum =%d ",
                 pLongRefList[i]->iLongTermPicNum, pLongRefList[i]->iFrameNum);
        SetUnref (pLongRefList[i]);
        DeleteLTRFromLongList (pCtx, i);
        pLtr->bLTRMarkEnable = true;
        if (pRefList->uiLongRefCount == 0) 	{
          pCtx->bEncCurFrmAsIdrFlag = true;
        }
      } else if (CompareFrameNum (pLongRefList[i]->iMarkFrameNum , pLtr->iLastCorFrameNumDec ,
                                  iMaxFrameNumPlus1) == FRAME_NUM_BIGGER
                 && (CompareFrameNum (pLongRefList[i]->iMarkFrameNum, pLtr->iCurFrameNumInDec ,
                                      iMaxFrameNumPlus1) & (FRAME_NUM_EQUAL | FRAME_NUM_SMALLER))
                 && pLtr->iLTRMarkMode == LTR_DELAY_MARK)	{
        WelsLog (pLogCtx, WELS_LOG_WARNING, "LTR ,iMarkFrameNum invalid LTR delete ,long_term_idx = %d , iFrameNum =%d ",
                 pLongRefList[i]->iLongTermPicNum, pLongRefList[i]->iFrameNum);
        SetUnref (pLongRefList[i]);
        DeleteLTRFromLongList (pCtx, i);
        pLtr->bLTRMarkEnable = true;
        if (pRefList->uiLongRefCount == 0) 	{
          pCtx->bEncCurFrmAsIdrFlag = true;
        }
      }
    }
  }

}
/*
*	handle LTR Mark feedback message
*/
static inline void HandleLTRMarkFeedback (sWelsEncCtx* pCtx) {
  SRefList* pRefList		= pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  SPicture** pLongRefList		= pRefList->pLongRefList;
  SLTRState* pLtr = &pCtx->pLtr[pCtx->uiDependencyId];
  int32_t i, j;

  if (pLtr->uiLtrMarkState == LTR_MARKING_SUCCESS) {
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING,
             "pLtr->uiLtrMarkState = %d, pLtr.iCurLtrIdx = %d , pLtr->iLtrMarkFbFrameNum = %d ,pCtx->iFrameNum = %d ",
             pLtr->uiLtrMarkState, pLtr->iCurLtrIdx, pLtr->iLtrMarkFbFrameNum, pCtx->iFrameNum);
    for (i = 0; i < pRefList->uiLongRefCount; i++)	{
      if (pLongRefList[i]->iFrameNum == pLtr->iLtrMarkFbFrameNum && pLongRefList[i]->uiRecieveConfirmed != RECIEVE_SUCCESS) {

        pLongRefList[i]->uiRecieveConfirmed = RECIEVE_SUCCESS;
        pCtx->pVaa->uiValidLongTermPicIdx = pLongRefList[i]->iLongTermPicNum;

        pLtr->iCurFrameNumInDec  =
          pLtr->iLastRecoverFrameNum =
            pLtr->iLastCorFrameNumDec = pLtr->iLtrMarkFbFrameNum;

        for (j = 0; j < pRefList->uiLongRefCount; j++)	{
          if (pLongRefList[j]->iLongTermPicNum != pLtr->iCurLtrIdx)	{
            SetUnref (pLongRefList[j]);
            DeleteLTRFromLongList (pCtx, j);
          }
        }

        pLtr->iLTRMarkSuccessNum++;
        pLtr->iCurLtrIdx = (pLtr->iCurLtrIdx + 1) % LONG_TERM_REF_NUM;
        pLtr->iLTRMarkMode = (pLtr->iLTRMarkSuccessNum >= (LONG_TERM_REF_NUM)) ? (LTR_DELAY_MARK) : (LTR_DIRECT_MARK);
        WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "LTR mark mode =%d", pLtr->iLTRMarkMode);
        pLtr->bLTRMarkEnable = true;
        break;
      }
    }
    pLtr->uiLtrMarkState = NO_LTR_MARKING_FEEDBACK;
  } else if (pLtr->uiLtrMarkState == LTR_MARKING_FAILED) {
    for (i = 0; i < pRefList->uiLongRefCount; i++)	{
      if (pLongRefList[i]->iFrameNum == pLtr->iLtrMarkFbFrameNum)	{
        SetUnref (pLongRefList[i]);
        DeleteLTRFromLongList (pCtx, i);
        break;
      }
    }
    pLtr->uiLtrMarkState = NO_LTR_MARKING_FEEDBACK;
    pLtr->bLTRMarkEnable = true;

    if (pLtr->iLTRMarkSuccessNum == 0) {
      pCtx->bEncCurFrmAsIdrFlag = true; // no LTR , means IDR recieve failed, force next frame IDR
    }
  }
}
/*
 *	LTR mark process
 */
static inline void LTRMarkProcess (sWelsEncCtx* pCtx) {
  SRefList* pRefList		= pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  SPicture** pLongRefList = pRefList->pLongRefList;
  SPicture** pShortRefList = pRefList->pShortRefList;
  SLTRState* pLtr = &pCtx->pLtr[pCtx->uiDependencyId];
  int32_t iGoPFrameNumInterval = ((pCtx->pSvcParam->uiGopSize >> 1) > 1) ? (pCtx->pSvcParam->uiGopSize >> 1) : (1);
  int32_t iMaxFrameNumPlus1 = (1 << pCtx->pSps->uiLog2MaxFrameNum);
  int32_t i = 0;
  int32_t j = 0;
  bool bMoveLtrFromShortToLong = false;

  if (pCtx->eSliceType == I_SLICE)	{
    i = 0;
    pShortRefList[i]->uiRecieveConfirmed = RECIEVE_SUCCESS;
  } else if (pLtr->bLTRMarkingFlag) {
    pCtx->pVaa->uiMarkLongTermPicIdx = pLtr->iCurLtrIdx;

    if (pLtr->iLTRMarkMode == LTR_DELAY_MARK)	{
      for (i = 0; i < pRefList->uiShortRefCount; i++)	{
        if (CompareFrameNum (pCtx->iFrameNum, pShortRefList[i]->iFrameNum + iGoPFrameNumInterval,
                             iMaxFrameNumPlus1) == FRAME_NUM_EQUAL) {
          break;
        }
      }
    }
  }

  if (pCtx->eSliceType == I_SLICE || pLtr->bLTRMarkingFlag) {
    pShortRefList[i]->bIsLongRef = true;
    pShortRefList[i]->iLongTermPicNum = pLtr->iCurLtrIdx;
    pShortRefList[i]->iMarkFrameNum = pCtx->iFrameNum;
  }

  // delay one gop to move LTR from int16_t list to int32_t list
  if (pLtr->iLTRMarkMode == LTR_DIRECT_MARK && pCtx->eSliceType != I_SLICE && !pLtr->bLTRMarkingFlag) {
    for (j = 0; j < pRefList->uiShortRefCount; j++) {
      if (pRefList->pShortRefList[j]->bIsLongRef)	{
        i = j;
        bMoveLtrFromShortToLong = true;
        break;
      }
    }
  }

  if ((pLtr->iLTRMarkMode == LTR_DELAY_MARK && pLtr->bLTRMarkingFlag) || ((pLtr->iLTRMarkMode == LTR_DIRECT_MARK)
      && (bMoveLtrFromShortToLong))) {
    if (pRefList->uiLongRefCount > 0) {
      memmove (&pRefList->pLongRefList[1], &pRefList->pLongRefList[0],
               pRefList->uiLongRefCount * sizeof (SPicture*));	// confirmed_safe_unsafe_usage
    }
    pLongRefList[0]	 = pShortRefList[i];
    pRefList->uiLongRefCount++;
    if (pRefList->uiLongRefCount > pCtx->pSvcParam->iLTRRefNum) {
      SetUnref (pRefList->pLongRefList[pRefList->uiLongRefCount - 1]);
      DeleteLTRFromLongList (pCtx, pRefList->uiLongRefCount - 1);
    }
    DeleteSTRFromShortList (pCtx, i);
  }
}

static inline void LTRMarkProcessScreen (sWelsEncCtx* pCtx) {
  SRefList* pRefList		= pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  SPicture** pLongRefList = pRefList->pLongRefList;
  int32_t iLtrIdx =  pCtx->pDecPic->iLongTermPicNum;
  pCtx->pVaa->uiMarkLongTermPicIdx = pCtx->pDecPic->iLongTermPicNum;

  if (pLongRefList[iLtrIdx] != NULL) {
    SetUnref (pLongRefList[iLtrIdx]);
    DeleteLTRFromLongList (pCtx, iLtrIdx);
  }
  pLongRefList[iLtrIdx] = pCtx->pDecPic;
  pRefList->uiLongRefCount++;
}

static inline void PrefetchNextBuffer (sWelsEncCtx* pCtx) {
  SRefList* pRefList		= pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  const int32_t kiNumRef	= pCtx->pSvcParam->iNumRefFrame;
  int32_t i;

  pRefList->pNextBuffer = NULL;
  for (i = 0; i < kiNumRef + 1; i++) {
    if (!pRefList->pRef[i]->bUsedAsRef) {
      pRefList->pNextBuffer = pRefList->pRef[i];
      break;
    }
  }

  if (pRefList->pNextBuffer == NULL && pRefList->uiShortRefCount > 0) {
    pRefList->pNextBuffer = pRefList->pShortRefList[pRefList->uiShortRefCount - 1];
    SetUnref (pRefList->pNextBuffer);
  }

  pCtx->pDecPic = pRefList->pNextBuffer;
}

/*
 *	update reference picture list
 */
bool WelsUpdateRefList (void* pEncCtx) {
  sWelsEncCtx* pCtx     = (sWelsEncCtx*)pEncCtx;
  SRefList* pRefList		= pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  SLTRState* pLtr			= &pCtx->pLtr[pCtx->uiDependencyId];
  SSpatialLayerInternal* pParamD	= &pCtx->pSvcParam->sDependencyLayers[pCtx->uiDependencyId];

  int32_t iRefIdx			= 0;
  const uint8_t kuiTid		= pCtx->uiTemporalId;
  const uint8_t kuiDid		= pCtx->uiDependencyId;
  const EWelsSliceType keSliceType		= pCtx->eSliceType;
  uint32_t i = 0;
  // Need update pRef list in case store base layer or target dependency layer construction
  if (NULL == pCtx->pCurDqLayer)
    return false;

  if (NULL == pRefList || NULL == pRefList->pRef[0])
    return false;

  if (NULL != pCtx->pDecPic) {
#if !defined(ENABLE_FRAME_DUMP)	// to save complexity, 1/6/2009
    if ((pParamD->iHighestTemporalId == 0) || (kuiTid < pParamD->iHighestTemporalId))
#endif// !ENABLE_FRAME_DUMP
      // Expanding picture for future reference
      ExpandReferencingPicture (pCtx->pDecPic->pData, pCtx->pDecPic->iWidthInPixel, pCtx->pDecPic->iHeightInPixel,
                                pCtx->pDecPic->iLineSize,
                                pCtx->pFuncList->sExpandPicFunc.pfExpandLumaPicture, pCtx->pFuncList->sExpandPicFunc.pfExpandChromaPicture);

    // move picture in list
    pCtx->pDecPic->uiTemporalId = kuiTid;
    pCtx->pDecPic->uiSpatialId	= kuiDid;
    pCtx->pDecPic->iFrameNum	= pCtx->iFrameNum;
    pCtx->pDecPic->iFramePoc	= pCtx->iPOC;
    pCtx->pDecPic->uiRecieveConfirmed = RECIEVE_UNKOWN;
    pCtx->pDecPic->bUsedAsRef	= true;

    for (iRefIdx = pRefList->uiShortRefCount - 1; iRefIdx >= 0; --iRefIdx)	{
      pRefList->pShortRefList[iRefIdx + 1] = pRefList->pShortRefList[iRefIdx];
    }
    pRefList->pShortRefList[0] = pCtx->pDecPic;
    pRefList->uiShortRefCount++;
  }

  if (keSliceType == P_SLICE) {
    if (pCtx->uiTemporalId == 0) {
      if (pCtx->pSvcParam->bEnableLongTermReference)	{
        LTRMarkProcess (pCtx);
        DeleteInvalidLTR (pCtx);
        HandleLTRMarkFeedback (pCtx);

        pLtr->bReceivedT0LostFlag = false; // reset to false due to the recovery is finished
        pLtr->bLTRMarkingFlag = false;
        ++pLtr->uiLtrMarkInterval;
      }

      for (i = pRefList->uiShortRefCount - 1; i > 0; i--) {
        SetUnref (pRefList->pShortRefList[i]);
        DeleteSTRFromShortList (pCtx, i);
      }
      if (pRefList->uiShortRefCount > 0 && (pRefList->pShortRefList[0]->uiTemporalId > 0
                                            || pRefList->pShortRefList[0]->iFrameNum != pCtx->iFrameNum)) {
        SetUnref (pRefList->pShortRefList[0]);
        DeleteSTRFromShortList (pCtx, 0);
      }
    }
  } else {	// in case IDR currently coding
    if (pCtx->pSvcParam->bEnableLongTermReference)	{
      LTRMarkProcess (pCtx);

      pLtr->iCurLtrIdx = (pLtr->iCurLtrIdx + 1) % LONG_TERM_REF_NUM;
      pLtr->iLTRMarkSuccessNum = 1; //IDR default suceess
      pLtr->bLTRMarkEnable =  true;
      pLtr->uiLtrMarkInterval = 0;

      pCtx->pVaa->uiValidLongTermPicIdx = 0;
      pCtx->pVaa->uiMarkLongTermPicIdx = 0;
    }
  }
  PrefetchNextBuffer (pCtx);
  return true;
}

bool CheckCurMarkFrameNumUsed (sWelsEncCtx* pCtx) {
  SLTRState* pLtr = &pCtx->pLtr[pCtx->uiDependencyId];
  SRefList* pRefList	= pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  SPicture** pLongRefList = pRefList->pLongRefList;
  int32_t iGoPFrameNumInterval = ((pCtx->pSvcParam->uiGopSize >> 1) > 1) ? (pCtx->pSvcParam->uiGopSize >> 1) : (1);
  int32_t iMaxFrameNumPlus1 = (1 << pCtx->pSps->uiLog2MaxFrameNum);
  int32_t i;

  for (i = 0; i < pRefList->uiLongRefCount; i++) {
    if ((pCtx->iFrameNum == pLongRefList[i]->iFrameNum && pLtr->iLTRMarkMode == LTR_DIRECT_MARK) ||
        (CompareFrameNum (pCtx->iFrameNum + iGoPFrameNumInterval, pLongRefList[i]->iFrameNum,
                          iMaxFrameNumPlus1) == FRAME_NUM_EQUAL  && pLtr->iLTRMarkMode == LTR_DELAY_MARK)) {
      return false;
    }
  }

  return true;
}
void WelsMarkPic (void* pEncCtx) {
  sWelsEncCtx* pCtx = (sWelsEncCtx*)pEncCtx;
  SLTRState* pLtr = &pCtx->pLtr[pCtx->uiDependencyId];
  const int32_t kiCountSliceNum			= GetCurrentSliceNum (pCtx->pCurDqLayer->pSliceEncCtx);
  int32_t iGoPFrameNumInterval = ((pCtx->pSvcParam->uiGopSize >> 1) > 1) ? (pCtx->pSvcParam->uiGopSize >> 1) : (1);
  int32_t iSliceIdx = 0;

  if (pCtx->pSvcParam->bEnableLongTermReference && pLtr->bLTRMarkEnable && pCtx->uiTemporalId == 0) {
    if (!pLtr->bReceivedT0LostFlag && pLtr->uiLtrMarkInterval > pCtx->pSvcParam->iLtrMarkPeriod
        && CheckCurMarkFrameNumUsed (pCtx)) {
      pLtr->bLTRMarkingFlag = true;
      pLtr->bLTRMarkEnable = false;
      pLtr->uiLtrMarkInterval = 0;
      for (int32_t i = 0 ; i < MAX_TEMPORAL_LAYER_NUM; ++i) {
        if (pCtx->uiTemporalId < i || pCtx->uiTemporalId == 0) {
          pLtr->iLastLtrIdx[i] = pLtr->iCurLtrIdx;
        }
      }
    } else {
      pLtr->bLTRMarkingFlag = false;
    }
  }

  for (iSliceIdx = 0; iSliceIdx < kiCountSliceNum; iSliceIdx++)	{
    SSliceHeaderExt*	pSliceHdrExt		= &pCtx->pCurDqLayer->sLayerInfo.pSliceInLayer[iSliceIdx].sSliceHeaderExt;
    SSliceHeader*		pSliceHdr			= &pSliceHdrExt->sSliceHeader;
    SRefPicMarking*		pRefPicMark		= &pSliceHdr->sRefMarking;

    memset (pRefPicMark, 0, sizeof (SRefPicMarking));

    if (pCtx->pSvcParam->bEnableLongTermReference && pLtr->bLTRMarkingFlag) {
      if (pLtr->iLTRMarkMode == LTR_DIRECT_MARK)	{
        pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount].iMaxLongTermFrameIdx = LONG_TERM_REF_NUM - 1;
        pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount++].iMmcoType = MMCO_SET_MAX_LONG;

        pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount].iDiffOfPicNum = iGoPFrameNumInterval;
        pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount++].iMmcoType = MMCO_SHORT2UNUSED;

        pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount].iLongTermFrameIdx = pLtr->iCurLtrIdx;
        pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount++].iMmcoType = MMCO_LONG;
      } else if (pLtr->iLTRMarkMode == LTR_DELAY_MARK)	{
        pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount].iDiffOfPicNum = iGoPFrameNumInterval;
        pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount].iLongTermFrameIdx = pLtr->iCurLtrIdx;
        pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount++].iMmcoType = MMCO_SHORT2LONG;
      }
    }
  }
}

int32_t FilterLTRRecoveryRequest (sWelsEncCtx* pCtx, SLTRRecoverRequest* pLTRRecoverRequest) {
  SLTRRecoverRequest* pRequest = pLTRRecoverRequest;
  SLTRState* pLtr = &pCtx->pLtr[pCtx->uiDependencyId];
  int32_t iMaxFrameNumPlus1 = (1 << pCtx->pSps->uiLog2MaxFrameNum);
  if (pCtx->pSvcParam->bEnableLongTermReference) {
    if (pRequest->uiFeedbackType == LTR_RECOVERY_REQUEST &&  pRequest->uiIDRPicId == pCtx->sPSOVector.uiIdrPicId) {
      if (pRequest->iLastCorrectFrameNum == -1) {
        pCtx->bEncCurFrmAsIdrFlag = true;
        return true;
      } else if (pRequest->iCurrentFrameNum == -1) {
        pLtr->bReceivedT0LostFlag = true;
        return true;
      } else if ((CompareFrameNum (pLtr->iLastRecoverFrameNum , pRequest->iLastCorrectFrameNum,
                                   iMaxFrameNumPlus1) & (FRAME_NUM_EQUAL | FRAME_NUM_SMALLER)) // t0 lost
                 || ((CompareFrameNum (pLtr->iLastRecoverFrameNum , pRequest->iCurrentFrameNum,
                                       iMaxFrameNumPlus1) & (FRAME_NUM_EQUAL | FRAME_NUM_SMALLER)) &&
                     CompareFrameNum (pLtr->iLastRecoverFrameNum , pRequest->iLastCorrectFrameNum,
                                      iMaxFrameNumPlus1) == FRAME_NUM_BIGGER)) { // recovery failed

        pLtr->bReceivedT0LostFlag = true;
        pLtr->iLastCorFrameNumDec = pRequest->iLastCorrectFrameNum;
        pLtr->iCurFrameNumInDec = pRequest->iCurrentFrameNum;
        WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO,
                 "Receive valid LTR recovery pRequest,feedback_type = %d ,uiIdrPicId = %d , current_frame_num = %d , last correct frame num = %d"
                 , pRequest->uiFeedbackType, pRequest->uiIDRPicId, pRequest->iCurrentFrameNum, pRequest->iLastCorrectFrameNum);
      }

      WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO,
               "Receive LTR recovery pRequest,feedback_type = %d ,uiIdrPicId = %d , current_frame_num = %d , last correct frame num = %d"
               , pRequest->uiFeedbackType, pRequest->uiIDRPicId, pRequest->iCurrentFrameNum, pRequest->iLastCorrectFrameNum);
    }
  } else if (!pCtx->pSvcParam->bEnableLongTermReference) {
    pCtx->bEncCurFrmAsIdrFlag = true;
  }
  return true;
}
void FilterLTRMarkingFeedback (sWelsEncCtx* pCtx, SLTRMarkingFeedback* pLTRMarkingFeedback) {
  SLTRState* pLtr = &pCtx->pLtr[pCtx->uiDependencyId];
  assert (pLTRMarkingFeedback);
  if (pCtx->pSvcParam->bEnableLongTermReference)	{
    if (pLTRMarkingFeedback->uiIDRPicId == pCtx->sPSOVector.uiIdrPicId
        && (pLTRMarkingFeedback->uiFeedbackType == LTR_MARKING_SUCCESS
            || pLTRMarkingFeedback->uiFeedbackType == LTR_MARKING_FAILED)) { // avoid error pData
      pLtr->uiLtrMarkState = pLTRMarkingFeedback->uiFeedbackType;
      pLtr->iLtrMarkFbFrameNum =  pLTRMarkingFeedback->iLTRFrameNum ;
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO,
               "Receive valid LTR marking feedback, feedback_type = %d , uiIdrPicId = %d , LTR_frame_num = %d , cur_idr_pic_id = %d",
               pLTRMarkingFeedback->uiFeedbackType, pLTRMarkingFeedback->uiIDRPicId, pLTRMarkingFeedback->iLTRFrameNum ,
               pCtx->sPSOVector.uiIdrPicId);

    } else {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO,
               "Receive LTR marking feedback, feedback_type = %d , uiIdrPicId = %d , LTR_frame_num = %d , cur_idr_pic_id = %d",
               pLTRMarkingFeedback->uiFeedbackType, pLTRMarkingFeedback->uiIDRPicId, pLTRMarkingFeedback->iLTRFrameNum ,
               pCtx->sPSOVector.uiIdrPicId);
    }
  }
}

/*
 *	build reference picture list
 */
bool WelsBuildRefList (void* pEncCtx, const int32_t iPOC, int32_t iBestLtrRefIdx) {
  sWelsEncCtx* pCtx     = (sWelsEncCtx*)pEncCtx;
  SRefList* pRefList		=  pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  SLTRState* pLtr			= &pCtx->pLtr[pCtx->uiDependencyId];
  const int32_t kiNumRef	= pCtx->pSvcParam->iNumRefFrame;
  const uint8_t kuiTid		= pCtx->uiTemporalId;
  uint32_t i				= 0;

  // to support any type of cur_dq->mgs_control
  //	[ 0:	using current layer to do ME/MC;
  //	  -1:	using store base layer to do ME/MC;
  //	  2:	using highest layer to do ME/MC; ]

  // build reference list 0/1 if applicable

  pCtx->iNumRef0	= 0;

  if (pCtx->eSliceType != I_SLICE) {
    if (pCtx->pSvcParam->bEnableLongTermReference && pLtr->bReceivedT0LostFlag && pCtx->uiTemporalId == 0) {
      for (i = 0; i < pRefList->uiLongRefCount; i++)	{
        if (pRefList->pLongRefList[i]->uiRecieveConfirmed == RECIEVE_SUCCESS)	{
          pCtx->pRefList0[pCtx->iNumRef0++] = pRefList->pLongRefList[i];
          pLtr->iLastRecoverFrameNum = pCtx->iFrameNum;
          WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO,
                   "pRef is int32_t !iLastRecoverFrameNum = %d, pRef iFrameNum = %d,LTR number = %d,",
                   pLtr->iLastRecoverFrameNum, pCtx->pRefList0[0]->iFrameNum, pRefList->uiLongRefCount);
          break;
        }
      }
    } else {
      for (i = 0; i < pRefList->uiShortRefCount; ++ i) {
        SPicture* pRef = pRefList->pShortRefList[i];
        if (pRef != NULL && pRef->bUsedAsRef && pRef->iFramePoc >= 0 && pRef->uiTemporalId <= kuiTid) {
          pCtx->pRefList0[pCtx->iNumRef0++]	= pRef;
          WelsLog (& (pCtx->sLogCtx), WELS_LOG_DETAIL,
                   "WelsBuildRefList pCtx->uiTemporalId = %d,pRef->iFrameNum = %d,pRef->uiTemporalId = %d",
                   pCtx->uiTemporalId, pRef->iFrameNum, pRef->uiTemporalId);
          break;
        }
      }
    }
  } else {	// safe for IDR
    WelsResetRefList (pCtx);  //for IDR, SHOULD reset pRef list.
    ResetLtrState (&pCtx->pLtr[pCtx->uiDependencyId]); //SHOULD update it when IDR.
    pCtx->pRefList0[0]	= NULL;
  }

  if (pCtx->iNumRef0 > kiNumRef)
    pCtx->iNumRef0 = kiNumRef;
  return (pCtx->iNumRef0 > 0 || pCtx->eSliceType == I_SLICE) ? (true) : (false);
}

/*
 *	update syntax for reference base related
 */
void WelsUpdateRefSyntax (sWelsEncCtx* pCtx, const int32_t iPOC, const int32_t uiFrameType) {
  SLTRState* pLtr = &pCtx->pLtr[pCtx->uiDependencyId];
  int32_t iIdx								= 0;
  const int32_t kiCountSliceNum			= GetCurrentSliceNum (pCtx->pCurDqLayer->pSliceEncCtx);
  int32_t	iAbsDiffPicNumMinus1			= -1;

  assert (kiCountSliceNum > 0);

  /*syntax for ref_pic_list_reordering()*/
  if (pCtx->iNumRef0 > 0)
    iAbsDiffPicNumMinus1 = pCtx->iFrameNum - (pCtx->pRefList0[0]->iFrameNum) - 1;

  for (iIdx = 0; iIdx < kiCountSliceNum; iIdx++) {
    SSliceHeaderExt*	pSliceHdrExt		= &pCtx->pCurDqLayer->sLayerInfo.pSliceInLayer[iIdx].sSliceHeaderExt;
    SSliceHeader*		pSliceHdr			= &pSliceHdrExt->sSliceHeader;
    SRefPicListReorderSyntax* pRefReorder	= &pSliceHdr->sRefReordering;
    SRefPicMarking* pRefPicMark			= &pSliceHdr->sRefMarking;

    /*syntax for num_ref_idx_l0_active_minus1*/
    pSliceHdr->uiRefCount = pCtx->iNumRef0;
    if (pCtx->iNumRef0 > 0) {
      if ((!pCtx->pRefList0[0]->bIsLongRef) || (!pCtx->pSvcParam->bEnableLongTermReference)) {
        if (iAbsDiffPicNumMinus1 < 0) {
          WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO, "WelsUpdateRefSyntax():::uiAbsDiffPicNumMinus1:%d", iAbsDiffPicNumMinus1);
          iAbsDiffPicNumMinus1 += (1 << (pCtx->pSps->uiLog2MaxFrameNum));
          WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO, "WelsUpdateRefSyntax():::uiAbsDiffPicNumMinus1< 0, update as:%d",
                   iAbsDiffPicNumMinus1);
        }

        pRefReorder->SReorderingSyntax[0].uiReorderingOfPicNumsIdc = 0;
        pRefReorder->SReorderingSyntax[0].uiAbsDiffPicNumMinus1    = iAbsDiffPicNumMinus1;
        pRefReorder->SReorderingSyntax[1].uiReorderingOfPicNumsIdc = 3;
      } else {
        pRefReorder->SReorderingSyntax[0].uiReorderingOfPicNumsIdc = 2;
        pRefReorder->SReorderingSyntax[0].iLongTermPicNum = pCtx->pRefList0[0]->iLongTermPicNum;
        pRefReorder->SReorderingSyntax[1].uiReorderingOfPicNumsIdc = 3;
      }
    }

    /*syntax for dec_ref_pic_marking()*/
    if (videoFrameTypeIDR == uiFrameType)		{
      pRefPicMark->bNoOutputOfPriorPicsFlag = false;
      pRefPicMark->bLongTermRefFlag = pCtx->pSvcParam->bEnableLongTermReference;
    } else {
      if (pCtx->pSvcParam->iUsageType == SCREEN_CONTENT_REAL_TIME)
        pRefPicMark->bAdaptiveRefPicMarkingModeFlag = pCtx->pSvcParam->bEnableLongTermReference;
      else
        pRefPicMark->bAdaptiveRefPicMarkingModeFlag = (pCtx->pSvcParam->bEnableLongTermReference
            && pLtr->bLTRMarkingFlag) ? (true) : (false);
    }
  }
}

static int32_t UpdateSrcPicList (sWelsEncCtx* pCtx) {
  int32_t iDIdx = pCtx->uiDependencyId;
  SPicture** pLongRefList = pCtx->ppRefPicListExt[iDIdx]->pLongRefList;
  SPicture** pLongRefSrcList = &pCtx->pVpp->m_pSpatialPic[iDIdx][0];

  //update info in src list
  if (pCtx->pEncPic) {
    pCtx->pEncPic->iPictureType	    = pCtx->pDecPic->iPictureType;
    pCtx->pEncPic->iFramePoc		    = pCtx->pDecPic->iFramePoc;
    pCtx->pEncPic->iFrameNum		    = pCtx->pDecPic->iFrameNum;
    pCtx->pEncPic->uiSpatialId		  = pCtx->pDecPic->uiSpatialId;
    pCtx->pEncPic->uiTemporalId	    = pCtx->pDecPic->uiTemporalId;
    pCtx->pEncPic->iLongTermPicNum  = pCtx->pDecPic->iLongTermPicNum;
    pCtx->pEncPic->bUsedAsRef       = pCtx->pDecPic->bUsedAsRef;
    pCtx->pEncPic->bIsLongRef       = pCtx->pDecPic->bIsLongRef;
    pCtx->pEncPic->bIsSceneLTR      = pCtx->pDecPic->bIsSceneLTR;
    pCtx->pEncPic->iFrameAverageQp  = pCtx->pDecPic->iFrameAverageQp;
  }
  PrefetchNextBuffer (pCtx);
  for (int32_t i = 0; i < MAX_REF_PIC_COUNT; ++i) {
    if (NULL == pLongRefSrcList[i + 1] || (NULL != pLongRefList[i] && pLongRefList[i]->bUsedAsRef
                                           && pLongRefList[i]->bIsLongRef)) {
      continue;
    } else {
      SetUnref (pLongRefSrcList[i + 1]);
    }
  }
  WelsExchangeSpatialPictures (&pCtx->pVpp->m_pSpatialPic[iDIdx][0],
                               &pCtx->pVpp->m_pSpatialPic[iDIdx][1 + pCtx->pVaa->uiMarkLongTermPicIdx]);
  SetUnref (pCtx->pVpp->m_pSpatialPic[iDIdx][0]);

  return 0;
}

bool WelsUpdateRefListScreen (void* pEncCtx) {
  sWelsEncCtx* pCtx     = (sWelsEncCtx*)pEncCtx;
  SRefList* pRefList		= pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  SLTRState* pLtr			= &pCtx->pLtr[pCtx->uiDependencyId];
  SSpatialLayerInternal* pParamD	= &pCtx->pSvcParam->sDependencyLayers[pCtx->uiDependencyId];
  const uint8_t kuiTid		= pCtx->uiTemporalId;
  // Need update ref list in case store base layer or target dependency layer construction
  if (NULL == pCtx->pCurDqLayer)
    return false;

  if (NULL == pRefList || NULL == pRefList->pRef[0])
    return false;

  if (NULL != pCtx->pDecPic) {
#if !defined(ENABLE_FRAME_DUMP)	// to save complexity, 1/6/2009
    if ((pParamD->iHighestTemporalId == 0) || (kuiTid < pParamD->iHighestTemporalId))
#endif// !ENABLE_FRAME_DUMP
      // Expanding picture for future reference
      ExpandReferencingPicture (pCtx->pDecPic->pData, pCtx->pDecPic->iWidthInPixel, pCtx->pDecPic->iHeightInPixel,
                                pCtx->pDecPic->iLineSize,
                                pCtx->pFuncList->sExpandPicFunc.pfExpandLumaPicture, pCtx->pFuncList->sExpandPicFunc.pfExpandChromaPicture);

    // move picture in list
    pCtx->pDecPic->uiTemporalId =  pCtx->uiTemporalId;
    pCtx->pDecPic->uiSpatialId	= pCtx->uiDependencyId;
    pCtx->pDecPic->iFrameNum		= pCtx->iFrameNum;
    pCtx->pDecPic->iFramePoc		= pCtx->iPOC;
    pCtx->pDecPic->bUsedAsRef	= true;
    pCtx->pDecPic->bIsLongRef = true;
    pCtx->pDecPic->bIsSceneLTR =  pLtr->bLTRMarkingFlag || (pCtx->pSvcParam->bEnableLongTermReference
                                  && pCtx->eSliceType == I_SLICE);
    pCtx->pDecPic->iLongTermPicNum = pLtr->iCurLtrIdx;
  }
  if (pCtx->eSliceType == P_SLICE) {
    DeleteNonSceneLTR (pCtx);
    LTRMarkProcessScreen (pCtx);
    pLtr->bLTRMarkingFlag = false;
    ++pLtr->uiLtrMarkInterval;
  } else {	// in case IDR currently coding
    LTRMarkProcessScreen (pCtx);
    pLtr->iCurLtrIdx = 1;
    pLtr->iSceneLtrIdx = 1;
    pLtr->uiLtrMarkInterval = 0;
    pCtx->pVaa->uiValidLongTermPicIdx = 0;
  }

  UpdateSrcPicList (pCtx);
  return true;
}
bool WelsBuildRefListScreen (void* pEncCtx, const int32_t iPOC, int32_t iBestLtrRefIdx) {
  sWelsEncCtx* pCtx     = (sWelsEncCtx*)pEncCtx;
  SRefList* pRefList		=  pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  SWelsSvcCodingParam* pParam = pCtx->pSvcParam;
  SVAAFrameInfoExt* pVaaExt = static_cast<SVAAFrameInfoExt*> (pCtx->pVaa);
  const int32_t iNumRef	= pParam->iNumRefFrame;
  pCtx->iNumRef0 = 0;

  if (pCtx->eSliceType != I_SLICE) {
    int iLtrRefIdx = 0;
    SPicture* pRefOri = NULL;
    for (int idx = 0; idx < pVaaExt->iNumOfAvailableRef; idx++) {
      iLtrRefIdx = pCtx->pVpp->GetRefFrameInfo (idx, pRefOri);
      if (iLtrRefIdx >= 0 && iLtrRefIdx <= pParam->iLTRRefNum) {
        SPicture* pRefPic = pRefList->pLongRefList[iLtrRefIdx];
        if (pRefPic != NULL && pRefPic->bUsedAsRef && pRefPic->bIsLongRef) {
          if (pRefPic->uiTemporalId <= pCtx->uiTemporalId && (!pCtx->bCurFrameMarkedAsSceneLtr || pRefPic->bIsSceneLTR)) {
            pCtx->pCurDqLayer->pRefOri[pCtx->iNumRef0] = pRefOri;
            pCtx->pRefList0[pCtx->iNumRef0++] = pRefPic;
            WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO,
                     "WelsBuildRefListScreen(), ref !current iFrameNum = %d, ref iFrameNum = %d,LTR number = %d,iNumRef = %d ref is Scene LTR = %d",
                     pCtx->iFrameNum, pCtx->pRefList0[pCtx->iNumRef0 - 1]->iFrameNum, pRefList->uiLongRefCount, iNumRef,
                     pRefPic->bIsSceneLTR);
            WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO,
                     "WelsBuildRefListScreen pCtx->uiTemporalId = %d,pRef->iFrameNum = %d,pRef->uiTemporalId = %d",
                     pCtx->uiTemporalId, pRefPic->iFrameNum, pRefPic->uiTemporalId);
          }
        }
      } else {
        for (int32_t i = iNumRef ; i >= 0 ; --i)	{
          if (pRefList->pLongRefList[i] == NULL) {
            continue;
          } else if (pRefList->pLongRefList[i]->uiTemporalId == 0
                     || pRefList->pLongRefList[i]->uiTemporalId < pCtx->uiTemporalId)	{
            pCtx->pCurDqLayer->pRefOri[pCtx->iNumRef0] = pRefOri;
            pCtx->pRefList0[pCtx->iNumRef0++] = pRefList->pLongRefList[i];
            WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO,
                     "WelsBuildRefListScreen(), ref !current iFrameNum = %d, ref iFrameNum = %d,LTR number = %d",
                     pCtx->iFrameNum, pCtx->pRefList0[pCtx->iNumRef0 - 1]->iFrameNum, pRefList->uiLongRefCount);
            break;
          }
        }
      }
    }
  } else {
    WelsResetRefList (pCtx);  //for IDR, SHOULD reset pRef list.
    ResetLtrState (&pCtx->pLtr[pCtx->uiDependencyId]); //SHOULD update it when IDR.
    pCtx->pRefList0[0]	= NULL;
  }
  if (pCtx->iNumRef0 > iNumRef) {
    pCtx->iNumRef0 = iNumRef;
  }
  //TBD info update for md &fme

  return (pCtx->iNumRef0 > 0 || pCtx->eSliceType == I_SLICE) ? (true) : (false);
}
void WelsMarkPicScreen (void* pEncCtx) {
#define STR_ROOM 1
  sWelsEncCtx* pCtx     = (sWelsEncCtx*)pEncCtx;
  SLTRState* pLtr = &pCtx->pLtr[pCtx->uiDependencyId];
  int32_t iMaxTid = WELS_LOG2 (pCtx->pSvcParam->uiGopSize);
  int32_t iMaxActualLtrIdx = -1;
  if (pCtx->pSvcParam->bEnableLongTermReference)
    iMaxActualLtrIdx = pCtx->pSvcParam->iNumRefFrame - STR_ROOM - 1 -  WELS_MAX (iMaxTid , 1);

  SRefList* pRefList =  pCtx->ppRefPicListExt[pCtx->uiDependencyId];
  SPicture** ppLongRefList		=  pRefList->pLongRefList;
  const int32_t iNumRef	= pCtx->pSvcParam->iNumRefFrame;
  int32_t i;
  const int32_t iLongRefNum = iNumRef - STR_ROOM;
  const bool bIsRefListNotFull = pRefList->uiLongRefCount < iLongRefNum;

  if (!pCtx->pSvcParam->bEnableLongTermReference) {
    pLtr->iCurLtrIdx = pCtx->uiTemporalId;
  } else {
    if (iMaxActualLtrIdx != -1 && pCtx->uiTemporalId == 0 && pCtx->bCurFrameMarkedAsSceneLtr) {
      //Scene LTR
      pLtr->bLTRMarkingFlag = true;
      pLtr->uiLtrMarkInterval = 0;
      pLtr->iCurLtrIdx  = pLtr->iSceneLtrIdx % (iMaxActualLtrIdx + 1);
      pLtr->iSceneLtrIdx++;
    } else {
      pLtr->bLTRMarkingFlag = false;
      //for other LTR
      if (bIsRefListNotFull) {
        for (int32_t i = 0; i < iLongRefNum; ++i) {
          if (pRefList->pLongRefList[i] == NULL)	{
            pLtr->iCurLtrIdx = i   ;
            break;
          }
        }
      } else {
        int32_t iMinSTRframe_num = 1 << 30; // is big enough
        int32_t iRefNum_t[MAX_TEMPORAL_LAYER_NUM] =	 {0};
        for (i = 0 ; i < pRefList->uiLongRefCount ; ++i) {
          if (ppLongRefList[i]->bUsedAsRef && ppLongRefList[i]->bIsLongRef && (!ppLongRefList[i]->bIsSceneLTR))	{
            ++iRefNum_t[ ppLongRefList[i]->uiTemporalId ];
          }
        }

        int32_t iMaxMultiRefTid = (iMaxTid) ? (iMaxTid - 1) : (0) ;
        for (i = 0; i < MAX_TEMPORAL_LAYER_NUM ; ++i) {
          if (iRefNum_t[i] > 1) {
            iMaxMultiRefTid = i;
          }
        }
        for (i = 0 ; i < pRefList->uiLongRefCount ; ++i) {
          if (ppLongRefList[i]->bUsedAsRef && ppLongRefList[i]->bIsLongRef && (!ppLongRefList[i]->bIsSceneLTR)
              && iMaxMultiRefTid == ppLongRefList[i]->uiTemporalId)	{
            if (ppLongRefList[i]->iFrameNum < iMinSTRframe_num) {
              pLtr->iCurLtrIdx = ppLongRefList[i]->iLongTermPicNum;
            }
          }
        }
      }
    }
  }

  for (i = 0 ; i < MAX_TEMPORAL_LAYER_NUM; ++i) {
    if ((pCtx->uiTemporalId <  i) || (pCtx->uiTemporalId == 0)) {
      pLtr->iLastLtrIdx[i] = pLtr->iCurLtrIdx;
    }
  }

  const int32_t iMaxLtrIdx = pCtx->pSvcParam->iNumRefFrame - STR_ROOM - 1;
  const int32_t iSliceNum = GetCurrentSliceNum (pCtx->pCurDqLayer->pSliceEncCtx);
  for (int32_t iSliceIdx = 0; iSliceIdx < iSliceNum; iSliceIdx++) {
    SSliceHeaderExt*	pSliceHdrExt		= &pCtx->pCurDqLayer->sLayerInfo.pSliceInLayer[iSliceIdx].sSliceHeaderExt;
    SSliceHeader*		pSliceHdr			= &pSliceHdrExt->sSliceHeader;
    SRefPicMarking*	pRefPicMark		= &pSliceHdr->sRefMarking;

    memset (pRefPicMark, 0, sizeof (SRefPicMarking));
    if (pCtx->pSvcParam->bEnableLongTermReference) {
      pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount].iMaxLongTermFrameIdx = iMaxLtrIdx;
      pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount++].iMmcoType = MMCO_SET_MAX_LONG;

      pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount].iLongTermFrameIdx = pLtr->iCurLtrIdx;
      pRefPicMark->SMmcoRef[pRefPicMark->uiMmcoCount++].iMmcoType = MMCO_LONG;

    }
  }
  return;
}
void InitRefListMgrFunc (SWelsFuncPtrList* pFuncList, EUsageType eUsageType) {
  if (eUsageType == SCREEN_CONTENT_REAL_TIME) {
    pFuncList->pBuildRefList =   WelsBuildRefListScreen;
    pFuncList->pMarkPic      =   WelsMarkPicScreen;
    pFuncList->pUpdateRefList =   WelsUpdateRefListScreen;
  } else {
    pFuncList->pBuildRefList =   WelsBuildRefList;
    pFuncList->pMarkPic      =   WelsMarkPic;
    pFuncList->pUpdateRefList =   WelsUpdateRefList;
  }
}
} // namespace WelsEnc

