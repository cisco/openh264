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
 * \file	svc motion estimate.c
 *
 * \brief	Interfaces introduced in svc mb motion estimation
 *
 * \date	08/11/2009 Created
 *
 *************************************************************************************
 */


#include "sample.h"
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


/*!
 * \brief	BL mb motion estimate search
 *
 * \param	enc			Wels encoder context
 * \param	pMe	        Wels me information
 *
 * \return	NONE
 */

void WelsMotionEstimateSearch (SWelsFuncPtrList* pFuncList, void* pLplayer, void* pLpme, void* pLpslice) {
  SDqLayer* pCurDqLayer			= (SDqLayer*)pLplayer;
  SWelsME* pMe						= (SWelsME*)pLpme;
  SSlice* pSlice					= (SSlice*)pLpslice;
  int32_t iStrideEnc = pCurDqLayer->iEncStride[0];
  int32_t iStrideRef = pCurDqLayer->pRefPic->iLineSize[0];

  //  Step 1: Initial point prediction
  if ( !WelsMotionEstimateInitialPoint (pFuncList, pMe, pSlice, iStrideEnc, iStrideRef) ) {
    WelsMotionEstimateIterativeSearch (pFuncList, pMe, iStrideEnc, iStrideRef, pMe->pRefMb);
    MeEndIntepelSearch(pMe);
  }

  pFuncList->pfCalculateSatd( pFuncList->sSampleDealingFuncs.pfSampleSatd[pMe->uiBlockSize], pMe, iStrideEnc, iStrideRef );
}

/*!
 * \brief	EL mb motion estimate initial point testing
 *
 * \param	pix_pFuncList	SSampleDealingFunc
 * \param	pMe	        Wels me information
 * \param	mv_range	search range in motion estimate
 * \param	point	    the best match point in motion estimation
 *
 * \return	NONE
 */
