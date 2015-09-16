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
 * \file    mc.c
 *
 * \brief   Interfaces implementation for motion compensation
 *
 * \date    03/17/2009 Created
 *
 *************************************************************************************
 */

#include "mc.h"

#include "cpu_core.h"
#include "ls_defines.h"
#include "macros.h"

typedef void (*PMcChromaWidthExtFunc) (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                       const uint8_t* kpABCD, int32_t iHeight);
typedef void (*PWelsSampleWidthAveragingFunc) (uint8_t*, int32_t, const uint8_t*, int32_t, const uint8_t*,
    int32_t, int32_t);
typedef void (*PWelsMcWidthHeightFunc) (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                        int32_t iWidth, int32_t iHeight);

namespace WelsCommon {

/*------------------weight for chroma fraction pixel interpolation------------------*/
//iA = (8 - dx) * (8 - dy);
//iB = dx * (8 - dy);
//iC = (8 - dx) * dy;
//iD = dx * dy
static const uint8_t g_kuiABCD[8][8][4] = { //g_kA[dy][dx], g_kB[dy][dx], g_kC[dy][dx], g_kD[dy][dx]
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

//***************************************************************************//
//                          C code implementation                            //
//***************************************************************************//
static inline void McCopyWidthEq2_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                     int32_t iHeight) {
  int32_t i;
  for (i = 0; i < iHeight; i++) { // iWidth == 2 only for chroma
    ST16A2 (pDst, LD16 (pSrc));
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

static inline void McCopyWidthEq4_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                     int32_t iHeight) {
  int32_t i;
  for (i = 0; i < iHeight; i++) {
    ST32A4 (pDst, LD32 (pSrc));
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

static inline void McCopyWidthEq8_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                     int32_t iHeight) {
  int32_t i;
  for (i = 0; i < iHeight; i++) {
    ST64A8 (pDst, LD64 (pSrc));
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

static inline void McCopyWidthEq16_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iHeight) {
  int32_t i;
  for (i = 0; i < iHeight; i++) {
    ST64A8 (pDst  , LD64 (pSrc));
    ST64A8 (pDst + 8, LD64 (pSrc + 8));
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

//--------------------Luma sample MC------------------//

static inline int32_t HorFilterInput16bit_c (const int16_t* pSrc) {
  int32_t iPix05 = pSrc[0] + pSrc[5];
  int32_t iPix14 = pSrc[1] + pSrc[4];
  int32_t iPix23 = pSrc[2] + pSrc[3];

  return (iPix05 - (iPix14 * 5) + (iPix23 * 20));
}
// h: iOffset=1 / v: iOffset=iSrcStride
static inline int32_t FilterInput8bitWithStride_c (const uint8_t* pSrc, const int32_t kiOffset) {
  const int32_t kiOffset1 = kiOffset;
  const int32_t kiOffset2 = (kiOffset << 1);
  const int32_t kiOffset3 = kiOffset + kiOffset2;
  const uint32_t kuiPix05   = * (pSrc - kiOffset2) + * (pSrc + kiOffset3);
  const uint32_t kuiPix14   = * (pSrc - kiOffset1) + * (pSrc + kiOffset2);
  const uint32_t kuiPix23   = * (pSrc) + * (pSrc + kiOffset1);

  return (kuiPix05 - ((kuiPix14 << 2) + kuiPix14) + (kuiPix23 << 4) + (kuiPix23 << 2));
}

static inline void PixelAvg_c (uint8_t* pDst, int32_t iDstStride, const uint8_t* pSrcA, int32_t iSrcAStride,
                               const uint8_t* pSrcB, int32_t iSrcBStride, int32_t iWidth, int32_t iHeight) {
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
static inline void McCopy_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
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

//horizontal filter to gain half sample, that is (2, 0) location in quarter sample
static inline void McHorVer20_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  int32_t i, j;
  for (i = 0; i < iHeight; i++) {
    for (j = 0; j < iWidth; j++) {
      pDst[j] = WelsClip1 ((FilterInput8bitWithStride_c (pSrc + j, 1) + 16) >> 5);
    }
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

//vertical filter to gain half sample, that is (0, 2) location in quarter sample
static inline void McHorVer02_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  int32_t i, j;
  for (i = 0; i < iHeight; i++) {
    for (j = 0; j < iWidth; j++) {
      pDst[j] = WelsClip1 ((FilterInput8bitWithStride_c (pSrc + j, iSrcStride) + 16) >> 5);
    }
    pDst += iDstStride;
    pSrc += iSrcStride;
  }
}

//horizontal and vertical filter to gain half sample, that is (2, 2) location in quarter sample
static inline void McHorVer22_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  int16_t iTmp[17 + 5];
  int32_t i, j, k;

  for (i = 0; i < iHeight; i++) {
    for (j = 0; j < iWidth + 5; j++) {
      iTmp[j] = FilterInput8bitWithStride_c (pSrc - 2 + j, iSrcStride);
    }
    for (k = 0; k < iWidth; k++) {
      pDst[k] = WelsClip1 ((HorFilterInput16bit_c (&iTmp[k]) + 512) >> 10);
    }
    pSrc += iSrcStride;
    pDst += iDstStride;
  }
}

/////////////////////luma MC//////////////////////////
static inline void McHorVer01_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiTmp[256];
  McHorVer02_c (pSrc, iSrcStride, uiTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, pSrc, iSrcStride, uiTmp, 16, iWidth, iHeight);
}
static inline void McHorVer03_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiTmp[256];
  McHorVer02_c (pSrc, iSrcStride, uiTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, pSrc + iSrcStride, iSrcStride, uiTmp, 16, iWidth, iHeight);
}
static inline void McHorVer10_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiTmp[256];
  McHorVer20_c (pSrc, iSrcStride, uiTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, pSrc, iSrcStride, uiTmp, 16, iWidth, iHeight);
}
static inline void McHorVer11_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiHorTmp[256];
  uint8_t uiVerTmp[256];
  McHorVer20_c (pSrc, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer02_c (pSrc, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiVerTmp, 16, iWidth, iHeight);
}
static inline void McHorVer12_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiVerTmp[256];
  uint8_t uiCtrTmp[256];
  McHorVer02_c (pSrc, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  McHorVer22_c (pSrc, iSrcStride, uiCtrTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiVerTmp, 16, uiCtrTmp, 16, iWidth, iHeight);
}
static inline void McHorVer13_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiHorTmp[256];
  uint8_t uiVerTmp[256];
  McHorVer20_c (pSrc + iSrcStride, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer02_c (pSrc, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiVerTmp, 16, iWidth, iHeight);
}
static inline void McHorVer21_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiHorTmp[256];
  uint8_t uiCtrTmp[256];
  McHorVer20_c (pSrc, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer22_c (pSrc, iSrcStride, uiCtrTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiCtrTmp, 16, iWidth, iHeight);
}
static inline void McHorVer23_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiHorTmp[256];
  uint8_t uiCtrTmp[256];
  McHorVer20_c (pSrc + iSrcStride, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer22_c (pSrc, iSrcStride, uiCtrTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiCtrTmp, 16, iWidth, iHeight);
}
static inline void McHorVer30_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiHorTmp[256];
  McHorVer20_c (pSrc, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, pSrc + 1, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
}
static inline void McHorVer31_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiHorTmp[256];
  uint8_t uiVerTmp[256];
  McHorVer20_c (pSrc, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer02_c (pSrc + 1, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiVerTmp, 16, iWidth, iHeight);
}
static inline void McHorVer32_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiVerTmp[256];
  uint8_t uiCtrTmp[256];
  McHorVer02_c (pSrc + 1, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  McHorVer22_c (pSrc, iSrcStride, uiCtrTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiVerTmp, 16, uiCtrTmp, 16, iWidth, iHeight);
}
static inline void McHorVer33_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth,
                                 int32_t iHeight) {
  uint8_t uiHorTmp[256];
  uint8_t uiVerTmp[256];
  McHorVer20_c (pSrc + iSrcStride, iSrcStride, uiHorTmp, 16, iWidth, iHeight);
  McHorVer02_c (pSrc + 1, iSrcStride, uiVerTmp, 16, iWidth, iHeight);
  PixelAvg_c (pDst, iDstStride, uiHorTmp, 16, uiVerTmp, 16, iWidth, iHeight);
}

void McLuma_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
               int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight)
//pSrc has been added the offset of mv
{
  static const PWelsMcWidthHeightFunc pWelsMcFunc[4][4] = { //[x][y]
    {McCopy_c,      McHorVer01_c, McHorVer02_c, McHorVer03_c},
    {McHorVer10_c,  McHorVer11_c, McHorVer12_c, McHorVer13_c},
    {McHorVer20_c,  McHorVer21_c, McHorVer22_c, McHorVer23_c},
    {McHorVer30_c,  McHorVer31_c, McHorVer32_c, McHorVer33_c},
  };

  pWelsMcFunc[iMvX & 0x03][iMvY & 0x03] (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
}

static inline void McChromaWithFragMv_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight) {
  int32_t i, j;
  int32_t iA, iB, iC, iD;
  const uint8_t* pSrcNext = pSrc + iSrcStride;
  const uint8_t* pABCD = g_kuiABCD[iMvY & 0x07][iMvX & 0x07];
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

void McChroma_c (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
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
static inline void McHorVer22WidthEq8_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int32_t iHeight) {
  ENFORCE_STACK_ALIGN_2D (int16_t, iTap, 21, 8, 16)
  McHorVer22Width8HorFirst_sse2 (pSrc - 2, iSrcStride, (uint8_t*)iTap, 16, iHeight + 5);
  McHorVer22Width8VerLastAlign_sse2 ((uint8_t*)iTap, 16, pDst, iDstStride, 8, iHeight);
}

static inline void McHorVer02WidthEq16_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int32_t iHeight) {
  McHorVer02WidthEq8_sse2 (pSrc,     iSrcStride, pDst,     iDstStride, iHeight);
  McHorVer02WidthEq8_sse2 (&pSrc[8], iSrcStride, &pDst[8], iDstStride, iHeight);
}

static inline void McHorVer22WidthEq16_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int32_t iHeight) {
  McHorVer22WidthEq8_sse2 (pSrc,     iSrcStride, pDst,     iDstStride, iHeight);
  McHorVer22WidthEq8_sse2 (&pSrc[8], iSrcStride, &pDst[8], iDstStride, iHeight);
}

