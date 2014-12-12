/*!
 * \copy
 *     Copyright (c)  2013, Cisco Systems
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
 *	cabac_decoder.cpp:	cabac parse for syntax elements
 */
#include "parse_mb_syn_cabac.h"
#include "mv_pred.h"
#include "error_code.h"
namespace WelsDec {
#define IDX_UNUSED -1
static const int16_t g_kMaxPos       [] = {IDX_UNUSED, 15, 14, 15, 3, 14, 3, 3, 14, 14};
static const int16_t g_kMaxC2       [] = {IDX_UNUSED, 4, 4, 4, 3, 4, 3, 3, 4, 4};
static const int16_t g_kBlockCat2CtxOffsetCBF[] = {IDX_UNUSED, 0, 4, 8, 12, 16, 12, 12, 16, 16};
static const int16_t g_kBlockCat2CtxOffsetMap [] = {IDX_UNUSED, 0, 15, 29, 44, 47, 44, 44, 47, 47};
static const int16_t g_kBlockCat2CtxOffsetLast[] = {IDX_UNUSED, 0, 15, 29, 44, 47, 44, 44, 47, 47};
static const int16_t g_kBlockCat2CtxOffsetOne [] = {IDX_UNUSED, 0 , 10, 20, 30, 39, 30, 30, 39, 39};
static const int16_t g_kBlockCat2CtxOffsetAbs [] = {IDX_UNUSED, 0 , 10, 20, 30, 39, 30, 30, 39, 39};

const uint8_t g_kTopBlkInsideMb[24] = { //for index with z-order 0~23
  //  0   1 | 4  5      luma 8*8 block           pNonZeroCount[16+8]
  0,  0,  1,  1,   //  2   3 | 6  7        0  |  1                  0   1   2   3
  0,  0,  1,  1,   //---------------      ---------                 4   5   6   7
  1,  1,  1,  1,   //  8   9 | 12 13       2  |  3                  8   9  10  11
  1,  1,  1,  1,  // 10  11 | 14 15-----------------------------> 12  13  14  15
  0,  0,  1,  1,   //----------------    chroma 8*8 block          16  17  18  19
  0,  0,  1,  1   // 16  17 | 20 21        0    1                 20  21  22  23
  // 18  19 | 22 23
};

const uint8_t g_kLeftBlkInsideMb[24] = { //for index with z-order 0~23
  //  0   1 | 4  5      luma 8*8 block           pNonZeroCount[16+8]
  0,  1,  0,  1,   //  2   3 | 6  7        0  |  1                  0   1   2   3
  1,  1,  1,  1,   //---------------      ---------                 4   5   6   7
  0,  1,  0,  1,   //  8   9 | 12 13       2  |  3                  8   9  10  11
  1,  1,  1,  1,  // 10  11 | 14 15-----------------------------> 12  13  14  15
  0,  1,  0,  1,   //----------------    chroma 8*8 block          16  17  18  19
  0,  1,  0,  1   // 16  17 | 20 21        0    1                 20  21  22  23
  // 18  19 | 22 23
};

void UpdateP16x8RefIdxCabac (PDqLayer pCurDqLayer, int8_t pRefIndex[LIST_A][30], int32_t iPartIdx, const int8_t iRef,
                             const int8_t iListIdx) {
  int32_t iRef32Bit = (int32_t) iRef;
  const int32_t iRef4Bytes = (iRef32Bit << 24) | (iRef32Bit << 16) | (iRef32Bit << 8) | iRef32Bit;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  const uint8_t iScan4Idx = g_kuiScan4[iPartIdx];
  const uint8_t iScan4Idx4 = 4 + iScan4Idx;
  const uint8_t iCacheIdx = g_kuiCache30ScanIdx[iPartIdx];
  const uint8_t iCacheIdx6 = 6 + iCacheIdx;
  //mb
  ST32 (&pCurDqLayer->pRefIndex[iListIdx][iMbXy][iScan4Idx ], iRef4Bytes);
  ST32 (&pCurDqLayer->pRefIndex[iListIdx][iMbXy][iScan4Idx4], iRef4Bytes);
  //cache
  ST32 (&pRefIndex[iListIdx][iCacheIdx ], iRef4Bytes);
  ST32 (&pRefIndex[iListIdx][iCacheIdx6], iRef4Bytes);
}

void UpdateP8x16RefIdxCabac (PDqLayer pCurDqLayer, int8_t pRefIndex[LIST_A][30], int32_t iPartIdx, const int8_t iRef,
                             const int8_t iListIdx) {
  int16_t iRef16Bit = (int16_t) iRef;
  const int16_t iRef2Bytes = (iRef16Bit << 8) | iRef16Bit;
  int32_t i;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  for (i = 0; i < 2; i++, iPartIdx += 8) {
    const uint8_t iScan4Idx = g_kuiScan4[iPartIdx];
    const uint8_t iCacheIdx = g_kuiCache30ScanIdx[iPartIdx];
    const uint8_t iScan4Idx4 = 4 + iScan4Idx;
    const uint8_t iCacheIdx6 = 6 + iCacheIdx;
    //mb
    ST16 (&pCurDqLayer->pRefIndex[iListIdx][iMbXy][iScan4Idx ], iRef2Bytes);
    ST16 (&pCurDqLayer->pRefIndex[iListIdx][iMbXy][iScan4Idx4], iRef2Bytes);
    //cache
    ST16 (&pRefIndex[iListIdx][iCacheIdx ], iRef2Bytes);
    ST16 (&pRefIndex[iListIdx][iCacheIdx6], iRef2Bytes);
  }
}

void UpdateP8x8RefIdxCabac (PDqLayer pCurDqLayer, int8_t pRefIndex[LIST_A][30], int32_t iPartIdx, const int8_t iRef,
                            const int8_t iListIdx) {
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  const uint8_t iScan4Idx = g_kuiScan4[iPartIdx];
  pCurDqLayer->pRefIndex[iListIdx][iMbXy][iScan4Idx] = pCurDqLayer->pRefIndex[iListIdx][iMbXy][iScan4Idx + 1] =
        pCurDqLayer->pRefIndex[iListIdx][iMbXy][iScan4Idx + 4] = pCurDqLayer->pRefIndex[iListIdx][iMbXy][iScan4Idx + 5] = iRef;
}

void UpdateP16x16MvdCabac (SDqLayer* pCurDqLayer, int16_t pMvd[2], const int8_t iListIdx) {
  int32_t pMvd32[2];
  ST32 (&pMvd32[0], LD32 (pMvd));
  ST32 (&pMvd32[1], LD32 (pMvd));
  int32_t i;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  for (i = 0; i < 16; i += 2) {
    ST64 (pCurDqLayer->pMvd[iListIdx][iMbXy][i], LD64 (pMvd32));
  }
}

void UpdateP16x8MvdCabac (SDqLayer* pCurDqLayer, int16_t pMvdCache[LIST_A][30][MV_A], int32_t iPartIdx, int16_t pMvd[2],
                          const int8_t iListIdx) {
  int32_t pMvd32[2];
  ST32 (&pMvd32[0], LD32 (pMvd));
  ST32 (&pMvd32[1], LD32 (pMvd));
  int32_t i;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  for (i = 0; i < 2; i++, iPartIdx += 4) {
    const uint8_t iScan4Idx = g_kuiScan4[iPartIdx];
    const uint8_t iScan4Idx4 = 4 + iScan4Idx;
    const uint8_t iCacheIdx = g_kuiCache30ScanIdx[iPartIdx];
    const uint8_t iCacheIdx6 = 6 + iCacheIdx;
    //mb
    ST64 (pCurDqLayer->pMvd[iListIdx][iMbXy][  iScan4Idx ], LD64 (pMvd32));
    ST64 (pCurDqLayer->pMvd[iListIdx][iMbXy][  iScan4Idx4], LD64 (pMvd32));
    //cache
    ST64 (pMvdCache[iListIdx][  iCacheIdx ], LD64 (pMvd32));
    ST64 (pMvdCache[iListIdx][  iCacheIdx6], LD64 (pMvd32));
  }
}

void UpdateP8x16MvdCabac (SDqLayer* pCurDqLayer, int16_t pMvdCache[LIST_A][30][MV_A], int32_t iPartIdx, int16_t pMvd[2],
                          const int8_t iListIdx) {
  int32_t pMvd32[2];
  ST32 (&pMvd32[0], LD32 (pMvd));
  ST32 (&pMvd32[1], LD32 (pMvd));
  int32_t i;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;

  for (i = 0; i < 2; i++, iPartIdx += 8) {
    const uint8_t iScan4Idx = g_kuiScan4[iPartIdx];
    const uint8_t iCacheIdx = g_kuiCache30ScanIdx[iPartIdx];
    const uint8_t iScan4Idx4 = 4 + iScan4Idx;
    const uint8_t iCacheIdx6 = 6 + iCacheIdx;
    //mb
    ST64 (pCurDqLayer->pMvd[iListIdx][iMbXy][  iScan4Idx ], LD64 (pMvd32));
    ST64 (pCurDqLayer->pMvd[iListIdx][iMbXy][  iScan4Idx4], LD64 (pMvd32));
    //cache
    ST64 (pMvdCache[iListIdx][  iCacheIdx ], LD64 (pMvd32));
    ST64 (pMvdCache[iListIdx][  iCacheIdx6], LD64 (pMvd32));
  }
}

int32_t ParseEndOfSliceCabac (PWelsDecoderContext pCtx, uint32_t& uiBinVal) {
  uiBinVal = 0;
  WELS_READ_VERIFY (DecodeTerminateCabac (pCtx->pCabacDecEngine, uiBinVal));
  return ERR_NONE;
}

int32_t ParseSkipFlagCabac (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, uint32_t& uiSkip) {
  uiSkip = 0;
  int32_t iCtxInc = (pNeighAvail->iLeftAvail && pNeighAvail->iLeftType != MB_TYPE_SKIP) + (pNeighAvail->iTopAvail
                    && pNeighAvail->iTopType  != MB_TYPE_SKIP);
  PWelsCabacCtx pBinCtx = (pCtx->pCabacCtx + NEW_CTX_OFFSET_SKIP + iCtxInc);
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pBinCtx, uiSkip));
  return ERR_NONE;
}


