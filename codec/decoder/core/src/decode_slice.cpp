/*!
 * \copy
 *     Copyright (c)  2008-2013, Cisco Systems
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
 *  Abstract
 *      current slice decoding
 *
 *  History
 *      07/10/2008 Created
 *      08/09/2013 Modified
 *
 *****************************************************************************/
#include <memory.h>

#include "typedefs.h"
#include "dec_golomb.h"

#include "fmo.h"
#include "deblocking.h"
#include "utils.h"

#include "decode_slice.h"

#include "error_code.h"
#include "decode_mb_aux.h"
#include "parse_mb_syn_cavlc.h"
#include "rec_mb.h"
#include "mv_pred.h"

#include "as264_common.h"
#include "cpu_core.h"
#include "expand_pic.h"

namespace WelsDec {

int32_t WelsTargetSliceConstruction (PWelsDecoderContext pCtx) {
  int32_t iPreQP = 0;

  PDqLayer pCurLayer = pCtx->pCurDqLayer;
  PSlice pCurSlice = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader = &pCurSlice->sSliceHeaderExt.sSliceHeader;

  int32_t iTotalMbTargetLayer = pSliceHeader->pSps->uiTotalMbCount;

  int32_t iCurLayerWidth  = pCurLayer->iMbWidth << 4;
  int32_t iCurLayerHeight = pCurLayer->iMbHeight << 4;

  int32_t iNextMbXyIndex = 0;
  PFmo pFmo = pCtx->pFmo;

  int32_t iTotalNumMb = pCurSlice->iTotalMbInCurSlice;
  int32_t iCountNumMb = 0;
  PDeblockingFilterMbFunc pDeblockMb;

  if (!pCtx->bAvcBasedFlag && iCurLayerWidth != pCtx->iCurSeqIntervalMaxPicWidth) {
    return -1;
  }

  iNextMbXyIndex   = pSliceHeader->iFirstMbInSlice;
  pCurLayer->iMbX  = iNextMbXyIndex % pCurLayer->iMbWidth;
  pCurLayer->iMbY  = iNextMbXyIndex / pCurLayer->iMbWidth;
  pCurLayer->iMbXyIndex = iNextMbXyIndex;

  if (0 == iNextMbXyIndex) {
    pCurLayer->pDec->iSpsId = pSliceHeader->iSpsId;
    pCurLayer->pDec->iPpsId = pSliceHeader->iPpsId;

    pCurLayer->pDec->uiQualityId = pCurLayer->sLayerInfo.sNalHeaderExt.uiQualityId;
  }

  do {
    iPreQP = pCurLayer->pLumaQp[pCurLayer->iMbXyIndex];

    if (WelsTargetMbConstruction (pCtx)) {
      WelsLog (pCtx, WELS_LOG_WARNING, "WelsTargetSliceConstruction():::MB(%d, %d) construction error. pCurSlice_type:%d\n",
               pCurLayer->iMbX, pCurLayer->iMbY, pCurSlice->eSliceType);

      return -1;
    }

    ++iCountNumMb;
    ++pCurLayer->pDec->iTotalNumMbRec;
    if (iCountNumMb >= iTotalNumMb) {
      break;
    }
    if (pCurLayer->pDec->iTotalNumMbRec > iTotalMbTargetLayer) {
      WelsLog (pCtx, WELS_LOG_WARNING, "WelsTargetSliceConstruction():::fdec->iTotalNumMbRec:%d, iTotalMbTargetLayer:%d\n",
               pCurLayer->pDec->iTotalNumMbRec, iTotalMbTargetLayer);

      return -1;
    }

    if (pSliceHeader->pPps->uiNumSliceGroups > 1) {
      iNextMbXyIndex = FmoNextMb (pFmo, iNextMbXyIndex);
    } else {
      ++iNextMbXyIndex;
    }
    if (-1 == iNextMbXyIndex || iNextMbXyIndex >= iTotalMbTargetLayer) {	// slice group boundary or end of a frame
      break;
    }
    pCurLayer->iMbX  = iNextMbXyIndex % pCurLayer->iMbWidth;
    pCurLayer->iMbY  = iNextMbXyIndex / pCurLayer->iMbWidth;
    pCurLayer->iMbXyIndex = iNextMbXyIndex;
  } while (1);

  pCtx->pDec->iWidthInPixel  = iCurLayerWidth;
  pCtx->pDec->iHeightInPixel = iCurLayerHeight;

  if ((pCurSlice->eSliceType != I_SLICE) && (pCurSlice->eSliceType != P_SLICE))
    return 0;

  pDeblockMb = WelsDeblockingMb;

  if (1 == pSliceHeader->uiDisableDeblockingFilterIdc) {
    return 0;//NO_SUPPORTED_FILTER_IDX
  } else {
    WelsDeblockingFilterSlice (pCtx, pDeblockMb);

  }
  // any other filter_idc not supported here, 7/22/2010

  return 0;
}

int32_t WelsMbInterSampleConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer,
                                       uint8_t* pDstY, uint8_t* pDstU, uint8_t* pDstV, int32_t iStrideL, int32_t iStrideC) {
  int32_t iMbXy = pCurLayer->iMbXyIndex;
  int32_t i, iIndex, iOffset;

  WelsChromaDcIdct (pCurLayer->pScaledTCoeff[iMbXy] + 256);	// 256 = 16*16
  WelsChromaDcIdct (pCurLayer->pScaledTCoeff[iMbXy] + 320);	// 320 = 16*16 + 16*4

  for (i = 0; i < 16; i++) { //luma
    iIndex = g_kuiMbNonZeroCountIdx[i];
    if (pCurLayer->pNzc[iMbXy][iIndex]) {
      iOffset = ((iIndex >> 2) << 2) * iStrideL + ((iIndex % 4) << 2);
      pCtx->pIdctResAddPredFunc (pDstY + iOffset, iStrideL, pCurLayer->pScaledTCoeff[iMbXy] + (i << 4));
    }
  }

  for (i = 0; i < 4; i++) { //chroma
    iIndex = g_kuiMbNonZeroCountIdx[i + 16]; //Cb
    if (pCurLayer->pNzc[iMbXy][iIndex] || * (pCurLayer->pScaledTCoeff[iMbXy] + ((i + 16) << 4))) {
      iOffset = (((iIndex - 16) >> 2) << 2) * iStrideC + (((iIndex - 16) % 4) << 2);
      pCtx->pIdctResAddPredFunc (pDstU + iOffset, iStrideC, pCurLayer->pScaledTCoeff[iMbXy] + ((i + 16) << 4));
    }

    iIndex = g_kuiMbNonZeroCountIdx[i + 20]; //Cr
    if (pCurLayer->pNzc[iMbXy][iIndex] || * (pCurLayer->pScaledTCoeff[iMbXy] + ((i + 20) << 4))) {
      iOffset = (((iIndex - 18) >> 2) << 2) * iStrideC + (((iIndex - 18) % 4) << 2);
      pCtx->pIdctResAddPredFunc (pDstV + iOffset, iStrideC , pCurLayer->pScaledTCoeff[iMbXy] + ((i + 20) << 4));
    }
  }

  return 0;
}
int32_t WelsMbInterConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer) {
  int32_t iMbX = pCurLayer->iMbX;
  int32_t iMbY = pCurLayer->iMbY;
  uint8_t*  pDstY, *pDstCb, *pDstCr;

  int32_t iLumaStride   = pCtx->pDec->iLinesize[0];
  int32_t iChromaStride = pCtx->pDec->iLinesize[1];

  pDstY  = pCurLayer->pDec->pData[0] + ((iMbY * iLumaStride + iMbX) << 4);
  pDstCb = pCurLayer->pDec->pData[1] + ((iMbY * iChromaStride + iMbX) << 3);
  pDstCr = pCurLayer->pDec->pData[2] + ((iMbY * iChromaStride + iMbX) << 3);

  GetInterPred (pDstY, pDstCb, pDstCr, pCtx);
  WelsMbInterSampleConstruction (pCtx, pCurLayer, pDstY, pDstCb, pDstCr, iLumaStride, iChromaStride);

  pCtx->sBlockFunc.pWelsSetNonZeroCountFunc (NULL,
      pCurLayer->pNzc[pCurLayer->iMbXyIndex]); // set all none-zero nzc to 1; dbk can be opti!
  return 0;
}

