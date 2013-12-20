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
 * \file	rec_mb.c
 *
 * \brief	implementation for all macroblock decoding process after mb syntax parsing and residual decoding with cavlc.
 *
 * \date	3/18/2009 Created
 *
 *************************************************************************************
 */
#include <memory.h>

#include "macros.h"

#include "rec_mb.h"
#include "parse_mb_syn_cavlc.h"
#include "get_intra_predictor.h"
#include "decode_mb_aux.h"
#include "decode_slice.h"

namespace WelsDec {

void_t WelsFillRecNeededMbInfo (PWelsDecoderContext pCtx, bool_t bOutput, PDqLayer pCurLayer) {
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
  PIdctResAddPredFunc	pIdctResAddPredFunc = pCtx->pIdctResAddPredFunc;


  /*************local variable********************/
  uint8_t i = 0;

  /*************real process*********************/
  for (i = 0; i < 16; i++) {

    uint8_t* pPredI4x4 = pPred + pBlockOffset[i];
    uint8_t uiMode = pIntra4x4PredMode[g_kuiScan4[i]];

    pGetI4x4LumaPredFunc[uiMode] (pPredI4x4, iLumaStride);

    if (pDqLayer->pNzc[iMBXY][g_kuiMbNonZeroCountIdx[i]]) {
      int16_t* pRSI4x4 = &pRS[i << 4];
      pIdctResAddPredFunc (pPredI4x4, iLumaStride, pRSI4x4);
    }
  }

  return ERR_NONE;
}


int32_t RecI4x4Chroma (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer) {
  int32_t iChromaStride = pCtx->pCurDqLayer->iCsStride[1];

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
  int32_t iUVStride = pCtx->pCurDqLayer->iCsStride[1];

  /*common use by decoder&encoder*/
  int32_t iYStride = pDqLayer->iLumaStride;
  int32_t* pBlockOffset = pCtx->iDecBlockOffsetArray;
  int16_t* pRS = pScoeffLevel;

  uint8_t* pPred = pDqLayer->pPred[0];

  PIdctResAddPredFunc pIdctResAddPredFunc = pCtx->pIdctResAddPredFunc;

  uint8_t i = 0;

  /*decode i16x16 y*/
  pGetI16x16LumaPredFunc[iI16x16PredMode] (pPred, iYStride);

  /*1 mb is divided 16 4x4_block to idct*/
  for (i = 0; i < 16; i++) {
    int16_t* pRSI4x4 = pRS + (i << 4);
    uint8_t* pPredI4x4 = pPred + pBlockOffset[i];

    if (pDqLayer->pNzc[iMBXY][g_kuiMbNonZeroCountIdx[i]] || pRSI4x4[0]) {
      pIdctResAddPredFunc (pPredI4x4, iYStride, pRSI4x4);
    }
  }

  /*decode intra mb cb&cr*/
  pPred = pDqLayer->pPred[1];
  pGetIChromaPredFunc[iChromaPredMode] (pPred, iUVStride);
  pPred = pDqLayer->pPred[2];
  pGetIChromaPredFunc[iChromaPredMode] (pPred, iUVStride);
  RecChroma (iMBXY, pCtx, pScoeffLevel, pDqLayer);

  return ERR_NONE;
}

typedef struct TagMCRefMember {
  uint8_t* pDstY;
  uint8_t* pDstU;
  uint8_t* pDstV;

  uint8_t* pSrcY;
  uint8_t* pSrcU;
  uint8_t* pSrcV;

  int32_t iSrcLineLuma;
  int32_t iSrcLineChroma;

  int32_t iDstLineLuma;
  int32_t iDstLineChroma;

  int32_t iPicWidth;
  int32_t iPicHeight;
} sMCRefMember;
//according to current 8*8 block ref_index to gain reference picture
static inline void_t GetRefPic (sMCRefMember* pMCRefMem, PWelsDecoderContext pCtx, int8_t* pRefIdxList,
                                int32_t iIndex) {
  PPicture pRefPic;

  int8_t iRefIdx = pRefIdxList[iIndex];
  pRefPic = pCtx->sRefPic.pRefList[LIST_0][iRefIdx];

  pMCRefMem->iSrcLineLuma   = pRefPic->iLinesize[0];
  pMCRefMem->iSrcLineChroma = pRefPic->iLinesize[1];

  pMCRefMem->pSrcY = pRefPic->pData[0];
  pMCRefMem->pSrcU = pRefPic->pData[1];
  pMCRefMem->pSrcV = pRefPic->pData[2];
}


#ifndef MC_FLOW_SIMPLE_JUDGE
#define MC_FLOW_SIMPLE_JUDGE 1
#endif //MC_FLOW_SIMPLE_JUDGE
static inline void_t BaseMC (sMCRefMember* pMCRefMem, int32_t iXOffset, int32_t iYOffset, SMcFunc* pMCFunc,
                             int32_t iBlkWidth, int32_t iBlkHeight, int16_t iMVs[2]) {
  int32_t iExpandWidth = PADDING_LENGTH;
  int32_t	iExpandHeight = PADDING_LENGTH;


  int16_t iMVX = iMVs[0] >> 2;
  int16_t iMVY = iMVs[1] >> 2;
  int32_t iMVOffsetLuma = iMVX + iMVY * pMCRefMem->iSrcLineLuma;
  int32_t iMVOffsetChroma = (iMVX >> 1) + (iMVY >> 1) * pMCRefMem->iSrcLineChroma;

  int32_t iFullMVx = (iXOffset << 2) + iMVs[0]; //quarter pixel
  int32_t iFullMVy = (iYOffset << 2) + iMVs[1];
  int32_t iIntMVx = iFullMVx >> 2;//integer pixel
  int32_t iIntMVy = iFullMVy >> 2;

  int32_t iSrcPixOffsetLuma = iXOffset + iYOffset * pMCRefMem->iSrcLineLuma;
  int32_t iSrcPixOffsetChroma = (iXOffset >> 1) + (iYOffset >> 1) * pMCRefMem->iSrcLineChroma;

  int32_t iBlkWidthChroma = iBlkWidth >> 1;
  int32_t iBlkHeightChroma = iBlkHeight >> 1;
  int32_t iPicWidthChroma = pMCRefMem->iPicWidth >> 1;
  int32_t iPicHeightChroma = pMCRefMem->iPicHeight >> 1;

  //the offset only for luma padding if MV violation as there was 5-tap (-2, -1, 0, 1, 2) filter for luma (horizon and vertical)
  int32_t iPadOffset = 2 + (pMCRefMem->iSrcLineLuma << 1); //(-2, -2) pixel location as the starting point

  uint8_t* pSrcY = pMCRefMem->pSrcY + iSrcPixOffsetLuma;
  uint8_t* pSrcU = pMCRefMem->pSrcU + iSrcPixOffsetChroma;
  uint8_t* pSrcV = pMCRefMem->pSrcV + iSrcPixOffsetChroma;
  uint8_t* pDstY = pMCRefMem->pDstY;
  uint8_t* pDstU = pMCRefMem->pDstU;
  uint8_t* pDstV = pMCRefMem->pDstV;
  bool_t bExpand = false;

  FORCE_STACK_ALIGN_1D (uint8_t, uiExpandBuf, (PADDING_LENGTH + 6) * (PADDING_LENGTH + 6), 16);

  if (iFullMVx & 0x07) {
    iExpandWidth -= 3;
  }
  if (iFullMVy & 0x07) {
    iExpandHeight -= 3;
  }

#ifdef MC_FLOW_SIMPLE_JUDGE
  if (iIntMVx < -iExpandWidth ||
      iIntMVy < -iExpandHeight ||
      iIntMVx + iBlkWidth > pMCRefMem->iPicWidth - 1 + iExpandWidth ||
      iIntMVy + iBlkHeight > pMCRefMem->iPicHeight - 1 + iExpandHeight)
#else
  if (iIntMVx < -iExpandWidth ||
      iIntMVy < -iExpandHeight ||
      iIntMVx + PADDING_LENGTH > pMCRefMem->iPicWidth + iExpandWidth ||
      iIntMVy + PADDING_LENGTH > pMCRefMem->iPicHeight + iExpandHeight)
#endif
  {
    FillBufForMc (uiExpandBuf, 21, pSrcY, pMCRefMem->iSrcLineLuma, iMVOffsetLuma - iPadOffset,
                  iBlkWidth + 5, iBlkHeight + 5, iIntMVx - 2, iIntMVy - 2, pMCRefMem->iPicWidth, pMCRefMem->iPicHeight);
    pMCFunc->pMcLumaFunc (uiExpandBuf + 44, 21, pDstY, pMCRefMem->iDstLineLuma, iFullMVx, iFullMVy, iBlkWidth,
                          iBlkHeight); //44=2+2*21
    bExpand = true;
  } else {
    pSrcY += iMVOffsetLuma;
    pMCFunc->pMcLumaFunc (pSrcY, pMCRefMem->iSrcLineLuma, pDstY, pMCRefMem->iDstLineLuma, iFullMVx, iFullMVy, iBlkWidth,
                          iBlkHeight);
  }

  if (bExpand) {
    FillBufForMc (uiExpandBuf, 21, pSrcU, pMCRefMem->iSrcLineChroma, iMVOffsetChroma, iBlkWidthChroma + 1,
                  iBlkHeightChroma + 1, iFullMVx >> 3, iFullMVy >> 3, iPicWidthChroma, iPicHeightChroma);
    pMCFunc->pMcChromaFunc (uiExpandBuf, 21, pDstU, pMCRefMem->iDstLineChroma, iFullMVx, iFullMVy, iBlkWidthChroma,
                            iBlkHeightChroma);

    FillBufForMc (uiExpandBuf, 21, pSrcV, pMCRefMem->iSrcLineChroma, iMVOffsetChroma, iBlkWidthChroma + 1,
                  iBlkHeightChroma + 1, iFullMVx >> 3, iFullMVy >> 3, iPicWidthChroma, iPicHeightChroma);
    pMCFunc->pMcChromaFunc (uiExpandBuf, 21, pDstV, pMCRefMem->iDstLineChroma, iFullMVx, iFullMVy, iBlkWidthChroma,
                            iBlkHeightChroma);
  } else {
    pSrcU += iMVOffsetChroma;
    pSrcV += iMVOffsetChroma;
    pMCFunc->pMcChromaFunc (pSrcU, pMCRefMem->iSrcLineChroma, pDstU, pMCRefMem->iDstLineChroma, iFullMVx, iFullMVy,
                            iBlkWidthChroma, iBlkHeightChroma);
    pMCFunc->pMcChromaFunc (pSrcV, pMCRefMem->iSrcLineChroma, pDstV, pMCRefMem->iDstLineChroma, iFullMVx, iFullMVy,
                            iBlkWidthChroma, iBlkHeightChroma);
  }
}

void_t GetInterPred (uint8_t* pPredY, uint8_t* pPredCb, uint8_t* pPredCr, PWelsDecoderContext pCtx) {
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
  switch (iMBType) {
  case MB_TYPE_SKIP:
  case MB_TYPE_16x16:
    iMVs[0] = pCurDqLayer->pMv[0][iMBXY][0][0];
    iMVs[1] = pCurDqLayer->pMv[0][iMBXY][0][1];
    GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], 0);
    BaseMC (&pMCRefMem, iMBOffsetX, iMBOffsetY, pMCFunc, 16, 16, iMVs);
    break;
  case MB_TYPE_16x8:
    iMVs[0] = pCurDqLayer->pMv[0][iMBXY][0][0];
    iMVs[1] = pCurDqLayer->pMv[0][iMBXY][0][1];
    GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], 0);
    BaseMC (&pMCRefMem, iMBOffsetX, iMBOffsetY, pMCFunc, 16, 8, iMVs);

    iMVs[0] = pCurDqLayer->pMv[0][iMBXY][8][0];
    iMVs[1] = pCurDqLayer->pMv[0][iMBXY][8][1];
    GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], 8);
    pMCRefMem.pDstY = pPredY  + (iDstLineLuma << 3);
    pMCRefMem.pDstU = pPredCb + (iDstLineChroma << 2);
    pMCRefMem.pDstV = pPredCr + (iDstLineChroma << 2);
    BaseMC (&pMCRefMem, iMBOffsetX, iMBOffsetY + 8, pMCFunc, 16, 8, iMVs);
    break;
  case MB_TYPE_8x16:
    iMVs[0] = pCurDqLayer->pMv[0][iMBXY][0][0];
    iMVs[1] = pCurDqLayer->pMv[0][iMBXY][0][1];
    GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], 0);
    BaseMC (&pMCRefMem, iMBOffsetX, iMBOffsetY, pMCFunc, 8, 16, iMVs);

    iMVs[0] = pCurDqLayer->pMv[0][iMBXY][2][0];
    iMVs[1] = pCurDqLayer->pMv[0][iMBXY][2][1];
    GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], 2);
    pMCRefMem.pDstY = pPredY + 8;
    pMCRefMem.pDstU = pPredCb + 4;
    pMCRefMem.pDstV = pPredCr + 4;
    BaseMC (&pMCRefMem, iMBOffsetX + 8, iMBOffsetY, pMCFunc, 8, 16, iMVs);
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
      GetRefPic (&pMCRefMem, pCtx, pCurDqLayer->pRefIndex[0][iMBXY], iIIdx);

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
        break;
      case SUB_MB_TYPE_8x4:
        iMVs[0] = pCurDqLayer->pMv[0][iMBXY][iIIdx][0];
        iMVs[1] = pCurDqLayer->pMv[0][iMBXY][iIIdx][1];
        BaseMC (&pMCRefMem, iXOffset, iYOffset, pMCFunc, 8, 4, iMVs);

        iMVs[0] = pCurDqLayer->pMv[0][iMBXY][iIIdx + 4][0];
        iMVs[1] = pCurDqLayer->pMv[0][iMBXY][iIIdx + 4][1];
        pMCRefMem.pDstY += (iDstLineLuma << 2);
        pMCRefMem.pDstU += (iDstLineChroma << 1);
        pMCRefMem.pDstV += (iDstLineChroma << 1);
        BaseMC (&pMCRefMem, iXOffset, iYOffset + 4, pMCFunc, 8, 4, iMVs);
        break;
      case SUB_MB_TYPE_4x8:
        iMVs[0] = pCurDqLayer->pMv[0][iMBXY][iIIdx][0];
        iMVs[1] = pCurDqLayer->pMv[0][iMBXY][iIIdx][1];
        BaseMC (&pMCRefMem, iXOffset, iYOffset, pMCFunc, 4, 8, iMVs);

        iMVs[0] = pCurDqLayer->pMv[0][iMBXY][iIIdx + 1][0];
        iMVs[1] = pCurDqLayer->pMv[0][iMBXY][iIIdx + 1][1];
        pMCRefMem.pDstY += 4;
        pMCRefMem.pDstU += 2;
        pMCRefMem.pDstV += 2;
        BaseMC (&pMCRefMem, iXOffset + 4, iYOffset, pMCFunc, 4, 8, iMVs);
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

