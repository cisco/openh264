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


#include "deblocking.h"

#include "decode_slice.h"

#include "parse_mb_syn_cavlc.h"
#include "parse_mb_syn_cabac.h"
#include "rec_mb.h"
#include "mv_pred.h"

#include "cpu_core.h"

namespace WelsDec {

static inline int32_t iAbs (int32_t x) {
  static const int32_t INT_BITS = (sizeof (int) * CHAR_BIT) - 1;
  int32_t y = x >> INT_BITS;
  return (x ^ y) - y;
}

extern PPicture AllocPicture (PWelsDecoderContext pCtx, const int32_t kiPicWidth, const int32_t kiPicHeight);

int32_t WelsTargetSliceConstruction (PWelsDecoderContext pCtx) {
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
    return ERR_INFO_WIDTH_MISMATCH;
  }

  iNextMbXyIndex   = pSliceHeader->iFirstMbInSlice;
  pCurLayer->iMbX  = iNextMbXyIndex % pCurLayer->iMbWidth;
  pCurLayer->iMbY  = iNextMbXyIndex / pCurLayer->iMbWidth;
  pCurLayer->iMbXyIndex = iNextMbXyIndex;

  if (0 == iNextMbXyIndex) {
    pCurLayer->pDec->iSpsId = pCtx->pSps->iSpsId;
    pCurLayer->pDec->iPpsId = pCtx->pPps->iPpsId;

    pCurLayer->pDec->uiQualityId = pCurLayer->sLayerInfo.sNalHeaderExt.uiQualityId;
  }

  do {
    if (iCountNumMb >= iTotalNumMb) {
      break;
    }

    if (!pCtx->pParam->bParseOnly) { //for parse only, actual recon MB unnecessary
      if (WelsTargetMbConstruction (pCtx)) {
        WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING,
                 "WelsTargetSliceConstruction():::MB(%d, %d) construction error. pCurSlice_type:%d",
                 pCurLayer->iMbX, pCurLayer->iMbY, pCurSlice->eSliceType);

        return ERR_INFO_MB_RECON_FAIL;
      }
    }

    ++iCountNumMb;
    if (!pCurLayer->pMbCorrectlyDecodedFlag[iNextMbXyIndex]) { //already con-ed, overwrite
      pCurLayer->pMbCorrectlyDecodedFlag[iNextMbXyIndex] = true;
      pCtx->pDec->iMbEcedPropNum += (pCurLayer->pMbRefConcealedFlag[iNextMbXyIndex] ? 1 : 0);
      ++pCtx->iTotalNumMbRec;
    }

    if (pCtx->iTotalNumMbRec > iTotalMbTargetLayer) {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING,
               "WelsTargetSliceConstruction():::pCtx->iTotalNumMbRec:%d, iTotalMbTargetLayer:%d",
               pCtx->iTotalNumMbRec, iTotalMbTargetLayer);

      return ERR_INFO_MB_NUM_EXCEED_FAIL;
    }

    if (pSliceHeader->pPps->uiNumSliceGroups > 1) {
      iNextMbXyIndex = FmoNextMb (pFmo, iNextMbXyIndex);
    } else {
      ++iNextMbXyIndex;
    }
    if (-1 == iNextMbXyIndex || iNextMbXyIndex >= iTotalMbTargetLayer) { // slice group boundary or end of a frame
      break;
    }
    pCurLayer->iMbX  = iNextMbXyIndex % pCurLayer->iMbWidth;
    pCurLayer->iMbY  = iNextMbXyIndex / pCurLayer->iMbWidth;
    pCurLayer->iMbXyIndex = iNextMbXyIndex;
  } while (1);

  pCtx->pDec->iWidthInPixel  = iCurLayerWidth;
  pCtx->pDec->iHeightInPixel = iCurLayerHeight;

  if ((pCurSlice->eSliceType != I_SLICE) && (pCurSlice->eSliceType != P_SLICE) && (pCurSlice->eSliceType != B_SLICE))
    return ERR_NONE; //no error but just ignore the type unsupported

  if (pCtx->pParam->bParseOnly) //for parse only, deblocking should not go on
    return ERR_NONE;

  pDeblockMb = WelsDeblockingMb;

  if (1 == pSliceHeader->uiDisableDeblockingFilterIdc
      || pCtx->pCurDqLayer->sLayerInfo.sSliceInLayer.iTotalMbInCurSlice <= 0) {
    return ERR_NONE;//NO_SUPPORTED_FILTER_IDX
  } else {
    WelsDeblockingFilterSlice (pCtx, pDeblockMb);
  }
  // any other filter_idc not supported here, 7/22/2010

  return ERR_NONE;
}

int32_t WelsMbInterSampleConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer,
                                       uint8_t* pDstY, uint8_t* pDstU, uint8_t* pDstV, int32_t iStrideL, int32_t iStrideC) {
  int32_t iMbXy = pCurLayer->iMbXyIndex;
  int32_t i, iIndex, iOffset;

  if (pCurLayer->pTransformSize8x8Flag[iMbXy]) {
    for (i = 0; i < 4; i++) {
      iIndex = g_kuiMbCountScan4Idx[i << 2];
      if (pCurLayer->pNzc[iMbXy][iIndex] || pCurLayer->pNzc[iMbXy][iIndex + 1] || pCurLayer->pNzc[iMbXy][iIndex + 4]
          || pCurLayer->pNzc[iMbXy][iIndex + 5]) {
        iOffset = ((iIndex >> 2) << 2) * iStrideL + ((iIndex % 4) << 2);
        pCtx->pIdctResAddPredFunc8x8 (pDstY + iOffset, iStrideL, pCurLayer->pScaledTCoeff[iMbXy] + (i << 6));
      }
    }
  } else {
    // luma.
    const int8_t* pNzc = pCurLayer->pNzc[iMbXy];
    int16_t* pScaledTCoeff = pCurLayer->pScaledTCoeff[iMbXy];
    pCtx->pIdctFourResAddPredFunc (pDstY + 0 * iStrideL + 0, iStrideL, pScaledTCoeff + 0 * 64, pNzc +  0);
    pCtx->pIdctFourResAddPredFunc (pDstY + 0 * iStrideL + 8, iStrideL, pScaledTCoeff + 1 * 64, pNzc +  2);
    pCtx->pIdctFourResAddPredFunc (pDstY + 8 * iStrideL + 0, iStrideL, pScaledTCoeff + 2 * 64, pNzc +  8);
    pCtx->pIdctFourResAddPredFunc (pDstY + 8 * iStrideL + 8, iStrideL, pScaledTCoeff + 3 * 64, pNzc + 10);
  }

  const int8_t* pNzc = pCurLayer->pNzc[iMbXy];
  int16_t* pScaledTCoeff = pCurLayer->pScaledTCoeff[iMbXy];
  // Cb.
  pCtx->pIdctFourResAddPredFunc (pDstU, iStrideC, pScaledTCoeff + 4 * 64, pNzc + 16);
  // Cr.
  pCtx->pIdctFourResAddPredFunc (pDstV, iStrideC, pScaledTCoeff + 5 * 64, pNzc + 18);

  return ERR_NONE;
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

  if (pCtx->eSliceType == P_SLICE) {
    GetInterPred (pDstY, pDstCb, pDstCr, pCtx);
  } else {
    if (pCtx->pTempDec == NULL)
      pCtx->pTempDec = AllocPicture (pCtx, pCtx->pSps->iMbWidth << 4, pCtx->pSps->iMbHeight << 4);
    uint8_t*   pTempDstYCbCr[3];
    uint8_t*   pDstYCbCr[3];
    pTempDstYCbCr[0] = pCtx->pTempDec->pData[0] + ((iMbY * iLumaStride + iMbX) << 4);
    pTempDstYCbCr[1] = pCtx->pTempDec->pData[1] + ((iMbY * iChromaStride + iMbX) << 3);
    pTempDstYCbCr[2] = pCtx->pTempDec->pData[2] + ((iMbY * iChromaStride + iMbX) << 3);
    pDstYCbCr[0] = pDstY;
    pDstYCbCr[1] = pDstCb;
    pDstYCbCr[2] = pDstCr;
    GetInterBPred (pDstYCbCr, pTempDstYCbCr, pCtx);
  }
  WelsMbInterSampleConstruction (pCtx, pCurLayer, pDstY, pDstCb, pDstCr, iLumaStride, iChromaStride);

  pCtx->sBlockFunc.pWelsSetNonZeroCountFunc (
    pCurLayer->pNzc[pCurLayer->iMbXyIndex]); // set all none-zero nzc to 1; dbk can be opti!
  return ERR_NONE;
}

void WelsLumaDcDequantIdct (int16_t* pBlock, int32_t iQp, PWelsDecoderContext pCtx) {
  const int32_t kiQMul = pCtx->bUseScalingList ? pCtx->pDequant_coeff4x4[0][iQp][0] : (g_kuiDequantCoeff[iQp][0] << 4);
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
    const int32_t kiI4 = i << 2; // 4*i
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

    pBlk[kiOffset] = ((kiZ0 + kiZ3) * kiQMul + (1 << 5)) >> 6; //FIXME think about merging this into decode_resdual
    pBlk[kiYOffset[1] + kiOffset] = ((kiZ1 + kiZ2) * kiQMul + (1 << 5)) >> 6;
    pBlk[kiYOffset[2] + kiOffset] = ((kiZ1 - kiZ2) * kiQMul + (1 << 5)) >> 6;
    pBlk[kiYOffset[3] + kiOffset] = ((kiZ0 - kiZ3) * kiQMul + (1 << 5)) >> 6;
  }
#undef STRIDE
}

int32_t WelsMbIntraPredictionConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer, bool bOutput) {
//seems IPCM should not enter this path
  int32_t iMbXy = pCurLayer->iMbXyIndex;

  WelsFillRecNeededMbInfo (pCtx, bOutput, pCurLayer);

  if (IS_INTRA16x16 (pCurLayer->pMbType[iMbXy])) {
    RecI16x16Mb (iMbXy, pCtx, pCurLayer->pScaledTCoeff[iMbXy], pCurLayer);
  } else if (IS_INTRA8x8 (pCurLayer->pMbType[iMbXy])) {
    RecI8x8Mb (iMbXy, pCtx, pCurLayer->pScaledTCoeff[iMbXy], pCurLayer);
  } else if (IS_INTRA4x4 (pCurLayer->pMbType[iMbXy])) {
    RecI4x4Mb (iMbXy, pCtx, pCurLayer->pScaledTCoeff[iMbXy], pCurLayer);
  }
  return ERR_NONE;
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

  if (pCtx->eSliceType == P_SLICE) {
    GetInterPred (pDstY, pDstCb, pDstCr, pCtx);
  } else {
    if (pCtx->pTempDec == NULL)
      pCtx->pTempDec = AllocPicture (pCtx, pCtx->pSps->iMbWidth << 4, pCtx->pSps->iMbHeight << 4);
    uint8_t*   pTempDstYCbCr[3];
    uint8_t*   pDstYCbCr[3];
    pTempDstYCbCr[0] = pCtx->pTempDec->pData[0] + ((iMbY * iLumaStride + iMbX) << 4);
    pTempDstYCbCr[1] = pCtx->pTempDec->pData[1] + ((iMbY * iChromaStride + iMbX) << 3);
    pTempDstYCbCr[2] = pCtx->pTempDec->pData[2] + ((iMbY * iChromaStride + iMbX) << 3);
    pDstYCbCr[0] = pDstY;
    pDstYCbCr[1] = pDstCb;
    pDstYCbCr[2] = pDstCr;
    GetInterBPred (pDstYCbCr, pTempDstYCbCr, pCtx);
  }
  return ERR_NONE;
}

int32_t WelsTargetMbConstruction (PWelsDecoderContext pCtx) {
  PDqLayer pCurLayer = pCtx->pCurDqLayer;
  if (MB_TYPE_INTRA_PCM == pCurLayer->pMbType[pCurLayer->iMbXyIndex]) {
    //already decoded and reconstructed when parsing
    return ERR_NONE;
  } else if (IS_INTRA (pCurLayer->pMbType[pCurLayer->iMbXyIndex])) {
    WelsMbIntraPredictionConstruction (pCtx, pCurLayer, 1);
  } else if (IS_INTER (pCurLayer->pMbType[pCurLayer->iMbXyIndex])) { //InterMB
    if (0 == pCurLayer->pCbp[pCurLayer->iMbXyIndex]) { //uiCbp==0 include SKIP
      WelsMbInterPrediction (pCtx, pCurLayer);
    } else {
      WelsMbInterConstruction (pCtx, pCurLayer);
    }
  } else {
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "WelsTargetMbConstruction():::::Unknown MB type: %d",
             pCurLayer->pMbType[pCurLayer->iMbXyIndex]);
    return ERR_INFO_MB_RECON_FAIL;
  }

  return ERR_NONE;
}

void WelsChromaDcIdct (int16_t* pBlock) {
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

  pBlk[0] = (iA + iC);
  pBlk[iXStride] = (iE + iB);
  pBlk[iStride] = (iA - iC);
  pBlk[iStride1] = (iE - iB);
}

void WelsMapNxNNeighToSampleNormal (PWelsNeighAvail pNeighAvail, int32_t* pSampleAvail) {
  if (pNeighAvail->iLeftAvail) {  //left
    pSampleAvail[ 6] =
      pSampleAvail[12] =
        pSampleAvail[18] =
          pSampleAvail[24] = 1;
  }
  if (pNeighAvail->iLeftTopAvail) { //top_left
    pSampleAvail[0] = 1;
  }
  if (pNeighAvail->iTopAvail) { //top
    pSampleAvail[1] =
      pSampleAvail[2] =
        pSampleAvail[3] =
          pSampleAvail[4] = 1;
  }
  if (pNeighAvail->iRightTopAvail) { //top_right
    pSampleAvail[5] = 1;
  }
}