void McHorVer20Width5Or9Or17_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
        int32_t iWidth, int32_t iHeight) {
    if (iWidth == 17 || iWidth == 9)
        McHorVer20Width9Or17_sse2 (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
    else //if (iWidth == 5)
        McHorVer20Width5_sse2 (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
}

void McHorVer02Height5Or9Or17_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
        int32_t iWidth, int32_t iHeight) {
    if (iWidth == 16 || iWidth == 8)
        McHorVer02Height9Or17_sse2 (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
    else //if (iWidth == 4)
        McHorVer02Height5_sse2 (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
}

void McHorVer22Width5Or9Or17Height5Or9Or17_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
        int32_t iWidth, int32_t iHeight) {
    ENFORCE_STACK_ALIGN_2D (int16_t, pTap, 22, 24, 16)
    if (iWidth == 17 || iWidth == 9){
        int32_t tmp1 = 2 * (iWidth - 8);
        McHorVer22HorFirst_sse2 (pSrc - 2, iSrcStride, (uint8_t*)pTap, 48, iWidth, iHeight + 5);
        McHorVer22Width8VerLastAlign_sse2 ((uint8_t*)pTap,  48, pDst, iDstStride, iWidth - 1, iHeight);
        McHorVer22Width8VerLastUnAlign_sse2 ((uint8_t*)pTap + tmp1,  48, pDst + iWidth - 8, iDstStride, 8, iHeight);
    }
    else{ //if(iWidth == 5)
        int32_t tmp1 = 2 * (iWidth - 4);
        McHorVer22Width5HorFirst_sse2 (pSrc - 2, iSrcStride, (uint8_t*)pTap, 48, iWidth, iHeight + 5);
        McHorVer22Width4VerLastAlign_sse2 ((uint8_t*)pTap,  48, pDst, iDstStride, iWidth - 1, iHeight);
        McHorVer22Width4VerLastUnAlign_sse2 ((uint8_t*)pTap + tmp1,  48, pDst + iWidth - 4, iDstStride, 4, iHeight);
    }

}

static inline void McCopy_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                int32_t iWidth,
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

static inline void McHorVer20_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer20WidthEq16_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer20WidthEq8_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else
    McHorVer20WidthEq4_mmx (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}

static inline void McHorVer02_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer02WidthEq16_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer02WidthEq8_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else
    McHorVer02_c (pSrc, iSrcStride, pDst, iDstStride, 4, iHeight);
}

static inline void McHorVer22_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer22WidthEq16_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer22WidthEq8_sse2 (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else
    McHorVer22_c (pSrc, iSrcStride, pDst, iDstStride, 4, iHeight);
}

static inline void McHorVer01_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pTmp, 256, 16);
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
static inline void McHorVer03_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pTmp, 256, 16);
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
static inline void McHorVer10_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pTmp, 256, 16);
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
static inline void McHorVer11_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
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
static inline void McHorVer12_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
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
static inline void McHorVer13_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
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
static inline void McHorVer21_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
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
static inline void McHorVer23_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
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
static inline void McHorVer30_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
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
static inline void McHorVer31_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
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
static inline void McHorVer32_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
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
static inline void McHorVer33_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
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

