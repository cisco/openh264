#include <gtest/gtest.h>
#include "WelsList.h"
#include "WelsTaskThread.h"

using namespace WelsCommon;

TEST (CWelsList, CWelsListOne) {
  CWelsList<int> cTestList;
  int a = 0;

  for (int i = 0; i < 60; i++) {
    ASSERT_TRUE (cTestList.push_back (&a));
    EXPECT_TRUE (1 == cTestList.size()) << "after push size=" << cTestList.size() ;

    cTestList.pop_front();
    EXPECT_TRUE (0 == cTestList.size()) << "after pop size=" << cTestList.size() ;
  }
}

TEST (CWelsList, CWelsListTen) {
  CWelsList<int> cTestList;
  int a = 0;
  int* pPointer = &a;

  for (int j = 0; j < 10; j++) {

    for (int i = 0; i < 10; i++) {
      EXPECT_TRUE (i == cTestList.size()) << "before push size=" << cTestList.size() ;
      ASSERT_TRUE (cTestList.push_back (pPointer));
    }
    EXPECT_TRUE (10 == cTestList.size()) << "after push size=" << cTestList.size() ;


    for (int i = 9; i >= 0; i--) {
      cTestList.pop_front();
      EXPECT_TRUE (i == cTestList.size()) << "after pop size=" << cTestList.size() ;
    }
  }
}


TEST (CWelsList, CWelsList_Null) {
  CWelsList<int> cTestList;
  int a = 0;
  int* pPointer = &a;

  ASSERT_TRUE (cTestList.push_back (pPointer));

  pPointer = NULL;
  EXPECT_FALSE (cTestList.push_back (pPointer));

  EXPECT_FALSE (cTestList.findNode (pPointer));

  pPointer = &a;
  EXPECT_TRUE (cTestList.findNode (pPointer));
  ASSERT_TRUE (cTestList.push_back (pPointer));
}


TEST (CWelsList, CWelsListExpand) {
  CWelsList<int> cTestList;
  int a = 0;
  int* pPointer = &a;

  const int kiIncreaseNum = (rand() % 100) + 1;
  const int kiDecreaseNum = rand() % kiIncreaseNum;

  for (int j = 0; j < 10; j++) {

    for (int i = 0; i < kiIncreaseNum; i++) {
      ASSERT_TRUE (cTestList.push_back (pPointer));
    }
    EXPECT_TRUE (kiIncreaseNum + j * (kiIncreaseNum - kiDecreaseNum) == cTestList.size()) << "after push size=" <<
        cTestList.size() ;

    for (int i = kiDecreaseNum; i > 0; i--) {
      cTestList.pop_front();
    }
    EXPECT_TRUE ((j + 1) * (kiIncreaseNum - kiDecreaseNum) == cTestList.size()) << "after pop size=" << cTestList.size() ;
  }
}

TEST (CWelsList, CWelsListOverPop) {
  CWelsList<int> cTestList;
  int a = 0;
  int* pPointer = &a;

  const int kiDecreaseNum = 30000;//(rand() % 65535) + 1;
  const int kiIncreaseNum = rand() % kiDecreaseNum;

  EXPECT_TRUE (0 == cTestList.size());
  cTestList.pop_front();
  EXPECT_TRUE (0 == cTestList.size());

  for (int i = 0; i < kiIncreaseNum; i++) {
    ASSERT_TRUE (cTestList.push_back (pPointer));
  }

  for (int i = kiDecreaseNum; i > 0; i--) {
    cTestList.pop_front();
  }

  EXPECT_TRUE (0 == cTestList.size());
}


void EraseOneInList (CWelsList<int>& cTestList, int* pPointer) {
  int iPrevSize = cTestList.size();
  EXPECT_TRUE (cTestList.erase (pPointer));
  EXPECT_TRUE (cTestList.size() == (iPrevSize - 1));
}

