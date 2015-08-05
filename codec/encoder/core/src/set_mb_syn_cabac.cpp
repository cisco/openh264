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
 * \file    set_mb_syn_cabac.cpp
 *
 * \brief   cabac coding engine
 *
 * \date    10/11/2014 Created
 *
 *************************************************************************************
 */
#include <string.h>
#include "typedefs.h"
#include "macros.h"
#include "set_mb_syn_cabac.h"
#include "encoder.h"

namespace WelsEnc {


void WelsCabacInit (void* pCtx) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  for (int32_t iModel = 0; iModel < 4; iModel++) {
    for (int32_t iQp = 0; iQp <= WELS_QP_MAX; iQp++)
      for (int32_t iIdx = 0; iIdx < WELS_CONTEXT_COUNT; iIdx++) {
        int32_t m               = g_kiCabacGlobalContextIdx[iIdx][iModel][0];
        int32_t n               = g_kiCabacGlobalContextIdx[iIdx][iModel][1];
        int32_t iPreCtxState    = WELS_CLIP3 ((((m * iQp) >> 4) + n), 1, 126);
        uint8_t uiValMps         = 0;
        uint8_t uiStateIdx       = 0;
        if (iPreCtxState <= 63) {
          uiStateIdx = 63 - iPreCtxState;
          uiValMps = 0;
        } else {
          uiStateIdx = iPreCtxState - 64;
          uiValMps = 1;
        }
        pEncCtx->sWelsCabacContexts[iModel][iQp][iIdx].m_uiState = uiStateIdx;
        pEncCtx->sWelsCabacContexts[iModel][iQp][iIdx].m_uiValMps = uiValMps;
      }
  }
}

void WelsCabacContextInit (void* pCtx, SCabacCtx* pCbCtx, int32_t iModel) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  int32_t iIdx =  pEncCtx->eSliceType == WelsCommon::I_SLICE ? 0 : iModel + 1;
  int32_t iQp = pEncCtx->iGlobalQp;
  memcpy (pCbCtx->m_sStateCtx, pEncCtx->sWelsCabacContexts[iIdx][iQp],
          WELS_CONTEXT_COUNT * sizeof (SStateCtx));
}

void  WelsCabacEncodeInit (SCabacCtx* pCbCtx, uint8_t* pBuf,  uint8_t* pEnd) {
  pCbCtx->m_uiLow     = 0;
  pCbCtx->m_uiRange   = 510;
  pCbCtx->m_iBitsOutstanding = 0;
  pCbCtx->m_uData = 0;
  pCbCtx->m_uiBitsUsed = 0;
  pCbCtx->m_iFirstFlag = 1;
  pCbCtx->m_pBufStart = pBuf;
  pCbCtx->m_pBufEnd = pEnd;
  pCbCtx->m_pBufCur = pBuf;
  pCbCtx->m_uiBinCountsInNalUnits = 0;
}

void WelsCabacPutBit (SCabacCtx* pCbCtx, uint32_t iValue) {
  if (pCbCtx->m_iFirstFlag != 0) {
    pCbCtx->m_iFirstFlag = 0;
  } else {
    pCbCtx->m_uData = (pCbCtx->m_uData << 1) | iValue;
    pCbCtx->m_uiBitsUsed++;
  }
  if (pCbCtx->m_iBitsOutstanding == 0) {
    while (pCbCtx->m_uiBitsUsed >= 8) {
      pCbCtx->m_uiBitsUsed -= 8;
      uint32_t uiByte = pCbCtx->m_uData >> (pCbCtx->m_uiBitsUsed);
      if (pCbCtx->m_uiBitsUsed == 0)
        pCbCtx->m_uData = 0;
      else
        pCbCtx->m_uData &= (uint32_t) ((0xFFFFFFFF) >> (32 - pCbCtx->m_uiBitsUsed));
      *pCbCtx->m_pBufCur ++ = uiByte;
    }
  } else {

    while (pCbCtx->m_iBitsOutstanding > 0) {
      pCbCtx->m_uData = (pCbCtx->m_uData << 1) | (1 - iValue);
      pCbCtx->m_iBitsOutstanding--;
      pCbCtx->m_uiBitsUsed++;
      while (pCbCtx->m_uiBitsUsed >= 8) {
        pCbCtx->m_uiBitsUsed -= 8;
        uint32_t uiByte = pCbCtx->m_uData >> (pCbCtx->m_uiBitsUsed);
        if (pCbCtx->m_uiBitsUsed == 0)
          pCbCtx->m_uData = 0;
        else
          pCbCtx->m_uData &= (uint32_t) ((0xFFFFFFFF) >> (32 - pCbCtx->m_uiBitsUsed));
        *pCbCtx->m_pBufCur ++ = uiByte;
      }
    }
  }
}
void WelsCabacEncodeRenorm (SCabacCtx* pCbCtx) {
  while (pCbCtx->m_uiRange < 256) {
    if (pCbCtx->m_uiLow < 256) {
      WelsCabacPutBit (pCbCtx, 0);
    } else {
      if (pCbCtx->m_uiLow >= 512) {
        pCbCtx->m_uiLow -= 512;
        WelsCabacPutBit (pCbCtx, 1);
      } else {
        pCbCtx->m_uiLow -= 256;
        pCbCtx->m_iBitsOutstanding++;
      }
    }
    pCbCtx->m_uiRange <<= 1;
    pCbCtx->m_uiLow <<= 1;
  }
}
void WelsCabacEncodeDecision (SCabacCtx* pCbCtx, int32_t iCtx, uint32_t uiBin) {
  uint8_t uiState = pCbCtx->m_sStateCtx[iCtx].m_uiState;
  uint8_t uiValMps = pCbCtx->m_sStateCtx[iCtx].m_uiValMps;
  uint32_t uiRangeLps = g_kuiCabacRangeLps[uiState][ (pCbCtx->m_uiRange >> 6) & 3];

  pCbCtx->m_uiRange -= uiRangeLps;
  if (uiBin != uiValMps) { //LPS
    pCbCtx->m_uiLow += pCbCtx->m_uiRange;
    pCbCtx->m_uiRange = uiRangeLps;
    if (uiState == 0)
      uiValMps = 1 - uiValMps;
    pCbCtx->m_sStateCtx[iCtx].m_uiState = g_kuiStateTransTable[uiState][0];
    pCbCtx->m_sStateCtx[iCtx].m_uiValMps = uiValMps;
  } else {
    pCbCtx->m_sStateCtx[iCtx].m_uiState = g_kuiStateTransTable[uiState][1];
  }
  WelsCabacEncodeRenorm (pCbCtx);
  pCbCtx->m_uiBinCountsInNalUnits++;
}

