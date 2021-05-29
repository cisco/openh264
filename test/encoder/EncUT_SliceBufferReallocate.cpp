#include "wels_common_basis.h"
#include "utils/BufferedData.h"
#include "BaseEncoderTest.h"
#include "svc_encode_slice.h"
#include "encoder.h"
#include "macros.h"
#include "EncUT_SliceBufferReallocate.h"

namespace WelsEnc {
extern void FreeDqLayer (SDqLayer*& pDq, CMemoryAlign* pMa);
extern void FreeMemorySvc (sWelsEncCtx** ppCtx);
extern int32_t AcquireLayersNals (sWelsEncCtx** ppCtx,
                                  SWelsSvcCodingParam* pParam,
                                  int32_t* pCountLayers,
                                  int32_t* pCountNals);
extern int32_t ExtendLayerBuffer (sWelsEncCtx* pCtx,
                                  const int32_t kiMaxSliceNumOld,
                                  const int32_t kiMaxSliceNumNew);
}

int32_t RandAvailableThread (sWelsEncCtx* pCtx, const int32_t kiMinBufferNum) {
  int32_t aiThrdList[MAX_THREADS_NUM] = { -1 };
  int32_t iCodedSlcNum = 0;
  int32_t iMaxSlcNumInThrd = 0;
  int32_t iAvailableThrdNum = 0;
  int32_t iRandThrdIdx = -1;

  if (NULL == pCtx || NULL == pCtx->pCurDqLayer || pCtx->iActiveThreadsNum <= 0) {
    return -1;
  }

  for (int32_t iThrdIdx = 0; iThrdIdx < pCtx->iActiveThreadsNum; iThrdIdx++) {
    iCodedSlcNum = pCtx->pCurDqLayer->sSliceBufferInfo[iThrdIdx].iCodedSliceNum;
    iMaxSlcNumInThrd = pCtx->pCurDqLayer->sSliceBufferInfo[iThrdIdx].iMaxSliceNum;

    if ((iCodedSlcNum + kiMinBufferNum) <= iMaxSlcNumInThrd) {
      aiThrdList[iAvailableThrdNum] = iThrdIdx;
      iAvailableThrdNum++;
    }
  }

  if (0 == iAvailableThrdNum) {
    return -1;
  }
  iRandThrdIdx = rand() % iAvailableThrdNum;

  return aiThrdList[iRandThrdIdx];
}

int32_t AllocateLayerBuffer (sWelsEncCtx* pCtx, const int32_t iLayerIdx) {
  SSpatialLayerConfig* pLayerCfg = &pCtx->pSvcParam->sSpatialLayers[iLayerIdx];
  SDqLayer* pDqLayer = (SDqLayer*)pCtx->pMemAlign->WelsMallocz (sizeof (SDqLayer), "pDqLayer");
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == pDqLayer))

  pDqLayer->iMbWidth = (pLayerCfg->iVideoWidth + 15) >> 4;
  pDqLayer->iMbHeight = (pLayerCfg->iVideoHeight + 15) >> 4;
  pDqLayer->iMaxSliceNum = GetInitialSliceNum (&pLayerCfg->sSliceArgument);

  int32_t iRet = InitSliceInLayer (pCtx, pDqLayer, iLayerIdx, pCtx->pMemAlign);
  if (ENC_RETURN_SUCCESS != iRet) {
    FreeDqLayer (pDqLayer, pCtx->pMemAlign);
    return ENC_RETURN_MEMALLOCERR;
  }

  pCtx->ppDqLayerList[iLayerIdx] = pDqLayer;
  return ENC_RETURN_SUCCESS;
}

void SetPartitonMBNum (SDqLayer* pCurDqLayer, SSpatialLayerConfig* pLayerCfg, int32_t iPartNum) {
  int32_t iMBWidth = (pLayerCfg->iVideoWidth + 15) >> 4;
  int32_t iMBHeight = (pLayerCfg->iVideoHeight + 15) >> 4;
  int32_t iMbNumInFrame = iMBWidth * iMBHeight;
  int32_t iMBPerPart = iMbNumInFrame / iPartNum;

  if (0 == iMBPerPart) {
    iPartNum = 1;
    iMBPerPart = iMbNumInFrame;
  }

  for (int32_t iPartIdx = 0; iPartIdx < (iPartNum - 1); iPartIdx++) {
    pCurDqLayer->FirstMbIdxOfPartition[iPartIdx] = iMBPerPart * iPartIdx;
    pCurDqLayer->EndMbIdxOfPartition[iPartIdx] = pCurDqLayer->FirstMbIdxOfPartition[iPartIdx] + iMBPerPart - 1;
  }

  pCurDqLayer->FirstMbIdxOfPartition[iPartNum - 1] = iMBPerPart * (iPartNum - 1);
  pCurDqLayer->EndMbIdxOfPartition[iPartNum - 1] = iMbNumInFrame - 1;

  for (int32_t iPartIdx = iPartNum; iPartIdx < MAX_THREADS_NUM; iPartIdx++) {
    pCurDqLayer->FirstMbIdxOfPartition[iPartIdx] = 0;
    pCurDqLayer->EndMbIdxOfPartition[iPartIdx] = 0;
  }
}

