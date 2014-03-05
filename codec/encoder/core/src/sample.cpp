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
 * \file	sample.c
 *
 * \brief	compute SAD and SATD
 *
 * \date	2009.06.02 Created
 *
 *************************************************************************************
 */

#include "sample.h"

#include "mc.h"
#include "cpu_core.h"

namespace WelsSVCEnc {
int32_t WelsSampleSad4x4_c (uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2) {
  int32_t iSadSum = 0;
  int32_t i = 0;
  uint8_t* pSrc1 = pSample1;
  uint8_t* pSrc2 = pSample2;
  for (i = 0; i < 4; i++) {
    iSadSum += WELS_ABS ((pSrc1[0] - pSrc2[0]));
    iSadSum += WELS_ABS ((pSrc1[1] - pSrc2[1]));
    iSadSum += WELS_ABS ((pSrc1[2] - pSrc2[2]));
    iSadSum += WELS_ABS ((pSrc1[3] - pSrc2[3]));

    pSrc1 += iStride1;
    pSrc2 += iStride2;
  }

  return iSadSum;
}

int32_t WelsSampleSad8x8_c (uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2) {
  int32_t iSadSum = 0;
  int32_t i = 0;
  uint8_t* pSrc1 = pSample1;
  uint8_t* pSrc2 = pSample2;
  for (i = 0; i < 8; i++) {
    iSadSum += WELS_ABS ((pSrc1[0] - pSrc2[0]));
    iSadSum += WELS_ABS ((pSrc1[1] - pSrc2[1]));
    iSadSum += WELS_ABS ((pSrc1[2] - pSrc2[2]));
    iSadSum += WELS_ABS ((pSrc1[3] - pSrc2[3]));
    iSadSum += WELS_ABS ((pSrc1[4] - pSrc2[4]));
    iSadSum += WELS_ABS ((pSrc1[5] - pSrc2[5]));
    iSadSum += WELS_ABS ((pSrc1[6] - pSrc2[6]));
    iSadSum += WELS_ABS ((pSrc1[7] - pSrc2[7]));

    pSrc1 += iStride1;
    pSrc2 += iStride2;
  }

  return iSadSum;
}
int32_t WelsSampleSad16x8_c (uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2) {
  int32_t iSadSum = 0;

  iSadSum += WelsSampleSad8x8_c (pSample1,     iStride1, pSample2,     iStride2);
  iSadSum += WelsSampleSad8x8_c (pSample1 + 8, iStride1, pSample2 + 8, iStride2);

  return iSadSum;
}
int32_t WelsSampleSad8x16_c (uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2) {
  int32_t iSadSum = 0;
  iSadSum += WelsSampleSad8x8_c (pSample1,                   iStride1, pSample2,                   iStride2);
  iSadSum += WelsSampleSad8x8_c (pSample1 + (iStride1 << 3), iStride1, pSample2 + (iStride2 << 3), iStride2);

  return iSadSum;
}
int32_t WelsSampleSad16x16_c (uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2) {
  int32_t iSadSum = 0;
  iSadSum += WelsSampleSad8x8_c (pSample1,                     iStride1, pSample2,                     iStride2);
  iSadSum += WelsSampleSad8x8_c (pSample1 + 8,                   iStride1, pSample2 + 8,                   iStride2);
  iSadSum += WelsSampleSad8x8_c (pSample1 + (iStride1 << 3),   iStride1, pSample2 + (iStride2 << 3),   iStride2);
  iSadSum += WelsSampleSad8x8_c (pSample1 + (iStride1 << 3) + 8, iStride1, pSample2 + (iStride2 << 3) + 8, iStride2);

  return iSadSum;
}

int32_t WelsSampleSatd4x4_c (uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2) {
  int32_t iSatdSum = 0;
  int32_t pSampleMix[4][4] = { 0 };
  int32_t iSample0, iSample1, iSample2, iSample3;
  int32_t i = 0;
  uint8_t* pSrc1 = pSample1;
  uint8_t* pSrc2 = pSample2;

  //step 1: get the difference
  for (i = 0; i < 4; i++) {
    pSampleMix[i][0] = pSrc1[0] - pSrc2[0];
    pSampleMix[i][1] = pSrc1[1] - pSrc2[1];
    pSampleMix[i][2] = pSrc1[2] - pSrc2[2];
    pSampleMix[i][3] = pSrc1[3] - pSrc2[3];

    pSrc1 += iStride1;
    pSrc2 += iStride2;
  }

  //step 2: horizontal transform
  for (i = 0; i < 4; i++) {
    iSample0 = pSampleMix[i][0] + pSampleMix[i][2];
    iSample1 = pSampleMix[i][1] + pSampleMix[i][3];
    iSample2 = pSampleMix[i][0] - pSampleMix[i][2];
    iSample3 = pSampleMix[i][1] - pSampleMix[i][3];

    pSampleMix[i][0] = iSample0 + iSample1;
    pSampleMix[i][1] = iSample2 + iSample3;
    pSampleMix[i][2] = iSample2 - iSample3;
    pSampleMix[i][3] = iSample0 - iSample1;
  }

  //step 3: vertical transform and get the sum of SATD
  for (i = 0; i < 4; i++) {
    iSample0 = pSampleMix[0][i] + pSampleMix[2][i];
    iSample1 = pSampleMix[1][i] + pSampleMix[3][i];
    iSample2 = pSampleMix[0][i] - pSampleMix[2][i];
    iSample3 = pSampleMix[1][i] - pSampleMix[3][i];

    pSampleMix[0][i] = iSample0 + iSample1;
    pSampleMix[1][i] = iSample2 + iSample3;
    pSampleMix[2][i] = iSample2 - iSample3;
    pSampleMix[3][i] = iSample0 - iSample1;

    iSatdSum += (WELS_ABS (pSampleMix[0][i]) + WELS_ABS (pSampleMix[1][i]) + WELS_ABS (pSampleMix[2][i]) + WELS_ABS (
                   pSampleMix[3][i]));
  }

  return ((iSatdSum + 1) >> 1);
}
int32_t WelsSampleSatd8x8_c (uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2) {
  int32_t iSatdSum = 0;

  iSatdSum += WelsSampleSatd4x4_c (pSample1,                     iStride1, pSample2,                     iStride2);
  iSatdSum += WelsSampleSatd4x4_c (pSample1 + 4,                   iStride1, pSample2 + 4,                   iStride2);
  iSatdSum += WelsSampleSatd4x4_c (pSample1 + (iStride1 << 2),   iStride1, pSample2 + (iStride2 << 2),   iStride2);
  iSatdSum += WelsSampleSatd4x4_c (pSample1 + (iStride1 << 2) + 4, iStride1, pSample2 + (iStride2 << 2) + 4, iStride2);

  return iSatdSum;
}
int32_t WelsSampleSatd16x8_c (uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2) {
  int32_t iSatdSum = 0;

  iSatdSum += WelsSampleSatd8x8_c (pSample1,   iStride1, pSample2,   iStride2);
  iSatdSum += WelsSampleSatd8x8_c (pSample1 + 8, iStride1, pSample2 + 8, iStride2);

  return iSatdSum;
}
int32_t WelsSampleSatd8x16_c (uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2) {
  int32_t iSatdSum = 0;

  iSatdSum += WelsSampleSatd8x8_c (pSample1,                   iStride1, pSample2,                   iStride2);
  iSatdSum += WelsSampleSatd8x8_c (pSample1 + (iStride1 << 3), iStride1, pSample2 + (iStride2 << 3), iStride2);

  return iSatdSum;
}
int32_t WelsSampleSatd16x16_c (uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2) {
  int32_t iSatdSum = 0;

  iSatdSum += WelsSampleSatd8x8_c (pSample1,                     iStride1, pSample2,                     iStride2);
  iSatdSum += WelsSampleSatd8x8_c (pSample1 + 8,                   iStride1, pSample2 + 8,                   iStride2);
  iSatdSum += WelsSampleSatd8x8_c (pSample1 + (iStride1 << 3),   iStride1, pSample2 + (iStride2 << 3),   iStride2);
  iSatdSum += WelsSampleSatd8x8_c (pSample1 + (iStride1 << 3) + 8, iStride1, pSample2 + (iStride2 << 3) + 8, iStride2);

  return iSatdSum;
}


void WelsSampleSadFour16x16_c (uint8_t* iSample1, int32_t iStride1, uint8_t* iSample2, int32_t iStride2,
                               int32_t* pSad) {
  * (pSad)     = WelsSampleSad16x16_c (iSample1, iStride1, (iSample2 - iStride2), iStride2);
  * (pSad + 1) = WelsSampleSad16x16_c (iSample1, iStride1, (iSample2 + iStride2), iStride2);
  * (pSad + 2) = WelsSampleSad16x16_c (iSample1, iStride1, (iSample2 - 1), iStride2);
  * (pSad + 3) = WelsSampleSad16x16_c (iSample1, iStride1, (iSample2 + 1), iStride2);
}
void WelsSampleSadFour16x8_c (uint8_t* iSample1, int32_t iStride1, uint8_t* iSample2, int32_t iStride2, int32_t* pSad) {
  * (pSad)     = WelsSampleSad16x8_c (iSample1, iStride1, (iSample2 - iStride2), iStride2);
  * (pSad + 1) = WelsSampleSad16x8_c (iSample1, iStride1, (iSample2 + iStride2), iStride2);
  * (pSad + 2) = WelsSampleSad16x8_c (iSample1, iStride1, (iSample2 - 1), iStride2);
  * (pSad + 3) = WelsSampleSad16x8_c (iSample1, iStride1, (iSample2 + 1), iStride2);
}
void WelsSampleSadFour8x16_c (uint8_t* iSample1, int32_t iStride1, uint8_t* iSample2, int32_t iStride2, int32_t* pSad) {
  * (pSad)     = WelsSampleSad8x16_c (iSample1, iStride1, (iSample2 - iStride2), iStride2);
  * (pSad + 1) = WelsSampleSad8x16_c (iSample1, iStride1, (iSample2 + iStride2), iStride2);
  * (pSad + 2) = WelsSampleSad8x16_c (iSample1, iStride1, (iSample2 - 1), iStride2);
  * (pSad + 3) = WelsSampleSad8x16_c (iSample1, iStride1, (iSample2 + 1), iStride2);

}
void WelsSampleSadFour8x8_c (uint8_t* iSample1, int32_t iStride1, uint8_t* iSample2, int32_t iStride2, int32_t* pSad) {
  * (pSad)     = WelsSampleSad8x8_c (iSample1, iStride1, (iSample2 - iStride2), iStride2);
  * (pSad + 1) = WelsSampleSad8x8_c (iSample1, iStride1, (iSample2 + iStride2), iStride2);
  * (pSad + 2) = WelsSampleSad8x8_c (iSample1, iStride1, (iSample2 - 1), iStride2);
  * (pSad + 3) = WelsSampleSad8x8_c (iSample1, iStride1, (iSample2 + 1), iStride2);
}
void WelsSampleSadFour4x4_c (uint8_t* iSample1, int32_t iStride1, uint8_t* iSample2, int32_t iStride2, int32_t* pSad) {
  * (pSad)     = WelsSampleSad4x4_c (iSample1, iStride1, (iSample2 - iStride2), iStride2);
  * (pSad + 1) = WelsSampleSad4x4_c (iSample1, iStride1, (iSample2 + iStride2), iStride2);
  * (pSad + 2) = WelsSampleSad4x4_c (iSample1, iStride1, (iSample2 - 1), iStride2);
  * (pSad + 3) = WelsSampleSad4x4_c (iSample1, iStride1, (iSample2 + 1), iStride2);
}

extern void WelsI4x4LumaPredDc_c (uint8_t* pPred, uint8_t* pRef, const int32_t iStride);
extern void WelsI4x4LumaPredH_c (uint8_t* pPred, uint8_t* pRef, const int32_t iStride);
extern void WelsI4x4LumaPredV_c (uint8_t* pPred, uint8_t* pRef, const int32_t iStride);

int32_t WelsSampleSatdIntra4x4Combined3_c (uint8_t* pDec, int32_t iDecStride, uint8_t* pEnc, int32_t iEncStride,
    uint8_t* pDst,
    int32_t* pBestMode, int32_t iLambda2, int32_t iLambda1, int32_t iLambda0) {
  int32_t iBestMode = -1;
  int32_t iCurCost, iBestCost = INT_MAX;
  ENFORCE_STACK_ALIGN_2D (uint8_t, uiLocalBuffer, 3, 16, 16)

  WelsI4x4LumaPredDc_c (uiLocalBuffer[2], pDec, iDecStride);
  iCurCost = WelsSampleSatd4x4_c (uiLocalBuffer[2], 4, pEnc, iEncStride) + iLambda2;
  if (iCurCost < iBestCost) {
    iBestMode = 2;
    iBestCost = iCurCost;
  }

  WelsI4x4LumaPredH_c (uiLocalBuffer[1], pDec, iDecStride);
  iCurCost = WelsSampleSatd4x4_c (uiLocalBuffer[1], 4, pEnc, iEncStride) + iLambda1;
  if (iCurCost < iBestCost) {
    iBestMode = 1;
    iBestCost = iCurCost;
  }
  WelsI4x4LumaPredV_c (uiLocalBuffer[0], pDec, iDecStride);
  iCurCost = WelsSampleSatd4x4_c (uiLocalBuffer[0], 4, pEnc, iEncStride) + iLambda0;
  if (iCurCost < iBestCost) {
    iBestMode = 0;
    iBestCost = iCurCost;
  }

  memcpy (pDst, uiLocalBuffer[iBestMode], 16 * sizeof (uint8_t));	// confirmed_safe_unsafe_usage
  *pBestMode = iBestMode;

  return iBestCost;
}
extern void WelsIChormaPredDc_c (uint8_t* pPred, uint8_t* pRef, const int32_t iStride);
extern void WelsIChormaPredH_c (uint8_t* pPred, uint8_t* pRef, const int32_t iStride);
extern void WelsIChormaPredV_c (uint8_t* pPred, uint8_t* pRef, const int32_t iStride);

int32_t WelsSampleSatdIntra8x8Combined3_c (uint8_t* pDecCb, int32_t iDecStride, uint8_t* pEncCb, int32_t iEncStride,
    int32_t* pBestMode, int32_t iLambda, uint8_t* pDstChroma, uint8_t* pDecCr, uint8_t* pEncCr) {
  int32_t iBestMode = -1;
  int32_t iCurCost, iBestCost = INT_MAX;

  WelsIChormaPredV_c (pDstChroma, pDecCb, iDecStride);
  WelsIChormaPredV_c (pDstChroma + 64, pDecCr, iDecStride);
  iCurCost = WelsSampleSatd8x8_c (pDstChroma, 8, pEncCb, iEncStride);
  iCurCost += WelsSampleSatd8x8_c (pDstChroma + 64, 8, pEncCr, iEncStride) + iLambda * 2;

  if (iCurCost < iBestCost) {
    iBestMode = 2;
    iBestCost = iCurCost;
  }

  WelsIChormaPredH_c (pDstChroma, pDecCb, iDecStride);
  WelsIChormaPredH_c (pDstChroma + 64, pDecCr, iDecStride);
  iCurCost = WelsSampleSatd8x8_c (pDstChroma, 8, pEncCb, iEncStride);
  iCurCost += WelsSampleSatd8x8_c (pDstChroma + 64, 8, pEncCr, iEncStride) + iLambda * 2;
  if (iCurCost < iBestCost) {
    iBestMode = 1;
    iBestCost = iCurCost;
  }
  WelsIChormaPredDc_c (pDstChroma, pDecCb, iDecStride);
  WelsIChormaPredDc_c (pDstChroma + 64, pDecCr, iDecStride);
  iCurCost = WelsSampleSatd8x8_c (pDstChroma, 8, pEncCb, iEncStride);
  iCurCost += WelsSampleSatd8x8_c (pDstChroma + 64, 8, pEncCr, iEncStride);
  if (iCurCost < iBestCost) {
    iBestMode = 0;
    iBestCost = iCurCost;
  }

  *pBestMode	= iBestMode;

  return iBestCost;


}
int32_t WelsSampleSadIntra8x8Combined3_c (uint8_t* pDecCb, int32_t iDecStride, uint8_t* pEncCb, int32_t iEncStride,
    int32_t* pBestMode, int32_t iLambda, uint8_t* pDstChroma, uint8_t* pDecCr, uint8_t* pEncCr) {
  int32_t iBestMode = -1;
  int32_t iCurCost, iBestCost = INT_MAX;

  WelsIChormaPredV_c (pDstChroma, pDecCb, iDecStride);
  WelsIChormaPredV_c (pDstChroma + 64, pDecCr, iDecStride);
  iCurCost = WelsSampleSad8x8_c (pDstChroma, 8, pEncCb, iEncStride);
  iCurCost += WelsSampleSad8x8_c (pDstChroma + 64, 8, pEncCr, iEncStride) + iLambda * 2;

  if (iCurCost < iBestCost) {
    iBestMode = 2;
    iBestCost = iCurCost;
  }

  WelsIChormaPredH_c (pDstChroma, pDecCb, iDecStride);
  WelsIChormaPredH_c (pDstChroma + 64, pDecCr, iDecStride);
  iCurCost = WelsSampleSad8x8_c (pDstChroma, 8, pEncCb, iEncStride);
  iCurCost += WelsSampleSad8x8_c (pDstChroma + 64, 8, pEncCr, iEncStride) + iLambda * 2;
  if (iCurCost < iBestCost) {
    iBestMode = 1;
    iBestCost = iCurCost;
  }
  WelsIChormaPredDc_c (pDstChroma, pDecCb, iDecStride);
  WelsIChormaPredDc_c (pDstChroma + 64, pDecCr, iDecStride);
  iCurCost = WelsSampleSad8x8_c (pDstChroma, 8, pEncCb, iEncStride);
  iCurCost += WelsSampleSad8x8_c (pDstChroma + 64, 8, pEncCr, iEncStride);
  if (iCurCost < iBestCost) {
    iBestMode = 0;
    iBestCost = iCurCost;
  }

  *pBestMode = iBestMode;

  return iBestCost;

}

extern void WelsI16x16LumaPredDc_c (uint8_t* pPred, uint8_t* pRef, const int32_t iStride);
extern void WelsI16x16LumaPredH_c (uint8_t* pPred, uint8_t* pRef, const int32_t iStride);
extern void WelsI16x16LumaPredV_c (uint8_t* pPred, uint8_t* pRef, const int32_t iStride);

int32_t WelsSampleSatdIntra16x16Combined3_c (uint8_t* pDec, int32_t iDecStride, uint8_t* pEnc, int32_t iEncStride,
    int32_t* pBestMode, int32_t iLambda, uint8_t* pDst) {
  int32_t iBestMode = -1;
  int32_t iCurCost, iBestCost = INT_MAX;

  WelsI16x16LumaPredV_c (pDst, pDec, iDecStride);
  iCurCost = WelsSampleSatd16x16_c (pDst, 16, pEnc, iEncStride);

  if (iCurCost < iBestCost) {
    iBestMode = 0;
    iBestCost = iCurCost;
  }

  WelsI16x16LumaPredH_c (pDst, pDec, iDecStride);
  iCurCost = WelsSampleSatd16x16_c (pDst, 16, pEnc, iEncStride) + iLambda * 2;
  if (iCurCost < iBestCost) {
    iBestMode = 1;
    iBestCost = iCurCost;
  }
  WelsI16x16LumaPredDc_c (pDst, pDec, iDecStride);
  iCurCost = WelsSampleSatd16x16_c (pDst, 16, pEnc, iEncStride) + iLambda * 2;
  if (iCurCost < iBestCost) {
    iBestMode = 2;
    iBestCost = iCurCost;
  }

  *pBestMode = iBestMode;

  return iBestCost;


}
int32_t WelsSampleSadIntra16x16Combined3_c (uint8_t* pDec, int32_t iDecStride, uint8_t* pEnc, int32_t iEncStride,
    int32_t* pBestMode, int32_t iLambda, uint8_t* pDst) {
  int32_t iBestMode = -1;
  int32_t iCurCost, iBestCost = INT_MAX;

  WelsI16x16LumaPredV_c (pDst, pDec, iDecStride);
  iCurCost = WelsSampleSad16x16_c (pDst, 16, pEnc, iEncStride);

  if (iCurCost < iBestCost) {
    iBestMode = 0;
    iBestCost = iCurCost;
  }

  WelsI16x16LumaPredH_c (pDst, pDec, iDecStride);
  iCurCost = WelsSampleSad16x16_c (pDst, 16, pEnc, iEncStride) + iLambda * 2;
  if (iCurCost < iBestCost) {
    iBestMode = 1;
    iBestCost = iCurCost;
  }
  WelsI16x16LumaPredDc_c (pDst, pDec, iDecStride);
  iCurCost = WelsSampleSad16x16_c (pDst, 16, pEnc, iEncStride) + iLambda * 2;
  if (iCurCost < iBestCost) {
    iBestMode = 2;
    iBestCost = iCurCost;
  }

  *pBestMode = iBestMode;

  return iBestCost;


}

void WelsInitSampleSadFunc (SWelsFuncPtrList* pFuncList, uint32_t uiCpuFlag) {
  //pfSampleSad init
  pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_16x16] = WelsSampleSad16x16_c;
  pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_16x8 ] = WelsSampleSad16x8_c;
  pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_8x16 ] = WelsSampleSad8x16_c;
  pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_8x8  ] = WelsSampleSad8x8_c;
  pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_4x4  ] = WelsSampleSad4x4_c;

  //pfSampleSatd init
  pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_16x16] = WelsSampleSatd16x16_c;
  pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_16x8 ] = WelsSampleSatd16x8_c;
  pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_8x16 ] = WelsSampleSatd8x16_c;
  pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_8x8  ] = WelsSampleSatd8x8_c;
  pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_4x4  ] = WelsSampleSatd4x4_c;

  pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_16x16] = WelsSampleSadFour16x16_c;
  pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_16x8] = WelsSampleSadFour16x8_c;
  pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_8x16] = WelsSampleSadFour8x16_c;
  pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_8x8] = WelsSampleSadFour8x8_c;
  pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_4x4] = WelsSampleSadFour4x4_c;

  pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3Satd   = NULL;
  pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Satd   = NULL;
  pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Sad    = NULL;
  pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Satd = NULL;
  pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Sad  = NULL;

