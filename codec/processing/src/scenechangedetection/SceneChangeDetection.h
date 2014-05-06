/*!
 * \copy
 *     Copyright (c)  2011-2013, Cisco Systems
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
 * \file	        :  SceneChangeDetection.h
 *
 * \brief	    :  scene change detection class of wels video processor class
 *
 * \date         :  2011/03/14
 *
 * \description  :  1. rewrite the package code of scene change detection class
 *
 *************************************************************************************
 */

#ifndef WELSVP_SCENECHANGEDETECTION_H
#define WELSVP_SCENECHANGEDETECTION_H

#include "util.h"
#include "memory.h"
#include "cpu.h"
#include "WelsFrameWork.h"
#include "IWelsVP.h"
#include "common.h"

#define HIGH_MOTION_BLOCK_THRESHOLD 320
#define SCENE_CHANGE_MOTION_RATIO_LARGE   0.85f
#define SCENE_CHANGE_MOTION_RATIO_MEDIUM  0.50f

WELSVP_NAMESPACE_BEGIN

class CSceneChangeDetectorVideo {
 public:
  CSceneChangeDetectorVideo (SSceneChangeResult& sParam, int32_t iCpuFlag) : m_sParam (sParam) {
    m_pfSad = WelsSampleSad8x8_c;
#ifdef X86_ASM
    if (iCpuFlag & WELS_CPU_SSE2) {
      m_pfSad = WelsSampleSad8x8_sse21;
    }
#endif
#ifdef HAVE_NEON
    if (iCpuFlag & WELS_CPU_NEON) {
      m_pfSad = WelsProcessingSampleSad8x8_neon;
    }
#endif
  }
  virtual ~CSceneChangeDetectorVideo() {
  }
  void operator() (uint8_t* pSrcY, int32_t iSrcStrideY, uint8_t* pRefY, int32_t iRefStrideY, uint8_t*& pStaticBlockIdc) {
    int32_t iSad = m_pfSad (pSrcY, iSrcStrideY, pRefY, iSrcStrideY);
    m_sParam.iMotionBlockNum += iSad > HIGH_MOTION_BLOCK_THRESHOLD;
  }
 protected:
  SadFuncPtr m_pfSad;
  SSceneChangeResult& m_sParam;
};

class CSceneChangeDetectorScreen : public CSceneChangeDetectorVideo {
 public:
  CSceneChangeDetectorScreen (SSceneChangeResult& sParam, int32_t iCpuFlag) : CSceneChangeDetectorVideo (sParam,
        iCpuFlag) {
  }
  virtual ~CSceneChangeDetectorScreen() {
  }
  void operator() (uint8_t* pSrcY, int32_t iSrcStrideY, uint8_t* pRefY, int32_t iRefStrideY, uint8_t*& pStaticBlockIdc) {
    int32_t iSad = m_pfSad (pSrcY, iSrcStrideY, pRefY, iSrcStrideY);
    if (iSad == 0) {
      *pStaticBlockIdc ++ = COLLOCATED_STATIC;
    } else {
      m_sParam.iFrameComplexity += iSad;
      m_sParam.iMotionBlockNum += iSad > HIGH_MOTION_BLOCK_THRESHOLD;
      *pStaticBlockIdc ++ = NO_STATIC;
    }
  }
};

template<typename T>
class CSceneChangeDetection : public IStrategy {
 public:
  CSceneChangeDetection (EMethods eMethod, int32_t iCpuFlag): m_cDetector (m_sSceneChangeParam, iCpuFlag) {
    m_eMethod   = eMethod;
    WelsMemset (&m_sSceneChangeParam, 0, sizeof (m_sSceneChangeParam));
  }

  ~CSceneChangeDetection() {
  }

  EResult Process (int32_t iType, SPixMap* pSrcPixMap, SPixMap* pRefPixMap) {
    EResult eReturn = RET_INVALIDPARAM;
    int32_t iWidth                  = pSrcPixMap->sRect.iRectWidth;
    int32_t iHeight                 = pSrcPixMap->sRect.iRectHeight;
    int32_t iBlock8x8Width      = iWidth  >> 3;
    int32_t iBlock8x8Height	 = iHeight >> 3;
    int32_t iBlock8x8Num       = iBlock8x8Width * iBlock8x8Height;
    int32_t iSceneChangeThresholdLarge = WelsStaticCast (int32_t,
                                         SCENE_CHANGE_MOTION_RATIO_LARGE * iBlock8x8Num + 0.5f + PESN);
    int32_t iSceneChangeThresholdMedium	= WelsStaticCast (int32_t,
                                          SCENE_CHANGE_MOTION_RATIO_MEDIUM * iBlock8x8Num + 0.5f + PESN);
    uint8_t* pRefY = NULL, *pCurY = NULL;
    int32_t iRefStride = 0, iCurStride = 0;
    int32_t iRefRowStride = 0, iCurRowStride = 0;
    uint8_t* pRefTmp = NULL, *pCurTmp = NULL;
    uint8_t* pStaticBlockIdc = m_sSceneChangeParam.pStaticBlockIdc;

    pRefY = (uint8_t*)pRefPixMap->pPixel[0];
    pCurY = (uint8_t*)pSrcPixMap->pPixel[0];

    iRefStride  = pRefPixMap->iStride[0];
    iCurStride  = pSrcPixMap->iStride[0];

    iRefRowStride  = pRefPixMap->iStride[0] << 3;
    iCurRowStride  = pSrcPixMap->iStride[0] << 3;

    m_sSceneChangeParam.iMotionBlockNum = 0;
    m_sSceneChangeParam.iFrameComplexity = 0;
    m_sSceneChangeParam.eSceneChangeIdc = SIMILAR_SCENE;

    for (int32_t j = 0; j < iBlock8x8Height; j ++) {
      pRefTmp	= pRefY;
      pCurTmp   = pCurY;

      for (int32_t i = 0; i < iBlock8x8Width; i++) {
        m_cDetector (pRefTmp, iRefStride, pCurTmp, iCurStride, pStaticBlockIdc);
        pRefTmp += 8;
        pCurTmp += 8;
      }

      pRefY += iRefRowStride;
      pCurY += iCurRowStride;
    }

    if (m_sSceneChangeParam.iMotionBlockNum >= iSceneChangeThresholdLarge) {
      m_sSceneChangeParam.eSceneChangeIdc = LARGE_CHANGED_SCENE;
    } else if (m_sSceneChangeParam.iMotionBlockNum >= iSceneChangeThresholdMedium) {
      m_sSceneChangeParam.eSceneChangeIdc = MEDIUM_CHANGED_SCENE;
    }

    eReturn = RET_SUCCESS;

    return eReturn;
  }

  EResult Get (int32_t iType, void* pParam) {
    if (pParam == NULL) {
      return RET_INVALIDPARAM;
    }
    * (SSceneChangeResult*)pParam = m_sSceneChangeParam;
    return RET_SUCCESS;
  }

  EResult Set (int32_t iType, void* pParam) {
    if (pParam == NULL) {
      return RET_INVALIDPARAM;
    }
    m_sSceneChangeParam = * (SSceneChangeResult*)pParam;
    return RET_SUCCESS;
  }
 private:
  SSceneChangeResult m_sSceneChangeParam;
  T          m_cDetector;
};

IStrategy* BuildSceneChangeDetection (EMethods eMethod, int32_t iCpuFlag);

WELSVP_NAMESPACE_END

#endif