void_t WelsLumaDcDequantIdct (int16_t* pBlock, int32_t iQp) {
  const int32_t kiQMul = g_kuiDequantCoeff[iQp][0];
#define STRIDE 16
  int32_t i;
  int32_t iTemp[16]; //FIXME check if this is a good idea
  int16_t* pBlk = pBlock;
  static const int32_t kiXOffset[4] = {0, STRIDE, STRIDE << 2,  5 * STRIDE};
  static const int32_t kiYOffset[4] = {0, STRIDE << 1, STRIDE << 3, 10 * STRIDE};

  for (i = 0; i < 4; i++) {
    const int32_t kiOffset = kiYOffset[i];
    const int32_t kiX1 = kiOffset + kiXOffset[2];
    const int32_t kiX2 = STRIDE + kiOffset;
    const int32_t kiX3 = kiOffset + kiXOffset[3];
    const int32_t kiI4 = i << 2;	// 4*i
    const int32_t kiZ0 = pBlk[kiOffset] + pBlk[kiX1];
    const int32_t kiZ1 = pBlk[kiOffset] - pBlk[kiX1];
    const int32_t kiZ2 = pBlk[kiX2] - pBlk[kiX3];
    const int32_t kiZ3 = pBlk[kiX2] + pBlk[kiX3];

    iTemp[kiI4]  = kiZ0 + kiZ3;
    iTemp[1 + kiI4] = kiZ1 + kiZ2;
    iTemp[2 + kiI4] = kiZ1 - kiZ2;
    iTemp[3 + kiI4] = kiZ0 - kiZ3;
  }

  for (i = 0; i < 4; i++) {
    const int32_t kiOffset = kiXOffset[i];
    const int32_t kiI4 = 4 + i;
    const int32_t kiZ0 = iTemp[i] + iTemp[4 + kiI4];
    const int32_t kiZ1 = iTemp[i] - iTemp[4 + kiI4];
    const int32_t kiZ2 = iTemp[kiI4] - iTemp[8 + kiI4];
    const int32_t kiZ3 = iTemp[kiI4] + iTemp[8 + kiI4];

    pBlk[kiOffset] = ((kiZ0 + kiZ3) * kiQMul + 2) >> 2; //FIXME think about merging this into decode_resdual
    pBlk[kiYOffset[1] + kiOffset] = ((kiZ1 + kiZ2) * kiQMul + 2) >> 2;
    pBlk[kiYOffset[2] + kiOffset] = ((kiZ1 - kiZ2) * kiQMul + 2) >> 2;
    pBlk[kiYOffset[3] + kiOffset] = ((kiZ0 - kiZ3) * kiQMul + 2) >> 2;
  }
#undef STRIDE
}

int32_t WelsMbIntraPredictionConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer, bool_t bOutput) {
//seems IPCM should not enter this path
  int32_t iMbXy = pCurLayer->iMbXyIndex;

  WelsFillRecNeededMbInfo (pCtx, bOutput, pCurLayer);

  if (IS_INTRA16x16 (pCurLayer->pMbType[iMbXy])) {
    WelsLumaDcDequantIdct (pCurLayer->pScaledTCoeff[iMbXy], pCurLayer->pLumaQp[iMbXy]);
    RecI16x16Mb (iMbXy, pCtx, pCurLayer->pScaledTCoeff[iMbXy], pCurLayer);

    return 0;
  }

  if (IS_INTRA4x4 (pCurLayer->pMbType[iMbXy]))
    RecI4x4Mb (iMbXy, pCtx, pCurLayer->pScaledTCoeff[iMbXy], pCurLayer);

  return 0;
}

int32_t WelsMbInterPrediction (PWelsDecoderContext pCtx, PDqLayer pCurLayer) {
  int32_t iMbX = pCurLayer->iMbX;
  int32_t iMbY = pCurLayer->iMbY;
  uint8_t*  pDstY, *pDstCb, *pDstCr;

  int32_t iLumaStride   = pCtx->pDec->iLinesize[0];
  int32_t iChromaStride = pCtx->pDec->iLinesize[1];

  pDstY  = pCurLayer->pDec->pData[0] + ((iMbY * iLumaStride + iMbX) << 4);
  pDstCb = pCurLayer->pDec->pData[1] + ((iMbY * iChromaStride + iMbX) << 3);
  pDstCr = pCurLayer->pDec->pData[2] + ((iMbY * iChromaStride + iMbX) << 3);

  GetInterPred (pDstY, pDstCb, pDstCr, pCtx);

  return 0;
}

void_t WelsMbCopy (uint8_t* pDst, int32_t iStrideDst, uint8_t* pSrc, int32_t iStrideSrc,
                   int32_t iHeight, int32_t iWidth) {
  int32_t i;
  int32_t iOffsetDst = 0, iOffsetSrc = 0;
  for (i = 0; i < iHeight; i++) {
    memcpy (pDst + iOffsetDst, pSrc + iOffsetSrc, iWidth);
    iOffsetDst += iStrideDst;
    iOffsetSrc += iStrideSrc;
  }
}


