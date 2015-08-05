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

#include "downsample.h"
#include "cpu.h"

WELSVP_NAMESPACE_BEGIN


///////////////////////////////////////////////////////////////////////////////////////////////////////////////

CDownsampling::CDownsampling (int32_t iCpuFlag) {
  m_iCPUFlag = iCpuFlag;
  m_eMethod   = METHOD_DOWNSAMPLE;
  WelsMemset (&m_pfDownsample, 0, sizeof (m_pfDownsample));
  InitDownsampleFuncs (m_pfDownsample, m_iCPUFlag);
}

CDownsampling::~CDownsampling() {
}

void CDownsampling::InitDownsampleFuncs (SDownsampleFuncs& sDownsampleFunc,  int32_t iCpuFlag) {
  sDownsampleFunc.pfHalfAverage[0] = DyadicBilinearDownsampler_c;
  sDownsampleFunc.pfHalfAverage[1] = DyadicBilinearDownsampler_c;
  sDownsampleFunc.pfHalfAverage[2] = DyadicBilinearDownsampler_c;
  sDownsampleFunc.pfHalfAverage[3] = DyadicBilinearDownsampler_c;
  sDownsampleFunc.pfGeneralRatioChroma  = GeneralBilinearAccurateDownsampler_c;
  sDownsampleFunc.pfGeneralRatioLuma    = GeneralBilinearFastDownsampler_c;
#if defined(X86_ASM)
  if (iCpuFlag & WELS_CPU_SSE) {
    sDownsampleFunc.pfHalfAverage[0]    = DyadicBilinearDownsamplerWidthx32_sse;
    sDownsampleFunc.pfHalfAverage[1]    = DyadicBilinearDownsamplerWidthx16_sse;
    sDownsampleFunc.pfHalfAverage[2]    = DyadicBilinearDownsamplerWidthx8_sse;
  }
  if (iCpuFlag & WELS_CPU_SSE2) {
    sDownsampleFunc.pfGeneralRatioChroma = GeneralBilinearAccurateDownsamplerWrap_sse2;
    sDownsampleFunc.pfGeneralRatioLuma   = GeneralBilinearFastDownsamplerWrap_sse2;
  }
  if (iCpuFlag & WELS_CPU_SSSE3) {
    sDownsampleFunc.pfHalfAverage[0]    = DyadicBilinearDownsamplerWidthx32_ssse3;
    sDownsampleFunc.pfHalfAverage[1]    = DyadicBilinearDownsamplerWidthx16_ssse3;
  }
  if (iCpuFlag & WELS_CPU_SSE41) {
    sDownsampleFunc.pfHalfAverage[0]    = DyadicBilinearDownsamplerWidthx32_sse4;
    sDownsampleFunc.pfHalfAverage[1]    = DyadicBilinearDownsamplerWidthx16_sse4;
  }
#endif//X86_ASM

#if defined(HAVE_NEON)
  if (iCpuFlag & WELS_CPU_NEON) {
    sDownsampleFunc.pfHalfAverage[0] = DyadicBilinearDownsamplerWidthx32_neon;
    sDownsampleFunc.pfHalfAverage[1] = DyadicBilinearDownsampler_neon;
    sDownsampleFunc.pfHalfAverage[2] = DyadicBilinearDownsampler_neon;
    sDownsampleFunc.pfHalfAverage[3] = DyadicBilinearDownsampler_neon;
    sDownsampleFunc.pfGeneralRatioChroma = GeneralBilinearAccurateDownsamplerWrap_neon;
    sDownsampleFunc.pfGeneralRatioLuma   = GeneralBilinearAccurateDownsamplerWrap_neon;
  }
#endif

#if defined(HAVE_NEON_AARCH64)
  if (iCpuFlag & WELS_CPU_NEON) {
    sDownsampleFunc.pfHalfAverage[0] = DyadicBilinearDownsamplerWidthx32_AArch64_neon;
    sDownsampleFunc.pfHalfAverage[1] = DyadicBilinearDownsampler_AArch64_neon;
    sDownsampleFunc.pfHalfAverage[2] = DyadicBilinearDownsampler_AArch64_neon;
    sDownsampleFunc.pfHalfAverage[3] = DyadicBilinearDownsampler_AArch64_neon;
    sDownsampleFunc.pfGeneralRatioChroma = GeneralBilinearAccurateDownsamplerWrap_AArch64_neon;
    sDownsampleFunc.pfGeneralRatioLuma   = GeneralBilinearAccurateDownsamplerWrap_AArch64_neon;
  }
#endif
}

