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
 * \file	svc motion estimate.h
 *
 * \brief	Interfaces introduced in svc mb motion estimation
 *
 * \date	08/11/2009 Created
 *
 *************************************************************************************
 */
#ifndef SVC_MOTION_ESTIMATE_
#define SVC_MOTION_ESTIMATE_

#include "typedefs.h"
#include "encoder_context.h"
#include "wels_func_ptr_def.h"

namespace WelsSVCEnc {
#define CAMERA_STARTMV_RANGE (64)
#define	ITERATIVE_TIMES	(16)
#define CAMERA_MV_RANGE (CAMERA_STARTMV_RANGE+ITERATIVE_TIMES)
#define CAMERA_MVD_RANGE  ((CAMERA_MV_RANGE+1)<<1) //mvd=mv_range*2;
#define	BASE_MV_MB_NMB	((2*CAMERA_MV_RANGE/MB_WIDTH_LUMA)-1)
#define CAMERA_HIGHLAYER_MVD_RANGE (243)//mvd range;
#define EXPANDED_MV_RANGE (504) //=512-8 rather than 511 to sacrifice same edge point but save complexity in assemblys
#define EXPANDED_MVD_RANGE ((504+1)<<1)

union SadPredISatdUnit {
uint32_t	uiSadPred;
uint32_t	uiSatd;    //reuse the sad_pred as a temp satd pData
};
typedef struct TagWelsME {
/* input */
uint16_t*					pMvdCost;
union SadPredISatdUnit	uSadPredISatd; //reuse the sad_pred as a temp pData
uint32_t					uiSadCost;  //used by ME and RC //max SAD should be max_delta*size+lambda*mvdsize = 255*256+91*33*2 = 65280 + 6006 = 71286 > (2^16)-1 = 65535
uint32_t					uiSatdCost; /* satd + lm * nbits */
uint32_t					uiSadCostThreshold;
int32_t						iCurMeBlockPixX;
int32_t						iCurMeBlockPixY;
uint8_t						uiBlockSize;   /* BLOCK_WxH */
uint8_t						uiReserved;

uint8_t*						pEncMb;
uint8_t*						pRefMb;
uint8_t*						pColoRefMb;

SMVUnitXY					sMvp;
SMVUnitXY					sMvBase;
SMVUnitXY					sDirectionalMv;

/* output */
SMVUnitXY					sMv;
} SWelsME;

#define  COST_MVD(table, mx, my)  (table[mx] + table[my])

// Function definitions below

void WelsInitMeFunc( SWelsFuncPtrList* pFuncList, uint32_t uiCpuFlag, bool bScreenContent );

/*!
 * \brief	BL mb motion estimate search
 *
 * \param	enc			Wels encoder context
 * \param	m	        Wels me information
 *
 * \return	NONE
 */
void WelsMotionEstimateSearch (SWelsFuncPtrList* pFuncList, void* pLplayer, void* pLpme, void* pLpslice);


/*!
 * \brief	BL mb motion estimate initial point testing
 *
 * \param	enc			Wels encoder context
 * \param	m	        Wels me information
 * \param	mv_range	search range in motion estimate
 * \param	point	    the best match point in motion estimation
 *
 * \return	NONE
 */


/*!
 * \brief	EL mb motion estimate initial point testing
 *
 * \param	pix_func	SSampleDealingFunc
 * \param	m	        Wels me information
 * \param	mv_range	search range in motion estimate
 * \param	point	    the best match point in motion estimation
 *
 * \return	NONE
 */

bool WelsMotionEstimateInitialPoint (SWelsFuncPtrList* pFuncList, SWelsME* pMe, SSlice* pSlice,
                                     const int32_t kiStrideEnc, const int32_t kiStrideRef);

/*!
 * \brief	mb iterative motion estimate search
 *
 * \param	enc			Wels encoder context
 * \param	m	        Wels me information
 * \param	point	    the best match point in motion estimation
 *
 * \return	NONE
 */
void WelsMotionEstimateIterativeSearch (SWelsFuncPtrList* pFuncList, SWelsME* pMe, const int32_t kiStrideEnc,
                                        const int32_t kiStrideRef, uint8_t* pRef);

bool WelsMeSadCostSelect (int32_t* pSadCost, const uint16_t* kpMvdCost, int32_t* pBestCost, const int32_t kiDx,
                            const int32_t kiDy, int32_t* pIx, int32_t* pIy);

void CalculateSatdCost( PSampleSadSatdCostFunc pSatd, void * vpMe, const int32_t kiEncStride, const int32_t kiRefStride );
void NotCalculateSatdCost( PSampleSadSatdCostFunc pSatd, void * vpMe, const int32_t kiEncStride, const int32_t kiRefStride );
bool CheckDirectionalMv(PSampleSadSatdCostFunc pSad, void * vpMe,
                      const SMVUnitXY ksMinMv, const SMVUnitXY ksMaxMv, const int32_t kiEncStride, const int32_t kiRefStride,
                      int32_t& iBestSadCost);
bool CheckDirectionalMvFalse(PSampleSadSatdCostFunc pSad, void * vpMe,
                      const SMVUnitXY ksMinMv, const SMVUnitXY ksMaxMv, const int32_t kiEncStride, const int32_t kiRefStride,
                      int32_t& iBestSadCost);

void LineFullSearch_c(	 void *pFunc, void *vpMe,
                        uint16_t* pMvdTable, const int32_t kiFixedMvd,
                        const int32_t kiEncStride, const int32_t kiRefStride,
                        const int32_t kiMinPos, const int32_t kiMaxPos,
                        const bool bVerticalSearch );
void VerticalFullSearchUsingSSE41( void *pFunc, void *vpMe,
														uint16_t* pMvdTable, const int32_t kiFixedMvd,
														const int32_t kiEncStride, const int32_t kiRefStride,
													const int32_t kiMinPos, const int32_t kiMaxPos,
                          const bool bVerticalSearch );
void WelsMotionCrossSearch(SWelsFuncPtrList *pFuncList,  SDqLayer* pCurLayer, SWelsME * pMe, const SSlice* pSlice);

inline void SetMvWithinIntegerMvRange( const int32_t kiMbWidth, const int32_t kiMbHeight, const int32_t kiMbX, const int32_t kiMbY,
                        const int32_t kiMaxMvRange,
                        SMVUnitXY* pMvMin, SMVUnitXY* pMvMax)
{
  pMvMin->iMvX = WELS_MAX(-1*((kiMbX + 1)<<4) + INTPEL_NEEDED_MARGIN, -1*kiMaxMvRange);
  pMvMin->iMvY = WELS_MAX(-1*((kiMbY + 1)<<4) + INTPEL_NEEDED_MARGIN, -1*kiMaxMvRange);
  pMvMax->iMvX = WELS_MIN( ((kiMbWidth - kiMbX)<<4) - INTPEL_NEEDED_MARGIN, kiMaxMvRange);
  pMvMax->iMvY = WELS_MIN( ((kiMbHeight - kiMbY)<<4) - INTPEL_NEEDED_MARGIN, kiMaxMvRange);
}

inline bool CheckMvInRange( const int16_t kiCurrentMv, const int16_t kiMinMv, const int16_t kiMaxMv )
{
  return ((kiCurrentMv >= kiMinMv) && (kiCurrentMv < kiMaxMv));
}
inline bool CheckMvInRange( const SMVUnitXY ksCurrentMv, const SMVUnitXY ksMinMv, const SMVUnitXY ksMaxMv )
{
  return (CheckMvInRange(ksCurrentMv.iMvX, ksMinMv.iMvX, ksMaxMv.iMvX)
    && CheckMvInRange(ksCurrentMv.iMvY, ksMinMv.iMvY, ksMaxMv.iMvY));
}
}
#endif
