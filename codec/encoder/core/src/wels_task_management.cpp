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
 * \file    wels_task_management.cpp
 *
 * \brief   function for task management
 *
 * \date    5/14/2012 Created
 *
 *************************************************************************************
 */
#include <string.h>
#include <assert.h>

#include "typedefs.h"
#include "utils.h"
#include "WelsLock.h"
#include "memory_align.h"

#include "wels_common_basis.h"
#include "encoder_context.h"
#include "wels_task_base.h"
#include "wels_task_encoder.h"
#include "wels_task_management.h"

namespace WelsEnc {



IWelsTaskManage*   IWelsTaskManage::CreateTaskManage (sWelsEncCtx* pCtx, const int32_t iSpatialLayer,
    const bool bNeedLock) {
  if (NULL == pCtx) {
    return NULL;
  }

  IWelsTaskManage* pTaskManage;
  if (iSpatialLayer > 1) {
    pTaskManage = WELS_NEW_OP (CWelsTaskManageMultiD(), CWelsTaskManageMultiD);
  } else {
    pTaskManage = WELS_NEW_OP (CWelsTaskManageBase(), CWelsTaskManageBase);
  }

  if (pTaskManage) {
    pTaskManage->Init (pCtx);
  }
  return pTaskManage;
}


CWelsTaskManageBase::CWelsTaskManageBase()
  : m_pEncCtx (NULL),
    m_pThreadPool (NULL),
    m_iTotalTaskNum (0),
    m_iWaitTaskNum (0) {
  m_cEncodingTaskList = new TASKLIST_TYPE();
  m_cPreEncodingTaskList = new TASKLIST_TYPE();
  WelsEventOpen (&m_hTaskEvent);
}

CWelsTaskManageBase::~CWelsTaskManageBase() {
  //printf ("~CWelsTaskManageBase\n");
  Uninit();
}

WelsErrorType CWelsTaskManageBase::Init (sWelsEncCtx* pEncCtx) {
  m_pEncCtx = pEncCtx;

  m_iThreadNum = m_pEncCtx->pSvcParam->iMultipleThreadIdc;
  m_pThreadPool = WELS_NEW_OP (WelsCommon::CWelsThreadPool (this, m_iThreadNum),
                               WelsCommon::CWelsThreadPool);
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, NULL == m_pThreadPool)

  m_pcAllTaskList[CWelsBaseTask::WELS_ENC_TASK_ENCODING] = m_cEncodingTaskList;
  m_pcAllTaskList[CWelsBaseTask::WELS_ENC_TASK_UPDATEMBMAP] = m_cPreEncodingTaskList;

  m_iCurrentTaskNum = pEncCtx->pSvcParam->sSpatialLayers[0].sSliceArgument.uiSliceNum;
  //printf ("CWelsTaskManageBase Init m_iThreadNum %d m_iCurrentTaskNum %d pEncCtx->iMaxSliceCount %d\n", m_iThreadNum, m_iCurrentTaskNum, pEncCtx->iMaxSliceCount);
  return CreateTasks (pEncCtx, pEncCtx->iMaxSliceCount);
}

void   CWelsTaskManageBase::Uninit() {
  DestroyTasks();
  WELS_DELETE_OP (m_pThreadPool);

  delete m_cEncodingTaskList;
  delete m_cPreEncodingTaskList;
  WelsEventClose (&m_hTaskEvent);
}

