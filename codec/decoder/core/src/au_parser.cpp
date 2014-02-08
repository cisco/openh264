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
 * \file	au_parser.c
 *
 * \brief	Interfaces introduced in Access Unit level based parser
 *
 * \date	03/10/2009 Created
 *
 *************************************************************************************
 */
#include "codec_def.h"
#include "au_parser.h"
#include "decoder.h"
#include "error_code.h"
#include "memmgr_nal_unit.h"
#include "decoder_core.h"
#include "decoder_core.h"

namespace WelsDec {
/*!
 *************************************************************************************
 * \brief	Start Code Prefix (0x 00 00 00 01) detection
 *
 * \param 	pBuf		bitstream payload buffer
 * \param	pOffset		offset between NAL rbsp and original bitsteam that
 * 				start code prefix is seperated from.
 * \param	iBufSize	count size of buffer
 *
 * \return	RBSP buffer of start code prefix exclusive
 *
 * \note	N/A
 *************************************************************************************
 */
uint8_t* DetectStartCodePrefix (const uint8_t* kpBuf, int32_t* pOffset, int32_t iBufSize) {
  uint8_t* pBits = (uint8_t*)kpBuf;

  do {
    int32_t iIdx = 0;
    while ((iIdx < iBufSize) && (! (*pBits))) {
      ++ pBits;
      ++ iIdx;
    }
    if (iIdx >= iBufSize)  break;

    ++ iIdx;
    ++ pBits;

    if ((iIdx >= 3) && ((* (pBits - 1)) == 0x1)) {
      *pOffset = ((uintptr_t)pBits) - ((uintptr_t)kpBuf);
      return pBits;
    }

    iBufSize -= iIdx;
  }  while (1);

  return NULL;
}

/*!
 *************************************************************************************
 * \brief	to parse nal unit
 *
 * \param	pCtx		    decoder context
 * \param 	pNalUnitHeader	parsed result of NAL Unit Header to output
 * \param   pSrcRbsp        bitstream buffer to input
 * \param   iSrcRbspLen     length size of bitstream buffer payload
 * \param	pSrcNal
 * \param	iSrcNalLen
 * \param	pConsumedBytes	consumed bytes during parsing
 *
 * \return	decoded bytes payload, might be (pSrcRbsp+1) if no escapes
 *
 * \note	N/A
 *************************************************************************************
 */
uint8_t* ParseNalHeader (PWelsDecoderContext pCtx, SNalUnitHeader* pNalUnitHeader, uint8_t* pSrcRbsp,
                         int32_t iSrcRbspLen, uint8_t* pSrcNal, int32_t iSrcNalLen, int32_t* pConsumedBytes) {
  PNalUnit pCurNal = NULL;
  uint8_t* pNal     = pSrcRbsp;
  int32_t iNalSize  = iSrcRbspLen;
  PBitStringAux pBs = NULL;
  bool bExtensionFlag = false;
  int32_t iErr	= ERR_NONE;
  int32_t iBitSize = 0;

  pNalUnitHeader->eNalUnitType = NAL_UNIT_UNSPEC_0;//SHOULD init it. because pCtx->sCurNalHead is common variable.

  //remove the consecutive ZERO at the end of current NAL in the reverse order.--2011.6.1
  {
    int32_t iIndex = iSrcRbspLen - 1;
    uint8_t uiBsZero = 0;
    while (iIndex >= 0) {
      uiBsZero = pSrcRbsp[iIndex];
      if (0 == uiBsZero) {
        --iNalSize;
        --iIndex;
      } else {
        break;
      }
    }
  }

  pNalUnitHeader->uiForbiddenZeroBit	= (uint8_t) (pNal[0] >> 7);			// uiForbiddenZeroBit
  if (pNalUnitHeader->uiForbiddenZeroBit) { //2010.4.14
    return NULL; //uiForbiddenZeroBit should always equal to 0
  }

  pNalUnitHeader->uiNalRefIdc		= (uint8_t) (pNal[0] >> 5);			// uiNalRefIdc
  pNalUnitHeader->eNalUnitType		= (ENalUnitType) (pNal[0] & 0x1f);	// eNalUnitType

  ++pNal;
  --iNalSize;
  ++ (*pConsumedBytes);

#ifdef DEBUG_PARSE_INFO
  WelsLog (pCtx, WELS_LOG_INFO, "nal type: %d \n", pNalUnitHeader->eNalUnitType);
#endif

  if (! (IS_SEI_NAL (pNalUnitHeader->eNalUnitType) || IS_SPS_NAL (pNalUnitHeader->eNalUnitType)
         || pCtx->bSpsExistAheadFlag)) {
    WelsLog (pCtx, WELS_LOG_WARNING,
             "parse_nal(), no exist Sequence Parameter Sets ahead of sequence when try to decode NAL(type:%d).\n",
             pNalUnitHeader->eNalUnitType);
    pCtx->iErrorCode	= dsNoParamSets;
    return NULL;
  }
  if (! (IS_SEI_NAL (pNalUnitHeader->eNalUnitType) || IS_PARAM_SETS_NALS (pNalUnitHeader->eNalUnitType)
         || pCtx->bPpsExistAheadFlag)) {
    WelsLog (pCtx, WELS_LOG_WARNING,
             "parse_nal(), no exist Picture Parameter Sets ahead of sequence when try to decode NAL(type:%d).\n",
             pNalUnitHeader->eNalUnitType);
    pCtx->iErrorCode	= dsNoParamSets;
    return NULL;
  }
  if ((IS_VCL_NAL_AVC_BASE (pNalUnitHeader->eNalUnitType) && ! (pCtx->bSpsExistAheadFlag || pCtx->bPpsExistAheadFlag)) ||
      (IS_NEW_INTRODUCED_NAL (pNalUnitHeader->eNalUnitType) && ! (pCtx->bSpsExistAheadFlag || pCtx->bSubspsExistAheadFlag
          || pCtx->bPpsExistAheadFlag))) {
    WelsLog (pCtx, WELS_LOG_WARNING,
             "ParseNalHeader(), no exist Parameter Sets ahead of sequence when try to decode slice(type:%d).\n",
             pNalUnitHeader->eNalUnitType);
    pCtx->iErrorCode	|= dsNoParamSets;
    return NULL;
  }


  switch (pNalUnitHeader->eNalUnitType) {
  case NAL_UNIT_SEI:

    if (pCtx->pAccessUnitList->uiAvailUnitsNum > 0) {
      pCtx->pAccessUnitList->uiEndPos = pCtx->pAccessUnitList->uiAvailUnitsNum - 1;
      pCtx->bAuReadyFlag = true;
    }

    break;

  case NAL_UNIT_SPS:

    if (pCtx->pAccessUnitList->uiAvailUnitsNum > 0) {
      pCtx->pAccessUnitList->uiEndPos = pCtx->pAccessUnitList->uiAvailUnitsNum - 1;
      pCtx->bAuReadyFlag = true;
    }

    break;

  case NAL_UNIT_PREFIX:
    pCurNal = &pCtx->sPrefixNal;

    if (iNalSize < NAL_UNIT_HEADER_EXT_SIZE) {
      pCtx->iErrorCode |= dsBitstreamError;

      PAccessUnit pCurAu	   = pCtx->pAccessUnitList;
      uint32_t uiAvailNalNum = pCurAu->uiAvailUnitsNum;
      ForceClearCurrentNal (pCurAu);

      if (uiAvailNalNum > 1) {
        pCurAu->uiEndPos = uiAvailNalNum - 2;
        pCtx->bAuReadyFlag = true;
      }
      return NULL;
    }

    DecodeNalHeaderExt (pCurNal, pNal);
    if ((pCurNal->sNalHeaderExt.uiQualityId != 0) || (pCurNal->sNalHeaderExt.bUseRefBasePicFlag != 0)) {
      WelsLog (pCtx, WELS_LOG_WARNING,
               "ParseNalHeader() in Prefix Nal Unit:uiQualityId (%d) != 0, bUseRefBasePicFlag (%d) != 0, not supported!\n",
               pCurNal->sNalHeaderExt.uiQualityId, pCurNal->sNalHeaderExt.bUseRefBasePicFlag);
      PAccessUnit pCurAu	   = pCtx->pAccessUnitList;
      uint32_t uiAvailNalNum = pCurAu->uiAvailUnitsNum;
      ForceClearCurrentNal (pCurAu);

      if (uiAvailNalNum > 1) {
        pCurAu->uiEndPos = uiAvailNalNum - 2;
        pCtx->bAuReadyFlag = true;
      }
      pCtx->iErrorCode |= dsInvalidArgument;
      return NULL;
    }

    pNal            += NAL_UNIT_HEADER_EXT_SIZE;
    iNalSize        -= NAL_UNIT_HEADER_EXT_SIZE;
    *pConsumedBytes += NAL_UNIT_HEADER_EXT_SIZE;

    pCurNal->sNalHeaderExt.sNalUnitHeader.uiForbiddenZeroBit = pNalUnitHeader->uiForbiddenZeroBit;
    pCurNal->sNalHeaderExt.sNalUnitHeader.uiNalRefIdc		  = pNalUnitHeader->uiNalRefIdc;
    pCurNal->sNalHeaderExt.sNalUnitHeader.eNalUnitType	      = pNalUnitHeader->eNalUnitType;

    pBs = &pCtx->sBs;

    iBitSize = (iNalSize << 3) - BsGetTrailingBits (pNal + iNalSize - 1); // convert into bit

    InitBits (pBs, pNal, iBitSize);

    ParsePrefixNalUnit (pCtx, pBs);

    break;
  case NAL_UNIT_CODED_SLICE_EXT:
    bExtensionFlag = true;
  case NAL_UNIT_CODED_SLICE:
  case NAL_UNIT_CODED_SLICE_IDR: {
    PAccessUnit pCurAu		= NULL;
    uint32_t uiAvailNalNum;
    pCurNal = MemGetNextNal (&pCtx->pAccessUnitList);
    if (NULL == pCurNal) {
      WelsLog (pCtx, WELS_LOG_WARNING, "MemGetNextNal() fail due out of memory.\n");
      pCtx->iErrorCode	|= dsOutOfMemory;
      return NULL;
    }

    pCurNal->sNalHeaderExt.sNalUnitHeader.uiForbiddenZeroBit = pNalUnitHeader->uiForbiddenZeroBit;
    pCurNal->sNalHeaderExt.sNalUnitHeader.uiNalRefIdc		  = pNalUnitHeader->uiNalRefIdc;
    pCurNal->sNalHeaderExt.sNalUnitHeader.eNalUnitType	  = pNalUnitHeader->eNalUnitType;
    pCurAu	      = pCtx->pAccessUnitList;
    uiAvailNalNum = pCurAu->uiAvailUnitsNum;


    if (pNalUnitHeader->eNalUnitType == NAL_UNIT_CODED_SLICE_EXT) {
      if (iNalSize < NAL_UNIT_HEADER_EXT_SIZE) {
        pCtx->iErrorCode |= dsBitstreamError;

        ForceClearCurrentNal (pCurAu);

        if (uiAvailNalNum > 1) {
          pCurAu->uiEndPos = uiAvailNalNum - 2;
          pCtx->bAuReadyFlag = true;
        }
        return NULL;
      }

      DecodeNalHeaderExt (pCurNal, pNal);
      if (pCurNal->sNalHeaderExt.uiQualityId != 0 ||
          pCurNal->sNalHeaderExt.bUseRefBasePicFlag) {
        if (pCurNal->sNalHeaderExt.uiQualityId != 0)
          WelsLog (pCtx, WELS_LOG_WARNING, "ParseNalHeader():uiQualityId (%d) != 0, MGS not supported!\n",
                   pCurNal->sNalHeaderExt.uiQualityId);
        if (pCurNal->sNalHeaderExt.bUseRefBasePicFlag != 0)
          WelsLog (pCtx, WELS_LOG_WARNING, "ParseNalHeader():bUseRefBasePicFlag (%d) != 0, MGS not supported!\n",
                   pCurNal->sNalHeaderExt.bUseRefBasePicFlag);

        pCtx->iErrorCode |= dsInvalidArgument;
        ForceClearCurrentNal (pCurAu);

        if (uiAvailNalNum > 1) {
          pCurAu->uiEndPos = uiAvailNalNum - 2;
          pCtx->bAuReadyFlag = true;
        }
        return NULL;
      }
      pNal            += NAL_UNIT_HEADER_EXT_SIZE;
      iNalSize        -= NAL_UNIT_HEADER_EXT_SIZE;
      *pConsumedBytes += NAL_UNIT_HEADER_EXT_SIZE;

    } else {


      if (NAL_UNIT_PREFIX == pCtx->sPrefixNal.sNalHeaderExt.sNalUnitHeader.eNalUnitType) {
        PrefetchNalHeaderExtSyntax (pCtx, pCurNal, &pCtx->sPrefixNal);
      }

      pCurNal->sNalHeaderExt.bIdrFlag = (NAL_UNIT_CODED_SLICE_IDR == pNalUnitHeader->eNalUnitType) ? true :
                                        false;   //SHOULD update this flag for AVC if no prefix NAL
      pCurNal->sNalHeaderExt.iNoInterLayerPredFlag = 1;
    }

    pBs = &pCurAu->pNalUnitsList[uiAvailNalNum - 1]->sNalData.sVclNal.sSliceBitsRead;
    iBitSize = (iNalSize << 3) - BsGetTrailingBits (pNal + iNalSize - 1); // convert into bit
    InitBits (pBs, pNal, iBitSize);
    iErr = ParseSliceHeaderSyntaxs (pCtx, pBs, bExtensionFlag);
    if (iErr != ERR_NONE) {
      //if current NAL occur error when parsing, should clean it from pNalUnitsList
      //otherwise, when Next good NAL decoding, this corrupt NAL is considered as normal NAL and lead to decoder crash
      ForceClearCurrentNal (pCurAu);

      if (uiAvailNalNum > 1) {
        pCurAu->uiEndPos = uiAvailNalNum - 2;
        pCtx->bAuReadyFlag = true;


      }
#ifdef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
      if (dsNoParamSets & pCtx->iErrorCode) {
        if (uiAvailNalNum <= 1) { //no any data to decode and SPS/PPS ID mismatch, SHOULD request IDR
#ifdef LONG_TERM_REF
          pCtx->bParamSetsLostFlag = true;
#else
          pCtx->bReferenceLostAtT0Flag = true;
#endif
          ResetParameterSetsState (pCtx);
        }
        return NULL;
      } else {
        return NULL;
      }
#else
      return NULL;
#endif //MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
    }

    if ((uiAvailNalNum > 1) &&
        CheckAccessUnitBoundary (pCurAu->pNalUnitsList[uiAvailNalNum - 1], pCurAu->pNalUnitsList[uiAvailNalNum - 2],
                                 pCurAu->pNalUnitsList[uiAvailNalNum - 1]->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader.pSps)) {
      pCurAu->uiEndPos = uiAvailNalNum - 2;
      pCtx->bAuReadyFlag = true;


    }
  }
  break;
  default:
    break;
  }

  return pNal;
}


bool CheckAccessUnitBoundaryExt (PNalUnitHeaderExt pLastNalHdrExt, PNalUnitHeaderExt pCurNalHeaderExt,
                                   PSliceHeader pLastSliceHeader, PSliceHeader pCurSliceHeader) {
  const PSps kpSps = pCurSliceHeader->pSps;

  //Sub-clause 7.1.4.1.1 temporal_id
  if (pLastNalHdrExt->uiTemporalId != pCurNalHeaderExt->uiTemporalId) {
    return TRUE;
  }

  // Subclause 7.4.1.2.5
  if (pLastSliceHeader->iRedundantPicCnt < pCurSliceHeader->iRedundantPicCnt)
    return FALSE;
  else if (pLastSliceHeader->iRedundantPicCnt > pCurSliceHeader->iRedundantPicCnt)
    return TRUE;

  // Subclause G7.4.1.2.4
  if (pLastNalHdrExt->uiDependencyId < pCurNalHeaderExt->uiDependencyId)
    return FALSE;
  else if (pLastNalHdrExt->uiDependencyId > pCurNalHeaderExt->uiDependencyId)
    return TRUE;
  if (pLastNalHdrExt->uiQualityId < pCurNalHeaderExt->uiQualityId)
    return FALSE;
  else if (pLastNalHdrExt->uiQualityId > pCurNalHeaderExt->uiQualityId)
    return TRUE;

  // Subclause 7.4.1.2.4
  if (pLastSliceHeader->iFrameNum != pCurSliceHeader->iFrameNum)
    return TRUE;
  if (pLastSliceHeader->iPpsId != pCurSliceHeader->iPpsId)
    return TRUE;
  if (pLastSliceHeader->bFieldPicFlag != pCurSliceHeader->bFieldPicFlag)
    return TRUE;
  if (pLastSliceHeader->bBottomFiledFlag != pCurSliceHeader->bBottomFiledFlag)
    return TRUE;
  if ((pLastNalHdrExt->sNalUnitHeader.uiNalRefIdc != NRI_PRI_LOWEST) != (pCurNalHeaderExt->sNalUnitHeader.uiNalRefIdc !=
      NRI_PRI_LOWEST))
    return TRUE;
  if (pLastNalHdrExt->bIdrFlag != pCurNalHeaderExt->bIdrFlag)
    return TRUE;
  if (pCurNalHeaderExt->bIdrFlag) {
    if (pLastSliceHeader->uiIdrPicId != pCurSliceHeader->uiIdrPicId)
      return TRUE;
  }
  if (kpSps->uiPocType == 0) {
    if (pLastSliceHeader->iPicOrderCntLsb != pCurSliceHeader->iPicOrderCntLsb)
      return TRUE;
    if (pLastSliceHeader->iDeltaPicOrderCntBottom != pCurSliceHeader->iDeltaPicOrderCntBottom)
      return TRUE;
  } else if (kpSps->uiPocType == 1) {
    if (pLastSliceHeader->iDeltaPicOrderCnt[0] != pCurSliceHeader->iDeltaPicOrderCnt[0])
      return TRUE;
    if (pLastSliceHeader->iDeltaPicOrderCnt[1] != pCurSliceHeader->iDeltaPicOrderCnt[1])
      return TRUE;
  }

  return FALSE;
}


bool CheckAccessUnitBoundary (const PNalUnit kpCurNal, const PNalUnit kpLastNal, const PSps kpSps) {
  const PNalUnitHeaderExt kpLastNalHeaderExt = &kpLastNal->sNalHeaderExt;
  const PNalUnitHeaderExt kpCurNalHeaderExt = &kpCurNal->sNalHeaderExt;
  const SSliceHeader* kpLastSliceHeader = &kpLastNal->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader;
  const SSliceHeader* kpCurSliceHeader = &kpCurNal->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader;

  //Sub-clause 7.1.4.1.1 temporal_id
  if (kpLastNalHeaderExt->uiTemporalId != kpCurNalHeaderExt->uiTemporalId) {
    return TRUE;
  }

  // Subclause 7.4.1.2.5
  if (kpLastSliceHeader->iRedundantPicCnt < kpCurSliceHeader->iRedundantPicCnt)
    return FALSE;
  else if (kpLastSliceHeader->iRedundantPicCnt > kpCurSliceHeader->iRedundantPicCnt)
    return TRUE;

  // Subclause G7.4.1.2.4
  if (kpLastNalHeaderExt->uiDependencyId < kpCurNalHeaderExt->uiDependencyId)
    return FALSE;
  else if (kpLastNalHeaderExt->uiDependencyId > kpCurNalHeaderExt->uiDependencyId)
    return TRUE;
  if (kpLastNalHeaderExt->uiQualityId < kpCurNalHeaderExt->uiQualityId)
    return FALSE;
  else if (kpLastNalHeaderExt->uiQualityId > kpCurNalHeaderExt->uiQualityId)
    return TRUE;

  // Subclause 7.4.1.2.4
  if (kpLastSliceHeader->iFrameNum != kpCurSliceHeader->iFrameNum)
    return TRUE;
  if (kpLastSliceHeader->iPpsId != kpCurSliceHeader->iPpsId)
    return TRUE;
  if (kpLastSliceHeader->bFieldPicFlag != kpCurSliceHeader->bFieldPicFlag)
    return TRUE;
  if (kpLastSliceHeader->bBottomFiledFlag != kpCurSliceHeader->bBottomFiledFlag)
    return TRUE;
  if ((kpLastNalHeaderExt->sNalUnitHeader.uiNalRefIdc != NRI_PRI_LOWEST) != (kpCurNalHeaderExt->sNalUnitHeader.uiNalRefIdc
      != NRI_PRI_LOWEST))
    return TRUE;
  if (kpLastNalHeaderExt->bIdrFlag != kpCurNalHeaderExt->bIdrFlag)
    return TRUE;
  if (kpCurNalHeaderExt->bIdrFlag) {
    if (kpLastSliceHeader->uiIdrPicId != kpCurSliceHeader->uiIdrPicId)
      return TRUE;
  }
  if (kpSps->uiPocType == 0) {
    if (kpLastSliceHeader->iPicOrderCntLsb != kpCurSliceHeader->iPicOrderCntLsb)
      return TRUE;
    if (kpLastSliceHeader->iDeltaPicOrderCntBottom != kpCurSliceHeader->iDeltaPicOrderCntBottom)
      return TRUE;
  } else if (kpSps->uiPocType == 1) {
    if (kpLastSliceHeader->iDeltaPicOrderCnt[0] != kpCurSliceHeader->iDeltaPicOrderCnt[0])
      return TRUE;
    if (kpLastSliceHeader->iDeltaPicOrderCnt[1] != kpCurSliceHeader->iDeltaPicOrderCnt[1])
      return TRUE;
  }

  return FALSE;
}

/*!
 *************************************************************************************
 * \brief	to parse NON VCL NAL Units
 *
 * \param 	pCtx		decoder context
 * \param	rbsp		rbsp buffer of NAL Unit
 * \param	src_len		length of rbsp buffer
 *
 * \return	0 - successed
 *	    	1 - failed
 *
 *************************************************************************************
 */
int32_t ParseNonVclNal (PWelsDecoderContext pCtx, uint8_t* pRbsp, const int32_t kiSrcLen) {
  PBitStringAux	pBs = NULL;
  ENalUnitType eNalType	= NAL_UNIT_UNSPEC_0; // make initial value as unspecified
  int32_t iPicWidth		= 0;
  int32_t iPicHeight		= 0;
  int32_t iBitSize		= 0;
  int32_t iErr				= ERR_NONE;
  if (kiSrcLen <= 0)
    return iErr;

  pBs	     = &pCtx->sBs;	// SBitStringAux instance for non VCL NALs decoding
  iBitSize = (kiSrcLen << 3) - BsGetTrailingBits (pRbsp + kiSrcLen - 1); // convert into bit
  eNalType = pCtx->sCurNalHead.eNalUnitType;

  switch (eNalType) {
  case NAL_UNIT_SPS:
  case NAL_UNIT_SUBSET_SPS:
    if (iBitSize > 0)
      InitBits (pBs, pRbsp, iBitSize);
#ifdef DEBUG_PARSE_INFO
    WelsLog (pCtx, WELS_LOG_INFO, "parsing nal: %d \n", eNalType);
#endif
    iErr = ParseSps (pCtx, pBs, &iPicWidth, &iPicHeight);
    if (ERR_NONE != iErr) {	// modified for pSps/pSubsetSps invalid, 12/1/2009
      pCtx->iErrorCode |= dsNoParamSets;
      return iErr;
    }

    break;

  case NAL_UNIT_PPS:
    if (iBitSize > 0)
      InitBits (pBs, pRbsp, iBitSize);
#ifdef DEBUG_PARSE_INFO
    WelsLog (pCtx, WELS_LOG_INFO, "parsing nal: %d \n", eNalType);
#endif
    iErr = ParsePps (pCtx, &pCtx->sPpsBuffer[0], pBs);
    if (ERR_NONE != iErr) {	// modified for pps invalid, 12/1/2009
      pCtx->iErrorCode |= dsNoParamSets;
      return iErr;
    }

    pCtx->bPpsExistAheadFlag	= true;

    break;

  case NAL_UNIT_SEI:

    break;

  case NAL_UNIT_PREFIX:
    break;
  case NAL_UNIT_CODED_SLICE_DPA:
  case NAL_UNIT_CODED_SLICE_DPB:
  case NAL_UNIT_CODED_SLICE_DPC:

    break;

  default:
    break;
  }

  return iErr;
}

void_t ParseRefBasePicMarking (PBitStringAux pBs, PRefBasePicMarking pRefBasePicMarking) {
  const bool kbAdaptiveMarkingModeFlag = !!BsGetOneBit (pBs);
  pRefBasePicMarking->bAdaptiveRefBasePicMarkingModeFlag = kbAdaptiveMarkingModeFlag;
  if (kbAdaptiveMarkingModeFlag) {
    int32_t iIdx = 0;
    do {
      const uint32_t kuiMmco = BsGetUe (pBs);

      pRefBasePicMarking->mmco_base[iIdx].uiMmcoType	= kuiMmco;

      if (kuiMmco == MMCO_END)
        break;

      if (kuiMmco == MMCO_SHORT2UNUSED) {
        pRefBasePicMarking->mmco_base[iIdx].uiDiffOfPicNums	= 1 + BsGetUe (pBs);
        pRefBasePicMarking->mmco_base[iIdx].iShortFrameNum	= 0;
      } else if (kuiMmco == MMCO_LONG2UNUSED) {
        pRefBasePicMarking->mmco_base[iIdx].uiLongTermPicNum	= BsGetUe (pBs);
      }
      ++ iIdx;
    } while (iIdx < MAX_MMCO_COUNT);
  }
}

void_t ParsePrefixNalUnit (PWelsDecoderContext pCtx, PBitStringAux pBs) {
  PNalUnit pCurNal = &pCtx->sPrefixNal;

  if (pCurNal->sNalHeaderExt.sNalUnitHeader.uiNalRefIdc != 0) {
    PNalUnitHeaderExt head_ext = &pCurNal->sNalHeaderExt;
    PPrefixNalUnit sPrefixNal = &pCurNal->sNalData.sPrefixNal;
    sPrefixNal->bStoreRefBasePicFlag	= !!BsGetOneBit (pBs);
    if ((head_ext->bUseRefBasePicFlag || sPrefixNal->bStoreRefBasePicFlag) && !head_ext->bIdrFlag) {
      ParseRefBasePicMarking (pBs, &sPrefixNal->sRefPicBaseMarking);
    }
    sPrefixNal->bPrefixNalUnitAdditionalExtFlag	= !!BsGetOneBit (pBs);
    if (sPrefixNal->bPrefixNalUnitAdditionalExtFlag) {
      sPrefixNal->bPrefixNalUnitExtFlag	= !!BsGetOneBit (pBs);
    }
  }
}


int32_t DecodeSpsSvcExt (PWelsDecoderContext pCtx, PSubsetSps pSpsExt, PBitStringAux pBs) {
  PSpsSvcExt  pExt			= NULL;
  uint8_t uiChromaArrayType	= 1;

  pExt	= &pSpsExt->sSpsSvcExt;

  pExt->bInterLayerDeblockingFilterCtrlPresentFlag	= !!BsGetOneBit (pBs);
  pExt->uiExtendedSpatialScalability						= BsGetBits (pBs, 2);
  if (pExt->uiExtendedSpatialScalability > 2) {
    WelsLog (pCtx, WELS_LOG_WARNING, "DecodeSpsSvcExt():extended_spatial_scalability (%d) != 0, ESS not supported!\n",
             pExt->uiExtendedSpatialScalability);
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_ESS);
  }

