/*!
 * \copy
 *     Copyright (c)  2010-2013, Cisco Systems
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
 * \file	deblocking.c
 *
 * \brief	Interfaces introduced in frame deblocking filtering
 *
 * \date	08/02/2010
 *
 *************************************************************************************
 */

#include "deblocking.h"
#include "cpu_core.h"
#include "fmo.h"

namespace WelsDec {

#define NO_SUPPORTED_FILTER_IDX     (-1)
#define LEFT_FLAG_BIT 0
#define TOP_FLAG_BIT 1
#define LEFT_FLAG_MASK 0x01
#define TOP_FLAG_MASK 0x02

#define SAME_MB_DIFF_REFIDX
#define g_kuiAlphaTable(x) g_kuiAlphaTable[(x)+12]
#define g_kiBetaTable(x)  g_kiBetaTable[(x)+12]
#define g_kiTc0Table(x)   g_kiTc0Table[(x)+12]

#define MB_BS_MV(iRefIndex, iMotionVector, iMbXy, iMbBn, iIndex, iNeighIndex) \
(\
    ( iRefIndex[iMbXy][iIndex] - iRefIndex[iMbBn][iNeighIndex] )||\
    ( WELS_ABS( iMotionVector[iMbXy][iIndex][0] - iMotionVector[iMbBn][iNeighIndex][0] ) >= 4 ) ||\
    ( WELS_ABS( iMotionVector[iMbXy][iIndex][1] - iMotionVector[iMbBn][iNeighIndex][1] ) >= 4 )\
)

#if defined(SAME_MB_DIFF_REFIDX)
#define SMB_EDGE_MV(iRefIndex, iMotionVector, iIndex, iNeighIndex) \
(\
    ( iRefIndex[iIndex] - iRefIndex[iNeighIndex] )||(\
    ( WELS_ABS( iMotionVector[iIndex][0] - iMotionVector[iNeighIndex][0] ) &(~3) ) |\
    ( WELS_ABS( iMotionVector[iIndex][1] - iMotionVector[iNeighIndex][1] ) &(~3) ))\
)
#else
#define SMB_EDGE_MV(iRefIndex, iMotionVector, iIndex, iNeighIndex) \
(\
    !!(( WELS_ABS( iMotionVector[iIndex][0] - iMotionVector[iNeighIndex][0] ) &(~3) ) |( WELS_ABS( iMotionVector[iIndex][1] - iMotionVector[iNeighIndex][1] ) &(~3) ))\
)
#endif

#define BS_EDGE(bsx1, iRefIndex, iMotionVector, iIndex, iNeighIndex) \
( (bsx1|SMB_EDGE_MV(iRefIndex, iMotionVector, iIndex, iNeighIndex))<<((uint8_t)(!!bsx1)))

#define GET_ALPHA_BETA_FROM_QP(iQp, iAlphaOffset, iBetaOffset, iIndex, iAlpha, iBeta) \
{\
	iIndex = (iQp + iAlphaOffset);\
	iAlpha = g_kuiAlphaTable(iIndex);\
	iBeta  = g_kiBetaTable((iQp + iBetaOffset));\
}

static const uint8_t g_kuiAlphaTable[52 + 24] = { //this table refers to Table 8-16 in H.264/AVC standard
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  4,  4,  5,  6,
  7,  8,  9, 10, 12, 13, 15, 17, 20, 22,
  25, 28, 32, 36, 40, 45, 50, 56, 63, 71,
  80, 90, 101, 113, 127, 144, 162, 182, 203, 226,
  255, 255
  , 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

static const int8_t g_kiBetaTable[52 + 24] = { //this table refers to Table 8-16 in H.264/AVC standard
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  2,  2,  2,  3,
  3,  3,  3,  4,  4,  4,  6,  6,  7,  7,
  8,  8,  9,  9, 10, 10, 11, 11, 12, 12,
  13, 13, 14, 14, 15, 15, 16, 16, 17, 17,
  18, 18
  , 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18
};

static const int8_t g_kiTc0Table[52 + 24][4] = { //this table refers Table 8-17 in H.264/AVC standard
  { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
  { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
  { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
  { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 },
  { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 0 }, { -1, 0, 0, 1 },
  { -1, 0, 0, 1 }, { -1, 0, 0, 1 }, { -1, 0, 0, 1 }, { -1, 0, 1, 1 }, { -1, 0, 1, 1 }, { -1, 1, 1, 1 },
  { -1, 1, 1, 1 }, { -1, 1, 1, 1 }, { -1, 1, 1, 1 }, { -1, 1, 1, 2 }, { -1, 1, 1, 2 }, { -1, 1, 1, 2 },
  { -1, 1, 1, 2 }, { -1, 1, 2, 3 }, { -1, 1, 2, 3 }, { -1, 2, 2, 3 }, { -1, 2, 2, 4 }, { -1, 2, 3, 4 },
  { -1, 2, 3, 4 }, { -1, 3, 3, 5 }, { -1, 3, 4, 6 }, { -1, 3, 4, 6 }, { -1, 4, 5, 7 }, { -1, 4, 5, 8 },
  { -1, 4, 6, 9 }, { -1, 5, 7, 10 }, { -1, 6, 8, 11 }, { -1, 6, 8, 13 }, { -1, 7, 10, 14 }, { -1, 8, 11, 16 },
  { -1, 9, 12, 18 }, { -1, 10, 13, 20 }, { -1, 11, 15, 23 }, { -1, 13, 17, 25 }
  , { -1, 13, 17, 25 }, { -1, 13, 17, 25 }, { -1, 13, 17, 25 }, { -1, 13, 17, 25 }, { -1, 13, 17, 25 }, { -1, 13, 17, 25 }
  , { -1, 13, 17, 25 }, { -1, 13, 17, 25 }, { -1, 13, 17, 25 }, { -1, 13, 17, 25 }, { -1, 13, 17, 25 }, { -1, 13, 17, 25 }
};

static const uint8_t g_kuiTableBIdx[2][8] = {
  {
    0,  4,  8,  12,
    3,  7,  11, 15
  },

  {
    0,  1,  2,  3 ,
    12, 13, 14, 15
  },
};

#define TC0_TBL_LOOKUP(tc, iIndexA, pBS, bChroma) \
{\
	tc[0] = g_kiTc0Table(iIndexA)[pBS[0]] + bChroma;\
	tc[1] = g_kiTc0Table(iIndexA)[pBS[1]] + bChroma;\
	tc[2] = g_kiTc0Table(iIndexA)[pBS[2]] + bChroma;\
	tc[3] = g_kiTc0Table(iIndexA)[pBS[3]] + bChroma;\
}

void_t inline DeblockingBSInsideMBAvsbase (int8_t* pNnzTab, uint8_t nBS[2][4][4], int32_t iLShiftFactor) {
  uint32_t uiNnz32b0, uiNnz32b1, uiNnz32b2, uiNnz32b3;
  FORCE_STACK_ALIGN_1D (uint8_t, uiBsx3, 4, 4);

  uiNnz32b0 = * (uint32_t*) (pNnzTab + 0);
  uiNnz32b1 = * (uint32_t*) (pNnzTab + 4);
  uiNnz32b2 = * (uint32_t*) (pNnzTab + 8);
  uiNnz32b3 = * (uint32_t*) (pNnzTab + 12);

  * (uint32_t*)uiBsx3 = (uiNnz32b0 | (uiNnz32b0 >> 8)) << iLShiftFactor;
  nBS[0][1][0] = uiBsx3[0];
  nBS[0][2][0] = uiBsx3[1];
  nBS[0][3][0] = uiBsx3[2];

  * (uint32_t*)uiBsx3 = (uiNnz32b1 | (uiNnz32b1 >> 8)) << iLShiftFactor;
  nBS[0][1][1] = uiBsx3[0];
  nBS[0][2][1] = uiBsx3[1];
  nBS[0][3][1] = uiBsx3[2];
  * (uint32_t*)nBS[1][1] = (uiNnz32b0 | uiNnz32b1) << iLShiftFactor;

  * (uint32_t*)uiBsx3 = (uiNnz32b2 | (uiNnz32b2 >> 8)) << iLShiftFactor;
  nBS[0][1][2] = uiBsx3[0];
  nBS[0][2][2] = uiBsx3[1];
  nBS[0][3][2] = uiBsx3[2];
  * (uint32_t*)nBS[1][2] = (uiNnz32b1 | uiNnz32b2) << iLShiftFactor;

  * (uint32_t*)uiBsx3 = (uiNnz32b3 | (uiNnz32b3 >> 8)) << iLShiftFactor;
  nBS[0][1][3] = uiBsx3[0];
  nBS[0][2][3] = uiBsx3[1];
  nBS[0][3][3] = uiBsx3[2];
  * (uint32_t*)nBS[1][3] = (uiNnz32b2 | uiNnz32b3) << iLShiftFactor;

}

void_t static inline DeblockingBSInsideMBNormal (PDqLayer pCurDqLayer, uint8_t nBS[2][4][4], int8_t* pNnzTab,
    int32_t iMbXy) {
  uint32_t uiNnz32b0, uiNnz32b1, uiNnz32b2, uiNnz32b3;
  int8_t* iRefIndex = pCurDqLayer->pRefIndex[LIST_0][iMbXy];
  FORCE_STACK_ALIGN_1D (uint8_t, uiBsx4, 4, 4);

  uiNnz32b0 = * (uint32_t*) (pNnzTab + 0);
  uiNnz32b1 = * (uint32_t*) (pNnzTab + 4);
  uiNnz32b2 = * (uint32_t*) (pNnzTab + 8);
  uiNnz32b3 = * (uint32_t*) (pNnzTab + 12);

  * (uint32_t*)uiBsx4 = (uiNnz32b0 | (uiNnz32b0 >> 8));
  nBS[0][1][0] = BS_EDGE (uiBsx4[0], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 1, 0);
  nBS[0][2][0] = BS_EDGE (uiBsx4[1], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 2, 1);
  nBS[0][3][0] = BS_EDGE (uiBsx4[2], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 3, 2);

  * (uint32_t*)uiBsx4 = (uiNnz32b1 | (uiNnz32b1 >> 8));
  nBS[0][1][1] = BS_EDGE (uiBsx4[0], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 5, 4);
  nBS[0][2][1] = BS_EDGE (uiBsx4[1], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 6, 5);
  nBS[0][3][1] = BS_EDGE (uiBsx4[2], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 7, 6);

  * (uint32_t*)uiBsx4 = (uiNnz32b2 | (uiNnz32b2 >> 8));
  nBS[0][1][2] = BS_EDGE (uiBsx4[0], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 9, 8);
  nBS[0][2][2] = BS_EDGE (uiBsx4[1], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 10, 9);
  nBS[0][3][2] = BS_EDGE (uiBsx4[2], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 11, 10);

  * (uint32_t*)uiBsx4 = (uiNnz32b3 | (uiNnz32b3 >> 8));
  nBS[0][1][3] = BS_EDGE (uiBsx4[0], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 13, 12);
  nBS[0][2][3] = BS_EDGE (uiBsx4[1], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 14, 13);
  nBS[0][3][3] = BS_EDGE (uiBsx4[2], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 15, 14);

  // horizontal
  * (uint32_t*)uiBsx4 = (uiNnz32b0 | uiNnz32b1);
  nBS[1][1][0] = BS_EDGE (uiBsx4[0], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 4, 0);
  nBS[1][1][1] = BS_EDGE (uiBsx4[1], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 5, 1);
  nBS[1][1][2] = BS_EDGE (uiBsx4[2], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 6, 2);
  nBS[1][1][3] = BS_EDGE (uiBsx4[3], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 7, 3);

  * (uint32_t*)uiBsx4 = (uiNnz32b1 | uiNnz32b2);
  nBS[1][2][0] = BS_EDGE (uiBsx4[0], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 8, 4);
  nBS[1][2][1] = BS_EDGE (uiBsx4[1], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 9, 5);
  nBS[1][2][2] = BS_EDGE (uiBsx4[2], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 10, 6);
  nBS[1][2][3] = BS_EDGE (uiBsx4[3], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 11, 7);

  * (uint32_t*)uiBsx4 = (uiNnz32b2 | uiNnz32b3);
  nBS[1][3][0] = BS_EDGE (uiBsx4[0], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 12, 8);
  nBS[1][3][1] = BS_EDGE (uiBsx4[1], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 13, 9);
  nBS[1][3][2] = BS_EDGE (uiBsx4[2], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 14, 10);
  nBS[1][3][3] = BS_EDGE (uiBsx4[3], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 15, 11);
}

uint32_t DeblockingBsMarginalMBAvcbase (PDqLayer pCurDqLayer, int32_t iEdge, int32_t iNeighMb, int32_t iMbXy) {
  int32_t i;
  uint32_t uiBSx4;
  //uint8_t* bS = static_cast<uint8_t*>(&uiBSx4);
  uint8_t* pBS = (uint8_t*) (&uiBSx4);
  uint32_t uiBIdx  = * (uint32_t*) (&g_kuiTableBIdx[iEdge][0]);
  uint32_t uiBnIdx = * (uint32_t*) (&g_kuiTableBIdx[iEdge][4]);

  for (i = 0; i < 4; i++) {
    if (pCurDqLayer->pNzc[iMbXy][uiBIdx & 0xff] | pCurDqLayer->pNzc[iNeighMb][uiBnIdx & 0xff]) {
      pBS[i] = 2;
    } else {
      pBS[i] = MB_BS_MV (pCurDqLayer->pRefIndex[LIST_0], pCurDqLayer->pMv[LIST_0], iMbXy, iNeighMb, (uiBIdx & 0xff),
                         (uiBnIdx & 0xff));
    }
    uiBIdx  = uiBIdx  >> 8;
    uiBnIdx = uiBnIdx >> 8;
  }
  return uiBSx4;
}
int32_t DeblockingAvailableNoInterlayer (PDqLayer pCurDqLayer, int32_t iFilterIdc) {
  int32_t iMbY = pCurDqLayer->iMbY;
  int32_t iMbX = pCurDqLayer->iMbX;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  BOOL_T bLeftFlag = FALSE;
  BOOL_T bTopFlag  = FALSE;

  if (2 == iFilterIdc) {
    bLeftFlag = (iMbX > 0) && (pCurDqLayer->pSliceIdc[iMbXy] == pCurDqLayer->pSliceIdc[iMbXy - 1]);
    bTopFlag  = (iMbY > 0) && (pCurDqLayer->pSliceIdc[iMbXy] == pCurDqLayer->pSliceIdc[iMbXy - pCurDqLayer->iMbWidth]);
  } else { //if ( 0 == iFilterIdc )
    bLeftFlag = (iMbX > 0);
    bTopFlag  = (iMbY > 0);
  }
  return (bLeftFlag << LEFT_FLAG_BIT) | (bTopFlag << TOP_FLAG_BIT);
}

void_t FilteringEdgeLumaH (SDeblockingFilter* pFilter, uint8_t* pPix, int32_t iStride, uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;
  FORCE_STACK_ALIGN_1D (int8_t, tc, 4, 16);

  GET_ALPHA_BETA_FROM_QP (pFilter->iLumaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);

  if (iAlpha | iBeta) {
    TC0_TBL_LOOKUP (tc, iIndexA, pBS, 0);
    pFilter->pLoopf->pfLumaDeblockingLT4Ver (pPix, iStride, iAlpha, iBeta, tc);
  }
  return;
}


void_t FilteringEdgeLumaV (SDeblockingFilter* pFilter, uint8_t* pPix, int32_t iStride, uint8_t* pBS) {
  int32_t  iIndexA;
  int32_t  iAlpha;
  int32_t  iBeta;
  FORCE_STACK_ALIGN_1D (int8_t, tc, 4, 16);

  GET_ALPHA_BETA_FROM_QP (pFilter->iLumaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);

  if (iAlpha | iBeta) {
    TC0_TBL_LOOKUP (tc, iIndexA, pBS, 0);
    pFilter->pLoopf->pfLumaDeblockingLT4Hor (pPix, iStride, iAlpha, iBeta, tc);
  }
  return;
}


void_t FilteringEdgeLumaIntraH (SDeblockingFilter* pFilter, uint8_t* pPix, int32_t iStride, uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;

  GET_ALPHA_BETA_FROM_QP (pFilter->iLumaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);

  if (iAlpha | iBeta) {
    pFilter->pLoopf->pfLumaDeblockingEQ4Ver (pPix, iStride, iAlpha, iBeta);
  }
  return;
}

void_t FilteringEdgeLumaIntraV (SDeblockingFilter* pFilter, uint8_t* pPix, int32_t iStride, uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;

  GET_ALPHA_BETA_FROM_QP (pFilter->iLumaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);

  if (iAlpha | iBeta) {
    pFilter->pLoopf->pfLumaDeblockingEQ4Hor (pPix, iStride, iAlpha, iBeta);
  }
  return;
}
void_t FilteringEdgeChromaH (SDeblockingFilter* pFilter, uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride,
                             uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;
  FORCE_STACK_ALIGN_1D (int8_t, tc, 4, 16);

  GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);

  if (iAlpha | iBeta) {
    TC0_TBL_LOOKUP (tc, iIndexA, pBS, 1);
    pFilter->pLoopf->pfChromaDeblockingLT4Ver (pPixCb, pPixCr, iStride, iAlpha, iBeta, tc);
  }
  return;
}
void_t FilteringEdgeChromaV (SDeblockingFilter* pFilter, uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride,
                             uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;
  FORCE_STACK_ALIGN_1D (int8_t, tc, 4, 16);

  GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);

  if (iAlpha | iBeta) {
    TC0_TBL_LOOKUP (tc, iIndexA, pBS, 1);
    pFilter->pLoopf->pfChromaDeblockingLT4Hor (pPixCb, pPixCr, iStride, iAlpha, iBeta, tc);
  }
  return;
}

void_t FilteringEdgeChromaIntraH (SDeblockingFilter* pFilter, uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride,
                                  uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;

  GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);

  if (iAlpha | iBeta) {
    pFilter->pLoopf->pfChromaDeblockingEQ4Ver (pPixCb, pPixCr, iStride, iAlpha, iBeta);
  }
  return;
}

void_t FilteringEdgeChromaIntraV (SDeblockingFilter* pFilter, uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride,
                                  uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;

  GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);

  if (iAlpha | iBeta) {
    pFilter->pLoopf->pfChromaDeblockinEQ4Hor (pPixCb, pPixCr, iStride, iAlpha, iBeta);
  }
  return;
}


void_t DeblockingInterMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, uint8_t nBS[2][4][4],
                          int32_t iBoundryFlag) {
  int32_t iMbXyIndex = pCurDqLayer->iMbXyIndex;
  int32_t iMbX = pCurDqLayer->iMbX;
  int32_t iMbY = pCurDqLayer->iMbY;

  int32_t iCurLumaQp = pCurDqLayer->pLumaQp[iMbXyIndex];
  int32_t iCurChromaQp = pCurDqLayer->pChromaQp[iMbXyIndex];
  int32_t iLineSize   = pFilter->iCsStride[0];
  int32_t iLineSizeUV = pFilter->iCsStride[1];

  uint8_t* pDestY, * pDestCb, * pDestCr;
  pDestY  = pFilter->pCsData[0] + ((iMbY * iLineSize + iMbX) << 4);
  pDestCb = pFilter->pCsData[1] + ((iMbY * iLineSizeUV + iMbX) << 3);
  pDestCr = pFilter->pCsData[2] + ((iMbY * iLineSizeUV + iMbX) << 3);

  if (iBoundryFlag & LEFT_FLAG_MASK) {
    int32_t iLeftXyIndex = iMbXyIndex - 1;
    pFilter->iLumaQP   = (iCurLumaQp + pCurDqLayer->pLumaQp[iLeftXyIndex] + 1) >> 1;
    pFilter->iChromaQP = (iCurChromaQp + pCurDqLayer->pChromaQp[iLeftXyIndex] + 1) >> 1;

    if (nBS[0][0][0] == 0x04) {
      FilteringEdgeLumaIntraV (pFilter, pDestY, iLineSize, NULL);
      FilteringEdgeChromaIntraV (pFilter, pDestCb, pDestCr, iLineSizeUV, NULL);
    } else {
      if (* (uint32_t*)nBS[0][0] != 0) {
        FilteringEdgeLumaV (pFilter, pDestY, iLineSize, nBS[0][0]);
        FilteringEdgeChromaV (pFilter, pDestCb, pDestCr, iLineSizeUV, nBS[0][0]);
      }
    }
  }

  pFilter->iLumaQP = iCurLumaQp;
  pFilter->iChromaQP = iCurChromaQp;

  if (* (uint32_t*)nBS[0][1] != 0) {
    FilteringEdgeLumaV (pFilter, &pDestY[1 << 2], iLineSize, nBS[0][1]);
  }

  if (* (uint32_t*)nBS[0][2] != 0) {
    FilteringEdgeLumaV (pFilter, &pDestY[2 << 2], iLineSize, nBS[0][2]);
    FilteringEdgeChromaV (pFilter, &pDestCb[2 << 1], &pDestCr[2 << 1], iLineSizeUV, nBS[0][2]);
  }

  if (* (uint32_t*)nBS[0][3] != 0) {
    FilteringEdgeLumaV (pFilter, &pDestY[3 << 2], iLineSize, nBS[0][3]);
  }

  if (iBoundryFlag & TOP_FLAG_MASK) {
    int32_t iTopXyIndex = iMbXyIndex - pCurDqLayer->iMbWidth;
    pFilter->iLumaQP = (iCurLumaQp + pCurDqLayer->pLumaQp[iTopXyIndex] + 1) >> 1;
    pFilter->iChromaQP = (iCurChromaQp + pCurDqLayer->pChromaQp[iTopXyIndex] + 1) >> 1;

    if (nBS[1][0][0] == 0x04) {
      FilteringEdgeLumaIntraH (pFilter, pDestY, iLineSize, NULL);
      FilteringEdgeChromaIntraH (pFilter, pDestCb, pDestCr, iLineSizeUV, NULL);
    } else {
      if (* (uint32_t*)nBS[1][0] != 0) {
        FilteringEdgeLumaH (pFilter, pDestY, iLineSize, nBS[1][0]);
        FilteringEdgeChromaH (pFilter, pDestCb, pDestCr, iLineSizeUV, nBS[1][0]);
      }
    }
  }

  pFilter->iLumaQP = iCurLumaQp;
  pFilter->iChromaQP = iCurChromaQp;

  if (* (uint32_t*)nBS[1][1] != 0) {
    FilteringEdgeLumaH (pFilter, &pDestY[ (1 << 2)*iLineSize], iLineSize, nBS[1][1]);
  }

  if (* (uint32_t*)nBS[1][2] != 0) {
    FilteringEdgeLumaH (pFilter, &pDestY[ (2 << 2)*iLineSize], iLineSize, nBS[1][2]);
    FilteringEdgeChromaH (pFilter, &pDestCb[ (2 << 1)*iLineSizeUV], &pDestCr[ (2 << 1)*iLineSizeUV], iLineSizeUV,
                          nBS[1][2]);
  }

  if (* (uint32_t*)nBS[1][3] != 0) {
    FilteringEdgeLumaH (pFilter, &pDestY[ (3 << 2)*iLineSize], iLineSize, nBS[1][3]);
  }
}

