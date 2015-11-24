/*!
 * \copy
 *     Copyright (c)  2009-2015, Cisco Systems
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
 * \file    wels_task_encoder.h
 *
 * \brief   interface for encoder tasks
 *
 * \date    07/06/2015 Created
 *
 *************************************************************************************
 */

#include <string.h>
#include <assert.h>

#include "typedefs.h"
#include "utils.h"
#include "measure_time.h"
#include "WelsTask.h"

#include "wels_task_base.h"
#include "wels_task_encoder.h"

#include "svc_enc_golomb.h"
#include "svc_encode_slice.h"
#include "slice_multi_threading.h"

namespace WelsEnc {

CWelsSliceEncodingTask::CWelsSliceEncodingTask (sWelsEncCtx* pCtx, const int32_t iSliceIdx) : m_eTaskResult(ENC_RETURN_SUCCESS) {
  m_pCtx = pCtx;
  m_iSliceIdx = iSliceIdx;
}

CWelsSliceEncodingTask::~CWelsSliceEncodingTask() {
}

WelsErrorType CWelsSliceEncodingTask::Execute() {
  WelsThreadSetName ("OpenH264Enc_CWelsSliceEncodingTask_Execute");

  m_eTaskResult = InitTask();
  WELS_VERIFY_RETURN_IFNEQ (m_eTaskResult, ENC_RETURN_SUCCESS)

  m_eTaskResult = ExecuteTask();

  FinishTask();
  return m_eTaskResult;
}

WelsErrorType CWelsSliceEncodingTask::SetBoundary (int32_t iStartIdx,  int32_t iEndIdx) {
  m_iStartMbIdx = iStartIdx;
  m_iEndMbIdx = iEndIdx;
  return ENC_RETURN_SUCCESS;
}

int32_t CWelsSliceEncodingTask::QueryEmptyThread (bool* pThreadBsBufferUsage) {
  for (int32_t k = 0; k < MAX_THREADS_NUM; k++) {
    if (pThreadBsBufferUsage[k] == false) {
      pThreadBsBufferUsage[k] = true;
      return k;
    }
  }
  return -1;
}

WelsErrorType CWelsSliceEncodingTask::InitTask() {
  m_eNalType          = m_pCtx->eNalType;
  m_eNalRefIdc        = m_pCtx->eNalPriority;
  m_bNeedPrefix       = m_pCtx->bNeedPrefixNalFlag;

  WelsMutexLock (&m_pCtx->pSliceThreading->mutexThreadBsBufferUsage);
  m_iThreadIdx = QueryEmptyThread (m_pCtx->pSliceThreading->bThreadBsBufferUsage);
  WelsMutexUnlock (&m_pCtx->pSliceThreading->mutexThreadBsBufferUsage);

  WelsLog (&m_pCtx->sLogCtx, WELS_LOG_DEBUG,
           "[MT] CWelsSliceEncodingTask()InitTask for m_iSliceIdx %d, lock thread %d",
           m_iSliceIdx, m_iThreadIdx);
  if (m_iThreadIdx < 0) {
    WelsLog (&m_pCtx->sLogCtx, WELS_LOG_WARNING,
             "[MT] CWelsSliceEncodingTask InitTask(), Cannot find available thread for m_iSliceIdx = %d", m_iSliceIdx);
    return ENC_RETURN_UNEXPECTED;
  }
  SetOneSliceBsBufferUnderMultithread (m_pCtx, m_iThreadIdx, m_iSliceIdx);

  m_pSlice = &m_pCtx->pCurDqLayer->sLayerInfo.pSliceInLayer[m_iSliceIdx];
  m_pSliceBs = &m_pCtx->pSliceBs[m_iSliceIdx];

  m_pSliceBs->uiBsPos       = 0;
  m_pSliceBs->iNalIndex     = 0;

  assert ((void*) (&m_pSliceBs->sBsWrite) == (void*)m_pSlice->pSliceBsa);
  InitBits (&m_pSliceBs->sBsWrite, m_pSliceBs->pBsBuffer, m_pSliceBs->uiSize);
  //printf ("CWelsSliceEncodingTask_InitTask slice %d\n", m_iSliceIdx);

  return ENC_RETURN_SUCCESS;
}

void CWelsSliceEncodingTask::FinishTask() {
  WelsMutexLock (&m_pCtx->pSliceThreading->mutexThreadBsBufferUsage);
  m_pCtx->pSliceThreading->bThreadBsBufferUsage[m_iThreadIdx] = false;
  WelsMutexUnlock (&m_pCtx->pSliceThreading->mutexThreadBsBufferUsage);

  WelsLog (&m_pCtx->sLogCtx, WELS_LOG_DEBUG,
           "[MT] CWelsSliceEncodingTask()FinishTask for m_iSliceIdx %d, unlock thread %d",
           m_iSliceIdx, m_iThreadIdx);

  //sync multi-threading error
  WelsMutexLock (&m_pCtx->mutexEncoderError);
  if (ENC_RETURN_SUCCESS != m_eTaskResult) {
    m_pCtx->iEncoderError |= m_eTaskResult;
  }
  WelsMutexUnlock (&m_pCtx->mutexEncoderError);
}

WelsErrorType CWelsSliceEncodingTask::ExecuteTask() {
#if MT_DEBUG_BS_WR
  m_pSliceBs->bSliceCodedFlag = false;
#endif//MT_DEBUG_BS_WR

  if (m_bNeedPrefix) {
    if (m_eNalRefIdc != NRI_PRI_LOWEST) {
      WelsLoadNalForSlice (m_pSliceBs, NAL_UNIT_PREFIX, m_eNalRefIdc);
      WelsWriteSVCPrefixNal (&m_pSliceBs->sBsWrite, m_eNalRefIdc, (NAL_UNIT_CODED_SLICE_IDR == m_eNalType));
      WelsUnloadNalForSlice (m_pSliceBs);
    } else { // No Prefix NAL Unit RBSP syntax here, but need add NAL Unit Header extension
      WelsLoadNalForSlice (m_pSliceBs, NAL_UNIT_PREFIX, m_eNalRefIdc);
      // No need write any syntax of prefix NAL Unit RBSP here
      WelsUnloadNalForSlice (m_pSliceBs);
    }
  }

  WelsLoadNalForSlice (m_pSliceBs, m_eNalType, m_eNalRefIdc);
  int32_t iReturn = WelsCodeOneSlice (m_pCtx, m_iSliceIdx, m_eNalType);
  if (ENC_RETURN_SUCCESS != iReturn) {
    return iReturn;
  }
  WelsUnloadNalForSlice (m_pSliceBs);

  m_iSliceSize = 0;
  int32_t iLeftBufferSize = (m_iSliceIdx > 0) ? (m_pSliceBs->uiSize - (int32_t) (m_pSliceBs->sBsWrite.pCurBuf -
                            m_pSliceBs->sBsWrite.pStartBuf)) : (m_pCtx->iFrameBsSize - m_pCtx->iPosBsBuffer);
  iReturn = WriteSliceBs (m_pCtx, m_pSliceBs->pBs,
                          &m_pSliceBs->iNalLen[0],
                          iLeftBufferSize,
                          m_iSliceIdx, m_iSliceSize);
  if (ENC_RETURN_SUCCESS != iReturn) {
    WelsLog (&m_pCtx->sLogCtx, WELS_LOG_WARNING,
             "[MT] CWelsSliceEncodingTask ExecuteTask(), WriteSliceBs not successful: coding_idx %d, um_iSliceIdx %d",
             m_pCtx->iCodingIndex,
             m_iSliceIdx);
    return iReturn;
  }
  if (0 == m_iSliceIdx) {
    m_pCtx->iPosBsBuffer += m_iSliceSize;
  }

  m_pCtx->pFuncList->pfDeblocking.pfDeblockingFilterSlice (m_pCtx->pCurDqLayer, m_pCtx->pFuncList, m_iSliceIdx);

  WelsLog (&m_pCtx->sLogCtx, WELS_LOG_DETAIL,
           "@pSlice=%-6d sliceType:%c idc:%d size:%-6d",  m_iSliceIdx,
           (m_pCtx->eSliceType == P_SLICE ? 'P' : 'I'),
           m_eNalRefIdc,
           m_iSliceSize);

#if MT_DEBUG_BS_WR
  m_pSliceBs->bSliceCodedFlag = true;
#endif//MT_DEBUG_BS_WR

  return ENC_RETURN_SUCCESS;
}


// CWelsLoadBalancingSlicingEncodingTask
WelsErrorType CWelsLoadBalancingSlicingEncodingTask::InitTask() {
  WelsErrorType iReturn = CWelsSliceEncodingTask::InitTask();
  if (ENC_RETURN_SUCCESS != iReturn) {
    return iReturn;
  }

  m_iSliceStart = WelsTime();
  WelsLog (&m_pCtx->sLogCtx, WELS_LOG_DEBUG,
           "[MT] CWelsLoadBalancingSlicingEncodingTask()InitTask for m_iSliceIdx %d at %" PRId64,
           m_iSliceIdx, m_iSliceStart);

  return ENC_RETURN_SUCCESS;
}

void CWelsLoadBalancingSlicingEncodingTask::FinishTask() {
  CWelsSliceEncodingTask::FinishTask();

  m_pSlice->uiSliceConsumeTime = (uint32_t) (WelsTime() - m_iSliceStart);
  WelsLog (&m_pCtx->sLogCtx, WELS_LOG_DEBUG,
           "[MT] CWelsLoadBalancingSlicingEncodingTask()FinishTask, coding_idx %d, um_iSliceIdx %d, uiSliceConsumeTime %d, iSliceSize %d, iFirstMbInSlice %d, count_num_mb_in_slice %d",
           m_pCtx->iCodingIndex,
           m_iSliceIdx,
           m_pSlice->uiSliceConsumeTime,
           m_iSliceSize,
           m_pCtx->pCurDqLayer->sLayerInfo.pSliceInLayer[m_iSliceIdx].sSliceHeaderExt.sSliceHeader.iFirstMbInSlice,
           m_pCtx->pCurDqLayer->sSliceEncCtx.pCountMbNumInSlice[m_iSliceIdx]);
}


CWelsUpdateMbMapTask::CWelsUpdateMbMapTask (sWelsEncCtx* pCtx, const int32_t iSliceIdx) {
  m_pCtx = pCtx;
  m_iSliceIdx = iSliceIdx;
}

CWelsUpdateMbMapTask::~CWelsUpdateMbMapTask() {
}

WelsErrorType CWelsUpdateMbMapTask::Execute() {
  UpdateMbListNeighborParallel (m_pCtx->pCurDqLayer, m_pCtx->pCurDqLayer->sMbDataP, m_iSliceIdx);
  return ENC_RETURN_SUCCESS;
}

}