void WelsCabacEncodeBypassOne (SCabacCtx* pCbCtx, uint32_t uiBin) {
  pCbCtx->m_uiLow <<= 1;
  if (uiBin) {
    pCbCtx->m_uiLow += pCbCtx->m_uiRange;
  }
  if (pCbCtx->m_uiLow >= 1024) {
    WelsCabacPutBit (pCbCtx, 1);
    pCbCtx->m_uiLow -= 1024;
  } else {
    if (pCbCtx->m_uiLow < 512)
      WelsCabacPutBit (pCbCtx, 0);
    else {
      pCbCtx->m_uiLow -= 512;
      pCbCtx->m_iBitsOutstanding++;
    }
  }
  pCbCtx->m_uiBinCountsInNalUnits++;
}
void WelsCabacEncodeTerminate (SCabacCtx* pCbCtx, uint32_t uiBin) {
  pCbCtx->m_uiRange -= 2;
  if (uiBin) {
    pCbCtx->m_uiLow  += pCbCtx->m_uiRange;
    pCbCtx->m_uiRange = 2;
    WelsCabacEncodeRenorm (pCbCtx);
    WelsCabacPutBit (pCbCtx, ((pCbCtx->m_uiLow >> 9) & 1));
    int32_t iLastTwoBits = (((pCbCtx->m_uiLow >> 7) & 3) | 1);
    pCbCtx->m_uData = (pCbCtx->m_uData << 2) | iLastTwoBits;
    pCbCtx->m_uiBitsUsed += 2;
  } else {
    WelsCabacEncodeRenorm (pCbCtx);
  }
  pCbCtx->m_uiBinCountsInNalUnits++;
}
void WelsCabacEncodeUeBypass (SCabacCtx* pCbCtx, int32_t iExpBits, uint32_t uiVal) {
  int32_t iSufS = uiVal;
  int32_t iStopLoop = 0;
  int32_t k = iExpBits;
  do {
    if (iSufS >= (1 << k)) {
      WelsCabacEncodeBypassOne (pCbCtx, 1);
      iSufS = iSufS - (1 << k);
      k++;
    } else {
      WelsCabacEncodeBypassOne (pCbCtx, 0);
      while (k--)
        WelsCabacEncodeBypassOne (pCbCtx, (iSufS >> k) & 1);
      iStopLoop = 1;
    }
  } while (!iStopLoop);
}

void WelsCabacEncodeFlush (SCabacCtx* pCbCtx) {
  WelsCabacEncodeTerminate (pCbCtx, 1);
  while (pCbCtx->m_uiBitsUsed > 0) {
    if (pCbCtx->m_uiBitsUsed > 8) {
      pCbCtx->m_uiBitsUsed -= 8;
      uint32_t uiByte = pCbCtx->m_uData >> (pCbCtx->m_uiBitsUsed);
      pCbCtx->m_uData &= (uint32_t) ((0xFFFFFFFF) >> (32 - pCbCtx->m_uiBitsUsed));
      *pCbCtx->m_pBufCur ++ = uiByte;
    } else {
      if (pCbCtx->m_uiBitsUsed == 8) {
        *pCbCtx->m_pBufCur ++ = pCbCtx->m_uData & 0xff;
      } else {
        *pCbCtx->m_pBufCur ++ = (pCbCtx->m_uData << (8 - pCbCtx->m_uiBitsUsed));
      }
      pCbCtx->m_uiBitsUsed = 0;
    }
  }

}

uint8_t* WelsCabacEncodeGetPtr (SCabacCtx* pCbCtx) {
  return pCbCtx->m_pBufCur;
}
}
