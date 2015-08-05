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
 * \file    set_mb_syn_cabac.h
 *
 * \brief   Seting all syntax elements of mb and encoding residual with cabac
 *
 * \date    09/27/2014 Created
 *
 *************************************************************************************
 */

#ifndef SET_MB_SYN_CABAC_H_
#define SET_MB_SYN_CABAC_H_

#include "typedefs.h"
#include "wels_common_defs.h"

using namespace WelsCommon;

namespace WelsEnc {

#define  WELS_QP_MAX    51

typedef struct TagStateCtx {
  uint8_t   m_uiState;
  uint8_t   m_uiValMps;
} SStateCtx;
typedef struct TagCabacCtx {
  uint32_t  m_uiLow;
  uint32_t  m_uiRange;
  SStateCtx   m_sStateCtx[WELS_CONTEXT_COUNT];
  uint8_t*   m_pBufStart;
  uint8_t*   m_pBufEnd;
  uint8_t*   m_pBufCur;
  uint8_t  m_iBitsOutstanding;
  uint32_t  m_uData;
  uint32_t  m_uiBitsUsed;
  uint32_t  m_iFirstFlag;
  uint32_t  m_uiBinCountsInNalUnits;
} SCabacCtx;


void WelsCabacContextInit (void* pCtx, SCabacCtx* pCbCtx, int32_t iModel);
void WelsCabacEncodeInit (SCabacCtx* pCbCtx, uint8_t* pBuf,  uint8_t* pEnd);
void WelsCabacEncodeDecision (SCabacCtx* pCbCtx, int32_t iCtx, uint32_t uiBin);
void WelsCabacEncodeBypassOne (SCabacCtx* pCbCtx, uint32_t uiBin);
void WelsCabacEncodeTerminate (SCabacCtx* pCbCtx, uint32_t uiBin);
void WelsCabacEncodeUeBypass (SCabacCtx* pCbCtx, int32_t iExpBits, uint32_t uiVal);
void WelsCabacEncodeFlush (SCabacCtx* pCbCtx);
uint8_t* WelsCabacEncodeGetPtr (SCabacCtx* pCbCtx);
int32_t  WriteBlockResidualCabac (void* pEncCtx,  int16_t* pCoffLevel, int32_t iEndIdx,
                                  int32_t iCalRunLevelFlag,
                                  int32_t iResidualProperty, int8_t iNC, SBitStringAux* pBs);

}
#endif
