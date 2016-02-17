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
 * \file    WelsThreadPool.cpp
 *
 * \brief   functions for Thread Pool
 *
 * \date    5/09/2012 Created
 *
 *************************************************************************************
 */
#include "typedefs.h"
#include "memory_align.h"
#include "WelsThreadPool.h"

namespace WelsCommon {


CWelsThreadPool::CWelsThreadPool (IWelsThreadPoolSink* pSink, int32_t iMaxThreadNum) :
  m_pSink (pSink) {
  m_cWaitedTasks = new CWelsCircleQueue<IWelsTask>();
  m_cIdleThreads = new CWelsCircleQueue<CWelsTaskThread>();
  m_cBusyThreads = new CWelsList<CWelsTaskThread>();
  m_iMaxThreadNum = 0;

  if (NULL == m_cWaitedTasks || NULL == m_cIdleThreads || NULL == m_cBusyThreads) {
    WELS_DELETE_OP(m_cWaitedTasks);
    WELS_DELETE_OP(m_cIdleThreads);
    WELS_DELETE_OP(m_cBusyThreads);

    return;
  }

  if (WELS_THREAD_ERROR_OK != Init (iMaxThreadNum)) {
    Uninit();

    WELS_DELETE_OP(m_cWaitedTasks);
    WELS_DELETE_OP(m_cIdleThreads);
    WELS_DELETE_OP(m_cBusyThreads);
  }
}


CWelsThreadPool::~CWelsThreadPool() {
  Uninit();

  WELS_DELETE_OP(m_cWaitedTasks);
  WELS_DELETE_OP(m_cIdleThreads);
  WELS_DELETE_OP(m_cBusyThreads);
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::OnTaskStart (CWelsTaskThread* pThread, IWelsTask* pTask) {
  AddThreadToBusyList (pThread);

  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::OnTaskStop (CWelsTaskThread* pThread, IWelsTask* pTask) {
  RemoveThreadFromBusyList (pThread);
  AddThreadToIdleQueue (pThread);

  if (m_pSink) {
    m_pSink->OnTaskExecuted (pTask);
  }

  //WELS_INFO_TRACE("ThreadPool: Task "<<(uint32_t)pTask<<" Finished, Thread "<<(uint32_t)pThread<<" put to idle list");

  SignalThread();
  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::Init (int32_t iMaxThreadNum) {
  CWelsAutoLock  cLock (m_cLockPool);
  //WELS_INFO_TRACE("Enter WelsThreadPool Init");

  int32_t i;

  if (iMaxThreadNum <= 0)  iMaxThreadNum = 1;
  m_iMaxThreadNum = iMaxThreadNum;

  for (i = 0; i < m_iMaxThreadNum; i++) {
    if (WELS_THREAD_ERROR_OK != CreateIdleThread()) {
      return WELS_THREAD_ERROR_GENERAL;
    }
  }

  if (WELS_THREAD_ERROR_OK != Start()) {
    return WELS_THREAD_ERROR_GENERAL;
  }

  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::Uninit() {
  WELS_THREAD_ERROR_CODE iReturn = WELS_THREAD_ERROR_OK;
  CWelsAutoLock  cLock (m_cLockPool);

  ClearWaitedTasks();

  while (GetBusyThreadNum() > 0) {
    //WELS_INFO_TRACE ("CWelsThreadPool::Uninit - Waiting all thread to exit");
    WelsSleep (10);
  }

  if (GetIdleThreadNum() != m_iMaxThreadNum) {
    iReturn = WELS_THREAD_ERROR_GENERAL;
  }

  m_cLockIdleTasks.Lock();
  while (m_cIdleThreads->size() > 0) {
    DestroyThread (m_cIdleThreads->begin());
    m_cIdleThreads->pop_front();
  }
  m_cLockIdleTasks.Unlock();

  m_iMaxThreadNum = 0;
  Kill();

  return iReturn;
}

void CWelsThreadPool::ExecuteTask() {
  //WELS_INFO_TRACE("ThreadPool: schedule tasks");
  CWelsTaskThread* pThread = NULL;
  IWelsTask*    pTask = NULL;
  while (GetWaitedTaskNum() > 0) {
    pThread = GetIdleThread();
    if (pThread == NULL) {
      break;
    }
    pTask = GetWaitedTask();
    //WELS_INFO_TRACE("ThreadPool:  ExecuteTask = "<<(uint32_t)(pTask)<<" at thread = "<<(uint32_t)(pThread));
    pThread->SetTask (pTask);
  }
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::QueueTask (IWelsTask* pTask) {
  CWelsAutoLock  cLock (m_cLockPool);

  //WELS_INFO_TRACE("ThreadPool:  QueueTask = "<<(uint32_t)(pTask));
  if (GetWaitedTaskNum() == 0) {
    CWelsTaskThread* pThread = GetIdleThread();

    if (pThread != NULL) {
      //WELS_INFO_TRACE("ThreadPool:  ExecuteTask = "<<(uint32_t)(pTask));
      pThread->SetTask (pTask);

      return WELS_THREAD_ERROR_OK;
    }
  }

  AddTaskToWaitedList (pTask);

  SignalThread();
  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::CreateIdleThread() {
  CWelsTaskThread* pThread = new CWelsTaskThread (this);

  if (NULL == pThread) {
    return WELS_THREAD_ERROR_GENERAL;
  }

  if (WELS_THREAD_ERROR_OK != pThread->Start()) {
    return WELS_THREAD_ERROR_GENERAL;
  }
  AddThreadToIdleQueue (pThread);

  return WELS_THREAD_ERROR_OK;
}

void  CWelsThreadPool::DestroyThread (CWelsTaskThread* pThread) {
  pThread->Kill();
  WELS_DELETE_OP(pThread);

  return;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::AddThreadToIdleQueue (CWelsTaskThread* pThread) {
  CWelsAutoLock cLock (m_cLockIdleTasks);
  m_cIdleThreads->push_back (pThread);
  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::AddThreadToBusyList (CWelsTaskThread* pThread) {
  CWelsAutoLock cLock (m_cLockBusyTasks);
  m_cBusyThreads->push_back (pThread);
  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::RemoveThreadFromBusyList (CWelsTaskThread* pThread) {
  CWelsAutoLock cLock (m_cLockBusyTasks);
  if (m_cBusyThreads->erase(pThread)) {
      return WELS_THREAD_ERROR_OK;
  } else {
        return WELS_THREAD_ERROR_GENERAL;
  }
}

void  CWelsThreadPool::AddTaskToWaitedList (IWelsTask* pTask) {
  CWelsAutoLock  cLock (m_cLockWaitedTasks);

  m_cWaitedTasks->push_back (pTask);
  return;
}

CWelsTaskThread*   CWelsThreadPool::GetIdleThread() {
  CWelsAutoLock cLock (m_cLockIdleTasks);

  if (m_cIdleThreads->size() == 0) {
    return NULL;
  }

  CWelsTaskThread* pThread = m_cIdleThreads->begin();
  m_cIdleThreads->pop_front();
  return pThread;
}

int32_t  CWelsThreadPool::GetBusyThreadNum() {
  return m_cBusyThreads->size();
}

int32_t  CWelsThreadPool::GetIdleThreadNum() {
  return m_cIdleThreads->size();
}

int32_t  CWelsThreadPool::GetWaitedTaskNum() {
  return m_cWaitedTasks->size();
}

IWelsTask* CWelsThreadPool::GetWaitedTask() {
  CWelsAutoLock lock (m_cLockWaitedTasks);

  if (m_cWaitedTasks->size() == 0) {
    return NULL;
  }

  IWelsTask* pTask = m_cWaitedTasks->begin();

  m_cWaitedTasks->pop_front();

  return pTask;
}

void  CWelsThreadPool::ClearWaitedTasks() {
  CWelsAutoLock cLock (m_cLockWaitedTasks);

  if (m_pSink) {
    while (0 != m_cWaitedTasks->size()) {
      m_pSink->OnTaskCancelled (m_cWaitedTasks->begin());
      m_cWaitedTasks->pop_front();
    }
  }
}

}