TEST (CWelsList, CWelsListEraseOne) {
#define TEST_LEN (4)
  CWelsList<int> cTestList;
  int a[TEST_LEN];
  int* pPointer;

  for (int i = 0; i < TEST_LEN; i++) {
    a[i] = i;
    ASSERT_TRUE (cTestList.push_back (&a[i]));
  }

  EXPECT_TRUE (cTestList.size() == TEST_LEN);

  int iEraseIdx = rand() % TEST_LEN;
  EraseOneInList (cTestList, &a[iEraseIdx]);
  EXPECT_TRUE (cTestList.size() == (TEST_LEN - 1));

  for (int i = 0; i < TEST_LEN; i++) {
    pPointer = cTestList.begin();
    cTestList.pop_front();
    if (!pPointer) {
      EXPECT_TRUE (cTestList.size() == 0);
      break;
    }
    if (i < iEraseIdx) {
      EXPECT_TRUE (a[i] == (*pPointer));
    } else {
      EXPECT_TRUE (a[i + 1] == (*pPointer));
    }
  }

  EXPECT_TRUE (0 == cTestList.size());
}

TEST (CWelsList, CWelsListEraseAll) {
#define TEST_LEN (4)
  CWelsList<int> cTestList;
  int data[TEST_LEN];
  int eraseidx[TEST_LEN] = {0};
  int* pPointer;

  for (int i = 0; i < TEST_LEN; i++) {
    data[i] = i;
    ASSERT_TRUE (cTestList.push_back (&data[i]));
  }
  EXPECT_TRUE (cTestList.size() == TEST_LEN);

  int iCurrentLen = TEST_LEN;
  do {
    int iEraseIdx = rand() % TEST_LEN;
    if (0 == eraseidx[iEraseIdx]) {
      eraseidx[iEraseIdx] = 1;
      EraseOneInList (cTestList, &data[iEraseIdx]);
      EXPECT_TRUE (cTestList.size() == (--iCurrentLen));
    }
    EXPECT_FALSE (cTestList.erase (&data[iEraseIdx]));

    if (cTestList.size() == 0) {
      break;
    }

    pPointer = cTestList.begin();
    for (int i = 0; i < TEST_LEN; i++) {
      if ((*pPointer) == data[i]) {
        EXPECT_TRUE (eraseidx[i] == 0);
        break;
      }
    }
  } while (cTestList.size());
  EXPECT_TRUE (0 == cTestList.size());
}

TEST (CWelsList, CWelsListEraseAndExpand) {
#define TEST_LEN_10 (10)
  CWelsList<int> cTestList;
  int data[TEST_LEN_10];
  int eraseidx[TEST_LEN_10] = {0};
  int* pPointer;

  for (int i = 0; i < TEST_LEN_10; i++) {
    data[i] = i;
    ASSERT_TRUE (cTestList.push_back (&data[i]));
  }
  EXPECT_TRUE (cTestList.size() == TEST_LEN_10);

  //erase some
  int iCurrentLen = TEST_LEN_10;
  do {
    int iEraseIdx = rand() % TEST_LEN_10;
    if (0 == eraseidx[iEraseIdx]) {
      eraseidx[iEraseIdx] = 1;
      EraseOneInList (cTestList, &data[iEraseIdx]);
      EXPECT_TRUE (cTestList.size() == (--iCurrentLen));
    }
    EXPECT_FALSE (cTestList.erase (&data[iEraseIdx]));

    if (cTestList.size() == 0) {
      break;
    }

    pPointer = cTestList.begin();
    for (int i = 0; i < TEST_LEN_10; i++) {
      if ((*pPointer) == data[i]) {
        EXPECT_TRUE (eraseidx[i] == 0);
        break;
      }
    }
  } while (iCurrentLen > (TEST_LEN_10 / 2));
  EXPECT_TRUE (iCurrentLen == cTestList.size());

  //expand
  int iAddLen = rand() % 65535;
  for (int i = 0; i < iAddLen; i++) {
    ASSERT_TRUE (cTestList.push_back (&data[0]));
  }
  EXPECT_TRUE ((iCurrentLen + iAddLen) == cTestList.size());
  EraseOneInList (cTestList, &data[0]);
  EXPECT_TRUE ((iCurrentLen + iAddLen - 1) == cTestList.size()) << (iCurrentLen + iAddLen - 1)  << "_" <<
      cTestList.size();

  //clear up
  do {
    pPointer = cTestList.begin();
    EXPECT_TRUE (NULL != pPointer);
    cTestList.pop_front();
  } while (cTestList.size());

  EXPECT_TRUE (0 == cTestList.size());
}