int32_t RecChroma (int32_t iMBXY, PWelsDecoderContext pCtx, int16_t* pScoeffLevel, PDqLayer pDqLayer) {
  int32_t iChromaStride = pCtx->pCurDqLayer->iCsStride[1];
  PIdctResAddPredFunc pIdctResAddPredFunc = pCtx->pIdctResAddPredFunc;

  uint8_t i = 0, j = 0;
  uint8_t uiCbpC = pDqLayer->pCbp[iMBXY] >> 4;

  if (1 == uiCbpC || 2 == uiCbpC) {
    WelsChromaDcIdct (pScoeffLevel + 256);	// 256 = 16*16
    WelsChromaDcIdct (pScoeffLevel + 320);	// 256 = 16*16
    for (i = 0; i < 2; i++) {
      int16_t* pRS = pScoeffLevel + 256 + (i << 6);
      uint8_t* pPred = pDqLayer->pPred[i + 1];
      int32_t* pBlockOffset = i == 0 ? &pCtx->iDecBlockOffsetArray[16] : &pCtx->iDecBlockOffsetArray[20];

      /*1 chroma is divided 4 4x4_block to idct*/
      for (j = 0; j < 4; j++) {
        int16_t* pRSI4x4 = &pRS[j << 4];
        uint8_t* pPredI4x4 = pPred + pBlockOffset[j];

        if (pDqLayer->pNzc[iMBXY][g_kuiMbNonZeroCountIdx[16 + (i << 2) + j]] || pRSI4x4[0]) {
          pIdctResAddPredFunc (pPredI4x4, iChromaStride, pRSI4x4);
        }
      }
    }
  }

  return ERR_NONE;
}

