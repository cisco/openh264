#include <gtest/gtest.h>

#include "wels_common_basis.h"
#include "memory_align.h"
#include "error_concealment.h"
#include "ls_defines.h"
#include "cpu.h"

using namespace WelsDec;

#define MAX_MB_WIDTH 260
#define MAX_MB_HEIGHT 130

typedef struct TagECInputCtx {
  int32_t iMbWidth;
  int32_t iMbHeight;
  uint32_t iLinesize[3];
  bool* pMbCorrectlyDecodedFlag; //actual memory
  PWelsDecoderContext pCtx;
  SDqLayer sDqLayer;
  SPicture sAncPic; //Anc picture for comparison
  SPicture sSrcPic; //Src picture as common input picture data
  SPicture sWelsPic; //Wels picture to be compared
} SECInputCtx, *PECInputCtx;

void FreeInputData (PECInputCtx pECCtx) {
  if (pECCtx != NULL) {
    if (pECCtx->pCtx != NULL) {
      WELS_SAFE_FREE (pECCtx->pCtx->pParam, "pECCtx->pCtx->pParam");
      WELS_SAFE_FREE (pECCtx->pCtx->pSps, "pECCtx->pCtx->pSps");
      WELS_SAFE_FREE (pECCtx->pCtx, "pECCtx->pCtx");
    }

    WELS_SAFE_FREE (pECCtx->pMbCorrectlyDecodedFlag, "pECCtx->pMbCorrectlyDecodedFlag");
    WELS_SAFE_FREE (pECCtx->sSrcPic.pData[0], "pECCtx->sSrcPic.pData");
    WELS_SAFE_FREE (pECCtx->sAncPic.pData[0], "pECCtx->sAncPic.pData");
    WELS_SAFE_FREE (pECCtx->sWelsPic.pData[0], "pECCtx->sWelsPic.pData");

    WELS_SAFE_FREE (pECCtx, "pECCtx");
  }
}