TEST (CWelsNonDuplicatedList, CWelsNonDuplicatedList) {
  int32_t a, b, c;
  CWelsNonDuplicatedList<int32_t> cNonDuplicatedIntList;
  int32_t* pObject1 = &a;
  int32_t* pObject2 = &b;
  int32_t* pObject3 = &c;

  //initial adding
  ASSERT_TRUE (cNonDuplicatedIntList.push_back (pObject1));
  ASSERT_TRUE (cNonDuplicatedIntList.push_back (pObject2));
  ASSERT_TRUE (cNonDuplicatedIntList.push_back (pObject3));
  EXPECT_TRUE (3 == cNonDuplicatedIntList.size());

  //try failed adding
  EXPECT_FALSE (cNonDuplicatedIntList.push_back (pObject3));
  EXPECT_TRUE (3 == cNonDuplicatedIntList.size());

  //try pop
  EXPECT_TRUE (pObject1 == cNonDuplicatedIntList.begin());
  cNonDuplicatedIntList.pop_front();
  EXPECT_TRUE (2 == cNonDuplicatedIntList.size());

  //try what currently in
  EXPECT_TRUE (cNonDuplicatedIntList.findNode (pObject2));
  EXPECT_FALSE (cNonDuplicatedIntList.push_back (pObject2));
  EXPECT_TRUE (cNonDuplicatedIntList.findNode (pObject3));
  EXPECT_FALSE (cNonDuplicatedIntList.push_back (pObject3));
  EXPECT_TRUE (2 == cNonDuplicatedIntList.size());

  //add back
  ASSERT_TRUE (cNonDuplicatedIntList.push_back (pObject1));
  EXPECT_TRUE (3 == cNonDuplicatedIntList.size());

  //another pop
  EXPECT_TRUE (pObject2 == cNonDuplicatedIntList.begin());
  cNonDuplicatedIntList.pop_front();
  cNonDuplicatedIntList.pop_front();
  EXPECT_TRUE (1 == cNonDuplicatedIntList.size());

  EXPECT_FALSE (cNonDuplicatedIntList.push_back (pObject1));
  EXPECT_TRUE (1 == cNonDuplicatedIntList.size());

  ASSERT_TRUE (cNonDuplicatedIntList.push_back (pObject3));
  EXPECT_TRUE (2 == cNonDuplicatedIntList.size());

  //clean-up
  while (NULL != cNonDuplicatedIntList.begin()) {
    cNonDuplicatedIntList.pop_front();
  }
  EXPECT_TRUE (0 == cNonDuplicatedIntList.size());
}

