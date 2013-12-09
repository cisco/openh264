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
 * \file	svc_encode_slice.c
 *
 * \brief	svc encoding slice 
 *
 * \date	2009.07.27 Created
 *
 *************************************************************************************
 */

#include <string.h>
#include <assert.h>
#include "ls_defines.h"
#include "svc_encode_slice.h"
#include "svc_enc_golomb.h"
#include "svc_base_layer_md.h"
#include "svc_encode_mb.h"
#include "mv_pred.h"
#include "svc_set_mb_syn_cavlc.h"
#include "encode_mb_aux.h"
#include "decode_mb_aux.h"
#include "svc_mode_decision.h"
#include "cpu_core.h"
#include "svc_motion_estimate.h"
#include "sample.h"
#include "wels_func_ptr_def.h"
#include "utils.h"

namespace WelsSVCEnc {
//#define ENC_TRACE
 
typedef void (*PWelsCodingSliceFunc)( sWelsEncCtx *pCtx, SSlice *pSlice );
typedef void (*PWelsSliceHeaderWriteFunc)( SBitStringAux* pBs, SDqLayer* pCurLayer, SSlice *pSlice, int32_t* pPpsIdDelta );

void UpdateNonZeroCountCache(SMB *pMb, SMbCache *pMbCache)
{
	ST32(&pMbCache->iNonZeroCoeffCount[9], LD32(&pMb->pNonZeroCount[ 0]));
	ST32(&pMbCache->iNonZeroCoeffCount[17], LD32(&pMb->pNonZeroCount[ 4]));
	ST32(&pMbCache->iNonZeroCoeffCount[25], LD32(&pMb->pNonZeroCount[ 8]));
	ST32(&pMbCache->iNonZeroCoeffCount[33], LD32(&pMb->pNonZeroCount[12]));	
	
	ST16(&pMbCache->iNonZeroCoeffCount[14], LD16(&pMb->pNonZeroCount[16]));
	ST16(&pMbCache->iNonZeroCoeffCount[38], LD16(&pMb->pNonZeroCount[18]));
	ST16(&pMbCache->iNonZeroCoeffCount[22], LD16(&pMb->pNonZeroCount[20]));
	ST16(&pMbCache->iNonZeroCoeffCount[46], LD16(&pMb->pNonZeroCount[22]));
}

void WelsSliceHeaderScalExtInit( SDqLayer* pCurLayer, SSlice *pSlice )
{
	SSliceHeaderExt* pSliceHeadExt	= &pSlice->sSliceHeaderExt;
	SNalUnitHeaderExt* pNalHeadExt= &pCurLayer->sLayerInfo.sNalHeaderExt;
	
	uint8_t uiDependencyId	= pNalHeadExt->uiDependencyId;

	pSliceHeadExt->bSliceSkipFlag = false;	

	if ( uiDependencyId > 0 ) //spatial EL
	{
		//bothe adaptive and default flags should equal to 0.
		pSliceHeadExt->bAdaptiveBaseModeFlag     = 
			pSliceHeadExt->bAdaptiveMotionPredFlag   = 
			pSliceHeadExt->bAdaptiveResidualPredFlag = false;

		pSliceHeadExt->bDefaultBaseModeFlag     = 
			pSliceHeadExt->bDefaultMotionPredFlag   =
			pSliceHeadExt->bDefaultResidualPredFlag = false;
	}
}

void WelsSliceHeaderExtInit( sWelsEncCtx* pEncCtx, SDqLayer* pCurLayer, SSlice *pSlice )
{
	SSliceHeaderExt* pCurSliceExt = &pSlice->sSliceHeaderExt;
	SSliceHeader* pCurSliceHeader  = &pCurSliceExt->sSliceHeader;	
	
	pCurSliceHeader->eSliceType	= pEncCtx->eSliceType;

	pCurSliceExt->bStoreRefBasePicFlag = false;	

	pCurSliceHeader->iFirstMbInSlice = WelsGetFirstMbOfSlice( pCurLayer->pSliceEncCtx, pSlice->uiSliceIdx );

	pCurSliceHeader->iFrameNum      = pEncCtx->iFrameNum;	
	pCurSliceHeader->uiIdrPicId     = pEncCtx->sPSOVector.uiIdrPicId; //??

	pCurSliceHeader->iPicOrderCntLsb          = pEncCtx->pEncPic->iFramePoc;	// 0

	if ( P_SLICE == pEncCtx->eSliceType  )
	{
		pCurSliceHeader->uiNumRefIdxL0Active	= 1;
		if ( pCurSliceHeader->uiRefCount > 0 && 
			pCurSliceHeader->uiRefCount < pCurLayer->sLayerInfo.pSpsP->iNumRefFrames )
		{
			pCurSliceHeader->bNumRefIdxActiveOverrideFlag = true;
			pCurSliceHeader->uiNumRefIdxL0Active	= pCurSliceHeader->uiRefCount;
		}
		//to solve mismatch between debug&release
		else
		{
			pCurSliceHeader->bNumRefIdxActiveOverrideFlag = false;
		}
	}

	pCurSliceHeader->iSliceQpDelta = pEncCtx->iGlobalQp - pCurLayer->sLayerInfo.pPpsP->iPicInitQp;

	//for deblocking initial
	pCurSliceHeader->uiDisableDeblockingFilterIdc			= pCurLayer->iLoopFilterDisableIdc;
	pCurSliceHeader->iSliceAlphaC0Offset					= pCurLayer->iLoopFilterAlphaC0Offset;	//	need update iSliceAlphaC0Offset & iSliceBetaOffset for pSlice-header if loop_filter_idc != 1
	pCurSliceHeader->iSliceBetaOffset						= pCurLayer->iLoopFilterBetaOffset;
	pCurSliceExt->uiDisableInterLayerDeblockingFilterIdc = pCurLayer->uiDisableInterLayerDeblockingFilterIdc;

	if ( pSlice->bSliceHeaderExtFlag )
	{
		WelsSliceHeaderScalExtInit( pCurLayer, pSlice );
	}
	else
	{
		//both adaptive and default flags should equal to 0.
		pCurSliceExt->bAdaptiveBaseModeFlag		= 
		pCurSliceExt->bAdaptiveMotionPredFlag		= 
		pCurSliceExt->bAdaptiveResidualPredFlag	= false;
		
		pCurSliceExt->bDefaultBaseModeFlag		= 
		pCurSliceExt->bDefaultMotionPredFlag		=
		pCurSliceExt->bDefaultResidualPredFlag	= false;
	}
}

/* count MB types if enabled FRAME_INFO_OUTPUT*/
#if defined(MB_TYPES_CHECK)
void WelsCountMbType(int32_t (*iMbCount)[18], const EWelsSliceType keSt, const SMB* kpMb)
{	
	if (NULL == iMbCount)
		return;
	
	switch( kpMb->uiMbType ) {
	case MB_TYPE_INTRA4x4:
		++ iMbCount[keSt][Intra4x4];
		break;
	case MB_TYPE_INTRA16x16:
		++ iMbCount[keSt][Intra16x16];
		break;
	case MB_TYPE_SKIP:
		++ iMbCount[keSt][PSkip];
		break;
	case MB_TYPE_16x16:
		++ iMbCount[keSt][Inter16x16];
		break;
	case MB_TYPE_16x8:
		++ iMbCount[keSt][Inter16x8];
		break;
	case MB_TYPE_8x16:
		++ iMbCount[eSt][Inter8x16];
		break;
	case MB_TYPE_8x8:
		++ iMbCount[keSt][Inter8x8];
		break;
	case MB_TYPE_INTRA_BL:
		++ iMbCount[keSt][7];
		break;
	default:
		break;
	}
}
#endif//MB_TYPES_CHECK

/*!
* \brief	write reference picture list on reordering syntax in Slice header	
*/
void WriteReferenceReorder( SBitStringAux *pBs, SSliceHeader *sSliceHeader )
{
	SRefPicListReorderSyntax *pRefOrdering	= &sSliceHeader->sRefReordering;
	uint8_t eSliceType						= sSliceHeader->eSliceType % 5;
	int16_t n = 0;

	if (  I_SLICE != eSliceType && SI_SLICE != eSliceType )	// !I && !SI
	{
		BsWriteOneBit( pBs, true );
//		{
			uint16_t uiReorderingOfPicNumsIdc;
			do 
			{
				uiReorderingOfPicNumsIdc = pRefOrdering->SReorderingSyntax[n].uiReorderingOfPicNumsIdc; 
				BsWriteUE( pBs, uiReorderingOfPicNumsIdc );
				if ( 0 == uiReorderingOfPicNumsIdc || 1 == uiReorderingOfPicNumsIdc )
					BsWriteUE( pBs, pRefOrdering->SReorderingSyntax[n].uiAbsDiffPicNumMinus1 );
				else if ( 2 == uiReorderingOfPicNumsIdc )
					BsWriteUE( pBs, pRefOrdering->SReorderingSyntax[n].iLongTermPicNum );

				n ++;
			} while ( 3 != uiReorderingOfPicNumsIdc );
//		}
	}
}

/*!
* \brief	write reference picture marking syntax in pSlice header	
*/
void WriteRefPicMarking( SBitStringAux *pBs, SSliceHeader *pSliceHeader, SNalUnitHeaderExt *pNalHdrExt )
{
	SRefPicMarking *sRefMarking	= &pSliceHeader->sRefMarking;
	int16_t n = 0;	

	if ( pNalHdrExt->bIdrFlag )
	{
		BsWriteOneBit( pBs, sRefMarking->bNoOutputOfPriorPicsFlag );
		BsWriteOneBit( pBs, sRefMarking->bLongTermRefFlag );
	}
	else 
	{
		BsWriteOneBit( pBs, sRefMarking->bAdaptiveRefPicMarkingModeFlag );

		if ( sRefMarking->bAdaptiveRefPicMarkingModeFlag )
		{
			int32_t iMmcoType;
			do 
			{
				iMmcoType = sRefMarking->SMmcoRef[n].iMmcoType;
				BsWriteUE( pBs, iMmcoType );
				if ( 1 == iMmcoType || 3 == iMmcoType )
					BsWriteUE( pBs, sRefMarking->SMmcoRef[n].iDiffOfPicNum - 1 );

				if ( 2 == iMmcoType )
					BsWriteUE( pBs, sRefMarking->SMmcoRef[n].iLongTermPicNum );

				if ( 3 == iMmcoType || 6 == iMmcoType )
					BsWriteUE( pBs, sRefMarking->SMmcoRef[n].iLongTermFrameIdx );

				if ( 4 == iMmcoType )
					BsWriteUE( pBs, sRefMarking->SMmcoRef[n].iMaxLongTermFrameIdx + 1 );

				n ++;
			} while ( 0 != iMmcoType );
		}

	}
}

void WelsSliceHeaderWrite( SBitStringAux* pBs, SDqLayer* pCurLayer, SSlice *pSlice, int32_t* pPpsIdDelta )
{
	SWelsSPS* pSps = pCurLayer->sLayerInfo.pSpsP;
	SWelsPPS* pPps = pCurLayer->sLayerInfo.pPpsP;
	SSliceHeader* pSliceHeader      = &pSlice->sSliceHeaderExt.sSliceHeader;	
	SNalUnitHeaderExt* pNalHead   = &pCurLayer->sLayerInfo.sNalHeaderExt;	

	BsWriteUE( pBs, pSliceHeader->iFirstMbInSlice );
	BsWriteUE( pBs, pSliceHeader->eSliceType );   /* same type things */

	BsWriteUE( pBs, pSliceHeader->pPps->iPpsId + pPpsIdDelta[pSliceHeader->pPps->iPpsId] );

	BsWriteBits( pBs, pSps->uiLog2MaxFrameNum, pSliceHeader->iFrameNum );

	if( pNalHead->bIdrFlag ) /* NAL IDR */
	{
		BsWriteUE( pBs, pSliceHeader->uiIdrPicId );
	}

	BsWriteBits( pBs, pSps->iLog2MaxPocLsb, pSliceHeader->iPicOrderCntLsb );

	if ( P_SLICE == pSliceHeader->eSliceType )
	{
		BsWriteOneBit( pBs, pSliceHeader->bNumRefIdxActiveOverrideFlag );
		if ( pSliceHeader->bNumRefIdxActiveOverrideFlag )
		{
			BsWriteUE( pBs, pSliceHeader->uiNumRefIdxL0Active - 1 );
		}
	}

	if ( !pNalHead->bIdrFlag )
		WriteReferenceReorder( pBs, pSliceHeader );

	if ( pNalHead->sNalHeader.uiNalRefIdc )
	{
		WriteRefPicMarking( pBs, pSliceHeader, pNalHead );
	}	

	BsWriteSE( pBs, pSliceHeader->iSliceQpDelta );      /* pSlice qp delta */

	if( pPps->bDeblockingFilterControlPresentFlag )
	{
		switch( pSliceHeader->uiDisableDeblockingFilterIdc )
		{
		case 0:
		case 3:
		case 4:
		case 6:
			BsWriteUE( pBs, 0 );
			break;
		case 1:
			BsWriteUE( pBs, 1 );
			break;
		case 2:
		case 5:
			BsWriteUE( pBs, 2 );
			break;
		default :
			fprintf( stderr, "pData error for deblocking" );
			break;
		}
		if ( 1 != pSliceHeader->uiDisableDeblockingFilterIdc )
		{
			BsWriteSE( pBs, pSliceHeader->iSliceAlphaC0Offset >> 1 );
			BsWriteSE( pBs, pSliceHeader->iSliceBetaOffset >> 1 );
		}
	}	
}

void WelsSliceHeaderExtWrite( SBitStringAux* pBs, SDqLayer* pCurLayer, SSlice *pSlice, int32_t *pPpsIdDelta )
{
	SWelsSPS* pSps           = pCurLayer->sLayerInfo.pSpsP;	
	SWelsPPS* pPps           = pCurLayer->sLayerInfo.pPpsP;
	SSubsetSps* pSubSps = pCurLayer->sLayerInfo.pSubsetSpsP;
	SSliceHeaderExt* pSliceHeadExt = &pSlice->sSliceHeaderExt;
	SSliceHeader* pSliceHeader      = &pSliceHeadExt->sSliceHeader;
	SNalUnitHeaderExt* pNalHead   = &pCurLayer->sLayerInfo.sNalHeaderExt;

	BsWriteUE( pBs, pSliceHeader->iFirstMbInSlice );
	BsWriteUE( pBs, pSliceHeader->eSliceType );   /* same type things */

	BsWriteUE( pBs, pSliceHeader->pPps->iPpsId + pPpsIdDelta[pSliceHeader->pPps->iPpsId] );

	BsWriteBits( pBs, pSps->uiLog2MaxFrameNum, pSliceHeader->iFrameNum );

	if( pNalHead->bIdrFlag ) /* NAL IDR */
	{
		BsWriteUE( pBs, pSliceHeader->uiIdrPicId );
	}

	BsWriteBits( pBs, pSps->iLog2MaxPocLsb, pSliceHeader->iPicOrderCntLsb );
//	{
		if ( P_SLICE == pSliceHeader->eSliceType )
		{
			BsWriteOneBit( pBs, pSliceHeader->bNumRefIdxActiveOverrideFlag );
			if ( pSliceHeader->bNumRefIdxActiveOverrideFlag )
			{
				BsWriteUE( pBs, pSliceHeader->uiNumRefIdxL0Active - 1 );
			}
		}

		if ( !pNalHead->bIdrFlag )
			WriteReferenceReorder( pBs, pSliceHeader );

		if ( pNalHead->sNalHeader.uiNalRefIdc )
		{
			WriteRefPicMarking( pBs, pSliceHeader, pNalHead );

			if ( !pSubSps->sSpsSvcExt.bSliceHeaderRestrictionFlag )
			{
				BsWriteOneBit( pBs, pSliceHeadExt->bStoreRefBasePicFlag );
			}
		}
//	}

	BsWriteSE( pBs, pSliceHeader->iSliceQpDelta );      /* pSlice qp delta */

	if( pPps->bDeblockingFilterControlPresentFlag )
	{
		BsWriteUE( pBs, pSliceHeader->uiDisableDeblockingFilterIdc );
		if ( 1 != pSliceHeader->uiDisableDeblockingFilterIdc )
		{
			BsWriteSE( pBs, pSliceHeader->iSliceAlphaC0Offset >> 1 );
			BsWriteSE( pBs, pSliceHeader->iSliceBetaOffset >> 1 );
		}
	}	

#if !defined(DISABLE_FMO_FEATURE)
	if ( pPps->uiNumSliceGroups > 1  &&
		pPps->uiSliceGroupMapType >= 3 && 
		pPps->uiSliceGroupMapType <= 5 )
	{
		int32_t iNumBits;
		if ( pPps->uiSliceGroupChangeRate )
		{
			iNumBits = WELS_CEILLOG2(1 + pPps->uiPicSizeInMapUnits / pPps->uiSliceGroupChangeRate);
			BsWriteBits( pBs, iNumBits, pSliceHeader->iSliceGroupChangeCycle );	
		}
	}
#endif//!DISABLE_FMO_FEATURE

	if ( false )
	{
		BsWriteOneBit( pBs, pSliceHeadExt->bSliceSkipFlag );
		if ( pSliceHeadExt->bSliceSkipFlag )
		{
			BsWriteUE( pBs, pSliceHeadExt->uiNumMbsInSlice - 1 );
		}
		else
		{
			BsWriteOneBit( pBs, pSliceHeadExt->bAdaptiveBaseModeFlag );
			if ( !pSliceHeadExt->bAdaptiveBaseModeFlag )  
			{
				BsWriteOneBit( pBs, pSliceHeadExt->bDefaultBaseModeFlag );
			}

			if ( !pSliceHeadExt->bDefaultBaseModeFlag )
			{
				BsWriteOneBit( pBs, 0 );
				BsWriteOneBit( pBs, 0 );
			}

			BsWriteOneBit( pBs, pSliceHeadExt->bAdaptiveResidualPredFlag );
			if ( !pSliceHeadExt->bAdaptiveResidualPredFlag )
			{
				BsWriteOneBit( pBs, 0);
			}
		}
		if ( 1 == pSubSps->sSpsSvcExt.bAdaptiveTcoeffLevelPredFlag )
		{
			BsWriteOneBit( pBs, pSliceHeadExt->bTcoeffLevelPredFlag );
		}

	}

	if ( !pSubSps->sSpsSvcExt.bSliceHeaderRestrictionFlag )
	{
		BsWriteBits( pBs, 4, 0 );
		BsWriteBits( pBs, 4, 15 );
	}
}

//only BaseLayer inter MB and SpatialLayer (uiQualityId = 0) inter MB calling this pFunc.
//only for inter part
void WelsInterMbEncode( sWelsEncCtx* pEncCtx, SSlice *pSlice, SMB* pCurMb )
{
	SMbCache* pMbCache = &pSlice->sMbCacheInfo;

	WelsDctMb(pMbCache->pCoeffLevel,  pMbCache->SPicData.pEncMb[0], pEncCtx->pCurDqLayer->iEncStride[0], pMbCache->pMemPredLuma, pEncCtx->pFuncList->pfDctFourT4 );
	WelsEncInterY( pEncCtx->pFuncList, pCurMb, pMbCache );
}


//only BaseLayer inter MB and SpatialLayer (uiQualityId = 0) inter MB calling this pFunc.
//only for I SSlice
void WelsIMbChromaEncode( sWelsEncCtx* pEncCtx, SMB* pCurMb, SMbCache *pMbCache )
{
	SWelsFuncPtrList *pFunc	= pEncCtx->pFuncList;
	SDqLayer* pCurLayer			= pEncCtx->pCurDqLayer;	
	const int32_t kiEncStride	= pCurLayer->iEncStride[1];
	const int32_t kiCsStride		= pCurLayer->iCsStride[1];
	int16_t *pCurRS				= pMbCache->pCoeffLevel;
	uint8_t* pBestPred			= pMbCache->pBestPredIntraChroma;
	uint8_t* pCsCb				= pMbCache->SPicData.pCsMb[1];
	uint8_t* pCsCr				= pMbCache->SPicData.pCsMb[2];

	//cb
	pFunc->pfDctFourT4( pCurRS,    pMbCache->SPicData.pEncMb[1], kiEncStride, pBestPred,    8);
	WelsEncRecUV( pFunc, pCurMb, pMbCache, pCurRS,    1 );
	pFunc->pfIDctFourT4( pCsCb, kiCsStride, pBestPred,    8, pCurRS    );
	
	//cr
	pFunc->pfDctFourT4( pCurRS+64, pMbCache->SPicData.pEncMb[2], kiEncStride, pBestPred+64, 8);
	WelsEncRecUV( pFunc, pCurMb, pMbCache, pCurRS+64, 2 );
	pFunc->pfIDctFourT4( pCsCr, kiCsStride, pBestPred+64, 8, pCurRS+64 );
}


//only BaseLayer inter MB and SpatialLayer (uiQualityId = 0) inter MB calling this pFunc.
//for P SSlice (intra part + inter part)
void WelsPMbChromaEncode( sWelsEncCtx* pEncCtx, SSlice *pSlice, SMB* pCurMb )
{
	SWelsFuncPtrList *pFunc	= pEncCtx->pFuncList;	
	SDqLayer* pCurLayer			= pEncCtx->pCurDqLayer;	
	const int32_t kiEncStride	= pCurLayer->iEncStride[1];
	SMbCache* pMbCache			= &pSlice->sMbCacheInfo;
	int16_t *pCurRS				= pMbCache->pCoeffLevel+256;
	uint8_t* pBestPred			= pMbCache->pMemPredChroma;		

	pFunc->pfDctFourT4(pCurRS,		pMbCache->SPicData.pEncMb[1],	kiEncStride,		pBestPred,		8);	
	pFunc->pfDctFourT4(pCurRS+64,	pMbCache->SPicData.pEncMb[2],	kiEncStride,		pBestPred+64,	8);	
	
	WelsEncRecUV(pFunc, pCurMb, pMbCache, pCurRS, 1);
	WelsEncRecUV(pFunc, pCurMb, pMbCache, pCurRS+64, 2);
}

void OutputPMbWithoutConstructCsRsNoCopy( sWelsEncCtx *pCtx, SDqLayer* pDq, SSlice *pSlice, SMB* pMb )
{	
	if ( IS_INTER( pMb->uiMbType ) || IS_I_BL(pMb->uiMbType) )		//intra have been reconstructed, NO COPY from CS to pDecPic--
	{
		SMbCache* pMbCache			= &pSlice->sMbCacheInfo;
		uint8_t* pDecY				= pMbCache->SPicData.pDecMb[0];
		uint8_t* pDecU				= pMbCache->SPicData.pDecMb[1];
		uint8_t* pDecV				= pMbCache->SPicData.pDecMb[2];
		int16_t *pScaledTcoeff		= pMbCache->pCoeffLevel;
		const int32_t kiDecStrideLuma	= pDq->pDecPic->iLineSize[0];
		const int32_t kiDecStrideChroma	= pDq->pDecPic->iLineSize[1];
		PIDctFunc pfIdctFour4x4				= pCtx->pFuncList->pfIDctFourT4;

		WelsIDctT4RecOnMb( pDecY, kiDecStrideLuma, pDecY, kiDecStrideLuma, pScaledTcoeff,  pfIdctFour4x4 );
		pfIdctFour4x4( pDecU, kiDecStrideChroma, pDecU, kiDecStrideChroma, pScaledTcoeff + 256 );
		pfIdctFour4x4( pDecV, kiDecStrideChroma, pDecV, kiDecStrideChroma, pScaledTcoeff + 320 );
	}
}

// for intra non-dynamic pSlice
//encapsulate two kinds of reconstruction:
//first. store base or highest Dependency Layer with only one quality (without CS RS reconstruction)
//second. lower than highest Dependency Layer, and for every Dependency Layer with one quality layer(single layer) 
void WelsISliceMdEnc( sWelsEncCtx* pEncCtx, SSlice *pSlice ) //pMd + encoding
{
	SDqLayer* pCurLayer				= pEncCtx->pCurDqLayer;
	SSliceCtx* pSliceCtx		= pCurLayer->pSliceEncCtx;
	SMbCache *pMbCache				= &pSlice->sMbCacheInfo;
	SSliceHeaderExt *pSliceHdExt	= &pSlice->sSliceHeaderExt;
	SMB* pMbList						= pCurLayer->sMbDataP;
	SMB* pCurMb						= NULL;	
	const int32_t kiSliceFirstMbXY	= pSliceHdExt->sSliceHeader.iFirstMbInSlice;
	int32_t iNextMbIdx				= kiSliceFirstMbXY;	
	const int32_t kiTotalNumMb		= pCurLayer->iMbWidth * pCurLayer->iMbHeight;
	int32_t iCurMbIdx				= 0, iNumMbCoded = 0;	
	const int32_t kiSliceIdx			= pSlice->uiSliceIdx;
	const uint8_t kuiChromaQpIndexOffset= pCurLayer->sLayerInfo.pPpsP->uiChromaQpIndexOffset;
	SWelsMD sMd;	
	
	for ( ; ; )
	{
		iCurMbIdx	= iNextMbIdx;
		pCurMb = &pMbList[ iCurMbIdx ];	
		pCurMb->uiLumaQp   = pEncCtx->iGlobalQp;
		pCurMb->uiChromaQp = g_kuiChromaQpTable[CLIP3_QP_0_51(pCurMb->uiLumaQp + kuiChromaQpIndexOffset)];

		pEncCtx->pFuncList->pfRc.pfWelsRcMbInit(pEncCtx, pCurMb, pSlice);		

		sMd.iLambda = g_kiQpCostTable[pCurMb->uiLumaQp];

		WelsMdIntraInit( pEncCtx, pCurMb, pMbCache, kiSliceFirstMbXY );
		WelsMdIntraMb( pEncCtx, &sMd, pCurMb, pMbCache );
		UpdateNonZeroCountCache( pCurMb, pMbCache );
		
		WelsSpatialWriteMbSyn( pEncCtx, pSlice, pCurMb );

		pCurMb->uiSliceIdc = kiSliceIdx;
		
        #if defined(MB_TYPES_CHECK) 
		WelsCountMbType( pEncCtx->sPerInfo.iMbCount, I_SLICE, pCurMb );		
        #endif//MB_TYPES_CHECK
	
		pEncCtx->pFuncList->pfRc.pfWelsRcMbInfoUpdate(pEncCtx,pCurMb,sMd.iCostLuma,pSlice);

		++iNumMbCoded;		

		iNextMbIdx = WelsGetNextMbOfSlice( pSliceCtx, iCurMbIdx );
		if ( iNextMbIdx == -1 || iNextMbIdx >= kiTotalNumMb || iNumMbCoded >= kiTotalNumMb )
		{
			break;
		}
	}
}

// Only for intra dynamic slicing
void WelsISliceMdEncDynamic( sWelsEncCtx* pEncCtx, SSlice *pSlice ) //pMd + encoding
{
	SBitStringAux* pBs				= pSlice->pSliceBsa;
	SDqLayer* pCurLayer				= pEncCtx->pCurDqLayer;
	SSliceCtx* pSliceCtx		= pCurLayer->pSliceEncCtx;
	SMbCache *pMbCache				= &pSlice->sMbCacheInfo;
	SSliceHeaderExt *pSliceHdExt	= &pSlice->sSliceHeaderExt;
	SMB* pMbList						= pCurLayer->sMbDataP;
	SMB* pCurMb						= NULL;	
	const int32_t kiSliceFirstMbXY	= pSliceHdExt->sSliceHeader.iFirstMbInSlice;
	int32_t iNextMbIdx				= kiSliceFirstMbXY;	
	const int32_t kiTotalNumMb		= pCurLayer->iMbWidth * pCurLayer->iMbHeight;
	int32_t iCurMbIdx				= 0, iNumMbCoded = 0;	
	const int32_t kiSliceIdx				= pSlice->uiSliceIdx;
	const int32_t kiPartitionId			= (kiSliceIdx % pEncCtx->iActiveThreadsNum);
	const uint8_t kuiChromaQpIndexOffset= pCurLayer->sLayerInfo.pPpsP->uiChromaQpIndexOffset;

	SWelsMD sMd;	
	SDynamicSlicingStack sDss;
	sDss.iStartPos = BsGetBitsPos(pBs);

	for ( ; ; )
	{
		iCurMbIdx	= iNextMbIdx;
		pCurMb = &pMbList[ iCurMbIdx ];	
		pCurMb->uiLumaQp   = pEncCtx->iGlobalQp;
		pCurMb->uiChromaQp = g_kuiChromaQpTable[CLIP3_QP_0_51(pCurMb->uiLumaQp + kuiChromaQpIndexOffset)];

		pEncCtx->pFuncList->pfRc.pfWelsRcMbInit(pEncCtx, pCurMb, pSlice);
		// if already reaches the largest number of slices, set QPs to the upper bound
		if (pSlice->bDynamicSlicingSliceSizeCtrlFlag)
		{			
			pCurMb->uiLumaQp = pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId].iMaxQp;
			pCurMb->uiChromaQp = g_kuiChromaQpTable[CLIP3_QP_0_51(pCurMb->uiLumaQp + kuiChromaQpIndexOffset)];
		}

		sMd.iLambda = g_kiQpCostTable[pCurMb->uiLumaQp];

		WelsMdIntraInit( pEncCtx, pCurMb, pMbCache, kiSliceFirstMbXY );
		WelsMdIntraMb( pEncCtx, &sMd, pCurMb, pMbCache );
		UpdateNonZeroCountCache( pCurMb, pMbCache );
		//stack pBs pointer
		sDss.pBsStackBufPtr	= pBs->pBufPtr;
		sDss.uiBsStackCurBits	= pBs->uiCurBits;
		sDss.iBsStackLeftBits	= pBs->iLeftBits;

		WelsSpatialWriteMbSyn( pEncCtx, pSlice, pCurMb );

		sDss.iCurrentPos = BsGetBitsPos(pBs);

		if ( DynSlcJudgeSliceBoundaryStepBack( pEncCtx, pSlice, pSliceCtx, pCurMb, &sDss ) )//islice
		{
			//stack pBs pointer
			pBs->pBufPtr		= sDss.pBsStackBufPtr;
			pBs->uiCurBits	= sDss.uiBsStackCurBits;
			pBs->iLeftBits	= sDss.iBsStackLeftBits;

			pCurLayer->pLastCodedMbIdxOfPartition[kiPartitionId] = iCurMbIdx-1;	// update pLastCodedMbIdxOfPartition, need to -1 due to stepping back
			++ pCurLayer->pNumSliceCodedOfPartition[kiPartitionId];

			break;
		}

		pCurMb->uiSliceIdc = kiSliceIdx;

#if defined(MB_TYPES_CHECK) 
		WelsCountMbType( pEncCtx->sPerInfo.iMbCount, I_SLICE, pCurMb );		
#endif//MB_TYPES_CHECK

		pEncCtx->pFuncList->pfRc.pfWelsRcMbInfoUpdate(pEncCtx,pCurMb,sMd.iCostLuma,pSlice);

		++iNumMbCoded;		

		iNextMbIdx = WelsGetNextMbOfSlice( pSliceCtx, iCurMbIdx );
		//whether all of MB in current pSlice encoded or not
		if ( iNextMbIdx == -1 || iNextMbIdx >= kiTotalNumMb || iNumMbCoded >= kiTotalNumMb )
		{
			pSliceCtx->pCountMbNumInSlice[kiSliceIdx]	= iCurMbIdx - pCurLayer->pLastCodedMbIdxOfPartition[kiPartitionId];
			pCurLayer->pLastCodedMbIdxOfPartition[kiPartitionId] = iCurMbIdx;	// update pLastCodedMbIdxOfPartition, finish coding, use iCurMbIdx directly
			break;
		}
	}
}

