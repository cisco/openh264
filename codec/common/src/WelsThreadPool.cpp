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

int32_t CWelsThreadPool::m_iRefCount = 0;
CWelsLock CWelsThreadPool::m_cInitLock;
int32_t CWelsThreadPool::m_iMaxThreadNum = DEFAULT_THREAD_NUM;

CWelsThreadPool::CWelsThreadPool() :
  m_cWaitedTasks (NULL), m_cIdleThreads (NULL), m_cBusyThreads (NULL), m_cBusyTasks (NULL) {
}


CWelsThreadPool::~CWelsThreadPool() {
  //fprintf(stdout, "CWelsThreadPool::~CWelsThreadPool: delete %x, %x, %x\n", m_cWaitedTasks, m_cIdleThreads, m_cBusyThreads);
  if (0 != m_iRefCount) {
    m_iRefCount = 0;
    Uninit();
  }
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::SetThreadNum (int32_t iMaxThreadNum) {
  CWelsAutoLock  cLock (m_cInitLock);

  if (m_iRefCount != 0) {
    return WELS_THREAD_ERROR_GENERAL;
  }

  if (iMaxThreadNum <= 0) {
    iMaxThreadNum = 1;
  }
  m_iMaxThreadNum = iMaxThreadNum;
  return WELS_THREAD_ERROR_OK;
}


CWelsThreadPool& CWelsThreadPool::AddReference() {
  CWelsAutoLock  cLock (m_cInitLock);
  static CWelsThreadPool m_cThreadPoolSelf;
  if (m_iRefCount == 0) {
    //TODO: will remove this afterwards
    if (WELS_THREAD_ERROR_OK != m_cThreadPoolSelf.Init()) {
      m_cThreadPoolSelf.Uninit();
    }
  }

  //fprintf(stdout, "m_iRefCount=%d, pSink=%x, iMaxThreadNum=%d\n", m_iRefCount, pSink, iMaxThreadNum);

  ++ m_iRefCount;
  //fprintf(stdout, "m_iRefCount2=%d\n", m_iRefCount);
  return m_cThreadPoolSelf;
}


void CWelsThreadPool::RemoveInstance (IWelsTaskSink* pSink) {
  fprintf (stdout, "RemoveInstance=%x\n", pSink);
  
  CWelsAutoLock  cLock (m_cInitLock);
  fprintf (stdout, "m_iRefCount=%d GetBusyThreadNum=%d\n", m_iRefCount, GetBusyThreadNum());
  //note: need to remove WaitedTask first in case that task in WaitedTask is added to running in between RemoveWaitedTask and RemoveBusyTask
  RemoveWaitedTask (pSink);
  RemoveBusyTask (pSink);
  -- m_iRefCount;

  if (0 == m_iRefCount) {
    //StopAllRunning();
    Uninit();
    //fprintf(stdout, "m_iRefCount=%d, IdleThreadNum=%d, BusyThreadNum=%d, WaitedTask=%d\n", m_iRefCount, GetIdleThreadNum(), GetBusyThreadNum(), GetWaitedTaskNum());
  }
    fprintf (stdout, "end RemoveInstance=%x, %d\n", pSink, m_iRefCount);
}


bool CWelsThreadPool::IsReferenced() {
  CWelsAutoLock  cLock (m_cInitLock);
  return (m_iRefCount > 0);
}


WELS_THREAD_ERROR_CODE CWelsThreadPool::OnTaskStart (CWelsTaskThread* pThread, IWelsTask* pTask) {
  //AddThreadToBusyList (pThread, pTask);
  //fprintf(stdout, "CWelsThreadPool::AddThreadToBusyList: Task %x at Thread %x\n", pTask, pThread);
  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::OnTaskStop (CWelsTaskThread* pThread, IWelsTask* pTask) {
  fprintf (stdout, "CWelsThreadPool::OnTaskStop 0: Task %x at Thread %x Finished\n", pTask, pThread);


  if (pTask->GetSink()) {
    fprintf (stdout, "CWelsThreadPool::OnTaskStop 1: Task %x at Thread %x Finished, sink=%x\n", pTask, pThread,
             pTask->GetSink());
    pTask->GetSink()->OnTaskExecuted();
  }
  RemoveThreadFromBusyList (pThread, pTask);
  AddThreadToIdleQueue (pThread);
  fprintf (stdout, "CWelsThreadPool::OnTaskStop 2: Task %x at Thread %x Finished\n", pTask, pThread);
 
  fprintf (stdout, "CWelsThreadPool::OnTaskStop 3: Task %x at Thread %x Finished\n", pTask, pThread);

  SignalThread();

  //fprintf(stdout, "ThreadPool: Task %x at Thread %x Finished\n", pTask, pThread);
  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::Init() {
  //fprintf(stdout, "Enter WelsThreadPool Init\n");

  CWelsAutoLock  cLock (m_cLockPool);

  m_cWaitedTasks = new CWelsList<IWelsTask>();
  m_cIdleThreads = new CWelsCircleQueue<CWelsTaskThread>();
  m_cBusyThreads = new CWelsList<CWelsTaskThread>();
  m_cBusyTasks = new CWelsList<IWelsTask>();
  if (NULL == m_cWaitedTasks || NULL == m_cIdleThreads || NULL == m_cBusyThreads || NULL == m_cBusyTasks) {
    return WELS_THREAD_ERROR_GENERAL;
  }

  for (int32_t i = 0; i < m_iMaxThreadNum; i++) {
    if (WELS_THREAD_ERROR_OK != CreateIdleThread()) {
      return WELS_THREAD_ERROR_GENERAL;
    }
  }

  if (WELS_THREAD_ERROR_OK != Start()) {
    return WELS_THREAD_ERROR_GENERAL;
  }

  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::StopAllRunning() {
  WELS_THREAD_ERROR_CODE iReturn = WELS_THREAD_ERROR_OK;

  fprintf (stdout, "CWelsThreadPool::Uninit ClearWaitedTasks\n");
  ClearWaitedTasks();

  fprintf (stdout, "CWelsThreadPool::Uninit GetBusyThreadNum %d\n", GetBusyThreadNum());
  while (GetBusyThreadNum() > 0) {
    fprintf (stdout, "CWelsThreadPool::Uninit - Waiting all thread to exit\n");
    WelsSleep (10);
  }

  fprintf (stdout, "StopAllRunning:  GetIdleThreadNum %d %d\n", GetIdleThreadNum(), m_iMaxThreadNum);
  if (GetIdleThreadNum() != m_iMaxThreadNum) {
    iReturn = WELS_THREAD_ERROR_GENERAL;
  }

  return iReturn;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::Uninit() {
  WELS_THREAD_ERROR_CODE iReturn = WELS_THREAD_ERROR_OK;
  CWelsAutoLock  cLock (m_cLockPool);

  iReturn = StopAllRunning();
  if (WELS_THREAD_ERROR_OK != iReturn) {
    return iReturn;
  }

  m_cLockIdleTasks.Lock();
  while (m_cIdleThreads->size() > 0) {
    DestroyThread (m_cIdleThreads->begin());
    m_cIdleThreads->pop_front();
  }
  m_cLockIdleTasks.Unlock();

  Kill();

  WELS_DELETE_OP (m_cWaitedTasks);
  WELS_DELETE_OP (m_cIdleThreads);
  WELS_DELETE_OP (m_cBusyThreads);

  return iReturn;
}

void CWelsThreadPool::ExecuteTask() {
  fprintf(stdout, "ThreadPool: scheduled tasks: ExecuteTask\n");
  CWelsTaskThread* pThread = NULL;
  IWelsTask*    pTask = NULL;
  while (GetWaitedTaskNum() > 0) {
    fprintf(stdout, "CWelsThreadPool::ExecuteTask: GetIdleThread00\n");
    pThread = GetIdleThread();
    fprintf(stdout, "CWelsThreadPool::ExecuteTask: GetIdleThread01 %x\n", pThread);
    if (pThread == NULL) {
      break;
    }
    pTask = GetWaitedTask();
    if (pTask) {
    fprintf(stdout, "ThreadPool:  ExecuteTask = %x at thread %x\n", pTask, pThread);
      AddThreadToBusyList (pThread, pTask);
    pThread->SetTask (pTask);
    }
  }
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::QueueTask (IWelsTask* pTask) {
  CWelsAutoLock  cLock (m_cLockPool);

  fprintf(stdout, "CWelsThreadPool::QueueTask: %d, pTask=%x\n", m_iRefCount, pTask);
  if (GetWaitedTaskNum() == 0) {
    fprintf(stdout, "CWelsThreadPool::QueueTask: GetIdleThread11\n");
    CWelsTaskThread* pThread = GetIdleThread();
fprintf(stdout, "CWelsThreadPool::ExecuteTask: GetIdleThread12\n");
    if (pThread != NULL) {
      fprintf (stdout, "ThreadPool:  QueueTask = %x at thread %x\n", pTask, pThread);
      AddThreadToBusyList (pThread, pTask);
      pThread->SetTask (pTask);

      return WELS_THREAD_ERROR_OK;
    }
  }
  //fprintf(stdout, "ThreadPool:  AddTaskToWaitedList: %x\n", pTask);
  AddTaskToWaitedList (pTask);

  //fprintf(stdout, "ThreadPool:  SignalThread: %x\n", pTask);
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
  //fprintf(stdout, "ThreadPool:  AddThreadToIdleQueue: %x\n", pThread);
  AddThreadToIdleQueue (pThread);

  return WELS_THREAD_ERROR_OK;
}

void  CWelsThreadPool::DestroyThread (CWelsTaskThread* pThread) {
  pThread->Kill();
  WELS_DELETE_OP (pThread);

  return;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::AddThreadToIdleQueue (CWelsTaskThread* pThread) {
  CWelsAutoLock cLock (m_cLockIdleTasks);
  m_cIdleThreads->push_back (pThread);
  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::AddThreadToBusyList (CWelsTaskThread* pThread, IWelsTask* pTask) {
  CWelsAutoLock cLock (m_cLockBusyTasks);
  m_cBusyThreads->push_back (pThread);
  m_cBusyTasks->push_back (pTask);
  return WELS_THREAD_ERROR_OK;
}

WELS_THREAD_ERROR_CODE CWelsThreadPool::RemoveThreadFromBusyList (CWelsTaskThread* pThread, IWelsTask* pTask) {
  CWelsAutoLock cLock (m_cLockBusyTasks);
  if (m_cBusyThreads->erase (pThread) && m_cBusyTasks->erase (pTask)) {
    return WELS_THREAD_ERROR_OK;
  } else {
    return WELS_THREAD_ERROR_GENERAL;
  }
}

void  CWelsThreadPool::AddTaskToWaitedList (IWelsTask* pTask) {
  CWelsAutoLock  cLock (m_cLockWaitedTasks);

  m_cWaitedTasks->push_back (pTask);
  fprintf (stdout, "CWelsThreadPool::AddTaskToWaitedList=%d, pTask=%x %x\n", m_cWaitedTasks->size(), pTask,
           pTask->GetSink());
  return;
}

CWelsTaskThread*   CWelsThreadPool::GetIdleThread() {
  CWelsAutoLock cLock (m_cLockIdleTasks);

  fprintf (stdout, "CWelsThreadPool::GetIdleThread=%d\n", m_cIdleThreads->size());
  if (m_cIdleThreads->size() == 0) {
    return NULL;
  }

  CWelsTaskThread* pThread = m_cIdleThreads->begin();
  m_cIdleThreads->pop_front();
  return pThread;
}

int32_t  CWelsThreadPool::GetBusyThreadNum() {
  //CWelsAutoLock cLock (m_cLockBusyTasks);
  return (m_cBusyThreads) ? (m_cBusyThreads->size()) : (-1);
}

int32_t  CWelsThreadPool::GetIdleThreadNum() {
   // CWelsAutoLock cLock (m_cLockIdleTasks);
  return (m_cIdleThreads) ? (m_cIdleThreads->size()) : (-1);
}

int32_t  CWelsThreadPool::GetWaitedTaskNum() {
  fprintf (stdout, "CWelsThreadPool::GetWaitedTaskNum\n");
  //CWelsAutoLock cLock (m_cLockWaitedTasks);
  fprintf (stdout, "CWelsThreadPool::GetWaitedTaskNum m_cWaitedTasks=%x\n", m_cWaitedTasks);
  fprintf (stdout, "CWelsThreadPool::GetWaitedTaskNum m_cWaitedTasks=%d\n", m_cWaitedTasks->size());
  return (m_cWaitedTasks) ? (m_cWaitedTasks->size()) : (-1);
}

IWelsTask* CWelsThreadPool::GetWaitedTask() {
  CWelsAutoLock lock (m_cLockWaitedTasks);

  if (m_cWaitedTasks->size() == 0) {
    return NULL;
  }

  IWelsTask* pTask = m_cWaitedTasks->begin();

  m_cWaitedTasks->pop_front();
  fprintf (stdout, "CWelsThreadPool::GetWaitedTask=%x %d\n", pTask, m_cWaitedTasks->size());
  return pTask;
}

void  CWelsThreadPool::ClearWaitedTasks() {
  CWelsAutoLock cLock (m_cLockWaitedTasks);
  IWelsTask* pTask = NULL;
  fprintf (stdout, "CWelsThreadPool::ClearWaitedTasks m_cWaitedTasks=%d\n", m_cWaitedTasks->size());
  while (0 != m_cWaitedTasks->size()) {
    fprintf (stdout, "CWelsThreadPool::ClearWaitedTasks m_cWaitedTasks=%d\n", m_cWaitedTasks->size());
    pTask = m_cWaitedTasks->begin();
    if (pTask->GetSink()) {
      pTask->GetSink()->OnTaskCancelled();
      fprintf (stdout, "CWelsThreadPool::ClearWaitedTasks pTask=%x pSink=%d\n", pTask, pTask->GetSink());
    }
    m_cWaitedTasks->pop_front();
  }
}

IWelsTask* FindTaskWithSink (CWelsList<IWelsTask>* pTaskList, IWelsTaskSink* pSink) {
  fprintf (stdout, "CWelsThreadPool::FindTaskWithSink\n");
  int32_t iTaskNum = pTaskList->size();
  fprintf (stdout, "CWelsThreadPool::FindTaskWithSink %d %x %x\n", iTaskNum, pTaskList, pSink);
  for (int32_t iIdx = 0; iIdx < iTaskNum; iIdx++) {
    IWelsTask* pTask = pTaskList->index (iIdx);
    if (pTask->GetSink() == pSink) {
      fprintf (stdout, "CWelsThreadPool::FindTaskWithSink %d %x %x\n", iIdx, pTask, pSink);
      return pTask;
    }
  }
  return NULL;
}

void CWelsThreadPool::RemoveWaitedTask (IWelsTaskSink* pSink) {
  CWelsAutoLock cLock (m_cLockWaitedTasks);
  fprintf (stdout, "CWelsThreadPool::RemoveWaitedTask %x %x\n", m_cWaitedTasks, pSink);
  if ((0 == m_cWaitedTasks->size()) || (NULL == pSink)) {
    fprintf (stdout, "CWelsThreadPool::RemoveWaitedTask m_cWaitedTasks %d or Sink==NULL\n", m_cWaitedTasks->size());
    return;
  }
  fprintf (stdout, "CWelsThreadPool::RemoveWaitedTask %d\n", m_cWaitedTasks->size());
  IWelsTask* pTask = FindTaskWithSink (m_cWaitedTasks, pSink);
  fprintf (stdout, "CWelsThreadPool::RemoveWaitedsTask0 %x\n", pTask);
  while (pTask) {
    fprintf (stdout, "CWelsThreadPool::RemoveWaitedTask1 %x\n", pTask);
    pTask->GetSink()->OnTaskCancelled();
    m_cWaitedTasks->erase (pTask);
    fprintf (stdout, "CWelsThreadPool::RemoveWaitedTask2 %x\n", pTask);
    pTask = FindTaskWithSink (m_cWaitedTasks, pSink);
  }
  fprintf (stdout, "CWelsThreadPool::RemoveWaitedTask %x %x\n", pSink, pTask);
}

void CWelsThreadPool::RemoveBusyTask (IWelsTaskSink* pSink) {
  while (GetBusyThreadNum() > 0) {
    fprintf (stdout, "CWelsThreadPool::RemoveBusyTask %d %x\n", GetBusyThreadNum(), pSink);
    m_cLockBusyTasks.Lock();
    IWelsTask* pTask = FindTaskWithSink (m_cBusyTasks, pSink);
    m_cLockBusyTasks.Unlock();
    if (pTask) {
      fprintf (stdout, "CWelsThreadPool::RemoveBusyTask waiting %x to finish\n", pTask);
      WelsSleep (10);
    } else {
      break;
    }
  }
}

}


