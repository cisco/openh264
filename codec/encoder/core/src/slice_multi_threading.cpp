/*!
 * \copy
 *     Copyright (c)  2010-2013, Cisco Systems
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
 * \file    slice_multi_threading.h
 *
 * \brief   pSlice based multiple threading
 *
 * \date    04/16/2010 Created
 *
 *************************************************************************************
 */


#include <assert.h>
#if !defined(_WIN32)
#include <semaphore.h>
#include <unistd.h>
#endif//!_WIN32
#ifndef SEM_NAME_MAX
// length of semaphore name should be system constrained at least on mac 10.7
#define  SEM_NAME_MAX 32
#endif//SEM_NAME_MAX
#include "slice_multi_threading.h"
#include "mt_defs.h"
#include "nal_encap.h"
#include "utils.h"
#include "encoder.h"
#include "svc_encode_slice.h"
#include "deblocking.h"
#include "svc_enc_golomb.h"
#include "crt_util_safe_x.h" // for safe crt like calls
#include "rc.h"

#include "cpu.h"

#include "measure_time.h"
#include "wels_task_management.h"

#if defined(ENABLE_TRACE_MT)
#define MT_TRACE_LOG(pLog, x, ...) WelsLog(pLog, x, __VA_ARGS__)
#else
#define MT_TRACE_LOG(x, ...)
#endif

#define WELS_THREAD_SIGNAL_AND_BREAK(CodedEventList, CodedMasterEvent, iEventIdx) {  \
    WelsEventSignal(&CodedEventList[iEventIdx]);  \
    WelsEventSignal (&CodedMasterEvent);  \
    break;  \
}

namespace WelsEnc {
void UpdateMbListNeighborParallel (SDqLayer* pCurDq,
                                   SMB* pMbList,
                                   const int32_t uiSliceIdc) {
  SSliceCtx* pSliceCtx           = pCurDq->pSliceEncCtx;
  const uint16_t* kpMbMap        = pSliceCtx->pOverallMbMap;
  const int32_t kiMbWidth        = pSliceCtx->iMbWidth;
  int32_t iIdx                   = pSliceCtx->pFirstMbInSlice[uiSliceIdc];
  const int32_t kiEndMbInSlice   = iIdx + pSliceCtx->pCountMbNumInSlice[uiSliceIdc] - 1;

  do {
    SMB* pMb                     = &pMbList[iIdx];
    uint32_t uiNeighborAvailFlag = 0;
    const int32_t kiMbXY         = pMb->iMbXY;
    const int32_t kiMbX          = pMb->iMbX;
    const int32_t kiMbY          = pMb->iMbY;
    bool     bLeft;
    bool     bTop;
    bool     bLeftTop;
    bool     bRightTop;
    int32_t   iLeftXY, iTopXY, iLeftTopXY, iRightTopXY;

    iLeftXY = kiMbXY - 1;
    iTopXY = kiMbXY - kiMbWidth;
    iLeftTopXY = iTopXY - 1;
    iRightTopXY = iTopXY + 1;

    bLeft = (kiMbX > 0) && (uiSliceIdc == kpMbMap[iLeftXY]);
    bTop = (kiMbY > 0) && (uiSliceIdc == kpMbMap[iTopXY]);
    bLeftTop = (kiMbX > 0) && (kiMbY > 0) && (uiSliceIdc == kpMbMap[iLeftTopXY]);
    bRightTop = (kiMbX < (kiMbWidth - 1)) && (kiMbY > 0) && (uiSliceIdc == kpMbMap[iRightTopXY]);

    if (bLeft) {
      uiNeighborAvailFlag |= LEFT_MB_POS;
    }
    if (bTop) {
      uiNeighborAvailFlag |= TOP_MB_POS;
    }
    if (bLeftTop) {
      uiNeighborAvailFlag |= TOPLEFT_MB_POS;
    }
    if (bRightTop) {
      uiNeighborAvailFlag |= TOPRIGHT_MB_POS;
    }
    pMb->uiNeighborAvail        = (uint8_t)uiNeighborAvailFlag;
    pMb->uiSliceIdc             = uiSliceIdc;

    ++ iIdx;
  } while (iIdx <= kiEndMbInSlice);
}

void CalcSliceComplexRatio (void* pRatio, SSliceCtx* pSliceCtx, uint32_t* pSliceConsume) {
  int32_t* pRatioList           = (int32_t*)pRatio;
  int32_t iAvI[MAX_SLICES_NUM];
  int32_t iSumAv                = 0;
  uint32_t* pSliceTime          = (uint32_t*)pSliceConsume;
  int32_t* pCountMbInSlice      = (int32_t*)pSliceCtx->pCountMbNumInSlice;
  const int32_t kiSliceCount    = pSliceCtx->iSliceNumInFrame;
  int32_t iSliceIdx             = 0;

  WelsEmms();

  while (iSliceIdx < kiSliceCount) {
    iAvI[iSliceIdx] = WELS_DIV_ROUND (INT_MULTIPLY * pCountMbInSlice[iSliceIdx], pSliceTime[iSliceIdx]);
    MT_TRACE_LOG (NULL, WELS_LOG_DEBUG, "[MT] CalcSliceComplexRatio(), pSliceConsumeTime[%d]= %d us, slice_run= %d",
                  iSliceIdx,
                  pSliceTime[iSliceIdx], pCountMbInSlice[iSliceIdx]);
    iSumAv += iAvI[iSliceIdx];

    ++ iSliceIdx;
  }
  while (-- iSliceIdx >= 0) {
    pRatioList[iSliceIdx] = WELS_DIV_ROUND (INT_MULTIPLY * iAvI[iSliceIdx], iSumAv);
  }
}

int32_t NeedDynamicAdjust (void* pConsumeTime, const int32_t iSliceNum) {
  uint32_t* pSliceConsume       = (uint32_t*)pConsumeTime;
  uint32_t uiTotalConsume       = 0;
  int32_t iSliceIdx             = 0;
  int32_t iNeedAdj              = false;

  WelsEmms();

  while (iSliceIdx < iSliceNum) {
    uiTotalConsume += pSliceConsume[iSliceIdx];
    iSliceIdx ++;
  }
  if (uiTotalConsume == 0) {
    MT_TRACE_LOG (NULL, WELS_LOG_DEBUG,
                  "[MT] NeedDynamicAdjust(), herein do no adjust due first picture, iCountSliceNum= %d",
                  iSliceNum);
    return false;
  }

  iSliceIdx = 0;
  float fThr                    = EPSN; // threshold for various cores cases
  float fRmse                   = .0f;  // root mean square error of pSlice consume ratios
  const float kfMeanRatio       = 1.0f / iSliceNum;
  do {
    const float fRatio = 1.0f * pSliceConsume[iSliceIdx] / uiTotalConsume;
    const float fDiffRatio = fRatio - kfMeanRatio;
    fRmse += (fDiffRatio * fDiffRatio);
    ++ iSliceIdx;
  } while (iSliceIdx + 1 < iSliceNum);
  fRmse = sqrtf (fRmse / iSliceNum);
  if (iSliceNum >= 8) {
    fThr += THRESHOLD_RMSE_CORE8;
  } else if (iSliceNum >= 4) {
    fThr += THRESHOLD_RMSE_CORE4;
  } else if (iSliceNum >= 2) {
    fThr += THRESHOLD_RMSE_CORE2;
  } else
    fThr = 1.0f;
  if (fRmse > fThr)
    iNeedAdj = true;
  MT_TRACE_LOG (NULL, WELS_LOG_DEBUG,
                "[MT] NeedDynamicAdjust(), herein adjustment decision is made (iNeedAdj= %d) by: fRmse of pSlice complexity ratios %.6f, the corresponding threshold %.6f, iCountSliceNum %d",
                iNeedAdj, fRmse, fThr, iSliceNum);

  return iNeedAdj;
}

void DynamicAdjustSlicing (sWelsEncCtx* pCtx,
                           SDqLayer* pCurDqLayer,
                           void* pComplexRatio,
                           int32_t iCurDid) {
  SSliceCtx* pSliceCtx  = pCurDqLayer->pSliceEncCtx;
  const int32_t kiCountSliceNum = pSliceCtx->iSliceNumInFrame;
  const int32_t kiCountNumMb    = pSliceCtx->iMbNumInFrame;
  int32_t iMinimalMbNum         =
    pSliceCtx->iMbWidth;  // in theory we need only 1 SMB, here let it as one SMB row required
  int32_t iMaximalMbNum         = 0;    // dynamically assign later
  int32_t* pSliceComplexRatio   = (int32_t*)pComplexRatio;
  int32_t iMbNumLeft            = kiCountNumMb;
  int32_t iRunLen[MAX_THREADS_NUM] = {0};
  int32_t iSliceIdx             = 0;

  int32_t iNumMbInEachGom = 0;
  SWelsSvcRc* pWelsSvcRc = &pCtx->pWelsSvcRc[iCurDid];
  if (pCtx->pSvcParam->iRCMode != RC_OFF_MODE) {
    iNumMbInEachGom = pWelsSvcRc->iNumberMbGom;

    if (iNumMbInEachGom <= 0) {
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_ERROR,
               "[MT] DynamicAdjustSlicing(), invalid iNumMbInEachGom= %d from RC, iDid= %d, iCountNumMb= %d", iNumMbInEachGom,
               iCurDid, kiCountNumMb);
      return;
    }

    // do not adjust in case no extra iNumMbInEachGom based left for slicing adjustment,
    // extra MB of non integrated GOM assigned at the last pSlice in default, keep up on early initial result.
    if (iNumMbInEachGom * kiCountSliceNum >= kiCountNumMb) {
      return;
    }
    iMinimalMbNum = iNumMbInEachGom;
  }

