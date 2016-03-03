#ifndef _WELS_THREAD_POOL_TEST_H_
#define _WELS_THREAD_POOL_TEST_H_

#include "WelsLock.h"
#include "WelsThreadPool.h"

using namespace WelsCommon;

class CThreadPoolTest : public IWelsTaskSink {
 public:
  CThreadPoolTest() {
    m_iTaskCount = 0;
  }

  ~CThreadPoolTest() {}

  virtual int32_t OnTaskExecuted (IWelsTask* pTask) {
    WelsCommon::CWelsAutoLock cAutoLock (m_cTaskCountLock);
    m_iTaskCount ++;
    //fprintf(stdout, "Task execute over count is %d\n", m_iTaskCount);
    return cmResultSuccess;
  }

  virtual int32_t OnTaskCancelled (IWelsTask* pTask) {
    WelsCommon::CWelsAutoLock cAutoLock (m_cTaskCountLock);
    m_iTaskCount ++;
    //fprintf(stdout, "Task execute cancelled count is %d\n", m_iTaskCount);
    return cmResultSuccess;
  }

  virtual int32_t OnTaskExecuted() {
    WelsCommon::CWelsAutoLock cAutoLock (m_cTaskCountLock);
    m_iTaskCount ++;
    //fprintf(stdout, "Task execute over count is %d\n", m_iTaskCount);
    return cmResultSuccess;
  }

  virtual int32_t OnTaskCancelled() {
    WelsCommon::CWelsAutoLock cAutoLock (m_cTaskCountLock);
    m_iTaskCount ++;
    //fprintf(stdout, "Task execute cancelled count is %d\n", m_iTaskCount);
    return cmResultSuccess;
  }

  int32_t  GetTaskCount() {
    return m_iTaskCount;
  }

 private:
  int32_t  m_iTaskCount;
  WelsCommon::CWelsLock  m_cTaskCountLock;
};



#endif