int32_t ParseMBTypeISliceCabac (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, uint32_t& uiBinVal) {
  uint32_t uiCode;
  int32_t iIdxA = 0, iIdxB = 0;
  int32_t iCtxInc;
  uiBinVal = 0;
  PWelsCabacDecEngine pCabacDecEngine = pCtx->pCabacDecEngine;
  PWelsCabacCtx pBinCtx = pCtx->pCabacCtx + NEW_CTX_OFFSET_MB_TYPE_I; //I mode in I slice
  iIdxA = (pNeighAvail->iLeftAvail) && (pNeighAvail->iLeftType != MB_TYPE_INTRA4x4
                                        && pNeighAvail->iLeftType != MB_TYPE_INTRA8x8);
  iIdxB = (pNeighAvail->iTopAvail) && (pNeighAvail->iTopType != MB_TYPE_INTRA4x4
                                       && pNeighAvail->iTopType != MB_TYPE_INTRA8x8);
  iCtxInc = iIdxA + iIdxB;
  WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + iCtxInc, uiCode));
  uiBinVal = uiCode;
  if (uiBinVal != 0) {  //I16x16
    WELS_READ_VERIFY (DecodeTerminateCabac (pCabacDecEngine, uiCode));
    if (uiCode == 1)
      uiBinVal = 25; //I_PCM
    else {
      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 3, uiCode));
      uiBinVal = 1 + uiCode * 12;
      //decoding of uiCbp:0,1,2
      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 4, uiCode));
      if (uiCode != 0) {
        WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 5, uiCode));
        uiBinVal += 4;
        if (uiCode != 0)
          uiBinVal += 4;
      }
      //decoding of I pred-mode: 0,1,2,3
      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 6, uiCode));
      uiBinVal += (uiCode << 1);
      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 7, uiCode));
      uiBinVal += uiCode;
    }
  }
  //I4x4
  return ERR_NONE;
}