void McLuma_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                  int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight)
//pSrc has been added the offset of mv
{
  static const PWelsMcWidthHeightFunc pWelsMcFunc[4][4] = { //[x][y]
    {McCopy_sse2,     McHorVer01_sse2, McHorVer02_sse2, McHorVer03_sse2},
    {McHorVer10_sse2, McHorVer11_sse2, McHorVer12_sse2, McHorVer13_sse2},
    {McHorVer20_sse2, McHorVer21_sse2, McHorVer22_sse2, McHorVer23_sse2},
    {McHorVer30_sse2, McHorVer31_sse2, McHorVer32_sse2, McHorVer33_sse2},
  };

  pWelsMcFunc[iMvX & 0x03][iMvY & 0x03] (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
}

void McChroma_sse2 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
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

void McChroma_ssse3 (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                     int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight) {
  static const PMcChromaWidthExtFunc kpMcChromaWidthFuncs[2] = {
    McChromaWidthEq4_mmx,
    McChromaWidthEq8_ssse3
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

void PixelAvg_sse2 (uint8_t* pDst, int32_t iDstStride, const uint8_t* pSrcA, int32_t iSrcAStride,
                    const uint8_t* pSrcB, int32_t iSrcBStride, int32_t iWidth, int32_t iHeight) {
  static const PWelsSampleWidthAveragingFunc kpfFuncs[2] = {
    PixelAvgWidthEq8_mmx,
    PixelAvgWidthEq16_sse2
  };
  kpfFuncs[iWidth >> 4] (pDst, iDstStride, pSrcA, iSrcAStride, pSrcB, iSrcBStride, iHeight);
}

#endif //X86_ASM
//***************************************************************************//
//                       NEON implementation                      //
//***************************************************************************//
#if defined(HAVE_NEON)
void McHorVer20Width5Or9Or17_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                int32_t iWidth, int32_t iHeight) {
  if (iWidth == 17)
    McHorVer20Width17_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 9)
    McHorVer20Width9_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else //if (iWidth == 5)
    McHorVer20Width5_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer02Height5Or9Or17_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                 int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer02Height17_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer02Height9_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else //if (iWidth == 4)
    McHorVer02Height5_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer22Width5Or9Or17Height5Or9Or17_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int32_t iWidth, int32_t iHeight) {
  if (iWidth == 17)
    McHorVer22Width17_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 9)
    McHorVer22Width9_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else //if (iWidth == 5)
    McHorVer22Width5_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McCopy_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                  int32_t iWidth, int32_t iHeight) {
  if (16 == iWidth)
    McCopyWidthEq16_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (8 == iWidth)
    McCopyWidthEq8_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (4 == iWidth)
    McCopyWidthEq4_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else
    McCopyWidthEq2_c (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer20_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer20WidthEq16_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer20WidthEq8_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer20WidthEq4_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer02_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer02WidthEq16_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer02WidthEq8_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer02WidthEq4_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer22_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer22WidthEq16_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer22WidthEq8_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer22WidthEq4_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}

void McHorVer01_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer01WidthEq16_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer01WidthEq8_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer01WidthEq4_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer03_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer03WidthEq16_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer03WidthEq8_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer03WidthEq4_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer10_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer10WidthEq16_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer10WidthEq8_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer10WidthEq4_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer11_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq4_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq4_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  }
}
void McHorVer12_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer02WidthEq16_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq16_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_neon (pDst, iDstStride, pVerTmp, pCtrTmp, iHeight);
  } else if (iWidth == 8) {
    McHorVer02WidthEq8_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq8_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_neon (pDst, iDstStride, pVerTmp, pCtrTmp, iHeight);
  } else if (iWidth == 4) {
    McHorVer02WidthEq4_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq4_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq4_neon (pDst, iDstStride, pVerTmp, pCtrTmp, iHeight);
  }
}
void McHorVer13_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq4_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq4_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  }
}
void McHorVer21_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq16_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_neon (pDst, iDstStride, pHorTmp, pCtrTmp, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq8_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_neon (pDst, iDstStride, pHorTmp, pCtrTmp, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq4_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq4_neon (pDst, iDstStride, pHorTmp, pCtrTmp, iHeight);
  }
}
void McHorVer23_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq16_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_neon (pDst, iDstStride, pHorTmp, pCtrTmp, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq8_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_neon (pDst, iDstStride, pHorTmp, pCtrTmp, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq4_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq4_neon (pDst, iDstStride, pHorTmp, pCtrTmp, iHeight);
  }
}
void McHorVer30_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer30WidthEq16_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer30WidthEq8_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer30WidthEq4_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer31_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq4_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq4_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  }
}
void McHorVer32_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer02WidthEq16_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq16_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_neon (pDst, iDstStride, pVerTmp, pCtrTmp, iHeight);
  } else if (iWidth == 8) {
    McHorVer02WidthEq8_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq8_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_neon (pDst, iDstStride, pVerTmp, pCtrTmp, iHeight);
  } else if (iWidth == 4) {
    McHorVer02WidthEq4_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq4_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq4_neon (pDst, iDstStride, pVerTmp, pCtrTmp, iHeight);
  }
}
void McHorVer33_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                      int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq4_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq4_neon (pDst, iDstStride, pHorTmp, pVerTmp, iHeight);
  }
}

