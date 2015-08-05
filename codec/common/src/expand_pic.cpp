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
#include <string.h>
#include "expand_pic.h"
#include "cpu_core.h"

// rewrite it (split into luma & chroma) that is helpful for mmx/sse2 optimization perform, 9/27/2009
static inline void ExpandPictureLuma_c (uint8_t* pDst, const int32_t kiStride, const int32_t kiPicW,
                                        const int32_t kiPicH) {
  uint8_t* pTmp              = pDst;
  uint8_t* pDstLastLine      = pTmp + (kiPicH - 1) * kiStride;
  const int32_t kiPaddingLen = PADDING_LENGTH;
  const uint8_t kuiTL        = pTmp[0];
  const uint8_t kuiTR        = pTmp[kiPicW - 1];
  const uint8_t kuiBL        = pDstLastLine[0];
  const uint8_t kuiBR        = pDstLastLine[kiPicW - 1];
  int32_t i                  = 0;

  do {
    const int32_t kiStrides = (1 + i) * kiStride;
    uint8_t* pTop           = pTmp - kiStrides;
    uint8_t* pBottom        = pDstLastLine + kiStrides;

    // pad pTop and pBottom
    memcpy (pTop, pTmp, kiPicW);                // confirmed_safe_unsafe_usage
    memcpy (pBottom, pDstLastLine, kiPicW);     // confirmed_safe_unsafe_usage

    // pad corners
    memset (pTop - kiPaddingLen, kuiTL, kiPaddingLen); //pTop left
    memset (pTop + kiPicW, kuiTR, kiPaddingLen); //pTop right
    memset (pBottom - kiPaddingLen, kuiBL, kiPaddingLen); //pBottom left
    memset (pBottom + kiPicW, kuiBR, kiPaddingLen); //pBottom right

    ++ i;
  } while (i < kiPaddingLen);

  // pad left and right
  i = 0;
  do {
    memset (pTmp - kiPaddingLen, pTmp[0], kiPaddingLen);
    memset (pTmp + kiPicW, pTmp[kiPicW - 1], kiPaddingLen);

    pTmp += kiStride;
    ++ i;
  } while (i < kiPicH);
}

static inline void ExpandPictureChroma_c (uint8_t* pDst, const int32_t kiStride, const int32_t kiPicW,
    const int32_t kiPicH) {
  uint8_t* pTmp                 = pDst;
  uint8_t* pDstLastLine         = pTmp + (kiPicH - 1) * kiStride;
  const int32_t kiPaddingLen    = (PADDING_LENGTH >> 1);
  const uint8_t kuiTL           = pTmp[0];
  const uint8_t kuiTR           = pTmp[kiPicW - 1];
  const uint8_t kuiBL           = pDstLastLine[0];
  const uint8_t kuiBR           = pDstLastLine[kiPicW - 1];
  int32_t i                     = 0;

  do {
    const int32_t kiStrides = (1 + i) * kiStride;
    uint8_t* pTop           = pTmp - kiStrides;
    uint8_t* pBottom        = pDstLastLine + kiStrides;

    // pad pTop and pBottom
    memcpy (pTop, pTmp, kiPicW);                // confirmed_safe_unsafe_usage
    memcpy (pBottom, pDstLastLine, kiPicW);     // confirmed_safe_unsafe_usage

    // pad corners
    memset (pTop - kiPaddingLen, kuiTL, kiPaddingLen); //pTop left
    memset (pTop + kiPicW, kuiTR, kiPaddingLen); //pTop right
    memset (pBottom - kiPaddingLen, kuiBL, kiPaddingLen); //pBottom left
    memset (pBottom + kiPicW, kuiBR, kiPaddingLen); //pBottom right

    ++ i;
  } while (i < kiPaddingLen);

  // pad left and right
  i = 0;
  do {
    memset (pTmp - kiPaddingLen, pTmp[0], kiPaddingLen);
    memset (pTmp + kiPicW, pTmp[kiPicW - 1], kiPaddingLen);

    pTmp += kiStride;
    ++ i;
  } while (i < kiPicH);
}

void InitExpandPictureFunc (SExpandPicFunc* pExpandPicFunc, const uint32_t kuiCPUFlag) {
  pExpandPicFunc->pfExpandLumaPicture        = ExpandPictureLuma_c;
  pExpandPicFunc->pfExpandChromaPicture[0]   = ExpandPictureChroma_c;
  pExpandPicFunc->pfExpandChromaPicture[1]   = ExpandPictureChroma_c;

#if defined(X86_ASM)
  if ((kuiCPUFlag & WELS_CPU_SSE2) == WELS_CPU_SSE2) {
    pExpandPicFunc->pfExpandLumaPicture      = ExpandPictureLuma_sse2;
    pExpandPicFunc->pfExpandChromaPicture[0] = ExpandPictureChromaUnalign_sse2;
    pExpandPicFunc->pfExpandChromaPicture[1] = ExpandPictureChromaAlign_sse2;
  }
#endif//X86_ASM
#if defined(HAVE_NEON)
  if (kuiCPUFlag & WELS_CPU_NEON) {
    pExpandPicFunc->pfExpandLumaPicture      = ExpandPictureLuma_neon;
    pExpandPicFunc->pfExpandChromaPicture[0] = ExpandPictureChroma_neon;
    pExpandPicFunc->pfExpandChromaPicture[1] = ExpandPictureChroma_neon;
  }
#endif//HAVE_NEON
#if defined(HAVE_NEON_AARCH64)
  if (kuiCPUFlag & WELS_CPU_NEON) {
    pExpandPicFunc->pfExpandLumaPicture      = ExpandPictureLuma_AArch64_neon;
    pExpandPicFunc->pfExpandChromaPicture[0] = ExpandPictureChroma_AArch64_neon;
    pExpandPicFunc->pfExpandChromaPicture[1] = ExpandPictureChroma_AArch64_neon;
  }
#endif//HAVE_NEON_AARCH64
}


//void ExpandReferencingPicture (SPicture* pPic, PExpandPictureFunc pExpLuma, PExpandPictureFunc pExpChrom[2]) {
void ExpandReferencingPicture (uint8_t* pData[3], int32_t iWidth, int32_t iHeight, int32_t iStride[3],
                               PExpandPictureFunc pExpLuma, PExpandPictureFunc pExpChrom[2]) {
  /*local variable*/
  uint8_t* pPicY  = pData[0];
  uint8_t* pPicCb = pData[1];
  uint8_t* pPicCr = pData[2];
  const int32_t kiWidthY    = iWidth;
  const int32_t kiHeightY   = iHeight;
  const int32_t kiWidthUV   = kiWidthY >> 1;
  const int32_t kiHeightUV  = kiHeightY >> 1;



  pExpLuma (pPicY, iStride[0], kiWidthY, kiHeightY);
  if (kiWidthUV >= 16) {
    // fix coding picture size as 16x16
    const bool kbChrAligned = /*(iWidthUV >= 16) && */ ((kiWidthUV & 0x0F) == 0); // chroma planes: (16+iWidthUV) & 15
    pExpChrom[kbChrAligned] (pPicCb, iStride[1], kiWidthUV, kiHeightUV);
    pExpChrom[kbChrAligned] (pPicCr, iStride[2], kiWidthUV, kiHeightUV);
  } else {
    // fix coding picture size as 16x16
    ExpandPictureChroma_c (pPicCb, iStride[1], kiWidthUV, kiHeightUV);
    ExpandPictureChroma_c (pPicCr, iStride[2], kiWidthUV, kiHeightUV);
  }



}