  pExt->uiChromaPhaseXPlus1Flag	=
    0;	// FIXME: Incoherent with JVT X201 standard (= 1), but conformance to JSVM (= 0) implementation.
  pExt->uiChromaPhaseYPlus1		= 1;
  uiChromaArrayType = pSpsExt->sSps.uiChromaArrayType;

  pExt->uiChromaPhaseXPlus1Flag	= BsGetOneBit (pBs);
  pExt->uiChromaPhaseYPlus1		= BsGetBits (pBs, 2);

  pExt->uiSeqRefLayerChromaPhaseXPlus1Flag	= pExt->uiChromaPhaseXPlus1Flag;
  pExt->uiSeqRefLayerChromaPhaseYPlus1		= pExt->uiChromaPhaseYPlus1;
  memset (&pExt->sSeqScaledRefLayer, 0, sizeof (SPosOffset));

  if (pExt->uiExtendedSpatialScalability == 1) {
    SPosOffset* const kpPos = &pExt->sSeqScaledRefLayer;
    pExt->uiSeqRefLayerChromaPhaseXPlus1Flag	= BsGetOneBit (pBs);
    pExt->uiSeqRefLayerChromaPhaseYPlus1		= BsGetBits (pBs, 2);

    kpPos->iLeftOffset	= BsGetSe (pBs);
    kpPos->iTopOffset	= BsGetSe (pBs);
    kpPos->iRightOffset	= BsGetSe (pBs);
    kpPos->iBottomOffset = BsGetSe (pBs);
  }