int32_t InitParamForSizeLimitSlcMode (sWelsEncCtx* pCtx, const int32_t iLayerIdx) {
  SSpatialLayerConfig* pLayerCfg = &pCtx->pSvcParam->sSpatialLayers[iLayerIdx];
  SSliceArgument* pSliceArgument = &pLayerCfg->sSliceArgument;
  int32_t iSliceBufferSize = 0;
  int32_t iLayerBsSize = WELS_ROUND (((3 * pLayerCfg->iVideoWidth * pLayerCfg->iVideoHeight) >> 1) * COMPRESS_RATIO_THR)
                         + MAX_MACROBLOCK_SIZE_IN_BYTE_x2;
  pSliceArgument->uiSliceSizeConstraint = 600;
  pCtx->pSvcParam->uiMaxNalSize = 1500;

  int32_t iMaxSliceNumEstimation = WELS_MIN (AVERSLICENUM_CONSTRAINT,
                                   (iLayerBsSize / pSliceArgument->uiSliceSizeConstraint) + 1);
  pCtx->iMaxSliceCount = WELS_MAX (pCtx->iMaxSliceCount, iMaxSliceNumEstimation);
  iSliceBufferSize = (WELS_MAX ((int32_t)pSliceArgument->uiSliceSizeConstraint,
                                iLayerBsSize / iMaxSliceNumEstimation) << 1) + MAX_MACROBLOCK_SIZE_IN_BYTE_x2;
  pCtx->iSliceBufferSize[iLayerIdx] = iSliceBufferSize;

  int32_t iRet = AllocateLayerBuffer (pCtx, iLayerIdx);
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (ENC_RETURN_SUCCESS != iRet))

  SetPartitonMBNum (pCtx->ppDqLayerList[iLayerIdx], pLayerCfg, pCtx->iActiveThreadsNum);
  return ENC_RETURN_SUCCESS;
}

void InitParamForRasterSlcMode (sWelsEncCtx* pCtx, const int32_t iLayerIdx) {
  SSpatialLayerConfig* pLayerCfg = &pCtx->pSvcParam->sSpatialLayers[iLayerIdx];
  SSliceArgument* pSliceArgument = &pLayerCfg->sSliceArgument;
  int32_t iMBWidth = (pLayerCfg->iVideoWidth + 15) >> 4;
  int32_t iMBHeight = (pLayerCfg->iVideoHeight + 15) >> 4;
  int32_t iMbNumInFrame = iMBWidth * iMBHeight;
  int32_t iSliceMBNum = 0;

  pSliceArgument->uiSliceMbNum[0] = rand() % 2;
  if (0 == pSliceArgument->uiSliceMbNum[0] && iMBHeight > MAX_SLICES_NUM) {
    pSliceArgument->uiSliceNum = MAX_SLICES_NUM;
    pSliceArgument->uiSliceMbNum[0] = 1;
  }

  if (0 != pSliceArgument->uiSliceMbNum[0]) {
    iSliceMBNum = iMbNumInFrame / pSliceArgument->uiSliceNum;
    for (int32_t iSlcIdx = 0; iSlcIdx < (int32_t)pSliceArgument->uiSliceNum - 1; iSlcIdx++) {
      pSliceArgument->uiSliceMbNum[iSlcIdx] = iSliceMBNum;
    }
    iSliceMBNum = iMbNumInFrame / pSliceArgument->uiSliceNum;
    pSliceArgument->uiSliceMbNum[pSliceArgument->uiSliceNum - 1] = iMbNumInFrame - iSliceMBNum *
        (pSliceArgument->uiSliceNum - 1);
  }
}

void SetParamForReallocateTest (sWelsEncCtx* pCtx, int32_t iLayerIdx,
                                int32_t iThreadIndex, int32_t iPartitionNum) {
  SSpatialLayerConfig* pLayerCfg = &pCtx->pSvcParam->sSpatialLayers[iLayerIdx];
  int32_t iPartitionID = rand() % iPartitionNum;
  int32_t iMBNumInPatition = 0;
  int32_t iCodedSlcNum = pCtx->pCurDqLayer->sSliceBufferInfo[iThreadIndex].iMaxSliceNum - 1;
  int32_t iLastCodeSlcIdx = iPartitionID + iCodedSlcNum * iPartitionNum;
  SSlice* pLastCodedSlc = &pCtx->pCurDqLayer->sSliceBufferInfo[iThreadIndex].pSliceBuffer[iCodedSlcNum - 1];
  pLastCodedSlc->iSliceIdx = iLastCodeSlcIdx;

  SetPartitonMBNum (pCtx->ppDqLayerList[iLayerIdx], pLayerCfg, iPartitionNum);

  iMBNumInPatition = pCtx->pCurDqLayer->EndMbIdxOfPartition[iPartitionID] -
                     pCtx->pCurDqLayer->FirstMbIdxOfPartition[iPartitionID] + 1;
  pCtx->pCurDqLayer->sSliceBufferInfo[iThreadIndex].iCodedSliceNum = iCodedSlcNum;
  pCtx->pCurDqLayer->LastCodedMbIdxOfPartition[iPartitionID] = rand() % iMBNumInPatition + 1;

}

int CSliceBufferReallocatTest::InitParamForTestCase (int32_t iLayerIdx) {
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (ENC_RETURN_SUCCESS != InitParam()))
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (ENC_RETURN_SUCCESS != InitFrameBsBuffer()))
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (ENC_RETURN_SUCCESS != InitLayerSliceBuffer (iLayerIdx)))

  //param validation
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR,
                         (cmResultSuccess != m_pEncoder->InitializeExt ((SEncParamExt*)m_EncContext.pSvcParam)))
  return ENC_RETURN_SUCCESS;
}

