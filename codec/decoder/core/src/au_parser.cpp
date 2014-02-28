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
      return NULL;
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
    return true;
  }

  // Subclause 7.4.1.2.5
  if (pLastSliceHeader->iRedundantPicCnt < pCurSliceHeader->iRedundantPicCnt)
    return false;
  else if (pLastSliceHeader->iRedundantPicCnt > pCurSliceHeader->iRedundantPicCnt)
    return true;

  // Subclause G7.4.1.2.4
  if (pLastNalHdrExt->uiDependencyId < pCurNalHeaderExt->uiDependencyId)
    return false;
  else if (pLastNalHdrExt->uiDependencyId > pCurNalHeaderExt->uiDependencyId)
    return true;
  if (pLastNalHdrExt->uiQualityId < pCurNalHeaderExt->uiQualityId)
    return false;
  else if (pLastNalHdrExt->uiQualityId > pCurNalHeaderExt->uiQualityId)
    return true;

  // Subclause 7.4.1.2.4
  if (pLastSliceHeader->iFrameNum != pCurSliceHeader->iFrameNum)
    return true;
  if (pLastSliceHeader->iPpsId != pCurSliceHeader->iPpsId)
    return true;
  if (pLastSliceHeader->bFieldPicFlag != pCurSliceHeader->bFieldPicFlag)
    return true;
  if (pLastSliceHeader->bBottomFiledFlag != pCurSliceHeader->bBottomFiledFlag)
    return true;
  if ((pLastNalHdrExt->sNalUnitHeader.uiNalRefIdc != NRI_PRI_LOWEST) != (pCurNalHeaderExt->sNalUnitHeader.uiNalRefIdc !=
      NRI_PRI_LOWEST))
    return true;
  if (pLastNalHdrExt->bIdrFlag != pCurNalHeaderExt->bIdrFlag)
    return true;
  if (pCurNalHeaderExt->bIdrFlag) {
    if (pLastSliceHeader->uiIdrPicId != pCurSliceHeader->uiIdrPicId)
      return true;
  }
  if (kpSps->uiPocType == 0) {
    if (pLastSliceHeader->iPicOrderCntLsb != pCurSliceHeader->iPicOrderCntLsb)
      return true;
    if (pLastSliceHeader->iDeltaPicOrderCntBottom != pCurSliceHeader->iDeltaPicOrderCntBottom)
      return true;
  } else if (kpSps->uiPocType == 1) {
    if (pLastSliceHeader->iDeltaPicOrderCnt[0] != pCurSliceHeader->iDeltaPicOrderCnt[0])
      return true;
    if (pLastSliceHeader->iDeltaPicOrderCnt[1] != pCurSliceHeader->iDeltaPicOrderCnt[1])
      return true;
  }

  return false;
}