void WelsMapNxNNeighToSampleConstrain1 (PWelsNeighAvail pNeighAvail, int32_t* pSampleAvail) {
  if (pNeighAvail->iLeftAvail && IS_INTRA (pNeighAvail->iLeftType)) {   //left
    pSampleAvail[ 6] =
      pSampleAvail[12] =
        pSampleAvail[18] =
          pSampleAvail[24] = 1;
  }
  if (pNeighAvail->iLeftTopAvail && IS_INTRA (pNeighAvail->iLeftTopType)) {  //top_left
    pSampleAvail[0] = 1;
  }
  if (pNeighAvail->iTopAvail && IS_INTRA (pNeighAvail->iTopType)) {  //top
    pSampleAvail[1] =
      pSampleAvail[2] =
        pSampleAvail[3] =
          pSampleAvail[4] = 1;
  }
  if (pNeighAvail->iRightTopAvail && IS_INTRA (pNeighAvail->iRightTopType)) {  //top_right
    pSampleAvail[5] = 1;
  }
}
void WelsMap16x16NeighToSampleNormal (PWelsNeighAvail pNeighAvail, uint8_t* pSampleAvail) {
  if (pNeighAvail->iLeftAvail) {
    *pSampleAvail = (1 << 2);
  }
  if (pNeighAvail->iLeftTopAvail) {
    *pSampleAvail |= (1 << 1);
  }
  if (pNeighAvail->iTopAvail) {
    *pSampleAvail |= 1;
  }
}

void WelsMap16x16NeighToSampleConstrain1 (PWelsNeighAvail pNeighAvail, uint8_t* pSampleAvail) {
  if (pNeighAvail->iLeftAvail && IS_INTRA (pNeighAvail->iLeftType)) {
    *pSampleAvail = (1 << 2);
  }
  if (pNeighAvail->iLeftTopAvail && IS_INTRA (pNeighAvail->iLeftTopType)) {
    *pSampleAvail |= (1 << 1);
  }
  if (pNeighAvail->iTopAvail && IS_INTRA (pNeighAvail->iTopType)) {
    *pSampleAvail |= 1;
  }
}

int32_t ParseIntra4x4Mode (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, int8_t* pIntraPredMode,
                           PBitStringAux pBs,
                           PDqLayer pCurDqLayer) {
  int32_t iSampleAvail[5 * 6] = { 0 }; //initialize as 0
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  int32_t iFinalMode, i;

  uint8_t uiNeighAvail = 0;
  uint32_t uiCode;
  int32_t iCode;
  pCtx->pMapNxNNeighToSampleFunc (pNeighAvail, iSampleAvail);
  uiNeighAvail = (iSampleAvail[6] << 2) | (iSampleAvail[0] << 1) | (iSampleAvail[1]);
  for (i = 0; i < 16; i++) {
    int32_t iPrevIntra4x4PredMode = 0;
    if (pCurDqLayer->sLayerInfo.pPps->bEntropyCodingModeFlag) {
      WELS_READ_VERIFY (ParseIntraPredModeLumaCabac (pCtx, iCode));
      iPrevIntra4x4PredMode = iCode;
    } else {
      WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode));
      iPrevIntra4x4PredMode = uiCode;
    }
    const int32_t kiPredMode = PredIntra4x4Mode (pIntraPredMode, i);

    int8_t iBestMode;
    if (pCurDqLayer->sLayerInfo.pPps->bEntropyCodingModeFlag) {
      if (iPrevIntra4x4PredMode == -1)
        iBestMode = kiPredMode;
      else
        iBestMode = iPrevIntra4x4PredMode + (iPrevIntra4x4PredMode >= kiPredMode);
    } else {
      if (iPrevIntra4x4PredMode) {
        iBestMode = kiPredMode;
      } else {
        WELS_READ_VERIFY (BsGetBits (pBs, 3, &uiCode));
        iBestMode = uiCode + ((int32_t) uiCode >= kiPredMode);
      }
    }

    iFinalMode = CheckIntraNxNPredMode (&iSampleAvail[0], &iBestMode, i, false);
    if (iFinalMode == GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INVALID_INTRA4X4_MODE)) {
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I4x4_PRED_MODE);
    }

    pCurDqLayer->pIntra4x4FinalMode[iMbXy][g_kuiScan4[i]] = iFinalMode;

    pIntraPredMode[g_kuiScan8[i]] = iBestMode;

    iSampleAvail[g_kuiCache30ScanIdx[i]] = 1;
  }
  ST32 (&pCurDqLayer->pIntraPredMode[iMbXy][0], LD32 (&pIntraPredMode[1 + 8 * 4]));
  pCurDqLayer->pIntraPredMode[iMbXy][4] = pIntraPredMode[4 + 8 * 1];
  pCurDqLayer->pIntraPredMode[iMbXy][5] = pIntraPredMode[4 + 8 * 2];
  pCurDqLayer->pIntraPredMode[iMbXy][6] = pIntraPredMode[4 + 8 * 3];

  if (pCtx->pSps->uiChromaFormatIdc == 0)//no need parse chroma
    return ERR_NONE;

  if (pCurDqLayer->sLayerInfo.pPps->bEntropyCodingModeFlag) {
    WELS_READ_VERIFY (ParseIntraPredModeChromaCabac (pCtx, uiNeighAvail, iCode));
    if (iCode > MAX_PRED_MODE_ID_CHROMA) {
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I_CHROMA_PRED_MODE);
    }
    pCurDqLayer->pChromaPredMode[iMbXy] = iCode;
  } else {
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //intra_chroma_pred_mode
    if (uiCode > MAX_PRED_MODE_ID_CHROMA) {
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I_CHROMA_PRED_MODE);
    }
    pCurDqLayer->pChromaPredMode[iMbXy] = uiCode;
  }

  if (-1 == pCurDqLayer->pChromaPredMode[iMbXy]
      || CheckIntraChromaPredMode (uiNeighAvail, &pCurDqLayer->pChromaPredMode[iMbXy])) {
    return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I_CHROMA_PRED_MODE);
  }
  return ERR_NONE;
}

int32_t ParseIntra8x8Mode (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, int8_t* pIntraPredMode,
                           PBitStringAux pBs,
                           PDqLayer pCurDqLayer) {
  // Similar with Intra_4x4, can put them together when needed
  int32_t iSampleAvail[5 * 6] = { 0 }; //initialize as 0
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  int32_t iFinalMode, i;

  uint8_t uiNeighAvail = 0;
  uint32_t uiCode;
  int32_t iCode;
  pCtx->pMapNxNNeighToSampleFunc (pNeighAvail, iSampleAvail);
  // Top-Right : Left : Top-Left : Top
  uiNeighAvail = (iSampleAvail[5] << 3) | (iSampleAvail[6] << 2) | (iSampleAvail[0] << 1) | (iSampleAvail[1]);

  pCurDqLayer->pIntraNxNAvailFlag[iMbXy] = uiNeighAvail;

  for (i = 0; i < 4; i++) {
    int32_t iPrevIntra4x4PredMode = 0;
    if (pCurDqLayer->sLayerInfo.pPps->bEntropyCodingModeFlag) {
      WELS_READ_VERIFY (ParseIntraPredModeLumaCabac (pCtx, iCode));
      iPrevIntra4x4PredMode = iCode;
    } else {
      WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode));
      iPrevIntra4x4PredMode = uiCode;
    }
    const int32_t kiPredMode = PredIntra4x4Mode (pIntraPredMode, i << 2);

    int8_t iBestMode;
    if (pCurDqLayer->sLayerInfo.pPps->bEntropyCodingModeFlag) {
      if (iPrevIntra4x4PredMode == -1)
        iBestMode = kiPredMode;
      else
        iBestMode = iPrevIntra4x4PredMode + (iPrevIntra4x4PredMode >= kiPredMode);
    } else {
      if (iPrevIntra4x4PredMode) {
        iBestMode = kiPredMode;
      } else {
        WELS_READ_VERIFY (BsGetBits (pBs, 3, &uiCode));
        iBestMode = uiCode + ((int32_t) uiCode >= kiPredMode);
      }
    }

    iFinalMode = CheckIntraNxNPredMode (&iSampleAvail[0], &iBestMode, i << 2, true);

    if (iFinalMode == GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INVALID_INTRA4X4_MODE)) {
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I4x4_PRED_MODE);
    }

    for (int j = 0; j < 4; j++) {
      pCurDqLayer->pIntra4x4FinalMode[iMbXy][g_kuiScan4[ (i << 2) + j]] = iFinalMode;
      pIntraPredMode[g_kuiScan8[ (i << 2) + j]] = iBestMode;
      iSampleAvail[g_kuiCache30ScanIdx[ (i << 2) + j]] = 1;
    }
  }
  ST32 (&pCurDqLayer->pIntraPredMode[iMbXy][0], LD32 (&pIntraPredMode[1 + 8 * 4]));
  pCurDqLayer->pIntraPredMode[iMbXy][4] = pIntraPredMode[4 + 8 * 1];
  pCurDqLayer->pIntraPredMode[iMbXy][5] = pIntraPredMode[4 + 8 * 2];
  pCurDqLayer->pIntraPredMode[iMbXy][6] = pIntraPredMode[4 + 8 * 3];

  if (pCtx->pSps->uiChromaFormatIdc == 0)
    return ERR_NONE;

  if (pCurDqLayer->sLayerInfo.pPps->bEntropyCodingModeFlag) {
    WELS_READ_VERIFY (ParseIntraPredModeChromaCabac (pCtx, uiNeighAvail, iCode));
    if (iCode > MAX_PRED_MODE_ID_CHROMA) {
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I_CHROMA_PRED_MODE);
    }
    pCurDqLayer->pChromaPredMode[iMbXy] = iCode;
  } else {
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //intra_chroma_pred_mode
    if (uiCode > MAX_PRED_MODE_ID_CHROMA) {
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I_CHROMA_PRED_MODE);
    }
    pCurDqLayer->pChromaPredMode[iMbXy] = uiCode;
  }

  if (-1 == pCurDqLayer->pChromaPredMode[iMbXy]
      || CheckIntraChromaPredMode (uiNeighAvail, &pCurDqLayer->pChromaPredMode[iMbXy])) {
    return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I_CHROMA_PRED_MODE);
  }

  return ERR_NONE;
}

int32_t ParseIntra16x16Mode (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, PBitStringAux pBs,
                             PDqLayer pCurDqLayer) {
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  uint8_t uiNeighAvail = 0; //0x07 = 0 1 1 1, means left, top-left, top avail or not. (1: avail, 0: unavail)
  uint32_t uiCode;
  int32_t iCode;
  pCtx->pMap16x16NeighToSampleFunc (pNeighAvail, &uiNeighAvail);

  if (CheckIntra16x16PredMode (uiNeighAvail,
                               &pCurDqLayer->pIntraPredMode[iMbXy][7])) { //invalid iPredMode, must stop decoding
    return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I16x16_PRED_MODE);
  }
  if (pCtx->pSps->uiChromaFormatIdc == 0)
    return ERR_NONE;

  if (pCurDqLayer->sLayerInfo.pPps->bEntropyCodingModeFlag) {
    WELS_READ_VERIFY (ParseIntraPredModeChromaCabac (pCtx, uiNeighAvail, iCode));
    if (iCode > MAX_PRED_MODE_ID_CHROMA) {
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I_CHROMA_PRED_MODE);
    }
    pCurDqLayer->pChromaPredMode[iMbXy] = iCode;
  } else {
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //intra_chroma_pred_mode
    if (uiCode > MAX_PRED_MODE_ID_CHROMA) {
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I_CHROMA_PRED_MODE);
    }
    pCurDqLayer->pChromaPredMode[iMbXy] = uiCode;
  }
  if (-1 == pCurDqLayer->pChromaPredMode[iMbXy]
      || CheckIntraChromaPredMode (uiNeighAvail, &pCurDqLayer->pChromaPredMode[iMbXy])) {
    return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_I_CHROMA_PRED_MODE);
  }

  return ERR_NONE;
}