void_t /*__FASTCALL*/ FilteringEdgeLumaHV (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag) {
  int32_t iMbXyIndex = pCurDqLayer->iMbXyIndex;
  int32_t iMbX      = pCurDqLayer->iMbX;
  int32_t iMbY      = pCurDqLayer->iMbY;
  int32_t iMbWidth  = pCurDqLayer->iMbWidth;
  int32_t iLineSize  = pFilter->iCsStride[0];

  uint8_t*  pDestY;
  int32_t  iCurQp;
  int32_t  iIndexA, iAlpha, iBeta;

  FORCE_STACK_ALIGN_1D (int8_t,  iTc,   4, 16);
  FORCE_STACK_ALIGN_1D (uint8_t, uiBSx4, 4, 4);

  pDestY  = pFilter->pCsData[0] + ((iMbY * iLineSize + iMbX) << 4);
  iCurQp  = pCurDqLayer->pLumaQp[iMbXyIndex];

  * (uint32_t*)uiBSx4 = 0x03030303;

  // luma v
  if (iBoundryFlag & LEFT_FLAG_MASK) {
    pFilter->iLumaQP   = (iCurQp   + pCurDqLayer->pLumaQp[iMbXyIndex - 1] + 1) >> 1;
    FilteringEdgeLumaIntraV (pFilter, pDestY, iLineSize, NULL);
  }

  pFilter->iLumaQP   = iCurQp;
  GET_ALPHA_BETA_FROM_QP (pFilter->iLumaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);
  if (iAlpha | iBeta) {
    TC0_TBL_LOOKUP (iTc, iIndexA, uiBSx4, 0);
    pFilter->pLoopf->pfLumaDeblockingLT4Hor (&pDestY[1 << 2], iLineSize, iAlpha, iBeta, iTc);
    pFilter->pLoopf->pfLumaDeblockingLT4Hor (&pDestY[2 << 2], iLineSize, iAlpha, iBeta, iTc);
    pFilter->pLoopf->pfLumaDeblockingLT4Hor (&pDestY[3 << 2], iLineSize, iAlpha, iBeta, iTc);
  }

  // luma h
  if (iBoundryFlag & TOP_FLAG_MASK) {
    pFilter->iLumaQP   = (iCurQp   + pCurDqLayer->pLumaQp[iMbXyIndex - iMbWidth] + 1) >> 1;
    FilteringEdgeLumaIntraH (pFilter, pDestY, iLineSize, NULL);
  }

  pFilter->iLumaQP   = iCurQp;
  if (iAlpha | iBeta) {
    pFilter->pLoopf->pfLumaDeblockingLT4Ver (&pDestY[ (1 << 2)*iLineSize], iLineSize, iAlpha, iBeta, iTc);
    pFilter->pLoopf->pfLumaDeblockingLT4Ver (&pDestY[ (2 << 2)*iLineSize], iLineSize, iAlpha, iBeta, iTc);
    pFilter->pLoopf->pfLumaDeblockingLT4Ver (&pDestY[ (3 << 2)*iLineSize], iLineSize, iAlpha, iBeta, iTc);
  }
}
void_t /*__FASTCALL*/ FilteringEdgeChromaHV (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag) {
  int32_t iMbXyIndex     = pCurDqLayer->iMbXyIndex;
  int32_t iMbX      = pCurDqLayer->iMbX;
  int32_t iMbY      = pCurDqLayer->iMbY;
  int32_t iMbWidth  = pCurDqLayer->iMbWidth;
  int32_t iLineSize  = pFilter->iCsStride[1];

  uint8_t*  pDestCb, *pDestCr;
  int32_t  iCurQp;
  int32_t  iIndexA, iAlpha, iBeta;

  FORCE_STACK_ALIGN_1D (int8_t,  iTc,   4, 16);
  FORCE_STACK_ALIGN_1D (uint8_t, uiBSx4, 4, 4);

  pDestCb = pFilter->pCsData[1] + ((iMbY * iLineSize + iMbX) << 3);
  pDestCr = pFilter->pCsData[2] + ((iMbY * iLineSize + iMbX) << 3);
  iCurQp  = pCurDqLayer->pChromaQp[iMbXyIndex];
  * (uint32_t*)uiBSx4 = 0x03030303;

  // chroma v
  if (iBoundryFlag & LEFT_FLAG_MASK) {
    pFilter->iChromaQP = (iCurQp + pCurDqLayer->pChromaQp[iMbXyIndex - 1] + 1) >> 1;
    FilteringEdgeChromaIntraV (pFilter, pDestCb, pDestCr, iLineSize, NULL);
  }

  pFilter->iChromaQP   = iCurQp;
  GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);
  if (iAlpha | iBeta) {
    TC0_TBL_LOOKUP (iTc, iIndexA, uiBSx4, 1);
    pFilter->pLoopf->pfChromaDeblockingLT4Hor (&pDestCb[2 << 1], &pDestCr[2 << 1], iLineSize, iAlpha, iBeta, iTc);
  }

  // chroma h
  if (iBoundryFlag & TOP_FLAG_MASK) {
    pFilter->iChromaQP = (iCurQp + pCurDqLayer->pChromaQp[iMbXyIndex - iMbWidth] + 1) >> 1;
    FilteringEdgeChromaIntraH (pFilter, pDestCb, pDestCr, iLineSize, NULL);
  }

  pFilter->iChromaQP   = iCurQp;
  if (iAlpha | iBeta) {
    pFilter->pLoopf->pfChromaDeblockingLT4Ver (&pDestCb[ (2 << 1)*iLineSize], &pDestCr[ (2 << 1)*iLineSize], iLineSize,
        iAlpha, iBeta, iTc);
  }
}