int32_t WelsTargetMbConstruction (PWelsDecoderContext pCtx) {
  PDqLayer pCurLayer = pCtx->pCurDqLayer;
  if (MB_TYPE_INTRA_PCM == pCurLayer->pMbType[pCurLayer->iMbXyIndex]) {
    //copy cs into fdec
    int32_t iCsStrideL = pCurLayer->iCsStride[0];
    int32_t iCsStrideC = pCurLayer->iCsStride[1];

    int32_t iDecStrideL = pCurLayer->pDec->iLinesize[0];
    int32_t iDecStrideC = pCurLayer->pDec->iLinesize[1];

    int32_t iCsOffsetL = (pCurLayer->iMbX + pCurLayer->iMbY * iCsStrideL) << 4;
    int32_t iCsOffsetC = (pCurLayer->iMbX + pCurLayer->iMbY * iCsStrideC) << 3;

    int32_t iDecOffsetL = (pCurLayer->iMbX + pCurLayer->iMbY * iDecStrideL) << 4;
    int32_t iDecOffsetC = (pCurLayer->iMbX + pCurLayer->iMbY * iDecStrideC) << 3;

    uint8_t* pSrcY = pCurLayer->pCsData[0] + iCsOffsetL;
    uint8_t* pSrcU = pCurLayer->pCsData[1] + iCsOffsetC;
    uint8_t* pSrcV = pCurLayer->pCsData[2] + iCsOffsetC;

    uint8_t* pDecY = pCurLayer->pDec->pData[0] + iDecOffsetL;
    uint8_t* pDecU = pCurLayer->pDec->pData[1] + iDecOffsetC;
    uint8_t* pDecV = pCurLayer->pDec->pData[2] + iDecOffsetC;

    WelsMbCopy (pDecY, iDecStrideL, pSrcY, iCsStrideL, 16, 16);
    WelsMbCopy (pDecU, iDecStrideC, pSrcU, iCsStrideC, 8, 8);
    WelsMbCopy (pDecV, iDecStrideC, pSrcV, iCsStrideC, 8, 8);

    return 0;
  } else if (IS_INTRA (pCurLayer->pMbType[pCurLayer->iMbXyIndex])) {
    WelsMbIntraPredictionConstruction (pCtx, pCurLayer, 1);
  } else if (IS_INTER (pCurLayer->pMbType[pCurLayer->iMbXyIndex])) { //InterMB
    if (0 == pCurLayer->pCbp[pCurLayer->iMbXyIndex]) { //uiCbp==0 include SKIP
      WelsMbInterPrediction (pCtx, pCurLayer);
    } else {
      WelsMbInterConstruction (pCtx, pCurLayer);
    }
  } else {
    WelsLog (pCtx, WELS_LOG_WARNING, "WelsTargetMbConstruction():::::Unknown MB type: %d\n",
             pCurLayer->pMbType[pCurLayer->iMbXyIndex]);
    return -1;
  }

  return 0;
}

void_t WelsChromaDcIdct (int16_t* pBlock) {
  int32_t iStride = 32;
  int32_t iXStride = 16;
  int32_t iStride1 = iXStride + iStride;
  int16_t* pBlk = pBlock;
  int32_t iA, iB, iC, iD, iE;

  iA = pBlk[0];
  iB = pBlk[iXStride];
  iC = pBlk[iStride];
  iD = pBlk[iStride1];

  iE = iA - iB;
  iA += iB;
  iB = iC - iD;
  iC += iD;

  pBlk[0] = (iA + iC) >> 1;
  pBlk[iXStride] = (iE + iB) >> 1;
  pBlk[iStride] = (iA - iC) >> 1;
  pBlk[iStride1] = (iE - iB) >> 1;
}