bool CheckAccessUnitBoundary (const PNalUnit kpCurNal, const PNalUnit kpLastNal, const PSps kpSps) {
  const PNalUnitHeaderExt kpLastNalHeaderExt = &kpLastNal->sNalHeaderExt;
  const PNalUnitHeaderExt kpCurNalHeaderExt = &kpCurNal->sNalHeaderExt;
  const SSliceHeader* kpLastSliceHeader = &kpLastNal->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader;
  const SSliceHeader* kpCurSliceHeader = &kpCurNal->sNalData.sVclNal.sSliceHeaderExt.sSliceHeader;

  //Sub-clause 7.1.4.1.1 temporal_id
  if (kpLastNalHeaderExt->uiTemporalId != kpCurNalHeaderExt->uiTemporalId) {
    return true;
  }

  // Subclause 7.4.1.2.5
  if (kpLastSliceHeader->iRedundantPicCnt < kpCurSliceHeader->iRedundantPicCnt)
    return false;
  else if (kpLastSliceHeader->iRedundantPicCnt > kpCurSliceHeader->iRedundantPicCnt)
    return true;

  // Subclause G7.4.1.2.4
  if (kpLastNalHeaderExt->uiDependencyId < kpCurNalHeaderExt->uiDependencyId)
    return false;
  else if (kpLastNalHeaderExt->uiDependencyId > kpCurNalHeaderExt->uiDependencyId)
    return true;
  if (kpLastNalHeaderExt->uiQualityId < kpCurNalHeaderExt->uiQualityId)
    return false;
  else if (kpLastNalHeaderExt->uiQualityId > kpCurNalHeaderExt->uiQualityId)
    return true;

  // Subclause 7.4.1.2.4
  if (kpLastSliceHeader->iFrameNum != kpCurSliceHeader->iFrameNum)
    return true;
  if (kpLastSliceHeader->iPpsId != kpCurSliceHeader->iPpsId)
    return true;
  if (kpLastSliceHeader->bFieldPicFlag != kpCurSliceHeader->bFieldPicFlag)
    return true;
  if (kpLastSliceHeader->bBottomFiledFlag != kpCurSliceHeader->bBottomFiledFlag)
    return true;
  if ((kpLastNalHeaderExt->sNalUnitHeader.uiNalRefIdc != NRI_PRI_LOWEST) != (kpCurNalHeaderExt->sNalUnitHeader.uiNalRefIdc
      != NRI_PRI_LOWEST))
    return true;
  if (kpLastNalHeaderExt->bIdrFlag != kpCurNalHeaderExt->bIdrFlag)
    return true;
  if (kpCurNalHeaderExt->bIdrFlag) {
    if (kpLastSliceHeader->uiIdrPicId != kpCurSliceHeader->uiIdrPicId)
      return true;
  }
  if (kpSps->uiPocType == 0) {
    if (kpLastSliceHeader->iPicOrderCntLsb != kpCurSliceHeader->iPicOrderCntLsb)
      return true;
    if (kpLastSliceHeader->iDeltaPicOrderCntBottom != kpCurSliceHeader->iDeltaPicOrderCntBottom)
      return true;
  } else if (kpSps->uiPocType == 1) {
    if (kpLastSliceHeader->iDeltaPicOrderCnt[0] != kpCurSliceHeader->iDeltaPicOrderCnt[0])
      return true;
    if (kpLastSliceHeader->iDeltaPicOrderCnt[1] != kpCurSliceHeader->iDeltaPicOrderCnt[1])
      return true;
  }

  return false;
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

int32_t ParseRefBasePicMarking (PBitStringAux pBs, PRefBasePicMarking pRefBasePicMarking) {
  uint32_t uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //adaptive_ref_base_pic_marking_mode_flag
  const bool kbAdaptiveMarkingModeFlag = !!uiCode;
  pRefBasePicMarking->bAdaptiveRefBasePicMarkingModeFlag = kbAdaptiveMarkingModeFlag;
  if (kbAdaptiveMarkingModeFlag) {
    int32_t iIdx = 0;
    do {
      WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //MMCO_base
      const uint32_t kuiMmco = uiCode;

      pRefBasePicMarking->mmco_base[iIdx].uiMmcoType	= kuiMmco;

      if (kuiMmco == MMCO_END)
        break;

      if (kuiMmco == MMCO_SHORT2UNUSED) {
        WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //difference_of_base_pic_nums_minus1
        pRefBasePicMarking->mmco_base[iIdx].uiDiffOfPicNums	= 1 + uiCode;
        pRefBasePicMarking->mmco_base[iIdx].iShortFrameNum	= 0;
      } else if (kuiMmco == MMCO_LONG2UNUSED) {
        WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //long_term_base_pic_num
        pRefBasePicMarking->mmco_base[iIdx].uiLongTermPicNum	= uiCode;
      }
      ++ iIdx;
    } while (iIdx < MAX_MMCO_COUNT);
  }
  return ERR_NONE;
}

int32_t ParsePrefixNalUnit (PWelsDecoderContext pCtx, PBitStringAux pBs) {
  PNalUnit pCurNal = &pCtx->sPrefixNal;
  uint32_t uiCode;

  if (pCurNal->sNalHeaderExt.sNalUnitHeader.uiNalRefIdc != 0) {
    PNalUnitHeaderExt head_ext = &pCurNal->sNalHeaderExt;
    PPrefixNalUnit sPrefixNal = &pCurNal->sNalData.sPrefixNal;
    WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //store_ref_base_pic_flag
    sPrefixNal->bStoreRefBasePicFlag	= !!uiCode;
    if ((head_ext->bUseRefBasePicFlag || sPrefixNal->bStoreRefBasePicFlag) && !head_ext->bIdrFlag) {
      WELS_READ_VERIFY (ParseRefBasePicMarking (pBs, &sPrefixNal->sRefPicBaseMarking));
    }
    WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //additional_prefix_nal_unit_extension_flag
    sPrefixNal->bPrefixNalUnitAdditionalExtFlag = !!uiCode;
    if (sPrefixNal->bPrefixNalUnitAdditionalExtFlag) {
      WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //additional_prefix_nal_unit_extension_data_flag
      sPrefixNal->bPrefixNalUnitExtFlag	= !!uiCode;
    }
  }
  return ERR_NONE;
}

#define SUBSET_SPS_SEQ_SCALED_REF_LAYER_LEFT_OFFSET_MIN -32768
#define SUBSET_SPS_SEQ_SCALED_REF_LAYER_LEFT_OFFSET_MAX 32767
#define SUBSET_SPS_SEQ_SCALED_REF_LAYER_TOP_OFFSET_MIN -32768
#define SUBSET_SPS_SEQ_SCALED_REF_LAYER_TOP_OFFSET_MAX 32767
#define SUBSET_SPS_SEQ_SCALED_REF_LAYER_RIGHT_OFFSET_MIN -32768
#define SUBSET_SPS_SEQ_SCALED_REF_LAYER_RIGHT_OFFSET_MAX 32767
#define SUBSET_SPS_SEQ_SCALED_REF_LAYER_BOTTOM_OFFSET_MIN -32768
#define SUBSET_SPS_SEQ_SCALED_REF_LAYER_BOTTOM_OFFSET_MAX 32767




int32_t DecodeSpsSvcExt (PWelsDecoderContext pCtx, PSubsetSps pSpsExt, PBitStringAux pBs) {
  PSpsSvcExt  pExt			= NULL;
  uint8_t uiChromaArrayType	= 1;
  uint32_t uiCode;
  int32_t iCode;

  pExt	= &pSpsExt->sSpsSvcExt;

  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //inter_layer_deblocking_filter_control_present_flag
  pExt->bInterLayerDeblockingFilterCtrlPresentFlag	= !!uiCode;
  WELS_READ_VERIFY (BsGetBits (pBs, 2, &uiCode)); //extended_spatial_scalability_idc
  pExt->uiExtendedSpatialScalability						= uiCode;
  if (pExt->uiExtendedSpatialScalability > 2) {
    WelsLog (pCtx, WELS_LOG_WARNING, "DecodeSpsSvcExt():extended_spatial_scalability (%d) != 0, ESS not supported!\n",
             pExt->uiExtendedSpatialScalability);
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_ESS);
  }

  pExt->uiChromaPhaseXPlus1Flag	=
    0;	// FIXME: Incoherent with JVT X201 standard (= 1), but conformance to JSVM (= 0) implementation.
  pExt->uiChromaPhaseYPlus1		= 1;
  uiChromaArrayType = pSpsExt->sSps.uiChromaArrayType;

  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //chroma_phase_x_plus1_flag
  pExt->uiChromaPhaseXPlus1Flag	= uiCode;
  WELS_READ_VERIFY (BsGetBits (pBs, 2, &uiCode)); //chroma_phase_y_plus1
  pExt->uiChromaPhaseYPlus1		= uiCode;

  pExt->uiSeqRefLayerChromaPhaseXPlus1Flag	= pExt->uiChromaPhaseXPlus1Flag;
  pExt->uiSeqRefLayerChromaPhaseYPlus1		= pExt->uiChromaPhaseYPlus1;
  memset (&pExt->sSeqScaledRefLayer, 0, sizeof (SPosOffset));

  if (pExt->uiExtendedSpatialScalability == 1) {
    SPosOffset* const kpPos = &pExt->sSeqScaledRefLayer;
    WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //seq_ref_layer_chroma_phase_x_plus1_flag
    pExt->uiSeqRefLayerChromaPhaseXPlus1Flag	= uiCode;
    WELS_READ_VERIFY (BsGetBits (pBs, 2, &uiCode)); //seq_ref_layer_chroma_phase_y_plus1
    pExt->uiSeqRefLayerChromaPhaseYPlus1		= uiCode;

    WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //seq_scaled_ref_layer_left_offset
    kpPos->iLeftOffset	= iCode;
    WELS_CHECK_SE_BOTH_WARNING (kpPos->iLeftOffset, SUBSET_SPS_SEQ_SCALED_REF_LAYER_LEFT_OFFSET_MIN,
                                SUBSET_SPS_SEQ_SCALED_REF_LAYER_LEFT_OFFSET_MAX, "seq_scaled_ref_layer_left_offset");
    WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //seq_scaled_ref_layer_top_offset
    kpPos->iTopOffset	= iCode;
    WELS_CHECK_SE_BOTH_WARNING (kpPos->iTopOffset, SUBSET_SPS_SEQ_SCALED_REF_LAYER_TOP_OFFSET_MIN,
                                SUBSET_SPS_SEQ_SCALED_REF_LAYER_TOP_OFFSET_MAX, "seq_scaled_ref_layer_top_offset");
    WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //seq_scaled_ref_layer_right_offset
    kpPos->iRightOffset	= iCode;
    WELS_CHECK_SE_BOTH_WARNING (kpPos->iRightOffset, SUBSET_SPS_SEQ_SCALED_REF_LAYER_RIGHT_OFFSET_MIN,
                                SUBSET_SPS_SEQ_SCALED_REF_LAYER_RIGHT_OFFSET_MAX, "seq_scaled_ref_layer_right_offset");
    WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //seq_scaled_ref_layer_bottom_offset
    kpPos->iBottomOffset = iCode;
    WELS_CHECK_SE_BOTH_WARNING (kpPos->iBottomOffset, SUBSET_SPS_SEQ_SCALED_REF_LAYER_BOTTOM_OFFSET_MIN,
                                SUBSET_SPS_SEQ_SCALED_REF_LAYER_BOTTOM_OFFSET_MAX, "seq_scaled_ref_layer_bottom_offset");
  }

  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //seq_tcoeff_level_prediction_flag
  pExt->bSeqTCoeffLevelPredFlag	= !!uiCode;
  pExt->bAdaptiveTCoeffLevelPredFlag	= false;
  if (pExt->bSeqTCoeffLevelPredFlag) {
    WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //adaptive_tcoeff_level_prediction_flag
    pExt->bAdaptiveTCoeffLevelPredFlag	= !!uiCode;
  }
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //slice_header_restriction_flag
  pExt->bSliceHeaderRestrictionFlag	= !!uiCode;



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

const SLevelLimits* GetLevelLimits (int32_t iLevelIdx, bool bConstraint3) {
  switch (iLevelIdx) {
  case 10:
    return &g_kSLevelLimits[0];
  case 11:
    if (bConstraint3)
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

#define  SPS_LOG2_MAX_FRAME_NUM_MINUS4_MAX 12
#define  SPS_LOG2_MAX_PIC_ORDER_CNT_LSB_MINUS4_MAX 12
#define  SPS_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE_MAX 255
#define  SPS_MAX_NUM_REF_FRAMES_MAX 16
#define  PPS_PIC_INIT_QP_QS_MIN 0
#define  PPS_PIC_INIT_QP_QS_MAX 51
#define  PPS_CHROMA_QP_INDEX_OFFSET_MIN -12
#define  PPS_CHROMA_QP_INDEX_OFFSET_MAX 12

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
  uint32_t uiCode;
  int32_t iCode;
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
  }

  WELS_READ_VERIFY (BsGetBits (pBs, 8, &uiCode)); //profile_idc
  uiProfileIdc	= uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //constraint_set0_flag
  bConstraintSetFlags[0]	= !!uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //constraint_set1_flag
  bConstraintSetFlags[1]	= !!uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //constraint_set2_flag
  bConstraintSetFlags[2]	= !!uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //constraint_set3_flag
  bConstraintSetFlags[3]	= !!uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //constraint_set4_flag
  bConstraintSetFlags[4]	= !!uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //constraint_set5_flag
  bConstraintSetFlags[5]	= !!uiCode;
  WELS_READ_VERIFY (BsGetBits (pBs, 2, &uiCode)); // reserved_zero_2bits, equal to 0
  WELS_READ_VERIFY (BsGetBits (pBs, 8, &uiCode)); // level_idc
  uiLevelIdc	= uiCode;
  WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //seq_parameter_set_id
  if (uiCode >= MAX_SPS_COUNT) {	// Modified to check invalid negative iSpsId, 12/1/2009
    WelsLog (pCtx, WELS_LOG_WARNING, " iSpsId is out of range! \n");
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_SPS_ID_OVERFLOW);
  }
  iSpsId		= uiCode;

  if (kbUseSubsetFlag) {
    pSubsetSps	= &pCtx->sSubsetSpsBuffer[iSpsId];
    pSps		= &pSubsetSps->sSps;
    pCtx->bSubspsAvailFlags[iSpsId]	= true; // added for EC, 10/28/2009
  } else {
    pSps = &pCtx->sSpsBuffer[iSpsId];
    pCtx->bSpsAvailFlags[iSpsId] = true; // added for EC, 10/28/2009
  }
  const SLevelLimits* pSLevelLimits = GetLevelLimits (uiLevelIdc, bConstraintSetFlags[3]);
  if (NULL == pSLevelLimits) {
    WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): level_idx (%d).\n", uiLevelIdc);
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_NON_BASELINE);
  } else pSps->pSLevelLimits = pSLevelLimits;
  // syntax elements in default
  pSps->uiChromaFormatIdc	= 1;

  pSps->uiProfileIdc	= uiProfileIdc;
  pSps->uiLevelIdc	= uiLevelIdc;
  pSps->iSpsId		= iSpsId;

  if (PRO_SCALABLE_BASELINE == uiProfileIdc || PRO_SCALABLE_HIGH == uiProfileIdc ||
      PRO_HIGH == uiProfileIdc || PRO_HIGH10 == uiProfileIdc ||
      PRO_HIGH422 == uiProfileIdc || PRO_HIGH444 == uiProfileIdc ||
      PRO_CAVLC444 == uiProfileIdc || 44 == uiProfileIdc) {

    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //chroma_format_idc
    pSps->uiChromaFormatIdc = uiCode;
    if (pSps->uiChromaFormatIdc != 1) {
      WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): chroma_format_idc (%d) = 1 supported.\n", pSps->uiChromaFormatIdc);
      return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_NON_BASELINE);
    }
    pSps->uiChromaArrayType = pSps->uiChromaFormatIdc;
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //bit_depth_luma_minus8
    if (uiCode != 0) {
      WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): bit_depth_luma (%d) Only 8 bit supported.\n", 8 + uiCode);
      return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_NON_BASELINE);
    }
    pSps->uiBitDepthLuma		= 8;

    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //bit_depth_chroma_minus8
    if (uiCode != 0) {
      WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): bit_depth_chroma (%d). Only 8 bit supported.\n", 8 + uiCode);
      return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_NON_BASELINE);
    }
    pSps->uiBitDepthChroma	= 8;

    WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //qpprime_y_zero_transform_bypass_flag
    pSps->bQpPrimeYZeroTransfBypassFlag	= !!uiCode;
    WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //seq_scaling_matrix_present_flag
    pSps->bSeqScalingMatrixPresentFlag	= !!uiCode;

    if (pSps->bSeqScalingMatrixPresentFlag) {	// For high profile, it is not used in current application. FIXME
      WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): seq_scaling_matrix_present_flag (%d). Feature not supported.\n",
               pSps->bSeqScalingMatrixPresentFlag);
      return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_NON_BASELINE);
    }
  }
  WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //log2_max_frame_num_minus4
  WELS_CHECK_SE_UPPER_ERROR (uiCode, SPS_LOG2_MAX_FRAME_NUM_MINUS4_MAX, "log2_max_frame_num_minus4",
                             GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_LOG2_MAX_FRAME_NUM_MINUS4));
  pSps->uiLog2MaxFrameNum	= LOG2_MAX_FRAME_NUM_OFFSET + uiCode;
  WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //pic_order_cnt_type
  pSps->uiPocType			= uiCode;

  if (0 == pSps->uiPocType) {
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //log2_max_pic_order_cnt_lsb_minus4
    // log2_max_pic_order_cnt_lsb_minus4 should be in range 0 to 12, inclusive. (sec. 7.4.3)
    WELS_CHECK_SE_UPPER_ERROR (uiCode, SPS_LOG2_MAX_PIC_ORDER_CNT_LSB_MINUS4_MAX, "log2_max_pic_order_cnt_lsb_minus4",
                               GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_LOG2_MAX_PIC_ORDER_CNT_LSB_MINUS4));
    pSps->iLog2MaxPocLsb	= LOG2_MAX_PIC_ORDER_CNT_LSB_OFFSET + uiCode;	// log2_max_pic_order_cnt_lsb_minus4

  } else if (1 == pSps->uiPocType) {
    int32_t i;
    WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //delta_pic_order_always_zero_flag
    pSps->bDeltaPicOrderAlwaysZeroFlag	= !!uiCode;
    WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //offset_for_non_ref_pic
    pSps->iOffsetForNonRefPic			= iCode;
    WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //offset_for_top_to_bottom_field
    pSps->iOffsetForTopToBottomField	= iCode;
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //num_ref_frames_in_pic_order_cnt_cycle
    WELS_CHECK_SE_UPPER_ERROR (uiCode, SPS_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE_MAX,
                               "num_ref_frames_in_pic_order_cnt_cycle", GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS,
                                   ERR_INFO_INVALID_NUM_REF_FRAME_IN_PIC_ORDER_CNT_CYCLE));
    pSps->iNumRefFramesInPocCycle		= uiCode;
    for (i = 0; i < pSps->iNumRefFramesInPocCycle; i++) {
      WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //offset_for_ref_frame[ i ]
      pSps->iOffsetForRefFrame[ i ]	= iCode;
    }
  }
  if (pSps->uiPocType > 2) {
    WelsLog (pCtx, WELS_LOG_WARNING, " illegal pic_order_cnt_type: %d ! \n", pSps->uiPocType);
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_POC_TYPE);
  }

  WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //max_num_ref_frames
  pSps->iNumRefFrames	= uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //gaps_in_frame_num_value_allowed_flag
  pSps->bGapsInFrameNumValueAllowedFlag	= !!uiCode;
  WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //pic_width_in_mbs_minus1
  pSps->iMbWidth		= PIC_WIDTH_IN_MBS_OFFSET + uiCode;
  if (pSps->iMbWidth > MAX_MB_SIZE) {
    WelsLog (pCtx, WELS_LOG_ERROR, "pic_width_in_mbs(%d) exceeds the maximum allowed!\n", pSps->iMbWidth);
    return  GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_MAX_MB_SIZE);
  }
  if (((uint64_t)pSps->iMbWidth * (uint64_t)pSps->iMbWidth) > (8 * pSLevelLimits->iMaxFS)) {
    WelsLog (pCtx, WELS_LOG_WARNING, " the pic_width_in_mbs exceeds the level limits!\n");
  }
  WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //pic_height_in_map_units_minus1
  pSps->iMbHeight		= PIC_HEIGHT_IN_MAP_UNITS_OFFSET + uiCode;
  if (pSps->iMbHeight > MAX_MB_SIZE) {
    WelsLog (pCtx, WELS_LOG_ERROR, "pic_height_in_mbs(%d) exceeds the maximum allowed!\n", pSps->iMbHeight);
    return  GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_MAX_MB_SIZE);
  }
  if (((uint64_t)pSps->iMbHeight * (uint64_t)pSps->iMbHeight) > (8 * pSLevelLimits->iMaxFS)) {
    WelsLog (pCtx, WELS_LOG_WARNING, " the pic_height_in_mbs exceeds the level limits!\n");
  }
  uint32_t uiTmp32 = pSps->iMbWidth * pSps->iMbHeight;
  if (uiTmp32 > (uint32_t)pSLevelLimits->iMaxFS) {
    WelsLog (pCtx, WELS_LOG_WARNING, " the total count of mb exceeds the level limits!\n");
  }
  pSps->uiTotalMbCount	= uiTmp32;
  WELS_CHECK_SE_UPPER_ERROR (pSps->iNumRefFrames, SPS_MAX_NUM_REF_FRAMES_MAX, "max_num_ref_frames",
                             GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_MAX_NUM_REF_FRAMES));
  // here we check max_num_ref_frames
  uint32_t uiMaxDpbMbs = pSLevelLimits->iMaxDPBMbs;
  uint32_t uiMaxDpbFrames = uiMaxDpbMbs / pSps->uiTotalMbCount;
  if (uiMaxDpbFrames > SPS_MAX_NUM_REF_FRAMES_MAX)
    uiMaxDpbFrames = SPS_MAX_NUM_REF_FRAMES_MAX;
  if ((uint32_t)pSps->iNumRefFrames > uiMaxDpbFrames) {
    WelsLog (pCtx, WELS_LOG_WARNING, " max_num_ref_frames exceeds level limits!\n");
  }
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //frame_mbs_only_flag
  pSps->bFrameMbsOnlyFlag	= !!uiCode;
  if (!pSps->bFrameMbsOnlyFlag) {
    WelsLog (pCtx, WELS_LOG_WARNING, "ParseSps(): frame_mbs_only_flag (%d) not supported.\n", pSps->bFrameMbsOnlyFlag);
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_MBAFF);
  }
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //direct_8x8_inference_flag
  pSps->bDirect8x8InferenceFlag	= !!uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //frame_cropping_flag
  pSps->bFrameCroppingFlag		= !!uiCode;
  if (pSps->bFrameCroppingFlag) {
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //frame_crop_left_offset
    pSps->sFrameCrop.iLeftOffset	= uiCode;
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //frame_crop_right_offset
    pSps->sFrameCrop.iRightOffset	= uiCode;
    if ((pSps->sFrameCrop.iLeftOffset + pSps->sFrameCrop.iRightOffset) > ((int32_t)pSps->iMbWidth * 16 / 2)) {
      WelsLog (pCtx, WELS_LOG_WARNING, "frame_crop_left_offset + frame_crop_right_offset exceeds limits!\n");
    }
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //frame_crop_top_offset
    pSps->sFrameCrop.iTopOffset		= uiCode;
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //frame_crop_bottom_offset
    pSps->sFrameCrop.iBottomOffset	= uiCode;
    if ((pSps->sFrameCrop.iTopOffset + pSps->sFrameCrop.iBottomOffset) > ((int32_t)pSps->iMbHeight * 16 / 2)) {
      WelsLog (pCtx, WELS_LOG_WARNING, "frame_crop_top_offset + frame_crop_right_offset exceeds limits!\n");
    }
  } else {
    pSps->sFrameCrop.iLeftOffset	= 0;				// frame_crop_left_offset
    pSps->sFrameCrop.iRightOffset	= 0;				// frame_crop_right_offset
    pSps->sFrameCrop.iTopOffset		= 0;				// frame_crop_top_offset
    pSps->sFrameCrop.iBottomOffset	= 0;				// frame_crop_bottom_offset
  }
  WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //vui_parameters_present_flag
  pSps->bVuiParamPresentFlag			= !!uiCode;

  // Check if SPS SVC extension applicated
  if (kbUseSubsetFlag && (PRO_SCALABLE_BASELINE == uiProfileIdc || PRO_SCALABLE_HIGH == uiProfileIdc)) {
    if (DecodeSpsSvcExt (pCtx, pSubsetSps, pBs) != ERR_NONE) {
      return -1;
    }

    WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //svc_vui_parameters_present_flag
    pSubsetSps->bSvcVuiParamPresentFlag = !!uiCode;
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
  uint32_t uiCode;
  int32_t iCode;

  WELS_READ_VERIFY (BsGetUe (pBsAux, &uiCode)); //pic_parameter_set_id
  uiPpsId = uiCode;
  if (uiPpsId >= MAX_PPS_COUNT) {
    return ERR_INFO_PPS_ID_OVERFLOW;
  }

  pPps = &pPpsList[uiPpsId];

  pPps->iPpsId = uiPpsId;
  WELS_READ_VERIFY (BsGetUe (pBsAux, &uiCode)); //seq_parameter_set_id
  pPps->iSpsId = uiCode;

  if (pPps->iSpsId >= MAX_SPS_COUNT) {
    return ERR_INFO_SPS_ID_OVERFLOW;
  }

  WELS_READ_VERIFY (BsGetOneBit (pBsAux, &uiCode)); //entropy_coding_mode_flag
  pPps->bEntropyCodingModeFlag = !!uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBsAux, &uiCode)); //bottom_field_pic_order_in_frame_present_flag
  pPps->bPicOrderPresentFlag   = !!uiCode;

  WELS_READ_VERIFY (BsGetUe (pBsAux, &uiCode)); //num_slice_groups_minus1
  pPps->uiNumSliceGroups = NUM_SLICE_GROUPS_OFFSET + uiCode;

  if (pPps->uiNumSliceGroups > MAX_SLICEGROUP_IDS) {
    return ERR_INFO_INVALID_SLICEGROUP;
  }

  if (pPps->uiNumSliceGroups > 1) {
    WELS_READ_VERIFY (BsGetUe (pBsAux, &uiCode)); //slice_group_map_type
    pPps->uiSliceGroupMapType = uiCode;
    if (pPps->uiSliceGroupMapType > 1) {
      WelsLog (pCtx, WELS_LOG_WARNING, "ParsePps(): slice_group_map_type (%d): support only 0,1.\n",
               pPps->uiSliceGroupMapType);
      return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_FMOTYPE);
    }

    switch (pPps->uiSliceGroupMapType) {
    case 0:
      for (iTmp = 0; iTmp < pPps->uiNumSliceGroups; iTmp++) {
        WELS_READ_VERIFY (BsGetUe (pBsAux, &uiCode)); //run_length_minus1[ iGroup ]
        pPps->uiRunLength[iTmp] = RUN_LENGTH_OFFSET + uiCode;
      }
      break;
    default:
      break;
    }
  }

  WELS_READ_VERIFY (BsGetUe (pBsAux, &uiCode)); //num_ref_idx_l0_default_active_minus1
  pPps->uiNumRefIdxL0Active = NUM_REF_IDX_L0_DEFAULT_ACTIVE_OFFSET + uiCode;
  WELS_READ_VERIFY (BsGetUe (pBsAux, &uiCode)); //num_ref_idx_l1_default_active_minus1
  pPps->uiNumRefIdxL1Active = NUM_REF_IDX_L1_DEFAULT_ACTIVE_OFFSET + uiCode;

  if (pPps->uiNumRefIdxL0Active > MAX_REF_PIC_COUNT ||
      pPps->uiNumRefIdxL1Active > MAX_REF_PIC_COUNT) {
    return ERR_INFO_REF_COUNT_OVERFLOW;
  }

  WELS_READ_VERIFY (BsGetOneBit (pBsAux, &uiCode)); //weighted_pred_flag
  pPps->bWeightedPredFlag  = !!uiCode;
  WELS_READ_VERIFY (BsGetBits (pBsAux, 2, &uiCode)); //weighted_bipred_idc
  pPps->uiWeightedBipredIdc = uiCode;
  if (pPps->bWeightedPredFlag || pPps->uiWeightedBipredIdc != 0) {
    WelsLog (pCtx, WELS_LOG_WARNING, "ParsePps(): weighted_pred_flag (%d) weighted_bipred_idc (%d) neither supported.\n",
             pPps->bWeightedPredFlag, pPps->uiWeightedBipredIdc);
    return GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_UNSUPPORTED_WP);
  }

  WELS_READ_VERIFY (BsGetSe (pBsAux, &iCode)); //pic_init_qp_minus26
  pPps->iPicInitQp = PIC_INIT_QP_OFFSET + iCode;
  WELS_CHECK_SE_BOTH_ERROR (pPps->iPicInitQp, PPS_PIC_INIT_QP_QS_MIN, PPS_PIC_INIT_QP_QS_MAX, "pic_init_qp_minus26 + 26",
                            GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_PIC_INIT_QP));
  WELS_READ_VERIFY (BsGetSe (pBsAux, &iCode)); //pic_init_qs_minus26
  pPps->iPicInitQs = PIC_INIT_QS_OFFSET + iCode;
  WELS_CHECK_SE_BOTH_ERROR (pPps->iPicInitQs, PPS_PIC_INIT_QP_QS_MIN, PPS_PIC_INIT_QP_QS_MAX, "pic_init_qs_minus26 + 26",
                            GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_PIC_INIT_QS));
  WELS_READ_VERIFY (BsGetSe (pBsAux, &iCode)); //chroma_qp_index_offset
  pPps->iChromaQpIndexOffset                  = iCode;
  WELS_CHECK_SE_BOTH_ERROR (pPps->iChromaQpIndexOffset, PPS_CHROMA_QP_INDEX_OFFSET_MIN, PPS_CHROMA_QP_INDEX_OFFSET_MAX,
                            "chroma_qp_index_offset", GENERATE_ERROR_NO (ERR_LEVEL_PARAM_SETS, ERR_INFO_INVALID_CHROMA_QP_INDEX_OFFSET));
  WELS_READ_VERIFY (BsGetOneBit (pBsAux, &uiCode)); //deblocking_filter_control_present_flag
  pPps->bDeblockingFilterControlPresentFlag   = !!uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBsAux, &uiCode)); //constrained_intra_pred_flag
  pPps->bConstainedIntraPredFlag              = !!uiCode;
  WELS_READ_VERIFY (BsGetOneBit (pBsAux, &uiCode)); //redundant_pic_cnt_present_flag
  pPps->bRedundantPicCntPresentFlag           = !!uiCode;

  pCtx->bPpsAvailFlags[uiPpsId] = true; // added for EC, 10/28/2009

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
int32_t ParseSei (void* pSei, PBitStringAux pBsAux) {	// reserved Sei_Msg type


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