// merge h&v lookup table operation to save performance
void_t DeblockingIntraMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag) {
  FilteringEdgeLumaHV (pCurDqLayer, pFilter, iBoundryFlag);
  FilteringEdgeChromaHV (pCurDqLayer, pFilter, iBoundryFlag);
}

void_t WelsDeblockingMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag) {
  uint8_t nBS[2][4][4] = { 0 };

  int32_t iMbXyIndex	= pCurDqLayer->iMbXyIndex;
  int32_t iCurMbType  = pCurDqLayer->pMbType[iMbXyIndex];
  int32_t iMbNb;

  switch (iCurMbType) {
  case MB_TYPE_INTRA4x4:
  case MB_TYPE_INTRA16x16:
  case MB_TYPE_INTRA_PCM:
    DeblockingIntraMb (pCurDqLayer, pFilter, iBoundryFlag);
    break;
  default:

    if (iBoundryFlag & LEFT_FLAG_MASK) {
      iMbNb = iMbXyIndex - 1;
      * (uint32_t*)nBS[0][0] = IS_INTRA (pCurDqLayer->pMbType[iMbNb]) ? 0x04040404 : DeblockingBsMarginalMBAvcbase (
                                 pCurDqLayer, 0, iMbNb, iMbXyIndex);
    } else {
      * (uint32_t*)nBS[0][0] = 0;
    }
    if (iBoundryFlag & TOP_FLAG_MASK) {
      iMbNb = iMbXyIndex - pCurDqLayer->iMbWidth;
      * (uint32_t*)nBS[1][0] = IS_INTRA (pCurDqLayer->pMbType[iMbNb]) ? 0x04040404 : DeblockingBsMarginalMBAvcbase (
                                 pCurDqLayer, 1, iMbNb, iMbXyIndex);
    } else {
      * (uint32_t*)nBS[1][0] = 0;
    }
    //SKIP MB_16x16 or others
    if (iCurMbType != MB_TYPE_SKIP) {
      if (iCurMbType == MB_TYPE_16x16) {
        DeblockingBSInsideMBAvsbase (pCurDqLayer->pNzc[iMbXyIndex], nBS, 1);
      } else {
        DeblockingBSInsideMBNormal (pCurDqLayer, nBS, pCurDqLayer->pNzc[iMbXyIndex], iMbXyIndex);
      }
    } else {
      * (uint32_t*)nBS[0][1] = * (uint32_t*)nBS[0][2] = * (uint32_t*)nBS[0][3] =
                                 * (uint32_t*)nBS[1][1] = * (uint32_t*)nBS[1][2] = * (uint32_t*)nBS[1][3] = 0;
    }
    DeblockingInterMb (pCurDqLayer, pFilter, nBS, iBoundryFlag);
    break;
  }
}

