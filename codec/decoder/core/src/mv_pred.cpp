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
 * \file	mv_pred.c
 *
 * \brief	Get MV predictor and update motion vector of mb cache
 *
 * \date	05/22/2009 Created
 *
 *************************************************************************************
 */

#include "mv_pred.h"
#include "ls_defines.h"
#include "mb_cache.h"

namespace WelsDec {

//basic iMVs prediction unit for iMVs partition width (4, 2, 1)
void_t PredMv(int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
			 int32_t iPartIdx, int32_t iPartWidth, int8_t iRef, int16_t iMVP[2])
{
	const uint8_t kuiLeftIdx	= g_kuiCache30ScanIdx[iPartIdx] - 1;
	const uint8_t kuiTopIdx		= g_kuiCache30ScanIdx[iPartIdx] - 6;
	const uint8_t kuiRightTopIdx= kuiTopIdx + iPartWidth;
	const uint8_t kuiLeftTopIdx	= kuiTopIdx - 1;

	const int8_t kiLeftRef      = iRefIndex[0][kuiLeftIdx];
	const int8_t kiTopRef       = iRefIndex[0][ kuiTopIdx];
	const int8_t kiRightTopRef = iRefIndex[0][kuiRightTopIdx];
	const int8_t kiLeftTopRef  = iRefIndex[0][ kuiLeftTopIdx];
	int8_t iDiagonalRef  = kiRightTopRef;

	int8_t iMatchRef = 0;


	int16_t iAMV[2], iBMV[2], iCMV[2];

	*(int32_t*)iAMV = INTD32(iMotionVector[0][     kuiLeftIdx]);
	*(int32_t*)iBMV = INTD32(iMotionVector[0][      kuiTopIdx]);
	*(int32_t*)iCMV = INTD32(iMotionVector[0][kuiRightTopIdx]);

	if (REF_NOT_AVAIL == iDiagonalRef)
	{
		iDiagonalRef = kiLeftTopRef;
		*(int32_t*)iCMV = INTD32(iMotionVector[0][kuiLeftTopIdx]);
	}

	iMatchRef = (iRef == kiLeftRef) + (iRef == kiTopRef) + (iRef == iDiagonalRef);

	if (REF_NOT_AVAIL == kiTopRef && REF_NOT_AVAIL == iDiagonalRef && kiLeftRef >= REF_NOT_IN_LIST)
	{
		ST32(iMVP, LD32(iAMV));
		return;
	}

	if (1 == iMatchRef)
	{
		if (iRef == kiLeftRef)
		{
			ST32(iMVP, LD32(iAMV));
		}
		else if (iRef == kiTopRef)
		{
			ST32(iMVP, LD32(iBMV));
		}
		else
		{
			ST32(iMVP, LD32(iCMV));
		}
	}
	else
	{
		iMVP[0] = WelsMedian(iAMV[0], iBMV[0], iCMV[0]);
		iMVP[1] = WelsMedian(iAMV[1], iBMV[1], iCMV[1]);
	}
}
void_t PredInter8x16Mv(int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
						int32_t iPartIdx, int8_t iRef, int16_t iMVP[2])
{
	if (0 == iPartIdx)
	{
		const int8_t kiLeftRef = iRefIndex[0][6];
		if (iRef == kiLeftRef)
		{
			ST32( iMVP, LD32(&iMotionVector[0][6][0]) );
			return;
		}
	}
	else // 1 == iPartIdx
	{
		int8_t iDiagonalRef = iRefIndex[0][5]; //top-right
		int8_t index = 5;
		if (REF_NOT_AVAIL == iDiagonalRef)
		{
			iDiagonalRef = iRefIndex[0][2]; //top-left for 8*8 block(index 1)
			index = 2;
		}
		if (iRef == iDiagonalRef)
		{
			ST32( iMVP, LD32(&iMotionVector[0][index][0]) );
			return;
		}
	}

	PredMv(iMotionVector, iRefIndex, iPartIdx, 2, iRef, iMVP);
}
void_t PredInter16x8Mv(int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
						int32_t iPartIdx, int8_t iRef, int16_t iMVP[2])
{
	if (0 == iPartIdx)
	{
		const int8_t kiTopRef = iRefIndex[0][1];
		if (iRef == kiTopRef)
		{
			ST32(iMVP, LD32(&iMotionVector[0][1][0]));
			return;
		}
	}
	else // 8 == iPartIdx
	{
		const int8_t kiLeftRef = iRefIndex[0][18];
		if (iRef == kiLeftRef)
		{
			ST32(iMVP, LD32(&iMotionVector[0][18][0]));
			return;
		}
	}

	PredMv(iMotionVector, iRefIndex, iPartIdx, 4, iRef, iMVP);
}

//update iMVs and iRefIndex cache for current MB, only for P_16*16 (SKIP inclusive)
/* can be further optimized */
void_t UpdateP16x16MotionInfo( PDqLayer pCurDqLayer, int8_t iRef, int16_t iMVs[2])
{
	const int16_t kiRef2		= (iRef << 8) | iRef;
	const int32_t kiMV32		= LD32(iMVs);
	int32_t i;
	int32_t iMbXy = pCurDqLayer->iMbXyIndex;

	for (i = 0; i < 16; i+=4)
	{
		//mb
		const uint8_t kuiScan4Idx = g_kuiScan4[i];
		const uint8_t kuiScan4IdxPlus4= 4 + kuiScan4Idx;

 		ST16( &pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4Idx ], kiRef2 );
		ST16( &pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4IdxPlus4], kiRef2 );

