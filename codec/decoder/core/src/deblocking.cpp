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
 * \file    deblocking.c
 *
 * \brief   Interfaces introduced in frame deblocking filtering
 *
 * \date    08/02/2010
 *
 *************************************************************************************
 */

#include "deblocking.h"
#include "deblocking_common.h"
#include "cpu_core.h"

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
    0,  1,  2,  3,
    12, 13, 14, 15
  },
};

static const uint8_t g_kuiTableB8x8Idx[2][16] = {
  {
    0,  1,  4,  5,  8,  9,  12, 13,   // 0   1 |  2  3
    2,  3,  6,  7, 10, 11,  14, 15    // 4   5 |  6  7
  },                                  // ------------
  // 8   9 | 10 11
  {
    // 12 13 | 14 15
    0,  1,  4,  5,  2,  3,  6,  7,
    8,  9,  12, 13, 10, 11, 14, 15
  },
};

#define TC0_TBL_LOOKUP(tc, iIndexA, pBS, bChroma) \
{\
  tc[0] = g_kiTc0Table(iIndexA)[pBS[0]] + bChroma;\
  tc[1] = g_kiTc0Table(iIndexA)[pBS[1]] + bChroma;\
  tc[2] = g_kiTc0Table(iIndexA)[pBS[2]] + bChroma;\
  tc[3] = g_kiTc0Table(iIndexA)[pBS[3]] + bChroma;\
}

void inline DeblockingBSInsideMBAvsbase (int8_t* pNnzTab, uint8_t nBS[2][4][4], int32_t iLShiftFactor) {
  uint32_t uiNnz32b0, uiNnz32b1, uiNnz32b2, uiNnz32b3;

  uiNnz32b0 = * (uint32_t*) (pNnzTab + 0);
  uiNnz32b1 = * (uint32_t*) (pNnzTab + 4);
  uiNnz32b2 = * (uint32_t*) (pNnzTab + 8);
  uiNnz32b3 = * (uint32_t*) (pNnzTab + 12);

  nBS[0][1][0] = (pNnzTab[0] | pNnzTab[1]) << iLShiftFactor;
  nBS[0][2][0] = (pNnzTab[1] | pNnzTab[2]) << iLShiftFactor;
  nBS[0][3][0] = (pNnzTab[2] | pNnzTab[3]) << iLShiftFactor;

  nBS[0][1][1] = (pNnzTab[4] | pNnzTab[5]) << iLShiftFactor;
  nBS[0][2][1] = (pNnzTab[5] | pNnzTab[6]) << iLShiftFactor;
  nBS[0][3][1] = (pNnzTab[6] | pNnzTab[7]) << iLShiftFactor;
  * (uint32_t*)nBS[1][1] = (uiNnz32b0 | uiNnz32b1) << iLShiftFactor;

  nBS[0][1][2] = (pNnzTab[8]  | pNnzTab[9])  << iLShiftFactor;
  nBS[0][2][2] = (pNnzTab[9]  | pNnzTab[10]) << iLShiftFactor;
  nBS[0][3][2] = (pNnzTab[10] | pNnzTab[11]) << iLShiftFactor;
  * (uint32_t*)nBS[1][2] = (uiNnz32b1 | uiNnz32b2) << iLShiftFactor;

  nBS[0][1][3] = (pNnzTab[12] | pNnzTab[13]) << iLShiftFactor;
  nBS[0][2][3] = (pNnzTab[13] | pNnzTab[14]) << iLShiftFactor;
  nBS[0][3][3] = (pNnzTab[14] | pNnzTab[15]) << iLShiftFactor;
  * (uint32_t*)nBS[1][3] = (uiNnz32b2 | uiNnz32b3) << iLShiftFactor;
}

void inline DeblockingBSInsideMBAvsbase8x8 (int8_t* pNnzTab, uint8_t nBS[2][4][4], int32_t iLShiftFactor) {
  int8_t i8x8NnzTab[4];
  for (int32_t i = 0; i < 4; i++) {
    int32_t iBlkIdx = i << 2;
    i8x8NnzTab[i] = (pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx]] | pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx + 1]] |
                     pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx + 2]] | pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx + 3]]);
  }

  //vertical
  nBS[0][2][0] = nBS[0][2][1] = (i8x8NnzTab[0] | i8x8NnzTab[1]) << iLShiftFactor;
  nBS[0][2][2] = nBS[0][2][3] = (i8x8NnzTab[2] | i8x8NnzTab[3]) << iLShiftFactor;
  //horizontal
  nBS[1][2][0] = nBS[1][2][1] = (i8x8NnzTab[0] | i8x8NnzTab[2]) << iLShiftFactor;
  nBS[1][2][2] = nBS[1][2][3] = (i8x8NnzTab[1] | i8x8NnzTab[3]) << iLShiftFactor;
}

void static inline DeblockingBSInsideMBNormal (PDqLayer pCurDqLayer, uint8_t nBS[2][4][4], int8_t* pNnzTab,
    int32_t iMbXy) {
  uint32_t uiNnz32b0, uiNnz32b1, uiNnz32b2, uiNnz32b3;
  int8_t* iRefIndex = pCurDqLayer->pRefIndex[LIST_0][iMbXy];
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiBsx4, 4, 4);

  int8_t i8x8NnzTab[4];

  if (pCurDqLayer->pTransformSize8x8Flag[iMbXy]) {
    for (int32_t i = 0; i < 4; i++) {
      int32_t iBlkIdx = i << 2;
      i8x8NnzTab[i] = (pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx]] | pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx + 1]] |
                       pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx + 2]] | pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx + 3]]);
    }
    //vertical
    nBS[0][2][0] = nBS[0][2][1] = BS_EDGE ((i8x8NnzTab[0] | i8x8NnzTab[1]), iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy],
                                           g_kuiMbCountScan4Idx[1 << 2], g_kuiMbCountScan4Idx[0]);
    nBS[0][2][2] = nBS[0][2][3] = BS_EDGE ((i8x8NnzTab[2] | i8x8NnzTab[3]), iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy],
                                           g_kuiMbCountScan4Idx[3 << 2], g_kuiMbCountScan4Idx[2 << 2]);

    //horizontal
    nBS[1][2][0] = nBS[1][2][1] = BS_EDGE ((i8x8NnzTab[0] | i8x8NnzTab[2]), iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy],
                                           g_kuiMbCountScan4Idx[2 << 2], g_kuiMbCountScan4Idx[0]);
    nBS[1][2][2] = nBS[1][2][3] = BS_EDGE ((i8x8NnzTab[1] | i8x8NnzTab[3]), iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy],
                                           g_kuiMbCountScan4Idx[3 << 2], g_kuiMbCountScan4Idx[1 << 2]);
  } else {
    uiNnz32b0 = * (uint32_t*) (pNnzTab + 0);
    uiNnz32b1 = * (uint32_t*) (pNnzTab + 4);
    uiNnz32b2 = * (uint32_t*) (pNnzTab + 8);
    uiNnz32b3 = * (uint32_t*) (pNnzTab + 12);

    for (int i = 0; i < 3; i++)
      uiBsx4[i] = pNnzTab[i] | pNnzTab[i + 1];
    nBS[0][1][0] = BS_EDGE (uiBsx4[0], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 1, 0);
    nBS[0][2][0] = BS_EDGE (uiBsx4[1], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 2, 1);
    nBS[0][3][0] = BS_EDGE (uiBsx4[2], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 3, 2);

    for (int i = 0; i < 3; i++)
      uiBsx4[i] = pNnzTab[4 + i] | pNnzTab[4 + i + 1];
    nBS[0][1][1] = BS_EDGE (uiBsx4[0], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 5, 4);
    nBS[0][2][1] = BS_EDGE (uiBsx4[1], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 6, 5);
    nBS[0][3][1] = BS_EDGE (uiBsx4[2], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 7, 6);

    for (int i = 0; i < 3; i++)
      uiBsx4[i] = pNnzTab[8 + i] | pNnzTab[8 + i + 1];
    nBS[0][1][2] = BS_EDGE (uiBsx4[0], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 9, 8);
    nBS[0][2][2] = BS_EDGE (uiBsx4[1], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 10, 9);
    nBS[0][3][2] = BS_EDGE (uiBsx4[2], iRefIndex, pCurDqLayer->pMv[LIST_0][iMbXy], 11, 10);

    for (int i = 0; i < 3; i++)
      uiBsx4[i] = pNnzTab[12 + i] | pNnzTab[12 + i + 1];
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
}

