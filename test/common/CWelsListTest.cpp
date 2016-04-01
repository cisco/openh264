#include <gtest/gtest.h>
#include "WelsList.h"

using namespace WelsCommon;

TEST (CWelsList, CWelsListOne) {
  CWelsList<int> cTestList;
  int a = 0;

  for (int i = 0; i < 60; i++) {
    cTestList.push_back (&a);
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
      cTestList.push_back (pPointer);
    }
    EXPECT_TRUE (10 == cTestList.size()) << "after push size=" << cTestList.size() ;


    for (int i = 9; i >= 0; i--) {
      cTestList.pop_front();
      EXPECT_TRUE (i == cTestList.size()) << "after pop size=" << cTestList.size() ;
    }
  }
}

TEST (CWelsList, CWelsListExpand) {
  CWelsList<int> cTestList;
  int a = 0;
  int* pPointer = &a;

  const int kiIncreaseNum = (rand() % 100) + 1;
  const int kiDecreaseNum = rand() % kiIncreaseNum;

  for (int j = 0; j < 10; j++) {

    for (int i = 0; i < kiIncreaseNum; i++) {
      EXPECT_TRUE (cTestList.push_back (pPointer));
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
    EXPECT_TRUE (cTestList.push_back (pPointer));
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
    cTestList.push_back (&a[i]);
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
    cTestList.push_back (&data[i]);
  }
  EXPECT_TRUE (cTestList.size() == TEST_LEN);

  for (int i = 0; i < TEST_LEN; i++) {
    EXPECT_TRUE ( *(cTestList.index(i)) == data[i]);
  }
  
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
    cTestList.push_back (&data[i]);
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
    EXPECT_TRUE (cTestList.push_back (&data[0]));
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