int32_t WelsDecodeSlice (PWelsDecoderContext pCtx, bool_t bFirstSliceInLayer, PNalUnit pNalCur) {
  PDqLayer pCurLayer = pCtx->pCurDqLayer;
  PFmo pFmo = pCtx->pFmo;
  int32_t i, iRet;
  int32_t iNextMbXyIndex, iSliceIdc;

  PSlice pSlice = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeaderExt pSliceHeaderExt = &pSlice->sSliceHeaderExt;
  PSliceHeader pSliceHeader = &pSliceHeaderExt->sSliceHeader;
  int32_t iMbX, iMbY;
  const int32_t kiCountNumMb = pSliceHeader->pSps->uiTotalMbCount; //need to be correct when fmo or multi slice
  PBitStringAux pBs = pCurLayer->pBitStringAux;
  int32_t iUsedBits  = 0;

  PWelsDecMbCavlcFunc pDecMbCavlcFunc;

  pSlice->iTotalMbInCurSlice = 0; //initialize at the starting of slice decoding.

  if (P_SLICE == pSliceHeader->eSliceType) {
    pDecMbCavlcFunc = WelsDecodeMbCavlcPSlice;
  } else { //I_SLICE
    pDecMbCavlcFunc = WelsDecodeMbCavlcISlice;
  }

  if (pSliceHeader->pPps->bConstainedIntraPredFlag) {
    pCtx->pFillInfoCacheIntra4x4Func = WelsFillCacheConstrain1Intra4x4;
    pCtx->pParseIntra4x4ModeFunc      = ParseIntra4x4ModeConstrain1;
    pCtx->pParseIntra16x16ModeFunc    = ParseIntra16x16ModeConstrain1;
  } else {
    pCtx->pFillInfoCacheIntra4x4Func = WelsFillCacheConstrain0Intra4x4;
    pCtx->pParseIntra4x4ModeFunc      = ParseIntra4x4ModeConstrain0;
    pCtx->pParseIntra16x16ModeFunc    = ParseIntra16x16ModeConstrain0;
  }

  pCtx->eSliceType = pSliceHeader->eSliceType;

  if (pCurLayer->sLayerInfo.pPps->bEntropyCodingModeFlag == 1) {
    //CABAC encoding is unsupported yet!
    return -1;
  }

  iNextMbXyIndex = pSliceHeader->iFirstMbInSlice;

  if (iNextMbXyIndex >= kiCountNumMb) {
    WelsLog (pCtx, WELS_LOG_ERROR,
             "WelsDecodeSlice()::iFirstMbInSlice(%d) > pSps->kiTotalMb(%d). ERROR!!! resolution change....\n",
             iNextMbXyIndex, kiCountNumMb);
    pCtx->iErrorCode |= dsNoParamSets;
    return dsNoParamSets;
  }

  iMbX = iNextMbXyIndex % pCurLayer->iMbWidth;
  iMbY = iNextMbXyIndex / pCurLayer->iMbWidth; // error is introduced by multiple slices case, 11/23/2009
  pSlice->iMbSkipRun = -1;
  iSliceIdc = (pSliceHeader->iFirstMbInSlice << 7) + pCurLayer->uiLayerDqId;

  pCurLayer->iMbX =  iMbX;
  pCurLayer->iMbY = iMbY;
  pCurLayer->iMbXyIndex = iNextMbXyIndex;

  if (pSliceHeaderExt->bSliceSkipFlag == 1) {
    for (i = 0; i < (int32_t)pSliceHeaderExt->uiNumMbsInSlice; i++) {
      pCurLayer->pSliceIdc[iNextMbXyIndex] = iSliceIdc;


      pCurLayer->pResidualPredFlag[iNextMbXyIndex] = 1;

      if (pSliceHeaderExt->sSliceHeader.pPps->uiNumSliceGroups > 1) {
        iNextMbXyIndex = FmoNextMb (pFmo, iNextMbXyIndex);
      } else {
        ++iNextMbXyIndex;
      }

      iMbX = iNextMbXyIndex % pCurLayer->iMbWidth;
      iMbY = iNextMbXyIndex % pCurLayer->iMbHeight;

      pCurLayer->iMbX =  iMbX;
      pCurLayer->iMbY = iMbY;
      pCurLayer->iMbXyIndex = iNextMbXyIndex;
    }
    return 0;
  }

  do {
    pCurLayer->pSliceIdc[iNextMbXyIndex] = iSliceIdc;
    iRet = pDecMbCavlcFunc (pCtx,  pNalCur);

    if (iRet != ERR_NONE) {
      return iRet;
    }

    ++pSlice->iTotalMbInCurSlice;

    if (pSliceHeader->pPps->uiNumSliceGroups > 1) {
      iNextMbXyIndex = FmoNextMb (pFmo, iNextMbXyIndex);
    } else {
      ++iNextMbXyIndex;
    }
    if ((-1 == iNextMbXyIndex) || (iNextMbXyIndex >= kiCountNumMb)) {	// slice group boundary or end of a frame
      break;
    }

    // check whether there is left bits to read next time in case multiple slices
    iUsedBits = ((pBs->pCurBuf - pBs->pStartBuf) << 3) - (16 - pBs->iLeftBits);
    if (iUsedBits == pBs->iBits && 0 >= pCurLayer->sLayerInfo.sSliceInLayer.iMbSkipRun) {	// slice boundary
      break;
    }
    if (iUsedBits > pBs->iBits) { //When BS incomplete, as long as find it, SHOULD stop decoding to avoid mosaic or crash.
      WelsLog (pCtx, WELS_LOG_WARNING,
               "WelsDecodeSlice()::::pBs incomplete, iUsedBits:%d > pBs->iBits:%d, MUST stop decoding.\n",
               iUsedBits, pBs->iBits);
      return -1;
    }
    iMbX = iNextMbXyIndex % pCurLayer->iMbWidth;
    iMbY = iNextMbXyIndex / pCurLayer->iMbWidth;
    pCurLayer->iMbX =  iMbX;
    pCurLayer->iMbY = iMbY;
    pCurLayer->iMbXyIndex = iNextMbXyIndex;
  } while (1);

  return ERR_NONE;
}