void McLuma_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                  int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight) {
  static const PWelsMcWidthHeightFunc pWelsMcFunc[4][4] = { //[x][y]
    {McCopy_neon,  McHorVer01_neon, McHorVer02_neon,    McHorVer03_neon},
    {McHorVer10_neon, McHorVer11_neon, McHorVer12_neon, McHorVer13_neon},
    {McHorVer20_neon,    McHorVer21_neon, McHorVer22_neon,    McHorVer23_neon},
    {McHorVer30_neon, McHorVer31_neon, McHorVer32_neon, McHorVer33_neon},
  };
  // pSrc += (iMvY >> 2) * iSrcStride + (iMvX >> 2);
  pWelsMcFunc[iMvX & 0x03][iMvY & 0x03] (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
}
void McChroma_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                    int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight) {
  if (0 == iMvX && 0 == iMvY) {
    if (8 == iWidth)
      McCopyWidthEq8_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
    else if (iWidth == 4)
      McCopyWidthEq4_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
    else //here iWidth == 2
      McCopyWidthEq2_c (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  } else {
    const int32_t kiD8x = iMvX & 0x07;
    const int32_t kiD8y = iMvY & 0x07;
    if (8 == iWidth)
      McChromaWidthEq8_neon (pSrc, iSrcStride, pDst, iDstStride, (int32_t*) (g_kuiABCD[kiD8y][kiD8x]), iHeight);
    else if (4 == iWidth)
      McChromaWidthEq4_neon (pSrc, iSrcStride, pDst, iDstStride, (int32_t*) (g_kuiABCD[kiD8y][kiD8x]), iHeight);
    else //here iWidth == 2
      McChromaWithFragMv_c (pSrc, iSrcStride, pDst, iDstStride, iMvX, iMvY, iWidth, iHeight);
  }
}
void PixelAvg_neon (uint8_t* pDst, int32_t iDstStride, const uint8_t* pSrcA, int32_t iSrcAStride,
                    const uint8_t* pSrcB, int32_t iSrcBStride, int32_t iWidth, int32_t iHeight) {
  static const PWelsSampleWidthAveragingFunc kpfFuncs[2] = {
    PixStrideAvgWidthEq8_neon,
    PixStrideAvgWidthEq16_neon
  };
  kpfFuncs[iWidth >> 4] (pDst, iDstStride, pSrcA, iSrcAStride, pSrcB, iSrcBStride, iHeight);
}
#endif
#if defined(HAVE_NEON_AARCH64)
void McHorVer20Width5Or9Or17_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                        int32_t iWidth, int32_t iHeight) {
  if (iWidth == 17)
    McHorVer20Width17_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 9)
    McHorVer20Width9_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else //if (iWidth == 5)
    McHorVer20Width5_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer02Height5Or9Or17_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer02Height17_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer02Height9_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else //if (iWidth == 4)
    McHorVer02Height5_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer22Width5Or9Or17Height5Or9Or17_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst,
    int32_t iDstStride,
    int32_t iWidth, int32_t iHeight) {
  if (iWidth == 17)
    McHorVer22Width17_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 9)
    McHorVer22Width9_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else //if (iWidth == 5)
    McHorVer22Width5_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McCopy_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                          int32_t iWidth, int32_t iHeight) {
  if (16 == iWidth)
    McCopyWidthEq16_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (8 == iWidth)
    McCopyWidthEq8_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (4 == iWidth)
    McCopyWidthEq4_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else
    McCopyWidthEq2_c (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer20_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer20WidthEq16_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer20WidthEq8_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer20WidthEq4_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer02_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer02WidthEq16_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer02WidthEq8_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer02WidthEq4_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer22_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer22WidthEq16_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer22WidthEq8_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer22WidthEq4_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}

void McHorVer01_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer01WidthEq16_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer01WidthEq8_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer01WidthEq4_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer03_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer03WidthEq16_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer03WidthEq8_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer03WidthEq4_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer10_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer10WidthEq16_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer10WidthEq8_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer10WidthEq4_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer11_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_AArch64_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_AArch64_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_AArch64_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_AArch64_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_AArch64_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq4_AArch64_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq4_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  }
}
void McHorVer12_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer02WidthEq16_AArch64_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq16_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_AArch64_neon (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer02WidthEq8_AArch64_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq8_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_AArch64_neon (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 4) {
    McHorVer02WidthEq4_AArch64_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq4_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq4_AArch64_neon (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  }
}
void McHorVer13_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_AArch64_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_AArch64_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_AArch64_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_AArch64_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_AArch64_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq4_AArch64_neon (pSrc, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq4_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  }
}
void McHorVer21_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_AArch64_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq16_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_AArch64_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq8_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_AArch64_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq4_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq4_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  }
}
void McHorVer23_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_AArch64_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq16_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_AArch64_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq8_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_AArch64_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer22WidthEq4_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq4_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pCtrTmp, 16, iHeight);
  }
}
void McHorVer30_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  if (iWidth == 16)
    McHorVer30WidthEq16_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 8)
    McHorVer30WidthEq8_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  else if (iWidth == 4)
    McHorVer30WidthEq4_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
}
void McHorVer31_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_AArch64_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_AArch64_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_AArch64_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_AArch64_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_AArch64_neon (pSrc, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq4_AArch64_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq4_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  }
}
void McHorVer32_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pCtrTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer02WidthEq16_AArch64_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq16_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq16_AArch64_neon (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer02WidthEq8_AArch64_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq8_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq8_AArch64_neon (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  } else if (iWidth == 4) {
    McHorVer02WidthEq4_AArch64_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    McHorVer22WidthEq4_AArch64_neon (pSrc, iSrcStride, pCtrTmp, 16, iHeight);
    PixelAvgWidthEq4_AArch64_neon (pDst, iDstStride, pVerTmp, 16, pCtrTmp, 16, iHeight);
  }
}
void McHorVer33_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                              int32_t iWidth, int32_t iHeight) {
  ENFORCE_STACK_ALIGN_1D (uint8_t, pHorTmp, 256, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pVerTmp, 256, 16);
  if (iWidth == 16) {
    McHorVer20WidthEq16_AArch64_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq16_AArch64_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq16_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 8) {
    McHorVer20WidthEq8_AArch64_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq8_AArch64_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq8_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  } else if (iWidth == 4) {
    McHorVer20WidthEq4_AArch64_neon (pSrc + iSrcStride, iSrcStride, pHorTmp, 16, iHeight);
    McHorVer02WidthEq4_AArch64_neon (pSrc + 1, iSrcStride, pVerTmp, 16, iHeight);
    PixelAvgWidthEq4_AArch64_neon (pDst, iDstStride, pHorTmp, 16, pVerTmp, 16, iHeight);
  }
}

