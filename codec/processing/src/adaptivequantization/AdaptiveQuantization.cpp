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
#include "AdaptiveQuantization.h"
#include "../common/cpu.h"

WELSVP_NAMESPACE_BEGIN



#define AVERAGE_TIME_MOTION                   (0.3) //0.3046875 // 1/4 + 1/16 - 1/128 ~ 0.3
#define AVERAGE_TIME_TEXTURE_QUALITYMODE  (1.0) //0.5 // 1/2
#define AVERAGE_TIME_TEXTURE_BITRATEMODE  (0.875) //0.5 // 1/2
#define MODEL_ALPHA                           (0.9910) //1.5 //1.1102
#define MODEL_TIME                            (5.8185) //9.0 //5.9842

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

CAdaptiveQuantization::CAdaptiveQuantization (int32_t iCpuFlag) {
  m_CPUFlag = iCpuFlag;
  m_eMethod   = METHOD_ADAPTIVE_QUANT;
  m_pfVar   = NULL;
  WelsMemset (&m_sAdaptiveQuantParam, 0, sizeof (m_sAdaptiveQuantParam));
  WelsInitVarFunc (m_pfVar, m_CPUFlag);
}

CAdaptiveQuantization::~CAdaptiveQuantization() {
}

EResult CAdaptiveQuantization::Process (int32_t iType, SPixMap* pSrcPixMap, SPixMap* pRefPixMap) {
  EResult eReturn = RET_INVALIDPARAM;

  int32_t iWidth     = pSrcPixMap->sRect.iRectWidth;
  int32_t iHeight    = pSrcPixMap->sRect.iRectHeight;
  int32_t iMbWidth  = iWidth  >> 4;
  int32_t iMbHeight = iHeight >> 4;
  int32_t iMbTotalNum    = iMbWidth * iMbHeight;

  SMotionTextureUnit* pMotionTexture = NULL;
  SVAACalcResult*     pVaaCalcResults = NULL;
  int8_t   iMotionTextureIndexToDeltaQp = 0;
  int32_t	 iAverMotionTextureIndexToDeltaQp = 0;	// double to uint32
  double_t dAverageMotionIndex = 0.0;	// double to float
  double_t dAverageTextureIndex = 0.0;

  double_t dQStep = 0.0;
  double_t dLumaMotionDeltaQp = 0;
  double_t dLumaTextureDeltaQp = 0;

  uint8_t* pRefFrameY = NULL, *pCurFrameY = NULL;
  int32_t iRefStride = 0, iCurStride = 0;

  uint8_t* pRefFrameTmp = NULL, *pCurFrameTmp = NULL;
  int32_t i = 0, j = 0;

  pRefFrameY = (uint8_t*)pRefPixMap->pPixel[0];
  pCurFrameY = (uint8_t*)pSrcPixMap->pPixel[0];

  iRefStride  = pRefPixMap->iStride[0];
  iCurStride  = pSrcPixMap->iStride[0];

  /////////////////////////////////////// motion //////////////////////////////////
  //  motion MB residual variance
  dAverageMotionIndex = 0.0;
  dAverageTextureIndex = 0.0;
  pMotionTexture = m_sAdaptiveQuantParam.pMotionTextureUnit;
  pVaaCalcResults = m_sAdaptiveQuantParam.pCalcResult;

  if (pVaaCalcResults->pRefY == pRefFrameY && pVaaCalcResults->pCurY == pCurFrameY) {
    int32_t iMbIndex = 0;
    int32_t iSumDiff, iSQDiff, uiSum, iSQSum;
    for (j = 0; j < iMbHeight; j ++) {
      pRefFrameTmp  = pRefFrameY;
      pCurFrameTmp  = pCurFrameY;
      for (i = 0; i < iMbWidth; i++) {
        iSumDiff =  pVaaCalcResults->pSad8x8[iMbIndex][0];
        iSumDiff += pVaaCalcResults->pSad8x8[iMbIndex][1];
        iSumDiff += pVaaCalcResults->pSad8x8[iMbIndex][2];
        iSumDiff += pVaaCalcResults->pSad8x8[iMbIndex][3];

        iSQDiff = pVaaCalcResults->pSsd16x16[iMbIndex];
        uiSum = pVaaCalcResults->pSum16x16[iMbIndex];
        iSQSum = pVaaCalcResults->pSumOfSquare16x16[iMbIndex];

        iSumDiff = iSumDiff >> 8;
        pMotionTexture->uiMotionIndex = (iSQDiff >> 8) - (iSumDiff * iSumDiff);

        uiSum = uiSum >> 8;
        pMotionTexture->uiTextureIndex = (iSQSum >> 8) - (uiSum * uiSum);

        dAverageMotionIndex += pMotionTexture->uiMotionIndex;
        dAverageTextureIndex += pMotionTexture->uiTextureIndex;
        pMotionTexture++;
        ++iMbIndex;
        pRefFrameTmp += MB_WIDTH_LUMA;
        pCurFrameTmp += MB_WIDTH_LUMA;
      }
      pRefFrameY += (iRefStride) << 4;
      pCurFrameY += (iCurStride) << 4;
    }
  } else {
    for (j = 0; j < iMbHeight; j ++) {
      pRefFrameTmp  = pRefFrameY;
      pCurFrameTmp  = pCurFrameY;
      for (i = 0; i < iMbWidth; i++) {
        m_pfVar (pRefFrameTmp, iRefStride, pCurFrameTmp, iCurStride, pMotionTexture);
        dAverageMotionIndex += pMotionTexture->uiMotionIndex;
        dAverageTextureIndex += pMotionTexture->uiTextureIndex;
        pMotionTexture++;
        pRefFrameTmp += MB_WIDTH_LUMA;
        pCurFrameTmp += MB_WIDTH_LUMA;

      }
      pRefFrameY += (iRefStride) << 4;
      pCurFrameY += (iCurStride) << 4;
    }
  }
  dAverageMotionIndex = dAverageMotionIndex / iMbTotalNum;
  dAverageTextureIndex = dAverageTextureIndex / iMbTotalNum;
  if ((dAverageMotionIndex <= PESN) && (dAverageMotionIndex >= -PESN)) {
    dAverageMotionIndex = 1.0;
  }
  if ((dAverageTextureIndex <= PESN) && (dAverageTextureIndex >= -PESN)) {
    dAverageTextureIndex = 1.0;
  }
  //  motion mb residual map to QP
  //  texture mb original map to QP
  iAverMotionTextureIndexToDeltaQp = 0;
  dAverageMotionIndex = AVERAGE_TIME_MOTION * dAverageMotionIndex;

  if (m_sAdaptiveQuantParam.iAdaptiveQuantMode == AQ_QUALITY_MODE) {
    dAverageTextureIndex = AVERAGE_TIME_TEXTURE_QUALITYMODE * dAverageTextureIndex;
  } else {
    dAverageTextureIndex = AVERAGE_TIME_TEXTURE_BITRATEMODE * dAverageTextureIndex;
  }

  pMotionTexture = m_sAdaptiveQuantParam.pMotionTextureUnit;
  for (j = 0; j < iMbHeight; j ++) {
    for (i = 0; i < iMbWidth; i++) {
      double_t a = pMotionTexture->uiTextureIndex / dAverageTextureIndex;
      dQStep = (a - 1) / (a + MODEL_ALPHA);
      dLumaTextureDeltaQp = MODEL_TIME * dQStep;// range +- 6

      iMotionTextureIndexToDeltaQp = (int8_t)dLumaTextureDeltaQp;

      a = pMotionTexture->uiMotionIndex / dAverageMotionIndex;
      dQStep = (a - 1) / (a + MODEL_ALPHA);
      dLumaMotionDeltaQp = MODEL_TIME * dQStep;// range +- 6

      if ((m_sAdaptiveQuantParam.iAdaptiveQuantMode == AQ_QUALITY_MODE && dLumaMotionDeltaQp < -PESN)
          || (m_sAdaptiveQuantParam.iAdaptiveQuantMode == AQ_BITRATE_MODE)) {
        iMotionTextureIndexToDeltaQp += (int8_t)dLumaMotionDeltaQp;
      }

      m_sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp[j * iMbWidth + i] = iMotionTextureIndexToDeltaQp;
      iAverMotionTextureIndexToDeltaQp += iMotionTextureIndexToDeltaQp;
      pMotionTexture++;
    }
  }
  m_sAdaptiveQuantParam.dAverMotionTextureIndexToDeltaQp = (1.0 * iAverMotionTextureIndexToDeltaQp) / iMbTotalNum;

  eReturn = RET_SUCCESS;

  return eReturn;
}