  if (kiCountSliceNum < 2 || (kiCountSliceNum & 0x01)) // we need suppose uiSliceNum is even for multiple threading
    return;

  iMaximalMbNum = kiCountNumMb - (kiCountSliceNum - 1) * iMinimalMbNum;

  WelsEmms();

  MT_TRACE_LOG (pCtx, WELS_LOG_DEBUG, "[MT] DynamicAdjustSlicing(), iDid= %d, iCountNumMb= %d", iCurDid, kiCountNumMb);

  iSliceIdx = 0;
  while (iSliceIdx + 1 < kiCountSliceNum) {
    int32_t iNumMbAssigning = WELS_DIV_ROUND (kiCountNumMb * pSliceComplexRatio[iSliceIdx], INT_MULTIPLY);

    // GOM boundary aligned
    if (pCtx->pSvcParam->iRCMode != RC_OFF_MODE) {
      iNumMbAssigning = iNumMbAssigning / iNumMbInEachGom * iNumMbInEachGom;
    }

    // make sure one GOM at least in each pSlice for safe
    if (iNumMbAssigning < iMinimalMbNum)
      iNumMbAssigning = iMinimalMbNum;
    else if (iNumMbAssigning > iMaximalMbNum)
      iNumMbAssigning = iMaximalMbNum;

    assert (iNumMbAssigning > 0);

    iMbNumLeft -= iNumMbAssigning;
    if (iMbNumLeft <= 0) { // error due to we can not support slice_skip now yet, do not adjust this time
      assert (0);
      return;
    }
    iRunLen[iSliceIdx] = iNumMbAssigning;
    MT_TRACE_LOG (pCtx, WELS_LOG_DEBUG,
                  "[MT] DynamicAdjustSlicing(), uiSliceIdx= %d, pSliceComplexRatio= %.2f, slice_run_org= %d, slice_run_adj= %d",
                  iSliceIdx, pSliceComplexRatio[iSliceIdx] * 1.0f / INT_MULTIPLY, pSliceCtx->pCountMbNumInSlice[iSliceIdx],
                  iNumMbAssigning);
    ++ iSliceIdx;
    iMaximalMbNum = iMbNumLeft - (kiCountSliceNum - iSliceIdx - 1) * iMinimalMbNum; // get maximal num_mb in left parts
  }
  iRunLen[iSliceIdx] = iMbNumLeft;
  MT_TRACE_LOG (pCtx, WELS_LOG_DEBUG,
                "[MT] DynamicAdjustSlicing(), iSliceIdx= %d, pSliceComplexRatio= %.2f, slice_run_org= %d, slice_run_adj= %d",
                iSliceIdx, pSliceComplexRatio[iSliceIdx] * 1.0f / INT_MULTIPLY, pSliceCtx->pCountMbNumInSlice[iSliceIdx], iMbNumLeft);


  if (DynamicAdjustSlicePEncCtxAll (pSliceCtx, iRunLen) == 0) {
    const int32_t kiThreadNum   = pCtx->pSvcParam->iCountThreadsNum;
    int32_t iThreadIdx          = 0;
    do {
      WelsEventSignal (&pCtx->pSliceThreading->pUpdateMbListEvent[iThreadIdx]);
      WelsEventSignal (&pCtx->pSliceThreading->pThreadMasterEvent[iThreadIdx]);
      ++ iThreadIdx;
    } while (iThreadIdx < kiThreadNum);

    WelsMultipleEventsWaitAllBlocking (kiThreadNum, &pCtx->pSliceThreading->pFinUpdateMbListEvent[0]);
  }
}

int32_t SetMultiSliceBuffer (sWelsEncCtx** ppCtx, CMemoryAlign* pMa, SSliceThreading* pSmt,
                             int32_t iMaxSliceNum, int32_t iSlice1Len, int32_t iSlice0Len, bool bDynamicSlice) {
  (*ppCtx)->pSliceBs = (SWelsSliceBs*)pMa->WelsMalloc (sizeof (SWelsSliceBs) * iMaxSliceNum, "pSliceBs");
  if (NULL == (*ppCtx)->pSliceBs) {
    return ENC_RETURN_MEMALLOCERR;
  }

  if (iSlice0Len <= 0) {
    return ENC_RETURN_UNEXPECTED;
  }
  //slice 0
  (*ppCtx)->pSliceBs[0].uiSize = iSlice0Len;
  (*ppCtx)->pSliceBs[0].pBs    = (*ppCtx)->pFrameBs;
  (*ppCtx)->pSliceBs[0].uiBsPos = 0;
  (*ppCtx)->pSliceBs[0].pBsBuffer = pSmt->pThreadBsBuffer[0];
  if ((iMaxSliceNum == 1) && (!bDynamicSlice)) {
    return ENC_RETURN_SUCCESS;
  }
  //slice >0
  if (iSlice1Len <= 0) {
    return ENC_RETURN_UNEXPECTED;
  }
  if ((*ppCtx)->iFrameBsSize < (iSlice0Len + (iMaxSliceNum - 1)*iSlice1Len)) {
    return ENC_RETURN_MEMALLOCERR;
  }
  for (int32_t k = 1; k < iMaxSliceNum; k++) {
    (*ppCtx)->pSliceBs[k].uiSize = iSlice1Len;
    (*ppCtx)->pSliceBs[k].pBs    = (*ppCtx)->pSliceBs[k - 1].pBs + (*ppCtx)->pSliceBs[k - 1].uiSize;
  }
  return ENC_RETURN_SUCCESS;

}

int32_t RequestMtResource (sWelsEncCtx** ppCtx, SWelsSvcCodingParam* pCodingParam, const int32_t iCountBsLen,
                           const int32_t iMaxSliceBufferSize, bool bDynamicSlice) {
  CMemoryAlign* pMa             = NULL;
  SWelsSvcCodingParam* pPara = NULL;
  SSliceThreading* pSmt         = NULL;
  int32_t iNumSpatialLayers     = 0;
  int32_t iThreadNum            = 0;
  int32_t iIdx                  = 0;
  int16_t iMaxSliceNum          = 1;
  int32_t iReturn = ENC_RETURN_SUCCESS;
  bool bWillUseTaskManage = false;

  if (NULL == ppCtx || NULL == pCodingParam || NULL == *ppCtx || iCountBsLen <= 0)
    return 1;

  pMa = (*ppCtx)->pMemAlign;
  pPara = pCodingParam;
  iNumSpatialLayers = pPara->iSpatialLayerNum;
  iThreadNum = pPara->iCountThreadsNum;
  iMaxSliceNum = (*ppCtx)->iMaxSliceCount;

  pSmt = (SSliceThreading*)pMa->WelsMalloc (sizeof (SSliceThreading), "SSliceThreading");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pSmt), FreeMemorySvc (ppCtx))
  (*ppCtx)->pSliceThreading = pSmt;
  pSmt->pThreadPEncCtx = (SSliceThreadPrivateData*)pMa->WelsMalloc (sizeof (SSliceThreadPrivateData) * iThreadNum,
                         "pThreadPEncCtx");
  WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pSmt->pThreadPEncCtx), FreeMemorySvc (ppCtx))

