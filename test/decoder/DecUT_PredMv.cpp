#include <gtest/gtest.h>

#include "wels_common_basis.h"
#include "memory_align.h"
#include "mv_pred.h"
#include "ls_defines.h"

using namespace WelsDec;

//Anchor functions
#define REF_NOT_AVAIL    -2
#define REF_NOT_IN_LIST  -1  //intra

//cache element equal to 30
const uint8_t g_kuiAnchorCache30ScanIdx[16] = { //mv or ref_index cache scan index, 4*4 block as basic unit
  7,  8, 13, 14,
  9, 10, 15, 16,
  19, 20, 25, 26,
  21, 22, 27, 28
};

typedef struct TagAnchorMvPred {
  int16_t iMvArray[2][30][2];
  int8_t iRefIdxArray[2][30];
  int32_t iPartIdx;
  int32_t iPartWidth;
  int32_t iRef;
  int16_t iMvp[2];
} SAnchorMvPred;

void AnchorPredMv (int16_t iMotionVector[LIST_A][30][MV_A], int8_t iRefIndex[LIST_A][30],
                   int32_t iPartIdx, int32_t iPartWidth, int8_t iRef, int16_t iMVP[2]) {
  const uint8_t kuiLeftIdx     = g_kuiAnchorCache30ScanIdx[iPartIdx] - 1;
  const uint8_t kuiTopIdx      = g_kuiAnchorCache30ScanIdx[iPartIdx] - 6;
  const uint8_t kuiRightTopIdx = kuiTopIdx + iPartWidth;
  const uint8_t kuiLeftTopIdx  = kuiTopIdx - 1;
  const int8_t kiLeftRef       = iRefIndex[0][kuiLeftIdx];
  const int8_t kiTopRef        = iRefIndex[0][kuiTopIdx];
  const int8_t kiRightTopRef   = iRefIndex[0][kuiRightTopIdx];
  const int8_t kiLeftTopRef    = iRefIndex[0][kuiLeftTopIdx];
  int8_t iDiagonalRef  = kiRightTopRef;
  int8_t iMatchRef = 0;

  int16_t iAMV[2], iBMV[2], iCMV[2];

  * (int32_t*)iAMV = INTD32 (iMotionVector[0][kuiLeftIdx]);
  * (int32_t*)iBMV = INTD32 (iMotionVector[0][kuiTopIdx]);
  * (int32_t*)iCMV = INTD32 (iMotionVector[0][kuiRightTopIdx]);

  if (REF_NOT_AVAIL == iDiagonalRef) {
    iDiagonalRef = kiLeftTopRef;
    * (int32_t*)iCMV = INTD32 (iMotionVector[0][kuiLeftTopIdx]);
  }

  iMatchRef = (iRef == kiLeftRef) + (iRef == kiTopRef) + (iRef == iDiagonalRef);

  if ((REF_NOT_AVAIL == kiTopRef) && (REF_NOT_AVAIL == iDiagonalRef) && (kiLeftRef >= REF_NOT_IN_LIST)) {
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

void AnchorPredInter8x16Mv (int16_t (&iMotionVector)[LIST_A][30][MV_A], int8_t (&iRefIndex)[LIST_A][30],
                            int32_t iPartIdx, int8_t iRef, int16_t iMVP[2]) {
  if (0 == iPartIdx) {
    const int8_t kiLeftRef = iRefIndex[0][6];
    if (iRef == kiLeftRef) {
      ST32 (iMVP, LD32 (&iMotionVector[0][6][0]));
      return;
    }
  } else { // 4 == iPartIdx
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

  AnchorPredMv (iMotionVector, iRefIndex, iPartIdx, 2, iRef, iMVP);
}

void AnchorPredInter16x8Mv (int16_t (&iMotionVector)[LIST_A][30][MV_A], int8_t (&iRefIndex)[LIST_A][30],
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

  AnchorPredMv (iMotionVector, iRefIndex, iPartIdx, 4, iRef, iMVP);
}


//Ref functions in WelsDec
//Input structure for test
typedef struct TagWelsMvPred {
  int16_t iMvArray[2][30][2];
  int8_t iRefIdxArray[2][30];
  int32_t iPartIdx;
  int32_t iPartWidth;
  int32_t iRef;
  int16_t iMvp[2];
} SWelsMvPred;

//mok input data
void AssignMvInputData (SAnchorMvPred* pAncMvPred) {
  int32_t i, j, k;
  //fill MV data and refIdx
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < 30; ++j) {
      for (k = 0; k < 2; ++k) {
        pAncMvPred->iMvArray[i][j][k] = (rand() - RAND_MAX / 2);
      }
      pAncMvPred->iRefIdxArray[i][j] = (rand() % 18) - 2; //-2 ~ 15. 8x8 may have different values, but it matters nothing
    }
  }
}

void CopyMvInputData (SWelsMvPred* pDstMvPred, SAnchorMvPred* pSrcMvPred) {
  int32_t i, j, k;
  //fill MV data and refIdx
  for (i = 0; i < 2; ++i) {
    for (j = 0; j < 30; ++j) {
      for (k = 0; k < 2; ++k) {
        pDstMvPred->iMvArray[i][j][k] = pSrcMvPred->iMvArray[i][j][k];
      }
      pDstMvPred->iRefIdxArray[i][j] = pSrcMvPred->iRefIdxArray[i][j];
    }
  }
}

#define INIT_MV_DATA \
  AssignMvInputData (&sAncMvPred); \
  CopyMvInputData (&sWelsMvPred, &sAncMvPred);

#define TEST_MV_PRED \
  AnchorPredMv (sAncMvPred.iMvArray,  sAncMvPred.iRefIdxArray, iIndex, iBlockWidth, iRef,  sAncMvPred.iMvp); \
  PredMv (sWelsMvPred.iMvArray, sWelsMvPred.iRefIdxArray, LIST_0, iIndex, iBlockWidth, iRef, sWelsMvPred.iMvp); \
  bOK = ((sAncMvPred.iMvp[0] == sWelsMvPred.iMvp[0]) && (sAncMvPred.iMvp[1] == sWelsMvPred.iMvp[1])); \
  EXPECT_EQ (bOK, true);


//TEST cases followed

TEST (PredMvTest, PredMv) {
  SWelsMvPred  sWelsMvPred;
  SAnchorMvPred sAncMvPred;
  int32_t i, iRef, iBlockWidth, iIndex;
  const int32_t kiRandTime = 100;
  bool bOK = true;

  //test specific input: 16x16
  iIndex = 0;
  iBlockWidth = 4;
  i = 0;
  while (i++ < kiRandTime) {
    iRef = (rand() % 18) - 2; //-2~15
    INIT_MV_DATA;
    TEST_MV_PRED;
  }
  //test specific input: 16x8
  iBlockWidth = 4;
  i = 0;
  while (i++ < kiRandTime) {
    iIndex = (rand() & 1) << 3; //0,8
    iRef = (rand() % 18) - 2; //-2~15
    INIT_MV_DATA;
    TEST_MV_PRED;
  }
  //test specific input: 8x16
  iBlockWidth = 2;
  i = 0;
  while (i++ < kiRandTime) {
    iIndex = (rand() & 1) << 2; //0,4
    iRef = (rand() % 18) - 2; //-2~15
    INIT_MV_DATA;
    TEST_MV_PRED;
  }
  //test specific input: 8x8
  iBlockWidth = 2;
  i = 0;
  while (i++ < kiRandTime) {
    iIndex = (rand() & 3) << 2; //0,4,8,12
    iRef = (rand() % 18) - 2; //-2~15
    INIT_MV_DATA;
    TEST_MV_PRED;
  }
  //test specific input: 4x4
  iBlockWidth = 1;
  i = 0;
  while (i++ < kiRandTime) {
    iIndex = rand() & 0x0f; //0~15
    iRef = (rand() % 18) - 2; //-2~15
    INIT_MV_DATA;
    TEST_MV_PRED;
  }
} //TEST PredMv


TEST (PredMvTest, PredInter16x8Mv) {
  SWelsMvPred  sWelsMvPred;
  SAnchorMvPred sAncMvPred;
  int32_t i, iRef, iIndex;
  const int32_t kiRandTime = 100;
  bool bOK = true;

  i = 0;
  while (i++ < kiRandTime) {
    iIndex = (rand() & 1) << 3; //0, 8
    iRef = (rand() % 18) - 2; //-2~15
    INIT_MV_DATA;
    AnchorPredInter16x8Mv (sAncMvPred.iMvArray,  sAncMvPred.iRefIdxArray, iIndex, iRef,  sAncMvPred.iMvp);
    PredInter16x8Mv (sWelsMvPred.iMvArray, sWelsMvPred.iRefIdxArray, LIST_0, iIndex, iRef, sWelsMvPred.iMvp);
    bOK = ((sAncMvPred.iMvp[0] == sWelsMvPred.iMvp[0]) && (sAncMvPred.iMvp[1] == sWelsMvPred.iMvp[1]));
    EXPECT_EQ (bOK, true);
  }
} //TEST PredInter16x8Mv

TEST (PredMvTest, PredInter8x16Mv) {
  SWelsMvPred  sWelsMvPred;
  SAnchorMvPred sAncMvPred;
  int32_t i, iRef, iIndex;
  const int32_t kiRandTime = 100;
  bool bOK = true;

  i = 0;
  while (i++ < kiRandTime) {
    iIndex = (rand() & 1) << 2; //0, 4
    iRef = (rand() % 18) - 2; //-2~15
    INIT_MV_DATA;
    AnchorPredInter8x16Mv (sAncMvPred.iMvArray,  sAncMvPred.iRefIdxArray, iIndex, iRef,  sAncMvPred.iMvp);
    PredInter8x16Mv (sWelsMvPred.iMvArray, sWelsMvPred.iRefIdxArray, LIST_0, iIndex, iRef, sWelsMvPred.iMvp);
    bOK = ((sAncMvPred.iMvp[0] == sWelsMvPred.iMvp[0]) && (sAncMvPred.iMvp[1] == sWelsMvPred.iMvp[1]));
    EXPECT_EQ (bOK, true);
  }
} //TEST PredInter16x8Mv

void AnchorPredPSkipMvFromNeighbor (PDqLayer pCurLayer, int16_t iMvp[2]) {
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



int32_t AllocLayerData (PDqLayer pDqLayer) {

  pDqLayer->pSliceIdc = (int32_t*) WelsMallocz (pDqLayer->iMbWidth * pDqLayer->iMbHeight * sizeof (int32_t),
                        "pDqLayer->pSliceIdc");
  if (pDqLayer->pSliceIdc == NULL)
    return 1;

  pDqLayer->pMbType = (uint32_t*) WelsMallocz (pDqLayer->iMbWidth * pDqLayer->iMbHeight * sizeof (uint32_t),
                      "pDqLayer->pMbType");
  if (pDqLayer->pMbType == NULL)
    return 1;

  pDqLayer->pMv[0] = (int16_t (*)[MB_BLOCK4x4_NUM][MV_A]) WelsMallocz (pDqLayer->iMbWidth * pDqLayer->iMbHeight * sizeof (
                       int16_t) * MV_A * MB_BLOCK4x4_NUM, "pDqLayer->pMv");
  if (pDqLayer->pMv[0] == NULL)
    return 1;

  pDqLayer->pRefIndex[0] = (int8_t (*)[MB_BLOCK4x4_NUM]) WelsMallocz (pDqLayer->iMbWidth * pDqLayer->iMbHeight * sizeof (
                             int8_t) * MB_BLOCK4x4_NUM, "pDqLayer->pRefIndex");
  if (pDqLayer->pRefIndex[0] == NULL)
    return 1;

  return 0;
}

int32_t FreeLayerData (PDqLayer pDqLayer) {

  if (pDqLayer->pSliceIdc != NULL) {
    WelsFree (pDqLayer->pSliceIdc, "pDqLayer->pSliceIdc");
    pDqLayer->pSliceIdc = NULL;
  }

  if (pDqLayer->pMbType != NULL) {
    WelsFree (pDqLayer->pMbType, "pDqLayer->pMbType");
    pDqLayer->pMbType = NULL;
  }

  if (pDqLayer->pMv[0] != NULL) {
    WelsFree (pDqLayer->pMv[0], "pDqlayer->pMv[0]");
    pDqLayer->pMv[0] = NULL;
  }

  if (pDqLayer->pRefIndex[0] != NULL) {
    WelsFree (pDqLayer->pRefIndex[0], "pDqlayer->pRefIndex[0]");
    pDqLayer->pRefIndex[0] = NULL;
  }

  return 0;
}

void InitRandomLayerSliceIdc (PDqLayer pDqLayer) {
  int32_t i = 0;
  int32_t iTotalMbNum = pDqLayer->iMbWidth * pDqLayer->iMbHeight;
  int32_t iMbFirstSliceEnd = rand() % (iTotalMbNum - 1); //assure 2 slices
  for (i = 0; i <= iMbFirstSliceEnd; ++i) {
    pDqLayer->pSliceIdc[i] = 0; //to keep simple value here
  }
  for (; i < iTotalMbNum; ++i) {
    pDqLayer->pSliceIdc[i] = 1; //to keep simple value here
  }
}

void InitRandomLayerMbType (PDqLayer pDqLayer) {
  for (int32_t i = 0; i < pDqLayer->iMbWidth * pDqLayer->iMbHeight; ++i) {
    pDqLayer->pMbType[i] = 1 << (rand() % 11); //2^(1 ~ 10)
  }
}

void InitRandomLayerMvData (PDqLayer pDqLayer) {
  for (int32_t i = 0; i < pDqLayer->iMbWidth * pDqLayer->iMbHeight; ++i) {
    for (int32_t j = 0; j < MB_BLOCK4x4_NUM; ++j) {
      for (int32_t k = 0; k < MV_A; ++k) {
        pDqLayer->pMv[0][i][j][k] = (rand() - RAND_MAX / 2);
      }
    }
  }
}

void InitRandomLayerRefIdxData (PDqLayer pDqLayer) {
  for (int32_t i = 0; i < pDqLayer->iMbWidth * pDqLayer->iMbHeight; ++i) {
    for (int32_t j = 0; j < MB_BLOCK4x4_NUM; ++j) {
      pDqLayer->pRefIndex[0][i][j] = (rand() % 18 - 2); //-2 ~ 15
    }
  }
}

void InitRandomLayerData (PDqLayer pDqLayer) {
  InitRandomLayerSliceIdc (pDqLayer);
  InitRandomLayerMbType (pDqLayer);
  InitRandomLayerMvData (pDqLayer);
  InitRandomLayerRefIdxData (pDqLayer);
}

#define TEST_SKIP_MV_PRED \
  PredPSkipMvFromNeighbor (&sDqLayer, iWelsMvp); \
  bOK = ((iWelsMvp[0] == iAncMvp[0]) && (iWelsMvp[1] == iAncMvp[1])); \
  EXPECT_EQ (bOK, true);

TEST (PredMvTest, PredSkipMvFromNeighbor) {
  const int32_t kiRandTime = 100;
  bool bOK = true;
  SDqLayer sDqLayer;
  int16_t iAncMvp[2], iWelsMvp[2];
  int i;

  memset (&sDqLayer, 0, sizeof (SDqLayer));
  //Assume the input data as 352x288 size
  //allocate the data
  sDqLayer.iMbWidth = 11;
  sDqLayer.iMbHeight = 9;
  if (AllocLayerData (&sDqLayer)) { //memory allocate failed
    FreeLayerData (&sDqLayer);
    return;
  }
  InitRandomLayerData (&sDqLayer); //init MV data, as it would not affect the following logic test

#define CURR_MB_IDX (sDqLayer.iMbXyIndex)
#define LEFT_MB_IDX (sDqLayer.iMbXyIndex - 1)
#define LEFT_MB_BLK 3
#define TOP_MB_IDX (sDqLayer.iMbXyIndex - sDqLayer.iMbWidth)
#define TOP_MB_BLK 12
#define LEFT_TOP_MB_IDX (sDqLayer.iMbXyIndex - sDqLayer.iMbWidth - 1)
#define LEFT_TOP_MB_BLK 15
#define RIGHT_TOP_MB_IDX (sDqLayer.iMbXyIndex - sDqLayer.iMbWidth + 1)
#define RIGHT_TOP_MB_BLK 12

  int32_t iTotalMbNum = sDqLayer.iMbHeight * sDqLayer.iMbWidth;
  //CASE 1: test MB [0,0], expect mvp = (0,0)
  sDqLayer.iMbX = 0;
  sDqLayer.iMbY = 0;
  sDqLayer.iMbXyIndex = sDqLayer.iMbY * sDqLayer.iMbWidth + sDqLayer.iMbX;
  iAncMvp[0] = iAncMvp[1] = 0; //expect anchor result to 0
  TEST_SKIP_MV_PRED;
  //CASE 2: test MB [ANY, 0], expect mvp = (0,0)
  sDqLayer.iMbX = rand() % sDqLayer.iMbWidth;
  sDqLayer.iMbY = 0;
  sDqLayer.iMbXyIndex = sDqLayer.iMbY * sDqLayer.iMbWidth + sDqLayer.iMbX;
  iAncMvp[0] = iAncMvp[1] = 0; //expect anchor result to 0
  TEST_SKIP_MV_PRED;
  //CASE 3: test MB [0, ANY], expect mvp = (0,0)
  sDqLayer.iMbX = 0;
  sDqLayer.iMbY = rand() % sDqLayer.iMbHeight;
  sDqLayer.iMbXyIndex = sDqLayer.iMbY * sDqLayer.iMbWidth + sDqLayer.iMbX;
  iAncMvp[0] = iAncMvp[1] = 0; //expect anchor result to 0
  TEST_SKIP_MV_PRED;
  //CASE 4.1: test MB [RIGHT_SIDE, ANY]
  sDqLayer.iMbX = sDqLayer.iMbWidth - 1;
  sDqLayer.iMbY = rand() % (sDqLayer.iMbHeight - 1) + 1; //not equal to 0
  sDqLayer.iMbXyIndex = sDqLayer.iMbY * sDqLayer.iMbWidth + sDqLayer.iMbX;
  //CASE 4.1.1: same slice_idc, assume = 0
  memset (sDqLayer.pSliceIdc, 0, iTotalMbNum * sizeof (int32_t));
  //CASE 4.1.1.1: ALL P modes
  for (i = 0; i < iTotalMbNum; ++i) {
    sDqLayer.pMbType[i] = MB_TYPE_16x16;
  }
  //CASE 4.1.1.1.1: ref_idx = 0, left MV = 0, top MV != 0, expect mvp = (0,0)
  memset (sDqLayer.pRefIndex[0], 0, iTotalMbNum * MB_BLOCK4x4_NUM * sizeof (int8_t));
  InitRandomLayerMvData (&sDqLayer); //reset Mv data
  sDqLayer.pMv[0][LEFT_MB_IDX][LEFT_MB_BLK][0] = sDqLayer.pMv[0][LEFT_MB_IDX][LEFT_MB_BLK][1] = 0; //left_mv = 0
  sDqLayer.pMv[0][ TOP_MB_IDX][ TOP_MB_BLK][0] = sDqLayer.pMv[0][ TOP_MB_IDX][ TOP_MB_BLK][1] = 1; //top_mv != 0
  iAncMvp[0] = iAncMvp[1] = 0; //expect anchor result to 0
  TEST_SKIP_MV_PRED;
  //CASE 4.1.1.1.2: ref_idx = 0, left MV != 0, top MV = 0, expect mvp = (0,0)
  memset (sDqLayer.pRefIndex[0], 0, iTotalMbNum * MB_BLOCK4x4_NUM * sizeof (int8_t));
  InitRandomLayerMvData (&sDqLayer); //reset Mv data
  sDqLayer.pMv[0][LEFT_MB_IDX][LEFT_MB_BLK][0] = sDqLayer.pMv[0][LEFT_MB_IDX][LEFT_MB_BLK][1] = 1; //left_mv != 0
  sDqLayer.pMv[0][ TOP_MB_IDX][ TOP_MB_BLK][0] = sDqLayer.pMv[0][ TOP_MB_IDX][ TOP_MB_BLK][1] = 0; //top_mv = 0
  iAncMvp[0] = iAncMvp[1] = 0; //expect anchor result to 0
  TEST_SKIP_MV_PRED;
  //CASE 4.1.1.1.3: ref_idx top = 0, others = 1, expect mvp = top mv
  InitRandomLayerMvData (&sDqLayer); //reset Mv data
  sDqLayer.pRefIndex[0][ TOP_MB_IDX][ TOP_MB_BLK] = 0; //top ref_idx = 0
  sDqLayer.pRefIndex[0][LEFT_MB_IDX][LEFT_MB_BLK] = 1; //left ref_idx = 1
  sDqLayer.pRefIndex[0][LEFT_TOP_MB_IDX][LEFT_TOP_MB_BLK] = 1; //left_top ref_idx = 1
  iAncMvp[0] = sDqLayer.pMv[0][TOP_MB_IDX][TOP_MB_BLK][0];
  iAncMvp[1] = sDqLayer.pMv[0][TOP_MB_IDX][TOP_MB_BLK][1];
  TEST_SKIP_MV_PRED;
  //CASE 4.1.1.1.4: ref_idx left = 0, others = 1, expect mvp = left mv
  sDqLayer.pRefIndex[0][ TOP_MB_IDX][ TOP_MB_BLK] = 1; //top ref_idx = 1
  sDqLayer.pRefIndex[0][LEFT_MB_IDX][LEFT_MB_BLK] = 0; //left ref_idx = 0
  sDqLayer.pRefIndex[0][LEFT_TOP_MB_IDX][LEFT_TOP_MB_BLK] = 1; //left_top ref_idx = 1
  iAncMvp[0] = sDqLayer.pMv[0][LEFT_MB_IDX][LEFT_MB_BLK][0];
  iAncMvp[1] = sDqLayer.pMv[0][LEFT_MB_IDX][LEFT_MB_BLK][1];
  TEST_SKIP_MV_PRED;
  //CASE 4.1.1.2: All I
  for (i = 0; i < iTotalMbNum; ++i) {
    sDqLayer.pMbType[i] = MB_TYPE_INTRA16x16;
  }
  //CASE 4.1.1.2.1: left P, expect mvp = left mv
  sDqLayer.pMbType[LEFT_MB_IDX] = MB_TYPE_16x16; //left P
  iAncMvp[0] = sDqLayer.pMv[0][LEFT_MB_IDX][LEFT_MB_BLK][0];
  iAncMvp[1] = sDqLayer.pMv[0][LEFT_MB_IDX][LEFT_MB_BLK][1];
  TEST_SKIP_MV_PRED;
  //CASE 4.1.1.3: only top P, top ref_idx = 0, expect mvp = top mv
  for (i = 0; i < iTotalMbNum; ++i) {  // All I MB
    sDqLayer.pMbType[i] = MB_TYPE_INTRA16x16;
  }
  memset (sDqLayer.pRefIndex[0], 1, iTotalMbNum * MB_BLOCK4x4_NUM * sizeof (int8_t)); // All ref_idx = 1
  sDqLayer.pMbType[TOP_MB_IDX] = MB_TYPE_16x16; //top P
  sDqLayer.pRefIndex[0][TOP_MB_IDX][TOP_MB_BLK] = 0; //top ref_idx = 0
  iAncMvp[0] = sDqLayer.pMv[0][TOP_MB_IDX][TOP_MB_BLK][0];
  iAncMvp[1] = sDqLayer.pMv[0][TOP_MB_IDX][TOP_MB_BLK][1];
  TEST_SKIP_MV_PRED;
  //CASE 4.1.1.4: only left_top P, left_top ref_idx = 0, expect mvp = 0
  sDqLayer.iMbX = (rand() % (sDqLayer.iMbWidth - 2)) + 1; //1 ~ (mb_width - 2)
  sDqLayer.iMbY = (rand() % (sDqLayer.iMbHeight - 2)) + 1; //1 ~ (mb_height - 2)
  sDqLayer.iMbXyIndex = sDqLayer.iMbY * sDqLayer.iMbWidth + sDqLayer.iMbX;
  for (i = 0; i < iTotalMbNum; ++i) {  // All I MB
    sDqLayer.pMbType[i] = MB_TYPE_INTRA16x16;
  }
  memset (sDqLayer.pRefIndex[0], 1, iTotalMbNum * MB_BLOCK4x4_NUM * sizeof (int8_t)); // All ref_idx = 1
  sDqLayer.pMbType[LEFT_TOP_MB_IDX] = MB_TYPE_16x16; //top P
  sDqLayer.pRefIndex[0][LEFT_TOP_MB_IDX][LEFT_TOP_MB_BLK] = 0; //top ref_idx = 0
  iAncMvp[0] = iAncMvp[1] = 0; //expect anchor result to 0
  TEST_SKIP_MV_PRED;
  //CASE 4.1.1.5: only right_top P, right_top ref_idx = 0, expect mvp = right_top mv
  sDqLayer.iMbX = (rand() % (sDqLayer.iMbWidth - 2)) + 1; //1 ~ (mb_width - 2)
  sDqLayer.iMbY = (rand() % (sDqLayer.iMbHeight - 2)) + 1; //1 ~ (mb_height - 2)
  sDqLayer.iMbXyIndex = sDqLayer.iMbY * sDqLayer.iMbWidth + sDqLayer.iMbX;
  for (i = 0; i < iTotalMbNum; ++i) {  // All I MB
    sDqLayer.pMbType[i] = MB_TYPE_INTRA16x16;
  }
  memset (sDqLayer.pRefIndex[0], 1, iTotalMbNum * MB_BLOCK4x4_NUM * sizeof (int8_t)); // All ref_idx = 1
  sDqLayer.pMbType[RIGHT_TOP_MB_IDX] = MB_TYPE_16x16; //top P
  sDqLayer.pRefIndex[0][RIGHT_TOP_MB_IDX][RIGHT_TOP_MB_BLK] = 0; //top ref_idx = 0
  iAncMvp[0] = sDqLayer.pMv[0][RIGHT_TOP_MB_IDX][RIGHT_TOP_MB_BLK][0];
  iAncMvp[1] = sDqLayer.pMv[0][RIGHT_TOP_MB_IDX][RIGHT_TOP_MB_BLK][1];
  TEST_SKIP_MV_PRED;
  //CASE 4.1.2: different neighbor slice idc for all P and ref_idx = 0, expect mvp = 0
  for (i = 0; i < iTotalMbNum; ++i) {  // All P MB
    sDqLayer.pMbType[i] = MB_TYPE_16x16;
  }
  memset (sDqLayer.pRefIndex[0], 0, iTotalMbNum * MB_BLOCK4x4_NUM * sizeof (int8_t)); // All ref_idx = 1
  sDqLayer.iMbX = (rand() % (sDqLayer.iMbWidth - 2)) + 1; //1 ~ (mb_width - 2)
  sDqLayer.iMbY = (rand() % (sDqLayer.iMbHeight - 2)) + 1; //1 ~ (mb_height - 2)
  sDqLayer.iMbXyIndex = sDqLayer.iMbY * sDqLayer.iMbWidth + sDqLayer.iMbX;
  sDqLayer.pSliceIdc[CURR_MB_IDX] = 5;
  sDqLayer.pSliceIdc[LEFT_MB_IDX] = 0;
  sDqLayer.pSliceIdc[TOP_MB_IDX] = 1;
  sDqLayer.pSliceIdc[LEFT_TOP_MB_IDX] = 2;
  sDqLayer.pSliceIdc[RIGHT_TOP_MB_IDX] = 3;
  iAncMvp[0] = iAncMvp[1] = 0;
  TEST_SKIP_MV_PRED;

  //add new specific tests here

  //normal tests
  i = 0;
  while (i++ < kiRandTime) {
    InitRandomLayerData (&sDqLayer); //init MV data, as it would not affect the following logic test
    AnchorPredPSkipMvFromNeighbor (&sDqLayer, iAncMvp);
    TEST_SKIP_MV_PRED;
  }

  FreeLayerData (&sDqLayer);
}