//  C code only
void_t DeblockLumaLt4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta,
                         int8_t* pTc) {
  for (int32_t i = 0; i < 16; i++) {
    int32_t iTc0 = pTc[i >> 2];
    if (iTc0 >= 0) {
      int32_t p0 = pPix[-iStrideX];
      int32_t p1 = pPix[-2 * iStrideX];
      int32_t p2 = pPix[-3 * iStrideX];
      int32_t q0 = pPix[0];
      int32_t q1 = pPix[iStrideX];
      int32_t q2 = pPix[2 * iStrideX];
      bool_t bDetaP0Q0 = WELS_ABS (p0 - q0) < iAlpha;
      bool_t bDetaP1P0 = WELS_ABS (p1 - p0) < iBeta;
      bool_t bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;
      int32_t iTc = iTc0;
      if (bDetaP0Q0 && bDetaP1P0 && bDetaQ1Q0) {
        bool_t bDetaP2P0 =  WELS_ABS (p2 - p0) < iBeta;
        bool_t bDetaQ2Q0 =  WELS_ABS (q2 - q0) < iBeta;
        if (bDetaP2P0) {
          pPix[-2 * iStrideX] = p1 + WELS_CLIP3 ((p2 + ((p0 + q0 + 1) >> 1) - (p1 << 1)) >> 1, -iTc0, iTc0);
          iTc++;
        }
        if (bDetaQ2Q0) {
          pPix[iStrideX] = q1 + WELS_CLIP3 ((q2 + ((p0 + q0 + 1) >> 1) - (q1 << 1)) >> 1, -iTc0, iTc0);
          iTc++;
        }
        int32_t iDeta = WELS_CLIP3 ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3, -iTc, iTc);
        pPix[-iStrideX] = WELS_CLIP1 (p0 + iDeta);     /* p0' */
        pPix[0]  = WELS_CLIP1 (q0 - iDeta);     /* q0' */
      }
    }
    pPix += iStrideY;
  }
}
void_t DeblockLumaEq4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta) {
  int32_t p0, p1, p2, q0, q1, q2;
  int32_t iDetaP0Q0;
  bool_t bDetaP1P0, bDetaQ1Q0;
  for (int32_t i = 0; i < 16; i++) {
    p0 = pPix[-iStrideX];
    p1 = pPix[-2 * iStrideX];
    p2 = pPix[-3 * iStrideX];
    q0 = pPix[0];
    q1 = pPix[iStrideX];
    q2 = pPix[2 * iStrideX];
    iDetaP0Q0 = WELS_ABS (p0 - q0);
    bDetaP1P0 = WELS_ABS (p1 - p0) < iBeta;
    bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;
    if ((iDetaP0Q0 < iAlpha) && bDetaP1P0 && bDetaQ1Q0) {
      if (iDetaP0Q0 < ((iAlpha >> 2) + 2)) {
        bool_t bDetaP2P0 = WELS_ABS (p2 - p0) < iBeta;
        bool_t bDetaQ2Q0 =  WELS_ABS (q2 - q0) < iBeta;
        if (bDetaP2P0) {
          const int32_t p3 = pPix[-4 * iStrideX];
          pPix[-iStrideX] = (p2 + (p1 << 1) + (p0 << 1) + (q0 << 1) + q1 + 4) >> 3;	   //p0
          pPix[-2 * iStrideX] = (p2 + p1 + p0 + q0 + 2) >> 2;	 //p1
          pPix[-3 * iStrideX] = ((p3 << 1) + p2 + (p2 << 1) + p1 + p0 + q0 + 4) >> 3;//p2
        } else {
          pPix[-1 * iStrideX] = ((p1 << 1) + p0 + q1 + 2) >> 2;	//p0
        }
        if (bDetaQ2Q0) {
          const int32_t q3 = pPix[3 * iStrideX];
          pPix[0] = (p1 + (p0 << 1) + (q0 << 1) + (q1 << 1) + q2 + 4) >> 3;   //q0
          pPix[iStrideX] = (p0 + q0 + q1 + q2 + 2) >> 2;   //q1
          pPix[2 * iStrideX] = ((q3 << 1) + q2 + (q2 << 1) + q1 + q0 + p0 + 4) >> 3;//q2
        } else {
          pPix[0] = ((q1 << 1) + q0 + p1 + 2) >> 2;   //q0
        }
      } else {
        pPix[-iStrideX] = ((p1 << 1) + p0 + q1 + 2) >> 2;   //p0
        pPix[ 0] = ((q1 << 1) + q0 + p1 + 2) >> 2;   //q0
      }
    }
    pPix += iStrideY;
  }
}
void_t DeblockLumaLt4V_c (uint8_t* pPix, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* tc) {
  DeblockLumaLt4_c (pPix, iStride, 1, iAlpha, iBeta, tc);
}
void_t DeblockLumaLt4H_c (uint8_t* pPix, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* tc) {
  DeblockLumaLt4_c (pPix, 1, iStride, iAlpha, iBeta, tc);
}
void_t DeblockLumaEq4V_c (uint8_t* pPix, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
  DeblockLumaEq4_c (pPix, iStride, 1, iAlpha, iBeta);
}
void_t DeblockLumaEq4H_c (uint8_t* pPix, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
  DeblockLumaEq4_c (pPix, 1, iStride, iAlpha, iBeta);
}
void_t DeblockChromaLt4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha,
                           int32_t iBeta, int8_t* pTc) {
  int32_t p0, p1, q0, q1, iDeta;
  bool_t bDetaP0Q0, bDetaP1P0, bDetaQ1Q0;

  for (int32_t i = 0; i < 8; i++) {
    int32_t iTc0 = pTc[i >> 1];
    if (iTc0 > 0) {
      p0 = pPixCb[-iStrideX];
      p1 = pPixCb[-2 * iStrideX];
      q0 = pPixCb[0];
      q1 = pPixCb[iStrideX];

      bDetaP0Q0 =  WELS_ABS (p0 - q0) < iAlpha;
      bDetaP1P0 =  WELS_ABS (p1 - p0) < iBeta;
      bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;
      if (bDetaP0Q0 && bDetaP1P0 &&	bDetaQ1Q0) {
        iDeta = WELS_CLIP3 ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3, -iTc0, iTc0);
        pPixCb[-iStrideX] = WELS_CLIP1 (p0 + iDeta);     /* p0' */
        pPixCb[0]  = WELS_CLIP1 (q0 - iDeta);     /* q0' */
      }


      p0 = pPixCr[-iStrideX];
      p1 = pPixCr[-2 * iStrideX];
      q0 = pPixCr[0];
      q1 = pPixCr[iStrideX];

      bDetaP0Q0 =  WELS_ABS (p0 - q0) < iAlpha;
      bDetaP1P0 =  WELS_ABS (p1 - p0) < iBeta;
      bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;

      if (bDetaP0Q0 && bDetaP1P0 &&	bDetaQ1Q0) {
        iDeta = WELS_CLIP3 ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3, -iTc0, iTc0);
        pPixCr[-iStrideX] = WELS_CLIP1 (p0 + iDeta);     /* p0' */
        pPixCr[0]  = WELS_CLIP1 (q0 - iDeta);     /* q0' */
      }
    }
    pPixCb += iStrideY;
    pPixCr += iStrideY;
  }
}
void_t DeblockChromaEq4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha,
                           int32_t iBeta) {
  int32_t i = 0, d = 0;
  int32_t p0, p1, q0, q1;
  bool_t bDetaP0Q0, bDetaP1P0, bDetaQ1Q0;
  for (int32_t i = 0; i < 8; i++) {
    //cb
    p0 = pPixCb[-iStrideX];
    p1 = pPixCb[-2 * iStrideX];
    q0 = pPixCb[0];
    q1 = pPixCb[iStrideX];
    bDetaP0Q0 = WELS_ABS (p0 - q0) < iAlpha;
    bDetaP1P0 = WELS_ABS (p1 - p0) < iBeta;
    bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;
    if (bDetaP0Q0 && bDetaP1P0 && bDetaQ1Q0) {
      pPixCb[-iStrideX] = ((p1 << 1) + p0 + q1 + 2) >> 2;     /* p0' */
      pPixCb[0]  = ((q1 << 1) + q0 + p1 + 2) >> 2;     /* q0' */
    }

    //cr
    p0 = pPixCr[-iStrideX];
    p1 = pPixCr[-2 * iStrideX];
    q0 = pPixCr[0];
    q1 = pPixCr[iStrideX];
    bDetaP0Q0 = WELS_ABS (p0 - q0) < iAlpha;
    bDetaP1P0 = WELS_ABS (p1 - p0) < iBeta;
    bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;
    if (bDetaP0Q0 && bDetaP1P0 && bDetaQ1Q0) {
      pPixCr[-iStrideX] = ((p1 << 1) + p0 + q1 + 2) >> 2;     /* p0' */
      pPixCr[0]  = ((q1 << 1) + q0 + p1 + 2) >> 2;     /* q0' */
    }
    pPixCr += iStrideY;
    pPixCb += iStrideY;
  }
}
void_t DeblockChromaLt4V_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                            int8_t* tc) {
  DeblockChromaLt4_c (pPixCb, pPixCr, iStride, 1, iAlpha, iBeta, tc);
}
void_t DeblockChromaLt4H_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                            int8_t* tc) {
  DeblockChromaLt4_c (pPixCb, pPixCr, 1, iStride, iAlpha, iBeta, tc);
}
void_t DeblockChromaEq4V_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
  DeblockChromaEq4_c (pPixCb, pPixCr, iStride, 1, iAlpha, iBeta);
}
void_t DeblockChromaEq4H_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
  DeblockChromaEq4_c (pPixCb, pPixCr, 1, iStride, iAlpha, iBeta);
}