void static inline DeblockingBSliceBSInsideMBNormal(PDqLayer pCurDqLayer, uint8_t nBS[2][4][4], int8_t* pNnzTab,
	int32_t iMbXy) {
	uint32_t uiNnz32b0, uiNnz32b1, uiNnz32b2, uiNnz32b3;
	int8_t* iRefIndex[LIST_A];
	iRefIndex[LIST_0] = pCurDqLayer->pRefIndex[LIST_0][iMbXy];
	iRefIndex[LIST_1] = pCurDqLayer->pRefIndex[LIST_1][iMbXy];
	ENFORCE_STACK_ALIGN_1D(uint8_t, uiBsx4, 4, 4);

	int8_t i8x8NnzTab[4];
	
	if (pCurDqLayer->pTransformSize8x8Flag[iMbXy]) {
		for (int32_t i = 0; i < 4; i++) {
			int32_t iBlkIdx = i << 2;
			i8x8NnzTab[i] = (pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx]] | pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx + 1]] |
				pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx + 2]] | pNnzTab[g_kuiMbCountScan4Idx[iBlkIdx + 3]]);
		}
		//vertical
		int8_t iIndex = g_kuiMbCountScan4Idx[1 << 2];
		int8_t iNeigborIndex = g_kuiMbCountScan4Idx[0];
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][iIndex] > REF_NOT_IN_LIST && iRefIndex[listIdx][iNeigborIndex] > REF_NOT_IN_LIST) {
				nBS[0][2][0] = nBS[0][2][1] = BS_EDGE((i8x8NnzTab[0] | i8x8NnzTab[1]), iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy],
					iIndex, iNeigborIndex);
				break;
			}
		}
		iIndex = g_kuiMbCountScan4Idx[3 << 2];
		iNeigborIndex = g_kuiMbCountScan4Idx[2 << 2];
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][iIndex] > REF_NOT_IN_LIST && iRefIndex[listIdx][iNeigborIndex] > REF_NOT_IN_LIST) {
				nBS[0][2][2] = nBS[0][2][3] = BS_EDGE((i8x8NnzTab[2] | i8x8NnzTab[3]), iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy],
					iIndex, iNeigborIndex);
				break;
			}
		}
	
		//horizontal
		iIndex = g_kuiMbCountScan4Idx[2 << 2];
		iNeigborIndex = g_kuiMbCountScan4Idx[0];
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][iIndex] > REF_NOT_IN_LIST && iRefIndex[listIdx][iNeigborIndex] > REF_NOT_IN_LIST) {
				nBS[1][2][0] = nBS[1][2][1] = BS_EDGE((i8x8NnzTab[0] | i8x8NnzTab[2]), iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy],
					iIndex, iNeigborIndex);
				break;
			}
		}
		
		iIndex = g_kuiMbCountScan4Idx[3 << 2];
		iNeigborIndex = g_kuiMbCountScan4Idx[1 << 2];
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][iIndex] > REF_NOT_IN_LIST && iRefIndex[listIdx][iNeigborIndex] > REF_NOT_IN_LIST) {
				nBS[1][2][2] = nBS[1][2][3] = BS_EDGE((i8x8NnzTab[1] | i8x8NnzTab[3]), iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy],
					iIndex, iNeigborIndex);
				break;
			}
		}
	}
	else {
		uiNnz32b0 = *(uint32_t*)(pNnzTab + 0);
		uiNnz32b1 = *(uint32_t*)(pNnzTab + 4);
		uiNnz32b2 = *(uint32_t*)(pNnzTab + 8);
		uiNnz32b3 = *(uint32_t*)(pNnzTab + 12);

		for (int i = 0; i < 3; i++)
			uiBsx4[i] = pNnzTab[i] | pNnzTab[i + 1];
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][1] > REF_NOT_IN_LIST && iRefIndex[listIdx][0] > REF_NOT_IN_LIST) {
				nBS[0][1][0] = BS_EDGE(uiBsx4[0], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 1, 0);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][2] > REF_NOT_IN_LIST && iRefIndex[listIdx][1] > REF_NOT_IN_LIST) {
				nBS[0][2][0] = BS_EDGE(uiBsx4[1], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 2, 1);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][3] > REF_NOT_IN_LIST && iRefIndex[listIdx][2] > REF_NOT_IN_LIST) {
				nBS[0][3][0] = BS_EDGE(uiBsx4[2], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 3, 2);
				break;
			}
		}

		for (int i = 0; i < 3; i++)
			uiBsx4[i] = pNnzTab[4 + i] | pNnzTab[4 + i + 1];
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][5] > REF_NOT_IN_LIST && iRefIndex[listIdx][4] > REF_NOT_IN_LIST) {
				nBS[0][1][1] = BS_EDGE(uiBsx4[0], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 5, 4);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][6] > REF_NOT_IN_LIST && iRefIndex[listIdx][5] > REF_NOT_IN_LIST) {
				nBS[0][2][1] = BS_EDGE(uiBsx4[1], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 6, 5);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][7] > REF_NOT_IN_LIST && iRefIndex[listIdx][6] > REF_NOT_IN_LIST) {
				nBS[0][2][1] = BS_EDGE(uiBsx4[2], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 7, 6);
				break;
			}
		}

		for (int i = 0; i < 3; i++)
			uiBsx4[i] = pNnzTab[8 + i] | pNnzTab[8 + i + 1];
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][9] > REF_NOT_IN_LIST && iRefIndex[listIdx][8] > REF_NOT_IN_LIST) {
				nBS[0][1][2] = BS_EDGE(uiBsx4[0], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 9, 8);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][10] > REF_NOT_IN_LIST && iRefIndex[listIdx][9] > REF_NOT_IN_LIST) {
				nBS[0][2][2] = BS_EDGE(uiBsx4[1], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 10, 9);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][11] > REF_NOT_IN_LIST && iRefIndex[listIdx][10] > REF_NOT_IN_LIST) {
				nBS[0][3][2] = BS_EDGE(uiBsx4[2], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 11, 10);
				break;
			}
		}

		for (int i = 0; i < 3; i++)
			uiBsx4[i] = pNnzTab[12 + i] | pNnzTab[12 + i + 1];
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][13] > REF_NOT_IN_LIST && iRefIndex[listIdx][12] > REF_NOT_IN_LIST) {
				nBS[0][1][3] = BS_EDGE(uiBsx4[0], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 13, 12);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][14] > REF_NOT_IN_LIST && iRefIndex[listIdx][13] > REF_NOT_IN_LIST) {
				nBS[0][2][3] = BS_EDGE(uiBsx4[1], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 14, 13);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][15] > REF_NOT_IN_LIST && iRefIndex[listIdx][14] > REF_NOT_IN_LIST) {
				nBS[0][3][3] = BS_EDGE(uiBsx4[2], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 15, 14);
				break;
			}
		}

		// horizontal
		*(uint32_t*)uiBsx4 = (uiNnz32b0 | uiNnz32b1);
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][4] > REF_NOT_IN_LIST && iRefIndex[listIdx][0] > REF_NOT_IN_LIST) {
				nBS[1][1][0] = BS_EDGE(uiBsx4[0], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 4, 0);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][5] > REF_NOT_IN_LIST && iRefIndex[listIdx][1] > REF_NOT_IN_LIST) {
				nBS[1][1][1] = BS_EDGE(uiBsx4[1], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 5, 1);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][6] > REF_NOT_IN_LIST && iRefIndex[listIdx][2] > REF_NOT_IN_LIST) {
				nBS[1][1][2] = BS_EDGE(uiBsx4[2], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 6, 2);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][7] > REF_NOT_IN_LIST && iRefIndex[listIdx][3] > REF_NOT_IN_LIST) {
				nBS[1][1][3] = BS_EDGE(uiBsx4[3], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 7, 3);
				break;
			}
		}

		*(uint32_t*)uiBsx4 = (uiNnz32b1 | uiNnz32b2);
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][8] > REF_NOT_IN_LIST && iRefIndex[listIdx][4] > REF_NOT_IN_LIST) {
				nBS[1][2][0] = BS_EDGE(uiBsx4[0], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 8, 4);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][9] > REF_NOT_IN_LIST && iRefIndex[listIdx][5] > REF_NOT_IN_LIST) {
				nBS[1][2][1] = BS_EDGE(uiBsx4[1], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 9, 5);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][10] > REF_NOT_IN_LIST && iRefIndex[listIdx][6] > REF_NOT_IN_LIST) {
				nBS[1][2][2] = BS_EDGE(uiBsx4[2], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 10, 6);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][11] > REF_NOT_IN_LIST && iRefIndex[listIdx][7] > REF_NOT_IN_LIST) {
				nBS[1][2][3] = BS_EDGE(uiBsx4[3], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 11, 7);
				break;
			}
		}

		*(uint32_t*)uiBsx4 = (uiNnz32b2 | uiNnz32b3);
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][12] > REF_NOT_IN_LIST && iRefIndex[listIdx][8] > REF_NOT_IN_LIST) {
				nBS[1][3][0] = BS_EDGE(uiBsx4[0], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 12, 8);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][13] > REF_NOT_IN_LIST && iRefIndex[listIdx][9] > REF_NOT_IN_LIST) {
				nBS[1][3][1] = BS_EDGE(uiBsx4[1], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 13, 9);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][14] > REF_NOT_IN_LIST && iRefIndex[listIdx][10] > REF_NOT_IN_LIST) {
				nBS[1][3][2] = BS_EDGE(uiBsx4[2], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 14, 10);
				break;
			}
		}
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			if (iRefIndex[listIdx][15] > REF_NOT_IN_LIST && iRefIndex[listIdx][11] > REF_NOT_IN_LIST) {
				nBS[1][3][3] = BS_EDGE(uiBsx4[3], iRefIndex[listIdx], pCurDqLayer->pMv[listIdx][iMbXy], 15, 11);
				break;
			}
		}
	}
}


