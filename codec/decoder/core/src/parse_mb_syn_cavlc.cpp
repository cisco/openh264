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
 * \file	parse_mb_syn_cavlc.c
 *
 * \brief	Interfaces implementation for parsing the syntax of MB
 *
 * \date	03/17/2009 Created
 *
 *************************************************************************************
 */

#include <string.h>

#include "parse_mb_syn_cavlc.h"
#include "error_code.h"
#include "dec_golomb.h"
#include "macros.h"
#include "vlc_decoder.h"
#include "bit_stream.h"
#include "ls_defines.h"
#include "mv_pred.h"
#include "decode_slice.h"

namespace WelsDec {

void_t PredPSkipMvFromNeighbor( PDqLayer pCurLayer, int16_t iMvp[2] )
{
	bool_t bTopAvail, bLeftTopAvail, bRightTopAvail, bLeftAvail;

	int32_t iCurSliceIdc, iTopSliceIdc, iLeftTopSliceIdc, iRightTopSliceIdc, iLeftSliceIdc;
	int32_t iLeftTopType, iRightTopType, iTopType, iLeftType;
	int32_t iCurX, iCurY, iCurXy, iLeftXy, iTopXy, iLeftTopXy, iRightTopXy;

	int8_t iLeftRef;
	int8_t iTopRef;
	int8_t iRightTopRef;
	int8_t iLeftTopRef;
	int8_t iDiagonalRef;
	int8_t iMatchRef;
	int16_t iMvA[2], iMvB[2], iMvC[2], iMvD[2];

	iCurXy = pCurLayer->iMbXyIndex;
	iCurX  = pCurLayer->iMbX;
	iCurY  = pCurLayer->iMbY;
	iCurSliceIdc = pCurLayer->pSliceIdc[iCurXy];

	if( iCurX != 0)
	{
		iLeftXy = iCurXy- 1;
		iLeftSliceIdc = pCurLayer->pSliceIdc[iLeftXy];
		bLeftAvail = (iLeftSliceIdc == iCurSliceIdc);
	}
	else
	{
		bLeftAvail = 0;
		bLeftTopAvail = 0;
	}

	if( iCurY != 0)
	{
		iTopXy = iCurXy - pCurLayer->iMbWidth;
		iTopSliceIdc = pCurLayer->pSliceIdc[iTopXy];
		bTopAvail = (iTopSliceIdc == iCurSliceIdc);
		if (iCurX != 0)
		{
			iLeftTopXy = iTopXy - 1;
			iLeftTopSliceIdc = pCurLayer->pSliceIdc[iLeftTopXy];
			bLeftTopAvail = (iLeftTopSliceIdc  == iCurSliceIdc);
		}
		else
		{
			bLeftTopAvail = 0;
		}
		if (iCurX != (pCurLayer->iMbWidth-1))
		{
			iRightTopXy = iTopXy + 1;
			iRightTopSliceIdc = pCurLayer->pSliceIdc[iRightTopXy];
			bRightTopAvail = (iRightTopSliceIdc == iCurSliceIdc);
		}
		else
		{
			bRightTopAvail = 0;
		}
	}
	else
	{
		bTopAvail = 0;
		bLeftTopAvail = 0;
		bRightTopAvail = 0;
	}

	iLeftType = ((iCurX!=0 && bLeftAvail) ? pCurLayer->pMbType[iLeftXy]: 0);
	iTopType = ((iCurY!=0 && bTopAvail) ? pCurLayer->pMbType[iTopXy]: 0);
	iLeftTopType = ((iCurX!=0 &&iCurY!=0 && bLeftTopAvail)
					? pCurLayer->pMbType[iLeftTopXy]: 0);
	iRightTopType = ((iCurX!=pCurLayer->iMbWidth-1 &&iCurY!=0 && bRightTopAvail)
					? pCurLayer->pMbType[iRightTopXy]: 0);

	/*get neb mv&iRefIdxArray*/
	/*left*/
	if (bLeftAvail && IS_INTER(iLeftType))
	{
		ST32(iMvA, LD32(pCurLayer->pMv[0][iLeftXy][3]));
		iLeftRef = pCurLayer->pRefIndex[0][iLeftXy][3];
	}
	else
	{
		ST32(iMvA, 0);
		if (0 == bLeftAvail) //not available
		{
			iLeftRef = REF_NOT_AVAIL;
		}
		else //available but is intra mb type
		{
			iLeftRef = REF_NOT_IN_LIST;
		}
	}
	if (REF_NOT_AVAIL == iLeftRef ||
		(0 == iLeftRef && 0 == *(int32_t*)iMvA))
	{
		ST32( iMvp, 0 );
		return;
	}

	/*top*/
	if (bTopAvail && IS_INTER(iTopType))
	{
		ST32( iMvB, LD32(pCurLayer->pMv[0][iTopXy][12]) );
		iTopRef = pCurLayer->pRefIndex[0][iTopXy][12];
	}
	else
	{
		ST32( iMvB, 0 );
		if (0 == bTopAvail) //not available
		{
		    iTopRef = REF_NOT_AVAIL;
		}
		else //available but is intra mb type
		{
			iTopRef = REF_NOT_IN_LIST;
		}
	}
	if (REF_NOT_AVAIL == iTopRef ||
		(0 == iTopRef  && 0 == *(int32_t*)iMvB))
	{
		ST32( iMvp, 0 );
		return;
	}

	/*right_top*/
	if (bRightTopAvail && IS_INTER(iRightTopType))
	{
		ST32(iMvC, LD32(pCurLayer->pMv[0][iRightTopXy][12]));
		iRightTopRef = pCurLayer->pRefIndex[0][iRightTopXy][12];
	}
	else
	{
		ST32(iMvC, 0);
		if (0 == bRightTopAvail) //not available
		{
			iRightTopRef = REF_NOT_AVAIL;
		}
		else //available but is intra mb type
		{
			iRightTopRef = REF_NOT_IN_LIST;
		}
	}

	/*left_top*/
	if (bLeftTopAvail && IS_INTER(iLeftTopType))
	{
		ST32(iMvD, LD32(pCurLayer->pMv[0][iLeftTopXy][15]));
		iLeftTopRef = pCurLayer->pRefIndex[0][iLeftTopXy][15];
	}
	else
	{
		ST32(iMvD, 0);
		if (0 == bLeftTopAvail) //not available
		{
			iLeftTopRef = REF_NOT_AVAIL;
		}
		else //available but is intra mb type
		{
			iLeftTopRef = REF_NOT_IN_LIST;
		}
	}

	iDiagonalRef = iRightTopRef;
	if (REF_NOT_AVAIL == iDiagonalRef)
	{
		iDiagonalRef = iLeftTopRef;
		*(int32_t*)iMvC = *(int32_t*)iMvD;
	}

	if (REF_NOT_AVAIL == iTopRef && REF_NOT_AVAIL == iDiagonalRef && iLeftRef >= REF_NOT_IN_LIST)
	{
		ST32(iMvp, LD32(iMvA));
		return;
	}

	iMatchRef = (0 == iLeftRef) + (0 == iTopRef) + (0 == iDiagonalRef);
	if (1 == iMatchRef)
	{
		if (0 == iLeftRef)
		{
			ST32(iMvp, LD32(iMvA));
		}
		else if (0 == iTopRef)
		{
			ST32(iMvp, LD32(iMvB));
		}
		else
		{
			ST32(iMvp, LD32(iMvC));
		}
	}
	else
	{
		iMvp[0] = WelsMedian(iMvA[0], iMvB[0], iMvC[0]);
		iMvp[1] = WelsMedian(iMvA[1], iMvB[1], iMvC[1]);
	}
}

void_t GetNeighborAvailMbType( PNeighAvail pNeighAvail, PDqLayer pCurLayer )
{
	int32_t iCurSliceIdc, iTopSliceIdc, iLeftTopSliceIdc, iRightTopSliceIdc, iLeftSliceIdc;
	int32_t iCurXy, iTopXy, iLeftXy, iLeftTopXy, iRightTopXy;
	int32_t iCurX, iCurY;

	iCurXy = pCurLayer->iMbXyIndex;
	iCurX  = pCurLayer->iMbX;
	iCurY  = pCurLayer->iMbY;
	iCurSliceIdc = pCurLayer->pSliceIdc[iCurXy];
	if( iCurX != 0)
	{
		iLeftXy = iCurXy- 1;
		iLeftSliceIdc = pCurLayer->pSliceIdc[iLeftXy];
		pNeighAvail->iLeftAvail = (iLeftSliceIdc == iCurSliceIdc);
	}
	else
	{
		pNeighAvail->iLeftAvail = 0;
		pNeighAvail->iLeftTopAvail = 0;
	}

	if( iCurY != 0)
	{
		iTopXy = iCurXy - pCurLayer->iMbWidth;
		iTopSliceIdc = pCurLayer->pSliceIdc[iTopXy];
		pNeighAvail->iTopAvail = (iTopSliceIdc == iCurSliceIdc);
		if (iCurX != 0)
		{
			iLeftTopXy = iTopXy - 1;
			iLeftTopSliceIdc = pCurLayer->pSliceIdc[iLeftTopXy];
			pNeighAvail->iLeftTopAvail = (iLeftTopSliceIdc == iCurSliceIdc);
		}
		else
		{
			pNeighAvail->iLeftTopAvail = 0;
		}
		if (iCurX != (pCurLayer->iMbWidth-1))
		{
			iRightTopXy = iTopXy + 1;
			iRightTopSliceIdc = pCurLayer->pSliceIdc[iRightTopXy];
			pNeighAvail->iRightTopAvail = (iRightTopSliceIdc == iCurSliceIdc);
		}
		else
		{
			pNeighAvail->iRightTopAvail = 0;
		}
	}
	else
	{
		pNeighAvail->iTopAvail = 0;
		pNeighAvail->iLeftTopAvail = 0;
		pNeighAvail->iRightTopAvail = 0;
	}

	pNeighAvail->iLeftType     = ( pNeighAvail->iLeftAvail     ? pCurLayer->pMbType[iLeftXy]     : 0 );
	pNeighAvail->iTopType      = ( pNeighAvail->iTopAvail      ? pCurLayer->pMbType[iTopXy]      : 0 );
	pNeighAvail->iLeftTopType  = ( pNeighAvail->iLeftTopAvail  ? pCurLayer->pMbType[iLeftTopXy]  : 0 );
	pNeighAvail->iRightTopType = ( pNeighAvail->iRightTopAvail ? pCurLayer->pMbType[iRightTopXy] : 0 );
}
void_t WelsFillCacheNonZeroCount(PNeighAvail pNeighAvail, uint8_t* pNonZeroCount, PDqLayer pCurLayer) //no matter slice type, intra_pred_constrained_flag
{
	int32_t iCurXy  = pCurLayer->iMbXyIndex;
	int32_t iTopXy  = 0;
	int32_t iLeftXy = 0;

	GetNeighborAvailMbType( pNeighAvail, pCurLayer );

	if ( pNeighAvail->iTopAvail )
	{
		iTopXy = iCurXy - pCurLayer->iMbWidth;
	}
	if ( pNeighAvail->iLeftAvail )
	{
		iLeftXy = iCurXy - 1;
	}

	//stuff non_zero_coeff_count from pNeighAvail(left and top)
	if (pNeighAvail->iTopAvail)
	{
		ST32(&pNonZeroCount[1], LD32(&pCurLayer->pNzc[iTopXy][12]));
        pNonZeroCount[0] = pNonZeroCount[5] = pNonZeroCount[29] = 0;
		ST16(&pNonZeroCount[6], LD16(&pCurLayer->pNzc[iTopXy][20]));
		ST16(&pNonZeroCount[30], LD16(&pCurLayer->pNzc[iTopXy][22]));
	}
	else
	{
		ST32(&pNonZeroCount[1], 0xFFFFFFFFU);
        pNonZeroCount[0] = pNonZeroCount[5] = pNonZeroCount[29] = 0xFF;
		ST16(&pNonZeroCount[6], 0xFFFF);
		ST16(&pNonZeroCount[30], 0xFFFF);
	}

	if (pNeighAvail->iLeftAvail)
	{
		pNonZeroCount[8 * 1] = pCurLayer->pNzc[iLeftXy][3];
		pNonZeroCount[8 * 2] = pCurLayer->pNzc[iLeftXy][7];
		pNonZeroCount[8 * 3] = pCurLayer->pNzc[iLeftXy][11];
		pNonZeroCount[8 * 4] = pCurLayer->pNzc[iLeftXy][15];

		pNonZeroCount[5 + 8 * 1] = pCurLayer->pNzc[iLeftXy][17];
		pNonZeroCount[5 + 8 * 2] = pCurLayer->pNzc[iLeftXy][21];
		pNonZeroCount[5 + 8 * 4] = pCurLayer->pNzc[iLeftXy][19];
		pNonZeroCount[5 + 8 * 5] = pCurLayer->pNzc[iLeftXy][23];
	}
	else
	{
		pNonZeroCount[8 * 1] =
		pNonZeroCount[8 * 2] =
		pNonZeroCount[8 * 3] =
		pNonZeroCount[8 * 4] = -1;//unavailable

		pNonZeroCount[5 + 8 * 1] =
		pNonZeroCount[5 + 8 * 2] = -1;//unavailable

		pNonZeroCount[5 + 8 * 4] =
		pNonZeroCount[5 + 8 * 5] = -1;//unavailable
	}
}
void_t WelsFillCacheConstrain1Intra4x4(PNeighAvail pNeighAvail, uint8_t* pNonZeroCount, int8_t* pIntraPredMode, PDqLayer pCurLayer) //no matter slice type
{
	int32_t iCurXy  = pCurLayer->iMbXyIndex;
	int32_t iTopXy  = 0;
	int32_t iLeftXy = 0;

	//stuff non_zero_coeff_count from pNeighAvail(left and top)
	WelsFillCacheNonZeroCount( pNeighAvail, pNonZeroCount, pCurLayer );

	if ( pNeighAvail->iTopAvail )
	{
		iTopXy = iCurXy - pCurLayer->iMbWidth;
	}
	if ( pNeighAvail->iLeftAvail )
	{
		iLeftXy = iCurXy - 1;
	}

	//intra4x4_pred_mode
	if (pNeighAvail->iTopAvail && IS_INTRA4x4(pNeighAvail->iTopType)) //top
	{
        ST32(pIntraPredMode+1, LD32(&pCurLayer->pIntraPredMode[iTopXy][0]));
	}
	else
	{
		int32_t iPred;
		if( IS_INTRA16x16( pNeighAvail->iTopType ) || ( MB_TYPE_INTRA_PCM == pNeighAvail->iTopType ) )
			iPred= 0x02020202;
		else
			iPred= 0xffffffff;
        ST32(pIntraPredMode+1, iPred);
	}

	if (pNeighAvail->iLeftAvail && IS_INTRA4x4(pNeighAvail->iLeftType)) //left
	{
		pIntraPredMode[ 0 + 8    ] = pCurLayer->pIntraPredMode[iLeftXy][4];
		pIntraPredMode[ 0 + 8 * 2] = pCurLayer->pIntraPredMode[iLeftXy][5];
		pIntraPredMode[ 0 + 8 * 3] = pCurLayer->pIntraPredMode[iLeftXy][6];
		pIntraPredMode[ 0 + 8 * 4] = pCurLayer->pIntraPredMode[iLeftXy][3];
	}
	else
	{
		int8_t iPred;
		if( IS_INTRA16x16( pNeighAvail->iLeftType ) || ( MB_TYPE_INTRA_PCM == pNeighAvail->iLeftType ) )
			iPred= 2;
		else
			iPred= -1;
		pIntraPredMode[ 0 + 8    ] =
		pIntraPredMode[ 0 + 8 * 2] =
		pIntraPredMode[ 0 + 8 * 3] =
		pIntraPredMode[ 0 + 8 * 4] = iPred;
	}
}

void_t WelsFillCacheConstrain0Intra4x4(PNeighAvail pNeighAvail, uint8_t* pNonZeroCount, int8_t* pIntraPredMode, PDqLayer pCurLayer) //no matter slice type
{
	int32_t iCurXy  = pCurLayer->iMbXyIndex;
	int32_t iTopXy  = 0;
	int32_t iLeftXy = 0;

	//stuff non_zero_coeff_count from pNeighAvail(left and top)
	WelsFillCacheNonZeroCount( pNeighAvail, pNonZeroCount, pCurLayer );

	if ( pNeighAvail->iTopAvail )
	{
		iTopXy = iCurXy - pCurLayer->iMbWidth;
	}
	if ( pNeighAvail->iLeftAvail )
	{
		iLeftXy = iCurXy - 1;
	}

	//intra4x4_pred_mode
	if (pNeighAvail->iTopAvail && IS_INTRA4x4(pNeighAvail->iTopType)) //top
	{
        ST32(pIntraPredMode + 1, LD32(&pCurLayer->pIntraPredMode[iTopXy][0]));
	}
	else
	{
		int32_t iPred;
		if( pNeighAvail->iTopAvail )
			iPred= 0x02020202;
		else
			iPred= 0xffffffff;
        ST32(pIntraPredMode + 1, iPred);
	}

	if (pNeighAvail->iLeftAvail && IS_INTRA4x4(pNeighAvail->iLeftType)) //left
	{
		pIntraPredMode[ 0 + 8 * 1] = pCurLayer->pIntraPredMode[iLeftXy][4];
		pIntraPredMode[ 0 + 8 * 2] = pCurLayer->pIntraPredMode[iLeftXy][5];
		pIntraPredMode[ 0 + 8 * 3] = pCurLayer->pIntraPredMode[iLeftXy][6];
		pIntraPredMode[ 0 + 8 * 4] = pCurLayer->pIntraPredMode[iLeftXy][3];
	}
	else
	{
		int8_t iPred;
		if( pNeighAvail->iLeftAvail )
			iPred= 2;
		else
			iPred= -1;
		pIntraPredMode[ 0 + 8 * 1] =
		pIntraPredMode[ 0 + 8 * 2] =
		pIntraPredMode[ 0 + 8 * 3] =
		pIntraPredMode[ 0 + 8 * 4] = iPred;
	}
}

void_t WelsFillCacheInter(PNeighAvail pNeighAvail, uint8_t* pNonZeroCount,
						  int16_t iMvArray[LIST_A][30][MV_A], int8_t iRefIdxArray[LIST_A][30], PDqLayer pCurLayer)
{
	int32_t iCurXy      = pCurLayer->iMbXyIndex;
	int32_t iTopXy      = 0;
	int32_t iLeftXy     = 0;
	int32_t iLeftTopXy  = 0;
	int32_t iRightTopXy = 0;

	//stuff non_zero_coeff_count from pNeighAvail(left and top)
	WelsFillCacheNonZeroCount( pNeighAvail, pNonZeroCount, pCurLayer );

	if ( pNeighAvail->iTopAvail )
	{
		iTopXy = iCurXy - pCurLayer->iMbWidth;
	}
	if ( pNeighAvail->iLeftAvail )
	{
		iLeftXy = iCurXy - 1;
	}
	if ( pNeighAvail->iLeftTopAvail )
	{
		iLeftTopXy = iCurXy - 1 - pCurLayer->iMbWidth;
	}
	if ( pNeighAvail->iRightTopAvail )
	{
		iRightTopXy = iCurXy + 1- pCurLayer->iMbWidth;
	}

	//stuff mv_cache and iRefIdxArray from left and top (inter)
	if (pNeighAvail->iLeftAvail && IS_INTER(pNeighAvail->iLeftType))
	{
		ST32(iMvArray[0][ 6], LD32(pCurLayer->pMv[0][iLeftXy][ 3]));
		ST32(iMvArray[0][12], LD32(pCurLayer->pMv[0][iLeftXy][ 7]));
		ST32(iMvArray[0][18], LD32(pCurLayer->pMv[0][iLeftXy][11]));
		ST32(iMvArray[0][24], LD32(pCurLayer->pMv[0][iLeftXy][15]));
		iRefIdxArray[0][ 6] = pCurLayer->pRefIndex[0][iLeftXy][ 3];
		iRefIdxArray[0][12] = pCurLayer->pRefIndex[0][iLeftXy][ 7];
		iRefIdxArray[0][18] = pCurLayer->pRefIndex[0][iLeftXy][11];
		iRefIdxArray[0][24] = pCurLayer->pRefIndex[0][iLeftXy][15];
	}
	else
	{
		ST32(iMvArray[0][ 6], 0);
		ST32(iMvArray[0][12], 0);
		ST32(iMvArray[0][18], 0);
		ST32(iMvArray[0][24], 0);

		if (0 == pNeighAvail->iLeftAvail) //not available
		{
			iRefIdxArray[0][ 6] =
			iRefIdxArray[0][12] =
			iRefIdxArray[0][18] =
			iRefIdxArray[0][24] = REF_NOT_AVAIL;
		}
		else //available but is intra mb type
		{
			iRefIdxArray[0][ 6] =
			iRefIdxArray[0][12] =
			iRefIdxArray[0][18] =
			iRefIdxArray[0][24] = REF_NOT_IN_LIST;
		}
	}
	if (pNeighAvail->iLeftTopAvail && IS_INTER(pNeighAvail->iLeftTopType))
	{
		ST32(iMvArray[0][0], LD32(pCurLayer->pMv[0][iLeftTopXy][15]));
        iRefIdxArray[0][0] = pCurLayer->pRefIndex[0][iLeftTopXy][15];
	}
	else
	{
		ST32(iMvArray[0][0], 0);
		if (0 == pNeighAvail->iLeftTopAvail) //not available
		{
			iRefIdxArray[0][0] = REF_NOT_AVAIL;
		}
		else //available but is intra mb type
		{
			iRefIdxArray[0][0] = REF_NOT_IN_LIST;
		}
	}

	if (pNeighAvail->iTopAvail && IS_INTER(pNeighAvail->iTopType))
	{
		ST64(iMvArray[0][1], LD64(pCurLayer->pMv[0][iTopXy][12]));
		ST64(iMvArray[0][3], LD64(pCurLayer->pMv[0][iTopXy][14]));
        ST32(&iRefIdxArray[0][1], LD32(&pCurLayer->pRefIndex[0][iTopXy][12]));
	}
	else
	{
		ST64(iMvArray[0][1], 0);
		ST64(iMvArray[0][3], 0);

		if (0 == pNeighAvail->iTopAvail) //not available
		{
			iRefIdxArray[0][1] =
			iRefIdxArray[0][2] =
			iRefIdxArray[0][3] =
			iRefIdxArray[0][4] = REF_NOT_AVAIL;
		}
		else //available but is intra mb type
		{
			iRefIdxArray[0][1] =
			iRefIdxArray[0][2] =
			iRefIdxArray[0][3] =
			iRefIdxArray[0][4] = REF_NOT_IN_LIST;
		}
	}

	if (pNeighAvail->iRightTopAvail && IS_INTER(pNeighAvail->iRightTopType))
	{
		ST32(iMvArray[0][5], LD32(pCurLayer->pMv[0][iRightTopXy][12]));
		iRefIdxArray[0][5] = pCurLayer->pRefIndex[0][iRightTopXy][12];
	}
	else
	{
		ST32(iMvArray[0][5], 0);
		if (0 == pNeighAvail->iRightTopAvail) //not available
		{
			iRefIdxArray[0][5] = REF_NOT_AVAIL;
		}
		else //available but is intra mb type
		{
			iRefIdxArray[0][5] = REF_NOT_IN_LIST;
		}
	}

	//right-top 4*4 block unavailable
	ST32(iMvArray[0][ 9], 0);
	ST32(iMvArray[0][21], 0);
	ST32(iMvArray[0][11], 0);
	ST32(iMvArray[0][17], 0);
	ST32(iMvArray[0][23], 0);
	iRefIdxArray[0][ 9] =
	iRefIdxArray[0][21] =
	iRefIdxArray[0][11] =
	iRefIdxArray[0][17] =
	iRefIdxArray[0][23] = REF_NOT_AVAIL;
}

int32_t PredIntra4x4Mode(int8_t* pIntraPredMode, int32_t iIdx4)
{
	int8_t iTopMode  = pIntraPredMode[g_kuiScan8[iIdx4] - 8];
	int8_t iLeftMode = pIntraPredMode[g_kuiScan8[iIdx4] - 1];
	int8_t iBestMode;

	if (-1 == iLeftMode || -1 == iTopMode)
	{
		iBestMode = 2;
	}
	else
	{
		iBestMode = WELS_MIN(iLeftMode, iTopMode);
	}
	return iBestMode;
}

#define MAX_PRED_MODE_ID_I16x16  3
#define MAX_PRED_MODE_ID_CHROMA  3
#define MAX_PRED_MODE_ID_I4x4    8
#define CHECK_I16_MODE(a, b, c, d)                           \
                      ((a == g_ksI16PredInfo[a].iPredMode) &&  \
					   (b >= g_ksI16PredInfo[a].iLeftAvail) && \
					   (c >= g_ksI16PredInfo[a].iTopAvail) &&  \
					   (d >= g_ksI16PredInfo[a].iLeftTopAvail));
#define CHECK_CHROMA_MODE(a, b, c, d)                              \
	                     ((a == g_ksChromaPredInfo[a].iPredMode) &&  \
					      (b >= g_ksChromaPredInfo[a].iLeftAvail) && \
					      (c >= g_ksChromaPredInfo[a].iTopAvail) &&  \
					      (d >= g_ksChromaPredInfo[a].iLeftTopAvail));
#define CHECK_I4_MODE(a, b, c, d)                              \
	                 ((a == g_ksI4PredInfo[a].iPredMode) &&      \
                      (b >= g_ksI4PredInfo[a].iLeftAvail) &&     \
                      (c >= g_ksI4PredInfo[a].iTopAvail) &&      \
                      (d >= g_ksI4PredInfo[a].iLeftTopAvail));


int32_t CheckIntra16x16PredMode(uint8_t uiSampleAvail, int8_t* pMode)
{
	int32_t iLeftAvail     = uiSampleAvail & 0x04;
	int32_t bLeftTopAvail  = uiSampleAvail & 0x02;
	int32_t iTopAvail      = uiSampleAvail & 0x01;

	if (*pMode > MAX_PRED_MODE_ID_I16x16)
	{
		return ERR_INFO_INVALID_I16x16_PRED_MODE;
	}

	if (I16_PRED_DC == *pMode)
	{
		if (iLeftAvail && iTopAvail)
		{
			return 0;
		}
		else if (iLeftAvail)
		{
			*pMode = I16_PRED_DC_L;
		}
		else if (iTopAvail)
		{
			*pMode = I16_PRED_DC_T;
		}
		else
		{
			*pMode = I16_PRED_DC_128;
		}
	}
	else
	{
		bool_t bModeAvail = CHECK_I16_MODE(*pMode, iLeftAvail, iTopAvail, bLeftTopAvail);
		if (0 == bModeAvail)
		{
			return ERR_INFO_INVALID_I16x16_PRED_MODE;
		}
	}
	return 0;
}


int32_t CheckIntraChromaPredMode(uint8_t uiSampleAvail, int8_t* pMode)
{
	int32_t iLeftAvail     = uiSampleAvail & 0x04;
	int32_t bLeftTopAvail  = uiSampleAvail & 0x02;
	int32_t iTopAvail      = uiSampleAvail & 0x01;

	if (*pMode > MAX_PRED_MODE_ID_CHROMA)
	{
		return ERR_INFO_INVALID_I_CHROMA_PRED_MODE;
	}

	if (C_PRED_DC == *pMode)
	{
		if (iLeftAvail && iTopAvail)
		{
			return 0;
		}
		else if (iLeftAvail)
		{
			*pMode = C_PRED_DC_L;
		}
		else if (iTopAvail)
		{
			*pMode = C_PRED_DC_T;
		}
		else
		{
			*pMode = C_PRED_DC_128;
		}
	}
	else
	{
		bool_t bModeAvail = CHECK_CHROMA_MODE(*pMode, iLeftAvail, iTopAvail, bLeftTopAvail);
		if (0 == bModeAvail)
		{
			return ERR_INFO_INVALID_I_CHROMA_PRED_MODE;
		}
	}
	return 0;
}

int32_t CheckIntra4x4PredMode(int32_t* pSampleAvail, int8_t* pMode, int32_t iIndex)
{
	int8_t iIdx = g_kuiCache30ScanIdx[iIndex];
	int32_t iLeftAvail     = pSampleAvail[iIdx-1];
	int32_t iTopAvail      = pSampleAvail[iIdx-6];
	int32_t bLeftTopAvail  = pSampleAvail[iIdx-7];
	int32_t bRightTopAvail = pSampleAvail[iIdx-5];

	int8_t iFinalMode;

	if (*pMode > MAX_PRED_MODE_ID_I4x4)
	{
		return -1;
	}

	if (I4_PRED_DC == *pMode)
	{
		if (iLeftAvail && iTopAvail)
		{
			return *pMode;
		}
		else if (iLeftAvail)
		{
			iFinalMode = I4_PRED_DC_L;
		}
		else if (iTopAvail)
		{
			iFinalMode = I4_PRED_DC_T;
		}
		else
		{
			iFinalMode = I4_PRED_DC_128;
		}
	}
	else
	{
		bool_t bModeAvail = CHECK_I4_MODE(*pMode, iLeftAvail, iTopAvail, bLeftTopAvail);
		if (0 == bModeAvail)
		{
			return -1;
		}

		iFinalMode = *pMode;

		//if right-top unavailable, modify mode DDL and VL (padding rightmost pixel of top)
		if (I4_PRED_DDL == iFinalMode && 0 == bRightTopAvail)
		{
			iFinalMode = I4_PRED_DDL_TOP;
		}
		else if (I4_PRED_VL == iFinalMode && 0 == bRightTopAvail)
		{
			iFinalMode = I4_PRED_VL_TOP;
		}
	}
	return iFinalMode;
}

void_t BsStartCavlc( PBitStringAux pBs )
{
	pBs->iIndex = ((pBs->pCurBuf - pBs->pStartBuf)<<3) - (16 - pBs->iLeftBits);
}
void_t BsEndCavlc( PBitStringAux pBs )
{
	pBs->pCurBuf   = pBs->pStartBuf + (pBs->iIndex>>3);
	pBs->uiCurBits = ((((pBs->pCurBuf[0] << 8) | pBs->pCurBuf[1]) << 16) | (pBs->pCurBuf[2] << 8) | pBs->pCurBuf[3]) << (pBs->iIndex & 0x07);
	pBs->pCurBuf  += 4;
	pBs->iLeftBits = -16 + (pBs->iIndex&0x07);
}


// return: used bits
static int32_t CavlcGetTrailingOnesAndTotalCoeff(uint8_t &uiTotalCoeff, uint8_t &uiTrailingOnes, SReadBitsCache *pBitsCache, SVlcTable* pVlcTable, bool_t bChromaDc, int8_t nC)
{
	const uint8_t *kpVlcTableMoreBitsCountList[3] = {g_kuiVlcTableMoreBitsCount0, g_kuiVlcTableMoreBitsCount1, g_kuiVlcTableMoreBitsCount2};
    int32_t iUsedBits = 0;
	int32_t iIndexVlc, iIndexValue, iNcMapIdx;
	uint32_t uiCount;
	uint32_t uiValue;

    if (bChromaDc)
	{
		uiValue        = pBitsCache->uiCache32Bit >> 24;
		iIndexVlc      = pVlcTable->kpChromaCoeffTokenVlcTable[uiValue][0];
		uiCount        = pVlcTable->kpChromaCoeffTokenVlcTable[uiValue][1];
		POP_BUFFER(pBitsCache, uiCount);
		iUsedBits     += uiCount;
		uiTrailingOnes = g_kuiVlcTrailingOneTotalCoeffTable[iIndexVlc][0];
		uiTotalCoeff   = g_kuiVlcTrailingOneTotalCoeffTable[iIndexVlc][1];
	}
	else //luma
	{
		iNcMapIdx = g_kuiNcMapTable[nC];
		if ( iNcMapIdx<= 2 )
		{
			uiValue = pBitsCache->uiCache32Bit >> 24;
			if ( uiValue < g_kuiVlcTableNeedMoreBitsThread[iNcMapIdx] )
			{
				POP_BUFFER(pBitsCache, 8);
				iUsedBits  += 8;
				iIndexValue = pBitsCache->uiCache32Bit >> ( 32 - kpVlcTableMoreBitsCountList[iNcMapIdx][uiValue]);
				iIndexVlc   = pVlcTable->kpCoeffTokenVlcTable[iNcMapIdx+1][uiValue][iIndexValue][0];
				uiCount     = pVlcTable->kpCoeffTokenVlcTable[iNcMapIdx+1][uiValue][iIndexValue][1];
				POP_BUFFER(pBitsCache, uiCount);
				iUsedBits  += uiCount;
			}
			else
			{
				iIndexVlc  = pVlcTable->kpCoeffTokenVlcTable[0][iNcMapIdx][uiValue][0];
				uiCount    = pVlcTable->kpCoeffTokenVlcTable[0][iNcMapIdx][uiValue][1];
				uiValue    = pBitsCache->uiCache32Bit >> (32 - uiCount);
				POP_BUFFER(pBitsCache, uiCount);
				iUsedBits += uiCount;
			}
		}
		else
		{
			uiValue    = pBitsCache->uiCache32Bit >> (32 - 6);
			POP_BUFFER(pBitsCache, 6);
			iUsedBits += 6;
			iIndexVlc  = pVlcTable->kpCoeffTokenVlcTable[0][3][uiValue][0];  //differ
		}
		uiTrailingOnes= g_kuiVlcTrailingOneTotalCoeffTable[iIndexVlc][0];
		uiTotalCoeff  = g_kuiVlcTrailingOneTotalCoeffTable[iIndexVlc][1];
	}

	return iUsedBits;
}

static int32_t CavlcGetLevelVal(int32_t iLevel[16], SReadBitsCache *pBitsCache, uint8_t uiTotalCoeff, uint8_t uiTrailingOnes)
{
    int32_t i, iUsedBits = 0;
    int32_t iSuffixLength, iSuffixLengthSize, iLevelPrefix, iPrefixBits, iLevelCode, iThreshold;
    uint32_t uiCache32Bit;
	for (i = 0; i < uiTrailingOnes; i++)
	{
		iLevel[i] = 1 - ((pBitsCache->uiCache32Bit >> (30 - i)) & 0x02);
	}
	POP_BUFFER(pBitsCache, uiTrailingOnes);
	iUsedBits += uiTrailingOnes;

	iSuffixLength = (uiTotalCoeff > 10 && uiTrailingOnes < 3);

	for (; i < uiTotalCoeff; i++)
	{
		if(pBitsCache->uiRemainBits <= 16)		SHIFT_BUFFER(pBitsCache);
#ifdef WIN32
        uiCache32Bit = pBitsCache->uiCache32Bit;
		WELS_GET_PREFIX_BITS(uiCache32Bit,iPrefixBits);
#else
		iPrefixBits = GetPrefixBits(pBitsCache->uiCache32Bit);
#endif
		POP_BUFFER(pBitsCache, iPrefixBits);
		iUsedBits   += iPrefixBits;
		iLevelPrefix = iPrefixBits - 1;

		iLevelCode = (WELS_MIN(15, iLevelPrefix)) << iSuffixLength; //differ
		iSuffixLengthSize = iSuffixLength;

		if (iLevelPrefix >= 14)
		{
			if (14 == iLevelPrefix && 0 == iSuffixLength)
				iSuffixLengthSize = 4;
			else if (15 == iLevelPrefix)
				iSuffixLengthSize = 12;
			else if(iLevelPrefix > 15)
				iLevelCode += (1 << (iLevelPrefix - 3)) - 4096;

			if (iLevelPrefix >= 15 && iSuffixLength == 0)
				iLevelCode += 15;
		}

		if(iSuffixLengthSize > 0)
		{
			if(pBitsCache->uiRemainBits <= iSuffixLengthSize) SHIFT_BUFFER(pBitsCache);
			if(pBitsCache->uiRemainBits <= iSuffixLengthSize)
			return 0;
			iLevelCode += (pBitsCache->uiCache32Bit >> (32 - iSuffixLengthSize));
			POP_BUFFER(pBitsCache, iSuffixLengthSize);
			iUsedBits  += iSuffixLengthSize;
		}

		iLevelCode += ((i == uiTrailingOnes) && (uiTrailingOnes < 3)) << 1;
		iLevel[i]   = ((iLevelCode + 2) >> 1);
		iLevel[i]  -= (iLevel[i] << 1) & (-(iLevelCode & 0x01));

		iSuffixLength += !iSuffixLength;
		iThreshold     = 3 << ( iSuffixLength - 1 );
		iSuffixLength += ((iLevel[i] > iThreshold) || (iLevel[i] < -iThreshold)) && (iSuffixLength < 6);
	}

	return iUsedBits;
}

static int32_t CavlcGetTotalZeros(int32_t &iZerosLeft, SReadBitsCache *pBitsCache, uint8_t uiTotalCoeff, SVlcTable* pVlcTable, bool_t bChromaDc)
{
	int32_t iCount, iUsedBits = 0;
	const uint8_t *kpBitNumMap;
	uint32_t uiValue;

	int32_t iTotalZeroVlcIdx;
	uint8_t uiTableType;
	//chroma_dc (0 < uiTotalCoeff < 4); others (chroma_ac or luma: 0 < uiTotalCoeff < 16)

	if ( bChromaDc )
	{
		iTotalZeroVlcIdx = uiTotalCoeff;
		kpBitNumMap = g_kuiTotalZerosBitNumChromaMap;
		uiTableType = bChromaDc;
	}
	else
	{
		iTotalZeroVlcIdx = uiTotalCoeff;
		kpBitNumMap = g_kuiTotalZerosBitNumMap;
		uiTableType = 0;
	}

	iCount = kpBitNumMap[iTotalZeroVlcIdx-1];
	if(pBitsCache->uiRemainBits < iCount) SHIFT_BUFFER(pBitsCache);// if uiRemainBits+16 still smaller than iCount?? potential bug
	if(pBitsCache->uiRemainBits < iCount)
		return 0;
	uiValue    = pBitsCache->uiCache32Bit >> ( 32 - iCount );
	iCount     = pVlcTable->kpTotalZerosTable[uiTableType][iTotalZeroVlcIdx-1][uiValue][1];
	POP_BUFFER(pBitsCache, iCount);
	iUsedBits += iCount;
	iZerosLeft = pVlcTable->kpTotalZerosTable[uiTableType][iTotalZeroVlcIdx-1][uiValue][0];

	return iUsedBits;
}
static int32_t	CavlcGetRunBefore(int32_t iRun[16], SReadBitsCache *pBitsCache, uint8_t uiTotalCoeff, SVlcTable* pVlcTable, int32_t iZerosLeft)
{
    int32_t i, iUsedBits = 0;
	uint32_t uiCount, uiValue, uiCache32Bit, iPrefixBits;

	for (i = 0; i < uiTotalCoeff-1; i++)
	{
		if (iZerosLeft > 0)
		{
			uiCount = g_kuiZeroLeftBitNumMap[iZerosLeft];
			if(pBitsCache->uiRemainBits < uiCount ) SHIFT_BUFFER(pBitsCache);
			if(pBitsCache->uiRemainBits < uiCount)
			return 0;
			uiValue = pBitsCache->uiCache32Bit >> ( 32 - uiCount );
			if ( iZerosLeft < 7 )
			{
				uiCount = pVlcTable->kpZeroTable[iZerosLeft-1][uiValue][1];
				POP_BUFFER(pBitsCache, uiCount);
				iUsedBits += uiCount;
				iRun[i] = pVlcTable->kpZeroTable[iZerosLeft-1][uiValue][0];
			}
			else
			{
				POP_BUFFER(pBitsCache, uiCount);
				iUsedBits += uiCount;
				if ( pVlcTable->kpZeroTable[6][uiValue][0] < 7 )
				{
					iRun[i] = pVlcTable->kpZeroTable[6][uiValue][0];
				}
				else
				{
					if(pBitsCache->uiRemainBits < 16) SHIFT_BUFFER(pBitsCache);
#ifdef WIN32
					uiCache32Bit = pBitsCache->uiCache32Bit;
					WELS_GET_PREFIX_BITS(uiCache32Bit, iPrefixBits);
#else
					iPrefixBits = GetPrefixBits(pBitsCache->uiCache32Bit);
#endif
					iRun[i] = iPrefixBits + 6;
					POP_BUFFER(pBitsCache, iPrefixBits);
					iUsedBits += iPrefixBits;
				}
			}
		}
		else
		{
			return iUsedBits;
		}

		iZerosLeft -= iRun[i];
	}

	iRun[uiTotalCoeff-1] = iZerosLeft;

	return iUsedBits;
}

int32_t WelsResidualBlockCavlc(SVlcTable* pVlcTable, uint8_t* pNonZeroCountCache, PBitStringAux pBs, int32_t iIndex, int32_t iMaxNumCoeff,
									 const uint8_t *kpZigzagTable, int32_t iResidualProperty, int16_t *pTCoeff, int32_t iMbMode, uint8_t uiQp, PWelsDecoderContext pCtx)
{
	int32_t iLevel[16], iZerosLeft, iCoeffNum;
	int32_t  iRun[16] = {0};
	const uint8_t *kpBitNumMap;
	int32_t iCurNonZeroCacheIdx, i;
	const uint16_t *kpDequantCoeff = g_kuiDequantCoeff[uiQp];
	int8_t nA, nB, nC;
	uint8_t uiTotalCoeff, uiTrailingOnes;
	int32_t iUsedBits = 0;
	int32_t iCurIdx   = pBs->iIndex;
	uint8_t *pBuf     = ((uint8_t *)pBs->pStartBuf) + (iCurIdx >> 3);
	bool_t  bChromaDc = (CHROMA_DC == iResidualProperty);
	uint8_t bChroma   = (bChromaDc || CHROMA_AC == iResidualProperty);
	SReadBitsCache sReadBitsCache;

	sReadBitsCache.uiCache32Bit =  ((((pBuf[0]<<8) | pBuf[1]) << 16) | (pBuf[2]<<8) | pBuf[3]) << (iCurIdx&0x07);
	sReadBitsCache.uiRemainBits = 32 - (iCurIdx & 0x07);
    sReadBitsCache.pBuf = pBuf;
	//////////////////////////////////////////////////////////////////////////

	if (bChroma)
	{
		iCurNonZeroCacheIdx = g_kuiCacheNzcScanIdx[iIndex];
		nA = pNonZeroCountCache[iCurNonZeroCacheIdx-1];
		nB = pNonZeroCountCache[iCurNonZeroCacheIdx-8];

		if (bChromaDc)
		{
			kpBitNumMap = g_kuiTotalZerosBitNumChromaMap;
		}
		else
		{
			kpBitNumMap = g_kuiTotalZerosBitNumMap;
		}
	}
	else //luma
	{
		iCurNonZeroCacheIdx = g_kuiCacheNzcScanIdx[iIndex];
		nA = pNonZeroCountCache[iCurNonZeroCacheIdx-1];
		nB = pNonZeroCountCache[iCurNonZeroCacheIdx-8];

		kpBitNumMap = g_kuiTotalZerosBitNumMap;
	}

	WELS_NON_ZERO_COUNT_AVERAGE( nC, nA, nB );

	iUsedBits += CavlcGetTrailingOnesAndTotalCoeff(uiTotalCoeff, uiTrailingOnes, &sReadBitsCache, pVlcTable, bChromaDc, nC);

	if ( iResidualProperty != CHROMA_DC && iResidualProperty != I16_LUMA_DC)
	{
		pNonZeroCountCache[iCurNonZeroCacheIdx] = uiTotalCoeff;
		//////////////////////////////////////////////////////////////////////////
	}
	if (0 == uiTotalCoeff)
	{
		pBs->iIndex += iUsedBits;
		return 0;
	}
	if ( uiTrailingOnes > 3 || uiTotalCoeff > 16 ) /////////////////check uiTrailingOnes and uiTotalCoeff
	{
		return -1;
	}
	iUsedBits += CavlcGetLevelVal(iLevel, &sReadBitsCache, uiTotalCoeff, uiTrailingOnes);

	if (uiTotalCoeff < iMaxNumCoeff)
	{
	    iUsedBits += CavlcGetTotalZeros(iZerosLeft, &sReadBitsCache, uiTotalCoeff, pVlcTable, bChromaDc);
	}
	else
	{
		iZerosLeft = 0;
	}

	if (iZerosLeft < 0)
	{
		return ERR_INFO_CAVLC_INVALID_ZERO_LEFT;
	}
	iUsedBits += CavlcGetRunBefore(iRun, &sReadBitsCache, uiTotalCoeff, pVlcTable, iZerosLeft);

	pBs->iIndex += iUsedBits;
	iCoeffNum = -1;

	if(iResidualProperty == CHROMA_DC){
		//chroma dc scaling process, is kpDequantCoeff[0]? LevelScale(qPdc%6,0,0))<<(qPdc/6-6), the transform is done at construction.
			switch(iMbMode)
			{
			case BASE_MB:
				for(i=uiTotalCoeff-1; i>=0; --i)
				{ //FIXME merge into rundecode?
					int32_t j;
					iCoeffNum += iRun[i] + 1; //FIXME add 1 earlier ?
					j          = kpZigzagTable[ iCoeffNum ];
					pTCoeff[j] = iLevel[i]*kpDequantCoeff[0];
				}
				break;
			default:
				break;
			}
	}
	else if(iResidualProperty == I16_LUMA_DC){ //DC coefficent, only call in Intra_16x16, base_mode_flag = 0
		for(i=uiTotalCoeff-1; i>=0; --i){ //FIXME merge into rundecode?
			int32_t j;
			iCoeffNum += iRun[i] + 1; //FIXME add 1 earlier ?
			j          = kpZigzagTable[ iCoeffNum ];
			pTCoeff[j] = iLevel[i];
		}
	}
    else{
		switch(iMbMode)
		{
		case BASE_MB:
			for(i=uiTotalCoeff-1; i>=0; --i){ //FIXME merge into  rundecode?
				int32_t j;
				iCoeffNum += iRun[i] + 1; //FIXME add 1 earlier ?
				j          = kpZigzagTable[ iCoeffNum ];
				pTCoeff[j] = iLevel[i]*kpDequantCoeff[j & 0x07];
			}
			break;
		default:
			break;
		}
	}

	return 0;
}

int32_t ParseIntra4x4ModeConstrain0(PNeighAvail pNeighAvail, int8_t* pIntraPredMode, PBitStringAux pBs, PDqLayer pCurDqLayer)
{
	int32_t iSampleAvail[5*6] = { 0 }; //initialize as 0
	int32_t iMbXy = pCurDqLayer->iMbXyIndex;
	int32_t iFinalMode, i;

	uint8_t uiNeighAvail = 0;

	if ( pNeighAvail->iLeftAvail )  //left
	{
		iSampleAvail[ 6] =
		iSampleAvail[12] =
		iSampleAvail[18] =
		iSampleAvail[24] = 1;
	}
	if ( pNeighAvail->iLeftTopAvail ) //top_left
	{
		iSampleAvail[0] = 1;
	}
	if ( pNeighAvail->iTopAvail ) //top
	{
		iSampleAvail[1] =
		iSampleAvail[2] =
		iSampleAvail[3] =
		iSampleAvail[4] = 1;
	}
	if ( pNeighAvail->iRightTopAvail ) //top_right
	{
		iSampleAvail[5] = 1;
	}

	uiNeighAvail = (iSampleAvail[6]<<2) | (iSampleAvail[0]<<1) | (iSampleAvail[1]);

	for(i = 0; i < 16; i++)
	{
		const int32_t kiPrevIntra4x4PredMode = BsGetOneBit(pBs);//1bit
		const int32_t kiPredMode = PredIntra4x4Mode(pIntraPredMode, i);

		int8_t iBestMode;
		if (kiPrevIntra4x4PredMode)
		{
			iBestMode = kiPredMode;
		}
		else //kPrevIntra4x4PredMode == 0
		{
			const int32_t kiRemIntra4x4PredMode = BsGetBits(pBs, 3);//3bits
			if (kiRemIntra4x4PredMode < kiPredMode)
			{
				iBestMode = kiRemIntra4x4PredMode;
			}
			else
			{
				iBestMode = kiRemIntra4x4PredMode + 1;
			}
		}

		iFinalMode = CheckIntra4x4PredMode(&iSampleAvail[0], &iBestMode, i);
		if (iFinalMode < 0)
		{
			return ERR_INFO_INVALID_I4x4_PRED_MODE;
		}

		pCurDqLayer->pIntra4x4FinalMode[iMbXy][g_kuiScan4[i]] = iFinalMode;

		pIntraPredMode[g_kuiScan8[i]] = iBestMode;

		iSampleAvail[g_kuiCache30ScanIdx[i]] = 1;
	}
	ST32(&pCurDqLayer->pIntraPredMode[iMbXy][0], LD32(&pIntraPredMode[1 + 8 * 4]));
	pCurDqLayer->pIntraPredMode[iMbXy][4] = pIntraPredMode[4 + 8 * 1];
	pCurDqLayer->pIntraPredMode[iMbXy][5] = pIntraPredMode[4 + 8 * 2];
	pCurDqLayer->pIntraPredMode[iMbXy][6] = pIntraPredMode[4 + 8 * 3];
	pCurDqLayer->pChromaPredMode[iMbXy] = BsGetUe(pBs);
	if (-1 == pCurDqLayer->pChromaPredMode[iMbXy] || CheckIntraChromaPredMode(uiNeighAvail, &pCurDqLayer->pChromaPredMode[iMbXy]))
	{
		return ERR_INFO_INVALID_I_CHROMA_PRED_MODE;
	}

	return 0;
}

int32_t ParseIntra4x4ModeConstrain1(PNeighAvail pNeighAvail, int8_t* pIntraPredMode, PBitStringAux pBs, PDqLayer pCurDqLayer)
{
	int32_t iSampleAvail[5*6] = { 0 }; //initialize as 0
	int32_t iMbXy = pCurDqLayer->iMbXyIndex;
	int32_t iFinalMode, i;

	uint8_t uiNeighAvail = 0;

	if ( pNeighAvail->iLeftAvail && IS_INTRA( pNeighAvail->iLeftType ) )  //left
	{
		iSampleAvail[ 6] =
		iSampleAvail[12] =
		iSampleAvail[18] =
		iSampleAvail[24] = 1;
	}
	if ( pNeighAvail->iLeftTopAvail && IS_INTRA( pNeighAvail->iLeftTopType ) ) //top_left
	{
		iSampleAvail[0] = 1;
	}
	if ( pNeighAvail->iTopAvail && IS_INTRA( pNeighAvail->iTopType ) ) //top
	{
		iSampleAvail[1] =
		iSampleAvail[2] =
		iSampleAvail[3] =
		iSampleAvail[4] = 1;
	}
	if ( pNeighAvail->iRightTopAvail && IS_INTRA( pNeighAvail->iRightTopType ) ) //top_right
	{
		iSampleAvail[5] = 1;
	}

	uiNeighAvail = (iSampleAvail[6]<<2) | (iSampleAvail[0]<<1) | (iSampleAvail[1]);

	for(i = 0; i < 16; i++)
	{
		const int32_t kiPrevIntra4x4PredMode = BsGetOneBit(pBs);//1bit
		const int32_t kiPredMode = PredIntra4x4Mode(pIntraPredMode, i);

		int8_t iBestMode;
		if (kiPrevIntra4x4PredMode)
		{
			iBestMode = kiPredMode;
		}
		else //kPrevIntra4x4PredMode == 0
		{
			const int32_t kiRemIntra4x4PredMode = BsGetBits(pBs, 3);//3bits
			if (kiRemIntra4x4PredMode < kiPredMode)
			{
				iBestMode = kiRemIntra4x4PredMode;
			}
			else
			{
				iBestMode = kiRemIntra4x4PredMode + 1;
			}
		}

		iFinalMode = CheckIntra4x4PredMode(&iSampleAvail[0], &iBestMode, i);
		if (iFinalMode < 0)
		{
			return ERR_INFO_INVALID_I4x4_PRED_MODE;
		}

		pCurDqLayer->pIntra4x4FinalMode[iMbXy][g_kuiScan4[i]] = iFinalMode;

		pIntraPredMode[g_kuiScan8[i]] = iBestMode;

		iSampleAvail[g_kuiCache30ScanIdx[i]] = 1;
	}
	ST32(&pCurDqLayer->pIntraPredMode[iMbXy][0], LD32(&pIntraPredMode[1 + 8 * 4]));
	pCurDqLayer->pIntraPredMode[iMbXy][4] = pIntraPredMode[4 + 8 * 1];
	pCurDqLayer->pIntraPredMode[iMbXy][5] = pIntraPredMode[4 + 8 * 2];
	pCurDqLayer->pIntraPredMode[iMbXy][6] = pIntraPredMode[4 + 8 * 3];

	pCurDqLayer->pChromaPredMode[iMbXy] = BsGetUe(pBs);
	if (-1 == pCurDqLayer->pChromaPredMode[iMbXy] || CheckIntraChromaPredMode(uiNeighAvail, &pCurDqLayer->pChromaPredMode[iMbXy]))
	{
		return ERR_INFO_INVALID_I_CHROMA_PRED_MODE;
	}

	return 0;
}

int32_t ParseIntra16x16ModeConstrain0(PNeighAvail pNeighAvail, PBitStringAux pBs, PDqLayer pCurDqLayer)
{
	int32_t iMbXy = pCurDqLayer->iMbXyIndex;
	uint8_t uiNeighAvail = 0; //0x07 = 0 1 1 1, means left, top-left, top avail or not. (1: avail, 0: unavail)

	if ( pNeighAvail->iLeftAvail )
	{
		uiNeighAvail = (1<<2);
	}
	if ( pNeighAvail->iLeftTopAvail )
	{
		uiNeighAvail |= (1<<1);
	}
	if ( pNeighAvail->iTopAvail )
	{
		uiNeighAvail |= 1;
	}

	if (CheckIntra16x16PredMode(uiNeighAvail, &pCurDqLayer->pIntraPredMode[iMbXy][7])) //invalid iPredMode, must stop decoding
	{
		return ERR_INFO_INVALID_I16x16_PRED_MODE;
	}
	pCurDqLayer->pChromaPredMode[iMbXy] = BsGetUe(pBs);

	if (-1 == pCurDqLayer->pChromaPredMode[iMbXy] || CheckIntraChromaPredMode(uiNeighAvail, &pCurDqLayer->pChromaPredMode[iMbXy]))
	{
		return ERR_INFO_INVALID_I_CHROMA_PRED_MODE;
	}

	return 0;
}

int32_t ParseIntra16x16ModeConstrain1(PNeighAvail pNeighAvail, PBitStringAux pBs, PDqLayer pCurDqLayer)
{
	int32_t iMbXy = pCurDqLayer->iMbXyIndex;
	uint8_t uiNeighAvail = 0; //0x07 = 0 1 1 1, means left, top-left, top avail or not. (1: avail, 0: unavail)

	if ( pNeighAvail->iLeftAvail && IS_INTRA( pNeighAvail->iLeftType ) )
	{
		uiNeighAvail = (1<<2);
	}
	if ( pNeighAvail->iLeftTopAvail && IS_INTRA( pNeighAvail->iLeftTopType ) )
	{
		uiNeighAvail |= (1<<1);
	}
	if ( pNeighAvail->iTopAvail && IS_INTRA( pNeighAvail->iTopType ) )
	{
		uiNeighAvail |= 1;
	}

	if (CheckIntra16x16PredMode(uiNeighAvail, &pCurDqLayer->pIntraPredMode[iMbXy][7])) //invalid iPredMode, must stop decoding
	{
		return ERR_INFO_INVALID_I16x16_PRED_MODE;
	}
	pCurDqLayer->pChromaPredMode[iMbXy] = BsGetUe(pBs);

	if (-1 == pCurDqLayer->pChromaPredMode[iMbXy] || CheckIntraChromaPredMode(uiNeighAvail, &pCurDqLayer->pChromaPredMode[iMbXy]))
	{
		return ERR_INFO_INVALID_I_CHROMA_PRED_MODE;
	}

	return 0;
}

int32_t ParseInterInfo(PWelsDecoderContext pCtx, int16_t iMvArray[LIST_A][30][MV_A], int8_t iRefIdxArray[LIST_A][30], PBitStringAux pBs)
{
	PSlice pSlice				= &pCtx->pCurDqLayer->sLayerInfo.sSliceInLayer;
	PSliceHeader pSliceHeader	= &pSlice->sSliceHeaderExt.sSliceHeader;
	int32_t iNumRefFrames		= pSliceHeader->pSps->iNumRefFrames;
	int32_t iRefCount[2];
	PDqLayer pCurDqLayer = pCtx->pCurDqLayer;
	int32_t i, j;
	int32_t iMbXy = pCurDqLayer->iMbXyIndex;
	int32_t iMotionPredFlag[4];
	int16_t iMv[2] = {0};

	iMotionPredFlag[0] = iMotionPredFlag[1] = iMotionPredFlag[2] = iMotionPredFlag[3] = pSlice->sSliceHeaderExt.bDefaultMotionPredFlag;
	iRefCount[0] = pSliceHeader->uiRefCount[0];
	iRefCount[1] = pSliceHeader->uiRefCount[1];

	switch( pCurDqLayer->pMbType[iMbXy] )
	{
	case MB_TYPE_16x16:
		{
			int8_t iRefIdx = 0;
			if(pSlice->sSliceHeaderExt.bAdaptiveMotionPredFlag)
			{
				iMotionPredFlag[0] = BsGetOneBit(pBs);
			}
			if (iMotionPredFlag[0] == 0)
			{
				iRefIdx = BsGetTe0(pBs, iRefCount[0]);
				if (iRefIdx < 0 || iRefIdx >= iNumRefFrames) //error ref_idx
				{
					return ERR_INFO_INVALID_REF_INDEX;
				}
			}
			else
            {
                WelsLog( pCtx, WELS_LOG_WARNING, "inter parse: iMotionPredFlag = 1 not supported. \n" );
                return GENERATE_ERROR_NO(ERR_LEVEL_MB_DATA, ERR_INFO_UNSUPPORTED_ILP);
            }
			PredMv(iMvArray, iRefIdxArray, 0, 4, iRefIdx, iMv);

			iMv[0] += BsGetSe(pBs);
			iMv[1] += BsGetSe(pBs);

			UpdateP16x16MotionInfo(pCurDqLayer, iRefIdx, iMv);
		}
		break;
	case MB_TYPE_16x8:
        {
            int32_t iRefIdx[2];
		    for (i = 0; i < 2; i++)
		    {
			    if(pSlice->sSliceHeaderExt.bAdaptiveMotionPredFlag)
			    {
				    iMotionPredFlag[i] = BsGetOneBit(pBs);
			    }
		    }

		    for (i = 0; i < 2; i++)
		    {
                if( iMotionPredFlag[i] )
                {
                    WelsLog( pCtx, WELS_LOG_WARNING, "inter parse: iMotionPredFlag = 1 not supported. \n" );
                    return GENERATE_ERROR_NO(ERR_LEVEL_MB_DATA, ERR_INFO_UNSUPPORTED_ILP);
                }
			    iRefIdx[i] = BsGetTe0(pBs, iRefCount[0]);
			    if (iRefIdx[i] < 0 || iRefIdx[i] >= iNumRefFrames) //error ref_idx
			    {
				    return ERR_INFO_INVALID_REF_INDEX;
			    }
		    }
		    for (i = 0; i < 2; i++)
		    {
			    PredInter16x8Mv(iMvArray, iRefIdxArray, i<<3, iRefIdx[i], iMv);

			    iMv[0] += BsGetSe(pBs);
			    iMv[1] += BsGetSe(pBs);

			    UpdateP16x8MotionInfo(pCurDqLayer, iMvArray, iRefIdxArray, i<<3, iRefIdx[i], iMv);
		    }
        }
		break;
	case MB_TYPE_8x16:
        {
            int32_t iRefIdx[2];
		    for (i = 0; i < 2; i++)
		    {
			    if(pSlice->sSliceHeaderExt.bAdaptiveMotionPredFlag)
			    {
				    iMotionPredFlag[i] = BsGetOneBit(pBs);
			    }
		    }

		    for (i = 0; i < 2; i++)
		    {
			    if (iMotionPredFlag[i] == 0)
			    {
				    iRefIdx[i] = BsGetTe0(pBs, iRefCount[0]);
				    if (iRefIdx[i] < 0 || iRefIdx[i] >= iNumRefFrames) //error ref_idx
				    {
					    return ERR_INFO_INVALID_REF_INDEX;
				    }
			    }
			    else
			    {
                    WelsLog( pCtx, WELS_LOG_WARNING, "inter parse: iMotionPredFlag = 1 not supported. \n" );
                    return GENERATE_ERROR_NO(ERR_LEVEL_MB_DATA, ERR_INFO_UNSUPPORTED_ILP);
			    }

		    }
		    for (i = 0; i < 2; i++)
		    {
			    PredInter8x16Mv( iMvArray, iRefIdxArray, i<<2, iRefIdx[i], iMv);

			    iMv[0] += BsGetSe(pBs);
			    iMv[1] += BsGetSe(pBs);

			    UpdateP8x16MotionInfo(pCurDqLayer, iMvArray, iRefIdxArray, i<<2, iRefIdx[i], iMv);
		    }
        }
		break;
	case MB_TYPE_8x8:
	case MB_TYPE_8x8_REF0:
		{
			int8_t iRefIdx[4] = {0}, iSubPartCount[4], iPartWidth[4];
			uint32_t uiSubMbType;

			if ( MB_TYPE_8x8_REF0 == pCurDqLayer->pMbType[iMbXy])
			{
				iRefCount[0]	=
				iRefCount[1]	= 1;
			}

			//uiSubMbType, partition
			for (i = 0; i < 4; i++)
			{
				uiSubMbType = BsGetUe(pBs);
				if (uiSubMbType >= 4) //invalid uiSubMbType
				{
					return ERR_INFO_INVALID_SUB_MB_TYPE;
				}
				pCurDqLayer->pSubMbType[iMbXy][i] = g_ksInterSubMbTypeInfo[uiSubMbType].iType;
				iSubPartCount[i] = g_ksInterSubMbTypeInfo[uiSubMbType].iPartCount;
				iPartWidth[i] = g_ksInterSubMbTypeInfo[uiSubMbType].iPartWidth;
			}

			if(pSlice->sSliceHeaderExt.bAdaptiveMotionPredFlag)
			{
				for(i=0; i<4; i++)
				{
					iMotionPredFlag[i] = BsGetOneBit(pBs);
				}
			}

			//iRefIdxArray
			if (MB_TYPE_8x8_REF0 == pCurDqLayer->pMbType[iMbXy])
			{
				memset(pCurDqLayer->pRefIndex[0][iMbXy], 0, 16);
			}
			else
			{
				for (i = 0; i < 4; i++)
				{
					int16_t iIndex8 = i << 2;
					uint8_t uiScan4Idx = g_kuiScan4[iIndex8];

					if (iMotionPredFlag[i] == 0)
					{
						iRefIdx[i] = BsGetTe0(pBs, iRefCount[0]);
						if (iRefIdx[i] < 0 || iRefIdx[i] >= iNumRefFrames) //error ref_idx
						{
							return ERR_INFO_INVALID_REF_INDEX;
						}

						pCurDqLayer->pRefIndex[0][iMbXy][uiScan4Idx  ] = pCurDqLayer->pRefIndex[0][iMbXy][uiScan4Idx+1] =
						pCurDqLayer->pRefIndex[0][iMbXy][uiScan4Idx+4] = pCurDqLayer->pRefIndex[0][iMbXy][uiScan4Idx+5] = iRefIdx[i];
					}
					else
                    {
                        WelsLog( pCtx, WELS_LOG_WARNING, "inter parse: iMotionPredFlag = 1 not supported. \n" );
                        return GENERATE_ERROR_NO(ERR_LEVEL_MB_DATA, ERR_INFO_UNSUPPORTED_ILP);
                    }
				}
			}

			//gain mv and update mv cache
			for (i = 0; i < 4; i++)
			{
				int8_t iPartCount = iSubPartCount[i];
				uint32_t uiSubMbType = pCurDqLayer->pSubMbType[iMbXy][i];
				int16_t iMv[2], iPartIdx, iBlockWidth = iPartWidth[i], iIdx = i << 2;
				uint8_t uiScan4Idx, uiCacheIdx;

				uint8_t uiIdx4Cache = g_kuiCache30ScanIdx[iIdx];

				iRefIdxArray[0][uiIdx4Cache  ] = iRefIdxArray[0][uiIdx4Cache+1] =
				iRefIdxArray[0][uiIdx4Cache+6] = iRefIdxArray[0][uiIdx4Cache+7] = iRefIdx[i];

				for (j = 0; j < iPartCount; j++)
				{
					iPartIdx = iIdx + j * iBlockWidth;
					uiScan4Idx = g_kuiScan4[iPartIdx];
					uiCacheIdx = g_kuiCache30ScanIdx[iPartIdx];
					PredMv(iMvArray, iRefIdxArray, iPartIdx, iBlockWidth, iRefIdx[i], iMv);

					iMv[0] += BsGetSe(pBs);
					iMv[1] += BsGetSe(pBs);

					if (SUB_MB_TYPE_8x8 == uiSubMbType)
					{
						ST32(pCurDqLayer->pMv[0][iMbXy][uiScan4Idx], LD32(iMv));
						ST32(pCurDqLayer->pMv[0][iMbXy][uiScan4Idx+1], LD32(iMv));
						ST32(pCurDqLayer->pMv[0][iMbXy][uiScan4Idx+4], LD32(iMv));
						ST32(pCurDqLayer->pMv[0][iMbXy][uiScan4Idx+5], LD32(iMv));
						ST32(iMvArray[0][uiCacheIdx  ], LD32(iMv));
						ST32(iMvArray[0][uiCacheIdx+1], LD32(iMv));
						ST32(iMvArray[0][uiCacheIdx+6], LD32(iMv));
						ST32(iMvArray[0][uiCacheIdx+7], LD32(iMv));
					}
					else if (SUB_MB_TYPE_8x4 == uiSubMbType)
					{
						ST32(pCurDqLayer->pMv[0][iMbXy][uiScan4Idx  ], LD32(iMv));
						ST32(pCurDqLayer->pMv[0][iMbXy][uiScan4Idx+1], LD32(iMv));
						ST32(iMvArray[0][uiCacheIdx  ], LD32(iMv));
						ST32(iMvArray[0][uiCacheIdx+1], LD32(iMv));
					}
					else if (SUB_MB_TYPE_4x8 == uiSubMbType)
					{
						ST32(pCurDqLayer->pMv[0][iMbXy][uiScan4Idx  ], LD32(iMv));
						ST32(pCurDqLayer->pMv[0][iMbXy][uiScan4Idx+4], LD32(iMv));
						ST32(iMvArray[0][uiCacheIdx  ], LD32(iMv));
						ST32(iMvArray[0][uiCacheIdx+6], LD32(iMv));
					}
					else //SUB_MB_TYPE_4x4 == uiSubMbType
					{
						ST32(pCurDqLayer->pMv[0][iMbXy][uiScan4Idx  ], LD32(iMv));
						ST32(iMvArray[0][uiCacheIdx  ], LD32(iMv));
					}
				}
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

} // namespace WelsDec