int32_t ParseMBTypePSliceCabac (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, uint32_t& uiMbType) {
  uint32_t uiCode;
  uiMbType = 0;
  PWelsCabacDecEngine pCabacDecEngine = pCtx->pCabacDecEngine;

  PWelsCabacCtx pBinCtx = pCtx->pCabacCtx + NEW_CTX_OFFSET_SKIP;
  WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 3, uiCode));
  if (uiCode) {
    // Intra MB
    WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 6, uiCode));
    if (uiCode) { // Intra 16x16
      WELS_READ_VERIFY (DecodeTerminateCabac (pCabacDecEngine, uiCode));
      if (uiCode) {
        uiMbType = 30;
        return ERR_NONE;//MB_TYPE_INTRA_PCM;
      }

      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 7, uiCode));
      uiMbType = 6 + uiCode * 12;

      //uiCbp: 0,1,2
      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 8, uiCode));
      if (uiCode) {
        uiMbType += 4;
        WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 8, uiCode));
        if (uiCode)
          uiMbType += 4;
      }

      //IPredMode: 0,1,2,3
      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 9, uiCode));
      uiMbType += (uiCode << 1);
      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 9, uiCode));
      uiMbType += uiCode;
    } else
      // Intra 4x4
      uiMbType = 5;
  } else { // P MB
    WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 4, uiCode));
    if (uiCode) { //second bit
      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 6, uiCode));
      if (uiCode)
        uiMbType = 1;
      else
        uiMbType = 2;
    } else {
      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 5, uiCode));
      if (uiCode)
        uiMbType = 3;
      else
        uiMbType = 0;
    }
  }
  return ERR_NONE;
}
int32_t ParseSubMBTypeCabac (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, uint32_t& uiSubMbType) {
  uint32_t uiCode;
  PWelsCabacDecEngine pCabacDecEngine = pCtx->pCabacDecEngine;
  PWelsCabacCtx pBinCtx = pCtx->pCabacCtx + NEW_CTX_OFFSET_SUBMB_TYPE;
  WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx, uiCode));
  if (uiCode)
    uiSubMbType = 0;
  else {
    WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 1, uiCode));
    if (uiCode) {
      WELS_READ_VERIFY (DecodeBinCabac (pCabacDecEngine, pBinCtx + 2, uiCode));
      uiSubMbType = 3 - uiCode;
    } else {
      uiSubMbType = 1;
    }
  }
  return ERR_NONE;
}

int32_t ParseIntraPredModeLumaCabac (PWelsDecoderContext pCtx, int32_t& iBinVal) {
  uint32_t uiCode;
  iBinVal = 0;
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_IPR, uiCode));
  if (uiCode == 1)
    iBinVal = -1;
  else {
    WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_IPR + 1, uiCode));
    iBinVal |= uiCode;
    WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_IPR + 1, uiCode));
    iBinVal |= (uiCode << 1);
    WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_IPR + 1, uiCode));
    iBinVal |= (uiCode << 2);
  }
  return ERR_NONE;
}

int32_t ParseIntraPredModeChromaCabac (PWelsDecoderContext pCtx, uint8_t uiNeighAvail, int32_t& iBinVal) {
  uint32_t uiCode;
  int32_t iIdxA, iIdxB, iCtxInc;
  int8_t* pChromaPredMode = pCtx->pCurDqLayer->pChromaPredMode;
  int8_t* pMbType = pCtx->pCurDqLayer->pMbType;
  int32_t iLeftAvail     = uiNeighAvail & 0x04;
  int32_t iTopAvail      = uiNeighAvail & 0x01;

  int32_t iMbXy = pCtx->pCurDqLayer->iMbXyIndex;
  int32_t iMbXyTop = iMbXy - pCtx->pCurDqLayer->iMbWidth;
  int32_t iMbXyLeft = iMbXy - 1;

  iBinVal = 0;

  iIdxB = iTopAvail  && (pChromaPredMode[iMbXyTop] > 0 && pChromaPredMode[iMbXyTop] <= 3)
          && pMbType[iMbXyTop]  != MB_TYPE_INTRA_PCM;
  iIdxA = iLeftAvail && (pChromaPredMode[iMbXyLeft] > 0 && pChromaPredMode[iMbXyLeft] <= 3)
          && pMbType[iMbXyLeft] != MB_TYPE_INTRA_PCM;
  iCtxInc = iIdxA + iIdxB;
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_CIPR + iCtxInc, uiCode));
  iBinVal = uiCode;
  if (iBinVal != 0) {
    uint32_t iSym;
    WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_CIPR + 3, iSym));
    if (iSym == 0) {
      iBinVal = (iSym + 1);
      return ERR_NONE;
    }
    iSym = 0;
    do {
      WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_CIPR + 3, uiCode));
      ++iSym;
    } while ((uiCode != 0) && (iSym < 1));

    if ((uiCode != 0) && (iSym == 1))
      ++ iSym;
    iBinVal = (iSym + 1);
    return ERR_NONE;
  }
  return ERR_NONE;
}

