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
#include "parse_mb_syn_cabac.h"

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

void PredMvBDirectSpatial(PDqLayer pCurLayer, int16_t iMvp[LIST_A][2], int8_t ref[LIST_A]) {
	bool bTopAvail, bLeftTopAvail, bRightTopAvail, bLeftAvail;
	int32_t iLeftTopType, iRightTopType, iTopType, iLeftType;
	int32_t iCurSliceIdc, iTopSliceIdc, iLeftTopSliceIdc, iRightTopSliceIdc, iLeftSliceIdc;
	int32_t iCurX, iCurY, iCurXy, iLeftXy, iTopXy = 0, iLeftTopXy = 0, iRightTopXy = 0;

	int8_t iLeftRef[LIST_A];
	int8_t iTopRef[LIST_A];
	int8_t iRightTopRef[LIST_A];
	int8_t iLeftTopRef[LIST_A];
	int8_t iDiagonalRef[LIST_A];
	int16_t iMvA[LIST_A][2], iMvB[LIST_A][2], iMvC[LIST_A][2], iMvD[LIST_A][2];

	iCurXy = pCurLayer->iMbXyIndex;
	iCurX = pCurLayer->iMbX;
	iCurY = pCurLayer->iMbY;
	iCurSliceIdc = pCurLayer->pSliceIdc[iCurXy];

	if (iCurX != 0) {
		iLeftXy = iCurXy - 1;
		iLeftSliceIdc = pCurLayer->pSliceIdc[iLeftXy];
		bLeftAvail = (iLeftSliceIdc == iCurSliceIdc);
	}
	else {
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
			bLeftTopAvail = (iLeftTopSliceIdc == iCurSliceIdc);
		}
		else {
			bLeftTopAvail = 0;
		}
		if (iCurX != (pCurLayer->iMbWidth - 1)) {
			iRightTopXy = iTopXy + 1;
			iRightTopSliceIdc = pCurLayer->pSliceIdc[iRightTopXy];
			bRightTopAvail = (iRightTopSliceIdc == iCurSliceIdc);
		}
		else {
			bRightTopAvail = 0;
		}
	}
	else {
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
	for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
	/*left*/
		if (bLeftAvail && IS_INTER(iLeftType)) {
			ST32(iMvA[listIdx], LD32(pCurLayer->pMv[listIdx][iLeftXy][3]));
			iLeftRef[listIdx] = pCurLayer->pRefIndex[listIdx][iLeftXy][3];
		}
		else {
			ST32(iMvA[listIdx], 0);
			if (0 == bLeftAvail) { //not available
				iLeftRef[listIdx] = REF_NOT_AVAIL;
			}
			else { //available but is intra mb type
				iLeftRef[listIdx] = REF_NOT_IN_LIST;
			}
		}

		if (REF_NOT_AVAIL == iLeftRef[listIdx] ||
			(iLeftRef[listIdx] >= REF_NOT_IN_LIST && 0 == *(int32_t*)iMvA[listIdx])) {
			ref[listIdx] = iLeftRef[listIdx];
			ST32(iMvp[listIdx], 0);
			continue;
		}

		/*top*/
		if (bTopAvail && IS_INTER(iTopType)) {
			ST32(iMvB[listIdx], LD32(pCurLayer->pMv[listIdx][iTopXy][12]));
			iTopRef[listIdx] = pCurLayer->pRefIndex[listIdx][iTopXy][12];
		}
		else {
			ST32(iMvB[listIdx], 0);
			if (0 == bTopAvail) { //not available
				iTopRef[listIdx] = REF_NOT_AVAIL;
			}
			else { //available but is intra mb type
				iTopRef[listIdx] = REF_NOT_IN_LIST;
			}
		}

		if (REF_NOT_AVAIL == iTopRef[listIdx] ||
			(iTopRef[listIdx] >= REF_NOT_IN_LIST && 0 == *(int32_t*)iMvB[listIdx])) {
			ref[listIdx] = iTopRef[listIdx];
			ST32(iMvp[listIdx], 0);
			continue;
		}

		/*right_top*/
		if (bRightTopAvail && IS_INTER(iRightTopType)) {
			ST32(iMvC[listIdx], LD32(pCurLayer->pMv[listIdx][iRightTopXy][12]));
			iRightTopRef[listIdx] = pCurLayer->pRefIndex[listIdx][iRightTopXy][12];
		}
		else {
			ST32(iMvC[listIdx], 0);
			if (0 == bRightTopAvail) { //not available
				iRightTopRef[listIdx] = REF_NOT_AVAIL;
			}
			else { //available but is intra mb type
				iRightTopRef[listIdx] = REF_NOT_IN_LIST;
			}
		}
		/*left_top*/
		if (bLeftTopAvail&& IS_INTER(iLeftTopType)) {
			ST32(iMvD[listIdx], LD32(pCurLayer->pMv[listIdx][iLeftTopXy][15]));
			iLeftTopRef[listIdx] = pCurLayer->pRefIndex[listIdx][iLeftTopXy][15];
		}
		else {
			ST32(iMvD[listIdx], 0);
			if (0 == bLeftTopAvail) { //not available
				iLeftTopRef[listIdx] = REF_NOT_AVAIL;
			}
			else { //available but is intra mb type
				iLeftTopRef[listIdx] = REF_NOT_IN_LIST;
			}
		}

		iDiagonalRef[listIdx] = iRightTopRef[listIdx];
		if (REF_NOT_AVAIL == iDiagonalRef[listIdx]) {
			iDiagonalRef[listIdx] = iLeftTopRef[listIdx];
			*(int32_t*)iMvC[listIdx] = *(int32_t*)iMvD[listIdx];
		}

		if (REF_NOT_AVAIL == iTopRef[listIdx] && REF_NOT_AVAIL == iDiagonalRef[listIdx] && iLeftRef[listIdx] >= REF_NOT_IN_LIST) {
			ref[listIdx] = iLeftRef[listIdx];
			ST32(iMvp[listIdx], LD32(iMvA[listIdx]));
			continue;
		}

		ref[listIdx] = WELS_MIN_POSITIVE(iLeftRef[listIdx], WELS_MIN_POSITIVE(iTopRef[listIdx], iRightTopRef[listIdx]));
		if (ref[listIdx] >= 0) {
			uint32_t match_count = (iLeftRef[listIdx] == ref[listIdx]) + (iTopRef[listIdx] == ref[listIdx]) + (iDiagonalRef[listIdx] == ref[listIdx]);
			if (match_count == 1) {
				if (iLeftRef[listIdx] == ref[listIdx]) {
					ST32(iMvp[listIdx], LD32(iMvA[listIdx]));
				}
				else if (iTopRef[listIdx] == ref[listIdx]) {
					ST32(iMvp[listIdx], LD32(iMvB[listIdx]));
				}
				else {
					ST32(iMvp[listIdx], LD32(iMvC[listIdx]));
				}
			}
			else {
				iMvp[listIdx][0] = WelsMedian(iMvA[listIdx][0], iMvB[listIdx][0], iMvC[listIdx][0]);
				iMvp[listIdx][1] = WelsMedian(iMvA[listIdx][1], iMvB[listIdx][1], iMvC[listIdx][1]);
			}
		}
		else {
			iMvp[listIdx][0] = 0;
			iMvp[listIdx][1] = 0;
			ref[listIdx] = REF_NOT_IN_LIST;
		}
	}
	if (ref[LIST_0] <= REF_NOT_IN_LIST && ref[LIST_1] <= REF_NOT_IN_LIST) {
		ref[LIST_0] = ref[LIST_1] = 0;
	}
}