bool WelsMotionEstimateInitialPoint (SWelsFuncPtrList* pFuncList, SWelsME* pMe, SSlice* pSlice, int32_t iStrideEnc,
                                     int32_t iStrideRef) {
  PSampleSadSatdCostFunc pSad		= pFuncList->sSampleDealingFuncs.pfSampleSad[pMe->uiBlockSize];
  const uint16_t* kpMvdCost	= pMe->pMvdCost;
  uint8_t* const kpEncMb		= pMe->pEncMb;
  int16_t iMvc0, iMvc1;
  int32_t iSadCost;
  int32_t iBestSadCost;
  uint8_t* pRefMb;
  uint8_t* pFref2;
  uint32_t i;
  const uint32_t kuiMvcNum		= pSlice->uiMvcNum;
  const SMVUnitXY* kpMvcList	= &pSlice->sMvc[0];
  const SMVUnitXY ksMvStartMin		= pSlice->sMvStartMin;
  const SMVUnitXY ksMvStartMax		= pSlice->sMvStartMax;
  const SMVUnitXY ksMvp		= pMe->sMvp;
  SMVUnitXY sMv;

  //  Step 1: Initial point prediction
  // init with sMvp
  sMv.iMvX	= WELS_CLIP3 ((2 + ksMvp.iMvX) >> 2, ksMvStartMin.iMvX, ksMvStartMax.iMvX);
  sMv.iMvY	= WELS_CLIP3 ((2 + ksMvp.iMvY) >> 2, ksMvStartMin.iMvY, ksMvStartMax.iMvY);

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

void WelsMotionEstimateIterativeSearch (SWelsFuncPtrList* pFuncList, SWelsME* pMe, const int32_t kiStrideEnc,
                                        const int32_t kiStrideRef, uint8_t* pFref) {
  PSample4SadCostFunc			pSad					=  pFuncList->sSampleDealingFuncs.pfSample4Sad[pMe->uiBlockSize];

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

void CalculateSatdCost( PSampleSadSatdCostFunc pSatd, void * vpMe, const int32_t kiEncStride, const int32_t kiRefStride )
{
  SWelsME* pMe						 = static_cast<SWelsME *>(vpMe);
  pMe->uSadPredISatd.uiSatd = pSatd(pMe->pEncMb, kiEncStride, pMe->pRefMb, kiRefStride);
  pMe->uiSatdCost = pMe->uSadPredISatd.uiSatd + COST_MVD (pMe->pMvdCost, pMe->sMv.iMvX - pMe->sMvp.iMvX,
                                                            pMe->sMv.iMvY - pMe->sMvp.iMvY);
}
void NotCalculateSatdCost( PSampleSadSatdCostFunc pSatd, void * vpMe, const int32_t kiEncStride, const int32_t kiRefStride )
{
}


bool CheckDirectionalMv(PSampleSadSatdCostFunc pSad, void * vpMe,
                      const SMVUnitXY ksMinMv, const SMVUnitXY ksMaxMv, const int32_t kiEncStride, const int32_t kiRefStride,
                      int32_t& iBestSadCost)
{
  SWelsME* pMe						 = static_cast<SWelsME *>(vpMe);
  const int16_t kiMvX = pMe->sDirectionalMv.iMvX;
  const int16_t kiMvY = pMe->sDirectionalMv.iMvY;

  //Check MV from scrolling detection
  if ( (BLOCK_16x16!=pMe->uiBlockSize) //scrolled_MV with P16x16 is checked SKIP checking function
    && ( kiMvX | kiMvY ) //(0,0) checked in ordinary initial point checking
    && CheckMvInRange( pMe->sDirectionalMv, ksMinMv, ksMaxMv ) )
  {
    uint8_t* pRef = &pMe->pColoRefMb[kiMvY * kiRefStride + kiMvX];
    uint32_t uiCurrentSadCost = pSad( pMe->pEncMb, kiEncStride,  pRef, kiRefStride ) +
      COST_MVD(pMe->pMvdCost, (kiMvX<<2) - pMe->sMvp.iMvX, (kiMvY<<2) - pMe->sMvp.iMvY );
    if( uiCurrentSadCost < pMe->uiSadCost )
    {
      iBestSadCost = uiCurrentSadCost;
      return true;
    }
  }
  return false;
}

bool CheckDirectionalMvFalse(PSampleSadSatdCostFunc pSad, void * vpMe,
                      const SMVUnitXY ksMinMv, const SMVUnitXY ksMaxMv, const int32_t kiEncStride, const int32_t kiRefStride,
                      int32_t& iBestSadCost)
{
  return false;
}

void VerticalFullSearchUsingSSE41( void *pFunc, void *vpMe,
														uint16_t* pMvdTable, const int32_t kiFixedMvd,
														const int32_t kiEncStride, const int32_t kiRefStride,
													const int32_t kiMinPos, const int32_t kiMaxPos,
                          const bool bVerticalSearch )
{
  SWelsFuncPtrList *pFuncList      = static_cast<SWelsFuncPtrList *>(pFunc);
  SWelsME *pMe				                    = static_cast<SWelsME *>(vpMe);
}
void LineFullSearch_c(	void *pFunc, void *vpMe,
													uint16_t* pMvdTable, const int32_t kiFixedMvd,
													const int32_t kiEncStride, const int32_t kiRefStride,
													const int32_t kiMinPos, const int32_t kiMaxPos,
                          const bool bVerticalSearch )
{
  SWelsFuncPtrList *pFuncList      = static_cast<SWelsFuncPtrList *>(pFunc);
  SWelsME *pMe				                    = static_cast<SWelsME *>(vpMe);
  PSampleSadSatdCostFunc pSad = pFuncList->sSampleDealingFuncs.pfSampleSad[pMe->uiBlockSize];
  const int32_t kiCurMeBlockPix	= bVerticalSearch?pMe->iCurMeBlockPixY:pMe->iCurMeBlockPixX;
  const int32_t kiStride = bVerticalSearch?kiRefStride:1;
  uint8_t* pRef			      = &pMe->pColoRefMb[(kiMinPos - kiCurMeBlockPix)*kiStride];
  uint16_t* pMvdCost  = &(pMvdTable[kiMinPos<<2]);
  uint32_t uiBestCost	  = 0xFFFFFFFF;
  int32_t iBestPos		   = 0;

  for ( int32_t iTargetPos = kiMinPos; iTargetPos < kiMaxPos; ++ iTargetPos ) {
    uint8_t* const kpEncMb	= pMe->pEncMb;
    uint32_t uiSadCost = pSad( kpEncMb, kiEncStride, pRef, kiRefStride ) + (kiFixedMvd + *pMvdCost);
    if (uiSadCost < uiBestCost) {
      uiBestCost	= uiSadCost;
      iBestPos	= iTargetPos;
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

void WelsMotionCrossSearch(SWelsFuncPtrList *pFuncList,  SDqLayer* pCurLayer, SWelsME * pMe,
											const SSlice* pSlice)
{
  //TODO:
  //PMOTION_VERFULL_SEARCH VerticalFullSearchFunc	= pFuncList->VerticalFullSearch_c;
  //PMOTION_HORFULL_SEARCH HorizontalFullSearchFunc	= pFuncList->HorizontalFullSearch_c;
  const int32_t kiEncStride = pCurLayer->iEncStride[0];
  const int32_t kiRefStride = pCurLayer->pRefPic->iLineSize[0];

  const int32_t iCurMeBlockPixX = pMe->iCurMeBlockPixX;
  const int32_t iCurMeBlockQpelPixX = ((iCurMeBlockPixX)<<2);
  const int32_t iCurMeBlockPixY = pMe->iCurMeBlockPixY;
  const int32_t iCurMeBlockQpelPixY = ((iCurMeBlockPixY)<<2);
  uint16_t* pMvdCostX = pMe->pMvdCost - iCurMeBlockQpelPixX - pMe->sMvp.iMvX;//do the offset here instead of in the search
  uint16_t* pMvdCostY = pMe->pMvdCost - iCurMeBlockQpelPixY - pMe->sMvp.iMvY;//do the offset here instead of in the search

  //vertical search
  LineFullSearch_c( pFuncList, pMe,
    pMvdCostY, pMvdCostX[ iCurMeBlockQpelPixX ],
    kiEncStride, kiRefStride,
    iCurMeBlockPixY + pSlice->sMvStartMin.iMvY,
    iCurMeBlockPixY + pSlice->sMvStartMax.iMvY, true );

  //horizontal search
  if (pMe->uiSadCost >= pMe->uiSadCostThreshold) {
    LineFullSearch_c( pFuncList, pMe,
      pMvdCostX, pMvdCostY[ iCurMeBlockQpelPixY ],
      kiEncStride, kiRefStride,
      iCurMeBlockPixX + pSlice->sMvStartMin.iMvX,
      iCurMeBlockPixX + pSlice->sMvStartMax.iMvX,
      false );
  }
}
} // namespace WelsSVCEnc