uint32_t DeblockingBsMarginalMBAvcbase (PDqLayer pCurDqLayer, int32_t iEdge, int32_t iNeighMb, int32_t iMbXy) {
  int32_t i, j;
  uint32_t uiBSx4;
  uint8_t* pBS = (uint8_t*) (&uiBSx4);
  const uint8_t* pBIdx      = &g_kuiTableBIdx[iEdge][0];
  const uint8_t* pBnIdx     = &g_kuiTableBIdx[iEdge][4];
  const uint8_t* pB8x8Idx   = &g_kuiTableB8x8Idx[iEdge][0];
  const uint8_t* pBn8x8Idx  = &g_kuiTableB8x8Idx[iEdge][8];

  if (pCurDqLayer->pTransformSize8x8Flag[iMbXy] && pCurDqLayer->pTransformSize8x8Flag[iNeighMb]) {
    for (i = 0; i < 2; i++) {
      uint8_t uiNzc = 0;
      for (j = 0; uiNzc == 0 && j < 4; j++) {
        uiNzc |= (pCurDqLayer->pNzc[iMbXy][* (pB8x8Idx + j)] | pCurDqLayer->pNzc[iNeighMb][* (pBn8x8Idx + j)]);
      }
      if (uiNzc) {
        pBS[i << 1] = pBS[1 + (i << 1)] = 2;
      } else {
        pBS[i << 1] = pBS[1 + (i << 1)] = MB_BS_MV (pCurDqLayer->pRefIndex[LIST_0], pCurDqLayer->pMv[LIST_0], iMbXy, iNeighMb,
                                          *pB8x8Idx, *pBn8x8Idx);
      }
      pB8x8Idx += 4;
      pBn8x8Idx += 4;
    }
  } else if (pCurDqLayer->pTransformSize8x8Flag[iMbXy]) {
    for (i = 0; i < 2; i++) {
      uint8_t uiNzc = 0;
      for (j = 0; uiNzc == 0 && j < 4; j++) {
        uiNzc |= pCurDqLayer->pNzc[iMbXy][* (pB8x8Idx + j)];
      }
      for (j = 0; j < 2; j++) {
        if (uiNzc | pCurDqLayer->pNzc[iNeighMb][*pBnIdx]) {
          pBS[j + (i << 1)] = 2;
        } else {
          pBS[j + (i << 1)] = MB_BS_MV (pCurDqLayer->pRefIndex[LIST_0], pCurDqLayer->pMv[LIST_0], iMbXy, iNeighMb, *pB8x8Idx,
                                        *pBnIdx);
        }
        pBnIdx++;
      }
      pB8x8Idx += 4;
    }
  } else if (pCurDqLayer->pTransformSize8x8Flag[iNeighMb]) {
    for (i = 0; i < 2; i++) {
      uint8_t uiNzc = 0;
      for (j = 0; uiNzc == 0 && j < 4; j++) {
        uiNzc |= pCurDqLayer->pNzc[iNeighMb][* (pBn8x8Idx + j)];
      }
      for (j = 0; j < 2; j++) {
        if (uiNzc | pCurDqLayer->pNzc[iMbXy][*pBIdx]) {
          pBS[j + (i << 1)] = 2;
        } else {
          pBS[j + (i << 1)] = MB_BS_MV (pCurDqLayer->pRefIndex[LIST_0], pCurDqLayer->pMv[LIST_0], iMbXy, iNeighMb, *pBIdx,
                                        *pBn8x8Idx);
        }
        pBIdx++;
      }
      pBn8x8Idx += 4;
    }
  } else {
    // only 4x4 transform
    for (i = 0; i < 4; i++) {
      if (pCurDqLayer->pNzc[iMbXy][*pBIdx] | pCurDqLayer->pNzc[iNeighMb][*pBnIdx]) {
        pBS[i] = 2;
      } else {
        pBS[i] = MB_BS_MV (pCurDqLayer->pRefIndex[LIST_0], pCurDqLayer->pMv[LIST_0], iMbXy, iNeighMb, *pBIdx,
                           *pBnIdx);
      }
      pBIdx++;
      pBnIdx++;
    }
  }

  return uiBSx4;
}
uint32_t DeblockingBSliceBsMarginalMBAvcbase(PDqLayer pCurDqLayer, int32_t iEdge, int32_t iNeighMb, int32_t iMbXy) {
	int32_t i, j;
	uint32_t uiBSx4;
	uint8_t* pBS = (uint8_t*)(&uiBSx4);
	const uint8_t* pBIdx = &g_kuiTableBIdx[iEdge][0];
	const uint8_t* pBnIdx = &g_kuiTableBIdx[iEdge][4];
	const uint8_t* pB8x8Idx = &g_kuiTableB8x8Idx[iEdge][0];
	const uint8_t* pBn8x8Idx = &g_kuiTableB8x8Idx[iEdge][8];

	if (pCurDqLayer->pTransformSize8x8Flag[iMbXy] && pCurDqLayer->pTransformSize8x8Flag[iNeighMb]) {
		for (i = 0; i < 2; i++) {
			uint8_t uiNzc = 0;
			for (j = 0; uiNzc == 0 && j < 4; j++) {
				uiNzc |= (pCurDqLayer->pNzc[iMbXy][*(pB8x8Idx + j)] | pCurDqLayer->pNzc[iNeighMb][*(pBn8x8Idx + j)]);
			}
			if (uiNzc) {
				pBS[i << 1] = pBS[1 + (i << 1)] = 2;
			}
			else {
				for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
					if (pCurDqLayer->pRefIndex[listIdx][iMbXy][*pB8x8Idx] > REF_NOT_IN_LIST && pCurDqLayer->pRefIndex[listIdx][iMbXy][*pBn8x8Idx] > REF_NOT_IN_LIST) {
						pBS[i << 1] = pBS[1 + (i << 1)] = MB_BS_MV(pCurDqLayer->pRefIndex[listIdx], pCurDqLayer->pMv[listIdx], iMbXy, iNeighMb, *pB8x8Idx, *pBn8x8Idx);
						break;
					}
				}
			}
			pB8x8Idx += 4;
			pBn8x8Idx += 4;
		}
	}
	else if (pCurDqLayer->pTransformSize8x8Flag[iMbXy]) {
		for (i = 0; i < 2; i++) {
			uint8_t uiNzc = 0;
			for (j = 0; uiNzc == 0 && j < 4; j++) {
				uiNzc |= pCurDqLayer->pNzc[iMbXy][*(pB8x8Idx + j)];
			}
			for (j = 0; j < 2; j++) {
				if (uiNzc | pCurDqLayer->pNzc[iNeighMb][*pBnIdx]) {
					pBS[j + (i << 1)] = 2;
				}
				else {
					for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
						if (pCurDqLayer->pRefIndex[listIdx][iMbXy][*pB8x8Idx] > REF_NOT_IN_LIST && pCurDqLayer->pRefIndex[listIdx][iMbXy][*pBnIdx] > REF_NOT_IN_LIST) {
							pBS[j + (i << 1)] = MB_BS_MV(pCurDqLayer->pRefIndex[listIdx], pCurDqLayer->pMv[listIdx], iMbXy, iNeighMb, *pB8x8Idx, *pBnIdx);
							break;
						}
					}
				}
				pBnIdx++;
			}
			pB8x8Idx += 4;
		}
	}
	else if (pCurDqLayer->pTransformSize8x8Flag[iNeighMb]) {
		for (i = 0; i < 2; i++) {
			uint8_t uiNzc = 0;
			for (j = 0; uiNzc == 0 && j < 4; j++) {
				uiNzc |= pCurDqLayer->pNzc[iNeighMb][*(pBn8x8Idx + j)];
			}
			for (j = 0; j < 2; j++) {
				if (uiNzc | pCurDqLayer->pNzc[iMbXy][*pBIdx]) {
					pBS[j + (i << 1)] = 2;
				}
				else {
					for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
						if (pCurDqLayer->pRefIndex[listIdx][iMbXy][*pBIdx] > REF_NOT_IN_LIST && pCurDqLayer->pRefIndex[listIdx][iMbXy][*pBn8x8Idx] > REF_NOT_IN_LIST) {
							pBS[j + (i << 1)] = MB_BS_MV(pCurDqLayer->pRefIndex[listIdx], pCurDqLayer->pMv[listIdx], iMbXy, iNeighMb, *pBIdx, *pBn8x8Idx);
							break;
						}
					}
				}
				pBIdx++;
			}
			pBn8x8Idx += 4;
		}
	}
	else {
		// only 4x4 transform
		for (i = 0; i < 4; i++) {
			if (pCurDqLayer->pNzc[iMbXy][*pBIdx] | pCurDqLayer->pNzc[iNeighMb][*pBnIdx]) {
				pBS[i] = 2;
			}
			else {
				for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
					if (pCurDqLayer->pRefIndex[listIdx][iMbXy][*pBIdx] > REF_NOT_IN_LIST && pCurDqLayer->pRefIndex[listIdx][iMbXy][*pBnIdx] > REF_NOT_IN_LIST) {
						pBS[i] = MB_BS_MV(pCurDqLayer->pRefIndex[listIdx], pCurDqLayer->pMv[listIdx], iMbXy, iNeighMb, *pBIdx, *pBnIdx);
						break;
					}
				}
			}
			pBIdx++;
			pBnIdx++;
		}
	}

	return uiBSx4;
}
int32_t DeblockingAvailableNoInterlayer (PDqLayer pCurDqLayer, int32_t iFilterIdc) {
  int32_t iMbY = pCurDqLayer->iMbY;
  int32_t iMbX = pCurDqLayer->iMbX;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  bool bLeftFlag = false;
  bool bTopFlag  = false;

  if (2 == iFilterIdc) {
    bLeftFlag = (iMbX > 0) && (pCurDqLayer->pSliceIdc[iMbXy] == pCurDqLayer->pSliceIdc[iMbXy - 1]);
    bTopFlag  = (iMbY > 0) && (pCurDqLayer->pSliceIdc[iMbXy] == pCurDqLayer->pSliceIdc[iMbXy - pCurDqLayer->iMbWidth]);
  } else { //if ( 0 == iFilterIdc )
    bLeftFlag = (iMbX > 0);
    bTopFlag  = (iMbY > 0);
  }
  return (bLeftFlag << LEFT_FLAG_BIT) | (bTopFlag << TOP_FLAG_BIT);
}

