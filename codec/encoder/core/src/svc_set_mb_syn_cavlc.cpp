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
 * \file	svc_set_mb_syn_cavlc.h
 *
 * \brief	Seting all syntax elements of mb and decoding residual with cavlc
 *
 * \date	2009.8.12 Created 
 *
 *************************************************************************************
 */

#include "svc_enc_golomb.h"
#include "vlc_encoder.h"
#include "ls_defines.h"
#include "svc_set_mb_syn_cavlc.h"

namespace WelsSVCEnc {
const uint32_t g_kuiIntra4x4CbpMap[48] =
{
	3, 29, 30, 17, 31, 18, 37,  8, 32, 38, 19,  9, 20, 10, 11, 2, //15
	16, 33, 34, 21, 35, 22, 39,  4, 36, 40, 23,  5, 24,  6,  7, 1, //31
	41, 42, 43, 25, 44, 26, 46, 12, 45, 47, 27, 13, 28, 14, 15, 0  //47
};

const uint32_t g_kuiInterCbpMap[48] = 
{
	0,  2,  3,  7,  4,  8, 17, 13,  5, 18,  9, 14, 10, 15, 16, 11, //15
	1, 32, 33, 36, 34, 37, 44, 40, 35, 45, 38, 41, 39, 42, 43, 19, //31
	6, 24, 25, 20, 26, 21, 46, 28, 27, 47, 22, 29, 23, 30, 31, 12 //47
};	

//============================Enhance Layer CAVLC Writing===========================
void WelsSpatialWriteMbPred( sWelsEncCtx *pEncCtx, SSlice *pSlice, SMB *pCurMb )
{
	SMbCache* pMbCache	= &pSlice->sMbCacheInfo;
	SBitStringAux *pBs	= pSlice->pSliceBsa;
	SSliceHeaderExt* pSliceHeadExt = &pSlice->sSliceHeaderExt;
	int32_t iNumRefIdxl0ActiveMinus1 = pSliceHeadExt->sSliceHeader.uiNumRefIdxL0Active - 1;

	Mb_Type uiMbType = pCurMb->uiMbType;
	int32_t iCbpChroma = pCurMb->uiCbp >> 4;
	int32_t iCbpLuma   = pCurMb->uiCbp & 15;
	int32_t i = 0;

	SMVUnitXY sMvd[2];
    bool_t* pPredFlag;
    int8_t* pRemMode;

	int32_t iMbOffset = 0;

	switch( pSliceHeadExt->sSliceHeader.eSliceType )
    {
        case I_SLICE:
            iMbOffset = 0;
            break;
        case P_SLICE:
            iMbOffset = 5;
            break;
        default:
            return;
    }	

	switch ( uiMbType )
	{		
	case MB_TYPE_INTRA4x4:			
		/* mb type */
        BsWriteUE( pBs, iMbOffset + 0 );
		
        /* prediction: luma */
        pPredFlag = &pMbCache->pPrevIntra4x4PredModeFlag[0];
        pRemMode  = &pMbCache->pRemIntra4x4PredModeFlag[0];
		do
        {
            BsWriteOneBit( pBs, *pPredFlag );  /* b_prev_intra4x4_pred_mode */
 
			if ( !*pPredFlag )
            {
                BsWriteBits( pBs, 3, *pRemMode );
            }
			
			pPredFlag++;
			pRemMode++;
			++ i;
        } while (i < 16);

        /* prediction: chroma */		
		BsWriteUE( pBs, g_kiMapModeIntraChroma[pMbCache->uiChmaI8x8Mode] );

		break;

	case MB_TYPE_INTRA16x16:		
		/* mb type */
		BsWriteUE( pBs, 1 + iMbOffset + g_kiMapModeI16x16[pMbCache->uiLumaI16x16Mode] + (iCbpChroma << 2) + ( iCbpLuma == 0 ? 0 : 12 ) );
		
        /* prediction: chroma */		
		BsWriteUE( pBs, g_kiMapModeIntraChroma[pMbCache->uiChmaI8x8Mode] );

		break;

	case MB_TYPE_16x16:	
        BsWriteUE( pBs, 0 );//uiMbType
		sMvd[0].sDeltaMv(pCurMb->sMv[0], pMbCache->sMbMvp[0]);

		if ( iNumRefIdxl0ActiveMinus1 > 0 )
		{
			BsWriteTE( pBs, iNumRefIdxl0ActiveMinus1, pCurMb->pRefIndex[0] );
		}

		BsWriteSE( pBs, sMvd[0].iMvX);
		BsWriteSE( pBs, sMvd[0].iMvY);
		
		break;

	case MB_TYPE_16x8:
		BsWriteUE( pBs, 1 );//uiMbType
		
		sMvd[0].sDeltaMv(pCurMb->sMv[0], pMbCache->sMbMvp[0]);
		sMvd[1].sDeltaMv(pCurMb->sMv[8], pMbCache->sMbMvp[1]);

		if ( iNumRefIdxl0ActiveMinus1 > 0 )
		{
			BsWriteTE( pBs, iNumRefIdxl0ActiveMinus1, pCurMb->pRefIndex[0] );
			BsWriteTE( pBs, iNumRefIdxl0ActiveMinus1, pCurMb->pRefIndex[2] );
		}
		BsWriteSE( pBs, sMvd[0].iMvX );//block0
		BsWriteSE( pBs, sMvd[0].iMvY );
		BsWriteSE( pBs, sMvd[1].iMvX );//block1
		BsWriteSE( pBs, sMvd[1].iMvY );
		
		break;

	case MB_TYPE_8x16:		
		BsWriteUE( pBs, 2 );//uiMbType
		
		sMvd[0].sDeltaMv(pCurMb->sMv[0], pMbCache->sMbMvp[0]);
		sMvd[1].sDeltaMv(pCurMb->sMv[2], pMbCache->sMbMvp[1]);

		if ( iNumRefIdxl0ActiveMinus1 > 0 )
		{
			BsWriteTE( pBs, iNumRefIdxl0ActiveMinus1, pCurMb->pRefIndex[0] );
			BsWriteTE( pBs, iNumRefIdxl0ActiveMinus1, pCurMb->pRefIndex[1] );
		}
		BsWriteSE( pBs, sMvd[0].iMvX );//block0
		BsWriteSE( pBs, sMvd[0].iMvY );
		BsWriteSE( pBs, sMvd[1].iMvX );//block1
		BsWriteSE( pBs, sMvd[1].iMvY );
		
		break;
	}
}

void WelsSpatialWriteSubMbPred( sWelsEncCtx *pEncCtx, SSlice *pSlice, SMB *pCurMb )
{
	SMbCache* pMbCache	= &pSlice->sMbCacheInfo;
	SBitStringAux *pBs	= pSlice->pSliceBsa;
	SSliceHeaderExt* pSliceHeadExt = &pSlice->sSliceHeaderExt;

	int32_t iNumRefIdxl0ActiveMinus1 = pSliceHeadExt->sSliceHeader.uiNumRefIdxL0Active - 1;
	int32_t i;

	bool_t bSubRef0 = false;	
	const uint8_t* kpScan4 = &(g_kuiMbCountScan4Idx[0]);

	/* mb type */
	if ( LD32(pCurMb->pRefIndex) == 0 )
	{
		BsWriteUE( pBs, 4 );
		bSubRef0 = false;
	}
	else
	{
		BsWriteUE( pBs, 3 );
		bSubRef0 = true;
	}

	//step 1: sub_mb_type
	for ( i = 0; i < 4; i++ )
	{
		BsWriteUE( pBs, 0 );
	}

	//step 2: get and write uiRefIndex and sMvd
	if ( iNumRefIdxl0ActiveMinus1 > 0 && bSubRef0 ) 
	{
		BsWriteTE( pBs, iNumRefIdxl0ActiveMinus1, pCurMb->pRefIndex[0] );
		BsWriteTE( pBs, iNumRefIdxl0ActiveMinus1, pCurMb->pRefIndex[1] );
		BsWriteTE( pBs, iNumRefIdxl0ActiveMinus1, pCurMb->pRefIndex[2] );
		BsWriteTE( pBs, iNumRefIdxl0ActiveMinus1, pCurMb->pRefIndex[3] );
	}			
	//write sMvd
	for ( i = 0; i < 4; i++ )
	{
		BsWriteSE( pBs, pCurMb->sMv[*kpScan4].iMvX - pMbCache->sMbMvp[i].iMvX );
		BsWriteSE( pBs, pCurMb->sMv[*kpScan4].iMvY - pMbCache->sMbMvp[i].iMvY );
		kpScan4 += 4;
	}
}

//============================Base Layer CAVLC Writing===============================
void WelsSpatialWriteMbSyn( sWelsEncCtx *pEncCtx, SSlice *pSlice, SMB *pCurMb )
{
	SBitStringAux *pBs = pSlice->pSliceBsa;
	SMbCache* pMbCache = &pSlice->sMbCacheInfo;
	
	/* Step 1: write mb type and pred */
	if ( IS_Inter_8x8(pCurMb->uiMbType))
	{
		WelsSpatialWriteSubMbPred( pEncCtx, pSlice, pCurMb );
	}
	else
	{
		WelsSpatialWriteMbPred( pEncCtx, pSlice, pCurMb );
	}

	/* Step 2: write coded block patern */	
    if( IS_INTRA4x4 ( pCurMb->uiMbType ) )
    {
        BsWriteUE( pBs, g_kuiIntra4x4CbpMap[pCurMb->uiCbp] );
    }
    else if( !IS_INTRA16x16(pCurMb->uiMbType) )
    {
        BsWriteUE( pBs, g_kuiInterCbpMap[pCurMb->uiCbp] );
    }

	/* Step 3: write QP and residual */
	if( pCurMb->uiCbp > 0 || IS_INTRA16x16(pCurMb->uiMbType) )
	{
		const int32_t kiDeltaQp = pCurMb->uiLumaQp - pSlice->uiLastMbQp;
		pSlice->uiLastMbQp = pCurMb->uiLumaQp;		

        BsWriteSE( pBs, kiDeltaQp );		
		WelsWriteMbResidual( pMbCache, pCurMb, pBs );
	}
	else
	{
		pCurMb->uiLumaQp = pSlice->uiLastMbQp;
		pCurMb->uiChromaQp = g_kuiChromaQpTable[CLIP3_QP_0_51(pCurMb->uiLumaQp + pEncCtx->pCurDqLayer->sLayerInfo.pPpsP->uiChromaQpIndexOffset)];
	}
}

void WelsWriteMbResidual( SMbCache* sMbCacheInfo, SMB *pCurMb, SBitStringAux *pBs )
{
	int32_t i;
	Mb_Type uiMbType					= pCurMb->uiMbType;	
	const int32_t kiCbpChroma		= pCurMb->uiCbp >> 4;
	const int32_t kiCbpLuma			= pCurMb->uiCbp & 0x0F;
	int8_t *pNonZeroCoeffCount	= sMbCacheInfo->iNonZeroCoeffCount;
	int16_t *pBlock;
	int8_t iA, iB, iC;

	if ( IS_INTRA16x16(uiMbType) )
	{		
        /* DC luma */
		iA = pNonZeroCoeffCount[8];
		iB = pNonZeroCoeffCount[ 1];
		WELS_NON_ZERO_COUNT_AVERAGE(iC,iA,iB);
		WriteBlockResidualCavlc( sMbCacheInfo->pDct->iLumaI16x16Dc, 15, 1, LUMA_4x4, iC, pBs);

		/* AC Luma */
        if( kiCbpLuma )
        {
			pBlock = sMbCacheInfo->pDct->iLumaBlock[0];		
		
			for( i=0;i<16;i++ )
            {
				int32_t iIdx = g_kuiCache48CountScan4Idx[i];
				iA = pNonZeroCoeffCount[iIdx-1];
				iB = pNonZeroCoeffCount[iIdx-8];
				WELS_NON_ZERO_COUNT_AVERAGE(iC,iA,iB);
				WriteBlockResidualCavlc( pBlock, 14, pNonZeroCoeffCount[iIdx]>0, LUMA_AC, iC, pBs);
				pBlock += 16;				
            } 
        }		
	}
	else
	{
        /* Luma DC AC */
        if ( kiCbpLuma )
		{			
			pBlock = sMbCacheInfo->pDct->iLumaBlock[0];
			
			for( i=0; i<16; i+=4 )
            {
				if( kiCbpLuma & (1 << (i >> 2)) )
				{
					int32_t iIdx = g_kuiCache48CountScan4Idx[i];
					const int8_t kiA = pNonZeroCoeffCount[iIdx];
					const int8_t kiB = pNonZeroCoeffCount[iIdx+1];
					const int8_t kiC = pNonZeroCoeffCount[iIdx+8];
					const int8_t kiD = pNonZeroCoeffCount[iIdx+9];
					iA = pNonZeroCoeffCount[iIdx-1];
					iB = pNonZeroCoeffCount[iIdx-8];
					WELS_NON_ZERO_COUNT_AVERAGE(iC,iA,iB);
					WriteBlockResidualCavlc( pBlock, 15, kiA>0, LUMA_4x4, iC, pBs );

					iA = kiA;
					iB = pNonZeroCoeffCount[iIdx-7];
					WELS_NON_ZERO_COUNT_AVERAGE(iC,iA,iB);
					WriteBlockResidualCavlc( pBlock + 16, 15, kiB>0, LUMA_4x4, iC, pBs );

					iA = pNonZeroCoeffCount[iIdx+7];
					iB = kiA;
					WELS_NON_ZERO_COUNT_AVERAGE(iC,iA,iB);
					WriteBlockResidualCavlc( pBlock + 32, 15, kiC>0, LUMA_4x4, iC, pBs );

					iA = kiC;
					iB = kiB;
					WELS_NON_ZERO_COUNT_AVERAGE(iC,iA,iB);
					WriteBlockResidualCavlc( pBlock + 48, 15, kiD>0, LUMA_4x4, iC, pBs );
				}
				pBlock += 64;				
           } 
        }				
	}

    if( kiCbpChroma )
    {
        /* Chroma DC residual present */
		pBlock = sMbCacheInfo->pDct->iChromaDc[0]; // Cb
        WriteBlockResidualCavlc( pBlock, 3, 1, CHROMA_DC, CHROMA_DC_NC_OFFSET, pBs );
		
		pBlock += 4; // Cr
		WriteBlockResidualCavlc( pBlock, 3, 1, CHROMA_DC, CHROMA_DC_NC_OFFSET, pBs );
 
		/* Chroma AC residual present */
        if( kiCbpChroma & 0x02 ) 
        {
			const uint8_t *kCache48CountScan4Idx16base = &g_kuiCache48CountScan4Idx[16];
			pBlock = sMbCacheInfo->pDct->iChromaBlock[0]; // Cb
			
			for( i=0; i<4; i++ )
            {
				int32_t iIdx = kCache48CountScan4Idx16base[i];
				iA = pNonZeroCoeffCount[iIdx-1];
				iB = pNonZeroCoeffCount[iIdx-8];
				WELS_NON_ZERO_COUNT_AVERAGE(iC,iA,iB);
				WriteBlockResidualCavlc( pBlock, 14, pNonZeroCoeffCount[iIdx]>0, CHROMA_AC, iC, pBs );
				pBlock += 16;			
            }

			pBlock = sMbCacheInfo->pDct->iChromaBlock[4]; // Cr
		
			for( i=0;i<4;i++ )
            {
				int32_t iIdx = 24+kCache48CountScan4Idx16base[i];
				iA = pNonZeroCoeffCount[iIdx-1];
				iB = pNonZeroCoeffCount[iIdx-8];
				WELS_NON_ZERO_COUNT_AVERAGE(iC,iA,iB);
				WriteBlockResidualCavlc( pBlock, 14,pNonZeroCoeffCount[iIdx]>0, CHROMA_AC, iC, pBs );
				pBlock += 16;			
            }
        }
    }	
}

} // namespace WelsSVCEnc
