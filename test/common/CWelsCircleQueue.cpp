#include <gtest/gtest.h>
#include "WelsCircleQueue.h"
#include "WelsTask.h"

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