void PredMvBDirectSpatial2(PWelsDecoderContext pCtx, int16_t iMvp[LIST_A][2], int8_t ref[LIST_A]) {
	PDqLayer pCurLayer = pCtx->pCurDqLayer;
	bool bTopAvail, bLeftTopAvail, bRightTopAvail, bLeftAvail;
	int32_t iLeftTopType, iRightTopType, iTopType, iLeftType;
	int32_t iCurSliceIdc, iTopSliceIdc, iLeftTopSliceIdc, iRightTopSliceIdc, iLeftSliceIdc;
	int32_t iCurX, iCurY, iCurXy, iLeftXy, iTopXy = 0, iLeftTopXy = 0, iRightTopXy = 0;

	int8_t iLeftRef[LIST_A];
	int8_t iTopRef[LIST_A];
	int8_t iRightTopRef[LIST_A];
	int8_t iLeftTopRef[LIST_A];
	int8_t iDiagonalRef[LIST_A];
	int16_t iMvA[LIST_A][2], iMvB[LIST_A][2], iMvC[LIST_A][2], iMvD[LIST_A][2];

	int32_t iMbXy = pCurLayer->iMbXyIndex;

	uint32_t is8x8 = IS_Inter_8x8(pCurLayer->pMbType[iMbXy]);
	MbType mbType = pCurLayer->pMbType[iMbXy];
	iCurXy = pCurLayer->iMbXyIndex;
	PPicture colocPic = pCtx->sRefPic.pRefList[LIST_1][0];
	MbType coloc_mbType = colocPic->pMbType[iMbXy];
	SubMbType sub_mb_type;
	if (IS_Inter_8x8(coloc_mbType) && !pCtx->pSps->bDirect8x8InferenceFlag) {
		sub_mb_type = SUB_MB_TYPE_4x4 | MB_TYPE_P0L0 | MB_TYPE_P0L1 | MB_TYPE_DIRECT;
		mbType |= MB_TYPE_8x8 | MB_TYPE_L0 | MB_TYPE_L1;
	}
	else if (!is8x8 && (IS_INTER_16x16(coloc_mbType) || IS_INTRA(coloc_mbType))) {
		sub_mb_type = SUB_MB_TYPE_8x8 | MB_TYPE_P0L0 | MB_TYPE_P0L1 | MB_TYPE_DIRECT;
		mbType |= MB_TYPE_16x16 | MB_TYPE_L0 | MB_TYPE_L1;
	}
	else {
		sub_mb_type = SUB_MB_TYPE_8x8 | MB_TYPE_P0L0 | MB_TYPE_P0L1 | MB_TYPE_DIRECT;
		mbType |= MB_TYPE_8x8 | MB_TYPE_L0 | MB_TYPE_L1;
	}

	iCurX = pCurLayer->iMbX;
	iCurY = pCurLayer->iMbY;
	iCurSliceIdc = pCurLayer->pSliceIdc[iCurXy];

	if (iCurX != 0) {
		iLeftXy = iCurXy - 1;
		iLeftSliceIdc = pCurLayer->pSliceIdc[iLeftXy];
		bLeftAvail = (iLeftSliceIdc == iCurSliceIdc);
	}
	else {
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
			bLeftTopAvail = (iLeftTopSliceIdc == iCurSliceIdc);
		}
		else {
			bLeftTopAvail = 0;
		}
		if (iCurX != (pCurLayer->iMbWidth - 1)) {
			iRightTopXy = iTopXy + 1;
			iRightTopSliceIdc = pCurLayer->pSliceIdc[iRightTopXy];
			bRightTopAvail = (iRightTopSliceIdc == iCurSliceIdc);
		}
		else {
			bRightTopAvail = 0;
		}
	}
	else {
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
	for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
		/*left*/
		if (bLeftAvail && IS_INTER(iLeftType)) {
			ST32(iMvA[listIdx], LD32(pCurLayer->pMv[listIdx][iLeftXy][3]));
			iLeftRef[listIdx] = pCurLayer->pRefIndex[listIdx][iLeftXy][3];
		}
		else {
			ST32(iMvA[listIdx], 0);
			if (0 == bLeftAvail) { //not available
				iLeftRef[listIdx] = REF_NOT_AVAIL;
			}
			else { //available but is intra mb type
				iLeftRef[listIdx] = REF_NOT_IN_LIST;
			}
		}

		/*top*/
		if (bTopAvail && IS_INTER(iTopType)) {
			ST32(iMvB[listIdx], LD32(pCurLayer->pMv[listIdx][iTopXy][12]));
			iTopRef[listIdx] = pCurLayer->pRefIndex[listIdx][iTopXy][12];
		}
		else {
			ST32(iMvB[listIdx], 0);
			if (0 == bTopAvail) { //not available
				iTopRef[listIdx] = REF_NOT_AVAIL;
			}
			else { //available but is intra mb type
				iTopRef[listIdx] = REF_NOT_IN_LIST;
			}
		}

		/*right_top*/
		if (bRightTopAvail && IS_INTER(iRightTopType)) {
			ST32(iMvC[listIdx], LD32(pCurLayer->pMv[listIdx][iRightTopXy][12]));
			iRightTopRef[listIdx] = pCurLayer->pRefIndex[listIdx][iRightTopXy][12];
		}
		else {
			ST32(iMvC[listIdx], 0);
			if (0 == bRightTopAvail) { //not available
				iRightTopRef[listIdx] = REF_NOT_AVAIL;
			}
			else { //available but is intra mb type
				iRightTopRef[listIdx] = REF_NOT_IN_LIST;
			}
		}
		/*left_top*/
		if (bLeftTopAvail&& IS_INTER(iLeftTopType)) {
			ST32(iMvD[listIdx], LD32(pCurLayer->pMv[listIdx][iLeftTopXy][15]));
			iLeftTopRef[listIdx] = pCurLayer->pRefIndex[listIdx][iLeftTopXy][15];
		}
		else {
			ST32(iMvD[listIdx], 0);
			if (0 == bLeftTopAvail) { //not available
				iLeftTopRef[listIdx] = REF_NOT_AVAIL;
			}
			else { //available but is intra mb type
				iLeftTopRef[listIdx] = REF_NOT_IN_LIST;
			}
		}

		iDiagonalRef[listIdx] = iRightTopRef[listIdx];
		if (REF_NOT_AVAIL == iDiagonalRef[listIdx]) {
			iDiagonalRef[listIdx] = iLeftTopRef[listIdx];
			*(int32_t*)iMvC[listIdx] = *(int32_t*)iMvD[listIdx];
		}

		ref[listIdx] = WELS_MIN_POSITIVE(iLeftRef[listIdx], WELS_MIN_POSITIVE(iTopRef[listIdx], iRightTopRef[listIdx]));
		if (ref[listIdx] >= 0) {
			uint32_t match_count = (iLeftRef[listIdx] == ref[listIdx]) + (iTopRef[listIdx] == ref[listIdx]) + (iDiagonalRef[listIdx] == ref[listIdx]);
			if (match_count == 1) {
				if (iLeftRef[listIdx] == ref[listIdx]) {
					ST32(iMvp[listIdx], LD32(iMvA[listIdx]));
				}
				else if (iTopRef[listIdx] == ref[listIdx]) {
					ST32(iMvp[listIdx], LD32(iMvB[listIdx]));
				}
				else {
					ST32(iMvp[listIdx], LD32(iMvC[listIdx]));
				}
			}
			else {
				iMvp[listIdx][0] = WelsMedian(iMvA[listIdx][0], iMvB[listIdx][0], iMvC[listIdx][0]);
				iMvp[listIdx][1] = WelsMedian(iMvA[listIdx][1], iMvB[listIdx][1], iMvC[listIdx][1]);
			}
		}
		else {
			iMvp[listIdx][0] = 0;
			iMvp[listIdx][1] = 0;
			ref[listIdx] = REF_NOT_IN_LIST;
		}
	}
	if (ref[LIST_0] <= REF_NOT_IN_LIST && ref[LIST_1] <= REF_NOT_IN_LIST) {
		ref[LIST_0] = ref[LIST_1] = 0;
	}
	else if (ref[1] <= REF_NOT_IN_LIST) {
		mbType &= ~MB_TYPE_L1;
	}
	else if (ref[0] <= REF_NOT_IN_LIST) {
		mbType &= ~MB_TYPE_L0;
	}
	pCurLayer->pMbType[iMbXy] = mbType;

	if (IS_INTER_16x16(mbType)) {
	}
	else {
		int8_t pSubPartCount[4], pPartW[4];
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			for (int32_t i = 0; i < 4; i++) { //Direct 8x8 Ref and mv
				pCurLayer->pSubMbType[iMbXy][i] = sub_mb_type;
				int16_t iIdx8 = i << 2;

				int8_t pRefIndex[LIST_A][30];
				int16_t pMvd[4] = { 0 };
				UpdateP8x8RefIdxCabac(pCurLayer, pRefIndex, iIdx8, ref[listIdx], listIdx);

				pSubPartCount[i] = g_ksInterBSubMbTypeInfo[12].iPartCount;
				pPartW[i] = g_ksInterBSubMbTypeInfo[12].iPartWidth;

				int8_t iPartCount = pSubPartCount[i];
				int16_t iPartIdx, iBlockW = pPartW[i];
				uint8_t iScan4Idx, iCacheIdx;
				iCacheIdx = g_kuiCache30ScanIdx[i << 2];

				for (int32_t j = 0; j < iPartCount; j++) {
					iPartIdx = (i << 2) + j * iBlockW;
					iScan4Idx = g_kuiScan4[iPartIdx];
					iCacheIdx = g_kuiCache30ScanIdx[iPartIdx];

					ST32(pCurLayer->pMv[listIdx][iMbXy][iScan4Idx], LD32(iMvp));
					ST32(pCurLayer->pMvd[listIdx][iMbXy][iScan4Idx], LD32(pMvd));
					//					ST32(pMotionVector[listIdx][iCacheIdx], LD32(iMvp));
					//					ST32(pMvdCache[listIdx][iCacheIdx], LD32(pMvd));
				}
			}
			// the motion vectors equal to '0' if ref_idx==0 && coloc_zero_flag is true
/*			if (0 == sHDecCtx->coloc_intra_cache[coloc_index] && 0 == l1_long_term_reference_flag
				&& (sHDecCtx->coloc_refidx_cache[0][coloc_index] == 0 || (sHDecCtx->coloc_refidx_cache[0][coloc_index] < 0 && sHDecCtx->coloc_refidx_cache[1][coloc_index] == 0))) {
					const int16_t(*mv_coloc)[2] = 0 == sHDecCtx->coloc_refidx_cache[0][coloc_index] ? sHDecCtx->coloc_motion_cache[0] : sHDecCtx->coloc_motion_cache[1];
			{	
				if (IS_SUB_8x8(sub_mb_type)) {
						const int16_t *mv = mv_coloc[coloc_index];
						if ((unsigned)(mv[0] + 1) <= 2 && (unsigned)(mv[1] + 1) <= 2) {
							if (0 == ref[0])	SetRectBlock(&sHDecCtx->motion_cache[0][2 + scan_index], 2, 2, MB_CACHE_STRIDE * 4, 0, 4);
							if (0 == ref[1])	SetRectBlock(&sHDecCtx->motion_cache[1][2 + scan_index], 2, 2, MB_CACHE_STRIDE * 4, 0, 4);
						}
					}
					else {
						for (b4 = 0; b4<4; b4++) {
							const int32_t scan_index = scan_cache[b8 * 4 + b4];
							const int16_t *mv = mv_coloc[scan_block[b8 * 4 + b4]];
							if ((unsigned)(mv[0] + 1) <= 2 && (unsigned)(mv[1] + 1) <= 2) {
								if (0 == ref[0])	*(uint32_t*)sHDecCtx->motion_cache[0][2 + scan_index] = 0;
								if (0 == ref[1])	*(uint32_t*)sHDecCtx->motion_cache[1][2 + scan_index] = 0;
							}
						}
					}
				}
			}*/
		}
	}
}

