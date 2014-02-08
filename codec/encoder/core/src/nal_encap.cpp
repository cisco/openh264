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
 * \file	nal_encap.c
 *
 * \brief	NAL pRawNal pData encapsulation
 *
 * \date	5/25/2009	Created
 *
 *************************************************************************************/
#include "nal_encap.h"
#include "svc_enc_golomb.h"
#include "ls_defines.h"
namespace WelsSVCEnc {
/*!
 * \brief	load an initialize NAL pRawNal pData
 */
void WelsLoadNal (SWelsEncoderOutput* pEncoderOuput, const int32_t/*EWelsNalUnitType*/ kiType,
                  const int32_t/*EWelsNalRefIdc*/ kiNalRefIdc) {
  SWelsEncoderOutput* pWelsEncoderOuput	= pEncoderOuput;
  SWelsNalRaw* pRawNal			= &pWelsEncoderOuput->sNalList[ pWelsEncoderOuput->iNalIndex ];
  SNalUnitHeader* sNalHeader	= &pRawNal->sNalExt.sNalHeader;
  const int32_t kiStartPos		= (BsGetBitsPos (&pWelsEncoderOuput->sBsWrite) >> 3);

  sNalHeader->eNalUnitType	= (EWelsNalUnitType)kiType;
  sNalHeader->uiNalRefIdc		= (EWelsNalRefIdc)kiNalRefIdc;
  sNalHeader->uiForbiddenZeroBit	= 0;

  pRawNal->pRawData		= &pWelsEncoderOuput->pBsBuffer[kiStartPos];
  pRawNal->iPayloadSize	= 0;
}

/*!
 * \brief	unload pRawNal NAL
 */
void WelsUnloadNal (SWelsEncoderOutput* pEncoderOuput) {
  SWelsEncoderOutput*	pWelsEncoderOuput = pEncoderOuput;
  int32_t*	pIdx			= &pWelsEncoderOuput->iNalIndex;
  SWelsNalRaw* pRawNal		= &pWelsEncoderOuput->sNalList[ *pIdx ];
  const int32_t kiEndPos		= (BsGetBitsPos (&pWelsEncoderOuput->sBsWrite) >> 3);

  /* count payload size of pRawNal NAL */
  pRawNal->iPayloadSize	= &pWelsEncoderOuput->pBsBuffer[kiEndPos] - pRawNal->pRawData;

  ++ (*pIdx);
}

/*!
 * \brief	load an initialize NAL pRawNal pData
 */
void WelsLoadNalForSlice (SWelsSliceBs* pSliceBsIn, const int32_t/*EWelsNalUnitType*/ kiType,
                          const int32_t/*EWelsNalRefIdc*/ kiNalRefIdc) {
  SWelsSliceBs* pSliceBs		    = pSliceBsIn;
  SWelsNalRaw* pRawNal		= &pSliceBs->sNalList[ pSliceBs->iNalIndex ];
  SNalUnitHeader* sNalHeader	= &pRawNal->sNalExt.sNalHeader;
  SBitStringAux* pBitStringAux	= &pSliceBs->sBsWrite;
  const int32_t kiStartPos		    = (BsGetBitsPos (pBitStringAux) >> 3);

  sNalHeader->eNalUnitType	= (EWelsNalUnitType)kiType;
  sNalHeader->uiNalRefIdc		= (EWelsNalRefIdc)kiNalRefIdc;
  sNalHeader->uiForbiddenZeroBit	= 0;

  pRawNal->pRawData		= &pSliceBs->pBsBuffer[kiStartPos];
  pRawNal->iPayloadSize	= 0;
}

/*!
 * \brief	unload pRawNal NAL
 */
void WelsUnloadNalForSlice (SWelsSliceBs* pSliceBsIn) {
  SWelsSliceBs* pSliceBs	        = pSliceBsIn;
  int32_t*	pIdx			            = &pSliceBs->iNalIndex;
  SWelsNalRaw* pRawNal		= &pSliceBs->sNalList[ *pIdx ];
  SBitStringAux* pBitStringAux	= &pSliceBs->sBsWrite;
  const int32_t kiEndPos		        = (BsGetBitsPos (pBitStringAux) >> 3);

  /* count payload size of pRawNal NAL */
  pRawNal->iPayloadSize	= &pSliceBs->pBsBuffer[kiEndPos] - pRawNal->pRawData;

  ++ (*pIdx);
}

/*!
 * \brief	encode NAL with emulation forbidden three bytes checking
 * \param	pDst			pDst NAL pData
 * \param	pDstLen		length of pDst NAL output
 * \param	annexeb		annexeb flag
 * \param	pRawNal			pRawNal NAL pData
 * \return	length of pDst NAL
 */
int32_t WelsEncodeNal (SWelsNalRaw* pRawNal, void* pDst, int32_t* pDstLen) {
  uint8_t* pDstStart	    = (uint8_t*)pDst;
  uint8_t* pDstPointer	= pDstStart;
  uint8_t* pSrcPointer	= pRawNal->pRawData;
  uint8_t* pSrcEnd		= pRawNal->pRawData + pRawNal->iPayloadSize;
  int32_t iZeroCount		= 0;
  int32_t iNalLength		= 0;

  static const uint8_t kuiStartCodePrefix[4] = { 0, 0, 0, 1 };
  ST32 (pDstPointer, LD32 (&kuiStartCodePrefix[0]));
  pDstPointer += 4;

  /* NAL Unit Header */
  *pDstPointer++	= (pRawNal->sNalExt.sNalHeader.uiNalRefIdc << 5) | (pRawNal->sNalExt.sNalHeader.eNalUnitType & 0x1f);

  while (pSrcPointer < pSrcEnd) {
    if (iZeroCount == 2 && *pSrcPointer <= 3) {
      *pDstPointer++	= 3;
      iZeroCount		= 0;
    }
    if (*pSrcPointer == 0) {
      ++ iZeroCount;
    } else {
      iZeroCount		= 0;
    }
    *pDstPointer++ = *pSrcPointer++;
  }

  /* count length of NAL Unit */
  iNalLength	= pDstPointer - pDstStart;
  if (NULL != pDstLen)
    *pDstLen	= iNalLength;

  return iNalLength;
}

/*!
 * \brief	encode a nal into a pBuffer for any type of NAL, involved WelsEncodeNal introduced in AVC
 *
 * \param	pDst			pDst NAL pData
 * \param	pDstLen		length of pDst NAL output
 * \param	annexeb		annexeb flag
 * \param	pRawNal			pRawNal NAL pData
 * \param	pNalHeaderExt	pointer of SNalUnitHeaderExt
 *
 * \return	length of pDst NAL
 */
int32_t WelsEncodeNalExt (SWelsNalRaw* pRawNal, void* pNalHeaderExt, void* pDst, int32_t* pDstLen) {
  SNalUnitHeaderExt* sNalExt	= (SNalUnitHeaderExt*)pNalHeaderExt;
  uint8_t* pDstStart				    = (uint8_t*)pDst;
  uint8_t* pDstPointer				= pDstStart;
  uint8_t* pSrcPointer				= pRawNal->pRawData;
  uint8_t* pSrcEnd					= pRawNal->pRawData + pRawNal->iPayloadSize;
  int32_t iZeroCount					= 0;
  int32_t iNalLength					= 0;

  if (pRawNal->sNalExt.sNalHeader.eNalUnitType != NAL_UNIT_PREFIX
      && pRawNal->sNalExt.sNalHeader.eNalUnitType != NAL_UNIT_CODED_SLICE_EXT) {
    return WelsEncodeNal (pRawNal, pDst, pDstLen);
  }

  /* FIXME this code doesn't check overflow */

  static const uint8_t kuiStartCodePrefixExt[4] = { 0, 0, 0, 1 };
  ST32 (pDstPointer, LD32 (&kuiStartCodePrefixExt[0]));
  pDstPointer += 4;

  /* NAL Unit Header */
  *pDstPointer++	= (pRawNal->sNalExt.sNalHeader.uiNalRefIdc << 5) | (pRawNal->sNalExt.sNalHeader.eNalUnitType & 0x1f);

  /* NAL UNIT Extension Header */
  *pDstPointer++ =	(0x80) |
                    (sNalExt->bIdrFlag << 6);

  *pDstPointer++ =	(0x80) |
                    (sNalExt->uiDependencyId << 4);

  *pDstPointer++ =	(sNalExt->uiTemporalId << 5) |
                    (sNalExt->bDiscardableFlag << 3) |
                    (0x07);

  while (pSrcPointer < pSrcEnd) {
    if (iZeroCount == 2 && *pSrcPointer <= 3) {
      *pDstPointer++	= 3;
      iZeroCount		= 0;
    }
    if (*pSrcPointer == 0) {
      ++ iZeroCount;
    } else {
      iZeroCount		= 0;
    }
    *pDstPointer++ = *pSrcPointer++;
  }

  /* count length of NAL Unit */
  iNalLength	= pDstPointer - pDstStart;
  if (NULL != pDstLen)
    *pDstLen	= iNalLength;

  return iNalLength;
}

/*!
 * \brief	write prefix nal
 */
int32_t WelsWriteSVCPrefixNal (SBitStringAux* pBitStringAux, const int32_t kiNalRefIdc,
                               const bool kbIdrFlag) {
  if (0 < kiNalRefIdc) {
    BsWriteOneBit (pBitStringAux, false/*bStoreRefBasePicFlag*/);
    BsWriteOneBit (pBitStringAux, false);
    BsRbspTrailingBits (pBitStringAux);
    BsFlush (pBitStringAux);
  }
  return 0;
}

} // namespace WelsSVCEnc
