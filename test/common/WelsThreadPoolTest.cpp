#include "WelsThreadPoolTest.h"

#include <gtest/gtest.h>
#include <string.h>

#include <list>
#include <map>
#include <string>

#include "WelsTask.h"
#include "WelsThreadLib.h"
#include "WelsThreadPool.h"
#include "WelsThreadPoolTestUtil.h"
#include "typedefs.h"

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

class CThreadPoolTestFixture : public ::testing::Test {
 protected:
  virtual void SetUp() { WelsThreadPoolTestUtil::SetupSignalHandler(); }

  virtual void TearDown() {
    WelsThreadPoolTestUtil::RemoveThreadLimit();
    WelsThreadPoolTestUtil::RestoreSignalHandler();
  }
};

TEST_F(CThreadPoolTestFixture, PartialInitLeakUAF) {
  WelsThreadPoolTestUtil::SThreadLimitResult sLimit =
      WelsThreadPoolTestUtil::FindSingleThreadLimit();
  if (!sLimit.bSupported) {
    printf("Thread limit manipulation is not supported, skipping test.\n");
    return;
  }

  WelsThreadPoolTestUtil::CThreadLimitGuard cLimitGuard;

  ASSERT_TRUE(WelsThreadPoolTestUtil::SetThreadLimit(sLimit.uiLimit));

  // Request 2 threads: the first worker thread (i = 0) succeeds, while
  // attempting to create the second (i = 1) fails due to our 1-thread limit,
  // triggering a controlled partial initialization failure.
  CWelsThreadPool::SetThreadNum(2);
  CWelsThreadPool* pPool = CWelsThreadPool::AddReference();
  if (pPool != NULL) {
    pPool->RemoveInstance();
    GTEST_SKIP() << "Thread limit did not force partial initialization failure in this environment.";
  }
  EXPECT_EQ(NULL, pPool);

  // Restore old rlimit immediately so we can create threads/signals normally.
  WelsThreadPoolTestUtil::RemoveThreadLimit();

  WelsThreadPoolTestUtil::SendSignalToOtherThreads();
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

class CImmediateTask : public IWelsTask {
 public:
  explicit CImmediateTask (WelsCommon::IWelsTaskSink* pSink) : IWelsTask (pSink) {}
  virtual ~CImmediateTask() {}
  virtual int32_t Execute() {
    return cmResultSuccess;
  }
};

// Sink whose OnTaskExecuted() signals the test thread upon entry and waits
// until the test thread is ready to call RemoveInstance() before lingering.
class CLingeringSink : public CThreadPoolTest {
 public:
  CLingeringSink() : m_iCallbacksReturned (0) {}

  virtual int32_t OnTaskExecuted() {
    int32_t iRet = CThreadPoolTest::OnTaskExecuted();
    m_cCallbackEntered.Signal();
    m_cRemoveStarted.Wait();
    WelsSleep (5);
    m_iCallbacksReturned ++;
    return iRet;
  }

  CSyncEvent m_cCallbackEntered;
  CSyncEvent m_cRemoveStarted;
  volatile int32_t m_iCallbacksReturned;
};

} // namespace

// Once RemoveInstance() has returned, the pool must no longer be inside any
// callback of the caller's sink, even when other references keep the pool and
// its worker threads alive.
TEST (CThreadPoolTest, NoCallbackInFlightAfterRemoveInstance) {
  CWelsThreadPool* pKeepAlive = CWelsThreadPool::AddReference();
  ASSERT_TRUE (pKeepAlive != NULL);

  for (int32_t i = 0; i < 5; i++) {
    CLingeringSink cSink;
    CImmediateTask cTask (&cSink);
    CWelsThreadPool* pPool = CWelsThreadPool::AddReference();
    ASSERT_TRUE (pPool != NULL);
    ASSERT_EQ (WELS_THREAD_ERROR_OK, pPool->QueueTask (&cTask));
    cSink.m_cCallbackEntered.Wait();
    cSink.m_cRemoveStarted.Signal();
    pPool->RemoveInstance();
    EXPECT_EQ (1, cSink.m_iCallbacksReturned) << "iteration " << i;
    while (cSink.m_iCallbacksReturned < 1) {
      WelsSleep (1);  // keep cSink alive until the worker is done with it
    }
  }

  pKeepAlive->RemoveInstance();
}