int32_t WelsDecodeMbCabacISliceBaseMode0 (PWelsDecoderContext pCtx, uint32_t& uiEosFlag) {
  PDqLayer pCurLayer             = pCtx->pCurDqLayer;
  PBitStringAux pBsAux           = pCurLayer->pBitStringAux;
  PSlice pSlice                  = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader      = &pSlice->sSliceHeaderExt.sSliceHeader;
  SWelsNeighAvail sNeighAvail;
  int32_t iScanIdxStart = pSlice->sSliceHeaderExt.uiScanIdxStart;
  int32_t iScanIdxEnd   = pSlice->sSliceHeaderExt.uiScanIdxEnd;
  int32_t iMbXy = pCurLayer->iMbXyIndex;
  int32_t i;
  uint32_t uiMbType = 0, uiCbp = 0, uiCbpLuma = 0, uiCbpChroma = 0;

  ENFORCE_STACK_ALIGN_1D (uint8_t, pNonZeroCount, 48, 16);

  pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] = true;
  pCurLayer->pTransformSize8x8Flag[iMbXy] = false;

  pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
  pCurLayer->pResidualPredFlag[iMbXy] = pSlice->sSliceHeaderExt.bDefaultResidualPredFlag;
  GetNeighborAvailMbType (&sNeighAvail, pCurLayer);
  WELS_READ_VERIFY (ParseMBTypeISliceCabac (pCtx, &sNeighAvail, uiMbType));
  if (uiMbType > 25) {
    return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_TYPE);
  } else if (!pCtx->pSps->uiChromaFormatIdc && ((uiMbType >= 5 && uiMbType <= 12) || (uiMbType >= 17
             && uiMbType <= 24))) {
    return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_TYPE);
  } else if (25 == uiMbType) {   //I_PCM
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_DEBUG, "I_PCM mode exists in I slice!");
    WELS_READ_VERIFY (ParseIPCMInfoCabac (pCtx));
    pSlice->iLastDeltaQp = 0;
    WELS_READ_VERIFY (ParseEndOfSliceCabac (pCtx, uiEosFlag));
    if (uiEosFlag) {
      RestoreCabacDecEngineToBS (pCtx->pCabacDecEngine, pCtx->pCurDqLayer->pBitStringAux);
    }
    return ERR_NONE;
  } else if (0 == uiMbType) { //I4x4
    ENFORCE_STACK_ALIGN_1D (int8_t, pIntraPredMode, 48, 16);
    pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA4x4;
    if (pCtx->pPps->bTransform8x8ModeFlag) {
      // Transform 8x8 cabac will be added soon
      WELS_READ_VERIFY (ParseTransformSize8x8FlagCabac (pCtx, &sNeighAvail, pCtx->pCurDqLayer->pTransformSize8x8Flag[iMbXy]));
    }
    if (pCtx->pCurDqLayer->pTransformSize8x8Flag[iMbXy]) {
      uiMbType = pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA8x8;
      pCtx->pFillInfoCacheIntraNxNFunc (&sNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
      WELS_READ_VERIFY (ParseIntra8x8Mode (pCtx, &sNeighAvail, pIntraPredMode, pBsAux, pCurLayer));
    } else {
      pCtx->pFillInfoCacheIntraNxNFunc (&sNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
      WELS_READ_VERIFY (ParseIntra4x4Mode (pCtx, &sNeighAvail, pIntraPredMode, pBsAux, pCurLayer));
    }
    //get uiCbp for I4x4
    WELS_READ_VERIFY (ParseCbpInfoCabac (pCtx, &sNeighAvail, uiCbp));
    pCurLayer->pCbp[iMbXy] = uiCbp;
    pSlice->iLastDeltaQp = uiCbp == 0 ? 0 : pSlice->iLastDeltaQp;
    uiCbpChroma = pCtx->pSps->uiChromaFormatIdc ? uiCbp >> 4 : 0;
    uiCbpLuma = uiCbp & 15;
  } else { //I16x16;
    pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA16x16;
    pCurLayer->pTransformSize8x8Flag[iMbXy] = false;
    pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] = true;
    pCurLayer->pIntraPredMode[iMbXy][7] = (uiMbType - 1) & 3;
    pCurLayer->pCbp[iMbXy] = g_kuiI16CbpTable[ (uiMbType - 1) >> 2];
    uiCbpChroma = pCtx->pSps->uiChromaFormatIdc ? pCurLayer->pCbp[iMbXy] >> 4 : 0 ;
    uiCbpLuma = pCurLayer->pCbp[iMbXy] & 15;
    WelsFillCacheNonZeroCount (&sNeighAvail, pNonZeroCount, pCurLayer);
    WELS_READ_VERIFY (ParseIntra16x16Mode (pCtx, &sNeighAvail, pBsAux, pCurLayer));
  }

  ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][20], 0);
  pCurLayer->pCbfDc[iMbXy] = 0;

  if (pCurLayer->pCbp[iMbXy] == 0 && IS_INTRANxN (pCurLayer->pMbType[iMbXy])) {
    pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp;
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 ((pCurLayer->pLumaQp[iMbXy] +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i]), 0, 51)];
    }
  }

  if (pCurLayer->pCbp[iMbXy] || MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
    memset (pCurLayer->pScaledTCoeff[iMbXy], 0, 384 * sizeof (pCurLayer->pScaledTCoeff[iMbXy][0]));
    int32_t iQpDelta, iId8x8, iId4x4;
    WELS_READ_VERIFY (ParseDeltaQpCabac (pCtx, iQpDelta));
    if (iQpDelta > 25 || iQpDelta < -26) {//out of iQpDelta range
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_QP);
    }
    pCurLayer->pLumaQp[iMbXy] = (pSlice->iLastMbQp + iQpDelta + 52) % 52; //update last_mb_qp
    pSlice->iLastMbQp = pCurLayer->pLumaQp[iMbXy];
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 ((pSlice->iLastMbQp +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i]), 0, 51)];
    }
    if (MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
      //step1: Luma DC
      WELS_READ_VERIFY (ParseResidualBlockCabac (&sNeighAvail, pNonZeroCount, pBsAux, 0, 16, g_kuiLumaDcZigzagScan,
                        I16_LUMA_DC, pCurLayer->pScaledTCoeff[iMbXy], pCurLayer->pLumaQp[iMbXy], pCtx));
      //step2: Luma AC
      if (uiCbpLuma) {
        for (i = 0; i < 16; i++) {
          WELS_READ_VERIFY (ParseResidualBlockCabac (&sNeighAvail, pNonZeroCount, pBsAux, i,
                            iScanIdxEnd - WELS_MAX (iScanIdxStart, 1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1), I16_LUMA_AC,
                            pCurLayer->pScaledTCoeff[iMbXy] + (i << 4), pCurLayer->pLumaQp[iMbXy], pCtx));
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
      if (pCurLayer->pTransformSize8x8Flag[iMbXy]) {
        // Transform 8x8 support for CABAC
        for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
          if (uiCbpLuma & (1 << iId8x8)) {
            WELS_READ_VERIFY (ParseResidualBlockCabac8x8 (&sNeighAvail, pNonZeroCount, pBsAux, (iId8x8 << 2),
                              iScanIdxEnd - iScanIdxStart + 1, g_kuiZigzagScan8x8 + iScanIdxStart, LUMA_DC_AC_INTRA_8,
                              pCurLayer->pScaledTCoeff[iMbXy] + (iId8x8 << 6), pCurLayer->pLumaQp[iMbXy], pCtx));
          } else {
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[ (iId8x8 << 2)]], 0);
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[ (iId8x8 << 2) + 2]], 0);
          }
        }
        ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      } else {
        for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
          if (uiCbpLuma & (1 << iId8x8)) {
            int32_t iIdx = (iId8x8 << 2);
            for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
              //Luma (DC and AC decoding together)
              WELS_READ_VERIFY (ParseResidualBlockCabac (&sNeighAvail, pNonZeroCount, pBsAux, iIdx, iScanIdxEnd - iScanIdxStart + 1,
                                g_kuiZigzagScan + iScanIdxStart, LUMA_DC_AC_INTRA, pCurLayer->pScaledTCoeff[iMbXy] + (iIdx << 4),
                                pCurLayer->pLumaQp[iMbXy], pCtx));
              iIdx++;
            }
          } else {
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[ (iId8x8 << 2)]], 0);
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[ (iId8x8 << 2) + 2]], 0);
          }
        }
        ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      }
    }
    int32_t iMbResProperty;
    //chroma
    //step1: DC
    if (1 == uiCbpChroma || 2 == uiCbpChroma) {
      //Cb Cr
      for (i = 0; i < 2; i++) {
        iMbResProperty = i ? CHROMA_DC_V : CHROMA_DC_U;
        WELS_READ_VERIFY (ParseResidualBlockCabac (&sNeighAvail, pNonZeroCount, pBsAux, 16 + (i << 2), 4, g_kuiChromaDcScan,
                          iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + 256 + (i << 6), pCurLayer->pChromaQp[iMbXy][i], pCtx));
      }
    }

    //step2: AC
    if (2 == uiCbpChroma) {
      for (i = 0; i < 2; i++) { //Cb Cr
        iMbResProperty = i ? CHROMA_AC_V : CHROMA_AC_U;
        int32_t iIdx = 16 + (i << 2);
        for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
          WELS_READ_VERIFY (ParseResidualBlockCabac (&sNeighAvail, pNonZeroCount, pBsAux, iIdx,
                            iScanIdxEnd - WELS_MAX (iScanIdxStart, 1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1), iMbResProperty,
                            pCurLayer->pScaledTCoeff[iMbXy] + (iIdx << 4), pCurLayer->pChromaQp[iMbXy][i], pCtx));
          iIdx++;
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
  } else {
    ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][20], 0);
  }

  WELS_READ_VERIFY (ParseEndOfSliceCabac (pCtx, uiEosFlag));
  if (uiEosFlag) {
    RestoreCabacDecEngineToBS (pCtx->pCabacDecEngine, pCtx->pCurDqLayer->pBitStringAux);
  }
  return ERR_NONE;
}

int32_t WelsDecodeMbCabacISlice (PWelsDecoderContext pCtx, PNalUnit pNalCur, uint32_t& uiEosFlag) {
  WELS_READ_VERIFY (WelsDecodeMbCabacISliceBaseMode0 (pCtx, uiEosFlag));
  return ERR_NONE;
}

int32_t WelsDecodeMbCabacPSliceBaseMode0 (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, uint32_t& uiEosFlag) {
  PDqLayer pCurLayer             = pCtx->pCurDqLayer;
  PBitStringAux pBsAux           = pCurLayer->pBitStringAux;
  PSlice pSlice                  = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader      = &pSlice->sSliceHeaderExt.sSliceHeader;

  int32_t iScanIdxStart = pSlice->sSliceHeaderExt.uiScanIdxStart;
  int32_t iScanIdxEnd   = pSlice->sSliceHeaderExt.uiScanIdxEnd;
  int32_t iMbXy = pCurLayer->iMbXyIndex;
  int32_t iMbResProperty;
  int32_t i;
  uint32_t uiMbType = 0, uiCbp = 0, uiCbpLuma = 0, uiCbpChroma = 0;

  ENFORCE_STACK_ALIGN_1D (uint8_t, pNonZeroCount, 48, 16);

  pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;

  WELS_READ_VERIFY (ParseMBTypePSliceCabac (pCtx, pNeighAvail, uiMbType));
  // uiMbType = 4 is not allowded.
  if (uiMbType < 4) { //Inter mode
    int16_t pMotionVector[LIST_A][30][MV_A];
    int16_t pMvdCache[LIST_A][30][MV_A];
    int8_t  pRefIndex[LIST_A][30];
    pCurLayer->pMbType[iMbXy] = g_ksInterPMbTypeInfo[uiMbType].iType;
    WelsFillCacheInterCabac (pNeighAvail, pNonZeroCount, pMotionVector, pMvdCache, pRefIndex, pCurLayer);
    WELS_READ_VERIFY (ParseInterPMotionInfoCabac (pCtx, pNeighAvail, pNonZeroCount, pMotionVector, pMvdCache, pRefIndex));
    pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
  } else { //Intra mode
    uiMbType -= 5;
    if (uiMbType > 25)
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_TYPE);
    if (!pCtx->pSps->uiChromaFormatIdc && ((uiMbType >= 5 && uiMbType <= 12) || (uiMbType >= 17 && uiMbType <= 24)))
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_TYPE);

    if (25 == uiMbType) {   //I_PCM
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_DEBUG, "I_PCM mode exists in P slice!");
      WELS_READ_VERIFY (ParseIPCMInfoCabac (pCtx));
      pSlice->iLastDeltaQp = 0;
      WELS_READ_VERIFY (ParseEndOfSliceCabac (pCtx, uiEosFlag));
      if (uiEosFlag) {
        RestoreCabacDecEngineToBS (pCtx->pCabacDecEngine, pCtx->pCurDqLayer->pBitStringAux);
      }
      return ERR_NONE;
    } else { //normal Intra mode
      if (0 == uiMbType) { //Intra4x4
        ENFORCE_STACK_ALIGN_1D (int8_t, pIntraPredMode, 48, 16);
        pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA4x4;
        if (pCtx->pPps->bTransform8x8ModeFlag) {
          WELS_READ_VERIFY (ParseTransformSize8x8FlagCabac (pCtx, pNeighAvail, pCtx->pCurDqLayer->pTransformSize8x8Flag[iMbXy]));
        }
        if (pCtx->pCurDqLayer->pTransformSize8x8Flag[iMbXy]) {
          uiMbType = pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA8x8;
          pCtx->pFillInfoCacheIntraNxNFunc (pNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
          WELS_READ_VERIFY (ParseIntra8x8Mode (pCtx, pNeighAvail, pIntraPredMode, pBsAux, pCurLayer));
        } else {
          pCtx->pFillInfoCacheIntraNxNFunc (pNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
          WELS_READ_VERIFY (ParseIntra4x4Mode (pCtx, pNeighAvail, pIntraPredMode, pBsAux, pCurLayer));
        }
      } else { //Intra16x16
        pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA16x16;
        pCurLayer->pTransformSize8x8Flag[iMbXy] = false;
        pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] = true;
        pCurLayer->pIntraPredMode[iMbXy][7] = (uiMbType - 1) & 3;
        pCurLayer->pCbp[iMbXy] = g_kuiI16CbpTable[ (uiMbType - 1) >> 2];
        uiCbpChroma = pCtx->pSps->uiChromaFormatIdc ? pCurLayer->pCbp[iMbXy] >> 4 : 0;
        uiCbpLuma = pCurLayer->pCbp[iMbXy] & 15;
        WelsFillCacheNonZeroCount (pNeighAvail, pNonZeroCount, pCurLayer);
        WELS_READ_VERIFY (ParseIntra16x16Mode (pCtx, pNeighAvail, pBsAux, pCurLayer));
      }
    }
  }

  ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][20], 0);

  if (MB_TYPE_INTRA16x16 != pCurLayer->pMbType[iMbXy]) {
    WELS_READ_VERIFY (ParseCbpInfoCabac (pCtx, pNeighAvail, uiCbp));

    pCurLayer->pCbp[iMbXy] = uiCbp;
    pSlice->iLastDeltaQp = uiCbp == 0 ? 0 : pSlice->iLastDeltaQp;
    uiCbpChroma = pCtx->pSps->uiChromaFormatIdc ? pCurLayer->pCbp[iMbXy] >> 4 : 0 ;
    uiCbpLuma = pCurLayer->pCbp[iMbXy] & 15;
  }

  if (pCurLayer->pCbp[iMbXy] || MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {

    if (MB_TYPE_INTRA16x16 != pCurLayer->pMbType[iMbXy]) {
      // Need modification when B picutre add in
      bool bNeedParseTransformSize8x8Flag =
        (((pCurLayer->pMbType[iMbXy] >= MB_TYPE_16x16 && pCurLayer->pMbType[iMbXy] <= MB_TYPE_8x16)
          || pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy])
         && (pCurLayer->pMbType[iMbXy] != MB_TYPE_INTRA8x8)
         && (pCurLayer->pMbType[iMbXy] != MB_TYPE_INTRA4x4)
         && ((pCurLayer->pCbp[iMbXy] & 0x0F) > 0)
         && (pCtx->pPps->bTransform8x8ModeFlag));

      if (bNeedParseTransformSize8x8Flag) {
        WELS_READ_VERIFY (ParseTransformSize8x8FlagCabac (pCtx, pNeighAvail,
                          pCtx->pCurDqLayer->pTransformSize8x8Flag[iMbXy])); //transform_size_8x8_flag
      }
    }

    memset (pCurLayer->pScaledTCoeff[iMbXy], 0, 384 * sizeof (pCurLayer->pScaledTCoeff[iMbXy][0]));

    int32_t iQpDelta, iId8x8, iId4x4;

    WELS_READ_VERIFY (ParseDeltaQpCabac (pCtx, iQpDelta));
    if (iQpDelta > 25 || iQpDelta < -26) { //out of iQpDelta range
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_QP);
    }
    pCurLayer->pLumaQp[iMbXy] = (pSlice->iLastMbQp + iQpDelta + 52) % 52; //update last_mb_qp
    pSlice->iLastMbQp = pCurLayer->pLumaQp[iMbXy];
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pSlice->iLastMbQp +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i], 0, 51)];
    }

    if (MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
      //step1: Luma DC
      WELS_READ_VERIFY (ParseResidualBlockCabac (pNeighAvail, pNonZeroCount, pBsAux, 0, 16, g_kuiLumaDcZigzagScan,
                        I16_LUMA_DC, pCurLayer->pScaledTCoeff[iMbXy], pCurLayer->pLumaQp[iMbXy], pCtx));
      //step2: Luma AC
      if (uiCbpLuma) {
        for (i = 0; i < 16; i++) {
          WELS_READ_VERIFY (ParseResidualBlockCabac (pNeighAvail, pNonZeroCount, pBsAux, i, iScanIdxEnd - WELS_MAX (iScanIdxStart,
                            1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1), I16_LUMA_AC, pCurLayer->pScaledTCoeff[iMbXy] + (i << 4),
                            pCurLayer->pLumaQp[iMbXy], pCtx));
        }
        ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      } else {
        ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
      }
    } else { //non-MB_TYPE_INTRA16x16
      if (pCtx->pCurDqLayer->pTransformSize8x8Flag[iMbXy]) {
        // Transform 8x8 support for CABAC
        for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
          if (uiCbpLuma & (1 << iId8x8)) {
            WELS_READ_VERIFY (ParseResidualBlockCabac8x8 (pNeighAvail, pNonZeroCount, pBsAux, (iId8x8 << 2),
                              iScanIdxEnd - iScanIdxStart + 1, g_kuiZigzagScan8x8 + iScanIdxStart,
                              IS_INTRA (pCurLayer->pMbType[iMbXy]) ? LUMA_DC_AC_INTRA_8 : LUMA_DC_AC_INTER_8,
                              pCurLayer->pScaledTCoeff[iMbXy] + (iId8x8 << 6), pCurLayer->pLumaQp[iMbXy], pCtx));
          } else {
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[ (iId8x8 << 2)]], 0);
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[ (iId8x8 << 2) + 2]], 0);
          }
        }
        ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      } else {
        iMbResProperty = (IS_INTRA (pCurLayer->pMbType[iMbXy])) ? LUMA_DC_AC_INTRA : LUMA_DC_AC_INTER;
        for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
          if (uiCbpLuma & (1 << iId8x8)) {
            int32_t iIdx = (iId8x8 << 2);
            for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
              //Luma (DC and AC decoding together)
              WELS_READ_VERIFY (ParseResidualBlockCabac (pNeighAvail, pNonZeroCount, pBsAux, iIdx, iScanIdxEnd - iScanIdxStart + 1,
                                g_kuiZigzagScan + iScanIdxStart, iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + (iIdx << 4),
                                pCurLayer->pLumaQp[iMbXy],
                                pCtx));
              iIdx++;
            }
          } else {
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[iId8x8 << 2]], 0);
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[ (iId8x8 << 2) + 2]], 0);
          }
        }
        ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      }
    }

    //chroma
    //step1: DC
    if (1 == uiCbpChroma || 2 == uiCbpChroma) {
      for (i = 0; i < 2; i++) {
        if (IS_INTRA (pCurLayer->pMbType[iMbXy]))
          iMbResProperty = i ? CHROMA_DC_V : CHROMA_DC_U;
        else
          iMbResProperty = i ? CHROMA_DC_V_INTER : CHROMA_DC_U_INTER;

        WELS_READ_VERIFY (ParseResidualBlockCabac (pNeighAvail, pNonZeroCount, pBsAux, 16 + (i << 2), 4, g_kuiChromaDcScan,
                          iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + 256 + (i << 6), pCurLayer->pChromaQp[iMbXy][i], pCtx));
      }
    }
    //step2: AC
    if (2 == uiCbpChroma) {
      for (i = 0; i < 2; i++) {
        if (IS_INTRA (pCurLayer->pMbType[iMbXy]))
          iMbResProperty = i ? CHROMA_AC_V : CHROMA_AC_U;
        else
          iMbResProperty = i ? CHROMA_AC_V_INTER : CHROMA_AC_U_INTER;
        int32_t index = 16 + (i << 2);
        for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
          WELS_READ_VERIFY (ParseResidualBlockCabac (pNeighAvail, pNonZeroCount, pBsAux, index,
                            iScanIdxEnd - WELS_MAX (iScanIdxStart, 1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1),
                            iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + (index << 4), pCurLayer->pChromaQp[iMbXy][i], pCtx));
          index++;
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
  } else {
    pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp;
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pCurLayer->pLumaQp[iMbXy] +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i], 0, 51)];
    }
  }

  WELS_READ_VERIFY (ParseEndOfSliceCabac (pCtx, uiEosFlag));
  if (uiEosFlag) {
    RestoreCabacDecEngineToBS (pCtx->pCabacDecEngine, pCtx->pCurDqLayer->pBitStringAux);
  }

  return ERR_NONE;
}