int32_t ParseInterMotionInfoCabac (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, uint8_t* pNonZeroCount,
                                   int16_t pMotionVector[LIST_A][30][MV_A], int16_t pMvdCache[LIST_A][30][MV_A], int8_t pRefIndex[LIST_A][30]) {
  PSlice pSlice				= &pCtx->pCurDqLayer->sLayerInfo.sSliceInLayer;
  PSliceHeader pSliceHeader	= &pSlice->sSliceHeaderExt.sSliceHeader;
  PDqLayer pCurDqLayer = pCtx->pCurDqLayer;
  PPicture* ppRefPic = pCtx->sRefPic.pRefList[LIST_0];
  int32_t pRefCount[2];
  int32_t i, j;
  int32_t iMbXy = pCurDqLayer->iMbXyIndex;
  int16_t pMv[4] = {0};
  int16_t pMvd[4] = {0};
  int8_t iRef[2] = {0};
  int32_t iPartIdx;
  int16_t iMinVmv = pSliceHeader->pSps->pSLevelLimits->iMinVmv;
  int16_t iMaxVmv = pSliceHeader->pSps->pSLevelLimits->iMaxVmv;
  pRefCount[0] = pSliceHeader->uiRefCount[0];
  pRefCount[1] = pSliceHeader->uiRefCount[1];

  switch (pCurDqLayer->pMbType[iMbXy]) {
  case MB_TYPE_16x16: {
    iPartIdx = 0;
    WELS_READ_VERIFY (ParseRefIdxCabac (pCtx, pNeighAvail, pNonZeroCount, pRefIndex, LIST_0, iPartIdx, pRefCount[0], 0,
                                        iRef[0]));
    if ((iRef[0] < 0) || (iRef[0] >= pRefCount[0]) || (ppRefPic[iRef[0]] == NULL)) { //error ref_idx
      pCtx->bMbRefConcealed = true;
      if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
        iRef[0] = 0;
        pCtx->iErrorCode |= dsBitstreamError;
      } else {
        return ERR_INFO_INVALID_REF_INDEX;
      }
    }
    pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || !(ppRefPic[iRef[0]]&&ppRefPic[iRef[0]]->bIsComplete);
    PredMv (pMotionVector, pRefIndex, 0, 4, iRef[0], pMv);
    WELS_READ_VERIFY (ParseMvdInfoCabac (pCtx, pNeighAvail, pRefIndex, pMvdCache, iPartIdx, LIST_0, 0, pMvd[0]));
    WELS_READ_VERIFY (ParseMvdInfoCabac (pCtx, pNeighAvail, pRefIndex, pMvdCache, iPartIdx, LIST_0, 1, pMvd[1]));
    pMv[0] += pMvd[0];
    pMv[1] += pMvd[1];
    WELS_CHECK_SE_BOTH_WARNING (pMv[1], iMinVmv, iMaxVmv, "vertical mv");
    UpdateP16x16MotionInfo (pCurDqLayer, iRef[0], pMv);
    UpdateP16x16MvdCabac (pCurDqLayer, pMvd, LIST_0);
  }
  break;
  case MB_TYPE_16x8:
    for (i = 0; i < 2; i++) {
      iPartIdx = i << 3;
      WELS_READ_VERIFY (ParseRefIdxCabac (pCtx, pNeighAvail, pNonZeroCount, pRefIndex, LIST_0, iPartIdx, pRefCount[0], 0,
                                          iRef[i]));
      if ((iRef[i] < 0) || (iRef[i] >= pRefCount[0]) || (ppRefPic[iRef[i]] == NULL)) { //error ref_idx
        pCtx->bMbRefConcealed = true;
        if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
          iRef[i] = 0;
          pCtx->iErrorCode |= dsBitstreamError;
        } else {
          return ERR_INFO_INVALID_REF_INDEX;
        }
      }
      pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || !(ppRefPic[iRef[i]]&&ppRefPic[iRef[i]]->bIsComplete);
      UpdateP16x8RefIdxCabac (pCurDqLayer, pRefIndex, iPartIdx, iRef[i], LIST_0);
    }
    for (i = 0; i < 2; i++) {
      iPartIdx = i << 3;
      PredInter16x8Mv (pMotionVector, pRefIndex, iPartIdx, iRef[i], pMv);
      WELS_READ_VERIFY (ParseMvdInfoCabac (pCtx, pNeighAvail, pRefIndex, pMvdCache, iPartIdx, LIST_0, 0, pMvd[0]));
      WELS_READ_VERIFY (ParseMvdInfoCabac (pCtx, pNeighAvail, pRefIndex, pMvdCache, iPartIdx, LIST_0, 1, pMvd[1]));
      pMv[0] += pMvd[0];
      pMv[1] += pMvd[1];
      WELS_CHECK_SE_BOTH_WARNING (pMv[1], iMinVmv, iMaxVmv, "vertical mv");
      UpdateP16x8MotionInfo (pCurDqLayer, pMotionVector, pRefIndex, iPartIdx, iRef[i], pMv);
      UpdateP16x8MvdCabac (pCurDqLayer, pMvdCache, iPartIdx, pMvd, LIST_0);
    }
    break;
  case MB_TYPE_8x16:
    for (i = 0; i < 2; i++) {
      iPartIdx = i << 2;
      WELS_READ_VERIFY (ParseRefIdxCabac (pCtx, pNeighAvail, pNonZeroCount, pRefIndex, LIST_0, iPartIdx, pRefCount[0], 0,
                                          iRef[i]));
      if ((iRef[i] < 0) || (iRef[i] >= pRefCount[0]) || (ppRefPic[iRef[i]] == NULL)) { //error ref_idx
        pCtx->bMbRefConcealed = true;
        if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
          iRef[i] = 0;
          pCtx->iErrorCode |= dsBitstreamError;
        } else {
          return ERR_INFO_INVALID_REF_INDEX;
        }
      }
      pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || !(ppRefPic[iRef[i]]&&ppRefPic[iRef[i]]->bIsComplete);
      UpdateP8x16RefIdxCabac (pCurDqLayer, pRefIndex, iPartIdx, iRef[i], LIST_0);
    }
    for (i = 0; i < 2; i++) {
      iPartIdx = i << 2;
      PredInter8x16Mv (pMotionVector, pRefIndex, i << 2, iRef[i], pMv/*&mv[0], &mv[1]*/);

      WELS_READ_VERIFY (ParseMvdInfoCabac (pCtx, pNeighAvail, pRefIndex, pMvdCache, iPartIdx, LIST_0, 0, pMvd[0]));
      WELS_READ_VERIFY (ParseMvdInfoCabac (pCtx, pNeighAvail, pRefIndex, pMvdCache, iPartIdx, LIST_0, 1, pMvd[1]));
      pMv[0] += pMvd[0];
      pMv[1] += pMvd[1];
      WELS_CHECK_SE_BOTH_WARNING (pMv[1], iMinVmv, iMaxVmv, "vertical mv");
      UpdateP8x16MotionInfo (pCurDqLayer, pMotionVector, pRefIndex, iPartIdx, iRef[i], pMv);
      UpdateP8x16MvdCabac (pCurDqLayer, pMvdCache, iPartIdx, pMvd, LIST_0);
    }
    break;
  case MB_TYPE_8x8:
  case MB_TYPE_8x8_REF0: {
    int8_t pRefIdx[4] = {0}, pSubPartCount[4], pPartW[4];
    uint32_t uiSubMbType;
    //sub_mb_type, partition
    for (i = 0; i < 4; i++) {
      WELS_READ_VERIFY (ParseSubMBTypeCabac (pCtx, pNeighAvail, uiSubMbType));
      if (uiSubMbType >= 4) { //invalid sub_mb_type
        return ERR_INFO_INVALID_SUB_MB_TYPE;
      }
      pCurDqLayer->pSubMbType[iMbXy][i] = g_ksInterSubMbTypeInfo[uiSubMbType].iType;
      pSubPartCount[i] = g_ksInterSubMbTypeInfo[uiSubMbType].iPartCount;
      pPartW[i] = g_ksInterSubMbTypeInfo[uiSubMbType].iPartWidth;
    }

    for (i = 0; i < 4; i++) {
      int16_t iIdx8 = i << 2;
      WELS_READ_VERIFY (ParseRefIdxCabac (pCtx, pNeighAvail, pNonZeroCount, pRefIndex, LIST_0, iIdx8, pRefCount[0], 1,
                                          pRefIdx[i]));
      if ((pRefIdx[i] < 0) || (pRefIdx[i] >= pRefCount[0]) || (ppRefPic[pRefIdx[i]] == NULL)) { //error ref_idx
        pCtx->bMbRefConcealed = true;
        if (pCtx->eErrorConMethod != ERROR_CON_DISABLE) {
          pRefIdx[i] = 0;
          pCtx->iErrorCode |= dsBitstreamError;
        } else {
          return ERR_INFO_INVALID_REF_INDEX;
        }
      }
      pCtx->bMbRefConcealed = pCtx->bRPLRError || pCtx->bMbRefConcealed || !(ppRefPic[pRefIdx[i]]&&ppRefPic[pRefIdx[i]]->bIsComplete);
      UpdateP8x8RefIdxCabac (pCurDqLayer, pRefIndex, iIdx8, pRefIdx[i], LIST_0);
    }
    //mv
    for (i = 0; i < 4; i++) {
      int8_t iPartCount = pSubPartCount[i];
      uiSubMbType = pCurDqLayer->pSubMbType[iMbXy][i];
      int16_t iPartIdx, iBlockW = pPartW[i];
      uint8_t iScan4Idx, iCacheIdx;
      iCacheIdx = g_kuiCache30ScanIdx[i << 2];
      pRefIndex[0][iCacheIdx ] = pRefIndex[0][iCacheIdx + 1]
                                 = pRefIndex[0][iCacheIdx + 6] = pRefIndex[0][iCacheIdx + 7] = pRefIdx[i];

      for (j = 0; j < iPartCount; j++) {
        iPartIdx = (i << 2) + j * iBlockW;
        iScan4Idx = g_kuiScan4[iPartIdx];
        iCacheIdx = g_kuiCache30ScanIdx[iPartIdx];
        PredMv (pMotionVector, pRefIndex, iPartIdx, iBlockW, pRefIdx[i], pMv);
        WELS_READ_VERIFY (ParseMvdInfoCabac (pCtx, pNeighAvail, pRefIndex, pMvdCache, iPartIdx, LIST_0, 0, pMvd[0]));
        WELS_READ_VERIFY (ParseMvdInfoCabac (pCtx, pNeighAvail, pRefIndex, pMvdCache, iPartIdx, LIST_0, 1, pMvd[1]));
        pMv[0] += pMvd[0];
        pMv[1] += pMvd[1];
        WELS_CHECK_SE_BOTH_WARNING (pMv[1], iMinVmv, iMaxVmv, "vertical mv");
        if (SUB_MB_TYPE_8x8 == uiSubMbType) {
          ST32 ((pMv + 2), LD32 (pMv));
          ST32 ((pMvd + 2), LD32 (pMvd));
          ST64 (pCurDqLayer->pMv[0][iMbXy][iScan4Idx], LD64 (pMv));
          ST64 (pCurDqLayer->pMv[0][iMbXy][iScan4Idx + 4], LD64 (pMv));
          ST64 (pCurDqLayer->pMvd[0][iMbXy][iScan4Idx], LD64 (pMvd));
          ST64 (pCurDqLayer->pMvd[0][iMbXy][iScan4Idx + 4], LD64 (pMvd));
          ST64 (pMotionVector[0][iCacheIdx  ], LD64 (pMv));
          ST64 (pMotionVector[0][iCacheIdx + 6], LD64 (pMv));
          ST64 (pMvdCache[0][iCacheIdx  ], LD64 (pMvd));
          ST64 (pMvdCache[0][iCacheIdx + 6], LD64 (pMvd));
        } else if (SUB_MB_TYPE_8x4 == uiSubMbType) {
          ST32 ((pMv + 2), LD32 (pMv));
          ST32 ((pMvd + 2), LD32 (pMvd));
          ST64 (pCurDqLayer->pMv[0][iMbXy][iScan4Idx  ], LD64 (pMv));
          ST64 (pCurDqLayer->pMvd[0][iMbXy][iScan4Idx  ], LD64 (pMvd));
          ST64 (pMotionVector[0][iCacheIdx  ], LD64 (pMv));
          ST64 (pMvdCache[0][iCacheIdx  ], LD64 (pMvd));
        } else if (SUB_MB_TYPE_4x8 == uiSubMbType) {
          ST32 (pCurDqLayer->pMv[0][iMbXy][iScan4Idx  ], LD32 (pMv));
          ST32 (pCurDqLayer->pMv[0][iMbXy][iScan4Idx + 4], LD32 (pMv));
          ST32 (pCurDqLayer->pMvd[0][iMbXy][iScan4Idx  ], LD32 (pMvd));
          ST32 (pCurDqLayer->pMvd[0][iMbXy][iScan4Idx + 4], LD32 (pMvd));
          ST32 (pMotionVector[0][iCacheIdx  ], LD32 (pMv));
          ST32 (pMotionVector[0][iCacheIdx + 6], LD32 (pMv));
          ST32 (pMvdCache[0][iCacheIdx  ], LD32 (pMvd));
          ST32 (pMvdCache[0][iCacheIdx + 6], LD32 (pMvd));
        } else {  //SUB_MB_TYPE_4x4
          ST32 (pCurDqLayer->pMv[0][iMbXy][iScan4Idx  ], LD32 (pMv));
          ST32 (pCurDqLayer->pMvd[0][iMbXy][iScan4Idx  ], LD32 (pMvd));
          ST32 (pMotionVector[0][iCacheIdx  ], LD32 (pMv));
          ST32 (pMvdCache[0][iCacheIdx  ], LD32 (pMvd));
        }
      }
    }
  }
  break;
  default:
    break;
  }
  return ERR_NONE;
}