//encapsulate two kinds of reconstruction:
// first. store base or highest Dependency Layer with only one quality (without CS RS reconstruction)
// second. lower than highest Dependency Layer, and for every Dependency Layer with one quality layer(single layer) 
void WelsPSliceMdEnc( sWelsEncCtx* pEncCtx, SSlice *pSlice,  const bool_t kbIsHighestDlayerFlag ) //pMd + encoding
{
	const SSliceHeaderExt	*kpShExt				= &pSlice->sSliceHeaderExt;
	const SSliceHeader		*kpSh					= &kpShExt->sSliceHeader;
	const int32_t			kiSliceFirstMbXY	= kpSh->iFirstMbInSlice;
	SWelsMD sMd;

	sMd.uiRef			= kpSh->uiRefIndex;
	sMd.bMdUsingSad		= kbIsHighestDlayerFlag;
	if (!pEncCtx->pCurDqLayer->bBaseLayerAvailableFlag || !kbIsHighestDlayerFlag)
		memset( &sMd.sMe, 0, sizeof(sMd.sMe) );

	//pMb loop
	WelsMdInterMbLoop( pEncCtx, pSlice, &sMd, kiSliceFirstMbXY );
}

void WelsPSliceMdEncDynamic( sWelsEncCtx* pEncCtx, SSlice *pSlice, const bool_t kbIsHighestDlayerFlag )
{
	const SSliceHeaderExt	*kpShExt				= &pSlice->sSliceHeaderExt;
	const SSliceHeader		*kpSh					= &kpShExt->sSliceHeader;
	const int32_t			kiSliceFirstMbXY	= kpSh->iFirstMbInSlice;
	SWelsMD sMd;

	sMd.uiRef			= kpSh->uiRefIndex;
	sMd.bMdUsingSad		= kbIsHighestDlayerFlag;
	if (!pEncCtx->pCurDqLayer->bBaseLayerAvailableFlag || !kbIsHighestDlayerFlag)
		memset( &sMd.sMe, 0, sizeof(sMd.sMe) );

	//mb loop
	WelsMdInterMbLoopOverDynamicSlice( pEncCtx, pSlice, &sMd, kiSliceFirstMbXY );
}

