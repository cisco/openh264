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

uint32_t CSimpleTask::id = 0;

WELS_THREAD_ROUTINE_TYPE OneCallingFunc(void *) {
  CThreadPoolTest cThreadPoolTest;
  CSimpleTask* aTasks[TEST_TASK_NUM];
  CWelsThreadPool* pThreadPool = (CWelsThreadPool::AddReference());
  if (pThreadPool == NULL)
    return 0;

  int32_t  i;
  for (i = 0; i < TEST_TASK_NUM; i++) {
    aTasks[i] = new CSimpleTask (&cThreadPoolTest);
  }

  for (i = 0; i < TEST_TASK_NUM; i++) {
    pThreadPool->QueueTask (aTasks[i]);
  }

  while (cThreadPoolTest.GetTaskCount() < TEST_TASK_NUM) {
    WelsSleep (1);
  }

  for (i = 0; i < TEST_TASK_NUM; i++) {
    delete aTasks[i];
  }
  pThreadPool->RemoveInstance();

  return 0;
}


TEST (CThreadPoolTest, CThreadPoolTest) {
  OneCallingFunc(NULL);

  int iRet = CWelsThreadPool::SetThreadNum (8);
  EXPECT_EQ (0, iRet);
  EXPECT_FALSE (CWelsThreadPool::IsReferenced());

  CWelsThreadPool* pThreadPool = (CWelsThreadPool::AddReference());
  ASSERT_TRUE (pThreadPool != NULL);

  EXPECT_TRUE (pThreadPool->IsReferenced());

  EXPECT_EQ (8, pThreadPool->GetThreadNum());

  iRet = CWelsThreadPool::SetThreadNum (4);
  EXPECT_TRUE (0 != iRet);
  EXPECT_EQ (8, pThreadPool->GetThreadNum());

  pThreadPool->RemoveInstance();

  iRet = CWelsThreadPool::SetThreadNum (4);
  EXPECT_EQ (0, iRet);

  pThreadPool = (CWelsThreadPool::AddReference());
  EXPECT_TRUE (pThreadPool->IsReferenced());
  EXPECT_EQ (4, pThreadPool->GetThreadNum());
  pThreadPool->RemoveInstance();

  EXPECT_FALSE (CWelsThreadPool::IsReferenced());
}


TEST (CThreadPoolTest, CThreadPoolTestMulti) {
  int iCallingNum = 10;
  WELS_THREAD_HANDLE mThreadID[30];
  int i = 0;
  WELS_THREAD_ERROR_CODE rc;
  for (i = 0; i < iCallingNum; i++) {
    rc = WelsThreadCreate (& (mThreadID[i]), OneCallingFunc, NULL, 0);
    ASSERT_TRUE (rc == WELS_THREAD_ERROR_OK);
    WelsSleep (1);
  }
  for (i = iCallingNum; i < iCallingNum * 2; i++) {
    rc = WelsThreadCreate (& (mThreadID[i]), OneCallingFunc, NULL, 0);
    ASSERT_TRUE (rc == WELS_THREAD_ERROR_OK);
    WelsSleep (1);
    WelsThreadJoin (mThreadID[i]);
  }
  for (i = 0; i < iCallingNum; i++) {
    WelsThreadJoin (mThreadID[i]);
  }
  for (i = iCallingNum * 2; i < iCallingNum * 3; i++) {
    rc = WelsThreadCreate (& (mThreadID[i]), OneCallingFunc, NULL, 0);
    ASSERT_TRUE (rc == WELS_THREAD_ERROR_OK);
    WelsSleep (1);
    WelsThreadJoin (mThreadID[i]);
  }

  EXPECT_FALSE (CWelsThreadPool::IsReferenced());
}