#ifdef _WIN32
  // Dummy event namespace, the windows events don't actually use this
  WelsSnprintf (pSmt->eventNamespace, sizeof (pSmt->eventNamespace), "%p", (void*) *ppCtx);
#else
  WelsSnprintf (pSmt->eventNamespace, sizeof (pSmt->eventNamespace), "%p%x", (void*) *ppCtx, getpid());
#endif//!_WIN32

  iIdx = 0;
  while (iIdx < iNumSpatialLayers) {
    SSliceArgument* pSliceArgument = &pPara->sSpatialLayers[iIdx].sSliceArgument;
    const int32_t kiSliceNum = pSliceArgument->uiSliceNum;
    if ((pSliceArgument->uiSliceMode == SM_FIXEDSLCNUM_SLICE)
        && (pPara->bUseLoadBalancing)
        && (pPara->iMultipleThreadIdc > 1)
        && (pPara->iMultipleThreadIdc >= kiSliceNum)) {
      pSmt->pSliceConsumeTime[iIdx] = (uint32_t*)pMa->WelsMallocz (kiSliceNum * sizeof (uint32_t), "pSliceConsumeTime[]");
      WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pSmt->pSliceConsumeTime[iIdx]), FreeMemorySvc (ppCtx))
      pSmt->pSliceComplexRatio[iIdx] = (int32_t*)pMa->WelsMalloc (kiSliceNum * sizeof (int32_t), "pSliceComplexRatio[]");
      WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pSmt->pSliceComplexRatio[iIdx]), FreeMemorySvc (ppCtx))
    } else {
      pSmt->pSliceConsumeTime[iIdx]     = NULL;
      pSmt->pSliceComplexRatio[iIdx]    = NULL;
    }

    if (pSliceArgument->uiSliceMode == SM_FIXEDSLCNUM_SLICE || pSliceArgument->uiSliceMode == SM_RASTER_SLICE) {
      bWillUseTaskManage = true;
    }
    ++ iIdx;
  }
  // NULL for pSliceConsumeTime[iIdx]: iIdx from iNumSpatialLayers to MAX_DEPENDENCY_LAYERS

#ifdef MT_DEBUG
  // file handle for MT debug
  pSmt->pFSliceDiff = NULL;

  if (pSmt->pFSliceDiff) {
    fclose (pSmt->pFSliceDiff);
    pSmt->pFSliceDiff = NULL;
  }
  pSmt->pFSliceDiff = fopen ("slice_time.txt", "wt+");
#endif//MT_DEBUG

  MT_TRACE_LOG (*ppCtx, WELS_LOG_INFO, "encpEncCtx= 0x%p", (void*) *ppCtx);

  char name[SEM_NAME_MAX] = {0};
  WELS_GCC_UNUSED WELS_THREAD_ERROR_CODE err = 0;

  iIdx = 0;
  while (iIdx < iThreadNum) {
    pSmt->pThreadPEncCtx[iIdx].pWelsPEncCtx     = (void*) *ppCtx;
    pSmt->pThreadPEncCtx[iIdx].iSliceIndex      = iIdx;
    pSmt->pThreadPEncCtx[iIdx].iThreadIndex     = iIdx;
    pSmt->pThreadHandles[iIdx]                  = 0;

    WelsSnprintf (name, SEM_NAME_MAX, "ee%d%s", iIdx, pSmt->eventNamespace);
    err = WelsEventOpen (&pSmt->pExitEncodeEvent[iIdx], name);
    MT_TRACE_LOG (*ppCtx, WELS_LOG_INFO, "[MT] Open pExitEncodeEvent%d named(%s) ret%d err%d", iIdx, name, err, errno);
    WelsSnprintf (name, SEM_NAME_MAX, "tm%d%s", iIdx, pSmt->eventNamespace);
    err = WelsEventOpen (&pSmt->pThreadMasterEvent[iIdx], name);
    MT_TRACE_LOG (*ppCtx, WELS_LOG_INFO, "[MT] Open pThreadMasterEvent%d named(%s) ret%d err%d", iIdx, name, err, errno);
    // length of semaphore name should be system constrained at least on mac 10.7
    WelsSnprintf (name, SEM_NAME_MAX, "ud%d%s", iIdx, pSmt->eventNamespace);
    err = WelsEventOpen (&pSmt->pUpdateMbListEvent[iIdx], name);
    MT_TRACE_LOG (*ppCtx, WELS_LOG_INFO, "[MT] Open pUpdateMbListEvent%d named(%s) ret%d err%d", iIdx, name, err, errno);
    WelsSnprintf (name, SEM_NAME_MAX, "fu%d%s", iIdx, pSmt->eventNamespace);
    err = WelsEventOpen (&pSmt->pFinUpdateMbListEvent[iIdx], name);
    MT_TRACE_LOG (*ppCtx, WELS_LOG_INFO, "[MT] Open pFinUpdateMbListEvent%d named(%s) ret%d err%d", iIdx, name, err,
                  errno);
    WelsSnprintf (name, SEM_NAME_MAX, "sc%d%s", iIdx, pSmt->eventNamespace);
    err = WelsEventOpen (&pSmt->pSliceCodedEvent[iIdx], name);
    MT_TRACE_LOG (*ppCtx, WELS_LOG_INFO, "[MT] Open pSliceCodedEvent%d named(%s) ret%d err%d", iIdx, name, err, errno);
    WelsSnprintf (name, SEM_NAME_MAX, "rc%d%s", iIdx, pSmt->eventNamespace);
    err = WelsEventOpen (&pSmt->pReadySliceCodingEvent[iIdx], name);
    MT_TRACE_LOG (*ppCtx, WELS_LOG_INFO, "[MT] Open pReadySliceCodingEvent%d = 0x%p named(%s) ret%d err%d", iIdx,
                  (void*)pSmt->pReadySliceCodingEvent[iIdx], name, err, errno);

    pSmt->pThreadBsBuffer[iIdx] = (uint8_t*)pMa->WelsMalloc (iCountBsLen, "pSmt->pThreadBsBuffer");
    WELS_VERIFY_RETURN_PROC_IF (1, (NULL == pSmt->pThreadBsBuffer[iIdx]), FreeMemorySvc (ppCtx))

    ++ iIdx;
  }
  for (; iIdx < MAX_THREADS_NUM; iIdx++) {
    pSmt->pThreadBsBuffer[iIdx] = NULL;
  }

  //previous conflict
  WelsSnprintf (name, SEM_NAME_MAX, "scm%s", pSmt->eventNamespace);
  err = WelsEventOpen (&pSmt->pSliceCodedMasterEvent, name);
  MT_TRACE_LOG (*ppCtx, WELS_LOG_INFO, "[MT] Open pSliceCodedMasterEvent named(%s) ret%d err%d", name, err, errno);
  //previous conflict ends

  iReturn = SetMultiSliceBuffer (ppCtx, pMa, pSmt, iMaxSliceNum,
                                 iMaxSliceBufferSize,
                                 iCountBsLen,
                                 bDynamicSlice);
  WELS_VERIFY_RETURN_PROC_IF (iReturn, (ENC_RETURN_SUCCESS != iReturn), FreeMemorySvc (ppCtx))

  iReturn = WelsMutexInit (&pSmt->mutexSliceNumUpdate);
  WELS_VERIFY_RETURN_PROC_IF (1, (WELS_THREAD_ERROR_OK != iReturn), FreeMemorySvc (ppCtx))

  if (bWillUseTaskManage) {
    (*ppCtx)->pTaskManage = IWelsTaskManage::CreateTaskManage (*ppCtx, iNumSpatialLayers, bDynamicSlice);
    WELS_VERIFY_RETURN_PROC_IF (iReturn, (NULL == (*ppCtx)->pTaskManage), FreeMemorySvc (ppCtx))
  }

  memset (&pSmt->bThreadBsBufferUsage, 0, MAX_THREADS_NUM * sizeof (bool));
  iReturn = WelsMutexInit (&pSmt->mutexThreadBsBufferUsage);
  WELS_VERIFY_RETURN_PROC_IF (1, (WELS_THREAD_ERROR_OK != iReturn), FreeMemorySvc (ppCtx))

  iReturn = WelsMutexInit (& (*ppCtx)->mutexEncoderError);
  WELS_VERIFY_RETURN_PROC_IF (1, (WELS_THREAD_ERROR_OK != iReturn), FreeMemorySvc (ppCtx))

  MT_TRACE_LOG (*ppCtx, WELS_LOG_INFO, "RequestMtResource(), iThreadNum=%d, iCountSliceNum= %d",
                pPara->iCountThreadsNum,
                iMaxSliceNum);

  return 0;
}