void PredBDirectTemporal(PWelsDecoderContext pCtx, int16_t iMvp[LIST_A][2], int8_t ref[LIST_A]) {
	PDqLayer pCurLayer = pCtx->pCurDqLayer;
	PSlice pSlice = &pCurLayer->sLayerInfo.sSliceInLayer;
	PSliceHeader pSliceHeader = &pSlice->sSliceHeaderExt.sSliceHeader;
	int32_t iMbXy = pCurLayer->iMbXyIndex;
	uint32_t uiShortRefCount = pCtx->sRefPic.uiShortRefCount[LIST_0];
	for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
		for (uint32_t refIdx = 0; refIdx < uiShortRefCount; ++refIdx) {
			ref[listIdx] = refIdx;
			iMvp[listIdx][0] = pCtx->sRefPic.pRefList[LIST_1][0]->pMv[LIST_1][iMbXy][0][0] * pSlice->iMvScale[listIdx][refIdx];
			iMvp[listIdx][1] = pCtx->sRefPic.pRefList[LIST_1][0]->pMv[LIST_1][iMbXy][0][1] * pSlice->iMvScale[listIdx][refIdx];
			break;
		}
	}
}

//basic iMVs prediction unit for iMVs partition width (4, 2, 1)
void PredMv (int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30], 
	int32_t listIdx, int32_t iPartIdx, int32_t iPartWidth, int8_t iRef, int16_t iMVP[2]) {
  const uint8_t kuiLeftIdx      = g_kuiCache30ScanIdx[iPartIdx] - 1;
  const uint8_t kuiTopIdx       = g_kuiCache30ScanIdx[iPartIdx] - 6;
  const uint8_t kuiRightTopIdx  = kuiTopIdx + iPartWidth;
  const uint8_t kuiLeftTopIdx   = kuiTopIdx - 1;

  const int8_t kiLeftRef      = iRefIndex[listIdx][kuiLeftIdx];
  const int8_t kiTopRef       = iRefIndex[listIdx][ kuiTopIdx];
  const int8_t kiRightTopRef  = iRefIndex[listIdx][kuiRightTopIdx];
  const int8_t kiLeftTopRef   = iRefIndex[listIdx][ kuiLeftTopIdx];
  int8_t iDiagonalRef  = kiRightTopRef;

  int8_t iMatchRef = 0;


  int16_t iAMV[2], iBMV[2], iCMV[2];

  ST32 (iAMV, LD32 (iMotionVector[listIdx][     kuiLeftIdx]));
  ST32 (iBMV, LD32 (iMotionVector[listIdx][      kuiTopIdx]));
  ST32 (iCMV, LD32 (iMotionVector[listIdx][kuiRightTopIdx]));

  if (REF_NOT_AVAIL == iDiagonalRef) {
    iDiagonalRef = kiLeftTopRef;
    ST32 (iCMV, LD32 (iMotionVector[listIdx][kuiLeftTopIdx]));
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
											int32_t listIdx, int32_t iPartIdx, int8_t iRef, int16_t iMVP[2]) {
  if (0 == iPartIdx) {
    const int8_t kiLeftRef = iRefIndex[listIdx][6];
    if (iRef == kiLeftRef) {
      ST32 (iMVP, LD32 (&iMotionVector[listIdx][6][0]));
      return;
    }
  } else { // 1 == iPartIdx
    int8_t iDiagonalRef = iRefIndex[listIdx][5]; //top-right
    int8_t index = 5;
    if (REF_NOT_AVAIL == iDiagonalRef) {
      iDiagonalRef = iRefIndex[listIdx][2]; //top-left for 8*8 block(index 1)
      index = 2;
    }
    if (iRef == iDiagonalRef) {
      ST32 (iMVP, LD32 (&iMotionVector[listIdx][index][0]));
      return;
    }
  }

  PredMv (iMotionVector, iRefIndex, listIdx, iPartIdx, 2, iRef, iMVP);
}
void PredInter16x8Mv (int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
											int32_t listIdx, int32_t iPartIdx, int8_t iRef, int16_t iMVP[2]) {
  if (0 == iPartIdx) {
    const int8_t kiTopRef = iRefIndex[listIdx][1];
    if (iRef == kiTopRef) {
      ST32 (iMVP, LD32 (&iMotionVector[listIdx][1][0]));
      return;
    }
  } else { // 8 == iPartIdx
    const int8_t kiLeftRef = iRefIndex[listIdx][18];
    if (iRef == kiLeftRef) {
      ST32 (iMVP, LD32 (&iMotionVector[listIdx][18][0]));
      return;
    }
  }

  PredMv (iMotionVector, iRefIndex, listIdx, iPartIdx, 4, iRef, iMVP);
}

//update iMVs and iRefIndex cache for current MB, only for P_16*16 (SKIP inclusive)
/* can be further optimized */
void UpdateP16x16MotionInfo (PDqLayer pCurDqLayer, int32_t listIdx, int8_t iRef, int16_t iMVs[2]) {
  const int16_t kiRef2 = (iRef << 8) | iRef;
  const int32_t kiMV32 = LD32 (iMVs);
  int32_t i;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;

  for (i = 0; i < 16; i += 4) {
    //mb
    const uint8_t kuiScan4Idx = g_kuiScan4[i];
    const uint8_t kuiScan4IdxPlus4 = 4 + kuiScan4Idx;

    ST16 (&pCurDqLayer->pRefIndex[listIdx][iMbXy][kuiScan4Idx ], kiRef2);
    ST16 (&pCurDqLayer->pRefIndex[listIdx][iMbXy][kuiScan4IdxPlus4], kiRef2);

    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][  kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][1 + kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][  kuiScan4IdxPlus4], kiMV32);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][1 + kuiScan4IdxPlus4], kiMV32);
  }
}