int32_t ParseRefIdxCabac (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, uint8_t* nzc,
                          int8_t ref_idx[LIST_A][30],
                          int32_t iListIdx, int32_t iZOrderIdx, int32_t iActiveRefNum, int32_t b8mode, int8_t& iRefIdxVal) {
  if (iActiveRefNum == 1) {
    iRefIdxVal = 0;
    return ERR_NONE;
  }
  uint32_t uiCode;
  int32_t iIdxA = 0, iIdxB = 0;
  int32_t iCtxInc;
  int8_t* pRefIdxInMB = pCtx->pCurDqLayer->pRefIndex[LIST_0][pCtx->pCurDqLayer->iMbXyIndex];
  if (iZOrderIdx == 0) {
    iIdxB = (pNeighAvail->iTopAvail && pNeighAvail->iTopType != MB_TYPE_INTRA_PCM
             && ref_idx[iListIdx][g_kuiCache30ScanIdx[iZOrderIdx] - 6] > 0);
    iIdxA = (pNeighAvail->iLeftAvail && pNeighAvail->iLeftType != MB_TYPE_INTRA_PCM
             && ref_idx[iListIdx][g_kuiCache30ScanIdx[iZOrderIdx] - 1] > 0);
  } else if (iZOrderIdx == 4) {
    iIdxB = (pNeighAvail->iTopAvail && pNeighAvail->iTopType != MB_TYPE_INTRA_PCM
             && ref_idx[iListIdx][g_kuiCache30ScanIdx[iZOrderIdx] - 6] > 0);
    iIdxA = pRefIdxInMB[g_kuiScan4[iZOrderIdx] - 1] > 0;
  } else if (iZOrderIdx == 8) {
    iIdxB = pRefIdxInMB[g_kuiScan4[iZOrderIdx] - 4] > 0;
    iIdxA = (pNeighAvail->iLeftAvail && pNeighAvail->iLeftType != MB_TYPE_INTRA_PCM
             && ref_idx[iListIdx][g_kuiCache30ScanIdx[iZOrderIdx] - 1] > 0);
  } else {
    iIdxB = pRefIdxInMB[g_kuiScan4[iZOrderIdx] - 4] > 0;
    iIdxA = pRefIdxInMB[g_kuiScan4[iZOrderIdx] - 1] > 0;
  }

  iCtxInc = iIdxA + (iIdxB << 1);
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_REF_NO + iCtxInc, uiCode));
  if (uiCode) {
    WELS_READ_VERIFY (DecodeUnaryBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_REF_NO + 4, 1, uiCode));
    ++uiCode;
  }
  iRefIdxVal = (int8_t) uiCode;
  return ERR_NONE;
}