WelsErrorType CWelsTaskManageBase::CreateTasks (sWelsEncCtx* pEncCtx, const int32_t kiTaskCount) {
  CWelsBaseTask* pTask = NULL;

  for (int idx = 0; idx < kiTaskCount; idx++) {
    pTask = WELS_NEW_OP (CWelsUpdateMbMapTask (pEncCtx, idx), CWelsUpdateMbMapTask);
    WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, NULL == pTask)
    m_cPreEncodingTaskList->push_back (pTask);
  }

  for (int idx = 0; idx < kiTaskCount; idx++) {
    pTask = WELS_NEW_OP (CWelsLoadBalancingSlicingEncodingTask (pEncCtx, idx), CWelsLoadBalancingSlicingEncodingTask);
    //TODO: set this after loadbalancing flagpTask = WELS_NEW_OP (CWelsSliceEncodingTask (pEncCtx, idx), CWelsSliceEncodingTask);
    WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, NULL == pTask)
    m_cEncodingTaskList->push_back (pTask);
  }
  m_iTotalTaskNum = kiTaskCount;

  //printf ("CWelsTaskManageBase CreateTasks m_iThreadNum %d kiTaskCount=%d\n", m_iThreadNum, kiTaskCount);
  return ENC_RETURN_SUCCESS;
}

void CWelsTaskManageBase::DestroyTaskList (TASKLIST_TYPE* pTargetTaskList) {
  if (pTargetTaskList->size() != m_iTotalTaskNum) {
    printf ("pTargetTaskList size=%d m_iTotalTaskNum=%d\n", static_cast<int32_t> (pTargetTaskList->size()),
            m_iTotalTaskNum);
  }
  //printf ("CWelsTaskManageBase: pTargetTaskList size=%d m_iTotalTaskNum=%d\n", static_cast<int32_t> (pTargetTaskList->size()), m_iTotalTaskNum);
  while (NULL != pTargetTaskList->begin()) {
    CWelsBaseTask* pTask = pTargetTaskList->begin();
    WELS_DELETE_OP (pTask);
    pTargetTaskList->pop_front();
  }
}

void CWelsTaskManageBase::DestroyTasks() {
  if (m_iTotalTaskNum == 0) {
    return;
  }

  DestroyTaskList (m_cEncodingTaskList);
  DestroyTaskList (m_cPreEncodingTaskList);
  //printf ("[MT] CWelsTaskManageBase() DestroyTasks, cleaned %d tasks\n", m_iTotalTaskNum);
  m_iTotalTaskNum = 0;
}

void  CWelsTaskManageBase::OnTaskMinusOne() {
  WelsCommon::CWelsAutoLock cAutoLock (m_cWaitTaskNumLock);
  m_iWaitTaskNum --;
  if (m_iWaitTaskNum <= 0) {
    WelsEventSignal (&m_hTaskEvent);
    //printf ("OnTaskMinusOne WelsEventSignal m_iWaitTaskNum=%d\n", m_iWaitTaskNum);
  }
  //printf ("OnTaskMinusOne m_iWaitTaskNum=%d\n", m_iWaitTaskNum);
}

WelsErrorType  CWelsTaskManageBase::OnTaskCancelled (WelsCommon::IWelsTask* pTask) {
  OnTaskMinusOne();
  return ENC_RETURN_SUCCESS;
}

WelsErrorType  CWelsTaskManageBase::OnTaskExecuted (WelsCommon::IWelsTask* pTask) {
  OnTaskMinusOne();
  return ENC_RETURN_SUCCESS;
}

WelsErrorType  CWelsTaskManageBase::ExecuteTaskList (TASKLIST_TYPE* pTargetTaskList) {
  m_iWaitTaskNum = m_iCurrentTaskNum;
  //printf ("ExecuteTaskList m_iWaitTaskNum=%d\n", m_iWaitTaskNum);
  if (0 == m_iWaitTaskNum) {
    return ENC_RETURN_SUCCESS;
  }

  int32_t iCurrentTaskCount = m_iWaitTaskNum; //if directly use m_iWaitTaskNum in the loop make cause sync problem
  int32_t iIdx = 0;
  while (iIdx < iCurrentTaskCount) {
    m_pThreadPool->QueueTask (pTargetTaskList->GetIndexNode (iIdx));
    iIdx ++;
  }
  WelsEventWait (&m_hTaskEvent);

  return ENC_RETURN_SUCCESS;
}

