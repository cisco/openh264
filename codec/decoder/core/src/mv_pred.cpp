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
 * \file    mv_pred.c
 *
 * \brief   Get MV predictor and update motion vector of mb cache
 *
 * \date    05/22/2009 Created
 *
 *************************************************************************************
 */

#include "mv_pred.h"
#include "ls_defines.h"
#include "mb_cache.h"

namespace WelsDec {
void PredPSkipMvFromNeighbor (PDqLayer pCurLayer, int16_t iMvp[2]) {
  bool bTopAvail, bLeftTopAvail, bRightTopAvail, bLeftAvail;

  int32_t iCurSliceIdc, iTopSliceIdc, iLeftTopSliceIdc, iRightTopSliceIdc, iLeftSliceIdc;
  int32_t iLeftTopType, iRightTopType, iTopType, iLeftType;
  int32_t iCurX, iCurY, iCurXy, iLeftXy, iTopXy = 0, iLeftTopXy = 0, iRightTopXy = 0;

  int8_t iLeftRef;
  int8_t iTopRef;
  int8_t iRightTopRef;
  int8_t iLeftTopRef;
  int8_t iDiagonalRef;
  int8_t iMatchRef;
  int16_t iMvA[2], iMvB[2], iMvC[2], iMvD[2];

  iCurXy = pCurLayer->iMbXyIndex;
  iCurX  = pCurLayer->iMbX;
  iCurY  = pCurLayer->iMbY;
  iCurSliceIdc = pCurLayer->pSliceIdc[iCurXy];

  if (iCurX != 0) {
    iLeftXy = iCurXy - 1;
    iLeftSliceIdc = pCurLayer->pSliceIdc[iLeftXy];
    bLeftAvail = (iLeftSliceIdc == iCurSliceIdc);
  } else {
    bLeftAvail = 0;
    bLeftTopAvail = 0;
  }

  if (iCurY != 0) {
    iTopXy = iCurXy - pCurLayer->iMbWidth;
    iTopSliceIdc = pCurLayer->pSliceIdc[iTopXy];
    bTopAvail = (iTopSliceIdc == iCurSliceIdc);
    if (iCurX != 0) {
      iLeftTopXy = iTopXy - 1;
      iLeftTopSliceIdc = pCurLayer->pSliceIdc[iLeftTopXy];
      bLeftTopAvail = (iLeftTopSliceIdc  == iCurSliceIdc);
    } else {
      bLeftTopAvail = 0;
    }
    if (iCurX != (pCurLayer->iMbWidth - 1)) {
      iRightTopXy = iTopXy + 1;
      iRightTopSliceIdc = pCurLayer->pSliceIdc[iRightTopXy];
      bRightTopAvail = (iRightTopSliceIdc == iCurSliceIdc);
    } else {
      bRightTopAvail = 0;
    }
  } else {
    bTopAvail = 0;
    bLeftTopAvail = 0;
    bRightTopAvail = 0;
  }

  iLeftType = ((iCurX != 0 && bLeftAvail) ? pCurLayer->pMbType[iLeftXy] : 0);
  iTopType = ((iCurY != 0 && bTopAvail) ? pCurLayer->pMbType[iTopXy] : 0);
  iLeftTopType = ((iCurX != 0 && iCurY != 0 && bLeftTopAvail)
                  ? pCurLayer->pMbType[iLeftTopXy] : 0);
  iRightTopType = ((iCurX != pCurLayer->iMbWidth - 1 && iCurY != 0 && bRightTopAvail)
                   ? pCurLayer->pMbType[iRightTopXy] : 0);

  /*get neb mv&iRefIdxArray*/
  /*left*/
  if (bLeftAvail && IS_INTER (iLeftType)) {
    ST32 (iMvA, LD32 (pCurLayer->pMv[0][iLeftXy][3]));
    iLeftRef = pCurLayer->pRefIndex[0][iLeftXy][3];
  } else {
    ST32 (iMvA, 0);
    if (0 == bLeftAvail) { //not available
      iLeftRef = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iLeftRef = REF_NOT_IN_LIST;
    }
  }
  if (REF_NOT_AVAIL == iLeftRef ||
      (0 == iLeftRef && 0 == * (int32_t*)iMvA)) {
    ST32 (iMvp, 0);
    return;
  }

  /*top*/
  if (bTopAvail && IS_INTER (iTopType)) {
    ST32 (iMvB, LD32 (pCurLayer->pMv[0][iTopXy][12]));
    iTopRef = pCurLayer->pRefIndex[0][iTopXy][12];
  } else {
    ST32 (iMvB, 0);
    if (0 == bTopAvail) { //not available
      iTopRef = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iTopRef = REF_NOT_IN_LIST;
    }
  }
  if (REF_NOT_AVAIL == iTopRef ||
      (0 == iTopRef  && 0 == * (int32_t*)iMvB)) {
    ST32 (iMvp, 0);
    return;
  }

  /*right_top*/
  if (bRightTopAvail && IS_INTER (iRightTopType)) {
    ST32 (iMvC, LD32 (pCurLayer->pMv[0][iRightTopXy][12]));
    iRightTopRef = pCurLayer->pRefIndex[0][iRightTopXy][12];
  } else {
    ST32 (iMvC, 0);
    if (0 == bRightTopAvail) { //not available
      iRightTopRef = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iRightTopRef = REF_NOT_IN_LIST;
    }
  }

  /*left_top*/
  if (bLeftTopAvail && IS_INTER (iLeftTopType)) {
    ST32 (iMvD, LD32 (pCurLayer->pMv[0][iLeftTopXy][15]));
    iLeftTopRef = pCurLayer->pRefIndex[0][iLeftTopXy][15];
  } else {
    ST32 (iMvD, 0);
    if (0 == bLeftTopAvail) { //not available
      iLeftTopRef = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iLeftTopRef = REF_NOT_IN_LIST;
    }
  }

  iDiagonalRef = iRightTopRef;
  if (REF_NOT_AVAIL == iDiagonalRef) {
    iDiagonalRef = iLeftTopRef;
    * (int32_t*)iMvC = * (int32_t*)iMvD;
  }

  if (REF_NOT_AVAIL == iTopRef && REF_NOT_AVAIL == iDiagonalRef && iLeftRef >= REF_NOT_IN_LIST) {
    ST32 (iMvp, LD32 (iMvA));
    return;
  }

  iMatchRef = (0 == iLeftRef) + (0 == iTopRef) + (0 == iDiagonalRef);
  if (1 == iMatchRef) {
    if (0 == iLeftRef) {
      ST32 (iMvp, LD32 (iMvA));
    } else if (0 == iTopRef) {
      ST32 (iMvp, LD32 (iMvB));
    } else {
      ST32 (iMvp, LD32 (iMvC));
    }
  } else {
    iMvp[0] = WelsMedian (iMvA[0], iMvB[0], iMvC[0]);
    iMvp[1] = WelsMedian (iMvA[1], iMvB[1], iMvC[1]);
  }
}


//basic iMVs prediction unit for iMVs partition width (4, 2, 1)
void PredMv (int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
             int32_t iPartIdx, int32_t iPartWidth, int8_t iRef, int16_t iMVP[2]) {
  const uint8_t kuiLeftIdx      = g_kuiCache30ScanIdx[iPartIdx] - 1;
  const uint8_t kuiTopIdx       = g_kuiCache30ScanIdx[iPartIdx] - 6;
  const uint8_t kuiRightTopIdx  = kuiTopIdx + iPartWidth;
  const uint8_t kuiLeftTopIdx   = kuiTopIdx - 1;

  const int8_t kiLeftRef      = iRefIndex[0][kuiLeftIdx];
  const int8_t kiTopRef       = iRefIndex[0][ kuiTopIdx];
  const int8_t kiRightTopRef  = iRefIndex[0][kuiRightTopIdx];
  const int8_t kiLeftTopRef   = iRefIndex[0][ kuiLeftTopIdx];
  int8_t iDiagonalRef  = kiRightTopRef;

  int8_t iMatchRef = 0;


  int16_t iAMV[2], iBMV[2], iCMV[2];

  ST32 (iAMV, LD32 (iMotionVector[0][     kuiLeftIdx]));
  ST32 (iBMV, LD32 (iMotionVector[0][      kuiTopIdx]));
  ST32 (iCMV, LD32 (iMotionVector[0][kuiRightTopIdx]));

  if (REF_NOT_AVAIL == iDiagonalRef) {
    iDiagonalRef = kiLeftTopRef;
    ST32 (iCMV, LD32 (iMotionVector[0][kuiLeftTopIdx]));
  }

  iMatchRef = (iRef == kiLeftRef) + (iRef == kiTopRef) + (iRef == iDiagonalRef);

  if (REF_NOT_AVAIL == kiTopRef && REF_NOT_AVAIL == iDiagonalRef && kiLeftRef >= REF_NOT_IN_LIST) {
    ST32 (iMVP, LD32 (iAMV));
    return;
  }

  if (1 == iMatchRef) {
    if (iRef == kiLeftRef) {
      ST32 (iMVP, LD32 (iAMV));
    } else if (iRef == kiTopRef) {
      ST32 (iMVP, LD32 (iBMV));
    } else {
      ST32 (iMVP, LD32 (iCMV));
    }
  } else {
    iMVP[0] = WelsMedian (iAMV[0], iBMV[0], iCMV[0]);
    iMVP[1] = WelsMedian (iAMV[1], iBMV[1], iCMV[1]);
  }
}
void PredInter8x16Mv (int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
                      int32_t iPartIdx, int8_t iRef, int16_t iMVP[2]) {
  if (0 == iPartIdx) {
    const int8_t kiLeftRef = iRefIndex[0][6];
    if (iRef == kiLeftRef) {
      ST32 (iMVP, LD32 (&iMotionVector[0][6][0]));
      return;
    }
  } else { // 1 == iPartIdx
    int8_t iDiagonalRef = iRefIndex[0][5]; //top-right
    int8_t index = 5;
    if (REF_NOT_AVAIL == iDiagonalRef) {
      iDiagonalRef = iRefIndex[0][2]; //top-left for 8*8 block(index 1)
      index = 2;
    }
    if (iRef == iDiagonalRef) {
      ST32 (iMVP, LD32 (&iMotionVector[0][index][0]));
      return;
    }
  }

  PredMv (iMotionVector, iRefIndex, iPartIdx, 2, iRef, iMVP);
}
void PredInter16x8Mv (int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
                      int32_t iPartIdx, int8_t iRef, int16_t iMVP[2]) {
  if (0 == iPartIdx) {
    const int8_t kiTopRef = iRefIndex[0][1];
    if (iRef == kiTopRef) {
      ST32 (iMVP, LD32 (&iMotionVector[0][1][0]));
      return;
    }
  } else { // 8 == iPartIdx
    const int8_t kiLeftRef = iRefIndex[0][18];
    if (iRef == kiLeftRef) {
      ST32 (iMVP, LD32 (&iMotionVector[0][18][0]));
      return;
    }
  }

  PredMv (iMotionVector, iRefIndex, iPartIdx, 4, iRef, iMVP);
}

//update iMVs and iRefIndex cache for current MB, only for P_16*16 (SKIP inclusive)
/* can be further optimized */
void UpdateP16x16MotionInfo (PDqLayer pCurDqLayer, int8_t iRef, int16_t iMVs[2]) {
  const int16_t kiRef2 = (iRef << 8) | iRef;
  const int32_t kiMV32 = LD32 (iMVs);
  int32_t i;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;

  for (i = 0; i < 16; i += 4) {
    //mb
    const uint8_t kuiScan4Idx = g_kuiScan4[i];
    const uint8_t kuiScan4IdxPlus4 = 4 + kuiScan4Idx;

    ST16 (&pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4Idx ], kiRef2);
    ST16 (&pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4IdxPlus4], kiRef2);

