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
 * \file	mc.c
 *
 * \brief	Interfaces implementation for motion compensation
 *
 * \date	03/17/2009 Created
 *
 *************************************************************************************
 */

#include "mc.h"

#include "cpu_core.h"

namespace WelsDec {

/*------------------weight for chroma fraction pixel interpolation------------------*/
//iA = (8 - dx) * (8 - dy);
//iB = dx * (8 - dy);
//iC = (8 - dx) * dy;
//iD = dx * dy
static const uint8_t g_kuiABCD[8][8][4] = {	//g_kA[dy][dx], g_kB[dy][dx], g_kC[dy][dx], g_kD[dy][dx]
  {
    {64, 0, 0, 0}, {56, 8, 0, 0}, {48, 16, 0, 0}, {40, 24, 0, 0},
    {32, 32, 0, 0}, {24, 40, 0, 0}, {16, 48, 0, 0}, {8, 56, 0, 0}
  },
  {
    {56, 0, 8, 0}, {49, 7, 7, 1}, {42, 14, 6, 2}, {35, 21, 5, 3},
    {28, 28, 4, 4}, {21, 35, 3, 5}, {14, 42, 2, 6}, {7, 49, 1, 7}
  },
  {
    {48, 0, 16, 0}, {42, 6, 14, 2}, {36, 12, 12, 4}, {30, 18, 10, 6},
    {24, 24, 8, 8}, {18, 30, 6, 10}, {12, 36, 4, 12}, {6, 42, 2, 14}
  },
  {
    {40, 0, 24, 0}, {35, 5, 21, 3}, {30, 10, 18, 6}, {25, 15, 15, 9},
    {20, 20, 12, 12}, {15, 25, 9, 15}, {10, 30, 6, 18}, {5, 35, 3, 21}
  },
  {
    {32, 0, 32, 0}, {28, 4, 28, 4}, {24, 8, 24, 8}, {20, 12, 20, 12},
    {16, 16, 16, 16}, {12, 20, 12, 20}, {8, 24, 8, 24}, {4, 28, 4, 28}
  },
  {
    {24, 0, 40, 0}, {21, 3, 35, 5}, {18, 6, 30, 10}, {15, 9, 25, 15},
    {12, 12, 20, 20}, {9, 15, 15, 25}, {6, 18, 10, 30}, {3, 21, 5, 35}
  },
  {
    {16, 0, 48, 0}, {14, 2, 42, 6}, {12, 4, 36, 12}, {10, 6, 30, 18},
    {8, 8, 24, 24}, {6, 10, 18, 30}, {4, 12, 12, 36}, {2, 14, 6, 42}
  },
  {
    {8, 0, 56, 0}, {7, 1, 49, 7}, {6, 2, 42, 14}, {5, 3, 35, 21},
    {4, 4, 28, 28}, {3, 5, 21, 35}, {2, 6, 14, 42}, {1, 7, 7, 49}
  }
};

typedef void_t (*PWelsMcWidthHeightFunc) (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int32_t iWidth, int32_t iHeight);

//***************************************************************************//
//                          C code implementation                            //
//***************************************************************************//
static inline void_t McCopyWidthEq2_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                       int32_t iHeight) {
  int32_t i;
  for (i = 0; i < iHeight; i++) { // iWidth == 2 only for chroma
    ST16 (pDst, LD16 (pSrc));
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

static inline void_t McCopyWidthEq4_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                       int32_t iHeight) {
  int32_t i;
  for (i = 0; i < iHeight; i++) {
    ST32 (pDst, LD32 (pSrc));
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

static inline void_t McCopyWidthEq8_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                       int32_t iHeight) {
  int32_t i;
  for (i = 0; i < iHeight; i++) {
    ST64 (pDst, LD64 (pSrc));
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

static inline void_t McCopyWidthEq16_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                        int32_t iHeight) {
  int32_t i;
  for (i = 0; i < iHeight; i++) {
    ST64 (pDst  , LD64 (pSrc));
    ST64 (pDst + 8, LD64 (pSrc + 8));
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

//--------------------Luma sample MC------------------//

static inline int32_t HorFilterInput16bit_c (int16_t* pSrc) {
  int32_t iPix05 = pSrc[-2] + pSrc[3];
  int32_t iPix14 = pSrc[-1] + pSrc[2];
  int32_t iPix23 = pSrc[ 0] + pSrc[1];

  return (iPix05 - ((iPix14 << 2) + iPix14) + (iPix23 << 4) + (iPix23 << 2));
}
// h: iOffset=1 / v: iOffset=iSrcStride
static inline int32_t FilterInput8bitWithStride_c (uint8_t* pSrc, const int32_t kiOffset) {
  const int32_t kiOffset1 = kiOffset;
  const int32_t kiOffset2 = (kiOffset << 1);
  const int32_t kiOffset3 = kiOffset + kiOffset2;
  const uint32_t kuiPix05   = * (pSrc - kiOffset2) + * (pSrc + kiOffset3);
  const uint32_t kuiPix14   = * (pSrc - kiOffset1) + * (pSrc + kiOffset2);
  const uint32_t kuiPix23   = * (pSrc) + * (pSrc + kiOffset1);

  return (kuiPix05 - ((kuiPix14 << 2) + kuiPix14) + (kuiPix23 << 4) + (kuiPix23 << 2));
}

static inline void_t PixelAvg_c (uint8_t* pDst, int32_t iDstStride, uint8_t* pSrcA, int32_t iSrcAStride,
                                 uint8_t* pSrcB, int32_t iSrcBStride, int32_t iWidth, int32_t iHeight) {
  int32_t i, j;
  for (i = 0; i < iHeight; i++) {
    for (j = 0; j < iWidth; j++) {
      pDst[j] = (pSrcA[j] + pSrcB[j] + 1) >> 1;
    }
    pDst  += iDstStride;
    pSrcA += iSrcAStride;
    pSrcB += iSrcBStride;
  }
}
static inline void_t McCopy_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                               int32_t iHeight) {
  if (iWidth == 16)
    McCopyWidthEq16_c (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McCopyWidthEq8_c (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McCopyWidthEq4_c (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else //here iWidth == 2
    McCopyWidthEq2_c (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}

static inline void_t McHorVer20_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  int32_t i, j;
  for (i = 0; i < iHeight; i++) {
    for (j = 0; j < iWidth; j++) {
      pDst[j] = WELS_CLIP1 ((FilterInput8bitWithStride_c (pSrc + j, 1) + 16) >> 5);
    }
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

static inline void_t McHorVer02_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  int32_t i, j;
  for (i = 0; i < iHeight; i++) {
    for (j = 0; j < iWidth; j++) {
      pDst[j] = WELS_CLIP1 ((FilterInput8bitWithStride_c (pSrc + j, iSrcStride) + 16) >> 5);
    }
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

static inline void_t McHorVer22_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  int16_t iTmp[16 + 5] = {0}; //16
  int32_t i, j, k;

  for (i = 0; i < iHeight; i++) {
    for (j = 0; j < iWidth + 5; j++) {
      iTmp[j] = FilterInput8bitWithStride_c (pSrc - 2 + j, iSrcStride);
    }
    for (k = 0; k < iWidth; k++) {
      pDst[k] = WELS_CLIP1 ((HorFilterInput16bit_c (&iTmp[2 + k]) + 512) >> 10);
    }
    pSrc += iSrcStride;
    pDst += iDstStride;
  }
}

/////////////////////luma MC//////////////////////////
static inline void_t McHorVer01_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiTmp[256] = { 0 };
  McHorVer02_c (pSrc, iSrcStride, uiTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, pSrc, iSrcStride, uiTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer03_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiTmp[256] = { 0 };
  McHorVer02_c (pSrc, iSrcStride, uiTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, pSrc + iSrcStride, iSrcStride, uiTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer10_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiTmp[256] = { 0 };
  McHorVer20_c (pSrc, iSrcStride, uiTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, pSrc, iSrcStride, uiTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer11_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiHorTmp[256] = { 0 };
  uint8_t uiVerTmp[256] = { 0 };
  McHorVer20_c (pSrc, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer02_c (pSrc, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiVerTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer12_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiVerTmp[256] = { 0 };
  uint8_t uiCtrTmp[256] = { 0 };
  McHorVer02_c (pSrc, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  McHorVer22_c (pSrc, iSrcStride, uiCtrTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiVerTmp, 16, uiCtrTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer13_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiHorTmp[256] = { 0 };
  uint8_t uiVerTmp[256] = { 0 };
  McHorVer20_c (pSrc + iSrcStride, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer02_c (pSrc, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiVerTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer21_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiHorTmp[256] = { 0 };
  uint8_t uiCtrTmp[256] = { 0 };
  McHorVer20_c (pSrc, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer22_c (pSrc, iSrcStride, uiCtrTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiCtrTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer23_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiHorTmp[256] = { 0 };
  uint8_t uiCtrTmp[256] = { 0 };
  McHorVer20_c (pSrc + iSrcStride, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer22_c (pSrc, iSrcStride, uiCtrTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiCtrTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer30_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiHorTmp[256] = { 0 };
  McHorVer20_c (pSrc, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, pSrc + 1, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer31_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiHorTmp[256] = { 0 };
  uint8_t uiVerTmp[256] = { 0 };
  McHorVer20_c (pSrc, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer02_c (pSrc + 1, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiVerTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer32_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiVerTmp[256] = { 0 };
  uint8_t uiCtrTmp[256] = { 0 };
  McHorVer02_c (pSrc + 1, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  McHorVer22_c (pSrc, iSrcStride, uiCtrTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiVerTmp, 16, uiCtrTmp, 16, iWidth, iHeight);
}
static inline void_t McHorVer33_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                   int32_t iHeight) {
  uint8_t uiHorTmp[256] = { 0 };
  uint8_t uiVerTmp[256] = { 0 };
  McHorVer20_c (pSrc + iSrcStride, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer02_c (pSrc + 1, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiVerTmp, 16, iWidth, iHeight);
}

void_t McLuma_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                 int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight)
//pSrc has been added the offset of mv
{
  PWelsMcWidthHeightFunc pWelsMcFunc[4][4] = { //[x][y]
    {McCopy_c,      McHorVer01_c, McHorVer02_c, McHorVer03_c},
    {McHorVer10_c,  McHorVer11_c, McHorVer12_c, McHorVer13_c},
    {McHorVer20_c,  McHorVer21_c, McHorVer22_c, McHorVer23_c},
    {McHorVer30_c,  McHorVer31_c, McHorVer32_c, McHorVer33_c},
  };

  pWelsMcFunc[iMvX & 0x03][iMvY & 0x03] (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
}

static inline void_t McChromaWithFragMv_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight) {
  int32_t i, j;
  int32_t iA, iB, iC, iD;
  uint8_t* pSrcNext = pSrc + iSrcStride;
  const uint8_t *pABCD = g_kuiABCD[iMvY & 0x07][iMvX & 0x07];
  iA = pABCD[0];
  iB = pABCD[1];
  iC = pABCD[2];
  iD = pABCD[3];
  for (i = 0; i < iHeight; i++) {
    for (j = 0; j < iWidth; j++) {
      pDst[j] = (iA * pSrc[j] + iB * pSrc[j + 1] + iC * pSrcNext[j] + iD * pSrcNext[j + 1] + 32) >> 6;
    }
    pDst     += iDstStride;
    pSrc      = pSrcNext;
    pSrcNext += iSrcStride;
  }
}

void_t McChroma_c (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                   int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight)
//pSrc has been added the offset of mv
{
  const int32_t kiD8x = iMvX & 0x07;
  const int32_t kiD8y = iMvY & 0x07;
  if (0 == kiD8x && 0 == kiD8y)
    McCopy_c (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
  else
    McChromaWithFragMv_c (pSrc, iSrcStride, pDst, iDstStride, iMvX, iMvY, iWidth, iHeight);
}

#if defined(X86_ASM)
//***************************************************************************//
//                       SSE2 implement                          //
//***************************************************************************//
static inline void_t McHorVer22WidthEq8_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int32_t iHeight) {
  ENFORCE_STACK_ALIGN_2D (int16_t, iTap, 21, 8, 16)
  McHorVer22Width8HorFirst_sse2 (pSrc - 2, iSrcStride, (uint8_t*)iTap, 16, iHeight + 5);
  McHorVer22Width8VerLastAlign_sse2 ((uint8_t*)iTap, 16, pDst, iDstStride, 8, iHeight);
}

static inline void_t McHorVer02WidthEq16_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int32_t iHeight) {
  McHorVer02WidthEq8_sse2 (pSrc,     iSrcStride, pDst,     iDstStride, iHeight);
  McHorVer02WidthEq8_sse2 (&pSrc[8], iSrcStride, &pDst[8], iDstStride, iHeight);
}

static inline void_t McHorVer22WidthEq16_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int32_t iHeight) {
  McHorVer22WidthEq8_sse2 (pSrc,     iSrcStride, pDst,     iDstStride, iHeight);
  McHorVer22WidthEq8_sse2 (&pSrc[8], iSrcStride, &pDst[8], iDstStride, iHeight);
}

static inline void_t McCopy_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                  int32_t iHeight) {
  if (iWidth == 16)
    McCopyWidthEq16_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McCopyWidthEq8_mmx (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McCopyWidthEq4_mmx (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else
    McCopyWidthEq2_c (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}

static inline void_t McHorVer20_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer20WidthEq16_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer20WidthEq8_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else
    McHorVer20WidthEq4_mmx (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}

static inline void_t McHorVer02_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer02WidthEq16_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer02WidthEq8_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else
    McHorVer02_c (pSrc, iSrcStride, pDst, iDstStride, 4, iHeight);
}

static inline void_t McHorVer22_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer22WidthEq16_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer22WidthEq8_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else
    McHorVer22_c (pSrc, iSrcStride, pDst, iDstStride, 4, iHeight);
}

static inline void_t McHorVer01_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer02WidthEq16_sse2 (pSrc, iSrcStride, pTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pSrc, iSrcStride, pTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer02WidthEq8_sse2 (pSrc, iSrcStride, pTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pSrc, iSrcStride, pTmp, 16, iHeight);
  } else {
    McHorVer02_c (pSrc, iSrcStride, pTmp, 16, 4, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pSrc, iSrcStride, pTmp, 16, iHeight);
  }
}
static inline void_t McHorVer03_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer02WidthEq16_sse2 (pSrc, iSrcStride, pTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pSrc + iSrcStride, iSrcStride, pTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer02WidthEq8_sse2 (pSrc, iSrcStride, pTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pSrc + iSrcStride, iSrcStride, pTmp, 16, iHeight);
  } else {
    McHorVer02_c (pSrc, iSrcStride, pTmp, 16, 4, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pSrc + iSrcStride, iSrcStride, pTmp, 16, iHeight);
  }
}
static inline void_t McHorVer10_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_sse2 (pSrc, iSrcStride, pTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pSrc, iSrcStride, pTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_sse2 (pSrc, iSrcStride, pTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pSrc, iSrcStride, pTmp, 16, iHeight);
  } else {
    McHorVer20WidthEq4_mmx (pSrc, iSrcStride, pTmp, 16, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pSrc, iSrcStride, pTmp, 16, iHeight);
  }
}
static inline void_t McHorVer11_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  FORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_sse2 (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_sse2 (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_sse2 (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_sse2 (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else {
    McHorVer20WidthEq4_mmx (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02_c (pSrc, iSrcStride, pVerTmp, 16, 4, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  }
}
static inline void_t McHorVer12_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  FORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer02WidthEq16_sse2 (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq16_sse2 (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer02WidthEq8_sse2 (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq8_sse2 (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  } else {
    McHorVer02_c (pSrc, iSrcStride, pVerTmp, 16, 4, iHeight);
    McHorVer22_c (pSrc, iSrcStride, pCtrTmp, 16, 4, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  }
}
static inline void_t McHorVer13_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  FORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_sse2 (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_sse2 (pSrc,            iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_sse2 (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_sse2 (pSrc,            iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else {
    McHorVer20WidthEq4_mmx (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02_c (pSrc,            iSrcStride, pVerTmp, 16, 4 , iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  }
}
static inline void_t McHorVer21_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  FORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_sse2 (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq16_sse2 (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_sse2 (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq8_sse2 (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  } else {
    McHorVer20WidthEq4_mmx (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22_c (pSrc, iSrcStride, pCtrTmp, 16, 4, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  }
}
static inline void_t McHorVer23_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  FORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_sse2 (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq16_sse2 (pSrc,            iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_sse2 (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq8_sse2 (pSrc,            iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  } else {
    McHorVer20WidthEq4_mmx (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22_c (pSrc,            iSrcStride, pCtrTmp, 16, 4, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  }
}
static inline void_t McHorVer30_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_sse2 (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pSrc + 1, iSrcStride, pHorTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_sse2 (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pSrc + 1, iSrcStride, pHorTmp, 16, iHeight);
  } else {
    McHorVer20WidthEq4_mmx (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pSrc + 1, iSrcStride, pHorTmp, 16, iHeight);
  }
}
static inline void_t McHorVer31_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  FORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_sse2 (pSrc,   iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_sse2 (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_sse2 (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_sse2 (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else {
    McHorVer20WidthEq4_mmx (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02_c (pSrc + 1, iSrcStride, pVerTmp, 16, 4, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  }
}
static inline void_t McHorVer32_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  FORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer02WidthEq16_sse2 (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq16_sse2 (pSrc,   iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer02WidthEq8_sse2 (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq8_sse2 (pSrc,   iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  } else {
    McHorVer02_c (pSrc + 1, iSrcStride, pVerTmp, 16, 4, iHeight);
    McHorVer22_c (pSrc,   iSrcStride, pCtrTmp, 16, 4, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  }
}
static inline void_t McHorVer33_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight) {
  FORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  FORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_sse2 (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_sse2 (pSrc + 1,          iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_sse2 (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_sse2 (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_sse2 (pSrc + 1,          iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_mmx (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else {
    McHorVer20WidthEq4_mmx (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02_c (pSrc + 1,          iSrcStride, pVerTmp, 16, 4, iHeight);
    PixelAvgWidthEq4_mmx (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  }
}

void_t McLuma_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                    int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight)
//pSrc has been added the offset of mv
{
  PWelsMcWidthHeightFunc pWelsMcFunc[4][4] = { //[x][y]
    {McCopy_sse2,     McHorVer01_sse2, McHorVer02_sse2, McHorVer03_sse2},
    {McHorVer10_sse2, McHorVer11_sse2, McHorVer12_sse2, McHorVer13_sse2},
    {McHorVer20_sse2, McHorVer21_sse2, McHorVer22_sse2, McHorVer23_sse2},
    {McHorVer30_sse2, McHorVer31_sse2, McHorVer32_sse2, McHorVer33_sse2},
  };

  pWelsMcFunc[iMvX & 0x03][iMvY & 0x03] (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
}

void_t McChroma_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight) {
  static const PMcChromaWidthExtFunc kpMcChromaWidthFuncs[2] = {
    McChromaWidthEq4_mmx,
    McChromaWidthEq8_sse2
  };
  const int32_t kiD8x = iMvX & 0x07;
  const int32_t kiD8y = iMvY & 0x07;
  if (kiD8x == 0 && kiD8y == 0) {
    McCopy_sse2 (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
    return;
  }
  if (iWidth != 2) {
    kpMcChromaWidthFuncs[iWidth >> 3] (pSrc, iSrcStride, pDst, iDstStride, g_kuiABCD[kiD8y][kiD8x], iHeight);
  } else
    McChromaWithFragMv_c (pSrc, iSrcStride, pDst, iDstStride, iMvX, iMvY, iWidth, iHeight);
}


#endif //X86_ASM

void_t InitMcFunc (SMcFunc* pMcFunc, int32_t iCpu) {
  pMcFunc->pMcLumaFunc   = McLuma_c;
  pMcFunc->pMcChromaFunc = McChroma_c;

#if defined (X86_ASM)
  if (iCpu & WELS_CPU_SSE2) {
    pMcFunc->pMcLumaFunc   = McLuma_sse2;
    pMcFunc->pMcChromaFunc = McChroma_sse2;
  }
#endif //(X86_ASM)
}

} // namespace WelsDec