void WelsCodePSlice( sWelsEncCtx* pEncCtx, SSlice *pSlice )
{
	//pSlice-level init should be outside and before this function
	SDqLayer* pCurLayer			= pEncCtx->pCurDqLayer;
	const bool_t kbBaseAvail		= pCurLayer->bBaseLayerAvailableFlag;
	const bool_t kbHighestSpatial= pEncCtx->pSvcParam->iNumDependencyLayer == (pCurLayer->sLayerInfo.sNalHeaderExt.uiDependencyId + 1);

	//MD switch	
	if ( kbBaseAvail && kbHighestSpatial ) 
	{
		//initial pMd pointer
		pEncCtx->pFuncList->pfInterMd			=  (PInterMdFunc)WelsMdInterMbEnhancelayer;
	}
	else
	{
		//initial pMd pointer
		pEncCtx->pFuncList->pfInterMd            =  (PInterMdFunc)WelsMdInterMb;
	}
	WelsPSliceMdEnc( pEncCtx, pSlice, kbHighestSpatial );
}

void WelsCodePOverDynamicSlice( sWelsEncCtx* pEncCtx, SSlice *pSlice )
{
	//pSlice-level init should be outside and before this function
	SDqLayer* pCurLayer			= pEncCtx->pCurDqLayer;
	const bool_t kbBaseAvail		= pCurLayer->bBaseLayerAvailableFlag;
	const bool_t kbHighestSpatial= pEncCtx->pSvcParam->iNumDependencyLayer == (pCurLayer->sLayerInfo.sNalHeaderExt.uiDependencyId + 1);

	//MD switch	
	if ( kbBaseAvail && kbHighestSpatial ) 
	{       	
		//initial pMd pointer
		pEncCtx->pFuncList->pfInterMd			=  (PInterMdFunc)WelsMdInterMbEnhancelayer;
	}
	else
	{
		//initial pMd pointer
		pEncCtx->pFuncList->pfInterMd            =  (PInterMdFunc)WelsMdInterMb;		
	}
	WelsPSliceMdEncDynamic( pEncCtx, pSlice, kbHighestSpatial );
}