//update iRefIndex and iMVs of Mb, only for P16x8
/*need further optimization, mb_cache not work */
void UpdateP16x8MotionInfo (PDqLayer pCurDqLayer, int16_t iMotionVector[LIST_A][30][MV_A], 
														int8_t iRefIndex[LIST_A][30],
														int32_t listIdx, int32_t iPartIdx, int8_t iRef, int16_t iMVs[2]) {
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
    ST16 (&pCurDqLayer->pRefIndex[listIdx][iMbXy][kuiScan4Idx ], kiRef2);
    ST16 (&pCurDqLayer->pRefIndex[listIdx][iMbXy][kuiScan4IdxPlus4], kiRef2);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][  kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][1 + kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][  kuiScan4IdxPlus4], kiMV32);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][1 + kuiScan4IdxPlus4], kiMV32);
    //cache
    ST16 (&iRefIndex[listIdx][kuiCacheIdx ], kiRef2);
    ST16 (&iRefIndex[listIdx][kuiCacheIdxPlus6], kiRef2);
    ST32 (iMotionVector[listIdx][  kuiCacheIdx ], kiMV32);
    ST32 (iMotionVector[listIdx][1 + kuiCacheIdx ], kiMV32);
    ST32 (iMotionVector[listIdx][  kuiCacheIdxPlus6], kiMV32);
    ST32 (iMotionVector[listIdx][1 + kuiCacheIdxPlus6], kiMV32);
  }
}
//update iRefIndex and iMVs of both Mb and Mb_cache, only for P8x16
void UpdateP8x16MotionInfo (PDqLayer pCurDqLayer, int16_t iMotionVector[LIST_A][30][MV_A], 
														int8_t iRefIndex[LIST_A][30], 
														int32_t listIdx, int32_t iPartIdx, int8_t iRef, int16_t iMVs[2]) {
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
    ST16 (&pCurDqLayer->pRefIndex[listIdx][iMbXy][kuiScan4Idx ], kiRef2);
    ST16 (&pCurDqLayer->pRefIndex[listIdx][iMbXy][kuiScan4IdxPlus4], kiRef2);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][  kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][1 + kuiScan4Idx ], kiMV32);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][  kuiScan4IdxPlus4], kiMV32);
    ST32 (pCurDqLayer->pMv[listIdx][iMbXy][1 + kuiScan4IdxPlus4], kiMV32);
    //cache
    ST16 (&iRefIndex[listIdx][kuiCacheIdx ], kiRef2);
    ST16 (&iRefIndex[listIdx][kuiCacheIdxPlus6], kiRef2);
    ST32 (iMotionVector[listIdx][  kuiCacheIdx ], kiMV32);
    ST32 (iMotionVector[listIdx][1 + kuiCacheIdx ], kiMV32);
    ST32 (iMotionVector[listIdx][  kuiCacheIdxPlus6], kiMV32);
    ST32 (iMotionVector[listIdx][1 + kuiCacheIdxPlus6], kiMV32);
  }
}

} // namespace WelsDec