void FilteringEdgeLumaH (SDeblockingFilter* pFilter, uint8_t* pPix, int32_t iStride, uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;
  ENFORCE_STACK_ALIGN_1D (int8_t, tc, 4, 16);

  GET_ALPHA_BETA_FROM_QP (pFilter->iLumaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);

  if (iAlpha | iBeta) {
    TC0_TBL_LOOKUP (tc, iIndexA, pBS, 0);
    pFilter->pLoopf->pfLumaDeblockingLT4Ver (pPix, iStride, iAlpha, iBeta, tc);
  }
  return;
}


void FilteringEdgeLumaV (SDeblockingFilter* pFilter, uint8_t* pPix, int32_t iStride, uint8_t* pBS) {
  int32_t  iIndexA;
  int32_t  iAlpha;
  int32_t  iBeta;
  ENFORCE_STACK_ALIGN_1D (int8_t, tc, 4, 16);

  GET_ALPHA_BETA_FROM_QP (pFilter->iLumaQP, pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                          iBeta);

  if (iAlpha | iBeta) {
    TC0_TBL_LOOKUP (tc, iIndexA, pBS, 0);
    pFilter->pLoopf->pfLumaDeblockingLT4Hor (pPix, iStride, iAlpha, iBeta, tc);
  }
  return;
}