int32_t WelsActualDecodeMbCavlcISlice (PWelsDecoderContext pCtx) {
  SVlcTable* pVlcTable     = &pCtx->sVlcTable;
  PDqLayer pCurLayer		 = pCtx->pCurDqLayer;
  PBitStringAux pBs		 = pCurLayer->pBitStringAux;
  PSlice pSlice			 = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader		     = &pSlice->sSliceHeaderExt.sSliceHeader;

  SNeighAvail sNeighAvail;

  int32_t iScanIdxStart = pSlice->sSliceHeaderExt.uiScanIdxStart;
  int32_t iScanIdxEnd   = pSlice->sSliceHeaderExt.uiScanIdxEnd;

  int32_t iMbX = pCurLayer->iMbX;
  int32_t iMbY = pCurLayer->iMbY;
  int32_t iMbXy = pCurLayer->iMbXyIndex;
  int32_t iNMbMode, i;
  uint32_t uiMbType = 0, uiCbp = 0, uiCbpL = 0, uiCbpC = 0;

  FORCE_STACK_ALIGN_1D (uint8_t, pNonZeroCount, 48, 16);

  pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
  pCurLayer->pResidualPredFlag[iMbXy] = pSlice->sSliceHeaderExt.bDefaultResidualPredFlag;

  uiMbType = BsGetUe (pBs);
  if (uiMbType > 25) {
    return ERR_INFO_INVALID_MB_TYPE;
  }

  if (25 == uiMbType) {
    int32_t iDecStrideL = pCurLayer->pDec->iLinesize[0];
    int32_t iDecStrideC = pCurLayer->pDec->iLinesize[1];

    int32_t iOffsetL = (iMbX + iMbY * iDecStrideL) << 4;
    int32_t iOffsetC = (iMbX + iMbY * iDecStrideC) << 3;

    uint8_t* pDecY = pCurLayer->pCsData[0] + iOffsetL;
    uint8_t* pDecU = pCurLayer->pCsData[1] + iOffsetC;
    uint8_t* pDecV = pCurLayer->pCsData[2] + iOffsetC;

    uint8_t* pTmpBsBuf;

    int32_t i;
    int32_t iCopySizeY  = (sizeof (uint8_t) << 4);
    int32_t iCopySizeUV = (sizeof (uint8_t) << 3);

    int32_t iIndex = ((-pBs->iLeftBits) >> 3) + 2;

    pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA_PCM;

    //step 1: locating bit-stream pointer [must align into integer byte]
    pBs->pCurBuf -= iIndex;

    //step 2: copy pixel from bit-stream into fdec [reconstruction]
    pTmpBsBuf = pBs->pCurBuf;
    for (i = 0; i < 16; i++) { //luma
      memcpy (pDecY , pTmpBsBuf, iCopySizeY);
      pDecY += iDecStrideL;
      pTmpBsBuf += 16;
    }
    for (i = 0; i < 8; i++) { //cb
      memcpy (pDecU, pTmpBsBuf, iCopySizeUV);
      pDecU += iDecStrideC;
      pTmpBsBuf += 8;
    }
    for (i = 0; i < 8; i++) { //cr
      memcpy (pDecV, pTmpBsBuf, iCopySizeUV);
      pDecV += iDecStrideC;
      pTmpBsBuf += 8;
    }

    pBs->pCurBuf += 384;
    InitReadBits (pBs);

    //step 3: update QP and pNonZeroCount
    pCurLayer->pLumaQp[iMbXy] = 0;
    pCurLayer->pChromaQp[iMbXy] = 0;
    memset (pCurLayer->pNzc[iMbXy], 16, sizeof (pCurLayer->pNzc[iMbXy]));   //JVT-x201wcm1.doc, page229, 2009.10.23
    return 0;
  } else if (0 == uiMbType) { //reference to JM
    FORCE_STACK_ALIGN_1D (int8_t, pIntraPredMode, 48, 16);
    pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA4x4;
    pCtx->pFillInfoCacheIntra4x4Func (&sNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
    if (pCtx->pParseIntra4x4ModeFunc (&sNeighAvail, pIntraPredMode, pBs, pCurLayer)) {
      return -1;
    }

    //uiCbp
    uiCbp = BsGetUe (pBs);
    //G.9.1 Alternative parsing process for coded pBlock pattern
    if (uiCbp > 47)
      return ERR_INFO_INVALID_CBP;

    uiCbp = g_kuiIntra4x4CbpTable[uiCbp];

    pCurLayer->pCbp[iMbXy] = uiCbp;
    uiCbpC = uiCbp >> 4;
    uiCbpL = uiCbp & 15;
  } else { //I_PCM exclude, we can ignore it
    pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA16x16;
    pCurLayer->pIntraPredMode[iMbXy][7] = (uiMbType - 1) & 3;
    pCurLayer->pCbp[iMbXy] = g_kuiI16CbpTable[ (uiMbType - 1) >> 2];
    uiCbpC = pCurLayer->pCbp[iMbXy] >> 4;
    uiCbpL = pCurLayer->pCbp[iMbXy] & 15;
    WelsFillCacheNonZeroCount (&sNeighAvail, pNonZeroCount, pCurLayer);
    if (pCtx->pParseIntra16x16ModeFunc (&sNeighAvail, pBs, pCurLayer)) {
      return -1;
    }
  }

  iNMbMode = BASE_MB;

  memset (pCurLayer->pScaledTCoeff[iMbXy], 0, 384 * sizeof (pCurLayer->pScaledTCoeff[iMbXy][0]));
  ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][20], 0);

  if (pCurLayer->pCbp[iMbXy] == 0 && IS_INTRA4x4 (pCurLayer->pMbType[iMbXy])) {
    pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp;
    pCurLayer->pChromaQp[iMbXy] = g_kuiChromaQp[WELS_CLIP3 (pCurLayer->pLumaQp[iMbXy] +
                                  pSliceHeader->pPps->iChromaQpIndexOffset, 0, 51)];

  }

  if (pCurLayer->pCbp[iMbXy] || MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
    int32_t iQpDelta, iId8x8, iId4x4;

    iQpDelta = BsGetSe (pBs);

    if (iQpDelta > 25 || iQpDelta < -26) { //out of iQpDelta range
      return ERR_INFO_INVALID_QP;
    }

    pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp + iQpDelta; //update iLastMbQp
    //refer to JVT-X201wcm1.doc equation(7-35)
    if ((unsigned) (pCurLayer->pLumaQp[iMbXy]) > 51) {
      if (pCurLayer->pLumaQp[iMbXy] < 0) {
        pCurLayer->pLumaQp[iMbXy] += 52;
      } else {
        pCurLayer->pLumaQp[iMbXy] -= 52;
      }
    }
    //QP should be in the range of [0, 51]
    if (pCurLayer->pLumaQp[iMbXy] < 0 || pCurLayer->pLumaQp[iMbXy] > 51) {
      return ERR_INFO_INVALID_QP;
    }
    pSlice->iLastMbQp = pCurLayer->pLumaQp[iMbXy];
    pCurLayer->pChromaQp[iMbXy] = g_kuiChromaQp[WELS_CLIP3 (pSlice->iLastMbQp + pSliceHeader->pPps->iChromaQpIndexOffset, 0,
                                  51)];


    BsStartCavlc (pBs);

    if (MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
      //step1: Luma DC
      if (WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, 0, 16,
                                  g_kuiLumaDcZigzagScan, I16_LUMA_DC, pCurLayer->pScaledTCoeff[iMbXy], iNMbMode, pCurLayer->pLumaQp[iMbXy], pCtx)) {
        return -1;//abnormal
      }
      //step2: Luma AC
      if (uiCbpL) {
        for (i = 0; i < 16; i++) {
          if (WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, i,
                                      iScanIdxEnd - WELS_MAX (iScanIdxStart, 1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1),
                                      I16_LUMA_AC, pCurLayer->pScaledTCoeff[iMbXy] + (i << 4), iNMbMode, pCurLayer->pLumaQp[iMbXy], pCtx)) {
            return -1;//abnormal
          }
        }
        ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      } else { //pNonZeroCount = 0
        ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
      }
    } else { //non-MB_TYPE_INTRA16x16
      for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
        if (uiCbpL & (1 << iId8x8)) {
          int32_t iIndex = (iId8x8 << 2);
          for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
            //Luma (DC and AC decoding together)
            if (WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, iIndex,
                                        iScanIdxEnd - iScanIdxStart + 1, g_kuiZigzagScan + iScanIdxStart,
                                        LUMA_DC_AC, pCurLayer->pScaledTCoeff[iMbXy] + (iIndex << 4), iNMbMode, pCurLayer->pLumaQp[iMbXy], pCtx)) {
              return -1;//abnormal
            }
            iIndex++;
          }
        } else {
          ST16 (&pNonZeroCount[g_kuiCacheNzcScanIdx[ (iId8x8 << 2)]], 0);
          ST16 (&pNonZeroCount[g_kuiCacheNzcScanIdx[ (iId8x8 << 2) + 2]], 0);
        }
      }
      ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
      ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
      ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
      ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
    }

    //chroma
    //step1: DC
    if (1 == uiCbpC || 2 == uiCbpC) {
      for (i = 0; i < 2; i++) { //Cb Cr
        if (WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs,
                                    16 + (i << 2), 4, g_kuiChromaDcScan, CHROMA_DC, pCurLayer->pScaledTCoeff[iMbXy] + 256 + (i << 6),
                                    iNMbMode, pCurLayer->pChromaQp[iMbXy], pCtx)) {
          return -1;//abnormal
        }
      }
    }

    //step2: AC
    if (2 == uiCbpC) {
      for (i = 0; i < 2; i++) { //Cb Cr
        int32_t iIndex = 16 + (i << 2);
        for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
          if (WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, iIndex,
                                      iScanIdxEnd - WELS_MAX (iScanIdxStart, 1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1),
                                      CHROMA_AC, pCurLayer->pScaledTCoeff[iMbXy] + (iIndex << 4), iNMbMode, pCurLayer->pChromaQp[iMbXy], pCtx)) {
            return -1;//abnormal
          }
          iIndex++;
        }
      }
      ST16 (&pCurLayer->pNzc[iMbXy][16], LD16 (&pNonZeroCount[6 + 8 * 1]));
      ST16 (&pCurLayer->pNzc[iMbXy][20], LD16 (&pNonZeroCount[6 + 8 * 2]));
      ST16 (&pCurLayer->pNzc[iMbXy][18], LD16 (&pNonZeroCount[6 + 8 * 4]));
      ST16 (&pCurLayer->pNzc[iMbXy][22], LD16 (&pNonZeroCount[6 + 8 * 5]));
    } else {
      ST16 (&pCurLayer->pNzc[iMbXy][16], 0);
      ST16 (&pCurLayer->pNzc[iMbXy][20], 0);
      ST16 (&pCurLayer->pNzc[iMbXy][18], 0);
      ST16 (&pCurLayer->pNzc[iMbXy][22], 0);
    }
    BsEndCavlc (pBs);
  } else {
    ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][20], 0);
  }

  return 0;
}