int32_t WelsDecodeMbCabacBSliceBaseMode0 (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, uint32_t& uiEosFlag) {
  PDqLayer pCurLayer = pCtx->pCurDqLayer;
  PBitStringAux pBsAux = pCurLayer->pBitStringAux;
  PSlice pSlice = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader = &pSlice->sSliceHeaderExt.sSliceHeader;

  int32_t iScanIdxStart = pSlice->sSliceHeaderExt.uiScanIdxStart;
  int32_t iScanIdxEnd = pSlice->sSliceHeaderExt.uiScanIdxEnd;
  int32_t iMbXy = pCurLayer->iMbXyIndex;
  int32_t iMbResProperty;
  int32_t i;
  uint32_t uiMbType = 0, uiCbp = 0, uiCbpLuma = 0, uiCbpChroma = 0;

  ENFORCE_STACK_ALIGN_1D (uint8_t, pNonZeroCount, 48, 16);

  pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;

  WELS_READ_VERIFY (ParseMBTypeBSliceCabac (pCtx, pNeighAvail, uiMbType));

  if (uiMbType < 23) { //Inter B mode
    int16_t pMotionVector[LIST_A][30][MV_A];
    int16_t pMvdCache[LIST_A][30][MV_A];
    int8_t  pRefIndex[LIST_A][30];
    int8_t  pDirect[30];
    pCurLayer->pMbType[iMbXy] = g_ksInterBMbTypeInfo[uiMbType].iType;
    WelsFillCacheInterCabac (pNeighAvail, pNonZeroCount, pMotionVector, pMvdCache, pRefIndex, pCurLayer);
    WelsFillDirectCacheCabac (pNeighAvail, pDirect, pCurLayer);
    WELS_READ_VERIFY (ParseInterBMotionInfoCabac (pCtx, pNeighAvail, pNonZeroCount, pMotionVector, pMvdCache, pRefIndex,
                      pDirect));
    pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
  } else { //Intra mode
    uiMbType -= 23;
    if (uiMbType > 25)
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_TYPE);
    if (!pCtx->pSps->uiChromaFormatIdc && ((uiMbType >= 5 && uiMbType <= 12) || (uiMbType >= 17 && uiMbType <= 24)))
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_TYPE);

    if (25 == uiMbType) {   //I_PCM
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_DEBUG, "I_PCM mode exists in P slice!");
      WELS_READ_VERIFY (ParseIPCMInfoCabac (pCtx));
      pSlice->iLastDeltaQp = 0;
      WELS_READ_VERIFY (ParseEndOfSliceCabac (pCtx, uiEosFlag));
      if (uiEosFlag) {
        RestoreCabacDecEngineToBS (pCtx->pCabacDecEngine, pCtx->pCurDqLayer->pBitStringAux);
      }
      return ERR_NONE;
    } else { //normal Intra mode
      if (0 == uiMbType) { //Intra4x4
        ENFORCE_STACK_ALIGN_1D (int8_t, pIntraPredMode, 48, 16);
        pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA4x4;
        if (pCtx->pPps->bTransform8x8ModeFlag) {
          WELS_READ_VERIFY (ParseTransformSize8x8FlagCabac (pCtx, pNeighAvail, pCtx->pCurDqLayer->pTransformSize8x8Flag[iMbXy]));
        }
        if (pCtx->pCurDqLayer->pTransformSize8x8Flag[iMbXy]) {
          uiMbType = pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA8x8;
          pCtx->pFillInfoCacheIntraNxNFunc (pNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
          WELS_READ_VERIFY (ParseIntra8x8Mode (pCtx, pNeighAvail, pIntraPredMode, pBsAux, pCurLayer));
        } else {
          pCtx->pFillInfoCacheIntraNxNFunc (pNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
          WELS_READ_VERIFY (ParseIntra4x4Mode (pCtx, pNeighAvail, pIntraPredMode, pBsAux, pCurLayer));
        }
      } else { //Intra16x16
        pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA16x16;
        pCurLayer->pTransformSize8x8Flag[iMbXy] = false;
        pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] = true;
        pCurLayer->pIntraPredMode[iMbXy][7] = (uiMbType - 1) & 3;
        pCurLayer->pCbp[iMbXy] = g_kuiI16CbpTable[ (uiMbType - 1) >> 2];
        uiCbpChroma = pCtx->pSps->uiChromaFormatIdc ? pCurLayer->pCbp[iMbXy] >> 4 : 0;
        uiCbpLuma = pCurLayer->pCbp[iMbXy] & 15;
        WelsFillCacheNonZeroCount (pNeighAvail, pNonZeroCount, pCurLayer);
        WELS_READ_VERIFY (ParseIntra16x16Mode (pCtx, pNeighAvail, pBsAux, pCurLayer));
      }
    }
  }

  ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
  ST32 (&pCurLayer->pNzc[iMbXy][20], 0);

  if (MB_TYPE_INTRA16x16 != pCurLayer->pMbType[iMbXy]) {
    WELS_READ_VERIFY (ParseCbpInfoCabac (pCtx, pNeighAvail, uiCbp));

    pCurLayer->pCbp[iMbXy] = uiCbp;
    pSlice->iLastDeltaQp = uiCbp == 0 ? 0 : pSlice->iLastDeltaQp;
    uiCbpChroma = pCtx->pSps->uiChromaFormatIdc ? pCurLayer->pCbp[iMbXy] >> 4 : 0;
    uiCbpLuma = pCurLayer->pCbp[iMbXy] & 15;
  }

  if (pCurLayer->pCbp[iMbXy] || MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {

    if (MB_TYPE_INTRA16x16 != pCurLayer->pMbType[iMbXy]) {
      // Need modification when B picutre add in
      bool bNeedParseTransformSize8x8Flag =
        (((IS_INTER_16x16 (pCurLayer->pMbType[iMbXy]) || IS_DIRECT (pCurLayer->pMbType[iMbXy])
           || IS_INTER_16x8 (pCurLayer->pMbType[iMbXy]) || IS_INTER_8x16 (pCurLayer->pMbType[iMbXy]))
          || pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy])
         && (pCurLayer->pMbType[iMbXy] != MB_TYPE_INTRA8x8)
         && (pCurLayer->pMbType[iMbXy] != MB_TYPE_INTRA4x4)
         && ((pCurLayer->pCbp[iMbXy] & 0x0F) > 0)
         && (pCtx->pPps->bTransform8x8ModeFlag));

      if (bNeedParseTransformSize8x8Flag) {
        WELS_READ_VERIFY (ParseTransformSize8x8FlagCabac (pCtx, pNeighAvail,
                          pCtx->pCurDqLayer->pTransformSize8x8Flag[iMbXy])); //transform_size_8x8_flag
      }
    }

    memset (pCurLayer->pScaledTCoeff[iMbXy], 0, 384 * sizeof (pCurLayer->pScaledTCoeff[iMbXy][0]));

    int32_t iQpDelta, iId8x8, iId4x4;

    WELS_READ_VERIFY (ParseDeltaQpCabac (pCtx, iQpDelta));
    if (iQpDelta > 25 || iQpDelta < -26) { //out of iQpDelta range
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_QP);
    }
    pCurLayer->pLumaQp[iMbXy] = (pSlice->iLastMbQp + iQpDelta + 52) % 52; //update last_mb_qp
    pSlice->iLastMbQp = pCurLayer->pLumaQp[iMbXy];
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pSlice->iLastMbQp +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i], 0, 51)];
    }

    if (MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
      //step1: Luma DC
      WELS_READ_VERIFY (ParseResidualBlockCabac (pNeighAvail, pNonZeroCount, pBsAux, 0, 16, g_kuiLumaDcZigzagScan,
                        I16_LUMA_DC, pCurLayer->pScaledTCoeff[iMbXy], pCurLayer->pLumaQp[iMbXy], pCtx));
      //step2: Luma AC
      if (uiCbpLuma) {
        for (i = 0; i < 16; i++) {
          WELS_READ_VERIFY (ParseResidualBlockCabac (pNeighAvail, pNonZeroCount, pBsAux, i, iScanIdxEnd - WELS_MAX (iScanIdxStart,
                            1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1), I16_LUMA_AC, pCurLayer->pScaledTCoeff[iMbXy] + (i << 4),
                            pCurLayer->pLumaQp[iMbXy], pCtx));
        }
        ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      } else {
        ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
        ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
      }
    } else { //non-MB_TYPE_INTRA16x16
      if (pCtx->pCurDqLayer->pTransformSize8x8Flag[iMbXy]) {
        // Transform 8x8 support for CABAC
        for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
          if (uiCbpLuma & (1 << iId8x8)) {
            WELS_READ_VERIFY (ParseResidualBlockCabac8x8 (pNeighAvail, pNonZeroCount, pBsAux, (iId8x8 << 2),
                              iScanIdxEnd - iScanIdxStart + 1, g_kuiZigzagScan8x8 + iScanIdxStart,
                              IS_INTRA (pCurLayer->pMbType[iMbXy]) ? LUMA_DC_AC_INTRA_8 : LUMA_DC_AC_INTER_8,
                              pCurLayer->pScaledTCoeff[iMbXy] + (iId8x8 << 6), pCurLayer->pLumaQp[iMbXy], pCtx));
          } else {
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[ (iId8x8 << 2)]], 0);
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[ (iId8x8 << 2) + 2]], 0);
          }
        }
        ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      } else {
        iMbResProperty = (IS_INTRA (pCurLayer->pMbType[iMbXy])) ? LUMA_DC_AC_INTRA : LUMA_DC_AC_INTER;
        for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
          if (uiCbpLuma & (1 << iId8x8)) {
            int32_t iIdx = (iId8x8 << 2);
            for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
              //Luma (DC and AC decoding together)
              WELS_READ_VERIFY (ParseResidualBlockCabac (pNeighAvail, pNonZeroCount, pBsAux, iIdx, iScanIdxEnd - iScanIdxStart + 1,
                                g_kuiZigzagScan + iScanIdxStart, iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + (iIdx << 4),
                                pCurLayer->pLumaQp[iMbXy],
                                pCtx));
              iIdx++;
            }
          } else {
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[iId8x8 << 2]], 0);
            ST16 (&pNonZeroCount[g_kCacheNzcScanIdx[ (iId8x8 << 2) + 2]], 0);
          }
        }
        ST32 (&pCurLayer->pNzc[iMbXy][0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32 (&pCurLayer->pNzc[iMbXy][4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32 (&pCurLayer->pNzc[iMbXy][8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32 (&pCurLayer->pNzc[iMbXy][12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      }
    }

    //chroma
    //step1: DC
    if (1 == uiCbpChroma || 2 == uiCbpChroma) {
      for (i = 0; i < 2; i++) {
        if (IS_INTRA (pCurLayer->pMbType[iMbXy]))
          iMbResProperty = i ? CHROMA_DC_V : CHROMA_DC_U;
        else
          iMbResProperty = i ? CHROMA_DC_V_INTER : CHROMA_DC_U_INTER;

        WELS_READ_VERIFY (ParseResidualBlockCabac (pNeighAvail, pNonZeroCount, pBsAux, 16 + (i << 2), 4, g_kuiChromaDcScan,
                          iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + 256 + (i << 6), pCurLayer->pChromaQp[iMbXy][i], pCtx));
      }
    }
    //step2: AC
    if (2 == uiCbpChroma) {
      for (i = 0; i < 2; i++) {
        if (IS_INTRA (pCurLayer->pMbType[iMbXy]))
          iMbResProperty = i ? CHROMA_AC_V : CHROMA_AC_U;
        else
          iMbResProperty = i ? CHROMA_AC_V_INTER : CHROMA_AC_U_INTER;
        int32_t index = 16 + (i << 2);
        for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
          WELS_READ_VERIFY (ParseResidualBlockCabac (pNeighAvail, pNonZeroCount, pBsAux, index,
                            iScanIdxEnd - WELS_MAX (iScanIdxStart, 1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1),
                            iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + (index << 4), pCurLayer->pChromaQp[iMbXy][i], pCtx));
          index++;
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
  } else {
    pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp;
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pCurLayer->pLumaQp[iMbXy] +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i], 0, 51)];
    }
  }

  WELS_READ_VERIFY (ParseEndOfSliceCabac (pCtx, uiEosFlag));
  if (uiEosFlag) {
    RestoreCabacDecEngineToBS (pCtx->pCabacDecEngine, pCtx->pCurDqLayer->pBitStringAux);
  }

  return ERR_NONE;
}


int32_t WelsDecodeMbCabacPSlice (PWelsDecoderContext pCtx, PNalUnit pNalCur, uint32_t& uiEosFlag) {
  PDqLayer pCurLayer             = pCtx->pCurDqLayer;
  PSlice pSlice                  = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader      = &pSlice->sSliceHeaderExt.sSliceHeader;
  PPicture* ppRefPic = pCtx->sRefPic.pRefList[LIST_0];
  uint32_t uiCode;
  int32_t iMbXy = pCurLayer->iMbXyIndex;
  int32_t i;
  SWelsNeighAvail uiNeighAvail;
  pCurLayer->pCbp[iMbXy] = 0;
  pCurLayer->pCbfDc[iMbXy] = 0;
  pCurLayer->pChromaPredMode[iMbXy] = C_PRED_DC;

  pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] = true;
  pCurLayer->pTransformSize8x8Flag[iMbXy] = false;

  GetNeighborAvailMbType (&uiNeighAvail, pCurLayer);
  WELS_READ_VERIFY (ParseSkipFlagCabac (pCtx, &uiNeighAvail, uiCode));

  if (uiCode) {
    int16_t pMv[2] = {0};
    pCurLayer->pMbType[iMbXy] = MB_TYPE_SKIP;
    ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][20], 0);

    pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
    memset (pCurLayer->pRefIndex[0][iMbXy], 0, sizeof (int8_t) * 16);
    pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || ! (ppRefPic[0] && ppRefPic[0]->bIsComplete);
    //predict mv
    PredPSkipMvFromNeighbor (pCurLayer, pMv);
    for (i = 0; i < 16; i++) {
      ST32 (pCurLayer->pMv[0][iMbXy][i], * (uint32_t*)pMv);
      ST32 (pCurLayer->pMvd[0][iMbXy][i], 0);
    }

    //if (!pSlice->sSliceHeaderExt.bDefaultResidualPredFlag) {
    //  memset (pCurLayer->pScaledTCoeff[iMbXy], 0, 384 * sizeof (int16_t));
    //}

    //reset rS
    pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp; //??????????????? dqaunt of previous mb
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pCurLayer->pLumaQp[iMbXy] +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i], 0, 51)];
    }

    //for neighboring CABAC usage
    pSlice->iLastDeltaQp = 0;

    WELS_READ_VERIFY (ParseEndOfSliceCabac (pCtx, uiEosFlag));

    return ERR_NONE;
  }

  WELS_READ_VERIFY (WelsDecodeMbCabacPSliceBaseMode0 (pCtx, &uiNeighAvail, uiEosFlag));
  return ERR_NONE;
}