int CSliceBufferReallocatTest::InitParamForSizeLimitSlcModeCase (int32_t iLayerIdx) {
  SSliceArgument* pSliceArgument = &m_EncContext.pSvcParam->sSpatialLayers[iLayerIdx].sSliceArgument;

  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (ENC_RETURN_SUCCESS != InitParam()))
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (ENC_RETURN_SUCCESS != InitFrameBsBuffer()))
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (ENC_RETURN_SUCCESS != InitLayerSliceBuffer (iLayerIdx)))

  if (SM_SIZELIMITED_SLICE != pSliceArgument->uiSliceMode && NULL != m_EncContext.ppDqLayerList[iLayerIdx]) {
    UnInitLayerSliceBuffer (iLayerIdx);
    pSliceArgument->uiSliceMode = SM_SIZELIMITED_SLICE;
    WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (ENC_RETURN_SUCCESS != InitParamForSizeLimitSlcMode (&m_EncContext,
                           iLayerIdx)))
    WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == m_EncContext.ppDqLayerList[iLayerIdx]))
  }

  //param validation
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR,
                         (cmResultSuccess != m_pEncoder->InitializeExt ((SEncParamExt*)m_EncContext.pSvcParam)))
  return ENC_RETURN_SUCCESS;
}

void CSliceBufferReallocatTest::UnInitParamForTestCase (int32_t iLayerIdx) {
  int32_t iRet = m_pEncoder->Uninitialize();
  ASSERT_TRUE (cmResultSuccess == iRet);

  UnInitFrameBsBuffer();
  UnInitLayerSliceBuffer (iLayerIdx);
  UnInitParam();
}

int CSliceBufferReallocatTest::InitParam() {
  sWelsEncCtx* pCtx = &m_EncContext;
  SWelsFuncPtrList sEncFunctionList;
  pCtx->pFuncList = &sEncFunctionList;

  //always multi thread cases
  pCtx->pSvcParam->iMultipleThreadIdc = (rand() % MAX_THREADS_NUM) + 1;
  pCtx->pSvcParam->iMultipleThreadIdc = (pCtx->pSvcParam->iMultipleThreadIdc <= 1) ? 2 :
                                        pCtx->pSvcParam->iMultipleThreadIdc;
  pCtx->iActiveThreadsNum = pCtx->pSvcParam->iMultipleThreadIdc;
  pCtx->pSvcParam->iSpatialLayerNum = 1;
  pCtx->pSvcParam->bSimulcastAVC = rand() % 2 == 1;

  pCtx->pSvcParam->iPicHeight = (((rand() % MAX_WIDTH) >> 4) << 4) + 16;
  pCtx->pSvcParam->iPicWidth  = (((rand() % MAX_HEIGH) >> 4) << 4) + 16;
  pCtx->iGlobalQp = WelsClip3 (rand() % MAX_QP, MIN_QP, MAX_QP);
  pCtx->pSvcParam->iRCMode = RC_OFF_MODE;
  pCtx->pSvcParam->iTargetBitrate = WelsClip3 (rand() % MAX_BIT_RATE, MIN_BIT_RATE, MAX_BIT_RATE);
  int32_t iParamStraIdx = rand() % 5;
  pCtx->pSvcParam->eSpsPpsIdStrategy = (EParameterSetStrategy) (iParamStraIdx == 4 ? 0x06 : iParamStraIdx);

  pCtx->pFuncList = (SWelsFuncPtrList*)pCtx->pMemAlign->WelsMallocz (sizeof (SWelsFuncPtrList), "SWelsFuncPtrList");
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == pCtx->pFuncList))

  pCtx->pFuncList->pParametersetStrategy = IWelsParametersetStrategy::CreateParametersetStrategy (
        pCtx->pSvcParam->eSpsPpsIdStrategy,
        pCtx->pSvcParam->bSimulcastAVC, pCtx->pSvcParam->iSpatialLayerNum);
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == pCtx->pFuncList->pParametersetStrategy))

  pCtx->ppDqLayerList = (SDqLayer**)pCtx->pMemAlign->WelsMallocz (pCtx->pSvcParam->iSpatialLayerNum * sizeof (SDqLayer*),
                        "ppDqLayerList");
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == pCtx->ppDqLayerList))
  return ENC_RETURN_SUCCESS;
}

void CSliceBufferReallocatTest::UnInitParam() {
  sWelsEncCtx* pCtx = &m_EncContext;
  if (NULL != pCtx->pFuncList->pParametersetStrategy) {
    delete pCtx->pFuncList->pParametersetStrategy;
    pCtx->pFuncList->pParametersetStrategy = NULL;
  }

  if (NULL != pCtx->pFuncList) {
    pCtx->pMemAlign->WelsFree (pCtx->pFuncList, "pCtx->pFuncList");
    pCtx->pFuncList = NULL;
  }

  if (NULL != pCtx->ppDqLayerList) {
    pCtx->pMemAlign->WelsFree (pCtx->ppDqLayerList, "pCtx->ppDqLayerList");
    pCtx->ppDqLayerList = NULL;
  }
}