    ST32 (pCurDqLayer->pMv[0][iMbXy][  kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[0][iMbXy][1 + kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[0][iMbXy][  kuiScan4IdxPlus4], kiMV32);
    ST32 (pCurDqLayer->pMv[0][iMbXy][1 + kuiScan4IdxPlus4], kiMV32);
  }
}

//update iRefIndex and iMVs of Mb, only for P16x8
/*need further optimization, mb_cache not work */
void UpdateP16x8MotionInfo (PDqLayer pCurDqLayer, int16_t iMotionVector[LIST_A][30][MV_A],
                            int8_t iRefIndex[LIST_A][30],
                            int32_t iPartIdx, int8_t iRef, int16_t iMVs[2]) {
  const int16_t kiRef2 = (iRef << 8) | iRef;
  const int32_t kiMV32 = LD32 (iMVs);
  int32_t i;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  for (i = 0; i < 2; i++, iPartIdx += 4) {
    const uint8_t kuiScan4Idx      = g_kuiScan4[iPartIdx];
    const uint8_t kuiScan4IdxPlus4 = 4 + kuiScan4Idx;
    const uint8_t kuiCacheIdx      = g_kuiCache30ScanIdx[iPartIdx];
    const uint8_t kuiCacheIdxPlus6 = 6 + kuiCacheIdx;

    //mb
    ST16 (&pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4Idx ], kiRef2);
    ST16 (&pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4IdxPlus4], kiRef2);
    ST32 (pCurDqLayer->pMv[0][iMbXy][  kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[0][iMbXy][1 + kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[0][iMbXy][  kuiScan4IdxPlus4], kiMV32);
    ST32 (pCurDqLayer->pMv[0][iMbXy][1 + kuiScan4IdxPlus4], kiMV32);
    //cache
    ST16 (&iRefIndex[0][kuiCacheIdx ], kiRef2);
    ST16 (&iRefIndex[0][kuiCacheIdxPlus6], kiRef2);
    ST32 (iMotionVector[0][  kuiCacheIdx ], kiMV32);
    ST32 (iMotionVector[0][1 + kuiCacheIdx ], kiMV32);
    ST32 (iMotionVector[0][  kuiCacheIdxPlus6], kiMV32);
    ST32 (iMotionVector[0][1 + kuiCacheIdxPlus6], kiMV32);
  }
}
//update iRefIndex and iMVs of both Mb and Mb_cache, only for P8x16
void UpdateP8x16MotionInfo (PDqLayer pCurDqLayer, int16_t iMotionVector[LIST_A][30][MV_A],
                            int8_t iRefIndex[LIST_A][30],
                            int32_t iPartIdx, int8_t iRef, int16_t iMVs[2]) {
  const int16_t kiRef2 = (iRef << 8) | iRef;
  const int32_t kiMV32 = LD32 (iMVs);
  int32_t i;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;

  for (i = 0; i < 2; i++, iPartIdx += 8) {
    const uint8_t kuiScan4Idx = g_kuiScan4[iPartIdx];
    const uint8_t kuiCacheIdx = g_kuiCache30ScanIdx[iPartIdx];
    const uint8_t kuiScan4IdxPlus4 = 4 + kuiScan4Idx;
    const uint8_t kuiCacheIdxPlus6 = 6 + kuiCacheIdx;

    //mb
    ST16 (&pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4Idx ], kiRef2);
    ST16 (&pCurDqLayer->pRefIndex[0][iMbXy][kuiScan4IdxPlus4], kiRef2);
    ST32 (pCurDqLayer->pMv[0][iMbXy][  kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[0][iMbXy][1 + kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[0][iMbXy][  kuiScan4IdxPlus4], kiMV32);
    ST32 (pCurDqLayer->pMv[0][iMbXy][1 + kuiScan4IdxPlus4], kiMV32);
    //cache
    ST16 (&iRefIndex[0][kuiCacheIdx ], kiRef2);
    ST16 (&iRefIndex[0][kuiCacheIdxPlus6], kiRef2);
    ST32 (iMotionVector[0][  kuiCacheIdx ], kiMV32);
    ST32 (iMotionVector[0][1 + kuiCacheIdx ], kiMV32);
    ST32 (iMotionVector[0][  kuiCacheIdxPlus6], kiMV32);
    ST32 (iMotionVector[0][1 + kuiCacheIdxPlus6], kiMV32);
  }
}

} // namespace WelsDec