  pExt->bSeqTCoeffLevelPredFlag	= !!BsGetOneBit (pBs);
  pExt->bAdaptiveTCoeffLevelPredFlag	= false;
  if (pExt->bSeqTCoeffLevelPredFlag)
    pExt->bAdaptiveTCoeffLevelPredFlag	= !!BsGetOneBit (pBs);
  pExt->bSliceHeaderRestrictionFlag	= !!BsGetOneBit (pBs);



  return 0;
}

// table A-1 - Level limits
static const SLevelLimits g_kSLevelLimits[17] = {
  {1485, 99, 396, 64, 175, -256, 255, 2, 0x7fff}, /* level 1 */
  {1485, 99, 396, 128, 350, -256, 255, 2, 0x7fff}, /* level 1.b */
  {3000, 396, 900, 192, 500, -512, 511, 2, 0x7fff}, /* level 1.1 */
  {6000, 396, 2376, 384, 1000, -512, 511, 2, 0x7fff}, /* level 1.2 */
  {11880, 396, 2376, 768, 2000, -512, 511, 2, 0x7fff}, /* level 1.3 */
  {11880, 396, 2376, 2000, 2000, -512, 511, 2, 0x7fff}, /* level 2 */
  {19800, 792, 4752, 4000, 4000, -1024, 1023, 2, 0x7fff}, /* level 2.1 */
  {20250, 1620, 8100, 4000, 4000, -1024, 1023, 2, 0x7fff}, /* level 2.2 */
  {40500, 1620, 8100, 10000, 10000, -1024, 1023, 2, 32 }, /* level 3 */
  {108000, 3600, 18000, 14000, 14000, -2048, 2047, 4, 16}, /* level 3.1 */
  {216000, 5120, 20480, 20000, 20000, -2048, 2047, 4, 16}, /* level 3.2 */
  {245760, 8192, 32768, 20000, 25000, -2048, 2047, 4, 16}, /* level 4 */
  {245760, 8192, 32768, 50000, 62500, -2048, 2047, 2, 16}, /* level 4.1 */
  {522240, 8704, 34816, 50000, 62500, -2048, 2047, 2, 16}, /* level 4.2 */
  {589824, 22080, 110400, 135000, 135000, -2048, 2047, 2, 16}, /* level 5 */
  {983040, 36864, 184320, 240000, 240000, -2048, 2047, 2, 16}, /* level 5.1 */
  {2073600, 36864, 184320, 240000, 240000, -2048, 2047, 2, 16} /* level 5.2 */
};