EResult CAdaptiveQuantization::Set (int32_t iType, void* pParam) {
  if (pParam == NULL) {
    return RET_INVALIDPARAM;
  }

  m_sAdaptiveQuantParam = * (SAdaptiveQuantizationParam*)pParam;

  return RET_SUCCESS;
}

EResult CAdaptiveQuantization::Get (int32_t iType, void* pParam) {
  if (pParam == NULL) {
    return RET_INVALIDPARAM;
  }

  SAdaptiveQuantizationParam* sAdaptiveQuantParam = (SAdaptiveQuantizationParam*)pParam;

  sAdaptiveQuantParam->dAverMotionTextureIndexToDeltaQp = m_sAdaptiveQuantParam.dAverMotionTextureIndexToDeltaQp;

  return RET_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////////////

void CAdaptiveQuantization::WelsInitVarFunc (PVarFunc& pfVar,  int32_t iCpuFlag) {
  pfVar = SampleVariance16x16_c;

#ifdef X86_ASM
  if (iCpuFlag & WELS_CPU_SSE2) {
     pfVar = SampleVariance16x16_sse2;
  }
#endif
}

void SampleVariance16x16_c (uint8_t* pRefY, int32_t iRefStride, uint8_t* pSrcY, int32_t iSrcStride,
                            SMotionTextureUnit* pMotionTexture) {
  uint32_t uiCurSquare = 0,  uiSquare = 0;
  uint16_t uiCurSum = 0,  uiSum = 0;

  for (int32_t y = 0; y < MB_WIDTH_LUMA; y++) {
    for (int32_t x = 0; x < MB_WIDTH_LUMA; x++) {
      uint32_t uiDiff = WELS_ABS (pRefY[x] - pSrcY[x]);
      uiSum += uiDiff;
      uiSquare += uiDiff * uiDiff;

      uiCurSum += pSrcY[x];
      uiCurSquare += pSrcY[x] * pSrcY[x];
    }
    pRefY += iRefStride;
    pSrcY += iSrcStride;
  }

  uiSum = uiSum >> 8;
  pMotionTexture->uiMotionIndex = (uiSquare >> 8) - (uiSum * uiSum);

  uiCurSum = uiCurSum >> 8;
  pMotionTexture->uiTextureIndex = (uiCurSquare >> 8) - (uiCurSum * uiCurSum);
}

WELSVP_NAMESPACE_END