int CSliceBufferReallocatTest::InitFrameBsBuffer() {
  const int32_t iLayerIdx = 0;
  sWelsEncCtx* pCtx = &m_EncContext;
  SSpatialLayerConfig* pLayerCfg = &pCtx->pSvcParam->sSpatialLayers[iLayerIdx];
  const int32_t kiSpsSize = pCtx->pFuncList->pParametersetStrategy->GetNeededSpsNum() * SPS_BUFFER_SIZE;
  const int32_t kiPpsSize = pCtx->pFuncList->pParametersetStrategy->GetNeededPpsNum() * PPS_BUFFER_SIZE;

  int32_t iNonVclLayersBsSizeCount = SSEI_BUFFER_SIZE + kiSpsSize + kiPpsSize;
  int32_t iLayerBsSize = WELS_ROUND (((3 * pLayerCfg->iVideoWidth * pLayerCfg->iVideoHeight) >> 1) * COMPRESS_RATIO_THR) +
                         MAX_MACROBLOCK_SIZE_IN_BYTE_x2;
  int32_t iVclLayersBsSizeCount = WELS_ALIGN (iLayerBsSize, 4);
  int32_t iCountBsLen = iNonVclLayersBsSizeCount + iVclLayersBsSizeCount;
  int32_t iCountNals = 0;

  int32_t iRet = AcquireLayersNals (&pCtx, pCtx->pSvcParam, &pCtx->pSvcParam->iSpatialLayerNum, &iCountNals);
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (0 != iRet))

  // Output
  pCtx->pOut = (SWelsEncoderOutput*)pCtx->pMemAlign->WelsMallocz (sizeof (SWelsEncoderOutput), "SWelsEncoderOutput");
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == pCtx->pOut))
  pCtx->pOut->pBsBuffer = (uint8_t*)pCtx->pMemAlign->WelsMallocz (iCountBsLen, "pOut->pBsBuffer");
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == pCtx->pOut->pBsBuffer))
  pCtx->pOut->uiSize = iCountBsLen;
  pCtx->pOut->sNalList = (SWelsNalRaw*)pCtx->pMemAlign->WelsMallocz (iCountNals * sizeof (SWelsNalRaw), "pOut->sNalList");
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == pCtx->pOut->sNalList))
  pCtx->pOut->pNalLen = (int32_t*)pCtx->pMemAlign->WelsMallocz (iCountNals * sizeof (int32_t), "pOut->pNalLen");
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == pCtx->pOut->pNalLen))
  pCtx->pOut->iCountNals = iCountNals;
  pCtx->pOut->iNalIndex = 0;
  pCtx->pOut->iLayerBsIndex = 0;
  pCtx->pFrameBs = (uint8_t*)pCtx->pMemAlign->WelsMalloc (iCountBsLen, "pFrameBs");
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == pCtx->pFrameBs))
  pCtx->iFrameBsSize = iCountBsLen;
  pCtx->iPosBsBuffer = 0;
  return ENC_RETURN_SUCCESS;
}

void CSliceBufferReallocatTest::UnInitFrameBsBuffer() {
  sWelsEncCtx* pCtx = &m_EncContext;

  if (NULL != pCtx->pOut->pBsBuffer) {
    pCtx->pMemAlign->WelsFree (pCtx->pOut->pBsBuffer, "pCtx->pOut->pBsBuffer");
    pCtx->pOut->pBsBuffer = NULL;
  }

  if (NULL != pCtx->pOut->sNalList) {
    pCtx->pMemAlign->WelsFree (pCtx->pOut->sNalList, "pCtx->pOut->sNalList");
    pCtx->pOut->sNalList = NULL;
  }

  if (NULL != pCtx->pOut->pNalLen) {
    pCtx->pMemAlign->WelsFree (pCtx->pOut->pNalLen, "pCtx->pOut->pNalLen");
    pCtx->pOut->pNalLen = NULL;
  }

  if (NULL != pCtx->pOut) {
    pCtx->pMemAlign->WelsFree (pCtx->pOut, "pCtx->pOut");
    pCtx->pOut = NULL;
  }

  if (NULL != pCtx->pFrameBs) {
    pCtx->pMemAlign->WelsFree (pCtx->pFrameBs, "pCtx->pFrameBs");
    pCtx->pFrameBs = NULL;
  }
}