int32_t ParseMvdInfoCabac (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, int8_t pRefIndex[LIST_A][30],
                           int16_t pMvdCache[LIST_A][30][2], int32_t index, int8_t iListIdx, int8_t iMvComp, int16_t& iMvdVal) {
  uint32_t uiCode;
  int32_t iIdxA = 0;
  //int32_t sym;
  int32_t iCtxInc;
  PWelsCabacCtx pBinCtx = pCtx->pCabacCtx + NEW_CTX_OFFSET_MVD + iMvComp * CTX_NUM_MVD;
  iMvdVal = 0;
  if (pRefIndex[iListIdx][g_kuiCache30ScanIdx[index] - 6] >= 0)
    iIdxA = WELS_ABS (pMvdCache[iListIdx][g_kuiCache30ScanIdx[index] - 6][iMvComp]);
  if (pRefIndex[iListIdx][g_kuiCache30ScanIdx[index] - 1] >= 0)
    iIdxA += WELS_ABS (pMvdCache[iListIdx][g_kuiCache30ScanIdx[index] - 1][iMvComp]);

  if (iIdxA < 3)
    iCtxInc = 0;
  else if (iIdxA > 32)
    iCtxInc = 2;
  else
    iCtxInc = 1;
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine,  pBinCtx + iCtxInc, uiCode));
  if (uiCode) {
    WELS_READ_VERIFY (DecodeUEGMvCabac (pCtx->pCabacDecEngine, pBinCtx + 3, 3, uiCode));
    iMvdVal = (int16_t) (uiCode + 1);
    WELS_READ_VERIFY (DecodeBypassCabac (pCtx->pCabacDecEngine, uiCode));
    if (uiCode) {
      iMvdVal = -iMvdVal;
    }
  } else {
    iMvdVal = 0;
  }
  return ERR_NONE;
}

int32_t ParseCbpInfoCabac (PWelsDecoderContext pCtx, PWelsNeighAvail pNeighAvail, uint32_t& uiCbp) {
  int32_t iIdxA = 0, iIdxB = 0, pALeftMb[2], pBTopMb[2];
  uiCbp = 0;
  uint32_t pCbpBit[6];
  int32_t iCtxInc;

  //Luma: bit by bit for 4 8x8 blocks in z-order
  pBTopMb[0]  = pNeighAvail->iTopAvail  && pNeighAvail->iTopType  != MB_TYPE_INTRA_PCM
                && ((pNeighAvail->iTopCbp  & (1 << 2)) == 0);
  pBTopMb[1]  = pNeighAvail->iTopAvail  && pNeighAvail->iTopType  != MB_TYPE_INTRA_PCM
                && ((pNeighAvail->iTopCbp  & (1 << 3)) == 0);
  pALeftMb[0] = pNeighAvail->iLeftAvail && pNeighAvail->iLeftType != MB_TYPE_INTRA_PCM
                && ((pNeighAvail->iLeftCbp & (1 << 1)) == 0);
  pALeftMb[1] = pNeighAvail->iLeftAvail && pNeighAvail->iLeftType != MB_TYPE_INTRA_PCM
                && ((pNeighAvail->iLeftCbp & (1 << 3)) == 0);

  //left_top 8x8 block
  iCtxInc = pALeftMb[0] + (pBTopMb[0] << 1);
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_CBP + iCtxInc, pCbpBit[0]));
  if (pCbpBit[0])
    uiCbp += 0x01;

  //right_top 8x8 block
  iIdxA = !pCbpBit[0];
  iCtxInc = iIdxA + (pBTopMb[1] << 1);
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_CBP + iCtxInc, pCbpBit[1]));
  if (pCbpBit[1])
    uiCbp += 0x02;

  //left_bottom 8x8 block
  iIdxB = !pCbpBit[0];
  iCtxInc = pALeftMb[1] + (iIdxB << 1);
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_CBP + iCtxInc, pCbpBit[2]));
  if (pCbpBit[2])
    uiCbp += 0x04;

  //right_bottom 8x8 block
  iIdxB = !pCbpBit[1];
  iIdxA = !pCbpBit[2];
  iCtxInc = iIdxA + (iIdxB << 1);
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_CBP + iCtxInc, pCbpBit[3]));
  if (pCbpBit[3])
    uiCbp += 0x08;

  //Chroma: bit by bit
  iIdxB = pNeighAvail->iTopAvail  && (pNeighAvail->iTopType  == MB_TYPE_INTRA_PCM || (pNeighAvail->iTopCbp  >> 4));
  iIdxA = pNeighAvail->iLeftAvail && (pNeighAvail->iLeftType == MB_TYPE_INTRA_PCM || (pNeighAvail->iLeftCbp >> 4));

  //BitIdx = 0
  iCtxInc = iIdxA + (iIdxB << 1);
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pCtx->pCabacCtx + NEW_CTX_OFFSET_CBP + CTX_NUM_CBP + iCtxInc,
                                    pCbpBit[4]));

  //BitIdx = 1
  if (pCbpBit[4]) {
    iIdxB = pNeighAvail->iTopAvail  && (pNeighAvail->iTopType  == MB_TYPE_INTRA_PCM || (pNeighAvail->iTopCbp  >> 4) == 2);
    iIdxA = pNeighAvail->iLeftAvail && (pNeighAvail->iLeftType == MB_TYPE_INTRA_PCM || (pNeighAvail->iLeftCbp >> 4) == 2);
    iCtxInc = iIdxA + (iIdxB << 1);
    WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine,
                                      pCtx->pCabacCtx + NEW_CTX_OFFSET_CBP + 2 * CTX_NUM_CBP + iCtxInc,
                                      pCbpBit[5]));
    uiCbp += 1 << (4 + pCbpBit[5]);
  }
  return ERR_NONE;
}