int32_t WelsDecodeMbCavlcISlice (PWelsDecoderContext pCtx, PNalUnit pNalCur) {
  PDqLayer pCurLayer = pCtx->pCurDqLayer;
  PBitStringAux pBs = pCurLayer->pBitStringAux;
  PSliceHeaderExt pSliceHeaderExt = &pCurLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt;
  int32_t iBaseModeFlag;
  int32_t iRet = 0; //should have the return value to indicate decoding error or not, It's NECESSARY--2010.4.15

  if (pSliceHeaderExt->bAdaptiveBaseModeFlag == 1) {
    iBaseModeFlag = BsGetOneBit (pBs);
  } else {
    iBaseModeFlag = pSliceHeaderExt->bDefaultBaseModeFlag;
  }
  if (!iBaseModeFlag) {
    iRet = WelsActualDecodeMbCavlcISlice (pCtx);
  } else {
    WelsLog (pCtx, WELS_LOG_WARNING, "iBaseModeFlag (%d) != 0, inter-layer prediction not supported.\n", iBaseModeFlag);
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_ILP);
  }
  if (iRet) { //occur error when parsing, MUST STOP decoding
    return iRet;
  }

  return 0;
}

int32_t WelsActualDecodeMbCavlcPSlice (PWelsDecoderContext pCtx) {
  SVlcTable* pVlcTable     = &pCtx->sVlcTable;
  PDqLayer pCurLayer		 = pCtx->pCurDqLayer;
  PBitStringAux pBs		 = pCurLayer->pBitStringAux;
  PSlice pSlice			 = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader		     = &pSlice->sSliceHeaderExt.sSliceHeader;

  SNeighAvail sNeighAvail;

  int32_t iScanIdxStart = pSlice->sSliceHeaderExt.uiScanIdxStart;
  int32_t iScanIdxEnd   = pSlice->sSliceHeaderExt.uiScanIdxEnd;

  int32_t iMbX = pCurLayer->iMbX;
  int32_t iMbY = pCurLayer->iMbY;
  int32_t iMbXy = pCurLayer->iMbXyIndex;

  int32_t iNMbMode, i;
  uint32_t uiMbType = 0, uiCbp = 0, uiCbpL = 0, uiCbpC = 0;

  FORCE_STACK_ALIGN_1D (uint8_t, pNonZeroCount, 48, 16);
  pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;//2009.10.23

  uiMbType = BsGetUe (pBs);
  if (uiMbType < 5) { //inter MB type
    int16_t iMotionVector[LIST_A][30][MV_A];

    int8_t	iRefIndex[LIST_A][30];
    pCurLayer->pMbType[iMbXy] = g_ksInterMbTypeInfo[uiMbType].iType;
    WelsFillCacheInter (&sNeighAvail, pNonZeroCount, iMotionVector, iRefIndex, pCurLayer);
    if (ParseInterInfo (pCtx, iMotionVector, iRefIndex, pBs)) {
      return -1;//abnormal
    }

    if (pSlice->sSliceHeaderExt.bAdaptiveResidualPredFlag == 1) {
      pCurLayer->pResidualPredFlag[iMbXy] =  BsGetOneBit (pBs);
    } else {
      pCurLayer->pResidualPredFlag[iMbXy] = pSlice->sSliceHeaderExt.bDefaultResidualPredFlag;
    }

    if (pCurLayer->pResidualPredFlag[iMbXy] == 0) {
      iNMbMode = BASE_MB;
      pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
    } else {
      WelsLog (pCtx, WELS_LOG_WARNING, "residual_pred_flag = 1 not supported.\n");
      return -1;
    }
  } else { //intra MB type
    uiMbType -= 5;
    if (uiMbType > 25) {
      return ERR_INFO_INVALID_MB_TYPE;
    }

    if (25 == uiMbType) {
      int32_t iDecStrideL = pCurLayer->pDec->iLinesize[0];
      int32_t iDecStrideC = pCurLayer->pDec->iLinesize[1];

      int32_t iOffsetL = (iMbX + iMbY * iDecStrideL) << 4;
      int32_t iOffsetC = (iMbX + iMbY * iDecStrideC) << 3;

      uint8_t* pDecY = pCurLayer->pCsData[0] + iOffsetL;
      uint8_t* pDecU = pCurLayer->pCsData[1] + iOffsetC;
      uint8_t* pDecV = pCurLayer->pCsData[2] + iOffsetC;

      uint8_t* pTmpBsBuf;

      int32_t i;
      int32_t iCopySizeY  = (sizeof (uint8_t) << 4);
      int32_t iCopySizeUV = (sizeof (uint8_t) << 3);

      int32_t iIndex = ((-pBs->iLeftBits) >> 3) + 2;

      pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA_PCM;

      //step 1: locating bit-stream pointer [must align into integer byte]
      pBs->pCurBuf -= iIndex;

      //step 2: copy pixel from bit-stream into fdec [reconstruction]
      pTmpBsBuf = pBs->pCurBuf;
      for (i = 0; i < 16; i++) { //luma
        memcpy (pDecY , pTmpBsBuf, iCopySizeY);
        pDecY += iDecStrideL;
        pTmpBsBuf += 16;
      }

      for (i = 0; i < 8; i++) { //cb
        memcpy (pDecU, pTmpBsBuf, iCopySizeUV);
        pDecU += iDecStrideC;
        pTmpBsBuf += 8;
      }
      for (i = 0; i < 8; i++) { //cr
        memcpy (pDecV, pTmpBsBuf, iCopySizeUV);
        pDecV += iDecStrideC;
        pTmpBsBuf += 8;
      }

      pBs->pCurBuf += 384;
      InitReadBits (pBs);

      //step 3: update QP and pNonZeroCount
      pCurLayer->pLumaQp[iMbXy] = 0;
      pCurLayer->pChromaQp[iMbXy] = 0;
      ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
      ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
      ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
      ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
      return 0;
    } else {
      if (0 == uiMbType) {
        FORCE_STACK_ALIGN_1D (int8_t, pIntraPredMode, 48, 16);
        pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA4x4;
        pCtx->pFillInfoCacheIntra4x4Func (&sNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
        if (pCtx->pParseIntra4x4ModeFunc (&sNeighAvail, pIntraPredMode, pBs, pCurLayer)) {
          return -1;
        }
        iNMbMode = BASE_MB;
      } else { //I_PCM exclude, we can ignore it
        pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA16x16;
        pCurLayer->pIntraPredMode[iMbXy][7] = (uiMbType - 1) & 3;
        pCurLayer->pCbp[iMbXy] = g_kuiI16CbpTable[ (uiMbType - 1) >> 2];
        uiCbpC = pCurLayer->pCbp[iMbXy] >> 4;
        uiCbpL = pCurLayer->pCbp[iMbXy] & 15;
        WelsFillCacheNonZeroCount (&sNeighAvail, pNonZeroCount, pCurLayer);
        if (pCtx->pParseIntra16x16ModeFunc (&sNeighAvail, pBs, pCurLayer)) {
          return -1;
        }
        iNMbMode = BASE_MB;
      }
    }
  }

  if (MB_TYPE_INTRA16x16 != pCurLayer->pMbType[iMbXy]) {
    uiCbp = BsGetUe (pBs);
    {
      if (uiCbp > 47)
        return ERR_INFO_INVALID_CBP;

      if (MB_TYPE_INTRA4x4 == pCurLayer->pMbType[iMbXy]) {
        uiCbp = g_kuiIntra4x4CbpTable[uiCbp];
      } else //inter
        uiCbp = g_kuiInterCbpTable[uiCbp];
    }

    pCurLayer->pCbp[iMbXy] = uiCbp;
    uiCbpC = pCurLayer->pCbp[iMbXy] >> 4;
    uiCbpL = pCurLayer->pCbp[iMbXy] & 15;
  }

  if (iNMbMode == BASE_MB) {
    pCtx->sBlockFunc.pWelsBlockZero16x16Func (pCurLayer->pScaledTCoeff[iMbXy], 16);
    pCtx->sBlockFunc.pWelsBlockZero8x8Func (pCurLayer->pScaledTCoeff[iMbXy] + 256, 8);
    pCtx->sBlockFunc.pWelsBlockZero8x8Func (pCurLayer->pScaledTCoeff[iMbXy] + 256 + 64, 8);

    ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][20], 0);
    if (pCurLayer->pCbp[iMbXy] == 0 && !IS_INTRA16x16 (pCurLayer->pMbType[iMbXy]) && !IS_I_BL (pCurLayer->pMbType[iMbXy])) {
      pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp;
      pCurLayer->pChromaQp[iMbXy] = g_kuiChromaQp[WELS_CLIP3 (pCurLayer->pLumaQp[iMbXy] +
                                    pSliceHeader->pPps->iChromaQpIndexOffset, 0, 51)];
    }
  }

  if (pCurLayer->pCbp[iMbXy] || MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
    int32_t iQpDelta, iId8x8, iId4x4;

    iQpDelta = BsGetSe (pBs);

    if (iQpDelta > 25 || iQpDelta < -26) { //out of iQpDelta range
      return ERR_INFO_INVALID_QP;
    }

    pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp + iQpDelta; //update iLastMbQp
    //refer to JVT-X201wcm1.doc equation(7-35)
    if ((unsigned) (pCurLayer->pLumaQp[iMbXy]) > 51) {
      if (pCurLayer->pLumaQp[iMbXy] < 0) {
        pCurLayer->pLumaQp[iMbXy] += 52;
      } else {
        pCurLayer->pLumaQp[iMbXy] -= 52;
      }
    }
    //QP should be in the range of [0, 51]
    if (pCurLayer->pLumaQp[iMbXy] < 0 || pCurLayer->pLumaQp[iMbXy] > 51) {
      return ERR_INFO_INVALID_QP;
    }
    pSlice->iLastMbQp = pCurLayer->pLumaQp[iMbXy];
    pCurLayer->pChromaQp[iMbXy] = g_kuiChromaQp[WELS_CLIP3 (pSlice->iLastMbQp + pSliceHeader->pPps->iChromaQpIndexOffset, 0,
                                  51)];

    BsStartCavlc (pBs);

    if (MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
      //step1: Luma DC
      if (WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, 0, 16, g_kuiLumaDcZigzagScan,
                                  I16_LUMA_DC, pCurLayer->pScaledTCoeff[iMbXy], iNMbMode, pCurLayer->pLumaQp[iMbXy], pCtx)) {
        return -1;//abnormal
      }
      //step2: Luma AC
      if (uiCbpL) {
        for (i = 0; i < 16; i++) {
          if (WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, i,
                                      iScanIdxEnd - WELS_MAX (iScanIdxStart, 1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1),
                                      I16_LUMA_AC, pCurLayer->pScaledTCoeff[iMbXy] + (i << 4), iNMbMode, pCurLayer->pLumaQp[iMbXy], pCtx)) {
            return -1;//abnormal
          }
        }
        ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      } else { //pNonZeroCount = 0
        ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
      }
    } else { //non-MB_TYPE_INTRA16x16
      for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
        if (uiCbpL & (1 << iId8x8)) {
          int32_t iIndex = (iId8x8 << 2);
          for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
            //Luma (DC and AC decoding together)
            if (WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, iIndex,
                                        iScanIdxEnd - iScanIdxStart + 1, g_kuiZigzagScan + iScanIdxStart, LUMA_DC_AC,
                                        pCurLayer->pScaledTCoeff[iMbXy] + (iIndex << 4), iNMbMode, pCurLayer->pLumaQp[iMbXy], pCtx)) {
              return -1;//abnormal
            }
            iIndex++;
          }
        } else {
          ST16 (&pNonZeroCount[g_kuiCacheNzcScanIdx[iId8x8 << 2]], 0);
          ST16 (&pNonZeroCount[g_kuiCacheNzcScanIdx[ (iId8x8 << 2) + 2]], 0);
        }
      }
      ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
      ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
      ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
      ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
    }


    //chroma
    //step1: DC
    if (1 == uiCbpC || 2 == uiCbpC) {
      for (i = 0; i < 2; i++) { //Cb Cr
        if (WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs,
                                    16 + (i << 2), 4, g_kuiChromaDcScan, CHROMA_DC, pCurLayer->pScaledTCoeff[iMbXy] + 256 + (i << 6),
                                    iNMbMode, pCurLayer->pChromaQp[iMbXy], pCtx)) {
          return -1;//abnormal
        }
      }
    } else {
    }
    //step2: AC
    if (2 == uiCbpC) {
      for (i = 0; i < 2; i++) { //Cb Cr
        int32_t iIndex = 16 + (i << 2);
        for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
          if (WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, iIndex,
                                      iScanIdxEnd - WELS_MAX (iScanIdxStart, 1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1),
                                      CHROMA_AC, pCurLayer->pScaledTCoeff[iMbXy] + (iIndex << 4), iNMbMode, pCurLayer->pChromaQp[iMbXy], pCtx)) {
            return -1;//abnormal
          }
          iIndex++;
        }
      }
      ST16 (&pCurLayer->pNzc[iMbXy][16], LD16 (&pNonZeroCount[6 + 8 * 1]));
      ST16 (&pCurLayer->pNzc[iMbXy][20], LD16 (&pNonZeroCount[6 + 8 * 2]));
      ST16 (&pCurLayer->pNzc[iMbXy][18], LD16 (&pNonZeroCount[6 + 8 * 4]));
      ST16 (&pCurLayer->pNzc[iMbXy][22], LD16 (&pNonZeroCount[6 + 8 * 5]));
    } else {
      ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
      ST32 (&pCurLayer->pNzc[iMbXy][20], 0);
    }
    BsEndCavlc (pBs);
  } else {
    ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][20], 0);
  }

  return 0;
}