int CSliceBufferReallocatTest::InitLayerSliceBuffer (const int32_t iLayerIdx) {
  sWelsEncCtx* pCtx = &m_EncContext;
  SSpatialLayerConfig* pLayerCfg = &pCtx->pSvcParam->sSpatialLayers[iLayerIdx];
  SSliceArgument* pSliceArgument = &pLayerCfg->sSliceArgument;
  int32_t iLayerBsSize = 0;
  int32_t iSliceBufferSize = 0;
  int32_t iMaxFrameRate = 0;

  pLayerCfg->iVideoWidth  = pCtx->pSvcParam->iPicWidth >> (pCtx->pSvcParam->iSpatialLayerNum - 1 - iLayerIdx);
  pLayerCfg->iVideoHeight = pCtx->pSvcParam->iPicHeight >> (pCtx->pSvcParam->iSpatialLayerNum - 1 - iLayerIdx);

  iMaxFrameRate         = MAX_SAMPLES_PER_SECOND / (pLayerCfg->iVideoWidth * pLayerCfg->iVideoHeight);
  pLayerCfg->fFrameRate = WelsClip3 (pLayerCfg->fFrameRate, (float)MIN_FRAME_RATE, (float)MAX_FRAME_RATE);
  pLayerCfg->fFrameRate = (pLayerCfg->fFrameRate > (float)iMaxFrameRate) ? (float)iMaxFrameRate : pLayerCfg->fFrameRate;

  pLayerCfg->iSpatialBitrate = pCtx->pSvcParam->iTargetBitrate / pCtx->pSvcParam->iSpatialLayerNum;

  //Slice argument
  pSliceArgument->uiSliceMode = (SliceModeEnum) (rand() % 4);
  pSliceArgument->uiSliceNum = rand() % MAX_SLICES_NUM + 1;

  if (pSliceArgument->uiSliceMode == SM_SIZELIMITED_SLICE) {
    WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (ENC_RETURN_SUCCESS != InitParamForSizeLimitSlcMode (pCtx, iLayerIdx)))
  } else {
    if (pSliceArgument->uiSliceMode == SM_RASTER_SLICE) {
      InitParamForRasterSlcMode (pCtx, iLayerIdx);
    }

    iLayerBsSize = WELS_ROUND (((3 * pLayerCfg->iVideoWidth * pLayerCfg->iVideoHeight) >> 1) * COMPRESS_RATIO_THR)
                   + MAX_MACROBLOCK_SIZE_IN_BYTE_x2;
    pCtx->iMaxSliceCount = WELS_MAX (pCtx->iMaxSliceCount, (int)pSliceArgument->uiSliceNum);
    iSliceBufferSize = ((iLayerBsSize / pSliceArgument->uiSliceNum) << 1) + MAX_MACROBLOCK_SIZE_IN_BYTE_x2;

    pCtx->iSliceBufferSize[iLayerIdx] = iSliceBufferSize;
    WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (ENC_RETURN_SUCCESS != AllocateLayerBuffer (pCtx, iLayerIdx)))
  }

  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, (NULL == pCtx->ppDqLayerList[iLayerIdx]))

  pCtx->uiDependencyId = iLayerIdx;
  pCtx->pCurDqLayer = pCtx->ppDqLayerList[iLayerIdx];
  return ENC_RETURN_SUCCESS;
}

void CSliceBufferReallocatTest::UnInitLayerSliceBuffer (const int32_t iLayerIdx) {
  sWelsEncCtx* pCtx = &m_EncContext;
  if (NULL != pCtx->ppDqLayerList[iLayerIdx]) {
    FreeDqLayer (pCtx->ppDqLayerList[iLayerIdx], pCtx->pMemAlign);
    pCtx->ppDqLayerList[iLayerIdx] = NULL;
  }
}

void CSliceBufferReallocatTest::SimulateEncodedOneSlice (const int32_t kiSlcIdx, const int32_t kiThreadIdx) {
  if (m_EncContext.pCurDqLayer->bThreadSlcBufferFlag) {
    int32_t iCodedSlcNumInThrd = m_EncContext.pCurDqLayer->sSliceBufferInfo[kiThreadIdx].iCodedSliceNum;

    ASSERT_TRUE (NULL != m_EncContext.pCurDqLayer->sSliceBufferInfo[kiThreadIdx].pSliceBuffer);
    ASSERT_TRUE (NULL != &m_EncContext.pCurDqLayer->sSliceBufferInfo[kiThreadIdx].pSliceBuffer[iCodedSlcNumInThrd]);

    m_EncContext.pCurDqLayer->sSliceBufferInfo[kiThreadIdx].pSliceBuffer[iCodedSlcNumInThrd].iSliceIdx = kiSlcIdx;
    m_EncContext.pCurDqLayer->sSliceBufferInfo[kiThreadIdx].iCodedSliceNum ++;
  } else {
    m_EncContext.pCurDqLayer->sSliceBufferInfo[0].pSliceBuffer[kiSlcIdx].iSliceIdx = kiSlcIdx;
  }
}

void CSliceBufferReallocatTest::SimulateSliceInOnePartition (const int32_t kiPartNum,
    const int32_t kiPartIdx,
    const int32_t kiSlcNumInPart) {
  int32_t iSlcIdxInPart = 0;

  //slice within same partition will encoded by same thread in current design
  int32_t iPartitionThrdIdx = RandAvailableThread (&m_EncContext, kiSlcNumInPart);
  ASSERT_TRUE (-1 != iPartitionThrdIdx);

  for (int32_t iSlcIdx = 0; iSlcIdx < kiSlcNumInPart; iSlcIdx++) {
    iSlcIdxInPart = kiPartIdx + kiPartNum * iSlcIdx;

    SimulateEncodedOneSlice (iSlcIdxInPart, iPartitionThrdIdx);

    m_EncContext.pCurDqLayer->NumSliceCodedOfPartition[kiPartIdx] ++;
    m_EncContext.pCurDqLayer->sSliceEncCtx.iSliceNumInFrame ++;
  }
}

