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


#include "expand_pic.h"
#include "cpu_core.h"

namespace WelsDec {

// rewrite it (split into luma & chroma) that is helpful for mmx/sse2 optimization perform, 9/27/2009
static inline void_t ExpandPictureLuma_c (uint8_t* pDst, const int32_t kiStride, const int32_t kiPicWidth,
    const int32_t kiPicHeight) {
  uint8_t* pTmp				= pDst;
  uint8_t* pDstLastLine		= pTmp + (kiPicHeight - 1) * kiStride;
  const int32_t kiPaddingLen	= PADDING_LENGTH;
  const uint8_t kuiTopLeft	= pTmp[0];
  const uint8_t kuiTopRight	= pTmp[kiPicWidth - 1];
  const uint8_t kuiBottomLeft	= pDstLastLine[0];
  const uint8_t kuiBottomRight = pDstLastLine[kiPicWidth - 1];
  int32_t i					= 0;

  do {
    const int32_t kiStrides	= (1 + i) * kiStride;
    uint8_t* pTop			= pTmp - kiStrides;
    uint8_t* pBottom		= pDstLastLine + kiStrides;

    // pad pTop and pBottom
    memcpy (pTop, pTmp, kiPicWidth);
    memcpy (pBottom, pDstLastLine, kiPicWidth);

    // pad corners
    memset (pTop - kiPaddingLen,    kuiTopLeft,     kiPaddingLen); //pTop left
    memset (pTop + kiPicWidth,      kuiTopRight,    kiPaddingLen); //pTop right
    memset (pBottom - kiPaddingLen, kuiBottomLeft,  kiPaddingLen); //pBottom left
    memset (pBottom + kiPicWidth,   kuiBottomRight, kiPaddingLen); //pBottom right

    ++ i;
  } while (i < kiPaddingLen);

  // pad left and right
  i = 0;
  do {
    memset (pTmp - kiPaddingLen, pTmp[0], kiPaddingLen);
    memset (pTmp + kiPicWidth, pTmp[kiPicWidth - 1], kiPaddingLen);

    pTmp += kiStride;
    ++ i;
  } while (i < kiPicHeight);
}

static inline void_t ExpandPictureChroma_c (uint8_t* pDst, const int32_t kiStride, const int32_t kiPicWidth,
    const int32_t kiPicHeight) {
  uint8_t* pTmp				= pDst;
  uint8_t* pDstLastLine		= pTmp + (kiPicHeight - 1) * kiStride;
  const int32_t kiPaddingLen	= (PADDING_LENGTH >> 1);
  const uint8_t kuiTopLeft	= pTmp[0];
  const uint8_t kuiTopRight	= pTmp[kiPicWidth - 1];
  const uint8_t kuiBottomLeft	= pDstLastLine[0];
  const uint8_t kuiBottomRight = pDstLastLine[kiPicWidth - 1];
  int32_t i					= 0;

  do {
    const int32_t kiStrides	= (1 + i) * kiStride;
    uint8_t* pTop			= pTmp - kiStrides;
    uint8_t* pBottom		= pDstLastLine + kiStrides;

    // pad pTop and pBottom
    memcpy (pTop, pTmp, kiPicWidth);
    memcpy (pBottom, pDstLastLine, kiPicWidth);

    // pad corners
    memset (pTop - kiPaddingLen,    kuiTopLeft,     kiPaddingLen); //pTop left
    memset (pTop + kiPicWidth,      kuiTopRight,    kiPaddingLen); //pTop right
    memset (pBottom - kiPaddingLen, kuiBottomLeft,  kiPaddingLen); //pBottom left
    memset (pBottom + kiPicWidth,   kuiBottomRight, kiPaddingLen); //pBottom right

    ++ i;
  } while (i < kiPaddingLen);

  // pad left and right
  i = 0;
  do {
    memset (pTmp - kiPaddingLen, pTmp[0], kiPaddingLen);
    memset (pTmp + kiPicWidth, pTmp[kiPicWidth - 1], kiPaddingLen);

    pTmp += kiStride;
    ++ i;
  } while (i < kiPicHeight);
}

void_t InitExpandPictureFunc (SExpandPicFunc* pExpandPicFunc, const uint32_t kuiCpuFlags) {
  pExpandPicFunc->pExpandLumaPicture	= ExpandPictureLuma_c;
  pExpandPicFunc->pExpandChromaPicture[0] = ExpandPictureChroma_c;
  pExpandPicFunc->pExpandChromaPicture[1] = ExpandPictureChroma_c;

#if defined(X86_ASM)
  if ((kuiCpuFlags & WELS_CPU_SSE2) == WELS_CPU_SSE2) {
    pExpandPicFunc->pExpandLumaPicture	   = ExpandPictureLuma_sse2;
    pExpandPicFunc->pExpandChromaPicture[0] = ExpandPictureChromaUnalign_sse2;
    pExpandPicFunc->pExpandChromaPicture[1] = ExpandPictureChromaAlign_sse2;
  }
#endif//X86_ASM
}

void_t ExpandReferencingPicture (PPicture pPic, PExpandPictureFunc pExpLuma, PExpandPictureFunc pExpChroma[2]) {
  /*local variable*/
  uint8_t* pPicY = pPic->pData[0];
  uint8_t* pPicCb = pPic->pData[1];
  uint8_t* pPicCr = pPic->pData[2];
  const int32_t kiWidthY	= pPic->iWidthInPixel;
  const int32_t kiHeightY	= pPic->iHeightInPixel;
  const int32_t kiWidthUV	= kiWidthY >> 1;
  const int32_t kiHeightUV = kiHeightY >> 1;

  pExpLuma (pPicY, pPic->iLinesize[0], kiWidthY, kiHeightY);
  if (kiWidthUV >= 16) {
    // fix coding picture size as 16x16 issues 7/27/2010
    const bool_t kbChrAligned = /*(kiWidthUV >= 16) && */ ((kiWidthUV & 0x0F) == 0);	// chroma planes: (16+kiWidthUV) & 15
    pExpChroma[kbChrAligned] (pPicCb, pPic->iLinesize[1], kiWidthUV, kiHeightUV);
    pExpChroma[kbChrAligned] (pPicCr, pPic->iLinesize[2], kiWidthUV, kiHeightUV);
  } else {
    // fix coding picture size as 16x16 issues 7/27/2010
    ExpandPictureChroma_c (pPicCb, pPic->iLinesize[1], kiWidthUV, kiHeightUV);
    ExpandPictureChroma_c (pPicCr, pPic->iLinesize[2], kiWidthUV, kiHeightUV);
  }
}

} // namespace WelsDec