void ReleaseMtResource (sWelsEncCtx** ppCtx) {
  SWelsSliceBs* pSliceB                 = NULL;
  SWelsSvcCodingParam* pCodingParam     = NULL;
  SSliceThreading* pSmt                 = NULL;
  CMemoryAlign* pMa                     = NULL;
  int32_t iIdx                          = 0;
  int32_t iThreadNum                    = 0;
  int16_t uiSliceNum                    = 0;

  if (NULL == ppCtx || NULL == *ppCtx)
    return;

  pMa           = (*ppCtx)->pMemAlign;
  pCodingParam  = (*ppCtx)->pSvcParam;
  uiSliceNum    = (*ppCtx)->iMaxSliceCount;
  iThreadNum    = (*ppCtx)->pSvcParam->iCountThreadsNum;
  pSmt          = (*ppCtx)->pSliceThreading;

  if (NULL == pSmt)
    return;

  char ename[SEM_NAME_MAX] = {0};
  while (iIdx < iThreadNum) {
    // length of semaphore name should be system constrained at least on mac 10.7
    WelsSnprintf (ename, SEM_NAME_MAX, "ee%d%s", iIdx, pSmt->eventNamespace);
    WelsEventClose (&pSmt->pExitEncodeEvent[iIdx], ename);
    WelsSnprintf (ename, SEM_NAME_MAX, "tm%d%s", iIdx, pSmt->eventNamespace);
    WelsEventClose (&pSmt->pThreadMasterEvent[iIdx], ename);
    WelsSnprintf (ename, SEM_NAME_MAX, "sc%d%s", iIdx, pSmt->eventNamespace);
    WelsEventClose (&pSmt->pSliceCodedEvent[iIdx], ename);
    WelsSnprintf (ename, SEM_NAME_MAX, "rc%d%s", iIdx, pSmt->eventNamespace);
    WelsEventClose (&pSmt->pReadySliceCodingEvent[iIdx], ename);
    WelsSnprintf (ename, SEM_NAME_MAX, "ud%d%s", iIdx, pSmt->eventNamespace);
    WelsEventClose (&pSmt->pUpdateMbListEvent[iIdx], ename);
    WelsSnprintf (ename, SEM_NAME_MAX, "fu%d%s", iIdx, pSmt->eventNamespace);
    WelsEventClose (&pSmt->pFinUpdateMbListEvent[iIdx], ename);

    ++ iIdx;
  }
  WelsSnprintf (ename, SEM_NAME_MAX, "scm%s", pSmt->eventNamespace);
  WelsEventClose (&pSmt->pSliceCodedMasterEvent, ename);

  WelsMutexDestroy (&pSmt->mutexSliceNumUpdate);
  WelsMutexDestroy (&pSmt->mutexThreadBsBufferUsage);
  WelsMutexDestroy (& ((*ppCtx)->mutexEncoderError));

  if (pSmt->pThreadPEncCtx != NULL) {
    pMa->WelsFree (pSmt->pThreadPEncCtx, "pThreadPEncCtx");
    pSmt->pThreadPEncCtx = NULL;
  }

  for (int i = 0; i < MAX_THREADS_NUM; i++) {
    if (pSmt->pThreadBsBuffer[i]) {
      pMa->WelsFree (pSmt->pThreadBsBuffer[i], "pSmt->pThreadBsBuffer");
      pSmt->pThreadBsBuffer[i] = NULL;
    }
  }
  memset (&pSmt->bThreadBsBufferUsage, 0, MAX_THREADS_NUM * sizeof (bool));

  pSliceB = (*ppCtx)->pSliceBs;
  iIdx = 0;
  while (pSliceB != NULL && iIdx < uiSliceNum) {
    pSliceB->pBsBuffer = NULL;
    pSliceB->uiSize = 0;
    pSliceB->uiBsPos = 0;
    ++ iIdx;
    ++ pSliceB;
  }
  if ((*ppCtx)->pSliceBs != NULL) {
    pMa->WelsFree ((*ppCtx)->pSliceBs, "pSliceBs");
    (*ppCtx)->pSliceBs = NULL;
  }

  if ((*ppCtx)->pTaskManage != NULL) {
    delete (*ppCtx)->pTaskManage;
    (*ppCtx)->pTaskManage = NULL;
  }

  iIdx = 0;
  while (iIdx < pCodingParam->iSpatialLayerNum) {
    if (pSmt->pSliceConsumeTime[iIdx]) {
      pMa->WelsFree (pSmt->pSliceConsumeTime[iIdx], "pSliceConsumeTime[]");
      pSmt->pSliceConsumeTime[iIdx] = NULL;
    }
    if (pSmt->pSliceComplexRatio[iIdx] != NULL) {
      pMa->WelsFree (pSmt->pSliceComplexRatio[iIdx], "pSliceComplexRatio[]");
      pSmt->pSliceComplexRatio[iIdx] = NULL;
    }
    ++ iIdx;
  }

#ifdef MT_DEBUG
  // file handle for debug
  if (pSmt->pFSliceDiff) {
    fclose (pSmt->pFSliceDiff);
    pSmt->pFSliceDiff = NULL;
  }
#endif//MT_DEBUG
  pMa->WelsFree ((*ppCtx)->pSliceThreading, "SSliceThreading");
  (*ppCtx)->pSliceThreading = NULL;
}

int32_t AppendSliceToFrameBs (sWelsEncCtx* pCtx, SLayerBSInfo* pLbi, const int32_t iSliceCount) {
  SWelsSvcCodingParam* pCodingParam     = pCtx->pSvcParam;
  SSpatialLayerConfig* pDlp             = &pCodingParam->sSpatialLayers[pCtx->uiDependencyId];
  SWelsSliceBs* pSliceBs                = NULL;
  const bool kbIsDynamicSlicingMode     = (pDlp->sSliceArgument.uiSliceMode == SM_SIZELIMITED_SLICE);

  int32_t iLayerSize    = 0;
  int32_t iNalIdxBase   = pLbi->iNalCount;
  int32_t iSliceIdx     = 0;

  if (!kbIsDynamicSlicingMode) {
    pSliceBs    = &pCtx->pSliceBs[0];
    iLayerSize  = pSliceBs->uiBsPos;    // assign with base pSlice first
    iSliceIdx   = 0;
    iNalIdxBase   = pLbi->iNalCount = 0;
    while (iSliceIdx < iSliceCount) {
      if (pSliceBs != NULL && pSliceBs->uiBsPos > 0) {
        int32_t iNalIdx = 0;
        const int32_t iCountNal = pSliceBs->iNalIndex;

#if MT_DEBUG_BS_WR
        assert (pSliceBs->bSliceCodedFlag);
#endif//MT_DEBUG_BS_WR

        if (iSliceIdx > 0) {
          // pSlice 0 bs has been written to pFrameBs yet by now, so uiSliceIdx base should be 1
          memmove (pCtx->pFrameBs + pCtx->iPosBsBuffer, pSliceBs->pBs, pSliceBs->uiBsPos); // confirmed_safe_unsafe_usage
          pCtx->iPosBsBuffer += pSliceBs->uiBsPos;

          iLayerSize += pSliceBs->uiBsPos;
        }
        while (iNalIdx < iCountNal) {
          pLbi->pNalLengthInByte[iNalIdxBase + iNalIdx] = pSliceBs->iNalLen[iNalIdx];
          ++ iNalIdx;
        }
        pLbi->iNalCount += iCountNal;
        iNalIdxBase     += iCountNal;
      }
      ++ iSliceIdx;
      ++ pSliceBs;
    }
  } else { // for SM_SIZELIMITED_SLICE
    const int32_t kiPartitionCnt        = iSliceCount;
    int32_t iPartitionIdx               = 0;

    // due partition_0 has been written to pFrameBsBuffer
    // so iLayerSize need add it
    while (iPartitionIdx < kiPartitionCnt) {
      const int32_t kiCountSlicesCoded = pCtx->pCurDqLayer->pNumSliceCodedOfPartition[iPartitionIdx];
      int32_t iIdx = 0;

      iSliceIdx = iPartitionIdx;
      while (iIdx < kiCountSlicesCoded) {
        pSliceBs = &pCtx->pSliceBs[iSliceIdx];
        if (pSliceBs != NULL && pSliceBs->uiBsPos > 0) {
          if (iSliceIdx > 0) {
            int32_t iNalIdx = 0;
            const int32_t iCountNal = pSliceBs->iNalIndex;

            memmove (pCtx->pFrameBs + pCtx->iPosBsBuffer, pSliceBs->pBs, pSliceBs->uiBsPos); // confirmed_safe_unsafe_usage
            pCtx->iPosBsBuffer += pSliceBs->uiBsPos;

            iLayerSize += pSliceBs->uiBsPos;

            while (iNalIdx < iCountNal) {
              pLbi->pNalLengthInByte[iNalIdxBase + iNalIdx] = pSliceBs->iNalLen[iNalIdx];
              ++ iNalIdx;
            }
            pLbi->iNalCount += iCountNal;
            iNalIdxBase     += iCountNal;
          } else {
            iLayerSize += pSliceBs->uiBsPos;
          }
        }

        iSliceIdx += kiPartitionCnt;
        ++ iIdx;
      }
      ++ iPartitionIdx;
    }
  }

  return iLayerSize;
}

