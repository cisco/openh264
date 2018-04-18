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
 * \file    rec_mb.c
 *
 * \brief   implementation for all macroblock decoding process after mb syntax parsing and residual decoding with cavlc.
 *
 * \date    3/18/2009 Created
 *
 *************************************************************************************
 */


#include "rec_mb.h"
#include "decode_slice.h"

namespace WelsDec {

void WelsFillRecNeededMbInfo (PWelsDecoderContext pCtx, bool bOutput, PDqLayer pCurLayer) {
  PPicture pCurPic = pCtx->pDec;
  int32_t iLumaStride   = pCurPic->iLinesize[0];
  int32_t iChromaStride = pCurPic->iLinesize[1];
  int32_t iMbX = pCurLayer->iMbX;
  int32_t iMbY = pCurLayer->iMbY;

  pCurLayer->iLumaStride = iLumaStride;
  pCurLayer->iChromaStride = iChromaStride;

  if (bOutput) {
    pCurLayer->pPred[0] = pCurPic->pData[0] + ((iMbY * iLumaStride + iMbX) << 4);
    pCurLayer->pPred[1] = pCurPic->pData[1] + ((iMbY * iChromaStride + iMbX) << 3);
    pCurLayer->pPred[2] = pCurPic->pData[2] + ((iMbY * iChromaStride + iMbX) << 3);
  }
}

int32_t RecI8x8Mb (int32_t iMbXy, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer) {
  RecI8x8Luma (iMbXy, pCtx, pScoeffLevel, pDqLayer);
  RecI4x4Chroma (iMbXy, pCtx, pScoeffLevel, pDqLayer);
  return ERR_NONE;
}

int32_t RecI8x8Luma (int32_t iMbXy, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer) {
  /*****get local variable from outer variable********/
  /*prediction info*/
  uint8_t* pPred = pDqLayer->pPred[0];

  int32_t iLumaStride = pDqLayer->iLumaStride;
  int32_t* pBlockOffset = pCtx->iDecBlockOffsetArray;
  PGetIntraPred8x8Func* pGetI8x8LumaPredFunc = pCtx->pGetI8x8LumaPredFunc;

  int8_t* pIntra8x8PredMode = pDqLayer->pIntra4x4FinalMode[iMbXy]; // I_NxN
  int16_t* pRS = pScoeffLevel;
  /*itransform info*/
  PIdctResAddPredFunc pIdctResAddPredFunc = pCtx->pIdctResAddPredFunc8x8;

  /*************local variable********************/
  uint8_t i = 0;
  bool bTLAvail[4], bTRAvail[4];
  // Top-Right : Left : Top-Left : Top
  bTLAvail[0] = !! (pDqLayer->pIntraNxNAvailFlag[iMbXy] & 0x02);
  bTLAvail[1] = !! (pDqLayer->pIntraNxNAvailFlag[iMbXy] & 0x01);
  bTLAvail[2] = !! (pDqLayer->pIntraNxNAvailFlag[iMbXy] & 0x04);
  bTLAvail[3] = true;

  bTRAvail[0] = !! (pDqLayer->pIntraNxNAvailFlag[iMbXy] & 0x01);
  bTRAvail[1] = !! (pDqLayer->pIntraNxNAvailFlag[iMbXy] & 0x08);
  bTRAvail[2] = true;
  bTRAvail[3] = false;

  /*************real process*********************/
  for (i = 0; i < 4; i++) {

    uint8_t* pPredI8x8 = pPred + pBlockOffset[i << 2];
    uint8_t uiMode = pIntra8x8PredMode[g_kuiScan4[i << 2]];

    pGetI8x8LumaPredFunc[uiMode] (pPredI8x8, iLumaStride, bTLAvail[i], bTRAvail[i]);

    int32_t iIndex = g_kuiMbCountScan4Idx[i << 2];
    if (pDqLayer->pNzc[iMbXy][iIndex] || pDqLayer->pNzc[iMbXy][iIndex + 1] || pDqLayer->pNzc[iMbXy][iIndex + 4]
        || pDqLayer->pNzc[iMbXy][iIndex + 5]) {
      int16_t* pRSI8x8 = &pRS[i << 6];
      pIdctResAddPredFunc (pPredI8x8, iLumaStride, pRSI8x8);
    }
  }

  return ERR_NONE;
}

int32_t RecI4x4Mb (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer) {
  RecI4x4Luma (iMBXY, pCtx, pScoeffLevel, pDqLayer);
  RecI4x4Chroma (iMBXY, pCtx, pScoeffLevel, pDqLayer);
  return ERR_NONE;
}


int32_t RecI4x4Luma (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer) {
  /*****get local variable from outer variable********/
  /*prediction info*/
  uint8_t* pPred = pDqLayer->pPred[0];

  int32_t iLumaStride = pDqLayer->iLumaStride;
  int32_t* pBlockOffset = pCtx->iDecBlockOffsetArray;
  PGetIntraPredFunc* pGetI4x4LumaPredFunc = pCtx->pGetI4x4LumaPredFunc;

  int8_t* pIntra4x4PredMode = pDqLayer->pIntra4x4FinalMode[iMBXY];
  int16_t* pRS = pScoeffLevel;
  /*itransform info*/
  PIdctResAddPredFunc pIdctResAddPredFunc = pCtx->pIdctResAddPredFunc;


  /*************local variable********************/
  uint8_t i = 0;

  /*************real process*********************/
  for (i = 0; i < 16; i++) {

    uint8_t* pPredI4x4 = pPred + pBlockOffset[i];
    uint8_t uiMode = pIntra4x4PredMode[g_kuiScan4[i]];

    pGetI4x4LumaPredFunc[uiMode] (pPredI4x4, iLumaStride);

    if (pDqLayer->pNzc[iMBXY][g_kuiMbCountScan4Idx[i]]) {
      int16_t* pRSI4x4 = &pRS[i << 4];
      pIdctResAddPredFunc (pPredI4x4, iLumaStride, pRSI4x4);
    }
  }

  return ERR_NONE;
}


int32_t RecI4x4Chroma (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer) {
  int32_t iChromaStride = pCtx->pCurDqLayer->pDec->iLinesize[1];

  int8_t iChromaPredMode = pDqLayer->pChromaPredMode[iMBXY];

  PGetIntraPredFunc* pGetIChromaPredFunc = pCtx->pGetIChromaPredFunc;

  uint8_t* pPred = pDqLayer->pPred[1];

  pGetIChromaPredFunc[iChromaPredMode] (pPred, iChromaStride);
  pPred = pDqLayer->pPred[2];
  pGetIChromaPredFunc[iChromaPredMode] (pPred, iChromaStride);

  RecChroma (iMBXY, pCtx, pScoeffLevel, pDqLayer);

  return ERR_NONE;
}


int32_t RecI16x16Mb (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer) {
  /*decoder use, encoder no use*/
  int8_t iI16x16PredMode = pDqLayer->pIntraPredMode[iMBXY][7];
  int8_t iChromaPredMode = pDqLayer->pChromaPredMode[iMBXY];
  PGetIntraPredFunc* pGetIChromaPredFunc = pCtx->pGetIChromaPredFunc;
  PGetIntraPredFunc* pGetI16x16LumaPredFunc = pCtx->pGetI16x16LumaPredFunc;
  int32_t iUVStride = pCtx->pCurDqLayer->pDec->iLinesize[1];

  /*common use by decoder&encoder*/
  int32_t iYStride = pDqLayer->iLumaStride;
  int16_t* pRS = pScoeffLevel;

  uint8_t* pPred = pDqLayer->pPred[0];

  PIdctFourResAddPredFunc pIdctFourResAddPredFunc = pCtx->pIdctFourResAddPredFunc;

  /*decode i16x16 y*/
  pGetI16x16LumaPredFunc[iI16x16PredMode] (pPred, iYStride);

  /*1 mb is divided 16 4x4_block to idct*/
  const int8_t* pNzc = pDqLayer->pNzc[iMBXY];
  pIdctFourResAddPredFunc (pPred + 0 * iYStride + 0, iYStride, pRS + 0 * 64, pNzc +  0);
  pIdctFourResAddPredFunc (pPred + 0 * iYStride + 8, iYStride, pRS + 1 * 64, pNzc +  2);
  pIdctFourResAddPredFunc (pPred + 8 * iYStride + 0, iYStride, pRS + 2 * 64, pNzc +  8);
  pIdctFourResAddPredFunc (pPred + 8 * iYStride + 8, iYStride, pRS + 3 * 64, pNzc + 10);

  /*decode intra mb cb&cr*/
  pPred = pDqLayer->pPred[1];
  pGetIChromaPredFunc[iChromaPredMode] (pPred, iUVStride);
  pPred = pDqLayer->pPred[2];
  pGetIChromaPredFunc[iChromaPredMode] (pPred, iUVStride);
  RecChroma (iMBXY, pCtx, pScoeffLevel, pDqLayer);

  return ERR_NONE;
}


//according to current 8*8 block ref_index to gain reference picture
static inline void GetRefPic (sMCRefMember* pMCRefMem, PWelsDecoderContext pCtx, int8_t* pRefIdxList,
                              int32_t iIndex, int32_t listIdx) {
  PPicture pRefPic;

  int8_t iRefIdx = pRefIdxList[iIndex];
  pRefPic = pCtx->sRefPic.pRefList[listIdx][iRefIdx];

  pMCRefMem->iSrcLineLuma   = pRefPic->iLinesize[0];
  pMCRefMem->iSrcLineChroma = pRefPic->iLinesize[1];

  pMCRefMem->pSrcY = pRefPic->pData[0];
  pMCRefMem->pSrcU = pRefPic->pData[1];
  pMCRefMem->pSrcV = pRefPic->pData[2];
}


#ifndef MC_FLOW_SIMPLE_JUDGE
#define MC_FLOW_SIMPLE_JUDGE 1
#endif //MC_FLOW_SIMPLE_JUDGE
void BaseMC (sMCRefMember* pMCRefMem, int32_t iXOffset, int32_t iYOffset, SMcFunc* pMCFunc,
             int32_t iBlkWidth, int32_t iBlkHeight, int16_t iMVs[2]) {
  int32_t iFullMVx = (iXOffset << 2) + iMVs[0]; //quarter pixel
  int32_t iFullMVy = (iYOffset << 2) + iMVs[1];
  iFullMVx = WELS_CLIP3 (iFullMVx, ((-PADDING_LENGTH + 2) * (1 << 2)),
                         ((pMCRefMem->iPicWidth + PADDING_LENGTH - 19) * (1 << 2)));
  iFullMVy = WELS_CLIP3 (iFullMVy, ((-PADDING_LENGTH + 2) * (1 << 2)),
                         ((pMCRefMem->iPicHeight + PADDING_LENGTH - 19) * (1 << 2)));

  int32_t iSrcPixOffsetLuma = (iFullMVx >> 2) + (iFullMVy >> 2) * pMCRefMem->iSrcLineLuma;
  int32_t iSrcPixOffsetChroma = (iFullMVx >> 3) + (iFullMVy >> 3) * pMCRefMem->iSrcLineChroma;

  int32_t iBlkWidthChroma = iBlkWidth >> 1;
  int32_t iBlkHeightChroma = iBlkHeight >> 1;

  uint8_t* pSrcY = pMCRefMem->pSrcY + iSrcPixOffsetLuma;
  uint8_t* pSrcU = pMCRefMem->pSrcU + iSrcPixOffsetChroma;
  uint8_t* pSrcV = pMCRefMem->pSrcV + iSrcPixOffsetChroma;
  uint8_t* pDstY = pMCRefMem->pDstY;
  uint8_t* pDstU = pMCRefMem->pDstU;
  uint8_t* pDstV = pMCRefMem->pDstV;

  pMCFunc->pMcLumaFunc (pSrcY, pMCRefMem->iSrcLineLuma, pDstY, pMCRefMem->iDstLineLuma, iFullMVx, iFullMVy, iBlkWidth,
                        iBlkHeight);
  pMCFunc->pMcChromaFunc (pSrcU, pMCRefMem->iSrcLineChroma, pDstU, pMCRefMem->iDstLineChroma, iFullMVx, iFullMVy,
                          iBlkWidthChroma, iBlkHeightChroma);
  pMCFunc->pMcChromaFunc (pSrcV, pMCRefMem->iSrcLineChroma, pDstV, pMCRefMem->iDstLineChroma, iFullMVx, iFullMVy,
                          iBlkWidthChroma, iBlkHeightChroma);

}

void WeightPrediction (PDqLayer pCurDqLayer, sMCRefMember* pMCRefMem, int32_t iRefIdx, int32_t iBlkWidth,
                       int32_t iBlkHeight) {


  int32_t iLog2denom, iWoc, iOoc;
  int32_t iPredTemp, iLineStride;
  int32_t iPixel = 0;
  uint8_t* pDst;
  //luma
  iLog2denom = pCurDqLayer->pPredWeightTable->uiLumaLog2WeightDenom;
  iWoc = pCurDqLayer->pPredWeightTable->sPredList[LIST_0].iLumaWeight[iRefIdx];
  iOoc = pCurDqLayer->pPredWeightTable->sPredList[LIST_0].iLumaOffset[iRefIdx];
  iLineStride = pMCRefMem->iDstLineLuma;

  for (int i = 0; i < iBlkHeight; i++) {
    for (int j = 0; j < iBlkWidth; j++) {
      iPixel = j + i * (iLineStride);
      if (iLog2denom >= 1) {
        iPredTemp = ((pMCRefMem->pDstY[iPixel] * iWoc + (1 << (iLog2denom - 1))) >> iLog2denom) + iOoc;

        pMCRefMem->pDstY[iPixel] = WELS_CLIP3 (iPredTemp, 0, 255);
      } else {
        iPredTemp = pMCRefMem->pDstY[iPixel] * iWoc + iOoc;

        pMCRefMem->pDstY[iPixel] = WELS_CLIP3 (iPredTemp, 0, 255);

      }
    }
  }


  //UV
  iBlkWidth = iBlkWidth >> 1;
  iBlkHeight = iBlkHeight >> 1;
  iLog2denom = pCurDqLayer->pPredWeightTable->uiChromaLog2WeightDenom;
  iLineStride = pMCRefMem->iDstLineChroma;

  for (int i = 0; i < 2; i++) {


    //iLog2denom = pCurDqLayer->pPredWeightTable->uiChromaLog2WeightDenom;
    iWoc =  pCurDqLayer->pPredWeightTable->sPredList[LIST_0].iChromaWeight[iRefIdx][i];
    iOoc = pCurDqLayer->pPredWeightTable->sPredList[LIST_0].iChromaOffset[iRefIdx][i];
    pDst = i ? pMCRefMem->pDstV : pMCRefMem->pDstU;
    //iLineStride = pMCRefMem->iDstLineChroma;

    for (int i = 0; i < iBlkHeight ; i++) {
      for (int j = 0; j < iBlkWidth; j++) {
        iPixel = j + i * (iLineStride);
        if (iLog2denom >= 1) {
          iPredTemp = ((pDst[iPixel] * iWoc + (1 << (iLog2denom - 1))) >> iLog2denom) + iOoc;

          pDst[iPixel] = WELS_CLIP3 (iPredTemp, 0, 255);
        } else {
          iPredTemp = pDst[iPixel] * iWoc + iOoc;

          pDst[iPixel] = WELS_CLIP3 (iPredTemp, 0, 255);

        }
      }

    }


  }
}


void GetInterPred (uint8_t* pPredY, uint8_t* pPredCb, uint8_t* pPredCr, PWelsDecoderContext pCtx) {
  sMCRefMember pMCRefMem;
  PDqLayer pCurDqLayer = pCtx->pCurDqLayer;
  SMcFunc* pMCFunc = &pCtx->sMcFunc;

  int32_t iMBXY = pCurDqLayer->iMbXyIndex;

  int16_t iMVs[2] = {0};

  int32_t iMBType = pCurDqLayer->pMbType[iMBXY];

  int32_t iMBOffsetX = pCurDqLayer->iMbX << 4;
  int32_t iMBOffsetY = pCurDqLayer->iMbY << 4;

  int32_t iDstLineLuma   = pCtx->pDec->iLinesize[0];
  int32_t iDstLineChroma = pCtx->pDec->iLinesize[1];

  int32_t iBlk8X, iBlk8Y, iBlk4X, iBlk4Y, i, j, iIIdx, iJIdx;

  pMCRefMem.iPicWidth = (pCurDqLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.iMbWidth << 4);
  pMCRefMem.iPicHeight = (pCurDqLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.iMbHeight << 4);

  pMCRefMem.pDstY = pPredY;
  pMCRefMem.pDstU = pPredCb;
  pMCRefMem.pDstV = pPredCr;

  pMCRefMem.iDstLineLuma   = iDstLineLuma;
  pMCRefMem.iDstLineChroma = iDstLineChroma;

  int32_t iRefIndex = 0;

  switch (iMBType) {
  case MB_TYPE_SKIP:
  case MB_TYPE_16x16:
    iMVs[0] = pCurDqLayer->pMv[0][iMBXY][0][0];
    iMVs[1] = pCurDqLayer->pMv[0][iMBXY][0][1];
    GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], 0, LIST_0);
    BaseMC (&pMCRefMem, iMBOffsetX, iMBOffsetY, pMCFunc, 16, 16, iMVs);