int32_t WelsDecodeMbCabacBSlice (PWelsDecoderContext pCtx, PNalUnit pNalCur, uint32_t& uiEosFlag) {
  PDqLayer pCurLayer = pCtx->pCurDqLayer;
  PSlice pSlice = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader = &pSlice->sSliceHeaderExt.sSliceHeader;
  PPicture* ppRefPicL0 = pCtx->sRefPic.pRefList[LIST_0];
  PPicture* ppRefPicL1 = pCtx->sRefPic.pRefList[LIST_1];
  uint32_t uiCode;
  int32_t iMbXy = pCurLayer->iMbXyIndex;
  int32_t i;
  SWelsNeighAvail uiNeighAvail;
  pCurLayer->pCbp[iMbXy] = 0;
  pCurLayer->pCbfDc[iMbXy] = 0;
  pCurLayer->pChromaPredMode[iMbXy] = C_PRED_DC;

  pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] = true;
  pCurLayer->pTransformSize8x8Flag[iMbXy] = false;

  GetNeighborAvailMbType (&uiNeighAvail, pCurLayer);
  WELS_READ_VERIFY (ParseSkipFlagCabac (pCtx, &uiNeighAvail, uiCode));

  memset (pCurLayer->pDirect[iMbXy], 0, sizeof (int8_t) * 16);

  if (uiCode) {
    int16_t pMv[LIST_A][2] = { {0, 0}, { 0, 0 } };
    int8_t  ref[LIST_A] = { 0 };
    pCurLayer->pMbType[iMbXy] = MB_TYPE_SKIP | MB_TYPE_DIRECT;
    ST32 (&pCurLayer->pNzc[iMbXy][0], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][4], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][8], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][12], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][16], 0);
    ST32 (&pCurLayer->pNzc[iMbXy][20], 0);

    pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
    memset (pCurLayer->pRefIndex[LIST_0][iMbXy], 0, sizeof (int8_t) * 16);
    memset (pCurLayer->pRefIndex[LIST_1][iMbXy], 0, sizeof (int8_t) * 16);
    pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || ! (ppRefPicL0[0] && ppRefPicL0[0]->bIsComplete)
                            || ! (ppRefPicL1[0] && ppRefPicL1[0]->bIsComplete);


    if (pSliceHeader->iDirectSpatialMvPredFlag) {

      //predict direct spatial mv
      SubMbType subMbType;
      int32_t ret = PredMvBDirectSpatial (pCtx, pMv, ref, subMbType);
      if (ret != ERR_NONE) {
        return ret;
      }
    } else {
      //temporal direct mode
      ComputeColocated (pCtx);
      int32_t ret = PredBDirectTemporal (pCtx, pMv, ref);
      if (ret != ERR_NONE) {
        return ret;
      }
    }


    //reset rS
    pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp; //??????????????? dqaunt of previous mb
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pCurLayer->pLumaQp[iMbXy] +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i], 0, 51)];
    }

    //for neighboring CABAC usage
    pSlice->iLastDeltaQp = 0;

    WELS_READ_VERIFY (ParseEndOfSliceCabac (pCtx, uiEosFlag));

    return ERR_NONE;
  }

  WELS_READ_VERIFY (WelsDecodeMbCabacBSliceBaseMode0 (pCtx, &uiNeighAvail, uiEosFlag));
  return ERR_NONE;
}

// Calculate deqaunt coeff scaling list value
int32_t  WelsCalcDeqCoeffScalingList (PWelsDecoderContext pCtx) {
  if (pCtx->pSps->bSeqScalingMatrixPresentFlag || pCtx->pPps->bPicScalingMatrixPresentFlag) {
    pCtx->bUseScalingList = true;

    if (!pCtx->bDequantCoeff4x4Init || (pCtx->iDequantCoeffPpsid != pCtx->pPps->iPpsId)) {
      int i, q, x, y;
      //Init dequant coeff value for different QP
      for (i = 0; i < 6; i++) {
        pCtx->pDequant_coeff4x4[i] = pCtx->pDequant_coeff_buffer4x4[i];
        pCtx->pDequant_coeff8x8[i] = pCtx->pDequant_coeff_buffer8x8[i];
        for (q = 0; q < 51; q++) {
          for (x = 0; x < 16; x++) {
            pCtx->pDequant_coeff4x4[i][q][x] = pCtx->pPps->bPicScalingMatrixPresentFlag ? pCtx->pPps->iScalingList4x4[i][x] *
                                               g_kuiDequantCoeff[q][x & 0x07] : pCtx->pSps->iScalingList4x4[i][x] * g_kuiDequantCoeff[q][x & 0x07];
          }
          for (y = 0; y < 64; y++) {
            pCtx->pDequant_coeff8x8[i][q][y] = pCtx->pPps->bPicScalingMatrixPresentFlag ? pCtx->pPps->iScalingList8x8[i][y] *
                                               g_kuiMatrixV[q % 6][y / 8][y % 8] : pCtx->pSps->iScalingList8x8[i][y] * g_kuiMatrixV[q % 6][y / 8][y % 8];
          }
        }
      }
      pCtx->bDequantCoeff4x4Init = true;
      pCtx->iDequantCoeffPpsid = pCtx->pPps->iPpsId;
    }
  } else
    pCtx->bUseScalingList = false;
  return ERR_NONE;
}