const SLevelLimits *GetLevelLimits(int32_t iLevelIdx, bool bConstraint3) {
  switch (iLevelIdx) {
  case 10:
    return &g_kSLevelLimits[0];
  case 11:
    if(bConstraint3)
      return &g_kSLevelLimits[1];
    else
      return &g_kSLevelLimits[2];
  case 12:
    return &g_kSLevelLimits[3];
  case 13:
    return &g_kSLevelLimits[4];
  case 20:
    return &g_kSLevelLimits[5];
  case 21:
    return &g_kSLevelLimits[6];
  case 22:
    return &g_kSLevelLimits[7];
  case 30:
    return &g_kSLevelLimits[8];
  case 31:
    return &g_kSLevelLimits[9];
  case 32:
    return &g_kSLevelLimits[10];
  case 40:
    return &g_kSLevelLimits[11];
  case 41:
    return &g_kSLevelLimits[12];
  case 42:
    return &g_kSLevelLimits[13];
  case 50:
    return &g_kSLevelLimits[14];
  case 51:
    return &g_kSLevelLimits[15];
  case 52:
    return &g_kSLevelLimits[16];
  default:
    return NULL;
  }
  return NULL;
}
/*!
 *************************************************************************************
 * \brief	to parse Sequence Parameter Set (SPS)
 *
 * \param	pCtx		Decoder context
 * \param	pBsAux		bitstream reader auxiliary
 * \param	pPicWidth	picture width current Sps represented
 * \param	pPicHeight	picture height current Sps represented
 *
 * \return	0 - successed
 *		1 - failed
 *
 * \note	Call it in case eNalUnitType is SPS.
 *************************************************************************************
 */


