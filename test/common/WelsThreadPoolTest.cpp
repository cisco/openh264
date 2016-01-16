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

  CSimpleTask() {
    m_uiID = id ++;
  }

  virtual ~CSimpleTask() {
  }

  virtual int32_t Execute() {
    uint32_t uiSleepTime = (m_uiID > 99) ? 10 : m_uiID;
    WelsSleep (uiSleepTime);
    //printf ("Task %d executing\n", m_uiID);
    return cmResultSuccess;
  }

 private:
  uint32_t m_uiID;
};

uint32_t CSimpleTask::id = 0;


TEST (CThreadPoolTest, CThreadPoolTest) {
  CSimpleTask tasks[TEST_TASK_NUM];
  CThreadPoolTest cThreadPoolTest;
  CWelsThreadPool  cThreadPool (&cThreadPoolTest);

  int32_t  i;

  for (i = 0; i < TEST_TASK_NUM; i++) {
    cThreadPool.QueueTask (&tasks[i]);
  }

  while (cThreadPoolTest.GetTaskCount() < TEST_TASK_NUM) {
    WelsSleep (1);
  }
}

