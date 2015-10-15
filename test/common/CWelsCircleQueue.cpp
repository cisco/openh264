#include <gtest/gtest.h>
#include "WelsCircleQueue.h"
#include "WelsTaskThread.h"

using namespace WelsCommon;

TEST (CWelsCircleQueue, CWelsCircleQueueOne) {
  CWelsCircleQueue<IWelsTask> cTaskList;
  IWelsTask* pTask = NULL;

  for (int i = 0; i < 60; i++) {
    cTaskList.push_back (pTask);
    EXPECT_TRUE (1 == cTaskList.size()) << "after push size=" << cTaskList.size() ;

    cTaskList.pop_front();
    EXPECT_TRUE (0 == cTaskList.size()) << "after pop size=" << cTaskList.size() ;
  }
}

TEST (CWelsCircleQueue, CWelsCircleQueueTen) {
  CWelsCircleQueue<IWelsTask> cTaskList;
  IWelsTask* pTask = NULL;

  for (int j = 0; j < 10; j++) {

    for (int i = 0; i < 10; i++) {
      EXPECT_TRUE (i == cTaskList.size()) << "before push size=" << cTaskList.size() ;
      cTaskList.push_back (pTask);
    }
    EXPECT_TRUE (10 == cTaskList.size()) << "after push size=" << cTaskList.size() ;


    for (int i = 9; i >= 0; i--) {
      cTaskList.pop_front();
      EXPECT_TRUE (i == cTaskList.size()) << "after pop size=" << cTaskList.size() ;
    }
  }
}

TEST (CWelsCircleQueue, CWelsCircleQueueExpand) {
  CWelsCircleQueue<IWelsTask> cTaskList;
  IWelsTask* pTask = NULL;

  const int kiIncreaseNum = (rand() % 65535) + 1;
  const int kiDecreaseNum = rand() % kiIncreaseNum;

  for (int j = 0; j < 10; j++) {

    for (int i = 0; i < kiIncreaseNum; i++) {
      cTaskList.push_back (pTask);
    }
    EXPECT_TRUE (kiIncreaseNum + j * (kiIncreaseNum - kiDecreaseNum) == cTaskList.size()) << "after push size=" <<
        cTaskList.size() ;

    for (int i = kiDecreaseNum; i > 0; i--) {
      cTaskList.pop_front();
    }
    EXPECT_TRUE ((j + 1) * (kiIncreaseNum - kiDecreaseNum) == cTaskList.size()) << "after pop size=" << cTaskList.size() ;
  }
}

TEST (CWelsCircleQueue, CWelsCircleQueueOverPop) {
  CWelsCircleQueue<IWelsTask> cTaskList;
  IWelsTask* pTask = NULL;

  const int kiDecreaseNum = (rand() % 65535) + 1;
  const int kiIncreaseNum = rand() % kiDecreaseNum;

  EXPECT_TRUE (0 == cTaskList.size());
  cTaskList.pop_front();
  EXPECT_TRUE (0 == cTaskList.size());

  for (int i = 0; i < kiIncreaseNum; i++) {
    cTaskList.push_back (pTask);
  }

  for (int i = kiDecreaseNum; i > 0; i--) {
    cTaskList.pop_front();
  }

  EXPECT_TRUE (0 == cTaskList.size());
}

