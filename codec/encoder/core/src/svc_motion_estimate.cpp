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
 * \file  svc motion estimate.c
 *
 * \brief  Interfaces introduced in svc mb motion estimation
 *
 * \date  08/11/2009 Created
 *
 *************************************************************************************
 */

#include "cpu_core.h"
#include "ls_defines.h"
#include "svc_motion_estimate.h"

namespace WelsSVCEnc {

static inline void UpdateMeResults( const SMVUnitXY ksBestMv, const uint32_t kiBestSadCost, uint8_t* pRef, SWelsME * pMe )
{
  pMe->sMv = ksBestMv;
  pMe->pRefMb = pRef;
  pMe->uiSadCost = kiBestSadCost;
}
static inline void MeEndIntepelSearch( SWelsME * pMe )
{
    /* -> qpel mv */
    pMe->sMv.iMvX <<= 2;
    pMe->sMv.iMvY <<= 2;
    pMe->uiSatdCost = pMe->uiSadCost;
}

void WelsInitMeFunc( SWelsFuncPtrList* pFuncList, uint32_t uiCpuFlag, bool bScreenContent ) {
  if (!bScreenContent) {
    pFuncList->pfCheckDirectionalMv = CheckDirectionalMvFalse;
  } else {
    pFuncList->pfCheckDirectionalMv = CheckDirectionalMv;

    //for cross serarch
    pFuncList->pfLineFullSearch = LineFullSearch_c;
    if ( uiCpuFlag & WELS_CPU_SSE41 ) {
    }

    //for feature search
    pFuncList->pfCalculateBlockFeatureOfFrame[0] = SumOf8x8BlockOfFrame_c;
    pFuncList->pfCalculateBlockFeatureOfFrame[1] = SumOf16x16BlockOfFrame_c;
    //TODO: it is possible to differentiate width that is times of 8, so as to accelerate the speed when width is times of 8?
    pFuncList->pfCalculateSingleBlockFeature[0] = SumOf8x8SingleBlock_c;
    pFuncList->pfCalculateSingleBlockFeature[1] = SumOf16x16SingleBlock_c;
  }
}

/*!
 * \brief  BL mb motion estimate search
 *
 * \param  enc      Wels encoder context
 * \param  pMe          Wels me information
 *
 * \return  NONE
 */

void WelsMotionEstimateSearch (SWelsFuncPtrList* pFuncList, void* pLplayer, void* pLpme, void* pLpslice) {
  SDqLayer* pCurDqLayer			= (SDqLayer*)pLplayer;
  SWelsME* pMe						= (SWelsME*)pLpme;
  SSlice* pSlice					= (SSlice*)pLpslice;
  const int32_t kiStrideEnc = pCurDqLayer->iEncStride[0];
  const int32_t kiStrideRef = pCurDqLayer->pRefPic->iLineSize[0];

  //  Step 1: Initial point prediction
  if ( !WelsMotionEstimateInitialPoint (pFuncList, pMe, pSlice, kiStrideEnc, kiStrideRef) ) {
    pFuncList->pfSearchMethod[pMe->uiBlockSize] (pFuncList, pMe, pSlice, kiStrideEnc, kiStrideRef);
    MeEndIntepelSearch(pMe);
  }

  pFuncList->pfCalculateSatd( pFuncList->sSampleDealingFuncs.pfSampleSatd[pMe->uiBlockSize], pMe, kiStrideEnc, kiStrideRef );
}

/*!
 * \brief  EL mb motion estimate initial point testing
 *
 * \param  pix_pFuncList  SSampleDealingFunc
 * \param  pMe          Wels me information
 * \param  mv_range  search range in motion estimate
 * \param  point      the best match point in motion estimation
 *
 * \return  NONE
 */
bool WelsMotionEstimateInitialPoint (SWelsFuncPtrList* pFuncList, SWelsME* pMe, SSlice* pSlice, int32_t iStrideEnc,
                                     int32_t iStrideRef) {
  PSampleSadSatdCostFunc pSad    = pFuncList->sSampleDealingFuncs.pfSampleSad[pMe->uiBlockSize];
  const uint16_t* kpMvdCost  = pMe->pMvdCost;
  uint8_t* const kpEncMb    = pMe->pEncMb;
  int16_t iMvc0, iMvc1;
  int32_t iSadCost;
  int32_t iBestSadCost;
  uint8_t* pRefMb;
  uint8_t* pFref2;
  uint32_t i;
  const uint32_t kuiMvcNum    = pSlice->uiMvcNum;
  const SMVUnitXY* kpMvcList  = &pSlice->sMvc[0];
  const SMVUnitXY ksMvStartMin    = pSlice->sMvStartMin;
  const SMVUnitXY ksMvStartMax    = pSlice->sMvStartMax;
  const SMVUnitXY ksMvp    = pMe->sMvp;
  SMVUnitXY sMv;

  //  Step 1: Initial point prediction
  // init with sMvp
  sMv.iMvX  = WELS_CLIP3 ((2 + ksMvp.iMvX) >> 2, ksMvStartMin.iMvX, ksMvStartMax.iMvX);
  sMv.iMvY  = WELS_CLIP3 ((2 + ksMvp.iMvY) >> 2, ksMvStartMin.iMvY, ksMvStartMax.iMvY);

  pRefMb = &pMe->pRefMb[sMv.iMvY * iStrideRef + sMv.iMvX];

  iBestSadCost = pSad (kpEncMb, iStrideEnc, pRefMb, iStrideRef);
  iBestSadCost += COST_MVD (kpMvdCost, ((sMv.iMvX) << 2) - ksMvp.iMvX, ((sMv.iMvY) << 2) - ksMvp.iMvY);

  for (i = 0; i < kuiMvcNum; i++) {
    //clipping here is essential since some pOut-of-range MVC may happen here (i.e., refer to baseMV)
    iMvc0 = WELS_CLIP3 ((2 + kpMvcList[i].iMvX) >> 2, ksMvStartMin.iMvX, ksMvStartMax.iMvX);
    iMvc1 = WELS_CLIP3 ((2 + kpMvcList[i].iMvY) >> 2, ksMvStartMin.iMvY, ksMvStartMax.iMvY);

    if (((iMvc0 - sMv.iMvX) || (iMvc1 - sMv.iMvY))) {
      pFref2 = &pMe->pRefMb[iMvc1 * iStrideRef + iMvc0];

      iSadCost = pSad (kpEncMb, iStrideEnc, pFref2, iStrideRef) +
                 COST_MVD (kpMvdCost, (iMvc0 << 2) - ksMvp.iMvX, (iMvc1 << 2) - ksMvp.iMvY);

      if (iSadCost < iBestSadCost) {
        sMv.iMvX = iMvc0;
        sMv.iMvY = iMvc1;
        pRefMb = pFref2;
        iBestSadCost = iSadCost;
      }
    }
  }

  if ( pFuncList->pfCheckDirectionalMv
    (pSad, pMe, ksMvStartMin, ksMvStartMax, iStrideEnc, iStrideRef, iSadCost) ) {
      sMv = pMe->sDirectionalMv;
      pRefMb =  &pMe->pColoRefMb[sMv.iMvY * iStrideRef + sMv.iMvX];
      iBestSadCost = iSadCost;
  }

  UpdateMeResults( sMv, iBestSadCost, pRefMb, pMe );
  if ( iBestSadCost < static_cast<int32_t>(pMe->uSadPredISatd.uiSadPred) ) {
    //Initial point early Stop
    MeEndIntepelSearch(pMe);
    return true;
  }
  return false;
}

void CalculateSatdCost( PSampleSadSatdCostFunc pSatd, void * vpMe,
                       const int32_t kiEncStride, const int32_t kiRefStride ) {
  SWelsME* pMe             = static_cast<SWelsME *>(vpMe);
  pMe->uSadPredISatd.uiSatd = pSatd(pMe->pEncMb, kiEncStride, pMe->pRefMb, kiRefStride);
  pMe->uiSatdCost = pMe->uSadPredISatd.uiSatd + COST_MVD (pMe->pMvdCost, pMe->sMv.iMvX - pMe->sMvp.iMvX,
                                                            pMe->sMv.iMvY - pMe->sMvp.iMvY);
}
void NotCalculateSatdCost( PSampleSadSatdCostFunc pSatd, void * vpMe,
                          const int32_t kiEncStride, const int32_t kiRefStride ) {
}


/////////////////////////
// Diamond Search Basics
/////////////////////////
bool WelsMeSadCostSelect (int32_t* iSadCost, const uint16_t* kpMvdCost, int32_t* pBestCost, const int32_t kiDx,
                            const int32_t kiDy, int32_t* pIx, int32_t* pIy) {
  int32_t iTempSadCost[4];
  int32_t iInputSadCost = *pBestCost;
  iTempSadCost[0] = iSadCost[0] + COST_MVD (kpMvdCost, kiDx, kiDy - 4);
  iTempSadCost[1] = iSadCost[1] + COST_MVD (kpMvdCost, kiDx, kiDy + 4);
  iTempSadCost[2] = iSadCost[2] + COST_MVD (kpMvdCost, kiDx - 4, kiDy);
  iTempSadCost[3] = iSadCost[3] + COST_MVD (kpMvdCost, kiDx + 4, kiDy);

  if (iTempSadCost[0] < *pBestCost) {
    *pBestCost = iTempSadCost[0];
    *pIx = 0;
    *pIy = 1;
  }

  if (iTempSadCost[1] < *pBestCost) {
    *pBestCost = iTempSadCost[1];
    *pIx = 0;
    *pIy = -1;
  }

  if (iTempSadCost[2] < *pBestCost) {
    *pBestCost = iTempSadCost[2];
    *pIx = 1;
    *pIy = 0;
  }

  if (iTempSadCost[3] < *pBestCost) {
    *pBestCost = iTempSadCost[3];
    *pIx = -1;
    *pIy = 0;
  }
  return (*pBestCost == iInputSadCost);
}

void WelsDiamondSearch (SWelsFuncPtrList* pFuncList, void* pLpme, void* pLpslice,
                        const int32_t kiStrideEnc,  const int32_t kiStrideRef) {
  SWelsME* pMe						= (SWelsME*)pLpme;
  PSample4SadCostFunc			pSad					=  pFuncList->sSampleDealingFuncs.pfSample4Sad[pMe->uiBlockSize];

  uint8_t* pFref = pMe->pRefMb;
  uint8_t* const kpEncMb = pMe->pEncMb;
  const uint16_t* kpMvdCost = pMe->pMvdCost;

  int32_t iMvDx = ((pMe->sMv.iMvX) << 2) - pMe->sMvp.iMvX;
  int32_t iMvDy = ((pMe->sMv.iMvY) << 2) - pMe->sMvp.iMvY;

  uint8_t* pRefMb = pFref;
  int32_t iBestCost = (pMe->uiSadCost);

  int32_t iTimeThreshold = ITERATIVE_TIMES;
  ENFORCE_STACK_ALIGN_1D (int32_t, iSadCosts, 4, 16)

  while (iTimeThreshold--) {
    pSad (kpEncMb, kiStrideEnc, pRefMb, kiStrideRef, &iSadCosts[0]);

    int32_t iX, iY;

    const bool kbIsBestCostWorse = WelsMeSadCostSelect (iSadCosts, kpMvdCost, &iBestCost, iMvDx, iMvDy, &iX, &iY);
    if (kbIsBestCostWorse)
      break;

    iMvDx -= iX << 2 ;
    iMvDy -= iY << 2 ;

    pRefMb -= (iX + iY * kiStrideRef);

  }

  /* integer-pel mv */
  pMe->sMv.iMvX = (iMvDx + pMe->sMvp.iMvX) >>2;
  pMe->sMv.iMvY = (iMvDy + pMe->sMvp.iMvY) >>2;
  pMe->uiSatdCost = pMe->uiSadCost = (iBestCost);
  pMe->pRefMb = pRefMb;
}

/////////////////////////
// DirectionalMv Basics
/////////////////////////
bool CheckDirectionalMv(PSampleSadSatdCostFunc pSad, void * vpMe,
                      const SMVUnitXY ksMinMv, const SMVUnitXY ksMaxMv, const int32_t kiEncStride, const int32_t kiRefStride,
                      int32_t& iBestSadCost) {
  SWelsME* pMe             = static_cast<SWelsME *>(vpMe);
  const int16_t kiMvX = pMe->sDirectionalMv.iMvX;
  const int16_t kiMvY = pMe->sDirectionalMv.iMvY;

  //Check MV from scrolling detection
  if ( (BLOCK_16x16!=pMe->uiBlockSize) //scrolled_MV with P16x16 is checked SKIP checking function
    && ( kiMvX | kiMvY ) //(0,0) checked in ordinary initial point checking
    && CheckMvInRange( pMe->sDirectionalMv, ksMinMv, ksMaxMv ) ) {
    uint8_t* pRef = &pMe->pColoRefMb[kiMvY * kiRefStride + kiMvX];
    uint32_t uiCurrentSadCost = pSad( pMe->pEncMb, kiEncStride,  pRef, kiRefStride ) +
      COST_MVD(pMe->pMvdCost, (kiMvX<<2) - pMe->sMvp.iMvX, (kiMvY<<2) - pMe->sMvp.iMvY );
    if( uiCurrentSadCost < pMe->uiSadCost ) {
      iBestSadCost = uiCurrentSadCost;
      return true;
    }
  }
  return false;
}

bool CheckDirectionalMvFalse(PSampleSadSatdCostFunc pSad, void * vpMe,
                      const SMVUnitXY ksMinMv, const SMVUnitXY ksMaxMv, const int32_t kiEncStride, const int32_t kiRefStride,
                      int32_t& iBestSadCost) {
  return false;
}

/////////////////////////
// Cross Search Basics
/////////////////////////
void VerticalFullSearchUsingSSE41( void *pFunc, void *vpMe,
                            uint16_t* pMvdTable, const int32_t kiFixedMvd,
                            const int32_t kiEncStride, const int32_t kiRefStride,
                          const int32_t kiMinPos, const int32_t kiMaxPos,
                          const bool bVerticalSearch ) {
  SWelsFuncPtrList *pFuncList      = static_cast<SWelsFuncPtrList *>(pFunc);
  SWelsME *pMe                            = static_cast<SWelsME *>(vpMe);
}
void LineFullSearch_c(  void *pFunc, void *vpMe,
                          uint16_t* pMvdTable, const int32_t kiFixedMvd,
                          const int32_t kiEncStride, const int32_t kiRefStride,
                          const int32_t kiMinPos, const int32_t kiMaxPos,
                          const bool bVerticalSearch ) {
  SWelsFuncPtrList *pFuncList      = static_cast<SWelsFuncPtrList *>(pFunc);
  SWelsME *pMe                            = static_cast<SWelsME *>(vpMe);
  PSampleSadSatdCostFunc pSad = pFuncList->sSampleDealingFuncs.pfSampleSad[pMe->uiBlockSize];
  const int32_t kiCurMeBlockPix  = bVerticalSearch?pMe->iCurMeBlockPixY:pMe->iCurMeBlockPixX;
  const int32_t kiStride = bVerticalSearch?kiRefStride:1;
  uint8_t* pRef            = &pMe->pColoRefMb[(kiMinPos - kiCurMeBlockPix)*kiStride];
  uint16_t* pMvdCost  = &(pMvdTable[kiMinPos<<2]);
  uint32_t uiBestCost    = 0xFFFFFFFF;
  int32_t iBestPos       = 0;

  for ( int32_t iTargetPos = kiMinPos; iTargetPos < kiMaxPos; ++ iTargetPos ) {
    uint8_t* const kpEncMb  = pMe->pEncMb;
    uint32_t uiSadCost = pSad( kpEncMb, kiEncStride, pRef, kiRefStride ) + (kiFixedMvd + *pMvdCost);
    if (uiSadCost < uiBestCost) {
      uiBestCost  = uiSadCost;
      iBestPos  = iTargetPos;
    }
    pRef += kiStride;
    pMvdCost+=4;
  }

  if (uiBestCost < pMe->uiSadCost) {
    SMVUnitXY sBestMv;
    sBestMv.iMvX = bVerticalSearch?0:(iBestPos - kiCurMeBlockPix);
    sBestMv.iMvY = bVerticalSearch?(iBestPos - kiCurMeBlockPix):0;
    UpdateMeResults( sBestMv, uiBestCost, &pMe->pColoRefMb[sBestMv.iMvY*kiStride], pMe );
  }
}

void WelsMotionCrossSearch(SWelsFuncPtrList *pFuncList,  SWelsME * pMe,
											const SSlice* pSlice, const int32_t kiEncStride,  const int32_t kiRefStride) {
  PLineFullSearchFunc pfVerticalFullSearchFunc	= pFuncList->pfLineFullSearch;
  PLineFullSearchFunc pfHorizontalFullSearchFunc	= pFuncList->pfLineFullSearch;

  const int32_t iCurMeBlockPixX = pMe->iCurMeBlockPixX;
  const int32_t iCurMeBlockQpelPixX = ((iCurMeBlockPixX)<<2);
  const int32_t iCurMeBlockPixY = pMe->iCurMeBlockPixY;
  const int32_t iCurMeBlockQpelPixY = ((iCurMeBlockPixY)<<2);
  uint16_t* pMvdCostX = pMe->pMvdCost - iCurMeBlockQpelPixX - pMe->sMvp.iMvX;//do the offset here instead of in the search
  uint16_t* pMvdCostY = pMe->pMvdCost - iCurMeBlockQpelPixY - pMe->sMvp.iMvY;//do the offset here instead of in the search

  //vertical search
  pfVerticalFullSearchFunc( pFuncList, pMe,
    pMvdCostY, pMvdCostX[ iCurMeBlockQpelPixX ],
    kiEncStride, kiRefStride,
    iCurMeBlockPixY + pSlice->sMvStartMin.iMvY,
    iCurMeBlockPixY + pSlice->sMvStartMax.iMvY, true );

  //horizontal search
  if (pMe->uiSadCost >= pMe->uiSadCostThreshold) {
    pfHorizontalFullSearchFunc( pFuncList, pMe,
      pMvdCostX, pMvdCostY[ iCurMeBlockQpelPixY ],
      kiEncStride, kiRefStride,
      iCurMeBlockPixX + pSlice->sMvStartMin.iMvX,
      iCurMeBlockPixX + pSlice->sMvStartMax.iMvX,
      false );
  }
}

/////////////////////////
// Feature Search Basics
/////////////////////////
//memory related
int32_t RequestFeatureSearchPreparation( CMemoryAlign *pMa, const int32_t kiFeatureStrategyIndex,
                                         const int32_t kiFrameWidth,  const int32_t kiFrameHeight, const bool bFme8x8,
                                         uint16_t*& pFeatureOfBlock) {
  const int32_t kiMarginSize = bFme8x8?8:16;
  const int32_t kiFrameSize = (kiFrameWidth-kiMarginSize) * (kiFrameHeight-kiMarginSize);
  int32_t iListOfFeatureOfBlock;

  if (0==kiFeatureStrategyIndex) {
    iListOfFeatureOfBlock =sizeof(uint16_t) * kiFrameSize;
  } else {
    iListOfFeatureOfBlock = sizeof(uint16_t) * kiFrameSize +
      (kiFrameWidth-kiMarginSize) * sizeof(uint32_t) + kiFrameWidth * 8 * sizeof(uint8_t);
  }
  pFeatureOfBlock =
    (uint16_t *)pMa->WelsMalloc(iListOfFeatureOfBlock, "pFeatureOfBlock");
  WELS_VERIFY_RETURN_IF(ENC_RETURN_MEMALLOCERR, NULL == pFeatureOfBlock)

  return ENC_RETURN_SUCCESS;
}
int32_t ReleaseFeatureSearchPreparation( CMemoryAlign *pMa, uint16_t*& pFeatureOfBlock) {
  if ( pMa && pFeatureOfBlock ) {
    pMa->WelsFree( pFeatureOfBlock, "pFeatureOfBlock");
    pFeatureOfBlock=NULL;
    return ENC_RETURN_SUCCESS;
  }
  return ENC_RETURN_UNEXPECTED;
}
int32_t RequestScreenBlockFeatureStorage( CMemoryAlign *pMa, const int32_t kiFeatureStrategyIndex,
                                         const int32_t kiFrameWidth,  const int32_t kiFrameHeight, const int32_t kiMe16x16,  const int32_t kiMe8x8,
                                         SScreenBlockFeatureStorage* pScreenBlockFeatureStorage) {
#define LIST_SIZE_SUM_16x16  0x0FF01    //(256*255+1)
#define LIST_SIZE_SUM_8x8      0x03FC1    //(64*255+1)

  if (((kiMe8x8&ME_FME)==ME_FME) && ((kiMe16x16&ME_FME)==ME_FME)) {
    return ENC_RETURN_UNSUPPORTED_PARA;
    //the following memory allocation cannot support when FME at both size
  }

  const bool bIsBlock8x8 = ((kiMe8x8 & ME_FME)==ME_FME);
  const int32_t kiMarginSize = bIsBlock8x8?8:16;
  const int32_t kiFrameSize = (kiFrameWidth-kiMarginSize) * (kiFrameHeight-kiMarginSize);
  const int32_t kiListSize  = (0==kiFeatureStrategyIndex)?(bIsBlock8x8 ? LIST_SIZE_SUM_8x8 : LIST_SIZE_SUM_16x16):256;

  pScreenBlockFeatureStorage->pTimesOfFeatureValue = (uint32_t*)pMa->WelsMalloc(kiListSize*sizeof(uint32_t),"pScreenBlockFeatureStorage->pTimesOfFeatureValue");
  WELS_VERIFY_RETURN_IF(ENC_RETURN_MEMALLOCERR, NULL == pScreenBlockFeatureStorage->pTimesOfFeatureValue)

  pScreenBlockFeatureStorage->pLocationOfFeature = (uint16_t**)pMa->WelsMalloc(kiListSize*sizeof(uint16_t*),"pScreenBlockFeatureStorage->pLocationOfFeature");
  WELS_VERIFY_RETURN_IF(ENC_RETURN_MEMALLOCERR, NULL == pScreenBlockFeatureStorage->pLocationOfFeature)

  pScreenBlockFeatureStorage->pLocationPointer = (uint16_t*)pMa->WelsMalloc(2*kiFrameSize*sizeof(uint16_t), "pScreenBlockFeatureStorage->pLocationPointer");
  WELS_VERIFY_RETURN_IF(ENC_RETURN_MEMALLOCERR, NULL == pScreenBlockFeatureStorage->pLocationPointer)

  pScreenBlockFeatureStorage->iActualListSize  = kiListSize;
  return ENC_RETURN_SUCCESS;
}
int32_t ReleaseScreenBlockFeatureStorage( CMemoryAlign *pMa, SScreenBlockFeatureStorage* pScreenBlockFeatureStorage ) {
  if ( pMa && pScreenBlockFeatureStorage ) {
    pMa->WelsFree( pScreenBlockFeatureStorage->pTimesOfFeatureValue, "pScreenBlockFeatureStorage->pTimesOfFeatureValue");
    pScreenBlockFeatureStorage->pTimesOfFeatureValue=NULL;

    pMa->WelsFree( pScreenBlockFeatureStorage->pLocationOfFeature, "pScreenBlockFeatureStorage->pLocationOfFeature");
    pScreenBlockFeatureStorage->pLocationOfFeature=NULL;

    pMa->WelsFree( pScreenBlockFeatureStorage->pLocationPointer, "pScreenBlockFeatureStorage->pLocationPointer");
    pScreenBlockFeatureStorage->pLocationPointer=NULL;

    return ENC_RETURN_SUCCESS;
  }
  return ENC_RETURN_UNEXPECTED;
}

//preprocess related
int32_t SumOf8x8SingleBlock_c(uint8_t* pRef, const int32_t kiRefStride)
{
  int32_t iSum = 0, i;
  for(i = 0; i < 8; i++)
  {
    iSum +=  pRef[0]    + pRef[1]  + pRef[2]  + pRef[3];
    iSum +=  pRef[4]    + pRef[5]  + pRef[6]  + pRef[7];
    pRef += kiRefStride;
  }
  return iSum;
}
int32_t SumOf16x16SingleBlock_c(uint8_t* pRef, const int32_t kiRefStride)
{
  int32_t iSum = 0, i;
  for(i = 0; i < 16; i++)
  {
    iSum +=  pRef[0]    + pRef[1]  + pRef[2]  + pRef[3];
    iSum +=  pRef[4]    + pRef[5]  + pRef[6]  + pRef[7];
    iSum    +=  pRef[8]    + pRef[9]  + pRef[10]  + pRef[11];
    iSum    +=  pRef[12]  + pRef[13]  + pRef[14]  + pRef[15];
    pRef += kiRefStride;
  }
  return iSum;
}

void SumOf8x8BlockOfFrame_c(uint8_t *pRefPicture, const int32_t kiWidth, const int32_t kiHeight, const int32_t kiRefStride,
                                              uint16_t* pFeatureOfBlock, uint32_t pTimesOfFeatureValue[])
{
  int32_t x, y;
  uint8_t *pRef;
  uint16_t *pBuffer;
  int32_t iSum;
  for(y = 0; y < kiHeight; y++) {
    pRef = pRefPicture  + kiRefStride * y;
    pBuffer  = pFeatureOfBlock + kiWidth * y;
    for(x = 0; x < kiWidth; x++) {
      iSum = SumOf8x8SingleBlock_c(pRef + x, kiRefStride);

      pBuffer[x] = iSum;
      pTimesOfFeatureValue[iSum]++;
    }
  }
}

void SumOf16x16BlockOfFrame_c(uint8_t *pRefPicture, const int32_t kiWidth, const int32_t kiHeight, const int32_t kiRefStride,
                                              uint16_t* pFeatureOfBlock, uint32_t pTimesOfFeatureValue[])
{//TODO: this is similar to SumOf8x8BlockOfFrame_c expect the calling of single block func, refactor-able?
  int32_t x, y;
  uint8_t *pRef;
  uint16_t *pBuffer;
  int32_t iSum;
  for(y = 0; y < kiHeight; y++) {
    pRef = pRefPicture  + kiRefStride * y;
    pBuffer  = pFeatureOfBlock + kiWidth * y;
    for(x = 0; x < kiWidth; x++) {
      iSum = SumOf16x16SingleBlock_c(pRef + x, kiRefStride);

      pBuffer[x] = iSum;
      pTimesOfFeatureValue[iSum]++;
    }
  }
}

void InitializeHashforFeature_c( uint32_t* pTimesOfFeatureValue, uint16_t* pBuf, const int32_t kiListSize,
                                uint16_t** pLocationOfFeature, uint16_t** pFeatureValuePointerList )
{
  //assign location pointer
  uint16_t *pBufPos  = pBuf;
  for( int32_t i = 0 ; i < kiListSize; ++i )
  {
    pLocationOfFeature[i] =
      pFeatureValuePointerList[i] = pBufPos;
    pBufPos      += (pTimesOfFeatureValue[i]<<1);
  }
}
void FillQpelLocationByFeatureValue_c( uint16_t* pFeatureOfBlock, const int32_t kiWidth, const int32_t kiHeight,
                                       uint16_t** pFeatureValuePointerList )
{
  //assign each pixel's position
  uint16_t* pSrcPointer  =  pFeatureOfBlock;
  int32_t iQpelY = 0;
  for(int32_t y = 0; y < kiHeight; y++)
  {
    for(int32_t x = 0; x < kiWidth; x++)
    {
      uint16_t uiFeature = pSrcPointer[x];
      ST32( &pFeatureValuePointerList[uiFeature][0], ((iQpelY<<16)|(x<<2)) );
      pFeatureValuePointerList[uiFeature] += 2;
    }
    iQpelY += 4;
    pSrcPointer += kiWidth;
  }
}
void CalculateFeatureOfBlock( SWelsFuncPtrList *pFunc, SPicture* pRef,
                         SScreenBlockFeatureStorage* pScreenBlockFeatureStorage)
{
  uint16_t* pFeatureOfBlock = pScreenBlockFeatureStorage->pFeatureOfBlockPointer;
  uint32_t* pTimesOfFeatureValue = pScreenBlockFeatureStorage->pTimesOfFeatureValue;
  uint16_t** pLocationOfFeature  = pScreenBlockFeatureStorage->pLocationOfFeature;
  uint16_t* pBuf = pScreenBlockFeatureStorage->pLocationPointer;

  uint8_t* pRefData = pRef->pData[0];
  const int32_t iRefStride = pRef->iLineSize[0];
  int32_t iIs16x16 = pScreenBlockFeatureStorage->iIs16x16;
  bool bUseSum = (pScreenBlockFeatureStorage->uiFeatureStrategyIndex == 0);
  const int32_t iEdgeDiscard = (iIs16x16?16:8);//this is to save complexity of padding on pRef
  const int32_t iWidth = pRef->iWidthInPixel - iEdgeDiscard;
  const int32_t kiHeight = pRef->iHeightInPixel - iEdgeDiscard;
  const int32_t kiActualListSize = pScreenBlockFeatureStorage->iActualListSize;
  uint16_t* pFeatureValuePointerList[WELS_MAX(LIST_SIZE_SUM_16x16,LIST_SIZE_MSE_16x16)] = {0};

  memset(pTimesOfFeatureValue, 0, sizeof(int32_t)*kiActualListSize);
  (pFunc->pfCalculateBlockFeatureOfFrame[iIs16x16])(pRefData,iWidth, kiHeight, iRefStride, pFeatureOfBlock, pTimesOfFeatureValue);

  //assign pLocationOfFeature pointer
  InitializeHashforFeature_c( pTimesOfFeatureValue, pBuf, kiActualListSize,
    pLocationOfFeature, pFeatureValuePointerList );

  //assign each pixel's pLocationOfFeature
  FillQpelLocationByFeatureValue_c(pFeatureOfBlock, iWidth, kiHeight, pFeatureValuePointerList);
}

void PerformFMEPreprocess( SWelsFuncPtrList *pFunc, SPicture* pRef,
                          SScreenBlockFeatureStorage* pScreenBlockFeatureStorage)
{
    CalculateFeatureOfBlock(pFunc, pRef, pScreenBlockFeatureStorage );
    pScreenBlockFeatureStorage->bRefBlockFeatureCalculated = true;
}

//search related
void SetFeatureSearchIn( SWelsFuncPtrList *pFunc,  const SWelsME& sMe,
                        const SSlice *pSlice, SScreenBlockFeatureStorage* pRefFeatureStorage,
                        const int32_t kiEncStride, const int32_t kiRefStride,
                        SFeatureSearchIn* pFeatureSearchIn ) {
  pFeatureSearchIn->pSad = pFunc->sSampleDealingFuncs.pfSampleSad[sMe.uiBlockSize];
  pFeatureSearchIn->iFeatureOfCurrent=pFunc->pfCalculateSingleBlockFeature[BLOCK_16x16==sMe.uiBlockSize](sMe.pEncMb, kiEncStride);

  pFeatureSearchIn->pEnc       = sMe.pEncMb;
  pFeatureSearchIn->pColoRef = sMe.pColoRefMb;
  pFeatureSearchIn->iEncStride = kiEncStride;
  pFeatureSearchIn->iRefStride = kiRefStride;
  pFeatureSearchIn->uiSadCostThresh = sMe.uiSadCostThreshold;

  pFeatureSearchIn->iCurPixX = sMe.iCurMeBlockPixX;
  pFeatureSearchIn->iCurPixXQpel = (pFeatureSearchIn->iCurPixX<<2);
  pFeatureSearchIn->iCurPixY = sMe.iCurMeBlockPixY;
  pFeatureSearchIn->iCurPixYQpel = (pFeatureSearchIn->iCurPixY<<2);

  pFeatureSearchIn->pTimesOfFeature = pRefFeatureStorage->pTimesOfFeatureValue;
  pFeatureSearchIn->pQpelLocationOfFeature = pRefFeatureStorage->pLocationOfFeature;
  pFeatureSearchIn->pMvdCostX = sMe.pMvdCost - pFeatureSearchIn->iCurPixXQpel - sMe.sMvp.iMvX;
  pFeatureSearchIn->pMvdCostY = sMe.pMvdCost - pFeatureSearchIn->iCurPixYQpel - sMe.sMvp.iMvY;

  pFeatureSearchIn->iMinQpelX = pFeatureSearchIn->iCurPixXQpel+((pSlice->sMvStartMin.iMvX)<<2);
  pFeatureSearchIn->iMinQpelY = pFeatureSearchIn->iCurPixYQpel+((pSlice->sMvStartMin.iMvY)<<2);
  pFeatureSearchIn->iMaxQpelX = pFeatureSearchIn->iCurPixXQpel+((pSlice->sMvStartMax.iMvX)<<2);
  pFeatureSearchIn->iMaxQpelY = pFeatureSearchIn->iCurPixYQpel+((pSlice->sMvStartMax.iMvY)<<2);
}
void SaveFeatureSearchOut( const SMVUnitXY sBestMv, const uint32_t uiBestSadCost, uint8_t* pRef, SFeatureSearchOut* pFeatureSearchOut) {
  pFeatureSearchOut->sBestMv = sBestMv;
  pFeatureSearchOut->uiBestSadCost = uiBestSadCost;
  pFeatureSearchOut->pBestRef = pRef;
}
bool FeatureSearchOne( SFeatureSearchIn &sFeatureSearchIn, const int32_t iFeatureDifference, const uint32_t kuiExpectedSearchTimes,
                      SFeatureSearchOut* pFeatureSearchOut) {
  const int32_t iFeatureOfRef = (sFeatureSearchIn.iFeatureOfCurrent + iFeatureDifference);
  if(iFeatureOfRef < 0 || iFeatureOfRef >= LIST_SIZE)
    return true;

  PSampleSadSatdCostFunc pSad = sFeatureSearchIn.pSad;
  uint8_t* pEnc =  sFeatureSearchIn.pEnc;
  uint8_t* pColoRef = sFeatureSearchIn.pColoRef;
  const int32_t iEncStride=  sFeatureSearchIn.iEncStride;
  const int32_t iRefStride =  sFeatureSearchIn.iRefStride;
  const uint16_t uiSadCostThresh = sFeatureSearchIn.uiSadCostThresh;

  const int32_t iCurPixX = sFeatureSearchIn.iCurPixX;
  const int32_t iCurPixY = sFeatureSearchIn.iCurPixY;
  const int32_t iCurPixXQpel = sFeatureSearchIn.iCurPixXQpel;
  const int32_t iCurPixYQpel = sFeatureSearchIn.iCurPixYQpel;

  const int32_t iMinQpelX =  sFeatureSearchIn.iMinQpelX;
  const int32_t iMinQpelY =  sFeatureSearchIn.iMinQpelY;
  const int32_t iMaxQpelX =  sFeatureSearchIn.iMaxQpelX;
  const int32_t iMaxQpelY =  sFeatureSearchIn.iMaxQpelY;

  const int32_t iSearchTimes = WELS_MIN(sFeatureSearchIn.pTimesOfFeature[iFeatureOfRef], kuiExpectedSearchTimes);
  const int32_t iSearchTimesx2 = (iSearchTimes<<1);
  const uint16_t* pQpelPosition = sFeatureSearchIn.pQpelLocationOfFeature[iFeatureOfRef];

  SMVUnitXY sBestMv;
  uint32_t uiBestCost, uiTmpCost;
  uint8_t *pBestRef, *pCurRef;
  int32_t iQpelX, iQpelY;
  int32_t iIntepelX, iIntepelY;
  int32_t i;

  sBestMv.iMvX = pFeatureSearchOut->sBestMv.iMvX;
  sBestMv.iMvY = pFeatureSearchOut->sBestMv.iMvY;
  uiBestCost = pFeatureSearchOut->uiBestSadCost;
  pBestRef = pFeatureSearchOut->pBestRef;

  for( i = 0; i < iSearchTimesx2; i+=2) {
    iQpelX = pQpelPosition[i];
    iQpelY = pQpelPosition[i+1];

    if((iQpelX > iMaxQpelX) || (iQpelX < iMinQpelX)
      || (iQpelY > iMaxQpelY) || (iQpelY < iMinQpelY)
      || (iQpelX == iCurPixXQpel) || (iQpelY == iCurPixYQpel) )
      continue;

    uiTmpCost = sFeatureSearchIn.pMvdCostX[ iQpelX ] + sFeatureSearchIn.pMvdCostY[ iQpelY ];
    if(uiTmpCost + iFeatureDifference >= uiBestCost)
      continue;

    iIntepelX = (iQpelX>>2) - iCurPixX;
    iIntepelY = (iQpelY>>2) - iCurPixY;
    pCurRef = &pColoRef[iIntepelX + iIntepelY * iRefStride];
    uiTmpCost += pSad( pEnc, iEncStride, pCurRef, iRefStride );
    if( uiTmpCost < uiBestCost ) {
      sBestMv.iMvX = iIntepelX;
      sBestMv.iMvY = iIntepelY;
      uiBestCost = uiTmpCost;
      pBestRef = pCurRef;

      if(uiBestCost < uiSadCostThresh)
        break;
    }
  }
  SaveFeatureSearchOut(sBestMv, uiBestCost, pBestRef, pFeatureSearchOut);
  return (i < iSearchTimesx2);
}

void MotionEstimateFeatureFullSearch( SFeatureSearchIn &sFeatureSearchIn,
                                        const uint32_t kuiMaxSearchPoint,
                                        SWelsME* pMe) {
  SFeatureSearchOut sFeatureSearchOut = { { 0 } };//TODO: this can be refactored and removed
  sFeatureSearchOut.uiBestSadCost = pMe->uiSadCost;
  sFeatureSearchOut.sBestMv = pMe->sMv;
  sFeatureSearchOut.pBestRef = pMe->pRefMb;

  int32_t iFeatureDifference = 0;//TODO: change it according to computational-complexity setting when needed
  FeatureSearchOne( sFeatureSearchIn, iFeatureDifference, kuiMaxSearchPoint, &sFeatureSearchOut );
  if ( sFeatureSearchOut.uiBestSadCost < pMe->uiSadCost ) {//TODO: this may be refactored and removed
    UpdateMeResults(sFeatureSearchOut.sBestMv,
      sFeatureSearchOut.uiBestSadCost, sFeatureSearchOut.pBestRef,
      pMe);
  }
}

/////////////////////////
// Search function options
/////////////////////////
void WelsDiamondCrossSearch(SWelsFuncPtrList *pFunc, void* vpMe, void* vpSlice, const int32_t kiEncStride,  const int32_t kiRefStride) {
    SWelsME* pMe			 = static_cast<SWelsME *>(vpMe);
    SSlice* pSlice				 = static_cast<SSlice *>(vpSlice);

    //  Step 1: diamond search
    WelsDiamondSearch(pFunc, vpMe, vpSlice, kiEncStride, kiRefStride);

    //  Step 2: CROSS search
    SScreenBlockFeatureStorage pRefBlockFeature; //TODO: use this structure from Ref
    pMe->uiSadCostThreshold = pRefBlockFeature.uiSadCostThreshold[pMe->uiBlockSize];
    if (pMe->uiSadCost >= pMe->uiSadCostThreshold) {
      WelsMotionCrossSearch(pFunc, pMe, pSlice, kiEncStride, kiRefStride);
    }
}
void WelsDiamondCrossFeatureSearch(SWelsFuncPtrList *pFunc, void* vpMe, void* vpSlice, const int32_t kiEncStride, const int32_t kiRefStride) {
    SWelsME* pMe			 = static_cast<SWelsME *>(vpMe);
    SSlice* pSlice				 = static_cast<SSlice *>(vpSlice);

    //  Step 1: diamond search + cross
    WelsDiamondCrossSearch(pFunc, pMe, pSlice, kiEncStride, kiRefStride);

    // Step 2: FeatureSearch
    if (pMe->uiSadCost >= pMe->uiSadCostThreshold) {
        pSlice->uiSliceFMECostDown += pMe->uiSadCost;

        SScreenBlockFeatureStorage tmpScreenBlockFeatureStorage; //TODO: use this structure from Ref
        uint32_t uiMaxSearchPoint = INT_MAX;//TODO: change it according to computational-complexity setting
        SFeatureSearchIn sFeatureSearchIn = {0};
        SetFeatureSearchIn(pFunc, *pMe, pSlice, &tmpScreenBlockFeatureStorage,
          kiEncStride, kiRefStride,
          &sFeatureSearchIn);
        MotionEstimateFeatureFullSearch( sFeatureSearchIn, uiMaxSearchPoint, pMe);

        pSlice->uiSliceFMECostDown -= pMe->uiSadCost;
    }
}
} // namespace WelsSVCEnc