int32_t WelsDecodeMbCavlcPSlice (PWelsDecoderContext pCtx, PNalUnit pNalCur) {
  PDqLayer pCurLayer		 = pCtx->pCurDqLayer;
  PBitStringAux pBs		 = pCurLayer->pBitStringAux;
  PSlice pSlice			 = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader		    = &pSlice->sSliceHeaderExt.sSliceHeader;

  int32_t iMbXy = pCurLayer->iMbXyIndex;
  int32_t iBaseModeFlag, i;
  int32_t iRet = 0; //should have the return value to indicate decoding error or not, It's NECESSARY--2010.4.15

  if (-1 == pSlice->iMbSkipRun) {
    pSlice->iMbSkipRun = BsGetUe (pBs);
    if (-1 == pSlice->iMbSkipRun) {
      return -1;
    }

  }
  if (pSlice->iMbSkipRun--) {
    int16_t iMv[2] = {0};

    pCurLayer->pMbType[iMbXy] = MB_TYPE_SKIP;
    ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][20], 0);

    pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
    memset (pCurLayer->pRefIndex[0][iMbXy], 0, sizeof (int8_t) * 16);

    //predict iMv
    PredPSkipMvFromNeighbor (pCurLayer, iMv);
    for (i = 0; i < 16; i++) {
      ST32 (pCurLayer->pMv[0][iMbXy][i], * (uint32_t*)iMv);
    }

    if (!pSlice->sSliceHeaderExt.bDefaultResidualPredFlag) {
      memset (pCurLayer->pScaledTCoeff[iMbXy], 0, 384 * sizeof (int16_t));
    }

    //reset rS
    if (!pSlice->sSliceHeaderExt.bDefaultResidualPredFlag ||
        (pNalCur->sNalHeaderExt.uiQualityId == 0 && pNalCur->sNalHeaderExt.uiDependencyId == 0)) {
      pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp;
      pCurLayer->pChromaQp[iMbXy] = g_kuiChromaQp[WELS_CLIP3 (pCurLayer->pLumaQp[iMbXy] +
                                    pSliceHeader->pPps->iChromaQpIndexOffset, 0, 51)];
    }

    pCurLayer->pCbp[iMbXy] = 0;

    return 0;
  }

  if (pSlice->sSliceHeaderExt.bAdaptiveBaseModeFlag == 1) {
    iBaseModeFlag = BsGetOneBit (pBs);
  } else {
    iBaseModeFlag = pSlice->sSliceHeaderExt.bDefaultBaseModeFlag;
  }
  if (!iBaseModeFlag) {
    iRet = WelsActualDecodeMbCavlcPSlice (pCtx);
  } else {
    WelsLog (pCtx, WELS_LOG_WARNING, "iBaseModeFlag (%d) != 0, inter-layer prediction not supported.\n", iBaseModeFlag);
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_ILP);
  }
  if (iRet) { //occur error when parsing, MUST STOP decoding
    return iRet;
  }

  return 0;
}