void CSliceBufferReallocatTest::SimulateSliceInOneLayer() {
  int32_t iLayerIdx = 0;
  SSpatialLayerConfig* pLayerCfg = &m_EncContext.pSvcParam->sSpatialLayers[iLayerIdx];
  int32_t iTotalSliceBuffer = m_EncContext.pCurDqLayer->iMaxSliceNum;
  int32_t iSimulateSliceNum = rand() % iTotalSliceBuffer + 1;

  if (SM_SIZELIMITED_SLICE == pLayerCfg->sSliceArgument.uiSliceMode) {
    int32_t iPartNum         = m_EncContext.iActiveThreadsNum;
    int32_t iSlicNumPerPart  = iSimulateSliceNum / iPartNum;
    int32_t iMaxSlcNumInThrd = m_EncContext.pCurDqLayer->sSliceBufferInfo[0].iMaxSliceNum;
    int32_t iLastPartSlcNum = 0;

    iSlicNumPerPart = WelsClip3 (iSlicNumPerPart, 1, iMaxSlcNumInThrd);
    iLastPartSlcNum = iSimulateSliceNum - iSlicNumPerPart * (iPartNum - 1);
    iLastPartSlcNum = WelsClip3 (iLastPartSlcNum, 1, iMaxSlcNumInThrd);

    for (int32_t iPartIdx = 0; iPartIdx < iPartNum; iPartIdx ++) {
      int32_t iSlcNumInPart = (iPartIdx < (iPartNum - 1)) ? iSlicNumPerPart : iLastPartSlcNum;
      SimulateSliceInOnePartition (iPartNum, iPartIdx, iSlcNumInPart);
    }
  } else {
    for (int32_t iSlcIdx = 0; iSlcIdx < iSimulateSliceNum; iSlcIdx ++) {
      int32_t iSlcThrdIdx = RandAvailableThread (&m_EncContext, 1);
      ASSERT_TRUE (-1 != iSlcThrdIdx);

      SimulateEncodedOneSlice (iSlcIdx, iSlcThrdIdx);
      m_EncContext.pCurDqLayer->sSliceEncCtx.iSliceNumInFrame++;
    }
  }
}

TEST_F (CSliceBufferReallocatTest, Reallocate_in_one_partition) {
  sWelsEncCtx* pCtx = &m_EncContext;
  int32_t iLayerIdx = 0;
  int32_t iRet      = 0;

  iRet = InitParamForSizeLimitSlcModeCase (iLayerIdx);
  ASSERT_TRUE (cmResultSuccess == iRet);
  pCtx->pCurDqLayer = pCtx->ppDqLayerList[iLayerIdx];
  iRet = InitAllSlicesInThread (pCtx);
  ASSERT_TRUE (cmResultSuccess == iRet);

  //case: reallocate during encoding one partition
  //      include cases which part num less than thread num
  //example: 3 threads but 2 partitions
  //          thrd_0: partition_0
  //          thrd_1: partition_1
  //          thrd_2: idle
  int32_t iThreadIndex  = rand() % pCtx->iActiveThreadsNum;
  int32_t iPartitionNum = rand() % pCtx->iActiveThreadsNum + 1; //include cases which part num less than thread num
  int32_t iSlcBufferNum = pCtx->pCurDqLayer->sSliceBufferInfo[iThreadIndex].iMaxSliceNum;

  SetParamForReallocateTest (pCtx, iLayerIdx, iThreadIndex, iPartitionNum);
  iRet = ReallocateSliceInThread (pCtx, pCtx->pCurDqLayer, iLayerIdx, iThreadIndex);
  EXPECT_TRUE (cmResultSuccess == iRet);
  EXPECT_TRUE (NULL != pCtx->pCurDqLayer->sSliceBufferInfo[iThreadIndex].pSliceBuffer);
  EXPECT_TRUE (iSlcBufferNum < pCtx->pCurDqLayer->sSliceBufferInfo[iThreadIndex].iMaxSliceNum);

  UnInitParamForTestCase (iLayerIdx);
}

TEST_F (CSliceBufferReallocatTest, Reallocate_in_one_thread) {
  sWelsEncCtx* pCtx = &m_EncContext;
  int32_t iLayerIdx = 0;
  int32_t iRet = 0;

  iRet = InitParamForSizeLimitSlcModeCase (iLayerIdx);
  ASSERT_TRUE (cmResultSuccess == iRet);
  pCtx->pCurDqLayer = pCtx->ppDqLayerList[iLayerIdx];
  iRet = InitAllSlicesInThread (pCtx);
  ASSERT_TRUE (cmResultSuccess == iRet);

  //case: all partitions encoded by one thread
  //example: 3 threads 3 partions
  //         thrd_0:  partion_0 -->partition_1 -->partition_2
  //         thrd_1:  idle
  //         thrd_2:  idle
  int32_t iThreadIndex  =  rand() % pCtx->iActiveThreadsNum;
  int32_t iPartitionNum = pCtx->iActiveThreadsNum;
  int32_t iSlcBufferNum = 0;

  SetParamForReallocateTest (pCtx, iLayerIdx, iThreadIndex, iPartitionNum);

  for (int32_t iPartIdx = 0; iPartIdx < iPartitionNum; iPartIdx++) {
    iSlcBufferNum = pCtx->pCurDqLayer->sSliceBufferInfo[iThreadIndex].iMaxSliceNum;

    iRet = ReallocateSliceInThread (pCtx, pCtx->pCurDqLayer, iLayerIdx, iThreadIndex);
    EXPECT_TRUE (cmResultSuccess == iRet);
    EXPECT_TRUE (NULL != pCtx->pCurDqLayer->sSliceBufferInfo[iThreadIndex].pSliceBuffer);
    EXPECT_TRUE (iSlcBufferNum < pCtx->pCurDqLayer->sSliceBufferInfo[iThreadIndex].iMaxSliceNum);
  }

  UnInitParamForTestCase (iLayerIdx);
}