int32_t WriteSliceBs (sWelsEncCtx* pCtx, uint8_t* pDst,
                      int32_t* pNalLen,
                      int32_t iTotalLeftLength,
                      const int32_t iSliceIdx,
                      int32_t& iSliceSize) {
  SWelsSliceBs* pSliceBs        = &pCtx->pSliceBs[iSliceIdx];
  SNalUnitHeaderExt* pNalHdrExt = &pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt;

  const int32_t kiNalCnt        = pSliceBs->iNalIndex;
  int32_t iNalIdx               = 0;
  int32_t iNalSize              = 0;
  int32_t iReturn = ENC_RETURN_SUCCESS;
  iSliceSize = 0;
  assert (kiNalCnt <= 2);
  if (kiNalCnt > 2)
    return 0;

  while (iNalIdx < kiNalCnt) {
    iNalSize = 0;
    iReturn = WelsEncodeNal (&pSliceBs->sNalList[iNalIdx], pNalHdrExt, iTotalLeftLength - iSliceSize,
                             pDst, &iNalSize);
    WELS_VERIFY_RETURN_IFNEQ (iReturn, ENC_RETURN_SUCCESS)
    pNalLen[iNalIdx] = iNalSize;
    iSliceSize += iNalSize;
    pDst += iNalSize;
    ++ iNalIdx;
  }
  pSliceBs->uiBsPos = iSliceSize;

  return iReturn;
}

