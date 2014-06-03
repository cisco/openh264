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
#include "svc_base_layer_md.h"

#include "svc_mode_decision.h"

namespace WelsSVCEnc {

//
// md in enhancement layer
///
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

void SetBlockStaticIdcToMd (void* pVaa, void* pMd, SMB* pCurMb, void* pDqLay) { //TODO: OPT?
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
bool WelsMdInterJudgeSCDPskip (void* pEncCtx, void* pWelsMd, SSlice* slice, SMB* pCurMb, SMbCache* pMbCache) {
  sWelsEncCtx* pCtx	= (sWelsEncCtx*)pEncCtx;
  SWelsMD* pMd					= (SWelsMD*)pWelsMd;
  SDqLayer* pCurDqLayer			= pCtx->pCurDqLayer;

  SetBlockStaticIdcToMd (pCtx->pVaa, pMd, pCurMb, pCurDqLayer);

  //try static Pskip;

  //try scrolled Pskip
  //TBD

  return false;
}
bool WelsMdInterJudgeSCDPskipFalse (void* pEncCtx, void* pWelsMd, SSlice* slice, SMB* pCurMb,
                                    SMbCache* pMbCache) {
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