#ifndef __APPLE__
TEST (CWelsNonDuplicatedList, CWelsNonDuplicatedListOnThread) {
  CWelsNonDuplicatedList<CWelsTaskThread> cThreadList;
  CWelsTaskThread* pTaskThread1 = new CWelsTaskThread (NULL); //this initialization seemed making prob on osx?
  EXPECT_TRUE (NULL != pTaskThread1);
  CWelsTaskThread* pTaskThread2 = new CWelsTaskThread (NULL);
  EXPECT_TRUE (NULL != pTaskThread2);
  CWelsTaskThread* pTaskThread3 = new CWelsTaskThread (NULL);
  EXPECT_TRUE (NULL != pTaskThread3);

  //initial adding
  ASSERT_TRUE (cThreadList.push_back (pTaskThread1));
  ASSERT_TRUE (cThreadList.push_back (pTaskThread2));
  ASSERT_TRUE (cThreadList.push_back (pTaskThread3));
  EXPECT_TRUE (3 == cThreadList.size());

  //try failed adding
  EXPECT_FALSE (cThreadList.push_back (pTaskThread3));
  EXPECT_TRUE (3 == cThreadList.size());

  //try pop
  EXPECT_TRUE (pTaskThread1 == cThreadList.begin());
  cThreadList.pop_front();
  EXPECT_TRUE (2 == cThreadList.size());

  //try what currently in
  EXPECT_TRUE (cThreadList.findNode (pTaskThread2));
  EXPECT_FALSE (cThreadList.push_back (pTaskThread2));
  EXPECT_TRUE (cThreadList.findNode (pTaskThread3));
  EXPECT_FALSE (cThreadList.push_back (pTaskThread3));
  EXPECT_TRUE (2 == cThreadList.size());

  //add back
  ASSERT_TRUE (cThreadList.push_back (pTaskThread1));
  EXPECT_TRUE (3 == cThreadList.size());

  //another pop
  EXPECT_TRUE (pTaskThread2 == cThreadList.begin());
  cThreadList.pop_front();
  cThreadList.pop_front();
  EXPECT_TRUE (1 == cThreadList.size());

  EXPECT_FALSE (cThreadList.push_back (pTaskThread1));
  EXPECT_TRUE (1 == cThreadList.size());

  ASSERT_TRUE (cThreadList.push_back (pTaskThread3));
  EXPECT_TRUE (2 == cThreadList.size());

  //clean-up
  while (NULL != cThreadList.begin()) {
    cThreadList.pop_front();
  }
  EXPECT_TRUE (0 == cThreadList.size());

  delete pTaskThread1;
  delete pTaskThread2;
  delete pTaskThread3;
}
#endif


TEST (CWelsList, CWelsListReadWithIdx) {
  CWelsList<int32_t> cThreadList;
  const int kiIncreaseNum = (rand() % 1000) + 1;
  const int kiDecreaseNum = rand() % kiIncreaseNum;

  int32_t* pInput = static_cast<int32_t*> (malloc (kiIncreaseNum * 10 * sizeof (int32_t)));
  if (!pInput) {
    return;
  }
  for (int32_t i = 0; i < kiIncreaseNum * 10; i++) {
    pInput[i] = i;
  }

  for (int j = 0; j < 10; j++) {
    const int iBias = j * (kiIncreaseNum - kiDecreaseNum);
    for (int i = 0; i < kiIncreaseNum; i++) {
      ASSERT_TRUE (cThreadList.push_back (&pInput[i + kiIncreaseNum * j]));
    }
    EXPECT_TRUE (kiIncreaseNum + iBias == cThreadList.size()) << "after push size=" <<
        cThreadList.size() ;

    EXPECT_TRUE ((kiDecreaseNum * j) == * (cThreadList.begin()));

    for (int i = 0; i < kiIncreaseNum; i++) {
      EXPECT_TRUE ((i + kiIncreaseNum * j) == * (cThreadList.getNode (i + iBias)));
    }
    for (int i = 0; i < cThreadList.size(); i++) {
      EXPECT_TRUE ((i + kiDecreaseNum * j) == * (cThreadList.getNode (i)));
    }

    for (int i = kiDecreaseNum; i > 0; i--) {
      cThreadList.pop_front();
    }
    EXPECT_TRUE ((j + 1) * (kiIncreaseNum - kiDecreaseNum) == cThreadList.size()) << "after pop size=" <<
        cThreadList.size() ;

    EXPECT_TRUE ((kiDecreaseNum * (j + 1)) == * (cThreadList.begin()));
  }

  //clean-up
  while (NULL != cThreadList.begin()) {
    cThreadList.pop_front();
  }
  EXPECT_TRUE (0 == cThreadList.size());
  free (pInput);
}