int32_t ParseSps (PWelsDecoderContext pCtx, PBitStringAux pBsAux, int32_t* pPicWidth, int32_t* pPicHeight) {
  PBitStringAux pBs		= pBsAux;
  PSps pSps				= NULL;
  PSubsetSps pSubsetSps	= NULL;
  SNalUnitHeader* pNalHead = &pCtx->sCurNalHead;
  ProfileIdc	uiProfileIdc;
  uint8_t	uiLevelIdc;
  int32_t iSpsId;
  bool bConstraintSetFlags[6] = { false };
  const bool kbUseSubsetFlag   = IS_SUBSET_SPS_NAL (pNalHead->eNalUnitType);


  if (kbUseSubsetFlag) {	// SubsetSps
    pCtx->bSubspsExistAheadFlag	= true;
  } else {	// Sps
    pCtx->bSpsExistAheadFlag		= true;

    // added for EC, 10/28/2009
    // for safe
    memset (&pCtx->bSpsAvailFlags[0], 0, sizeof (pCtx->bSpsAvailFlags));
    memset (&pCtx->bSubspsAvailFlags[0], 0, sizeof (pCtx->bSubspsAvailFlags));
    memset (&pCtx->bPpsAvailFlags[0], 0, sizeof (pCtx->bPpsAvailFlags));

#ifdef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
    pCtx->iSpsTotalNum    = 0;
    pCtx->iSubspsTotalNum = 0;
    pCtx->iPpsTotalNum    = 0;
#endif //MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
  }

  uiProfileIdc	= BsGetBits (pBs, 8);
  bConstraintSetFlags[0]	= !!BsGetOneBit (pBs);	// constraint_set0_flag
  bConstraintSetFlags[1]	= !!BsGetOneBit (pBs);	// constraint_set1_flag
  bConstraintSetFlags[2]	= !!BsGetOneBit (pBs);	// constraint_set2_flag
  bConstraintSetFlags[3]	= !!BsGetOneBit (pBs);	// constraint_set3_flag
  bConstraintSetFlags[4]	= !!BsGetOneBit (pBs);	// constraint_set4_flag
  bConstraintSetFlags[5]	= !!BsGetOneBit (pBs);	// constraint_set5_flag
  BsGetBits (pBs, 2);							// reserved_zero_2bits, equal to 0
  uiLevelIdc	= BsGetBits (pBs, 8);				// level_idc
  iSpsId		= BsGetUe (pBs);					// seq_parameter_set_id
  if (iSpsId >= MAX_SPS_COUNT || iSpsId < 0) {	// Modified to check invalid negative iSpsId, 12/1/2009
    WelsLog (pCtx, WELS_LOG_WARNING, " iSpsId is out of range! \n");
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_SPS_ID_OVERFLOW);
  }

  if (kbUseSubsetFlag) {
#ifdef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
    pSubsetSps = &pCtx->sSubsetSpsBuffer[pCtx->iSubspsTotalNum];
    pCtx->bSubspsAvailFlags[pCtx->iSubspsTotalNum] = true;

    pSubsetSps->sSps.iSpsId = iSpsId;
    pSps = &pSubsetSps->sSps;
    ++pCtx->iSubspsTotalNum;
#else
    pSubsetSps	= &pCtx->sSubsetSpsBuffer[iSpsId];
    pSps		= &pSubsetSps->sSps;
    pCtx->bSubspsAvailFlags[iSpsId]	= true; // added for EC, 10/28/2009
#endif //MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
  } else {
#ifdef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
    pSps = &pCtx->sSpsBuffer[pCtx->iSpsTotalNum];
    pCtx->bSpsAvailFlags[pCtx->iSpsTotalNum] = true;

    pSps->iSpsId = iSpsId;
    ++pCtx->iSpsTotalNum;
#else
    pSps = &pCtx->sSpsBuffer[iSpsId];
    pCtx->bSpsAvailFlags[iSpsId] = true; // added for EC, 10/28/2009
#endif //MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
  }
  const SLevelLimits *pSLevelLimits = GetLevelLimits(uiLevelIdc, bConstraintSetFlags[3]);
  if (NULL == pSLevelLimits) {
     WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): level_idx (%d).\n", uiLevelIdc);
     return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_NON_BASELINE);
  }
  else pSps->pSLevelLimits = pSLevelLimits;
  // syntax elements in default
  pSps->uiChromaFormatIdc	= 1;
  pSps->uiBitDepthLuma		=
    pSps->uiBitDepthChroma	= 8;

  pSps->uiProfileIdc	= uiProfileIdc;
  pSps->uiLevelIdc	= uiLevelIdc;
  pSps->iSpsId		= iSpsId;

  if (PRO_SCALABLE_BASELINE == uiProfileIdc || PRO_SCALABLE_HIGH == uiProfileIdc ||
      PRO_HIGH == uiProfileIdc || PRO_HIGH10 == uiProfileIdc ||
      PRO_HIGH422 == uiProfileIdc || PRO_HIGH444 == uiProfileIdc ||
      PRO_CAVLC444 == uiProfileIdc || 44 == uiProfileIdc) {

    pSps->uiChromaFormatIdc = BsGetUe (pBs);
    if (pSps->uiChromaFormatIdc != 1) {
      WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): chroma_format_idc (%d) = 1 supported.\n", pSps->uiChromaFormatIdc);
      return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_NON_BASELINE);
    }
    pSps->uiChromaArrayType = pSps->uiChromaFormatIdc;
    pSps->uiBitDepthLuma		= 8 + BsGetUe (pBs);
    if (pSps->uiBitDepthLuma != 8) {
      WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): bit_depth_luma (%d) Only 8 bit supported.\n", pSps->uiBitDepthLuma);
      return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_NON_BASELINE);
    }

    pSps->uiBitDepthChroma	= 8 + BsGetUe (pBs);
    if (pSps->uiBitDepthChroma != 8) {
      WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): bit_depth_chroma (%d). Only 8 bit supported.\n", pSps->uiBitDepthChroma);
      return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_NON_BASELINE);
    }
    pSps->bQpPrimeYZeroTransfBypassFlag	= !!BsGetOneBit (pBs);
    pSps->bSeqScalingMatrixPresentFlag	= !!BsGetOneBit (pBs);

    if (pSps->bSeqScalingMatrixPresentFlag) {	// For high profile, it is not used in current application. FIXME
      WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): seq_scaling_matrix_present_flag (%d). Feature not supported.\n",
               pSps->bSeqScalingMatrixPresentFlag);
      return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_NON_BASELINE);
    }
  }

  pSps->uiLog2MaxFrameNum	= 4 + BsGetUe (pBs);	// log2_max_frame_num_minus4
  pSps->uiPocType			= BsGetUe (pBs);		// pic_order_cnt_type

  if (0 == pSps->uiPocType) {
    pSps->iLog2MaxPocLsb	= 4 + BsGetUe (pBs);	// log2_max_pic_order_cnt_lsb_minus4

  } else if (1 == pSps->uiPocType) {
    int32_t i;
    pSps->bDeltaPicOrderAlwaysZeroFlag	= !!BsGetOneBit (pBs);	// bDeltaPicOrderAlwaysZeroFlag
    pSps->iOffsetForNonRefPic			= BsGetSe (pBs);		// iOffsetForNonRefPic
    pSps->iOffsetForTopToBottomField	= BsGetSe (pBs);		// iOffsetForTopToBottomField
    pSps->iNumRefFramesInPocCycle		= BsGetUe (pBs);	// num_ref_frames_in_pic_order_cnt_cycle
    for (i = 0; i < pSps->iNumRefFramesInPocCycle; i++)
      pSps->iOffsetForRefFrame[ i ]	= BsGetSe (pBs);		// iOffsetForRefFrame[ i ]
  }
  if (pSps->uiPocType > 2) {
    WelsLog (pCtx, WELS_LOG_WARNING, " illegal pic_order_cnt_type: %d ! \n", pSps->uiPocType);
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_POC_TYPE);
  }

  pSps->iNumRefFrames	= BsGetUe (pBs);		// max_num_ref_frames
  pSps->bGapsInFrameNumValueAllowedFlag	= !!BsGetOneBit (pBs);	// bGapsInFrameNumValueAllowedFlag
  pSps->iMbWidth		= 1 + BsGetUe (pBs);		// pic_width_in_mbs_minus1
  pSps->iMbHeight		= 1 + BsGetUe (pBs);		// pic_height_in_map_units_minus1
  pSps->uiTotalMbCount	= pSps->iMbWidth * pSps->iMbHeight;
  pSps->bFrameMbsOnlyFlag	= !!BsGetOneBit (pBs);	// frame_mbs_only_flag

  if (!pSps->bFrameMbsOnlyFlag) {
    WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): frame_mbs_only_flag (%d) not supported.\n", pSps->bFrameMbsOnlyFlag);
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_MBAFF);
  }
  pSps->bDirect8x8InferenceFlag	= !!BsGetOneBit (pBs);	// direct_8x8_inference_flag
  pSps->bFrameCroppingFlag		= !!BsGetOneBit (pBs);	// frame_cropping_flag
  if (pSps->bFrameCroppingFlag) {
    pSps->sFrameCrop.iLeftOffset	= BsGetUe (pBs);	// frame_crop_left_offset
    pSps->sFrameCrop.iRightOffset	= BsGetUe (pBs);	// frame_crop_right_offset
    pSps->sFrameCrop.iTopOffset		= BsGetUe (pBs);	// frame_crop_top_offset
    pSps->sFrameCrop.iBottomOffset	= BsGetUe (pBs);	// frame_crop_bottom_offset
  } else {
    pSps->sFrameCrop.iLeftOffset	= 0;				// frame_crop_left_offset
    pSps->sFrameCrop.iRightOffset	= 0;				// frame_crop_right_offset
    pSps->sFrameCrop.iTopOffset		= 0;				// frame_crop_top_offset
    pSps->sFrameCrop.iBottomOffset	= 0;				// frame_crop_bottom_offset
  }
  pSps->bVuiParamPresentFlag			= !!BsGetOneBit (pBs);	// vui_parameters_present_flag

  // Check if SPS SVC extension applicated
  if (kbUseSubsetFlag && (PRO_SCALABLE_BASELINE == uiProfileIdc || PRO_SCALABLE_HIGH == uiProfileIdc)) {
    if (DecodeSpsSvcExt (pCtx, pSubsetSps, pBs) != ERR_NONE) {
      return -1;
    }

    pSubsetSps->bSvcVuiParamPresentFlag = !!BsGetOneBit (pBs);
    if (pSubsetSps->bSvcVuiParamPresentFlag) {
    }
  }


  if (PRO_SCALABLE_BASELINE == uiProfileIdc || PRO_SCALABLE_HIGH == uiProfileIdc)
    pCtx->bAvcBasedFlag	= false;
  else
    pCtx->bAvcBasedFlag	= true;	// added for avc base pBs

  *pPicWidth	= pSps->iMbWidth << 4;
  *pPicHeight	= pSps->iMbHeight << 4;

  return 0;
}