// 1st index: 0: for P pSlice; 1: for I pSlice;
// 2nd index: 0: for non-dynamic pSlice; 1: for dynamic I pSlice;
PWelsCodingSliceFunc	g_pWelsSliceCoding[2][2] =
{
	{ WelsCodePSlice, WelsCodePOverDynamicSlice },	// P SSlice
	{ WelsISliceMdEnc, WelsISliceMdEncDynamic }	// I SSlice
};
PWelsSliceHeaderWriteFunc		g_pWelsWriteSliceHeader[2] =	// 0: for base; 1: for ext;
{
	WelsSliceHeaderWrite,
	WelsSliceHeaderExtWrite
};


void WelsCodeOneSlice( sWelsEncCtx* pEncCtx, const int32_t kiSliceIdx, const int32_t kiNalType )
{	
	SDqLayer* pCurLayer					= pEncCtx->pCurDqLayer;
	SNalUnitHeaderExt* pNalHeadExt	= &pCurLayer->sLayerInfo.sNalHeaderExt;
	SSlice *pCurSlice					= &pCurLayer->sLayerInfo.pSliceInLayer[kiSliceIdx];
	SBitStringAux* pBs					= pCurSlice->pSliceBsa;
	const int32_t kiDynamicSliceFlag	= (pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId].sMso.uiSliceMode == SM_DYN_SLICE);

	assert( kiSliceIdx == pCurSlice->uiSliceIdx );

	if ( I_SLICE == pEncCtx->eSliceType )
	{
		pNalHeadExt->bIdrFlag = 1;
		pCurSlice->sScaleShift = 0;
	}
	else
	{
		const uint32_t kuiTemporalId = pNalHeadExt->uiTemporalId;
		pCurSlice->sScaleShift = kuiTemporalId ? (kuiTemporalId - pEncCtx->pRefPic->uiTemporalId) : 0;
	}

	WelsSliceHeaderExtInit( pEncCtx, pCurLayer, pCurSlice );	


	g_pWelsWriteSliceHeader[pCurSlice->bSliceHeaderExtFlag]( pBs, pCurLayer, pCurSlice, &(pEncCtx->sPSOVector.sParaSetOffsetVariable[PARA_SET_TYPE_PPS].iParaSetIdDelta[0]) );