// thread process for coding one pSlice
WELS_THREAD_ROUTINE_TYPE CodingSliceThreadProc (void* arg) {
  SSliceThreadPrivateData* pPrivateData = (SSliceThreadPrivateData*)arg;
  sWelsEncCtx* pEncPEncCtx      = NULL;
  SDqLayer* pCurDq              = NULL;
  SSlice* pSlice                = NULL;
  SWelsSliceBs* pSliceBs        = NULL;
  WELS_EVENT pEventsList[3];
  int32_t iEventCount           = 0;
  WELS_THREAD_ERROR_CODE iWaitRet = WELS_THREAD_ERROR_GENERAL;
  uint32_t uiThrdRet            = 0;
  int32_t iSliceSize            = 0;
  int32_t iSliceIdx             = -1;
  int32_t iThreadIdx            = -1;
  int32_t iEventIdx             = -1;
  bool bNeedPrefix              = false;
  EWelsNalUnitType eNalType     = NAL_UNIT_UNSPEC_0;
  EWelsNalRefIdc eNalRefIdc     = NRI_PRI_LOWEST;
  int32_t iReturn = ENC_RETURN_SUCCESS;

  if (NULL == pPrivateData)
    WELS_THREAD_ROUTINE_RETURN (1);

  pEncPEncCtx   = (sWelsEncCtx*)pPrivateData->pWelsPEncCtx;

  iThreadIdx    = pPrivateData->iThreadIndex;
  iEventIdx     = iThreadIdx;

  pEventsList[iEventCount++] = pEncPEncCtx->pSliceThreading->pReadySliceCodingEvent[iEventIdx];
  pEventsList[iEventCount++] = pEncPEncCtx->pSliceThreading->pExitEncodeEvent[iEventIdx];
  pEventsList[iEventCount++] = pEncPEncCtx->pSliceThreading->pUpdateMbListEvent[iEventIdx];

  WelsThreadSetName ("OpenH264Enc_CodingSliceThreadProc");

  do {
    MT_TRACE_LOG (pEncPEncCtx, WELS_LOG_INFO,
                  "[MT] CodingSliceThreadProc(), try to call WelsMultipleEventsWaitSingleBlocking(pEventsList= %p %p %p), pEncPEncCtx= %p!",
                  pEventsList[0], pEventsList[1], pEventsList[1], (void*)pEncPEncCtx);
    iWaitRet = WelsMultipleEventsWaitSingleBlocking (iEventCount,
               &pEventsList[0],
               &pEncPEncCtx->pSliceThreading->pThreadMasterEvent[iEventIdx]); // blocking until at least one event is signalled
    if (WELS_THREAD_ERROR_WAIT_OBJECT_0 == iWaitRet) { // start pSlice coding signal waited
      int             iLayerIndex  = pEncPEncCtx->pOut->iLayerBsIndex;
      SFrameBSInfo*   pFrameBsInfo = pPrivateData->pFrameBsInfo;
      SLayerBSInfo*   pLbi = &pFrameBsInfo->sLayerInfo [iLayerIndex];

      const int32_t kiCurDid            = pEncPEncCtx->uiDependencyId;
      SWelsSvcCodingParam* pCodingParam = pEncPEncCtx->pSvcParam;
      SSpatialLayerConfig* pParamD      = &pCodingParam->sSpatialLayers[kiCurDid];

      pCurDq            = pEncPEncCtx->pCurDqLayer;
      eNalType          = pEncPEncCtx->eNalType;
      eNalRefIdc        = pEncPEncCtx->eNalPriority;
      bNeedPrefix       = pEncPEncCtx->bNeedPrefixNalFlag;

      if (pParamD->sSliceArgument.uiSliceMode != SM_SIZELIMITED_SLICE) {
        int64_t iSliceStart = 0;
        bool bDsaFlag = false;
        iSliceIdx               = pPrivateData->iSliceIndex;
        pSlice                  = &pCurDq->sLayerInfo.pSliceInLayer[iSliceIdx];
        pSliceBs                = &pEncPEncCtx->pSliceBs[iSliceIdx];

        bDsaFlag = ((pParamD->sSliceArgument.uiSliceMode == SM_FIXEDSLCNUM_SLICE) &&
                    pCodingParam->iMultipleThreadIdc > 1 &&
                    pCodingParam->iMultipleThreadIdc >= pParamD->sSliceArgument.uiSliceNum);
        if (bDsaFlag)
          iSliceStart = WelsTime();

        pSliceBs->uiBsPos       = 0;
        pSliceBs->iNalIndex     = 0;
        assert ((void*) (&pSliceBs->sBsWrite) == (void*)pSlice->pSliceBsa);
        InitBits (&pSliceBs->sBsWrite, pSliceBs->pBsBuffer, pSliceBs->uiSize);

#if MT_DEBUG_BS_WR
        pSliceBs->bSliceCodedFlag = false;
#endif//MT_DEBUG_BS_WR

        if (bNeedPrefix) {
          if (eNalRefIdc != NRI_PRI_LOWEST) {
            WelsLoadNalForSlice (pSliceBs, NAL_UNIT_PREFIX, eNalRefIdc);
            WelsWriteSVCPrefixNal (&pSliceBs->sBsWrite, eNalRefIdc, (NAL_UNIT_CODED_SLICE_IDR == eNalType));
            WelsUnloadNalForSlice (pSliceBs);
          } else { // No Prefix NAL Unit RBSP syntax here, but need add NAL Unit Header extension
            WelsLoadNalForSlice (pSliceBs, NAL_UNIT_PREFIX, eNalRefIdc);
            // No need write any syntax of prefix NAL Unit RBSP here
            WelsUnloadNalForSlice (pSliceBs);
          }
        }

        WelsLoadNalForSlice (pSliceBs, eNalType, eNalRefIdc);

        iReturn = WelsCodeOneSlice (pEncPEncCtx, iSliceIdx, eNalType);
        if (ENC_RETURN_SUCCESS != iReturn) {
          uiThrdRet = iReturn;
          WELS_THREAD_SIGNAL_AND_BREAK (pEncPEncCtx->pSliceThreading->pSliceCodedEvent,
                                        pEncPEncCtx->pSliceThreading->pSliceCodedMasterEvent,
                                        iEventIdx);
        }

        WelsUnloadNalForSlice (pSliceBs);

        int32_t iLeftBufferSize = (iSliceIdx > 0) ?
                                  (pSliceBs->uiSize - pSliceBs->uiBsPos)
                                  : (pEncPEncCtx->iFrameBsSize - pEncPEncCtx->iPosBsBuffer);
        iReturn = WriteSliceBs (pEncPEncCtx, pSliceBs->pBs,
                                &pSliceBs->iNalLen[0],
                                iLeftBufferSize,
                                iSliceIdx, iSliceSize);
        if (ENC_RETURN_SUCCESS != iReturn) {
          uiThrdRet = iReturn;
          WELS_THREAD_SIGNAL_AND_BREAK (pEncPEncCtx->pSliceThreading->pSliceCodedEvent,
                                        pEncPEncCtx->pSliceThreading->pSliceCodedMasterEvent,
                                        iEventIdx);
        }
        if (0 == iSliceIdx) {
          pEncPEncCtx->iPosBsBuffer += iSliceSize;
        }

        pEncPEncCtx->pFuncList->pfDeblocking.pfDeblockingFilterSlice (pCurDq, pEncPEncCtx->pFuncList, iSliceIdx);

        if (bDsaFlag) {
          pEncPEncCtx->pSliceThreading->pSliceConsumeTime[pEncPEncCtx->uiDependencyId][iSliceIdx] = (uint32_t) (
                WelsTime() - iSliceStart);
          MT_TRACE_LOG (& (pEncPEncCtx->sLogCtx), WELS_LOG_INFO,
                        "[MT] CodingSliceThreadProc(), coding_idx %d, uiSliceIdx %d, pSliceConsumeTime %d, iSliceSize %d, pFirstMbInSlice %d, count_num_mb_in_slice %d",
                        pEncPEncCtx->iCodingIndex, iSliceIdx,
                        pEncPEncCtx->pSliceThreading->pSliceConsumeTime[pEncPEncCtx->uiDependencyId][iSliceIdx], iSliceSize,
                        pCurDq->pSliceEncCtx->pFirstMbInSlice[iSliceIdx], pCurDq->pSliceEncCtx->pCountMbNumInSlice[iSliceIdx]);
        }

#if defined(SLICE_INFO_OUTPUT)
        fprintf (stderr,
                 "@pSlice=%-6d sliceType:%c idc:%d size:%-6d\n",
                 iSliceIdx,
                 (pEncPEncCtx->eSliceType == P_SLICE ? 'P' : 'I'),
                 eNalRefIdc,
                 iSliceSize
                );
#endif//SLICE_INFO_OUTPUT

#if MT_DEBUG_BS_WR
        pSliceBs->bSliceCodedFlag = true;
#endif//MT_DEBUG_BS_WR

        WelsEventSignal (
          &pEncPEncCtx->pSliceThreading->pSliceCodedEvent[iEventIdx]); // mean finished coding current pSlice
        WelsEventSignal (
          &pEncPEncCtx->pSliceThreading->pSliceCodedMasterEvent);
      } else { // for SM_SIZELIMITED_SLICE parallelization
        SSliceCtx* pSliceCtx                    = pCurDq->pSliceEncCtx;
        const int32_t kiPartitionId             = iThreadIdx;
        const int32_t kiSliceIdxStep            = pEncPEncCtx->iActiveThreadsNum;
        const int32_t kiFirstMbInPartition      = pPrivateData->iStartMbIndex;  // inclusive
        const int32_t kiEndMbInPartition        = pPrivateData->iEndMbIndex;            // exclusive
        int32_t iAnyMbLeftInPartition           = kiEndMbInPartition - kiFirstMbInPartition;

        iSliceIdx = pPrivateData->iSliceIndex;

        pSliceCtx->pFirstMbInSlice[iSliceIdx]                   = kiFirstMbInPartition;
        pCurDq->pNumSliceCodedOfPartition[kiPartitionId]        =
          1;    // one pSlice per partition intialized, dynamic slicing inside
        pCurDq->pLastMbIdxOfPartition[kiPartitionId]            = kiEndMbInPartition - 1;

        pCurDq->pLastCodedMbIdxOfPartition[kiPartitionId]       = 0;

        while (iAnyMbLeftInPartition > 0) {
          if (iSliceIdx >= pSliceCtx->iMaxSliceNumConstraint) {
            // TODO: need exception handler for not large enough of MAX_SLICES_NUM related memory usage
            // No idea about its solution due MAX_SLICES_NUM is fixed lenght in relevent pData structure
            uiThrdRet = 1;
            WELS_THREAD_SIGNAL_AND_BREAK (pEncPEncCtx->pSliceThreading->pSliceCodedEvent,
                                          pEncPEncCtx->pSliceThreading->pSliceCodedMasterEvent,
                                          iEventIdx);
          }

          SetOneSliceBsBufferUnderMultithread (pEncPEncCtx, kiPartitionId, iSliceIdx);
          pSlice                = &pCurDq->sLayerInfo.pSliceInLayer[iSliceIdx];
          pSliceBs              = &pEncPEncCtx->pSliceBs[iSliceIdx];

          pSliceBs->uiBsPos     = 0;
          pSliceBs->iNalIndex   = 0;
          InitBits (&pSliceBs->sBsWrite, pSliceBs->pBsBuffer, pSliceBs->uiSize);

          if (bNeedPrefix) {
            if (eNalRefIdc != NRI_PRI_LOWEST) {
              WelsLoadNalForSlice (pSliceBs, NAL_UNIT_PREFIX, eNalRefIdc);
              WelsWriteSVCPrefixNal (&pSliceBs->sBsWrite, eNalRefIdc, (NAL_UNIT_CODED_SLICE_IDR == eNalType));
              WelsUnloadNalForSlice (pSliceBs);
            } else { // No Prefix NAL Unit RBSP syntax here, but need add NAL Unit Header extension
              WelsLoadNalForSlice (pSliceBs, NAL_UNIT_PREFIX, eNalRefIdc);
              // No need write any syntax of prefix NAL Unit RBSP here
              WelsUnloadNalForSlice (pSliceBs);
            }
          }


          WelsLoadNalForSlice (pSliceBs, eNalType, eNalRefIdc);

          iReturn = WelsCodeOneSlice (pEncPEncCtx, iSliceIdx, eNalType);
          if (ENC_RETURN_SUCCESS != iReturn) {
            uiThrdRet = iReturn;
            WELS_THREAD_SIGNAL_AND_BREAK (pEncPEncCtx->pSliceThreading->pSliceCodedEvent,
                                          pEncPEncCtx->pSliceThreading->pSliceCodedMasterEvent,
                                          iEventIdx);
          }

          WelsUnloadNalForSlice (pSliceBs);

          if (0 == iSliceIdx) {
            iReturn = WriteSliceBs (pEncPEncCtx, pLbi->pBsBuf,
                                    &pLbi->pNalLengthInByte[pLbi->iNalCount],
                                    pEncPEncCtx->iFrameBsSize - pEncPEncCtx->iPosBsBuffer,
                                    iSliceIdx, iSliceSize);
            pLbi->iNalCount    += pSliceBs->iNalIndex;
            if (ENC_RETURN_SUCCESS != iReturn) {
              uiThrdRet = iReturn;
              WELS_THREAD_SIGNAL_AND_BREAK (pEncPEncCtx->pSliceThreading->pSliceCodedEvent,
                                            pEncPEncCtx->pSliceThreading->pSliceCodedMasterEvent,
                                            iEventIdx);
            }
            pEncPEncCtx->iPosBsBuffer += iSliceSize;
          } else {
            iReturn = WriteSliceBs (pEncPEncCtx, pSliceBs->pBs, &pSliceBs->iNalLen[0],
                                    pSliceBs->uiSize - pSliceBs->uiBsPos,
                                    iSliceIdx, iSliceSize);
            if (ENC_RETURN_SUCCESS != iReturn) {
              uiThrdRet = iReturn;
              WELS_THREAD_SIGNAL_AND_BREAK (pEncPEncCtx->pSliceThreading->pSliceCodedEvent,
                                            pEncPEncCtx->pSliceThreading->pSliceCodedMasterEvent,
                                            iEventIdx);
            }
          }

          pEncPEncCtx->pFuncList->pfDeblocking.pfDeblockingFilterSlice (pCurDq, pEncPEncCtx->pFuncList, iSliceIdx);

#if defined(SLICE_INFO_OUTPUT)
          fprintf (stderr,
                   "@pSlice=%-6d sliceType:%c idc:%d size:%-6d\n",
                   iSliceIdx,
                   (pEncPEncCtx->eSliceType == P_SLICE ? 'P' : 'I'),
                   eNalRefIdc,
                   iSliceSize
                  );
#endif//SLICE_INFO_OUTPUT

          MT_TRACE_LOG (pEncPEncCtx, WELS_LOG_INFO,
                        "[MT] CodingSliceThreadProc(), coding_idx %d, iPartitionId %d, uiSliceIdx %d, iSliceSize %d, count_mb_slice %d, iEndMbInPartition %d, pCurDq->pLastCodedMbIdxOfPartition[%d] %d\n",
                        pEncPEncCtx->iCodingIndex, kiPartitionId, iSliceIdx, iSliceSize, pCurDq->pSliceEncCtx->pCountMbNumInSlice[iSliceIdx],
                        kiEndMbInPartition, kiPartitionId, pCurDq->pLastCodedMbIdxOfPartition[kiPartitionId]);

          iAnyMbLeftInPartition = kiEndMbInPartition - (1 + pCurDq->pLastCodedMbIdxOfPartition[kiPartitionId]);
          iSliceIdx += kiSliceIdxStep;
        }

        if (uiThrdRet) { // any exception??
          WELS_THREAD_SIGNAL_AND_BREAK (pEncPEncCtx->pSliceThreading->pSliceCodedEvent,
                                        pEncPEncCtx->pSliceThreading->pSliceCodedMasterEvent,
                                        iEventIdx);
        }

        WelsEventSignal (&pEncPEncCtx->pSliceThreading->pSliceCodedEvent[iEventIdx]); // mean finished coding current pSlice
        WelsEventSignal (&pEncPEncCtx->pSliceThreading->pSliceCodedMasterEvent);
      }
    } else if (WELS_THREAD_ERROR_WAIT_OBJECT_0 + 1 == iWaitRet) { // exit thread signal
      uiThrdRet = 0;
      break;
    } else if (WELS_THREAD_ERROR_WAIT_OBJECT_0 + 2 == iWaitRet) { // update pMb list singal
      iSliceIdx =
        iEventIdx; // pPrivateData->iSliceIndex; old threads can not be terminated, pPrivateData is not correct for applicable
      pCurDq = pEncPEncCtx->pCurDqLayer;
      UpdateMbListNeighborParallel (pCurDq, pCurDq->sMbDataP, iSliceIdx);
      WelsEventSignal (
        &pEncPEncCtx->pSliceThreading->pFinUpdateMbListEvent[iEventIdx]); // mean finished update pMb list for this pSlice
    } else { // WELS_THREAD_ERROR_WAIT_TIMEOUT, or WELS_THREAD_ERROR_WAIT_FAILED
      WelsLog (& (pEncPEncCtx->sLogCtx), WELS_LOG_WARNING,
               "[MT] CodingSliceThreadProc(), waiting pReadySliceCodingEvent[%d] failed(%d) and thread%d terminated!", iEventIdx,
               iWaitRet, iThreadIdx);
      uiThrdRet = 1;
      break;
    }
  } while (1);

  //sync multi-threading error
  WelsMutexLock (&pEncPEncCtx->mutexEncoderError);
  if (uiThrdRet) pEncPEncCtx->iEncoderError |= uiThrdRet;
  WelsMutexUnlock (&pEncPEncCtx->mutexEncoderError);

  WELS_THREAD_ROUTINE_RETURN (uiThrdRet);
}

