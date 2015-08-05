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
 * \file    parse_mb_syn_cavlc.c
 *
 * \brief   Interfaces implementation for parsing the syntax of MB
 *
 * \date    03/17/2009 Created
 *
 *************************************************************************************
 */


#include "parse_mb_syn_cavlc.h"
#include "error_code.h"
#include "mv_pred.h"

namespace WelsDec {
#define MAX_LEVEL_PREFIX 15

typedef struct TagReadBitsCache {
  uint32_t uiCache32Bit;
  uint8_t  uiRemainBits;
  uint8_t*  pBuf;
} SReadBitsCache;

void GetNeighborAvailMbType (PWelsNeighAvail pNeighAvail, PDqLayer pCurLayer) {
  int32_t iCurSliceIdc, iTopSliceIdc, iLeftTopSliceIdc, iRightTopSliceIdc, iLeftSliceIdc;
  int32_t iCurXy, iTopXy = 0, iLeftXy = 0, iLeftTopXy = 0, iRightTopXy = 0;
  int32_t iCurX, iCurY;

  iCurXy = pCurLayer->iMbXyIndex;
  iCurX  = pCurLayer->iMbX;
  iCurY  = pCurLayer->iMbY;
  iCurSliceIdc = pCurLayer->pSliceIdc[iCurXy];
  if (iCurX != 0) {
    iLeftXy = iCurXy - 1;
    iLeftSliceIdc = pCurLayer->pSliceIdc[iLeftXy];
    pNeighAvail->iLeftAvail = (iLeftSliceIdc == iCurSliceIdc);
    pNeighAvail->iLeftCbp   = pNeighAvail->iLeftAvail ? pCurLayer->pCbp[iLeftXy] : 0;
  } else {
    pNeighAvail->iLeftAvail = 0;
    pNeighAvail->iLeftTopAvail = 0;
    pNeighAvail->iLeftCbp = 0;
  }

  if (iCurY != 0) {
    iTopXy = iCurXy - pCurLayer->iMbWidth;
    iTopSliceIdc = pCurLayer->pSliceIdc[iTopXy];
    pNeighAvail->iTopAvail = (iTopSliceIdc == iCurSliceIdc);
    pNeighAvail->iTopCbp   = pNeighAvail->iTopAvail ? pCurLayer->pCbp[iTopXy] : 0;
    if (iCurX != 0) {
      iLeftTopXy = iTopXy - 1;
      iLeftTopSliceIdc = pCurLayer->pSliceIdc[iLeftTopXy];
      pNeighAvail->iLeftTopAvail = (iLeftTopSliceIdc == iCurSliceIdc);
    } else {
      pNeighAvail->iLeftTopAvail = 0;
    }
    if (iCurX != (pCurLayer->iMbWidth - 1)) {
      iRightTopXy = iTopXy + 1;
      iRightTopSliceIdc = pCurLayer->pSliceIdc[iRightTopXy];
      pNeighAvail->iRightTopAvail = (iRightTopSliceIdc == iCurSliceIdc);
    } else {
      pNeighAvail->iRightTopAvail = 0;
    }
  } else {
    pNeighAvail->iTopAvail = 0;
    pNeighAvail->iLeftTopAvail = 0;
    pNeighAvail->iRightTopAvail = 0;
    pNeighAvail->iTopCbp   = 0;
  }

  pNeighAvail->iLeftType     = (pNeighAvail->iLeftAvail     ? pCurLayer->pMbType[iLeftXy]     : 0);
  pNeighAvail->iTopType      = (pNeighAvail->iTopAvail      ? pCurLayer->pMbType[iTopXy]      : 0);
  pNeighAvail->iLeftTopType  = (pNeighAvail->iLeftTopAvail  ? pCurLayer->pMbType[iLeftTopXy]  : 0);
  pNeighAvail->iRightTopType = (pNeighAvail->iRightTopAvail ? pCurLayer->pMbType[iRightTopXy] : 0);
}
void WelsFillCacheNonZeroCount (PWelsNeighAvail pNeighAvail, uint8_t* pNonZeroCount,
                                PDqLayer pCurLayer) { //no matter slice type, intra_pred_constrained_flag
  int32_t iCurXy  = pCurLayer->iMbXyIndex;
  int32_t iTopXy  = 0;
  int32_t iLeftXy = 0;
  if (pNeighAvail->iTopAvail) {
    iTopXy = iCurXy - pCurLayer->iMbWidth;
  }
  if (pNeighAvail->iLeftAvail) {
    iLeftXy = iCurXy - 1;
  }

  //stuff non_zero_coeff_count from pNeighAvail(left and top)
  if (pNeighAvail->iTopAvail) {
    ST32 (&pNonZeroCount[1], LD32 (&pCurLayer->pNzc[iTopXy][12]));
    pNonZeroCount[0] = pNonZeroCount[5] = pNonZeroCount[29] = 0;
    ST16 (&pNonZeroCount[6], LD16 (&pCurLayer->pNzc[iTopXy][20]));
    ST16 (&pNonZeroCount[30], LD16 (&pCurLayer->pNzc[iTopXy][22]));
  } else {
    ST32 (&pNonZeroCount[1], 0xFFFFFFFFU);
    pNonZeroCount[0] = pNonZeroCount[5] = pNonZeroCount[29] = 0xFF;
    ST16 (&pNonZeroCount[6], 0xFFFF);
    ST16 (&pNonZeroCount[30], 0xFFFF);
  }

  if (pNeighAvail->iLeftAvail) {
    pNonZeroCount[8 * 1] = pCurLayer->pNzc[iLeftXy][3];
    pNonZeroCount[8 * 2] = pCurLayer->pNzc[iLeftXy][7];
    pNonZeroCount[8 * 3] = pCurLayer->pNzc[iLeftXy][11];
    pNonZeroCount[8 * 4] = pCurLayer->pNzc[iLeftXy][15];

    pNonZeroCount[5 + 8 * 1] = pCurLayer->pNzc[iLeftXy][17];
    pNonZeroCount[5 + 8 * 2] = pCurLayer->pNzc[iLeftXy][21];
    pNonZeroCount[5 + 8 * 4] = pCurLayer->pNzc[iLeftXy][19];
    pNonZeroCount[5 + 8 * 5] = pCurLayer->pNzc[iLeftXy][23];
  } else {
    pNonZeroCount[8 * 1] =
      pNonZeroCount[8 * 2] =
        pNonZeroCount[8 * 3] =
          pNonZeroCount[8 * 4] = -1;//unavailable

    pNonZeroCount[5 + 8 * 1] =
      pNonZeroCount[5 + 8 * 2] = -1;//unavailable

    pNonZeroCount[5 + 8 * 4] =
      pNonZeroCount[5 + 8 * 5] = -1;//unavailable
  }
}
void WelsFillCacheConstrain1IntraNxN (PWelsNeighAvail pNeighAvail, uint8_t* pNonZeroCount, int8_t* pIntraPredMode,
                                      PDqLayer pCurLayer) { //no matter slice type
  int32_t iCurXy  = pCurLayer->iMbXyIndex;
  int32_t iTopXy  = 0;
  int32_t iLeftXy = 0;

  //stuff non_zero_coeff_count from pNeighAvail(left and top)
  WelsFillCacheNonZeroCount (pNeighAvail, pNonZeroCount, pCurLayer);

  if (pNeighAvail->iTopAvail) {
    iTopXy = iCurXy - pCurLayer->iMbWidth;
  }
  if (pNeighAvail->iLeftAvail) {
    iLeftXy = iCurXy - 1;
  }

  //intra4x4_pred_mode
  if (pNeighAvail->iTopAvail && IS_INTRA4x4 (pNeighAvail->iTopType)) { //top
    ST32 (pIntraPredMode + 1, LD32 (&pCurLayer->pIntraPredMode[iTopXy][0]));
  } else {
    int32_t iPred;
    if (IS_INTRA16x16 (pNeighAvail->iTopType) || (MB_TYPE_INTRA_PCM == pNeighAvail->iTopType))
      iPred = 0x02020202;
    else
      iPred = 0xffffffff;
    ST32 (pIntraPredMode + 1, iPred);
  }

  if (pNeighAvail->iLeftAvail && IS_INTRA4x4 (pNeighAvail->iLeftType)) { //left
    pIntraPredMode[ 0 + 8    ] = pCurLayer->pIntraPredMode[iLeftXy][4];
    pIntraPredMode[ 0 + 8 * 2] = pCurLayer->pIntraPredMode[iLeftXy][5];
    pIntraPredMode[ 0 + 8 * 3] = pCurLayer->pIntraPredMode[iLeftXy][6];
    pIntraPredMode[ 0 + 8 * 4] = pCurLayer->pIntraPredMode[iLeftXy][3];
  } else {
    int8_t iPred;
    if (IS_INTRA16x16 (pNeighAvail->iLeftType) || (MB_TYPE_INTRA_PCM == pNeighAvail->iLeftType))
      iPred = 2;
    else
      iPred = -1;
    pIntraPredMode[ 0 + 8    ] =
      pIntraPredMode[ 0 + 8 * 2] =
        pIntraPredMode[ 0 + 8 * 3] =
          pIntraPredMode[ 0 + 8 * 4] = iPred;
  }
}

void WelsFillCacheConstrain0IntraNxN (PWelsNeighAvail pNeighAvail, uint8_t* pNonZeroCount, int8_t* pIntraPredMode,
                                      PDqLayer pCurLayer) { //no matter slice type
  int32_t iCurXy  = pCurLayer->iMbXyIndex;
  int32_t iTopXy  = 0;
  int32_t iLeftXy = 0;

  //stuff non_zero_coeff_count from pNeighAvail(left and top)
  WelsFillCacheNonZeroCount (pNeighAvail, pNonZeroCount, pCurLayer);

  if (pNeighAvail->iTopAvail) {
    iTopXy = iCurXy - pCurLayer->iMbWidth;
  }
  if (pNeighAvail->iLeftAvail) {
    iLeftXy = iCurXy - 1;
  }

  //intra4x4_pred_mode
  if (pNeighAvail->iTopAvail && IS_INTRANxN (pNeighAvail->iTopType)) { //top
    ST32 (pIntraPredMode + 1, LD32 (&pCurLayer->pIntraPredMode[iTopXy][0]));
  } else {
    int32_t iPred;
    if (pNeighAvail->iTopAvail)
      iPred = 0x02020202;
    else
      iPred = 0xffffffff;
    ST32 (pIntraPredMode + 1, iPred);
  }

  if (pNeighAvail->iLeftAvail && IS_INTRANxN (pNeighAvail->iLeftType)) { //left
    pIntraPredMode[ 0 + 8 * 1] = pCurLayer->pIntraPredMode[iLeftXy][4];
    pIntraPredMode[ 0 + 8 * 2] = pCurLayer->pIntraPredMode[iLeftXy][5];
    pIntraPredMode[ 0 + 8 * 3] = pCurLayer->pIntraPredMode[iLeftXy][6];
    pIntraPredMode[ 0 + 8 * 4] = pCurLayer->pIntraPredMode[iLeftXy][3];
  } else {
    int8_t iPred;
    if (pNeighAvail->iLeftAvail)
      iPred = 2;
    else
      iPred = -1;
    pIntraPredMode[ 0 + 8 * 1] =
      pIntraPredMode[ 0 + 8 * 2] =
        pIntraPredMode[ 0 + 8 * 3] =
          pIntraPredMode[ 0 + 8 * 4] = iPred;
  }
}

void WelsFillCacheInterCabac (PWelsNeighAvail pNeighAvail, uint8_t* pNonZeroCount, int16_t iMvArray[LIST_A][30][MV_A],
                              int16_t iMvdCache[LIST_A][30][MV_A], int8_t iRefIdxArray[LIST_A][30], PDqLayer pCurLayer) {
  int32_t iCurXy      = pCurLayer->iMbXyIndex;
  int32_t iTopXy      = 0;
  int32_t iLeftXy     = 0;
  int32_t iLeftTopXy  = 0;
  int32_t iRightTopXy = 0;

  //stuff non_zero_coeff_count from pNeighAvail(left and top)
  WelsFillCacheNonZeroCount (pNeighAvail, pNonZeroCount, pCurLayer);

  if (pNeighAvail->iTopAvail) {
    iTopXy = iCurXy - pCurLayer->iMbWidth;
  }
  if (pNeighAvail->iLeftAvail) {
    iLeftXy = iCurXy - 1;
  }
  if (pNeighAvail->iLeftTopAvail) {
    iLeftTopXy = iCurXy - 1 - pCurLayer->iMbWidth;
  }
  if (pNeighAvail->iRightTopAvail) {
    iRightTopXy = iCurXy + 1 - pCurLayer->iMbWidth;
  }

  //stuff mv_cache and iRefIdxArray from left and top (inter)
  if (pNeighAvail->iLeftAvail && IS_INTER (pNeighAvail->iLeftType)) {
    ST32 (iMvArray[0][ 6], LD32 (pCurLayer->pMv[0][iLeftXy][ 3]));
    ST32 (iMvArray[0][12], LD32 (pCurLayer->pMv[0][iLeftXy][ 7]));
    ST32 (iMvArray[0][18], LD32 (pCurLayer->pMv[0][iLeftXy][11]));
    ST32 (iMvArray[0][24], LD32 (pCurLayer->pMv[0][iLeftXy][15]));

    ST32 (iMvdCache[0][ 6], LD32 (pCurLayer->pMvd[0][iLeftXy][ 3]));
    ST32 (iMvdCache[0][12], LD32 (pCurLayer->pMvd[0][iLeftXy][ 7]));
    ST32 (iMvdCache[0][18], LD32 (pCurLayer->pMvd[0][iLeftXy][11]));
    ST32 (iMvdCache[0][24], LD32 (pCurLayer->pMvd[0][iLeftXy][15]));

    iRefIdxArray[0][ 6] = pCurLayer->pRefIndex[0][iLeftXy][ 3];
    iRefIdxArray[0][12] = pCurLayer->pRefIndex[0][iLeftXy][ 7];
    iRefIdxArray[0][18] = pCurLayer->pRefIndex[0][iLeftXy][11];
    iRefIdxArray[0][24] = pCurLayer->pRefIndex[0][iLeftXy][15];
  } else {
    ST32 (iMvArray[0][ 6], 0);
    ST32 (iMvArray[0][12], 0);
    ST32 (iMvArray[0][18], 0);
    ST32 (iMvArray[0][24], 0);

    ST32 (iMvdCache[0][ 6], 0);
    ST32 (iMvdCache[0][12], 0);
    ST32 (iMvdCache[0][18], 0);
    ST32 (iMvdCache[0][24], 0);


    if (0 == pNeighAvail->iLeftAvail) { //not available
      iRefIdxArray[0][ 6] =
        iRefIdxArray[0][12] =
          iRefIdxArray[0][18] =
            iRefIdxArray[0][24] = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iRefIdxArray[0][ 6] =
        iRefIdxArray[0][12] =
          iRefIdxArray[0][18] =
            iRefIdxArray[0][24] = REF_NOT_IN_LIST;
    }
  }
  if (pNeighAvail->iLeftTopAvail && IS_INTER (pNeighAvail->iLeftTopType)) {
    ST32 (iMvArray[0][0], LD32 (pCurLayer->pMv[0][iLeftTopXy][15]));
    ST32 (iMvdCache[0][0], LD32 (pCurLayer->pMvd[0][iLeftTopXy][15]));
    iRefIdxArray[0][0] = pCurLayer->pRefIndex[0][iLeftTopXy][15];
  } else {
    ST32 (iMvArray[0][0], 0);
    ST32 (iMvdCache[0][0], 0);
    if (0 == pNeighAvail->iLeftTopAvail) { //not available
      iRefIdxArray[0][0] = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iRefIdxArray[0][0] = REF_NOT_IN_LIST;
    }
  }

  if (pNeighAvail->iTopAvail && IS_INTER (pNeighAvail->iTopType)) {
    ST64 (iMvArray[0][1], LD64 (pCurLayer->pMv[0][iTopXy][12]));
    ST64 (iMvArray[0][3], LD64 (pCurLayer->pMv[0][iTopXy][14]));
    ST64 (iMvdCache[0][1], LD64 (pCurLayer->pMvd[0][iTopXy][12]));
    ST64 (iMvdCache[0][3], LD64 (pCurLayer->pMvd[0][iTopXy][14]));
    ST32 (&iRefIdxArray[0][1], LD32 (&pCurLayer->pRefIndex[0][iTopXy][12]));
  } else {
    ST64 (iMvArray[0][1], 0);
    ST64 (iMvArray[0][3], 0);
    ST64 (iMvdCache[0][1], 0);
    ST64 (iMvdCache[0][3], 0);
    if (0 == pNeighAvail->iTopAvail) { //not available
      iRefIdxArray[0][1] =
        iRefIdxArray[0][2] =
          iRefIdxArray[0][3] =
            iRefIdxArray[0][4] = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iRefIdxArray[0][1] =
        iRefIdxArray[0][2] =
          iRefIdxArray[0][3] =
            iRefIdxArray[0][4] = REF_NOT_IN_LIST;
    }
  }

  if (pNeighAvail->iRightTopAvail && IS_INTER (pNeighAvail->iRightTopType)) {
    ST32 (iMvArray[0][5], LD32 (pCurLayer->pMv[0][iRightTopXy][12]));
    ST32 (iMvdCache[0][5], LD32 (pCurLayer->pMvd[0][iRightTopXy][12]));
    iRefIdxArray[0][5] = pCurLayer->pRefIndex[0][iRightTopXy][12];
  } else {
    ST32 (iMvArray[0][5], 0);
    if (0 == pNeighAvail->iRightTopAvail) { //not available
      iRefIdxArray[0][5] = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iRefIdxArray[0][5] = REF_NOT_IN_LIST;
    }
  }

  //right-top 4*4 block unavailable
  ST32 (iMvArray[0][ 9], 0);
  ST32 (iMvArray[0][21], 0);
  ST32 (iMvArray[0][11], 0);
  ST32 (iMvArray[0][17], 0);
  ST32 (iMvArray[0][23], 0);
  ST32 (iMvdCache[0][ 9], 0);
  ST32 (iMvdCache[0][21], 0);
  ST32 (iMvdCache[0][11], 0);
  ST32 (iMvdCache[0][17], 0);
  ST32 (iMvdCache[0][23], 0);
  iRefIdxArray[0][ 9] =
    iRefIdxArray[0][21] =
      iRefIdxArray[0][11] =
        iRefIdxArray[0][17] =
          iRefIdxArray[0][23] = REF_NOT_AVAIL;
}

void WelsFillCacheInter (PWelsNeighAvail pNeighAvail, uint8_t* pNonZeroCount,
                         int16_t iMvArray[LIST_A][30][MV_A], int8_t iRefIdxArray[LIST_A][30], PDqLayer pCurLayer) {
  int32_t iCurXy      = pCurLayer->iMbXyIndex;
  int32_t iTopXy      = 0;
  int32_t iLeftXy     = 0;
  int32_t iLeftTopXy  = 0;
  int32_t iRightTopXy = 0;

  //stuff non_zero_coeff_count from pNeighAvail(left and top)
  WelsFillCacheNonZeroCount (pNeighAvail, pNonZeroCount, pCurLayer);

  if (pNeighAvail->iTopAvail) {
    iTopXy = iCurXy - pCurLayer->iMbWidth;
  }
  if (pNeighAvail->iLeftAvail) {
    iLeftXy = iCurXy - 1;
  }
  if (pNeighAvail->iLeftTopAvail) {
    iLeftTopXy = iCurXy - 1 - pCurLayer->iMbWidth;
  }
  if (pNeighAvail->iRightTopAvail) {
    iRightTopXy = iCurXy + 1 - pCurLayer->iMbWidth;
  }

  //stuff mv_cache and iRefIdxArray from left and top (inter)
  if (pNeighAvail->iLeftAvail && IS_INTER (pNeighAvail->iLeftType)) {
    ST32 (iMvArray[0][ 6], LD32 (pCurLayer->pMv[0][iLeftXy][ 3]));
    ST32 (iMvArray[0][12], LD32 (pCurLayer->pMv[0][iLeftXy][ 7]));
    ST32 (iMvArray[0][18], LD32 (pCurLayer->pMv[0][iLeftXy][11]));
    ST32 (iMvArray[0][24], LD32 (pCurLayer->pMv[0][iLeftXy][15]));
    iRefIdxArray[0][ 6] = pCurLayer->pRefIndex[0][iLeftXy][ 3];
    iRefIdxArray[0][12] = pCurLayer->pRefIndex[0][iLeftXy][ 7];
    iRefIdxArray[0][18] = pCurLayer->pRefIndex[0][iLeftXy][11];
    iRefIdxArray[0][24] = pCurLayer->pRefIndex[0][iLeftXy][15];
  } else {
    ST32 (iMvArray[0][ 6], 0);
    ST32 (iMvArray[0][12], 0);
    ST32 (iMvArray[0][18], 0);
    ST32 (iMvArray[0][24], 0);

    if (0 == pNeighAvail->iLeftAvail) { //not available
      iRefIdxArray[0][ 6] =
        iRefIdxArray[0][12] =
          iRefIdxArray[0][18] =
            iRefIdxArray[0][24] = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iRefIdxArray[0][ 6] =
        iRefIdxArray[0][12] =
          iRefIdxArray[0][18] =
            iRefIdxArray[0][24] = REF_NOT_IN_LIST;
    }
  }
  if (pNeighAvail->iLeftTopAvail && IS_INTER (pNeighAvail->iLeftTopType)) {
    ST32 (iMvArray[0][0], LD32 (pCurLayer->pMv[0][iLeftTopXy][15]));
    iRefIdxArray[0][0] = pCurLayer->pRefIndex[0][iLeftTopXy][15];
  } else {
    ST32 (iMvArray[0][0], 0);
    if (0 == pNeighAvail->iLeftTopAvail) { //not available
      iRefIdxArray[0][0] = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iRefIdxArray[0][0] = REF_NOT_IN_LIST;
    }
  }
  if (pNeighAvail->iTopAvail && IS_INTER (pNeighAvail->iTopType)) {
    ST64 (iMvArray[0][1], LD64 (pCurLayer->pMv[0][iTopXy][12]));
    ST64 (iMvArray[0][3], LD64 (pCurLayer->pMv[0][iTopXy][14]));
    ST32 (&iRefIdxArray[0][1], LD32 (&pCurLayer->pRefIndex[0][iTopXy][12]));
  } else {
    ST64 (iMvArray[0][1], 0);
    ST64 (iMvArray[0][3], 0);
    if (0 == pNeighAvail->iTopAvail) { //not available
      iRefIdxArray[0][1] =
        iRefIdxArray[0][2] =
          iRefIdxArray[0][3] =
            iRefIdxArray[0][4] = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iRefIdxArray[0][1] =
        iRefIdxArray[0][2] =
          iRefIdxArray[0][3] =
            iRefIdxArray[0][4] = REF_NOT_IN_LIST;
    }
  }
  if (pNeighAvail->iRightTopAvail && IS_INTER (pNeighAvail->iRightTopType)) {
    ST32 (iMvArray[0][5], LD32 (pCurLayer->pMv[0][iRightTopXy][12]));
    iRefIdxArray[0][5] = pCurLayer->pRefIndex[0][iRightTopXy][12];
  } else {
    ST32 (iMvArray[0][5], 0);
    if (0 == pNeighAvail->iRightTopAvail) { //not available
      iRefIdxArray[0][5] = REF_NOT_AVAIL;
    } else { //available but is intra mb type
      iRefIdxArray[0][5] = REF_NOT_IN_LIST;
    }
  }
  //right-top 4*4 block unavailable
  ST32 (iMvArray[0][ 9], 0);
  ST32 (iMvArray[0][21], 0);
  ST32 (iMvArray[0][11], 0);
  ST32 (iMvArray[0][17], 0);
  ST32 (iMvArray[0][23], 0);
  iRefIdxArray[0][ 9] =
    iRefIdxArray[0][21] =
      iRefIdxArray[0][11] =
        iRefIdxArray[0][17] =
          iRefIdxArray[0][23] = REF_NOT_AVAIL;
}

int32_t PredIntra4x4Mode (int8_t* pIntraPredMode, int32_t iIdx4) {
  int8_t iTopMode  = pIntraPredMode[g_kuiScan8[iIdx4] - 8];
  int8_t iLeftMode = pIntraPredMode[g_kuiScan8[iIdx4] - 1];
  int8_t iBestMode;

  if (-1 == iLeftMode || -1 == iTopMode) {
    iBestMode = 2;
  } else {
    iBestMode = WELS_MIN (iLeftMode, iTopMode);
  }
  return iBestMode;
}

#define CHECK_I16_MODE(a, b, c, d)                           \
                      ((a == g_ksI16PredInfo[a].iPredMode) &&  \
                       (b >= g_ksI16PredInfo[a].iLeftAvail) && \
                       (c >= g_ksI16PredInfo[a].iTopAvail) &&  \
                       (d >= g_ksI16PredInfo[a].iLeftTopAvail));
#define CHECK_CHROMA_MODE(a, b, c, d)                              \
                        ((a == g_ksChromaPredInfo[a].iPredMode) &&  \
                         (b >= g_ksChromaPredInfo[a].iLeftAvail) && \
                         (c >= g_ksChromaPredInfo[a].iTopAvail) &&  \
                         (d >= g_ksChromaPredInfo[a].iLeftTopAvail));
#define CHECK_I4_MODE(a, b, c, d)                              \
                     ((a == g_ksI4PredInfo[a].iPredMode) &&      \
                      (b >= g_ksI4PredInfo[a].iLeftAvail) &&     \
                      (c >= g_ksI4PredInfo[a].iTopAvail) &&      \
                      (d >= g_ksI4PredInfo[a].iLeftTopAvail));


int32_t CheckIntra16x16PredMode (uint8_t uiSampleAvail, int8_t* pMode) {
  int32_t iLeftAvail     = uiSampleAvail & 0x04;
  int32_t bLeftTopAvail  = uiSampleAvail & 0x02;
  int32_t iTopAvail      = uiSampleAvail & 0x01;

  if ((*pMode < 0) || (*pMode > MAX_PRED_MODE_ID_I16x16)) {
    return ERR_INFO_INVALID_I16x16_PRED_MODE;
  }

  if (I16_PRED_DC == *pMode) {
    if (iLeftAvail && iTopAvail) {
      return 0;
    } else if (iLeftAvail) {
      *pMode = I16_PRED_DC_L;
    } else if (iTopAvail) {
      *pMode = I16_PRED_DC_T;
    } else {
      *pMode = I16_PRED_DC_128;
    }
  } else {
    bool bModeAvail = CHECK_I16_MODE (*pMode, iLeftAvail, iTopAvail, bLeftTopAvail);
    if (0 == bModeAvail) {
      return ERR_INFO_INVALID_I16x16_PRED_MODE;
    }
  }
  return 0;
}


int32_t CheckIntraChromaPredMode (uint8_t uiSampleAvail, int8_t* pMode) {
  int32_t iLeftAvail     = uiSampleAvail & 0x04;
  int32_t bLeftTopAvail  = uiSampleAvail & 0x02;
  int32_t iTopAvail      = uiSampleAvail & 0x01;

  if (C_PRED_DC == *pMode) {
    if (iLeftAvail && iTopAvail) {
      return 0;
    } else if (iLeftAvail) {
      *pMode = C_PRED_DC_L;
    } else if (iTopAvail) {
      *pMode = C_PRED_DC_T;
    } else {
      *pMode = C_PRED_DC_128;
    }
  } else {
    bool bModeAvail = CHECK_CHROMA_MODE (*pMode, iLeftAvail, iTopAvail, bLeftTopAvail);
    if (0 == bModeAvail) {
      return ERR_INFO_INVALID_I_CHROMA_PRED_MODE;
    }
  }
  return 0;
}

int32_t CheckIntraNxNPredMode (int32_t* pSampleAvail, int8_t* pMode, int32_t iIndex, bool b8x8) {
  int8_t iIdx = g_kuiCache30ScanIdx[iIndex];

  int32_t iLeftAvail     = pSampleAvail[iIdx - 1];
  int32_t iTopAvail      = pSampleAvail[iIdx - 6];
  int32_t bLeftTopAvail  = pSampleAvail[iIdx - 7];
  int32_t bRightTopAvail = pSampleAvail[iIdx - (b8x8 ? 4 : 5)];  // Diff with 4x4 Pred

  int8_t iFinalMode;

  if ((*pMode < 0) || (*pMode > MAX_PRED_MODE_ID_I4x4)) {
    return ERR_INVALID_INTRA4X4_MODE;
  }

  if (I4_PRED_DC == *pMode) {
    if (iLeftAvail && iTopAvail) {
      return *pMode;
    } else if (iLeftAvail) {
      iFinalMode = I4_PRED_DC_L;
    } else if (iTopAvail) {
      iFinalMode = I4_PRED_DC_T;
    } else {
      iFinalMode = I4_PRED_DC_128;
    }
  } else {
    bool bModeAvail = CHECK_I4_MODE (*pMode, iLeftAvail, iTopAvail, bLeftTopAvail);
    if (0 == bModeAvail) {
      return ERR_INVALID_INTRA4X4_MODE;
    }

    iFinalMode = *pMode;

    //if right-top unavailable, modify mode DDL and VL (padding rightmost pixel of top)
    if (I4_PRED_DDL == iFinalMode && 0 == bRightTopAvail) {
      iFinalMode = I4_PRED_DDL_TOP;
    } else if (I4_PRED_VL == iFinalMode && 0 == bRightTopAvail) {
      iFinalMode = I4_PRED_VL_TOP;
    }
  }
  return iFinalMode;
}

void BsStartCavlc (PBitStringAux pBs) {
  pBs->iIndex = ((pBs->pCurBuf - pBs->pStartBuf) << 3) - (16 - pBs->iLeftBits);
}
void BsEndCavlc (PBitStringAux pBs) {
  pBs->pCurBuf   = pBs->pStartBuf + (pBs->iIndex >> 3);
  uint32_t uiCache32Bit = (uint32_t) ((((pBs->pCurBuf[0] << 8) | pBs->pCurBuf[1]) << 16) |
                                      (pBs->pCurBuf[2] << 8) | pBs->pCurBuf[3]);
  pBs->uiCurBits = uiCache32Bit << (pBs->iIndex & 0x07);
  pBs->pCurBuf  += 4;
  pBs->iLeftBits = -16 + (pBs->iIndex & 0x07);
}


// return: used bits
static int32_t CavlcGetTrailingOnesAndTotalCoeff (uint8_t& uiTotalCoeff, uint8_t& uiTrailingOnes,
    SReadBitsCache* pBitsCache, SVlcTable* pVlcTable, bool bChromaDc, int8_t nC) {
  const uint8_t* kpVlcTableMoreBitsCountList[3] = {g_kuiVlcTableMoreBitsCount0, g_kuiVlcTableMoreBitsCount1, g_kuiVlcTableMoreBitsCount2};
  int32_t iUsedBits = 0;
  int32_t iIndexVlc, iIndexValue, iNcMapIdx;
  uint32_t uiCount;
  uint32_t uiValue;

  if (bChromaDc) {
    uiValue        = pBitsCache->uiCache32Bit >> 24;
    iIndexVlc      = pVlcTable->kpChromaCoeffTokenVlcTable[uiValue][0];
    uiCount        = pVlcTable->kpChromaCoeffTokenVlcTable[uiValue][1];
    POP_BUFFER (pBitsCache, uiCount);
    iUsedBits     += uiCount;
    uiTrailingOnes = g_kuiVlcTrailingOneTotalCoeffTable[iIndexVlc][0];
    uiTotalCoeff   = g_kuiVlcTrailingOneTotalCoeffTable[iIndexVlc][1];
  } else { //luma
    iNcMapIdx = g_kuiNcMapTable[nC];
    if (iNcMapIdx <= 2) {
      uiValue = pBitsCache->uiCache32Bit >> 24;
      if (uiValue < g_kuiVlcTableNeedMoreBitsThread[iNcMapIdx]) {
        POP_BUFFER (pBitsCache, 8);
        iUsedBits  += 8;
        iIndexValue = pBitsCache->uiCache32Bit >> (32 - kpVlcTableMoreBitsCountList[iNcMapIdx][uiValue]);
        iIndexVlc   = pVlcTable->kpCoeffTokenVlcTable[iNcMapIdx + 1][uiValue][iIndexValue][0];
        uiCount     = pVlcTable->kpCoeffTokenVlcTable[iNcMapIdx + 1][uiValue][iIndexValue][1];
        POP_BUFFER (pBitsCache, uiCount);
        iUsedBits  += uiCount;
      } else {
        iIndexVlc  = pVlcTable->kpCoeffTokenVlcTable[0][iNcMapIdx][uiValue][0];
        uiCount    = pVlcTable->kpCoeffTokenVlcTable[0][iNcMapIdx][uiValue][1];
        uiValue    = pBitsCache->uiCache32Bit >> (32 - uiCount);
        POP_BUFFER (pBitsCache, uiCount);
        iUsedBits += uiCount;
      }
    } else {
      uiValue    = pBitsCache->uiCache32Bit >> (32 - 6);
      POP_BUFFER (pBitsCache, 6);
      iUsedBits += 6;
      iIndexVlc  = pVlcTable->kpCoeffTokenVlcTable[0][3][uiValue][0];  //differ
    }
    uiTrailingOnes = g_kuiVlcTrailingOneTotalCoeffTable[iIndexVlc][0];
    uiTotalCoeff  = g_kuiVlcTrailingOneTotalCoeffTable[iIndexVlc][1];
  }

  return iUsedBits;
}

static int32_t CavlcGetLevelVal (int32_t iLevel[16], SReadBitsCache* pBitsCache, uint8_t uiTotalCoeff,
                                 uint8_t uiTrailingOnes) {
  int32_t i, iUsedBits = 0;
  int32_t iSuffixLength, iSuffixLengthSize, iLevelPrefix, iPrefixBits, iLevelCode, iThreshold;
  for (i = 0; i < uiTrailingOnes; i++) {
    iLevel[i] = 1 - ((pBitsCache->uiCache32Bit >> (30 - i)) & 0x02);
  }
  POP_BUFFER (pBitsCache, uiTrailingOnes);
  iUsedBits += uiTrailingOnes;

  iSuffixLength = (uiTotalCoeff > 10 && uiTrailingOnes < 3);

  for (; i < uiTotalCoeff; i++) {
    if (pBitsCache->uiRemainBits <= 16) SHIFT_BUFFER (pBitsCache);
    WELS_GET_PREFIX_BITS (pBitsCache->uiCache32Bit, iPrefixBits);
    if (iPrefixBits > MAX_LEVEL_PREFIX + 1) //iPrefixBits includes leading "0"s and first "1", should +1
      return -1;
    POP_BUFFER (pBitsCache, iPrefixBits);
    iUsedBits   += iPrefixBits;
    iLevelPrefix = iPrefixBits - 1;

    iLevelCode = iLevelPrefix << iSuffixLength; //differ
    iSuffixLengthSize = iSuffixLength;

    if (iLevelPrefix >= 14) {
      if (14 == iLevelPrefix && 0 == iSuffixLength)
        iSuffixLengthSize = 4;
      else if (15 == iLevelPrefix) {
        iSuffixLengthSize = 12;
        if (iSuffixLength == 0)
          iLevelCode += 15;
      }
    }

    if (iSuffixLengthSize > 0) {
      if (pBitsCache->uiRemainBits <= iSuffixLengthSize) SHIFT_BUFFER (pBitsCache);
      iLevelCode += (pBitsCache->uiCache32Bit >> (32 - iSuffixLengthSize));
      POP_BUFFER (pBitsCache, iSuffixLengthSize);
      iUsedBits  += iSuffixLengthSize;
    }

    iLevelCode += ((i == uiTrailingOnes) && (uiTrailingOnes < 3)) << 1;
    iLevel[i]   = ((iLevelCode + 2) >> 1);
    iLevel[i]  -= (iLevel[i] << 1) & (- (iLevelCode & 0x01));

    iSuffixLength += !iSuffixLength;
    iThreshold     = 3 << (iSuffixLength - 1);
    iSuffixLength += ((iLevel[i] > iThreshold) || (iLevel[i] < -iThreshold)) && (iSuffixLength < 6);
  }

  return iUsedBits;
}

static int32_t CavlcGetTotalZeros (int32_t& iZerosLeft, SReadBitsCache* pBitsCache, uint8_t uiTotalCoeff,
                                   SVlcTable* pVlcTable, bool bChromaDc) {
  int32_t iCount, iUsedBits = 0;
  const uint8_t* kpBitNumMap;
  uint32_t uiValue;

  int32_t iTotalZeroVlcIdx;
  uint8_t uiTableType;
  //chroma_dc (0 < uiTotalCoeff < 4); others (chroma_ac or luma: 0 < uiTotalCoeff < 16)

  if (bChromaDc) {
    iTotalZeroVlcIdx = uiTotalCoeff;
    kpBitNumMap = g_kuiTotalZerosBitNumChromaMap;
    uiTableType = bChromaDc;
  } else {
    iTotalZeroVlcIdx = uiTotalCoeff;
    kpBitNumMap = g_kuiTotalZerosBitNumMap;
    uiTableType = 0;
  }

  iCount = kpBitNumMap[iTotalZeroVlcIdx - 1];
  if (pBitsCache->uiRemainBits < iCount) SHIFT_BUFFER (
      pBitsCache); // if uiRemainBits+16 still smaller than iCount?? potential bug
  uiValue    = pBitsCache->uiCache32Bit >> (32 - iCount);
  iCount     = pVlcTable->kpTotalZerosTable[uiTableType][iTotalZeroVlcIdx - 1][uiValue][1];
  POP_BUFFER (pBitsCache, iCount);
  iUsedBits += iCount;
  iZerosLeft = pVlcTable->kpTotalZerosTable[uiTableType][iTotalZeroVlcIdx - 1][uiValue][0];

  return iUsedBits;
}
static int32_t CavlcGetRunBefore (int32_t iRun[16], SReadBitsCache* pBitsCache, uint8_t uiTotalCoeff,
                                  SVlcTable* pVlcTable, int32_t iZerosLeft) {
  int32_t i, iUsedBits = 0;
  uint32_t uiCount, uiValue, iPrefixBits;

  for (i = 0; i < uiTotalCoeff - 1; i++) {
    if (iZerosLeft > 0) {
      uiCount = g_kuiZeroLeftBitNumMap[iZerosLeft];
      if (pBitsCache->uiRemainBits < uiCount) SHIFT_BUFFER (pBitsCache);
      uiValue = pBitsCache->uiCache32Bit >> (32 - uiCount);
      if (iZerosLeft < 7) {
        uiCount = pVlcTable->kpZeroTable[iZerosLeft - 1][uiValue][1];
        POP_BUFFER (pBitsCache, uiCount);
        iUsedBits += uiCount;
        iRun[i] = pVlcTable->kpZeroTable[iZerosLeft - 1][uiValue][0];
      } else {
        POP_BUFFER (pBitsCache, uiCount);
        iUsedBits += uiCount;
        if (pVlcTable->kpZeroTable[6][uiValue][0] < 7) {
          iRun[i] = pVlcTable->kpZeroTable[6][uiValue][0];
        } else {
          if (pBitsCache->uiRemainBits < 16) SHIFT_BUFFER (pBitsCache);
          WELS_GET_PREFIX_BITS (pBitsCache->uiCache32Bit, iPrefixBits);
          iRun[i] = iPrefixBits + 6;
          if (iRun[i] > iZerosLeft)
            return -1;
          POP_BUFFER (pBitsCache, iPrefixBits);
          iUsedBits += iPrefixBits;
        }
      }
    } else {
      for (int j = i; j < uiTotalCoeff; j++) {
        iRun[j] = 0;
      }
      return iUsedBits;
    }

    iZerosLeft -= iRun[i];
  }

  iRun[uiTotalCoeff - 1] = iZerosLeft;

  return iUsedBits;
}

int32_t WelsResidualBlockCavlc (SVlcTable* pVlcTable, uint8_t* pNonZeroCountCache, PBitStringAux pBs, int32_t iIndex,
                                int32_t iMaxNumCoeff,
                                const uint8_t* kpZigzagTable, int32_t iResidualProperty, int16_t* pTCoeff, uint8_t uiQp,
                                PWelsDecoderContext pCtx) {
  int32_t iLevel[16], iZerosLeft, iCoeffNum;
  int32_t  iRun[16];
  int32_t iCurNonZeroCacheIdx, i;


  int32_t iMbResProperty = 0;
  GetMbResProperty (&iMbResProperty, &iResidualProperty, 1);
  const uint16_t* kpDequantCoeff = pCtx->bUseScalingList ? pCtx->pDequant_coeff4x4[iMbResProperty][uiQp] :
                                   g_kuiDequantCoeff[uiQp];

  int8_t nA, nB, nC;
  uint8_t uiTotalCoeff, uiTrailingOnes;
  int32_t iUsedBits = 0;
  intX_t iCurIdx   = pBs->iIndex;
  uint8_t* pBuf     = ((uint8_t*)pBs->pStartBuf) + (iCurIdx >> 3);
  bool  bChromaDc = (CHROMA_DC == iResidualProperty);
  uint8_t bChroma   = (bChromaDc || CHROMA_AC == iResidualProperty);
  SReadBitsCache sReadBitsCache;

  uint32_t uiCache32Bit = (uint32_t) ((((pBuf[0] << 8) | pBuf[1]) << 16) | (pBuf[2] << 8) | pBuf[3]);
  sReadBitsCache.uiCache32Bit = uiCache32Bit << (iCurIdx & 0x07);
  sReadBitsCache.uiRemainBits = 32 - (iCurIdx & 0x07);
  sReadBitsCache.pBuf = pBuf;
  //////////////////////////////////////////////////////////////////////////

  if (bChroma) {
    iCurNonZeroCacheIdx = g_kuiCache48CountScan4Idx[iIndex];
    nA = pNonZeroCountCache[iCurNonZeroCacheIdx - 1];
    nB = pNonZeroCountCache[iCurNonZeroCacheIdx - 8];
  } else { //luma
    iCurNonZeroCacheIdx = g_kuiCache48CountScan4Idx[iIndex];
    nA = pNonZeroCountCache[iCurNonZeroCacheIdx - 1];
    nB = pNonZeroCountCache[iCurNonZeroCacheIdx - 8];
  }

  WELS_NON_ZERO_COUNT_AVERAGE (nC, nA, nB);

  iUsedBits += CavlcGetTrailingOnesAndTotalCoeff (uiTotalCoeff, uiTrailingOnes, &sReadBitsCache, pVlcTable, bChromaDc,
               nC);

  if (iResidualProperty != CHROMA_DC && iResidualProperty != I16_LUMA_DC) {
    pNonZeroCountCache[iCurNonZeroCacheIdx] = uiTotalCoeff;
    //////////////////////////////////////////////////////////////////////////
  }
  if (0 == uiTotalCoeff) {
    pBs->iIndex += iUsedBits;
    return 0;
  }
  if ((uiTrailingOnes > 3) || (uiTotalCoeff > 16)) { /////////////////check uiTrailingOnes and uiTotalCoeff
    return ERR_INFO_CAVLC_INVALID_TOTAL_COEFF_OR_TRAILING_ONES;
  }
  if ((i = CavlcGetLevelVal (iLevel, &sReadBitsCache, uiTotalCoeff, uiTrailingOnes)) == -1) {
    return ERR_INFO_CAVLC_INVALID_LEVEL;
  }
  iUsedBits += i;
  if (uiTotalCoeff < iMaxNumCoeff) {
    iUsedBits += CavlcGetTotalZeros (iZerosLeft, &sReadBitsCache, uiTotalCoeff, pVlcTable, bChromaDc);
  } else {
    iZerosLeft = 0;
  }

  if ((iZerosLeft < 0) || ((iZerosLeft + uiTotalCoeff) > iMaxNumCoeff)) {
    return ERR_INFO_CAVLC_INVALID_ZERO_LEFT;
  }
  if ((i = CavlcGetRunBefore (iRun, &sReadBitsCache, uiTotalCoeff, pVlcTable, iZerosLeft)) == -1) {
    return ERR_INFO_CAVLC_INVALID_RUN_BEFORE;
  }
  iUsedBits += i;
  pBs->iIndex += iUsedBits;
  iCoeffNum = -1;

  if (iResidualProperty == CHROMA_DC) {
    //chroma dc scaling process, is kpDequantCoeff[0]? LevelScale(qPdc%6,0,0))<<(qPdc/6-6), the transform is done at construction.
    for (i = uiTotalCoeff - 1; i >= 0; --i) {
      //FIXME merge into rundecode?
      int32_t j;
      iCoeffNum += iRun[i] + 1; //FIXME add 1 earlier ?
      j          = kpZigzagTable[ iCoeffNum ];
      pTCoeff[j] = pCtx->bUseScalingList ? (iLevel[i] * kpDequantCoeff[0]) >> 4 : (iLevel[i] * kpDequantCoeff[0]);
    }
  } else if (iResidualProperty == I16_LUMA_DC) { //DC coefficent, only call in Intra_16x16, base_mode_flag = 0
    for (i = uiTotalCoeff - 1; i >= 0; --i) { //FIXME merge into rundecode?
      int32_t j;
      iCoeffNum += iRun[i] + 1; //FIXME add 1 earlier ?
      j          = kpZigzagTable[ iCoeffNum ];
      pTCoeff[j] = iLevel[i];
    }
  } else {
    for (i = uiTotalCoeff - 1; i >= 0; --i) { //FIXME merge into  rundecode?
      int32_t j;
      iCoeffNum += iRun[i] + 1; //FIXME add 1 earlier ?
      j          = kpZigzagTable[ iCoeffNum ];
      pTCoeff[j] = pCtx->bUseScalingList ? (iLevel[i] * kpDequantCoeff[j]) >> 4 : (iLevel[i] * kpDequantCoeff[j & 0x07]);
    }
  }

  return 0;
}

int32_t WelsResidualBlockCavlc8x8 (SVlcTable* pVlcTable, uint8_t* pNonZeroCountCache, PBitStringAux pBs, int32_t iIndex,
                                   int32_t iMaxNumCoeff, const uint8_t* kpZigzagTable, int32_t iResidualProperty,
                                   int16_t* pTCoeff, int32_t  iIdx4x4, uint8_t uiQp,
                                   PWelsDecoderContext pCtx) {
  int32_t iLevel[16], iZerosLeft, iCoeffNum;
  int32_t  iRun[16];
  int32_t iCurNonZeroCacheIdx, i;

  int32_t iMbResProperty = 0;
  GetMbResProperty (&iMbResProperty, &iResidualProperty, 1);

  const uint16_t* kpDequantCoeff = pCtx->bUseScalingList ? pCtx->pDequant_coeff8x8[iMbResProperty - 6][uiQp] :
                                   g_kuiDequantCoeff8x8[uiQp];

  int8_t nA, nB, nC;
  uint8_t uiTotalCoeff, uiTrailingOnes;
  int32_t iUsedBits = 0;
  intX_t iCurIdx   = pBs->iIndex;
  uint8_t* pBuf     = ((uint8_t*)pBs->pStartBuf) + (iCurIdx >> 3);
  bool  bChromaDc = (CHROMA_DC == iResidualProperty);
  uint8_t bChroma   = (bChromaDc || CHROMA_AC == iResidualProperty);
  SReadBitsCache sReadBitsCache;

  uint32_t uiCache32Bit = (uint32_t) ((((pBuf[0] << 8) | pBuf[1]) << 16) | (pBuf[2] << 8) | pBuf[3]);
  sReadBitsCache.uiCache32Bit = uiCache32Bit << (iCurIdx & 0x07);
  sReadBitsCache.uiRemainBits = 32 - (iCurIdx & 0x07);
  sReadBitsCache.pBuf = pBuf;
  //////////////////////////////////////////////////////////////////////////

  if (bChroma) {
    iCurNonZeroCacheIdx = g_kuiCache48CountScan4Idx[iIndex];
    nA = pNonZeroCountCache[iCurNonZeroCacheIdx - 1];
    nB = pNonZeroCountCache[iCurNonZeroCacheIdx - 8];
  } else { //luma
    iCurNonZeroCacheIdx = g_kuiCache48CountScan4Idx[iIndex];
    nA = pNonZeroCountCache[iCurNonZeroCacheIdx - 1];
    nB = pNonZeroCountCache[iCurNonZeroCacheIdx - 8];
  }

  WELS_NON_ZERO_COUNT_AVERAGE (nC, nA, nB);

  iUsedBits += CavlcGetTrailingOnesAndTotalCoeff (uiTotalCoeff, uiTrailingOnes, &sReadBitsCache, pVlcTable, bChromaDc,
               nC);

  if (iResidualProperty != CHROMA_DC && iResidualProperty != I16_LUMA_DC) {
    pNonZeroCountCache[iCurNonZeroCacheIdx] = uiTotalCoeff;
    //////////////////////////////////////////////////////////////////////////
  }
  if (0 == uiTotalCoeff) {
    pBs->iIndex += iUsedBits;
    return 0;
  }
  if ((uiTrailingOnes > 3) || (uiTotalCoeff > 16)) { /////////////////check uiTrailingOnes and uiTotalCoeff
    return ERR_INFO_CAVLC_INVALID_TOTAL_COEFF_OR_TRAILING_ONES;
  }
  if ((i = CavlcGetLevelVal (iLevel, &sReadBitsCache, uiTotalCoeff, uiTrailingOnes)) == -1) {
    return ERR_INFO_CAVLC_INVALID_LEVEL;
  }
  iUsedBits += i;
  if (uiTotalCoeff < iMaxNumCoeff) {
    iUsedBits += CavlcGetTotalZeros (iZerosLeft, &sReadBitsCache, uiTotalCoeff, pVlcTable, bChromaDc);
  } else {
    iZerosLeft = 0;
  }

  if ((iZerosLeft < 0) || ((iZerosLeft + uiTotalCoeff) > iMaxNumCoeff)) {
    return ERR_INFO_CAVLC_INVALID_ZERO_LEFT;
  }
  if ((i = CavlcGetRunBefore (iRun, &sReadBitsCache, uiTotalCoeff, pVlcTable, iZerosLeft)) == -1) {
    return ERR_INFO_CAVLC_INVALID_RUN_BEFORE;
  }
  iUsedBits += i;
  pBs->iIndex += iUsedBits;
  iCoeffNum = -1;

  for (i = uiTotalCoeff - 1; i >= 0; --i) { //FIXME merge into  rundecode?
    int32_t j;
    iCoeffNum += iRun[i] + 1; //FIXME add 1 earlier ?
    j = (iCoeffNum << 2) + iIdx4x4;
    j          = kpZigzagTable[ j ];
    pTCoeff[j] = uiQp >= 36 ? ((iLevel[i] * kpDequantCoeff[j]) << (uiQp / 6 - 6))
                 : ((iLevel[i] * kpDequantCoeff[j] + (1 << (5 - uiQp / 6))) >> (6 - uiQp / 6));
  }

  return 0;
}

int32_t ParseInterInfo (PWelsDecoderContext pCtx, int16_t iMvArray[LIST_A][30][MV_A], int8_t iRefIdxArray[LIST_A][30],
                        PBitStringAux pBs) {
  PSlice pSlice                 = &pCtx->pCurDqLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader     = &pSlice->sSliceHeaderExt.sSliceHeader;
  PPicture* ppRefPic = pCtx->sRefPic.pRefList[LIST_0];
  int32_t iRefCount[2];
  PDqLayer pCurDqLayer = pCtx->pCurDqLayer;
  int32_t i, j;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  int32_t iMotionPredFlag[4];
  int16_t iMv[2];
  uint32_t uiCode;
  int32_t iCode;
  int16_t iMinVmv = pSliceHeader->pSps->pSLevelLimits->iMinVmv;
  int16_t iMaxVmv = pSliceHeader->pSps->pSLevelLimits->iMaxVmv;
  iMotionPredFlag[0] = iMotionPredFlag[1] = iMotionPredFlag[2] = iMotionPredFlag[3] =
                         pSlice->sSliceHeaderExt.bDefaultMotionPredFlag;
  iRefCount[0] = pSliceHeader->uiRefCount[0];
  iRefCount[1] = pSliceHeader->uiRefCount[1];

  switch (pCurDqLayer->pMbType[iMbXy]) {
  case MB_TYPE_16x16: {
    int32_t iRefIdx = 0;
    if (pSlice->sSliceHeaderExt.bAdaptiveMotionPredFlag) {
      WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //motion_prediction_flag_l0[ mbPartIdx ]
      iMotionPredFlag[0] = uiCode;
    }
    if (iMotionPredFlag[0] == 0) {
      WELS_READ_VERIFY (BsGetTe0 (pBs, iRefCount[0], &uiCode)); //motion_prediction_flag_l1[ mbPartIdx ]
      iRefIdx = uiCode;
      // Security check: iRefIdx should be in range 0 to num_ref_idx_l0_active_minus1, includsive
      // ref to standard section 7.4.5.1. iRefCount[0] is 1 + num_ref_idx_l0_active_minus1.
      if ((iRefIdx < 0) || (iRefIdx >= iRefCount[0]) || (ppRefPic[iRefIdx] == NULL)) { //error ref_idx
        pCtx->bMbRefConcealed = true;
        if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
          iRefIdx = 0;
          pCtx->iErrorCode |= dsBitstreamError;
        } else {
          return ERR_INFO_INVALID_REF_INDEX;
        }
      }
      pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || ! (ppRefPic[iRefIdx]
                              && ppRefPic[iRefIdx]->bIsComplete);
    } else {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "inter parse: iMotionPredFlag = 1 not supported. ");
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_UNSUPPORTED_ILP);
    }
    PredMv (iMvArray, iRefIdxArray, 0, 4, iRefIdx, iMv);

    WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //mvd_l0[ mbPartIdx ][ 0 ][ compIdx ]
    iMv[0] += iCode;
    WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //mvd_l1[ mbPartIdx ][ 0 ][ compIdx ]
    iMv[1] += iCode;
    WELS_CHECK_SE_BOTH_WARNING (iMv[1], iMinVmv, iMaxVmv, "vertical mv");
    UpdateP16x16MotionInfo (pCurDqLayer, iRefIdx, iMv);
  }
  break;
  case MB_TYPE_16x8: {
    int32_t iRefIdx[2];
    for (i = 0; i < 2; i++) {
      if (pSlice->sSliceHeaderExt.bAdaptiveMotionPredFlag) {
        WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //motion_prediction_flag_l0[ mbPartIdx ]
        iMotionPredFlag[i] = uiCode;
      }
    }

    for (i = 0; i < 2; i++) {
      if (iMotionPredFlag[i]) {
        WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "inter parse: iMotionPredFlag = 1 not supported. ");
        return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_UNSUPPORTED_ILP);
      }
      WELS_READ_VERIFY (BsGetTe0 (pBs, iRefCount[0], &uiCode)); //ref_idx_l0[ mbPartIdx ]
      iRefIdx[i] = uiCode;
      if ((iRefIdx[i] < 0) || (iRefIdx[i] >= iRefCount[0]) || (ppRefPic[iRefIdx[i]] == NULL)) { //error ref_idx
        pCtx->bMbRefConcealed = true;
        if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
          iRefIdx[i] = 0;
          pCtx->iErrorCode |= dsBitstreamError;
        } else {
          return ERR_INFO_INVALID_REF_INDEX;
        }
      }
      pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || ! (ppRefPic[iRefIdx[i]]
                              && ppRefPic[iRefIdx[i]]->bIsComplete);
    }
    for (i = 0; i < 2; i++) {
      PredInter16x8Mv (iMvArray, iRefIdxArray, i << 3, iRefIdx[i], iMv);

      WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //mvd_l0[ mbPartIdx ][ 0 ][ compIdx ]
      iMv[0] += iCode;
      WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //mvd_l1[ mbPartIdx ][ 0 ][ compIdx ]
      iMv[1] += iCode;
      WELS_CHECK_SE_BOTH_WARNING (iMv[1], iMinVmv, iMaxVmv, "vertical mv");
      UpdateP16x8MotionInfo (pCurDqLayer, iMvArray, iRefIdxArray, i << 3, iRefIdx[i], iMv);
    }
  }
  break;
  case MB_TYPE_8x16: {
    int32_t iRefIdx[2];
    for (i = 0; i < 2; i++) {
      if (pSlice->sSliceHeaderExt.bAdaptiveMotionPredFlag) {
        WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //motion_prediction_flag_l0[ mbPartIdx ]
        iMotionPredFlag[i] = uiCode;
      }
    }

    for (i = 0; i < 2; i++) {
      if (iMotionPredFlag[i] == 0) {
        WELS_READ_VERIFY (BsGetTe0 (pBs, iRefCount[0], &uiCode)); //ref_idx_l0[ mbPartIdx ]
        iRefIdx[i] = uiCode;
        if ((iRefIdx[i] < 0) || (iRefIdx[i] >= iRefCount[0]) || (ppRefPic[iRefIdx[i]] == NULL)) { //error ref_idx
          pCtx->bMbRefConcealed = true;
          if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
            iRefIdx[i] = 0;
            pCtx->iErrorCode |= dsBitstreamError;
          } else {
            return ERR_INFO_INVALID_REF_INDEX;
          }
        }
        pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || ! (ppRefPic[iRefIdx[i]]
                                && ppRefPic[iRefIdx[i]]->bIsComplete);
      } else {
        WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "inter parse: iMotionPredFlag = 1 not supported. ");
        return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_UNSUPPORTED_ILP);
      }

    }
    for (i = 0; i < 2; i++) {
      PredInter8x16Mv (iMvArray, iRefIdxArray, i << 2, iRefIdx[i], iMv);

      WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //mvd_l0[ mbPartIdx ][ 0 ][ compIdx ]
      iMv[0] += iCode;
      WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //mvd_l1[ mbPartIdx ][ 0 ][ compIdx ]
      iMv[1] += iCode;
      WELS_CHECK_SE_BOTH_WARNING (iMv[1], iMinVmv, iMaxVmv, "vertical mv");
      UpdateP8x16MotionInfo (pCurDqLayer, iMvArray, iRefIdxArray, i << 2, iRefIdx[i], iMv);
    }
  }
  break;
  case MB_TYPE_8x8:
  case MB_TYPE_8x8_REF0: {
    int32_t iRefIdx[4] = {0}, iSubPartCount[4], iPartWidth[4];
    uint32_t uiSubMbType;

    if (MB_TYPE_8x8_REF0 == pCurDqLayer->pMbType[iMbXy]) {
      iRefCount[0] =
        iRefCount[1] = 1;
    }

    //uiSubMbType, partition
    for (i = 0; i < 4; i++) {
      WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //sub_mb_type[ mbPartIdx ]
      uiSubMbType = uiCode;
      if (uiSubMbType >= 4) { //invalid uiSubMbType
        return ERR_INFO_INVALID_SUB_MB_TYPE;
      }
      pCurDqLayer->pSubMbType[iMbXy][i] = g_ksInterSubMbTypeInfo[uiSubMbType].iType;
      iSubPartCount[i] = g_ksInterSubMbTypeInfo[uiSubMbType].iPartCount;
      iPartWidth[i] = g_ksInterSubMbTypeInfo[uiSubMbType].iPartWidth;

      // Need modification when B picture add in, reference to 7.3.5
      pCurDqLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] &= (uiSubMbType == 0);
    }

    if (pSlice->sSliceHeaderExt.bAdaptiveMotionPredFlag) {
      for (i = 0; i < 4; i++) {
        WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //motion_prediction_flag_l0[ mbPartIdx ]
        iMotionPredFlag[i] = uiCode;
      }
    }

    //iRefIdxArray
    if (MB_TYPE_8x8_REF0 == pCurDqLayer->pMbType[iMbXy]) {
      memset (pCurDqLayer->pRefIndex[0][iMbXy], 0, 16);
    } else {
      for (i = 0; i < 4; i++) {
        int16_t iIndex8 = i << 2;
        uint8_t uiScan4Idx = g_kuiScan4[iIndex8];

        if (iMotionPredFlag[i] == 0) {
          WELS_READ_VERIFY (BsGetTe0 (pBs, iRefCount[0], &uiCode)); //ref_idx_l0[ mbPartIdx ]
          iRefIdx[i] = uiCode;
          if ((iRefIdx[i] < 0) || (iRefIdx[i] >= iRefCount[0]) || (ppRefPic[iRefIdx[i]] == NULL)) { //error ref_idx
            pCtx->bMbRefConcealed = true;
            if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
              iRefIdx[i] = 0;
              pCtx->iErrorCode |= dsBitstreamError;
            } else {
              return ERR_INFO_INVALID_REF_INDEX;
            }
          }
          pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || ! (ppRefPic[iRefIdx[i]]
                                  && ppRefPic[iRefIdx[i]]->bIsComplete);

          pCurDqLayer->pRefIndex[0][iMbXy][uiScan4Idx  ] = pCurDqLayer->pRefIndex[0][iMbXy][uiScan4Idx + 1] =
                pCurDqLayer->pRefIndex[0][iMbXy][uiScan4Idx + 4] = pCurDqLayer->pRefIndex[0][iMbXy][uiScan4Idx + 5] = iRefIdx[i];
        } else {
          WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "inter parse: iMotionPredFlag = 1 not supported. ");
          return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_UNSUPPORTED_ILP);
        }
      }
    }

    //gain mv and update mv cache
    for (i = 0; i < 4; i++) {
      int8_t iPartCount = iSubPartCount[i];
      uint32_t uiSubMbType = pCurDqLayer->pSubMbType[iMbXy][i];
      int16_t iMv[2], iPartIdx, iBlockWidth = iPartWidth[i], iIdx = i << 2;
      uint8_t uiScan4Idx, uiCacheIdx;

      uint8_t uiIdx4Cache = g_kuiCache30ScanIdx[iIdx];

      iRefIdxArray[0][uiIdx4Cache  ] = iRefIdxArray[0][uiIdx4Cache + 1] =
                                         iRefIdxArray[0][uiIdx4Cache + 6] = iRefIdxArray[0][uiIdx4Cache + 7] = iRefIdx[i];

      for (j = 0; j < iPartCount; j++) {
        iPartIdx = iIdx + j * iBlockWidth;
        uiScan4Idx = g_kuiScan4[iPartIdx];
        uiCacheIdx = g_kuiCache30ScanIdx[iPartIdx];
        PredMv (iMvArray, iRefIdxArray, iPartIdx, iBlockWidth, iRefIdx[i], iMv);

        WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //mvd_l0[ mbPartIdx ][ subMbPartIdx ][ compIdx ]
        iMv[0] += iCode;
        WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //mvd_l1[ mbPartIdx ][ subMbPartIdx ][ compIdx ]
        iMv[1] += iCode;
        WELS_CHECK_SE_BOTH_WARNING (iMv[1], iMinVmv, iMaxVmv, "vertical mv");
        if (SUB_MB_TYPE_8x8 == uiSubMbType) {
          ST32 (pCurDqLayer->pMv[0][iMbXy][uiScan4Idx], LD32 (iMv));
          ST32 (pCurDqLayer->pMv[0][iMbXy][uiScan4Idx + 1], LD32 (iMv));
          ST32 (pCurDqLayer->pMv[0][iMbXy][uiScan4Idx + 4], LD32 (iMv));
          ST32 (pCurDqLayer->pMv[0][iMbXy][uiScan4Idx + 5], LD32 (iMv));
          ST32 (iMvArray[0][uiCacheIdx  ], LD32 (iMv));
          ST32 (iMvArray[0][uiCacheIdx + 1], LD32 (iMv));
          ST32 (iMvArray[0][uiCacheIdx + 6], LD32 (iMv));
          ST32 (iMvArray[0][uiCacheIdx + 7], LD32 (iMv));
        } else if (SUB_MB_TYPE_8x4 == uiSubMbType) {
          ST32 (pCurDqLayer->pMv[0][iMbXy][uiScan4Idx  ], LD32 (iMv));
          ST32 (pCurDqLayer->pMv[0][iMbXy][uiScan4Idx + 1], LD32 (iMv));
          ST32 (iMvArray[0][uiCacheIdx  ], LD32 (iMv));
          ST32 (iMvArray[0][uiCacheIdx + 1], LD32 (iMv));
        } else if (SUB_MB_TYPE_4x8 == uiSubMbType) {
          ST32 (pCurDqLayer->pMv[0][iMbXy][uiScan4Idx  ], LD32 (iMv));
          ST32 (pCurDqLayer->pMv[0][iMbXy][uiScan4Idx + 4], LD32 (iMv));
          ST32 (iMvArray[0][uiCacheIdx  ], LD32 (iMv));
          ST32 (iMvArray[0][uiCacheIdx + 6], LD32 (iMv));
        } else { //SUB_MB_TYPE_4x4 == uiSubMbType
          ST32 (pCurDqLayer->pMv[0][iMbXy][uiScan4Idx  ], LD32 (iMv));
          ST32 (iMvArray[0][uiCacheIdx  ], LD32 (iMv));
        }
      }
    }
  }
  break;
  default:
    break;
  }

  return 0;
}

} // namespace WelsDec