void McLuma_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                          int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight) {
  static const PWelsMcWidthHeightFunc pWelsMcFunc[4][4] = { //[x][y]
    {McCopy_AArch64_neon,  McHorVer01_AArch64_neon, McHorVer02_AArch64_neon,    McHorVer03_AArch64_neon},
    {McHorVer10_AArch64_neon, McHorVer11_AArch64_neon, McHorVer12_AArch64_neon, McHorVer13_AArch64_neon},
    {McHorVer20_AArch64_neon,    McHorVer21_AArch64_neon, McHorVer22_AArch64_neon,    McHorVer23_AArch64_neon},
    {McHorVer30_AArch64_neon, McHorVer31_AArch64_neon, McHorVer32_AArch64_neon, McHorVer33_AArch64_neon},
  };
  // pSrc += (iMvY >> 2) * iSrcStride + (iMvX >> 2);
  pWelsMcFunc[iMvX & 0x03][iMvY & 0x03] (pSrc, iSrcStride, pDst, iDstStride, iWidth, iHeight);
}
void McChroma_AArch64_neon (const uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                            int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight) {
  if (0 == iMvX && 0 == iMvY) {
    if (8 == iWidth)
      McCopyWidthEq8_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
    else if (iWidth == 4)
      McCopyWidthEq4_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, iHeight);
    else //here iWidth == 2
      McCopyWidthEq2_c (pSrc, iSrcStride, pDst, iDstStride, iHeight);
  } else {
    const int32_t kiD8x = iMvX & 0x07;
    const int32_t kiD8y = iMvY & 0x07;
    if (8 == iWidth)
      McChromaWidthEq8_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, (int32_t*) (g_kuiABCD[kiD8y][kiD8x]), iHeight);
    else if (4 == iWidth)
      McChromaWidthEq4_AArch64_neon (pSrc, iSrcStride, pDst, iDstStride, (int32_t*) (g_kuiABCD[kiD8y][kiD8x]), iHeight);
    else //here iWidth == 2
      McChromaWithFragMv_c (pSrc, iSrcStride, pDst, iDstStride, iMvX, iMvY, iWidth, iHeight);
  }
}
void PixelAvg_AArch64_neon (uint8_t* pDst, int32_t iDstStride, const uint8_t* pSrcA, int32_t iSrcAStride,
                            const uint8_t* pSrcB, int32_t iSrcBStride, int32_t iWidth, int32_t iHeight) {
  static const PWelsSampleWidthAveragingFunc kpfFuncs[2] = {
    PixStrideAvgWidthEq8_AArch64_neon,
    PixStrideAvgWidthEq16_AArch64_neon
  };
  kpfFuncs[iWidth >> 4] (pDst, iDstStride, pSrcA, iSrcAStride, pSrcB, iSrcBStride, iHeight);
}
#endif