#if _DEBUG 
	if ( pEncCtx->sPSOVector.bEnableSpsPpsIdAddition )
	{
		const int32_t kiEncoderPpsId    = pCurSlice->sSliceHeaderExt.sSliceHeader.pPps->iPpsId;
		const int32_t kiTmpPpsIdInBs = kiEncoderPpsId + pEncCtx->sPSOVector.sParaSetOffsetVariable[PARA_SET_TYPE_PPS].iParaSetIdDelta[ kiEncoderPpsId ];
		assert ( MAX_PPS_COUNT > kiTmpPpsIdInBs );
		
		//when activated need to sure there is avialable PPS
		assert ( pEncCtx->sPSOVector.sParaSetOffsetVariable[PARA_SET_TYPE_PPS].bUsedParaSetIdInBs[kiTmpPpsIdInBs] );
	}
#endif

	pCurSlice->uiLastMbQp = pCurLayer->sLayerInfo.pPpsP->iPicInitQp + pCurSlice->sSliceHeaderExt.sSliceHeader.iSliceQpDelta;	

	g_pWelsSliceCoding[pNalHeadExt->bIdrFlag][kiDynamicSliceFlag]( pEncCtx, pCurSlice );

	BsRbspTrailingBits( pBs );

	BsFlush( pBs );
}