void FilteringEdgeLumaIntraH (SDeblockingFilter* pFilter, uint8_t* pPix, int32_t iStride, uint8_t* pBS) {
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

void FilteringEdgeLumaIntraV (SDeblockingFilter* pFilter, uint8_t* pPix, int32_t iStride, uint8_t* pBS) {
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
void FilteringEdgeChromaH (SDeblockingFilter* pFilter, uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride,
                           uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;
  ENFORCE_STACK_ALIGN_1D (int8_t, tc, 4, 16);
  if (pFilter->iChromaQP[0] == pFilter->iChromaQP[1]) {

    GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[0], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                            iBeta);

    if (iAlpha | iBeta) {
      TC0_TBL_LOOKUP (tc, iIndexA, pBS, 1);
      pFilter->pLoopf->pfChromaDeblockingLT4Ver (pPixCb, pPixCr, iStride, iAlpha, iBeta, tc);
    }
  } else {

    for (int i = 0; i < 2; i++) {


      GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[i], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                              iBeta);

      if (iAlpha | iBeta) {
        uint8_t* pPixCbCr = (i == 0) ? pPixCb : pPixCr;
        TC0_TBL_LOOKUP (tc, iIndexA, pBS, 1);
        pFilter->pLoopf->pfChromaDeblockingLT4Ver2 (pPixCbCr, iStride, iAlpha, iBeta, tc);
      }



    }

  }
  return;
}
void FilteringEdgeChromaV (SDeblockingFilter* pFilter, uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride,
                           uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;
  ENFORCE_STACK_ALIGN_1D (int8_t, tc, 4, 16);
  if (pFilter->iChromaQP[0] == pFilter->iChromaQP[1]) {


    GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[0], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                            iBeta);

    if (iAlpha | iBeta) {
      TC0_TBL_LOOKUP (tc, iIndexA, pBS, 1);
      pFilter->pLoopf->pfChromaDeblockingLT4Hor (pPixCb, pPixCr, iStride, iAlpha, iBeta, tc);
    }


  } else {

    for (int i = 0; i < 2; i++) {

      GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[i], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                              iBeta);

      if (iAlpha | iBeta) {
        uint8_t* pPixCbCr = (i == 0) ? pPixCb : pPixCr;
        TC0_TBL_LOOKUP (tc, iIndexA, pBS, 1);
        pFilter->pLoopf->pfChromaDeblockingLT4Hor2 (pPixCbCr, iStride, iAlpha, iBeta, tc);
      }


    }
  }
  return;
}

void FilteringEdgeChromaIntraH (SDeblockingFilter* pFilter, uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride,
                                uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;
  if (pFilter->iChromaQP[0] == pFilter->iChromaQP[1]) {

    GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[0], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                            iBeta);

    if (iAlpha | iBeta) {
      pFilter->pLoopf->pfChromaDeblockingEQ4Ver (pPixCb, pPixCr, iStride, iAlpha, iBeta);
    }
  } else {

    for (int i = 0; i < 2; i++) {

      GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[i], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                              iBeta);

      if (iAlpha | iBeta) {
        uint8_t* pPixCbCr = (i == 0) ? pPixCb : pPixCr;
        pFilter->pLoopf->pfChromaDeblockingEQ4Ver2 (pPixCbCr, iStride, iAlpha, iBeta);
      }

    }
  }
  return;
}

void FilteringEdgeChromaIntraV (SDeblockingFilter* pFilter, uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride,
                                uint8_t* pBS) {
  int32_t iIndexA;
  int32_t iAlpha;
  int32_t iBeta;
  if (pFilter->iChromaQP[0] == pFilter->iChromaQP[1]) { // QP of cb and cr are the same




    GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[0], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                            iBeta);
    if (iAlpha | iBeta) {
      pFilter->pLoopf->pfChromaDeblockingEQ4Hor (pPixCb, pPixCr, iStride, iAlpha, iBeta);
    }
  } else {

    for (int i = 0; i < 2; i++) {


      GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[i], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                              iBeta);
      if (iAlpha | iBeta) {
        uint8_t* pPixCbCr = (i == 0) ? pPixCb : pPixCr;
        pFilter->pLoopf->pfChromaDeblockingEQ4Hor2 (pPixCbCr, iStride, iAlpha, iBeta);
      }
    }

  }
  return;
}


void DeblockingInterMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, uint8_t nBS[2][4][4],
                        int32_t iBoundryFlag) {
  int32_t iMbXyIndex = pCurDqLayer->iMbXyIndex;
  int32_t iMbX = pCurDqLayer->iMbX;
  int32_t iMbY = pCurDqLayer->iMbY;

  int32_t iCurLumaQp = pCurDqLayer->pLumaQp[iMbXyIndex];
  //int32_t* iCurChromaQp = pCurDqLayer->pChromaQp[iMbXyIndex];
  int8_t* pCurChromaQp = pCurDqLayer->pChromaQp[iMbXyIndex];
  int32_t iLineSize   = pFilter->iCsStride[0];
  int32_t iLineSizeUV = pFilter->iCsStride[1];

  uint8_t* pDestY, * pDestCb, * pDestCr;
  pDestY  = pFilter->pCsData[0] + ((iMbY * iLineSize + iMbX) << 4);
  pDestCb = pFilter->pCsData[1] + ((iMbY * iLineSizeUV + iMbX) << 3);
  pDestCr = pFilter->pCsData[2] + ((iMbY * iLineSizeUV + iMbX) << 3);

//Vertical margrin
  if (iBoundryFlag & LEFT_FLAG_MASK) {
    int32_t iLeftXyIndex = iMbXyIndex - 1;
    pFilter->iLumaQP   = (iCurLumaQp + pCurDqLayer->pLumaQp[iLeftXyIndex] + 1) >> 1;
    for (int i = 0; i < 2; i++) {
      pFilter->iChromaQP[i] = (pCurChromaQp[i] + pCurDqLayer->pChromaQp[iLeftXyIndex][i] + 1) >> 1;
    }
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
  pFilter->iChromaQP[0] = pCurChromaQp[0];
  pFilter->iChromaQP[1] = pCurChromaQp[1];

  if (* (uint32_t*)nBS[0][1] != 0 && !pCurDqLayer->pTransformSize8x8Flag[iMbXyIndex]) {
    FilteringEdgeLumaV (pFilter, &pDestY[1 << 2], iLineSize, nBS[0][1]);
  }

  if (* (uint32_t*)nBS[0][2] != 0) {
    FilteringEdgeLumaV (pFilter, &pDestY[2 << 2], iLineSize, nBS[0][2]);
    FilteringEdgeChromaV (pFilter, &pDestCb[2 << 1], &pDestCr[2 << 1], iLineSizeUV, nBS[0][2]);
  }

  if (* (uint32_t*)nBS[0][3] != 0  && !pCurDqLayer->pTransformSize8x8Flag[iMbXyIndex]) {
    FilteringEdgeLumaV (pFilter, &pDestY[3 << 2], iLineSize, nBS[0][3]);
  }

  if (iBoundryFlag & TOP_FLAG_MASK) {
    int32_t iTopXyIndex = iMbXyIndex - pCurDqLayer->iMbWidth;
    pFilter->iLumaQP = (iCurLumaQp + pCurDqLayer->pLumaQp[iTopXyIndex] + 1) >> 1;
    for (int i = 0; i < 2; i++) {
      pFilter->iChromaQP[i] = (pCurChromaQp[i] + pCurDqLayer->pChromaQp[iTopXyIndex][i] + 1) >> 1;
    }

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
  pFilter->iChromaQP[0] = pCurChromaQp[0];
  pFilter->iChromaQP[1] = pCurChromaQp[1];

  if (* (uint32_t*)nBS[1][1] != 0  && !pCurDqLayer->pTransformSize8x8Flag[iMbXyIndex]) {
    FilteringEdgeLumaH (pFilter, &pDestY[ (1 << 2)*iLineSize], iLineSize, nBS[1][1]);
  }

  if (* (uint32_t*)nBS[1][2] != 0) {
    FilteringEdgeLumaH (pFilter, &pDestY[ (2 << 2)*iLineSize], iLineSize, nBS[1][2]);
    FilteringEdgeChromaH (pFilter, &pDestCb[ (2 << 1)*iLineSizeUV], &pDestCr[ (2 << 1)*iLineSizeUV], iLineSizeUV,
                          nBS[1][2]);
  }

  if (* (uint32_t*)nBS[1][3] != 0  && !pCurDqLayer->pTransformSize8x8Flag[iMbXyIndex]) {
    FilteringEdgeLumaH (pFilter, &pDestY[ (3 << 2)*iLineSize], iLineSize, nBS[1][3]);
  }
}

void FilteringEdgeLumaHV (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag) {
  int32_t iMbXyIndex = pCurDqLayer->iMbXyIndex;
  int32_t iMbX      = pCurDqLayer->iMbX;
  int32_t iMbY      = pCurDqLayer->iMbY;
  int32_t iMbWidth  = pCurDqLayer->iMbWidth;
  int32_t iLineSize  = pFilter->iCsStride[0];

  uint8_t*  pDestY;
  int32_t  iCurQp;
  int32_t  iIndexA, iAlpha, iBeta;

  ENFORCE_STACK_ALIGN_1D (int8_t,  iTc,   4, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiBSx4, 4, 4);

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

    if (!pCurDqLayer->pTransformSize8x8Flag[iMbXyIndex]) {
      pFilter->pLoopf->pfLumaDeblockingLT4Hor (&pDestY[1 << 2], iLineSize, iAlpha, iBeta, iTc);
    }

    pFilter->pLoopf->pfLumaDeblockingLT4Hor (&pDestY[2 << 2], iLineSize, iAlpha, iBeta, iTc);

    if (!pCurDqLayer->pTransformSize8x8Flag[iMbXyIndex]) {
      pFilter->pLoopf->pfLumaDeblockingLT4Hor (&pDestY[3 << 2], iLineSize, iAlpha, iBeta, iTc);
    }
  }

  // luma h
  if (iBoundryFlag & TOP_FLAG_MASK) {
    pFilter->iLumaQP   = (iCurQp   + pCurDqLayer->pLumaQp[iMbXyIndex - iMbWidth] + 1) >> 1;
    FilteringEdgeLumaIntraH (pFilter, pDestY, iLineSize, NULL);
  }

  pFilter->iLumaQP   = iCurQp;
  if (iAlpha | iBeta) {
    if (!pCurDqLayer->pTransformSize8x8Flag[iMbXyIndex]) {
      pFilter->pLoopf->pfLumaDeblockingLT4Ver (&pDestY[ (1 << 2)*iLineSize], iLineSize, iAlpha, iBeta, iTc);
    }

    pFilter->pLoopf->pfLumaDeblockingLT4Ver (&pDestY[ (2 << 2)*iLineSize], iLineSize, iAlpha, iBeta, iTc);

    if (!pCurDqLayer->pTransformSize8x8Flag[iMbXyIndex]) {
      pFilter->pLoopf->pfLumaDeblockingLT4Ver (&pDestY[ (3 << 2)*iLineSize], iLineSize, iAlpha, iBeta, iTc);
    }
  }
}
void FilteringEdgeChromaHV (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag) {
  int32_t iMbXyIndex     = pCurDqLayer->iMbXyIndex;
  int32_t iMbX      = pCurDqLayer->iMbX;
  int32_t iMbY      = pCurDqLayer->iMbY;
  int32_t iMbWidth  = pCurDqLayer->iMbWidth;
  int32_t iLineSize  = pFilter->iCsStride[1];

  uint8_t* pDestCb;
  uint8_t* pDestCr;
  //int32_t  iCurQp;
  int8_t* pCurQp;
  int32_t  iIndexA, iAlpha, iBeta;

  ENFORCE_STACK_ALIGN_1D (int8_t,  iTc,   4, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, uiBSx4, 4, 4);

  pDestCb = pFilter->pCsData[1] + ((iMbY * iLineSize + iMbX) << 3);
  pDestCr = pFilter->pCsData[2] + ((iMbY * iLineSize + iMbX) << 3);
  pCurQp  = pCurDqLayer->pChromaQp[iMbXyIndex];

  * (uint32_t*)uiBSx4 = 0x03030303;


// chroma v
  if (iBoundryFlag & LEFT_FLAG_MASK) {

    for (int i = 0; i < 2; i++) {
      pFilter->iChromaQP[i] = (pCurQp[i] + pCurDqLayer->pChromaQp[iMbXyIndex - 1][i] + 1) >> 1;

    }
    FilteringEdgeChromaIntraV (pFilter, pDestCb, pDestCr, iLineSize, NULL);
  }

  pFilter->iChromaQP[0]   = pCurQp[0];
  pFilter->iChromaQP[1]   = pCurQp[1];
  if (pFilter->iChromaQP[0] == pFilter->iChromaQP[1]) {
    GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[0], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                            iBeta);
    if (iAlpha | iBeta) {
      TC0_TBL_LOOKUP (iTc, iIndexA, uiBSx4, 1);
      pFilter->pLoopf->pfChromaDeblockingLT4Hor (&pDestCb[2 << 1], &pDestCr[2 << 1], iLineSize, iAlpha, iBeta, iTc);
    }
  } else {

    for (int i = 0; i < 2; i++) {
      GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[i], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                              iBeta);
      if (iAlpha | iBeta) {
        uint8_t* pDestCbCr = (i == 0) ? &pDestCb[2 << 1] : &pDestCr[2 << 1];
        TC0_TBL_LOOKUP (iTc, iIndexA, uiBSx4, 1);
        pFilter->pLoopf->pfChromaDeblockingLT4Hor2 (pDestCbCr, iLineSize, iAlpha, iBeta, iTc);
      }

    }
  }

  // chroma h

  if (iBoundryFlag & TOP_FLAG_MASK) {
    for (int i = 0; i < 2; i++) {
      pFilter->iChromaQP[i] = (pCurQp[i] + pCurDqLayer->pChromaQp[iMbXyIndex - iMbWidth][i] + 1) >> 1;
    }
    FilteringEdgeChromaIntraH (pFilter, pDestCb, pDestCr, iLineSize, NULL);
  }

  pFilter->iChromaQP[0]   = pCurQp[0];
  pFilter->iChromaQP[1]   = pCurQp[1];

  if (pFilter->iChromaQP[0] == pFilter->iChromaQP[1]) {
    GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[0], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                            iBeta);
    if (iAlpha | iBeta) {
      TC0_TBL_LOOKUP (iTc, iIndexA, uiBSx4, 1);
      pFilter->pLoopf->pfChromaDeblockingLT4Ver (&pDestCb[ (2 << 1)*iLineSize], &pDestCr[ (2 << 1)*iLineSize], iLineSize,
          iAlpha, iBeta, iTc);
    }
  } else {
    for (int i = 0; i < 2; i++) {

      GET_ALPHA_BETA_FROM_QP (pFilter->iChromaQP[i], pFilter->iSliceAlphaC0Offset, pFilter->iSliceBetaOffset, iIndexA, iAlpha,
                              iBeta);
      if (iAlpha | iBeta) {
        TC0_TBL_LOOKUP (iTc, iIndexA, uiBSx4, 1);
        uint8_t* pDestCbCr = (i == 0) ? &pDestCb[ (2 << 1) * iLineSize] : &pDestCr[ (2 << 1) * iLineSize];
        pFilter->pLoopf->pfChromaDeblockingLT4Ver2 (pDestCbCr, iLineSize,
            iAlpha, iBeta, iTc);
      }
    }


  }
}

