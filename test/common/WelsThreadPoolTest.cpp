#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include <list>
#include <map>

#include "typedefs.h"
#include "WelsThreadLib.h"
#include "WelsThreadPool.h"
#include "WelsTask.h"
#include "WelsThreadPoolTest.h"

#define  TEST_TASK_NUM  20

class CSimpleTask : public IWelsTask {
 public:
  static uint32_t id;

  CSimpleTask (WelsCommon::IWelsTaskSink* pSink) : IWelsTask (pSink) {
    m_uiID = id ++;
  }

  virtual ~CSimpleTask() {
  }

  virtual int32_t Execute() {
    WelsSleep (300 - m_uiID);
    //printf ("Task %d executing\n", m_uiID);
    return cmResultSuccess;
  }

 private:
  uint32_t m_uiID;
};

uint32_t CSimpleTask::id = 0;


TEST (CThreadPoolTest, CThreadPoolTest) {
  CThreadPoolTest cThreadPoolTest;
  CSimpleTask* aTasks[TEST_TASK_NUM];
  CWelsThreadPool  cThreadPool (&cThreadPoolTest);

  int32_t  i;
  for (i = 0; i < TEST_TASK_NUM; i++) {
    aTasks[i] = new CSimpleTask (&cThreadPoolTest);
  }

  for (i = 0; i < TEST_TASK_NUM; i++) {
    cThreadPool.QueueTask (aTasks[i]);
  }

  while (cThreadPoolTest.GetTaskCount() < TEST_TASK_NUM) {
    WelsSleep (1);
  }

  for (i = 0; i < TEST_TASK_NUM; i++) {
    delete aTasks[i];
  }
}