int32_t CreateSliceThreads (sWelsEncCtx* pCtx) {
  const int32_t kiThreadCount = pCtx->pSvcParam->iCountThreadsNum;
  int32_t iIdx = 0;

  while (iIdx < kiThreadCount) {
    if (WelsThreadCreate (&pCtx->pSliceThreading->pThreadHandles[iIdx], CodingSliceThreadProc,
                          &pCtx->pSliceThreading->pThreadPEncCtx[iIdx], 0)) {
      return 1;
    }


    ++ iIdx;
  }
  MT_TRACE_LOG (pCtx, WELS_LOG_INFO, "CreateSliceThreads() exit..");
  return 0;
}

int32_t FiredSliceThreads (sWelsEncCtx* pCtx, SSliceThreadPrivateData* pPriData, WELS_EVENT* pEventsList,
                           WELS_EVENT* pMasterEventsList, SFrameBSInfo* pFrameBsInfo,
                           const uint32_t uiNumThreads, SSliceCtx* pSliceCtx, const bool bIsDynamicSlicingMode) {
  int32_t iEndMbIdx         = 0;
  int32_t iIdx              = 0;
  const int32_t kiEventCnt  = uiNumThreads;
  int32_t       iLayerBsIdx = pCtx->pOut->iLayerBsIndex;
  SLayerBSInfo* pLbi = &pFrameBsInfo->sLayerInfo[iLayerBsIdx];

  if (pPriData == NULL || pFrameBsInfo == NULL || pLbi == NULL || kiEventCnt <= 0 || pEventsList == NULL) {
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_ERROR,
             "FiredSliceThreads(), fail due pPriData == %p ||pFrameBsInfo == %p || pLbi == %p || iEventCnt(%d) <= 0 || pEventsList == %p!!",
             (void*)pPriData, (void*)pFrameBsInfo, (void*)pLbi, uiNumThreads, (void*)pEventsList);
    return 1;
  }

  ////////////////////////////////////////
  if (bIsDynamicSlicingMode) {
    iEndMbIdx = pSliceCtx->iMbNumInFrame;
    for (iIdx = kiEventCnt - 1; iIdx >= 0; --iIdx) {
      const int32_t iFirstMbIdx         = pSliceCtx->pFirstMbInSlice[iIdx];
      pPriData[iIdx].iStartMbIndex      = iFirstMbIdx;
      pPriData[iIdx].iEndMbIndex        = iEndMbIdx;
      iEndMbIdx                         = iFirstMbIdx;
    }
  }

  pLbi->pBsBuf = pCtx->pFrameBs + pCtx->iPosBsBuffer;
  pLbi->uiLayerType   = VIDEO_CODING_LAYER;
  pLbi->uiSpatialId   = pCtx->uiDependencyId;
  pLbi->uiTemporalId  = pCtx->uiTemporalId;
  pLbi->uiQualityId   = 0;
  pLbi->iNalCount     = 0;
  pCtx->pSliceBs[0].pBs = pLbi->pBsBuf;

  iIdx = 0;
  while (iIdx < kiEventCnt) {
    pPriData[iIdx].pFrameBsInfo = pFrameBsInfo;
    pPriData[iIdx].iSliceIndex  = iIdx;
    SetOneSliceBsBufferUnderMultithread (pCtx, iIdx, iIdx);
    if (pEventsList[iIdx])
      WelsEventSignal (&pEventsList[iIdx]);
    if (pMasterEventsList[iIdx])
      WelsEventSignal (&pMasterEventsList[iIdx]);
    ++ iIdx;
  }

  return 0;
}

int32_t DynamicDetectCpuCores() {
  WelsLogicalProcessInfo  info;
  WelsQueryLogicalProcessInfo (&info);
  return info.ProcessorCount;
}