void_t WelsBlockInit (int16_t* pBlock, int32_t iWidth, int32_t iHeight, int32_t iStride, uint8_t uiVal) {
  int32_t i;
  int16_t* pDst = pBlock;

  for (i = 0; i < iHeight; i++) {
    memset (pDst, uiVal, iWidth * sizeof (int16_t));
    pDst += iStride;
  }
}

void_t WelsBlockFuncInit (SBlockFunc*   pFunc,  int32_t iCpu) {
  pFunc->pWelsBlockZero16x16Func		= WelsBlockZero16x16_c;
  pFunc->pWelsBlockZero8x8Func	    = WelsBlockZero8x8_c;
  pFunc->pWelsSetNonZeroCountFunc	    = SetNonZeroCount_c;

#ifdef  X86_ASM
  if (iCpu & WELS_CPU_SSE2) {
    pFunc->pWelsBlockZero16x16Func		= WelsResBlockZero16x16_sse2;
    pFunc->pWelsBlockZero8x8Func	    = WelsResBlockZero8x8_sse2;
  }
#endif
}
void_t WelsBlockZero16x16_c (int16_t* pBlock, int32_t iStride) {
  WelsBlockInit (pBlock, 16, 16, iStride, 0);
}

void_t WelsBlockZero8x8_c (int16_t* pBlock, int32_t iStride) {
  WelsBlockInit (pBlock, 8, 8, iStride, 0);
}

void_t SetNonZeroCount_c (int16_t* pBlock, int8_t* pNonZeroCount) {
  int32_t i;
  int32_t iIndex;

  for (i = 0; i < 24; i++) {
    iIndex = g_kuiMbNonZeroCountIdx[i];
    pNonZeroCount[iIndex] = !!pNonZeroCount[iIndex];
  }
}

} // namespace WelsDec
