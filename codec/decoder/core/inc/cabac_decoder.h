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
 * \file    cabac_decoder.h
 *
 * \brief   Interfaces introduced for cabac decoder
 *
 * \date    10/10/2014 Created
 *
 *************************************************************************************
 */
#ifndef WELS_CABAC_DECODER_H__
#define WELS_CABAC_DECODER_H__

#include "decoder_context.h"
#include "error_code.h"
#include "wels_common_defs.h"
namespace WelsDec {
static const uint8_t g_kRenormTable256[256] = {
  6, 6, 6, 6, 6, 6, 6, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4,
  3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1
};


#define WELS_CABAC_HALF    0x01FE
#define WELS_CABAC_QUARTER 0x0100

//1. CABAC context initialization
void WelsCabacGlobalInit (PWelsDecoderContext pCabacCtx);
void WelsCabacContextInit (PWelsDecoderContext  pCtx, uint8_t eSliceType, int32_t iCabacInitIdc, int32_t iQp);

//2. decoding Engine initialization
int32_t InitCabacDecEngineFromBS (PWelsCabacDecEngine pDecEngine, SBitStringAux* pBsAux);
void RestoreCabacDecEngineToBS (PWelsCabacDecEngine pDecEngine, SBitStringAux* pBsAux);
//3. actual decoding
int32_t Read32BitsCabac (PWelsCabacDecEngine pDecEngine, uint32_t& uiValue, int32_t& iNumBitsRead);
int32_t  DecodeTerminateCabac (PWelsCabacDecEngine pDecEngine, uint32_t& uiBinVal);

// DecodeBinCabac() and DecodeBypassCabac() run once per bin, i.e. many times per macroblock, and
// are called from parse_mb_syn_cabac.cpp. They are defined inline here so the compiler can inline
// them into the syntax element parsers instead of paying for a cross-file call for every bin; only
// the rare bitstream refill (Read32BitsCabac) stays out of line.
static inline int32_t DecodeBinCabac (PWelsCabacDecEngine pDecEngine, PWelsCabacCtx pBinCtx, uint32_t& uiBinVal) {
  int32_t iErrorInfo = ERR_NONE;
  uint32_t uiState = pBinCtx->uiState;
  uiBinVal = pBinCtx->uiMPS;
  uint64_t uiOffset = pDecEngine->uiOffset;
  uint64_t uiRange = pDecEngine->uiRange;

  int32_t iRenorm = 1;
  uint32_t uiRangeLPS = g_kuiCabacRangeLps[uiState][ (uiRange >> 6) & 0x03];
  uiRange -= uiRangeLPS;
  if (uiOffset >= (uiRange << pDecEngine->iBitsLeft)) { //LPS
    uiOffset -= (uiRange << pDecEngine->iBitsLeft);
    uiBinVal ^= 0x0001;
    if (!uiState)
      pBinCtx->uiMPS ^= 0x01;
    pBinCtx->uiState = g_kuiStateTransTable[uiState][0];
    iRenorm = g_kRenormTable256[uiRangeLPS];
    uiRange = (uiRangeLPS << iRenorm);
  } else {  //MPS
    pBinCtx->uiState = g_kuiStateTransTable[uiState][1];
    if (uiRange >= WELS_CABAC_QUARTER) {
      pDecEngine->uiRange = uiRange;
      return ERR_NONE;
    } else {
      uiRange <<= 1;
    }
  }
  //Renorm
  pDecEngine->uiRange = uiRange;
  pDecEngine->iBitsLeft -= iRenorm;
  if (pDecEngine->iBitsLeft > 0) {
    pDecEngine->uiOffset = uiOffset;
    return ERR_NONE;
  }
  uint32_t uiVal = 0;
  int32_t iNumBitsRead = 0;
  iErrorInfo = Read32BitsCabac (pDecEngine, uiVal, iNumBitsRead);
  pDecEngine->uiOffset = (uiOffset << iNumBitsRead) | uiVal;
  pDecEngine->iBitsLeft += iNumBitsRead;
  if (iErrorInfo && pDecEngine->iBitsLeft < 0) {
    return iErrorInfo;
  }
  return ERR_NONE;
}

static inline int32_t DecodeBypassCabac (PWelsCabacDecEngine pDecEngine, uint32_t& uiBinVal) {
  int32_t iErrorInfo = ERR_NONE;
  int32_t iBitsLeft = pDecEngine->iBitsLeft;
  uint64_t uiOffset = pDecEngine->uiOffset;
  uint64_t uiRangeValue;


  if (iBitsLeft <= 0) {
    uint32_t uiVal = 0;
    int32_t iNumBitsRead = 0;
    iErrorInfo = Read32BitsCabac (pDecEngine, uiVal, iNumBitsRead);
    uiOffset = (uiOffset << iNumBitsRead) | uiVal;
    iBitsLeft = iNumBitsRead;
    if (iErrorInfo && iBitsLeft == 0) {
      return iErrorInfo;
    }
  }
  iBitsLeft--;
  uiRangeValue = (pDecEngine->uiRange << iBitsLeft);
  if (uiOffset >= uiRangeValue) {
    pDecEngine->iBitsLeft = iBitsLeft;
    pDecEngine->uiOffset = uiOffset - uiRangeValue;
    uiBinVal = 1;
    return ERR_NONE;
  }
  pDecEngine->iBitsLeft = iBitsLeft;
  pDecEngine->uiOffset = uiOffset;
  uiBinVal = 0;
  return ERR_NONE;
}

//4. unary parsing
int32_t DecodeUnaryBinCabac (PWelsCabacDecEngine pDecEngine, PWelsCabacCtx pBinCtx, int32_t iCtxOffset,
                             uint32_t& uiSymVal);

//5. EXGk parsing
int32_t DecodeExpBypassCabac (PWelsCabacDecEngine pDecEngine, int32_t iCount, uint32_t& uiSymVal);
uint32_t DecodeUEGLevelCabac (PWelsCabacDecEngine pDecEngine, PWelsCabacCtx pBinCtx, uint32_t& uiBinVal);
int32_t DecodeUEGMvCabac (PWelsCabacDecEngine pDecEngine, PWelsCabacCtx pBinCtx, uint32_t iMaxC,  uint32_t& uiCode);

#define WELS_CABAC_FALSE_RETURN(iErrorInfo) \
if(iErrorInfo) { \
  return iErrorInfo; \
}
}
#endif