//pFunc: UpdateMbNeighbourInfoForNextSlice()
void UpdateMbNeighbourInfoForNextSlice(	SSliceCtx *pSliceCtx,
											 SMB *pMbList,
											 const int32_t kiFirstMbIdxOfNextSlice,
											 const int32_t kiLastMbIdxInPartition	)
{	
	const int32_t kiMbWidth					= pSliceCtx->iMbWidth;
	int32_t iIdx								= kiFirstMbIdxOfNextSlice;
	int32_t	iNextSliceFirstMbIdxRowStart= (( kiFirstMbIdxOfNextSlice % kiMbWidth ) ? 1:0);
	int32_t iCountMbUpdate					= kiMbWidth + iNextSliceFirstMbIdxRowStart; //need to update MB(iMbXY+1) to MB(iMbXY+1+row) in common case
	const int32_t kiEndMbNeedUpdate		= kiFirstMbIdxOfNextSlice + iCountMbUpdate;
	SMB *pMb									= &pMbList[iIdx];
	
	do {
        uint32_t uiNeighborAvailFlag	= 0;
		const int32_t kiMbXY				= pMb->iMbXY;
		const int32_t kiMbX				= pMb->iMbX;
		const int32_t kiMbY				= pMb->iMbY;
		BOOL_T     bLeft;
		BOOL_T     bTop;
		BOOL_T     bLeftTop;
		BOOL_T     bRightTop;		
		int32_t   iLeftXY, iTopXY, iLeftTopXY, iRightTopXY;
		const uint8_t  kuiSliceIdc		= WelsMbToSliceIdc(pSliceCtx, kiMbXY);
		
		pMb->uiSliceIdc	= kuiSliceIdc;
		iLeftXY = kiMbXY - 1;
		iTopXY = kiMbXY - kiMbWidth;
		iLeftTopXY = iTopXY - 1;
		iRightTopXY = iTopXY + 1;
		
		bLeft = (kiMbX > 0) && (kuiSliceIdc == WelsMbToSliceIdc(pSliceCtx, iLeftXY));
		bTop = (kiMbY > 0) && (kuiSliceIdc == WelsMbToSliceIdc(pSliceCtx, iTopXY));
		bLeftTop = (kiMbX > 0) && (kiMbY > 0) && (kuiSliceIdc == WelsMbToSliceIdc(pSliceCtx, iLeftTopXY));
		bRightTop = (kiMbX < (kiMbWidth-1)) && (kiMbY > 0) && (kuiSliceIdc == WelsMbToSliceIdc(pSliceCtx, iRightTopXY));		
		
		if( bLeft ){
			uiNeighborAvailFlag |= LEFT_MB_POS;
		}
		if( bTop ){
			uiNeighborAvailFlag |= TOP_MB_POS;
		}
		if( bLeftTop ){
			uiNeighborAvailFlag |= TOPLEFT_MB_POS;
		}
		if( bRightTop ){
			uiNeighborAvailFlag |= TOPRIGHT_MB_POS;
		}
		pMb->uiNeighborAvail	= (uint8_t)uiNeighborAvailFlag;
		
		++ pMb;
		++ iIdx;
	}while (	( iIdx < kiEndMbNeedUpdate) && 
				( iIdx <= kiLastMbIdxInPartition ) );
} 


void AddSliceBoundary(sWelsEncCtx* pEncCtx, SSlice * pCurSlice, SSliceCtx *pSliceCtx, SMB* pCurMb, int32_t iFirstMbIdxOfNextSlice, const int32_t kiLastMbIdxInPartition )
{
	SDqLayer*	pCurLayer = pEncCtx->pCurDqLayer;
	int32_t		iCurMbIdx		= pCurMb->iMbXY;
	int32_t		iCurSliceIdc	= pSliceCtx->pOverallMbMap[ iCurMbIdx ];
	const int32_t kiSliceIdxStep= pEncCtx->iActiveThreadsNum;
	int32_t		iNextSliceIdc	= iCurSliceIdc + kiSliceIdxStep;
	SSlice		*pNextSlice		= NULL;

	SMB *pMbList					= pCurLayer->sMbDataP;	

	//update cur pSlice info 	
	pCurSlice->sSliceHeaderExt.uiNumMbsInSlice	= 1 + iCurMbIdx - pCurSlice->sSliceHeaderExt.sSliceHeader.iFirstMbInSlice;
	
	//pNextSlice pointer/initialization
		pNextSlice = &( pCurLayer->sLayerInfo.pSliceInLayer[ iNextSliceIdc ] );

#if _DEBUG
	assert( NULL != pNextSlice );
	// now ( pSliceCtx->iSliceNumInFrame < pSliceCtx->iMaxSliceNumConstraint ) always true by the call of this pFunc
#endif

	//init next pSlice info
	pNextSlice->bSliceHeaderExtFlag = 
		(NAL_UNIT_CODED_SLICE_EXT == pCurLayer->sLayerInfo.sNalHeaderExt.sNalHeader.eNalUnitType);
	memcpy( &pNextSlice->sSliceHeaderExt, &pCurSlice->sSliceHeaderExt, sizeof(SSliceHeaderExt) );	// confirmed_safe_unsafe_usage

	pSliceCtx->pFirstMbInSlice[iNextSliceIdc] = iFirstMbIdxOfNextSlice;

#if !defined(MT_ENABLED)
	pNextSlice->uiSliceIdx = iNextSliceIdc;
	pNextSlice->pSliceBsa = &(pEncCtx->pOut->sBsWrite);
#endif//!MT_ENABLED

	memset(pSliceCtx->pOverallMbMap+iFirstMbIdxOfNextSlice, (uint8_t)iNextSliceIdc, (kiLastMbIdxInPartition-iFirstMbIdxOfNextSlice+1)*sizeof(uint8_t));

	//DYNAMIC_SLICING_ONE_THREAD: update pMbList slice_neighbor_info
	UpdateMbNeighbourInfoForNextSlice( pSliceCtx, pMbList, iFirstMbIdxOfNextSlice, kiLastMbIdxInPartition );
}

