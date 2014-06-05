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
 * \file	svc_mode_decision.c
 *
 * \brief Algorithmetic MD for:
 * - multi-spatial Enhancement Layer MD;
 * - Scrolling PSkip Decision for screen content
 *
 * \date	2009.7.29
 *

 **************************************************************************************
 */
#include "mv_pred.h"
#include "ls_defines.h"
#include "svc_base_layer_md.h"
#include "svc_mode_decision.h"

namespace WelsSVCEnc {

//
// md in enhancement layer
///

inline bool IsMbStatic (int32_t* pBlockType, EStaticBlockIdc eType) {
  return (pBlockType != NULL &&
          eType == pBlockType[0] &&
          eType == pBlockType[1] &&
          eType == pBlockType[2] &&
          eType == pBlockType[3]);
}
inline bool IsMbCollocatedStatic (int32_t* pBlockType) {
  return IsMbStatic (pBlockType, COLLOCATED_STATIC);
}

inline bool IsMbScrolledStatic (int32_t* pBlockType) {
  return IsMbStatic (pBlockType, SCROLLED_STATIC);
}

inline int32_t CalUVSadCost (SWelsFuncPtrList* pFunc, uint8_t* pEncOri, int32_t iStrideUV, uint8_t* pRefOri,
                             int32_t iRefLineSize) {
  return pFunc->sSampleDealingFuncs.pfSampleSad[BLOCK_8x8] (pEncOri, iStrideUV, pRefOri, iRefLineSize);
}

inline bool CheckBorder (int32_t iMbX, int32_t iMbY, int32_t iScrollMvX, int32_t iScrollMvY, int32_t iMbWidth,
                         int32_t iMbHeight) {
  return ((iMbX << 4) + iScrollMvX < 0 ||
          (iMbX << 4) + iScrollMvX > (iMbWidth - 1) << 4 ||
          (iMbY << 4) + iScrollMvY < 0 ||
          (iMbY << 4) + iScrollMvY > (iMbHeight - 1) << 4
         ); //border check for safety
}

void WelsMdSpatialelInterMbIlfmdNoilp (sWelsEncCtx* pEncCtx, SWelsMD* pWelsMd, SSlice* pSlice,
                                       SMB* pCurMb, const Mb_Type kuiRefMbType) {
  SDqLayer* pCurDqLayer = pEncCtx->pCurDqLayer;
  SMbCache* pMbCache = &pSlice->sMbCacheInfo;

  const uint32_t kuiNeighborAvail = pCurMb->uiNeighborAvail;
  const int32_t kiMbWidth = pCurDqLayer->iMbWidth;
  const  SMB* kpTopMb = pCurMb - kiMbWidth;
  const bool kbMbLeftAvailPskip	= ((kuiNeighborAvail & LEFT_MB_POS) ? IS_SKIP ((pCurMb - 1)->uiMbType) : false);
  const bool kbMbTopAvailPskip			= ((kuiNeighborAvail & TOP_MB_POS) ? IS_SKIP (kpTopMb->uiMbType) : false);
  const bool kbMbTopLeftAvailPskip		= ((kuiNeighborAvail & TOPLEFT_MB_POS) ? IS_SKIP ((kpTopMb - 1)->uiMbType) : false);
  const bool kbMbTopRightAvailPskip	= ((kuiNeighborAvail & TOPRIGHT_MB_POS) ? IS_SKIP ((
                                         kpTopMb + 1)->uiMbType) : false);

  bool bTrySkip  = kbMbLeftAvailPskip | kbMbTopAvailPskip | kbMbTopLeftAvailPskip | kbMbTopRightAvailPskip;
  bool bKeepSkip = kbMbLeftAvailPskip & kbMbTopAvailPskip & kbMbTopRightAvailPskip;
  bool bSkip = false;

  if (pEncCtx->pFuncList->pfInterMdBackgroundDecision (pEncCtx, pWelsMd, pSlice, pCurMb, pMbCache, &bKeepSkip)) {
    return;
  }

  //step 1: try SKIP
  bSkip = WelsMdInterJudgePskip (pEncCtx, pWelsMd, pSlice, pCurMb, pMbCache, bTrySkip);

  if (bSkip && bKeepSkip) {
    WelsMdInterDecidedPskip (pEncCtx,  pSlice,  pCurMb, pMbCache);
    return;
  }

  if (! IS_SVC_INTRA (kuiRefMbType)) {
    if (!bSkip) {
      PredictSad (pMbCache->sMvComponents.iRefIndexCache, pMbCache->iSadCost, 0, &pWelsMd->iSadPredMb);

      //step 2: P_16x16
      pWelsMd->iCostLuma = WelsMdP16x16 (pEncCtx->pFuncList, pCurDqLayer, pWelsMd, pSlice, pCurMb);
      pCurMb->uiMbType = MB_TYPE_16x16;
    }

    WelsMdInterSecondaryModesEnc (pEncCtx, pWelsMd, pSlice, pCurMb, pMbCache, bSkip);
  } else { //BLMODE == SVC_INTRA
    //initial prediction memory for I_16x16
    const int32_t kiCostI16x16 = WelsMdI16x16 (pEncCtx->pFuncList, pEncCtx->pCurDqLayer, pMbCache, pWelsMd->iLambda);
    if (bSkip && (pWelsMd->iCostLuma <= kiCostI16x16)) {
      WelsMdInterDecidedPskip (pEncCtx,  pSlice,  pCurMb, pMbCache);
    } else {
      pWelsMd->iCostLuma = kiCostI16x16;
      pCurMb->uiMbType = MB_TYPE_INTRA16x16;

      WelsMdIntraSecondaryModesEnc (pEncCtx, pWelsMd, pCurMb, pMbCache);
    }
  }
}



void WelsMdInterMbEnhancelayer (void* pEnc, void* pMd, SSlice* pSlice, SMB* pCurMb, SMbCache* pMbCache) {
  sWelsEncCtx* pEncCtx	= (sWelsEncCtx*)pEnc;
  SDqLayer* pCurLayer				= pEncCtx->pCurDqLayer;
  SWelsMD* pWelsMd					= (SWelsMD*)pMd;
  const SMB* kpInterLayerRefMb		= GetRefMb (pCurLayer, pCurMb);
  const Mb_Type kuiInterLayerRefMbType	= kpInterLayerRefMb->uiMbType;

  SetMvBaseEnhancelayer (pWelsMd, pCurMb,
                         kpInterLayerRefMb); // initial sMvBase here only when pRef mb type is inter, if not sMvBase will be not used!
  //step (3): do the MD process
  WelsMdSpatialelInterMbIlfmdNoilp (pEncCtx, pWelsMd, pSlice, pCurMb, kuiInterLayerRefMbType); //MD process
}

///////////////////////
// do initiation for noILP (needed by ILFMD)
////////////////////////

SMB* GetRefMb (SDqLayer* pCurLayer, SMB* pCurMb) {
  const SDqLayer*  kpRefLayer		= pCurLayer->pRefLayer;
  const int32_t  kiRefMbIdx = (pCurMb->iMbY >> 1) * kpRefLayer->iMbWidth + (pCurMb->iMbX >>
                              1); //because current lower layer is half size on both vertical and horizontal
  return (&kpRefLayer->sMbDataP[kiRefMbIdx]);
}

void SetMvBaseEnhancelayer (SWelsMD* pMd, SMB* pCurMb, const SMB* kpRefMb) {
  const Mb_Type kuiRefMbType = kpRefMb->uiMbType;

  if (! IS_SVC_INTRA (kuiRefMbType)) {
    SMVUnitXY sMv;
    int32_t iRefMbPartIdx = ((pCurMb->iMbY & 0x01) << 1) + (pCurMb->iMbX & 0x01); //may be need modified
    int32_t iScan4RefPartIdx = g_kuiMbCountScan4Idx[ (iRefMbPartIdx << 2)];
    sMv.iMvX = kpRefMb->sMv[iScan4RefPartIdx].iMvX << 1;
    sMv.iMvY = kpRefMb->sMv[iScan4RefPartIdx].iMvY << 1;

    pMd->sMe.sMe16x16.sMvBase = sMv;

    pMd->sMe.sMe8x8[0].sMvBase =
      pMd->sMe.sMe8x8[1].sMvBase =
        pMd->sMe.sMe8x8[2].sMvBase =
          pMd->sMe.sMe8x8[3].sMvBase = sMv;

    pMd->sMe.sMe16x8[0].sMvBase =
      pMd->sMe.sMe16x8[1].sMvBase =
        pMd->sMe.sMe8x16[0].sMvBase =
          pMd->sMe.sMe8x16[1].sMvBase = sMv;
  }
}

bool JudgeStaticSkip (sWelsEncCtx* pEncCtx, SMB* pCurMb, SMbCache* pMbCache, SWelsMD* pWelsMd) {
  SDqLayer* pCurDqLayer			= pEncCtx->pCurDqLayer;
  const int32_t kiMbX = pCurMb->iMbX;
  const int32_t kiMbY = pCurMb->iMbY;

  bool bTryStaticSkip = IsMbCollocatedStatic (pWelsMd->iBlock8x8StaticIdc);

  if (bTryStaticSkip) {//test if UV component can be skipped
    int32_t iStrideUV, iOffsetUV;
    SWelsFuncPtrList* pFunc = pEncCtx->pFuncList;
    SPicture* pRefOri = pCurDqLayer->pRefOri;
    if (pRefOri != NULL) {
      iStrideUV	= pCurDqLayer->iEncStride[1];
      iOffsetUV	= (kiMbX + kiMbY * iStrideUV) <<
                  3;//TODO: To Li: can this * be replaced with +, at the begining of MB calc? Sijia, 121231

      int32_t iSadCostCb = CalUVSadCost (pFunc, pMbCache->SPicData.pEncMb[1], iStrideUV, pRefOri->pData[1] + iOffsetUV,
                                         pRefOri->iLineSize[1]);
      int32_t iSadCostCr = CalUVSadCost (pFunc, pMbCache->SPicData.pEncMb[2], iStrideUV, pRefOri->pData[2] + iOffsetUV,
                                         pRefOri->iLineSize[1]);
      bTryStaticSkip = (iSadCostCb + iSadCostCr == 0);
    } else { //this should not happen!!
      bTryStaticSkip = false;
    }
  }
  return bTryStaticSkip;
}

bool JudgeScrollSkip (sWelsEncCtx* pEncCtx, SMB* pCurMb, SMbCache* pMbCache, SWelsMD* pWelsMd) {
  SDqLayer* pCurDqLayer			= pEncCtx->pCurDqLayer;
  const int32_t kiMbX = pCurMb->iMbX;
  const int32_t kiMbY = pCurMb->iMbY;
  const int32_t kiMbWidth = pCurDqLayer->iMbWidth;
  const int32_t kiMbHeight = pCurDqLayer->iMbHeight;
  //	const int32_t block_width = mb_width << 1;
  SVAAFrameInfoExt_t* pVaaExt = static_cast<SVAAFrameInfoExt_t*> (pEncCtx->pVaa);

  bool bTryScrollSkip = 0;

  if (pVaaExt->sScrollDetectInfo.bScrollDetectFlag)
    bTryScrollSkip = IsMbCollocatedStatic (pWelsMd->iBlock8x8StaticIdc);
  else return 0;

  if (bTryScrollSkip) { //test if UV component can be skipped
    int32_t iStrideUV, iOffsetUV;
    SWelsFuncPtrList* pFunc = pEncCtx->pFuncList;
    SPicture* pRefOri = pCurDqLayer->pRefOri;
    if (pRefOri != NULL) {
      int32_t iScrollMvX = pVaaExt->sScrollDetectInfo.iScrollMvX;
      int32_t iScrollMvY = pVaaExt->sScrollDetectInfo.iScrollMvY;
      if (CheckBorder (kiMbX, kiMbY, iScrollMvX, iScrollMvY, kiMbWidth, kiMbHeight)) {
        bTryScrollSkip =  false;
      } else {
        iStrideUV	= pCurDqLayer->iEncStride[1];
        iOffsetUV	= (kiMbX << 3) + (iScrollMvX >> 1) + ((kiMbX << 3) + (iScrollMvY >> 1)) * iStrideUV;

        int32_t iSadCostCb = CalUVSadCost (pFunc, pMbCache->SPicData.pEncMb[1], iStrideUV, pRefOri->pData[1] + iOffsetUV,
                                           pRefOri->iLineSize[1]);
        int32_t iSadCostCr = CalUVSadCost (pFunc, pMbCache->SPicData.pEncMb[2], iStrideUV, pRefOri->pData[2] + iOffsetUV,
                                           pRefOri->iLineSize[1]);

        bTryScrollSkip = (iSadCostCb + iSadCostCr == 0);
      }
    } else { //this should not happen
      bTryScrollSkip = false;
    }
  }
  return bTryScrollSkip;
}

void SvcMdSCDMbEnc (sWelsEncCtx* pEncCtx, SWelsMD* pWelsMd, SMB* pCurMb, SMbCache* pMbCache, SSlice* pSlice,
                    bool bQpSimilarFlag,
                    bool bMbSkipFlag, SMVUnitXY sCurMbMv[], ESkipModes eSkipMode) {
  SDqLayer* pCurDqLayer		= pEncCtx->pCurDqLayer;
  SWelsFuncPtrList* pFunc	= pEncCtx->pFuncList;
  SMVUnitXY sMvp					= { 0};
  ST16 (&sMvp.iMvX, sCurMbMv[eSkipMode].iMvX);
  ST16 (&sMvp.iMvY, sCurMbMv[eSkipMode].iMvY);
  uint8_t* pRefLuma			= pMbCache->SPicData.pRefMb[0];
  uint8_t* pRefCb				= pMbCache->SPicData.pRefMb[1];
  uint8_t* pRefCr				= pMbCache->SPicData.pRefMb[2];
  int32_t iLineSizeY		= pCurDqLayer->pRefPic->iLineSize[0];
  int32_t iLineSizeUV		= pCurDqLayer->pRefPic->iLineSize[1];
  uint8_t* pDstLuma			= pMbCache->pSkipMb;
  uint8_t* pDstCb				= pMbCache->pSkipMb + 256;
  uint8_t* pDstCr				= pMbCache->pSkipMb + 256 + 64;

  const int32_t iOffsetY	= (sCurMbMv[eSkipMode].iMvX >> 2) + (sCurMbMv[eSkipMode].iMvY >> 2) * iLineSizeY;
  const int32_t iOffsetUV = (sCurMbMv[eSkipMode].iMvX >> 3) + (sCurMbMv[eSkipMode].iMvY >> 3) * iLineSizeUV;

  if (!bQpSimilarFlag || !bMbSkipFlag) {
    pDstLuma = pMbCache->pMemPredLuma;
    pDstCb	= pMbCache->pMemPredChroma;
    pDstCr	= pMbCache->pMemPredChroma + 64;
  }
  //MC
  pFunc->sMcFuncs.pfLumaQuarpelMc[0] (pRefLuma + iOffsetY, iLineSizeY, pDstLuma, 16, 16);
  pFunc->sMcFuncs.pfChromaMc (pRefCb + iOffsetUV, iLineSizeUV, pDstCb, 8, sMvp, 8, 8);
  pFunc->sMcFuncs.pfChromaMc (pRefCr + iOffsetUV, iLineSizeUV, pDstCr, 8, sMvp, 8, 8);

  pCurMb->uiCbp = 0;
  pWelsMd->iCostLuma = 0;
  pCurMb->pSadCost[0] = pFunc->sSampleDealingFuncs.pfSampleSad[BLOCK_16x16] (pMbCache->SPicData.pEncMb[0],
                        pCurDqLayer->iEncStride[0], pRefLuma + iOffsetY, iLineSizeY);

  pWelsMd->iCostSkipMb = pCurMb->pSadCost[0];

  ST16 (& (pCurMb->sP16x16Mv.iMvX), sCurMbMv[eSkipMode].iMvX);
  ST16 (& (pCurMb->sP16x16Mv.iMvY), sCurMbMv[eSkipMode].iMvY);

  ST16 (& (pCurDqLayer->pDecPic->sMvList[pCurMb->iMbXY].iMvX), sCurMbMv[eSkipMode].iMvX);
  ST16 (& (pCurDqLayer->pDecPic->sMvList[pCurMb->iMbXY].iMvY), sCurMbMv[eSkipMode].iMvY);

  if (bQpSimilarFlag && bMbSkipFlag) {
    //update motion info to current MB
    ST32 (pCurMb->pRefIndex, 0);
    pFunc->pfUpdateMbMv (pCurMb->sMv, sMvp);
    pCurMb->uiMbType = MB_TYPE_SKIP;
    WelsRecPskip (pCurDqLayer, pEncCtx->pFuncList, pCurMb, pMbCache);
    WelsMdInterUpdatePskip (pCurDqLayer, pSlice, pCurMb, pMbCache);
    return;
  }

  pCurMb->uiMbType = MB_TYPE_16x16;

  pWelsMd->sMe.sMe16x16.sMv.iMvX = sCurMbMv[eSkipMode].iMvX;
  pWelsMd->sMe.sMe16x16.sMv.iMvY = sCurMbMv[eSkipMode].iMvY;
  PredMv (&pMbCache->sMvComponents, 0, 4, 0, &pWelsMd->sMe.sMe16x16.sMvp);
  pMbCache->sMbMvp[0] = pWelsMd->sMe.sMe16x16.sMvp;

  UpdateP16x16MotionInfo (pMbCache, pCurMb, 0, &pWelsMd->sMe.sMe16x16.sMv);

  if (pWelsMd->bMdUsingSad)
    pWelsMd->iCostLuma = pCurMb->pSadCost[0];
  else
    pWelsMd->iCostLuma = pFunc->sSampleDealingFuncs.pfSampleSad[BLOCK_16x16] (pMbCache->SPicData.pEncMb[0],
                         pCurDqLayer->iEncStride[0], pRefLuma, iLineSizeY);

  WelsInterMbEncode (pEncCtx, pSlice, pCurMb);
  WelsPMbChromaEncode (pEncCtx, pSlice, pCurMb);

  pFunc->pfCopy16x16Aligned (pMbCache->SPicData.pCsMb[0], pCurDqLayer->iCsStride[0], pMbCache->pMemPredLuma, 16);
  pFunc->pfCopy8x8Aligned (pMbCache->SPicData.pCsMb[1], pCurDqLayer->iCsStride[1], pMbCache->pMemPredChroma, 8);
  pFunc->pfCopy8x8Aligned (pMbCache->SPicData.pCsMb[2], pCurDqLayer->iCsStride[1], pMbCache->pMemPredChroma + 64, 8);
}

bool MdInterSCDPskipProcess (sWelsEncCtx* pEncCtx, SWelsMD* pWelsMd, SSlice* pSlice, SMB* pCurMb, SMbCache* pMbCache,
                             ESkipModes eSkipMode) {
  SVAAFrameInfoExt_t* pVaaExt		= static_cast<SVAAFrameInfoExt_t*> (pEncCtx->pVaa);
  SDqLayer* pCurDqLayer			= pEncCtx->pCurDqLayer;

  const int32_t kiRefMbQp = pCurDqLayer->pRefPic->pRefMbQp[pCurMb->iMbXY];
  const int32_t kiCurMbQp = pCurMb->uiLumaQp;// unsigned -> signed

  pJudgeSkipFun pJudeSkip[2] = {JudgeStaticSkip, JudgeScrollSkip};
  bool bSkipFlag = pJudeSkip[eSkipMode] (pEncCtx, pCurMb, pMbCache, pWelsMd);

  if (bSkipFlag) {
    bool bQpSimilarFlag = (kiRefMbQp - kiCurMbQp <= DELTA_QP_SCD_THD || kiRefMbQp <= 26);
    SMVUnitXY	sVaaPredSkipMv = { 0 }, sCurMbMv[2] = {0, 0, 0, 0};
    PredSkipMv (pMbCache, &sVaaPredSkipMv);

    if (eSkipMode == SCROLLED) {
      sCurMbMv[1].iMvX = static_cast<int16_t> (pVaaExt->sScrollDetectInfo.iScrollMvX << 2);
      sCurMbMv[1].iMvY = static_cast<int16_t> (pVaaExt->sScrollDetectInfo.iScrollMvY << 2);
    }

    bool bMbSkipFlag = (LD32 (&sVaaPredSkipMv) ==  LD32 (&sCurMbMv[eSkipMode])) ;
    SvcMdSCDMbEnc (pEncCtx, pWelsMd, pCurMb, pMbCache, pSlice, bQpSimilarFlag, bMbSkipFlag, sCurMbMv, eSkipMode);

    return true;
  }

  return false;
}

void SetBlockStaticIdcToMd (void* pVaa, void* pMd, SMB* pCurMb, void* pDqLay) {
  SVAAFrameInfoExt_t* pVaaExt = static_cast<SVAAFrameInfoExt_t*> (pVaa);
  SWelsMD* pWelsMd = static_cast<SWelsMD*> (pMd);
  SDqLayer* pDqLayer = static_cast<SDqLayer*> (pDqLay);

  const int32_t kiMbX = pCurMb->iMbX;
  const int32_t kiMbY = pCurMb->iMbY;
  const int32_t kiMbWidth = pDqLayer->iMbWidth;
  const int32_t kiWidth = kiMbWidth << 1;

  const int32_t kiBlockIndexUp = (kiMbY << 1) * kiWidth + (kiMbX << 1);
  const int32_t kiBlockIndexLow = ((kiMbY << 1) + 1) * kiWidth + (kiMbX << 1);

  //fill_blockstaticidc with pVaaExt->pVaaBestBlockStaticIdc
  pWelsMd->iBlock8x8StaticIdc[0] = pVaaExt->pVaaBestBlockStaticIdc[kiBlockIndexUp];
  pWelsMd->iBlock8x8StaticIdc[1] = pVaaExt->pVaaBestBlockStaticIdc[kiBlockIndexUp + 1];
  pWelsMd->iBlock8x8StaticIdc[2] = pVaaExt->pVaaBestBlockStaticIdc[kiBlockIndexLow];
  pWelsMd->iBlock8x8StaticIdc[3] = pVaaExt->pVaaBestBlockStaticIdc[kiBlockIndexLow + 1];

}

///////////////////////
// Scene Change Detection (SCD) PSkip Decision for screen content
////////////////////////
bool WelsMdInterJudgeSCDPskip (void* pCtx, void* pMd, SSlice* slice, SMB* pCurMb, SMbCache* pMbCache) {
  sWelsEncCtx* pEncCtx	= (sWelsEncCtx*)pCtx;
  SWelsMD* pWelsMd					= (SWelsMD*)pMd;
  SDqLayer* pCurDqLayer			= pEncCtx->pCurDqLayer;

  SetBlockStaticIdcToMd (pEncCtx->pVaa, pWelsMd, pCurMb, pCurDqLayer);

  //try static Pskip;

  //try scrolled Pskip
  //TBD

  return false;
}
bool WelsMdInterJudgeSCDPskipFalse (void* pEncCtx, void* pWelsMd, SSlice* slice, SMB* pCurMb, SMbCache* pMbCache) {
  return false;
}


void WelsInitScrollingSkipFunc (SWelsFuncPtrList* pFuncList, const bool bScrollingDetection) {
  if (bScrollingDetection) {
    pFuncList->pfScrollingPSkipDecision = WelsMdInterJudgeSCDPskip;
  } else {
    pFuncList->pfScrollingPSkipDecision = WelsMdInterJudgeSCDPskipFalse;
  }
}


} // namespace WelsSVCEnc