// merge h&v lookup table operation to save performance
void DeblockingIntraMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag) {
  FilteringEdgeLumaHV (pCurDqLayer, pFilter, iBoundryFlag);
  FilteringEdgeChromaHV (pCurDqLayer, pFilter, iBoundryFlag);
}

void WelsDeblockingMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag) {
  uint8_t nBS[2][4][4] = {{{ 0 }}};

  int32_t iMbXyIndex  = pCurDqLayer->iMbXyIndex;
  uint32_t iCurMbType  = pCurDqLayer->pMbType[iMbXyIndex];
  int32_t iMbNb;

	PSlice pSlice = &pCurDqLayer->sLayerInfo.sSliceInLayer;
	PSliceHeader pSliceHeader = &pSlice->sSliceHeaderExt.sSliceHeader;
	bool bBSlice = pSliceHeader->eSliceType == B_SLICE;

  switch (iCurMbType) {
  case MB_TYPE_INTRA4x4:
  case MB_TYPE_INTRA8x8:
  case MB_TYPE_INTRA16x16:
  case MB_TYPE_INTRA_PCM:
    DeblockingIntraMb (pCurDqLayer, pFilter, iBoundryFlag);
    break;
  default:

    if (iBoundryFlag & LEFT_FLAG_MASK) {
      iMbNb = iMbXyIndex - 1;
			if (bBSlice) {
				*(uint32_t*)nBS[0][0] = IS_INTRA(pCurDqLayer->pMbType[iMbNb]) ? 0x04040404 : DeblockingBSliceBsMarginalMBAvcbase(
					pCurDqLayer, 0, iMbNb, iMbXyIndex);
			}
			else {
				*(uint32_t*)nBS[0][0] = IS_INTRA(pCurDqLayer->pMbType[iMbNb]) ? 0x04040404 : DeblockingBsMarginalMBAvcbase(
					pCurDqLayer, 0, iMbNb, iMbXyIndex);
			}
    } else {
      * (uint32_t*)nBS[0][0] = 0;
    }
    if (iBoundryFlag & TOP_FLAG_MASK) {
      iMbNb = iMbXyIndex - pCurDqLayer->iMbWidth;
			if (bBSlice) {
				*(uint32_t*)nBS[1][0] = IS_INTRA(pCurDqLayer->pMbType[iMbNb]) ? 0x04040404 : DeblockingBSliceBsMarginalMBAvcbase(
					pCurDqLayer, 1, iMbNb, iMbXyIndex);
			}
			else {
				*(uint32_t*)nBS[1][0] = IS_INTRA(pCurDqLayer->pMbType[iMbNb]) ? 0x04040404 : DeblockingBsMarginalMBAvcbase(
					pCurDqLayer, 1, iMbNb, iMbXyIndex);
			}
    } else {
      * (uint32_t*)nBS[1][0] = 0;
    }
    //SKIP MB_16x16 or others
    if (!IS_SKIP(iCurMbType)) {
      if (IS_INTER_16x16(iCurMbType)) {
        if (!pCurDqLayer->pTransformSize8x8Flag[pCurDqLayer->iMbXyIndex]) {
          DeblockingBSInsideMBAvsbase (pCurDqLayer->pNzc[iMbXyIndex], nBS, 1);
        } else {
          DeblockingBSInsideMBAvsbase8x8 (pCurDqLayer->pNzc[iMbXyIndex], nBS, 1);
        }
      } else {

				if (bBSlice) {
					DeblockingBSliceBSInsideMBNormal(pCurDqLayer, nBS, pCurDqLayer->pNzc[iMbXyIndex], iMbXyIndex);
				}
				else {
					DeblockingBSInsideMBNormal(pCurDqLayer, nBS, pCurDqLayer->pNzc[iMbXyIndex], iMbXyIndex);
				}
      }
    } else {
      * (uint32_t*)nBS[0][1] = * (uint32_t*)nBS[0][2] = * (uint32_t*)nBS[0][3] =
                                 * (uint32_t*)nBS[1][1] = * (uint32_t*)nBS[1][2] = * (uint32_t*)nBS[1][3] = 0;
    }
    DeblockingInterMb (pCurDqLayer, pFilter, nBS, iBoundryFlag);
    break;
  }
}