BOOL_T DynSlcJudgeSliceBoundaryStepBack(void* pCtx, void *pSlice, SSliceCtx *pSliceCtx, SMB* pCurMb, SDynamicSlicingStack* pDss )
{
	sWelsEncCtx *pEncCtx = (sWelsEncCtx*)pCtx;
	SSlice * pCurSlice = (SSlice *)pSlice;
	int32_t		   iCurMbIdx  = pCurMb->iMbXY;
	uint32_t        uiLen = 0;
	int32_t		   iPosBitOffset = 0;
	const int32_t  kiActiveThreadsNum = pEncCtx->iActiveThreadsNum;
	const int32_t  kiPartitaionId = pCurSlice->uiSliceIdx % kiActiveThreadsNum;
	const int32_t  kiLastMbIdxInPartition	= pEncCtx->pCurDqLayer->pLastMbIdxOfPartition[kiPartitaionId];

	const BOOL_T    kbCurMbNotFirstMbOfCurSlice      = (pSliceCtx->pOverallMbMap[iCurMbIdx] == pSliceCtx->pOverallMbMap[iCurMbIdx-1]);
	const BOOL_T    kbCurMbNotLastMbOfCurPartition = iCurMbIdx < kiLastMbIdxInPartition;
	const BOOL_T    kbSliceNumNotExceedConstraint       = pSliceCtx->iSliceNumInFrame < pSliceCtx->iMaxSliceNumConstraint; /*tmp choice to avoid complex memory operation, 100520, to be modify*/
	const BOOL_T    kbSliceNumReachConstraint               = (pSliceCtx->iSliceNumInFrame == pSliceCtx->iMaxSliceNumConstraint);

	if ( pCurSlice->bDynamicSlicingSliceSizeCtrlFlag ) 
		return false;

	iPosBitOffset = ( pDss->iCurrentPos - pDss->iStartPos );
#if _DEBUG
	assert(iPosBitOffset>=0);
#endif
	uiLen = ( ( iPosBitOffset>>3 ) + (( iPosBitOffset & 0x07 )? 1: 0) );	

#ifdef MT_ENABLED
	if ( pEncCtx->pSvcParam->iMultipleThreadIdc > 1 )
		WelsMutexLock( &pEncCtx->pSliceThreading->mutexSliceNumUpdate );
#endif//MT_ENABLED

	//DYNAMIC_SLICING_ONE_THREAD: judge jump_avoiding_pack_exceed
	if (
		( ( kbCurMbNotFirstMbOfCurSlice
		&& JUMPPACKETSIZE_JUDGE(uiLen,iCurMbIdx,pSliceCtx->uiSliceSizeConstraint) )/*jump_avoiding_pack_exceed*/ 
		&& kbCurMbNotLastMbOfCurPartition )//decide to add new pSlice
		&& ( kbSliceNumNotExceedConstraint
#ifdef MT_ENABLED
		&& ( ( pCurSlice->uiSliceIdx + kiActiveThreadsNum ) < pSliceCtx->iMaxSliceNumConstraint )
#endif//MT_ENABLED	
		)//able to add new pSlice

		)
	{	
		
		AddSliceBoundary( pEncCtx, pCurSlice, pSliceCtx, pCurMb, iCurMbIdx, kiLastMbIdxInPartition );

		++ pSliceCtx->iSliceNumInFrame;

#ifdef MT_ENABLED
		if (pEncCtx->pSvcParam->iMultipleThreadIdc > 1)
			WelsMutexUnlock( &pEncCtx->pSliceThreading->mutexSliceNumUpdate );
#endif//MT_ENABLED

		return TRUE;
	}

	if (
		( kbSliceNumReachConstraint
#ifdef MT_ENABLED
		|| ( ( pCurSlice->uiSliceIdx + kiActiveThreadsNum ) >= pSliceCtx->iMaxSliceNumConstraint )
#endif//MT_ENABLED
		)
		&& ( ( JUMPPACKETSIZE_JUDGE(uiLen,	iCurMbIdx,
		pSliceCtx->uiSliceSizeConstraint - ( ( kiLastMbIdxInPartition - iCurMbIdx ) << ( pCurSlice->uiAssumeLog2BytePerMb ) /* assume each MB consumes two byte under largest QP */) ) )
		&& kbCurMbNotLastMbOfCurPartition )//risk of exceeding the size constraint when pSlice num reaches constraint
		)
	{		
		pCurSlice->bDynamicSlicingSliceSizeCtrlFlag = true;
	}

#ifdef MT_ENABLED
	if (pEncCtx->pSvcParam->iMultipleThreadIdc > 1)
		WelsMutexUnlock( &pEncCtx->pSliceThreading->mutexSliceNumUpdate );
#endif//MT_ENABLED

	return FALSE;
}

///////////////
//  pMb loop
///////////////
// for inter non-dynamic pSlice
void WelsMdInterMbLoop( sWelsEncCtx* pEncCtx, SSlice *pSlice, void* pWelsMd, const int32_t kiSliceFirstMbXY )
{
	SWelsMD* pMd					= (SWelsMD*)pWelsMd;
	SBitStringAux* pBs			= pSlice->pSliceBsa;
	SDqLayer *pCurLayer			= pEncCtx->pCurDqLayer;
	SSliceCtx *pSliceCtx	= pCurLayer->pSliceEncCtx;
	SMbCache *pMbCache			= &pSlice->sMbCacheInfo;
	SMB *pMbList					= pCurLayer->sMbDataP;
	SMB *pCurMb					= NULL;
	int32_t iNumMbCoded		= 0;
	int32_t	iNextMbIdx			= kiSliceFirstMbXY;
	int32_t	iCurMbIdx			= -1;	
	int32_t	iMbSkipRun			= 0;
	const int32_t kiTotalNumMb	= pCurLayer->iMbWidth * pCurLayer->iMbHeight;
	const int32_t kiMvdInterTableSize	= (pEncCtx->pSvcParam->iNumDependencyLayer == 1 ? 648: 972);
	const int32_t kiMvdInterTableStride= 1+(kiMvdInterTableSize<<1);
	uint16_t *pMvdCostTableInter		= &pEncCtx->pMvdCostTableInter[kiMvdInterTableSize];
	const int32_t kiSliceIdx				= pSlice->uiSliceIdx;
	const uint8_t kuiChromaQpIndexOffset= pCurLayer->sLayerInfo.pPpsP->uiChromaQpIndexOffset;

	for(;;)
	{
		//point to current pMb
		iCurMbIdx	= iNextMbIdx;
		pCurMb = &pMbList[ iCurMbIdx ];		

		//step(1): set QP for the current MB
		pEncCtx->pFuncList->pfRc.pfWelsRcMbInit(pEncCtx, pCurMb, pSlice);
		
        //step (2). save some vale for future use, initial pWelsMd
		pMd->iLambda = g_kiQpCostTable[pCurMb->uiLumaQp];
		pMd->pMvdCost = &pMvdCostTableInter[pCurMb->uiLumaQp*kiMvdInterTableStride];
		WelsMdIntraInit(pEncCtx, pCurMb, pMbCache, kiSliceFirstMbXY);
        WelsMdInterInit(pEncCtx, pSlice, pCurMb, kiSliceFirstMbXY);
		pEncCtx->pFuncList->pfInterMd(pEncCtx, pMd, pSlice, pCurMb, pMbCache);
		//mb_qp

		//step (4): save from the MD process from future use
		WelsMdInterSaveSadAndRefMbType( (pCurLayer->pDecPic->uiRefMbType), pMbCache, pCurMb, pMd);

		pEncCtx->pFuncList->pfInterMdBackgroundInfoUpdate( pCurLayer, pCurMb, pMbCache->bCollocatedPredFlag, pEncCtx->pRefPic->iPictureType );

		//step (5): update cache
		UpdateNonZeroCountCache( pCurMb, pMbCache );

		//step (6): begin to write bit stream; if the pSlice size is controlled, the writing may be skipped
		if( IS_SKIP (pCurMb->uiMbType) )
		{
			pCurMb->uiLumaQp	= pSlice->uiLastMbQp;
			pCurMb->uiChromaQp = g_kuiChromaQpTable[CLIP3_QP_0_51(pCurMb->uiLumaQp + kuiChromaQpIndexOffset)];
			
			iMbSkipRun++;
		}
		else
		{
			BsWriteUE( pBs, iMbSkipRun );
			iMbSkipRun = 0;
			WelsSpatialWriteMbSyn( pEncCtx, pSlice, pCurMb );
		}
		
		//step (7): reconstruct current MB
		pCurMb->uiSliceIdc = kiSliceIdx;
		OutputPMbWithoutConstructCsRsNoCopy( pEncCtx, pCurLayer, pSlice, pCurMb );
		
        #if defined(MB_TYPES_CHECK) 
		WelsCountMbType( pEncCtx->sPerInfo.iMbCount, P_SLICE, pCurMb );		
        #endif//MB_TYPES_CHECK			

		//step (8): update status and other parameters
		pEncCtx->pFuncList->pfRc.pfWelsRcMbInfoUpdate(pEncCtx,pCurMb,pMd->iCostLuma,pSlice);
		
		/*judge if all pMb in cur pSlice has been encoded*/
		++ iNumMbCoded;
		iNextMbIdx = WelsGetNextMbOfSlice( pSliceCtx, iCurMbIdx );
		//whether all of MB in current pSlice encoded or not
		if ( iNextMbIdx == -1 || iNextMbIdx >= kiTotalNumMb || iNumMbCoded >= kiTotalNumMb )
		{
			break;
		}
	}

	if ( iMbSkipRun )
	{
		BsWriteUE( pBs, iMbSkipRun );
	}
}