int32_t InitAndAllocInputData (PECInputCtx& pECCtx) {
  FreeInputData (pECCtx);

  pECCtx = (PECInputCtx) WelsMallocz (sizeof (SECInputCtx), "pECCtx");
  if (pECCtx == NULL)
    return 1;
  memset (pECCtx, 0, sizeof (SECInputCtx));

  pECCtx->iMbWidth = rand() % (MAX_MB_WIDTH - 1) + 1; //give a constrained max width
  pECCtx->iMbHeight = rand() % (MAX_MB_HEIGHT - 1) + 1; //give a constrained max height
  pECCtx->iLinesize[0] = pECCtx->iMbWidth << 4;
  pECCtx->iLinesize[1] = pECCtx->iLinesize[2] = pECCtx->iLinesize[0] >> 1;

  const uint32_t kiLumaSize = pECCtx->iMbWidth * pECCtx->iMbHeight * 256;

  //allocate picture data
  pECCtx->sWelsPic.pData[0] = (uint8_t*) WelsMallocz (kiLumaSize * 3 / 2 * sizeof (uint8_t), "pECCtx->sWelsPic.pData");
  if (pECCtx->sWelsPic.pData[0] == NULL)
    return 1;
  pECCtx->sWelsPic.pData[1] = pECCtx->sWelsPic.pData[0] + kiLumaSize;
  pECCtx->sWelsPic.pData[2] = pECCtx->sWelsPic.pData[1] + (kiLumaSize >> 2);

  pECCtx->sAncPic.pData[0] = (uint8_t*) WelsMallocz (kiLumaSize * 3 / 2 * sizeof (uint8_t), "pECCtx->sAncPic.pData");
  if (pECCtx->sAncPic.pData[0] == NULL)
    return 1;
  pECCtx->sAncPic.pData[1] = pECCtx->sAncPic.pData[0] + kiLumaSize;
  pECCtx->sAncPic.pData[2] = pECCtx->sAncPic.pData[1] + (kiLumaSize >> 2);

  pECCtx->sSrcPic.pData[0] = (uint8_t*) WelsMallocz (kiLumaSize * 3 / 2 * sizeof (uint8_t), "pECCtx->sSrcPic.pData");
  if (pECCtx->sSrcPic.pData[0] == NULL)
    return 1;
  pECCtx->sSrcPic.pData[1] = pECCtx->sSrcPic.pData[0] + kiLumaSize;
  pECCtx->sSrcPic.pData[2] = pECCtx->sSrcPic.pData[1] + (kiLumaSize >> 2);

  pECCtx->sWelsPic.iLinesize[0] = pECCtx->sAncPic.iLinesize[0] = pECCtx->sSrcPic.iLinesize[0] = pECCtx->iLinesize[0];
  pECCtx->sWelsPic.iLinesize[1] = pECCtx->sAncPic.iLinesize[1] = pECCtx->sSrcPic.iLinesize[1] = pECCtx->iLinesize[1];
  pECCtx->sWelsPic.iLinesize[2] = pECCtx->sAncPic.iLinesize[2] = pECCtx->sSrcPic.iLinesize[2] = pECCtx->iLinesize[2];

  pECCtx->pMbCorrectlyDecodedFlag = (bool*) WelsMallocz (pECCtx->iMbWidth * pECCtx->iMbHeight * sizeof (bool),
                                    "pECCtx->pMbCorrectlyDecodedFlag");
  if (pECCtx->pMbCorrectlyDecodedFlag == NULL)
    return 1;

  pECCtx->pCtx = (PWelsDecoderContext) WelsMallocz (sizeof (SWelsDecoderContext), "pECCtx->pCtx");
  if (pECCtx->pCtx == NULL)
    return 1;

  pECCtx->pCtx->pDec = &pECCtx->sWelsPic;
  pECCtx->pCtx->pCurDqLayer = &pECCtx->sDqLayer;
  pECCtx->pCtx->pCurDqLayer->pMbCorrectlyDecodedFlag = pECCtx->pMbCorrectlyDecodedFlag;

  pECCtx->pCtx->pSps = (PSps) WelsMallocz (sizeof (SSps), "pECCtx->pCtx->pSps");
  if (pECCtx->pCtx->pSps == NULL)
    return 1;
  pECCtx->pCtx->pSps->iMbWidth = pECCtx->iMbWidth;
  pECCtx->pCtx->pSps->iMbHeight = pECCtx->iMbHeight;
  pECCtx->pCtx->pParam = (PDecodingParam) WelsMallocz (sizeof (SDecodingParam), "pECCtx->pCtx->pParam");
  if (pECCtx->pCtx->pParam == NULL)
    return 1;

  return 0;
}

void InitECCopyData (PECInputCtx pECCtx) {
  const int32_t kiMbNum = pECCtx->iMbWidth * pECCtx->iMbHeight;
  int i;
  //init pMbCorrectlyDecodedFlag
  for (i = 0; i < kiMbNum; ++i) {
    pECCtx->pMbCorrectlyDecodedFlag[i] = !! (rand() & 1);
  }
  //init Data
  const int32_t iPixNum = kiMbNum * 256 * 3 / 2;
  for (i = 0; i < iPixNum; ++i) {
    pECCtx->sSrcPic.pData[0][i] = rand() & 0xff;
  }
  int32_t iCpuCores = 1;
  pECCtx->pCtx->uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores);
  InitErrorCon (pECCtx->pCtx);
}