/*!
 * \brief   AVC slice deblocking filtering target layer
 *
 * \param   dec         Wels avc decoder context
 *
 * \return  NONE
 */
void WelsDeblockingFilterSlice (PWelsDecoderContext pCtx, PDeblockingFilterMbFunc pDeblockMb) {
  PDqLayer pCurDqLayer = pCtx->pCurDqLayer;
  PSliceHeaderExt pSliceHeaderExt = &pCurDqLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt;
  int32_t iMbWidth  = pCurDqLayer->iMbWidth;
  int32_t iTotalMbCount = pSliceHeaderExt->sSliceHeader.pSps->uiTotalMbCount;

  SDeblockingFilter pFilter;
  memset (&pFilter, 0, sizeof (pFilter));
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

  pFilter.eSliceType = (EWelsSliceType) pCurDqLayer->sLayerInfo.sSliceInLayer.eSliceType;

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
      if (-1 == iNextMbXyIndex || iNextMbXyIndex >= iTotalMbCount) { // slice group boundary or end of a frame
        break;
      }

      pCurDqLayer->iMbX  = iNextMbXyIndex % iMbWidth;
      pCurDqLayer->iMbY  = iNextMbXyIndex / iMbWidth;
      pCurDqLayer->iMbXyIndex = iNextMbXyIndex;
    } while (1);
  }
}
/*!
 * \brief   deblocking module initialize
 *
 * \param   pf
 *          cpu
 *
 * \return  NONE
 */

void  DeblockingInit (SDeblockingFunc*  pFunc,  int32_t iCpu) {
  pFunc->pfLumaDeblockingLT4Ver     = DeblockLumaLt4V_c;
  pFunc->pfLumaDeblockingEQ4Ver     = DeblockLumaEq4V_c;
  pFunc->pfLumaDeblockingLT4Hor     = DeblockLumaLt4H_c;
  pFunc->pfLumaDeblockingEQ4Hor     = DeblockLumaEq4H_c;

  pFunc->pfChromaDeblockingLT4Ver   = DeblockChromaLt4V_c;
  pFunc->pfChromaDeblockingEQ4Ver   = DeblockChromaEq4V_c;
  pFunc->pfChromaDeblockingLT4Hor   = DeblockChromaLt4H_c;
  pFunc->pfChromaDeblockingEQ4Hor   = DeblockChromaEq4H_c;

  pFunc->pfChromaDeblockingLT4Ver2  = DeblockChromaLt4V2_c;
  pFunc->pfChromaDeblockingEQ4Ver2  = DeblockChromaEq4V2_c;
  pFunc->pfChromaDeblockingLT4Hor2  = DeblockChromaLt4H2_c;
  pFunc->pfChromaDeblockingEQ4Hor2  = DeblockChromaEq4H2_c;

#ifdef X86_ASM
  if (iCpu & WELS_CPU_SSSE3) {
    pFunc->pfLumaDeblockingLT4Ver   = DeblockLumaLt4V_ssse3;
    pFunc->pfLumaDeblockingEQ4Ver   = DeblockLumaEq4V_ssse3;
    pFunc->pfLumaDeblockingLT4Hor   = DeblockLumaLt4H_ssse3;
    pFunc->pfLumaDeblockingEQ4Hor   = DeblockLumaEq4H_ssse3;
    pFunc->pfChromaDeblockingLT4Ver = DeblockChromaLt4V_ssse3;
    pFunc->pfChromaDeblockingEQ4Ver = DeblockChromaEq4V_ssse3;
    pFunc->pfChromaDeblockingLT4Hor = DeblockChromaLt4H_ssse3;
    pFunc->pfChromaDeblockingEQ4Hor = DeblockChromaEq4H_ssse3;
  }
#endif

#if defined(HAVE_NEON)
  if (iCpu & WELS_CPU_NEON) {
    pFunc->pfLumaDeblockingLT4Ver   = DeblockLumaLt4V_neon;
    pFunc->pfLumaDeblockingEQ4Ver   = DeblockLumaEq4V_neon;
    pFunc->pfLumaDeblockingLT4Hor   = DeblockLumaLt4H_neon;
    pFunc->pfLumaDeblockingEQ4Hor   = DeblockLumaEq4H_neon;

    pFunc->pfChromaDeblockingLT4Ver = DeblockChromaLt4V_neon;
    pFunc->pfChromaDeblockingEQ4Ver = DeblockChromaEq4V_neon;
    pFunc->pfChromaDeblockingLT4Hor = DeblockChromaLt4H_neon;
    pFunc->pfChromaDeblockingEQ4Hor = DeblockChromaEq4H_neon;
  }
#endif

#if defined(HAVE_NEON_AARCH64)
  if (iCpu & WELS_CPU_NEON) {
    pFunc->pfLumaDeblockingLT4Ver   = DeblockLumaLt4V_AArch64_neon;
    pFunc->pfLumaDeblockingEQ4Ver   = DeblockLumaEq4V_AArch64_neon;
    pFunc->pfLumaDeblockingLT4Hor   = DeblockLumaLt4H_AArch64_neon;
    pFunc->pfLumaDeblockingEQ4Hor   = DeblockLumaEq4H_AArch64_neon;

    pFunc->pfChromaDeblockingLT4Ver = DeblockChromaLt4V_AArch64_neon;
    pFunc->pfChromaDeblockingEQ4Ver = DeblockChromaEq4V_AArch64_neon;
    pFunc->pfChromaDeblockingLT4Hor = DeblockChromaLt4H_AArch64_neon;
    pFunc->pfChromaDeblockingEQ4Hor = DeblockChromaEq4H_AArch64_neon;
  }
#endif
}

} // namespace WelsDec