void InitMcFunc (SMcFunc* pMcFuncs, uint32_t uiCpuFlag) {
  pMcFuncs->pfLumaHalfpelHor  = McHorVer20_c;
  pMcFuncs->pfLumaHalfpelVer  = McHorVer02_c;
  pMcFuncs->pfLumaHalfpelCen  = McHorVer22_c;
  pMcFuncs->pfSampleAveraging = PixelAvg_c;
  pMcFuncs->pMcChromaFunc     = McChroma_c;
  pMcFuncs->pMcLumaFunc       = McLuma_c;

#if defined (X86_ASM)
  if (uiCpuFlag & WELS_CPU_SSE2) {
    pMcFuncs->pfLumaHalfpelHor  = McHorVer20Width5Or9Or17_sse2;
    pMcFuncs->pfLumaHalfpelVer  = McHorVer02Height5Or9Or17_sse2;
    pMcFuncs->pfLumaHalfpelCen  = McHorVer22Width5Or9Or17Height5Or9Or17_sse2;
    pMcFuncs->pfSampleAveraging = PixelAvg_sse2;
    pMcFuncs->pMcChromaFunc     = McChroma_sse2;
    pMcFuncs->pMcLumaFunc       = McLuma_sse2;
  }

  if (uiCpuFlag & WELS_CPU_SSSE3) {
    pMcFuncs->pMcChromaFunc = McChroma_ssse3;
  }
#endif //(X86_ASM)

#if defined(HAVE_NEON)
  if (uiCpuFlag & WELS_CPU_NEON) {
    pMcFuncs->pMcLumaFunc       = McLuma_neon;
    pMcFuncs->pMcChromaFunc     = McChroma_neon;
    pMcFuncs->pfSampleAveraging = PixelAvg_neon;
    pMcFuncs->pfLumaHalfpelHor  = McHorVer20Width5Or9Or17_neon;//iWidth+1:4/8/16
    pMcFuncs->pfLumaHalfpelVer  = McHorVer02Height5Or9Or17_neon;//heigh+1:4/8/16
    pMcFuncs->pfLumaHalfpelCen  = McHorVer22Width5Or9Or17Height5Or9Or17_neon;//iWidth+1/heigh+1
  }
#endif
#if defined(HAVE_NEON_AARCH64)
  if (uiCpuFlag & WELS_CPU_NEON) {
    pMcFuncs->pMcLumaFunc       = McLuma_AArch64_neon;
    pMcFuncs->pMcChromaFunc     = McChroma_AArch64_neon;
    pMcFuncs->pfSampleAveraging = PixelAvg_AArch64_neon;
    pMcFuncs->pfLumaHalfpelHor  = McHorVer20Width5Or9Or17_AArch64_neon;//iWidth+1:4/8/16
    pMcFuncs->pfLumaHalfpelVer  = McHorVer02Height5Or9Or17_AArch64_neon;//heigh+1:4/8/16
    pMcFuncs->pfLumaHalfpelCen  = McHorVer22Width5Or9Or17Height5Or9Or17_AArch64_neon;//iWidth+1/heigh+1
  }
#endif
}
} // namespace WelsCommon