void CWelsTaskManageBase::InitFrame (const int32_t kiCurDid) {
  if (m_pEncCtx->pCurDqLayer->bNeedAdjustingSlicing) {
    ExecuteTaskList (m_pcAllTaskList[CWelsBaseTask::WELS_ENC_TASK_UPDATEMBMAP]);
  }
}

WelsErrorType  CWelsTaskManageBase::ExecuteTasks (const CWelsBaseTask::ETaskType iTaskType) {
  return ExecuteTaskList (m_pcAllTaskList[iTaskType]);
}

WelsErrorType CWelsTaskManageMultiD::Init (sWelsEncCtx* pEncCtx) {
  WelsErrorType ret = CWelsTaskManageBase::Init (pEncCtx);

  //TODO: the iMaxTaskNum logic here is for protection for now, may remove later
  int32_t iMaxTaskNum = 0;
  for (int32_t i = 0; i < m_pEncCtx->pSvcParam->iSpatialLayerNum; i++) {
    m_iTaskNumD[i] = m_pEncCtx->pSvcParam->sSpatialLayers[i].sSliceArgument.uiSliceNum;
    iMaxTaskNum = WELS_MAX (m_iTaskNumD[i], iMaxTaskNum);
  }
  //printf("CWelsTaskManageMultiD::Init, m_iTotalTaskNum=%d, iMaxTaskNum=%d\n", m_iTotalTaskNum, iMaxTaskNum);
  assert(m_iTotalTaskNum==iMaxTaskNum);
  //

  return ret;
}

void CWelsTaskManageMultiD::InitFrame (const int32_t kiCurDid) {
  //printf("CWelsTaskManageMultiD: InitFrame: m_iCurDid=%d, m_iCurrentTaskNum=%d\n", m_iCurDid, m_iCurrentTaskNum);
  m_iCurDid = kiCurDid;
  m_iCurrentTaskNum = m_iTaskNumD[kiCurDid];
  if (m_pEncCtx->pCurDqLayer->bNeedAdjustingSlicing) {
    ExecuteTaskList (m_pcAllTaskList[CWelsBaseTask::WELS_ENC_TASK_UPDATEMBMAP]);
  }
}

WelsErrorType CWelsTaskManageMultiD::ExecuteTasks (const CWelsBaseTask::ETaskType iTaskType) {
  m_iCurrentTaskNum = m_iTaskNumD[m_iCurDid];
  return CWelsTaskManageBase::ExecuteTasks (iTaskType);
}


//TODO: at present there is no diff betweenCWelsTaskManageParallel and CWelsTaskManageBase, to finish later
WelsErrorType  CWelsTaskManageParallel::ExecuteTasks (const CWelsBaseTask::ETaskType iTaskType) {
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, NULL == m_pThreadPool)

  // need lock here?
  m_iWaitTaskNum = static_cast<int32_t> (m_cEncodingTaskList->size());

  while (NULL != m_cEncodingTaskList->begin()) {
    m_pThreadPool->QueueTask (m_cEncodingTaskList->begin());
    m_cEncodingTaskList->pop_front();
  }
  WelsEventWait (&m_hTaskEvent);

  return ENC_RETURN_SUCCESS;
}

WelsErrorType   CWelsTaskManageParallel::CreateTasks (sWelsEncCtx* pEncCtx, const int32_t kiTaskCount) {
  return ENC_RETURN_SUCCESS;
}

// CWelsTaskManageOne is for test
WelsErrorType CWelsTaskManageOne::Init (sWelsEncCtx* pEncCtx) {
  m_pEncCtx = pEncCtx;

  return CreateTasks (pEncCtx, pEncCtx->iMaxSliceCount);
}

WelsErrorType  CWelsTaskManageOne::ExecuteTasks (const CWelsBaseTask::ETaskType iTaskType) {
  while (NULL != m_cEncodingTaskList->begin()) {
    (m_cEncodingTaskList->begin())->Execute();
    m_cEncodingTaskList->pop_front();
  }
  return ENC_RETURN_SUCCESS;
}
// CWelsTaskManageOne is for test

}




