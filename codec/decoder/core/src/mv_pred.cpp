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

	static inline  void SetRectBlock(void *vp, int32_t w, const int32_t h, int32_t stride, const uint32_t val, const int32_t size) {
		uint8_t *p = (uint8_t*)vp;
		w *= size;
		if (w == 1 && h == 4) {
			*(uint8_t*)(p + 0 * stride) =
				*(uint8_t*)(p + 1 * stride) =
				*(uint8_t*)(p + 2 * stride) =
				*(uint8_t*)(p + 3 * stride) = (uint8_t)val;
		}
		else if (w == 2 && h == 2) {
			*(uint16_t*)(p + 0 * stride) =
				*(uint16_t*)(p + 1 * stride) = size == 4 ? (uint16_t)val : (uint16_t)(val * 0x0101U);
		}
		else if (w == 2 && h == 4) {
			*(uint16_t*)(p + 0 * stride) =
				*(uint16_t*)(p + 1 * stride) =
				*(uint16_t*)(p + 2 * stride) =
				*(uint16_t*)(p + 3 * stride) = size == 4 ? (uint16_t)val : (uint16_t)(val * 0x0101U);
		}
		else if (w == 4 && h == 2) {
			*(uint32_t*)(p + 0 * stride) =
				*(uint32_t*)(p + 1 * stride) = size == 4 ? val : (uint32_t)(val * 0x01010101UL);
		}
		else if (w == 4 && h == 4) {
			*(uint32_t*)(p + 0 * stride) =
				*(uint32_t*)(p + 1 * stride) =
				*(uint32_t*)(p + 2 * stride) =
				*(uint32_t*)(p + 3 * stride) = size == 4 ? val : (uint32_t)(val * 0x01010101UL);
		}
		else if (w == 8 && h == 1) {
			*(uint32_t*)(p + 0 * stride) =
				*(uint32_t*)(p + 0 * stride + 4) = size == 4 ? val : (uint32_t)(val * 0x01010101UL);
		}
		else if (w == 8 && h == 2) {
			*(uint32_t*)(p + 0 * stride) =
				*(uint32_t*)(p + 0 * stride + 4) =
				*(uint32_t*)(p + 1 * stride) =
				*(uint32_t*)(p + 1 * stride + 4) = size == 4 ? val : (uint32_t)(val * 0x01010101UL);
		}
		else if (w == 8 && h == 4) {
			*(uint32_t*)(p + 0 * stride) =
				*(uint32_t*)(p + 0 * stride + 4) =
				*(uint32_t*)(p + 1 * stride) =
				*(uint32_t*)(p + 1 * stride + 4) =
				*(uint32_t*)(p + 2 * stride) =
				*(uint32_t*)(p + 2 * stride + 4) =
				*(uint32_t*)(p + 3 * stride) =
				*(uint32_t*)(p + 3 * stride + 4) = size == 4 ? val : (uint32_t)(val * 0x01010101UL);
		}
		else if (w == 16 && h == 2) {
			*(uint32_t*)(p + 0 * stride + 0) =
				*(uint32_t*)(p + 0 * stride + 4) =
				*(uint32_t*)(p + 0 * stride + 8) =
				*(uint32_t*)(p + 0 * stride + 12) =
				*(uint32_t*)(p + 1 * stride + 0) =
				*(uint32_t*)(p + 1 * stride + 4) =
				*(uint32_t*)(p + 1 * stride + 8) =
				*(uint32_t*)(p + 1 * stride + 12) = size == 4 ? val : (uint32_t)(val * 0x01010101UL);
		}
		else if (w == 16 && h == 3) {
			*(uint32_t*)(p + 0 * stride + 0) =
				*(uint32_t*)(p + 0 * stride + 4) =
				*(uint32_t*)(p + 0 * stride + 8) =
				*(uint32_t*)(p + 0 * stride + 12) =
				*(uint32_t*)(p + 1 * stride + 0) =
				*(uint32_t*)(p + 1 * stride + 4) =
				*(uint32_t*)(p + 1 * stride + 8) =
				*(uint32_t*)(p + 1 * stride + 12) =
				*(uint32_t*)(p + 2 * stride + 0) =
				*(uint32_t*)(p + 2 * stride + 4) =
				*(uint32_t*)(p + 2 * stride + 8) =
				*(uint32_t*)(p + 2 * stride + 12) = size == 4 ? val : (uint32_t)(val * 0x01010101UL);
		}
		else if (w == 16 && h == 4) {
			*(uint32_t*)(p + 0 * stride + 0) =
				*(uint32_t*)(p + 0 * stride + 4) =
				*(uint32_t*)(p + 0 * stride + 8) =
				*(uint32_t*)(p + 0 * stride + 12) =
				*(uint32_t*)(p + 1 * stride + 0) =
				*(uint32_t*)(p + 1 * stride + 4) =
				*(uint32_t*)(p + 1 * stride + 8) =
				*(uint32_t*)(p + 1 * stride + 12) =
				*(uint32_t*)(p + 2 * stride + 0) =
				*(uint32_t*)(p + 2 * stride + 4) =
				*(uint32_t*)(p + 2 * stride + 8) =
				*(uint32_t*)(p + 2 * stride + 12) =
				*(uint32_t*)(p + 3 * stride + 0) =
				*(uint32_t*)(p + 3 * stride + 4) =
				*(uint32_t*)(p + 3 * stride + 8) =
				*(uint32_t*)(p + 3 * stride + 12) = size == 4 ? val : (uint32_t)(val * 0x01010101UL);
		}
	}
	void CopyRectBlock4Cols(void *vdst, void *vsrc, const int32_t stride_dst, const int32_t stride_src, int32_t w, const int32_t size) {
		uint8_t *dst = (uint8_t*)vdst;
		uint8_t *src = (uint8_t*)vsrc;
		w *= size;
		if (w == 1) {
			dst[stride_dst * 0] = src[stride_src * 0];
			dst[stride_dst * 1] = src[stride_src * 1];
			dst[stride_dst * 2] = src[stride_src * 2];
			dst[stride_dst * 3] = src[stride_src * 3];
		}
		else if (w == 2) {
			*(uint16_t*)(&dst[stride_dst * 0]) = *(uint16_t*)(&src[stride_src * 0]);
			*(uint16_t*)(&dst[stride_dst * 1]) = *(uint16_t*)(&src[stride_src * 1]);
			*(uint16_t*)(&dst[stride_dst * 2]) = *(uint16_t*)(&src[stride_src * 2]);
			*(uint16_t*)(&dst[stride_dst * 3]) = *(uint16_t*)(&src[stride_src * 3]);
		}
		else if (w == 4) {
			*(uint32_t*)(&dst[stride_dst * 0]) = *(uint32_t*)(&src[stride_src * 0]);
			*(uint32_t*)(&dst[stride_dst * 1]) = *(uint32_t*)(&src[stride_src * 1]);
			*(uint32_t*)(&dst[stride_dst * 2]) = *(uint32_t*)(&src[stride_src * 2]);
			*(uint32_t*)(&dst[stride_dst * 3]) = *(uint32_t*)(&src[stride_src * 3]);
		}
		else if (w == 16) {
			memcpy(&dst[stride_dst * 0], &src[stride_src * 0], 16);
			memcpy(&dst[stride_dst * 1], &src[stride_src * 1], 16);
			memcpy(&dst[stride_dst * 2], &src[stride_src * 2], 16);
			memcpy(&dst[stride_dst * 3], &src[stride_src * 3], 16);
		}
	}
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

		int8_t tmp = WELS_MIN_POSITIVE(iTopRef[listIdx], iRightTopRef[listIdx]);
		ref[listIdx] = WELS_MIN_POSITIVE(iLeftRef[listIdx], tmp);
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

