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

#define  TEST_TASK_NUM  30

class CSimpleTask : public IWelsTask {
 public:
  static uint32_t id;

  CSimpleTask (WelsCommon::IWelsTaskSink* pSink) : IWelsTask (pSink) {
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

void* OneCallingFunc() {
  CThreadPoolTest cThreadPoolTest;
  CSimpleTask* aTasks[TEST_TASK_NUM];
  CWelsThreadPool* pThreadPool = & (CWelsThreadPool::AddReference());

  int32_t  i;
  for (i = 0; i < TEST_TASK_NUM; i++) {
    aTasks[i] = new CSimpleTask (&cThreadPoolTest);
    //fprintf (stdout, "OneCallingFunc pTask=%x pSink=%x\n", aTasks[i], aTasks[i]->GetSink());
  }

  for (i = 0; i < TEST_TASK_NUM; i++) {
    pThreadPool->QueueTask (aTasks[i]);
  }

  //while (cThreadPoolTest.GetTaskCount() < TEST_TASK_NUM) {
  //  WelsSleep (1);
  //}

  pThreadPool->RemoveInstance (&cThreadPoolTest);
  for (i = 0; i < TEST_TASK_NUM; i++) {
    //fprintf (stdout, "OneCallingFunc delete pTask=%x pSink=%x\n", aTasks[i], aTasks[i]->GetSink());
    delete aTasks[i];
  }
  return 0;
}


TEST (CThreadPoolTest, CThreadPoolTest) {
  OneCallingFunc();

  int iRet = CWelsThreadPool::SetThreadNum (8);
  EXPECT_EQ (0, iRet);
  EXPECT_FALSE (CWelsThreadPool::IsReferenced());

  CWelsThreadPool* pThreadPool = & (CWelsThreadPool::AddReference());
  EXPECT_TRUE (pThreadPool->IsReferenced());

  EXPECT_EQ (8, pThreadPool->GetThreadNum());

  iRet = CWelsThreadPool::SetThreadNum (4);
  EXPECT_TRUE (0 != iRet);
  EXPECT_EQ (8, pThreadPool->GetThreadNum());

  pThreadPool->RemoveInstance (NULL);

  iRet = CWelsThreadPool::SetThreadNum (4);
  EXPECT_EQ (0, iRet);

  pThreadPool = & (CWelsThreadPool::AddReference());
  EXPECT_TRUE (pThreadPool->IsReferenced());
  EXPECT_EQ (4, pThreadPool->GetThreadNum());
  pThreadPool->RemoveInstance (NULL);

  EXPECT_FALSE (CWelsThreadPool::IsReferenced());
}


TEST (CThreadPoolTest, CThreadPoolTestMulti) {
  int iCallingNum = 1;
  WELS_THREAD_HANDLE mThreadID[30];
  int i = 0;
  while (1) {
    for (i = 0; i < iCallingNum; i++) {
      WelsThreadCreate (& (mThreadID[i]), (LPWELS_THREAD_ROUTINE)OneCallingFunc, NULL, 0);
      WelsSleep (1);
    }

    for (i = iCallingNum; i < iCallingNum * 2; i++) {
      WelsThreadCreate (& (mThreadID[i]), (LPWELS_THREAD_ROUTINE)OneCallingFunc, NULL, 0);
      WelsSleep (1);
      WelsThreadJoin (mThreadID[i]);
    }

    for (i = 0; i < iCallingNum; i++) {
      WelsThreadJoin (mThreadID[i]);
    }

    for (i = iCallingNum * 2; i < iCallingNum * 3; i++) {
      WelsThreadCreate (& (mThreadID[i]), (LPWELS_THREAD_ROUTINE)OneCallingFunc, NULL, 0);
      WelsSleep (1);
      WelsThreadJoin (mThreadID[i]);
    }

    EXPECT_FALSE (CWelsThreadPool::IsReferenced());
  }
}

