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
 * \file    wels_task_management.h
 *
 * \brief   interface for task management
 *
 * \date    5/14/2012 Created
 *
 *************************************************************************************
 */

#ifndef _WELS_ENCODER_TASK_MANAGE_H_
#define _WELS_ENCODER_TASK_MANAGE_H_

#include "wels_common_basis.h"
#include "WelsLock.h"
#include "WelsThreadPool.h"
#include "wels_task_base.h"

namespace WelsEnc {

class IWelsTaskManage {
 public:
  virtual ~IWelsTaskManage() { }

  virtual WelsErrorType   Init (sWelsEncCtx*   pEncCtx) = 0;
  virtual void            Uninit() = 0;

  virtual void            InitFrame (const int32_t kiCurDid) {}
  virtual WelsErrorType   ExecuteTasks(const CWelsBaseTask::ETaskType iTaskType = CWelsBaseTask::WELS_ENC_TASK_ENCODING) = 0;

  static IWelsTaskManage* CreateTaskManage (sWelsEncCtx* pCtx, bool bNeedLock);
};


class  CWelsTaskManageBase : public IWelsTaskManage, public WelsCommon::IWelsThreadPoolSink {
 public:
  typedef  CWelsCircleQueue<CWelsBaseTask>            TASKLIST_TYPE;
  //typedef  std::pair<int, int>                  SLICE_BOUNDARY_PAIR;
  //typedef  CWelsCircleQueue<SLICE_BOUNDARY_PAIR>       SLICE_PAIR_LIST;

  CWelsTaskManageBase();
  virtual ~ CWelsTaskManageBase();

  virtual WelsErrorType   Init (sWelsEncCtx*   pEncCtx);
  void    Uninit();

  virtual void            InitFrame (const int32_t kiCurDid);
  virtual WelsErrorType   ExecuteTasks(const CWelsBaseTask::ETaskType iTaskType = CWelsBaseTask::WELS_ENC_TASK_ENCODING);

  //IWelsThreadPoolSink
  virtual WelsErrorType  OnTaskExecuted (WelsCommon::IWelsTask* pTask);
  virtual WelsErrorType  OnTaskCancelled (WelsCommon::IWelsTask* pTask);

 protected:
  virtual  WelsErrorType  CreateTasks (sWelsEncCtx* pEncCtx, const int32_t kiTaskCount);
  void                    DestroyTasks();
  WelsErrorType  ExecuteTaskList(TASKLIST_TYPE* pTargetTaskList);

 protected:
  sWelsEncCtx*    m_pEncCtx;
  WelsCommon::CWelsThreadPool*   m_pThreadPool;

  TASKLIST_TYPE*   m_pcAllTaskList[CWelsBaseTask::WELS_ENC_TASK_ALL];
  TASKLIST_TYPE*   m_cEncodingTaskList;
  TASKLIST_TYPE*   m_cPreEncodingTaskList;
  int32_t         m_iTaskNum;

  //SLICE_PAIR_LIST *m_cSliceList;

  int32_t         m_iThreadNum;

  int32_t          m_iWaitTaskNum;
  WELS_EVENT       m_hTaskEvent;

  WelsCommon::CWelsLock  m_cWaitTaskNumLock;

 private:
  DISALLOW_COPY_AND_ASSIGN (CWelsTaskManageBase);
  void  OnTaskMinusOne();
};

class  CWelsTaskManageOne : public CWelsTaskManageBase {
 public:
  CWelsTaskManageOne();
  virtual ~CWelsTaskManageOne();

  WelsErrorType   Init (sWelsEncCtx* pEncCtx);
  virtual WelsErrorType  ExecuteTasks(const CWelsBaseTask::ETaskType iTaskType = CWelsBaseTask::WELS_ENC_TASK_ENCODING);
};

class  CWelsTaskManageParallel : public CWelsTaskManageBase {
 public:
  virtual WelsErrorType  ExecuteTasks(const CWelsBaseTask::ETaskType iTaskType = CWelsBaseTask::WELS_ENC_TASK_ENCODING);

 protected:
  virtual  WelsErrorType  CreateTasks (sWelsEncCtx* pEncCtx, const int32_t kiTaskCount);
};

}       //namespace
#endif  //header guard