int32_t ParseDeltaQpCabac (PWelsDecoderContext pCtx, int32_t& iQpDelta) {
  uint32_t uiCode;
  PSlice pCurrSlice = & (pCtx->pCurDqLayer->sLayerInfo.sSliceInLayer);
  iQpDelta = 0;
  PWelsCabacCtx pBinCtx = pCtx->pCabacCtx + NEW_CTX_OFFSET_DELTA_QP;
  int32_t iCtxInc = (pCurrSlice->iLastDeltaQp != 0);
  WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pBinCtx + iCtxInc, uiCode));
  if (uiCode != 0) {
    WELS_READ_VERIFY (DecodeUnaryBinCabac (pCtx->pCabacDecEngine, pBinCtx + 2, 1, uiCode));
    uiCode++;
    iQpDelta = (uiCode + 1) >> 1;
    if ((uiCode & 1) == 0)
      iQpDelta = - iQpDelta;
  }
  pCurrSlice->iLastDeltaQp = iQpDelta;
  return ERR_NONE;
}

int32_t ParseCbfInfoCabac (PWelsNeighAvail pNeighAvail, uint8_t* pNzcCache, int32_t iZIndex, int32_t iResProperty,
                           PWelsDecoderContext pCtx, uint32_t& uiCbfBit) {
  int8_t nA, nB/*, zigzag_idx = 0*/;
  int32_t iCurrBlkXy = pCtx->pCurDqLayer->iMbXyIndex;
  int32_t iTopBlkXy = iCurrBlkXy - pCtx->pCurDqLayer->iMbWidth; //default value: MB neighboring
  int32_t iLeftBlkXy = iCurrBlkXy - 1; //default value: MB neighboring
  uint8_t* pCbfDc = pCtx->pCurDqLayer->pCbfDc;
  int8_t* pMbType = pCtx->pCurDqLayer->pMbType;
  int32_t iCtxInc;
  uiCbfBit = 0;
  nA = nB = IS_INTRA (pMbType[iCurrBlkXy]);

  if (iResProperty == I16_LUMA_DC || iResProperty == CHROMA_DC_U || iResProperty == CHROMA_DC_V) { //DC
    if (pNeighAvail->iTopAvail)
      nB = (pMbType[iTopBlkXy] == MB_TYPE_INTRA_PCM) || ((pCbfDc[iTopBlkXy] >> iResProperty) & 1);
    if (pNeighAvail->iLeftAvail)
      nA = (pMbType[iLeftBlkXy] == MB_TYPE_INTRA_PCM) || ((pCbfDc[iLeftBlkXy] >> iResProperty) & 1);
    iCtxInc = nA + (nB << 1);
    WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine,
                                      pCtx->pCabacCtx + NEW_CTX_OFFSET_CBF + g_kBlockCat2CtxOffsetCBF[iResProperty] + iCtxInc, uiCbfBit));
    if (uiCbfBit)
      pCbfDc[iCurrBlkXy] |= (1 << iResProperty);
  } else { //AC
    //for 4x4 blk, make sure blk-idx is correct
    if (pNzcCache[g_kCacheNzcScanIdx[iZIndex] - 8] != 0xff) { //top blk available
      if (g_kTopBlkInsideMb[iZIndex])
        iTopBlkXy = iCurrBlkXy;
      nB = pNzcCache[g_kCacheNzcScanIdx[iZIndex] - 8] || pMbType[iTopBlkXy]  == MB_TYPE_INTRA_PCM;
    }
    if (pNzcCache[g_kCacheNzcScanIdx[iZIndex] - 1] != 0xff) { //left blk available
      if (g_kLeftBlkInsideMb[iZIndex])
        iLeftBlkXy = iCurrBlkXy;
      nA = pNzcCache[g_kCacheNzcScanIdx[iZIndex] - 1] || pMbType[iLeftBlkXy] == MB_TYPE_INTRA_PCM;
    }

    iCtxInc = nA + (nB << 1);
    WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine,
                                      pCtx->pCabacCtx + NEW_CTX_OFFSET_CBF + g_kBlockCat2CtxOffsetCBF[iResProperty] + iCtxInc, uiCbfBit));
  }
  return ERR_NONE;
}

int32_t ParseSignificantMapCabac (int32_t* pSignificantMap, int32_t iResProperty, PWelsDecoderContext pCtx,
                                  uint32_t& uiCoeffNum) {
  uint32_t uiCode;
  PWelsCabacCtx pMapCtx  = pCtx->pCabacCtx + NEW_CTX_OFFSET_MAP + g_kBlockCat2CtxOffsetMap [iResProperty];
  PWelsCabacCtx pLastCtx = pCtx->pCabacCtx + NEW_CTX_OFFSET_LAST + g_kBlockCat2CtxOffsetLast[iResProperty];

  int32_t i;
  uiCoeffNum = 0;
  int32_t i0 = 0;
  int32_t i1 = g_kMaxPos[iResProperty];

  for (i = i0; i < i1; ++i) {
    //read significant
    WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pMapCtx + i, uiCode));
    if (uiCode) {
      * (pSignificantMap++) = 1;
      ++ uiCoeffNum;
      //read last significant
      WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pLastCtx + i, uiCode));
      if (uiCode) {
        memset (pSignificantMap, 0, (i1 - i) * sizeof (int32_t));
        return ERR_NONE;
      }
    } else
      * (pSignificantMap++) = 0;
  }

  //deal with last pSignificantMap if no data
  //if(i < i1+1)
  {
    *pSignificantMap = 1;
    ++uiCoeffNum;
  }

  return ERR_NONE;
}