TEST_F (CSliceBufferReallocatTest, ExtendLayerBufferTest) {
  sWelsEncCtx* pCtx = &m_EncContext;
  SSlice* pSlcListInThrd = NULL;
  int32_t iLayerIdx = 0;
  int32_t iRet = 0;
  int32_t iMaxSliceNumNew = 0;
  int32_t iSlcBuffNumInThrd = 0;

  iRet = InitParamForSizeLimitSlcModeCase (iLayerIdx);
  ASSERT_TRUE (cmResultSuccess == iRet);
  pCtx->pCurDqLayer = pCtx->ppDqLayerList[iLayerIdx];
  iRet = InitAllSlicesInThread (pCtx);
  ASSERT_TRUE (cmResultSuccess == iRet);

  //before extend, simulate reallocate slice buffer in one thread
  int32_t iReallocateThrdIdx = rand() % pCtx->iActiveThreadsNum;
  iSlcBuffNumInThrd = pCtx->pCurDqLayer->sSliceBufferInfo[iReallocateThrdIdx].iMaxSliceNum;
  pSlcListInThrd = pCtx->pCurDqLayer->sSliceBufferInfo[iReallocateThrdIdx].pSliceBuffer;

  iRet = ReallocateSliceList (pCtx, &pCtx->pSvcParam->sSpatialLayers[iLayerIdx].sSliceArgument,
                              pSlcListInThrd, iSlcBuffNumInThrd, iSlcBuffNumInThrd * 2);
  ASSERT_TRUE (cmResultSuccess == iRet);
  ASSERT_TRUE (NULL != pSlcListInThrd);
  pCtx->pCurDqLayer->sSliceBufferInfo[iReallocateThrdIdx].pSliceBuffer = pSlcListInThrd;
  pCtx->pCurDqLayer->sSliceBufferInfo[iReallocateThrdIdx].iMaxSliceNum   = iSlcBuffNumInThrd * 2;

  for (int32_t iThreadIdx = 0; iThreadIdx < pCtx->iActiveThreadsNum; iThreadIdx++) {
    iMaxSliceNumNew += pCtx->pCurDqLayer->sSliceBufferInfo[iThreadIdx].iMaxSliceNum;
  }

  iRet = ExtendLayerBuffer (pCtx, pCtx->pCurDqLayer->iMaxSliceNum, iMaxSliceNumNew);
  EXPECT_TRUE (cmResultSuccess == iRet);
  EXPECT_TRUE (NULL != pCtx->pCurDqLayer->ppSliceInLayer);
  EXPECT_TRUE (NULL != pCtx->pCurDqLayer->pFirstMbIdxOfSlice);
  EXPECT_TRUE (NULL != pCtx->pCurDqLayer->pCountMbNumInSlice);

  UnInitParamForTestCase (iLayerIdx);
}

TEST_F (CSliceBufferReallocatTest, FrameBsReallocateTest) {
  sWelsEncCtx* pCtx = &m_EncContext;
  int32_t iLayerIdx = 0;
  int32_t iRet = 0;
  SFrameBSInfo FrameBsInfo;
  SLayerBSInfo* pLayerBsInfo = NULL;
  int32_t iCurLayerIdx = rand() % MAX_LAYER_NUM_OF_FRAME;

  memset (&FrameBsInfo, 0, sizeof (SFrameBSInfo));
  InitParamForTestCase (iLayerIdx);

  //init for FrameBs and LayerBs
  pCtx->iPosBsBuffer = rand() % pCtx->iFrameBsSize + 1;
  pLayerBsInfo = &FrameBsInfo.sLayerInfo[iCurLayerIdx];
  pLayerBsInfo->pBsBuf = pCtx->pFrameBs + pCtx->iPosBsBuffer;
  pCtx->bNeedPrefixNalFlag = rand() % 2 == 1;

  int32_t iCodedNalCount = pCtx->pOut->iCountNals;
  iRet = FrameBsRealloc (pCtx, &FrameBsInfo, pLayerBsInfo, iCodedNalCount);

  EXPECT_TRUE (cmResultSuccess == iRet);
  EXPECT_TRUE (iCodedNalCount < pCtx->pOut->iCountNals);
  EXPECT_TRUE (NULL != pCtx->pOut->sNalList);
  EXPECT_TRUE (NULL != pCtx->pOut->pNalLen);

  UnInitParamForTestCase (iLayerIdx);
}

TEST_F (CSliceBufferReallocatTest, ReorderTest) {
  int32_t iLayerIdx = 0;
  int32_t iRet = 0;
  sWelsEncCtx* pCtx = &m_EncContext;
  SSliceArgument* pSliceArgument = &pCtx->pSvcParam->sSpatialLayers[iLayerIdx].sSliceArgument;

  iRet = InitParamForSizeLimitSlcModeCase (iLayerIdx);
  ASSERT_TRUE (cmResultSuccess == iRet);
  pCtx->pCurDqLayer = pCtx->ppDqLayerList[iLayerIdx];
  iRet = InitAllSlicesInThread (pCtx);
  ASSERT_TRUE (cmResultSuccess == iRet);

  SimulateSliceInOneLayer();

  iRet = ReOrderSliceInLayer (pCtx, pSliceArgument->uiSliceMode, pCtx->iActiveThreadsNum);
  EXPECT_TRUE (cmResultSuccess == iRet);

  int32_t iCodedSlcNum = pCtx->pCurDqLayer->sSliceEncCtx.iSliceNumInFrame;
  int32_t iMaxSlicNum = pCtx->pCurDqLayer->iMaxSliceNum;
  EXPECT_TRUE (iCodedSlcNum <= iMaxSlicNum);
  EXPECT_TRUE (NULL != pCtx->pCurDqLayer);
  EXPECT_TRUE (NULL != pCtx->pCurDqLayer->ppSliceInLayer);
  for (int32_t iSlcIdx = 0; iSlcIdx < iCodedSlcNum; iSlcIdx++) {
    EXPECT_TRUE (NULL != &pCtx->pCurDqLayer->ppSliceInLayer[iSlcIdx]);
    EXPECT_TRUE (iSlcIdx == pCtx->pCurDqLayer->ppSliceInLayer[iSlcIdx]->iSliceIdx);
  }

  UnInitParamForTestCase (iLayerIdx);
}