TEST (CWelsCircleQueue, CWelsCircleQueueOnDuplication) {
  int32_t a, b, c;
  CWelsCircleQueue<int32_t> cThreadQueue;
  //CWelsCircleQueue<IWelsTask> cThreadQueue;
  int32_t* pObject1 = &a;
  int32_t* pObject2 = &b;
  int32_t* pObject3 = &c;

  //initial adding
  EXPECT_TRUE (0 == cThreadQueue.push_back (pObject1));
  EXPECT_TRUE (0 == cThreadQueue.push_back (pObject2));
  EXPECT_TRUE (0 == cThreadQueue.push_back (pObject3));
  EXPECT_TRUE (3 == cThreadQueue.size());

  //try failed adding
  EXPECT_FALSE (0 == cThreadQueue.push_back (pObject3));
  EXPECT_TRUE (3 == cThreadQueue.size());

  //try pop
  EXPECT_TRUE (pObject1 == cThreadQueue.begin());
  cThreadQueue.pop_front();
  EXPECT_TRUE (2 == cThreadQueue.size());

  //try what currently in
  EXPECT_TRUE (cThreadQueue.find (pObject2));
  EXPECT_FALSE (0 == cThreadQueue.push_back (pObject2));
  EXPECT_TRUE (cThreadQueue.find (pObject3));
  EXPECT_FALSE (0 == cThreadQueue.push_back (pObject3));
  EXPECT_TRUE (2 == cThreadQueue.size());

  //add back
  EXPECT_TRUE (0 == cThreadQueue.push_back (pObject1));
  EXPECT_TRUE (3 == cThreadQueue.size());

  //another pop
  EXPECT_TRUE (pObject2 == cThreadQueue.begin());
  cThreadQueue.pop_front();
  cThreadQueue.pop_front();
  EXPECT_TRUE (1 == cThreadQueue.size());

  EXPECT_FALSE (0 == cThreadQueue.push_back (pObject1));
  EXPECT_TRUE (1 == cThreadQueue.size());

  EXPECT_TRUE (0 == cThreadQueue.push_back (pObject3));
  EXPECT_TRUE (2 == cThreadQueue.size());

  //clean-up
  while (NULL != cThreadQueue.begin()) {
    cThreadQueue.pop_front();
  }
  EXPECT_TRUE (0 == cThreadQueue.size());
}

#ifndef __APPLE__
TEST (CWelsCircleQueue, CWelsCircleQueueOnThread) {
  CWelsCircleQueue<CWelsTaskThread> cThreadQueue;
  CWelsTaskThread* pTaskThread1 = new CWelsTaskThread (NULL); //this initialization seemed making prob on osx?
  EXPECT_TRUE (NULL != pTaskThread1);
  CWelsTaskThread* pTaskThread2 = new CWelsTaskThread (NULL);
  EXPECT_TRUE (NULL != pTaskThread2);
  CWelsTaskThread* pTaskThread3 = new CWelsTaskThread (NULL);
  EXPECT_TRUE (NULL != pTaskThread3);

  //initial adding
  EXPECT_TRUE (0 == cThreadQueue.push_back (pTaskThread1));
  EXPECT_TRUE (0 == cThreadQueue.push_back (pTaskThread2));
  EXPECT_TRUE (0 == cThreadQueue.push_back (pTaskThread3));
  EXPECT_TRUE (3 == cThreadQueue.size());

  //try failed adding
  EXPECT_FALSE (0 == cThreadQueue.push_back (pTaskThread3));
  EXPECT_TRUE (3 == cThreadQueue.size());

  //try pop
  EXPECT_TRUE (pTaskThread1 == cThreadQueue.begin());
  cThreadQueue.pop_front();
  EXPECT_TRUE (2 == cThreadQueue.size());

  //try what currently in
  EXPECT_TRUE (cThreadQueue.find (pTaskThread2));
  EXPECT_FALSE (0 == cThreadQueue.push_back (pTaskThread2));
  EXPECT_TRUE (cThreadQueue.find (pTaskThread3));
  EXPECT_FALSE (0 == cThreadQueue.push_back (pTaskThread3));
  EXPECT_TRUE (2 == cThreadQueue.size());

  //add back
  EXPECT_TRUE (0 == cThreadQueue.push_back (pTaskThread1));
  EXPECT_TRUE (3 == cThreadQueue.size());

  //another pop
  EXPECT_TRUE (pTaskThread2 == cThreadQueue.begin());
  cThreadQueue.pop_front();
  cThreadQueue.pop_front();
  EXPECT_TRUE (1 == cThreadQueue.size());

  EXPECT_FALSE (0 == cThreadQueue.push_back (pTaskThread1));
  EXPECT_TRUE (1 == cThreadQueue.size());

  EXPECT_TRUE (0 == cThreadQueue.push_back (pTaskThread3));
  EXPECT_TRUE (2 == cThreadQueue.size());

  //clean-up
  while (NULL != cThreadQueue.begin()) {
    cThreadQueue.pop_front();
  }
  EXPECT_TRUE (0 == cThreadQueue.size());

  delete pTaskThread1;
  delete pTaskThread2;
  delete pTaskThread3;
}
#endif

