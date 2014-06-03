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
 * \file	    :  downsample.h
 *
 * \brief	    :  downsample class of wels video processor class
 *
 * \date        :  2011/03/33
 *
 * \description :  1. rewrite the package code of downsample class
 *
 *************************************************************************************
 */

#ifndef WELSVP_DOWNSAMPLE_H
#define WELSVP_DOWNSAMPLE_H

#include "util.h"
#include "WelsFrameWork.h"
#include "IWelsVP.h"
#include "macros.h"

WELSVP_NAMESPACE_BEGIN


typedef void (HalveDownsampleFunc) (uint8_t* pDst, const int32_t kiDstStride,
                                    uint8_t* pSrc, const int32_t kiSrcStride,
                                    const int32_t kiSrcWidth, const int32_t kiSrcHeight);

typedef void (GeneralDownsampleFunc) (uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth,
                                      const int32_t kiDstHeight,
                                      uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight);

typedef HalveDownsampleFunc*		PHalveDownsampleFunc;
typedef GeneralDownsampleFunc*	PGeneralDownsampleFunc;

HalveDownsampleFunc   DyadicBilinearDownsampler_c;
GeneralDownsampleFunc GeneralBilinearFastDownsampler_c;
GeneralDownsampleFunc GeneralBilinearAccurateDownsampler_c;

typedef struct {
  // align_index: 0 = x32; 1 = x16; 2 = x8; 3 = common case left;
  PHalveDownsampleFunc			pfHalfAverage[4];
  PGeneralDownsampleFunc		pfGeneralRatioLuma;
  PGeneralDownsampleFunc		pfGeneralRatioChroma;
} SDownsampleFuncs;


#ifdef X86_ASM
WELSVP_EXTERN_C_BEGIN
// used for scr width is multipler of 8 pixels
HalveDownsampleFunc		DyadicBilinearDownsamplerWidthx8_sse;
// iSrcWidth= x16 pixels
HalveDownsampleFunc		DyadicBilinearDownsamplerWidthx16_sse;
// iSrcWidth= x32 pixels
HalveDownsampleFunc		DyadicBilinearDownsamplerWidthx32_sse;
// used for scr width is multipler of 16 pixels
HalveDownsampleFunc		DyadicBilinearDownsamplerWidthx16_ssse3;
// iSrcWidth= x32 pixels
HalveDownsampleFunc		DyadicBilinearDownsamplerWidthx32_ssse3;
// iSrcWidth= x16 pixels
HalveDownsampleFunc		DyadicBilinearDownsamplerWidthx16_sse4;
// iSrcWidth= x32 pixels
HalveDownsampleFunc		DyadicBilinearDownsamplerWidthx32_sse4;

GeneralDownsampleFunc GeneralBilinearFastDownsamplerWrap_sse2;
GeneralDownsampleFunc GeneralBilinearAccurateDownsamplerWrap_sse2;

void GeneralBilinearFastDownsampler_sse2 (uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth,
    const int32_t kiDstHeight,
    uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight,
    const uint32_t kuiScaleX, const uint32_t kuiScaleY);
void GeneralBilinearAccurateDownsampler_sse2 (uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth,
    const int32_t kiDstHeight,
    uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight,
    const uint32_t kuiScaleX, const uint32_t kuiScaleY);
WELSVP_EXTERN_C_END
#endif

#ifdef HAVE_NEON
WELSVP_EXTERN_C_BEGIN
// iSrcWidth no limitation
HalveDownsampleFunc		DyadicBilinearDownsampler_neon;
// iSrcWidth = x32 pixels
HalveDownsampleFunc		DyadicBilinearDownsamplerWidthx32_neon;

GeneralDownsampleFunc   GeneralBilinearAccurateDownsamplerWrap_neon;

void GeneralBilinearAccurateDownsampler_neon (uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth,
    const int32_t kiDstHeight,
    uint8_t* pSrc, const int32_t kiSrcStride, const uint32_t kuiScaleX, const uint32_t kuiScaleY);

WELSVP_EXTERN_C_END
#endif


class CDownsampling : public IStrategy {
 public:
  CDownsampling (int32_t iCpuFlag);
  ~CDownsampling();

  EResult Process (int32_t iType, SPixMap* pSrc, SPixMap* pDst);

 private:
  void InitDownsampleFuncs (SDownsampleFuncs& sDownsampleFunc, int32_t iCpuFlag);

  int32_t GetAlignedIndex (const int32_t kiSrcWidth);

 private:
  SDownsampleFuncs m_pfDownsample;
  int32_t  m_iCPUFlag;
};

WELSVP_NAMESPACE_END

#endif
