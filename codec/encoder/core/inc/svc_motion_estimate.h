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
#define MV_RANGE (64)
#define	ITERATIVE_TIMES	(16)
#define	BASE_MV_MB_NMB	((2*(MV_RANGE+ITERATIVE_TIMES)/MB_WIDTH_LUMA)-1)

union SadPredISatdUnit{
	uint16_t	uiSadPred;
	uint16_t	uiSatd;    //reuse the sad_pred as a temp satd pData 
};
typedef struct TagWelsME {
    /* input */
	uint16_t					*pMvdCost;
    union SadPredISatdUnit	uSadPredISatd; //reuse the sad_pred as a temp pData
	uint16_t					uiSadCost;  //used by ME and RC 
    uint16_t					uiSatdCost; /* satd + lm * nbits */
    uint8_t						uiPixel;   /* PIXEL_WxH */
    uint8_t						uiReserved;
	
    uint8_t						*pEncMb;
    uint8_t						*pRefMb;

	SMVUnitXY					sMvp;
	SMVUnitXY					sMvBase;
	/* output */
    SMVUnitXY					sMv;
}SWelsME;

#define  COST_MVD(table, mx, my)  (table[mx] + table[my])



/*!
 * \brief	BL mb motion estimate search
 *
 * \param	enc			Wels encoder context
 * \param	m	        Wels me information
 *
 * \return	NONE
 */
void WelsMotionEstimateSearchSatd(SWelsFuncPtrList *pFuncList, void* pLplayer, void* pLpme, void* pLpslice );

void WelsMotionEstimateSearchSad(SWelsFuncPtrList *pFuncList, void* pLplayer, void* pLpme, void* pLpslice );



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

void WelsMotionEstimateInitialPoint(SWelsFuncPtrList *pFuncList, SWelsME *pMe, SSlice *pSlice, const int32_t kiStrideEnc, const int32_t kiStrideRef );

/*!
 * \brief	mb iterative motion estimate search
 *
 * \param	enc			Wels encoder context
 * \param	m	        Wels me information
 * \param	point	    the best match point in motion estimation
 *
 * \return	NONE
 */
void WelsMotionEstimateIterativeSearch( SWelsFuncPtrList *pFuncList, SWelsME *pMe, const int32_t kiStrideEnc, const int32_t kiStrideRef, uint8_t *pRef );

bool_t WelsMeSadCostSelect( int32_t *pSadCost, const uint16_t *kpMvdCost, int32_t *pBestCost, const int32_t kiDx, const int32_t kiDy, int32_t *pIx, int32_t *pIy);

}
#endif