void DoAncErrorConSliceCopy (PECInputCtx pECCtx) {
  int32_t iMbWidth = (int32_t) pECCtx->iMbWidth;
  int32_t iMbHeight = (int32_t) pECCtx->iMbHeight;
  PPicture pDstPic = &pECCtx->sAncPic;
  PPicture pSrcPic = pECCtx->pCtx->pPreviousDecodedPictureInDpb;
  if ((pECCtx->pCtx->pParam->eEcActiveIdc == ERROR_CON_SLICE_COPY)
      && (pECCtx->pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt.bIdrFlag))
    pSrcPic = NULL;

  //uint8_t *pDstData[3], *pSrcData[3];
  bool* pMbCorrectlyDecodedFlag = pECCtx->pMbCorrectlyDecodedFlag;

  //Do slice copy late
  int32_t iMbXyIndex, i;
  uint8_t* pSrcData, *pDstData;
  uint32_t iSrcStride = pECCtx->iLinesize[0];
  uint32_t iDstStride = pECCtx->iLinesize[0];
  for (int32_t iMbY = 0; iMbY < iMbHeight; ++iMbY) {
    for (int32_t iMbX = 0; iMbX < iMbWidth; ++iMbX) {
      iMbXyIndex = iMbY * iMbWidth + iMbX;
      if (!pMbCorrectlyDecodedFlag[iMbXyIndex]) {
        if (pSrcPic != NULL) {
          //Y component
          pDstData = pDstPic->pData[0] + iMbY * 16 * iDstStride + iMbX * 16;
          pSrcData = pSrcPic->pData[0] + iMbY * 16 * iSrcStride + iMbX * 16;
          for (i = 0; i < 16; ++i) {
            memcpy (pDstData, pSrcData, 16);
            pDstData += iDstStride;
            pSrcData += iSrcStride;
          }
          //U component
          pDstData = pDstPic->pData[1] + iMbY * 8 * iDstStride / 2 + iMbX * 8;
          pSrcData = pSrcPic->pData[1] + iMbY * 8 * iSrcStride / 2 + iMbX * 8;
          for (i = 0; i < 8; ++i) {
            memcpy (pDstData, pSrcData, 8);
            pDstData += iDstStride / 2;
            pSrcData += iSrcStride / 2;
          }
          //V component
          pDstData = pDstPic->pData[2] + iMbY * 8 * iDstStride / 2 + iMbX * 8;
          pSrcData = pSrcPic->pData[2] + iMbY * 8 * iSrcStride / 2 + iMbX * 8;
          for (i = 0; i < 8; ++i) {
            memcpy (pDstData, pSrcData, 8);
            pDstData += iDstStride / 2;
            pSrcData += iSrcStride / 2;
          }
        } else { //pSrcPic == NULL
          //Y component
          pDstData = pDstPic->pData[0] + iMbY * 16 * iDstStride + iMbX * 16;
          for (i = 0; i < 16; ++i) {
            memset (pDstData, 128, 16);
            pDstData += iDstStride;
          }
          //U component
          pDstData = pDstPic->pData[1] + iMbY * 8 * iDstStride / 2 + iMbX * 8;
          for (i = 0; i < 8; ++i) {
            memset (pDstData, 128, 8);
            pDstData += iDstStride / 2;
          }
          //V component
          pDstData = pDstPic->pData[2] + iMbY * 8 * iDstStride / 2 + iMbX * 8;
          for (i = 0; i < 8; ++i) {
            memset (pDstData, 128, 8);
            pDstData += iDstStride / 2;
          }
        } //
      } //!pMbCorrectlyDecodedFlag[iMbXyIndex]
    } //iMbX
  } //iMbY
}



bool ComparePictureDataI420 (uint8_t* pSrcData, uint8_t* pDstData, const uint32_t kiStride, const int32_t kiHeight) {
  bool bSame = true;
  uint8_t* pAncData; // = pECCtx->sAncPic.pData[0];
  uint8_t* pCompData;
  int32_t iStride;
  int32_t iCurHeight;
  int32_t iHeight = kiHeight;

  //Y component
  iStride = kiStride;
  pAncData = pSrcData;
  pCompData = pDstData;
  for (iCurHeight = 0; bSame && (iCurHeight < kiHeight); ++iCurHeight) {
    bSame = (memcmp (pAncData, pCompData, iStride * sizeof (uint8_t)) == 0);
    pAncData += iStride;
    pCompData += iStride;
  }
  //chroma component
  iHeight >>= 1;
  iStride >>= 1;
  //U component
  for (iCurHeight = 0; bSame && (iCurHeight < kiHeight / 2); ++iCurHeight) {
    bSame = (memcmp (pAncData, pCompData, iStride * sizeof (uint8_t)) == 0);
    pAncData += iStride;
    pCompData += iStride;
  }
  //V component
  for (iCurHeight = 0; bSame && (iCurHeight < kiHeight / 2); ++iCurHeight) {
    bSame = (memcmp (pAncData, pCompData, iStride * sizeof (uint8_t)) == 0);
    pAncData += iStride;
    pCompData += iStride;
  }

  return bSame;
}

