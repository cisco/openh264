/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
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
 */

#include "SceneChangeDetection.h"
#include "../common/cpu.h"

WELSVP_NAMESPACE_BEGIN

#define HIGH_MOTION_BLOCK_THRESHOLD 320
#define SCENE_CHANGE_MOTION_RATIO	0.85f



///////////////////////////////////////////////////////////////////////////////////////////////////////////////

CSceneChangeDetection::CSceneChangeDetection (int32_t iCpuFlag) {
  m_iCpuFlag = iCpuFlag;
  m_eMethod   = METHOD_SCENE_CHANGE_DETECTION;
  m_pfSad   = NULL;
  WelsMemset (&m_sSceneChangeParam, 0, sizeof (m_sSceneChangeParam));
  InitSadFuncs (m_pfSad, m_iCpuFlag);
}

CSceneChangeDetection::~CSceneChangeDetection() {
}

EResult CSceneChangeDetection::Process (int32_t iType, SPixMap* pSrcPixMap, SPixMap* pRefPixMap) {
  EResult eReturn = RET_INVALIDPARAM;

  int32_t iWidth                  = pSrcPixMap->sRect.iRectWidth;
  int32_t iHeight                 = pSrcPixMap->sRect.iRectHeight;
  int32_t iBlock8x8Width      = iWidth  >> 3;
  int32_t iBlock8x8Height	 = iHeight >> 3;
  int32_t iBlock8x8Num       = iBlock8x8Width * iBlock8x8Height;
  int32_t iSceneChangeThreshold = WelsStaticCast (int32_t, SCENE_CHANGE_MOTION_RATIO * iBlock8x8Num + 0.5f + PESN);

  int32_t iBlockSad = 0;
  int32_t iMotionBlockNum = 0;

  uint8_t* pRefY = NULL, *pCurY = NULL;
  int32_t iRefStride = 0, iCurStride = 0;
  int32_t iRefRowStride = 0, iCurRowStride = 0;

  uint8_t* pRefTmp = NULL, *pCurTmp = NULL;

  pRefY = (uint8_t*)pRefPixMap->pPixel[0];
  pCurY = (uint8_t*)pSrcPixMap->pPixel[0];

  iRefStride  = pRefPixMap->iStride[0];
  iCurStride  = pSrcPixMap->iStride[0];

  iRefRowStride  = pRefPixMap->iStride[0] << 3;
  iCurRowStride  = pSrcPixMap->iStride[0] << 3;

  m_sSceneChangeParam.bSceneChangeFlag = 0;

  for (int32_t j = 0; j < iBlock8x8Height; j ++) {
    pRefTmp	= pRefY;
    pCurTmp 	= pCurY;

    for (int32_t i = 0; i < iBlock8x8Width; i++) {
      iBlockSad = m_pfSad (pRefTmp, iRefStride, pCurTmp, iCurStride);

      iMotionBlockNum += (iBlockSad > HIGH_MOTION_BLOCK_THRESHOLD);

      pRefTmp += 8;
      pCurTmp += 8;
    }

    pRefY += iRefRowStride;
    pCurY += iCurRowStride;
  }

  if (iMotionBlockNum >= iSceneChangeThreshold) {
    m_sSceneChangeParam.bSceneChangeFlag = 1;
  }

  eReturn = RET_SUCCESS;

  return eReturn;
}


EResult CSceneChangeDetection::Get (int32_t iType, void* pParam) {
  if (pParam == NULL) {
    return RET_INVALIDPARAM;
  }

  * (SSceneChangeResult*)pParam = m_sSceneChangeParam;

  return RET_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CSceneChangeDetection::InitSadFuncs (SadFuncPtr& pfSad,  int32_t iCpuFlag) {
  pfSad = WelsSampleSad8x8_c;

#ifdef X86_ASM
  if (iCpuFlag & WELS_CPU_SSE2) {
    pfSad = WelsSampleSad8x8_sse21;
  }
#endif
}


WELSVP_NAMESPACE_END