#ifdef X86_ASM
extern "C" {
  void DeblockLumaLt4H_sse2 (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc) {
    FORCE_STACK_ALIGN_1D (uint8_t,  uiBuf,   16 * 8, 16);

    DeblockLumaTransposeH2V_sse2 (pPixY - 4, iStride, &uiBuf[0]);
    DeblockLumaLt4V_sse2 (&uiBuf[4 * 16], 16, iAlpha, iBeta, pTc);
    DeblockLumaTransposeV2H_sse2 (pPixY - 4, iStride, &uiBuf[0]);
  }

  void DeblockLumaEq4H_sse2 (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
    FORCE_STACK_ALIGN_1D (uint8_t,  uiBuf,   16 * 8, 16);

    DeblockLumaTransposeH2V_sse2 (pPixY - 4, iStride, &uiBuf[0]);
    DeblockLumaEq4V_sse2 (&uiBuf[4 * 16], 16, iAlpha, iBeta);
    DeblockLumaTransposeV2H_sse2 (pPixY - 4, iStride, &uiBuf[0]);
  }

}

#endif
/*!
 * \brief	AVC slice deblocking filtering target layer
 *
 * \param	dec			Wels avc decoder context
 *
 * \return	NONE
 */
void_t WelsDeblockingFilterSlice (PWelsDecoderContext pCtx, PDeblockingFilterMbFunc pDeblockMb) {
  PDqLayer pCurDqLayer = pCtx->pCurDqLayer;
  PSliceHeaderExt pSliceHeaderExt = &pCurDqLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt;
  int32_t iMbWidth  = pCurDqLayer->iMbWidth;
  int32_t iTotalMbCount = pSliceHeaderExt->sSliceHeader.pSps->uiTotalMbCount;

  SDeblockingFilter pFilter = {0};

  PFmo pFmo = pCtx->pFmo;
  int32_t iNextMbXyIndex = 0;
  int32_t iTotalNumMb = pCurDqLayer->sLayerInfo.sSliceInLayer.iTotalMbInCurSlice;
  int32_t iCountNumMb = 0;
  int32_t iBoundryFlag;
  int32_t iFilterIdc = pCurDqLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.uiDisableDeblockingFilterIdc;

  /* Step1: parameters set */
  pFilter.pCsData[0] = pCtx->pDec->pData[0];
  pFilter.pCsData[1] = pCtx->pDec->pData[1];
  pFilter.pCsData[2] = pCtx->pDec->pData[2];

  pFilter.iCsStride[0] = pCtx->pDec->iLinesize[0];
  pFilter.iCsStride[1] = pCtx->pDec->iLinesize[1];

  pFilter.eSliceType = (ESliceType) pCurDqLayer->sLayerInfo.sSliceInLayer.eSliceType;

  pFilter.iSliceAlphaC0Offset = pSliceHeaderExt->sSliceHeader.iSliceAlphaC0Offset;
  pFilter.iSliceBetaOffset     = pSliceHeaderExt->sSliceHeader.iSliceBetaOffset;

  pFilter.pLoopf = &pCtx->sDeblockingFunc;

  /* Step2: macroblock deblocking */
  if (0 == iFilterIdc || 2 == iFilterIdc) {
    iNextMbXyIndex = pSliceHeaderExt->sSliceHeader.iFirstMbInSlice;
    pCurDqLayer->iMbX  = iNextMbXyIndex % iMbWidth;
    pCurDqLayer->iMbY  = iNextMbXyIndex / iMbWidth;
    pCurDqLayer->iMbXyIndex = iNextMbXyIndex;

    do {
      iBoundryFlag = DeblockingAvailableNoInterlayer (pCurDqLayer, iFilterIdc);

      pDeblockMb (pCurDqLayer, &pFilter, iBoundryFlag);

      ++iCountNumMb;
      if (iCountNumMb >= iTotalNumMb) {
        break;
      }

      if (pSliceHeaderExt->sSliceHeader.pPps->uiNumSliceGroups > 1) {
        iNextMbXyIndex = FmoNextMb (pFmo, iNextMbXyIndex);
      } else {
        ++iNextMbXyIndex;
      }
      if (-1 == iNextMbXyIndex || iNextMbXyIndex >= iTotalMbCount) {	// slice group boundary or end of a frame
        break;
      }

      pCurDqLayer->iMbX  = iNextMbXyIndex % iMbWidth;
      pCurDqLayer->iMbY  = iNextMbXyIndex / iMbWidth;
      pCurDqLayer->iMbXyIndex = iNextMbXyIndex;
    } while (1);
  }
}
/*!
 * \brief	deblocking module initialize
 *
 * \param	pf
 *          cpu
 *
 * \return	NONE
 */