EResult CDownsampling::Process (int32_t iType, SPixMap* pSrcPixMap, SPixMap* pDstPixMap) {
  int32_t iSrcWidthY = pSrcPixMap->sRect.iRectWidth;
  int32_t iSrcHeightY = pSrcPixMap->sRect.iRectHeight;
  int32_t iDstWidthY = pDstPixMap->sRect.iRectWidth;
  int32_t iDstHeightY = pDstPixMap->sRect.iRectHeight;

  int32_t iSrcWidthUV = iSrcWidthY >> 1;
  int32_t iSrcHeightUV = iSrcHeightY >> 1;
  int32_t iDstWidthUV = iDstWidthY >> 1;
  int32_t iDstHeightUV = iDstHeightY >> 1;

  if (iSrcWidthY <= iDstWidthY || iSrcHeightY <= iDstHeightY) {
    return RET_INVALIDPARAM;
  }

  if ((iSrcWidthY >> 1) == iDstWidthY && (iSrcHeightY >> 1) == iDstHeightY) {
    // use half average functions
    uint8_t iAlignIndex = GetAlignedIndex (iSrcWidthY);
    m_pfDownsample.pfHalfAverage[iAlignIndex] ((uint8_t*)pDstPixMap->pPixel[0], pDstPixMap->iStride[0],
        (uint8_t*)pSrcPixMap->pPixel[0], pSrcPixMap->iStride[0], iSrcWidthY, iSrcHeightY);

    iAlignIndex = GetAlignedIndex (iSrcWidthUV);
    m_pfDownsample.pfHalfAverage[iAlignIndex] ((uint8_t*)pDstPixMap->pPixel[1], pDstPixMap->iStride[1],
        (uint8_t*)pSrcPixMap->pPixel[1], pSrcPixMap->iStride[1], iSrcWidthUV, iSrcHeightUV);
    m_pfDownsample.pfHalfAverage[iAlignIndex] ((uint8_t*)pDstPixMap->pPixel[2], pDstPixMap->iStride[2],
        (uint8_t*)pSrcPixMap->pPixel[2], pSrcPixMap->iStride[2], iSrcWidthUV, iSrcHeightUV);
  } else {
    m_pfDownsample.pfGeneralRatioLuma ((uint8_t*)pDstPixMap->pPixel[0], pDstPixMap->iStride[0], iDstWidthY, iDstHeightY,
                                       (uint8_t*)pSrcPixMap->pPixel[0], pSrcPixMap->iStride[0], iSrcWidthY, iSrcHeightY);

    m_pfDownsample.pfGeneralRatioChroma ((uint8_t*)pDstPixMap->pPixel[1], pDstPixMap->iStride[1], iDstWidthUV, iDstHeightUV,
                                         (uint8_t*)pSrcPixMap->pPixel[1], pSrcPixMap->iStride[1], iSrcWidthUV, iSrcHeightUV);

    m_pfDownsample.pfGeneralRatioChroma ((uint8_t*)pDstPixMap->pPixel[2], pDstPixMap->iStride[2], iDstWidthUV, iDstHeightUV,
                                         (uint8_t*)pSrcPixMap->pPixel[2], pSrcPixMap->iStride[2], iSrcWidthUV, iSrcHeightUV);
  }
  return RET_SUCCESS;
}

int32_t CDownsampling::GetAlignedIndex (const int32_t kiSrcWidth) {
  int32_t iAlignIndex;
  if ((kiSrcWidth & 0x1f) == 0)         // x32
    iAlignIndex = 0;
  else if ((kiSrcWidth & 0x0f) == 0)    // x16
    iAlignIndex = 1;
  else if ((kiSrcWidth & 0x07) == 0)    // x8
    iAlignIndex = 2;
  else
    iAlignIndex = 3;
  return iAlignIndex;
}


WELSVP_NAMESPACE_END