// Only for inter dynamic slicing
void WelsMdInterMbLoopOverDynamicSlice( sWelsEncCtx* pEncCtx, SSlice *pSlice, void* pWelsMd, const int32_t kiSliceFirstMbXY )
{
	SWelsMD* pMd					= (SWelsMD*)pWelsMd;
	SBitStringAux* pBs			= pSlice->pSliceBsa;
	SDqLayer *pCurLayer			= pEncCtx->pCurDqLayer;
	SSliceCtx *pSliceCtx	= pCurLayer->pSliceEncCtx;
	SMbCache *pMbCache			= &pSlice->sMbCacheInfo;
	SMB *pMbList					= pCurLayer->sMbDataP;
	SMB *pCurMb					= NULL;
	int32_t iNumMbCoded		= 0;
	const int32_t kiTotalNumMb	= pCurLayer->iMbWidth * pCurLayer->iMbHeight;
	int32_t	iNextMbIdx			= kiSliceFirstMbXY;
	int32_t	iCurMbIdx			= -1;
	int32_t	iMbSkipRun			= 0;	
	const int32_t kiMvdInterTableSize	= (pEncCtx->pSvcParam->iNumDependencyLayer == 1 ? 648: 972);
	const int32_t kiMvdInterTableStride= 1+(kiMvdInterTableSize<<1);
	uint16_t *pMvdCostTableInter		= &pEncCtx->pMvdCostTableInter[kiMvdInterTableSize];
	const int32_t kiSliceIdx				= pSlice->uiSliceIdx;
	const int32_t kiPartitionId			= (kiSliceIdx % pEncCtx->iActiveThreadsNum);
	const uint8_t kuiChromaQpIndexOffset= pCurLayer->sLayerInfo.pPpsP->uiChromaQpIndexOffset;

	SDynamicSlicingStack sDss;
	sDss.iStartPos = BsGetBitsPos(pBs);
	for(;;)
	{
		//point to current pMb
		iCurMbIdx	= iNextMbIdx;
		pCurMb = &pMbList[ iCurMbIdx ];		

		//step(1): set QP for the current MB
		pEncCtx->pFuncList->pfRc.pfWelsRcMbInit(pEncCtx, pCurMb, pSlice);
		// if already reaches the largest number of slices, set QPs to the upper bound
		if (pSlice->bDynamicSlicingSliceSizeCtrlFlag)
		{
			//a clearer logic may be: 
			//if there is no need from size control from the pSlice size, the QP will be decided by RC; else it will be set to the max QP
			//    however, there are some parameter updating in the rc_mb_init() function, so it cannot be skipped?
			pCurMb->uiLumaQp = pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId].iMaxQp;
			pCurMb->uiChromaQp = g_kuiChromaQpTable[CLIP3_QP_0_51(pCurMb->uiLumaQp + kuiChromaQpIndexOffset)];
		}

		//step (2). save some vale for future use, initial pWelsMd
		pMd->iLambda = g_kiQpCostTable[pCurMb->uiLumaQp];
		pMd->pMvdCost = &pMvdCostTableInter[pCurMb->uiLumaQp*kiMvdInterTableStride];
		
		WelsMdIntraInit(pEncCtx, pCurMb, pMbCache, kiSliceFirstMbXY);
		WelsMdInterInit(pEncCtx, pSlice, pCurMb, kiSliceFirstMbXY);
		pEncCtx->pFuncList->pfInterMd(pEncCtx, pMd, pSlice, pCurMb, pMbCache);
		//mb_qp

		//step (4): save from the MD process from future use
		WelsMdInterSaveSadAndRefMbType( (pCurLayer->pDecPic->uiRefMbType), pMbCache, pCurMb, pMd);

		pEncCtx->pFuncList->pfInterMdBackgroundInfoUpdate( pCurLayer, pCurMb, pMbCache->bCollocatedPredFlag, pEncCtx->pRefPic->iPictureType );

		//step (5): update cache
		UpdateNonZeroCountCache( pCurMb, pMbCache );

		//step (6): begin to write bit stream; if the pSlice size is controlled, the writing may be skipped

		//DYNAMIC_SLICING_ONE_THREAD - MultiD
		//stack pBs pointer
		sDss.pBsStackBufPtr	= pBs->pBufPtr;
		sDss.uiBsStackCurBits	= pBs->uiCurBits;
		sDss.iBsStackLeftBits	= pBs->iLeftBits;
		//stack Pskip status
		sDss.iMbSkipRunStack = iMbSkipRun;
		//DYNAMIC_SLICING_ONE_THREAD - MultiD

		if( IS_SKIP (pCurMb->uiMbType) )
		{
			pCurMb->uiLumaQp	= pSlice->uiLastMbQp;
			pCurMb->uiChromaQp = g_kuiChromaQpTable[CLIP3_QP_0_51(pCurMb->uiLumaQp + kuiChromaQpIndexOffset)];

			iMbSkipRun++;
		}
		else
		{
			BsWriteUE( pBs, iMbSkipRun );
			iMbSkipRun = 0;
			WelsSpatialWriteMbSyn( pEncCtx, pSlice, pCurMb );
		}		

		//DYNAMIC_SLICING_ONE_THREAD - MultiD
		sDss.iCurrentPos = BsGetBitsPos(pBs);
		if ( DynSlcJudgeSliceBoundaryStepBack( pEncCtx, pSlice, pSliceCtx, pCurMb, &sDss ) )
		{
			//stack pBs pointer
			pBs->pBufPtr		= sDss.pBsStackBufPtr;
			pBs->uiCurBits	= sDss.uiBsStackCurBits;
			pBs->iLeftBits	= sDss.iBsStackLeftBits;

			iMbSkipRun = sDss.iMbSkipRunStack;

			pCurLayer->pLastCodedMbIdxOfPartition[kiPartitionId] = iCurMbIdx-1;	// update pLastCodedMbIdxOfPartition, need to -1 due to stepping back
			++ pCurLayer->pNumSliceCodedOfPartition[kiPartitionId];

			break;
		}

		//step (7): reconstruct current MB
		pCurMb->uiSliceIdc = kiSliceIdx;
		OutputPMbWithoutConstructCsRsNoCopy( pEncCtx, pCurLayer, pSlice, pCurMb );

#if defined(MB_TYPES_CHECK) 
		WelsCountMbType( pEncCtx->sPerInfo.iMbCount, P_SLICE, pCurMb );		
#endif//MB_TYPES_CHECK			

		//step (8): update status and other parameters
		pEncCtx->pFuncList->pfRc.pfWelsRcMbInfoUpdate(pEncCtx,pCurMb,pMd->iCostLuma,pSlice);

		/*judge if all pMb in cur pSlice has been encoded*/
		++ iNumMbCoded;
		iNextMbIdx = WelsGetNextMbOfSlice( pSliceCtx, iCurMbIdx );
		//whether all of MB in current pSlice encoded or not
		if ( iNextMbIdx == -1 || iNextMbIdx >= kiTotalNumMb || iNumMbCoded >= kiTotalNumMb )
		{
			pCurLayer->pLastCodedMbIdxOfPartition[kiPartitionId] = iCurMbIdx;	// update pLastCodedMbIdxOfPartition, finish coding, use pCurMb_idx directly				
			break;
		}
	}

	if ( iMbSkipRun )
	{
		BsWriteUE( pBs, iMbSkipRun );
	}
}

}//namespace WelsSVCEnc