void_t  DeblockingInit (SDeblockingFunc*  pFunc,  int32_t iCpu) {
  pFunc->pfLumaDeblockingLT4Ver		= DeblockLumaLt4V_c;
  pFunc->pfLumaDeblockingEQ4Ver		= DeblockLumaEq4V_c;
  pFunc->pfLumaDeblockingLT4Hor		= DeblockLumaLt4H_c;
  pFunc->pfLumaDeblockingEQ4Hor		= DeblockLumaEq4H_c;

  pFunc->pfChromaDeblockingLT4Ver	    = DeblockChromaLt4V_c;
  pFunc->pfChromaDeblockingEQ4Ver	    = DeblockChromaEq4V_c;
  pFunc->pfChromaDeblockingLT4Hor	    = DeblockChromaLt4H_c;
  pFunc->pfChromaDeblockinEQ4Hor	    = DeblockChromaEq4H_c;

#ifdef X86_ASM
  if (iCpu & WELS_CPU_SSE2) {
    pFunc->pfLumaDeblockingLT4Ver	= DeblockLumaLt4V_sse2;
    pFunc->pfLumaDeblockingEQ4Ver	= DeblockLumaEq4V_sse2;
    pFunc->pfLumaDeblockingLT4Hor   = DeblockLumaLt4H_sse2;
    pFunc->pfLumaDeblockingEQ4Hor   = DeblockLumaEq4H_sse2;
    pFunc->pfChromaDeblockingLT4Ver	= DeblockChromaLt4V_sse2;
    pFunc->pfChromaDeblockingEQ4Ver	= DeblockChromaEq4V_sse2;
    pFunc->pfChromaDeblockingLT4Hor	= DeblockChromaLt4H_sse2;
    pFunc->pfChromaDeblockinEQ4Hor	= DeblockChromaEq4H_sse2;
  }
#endif

}

} // namespace WelsDec