#if defined (X86_ASM)
  if (uiCpuFlag & WELS_CPU_MMXEXT) {
    pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_4x4  ] = WelsSampleSad4x4_mmx;
  }

  if (uiCpuFlag & WELS_CPU_SSE2) {
    pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_16x16] = WelsSampleSad16x16_sse2;
    pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_16x8 ] = WelsSampleSad16x8_sse2;
    pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_8x16] = WelsSampleSad8x16_sse2;
    pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_8x8] = WelsSampleSad8x8_sse21;

    pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_16x16] = WelsSampleSadFour16x16_sse2;
    pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_16x8] = WelsSampleSadFour16x8_sse2;
    pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_8x16] = WelsSampleSadFour8x16_sse2;
    pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_8x8] = WelsSampleSadFour8x8_sse2;
    pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_4x4] = WelsSampleSadFour4x4_sse2;

    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_4x4  ] = WelsSampleSatd4x4_sse2;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_8x8  ] = WelsSampleSatd8x8_sse2;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_8x16 ] = WelsSampleSatd8x16_sse2;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_16x8 ] = WelsSampleSatd16x8_sse2;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_16x16] = WelsSampleSatd16x16_sse2;
    //pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3Satd =  WelsSmpleSatdThree4x4_sse2;
  }

  if (uiCpuFlag & WELS_CPU_SSSE3) {
    //pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Sad = WelsIntra16x16Combined3Sad_ssse3;
  }

  if (uiCpuFlag & WELS_CPU_SSE41) {
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_16x16] = WelsSampleSatd16x16_sse41;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_16x8] = WelsSampleSatd16x8_sse41;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_8x16] = WelsSampleSatd8x16_sse41;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_8x8] = WelsSampleSatd8x8_sse41;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_4x4] = WelsSampleSatd4x4_sse41;
    //pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Satd = WelsIntra16x16Combined3Satd_sse41;
    //pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Satd = WelsIntraChroma8x8Combined3Satd_sse41;
  }

