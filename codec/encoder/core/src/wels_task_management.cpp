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



IWelsTaskManage*   IWelsTaskManage::CreateTaskManage (sWelsEncCtx* pCtx, bool bNeedLock) {
  if (NULL == pCtx) {
    return NULL;
  }
  IWelsTaskManage* pTaskManage;
  if (bNeedLock) {
    pTaskManage = WELS_NEW_OP (CWelsTaskManageParallel(), CWelsTaskManageParallel);
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
    m_iTaskNum (0),
    m_iWaitTaskNum (0) {
  m_cTaskList = new TASKLIST_TYPE();
  WelsEventOpen (&m_hTaskEvent);
}

CWelsTaskManageBase::~CWelsTaskManageBase() {
  //printf("~CWelsTaskManageBase\n");
  Uninit();
}

WelsErrorType CWelsTaskManageBase::Init (sWelsEncCtx* pEncCtx) {
  m_pEncCtx = pEncCtx;

  m_iThreadNum = m_pEncCtx->pSvcParam->iMultipleThreadIdc;
  m_pThreadPool = WELS_NEW_OP (WelsCommon::CWelsThreadPool (this, m_iThreadNum),
                               WelsCommon::CWelsThreadPool);
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, NULL == m_pThreadPool)
  //printf("CWelsTaskManageBase Init m_iThreadNum %d pEncCtx->iMaxSliceCount=%d\n", m_iThreadNum, pEncCtx->iMaxSliceCount);
  return CreateTasks (pEncCtx, pEncCtx->iMaxSliceCount);
}

void   CWelsTaskManageBase::Uninit() {
  DestroyTasks();
  WELS_DELETE_OP (m_pThreadPool);

  delete m_cTaskList;
  WelsEventClose (&m_hTaskEvent);
}

WelsErrorType CWelsTaskManageBase::CreateTasks (sWelsEncCtx* pEncCtx, const int32_t kiTaskCount) {
  CWelsBaseTask* pTask = NULL;

  for (int idx = 0; idx < kiTaskCount; idx++) {
    pTask = WELS_NEW_OP (CWelsSliceEncodingTask (pEncCtx, idx), CWelsSliceEncodingTask);
    WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, NULL == pTask)
    m_cTaskList->push_back (pTask);
  }
  m_iTaskNum = kiTaskCount;

  //printf("CWelsTaskManageBase CreateTasks m_iThreadNum %d kiTaskCount=%d\n", m_iThreadNum, kiTaskCount);
  return ENC_RETURN_SUCCESS;
}

void CWelsTaskManageBase::DestroyTasks() {
  if (m_iTaskNum == 0) {
    return;
  }

  if (m_cTaskList->size() != m_iTaskNum) {
    //printf("m_cTaskList %d %d\n", static_cast<int32_t>(m_cTaskList->size()), m_iTaskNum);
    //WELS_ERROR_TRACE ("CWelsTaskManage::DestroyTasks:  Incorrect task numbers");
  }

  while (NULL != m_cTaskList->begin()) {
    CWelsBaseTask* pTask = m_cTaskList->begin();
    WELS_DELETE_OP (pTask);
    m_cTaskList->pop_front();
  }
  //WelsLog (&m_pEncCtx->sLogCtx, WELS_LOG_INFO,
  //         "[MT] CWelsTaskManageParallel()DestroyTasks, cleaned %d tasks", m_iTaskNum);
  //printf ("[MT] CWelsTaskManageBase() DestroyTasks, cleaned %d tasks\n", m_iTaskNum);
  m_iTaskNum = 0;
}

void  CWelsTaskManageBase::OnTaskMinusOne() {
  WelsCommon::CWelsAutoLock cAutoLock (m_cWaitTaskNumLock);
  m_iWaitTaskNum --;
  if (m_iWaitTaskNum <= 0) {
    WelsEventSignal (&m_hTaskEvent);
  }
  //printf("OnTaskMinusOne m_iWaitTaskNum=%d\n", m_iWaitTaskNum);
}

WelsErrorType  CWelsTaskManageBase::OnTaskCancelled (WelsCommon::IWelsTask* pTask) {
  OnTaskMinusOne();
  return ENC_RETURN_SUCCESS;
}

WelsErrorType  CWelsTaskManageBase::OnTaskExecuted (WelsCommon::IWelsTask* pTask) {
  OnTaskMinusOne();
  return ENC_RETURN_SUCCESS;
}

void CWelsTaskManageBase::InitFrame (const int32_t kiCurDid) {
  m_iWaitTaskNum = m_pEncCtx->pSvcParam->sSpatialLayers[kiCurDid].sSliceCfg.sSliceArgument.uiSliceNum;
  //printf("InitFrame m_iWaitTaskNum=%d, slice_mode=%d\n", m_iWaitTaskNum, m_pEncCtx->pSvcParam->sSpatialLayers[kiCurDid].sSliceCfg.uiSliceMode);
  //TODO: update mbmap;
}

WelsErrorType  CWelsTaskManageBase::ExecuteTasks() {
  //printf("ExecuteTasks m_iWaitTaskNum=%d\n", m_iWaitTaskNum);
  int32_t iCurrentTaskCount = m_iWaitTaskNum; //if directly use m_iWaitTaskNum in the loop make cause sync problem
  int32_t iIdx = 0;
   while (iIdx < iCurrentTaskCount) {
    m_pThreadPool->QueueTask (m_cTaskList->GetIndexNode(iIdx));
    iIdx ++;
  }
  WelsEventWait (&m_hTaskEvent);

  return ENC_RETURN_SUCCESS;
}

WelsErrorType CWelsTaskManageOne::Init (sWelsEncCtx* pEncCtx) {
  Uninit();
  m_pEncCtx = pEncCtx;

  return CreateTasks (pEncCtx, pEncCtx->iMaxSliceCount);
}

WelsErrorType  CWelsTaskManageOne::ExecuteTasks() {
  while (NULL != m_cTaskList->begin()) {
    (m_cTaskList->begin())->Execute();
    m_cTaskList->pop_front();
  }
  return ENC_RETURN_SUCCESS;
}

//TODO: at present there is no diff betweenCWelsTaskManageParallel and CWelsTaskManageBase, to finish later
WelsErrorType  CWelsTaskManageParallel::ExecuteTasks() {
  WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, NULL == m_pThreadPool)

  // need lock here?
  m_iWaitTaskNum = static_cast<int32_t> (m_cTaskList->size());

  while (NULL != m_cTaskList->begin()) {
    m_pThreadPool->QueueTask (m_cTaskList->begin());
    m_cTaskList->pop_front();
  }
  WelsEventWait (&m_hTaskEvent);

  return ENC_RETURN_SUCCESS;
}

WelsErrorType   CWelsTaskManageParallel::CreateTasks (sWelsEncCtx* pEncCtx, const int32_t kiTaskCount) {
  return ENC_RETURN_SUCCESS;
}

}