int32_t AdjustBaseLayer (sWelsEncCtx* pCtx) {
  SDqLayer* pCurDq      = pCtx->ppDqLayerList[0];
  int32_t iNeedAdj      = 1;
#ifdef MT_DEBUG
  int64_t iT0 = WelsTime();
#endif//MT_DEBUG

  pCtx->pCurDqLayer = pCurDq;
  memcpy ((pCtx->pSliceThreading->pSliceConsumeTime[0]), (pCurDq->pSliceEncCtx->pSliceConsumeTime),
          pCurDq->pSliceEncCtx->iSliceNumInFrame * sizeof (uint32_t));
  // do not need adjust due to not different at both slices of consumed time
  iNeedAdj = NeedDynamicAdjust (pCtx->pSliceThreading->pSliceConsumeTime[0], pCurDq->pSliceEncCtx->iSliceNumInFrame);
  if (iNeedAdj)
    DynamicAdjustSlicing (pCtx,
                          pCurDq,
                          pCtx->pSliceThreading->pSliceComplexRatio[0],
                          0);
#ifdef MT_DEBUG
  iT0 = WelsTime() - iT0;
  if (pCtx->pSliceThreading->pFSliceDiff) {
    fprintf (pCtx->pSliceThreading->pFSliceDiff,
             "%6"PRId64" us adjust time at base spatial layer, iNeedAdj %d, DynamicAdjustSlicing()\n",
             iT0, iNeedAdj);
  }
#endif//MT_DEBUG

  return iNeedAdj;
}

int32_t AdjustEnhanceLayer (sWelsEncCtx* pCtx, int32_t iCurDid) {
#ifdef MT_DEBUG
  int64_t iT1 = WelsTime();
#endif//MT_DEBUG
  int32_t iNeedAdj = 1;
  // uiSliceMode of referencing spatial should be SM_FIXEDSLCNUM_SLICE
  // if using spatial base layer for complexity estimation

  const bool kbModelingFromSpatial = (pCtx->pCurDqLayer->pRefLayer != NULL && iCurDid > 0)
                                     && (pCtx->pSvcParam->sSpatialLayers[iCurDid - 1].sSliceArgument.uiSliceMode == SM_FIXEDSLCNUM_SLICE
                                         && pCtx->pSvcParam->iMultipleThreadIdc >= pCtx->pSvcParam->sSpatialLayers[iCurDid -
                                             1].sSliceArgument.uiSliceNum);
  memcpy ((pCtx->pSliceThreading->pSliceConsumeTime[iCurDid]), (pCtx->pCurDqLayer->pSliceEncCtx->pSliceConsumeTime),
          pCtx->pCurDqLayer->pSliceEncCtx->iSliceNumInFrame * sizeof (uint32_t));

  if (kbModelingFromSpatial) { // using spatial base layer for complexity estimation
    // do not need adjust due to not different at both slices of consumed time
    iNeedAdj = NeedDynamicAdjust (pCtx->pSliceThreading->pSliceConsumeTime[iCurDid - 1],
                                  pCtx->pCurDqLayer->pSliceEncCtx->iSliceNumInFrame);
    if (iNeedAdj)
      DynamicAdjustSlicing (pCtx,
                            pCtx->pCurDqLayer,
                            pCtx->pSliceThreading->pSliceComplexRatio[iCurDid - 1],
                            iCurDid
                           );
  } else { // use temporal layer for complexity estimation
    // do not need adjust due to not different at both slices of consumed time
    iNeedAdj = NeedDynamicAdjust (pCtx->pSliceThreading->pSliceConsumeTime[iCurDid],
                                  pCtx->pCurDqLayer->pSliceEncCtx->iSliceNumInFrame);
    if (iNeedAdj)
      DynamicAdjustSlicing (pCtx,
                            pCtx->pCurDqLayer,
                            pCtx->pSliceThreading->pSliceComplexRatio[iCurDid],
                            iCurDid
                           );
  }

#ifdef MT_DEBUG
  iT1 = WelsTime() - iT1;
  if (pCtx->pSliceThreading->pFSliceDiff) {
    fprintf (pCtx->pSliceThreading->pFSliceDiff,
             "%6"PRId64" us adjust time at spatial layer %d, iNeedAdj %d, DynamicAdjustSlicing()\n",
             iT1, iCurDid, iNeedAdj);
  }
#endif//MT_DEBUG

  return iNeedAdj;
}



#if defined(MT_DEBUG)
void TrackSliceComplexities (sWelsEncCtx* pCtx, const int32_t iCurDid) {
  const int32_t kiCountSliceNum = pCtx->pCurDqLayer->pSliceEncCtx->iSliceNumInFrame;
  if (kiCountSliceNum > 0) {
    int32_t iSliceIdx = 0;
    do {
      fprintf (pCtx->pSliceThreading->pFSliceDiff, "%6.3f complexity pRatio at iDid %d pSlice %d\n",
               pCtx->pSliceThreading->pSliceComplexRatio[iCurDid][iSliceIdx], iCurDid, iSliceIdx);
      ++ iSliceIdx;
    } while (iSliceIdx < kiCountSliceNum);
  }
}
#endif

#if defined(MT_DEBUG)
void TrackSliceConsumeTime (sWelsEncCtx* pCtx, int32_t* pDidList, const int32_t iSpatialNum) {
  SWelsSvcCodingParam* pPara = NULL;
  int32_t iSpatialIdx = 0;

  if (iSpatialNum > MAX_DEPENDENCY_LAYER)
    return;

  pPara = pCtx->pSvcParam;
  while (iSpatialIdx < iSpatialNum) {
    const int32_t kiDid         = pDidList[iSpatialIdx];
    SSpatialLayerInternal* pDlp = &pPara->sDependencyLayers[kiDid];
    SSliceConfig* pSliceArgument          = &pDlp->sSliceArgument;
    SDqLayer* pCurDq            = pCtx->ppDqLayerList[kiDid];
    SSliceCtx* pSliceCtx = pCurDq->pSliceEncCtx;
    const uint32_t kuiCountSliceNum = pSliceCtx->iSliceNumInFrame;
    if (pCtx->pSliceThreading) {
      if (pCtx->pSliceThreading->pFSliceDiff
          && ((pSliceArgument->uiSliceMode == SM_FIXEDSLCNUM_SLICE) || (pSliceArgument->uiSliceMode == SM_AUTO_SLICE))
          && pPara->iMultipleThreadIdc > 1
          && pPara->iMultipleThreadIdc >= kuiCountSliceNum) {
        uint32_t i = 0;
        uint32_t uiMaxT = 0;
        int32_t iMaxI = 0;
        while (i < kuiCountSliceNum) {
          if (pCtx->pSliceThreading->pSliceConsumeTime[kiDid] != NULL)
            fprintf (pCtx->pSliceThreading->pFSliceDiff, "%6d us consume_time coding_idx %d iDid %d pSlice %d\n",
                     pCtx->pSliceThreading->pSliceConsumeTime[kiDid][i], pCtx->iCodingIndex, kiDid, i /*/ 1000*/);
          if (pCtx->pSliceThreading->pSliceConsumeTime[kiDid][i] > uiMaxT) {
            uiMaxT = pCtx->pSliceThreading->pSliceConsumeTime[kiDid][i];
            iMaxI = i;
          }
          ++ i;
        }
        fprintf (pCtx->pSliceThreading->pFSliceDiff, "%6d us consume_time_max coding_idx %d iDid %d pSlice %d\n", uiMaxT,
                 pCtx->iCodingIndex, kiDid, iMaxI /*/ 1000*/);
      }
    }
    ++ iSpatialIdx;
  }
}
#endif//#if defined(MT_DEBUG)

void SetOneSliceBsBufferUnderMultithread (sWelsEncCtx* pCtx, const int32_t kiThreadIdx, const int32_t iSliceIdx) {
  pCtx->pSliceBs[iSliceIdx].pBsBuffer = pCtx->pSliceThreading->pThreadBsBuffer[kiThreadIdx];
  pCtx->pSliceBs[iSliceIdx].uiBsPos = 0;
  //printf("SetOneSliceBsBufferUnderMultithread, thread %d, slice %d, buffer=%x\n", kiThreadIdx, iSliceIdx, pCtx->pSliceBs[iSliceIdx].pBsBuffer);
}
}