int32_t WelsDecodeSlice (PWelsDecoderContext pCtx, bool bFirstSliceInLayer, PNalUnit pNalCur) {
  PDqLayer pCurLayer = pCtx->pCurDqLayer;
  PFmo pFmo = pCtx->pFmo;
  int32_t iRet;
  int32_t iNextMbXyIndex, iSliceIdc;

  PSlice pSlice = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeaderExt pSliceHeaderExt = &pSlice->sSliceHeaderExt;
  PSliceHeader pSliceHeader = &pSliceHeaderExt->sSliceHeader;
  int32_t iMbX, iMbY;
  const int32_t kiCountNumMb = pSliceHeader->pSps->uiTotalMbCount; //need to be correct when fmo or multi slice
  uint32_t uiEosFlag = 0;
  PWelsDecMbFunc pDecMbFunc;

  pSlice->iTotalMbInCurSlice = 0; //initialize at the starting of slice decoding.

  if (pCtx->pPps->bEntropyCodingModeFlag) {
    if (pSlice->sSliceHeaderExt.bAdaptiveMotionPredFlag ||
        pSlice->sSliceHeaderExt.bAdaptiveBaseModeFlag ||
        pSlice->sSliceHeaderExt.bAdaptiveResidualPredFlag) {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_ERROR,
               "WelsDecodeSlice()::::ILP flag exist, not supported with CABAC enabled!");
      pCtx->iErrorCode |= dsBitstreamError;
      return dsBitstreamError;
    }
    if (P_SLICE == pSliceHeader->eSliceType)
      pDecMbFunc = WelsDecodeMbCabacPSlice;
    else if (B_SLICE == pSliceHeader->eSliceType)
      pDecMbFunc = WelsDecodeMbCabacBSlice;
    else //I_SLICE. B_SLICE is being supported
      pDecMbFunc = WelsDecodeMbCabacISlice;
  } else {
    if (P_SLICE == pSliceHeader->eSliceType) {
      pDecMbFunc = WelsDecodeMbCavlcPSlice;
    } else { //I_SLICE
      pDecMbFunc = WelsDecodeMbCavlcISlice;
    }
  }

  if (pSliceHeader->pPps->bConstainedIntraPredFlag) {
    pCtx->pFillInfoCacheIntraNxNFunc = WelsFillCacheConstrain1IntraNxN;
    pCtx->pMapNxNNeighToSampleFunc    = WelsMapNxNNeighToSampleConstrain1;
    pCtx->pMap16x16NeighToSampleFunc  = WelsMap16x16NeighToSampleConstrain1;
  } else {
    pCtx->pFillInfoCacheIntraNxNFunc = WelsFillCacheConstrain0IntraNxN;
    pCtx->pMapNxNNeighToSampleFunc    = WelsMapNxNNeighToSampleNormal;
    pCtx->pMap16x16NeighToSampleFunc  = WelsMap16x16NeighToSampleNormal;
  }

  pCtx->eSliceType = pSliceHeader->eSliceType;
  if (pCurLayer->sLayerInfo.pPps->bEntropyCodingModeFlag == 1) {
    int32_t iQp = pSlice->sSliceHeaderExt.sSliceHeader.iSliceQp;
    int32_t iCabacInitIdc = pSlice->sSliceHeaderExt.sSliceHeader.iCabacInitIdc;
    WelsCabacContextInit (pCtx, pSlice->eSliceType, iCabacInitIdc, iQp);
    //InitCabacCtx (pCtx->pCabacCtx, pSlice->eSliceType, iCabacInitIdc, iQp);
    pSlice->iLastDeltaQp = 0;
    WELS_READ_VERIFY (InitCabacDecEngineFromBS (pCtx->pCabacDecEngine, pCtx->pCurDqLayer->pBitStringAux));
  }
  //try to calculate  the dequant_coeff
  WelsCalcDeqCoeffScalingList (pCtx);

  iNextMbXyIndex = pSliceHeader->iFirstMbInSlice;
  iMbX = iNextMbXyIndex % pCurLayer->iMbWidth;
  iMbY = iNextMbXyIndex / pCurLayer->iMbWidth; // error is introduced by multiple slices case, 11/23/2009
  pSlice->iMbSkipRun = -1;
  iSliceIdc = (pSliceHeader->iFirstMbInSlice << 7) + pCurLayer->uiLayerDqId;

  pCurLayer->iMbX =  iMbX;
  pCurLayer->iMbY = iMbY;
  pCurLayer->iMbXyIndex = iNextMbXyIndex;

  do {
    if ((-1 == iNextMbXyIndex) || (iNextMbXyIndex >= kiCountNumMb)) { // slice group boundary or end of a frame
      break;
    }

    pCurLayer->pSliceIdc[iNextMbXyIndex] = iSliceIdc;
    pCtx->bMbRefConcealed = false;
    iRet = pDecMbFunc (pCtx,  pNalCur, uiEosFlag);
    pCurLayer->pMbRefConcealedFlag[iNextMbXyIndex] = pCtx->bMbRefConcealed;
    if (iRet != ERR_NONE) {
      return iRet;
    }

    ++pSlice->iTotalMbInCurSlice;
    if (uiEosFlag) { //end of slice
      break;
    }
    if (pSliceHeader->pPps->uiNumSliceGroups > 1) {
      iNextMbXyIndex = FmoNextMb (pFmo, iNextMbXyIndex);
    } else {
      ++iNextMbXyIndex;
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
  PDqLayer pCurLayer             = pCtx->pCurDqLayer;
  PBitStringAux pBs              = pCurLayer->pBitStringAux;
  PSlice pSlice                  = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader      = &pSlice->sSliceHeaderExt.sSliceHeader;

  SWelsNeighAvail sNeighAvail;
  int32_t iMbResProperty;

  int32_t iScanIdxStart = pSlice->sSliceHeaderExt.uiScanIdxStart;
  int32_t iScanIdxEnd   = pSlice->sSliceHeaderExt.uiScanIdxEnd;

  int32_t iMbX = pCurLayer->iMbX;
  int32_t iMbY = pCurLayer->iMbY;
  const int32_t iMbXy = pCurLayer->iMbXyIndex;
  int8_t* pNzc = pCurLayer->pNzc[iMbXy];
  int32_t i;
  int32_t iRet = ERR_NONE;
  uint32_t uiMbType = 0, uiCbp = 0, uiCbpL = 0, uiCbpC = 0;
  uint32_t uiCode;
  int32_t iCode;

  ENFORCE_STACK_ALIGN_1D (uint8_t, pNonZeroCount, 48, 16);
  GetNeighborAvailMbType (&sNeighAvail, pCurLayer);
  pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
  pCurLayer->pResidualPredFlag[iMbXy] = pSlice->sSliceHeaderExt.bDefaultResidualPredFlag;

  pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] = true;
  pCurLayer->pTransformSize8x8Flag[iMbXy] = false;

  WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //uiMbType
  uiMbType = uiCode;
  if (uiMbType > 25)
    return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_TYPE);
  if (!pCtx->pSps->uiChromaFormatIdc && ((uiMbType >= 5 && uiMbType <= 12) || (uiMbType >= 17 && uiMbType <= 24)))
    return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_TYPE);

  if (25 == uiMbType) {
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_DEBUG, "I_PCM mode exists in I slice!");
    int32_t iDecStrideL = pCurLayer->pDec->iLinesize[0];
    int32_t iDecStrideC = pCurLayer->pDec->iLinesize[1];

    int32_t iOffsetL = (iMbX + iMbY * iDecStrideL) << 4;
    int32_t iOffsetC = (iMbX + iMbY * iDecStrideC) << 3;

    uint8_t* pDecY = pCurLayer->pDec->pData[0] + iOffsetL;
    uint8_t* pDecU = pCurLayer->pDec->pData[1] + iOffsetC;
    uint8_t* pDecV = pCurLayer->pDec->pData[2] + iOffsetC;

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
    if (!pCtx->pParam->bParseOnly) {
      for (i = 0; i < 16; i++) { //luma
        memcpy (pDecY, pTmpBsBuf, iCopySizeY);
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
    }

    pBs->pCurBuf += 384;

    //step 3: update QP and pNonZeroCount
    pCurLayer->pLumaQp[iMbXy] = 0;
    memset (pCurLayer->pChromaQp[iMbXy], 0, sizeof (pCurLayer->pChromaQp[iMbXy]));
    memset (pNzc, 16, sizeof (pCurLayer->pNzc[iMbXy]));   //Rec. 9.2.1 for PCM, nzc=16
    WELS_READ_VERIFY (InitReadBits (pBs, 0));
    return ERR_NONE;
  } else if (0 == uiMbType) { //reference to JM
    ENFORCE_STACK_ALIGN_1D (int8_t, pIntraPredMode, 48, 16);
    pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA4x4;
    if (pCtx->pPps->bTransform8x8ModeFlag) {
      WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //transform_size_8x8_flag
      pCurLayer->pTransformSize8x8Flag[iMbXy] = !!uiCode;
      if (pCurLayer->pTransformSize8x8Flag[iMbXy]) {
        uiMbType = pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA8x8;
      }
    }
    if (!pCurLayer->pTransformSize8x8Flag[iMbXy]) {
      pCtx->pFillInfoCacheIntraNxNFunc (&sNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
      WELS_READ_VERIFY (ParseIntra4x4Mode (pCtx, &sNeighAvail, pIntraPredMode, pBs, pCurLayer));
    } else {
      pCtx->pFillInfoCacheIntraNxNFunc (&sNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
      WELS_READ_VERIFY (ParseIntra8x8Mode (pCtx, &sNeighAvail, pIntraPredMode, pBs, pCurLayer));
    }

    //uiCbp
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //coded_block_pattern
    uiCbp = uiCode;
    //G.9.1 Alternative parsing process for coded pBlock pattern
    if (pCtx->pSps->uiChromaFormatIdc && (uiCbp > 47))
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_CBP);
    if (!pCtx->pSps->uiChromaFormatIdc && (uiCbp > 15))
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_CBP);

    if (pCtx->pSps->uiChromaFormatIdc)
      uiCbp = g_kuiIntra4x4CbpTable[uiCbp];
    else
      uiCbp = g_kuiIntra4x4CbpTable400[uiCbp];
    pCurLayer->pCbp[iMbXy] = uiCbp;
    uiCbpC = uiCbp >> 4;
    uiCbpL = uiCbp & 15;
  } else { //I_PCM exclude, we can ignore it
    pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA16x16;
    pCurLayer->pTransformSize8x8Flag[iMbXy] = false;
    pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] = true;
    pCurLayer->pIntraPredMode[iMbXy][7] = (uiMbType - 1) & 3;
    pCurLayer->pCbp[iMbXy] = g_kuiI16CbpTable[ (uiMbType - 1) >> 2];
    uiCbpC = pCtx->pSps->uiChromaFormatIdc ? pCurLayer->pCbp[iMbXy] >> 4 : 0;
    uiCbpL = pCurLayer->pCbp[iMbXy] & 15;
    WelsFillCacheNonZeroCount (&sNeighAvail, pNonZeroCount, pCurLayer);
    WELS_READ_VERIFY (ParseIntra16x16Mode (pCtx, &sNeighAvail, pBs, pCurLayer));
  }

  ST32A4 (&pNzc[0], 0);
  ST32A4 (&pNzc[4], 0);
  ST32A4 (&pNzc[8], 0);
  ST32A4 (&pNzc[12], 0);
  ST32A4 (&pNzc[16], 0);
  ST32A4 (&pNzc[20], 0);

  if (pCurLayer->pCbp[iMbXy] == 0 && IS_INTRANxN (pCurLayer->pMbType[iMbXy])) {
    pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp;
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pCurLayer->pLumaQp[iMbXy] +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i], 0, 51)];
    }

  }

  if (pCurLayer->pCbp[iMbXy] || MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
    memset (pCurLayer->pScaledTCoeff[iMbXy], 0, 384 * sizeof (pCurLayer->pScaledTCoeff[iMbXy][0]));
    int32_t iQpDelta, iId8x8, iId4x4;

    WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //mb_qp_delta
    iQpDelta = iCode;

    if (iQpDelta > 25 || iQpDelta < -26) { //out of iQpDelta range
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_QP);
    }

    pCurLayer->pLumaQp[iMbXy] = (pSlice->iLastMbQp + iQpDelta + 52) % 52; //update last_mb_qp
    pSlice->iLastMbQp = pCurLayer->pLumaQp[iMbXy];
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pSlice->iLastMbQp +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i], 0,
                                       51)];
    }


    BsStartCavlc (pBs);

    if (MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
      //step1: Luma DC
      if ((iRet = WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, 0, 16, g_kuiLumaDcZigzagScan, I16_LUMA_DC,
                                          pCurLayer->pScaledTCoeff[iMbXy], pCurLayer->pLumaQp[iMbXy], pCtx)) != ERR_NONE) {
        return iRet;//abnormal
      }
      //step2: Luma AC
      if (uiCbpL) {
        for (i = 0; i < 16; i++) {
          if ((iRet = WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, i, iScanIdxEnd - WELS_MAX (iScanIdxStart, 1) + 1,
                                              g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1), I16_LUMA_AC, pCurLayer->pScaledTCoeff[iMbXy] + (i << 4),
                                              pCurLayer->pLumaQp[iMbXy], pCtx)) != ERR_NONE) {
            return iRet;//abnormal
          }
        }
        ST32A4 (&pNzc[0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32A4 (&pNzc[4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32A4 (&pNzc[8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32A4 (&pNzc[12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      }
    } else { //non-MB_TYPE_INTRA16x16
      if (pCurLayer->pTransformSize8x8Flag[iMbXy]) {
        for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
          iMbResProperty = (IS_INTRA (pCurLayer->pMbType[iMbXy])) ? LUMA_DC_AC_INTRA_8 : LUMA_DC_AC_INTER_8;
          if (uiCbpL & (1 << iId8x8)) {
            int32_t iIndex = (iId8x8 << 2);
            for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
              if ((iRet = WelsResidualBlockCavlc8x8 (pVlcTable, pNonZeroCount, pBs, iIndex, iScanIdxEnd - iScanIdxStart + 1,
                                                     g_kuiZigzagScan8x8 + iScanIdxStart, iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + (iId8x8 << 6), iId4x4,
                                                     pCurLayer->pLumaQp[iMbXy], pCtx)) != ERR_NONE) {
                return iRet;
              }
              iIndex++;
            }
          } else {
            ST16 (&pNonZeroCount[g_kuiCache48CountScan4Idx[iId8x8 << 2]], 0);
            ST16 (&pNonZeroCount[g_kuiCache48CountScan4Idx[ (iId8x8 << 2) + 2]], 0);
          }
        }
        ST32A4 (&pNzc[0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32A4 (&pNzc[4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32A4 (&pNzc[8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32A4 (&pNzc[12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      } else {
        for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
          if (uiCbpL & (1 << iId8x8)) {
            int32_t iIndex = (iId8x8 << 2);
            for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
              //Luma (DC and AC decoding together)
              if ((iRet = WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, iIndex, iScanIdxEnd - iScanIdxStart + 1,
                                                  g_kuiZigzagScan + iScanIdxStart, LUMA_DC_AC_INTRA, pCurLayer->pScaledTCoeff[iMbXy] + (iIndex << 4),
                                                  pCurLayer->pLumaQp[iMbXy], pCtx)) != ERR_NONE) {
                return iRet;//abnormal
              }
              iIndex++;
            }
          } else {
            ST16 (&pNonZeroCount[g_kuiCache48CountScan4Idx[ (iId8x8 << 2)]], 0);
            ST16 (&pNonZeroCount[g_kuiCache48CountScan4Idx[ (iId8x8 << 2) + 2]], 0);
          }
        }
        ST32A4 (&pNzc[0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32A4 (&pNzc[4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32A4 (&pNzc[8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32A4 (&pNzc[12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      }
    }

    //chroma
    //step1: DC
    if (1 == uiCbpC || 2 == uiCbpC) {
      for (i = 0; i < 2; i++) { //Cb Cr
        iMbResProperty = i ? CHROMA_DC_V : CHROMA_DC_U;
        if ((iRet = WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, 16 + (i << 2), 4, g_kuiChromaDcScan, iMbResProperty,
                                            pCurLayer->pScaledTCoeff[iMbXy] + 256 + (i << 6), pCurLayer->pChromaQp[iMbXy][i], pCtx)) != ERR_NONE) {
          return iRet;//abnormal
        }
      }
    }

    //step2: AC
    if (2 == uiCbpC) {
      for (i = 0; i < 2; i++) { //Cb Cr
        iMbResProperty = i ? CHROMA_AC_V : CHROMA_AC_U;
        int32_t iIndex = 16 + (i << 2);
        for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
          if ((iRet = WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, iIndex, iScanIdxEnd - WELS_MAX (iScanIdxStart,
                                              1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1), iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + (iIndex << 4),
                                              pCurLayer->pChromaQp[iMbXy][i], pCtx)) != ERR_NONE) {
            return iRet;//abnormal
          }
          iIndex++;
        }
      }
      ST16A2 (&pNzc[16], LD16A2 (&pNonZeroCount[6 + 8 * 1]));
      ST16A2 (&pNzc[20], LD16A2 (&pNonZeroCount[6 + 8 * 2]));
      ST16A2 (&pNzc[18], LD16A2 (&pNonZeroCount[6 + 8 * 4]));
      ST16A2 (&pNzc[22], LD16A2 (&pNonZeroCount[6 + 8 * 5]));
    }
    BsEndCavlc (pBs);
  }

  return ERR_NONE;
}

int32_t WelsDecodeMbCavlcISlice (PWelsDecoderContext pCtx, PNalUnit pNalCur, uint32_t& uiEosFlag) {
  PDqLayer pCurLayer = pCtx->pCurDqLayer;
  PBitStringAux pBs = pCurLayer->pBitStringAux;
  PSliceHeaderExt pSliceHeaderExt = &pCurLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt;
  int32_t iBaseModeFlag;
  int32_t iRet = 0; //should have the return value to indicate decoding error or not, It's NECESSARY--2010.4.15
  uint32_t uiCode;
  intX_t iUsedBits;
  if (pSliceHeaderExt->bAdaptiveBaseModeFlag == 1) {
    WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //base_mode_flag
    iBaseModeFlag = uiCode;
  } else {
    iBaseModeFlag = pSliceHeaderExt->bDefaultBaseModeFlag;
  }
  if (!iBaseModeFlag) {
    iRet = WelsActualDecodeMbCavlcISlice (pCtx);
  } else {
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "iBaseModeFlag (%d) != 0, inter-layer prediction not supported.",
             iBaseModeFlag);
    return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_ILP);
  }
  if (iRet) { //occur error when parsing, MUST STOP decoding
    return iRet;
  }

  // check whether there is left bits to read next time in case multiple slices
  iUsedBits = ((pBs->pCurBuf - pBs->pStartBuf) << 3) - (16 - pBs->iLeftBits);
  // sub 1, for stop bit
  if ((iUsedBits == (pBs->iBits - 1)) && (0 >= pCurLayer->sLayerInfo.sSliceInLayer.iMbSkipRun)) { // slice boundary
    uiEosFlag = 1;
  }
  if (iUsedBits > (pBs->iBits -
                   1)) { //When BS incomplete, as long as find it, SHOULD stop decoding to avoid mosaic or crash.
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING,
             "WelsDecodeMbCavlcISlice()::::pBs incomplete, iUsedBits:%" PRId64 " > pBs->iBits:%d, MUST stop decoding.",
             (int64_t) iUsedBits, pBs->iBits);
    return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_BS_INCOMPLETE);
  }
  return ERR_NONE;
}

int32_t WelsActualDecodeMbCavlcPSlice (PWelsDecoderContext pCtx) {
  SVlcTable* pVlcTable     = &pCtx->sVlcTable;
  PDqLayer pCurLayer             = pCtx->pCurDqLayer;
  PBitStringAux pBs              = pCurLayer->pBitStringAux;
  PSlice pSlice                  = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader      = &pSlice->sSliceHeaderExt.sSliceHeader;

  int32_t iScanIdxStart = pSlice->sSliceHeaderExt.uiScanIdxStart;
  int32_t iScanIdxEnd   = pSlice->sSliceHeaderExt.uiScanIdxEnd;

  SWelsNeighAvail sNeighAvail;
  int32_t iMbX = pCurLayer->iMbX;
  int32_t iMbY = pCurLayer->iMbY;
  const int32_t iMbXy = pCurLayer->iMbXyIndex;
  int8_t* pNzc = pCurLayer->pNzc[iMbXy];
  int32_t i;
  int32_t iRet = ERR_NONE;
  uint32_t uiMbType = 0, uiCbp = 0, uiCbpL = 0, uiCbpC = 0;
  uint32_t uiCode;
  int32_t iCode;
  int32_t iMbResProperty;

  GetNeighborAvailMbType (&sNeighAvail, pCurLayer);
  ENFORCE_STACK_ALIGN_1D (uint8_t, pNonZeroCount, 48, 16);
  pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;//2009.10.23
  WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //uiMbType
  uiMbType = uiCode;
  if (uiMbType < 5) { //inter MB type
    int16_t iMotionVector[LIST_A][30][MV_A];
    int8_t  iRefIndex[LIST_A][30];
    pCurLayer->pMbType[iMbXy] = g_ksInterPMbTypeInfo[uiMbType].iType;
    WelsFillCacheInter (&sNeighAvail, pNonZeroCount, iMotionVector, iRefIndex, pCurLayer);

    if ((iRet = ParseInterInfo (pCtx, iMotionVector, iRefIndex, pBs)) != ERR_NONE) {
      return iRet;//abnormal
    }

    if (pSlice->sSliceHeaderExt.bAdaptiveResidualPredFlag == 1) {
      WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //residual_prediction_flag
      pCurLayer->pResidualPredFlag[iMbXy] =  uiCode;
    } else {
      pCurLayer->pResidualPredFlag[iMbXy] = pSlice->sSliceHeaderExt.bDefaultResidualPredFlag;
    }

    if (pCurLayer->pResidualPredFlag[iMbXy] == 0) {
      pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
    } else {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "residual_pred_flag = 1 not supported.");
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_UNSUPPORTED_ILP);
    }
  } else { //intra MB type
    uiMbType -= 5;
    if (uiMbType > 25)
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_TYPE);
    if (!pCtx->pSps->uiChromaFormatIdc && ((uiMbType >= 5 && uiMbType <= 12) || (uiMbType >= 17 && uiMbType <= 24)))
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_TYPE);

    if (25 == uiMbType) {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_DEBUG, "I_PCM mode exists in P slice!");
      int32_t iDecStrideL = pCurLayer->pDec->iLinesize[0];
      int32_t iDecStrideC = pCurLayer->pDec->iLinesize[1];

      int32_t iOffsetL = (iMbX + iMbY * iDecStrideL) << 4;
      int32_t iOffsetC = (iMbX + iMbY * iDecStrideC) << 3;

      uint8_t* pDecY = pCurLayer->pDec->pData[0] + iOffsetL;
      uint8_t* pDecU = pCurLayer->pDec->pData[1] + iOffsetC;
      uint8_t* pDecV = pCurLayer->pDec->pData[2] + iOffsetC;

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
      if (!pCtx->pParam->bParseOnly) {
        for (i = 0; i < 16; i++) { //luma
          memcpy (pDecY, pTmpBsBuf, iCopySizeY);
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
      }

      pBs->pCurBuf += 384;

      //step 3: update QP and pNonZeroCount
      pCurLayer->pLumaQp[iMbXy] = 0;
      pCurLayer->pChromaQp[iMbXy][0] = pCurLayer->pChromaQp[iMbXy][1] = 0;
      //Rec. 9.2.1 for PCM, nzc=16
      ST32A4 (&pNzc[0], 0x10101010);
      ST32A4 (&pNzc[4], 0x10101010);
      ST32A4 (&pNzc[8], 0x10101010);
      ST32A4 (&pNzc[12], 0x10101010);
      ST32A4 (&pNzc[16], 0x10101010);
      ST32A4 (&pNzc[20], 0x10101010);
      WELS_READ_VERIFY (InitReadBits (pBs, 0));
      return ERR_NONE;
    } else {
      if (0 == uiMbType) {
        ENFORCE_STACK_ALIGN_1D (int8_t, pIntraPredMode, 48, 16);
        pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA4x4;
        if (pCtx->pPps->bTransform8x8ModeFlag) {
          WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //transform_size_8x8_flag
          pCurLayer->pTransformSize8x8Flag[iMbXy] = !!uiCode;
          if (pCurLayer->pTransformSize8x8Flag[iMbXy]) {
            uiMbType = pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA8x8;
          }
        }
        if (!pCurLayer->pTransformSize8x8Flag[iMbXy]) {
          pCtx->pFillInfoCacheIntraNxNFunc (&sNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
          WELS_READ_VERIFY (ParseIntra4x4Mode (pCtx, &sNeighAvail, pIntraPredMode, pBs, pCurLayer));
        } else {
          pCtx->pFillInfoCacheIntraNxNFunc (&sNeighAvail, pNonZeroCount, pIntraPredMode, pCurLayer);
          WELS_READ_VERIFY (ParseIntra8x8Mode (pCtx, &sNeighAvail, pIntraPredMode, pBs, pCurLayer));
        }
      } else { //I_PCM exclude, we can ignore it
        pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA16x16;
        pCurLayer->pTransformSize8x8Flag[iMbXy] = false;
        pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] = true;
        pCurLayer->pIntraPredMode[iMbXy][7] = (uiMbType - 1) & 3;
        pCurLayer->pCbp[iMbXy] = g_kuiI16CbpTable[ (uiMbType - 1) >> 2];
        uiCbpC = pCtx->pSps->uiChromaFormatIdc ? pCurLayer->pCbp[iMbXy] >> 4 : 0;
        uiCbpL = pCurLayer->pCbp[iMbXy] & 15;
        WelsFillCacheNonZeroCount (&sNeighAvail, pNonZeroCount, pCurLayer);
        if ((iRet = ParseIntra16x16Mode (pCtx, &sNeighAvail, pBs, pCurLayer)) != ERR_NONE) {
          return iRet;
        }
      }
    }
  }

  if (MB_TYPE_INTRA16x16 != pCurLayer->pMbType[iMbXy]) {
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //coded_block_pattern
    uiCbp = uiCode;
    {
      if (pCtx->pSps->uiChromaFormatIdc && (uiCbp > 47))
        return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_CBP);
      if (!pCtx->pSps->uiChromaFormatIdc && (uiCbp > 15))
        return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_CBP);
      if (MB_TYPE_INTRA4x4 == pCurLayer->pMbType[iMbXy] || MB_TYPE_INTRA8x8 == pCurLayer->pMbType[iMbXy]) {

        uiCbp = pCtx->pSps->uiChromaFormatIdc ? g_kuiIntra4x4CbpTable[uiCbp] : g_kuiIntra4x4CbpTable400[uiCbp];
      } else //inter
        uiCbp = pCtx->pSps->uiChromaFormatIdc ?  g_kuiInterCbpTable[uiCbp] : g_kuiInterCbpTable400[uiCbp];
    }

    pCurLayer->pCbp[iMbXy] = uiCbp;
    uiCbpC = pCurLayer->pCbp[iMbXy] >> 4;
    uiCbpL = pCurLayer->pCbp[iMbXy] & 15;

    // Need modification when B picutre add in
    bool bNeedParseTransformSize8x8Flag =
      (((pCurLayer->pMbType[iMbXy] >= MB_TYPE_16x16 && pCurLayer->pMbType[iMbXy] <= MB_TYPE_8x16)
        || pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy])
       && (pCurLayer->pMbType[iMbXy] != MB_TYPE_INTRA8x8)
       && (pCurLayer->pMbType[iMbXy] != MB_TYPE_INTRA4x4)
       && (uiCbpL > 0)
       && (pCtx->pPps->bTransform8x8ModeFlag));

    if (bNeedParseTransformSize8x8Flag) {
      WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //transform_size_8x8_flag
      pCurLayer->pTransformSize8x8Flag[iMbXy] = !!uiCode;
    }
  }

  ST32A4 (&pNzc[0], 0);
  ST32A4 (&pNzc[4], 0);
  ST32A4 (&pNzc[8], 0);
  ST32A4 (&pNzc[12], 0);
  ST32A4 (&pNzc[16], 0);
  ST32A4 (&pNzc[20], 0);
  if (pCurLayer->pCbp[iMbXy] == 0 && !IS_INTRA16x16 (pCurLayer->pMbType[iMbXy]) && !IS_I_BL (pCurLayer->pMbType[iMbXy])) {
    pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp;
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pCurLayer->pLumaQp[iMbXy] +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i], 0, 51)];
    }
  }

  if (pCurLayer->pCbp[iMbXy] || MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
    int32_t iQpDelta, iId8x8, iId4x4;
    memset (pCurLayer->pScaledTCoeff[iMbXy], 0, MB_COEFF_LIST_SIZE * sizeof (int16_t));
    WELS_READ_VERIFY (BsGetSe (pBs, &iCode)); //mb_qp_delta
    iQpDelta = iCode;

    if (iQpDelta > 25 || iQpDelta < -26) { //out of iQpDelta range
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_QP);
    }

    pCurLayer->pLumaQp[iMbXy] = (pSlice->iLastMbQp + iQpDelta + 52) % 52; //update last_mb_qp
    pSlice->iLastMbQp = pCurLayer->pLumaQp[iMbXy];
    for (i = 0; i < 2; i++) {
      pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pSlice->iLastMbQp +
                                       pSliceHeader->pPps->iChromaQpIndexOffset[i], 0,
                                       51)];
    }

    BsStartCavlc (pBs);

    if (MB_TYPE_INTRA16x16 == pCurLayer->pMbType[iMbXy]) {
      //step1: Luma DC
      if ((iRet = WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, 0, 16, g_kuiLumaDcZigzagScan, I16_LUMA_DC,
                                          pCurLayer->pScaledTCoeff[iMbXy], pCurLayer->pLumaQp[iMbXy], pCtx)) != ERR_NONE) {
        return iRet;//abnormal
      }
      //step2: Luma AC
      if (uiCbpL) {
        for (i = 0; i < 16; i++) {
          if ((iRet = WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, i, iScanIdxEnd - WELS_MAX (iScanIdxStart, 1) + 1,
                                              g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1), I16_LUMA_AC, pCurLayer->pScaledTCoeff[iMbXy] + (i << 4),
                                              pCurLayer->pLumaQp[iMbXy], pCtx)) != ERR_NONE) {
            return iRet;//abnormal
          }
        }
        ST32A4 (&pNzc[0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32A4 (&pNzc[4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32A4 (&pNzc[8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32A4 (&pNzc[12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      }
    } else { //non-MB_TYPE_INTRA16x16
      if (pCurLayer->pTransformSize8x8Flag[iMbXy]) {
        for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
          iMbResProperty = (IS_INTRA (pCurLayer->pMbType[iMbXy])) ? LUMA_DC_AC_INTRA_8 : LUMA_DC_AC_INTER_8;
          if (uiCbpL & (1 << iId8x8)) {
            int32_t iIndex = (iId8x8 << 2);
            for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
              if ((iRet = WelsResidualBlockCavlc8x8 (pVlcTable, pNonZeroCount, pBs, iIndex, iScanIdxEnd - iScanIdxStart + 1,
                                                     g_kuiZigzagScan8x8 + iScanIdxStart, iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + (iId8x8 << 6), iId4x4,
                                                     pCurLayer->pLumaQp[iMbXy], pCtx)) != ERR_NONE) {
                return iRet;
              }
              iIndex++;
            }
          } else {
            ST16 (&pNonZeroCount[g_kuiCache48CountScan4Idx[iId8x8 << 2]], 0);
            ST16 (&pNonZeroCount[g_kuiCache48CountScan4Idx[ (iId8x8 << 2) + 2]], 0);
          }
        }
        ST32A4 (&pNzc[0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32A4 (&pNzc[4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32A4 (&pNzc[8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32A4 (&pNzc[12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      } else { // Normal T4x4
        for (iId8x8 = 0; iId8x8 < 4; iId8x8++) {
          iMbResProperty = (IS_INTRA (pCurLayer->pMbType[iMbXy])) ? LUMA_DC_AC_INTRA : LUMA_DC_AC_INTER;
          if (uiCbpL & (1 << iId8x8)) {
            int32_t iIndex = (iId8x8 << 2);
            for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
              //Luma (DC and AC decoding together)
              if ((iRet = WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, iIndex, iScanIdxEnd - iScanIdxStart + 1,
                                                  g_kuiZigzagScan + iScanIdxStart, iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + (iIndex << 4),
                                                  pCurLayer->pLumaQp[iMbXy], pCtx)) != ERR_NONE) {
                return iRet;//abnormal
              }
              iIndex++;
            }
          } else {
            ST16 (&pNonZeroCount[g_kuiCache48CountScan4Idx[iId8x8 << 2]], 0);
            ST16 (&pNonZeroCount[g_kuiCache48CountScan4Idx[ (iId8x8 << 2) + 2]], 0);
          }
        }
        ST32A4 (&pNzc[0], LD32 (&pNonZeroCount[1 + 8 * 1]));
        ST32A4 (&pNzc[4], LD32 (&pNonZeroCount[1 + 8 * 2]));
        ST32A4 (&pNzc[8], LD32 (&pNonZeroCount[1 + 8 * 3]));
        ST32A4 (&pNzc[12], LD32 (&pNonZeroCount[1 + 8 * 4]));
      }
    }


    //chroma
    //step1: DC
    if (1 == uiCbpC || 2 == uiCbpC) {
      for (i = 0; i < 2; i++) { //Cb Cr
        if (IS_INTRA (pCurLayer->pMbType[iMbXy]))
          iMbResProperty = i ? CHROMA_DC_V : CHROMA_DC_U;
        else
          iMbResProperty = i ? CHROMA_DC_V_INTER : CHROMA_DC_U_INTER;

        if ((iRet = WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, 16 + (i << 2), 4, g_kuiChromaDcScan, iMbResProperty,
                                            pCurLayer->pScaledTCoeff[iMbXy] + 256 + (i << 6), pCurLayer->pChromaQp[iMbXy][i], pCtx)) != ERR_NONE) {
          return iRet;//abnormal
        }
      }
    } else {
    }
    //step2: AC
    if (2 == uiCbpC) {
      for (i = 0; i < 2; i++) { //Cb Cr
        if (IS_INTRA (pCurLayer->pMbType[iMbXy]))
          iMbResProperty = i ? CHROMA_AC_V : CHROMA_AC_U;
        else
          iMbResProperty = i ? CHROMA_AC_V_INTER : CHROMA_AC_U_INTER;

        int32_t iIndex = 16 + (i << 2);
        for (iId4x4 = 0; iId4x4 < 4; iId4x4++) {
          if ((iRet = WelsResidualBlockCavlc (pVlcTable, pNonZeroCount, pBs, iIndex, iScanIdxEnd - WELS_MAX (iScanIdxStart,
                                              1) + 1, g_kuiZigzagScan + WELS_MAX (iScanIdxStart, 1), iMbResProperty, pCurLayer->pScaledTCoeff[iMbXy] + (iIndex << 4),
                                              pCurLayer->pChromaQp[iMbXy][i], pCtx)) != ERR_NONE) {
            return iRet;//abnormal
          }
          iIndex++;
        }
      }
      ST16A2 (&pNzc[16], LD16A2 (&pNonZeroCount[6 + 8 * 1]));
      ST16A2 (&pNzc[20], LD16A2 (&pNonZeroCount[6 + 8 * 2]));
      ST16A2 (&pNzc[18], LD16A2 (&pNonZeroCount[6 + 8 * 4]));
      ST16A2 (&pNzc[22], LD16A2 (&pNonZeroCount[6 + 8 * 5]));
    }
    BsEndCavlc (pBs);
  }

  return ERR_NONE;
}

int32_t WelsDecodeMbCavlcPSlice (PWelsDecoderContext pCtx, PNalUnit pNalCur, uint32_t& uiEosFlag) {
  PDqLayer pCurLayer             = pCtx->pCurDqLayer;
  PBitStringAux pBs              = pCurLayer->pBitStringAux;
  PSlice pSlice                  = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader      = &pSlice->sSliceHeaderExt.sSliceHeader;
  PPicture* ppRefPic = pCtx->sRefPic.pRefList[LIST_0];
  intX_t iUsedBits;
  const int32_t iMbXy = pCurLayer->iMbXyIndex;
  int8_t* pNzc = pCurLayer->pNzc[iMbXy];
  int32_t iBaseModeFlag, i;
  int32_t iRet = 0; //should have the return value to indicate decoding error or not, It's NECESSARY--2010.4.15
  uint32_t uiCode;

  pCurLayer->pNoSubMbPartSizeLessThan8x8Flag[iMbXy] = true;
  pCurLayer->pTransformSize8x8Flag[iMbXy] = false;

  if (-1 == pSlice->iMbSkipRun) {
    WELS_READ_VERIFY (BsGetUe (pBs, &uiCode)); //mb_skip_run
    pSlice->iMbSkipRun = uiCode;
    if (-1 == pSlice->iMbSkipRun) {
      return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_INVALID_MB_SKIP_RUN);
    }
  }
  if (pSlice->iMbSkipRun--) {
    int16_t iMv[2];

    pCurLayer->pMbType[iMbXy] = MB_TYPE_SKIP;
    ST32A4 (&pNzc[0], 0);
    ST32A4 (&pNzc[4], 0);
    ST32A4 (&pNzc[8], 0);
    ST32A4 (&pNzc[12], 0);
    ST32A4 (&pNzc[16], 0);
    ST32A4 (&pNzc[20], 0);

    pCurLayer->pInterPredictionDoneFlag[iMbXy] = 0;
    memset (pCurLayer->pRefIndex[0][iMbXy], 0, sizeof (int8_t) * 16);
    pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || ! (ppRefPic[0] && ppRefPic[0]->bIsComplete);
    //predict iMv
    PredPSkipMvFromNeighbor (pCurLayer, iMv);
    for (i = 0; i < 16; i++) {
      ST32A2 (pCurLayer->pMv[0][iMbXy][i], * (uint32_t*)iMv);
    }

    //if (!pSlice->sSliceHeaderExt.bDefaultResidualPredFlag) {
    //  memset (pCurLayer->pScaledTCoeff[iMbXy], 0, 384 * sizeof (int16_t));
    //}

    //reset rS
    if (!pSlice->sSliceHeaderExt.bDefaultResidualPredFlag ||
        (pNalCur->sNalHeaderExt.uiQualityId == 0 && pNalCur->sNalHeaderExt.uiDependencyId == 0)) {
      pCurLayer->pLumaQp[iMbXy] = pSlice->iLastMbQp;
      for (i = 0; i < 2; i++) {
        pCurLayer->pChromaQp[iMbXy][i] = g_kuiChromaQpTable[WELS_CLIP3 (pCurLayer->pLumaQp[iMbXy] +
                                         pSliceHeader->pPps->iChromaQpIndexOffset[i], 0, 51)];
      }
    }

    pCurLayer->pCbp[iMbXy] = 0;
  } else {
    if (pSlice->sSliceHeaderExt.bAdaptiveBaseModeFlag == 1) {
      WELS_READ_VERIFY (BsGetOneBit (pBs, &uiCode)); //base_mode_flag
      iBaseModeFlag = uiCode;
    } else {
      iBaseModeFlag = pSlice->sSliceHeaderExt.bDefaultBaseModeFlag;
    }
    if (!iBaseModeFlag) {
      iRet = WelsActualDecodeMbCavlcPSlice (pCtx);
    } else {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "iBaseModeFlag (%d) != 0, inter-layer prediction not supported.",
               iBaseModeFlag);
      return GENERATE_ERROR_NO (ERR_LEVEL_SLICE_HEADER, ERR_INFO_UNSUPPORTED_ILP);
    }
    if (iRet) { //occur error when parsing, MUST STOP decoding
      return iRet;
    }
  }
  // check whether there is left bits to read next time in case multiple slices
  iUsedBits = ((pBs->pCurBuf - pBs->pStartBuf) << 3) - (16 - pBs->iLeftBits);
  // sub 1, for stop bit
  if ((iUsedBits == (pBs->iBits - 1)) && (0 >= pCurLayer->sLayerInfo.sSliceInLayer.iMbSkipRun)) { // slice boundary
    uiEosFlag = 1;
  }
  if (iUsedBits > (pBs->iBits -
                   1)) { //When BS incomplete, as long as find it, SHOULD stop decoding to avoid mosaic or crash.
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING,
             "WelsDecodeMbCavlcISlice()::::pBs incomplete, iUsedBits:%" PRId64 " > pBs->iBits:%d, MUST stop decoding.",
             (int64_t) iUsedBits, pBs->iBits);
    return GENERATE_ERROR_NO (ERR_LEVEL_MB_DATA, ERR_INFO_BS_INCOMPLETE);
  }
  return ERR_NONE;
}

void WelsBlockFuncInit (SBlockFunc*   pFunc,  int32_t iCpu) {
  pFunc->pWelsSetNonZeroCountFunc   = WelsNonZeroCount_c;
  pFunc->pWelsBlockZero16x16Func    = WelsBlockZero16x16_c;
  pFunc->pWelsBlockZero8x8Func      = WelsBlockZero8x8_c;

#ifdef HAVE_NEON
  if (iCpu & WELS_CPU_NEON) {
    pFunc->pWelsSetNonZeroCountFunc = WelsNonZeroCount_neon;
    pFunc->pWelsBlockZero16x16Func  = WelsBlockZero16x16_neon;
    pFunc->pWelsBlockZero8x8Func    = WelsBlockZero8x8_neon;
  }
#endif

#ifdef HAVE_NEON_AARCH64
  if (iCpu & WELS_CPU_NEON) {
    pFunc->pWelsSetNonZeroCountFunc = WelsNonZeroCount_AArch64_neon;
    pFunc->pWelsBlockZero16x16Func  = WelsBlockZero16x16_AArch64_neon;
    pFunc->pWelsBlockZero8x8Func    = WelsBlockZero8x8_AArch64_neon;
  }
#endif

#if defined(X86_ASM)
  if (iCpu & WELS_CPU_SSE2) {
    pFunc->pWelsSetNonZeroCountFunc = WelsNonZeroCount_sse2;
    pFunc->pWelsBlockZero16x16Func  = WelsBlockZero16x16_sse2;
    pFunc->pWelsBlockZero8x8Func    = WelsBlockZero8x8_sse2;
  }
#endif

}

void WelsBlockInit (int16_t* pBlock, int iW, int iH, int iStride, uint8_t uiVal) {
  int32_t i;
  int16_t* pDst = pBlock;

  for (i = 0; i < iH; i++) {
    memset (pDst, uiVal, iW * sizeof (int16_t));
    pDst += iStride;
  }
}
void WelsBlockZero16x16_c (int16_t* pBlock, int32_t iStride) {
  WelsBlockInit (pBlock, 16, 16, iStride, 0);
}

void WelsBlockZero8x8_c (int16_t* pBlock, int32_t iStride) {
  WelsBlockInit (pBlock, 8, 8, iStride, 0);
}
bool ComputeColocated (PWelsDecoderContext pCtx) {
  PDqLayer pCurLayer = pCtx->pCurDqLayer;
  PSlice pCurSlice = &pCurLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader = &pCurSlice->sSliceHeaderExt.sSliceHeader;
  if (!pSliceHeader->iDirectSpatialMvPredFlag) {
    uint32_t uiShortRefCount = pCtx->sRefPic.uiShortRefCount[LIST_0];
    for (int32_t listIdx = LIST_0; listIdx < LIST_A; ++listIdx) {
      for (uint32_t i = 0; i < uiShortRefCount; ++i) {
        int32_t iTRb = WELS_CLIP3 (-128, 127, pSliceHeader->iPicOrderCntLsb - pCtx->sRefPic.pRefList[listIdx][i]->iFramePoc);
        int32_t iTRp = WELS_CLIP3 (-128, 127,
                                   pCtx->sRefPic.pRefList[LIST_1][i]->iFramePoc - pCtx->sRefPic.pRefList[LIST_0][i]->iFramePoc);
        if (iTRp != 0) {
          int32_t prescale = (16384 + iAbs (iTRp / 2)) / iTRp;
          pCurSlice->iMvScale[listIdx][i] = WELS_CLIP3 (-1024, 1023, (iTRb * prescale + 32) >> 6);
        } else {
          pCurSlice->iMvScale[listIdx][i] = 0x03FFF;
        }
      }
    }
  }
  //Implement the following
  //get Mv_colocated_L1
  //and do calculation
  //iMvp[LIST_0] = Mv_colocated_L1 * (POC(cur) - POC(L0))/POC(L1) - POC(L0))
  //iMvp[LIST_1] = Mv_colocated_L1 * (POC(cur) - POC(L1))/POC(L1) - POC(L0))
  return true;
}
} // namespace WelsDec