#endif //(X86_ASM)

#if defined (HAVE_NEON)
  if (uiCpuFlag & WELS_CPU_NEON) {
    pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_4x4  ] = WelsSampleSad4x4_neon;
    pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_16x16] = WelsSampleSad16x16_neon;
    pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_16x8 ] = WelsSampleSad16x8_neon;
    pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_8x16] = WelsSampleSad8x16_neon;
    pFuncList->sSampleDealingFuncs.pfSampleSad[BLOCK_8x8] = WelsSampleSad8x8_neon;

    pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_16x16] = WelsSampleSadFour16x16_neon;
    pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_16x8] = WelsSampleSadFour16x8_neon;
    pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_8x16] = WelsSampleSadFour8x16_neon;
    pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_8x8] = WelsSampleSadFour8x8_neon;
    pFuncList->sSampleDealingFuncs.pfSample4Sad[BLOCK_4x4] = WelsSampleSadFour4x4_neon;

    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_4x4  ] = WelsSampleSatd4x4_neon;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_8x8  ] = WelsSampleSatd8x8_neon;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_8x16 ] = WelsSampleSatd8x16_neon;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_16x8 ] = WelsSampleSatd16x8_neon;
    pFuncList->sSampleDealingFuncs.pfSampleSatd[BLOCK_16x16] = WelsSampleSatd16x16_neon;

    pFuncList->sSampleDealingFuncs.pfIntra4x4Combined3Satd   = WelsIntra4x4Combined3Satd_neon;
    pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Satd   = WelsIntra8x8Combined3Satd_neon;
    pFuncList->sSampleDealingFuncs.pfIntra8x8Combined3Sad    = WelsIntra8x8Combined3Sad_neon;
    pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Satd = WelsIntra16x16Combined3Satd_neon;
    pFuncList->sSampleDealingFuncs.pfIntra16x16Combined3Sad  = WelsIntra16x16Combined3Sad_neon;
  }
#endif
}

} // namespace WelsSVCEnc
