/*!
 * \copy
 *     Copyright (c)  2008-2013, Cisco Systems
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
 *  downsample_yuv.c
 *
 *  Abstract
 *      Implementation for source yuv data downsampling used before spatial encoding.
 *
 *  History
 *      10/24/2008 Created
 *
 *****************************************************************************/

#include "downsample.h"


WELSVP_NAMESPACE_BEGIN


void DyadicBilinearDownsampler_c (uint8_t* pDst, const int32_t kiDstStride,
                                  uint8_t* pSrc, const int32_t kiSrcStride,
                                  const int32_t kiSrcWidth, const int32_t kiSrcHeight)

{
  uint8_t* pDstLine	= pDst;
  uint8_t* pSrcLine	= pSrc;
  const int32_t kiSrcStridex2	= kiSrcStride << 1;
  const int32_t kiDstWidth		= kiSrcWidth >> 1;
  const int32_t kiDstHeight	= kiSrcHeight >> 1;

  for (int32_t j = 0; j < kiDstHeight; j ++) {
    for (int32_t i = 0; i < kiDstWidth; i ++) {
      const int32_t kiSrcX = i << 1;
      const int32_t kiTempRow1 = (pSrcLine[kiSrcX] + pSrcLine[kiSrcX + 1] + 1) >> 1;
      const int32_t kiTempRow2 = (pSrcLine[kiSrcX + kiSrcStride] + pSrcLine[kiSrcX + kiSrcStride + 1] + 1) >> 1;

      pDstLine[i] = (uint8_t) ((kiTempRow1 + kiTempRow2 + 1) >> 1);
    }
    pDstLine	+= kiDstStride;
    pSrcLine	+= kiSrcStridex2;
  }
}

void GeneralBilinearFastDownsampler_c (uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth,
                                       const int32_t kiDstHeight,
                                       uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight) {
  const uint32_t kuiScaleBitWidth = 16, kuiScaleBitHeight = 15;
  const uint32_t kuiScaleWidth = (1 << kuiScaleBitWidth), kuiScaleHeight = (1 << kuiScaleBitHeight);
  int32_t fScalex = (int32_t) ((float_t)kiSrcWidth / (float_t)kiDstWidth * kuiScaleWidth);
  int32_t fScaley = (int32_t) ((float_t)kiSrcHeight / (float_t)kiDstHeight * kuiScaleHeight);
  uint32_t x;
  int32_t iYInverse, iXInverse;

  uint8_t* pByDst = pDst;
  uint8_t* pByLineDst = pDst;

  iYInverse = 1 << (kuiScaleBitHeight - 1);
  for (int32_t i = 0; i < kiDstHeight - 1; i++) {
    int32_t iYy = iYInverse >> kuiScaleBitHeight;
    int32_t fv = iYInverse & (kuiScaleHeight - 1);

    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;

    pByDst = pByLineDst;
    iXInverse = 1 << (kuiScaleBitWidth - 1);
    for (int32_t j = 0; j < kiDstWidth - 1; j++) {
      int32_t iXx = iXInverse >> kuiScaleBitWidth;
      int32_t iFu = iXInverse & (kuiScaleWidth - 1);

      uint8_t* pByCurrent = pBySrc + iXx;
      uint8_t a, b, c, d;

      a = *pByCurrent;
      b = * (pByCurrent + 1);
      c = * (pByCurrent + kiSrcStride);
      d = * (pByCurrent + kiSrcStride + 1);

      x  = (((uint32_t) (kuiScaleWidth - 1 - iFu)) * (kuiScaleHeight - 1 - fv) >> kuiScaleBitWidth) * a;
      x += (((uint32_t) (iFu)) * (kuiScaleHeight - 1 - fv) >> kuiScaleBitWidth) * b;
      x += (((uint32_t) (kuiScaleWidth - 1 - iFu)) * (fv) >> kuiScaleBitWidth) * c;
      x += (((uint32_t) (iFu)) * (fv) >> kuiScaleBitWidth) * d;
      x >>= (kuiScaleBitHeight - 1);
      x += 1;
      x >>= 1;
      //x = (((__int64)(SCALE_BIG - 1 - iFu))*(SCALE_BIG - 1 - fv)*a + ((__int64)iFu)*(SCALE_BIG - 1 -fv)*b + ((__int64)(SCALE_BIG - 1 -iFu))*fv*c +
      //		 ((__int64)iFu)*fv*d + (1 << (2*SCALE_BIT_BIG-1)) ) >> (2*SCALE_BIT_BIG);
      x = WELS_CLAMP (x, 0, 255);
      *pByDst++ = (uint8_t)x;

      iXInverse += fScalex;
    }
    *pByDst = * (pBySrc + (iXInverse >> kuiScaleBitWidth));
    pByLineDst += kiDstStride;
    iYInverse += fScaley;
  }

  // last row special
  {
    int32_t iYy = iYInverse >> kuiScaleBitHeight;
    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;

    pByDst = pByLineDst;
    iXInverse = 1 << (kuiScaleBitWidth - 1);
    for (int32_t j = 0; j < kiDstWidth; j++) {
      int32_t iXx = iXInverse >> kuiScaleBitWidth;
      *pByDst++ = * (pBySrc + iXx);

      iXInverse += fScalex;
    }
  }
}