/*!
 *************************************************************************************
 * \brief	to parse Picture Parameter Set (PPS)
 *
 * \param	pCtx		Decoder context
 * \param 	pPpsList	pps list
 * \param	pBsAux		bitstream reader auxiliary
 *
 * \return	0 - successed
 *		1 - failed
 *
 * \note	Call it in case eNalUnitType is PPS.
 *************************************************************************************
 */
int32_t ParsePps (PWelsDecoderContext pCtx, PPps pPpsList, PBitStringAux pBsAux) {

  PPps pPps = NULL;
  uint32_t uiPpsId = 0;
  uint32_t iTmp;

  uiPpsId = BsGetUe (pBsAux);
  if (uiPpsId >= MAX_PPS_COUNT) {
    return ERR_INFO_PPS_ID_OVERFLOW;
  }

#ifdef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
  pPps = &pPpsList[pCtx->iPpsTotalNum];
#else
  pPps = &pPpsList[uiPpsId];
#endif //MOSAIC_AVOID_BASED_ON_SPS_PPS_ID


  pPps->iPpsId = uiPpsId;
  pPps->iSpsId = BsGetUe (pBsAux);

  if (pPps->iSpsId >= MAX_SPS_COUNT) {
    return ERR_INFO_SPS_ID_OVERFLOW;
  }

  pPps->bEntropyCodingModeFlag = !!BsGetOneBit (pBsAux);
  pPps->bPicOrderPresentFlag   = !!BsGetOneBit (pBsAux);

  pPps->uiNumSliceGroups = 1 + BsGetUe (pBsAux);

  if (pPps->uiNumSliceGroups > MAX_SLICEGROUP_IDS) {
    return ERR_INFO_INVALID_SLICEGROUP;
  }

  if (pPps->uiNumSliceGroups > 1) {
    pPps->uiSliceGroupMapType = BsGetUe (pBsAux);
    if (pPps->uiSliceGroupMapType > 1) {
      WelsLog (pCtx, WELS_LOG_WARNING, "ParsePps(): slice_group_map_type (%d): support only 0,1.\n",
               pPps->uiSliceGroupMapType);
      return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_FMOTYPE);
    }

    switch (pPps->uiSliceGroupMapType) {
    case 0:
      for (iTmp = 0; iTmp < pPps->uiNumSliceGroups; iTmp++) {
        pPps->uiRunLength[iTmp] = 1 + BsGetUe (pBsAux);
      }
      break;
    default:
      break;
    }
  }

  pPps->uiNumRefIdxL0Active = 1 + BsGetUe (pBsAux);
  pPps->uiNumRefIdxL1Active = 1 + BsGetUe (pBsAux);

  if (pPps->uiNumRefIdxL0Active > MAX_REF_PIC_COUNT ||
      pPps->uiNumRefIdxL1Active > MAX_REF_PIC_COUNT) {
    return ERR_INFO_REF_COUNT_OVERFLOW;
  }

  pPps->bWeightedPredFlag  = !!BsGetOneBit (pBsAux);
  pPps->uiWeightedBipredIdc = BsGetBits (pBsAux, 2);
  if (pPps->bWeightedPredFlag || pPps->uiWeightedBipredIdc != 0) {
    WelsLog (pCtx, WELS_LOG_WARNING, "ParsePps(): weighted_pred_flag (%d) weighted_bipred_idc (%d) neither supported.\n",
             pPps->bWeightedPredFlag, pPps->uiWeightedBipredIdc);
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_WP);
  }

  pPps->iPicInitQp = 26 + BsGetSe (pBsAux);
  pPps->iPicInitQs = 26 + BsGetSe (pBsAux);

  pPps->iChromaQpIndexOffset                  = BsGetSe (pBsAux);
  pPps->bDeblockingFilterControlPresentFlag   = !!BsGetOneBit (pBsAux);
  pPps->bConstainedIntraPredFlag              = !!BsGetOneBit (pBsAux);
  pPps->bRedundantPicCntPresentFlag           = !!BsGetOneBit (pBsAux);