SubMbType PredMvBDirectSpatial2(PWelsDecoderContext pCtx, int16_t iMvp[LIST_A][2], int8_t ref[LIST_A]) {
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
	bool bSkipOrDirect = (IS_SKIP(pCurLayer->pMbType[iMbXy]) | IS_DIRECT(pCurLayer->pMbType[iMbXy])) > 0;
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

		int8_t ref_temp = WELS_MIN_POSITIVE(iTopRef[listIdx], iDiagonalRef[listIdx]);
		ref[listIdx] = WELS_MIN_POSITIVE(iLeftRef[listIdx], ref_temp);
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
	else if (ref[LIST_1] < 0) {
		mbType &= ~MB_TYPE_L1;
		sub_mb_type &= ~MB_TYPE_L1;
	}
	else if (ref[LIST_0] < 0) {
		mbType &= ~MB_TYPE_L0;
		sub_mb_type &= ~MB_TYPE_L0;
	}
	pCurLayer->pMbType[iMbXy] = mbType;


	if (IS_INTRA(coloc_mbType)) {
		SetRectBlock(pCurLayer->iColocIntra, 4, 4, 4 * sizeof(int8_t), 1, sizeof(int8_t));
	}
	else {
		SetRectBlock(pCurLayer->iColocIntra, 4, 4, 4 * sizeof(int8_t), 0, sizeof(int8_t));
	}
	int16_t pMvd[4] = { 0 };
	/*if (!(is8x8 | *(int32_t*)iMvp[LIST_0] | *(int32_t*)iMvp[LIST_1])) {
		for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
			UpdateP16x16MotionInfo(pCurLayer, listIdx, ref[listIdx], iMvp[listIdx]);
			UpdateP16x16MvdCabac(pCurLayer, pMvd, listIdx);
		}
	}
	else
*/
	{
		if (IS_INTER_16x16(mbType)) {
			int16_t iMVZero[2] = { 0 };
			int16_t * pMv = IS_TYPE_L1(coloc_mbType) ? colocPic->pMv[LIST_1][iMbXy][0] : iMVZero;
			ST32(pCurLayer->iColocMv[LIST_0][0], LD32(colocPic->pMv[LIST_0][iMbXy][0]));
			ST32(pCurLayer->iColocMv[LIST_1][0], LD32(pMv));
			pCurLayer->iColocRefIndex[LIST_0][0] = colocPic->pRefIndex[LIST_0][iMbXy][0];
			pCurLayer->iColocRefIndex[LIST_1][0] = IS_TYPE_L1(coloc_mbType) ? colocPic->pRefIndex[LIST_1][iMbXy][0] : REF_NOT_IN_LIST;
			if (0 == pCurLayer->iColocIntra[0] && !colocPic->bIsLongRef
				&& ((pCurLayer->iColocRefIndex[LIST_0][0] == 0 && (unsigned)(pCurLayer->iColocMv[LIST_0][0][0] + 1) <= 2 && (unsigned)(pCurLayer->iColocMv[LIST_0][0][1] + 1) <= 2)
					|| (pCurLayer->iColocRefIndex[LIST_0][0] <0 && pCurLayer->iColocRefIndex[LIST_1][0] == 0 && (unsigned)(pCurLayer->iColocMv[LIST_1][0][0] + 1) <= 2 && (unsigned)(pCurLayer->iColocMv[LIST_1][0][1] + 1) <= 2))) {
				if (0 >= ref[0])	*(uint32_t*)iMvp[LIST_0] = 0;
				if (0 >= ref[1])	*(uint32_t*)iMvp[LIST_1] = 0;
			}
			for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
				UpdateP16x16MotionInfo(pCurLayer, listIdx, ref[listIdx], iMvp[listIdx]);
				UpdateP16x16MvdCabac(pCurLayer, pMvd, listIdx);
			}
		}
		else {
			if (!pCtx->pSps->bDirect8x8InferenceFlag) {
				CopyRectBlock4Cols(pCurLayer->iColocMv[LIST_0], colocPic->pMv[LIST_0][iMbXy], 16, 16, 4, 4);
				CopyRectBlock4Cols(pCurLayer->iColocRefIndex[LIST_0], colocPic->pRefIndex[LIST_0][iMbXy], 4, 4, 4, 1);
				if (IS_TYPE_L1(coloc_mbType)) {
					CopyRectBlock4Cols(pCurLayer->iColocMv[LIST_1], colocPic->pMv[LIST_1][iMbXy], 16, 16, 4, 4);
					CopyRectBlock4Cols(pCurLayer->iColocRefIndex[LIST_1], colocPic->pRefIndex[LIST_1][iMbXy], 4, 4, 4, 1);
				}
				else {// only forward prediction
					SetRectBlock(pCurLayer->iColocRefIndex[LIST_1], 4, 4, 4, (uint8_t)REF_NOT_IN_LIST, 1);
				}
			}
			else {
				for (int32_t listIdx = 0; listIdx < 1 + !!(coloc_mbType & MB_TYPE_L1); listIdx++) {
					SetRectBlock(&pCurLayer->iColocRefIndex[listIdx][0], 2, 2, 4, colocPic->pRefIndex[listIdx][iMbXy][0], 1);
					SetRectBlock(&pCurLayer->iColocRefIndex[listIdx][2], 2, 2, 4, colocPic->pRefIndex[listIdx][iMbXy][3], 1);
					SetRectBlock(&pCurLayer->iColocRefIndex[listIdx][8], 2, 2, 4, colocPic->pRefIndex[listIdx][iMbXy][12], 1);
					SetRectBlock(&pCurLayer->iColocRefIndex[listIdx][10], 2, 2, 4, colocPic->pRefIndex[listIdx][iMbXy][15], 1);

					SetRectBlock(pCurLayer->iColocMv[listIdx][0], 2, 2, 16, LD32(colocPic->pMv[listIdx][iMbXy][0]), 4);
					SetRectBlock(pCurLayer->iColocMv[listIdx][2], 2, 2, 16, LD32(colocPic->pMv[listIdx][iMbXy][3]), 4);
					SetRectBlock(pCurLayer->iColocMv[listIdx][8], 2, 2, 16, LD32(colocPic->pMv[listIdx][iMbXy][12]), 4);
					SetRectBlock(pCurLayer->iColocMv[listIdx][10], 2, 2, 16, LD32(colocPic->pMv[listIdx][iMbXy][15]), 4);
				}
				if (!(coloc_mbType & MB_TYPE_L1))// only forward prediction
					SetRectBlock(&pCurLayer->iColocRefIndex[1][0], 4, 4, 4, (uint8_t)REF_NOT_IN_LIST, 1);
			}
			if (bSkipOrDirect) {
				int8_t pSubPartCount[4], pPartW[4];
				for (int32_t i = 0; i < 4; i++) { //Direct 8x8 Ref and mv
					int16_t iIdx8 = i << 2;
					pCurLayer->pSubMbType[iMbXy][i] = sub_mb_type;
					int8_t pRefIndex[LIST_A][30];
					UpdateP8x8RefIdxCabac(pCurLayer, pRefIndex, iIdx8, ref[LIST_0], LIST_0);
					UpdateP8x8RefIdxCabac(pCurLayer, pRefIndex, iIdx8, ref[LIST_1], LIST_1);

					pSubPartCount[i] = g_ksInterBSubMbTypeInfo[0].iPartCount;
					pPartW[i] = g_ksInterBSubMbTypeInfo[0].iPartWidth;

					if (IS_SUB_4x4(sub_mb_type)) {
						pSubPartCount[i] = 4;
						pPartW[i] = 1;
					}

					int8_t iPartCount = pSubPartCount[i];
					int16_t iPartIdx, iBlockW = pPartW[i];
					uint8_t iScan4Idx, iCacheIdx, iColocIdx;
					iCacheIdx = g_kuiCache30ScanIdx[iIdx8];

					for (int32_t j = 0; j < iPartCount; j++) {
						iPartIdx = iIdx8 + j * iBlockW;
						iScan4Idx = g_kuiScan4[iPartIdx];
						iColocIdx = g_kuiScan4[iPartIdx];
						iCacheIdx = g_kuiCache30ScanIdx[iPartIdx];

						int16_t pMV[4] = { 0 };
						if (IS_SUB_8x8(sub_mb_type)) {
							*(uint32_t*)pMV = *(uint32_t*)iMvp[LIST_0];
							ST32((pMV + 2), LD32(pMV));
							ST32((pMvd + 2), LD32(pMvd));
							ST64(pCurLayer->pMv[LIST_0][iMbXy][iScan4Idx], LD64(pMV));
							ST64(pCurLayer->pMv[LIST_0][iMbXy][iScan4Idx + 4], LD64(pMV));
							ST64(pCurLayer->pMvd[LIST_0][iMbXy][iScan4Idx], LD64(pMvd));
							ST64(pCurLayer->pMvd[LIST_0][iMbXy][iScan4Idx + 4], LD64(pMvd));
							*(uint32_t*)pMV = *(uint32_t*)iMvp[LIST_1];
							ST32((pMV + 2), LD32(pMV));
							ST32((pMvd + 2), LD32(pMvd));
							ST64(pCurLayer->pMv[LIST_1][iMbXy][iScan4Idx], LD64(pMV));
							ST64(pCurLayer->pMv[LIST_1][iMbXy][iScan4Idx + 4], LD64(pMV));
							ST64(pCurLayer->pMvd[LIST_1][iMbXy][iScan4Idx], LD64(pMvd));
							ST64(pCurLayer->pMvd[LIST_1][iMbXy][iScan4Idx + 4], LD64(pMvd));
						}
						else { //SUB_4x4
							*(uint32_t*)pMV = *(uint32_t*)iMvp[LIST_0];
							ST32(pCurLayer->pMv[LIST_0][iMbXy][iScan4Idx], LD32(pMV));
							ST32(pCurLayer->pMvd[LIST_0][iMbXy][iScan4Idx], LD32(pMvd));
							*(uint32_t*)pMV = *(uint32_t*)iMvp[LIST_1];
							ST32(pCurLayer->pMv[LIST_1][iMbXy][iScan4Idx], LD32(pMV));
							ST32(pCurLayer->pMvd[LIST_1][iMbXy][iScan4Idx], LD32(pMvd));
						}
						uint32_t uiColZeroFlag = (0 == pCurLayer->iColocIntra[iColocIdx]) && !colocPic->bIsLongRef &&
							(pCurLayer->iColocRefIndex[LIST_0][iColocIdx] == 0 || (pCurLayer->iColocRefIndex[LIST_0][iColocIdx] < 0 && pCurLayer->iColocRefIndex[LIST_1][iColocIdx] == 0));
						const int16_t(*mvColoc)[2] = 0 == pCurLayer->iColocRefIndex[LIST_0][iColocIdx] ? pCurLayer->iColocMv[LIST_0] : pCurLayer->iColocMv[LIST_1];
						const int16_t *mv = mvColoc[iColocIdx];
						if (IS_SUB_8x8(sub_mb_type)) {
							if (uiColZeroFlag && ((unsigned)(mv[0] + 1) <= 2 && (unsigned)(mv[1] + 1) <= 2)) {
								*(uint32_t*)pMV = 0;
								if (ref[LIST_0] == 0) {
									ST32((pMV + 2), LD32(pMV));
									ST32((pMvd + 2), LD32(pMvd));
									ST64(pCurLayer->pMv[LIST_0][iMbXy][iScan4Idx], LD64(pMV));
									ST64(pCurLayer->pMv[LIST_0][iMbXy][iScan4Idx + 4], LD64(pMV));
									ST64(pCurLayer->pMvd[LIST_0][iMbXy][iScan4Idx], LD64(pMvd));
									ST64(pCurLayer->pMvd[LIST_0][iMbXy][iScan4Idx + 4], LD64(pMvd));
								}

								if (ref[LIST_1] == 0) {
									*(uint32_t*)pMV = 0;
									ST32((pMV + 2), LD32(pMV));
									ST32((pMvd + 2), LD32(pMvd));
									ST64(pCurLayer->pMv[LIST_1][iMbXy][iScan4Idx], LD64(pMV));
									ST64(pCurLayer->pMv[LIST_1][iMbXy][iScan4Idx + 4], LD64(pMV));
									ST64(pCurLayer->pMvd[LIST_1][iMbXy][iScan4Idx], LD64(pMvd));
									ST64(pCurLayer->pMvd[LIST_1][iMbXy][iScan4Idx + 4], LD64(pMvd));
								}
							}
						}
						else {
							if (uiColZeroFlag && ((unsigned)(mv[0] + 1) <= 2 && (unsigned)(mv[1] + 1) <= 2)) {
								if (ref[LIST_0] == 0) {
									*(uint32_t*)pMV = 0;
									ST32(pCurLayer->pMv[LIST_0][iMbXy][iScan4Idx], LD32(pMV));
									ST32(pCurLayer->pMvd[LIST_0][iMbXy][iScan4Idx], LD32(pMvd));
								}
								if (ref[LIST_1] == 0) {
									*(uint32_t*)pMV = 0;
									ST32(pCurLayer->pMv[LIST_1][iMbXy][iScan4Idx], LD32(pMV));
									ST32(pCurLayer->pMvd[LIST_1][iMbXy][iScan4Idx], LD32(pMvd));
								}
							}
						}
					}
				}
			}
		}
	}
	return sub_mb_type;
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
