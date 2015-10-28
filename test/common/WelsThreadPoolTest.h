#ifndef _WELS_THREAD_POOL_TEST_H_
#define _WELS_THREAD_POOL_TEST_H_

#include "WelsThreadPool.h"

using namespace WelsCommon;

class CThreadPoolTest : public IWelsThreadPoolSink {
 public:
  CThreadPoolTest() {
    m_iTaskCount = 0;
  }

  ~CThreadPoolTest() {}

  virtual int32_t OnTaskExecuted (IWelsTask* pTask) {
    m_iTaskCount ++;
    //printf("Task execute over count is %d\n", m_iTaskCount);
    return cmResultSuccess;
  }

  virtual int32_t OnTaskCancelled (IWelsTask* pTask) {
    m_iTaskCount ++;
    //printf("Task execute cancelled count is %d\n", m_iTaskCount);
    return cmResultSuccess;
  }

  int32_t  GetTaskCount() {
    return m_iTaskCount;
  }

 private:
  int32_t  m_iTaskCount;
};



#endif