		ST32( pCurDqLayer->pMv[0][iMbXy][  kuiScan4Idx ], kiMV32 );
		ST32( pCurDqLayer->pMv[0][iMbXy][1+kuiScan4Idx ], kiMV32 );
		ST32( pCurDqLayer->pMv[0][iMbXy][  kuiScan4IdxPlus4], kiMV32 );
		ST32( pCurDqLayer->pMv[0][iMbXy][1+kuiScan4IdxPlus4], kiMV32 );
	}
}

//update iRefIndex and iMVs of Mb, only for P16x8
/*need further optimization, mb_cache not work */
void_t UpdateP16x8MotionInfo(PDqLayer pCurDqLayer, int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
							  int32_t iPartIdx, int8_t iRef, int16_t iMVs[2])
{
	const int16_t kiRef2 = (iRef << 8) | iRef;
	const int32_t kiMV32 = LD32(iMVs);
	int32_t i;
	int32_t iMbXy = pCurDqLayer->iMbXyIndex;
	for (i = 0; i < 2; i++, iPartIdx+=4)
	{
		const uint8_t kuiScan4Idx      = g_kuiScan4[iPartIdx];
		const uint8_t kuiScan4IdxPlus4 = 4 + kuiScan4Idx;
		const uint8_t kuiCacheIdx      = g_kuiCache30ScanIdx[iPartIdx];
		const uint8_t kuiCacheIdxPlus6 = 6 + kuiCacheIdx;

		//mb
		ST16( &pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4Idx ], kiRef2 );
		ST16( &pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4IdxPlus4], kiRef2 );
		ST32( pCurDqLayer->pMv[0][iMbXy][  kuiScan4Idx ], kiMV32 );
		ST32( pCurDqLayer->pMv[0][iMbXy][1+kuiScan4Idx ], kiMV32 );
		ST32( pCurDqLayer->pMv[0][iMbXy][  kuiScan4IdxPlus4], kiMV32 );
		ST32( pCurDqLayer->pMv[0][iMbXy][1+kuiScan4IdxPlus4], kiMV32 );
		//cache
		ST16( &iRefIndex[0][kuiCacheIdx ], kiRef2 );
		ST16( &iRefIndex[0][kuiCacheIdxPlus6], kiRef2 );
		ST32( iMotionVector[0][  kuiCacheIdx ], kiMV32 );
		ST32( iMotionVector[0][1+kuiCacheIdx ], kiMV32 );
		ST32( iMotionVector[0][  kuiCacheIdxPlus6], kiMV32 );
		ST32( iMotionVector[0][1+kuiCacheIdxPlus6], kiMV32 );
	}
}
//update iRefIndex and iMVs of both Mb and Mb_cache, only for P8x16
void_t UpdateP8x16MotionInfo(PDqLayer pCurDqLayer, int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
							  int32_t iPartIdx, int8_t iRef, int16_t iMVs[2])
{
	const int16_t kiRef2 = (iRef << 8) | iRef;
	const int32_t kiMV32 = LD32(iMVs);
	int32_t i;
	int32_t iMbXy = pCurDqLayer->iMbXyIndex;

	for (i = 0; i < 2; i++, iPartIdx+=8)
	{
		const uint8_t kuiScan4Idx = g_kuiScan4[iPartIdx];
		const uint8_t kuiCacheIdx = g_kuiCache30ScanIdx[iPartIdx];
		const uint8_t kuiScan4IdxPlus4= 4 + kuiScan4Idx;
		const uint8_t kuiCacheIdxPlus6= 6 + kuiCacheIdx;

		//mb
		ST16( &pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4Idx ], kiRef2 );
		ST16( &pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4IdxPlus4], kiRef2 );
		ST32( pCurDqLayer->pMv[0][iMbXy][  kuiScan4Idx ], kiMV32 );
		ST32( pCurDqLayer->pMv[0][iMbXy][1+kuiScan4Idx ], kiMV32 );
		ST32( pCurDqLayer->pMv[0][iMbXy][  kuiScan4IdxPlus4], kiMV32 );
		ST32( pCurDqLayer->pMv[0][iMbXy][1+kuiScan4IdxPlus4], kiMV32 );
		//cache
		ST16( &iRefIndex[0][kuiCacheIdx ], kiRef2 );
		ST16( &iRefIndex[0][kuiCacheIdxPlus6], kiRef2 );
		ST32( iMotionVector[0][  kuiCacheIdx ], kiMV32 );
		ST32( iMotionVector[0][1+kuiCacheIdx ], kiMV32 );
		ST32( iMotionVector[0][  kuiCacheIdxPlus6], kiMV32 );
		ST32( iMotionVector[0][1+kuiCacheIdxPlus6], kiMV32 );
	}
}

} // namespace WelsDec