#ifdef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
  pCtx->bPpsAvailFlags[pCtx->iPpsTotalNum] = true;
  ++pCtx->iPpsTotalNum;
#else
  pCtx->bPpsAvailFlags[uiPpsId] = true; // added for EC, 10/28/2009
#endif //MOSAIC_AVOID_BASED_ON_SPS_PPS_ID

  return ERR_NONE;
}

/*!
 *************************************************************************************
 * \brief	to parse SEI message payload
 *
 * \param 	pSei		sei message to be parsed output
 * \param	pBsAux		bitstream reader auxiliary
 *
 * \return	0 - successed
 *		1 - failed
 *
 * \note	Call it in case eNalUnitType is NAL_UNIT_SEI.
 *************************************************************************************
 */
int32_t ParseSei (void_t* pSei, PBitStringAux pBsAux) {	// reserved Sei_Msg type


  return ERR_NONE;
}

/*!
 *************************************************************************************
 * \brief	reset fmo list due to got Sps now
 *
 * \param	pCtx	decoder context
 *
 * \return	count number of fmo context units are reset
 *************************************************************************************
 */
int32_t ResetFmoList (PWelsDecoderContext pCtx) {
  int32_t iCountNum = 0;
  if (NULL != pCtx) {
    // Fixed memory leak due to PPS_ID might not be continuous sometimes, 1/5/2010
    UninitFmoList (&pCtx->sFmoList[0], MAX_PPS_COUNT, pCtx->iActiveFmoNum);
    iCountNum	= pCtx->iActiveFmoNum;
    pCtx->iActiveFmoNum	= 0;
  }
  return iCountNum;
}

} // namespace WelsDec