int32_t ParseSignificantCoeffCabac (int32_t* pSignificant, int32_t iResProperty, PWelsDecoderContext pCtx) {
  uint32_t uiCode;
  PWelsCabacCtx pOneCtx = pCtx->pCabacCtx + NEW_CTX_OFFSET_ONE + g_kBlockCat2CtxOffsetOne[iResProperty];
  PWelsCabacCtx pAbsCtx = pCtx->pCabacCtx + NEW_CTX_OFFSET_ABS + g_kBlockCat2CtxOffsetAbs[iResProperty];
  const int16_t iMaxType = g_kMaxC2[iResProperty];
  int32_t i = g_kMaxPos[iResProperty];
  int32_t* pCoff = pSignificant + i;
  int32_t c1 = 1;
  int32_t c2 = 0;
  for (; i >= 0; --i) {
    if (*pCoff != 0) {
      WELS_READ_VERIFY (DecodeBinCabac (pCtx->pCabacDecEngine, pOneCtx + c1, uiCode));
      *pCoff += uiCode;
      if (*pCoff == 2) {
        WELS_READ_VERIFY (DecodeUEGLevelCabac (pCtx->pCabacDecEngine, pAbsCtx + c2, uiCode));
        *pCoff += uiCode;
        ++c2;
        c2 = WELS_MIN (c2, iMaxType);
        c1 = 0;
      } else if (c1) {
        ++c1;
        c1 = WELS_MIN (c1, 4);
      }
      WELS_READ_VERIFY (DecodeBypassCabac (pCtx->pCabacDecEngine, uiCode));
      if (uiCode)
        *pCoff = - *pCoff;
    }
    pCoff--;
  }
  return ERR_NONE;
}

int32_t ParseResidualBlockCabac (PWelsNeighAvail pNeighAvail, uint8_t* pNonZeroCountCache, SBitStringAux* pBsAux,
                                 int32_t iIndex, int32_t iMaxNumCoeff,
                                 const uint8_t* pScanTable, int32_t iResProperty, short* sTCoeff, /*int mb_mode*/ uint8_t uiQp,
                                 PWelsDecoderContext pCtx) {
  int32_t iCurNzCacheIdx;
  const uint16_t* pDeQuantMul = g_kuiDequantCoeff[uiQp];
  uint32_t uiTotalCoeffNum = 0;
  uint32_t uiCbpBit;
  int32_t pSignificantMap[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  WELS_READ_VERIFY (ParseCbfInfoCabac (pNeighAvail, pNonZeroCountCache, iIndex, iResProperty, pCtx, uiCbpBit));
  if (uiCbpBit) { //has coeff
    WELS_READ_VERIFY (ParseSignificantMapCabac (pSignificantMap, iResProperty, pCtx, uiTotalCoeffNum));
    WELS_READ_VERIFY (ParseSignificantCoeffCabac (pSignificantMap, iResProperty, pCtx));
  }

  iCurNzCacheIdx = g_kCacheNzcScanIdx[iIndex];
  pNonZeroCountCache[iCurNzCacheIdx] = (uint8_t)uiTotalCoeffNum;
  if (uiTotalCoeffNum == 0) {
    return ERR_NONE;
  }
  int32_t j = 0;
  if (iResProperty == I16_LUMA_DC) {
    do {
      if (pSignificantMap[j] != 0)
        sTCoeff[pScanTable[j]] = pSignificantMap[j];
      ++j;
    } while (j < 16);
  } else if (iResProperty == CHROMA_DC_U) {
    do {
      if (pSignificantMap[j] != 0)
        sTCoeff[pScanTable[j]] = pSignificantMap[j] * pDeQuantMul[0];
      ++j;
    } while (j < 16);
  } else { //luma ac, chroma ac
    do {
      if (pSignificantMap[j] != 0)
        sTCoeff[pScanTable[j]] = pSignificantMap[j] * pDeQuantMul[pScanTable[j] & 0x07];
      ++j;
    } while (j < 16);
  }
  return ERR_NONE;
}

int32_t ParseIPCMInfoCabac (PWelsDecoderContext pCtx) {
  int32_t i;
  PWelsCabacDecEngine pCabacDecEngine = pCtx->pCabacDecEngine;
  SBitStringAux* pBsAux = pCtx->pCurDqLayer->pBitStringAux;
  SDqLayer* pCurLayer = pCtx->pCurDqLayer;
  int32_t iDstStrideLuma = pCurLayer->pDec->iLinesize[0];
  int32_t iDstStrideChroma = pCurLayer->pDec->iLinesize[1];
  int32_t iMbX = pCurLayer->iMbX;
  int32_t iMbY = pCurLayer->iMbY;
  int32_t iMbXy = pCurLayer->iMbXyIndex;

  int32_t iMbOffsetLuma = (iMbX + iMbY * iDstStrideLuma) << 4;
  int32_t iMbOffsetChroma = (iMbX + iMbY * iDstStrideChroma) << 3;

  uint8_t* pMbDstY = pCtx->pDec->pData[0] + iMbOffsetLuma;
  uint8_t* pMbDstU = pCtx->pDec->pData[1] + iMbOffsetChroma;
  uint8_t* pMbDstV = pCtx->pDec->pData[2] + iMbOffsetChroma;

  uint8_t* pPtrSrc;

  pCurLayer->pMbType[iMbXy] = MB_TYPE_INTRA_PCM;
  RestoreCabacDecEngineToBS (pCabacDecEngine, pBsAux);
  intX_t iBytesLeft = pBsAux->pEndBuf - pBsAux->pCurBuf;
  if (iBytesLeft < 384) {
    return ERR_CABAC_NO_BS_TO_READ;
  }
  pPtrSrc = pBsAux->pCurBuf;
  for (i = 0; i < 16; i++) {   //luma
    memcpy (pMbDstY , pPtrSrc, 16);
    pMbDstY += iDstStrideLuma;
    pPtrSrc += 16;
  }
  for (i = 0; i < 8; i++) {   //cb
    memcpy (pMbDstU, pPtrSrc, 8);
    pMbDstU += iDstStrideChroma;
    pPtrSrc += 8;
  }
  for (i = 0; i < 8; i++) {   //cr
    memcpy (pMbDstV, pPtrSrc, 8);
    pMbDstV += iDstStrideChroma;
    pPtrSrc += 8;
  }

  pBsAux->pCurBuf += 384;

  pCurLayer->pLumaQp[iMbXy] = 0;
  pCurLayer->pChromaQp[iMbXy] = 0;
  memset (pCurLayer->pNzc[iMbXy], 16, sizeof (pCurLayer->pNzc[iMbXy]));

  //step 4: cabac engine init
  WELS_READ_VERIFY (InitReadBits (pBsAux, 1));
  WELS_READ_VERIFY (InitCabacDecEngineFromBS (pCabacDecEngine, pBsAux));
  return ERR_NONE;
}
}
