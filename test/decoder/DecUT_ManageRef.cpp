#include <gtest/gtest.h>
#include <cstring>

#include "decoder_context.h"
#include "manage_dec_ref.h"

using namespace WelsDec;

// Regression test for the MMCO5 stale sTmpRefPic snapshot.
//
// In the threaded predecessor-handoff path (decoder_core.cpp), the decoder
// snapshots pCtx->sRefPic into pCtx->sTmpRefPic, calls WelsMarkAsRef() with a
// non-NULL pLastDec (which makes WelsMarkAsRef() operate on sTmpRefPic rather
// than sRefPic), and later publishes the (possibly mutated) sTmpRefPic back
// into the successor context's sRefPic. Before the fix, MMCO_RESET (MMCO5)
// called WelsResetRefPic(pCtx), which hard-codes &pCtx->sRefPic and ignores
// the active list the caller selected, so sTmpRefPic kept a pointer to a
// picture that had just been unreferenced and made recyclable.
//
// This test drives WelsMarkAsRef() directly (bypassing the full decode
// pipeline) with a synthetic MMCO5 marking and a pre-populated stale
// short-term reference shared between sRefPic and sTmpRefPic (mirroring the
// real "sTmpRefPic = sRefPic" snapshot), then asserts the stale entry does
// not survive in sTmpRefPic. Unlike an end-to-end bitstream decode, this is
// deterministic and isolated from unrelated threaded-decode defects that can
// otherwise also crash a full decode of a real stream under threading.
class ManageDecRefMmco5Test : public ::testing::Test {
 protected:
  virtual void SetUp() {
    memset (&ctx_, 0, sizeof (ctx_));
    memset (&dqLayer_, 0, sizeof (dqLayer_));
    memset (&refMarking_, 0, sizeof (refMarking_));
    memset (&sps_, 0, sizeof (sps_));
    memset (&pps_, 0, sizeof (pps_));
    memset (&lastDecPicInfo_, 0, sizeof (lastDecPicInfo_));
    memset (&nalUnit_, 0, sizeof (nalUnit_));
    memset (&stalePic_, 0, sizeof (stalePic_));
    memset (&newPic_, 0, sizeof (newPic_));

    // A non-IDR slice signalling adaptive reference picture marking with a
    // single MMCO_RESET (MMCO5) command.
    refMarking_.bAdaptiveRefPicMarkingModeFlag = true;
    refMarking_.sMmcoRef[0].uiMmcoType = MMCO_RESET;
    refMarking_.sMmcoRef[1].uiMmcoType = MMCO_END;

    sps_.iSpsId = 0;
    sps_.iNumRefFrames = 4;
    sps_.uiLog2MaxFrameNum = 4;
    pps_.iPpsId = 0;

    dqLayer_.pRefPicMarking = &refMarking_;
    dqLayer_.sLayerInfo.pSps = &sps_;  // read directly by MMCO()

    nalUnit_.sNalHeaderExt.sNalUnitHeader.eNalUnitType = NAL_UNIT_CODED_SLICE;  // non-IDR
    nalUnit_.sNalHeaderExt.bIdrFlag = false;
    nalUnitPtr_ = &nalUnit_;

    accessUnit_.pNalUnitsList = &nalUnitPtr_;
    accessUnit_.uiStartPos = 0;
    accessUnit_.uiEndPos = 0;

    ctx_.pCurDqLayer = &dqLayer_;
    ctx_.pSps = &sps_;
    ctx_.pPps = &pps_;
    ctx_.pAccessUnitList = &accessUnit_;
    ctx_.pLastDecPicInfo = &lastDecPicInfo_;

    // Stale short-term reference picture shared by sRefPic and sTmpRefPic,
    // mirroring the real threaded snapshot (sTmpRefPic = sRefPic is a plain
    // struct copy of the same picture pointers, done once in decoder_core.cpp
    // just before WelsMarkAsRef() is called).
    stalePic_.iRefCount = 0;         // eligible for SetUnRef() to clear it
    stalePic_.eSliceType = I_SLICE;  // makes SetUnRef() return before it
                                     // touches pRefPic[list][], which this
                                     // test does not populate
    stalePic_.iFrameNum = 999;       // must differ from newPic_.iFrameNum so
                                     // AddShortTermToList() does not treat
                                     // this as a duplicate-frame_num replace

    ctx_.sRefPic.pShortRefList[LIST_0][0] = &stalePic_;
    ctx_.sRefPic.uiShortRefCount[LIST_0] = 1;
    ctx_.sTmpRefPic = ctx_.sRefPic;  // the real snapshot idiom

    newPic_.iFrameNum = 0;
  }

  SWelsDecoderContext ctx_;
  SDqLayer dqLayer_;
  SRefPicMarking refMarking_;
  SSps sps_;
  SPps pps_;
  SWelsLastDecPicInfo lastDecPicInfo_;
  SNalUnit nalUnit_;
  PNalUnit nalUnitPtr_;
  SAccessUnit accessUnit_;
  SPicture stalePic_;
  SPicture newPic_;
};

TEST_F (ManageDecRefMmco5Test, Mmco5ResetInvalidatesThreadedSnapshot) {
  ASSERT_EQ (0, WelsMarkAsRef (&ctx_, &newPic_));

  // With the bug, the pre-reset stale pointer survives in sTmpRefPic
  // (shifted, not cleared) alongside the newly-added picture, inflating the
  // short-term ref count to 2. With the fix, only the newly-added picture
  // remains.
  EXPECT_EQ (1u, ctx_.sTmpRefPic.uiShortRefCount[LIST_0]);
  for (uint32_t i = 0; i < ctx_.sTmpRefPic.uiShortRefCount[LIST_0]; ++i) {
    EXPECT_NE (&stalePic_, ctx_.sTmpRefPic.pShortRefList[LIST_0][i])
        << "stale reference picture pointer leaked into sTmpRefPic after MMCO5 reset";
  }

  // sRefPic (the persistent, non-threaded list) is always cleared directly
  // by WelsResetRefPic(), independent of this fix.
  EXPECT_EQ (0u, ctx_.sRefPic.uiShortRefCount[LIST_0]);
  EXPECT_EQ (0u, ctx_.sRefPic.uiLongRefCount[LIST_0]);
}