    if (pCurDqLayer->bUseWeightPredictionFlag) {
      iRefIndex = pCurDqLayer->pRefIndex[0][iMBXY][0];
      WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 16, 16);
    }
    break;
  case MB_TYPE_16x8:
    iMVs[0] = pCurDqLayer->pMv[0][iMBXY][0][0];
    iMVs[1] = pCurDqLayer->pMv[0][iMBXY][0][1];
    GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], 0, LIST_0);
    BaseMC (&pMCRefMem, iMBOffsetX, iMBOffsetY, pMCFunc, 16, 8, iMVs);

    if (pCurDqLayer->bUseWeightPredictionFlag) {
      iRefIndex = pCurDqLayer->pRefIndex[0][iMBXY][0];
      WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 16, 8);
    }

    iMVs[0] = pCurDqLayer->pMv[0][iMBXY][8][0];
    iMVs[1] = pCurDqLayer->pMv[0][iMBXY][8][1];
    GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], 8, LIST_0);
    pMCRefMem.pDstY = pPredY  + (iDstLineLuma << 3);
    pMCRefMem.pDstU = pPredCb + (iDstLineChroma << 2);
    pMCRefMem.pDstV = pPredCr + (iDstLineChroma << 2);
    BaseMC (&pMCRefMem, iMBOffsetX, iMBOffsetY + 8, pMCFunc, 16, 8, iMVs);

    if (pCurDqLayer->bUseWeightPredictionFlag) {
      iRefIndex = pCurDqLayer->pRefIndex[0][iMBXY][8];
      WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 16, 8);
    }
    break;
  case MB_TYPE_8x16:
    iMVs[0] = pCurDqLayer->pMv[0][iMBXY][0][0];
    iMVs[1] = pCurDqLayer->pMv[0][iMBXY][0][1];
    GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], 0, LIST_0);
    BaseMC (&pMCRefMem, iMBOffsetX, iMBOffsetY, pMCFunc, 8, 16, iMVs);
    if (pCurDqLayer->bUseWeightPredictionFlag) {
      iRefIndex = pCurDqLayer->pRefIndex[0][iMBXY][0];
      WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 8, 16);
    }

    iMVs[0] = pCurDqLayer->pMv[0][iMBXY][2][0];
    iMVs[1] = pCurDqLayer->pMv[0][iMBXY][2][1];
    GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], 2, LIST_0);
    pMCRefMem.pDstY = pPredY + 8;
    pMCRefMem.pDstU = pPredCb + 4;
    pMCRefMem.pDstV = pPredCr + 4;
    BaseMC (&pMCRefMem, iMBOffsetX + 8, iMBOffsetY, pMCFunc, 8, 16, iMVs);

    if (pCurDqLayer->bUseWeightPredictionFlag) {
      iRefIndex = pCurDqLayer->pRefIndex[0][iMBXY][2];
      WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 8, 16);
    }
    break;
  case MB_TYPE_8x8:
  case MB_TYPE_8x8_REF0: {
    uint32_t iSubMBType;
    int32_t iXOffset, iYOffset;
    uint8_t* pDstY, *pDstU, *pDstV;
    for (i = 0; i < 4; i++) {
      iSubMBType = pCurDqLayer->pSubMbType[iMBXY][i];
      iBlk8X = (i & 1) << 3;
      iBlk8Y = (i >> 1) << 3;
      iXOffset = iMBOffsetX + iBlk8X;
      iYOffset = iMBOffsetY + iBlk8Y;

      iIIdx = ((i >> 1) << 3) + ((i & 1) << 1);
      GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], iIIdx, LIST_0);
      iRefIndex = pCurDqLayer->bUseWeightPredictionFlag ? pCurDqLayer->pRefIndex[0][iMBXY][iIIdx] : 0;

      pDstY = pPredY + iBlk8X + iBlk8Y * iDstLineLuma;
      pDstU = pPredCb + (iBlk8X >> 1) + (iBlk8Y >> 1) * iDstLineChroma;
      pDstV = pPredCr + (iBlk8X >> 1) + (iBlk8Y >> 1) * iDstLineChroma;
      pMCRefMem.pDstY = pDstY;
      pMCRefMem.pDstU = pDstU;
      pMCRefMem.pDstV = pDstV;
      switch (iSubMBType) {
      case SUB_MB_TYPE_8x8:
        iMVs[0] = pCurDqLayer->pMv[0][iMBXY][iIIdx][0];
        iMVs[1] = pCurDqLayer->pMv[0][iMBXY][iIIdx][1];
        BaseMC (&pMCRefMem, iXOffset, iYOffset, pMCFunc, 8, 8, iMVs);
        if (pCurDqLayer->bUseWeightPredictionFlag) {

          WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 8, 8);
        }

        break;
      case SUB_MB_TYPE_8x4:
        iMVs[0] = pCurDqLayer->pMv[0][iMBXY][iIIdx][0];
        iMVs[1] = pCurDqLayer->pMv[0][iMBXY][iIIdx][1];
        BaseMC (&pMCRefMem, iXOffset, iYOffset, pMCFunc, 8, 4, iMVs);
        if (pCurDqLayer->bUseWeightPredictionFlag) {

          WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 8, 4);
        }


        iMVs[0] = pCurDqLayer->pMv[0][iMBXY][iIIdx + 4][0];
        iMVs[1] = pCurDqLayer->pMv[0][iMBXY][iIIdx + 4][1];
        pMCRefMem.pDstY += (iDstLineLuma << 2);
        pMCRefMem.pDstU += (iDstLineChroma << 1);
        pMCRefMem.pDstV += (iDstLineChroma << 1);
        BaseMC (&pMCRefMem, iXOffset, iYOffset + 4, pMCFunc, 8, 4, iMVs);
        if (pCurDqLayer->bUseWeightPredictionFlag) {

          WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 8, 4);
        }

        break;
      case SUB_MB_TYPE_4x8:
        iMVs[0] = pCurDqLayer->pMv[0][iMBXY][iIIdx][0];
        iMVs[1] = pCurDqLayer->pMv[0][iMBXY][iIIdx][1];
        BaseMC (&pMCRefMem, iXOffset, iYOffset, pMCFunc, 4, 8, iMVs);
        if (pCurDqLayer->bUseWeightPredictionFlag) {

          WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 4, 8);
        }


        iMVs[0] = pCurDqLayer->pMv[0][iMBXY][iIIdx + 1][0];
        iMVs[1] = pCurDqLayer->pMv[0][iMBXY][iIIdx + 1][1];
        pMCRefMem.pDstY += 4;
        pMCRefMem.pDstU += 2;
        pMCRefMem.pDstV += 2;
        BaseMC (&pMCRefMem, iXOffset + 4, iYOffset, pMCFunc, 4, 8, iMVs);
        if (pCurDqLayer->bUseWeightPredictionFlag) {

          WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 4, 8);
        }

        break;
      case SUB_MB_TYPE_4x4: {
        for (j = 0; j < 4; j++) {
          int32_t iUVLineStride;
          iJIdx = ((j >> 1) << 2) + (j & 1);

          iBlk4X = (j & 1) << 2;
          iBlk4Y = (j >> 1) << 2;

          iUVLineStride = (iBlk4X >> 1) + (iBlk4Y >> 1) * iDstLineChroma;
          pMCRefMem.pDstY = pDstY + iBlk4X + iBlk4Y * iDstLineLuma;
          pMCRefMem.pDstU = pDstU + iUVLineStride;
          pMCRefMem.pDstV = pDstV + iUVLineStride;

          iMVs[0] = pCurDqLayer->pMv[0][iMBXY][iIIdx + iJIdx][0];
          iMVs[1] = pCurDqLayer->pMv[0][iMBXY][iIIdx + iJIdx][1];
          BaseMC (&pMCRefMem, iXOffset + iBlk4X, iYOffset + iBlk4Y, pMCFunc, 4, 4, iMVs);
          if (pCurDqLayer->bUseWeightPredictionFlag) {

            WeightPrediction (pCurDqLayer, &pMCRefMem, iRefIndex, 4, 4);
          }

        }
      }
      break;
      default:
        break;
      }
    }
  }
  break;
  default:
    break;
  }
}

