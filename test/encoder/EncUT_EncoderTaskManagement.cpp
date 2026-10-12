#include <gtest/gtest.h>

#include "utils/DataGenerator.h"
#include "encoder_context.h"
#include "wels_task_management.h"

using namespace WelsEnc;


TEST (EncoderTaskManagement, CWelsTaskManageBase) {
  sWelsEncCtx sCtx;
  SWelsSvcCodingParam sWelsSvcCodingParam;

  sCtx.pSvcParam = &sWelsSvcCodingParam;
  sWelsSvcCodingParam.iMultipleThreadIdc = 4;
  sCtx.iMaxSliceCount = 35;
  IWelsTaskManage*  pTaskManage = IWelsTaskManage::CreateTaskManage (&sCtx, 1, false);
  ASSERT_TRUE (NULL != pTaskManage);

  delete pTaskManage;
}

TEST (EncoderTaskManagement, CWelsTaskManageParallel) {
  sWelsEncCtx sCtx;
  SWelsSvcCodingParam sWelsSvcCodingParam;

  sCtx.pSvcParam = &sWelsSvcCodingParam;
  sWelsSvcCodingParam.iMultipleThreadIdc = 4;
  sCtx.iMaxSliceCount = 35;
  IWelsTaskManage*  pTaskManage = IWelsTaskManage::CreateTaskManage (&sCtx, 1, true);
  ASSERT_TRUE (NULL != pTaskManage);

  delete pTaskManage;
}

TEST (EncoderTaskManagement, CWelsTaskManageMultiD) {
  sWelsEncCtx sCtx;
  SWelsSvcCodingParam sWelsSvcCodingParam;

  sCtx.pSvcParam = &sWelsSvcCodingParam;
  sWelsSvcCodingParam.iMultipleThreadIdc = 4;
  sWelsSvcCodingParam.sSpatialLayers[0].sSliceArgument.uiSliceNum = 35;
  sCtx.iMaxSliceCount = 35;

  IWelsTaskManage*  pTaskManage = IWelsTaskManage::CreateTaskManage (&sCtx, 4, true);
  ASSERT_TRUE (NULL != pTaskManage);

  delete pTaskManage;
}

namespace {

class CSyncEvent {
 public:
  CSyncEvent() : m_iCond (1) {
    WelsMutexInit (&m_hMutex);
    WelsEventOpen (&m_hEvent);
  }
  ~CSyncEvent() {
    WelsEventClose (&m_hEvent);
    WelsMutexDestroy (&m_hMutex);
  }
  void Signal() {
    WelsEventSignal (&m_hEvent, &m_hMutex, &m_iCond);
  }
  void Wait() {
    WelsEventWait (&m_hEvent, &m_hMutex, m_iCond);
  }

 private:
  WELS_EVENT m_hEvent;
  WELS_MUTEX m_hMutex;
  int m_iCond;
};

class CNoopTask : public CWelsBaseTask {
 public:
  explicit CNoopTask (WelsCommon::IWelsTaskSink* pSink) : CWelsBaseTask (pSink) {}
  virtual ~CNoopTask() {}
  virtual WelsErrorType Execute() {
    return ENC_RETURN_SUCCESS;
  }
  virtual uint32_t GetTaskType() const {
    return CWelsBaseTask::WELS_ENC_TASK_ENCODING;
  }
};

// Number of OnTaskExecuted() callbacks that have run to completion.
static WelsCommon::CWelsLock g_cCallbackLock;
static int32_t g_iCallbacksDone = 0;

static int32_t GetCallbacksDone() {
  WelsCommon::CWelsAutoLock cLock (g_cCallbackLock);
  return g_iCallbacksDone;
}

// Task manager with two trivial tasks whose final OnTaskExecuted() callback
// waits until the owner thread has entered ~CLingeringTaskManage() (right
// before ~CWelsTaskManageBase() calls Uninit()) and then lingers briefly.
class CLingeringTaskManage : public CWelsTaskManageBase {
 public:
  explicit CLingeringTaskManage (CSyncEvent* pDestroyStarted)
      : m_pDestroyStarted (pDestroyStarted), m_iCallbackOrder (0) {}
  virtual ~CLingeringTaskManage() {
    m_pDestroyStarted->Signal();
  }

