#include<gtest/gtest.h>
#include<stdlib.h>

#include "wels_func_ptr_def.h"
#include "expand_pic.h"

using namespace WelsSVCEnc;

TEST (ExpandPicTest, TestExpandPictureLuma_c) {
  SWelsFuncPtrList sFuncList;
  InitExpandPictureFunc (& (sFuncList.sExpandPicFunc), 0);

  int32_t iPicW = rand() % 256 + 1;
  int32_t iPicH = rand() % 256 + 1;
  int32_t iStride = iPicW + rand() % 16 + 1 + PADDING_LENGTH * 2;

  const int32_t kiPaddingLen = PADDING_LENGTH;
  const int32_t kiMemSize = (iStride + kiPaddingLen * 2) * (iPicH + kiPaddingLen * 2);

  uint8_t* pRef = new uint8_t[kiMemSize];

  for (int i = 0; i < kiMemSize; i++)
    pRef[i] = rand() % 256 + 1;

  uint8_t* pDst = pRef + kiPaddingLen * iStride + kiPaddingLen;

  sFuncList.sExpandPicFunc.pfExpandLumaPicture (pDst, iStride, iPicW, iPicH);

  int k = 0;
  //top and top corner
  for (int i = 0; i < kiPaddingLen; i++) {
    for (int j = 0; j < iPicW + 2 * kiPaddingLen; j++) {
      if (j < kiPaddingLen) {
        EXPECT_EQ (pRef[k + j], pDst[0]);
      } else if (j >= iPicW + kiPaddingLen) {
        EXPECT_EQ (pRef[k + j], pDst[iPicW - 1]);
      } else
        EXPECT_EQ (pRef[k + j], pDst[j - kiPaddingLen]);
    }
    k += iStride;
  }

  k = (iPicH + kiPaddingLen - 1) * iStride;
  //bottom and bottom corner
  for (int i = iPicH + kiPaddingLen; i < iPicH + 2 * kiPaddingLen; i++) {
    for (int j = 0; j < iPicW + 2 * kiPaddingLen; j++) {
      if (j < kiPaddingLen) {
        EXPECT_EQ (pRef[k + j], pDst[ (iPicH - 1) * iStride]);
      } else if (j >= iPicW + kiPaddingLen) {
        EXPECT_EQ (pRef[k + j], pDst[ (iPicH - 1) * iStride + iPicW - 1]);
      } else
        EXPECT_EQ (pRef[k + j], pDst[ (iPicH - 1) * iStride + j - kiPaddingLen]);
    }
    k += iStride;
  }

  k = kiPaddingLen * iStride;
  int l = 0;
  for (int i = 0; i < iPicH - 1; i++)	{ //left
    for (int j = 0; j < kiPaddingLen; j++) {
      EXPECT_EQ (pRef[k + j], pDst[l]);
    }
    k += iStride;
    l += iStride;
  }

  k = kiPaddingLen * iStride;
  l = 0;
  for (int i = 0; i < iPicH - 1; i++) { //right
    for (int j = iPicW + kiPaddingLen; j < iPicW + 2 * kiPaddingLen; j++) {
      EXPECT_EQ (pRef[k + j], pDst[l + iPicW - 1]);
    }
    k += iStride;
    l += iStride;
  }

  delete []pRef;
}

TEST (ExpandPicTest, TestExpandPictureChroma_c) {
  SWelsFuncPtrList sFuncList;
  InitExpandPictureFunc (& (sFuncList.sExpandPicFunc), 0);

  int32_t iPicW = rand() % 256 + 1;
  int32_t iPicH = rand() % 256 + 1;

  const int32_t kiPaddingLen = (PADDING_LENGTH >> 1);
  int32_t iStride = iPicW + rand() % 16 + 1 + kiPaddingLen * 2;

  const int32_t kiMemSize = (iStride + kiPaddingLen * 2) * (iPicH + kiPaddingLen * 2);

  uint8_t* pRef = new uint8_t[kiMemSize];

  for (int i = 0; i < kiMemSize; i++)
    pRef[i] = rand() % 256 + 1;

  uint8_t* pDst = pRef + kiPaddingLen * iStride + kiPaddingLen;

  sFuncList.sExpandPicFunc.pfExpandChromaPicture[0] (pDst, iStride, iPicW, iPicH);

  int k = 0;
  //top and top corner
  for (int i = 0; i < kiPaddingLen; i++) {
    for (int j = 0; j < iPicW + 2 * kiPaddingLen; j++) {
      if (j < kiPaddingLen) {
        EXPECT_EQ (pRef[k + j], pDst[0]);
      } else if (j >= iPicW + kiPaddingLen) {
        EXPECT_EQ (pRef[k + j], pDst[iPicW - 1]);
      } else
        EXPECT_EQ (pRef[k + j], pDst[j - kiPaddingLen]);
    }
    k += iStride;
  }

  k = (iPicH + kiPaddingLen - 1) * iStride;
  //bottom and bottom corner
  for (int i = iPicH + kiPaddingLen; i < iPicH + 2 * kiPaddingLen; i++) {
    for (int j = 0; j < iPicW + 2 * kiPaddingLen; j++) {
      if (j < kiPaddingLen) {
        EXPECT_EQ (pRef[k + j], pDst[ (iPicH - 1) * iStride]);
      } else if (j >= iPicW + kiPaddingLen) {
        EXPECT_EQ (pRef[k + j], pDst[ (iPicH - 1) * iStride + iPicW - 1]);
      } else
        EXPECT_EQ (pRef[k + j], pDst[ (iPicH - 1) * iStride + j - kiPaddingLen]);
    }
    k += iStride;
  }

  k = kiPaddingLen * iStride;
  int l = 0;
  for (int i = 0; i < iPicH - 1; i++)	{ //left
    for (int j = 0; j < kiPaddingLen; j++) {
      EXPECT_EQ (pRef[k + j], pDst[l]);
    }
    k += iStride;
    l += iStride;
  }

  k = kiPaddingLen * iStride;
  l = 0;
  for (int i = 0; i < iPicH - 1; i++)	{ //right
    for (int j = iPicW + kiPaddingLen; j < iPicW + 2 * kiPaddingLen; j++) {
      EXPECT_EQ (pRef[k + j], pDst[l + iPicW - 1]);
    }
    k += iStride;
    l += iStride;
  }

  delete []pRef;
}