void_t FillBufForMc (uint8_t* pBuf, int32_t iBufStride, uint8_t* pSrc, int32_t iSrcStride, int32_t iSrcOffset,
                     int32_t iBlockWidth, int32_t iBlockHeight, int32_t iSrcX, int32_t iSrcY, int32_t iPicWidth, int32_t iPicHeight) {
  int32_t iY;
  int32_t iStartY, iStartX, iEndY, iEndX;
  int32_t iOffsetAdj = 0;
  int32_t iAddrSrc, iAddrBuf;
  int32_t iNum, iNum1;
  uint8_t* pBufSrc, *pBufDst;
  uint8_t* pBufSrc1, *pBufDst1;

  if (iSrcY >= iPicHeight) {
    iOffsetAdj += (iPicHeight - 1 - iSrcY) * iSrcStride;
    iSrcY = iPicHeight - 1;
  } else if (iSrcY <= -iBlockHeight) {
    iOffsetAdj += (1 - iBlockHeight - iSrcY) * iSrcStride;
    iSrcY = 1 - iBlockHeight;
  }
  if (iSrcX >= iPicWidth) {
    iOffsetAdj += (iPicWidth - 1 - iSrcX);
    iSrcX = iPicWidth - 1;
  } else if (iSrcX <= -iBlockWidth) {
    iOffsetAdj += (1 - iBlockWidth - iSrcX);
    iSrcX = 1 - iBlockWidth;
  }

  iOffsetAdj += iSrcOffset;

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) > (b) ? (b) : (a))

  iStartY = MAX (0, -iSrcY);
  iStartX = MAX (0, -iSrcX);
  iEndY = MIN (iBlockHeight, iPicHeight - iSrcY);
  iEndX = MIN (iBlockWidth, iPicWidth - iSrcX);

  // copy existing part
  iAddrSrc = iStartX + iStartY * iSrcStride;
  iAddrBuf = iStartX + iStartY * iBufStride;
  iNum = iEndX - iStartX;
  for (iY = iStartY; iY < iEndY; iY++) {
    memcpy (pBuf + iAddrBuf, pSrc + iOffsetAdj + iAddrSrc, iNum);
    iAddrSrc += iSrcStride;
    iAddrBuf += iBufStride;
  }

  //top
  pBufSrc = pBuf + iStartX + iStartY * iBufStride;
  pBufDst = pBuf + iStartX;
  iNum = iEndX - iStartX;
  for (iY = 0; iY < iStartY; iY++) {
    memcpy (pBufDst, pBufSrc, iNum);
    pBufDst += iBufStride;
  }

  //bottom
  pBufSrc = pBuf + iStartX + (iEndY - 1) * iBufStride;
  pBufDst = pBuf + iStartX + iEndY * iBufStride;
  iNum = iEndX - iStartX;
  for (iY = iEndY; iY < iBlockHeight; iY++) {
    memcpy (pBufDst, pBufSrc, iNum);
    pBufDst += iBufStride;
  }


  pBufSrc = pBuf + iStartX;
  pBufDst = pBuf;
  iNum = iStartX;

  pBufSrc1 = pBuf + iEndX - 1;
  pBufDst1 = pBuf + iEndX;
  iNum1 = iBlockWidth - iEndX;
  for (iY = 0; iY < iBlockHeight; iY++) {
    //left
    memset (pBufDst, pBufSrc[0], iNum);
    pBufDst += iBufStride;
    pBufSrc += iBufStride;

    //right
    memset (pBufDst1, pBufSrc1[0], iNum1);
    pBufDst1 += iBufStride;
    pBufSrc1 += iBufStride;
  }
}

} // namespace WelsDec