  virtual WelsErrorType OnTaskExecuted() {
    CSyncEvent* pDestroyStarted = m_pDestroyStarted;
    int32_t iOrder;
    {
      WelsCommon::CWelsAutoLock cLock (g_cCallbackLock);
      iOrder = ++m_iCallbackOrder;
    }
    WelsErrorType eRet = CWelsTaskManageBase::OnTaskExecuted();
    if (iOrder == 2) {
      pDestroyStarted->Wait();
      WelsSleep (5);
    }
    WelsCommon::CWelsAutoLock cLock (g_cCallbackLock);
    g_iCallbacksDone ++;
    return eRet;
  }

 protected:
  virtual WelsErrorType CreateTasks (sWelsEncCtx* pEncCtx, const int32_t kiCurDid) {
    if (kiCurDid > 0) {
      return ENC_RETURN_SUCCESS;
    }
    m_iTaskNum[0] = 2;
    for (int32_t idx = 0; idx < m_iTaskNum[0]; ++idx) {
      CWelsBaseTask* pTask = WELS_NEW_OP (CNoopTask (this), CNoopTask);
      WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, NULL == pTask)
      WELS_VERIFY_RETURN_IF (ENC_RETURN_MEMALLOCERR, true != m_cEncodingTaskList[0]->push_back (pTask))
    }
    return ENC_RETURN_SUCCESS;
  }

 private:
  CSyncEvent* m_pDestroyStarted;
  int32_t m_iCallbackOrder;
};

} // namespace

// Destroying a task manager right after ExecuteTasks() returns must not leave a
// pool worker still inside one of its callbacks.
TEST (EncoderTaskManagement, NoCallbackInFlightAfterDestroy) {
  // A second task manager keeps the shared thread pool alive, so that tearing
  // down the task manager under test does not stop and join the workers.
  sWelsEncCtx sKeepAliveCtx;
  SWelsSvcCodingParam sKeepAliveParam;
  memset (&sKeepAliveCtx, 0, sizeof (sKeepAliveCtx));
  sKeepAliveCtx.pSvcParam = &sKeepAliveParam;
  sKeepAliveParam.iMultipleThreadIdc = 2;
  IWelsTaskManage* pKeepAlive = IWelsTaskManage::CreateTaskManage (&sKeepAliveCtx, 1, false);
  ASSERT_TRUE (NULL != pKeepAlive);

  sWelsEncCtx sCtx;
  SWelsSvcCodingParam sParam;
  SDqLayer sCurDqLayer;
  memset (&sCtx, 0, sizeof (sCtx));
  memset (&sCurDqLayer, 0, sizeof (sCurDqLayer));
  sCtx.pSvcParam = &sParam;
  sCtx.pCurDqLayer = &sCurDqLayer;
  sParam.iMultipleThreadIdc = 2;

  {
    WelsCommon::CWelsAutoLock cLock (g_cCallbackLock);
    g_iCallbacksDone = 0;
  }

  const int32_t kiIterations = 5;
  for (int32_t i = 0; i < kiIterations; ++i) {
    CSyncEvent cDestroyStarted;
    CLingeringTaskManage* pTaskManage = new CLingeringTaskManage (&cDestroyStarted);
    ASSERT_EQ (ENC_RETURN_SUCCESS, pTaskManage->Init (&sCtx));
    pTaskManage->InitFrame (0);
    ASSERT_EQ (ENC_RETURN_SUCCESS, pTaskManage->ExecuteTasks());
    delete pTaskManage;

    // Both callbacks must have completed before the destructor returned.
    const int32_t iExpected = 2 * (i + 1);
    const int32_t iDone = GetCallbacksDone();
    EXPECT_EQ (iExpected, iDone) << "iteration " << i;
    if (iExpected != iDone) {
      // Let the lingering worker finish before leaving the test.
      while (GetCallbacksDone() < iExpected) {
        WelsSleep (1);
      }
      break;
    }
  }

  delete pKeepAlive;
}