#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include <list>
#include <map>

#include "typedefs.h"
#include "WelsList.h"
#include "WelsTask.h"
#include "WelsThreadPoolTest.h"


TEST (CThreadPoolTest, CThreadPoolTest_List) {
  
  CWelsList<IWelsTask>* pTaskList;
  pTaskList = new CWelsList<IWelsTask>();
  ASSERT_TRUE(NULL != pTaskList);
  
  CThreadPoolTest cThreadPoolTest;
  CThreadPoolTest cThreadPoolTest1;
  CSimpleTask* aTasks[2] = NULL;
  IWelsTask* pCurTask;
  
  int32_t  i;
  aTasks[0] = new CSimpleTask (&cThreadPoolTest);
  aTasks[1] = new CSimpleTask (&cThreadPoolTest1);
  pTaskList->push_back(aTasks[0]);
  pTaskList->push_back(aTasks[1]);
  EXPECT_TRUE(2 == pTaskList->size());
  
  pCurTask = pTaskList->begin();
  EXPECT_TRUE(pCurTask->GetSink() == &cThreadPoolTest);
  pCurTask->GetSink()->OnTaskExecuted();//cThreadPoolTest
  pTaskList->pop_front();
  
  EXPECT_TRUE(1 == pTaskList->size());
  
  pCurTask = pTaskList->begin();
  EXPECT_TRUE(pCurTask->GetSink() == &cThreadPoolTest1);
  pCurTask->GetSink()->OnTaskExecuted();//cThreadPoolTest1
  pTaskList->pop_front();
  
  pTaskList->push_back(aTasks[1]);
  EXPECT_TRUE(1 == pTaskList->size());
  
  pCurTask = pTaskList->begin();
  EXPECT_TRUE(pCurTask->GetSink() == &cThreadPoolTest1);
  pCurTask->GetSink()->OnTaskExecuted();//cThreadPoolTest1
  pTaskList->pop_front();
  
  EXPECT_TRUE(1 == cThreadPoolTest.GetTaskCount());
  EXPECT_TRUE(2 == cThreadPoolTest1.GetTaskCount());
  
  EXPECT_TRUE(0 == pTaskList->size());
  for (i = 0; i < 2; i++) {
    delete aTasks[i];
    aTasks[i] = NULL;
  }
  
  aTasks[0] = new CSimpleTask (&cThreadPoolTest1);
  aTasks[1] = new CSimpleTask (&cThreadPoolTest);
  pTaskList->push_back(aTasks[0]);
  pTaskList->push_back(aTasks[1]);
  pCurTask = pTaskList->begin();
  EXPECT_TRUE(pCurTask->GetSink() == &cThreadPoolTest1);
  pCurTask->GetSink()->OnTaskExecuted();//cThreadPoolTest1:3
  pTaskList->pop_front();
  EXPECT_TRUE(1 == pTaskList->size());

  pTaskList->push_back(aTasks[0]);
  EXPECT_TRUE(2 == pTaskList->size());
  
  pCurTask = pTaskList->begin();
  EXPECT_TRUE(pCurTask->GetSink() == &cThreadPoolTest);
  pCurTask->GetSink()->OnTaskExecuted();//cThreadPoolTest:2
  pTaskList->pop_front();
  pCurTask = pTaskList->begin();
  EXPECT_TRUE(pCurTask->GetSink() == &cThreadPoolTest1);
  pCurTask->GetSink()->OnTaskExecuted();//cThreadPoolTest1:4
  pTaskList->pop_front();
  
  EXPECT_TRUE(2 == cThreadPoolTest.GetTaskCount());
  EXPECT_TRUE(4 == cThreadPoolTest1.GetTaskCount());
  
  for (i = 0; i < 2; i++) {
    delete aTasks[i];
    aTasks[i] = NULL;
  }
  delete pTaskList;
}