void GetInterBPred(uint8_t* pPredY, uint8_t* pPredCb, uint8_t* pPredCr, PWelsDecoderContext pCtx) {
	sMCRefMember pMCRefMem;
	sMCRefMember pMCRefMem1;
	sMCRefMember pMCRefMem2;
	PDqLayer pCurDqLayer = pCtx->pCurDqLayer;
	SMcFunc* pMCFunc = &pCtx->sMcFunc;

	int32_t iMBXY = pCurDqLayer->iMbXyIndex;

	int16_t iMVs1[2] = { 0 };
	int16_t iMVs2[2] = { 0 };

	int32_t iMBType = pCurDqLayer->pMbType[iMBXY];

	int32_t iMBOffsetX = pCurDqLayer->iMbX << 4;
	int32_t iMBOffsetY = pCurDqLayer->iMbY << 4;

	int32_t iDstLineLuma = pCtx->pDec->iLinesize[0];
	int32_t iDstLineChroma = pCtx->pDec->iLinesize[1];


	pMCRefMem.iPicWidth = (pCurDqLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.iMbWidth << 4);
	pMCRefMem.iPicHeight = (pCurDqLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.iMbHeight << 4);

	pMCRefMem.pDstY = pPredY;
	pMCRefMem.pDstU = pPredCb;
	pMCRefMem.pDstV = pPredCr;

	pMCRefMem.iDstLineLuma = iDstLineLuma;
	pMCRefMem.iDstLineChroma = iDstLineChroma;

	int32_t iRefIndex1 = 0;
	int32_t iRefIndex2 = 0;

	if (IS_INTER_16x16(iMBType)) {
		if ((iMBType & MB_TYPE_P0L0) && (iMBType & MB_TYPE_P0L1)) {
			pMCRefMem1 = pMCRefMem;
			pMCRefMem2 = pMCRefMem;
			iMVs1[0] = pCurDqLayer->pMv[LIST_0][iMBXY][0][0];
			iMVs1[1] = pCurDqLayer->pMv[LIST_0][iMBXY][0][1];
			GetRefPic(&pMCRefMem1, pCtx, pCurDqLayer->pRefIndex[LIST_0][iMBXY], 0, LIST_0);
			BaseMC(&pMCRefMem1, iMBOffsetX, iMBOffsetY, pMCFunc, 16, 16, iMVs1);

			iMVs2[0] = pCurDqLayer->pMv[LIST_1][iMBXY][0][0];
			iMVs2[1] = pCurDqLayer->pMv[LIST_1][iMBXY][0][1];
			GetRefPic(&pMCRefMem2, pCtx, pCurDqLayer->pRefIndex[LIST_1][iMBXY], 0, LIST_1);
			BaseMC(&pMCRefMem2, iMBOffsetX, iMBOffsetY, pMCFunc, 16, 16, iMVs1);
			if (pCurDqLayer->sLayerInfo.pPps->uiWeightedBipredIdc) {
				iRefIndex1 = pCurDqLayer->pRefIndex[LIST_0][iMBXY][0];
				iRefIndex2 = pCurDqLayer->pRefIndex[LIST_0][iMBXY][0];
//				BiWeightPrediction(pCurDqLayer, &pMCRefMem, &pMCRefMem1, &pMCRefMem2, iRefIndex1, iRefIndex2, 16, 16);
			}
		}
		else if ((iMBType & MB_TYPE_P0L0) || (iMBType & MB_TYPE_P0L1)) {
			int32_t listIdx = (iMBType & MB_TYPE_P0L0) ? LIST_0 : LIST_1;
			iMVs1[0] = pCurDqLayer->pMv[listIdx][iMBXY][0][0];
			iMVs1[1] = pCurDqLayer->pMv[listIdx][iMBXY][0][1];
			GetRefPic(&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[listIdx][iMBXY], 0, listIdx);
			BaseMC(&pMCRefMem, iMBOffsetX, iMBOffsetY, pMCFunc, 16, 16, iMVs1);
			if (pCurDqLayer->bUseWeightPredictionFlag) {
				iRefIndex1 = pCurDqLayer->pRefIndex[listIdx][iMBXY][0];
				WeightPrediction(pCurDqLayer, &pMCRefMem, iRefIndex1, 16, 16);
			}
		}
	}
}

int32_t RecChroma (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer) {
  int32_t iChromaStride = pCtx->pCurDqLayer->pDec->iLinesize[1];
  PIdctFourResAddPredFunc pIdctFourResAddPredFunc = pCtx->pIdctFourResAddPredFunc;

  uint8_t i = 0;
  uint8_t uiCbpC = pDqLayer->pCbp[iMBXY] >> 4;

  if (1 == uiCbpC || 2 == uiCbpC) {
    for (i = 0; i < 2; i++) {
      int16_t* pRS = pScoeffLevel + 256 + (i << 6);
      uint8_t* pPred = pDqLayer->pPred[i + 1];
      const int8_t* pNzc = pDqLayer->pNzc[iMBXY] + 16 + 2 * i;

      /*1 chroma is divided 4 4x4_block to idct*/
      pIdctFourResAddPredFunc (pPred, iChromaStride, pRS, pNzc);
    }
  }

  return ERR_NONE;
}

} // namespace WelsDec