TEST_F (CSliceBufferReallocatTest, LayerInfoUpdateTest) {
  sWelsEncCtx* pCtx = &m_EncContext;
  int32_t iLayerIdx = 0;
  int32_t iRet = 0;
  SFrameBSInfo FrameBsInfo;
  SLayerBSInfo* pLayerBsInfo = NULL;

  iRet = InitParamForSizeLimitSlcModeCase (iLayerIdx);
  ASSERT_TRUE (cmResultSuccess == iRet);
  pCtx->pCurDqLayer = pCtx->ppDqLayerList[iLayerIdx];
  iRet = InitAllSlicesInThread (pCtx);
  ASSERT_TRUE (cmResultSuccess == iRet);

  SimulateSliceInOneLayer();

  //simulate reallocate slice buffer in one thread
  int32_t iReallocateThrdIdx = rand() % pCtx->iActiveThreadsNum;
  int32_t iSlcBuffNumInThrd  = pCtx->pCurDqLayer->sSliceBufferInfo[iReallocateThrdIdx].iMaxSliceNum;
  int32_t iCodedSlcNumInThrd = pCtx->pCurDqLayer->sSliceBufferInfo[iReallocateThrdIdx].iCodedSliceNum;
  SSlice* pSlcListInThrd     = pCtx->pCurDqLayer->sSliceBufferInfo[iReallocateThrdIdx].pSliceBuffer;
  SliceModeEnum eSlcMode     = pCtx->pSvcParam->sSpatialLayers[iLayerIdx].sSliceArgument.uiSliceMode;
  int32_t iSlcIdxInThrd      = 0;
  int32_t iPartitionNum      = (SM_SIZELIMITED_SLICE == eSlcMode) ? pCtx->iActiveThreadsNum : 1;
  int32_t iPartitionIdx      = 0;
  iPartitionIdx = (iCodedSlcNumInThrd <= 0) ? 0 :
                  pSlcListInThrd[iCodedSlcNumInThrd - 1].iSliceIdx % pCtx->iActiveThreadsNum;

  iRet = ReallocateSliceList (pCtx, &pCtx->pSvcParam->sSpatialLayers[iLayerIdx].sSliceArgument,
                              pSlcListInThrd, iSlcBuffNumInThrd, iSlcBuffNumInThrd * 2);
  ASSERT_TRUE (cmResultSuccess == iRet);
  ASSERT_TRUE (NULL != pSlcListInThrd);
  pCtx->pCurDqLayer->sSliceBufferInfo[iReallocateThrdIdx].pSliceBuffer = pSlcListInThrd;
  pCtx->pCurDqLayer->sSliceBufferInfo[iReallocateThrdIdx].iMaxSliceNum = iSlcBuffNumInThrd * 2;

  //update reallocate slice idx/NalNum info
  for (int32_t iSlcIdx = iCodedSlcNumInThrd; iSlcIdx < iSlcBuffNumInThrd * 2; iSlcIdx++) {
    if (SM_SIZELIMITED_SLICE == eSlcMode) {
      iSlcIdxInThrd = iPartitionIdx + pCtx->pCurDqLayer->NumSliceCodedOfPartition[iPartitionIdx] * iPartitionNum;
      pCtx->pCurDqLayer->NumSliceCodedOfPartition[iPartitionIdx] ++;
    } else {
      iSlcIdxInThrd = pCtx->pCurDqLayer->sSliceEncCtx.iSliceNumInFrame;
    }
    ASSERT_TRUE (NULL != &pSlcListInThrd[iSlcIdx]);
    pSlcListInThrd[iSlcIdx].iSliceIdx = iSlcIdxInThrd;
    pSlcListInThrd[iSlcIdx].sSliceBs.iNalIndex = rand() % 2 + 1;
    pSlcListInThrd[iSlcIdx].sSliceBs.uiBsPos = rand() % pSlcListInThrd[iSlcIdx].sSliceBs.uiSize + 1;
    pCtx->pCurDqLayer->sSliceEncCtx.iSliceNumInFrame ++;
    pCtx->pCurDqLayer->sSliceBufferInfo[iReallocateThrdIdx].iCodedSliceNum++;
  }

  //simulate for layer bs
  int32_t iCurLayerIdx = rand() % MAX_LAYER_NUM_OF_FRAME;
  memset (&FrameBsInfo, 0, sizeof (SFrameBSInfo));

  pCtx->iPosBsBuffer = rand() % pCtx->iFrameBsSize + 1;
  pLayerBsInfo = &FrameBsInfo.sLayerInfo[iCurLayerIdx];
  pLayerBsInfo->pBsBuf = pCtx->pFrameBs + pCtx->iPosBsBuffer;
  pCtx->bNeedPrefixNalFlag = rand() % 2 == 1;

  iRet = SliceLayerInfoUpdate (pCtx, &FrameBsInfo, pLayerBsInfo, eSlcMode);
  EXPECT_TRUE (cmResultSuccess == iRet);

  UnInitParamForTestCase (iLayerIdx);
}