//TEST cases followed
TEST (ErrorConTest, DoErrorConFrameCopy) {
  bool bOK = true;
  PECInputCtx pECCtx = NULL;
  if (InitAndAllocInputData (pECCtx)) {
    FreeInputData (pECCtx);
    return;
  }

  for (int iEC = 0; iEC < 2; ++ iEC) { //ERROR_CON_FRAME_COPY, ERROR_CON_FRAME_COPY_CROSS_IDR
    pECCtx->pCtx->pParam->eEcActiveIdc = iEC > 0 ? ERROR_CON_FRAME_COPY_CROSS_IDR : ERROR_CON_FRAME_COPY;
    InitECCopyData (pECCtx);
    int32_t iLumaSize = pECCtx->iMbWidth * pECCtx->iMbHeight * 256;

    for (int iRef = 0; iRef < 2; ++ iRef) { //no ref, with ref
      pECCtx->pCtx->pPreviousDecodedPictureInDpb = iRef ? &pECCtx->sSrcPic : NULL;
      for (int iIDR = 0; iIDR < 2; ++ iIDR) { //non IDR, IDR
        pECCtx->pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt.bIdrFlag = (iIDR > 0);
        //Do reference code method
        DoErrorConFrameCopy (pECCtx->pCtx);
        //Do anchor method
        if (iRef && ! ((pECCtx->pCtx->pParam->eEcActiveIdc == ERROR_CON_FRAME_COPY)
                       && (pECCtx->pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt.bIdrFlag)))
          memcpy (pECCtx->sAncPic.pData[0], pECCtx->sSrcPic.pData[0], iLumaSize * 3 / 2);
        else
          memset (pECCtx->sAncPic.pData[0], 128, iLumaSize * 3 / 2); //should be the same as known EC method, here all 128
        //Compare results
        bOK = ComparePictureDataI420 (pECCtx->sAncPic.pData[0], pECCtx->sWelsPic.pData[0], pECCtx->iLinesize[0],
                                      pECCtx->iMbHeight * 16);
        EXPECT_EQ (bOK, true);
      } //non IDR, IDR
    } // no ref, with ref
  } //FRAME_COPY methods

  FreeInputData (pECCtx);
}

//TEST cases followed
TEST (ErrorConTest, DoErrorConSliceCopy) {
  bool bOK = true;
  PECInputCtx pECCtx = NULL;
  if (InitAndAllocInputData (pECCtx)) {
    FreeInputData (pECCtx);
    return;
  }

  for (int iEC = 0; iEC < 2; ++ iEC) { //ERROR_CON_SLICE_COPY, ERROR_CON_SLICE_COPY_CROSS_IDR
    pECCtx->pCtx->pParam->eEcActiveIdc = iEC > 0 ? ERROR_CON_SLICE_COPY_CROSS_IDR : ERROR_CON_SLICE_COPY;
    InitECCopyData (pECCtx);
    for (int iRef = 0; iRef < 2; ++ iRef) { //no ref, with ref
      pECCtx->pCtx->pPreviousDecodedPictureInDpb = iRef ? &pECCtx->sSrcPic : NULL;
      for (int iIDR = 0; iIDR < 2; ++ iIDR) { //non IDR, IDR
        pECCtx->pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt.bIdrFlag = (iIDR > 0);
        //Do reference code method
        DoErrorConSliceCopy (pECCtx->pCtx);
        //Do anchor method
        DoAncErrorConSliceCopy (pECCtx);
        //Compare results
        bOK = ComparePictureDataI420 (pECCtx->sAncPic.pData[0], pECCtx->sWelsPic.pData[0], pECCtx->iLinesize[0],
                                      pECCtx->iMbHeight * 16);
        EXPECT_EQ (bOK, true);
      } //non IDR, IDR
    } // no ref, with ref
  } //FRAME_COPY methods

  FreeInputData (pECCtx);
}
