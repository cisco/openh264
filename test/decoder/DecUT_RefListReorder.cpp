#include <gtest/gtest.h>
#include <cstring>
#include "decoder_context.h"
#include "manage_dec_ref.h"

using namespace WelsDec;

namespace {

const int kLongRefCount = 3;
const int kTargetFrameIdx = 2; // request the long-term reference with iLongTermFrameIdx == 2

class RefListReorderTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    m_pCtx = new SWelsDecoderContext;
    m_pDqLayer = new SDqLayer;
    m_pReorder = new SRefPicListReorderSyn;
    m_pSps = new SSps;
    memset (m_pCtx, 0, sizeof (SWelsDecoderContext));
    memset (m_pDqLayer, 0, sizeof (SDqLayer));
    memset (m_pReorder, 0, sizeof (SRefPicListReorderSyn));
    memset (m_pSps, 0, sizeof (SSps));
    memset (m_aPics, 0, sizeof (m_aPics));

    // Three long-term references with distinct frame indices but, as MMCO-6 marking produces,
    // an identical (zero) uiLongTermPicNum.
    for (int i = 0; i < kLongRefCount; ++i) {
      m_aPics[i].bIsLongRef = true;
      m_aPics[i].iLongTermFrameIdx = i;
      m_aPics[i].uiLongTermPicNum = 0;
    }

    // Reference list starts in default frame-index order [idx0, idx1, idx2].
    SRefPic& sRefPic = m_pCtx->sRefPic;
    for (int i = 0; i < kLongRefCount; ++i) {
      sRefPic.pLongRefList[LIST_0][i] = &m_aPics[i];
      sRefPic.pRefList[LIST_0][i] = &m_aPics[i];
    }
    sRefPic.uiLongRefCount[LIST_0] = kLongRefCount;
    sRefPic.uiShortRefCount[LIST_0] = 0;
    sRefPic.uiRefCount[LIST_0] = 0;

    // A single long-term reorder command requesting long_term_pic_num == kTargetFrameIdx,
    // terminated by idc == 3.
    m_pReorder->bRefPicListReorderingFlag[LIST_0] = true;
    m_pReorder->sReorderingSyn[LIST_0][0].uiReorderingOfPicNumsIdc = 2;
    m_pReorder->sReorderingSyn[LIST_0][0].uiLongTermPicNum = kTargetFrameIdx;
    m_pReorder->sReorderingSyn[LIST_0][1].uiReorderingOfPicNumsIdc = 3;

    m_pDqLayer->pRefPicListReordering = m_pReorder;
    PSliceHeader pSliceHeader = &m_pDqLayer->sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader;
    pSliceHeader->iFrameNum = 5;
    pSliceHeader->uiRefCount[LIST_0] = kLongRefCount;
    pSliceHeader->pSps = m_pSps;
    m_pSps->uiLog2MaxFrameNum = 4;

    m_pCtx->pCurDqLayer = m_pDqLayer;
    m_pCtx->eSliceType = P_SLICE;
    m_pCtx->iPicQueueNumber = MAX_REF_PIC_COUNT;
  }

  virtual void TearDown() {
    delete m_pSps;
    delete m_pReorder;
    delete m_pDqLayer;
    delete m_pCtx;
  }

  PWelsDecoderContext m_pCtx;
  PDqLayer m_pDqLayer;
  PRefPicListReorderSyn m_pReorder;
  PSps m_pSps;
  SPicture m_aPics[kLongRefCount];
};

// The long-term reorder command asks for the reference with LongTermFrameIdx == 2 to be placed
// first.
TEST_F (RefListReorderTest, LongTermReorderMatchesByFrameIdx) {
  int32_t iRet = WelsReorderRefList2 (m_pCtx);
  ASSERT_EQ (iRet, ERR_NONE);

  PPicture pFirst = m_pCtx->sRefPic.pRefList[LIST_0][0];
  ASSERT_TRUE (pFirst != NULL);
  EXPECT_TRUE (pFirst->bIsLongRef);
  EXPECT_EQ (pFirst->iLongTermFrameIdx, kTargetFrameIdx);
}

} // anonymous namespace