void GeneralBilinearAccurateDownsampler_c (uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth,
    const int32_t kiDstHeight,
    uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight) {
  const int32_t kiScaleBit = 15;
  const int32_t kiScale = (1 << kiScaleBit);
  int32_t iScalex = (int32_t) ((float_t)kiSrcWidth / (float_t)kiDstWidth * kiScale);
  int32_t iScaley = (int32_t) ((float_t)kiSrcHeight / (float_t)kiDstHeight * kiScale);
  int64_t x;
  int32_t iYInverse, iXInverse;

  uint8_t* pByDst = pDst;
  uint8_t* pByLineDst = pDst;

  iYInverse = 1 << (kiScaleBit - 1);
  for (int32_t i = 0; i < kiDstHeight - 1; i++) {
    int32_t iYy = iYInverse >> kiScaleBit;
    int32_t iFv = iYInverse & (kiScale - 1);

    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;

    pByDst = pByLineDst;
    iXInverse = 1 << (kiScaleBit - 1);
    for (int32_t j = 0; j < kiDstWidth - 1; j++) {
      int32_t iXx = iXInverse >> kiScaleBit;
      int32_t iFu = iXInverse & (kiScale - 1);

      uint8_t* pByCurrent = pBySrc + iXx;
      uint8_t a, b, c, d;

      a = *pByCurrent;
      b = * (pByCurrent + 1);
      c = * (pByCurrent + kiSrcStride);
      d = * (pByCurrent + kiSrcStride + 1);

      x = (((int64_t) (kiScale - 1 - iFu)) * (kiScale - 1 - iFv) * a + ((int64_t)iFu) * (kiScale - 1 - iFv) * b + ((int64_t) (
             kiScale - 1 - iFu)) * iFv * c +
           ((int64_t)iFu) * iFv * d + (int64_t) (1 << (2 * kiScaleBit - 1))) >> (2 * kiScaleBit);
      x = WELS_CLAMP (x, 0, 255);
      *pByDst++ = (uint8_t)x;

      iXInverse += iScalex;
    }
    *pByDst = * (pBySrc + (iXInverse >> kiScaleBit));
    pByLineDst += kiDstStride;
    iYInverse += iScaley;
  }

  // last row special
  {
    int32_t iYy = iYInverse >> kiScaleBit;
    uint8_t* pBySrc = pSrc + iYy * kiSrcStride;

    pByDst = pByLineDst;
    iXInverse = 1 << (kiScaleBit - 1);
    for (int32_t j = 0; j < kiDstWidth; j++) {
      int32_t iXx = iXInverse >> kiScaleBit;
      *pByDst++ = * (pBySrc + iXx);

      iXInverse += iScalex;
    }
  }
}


#ifdef X86_ASM
//void GeneralBilinearFastDownsamplerWrap_sse2 (uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth,
//    const int32_t kiDstHeight,
//    uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight) {
//  const int32_t kiScaleBitWidth = 16, kiScaleBitHeight = 15;
//  const uint32_t kuiScaleWidth = (1 << kiScaleBitWidth), kuiScaleHeight = (1 << kiScaleBitHeight);
//
//  uint32_t uiScalex = (uint32_t) ((float_t)kiSrcWidth / (float_t)kiDstWidth * kuiScaleWidth);
//  uint32_t uiScaley = (uint32_t) ((float_t)kiSrcHeight / (float_t)kiDstHeight * kuiScaleHeight);
//
//  GeneralBilinearFastDownsampler_sse2 (pDst, kiDstStride, kiDstWidth, kiDstHeight,
//                                       pSrc, kiSrcStride, kiSrcWidth, kiSrcHeight, uiScalex, uiScaley);
//}
//
//void GeneralBilinearAccurateDownsamplerWrap_sse2 (uint8_t* pDst, const int32_t kiDstStride, const int32_t kiDstWidth,
//    const int32_t kiDstHeight,
//    uint8_t* pSrc, const int32_t kiSrcStride, const int32_t kiSrcWidth, const int32_t kiSrcHeight) {
//  const int32_t kiScaleBit = 15;
//  const uint32_t kuiScale = (1 << kiScaleBit);
//
//  uint32_t uiScalex = (uint32_t) ((float_t)kiSrcWidth / (float_t)kiDstWidth * kuiScale);
//  uint32_t uiScaley = (uint32_t) ((float_t)kiSrcHeight / (float_t)kiDstHeight * kuiScale);
//
//  GeneralBilinearAccurateDownsampler_sse2 (pDst, kiDstStride, kiDstWidth, kiDstHeight,
//      pSrc, kiSrcStride, kiSrcWidth, kiSrcHeight, uiScalex, uiScaley);
//}
#endif //X86_ASM

WELSVP_NAMESPACE_END
