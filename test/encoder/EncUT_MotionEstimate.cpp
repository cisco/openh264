#include <stdlib.h>
#include "gtest/gtest.h"
#include "utils/DataGenerator.h"
#include "md.h"
#include "sample.h"
#include "svc_motion_estimate.h"
#include "wels_func_ptr_def.h"
#include "cpu.h"

using namespace WelsEnc;

void CopyTargetBlock (uint8_t* pSrcBlock, const int32_t kiBlockSize, SMVUnitXY sTargetMv, const int32_t kiRefPicStride,
                      uint8_t* pRefPic) {
  uint8_t* pTargetPos = pRefPic + sTargetMv.iMvY * kiRefPicStride + sTargetMv.iMvX;
  uint8_t* pSourcePos = pSrcBlock;

  for (int i = 0; i < kiBlockSize; i++) {
    memcpy (pSourcePos, pTargetPos, kiBlockSize * sizeof (uint8_t));
    pTargetPos += kiRefPicStride;
    pSourcePos += kiBlockSize;
  }
}


void InitMe (const uint8_t kuiQp, const uint32_t kuiMvdTableMiddle, const uint32_t kuiMvdTableStride,
             uint16_t* pMvdCostTable, SWelsME* pMe) {
  MvdCostInit (pMvdCostTable, kuiMvdTableStride);
  pMe->pMvdCost = &pMvdCostTable[kuiQp * kuiMvdTableStride + kuiMvdTableMiddle];
  pMe->sMvp.iMvX = pMe->sMvp.iMvY = 0;
  pMe->sMvBase.iMvX = pMe->sMvBase.iMvY = 0;
  pMe->sMv.iMvX = pMe->sMv.iMvY = 0;
}

class MotionEstimateTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    m_pRefData = NULL;
    m_pSrcBlock = NULL;
    m_pMvdCostTable = NULL;

    m_iWidth = 64;//size of search window
    m_iHeight = 64;//size of search window
    m_iMaxSearchBlock = 16;
    m_uiMvdTableSize = (1 + (648 << 1));

    pMa = new CMemoryAlign (0);
    m_pRefData = static_cast<uint8_t*>
                 (pMa->WelsMalloc (m_iWidth * m_iHeight, "RefPic"));
    ASSERT_TRUE (NULL != m_pRefData);
    m_pSrcBlock = static_cast<uint8_t*>
                  (pMa->WelsMalloc (m_iMaxSearchBlock * m_iMaxSearchBlock, "SrcBlock"));
    ASSERT_TRUE (NULL != m_pSrcBlock);
    m_pMvdCostTable = new uint16_t[52 * m_uiMvdTableSize];
    ASSERT_TRUE (NULL != m_pMvdCostTable);
  }
  void DoLineTest (PLineFullSearchFunc func, bool horizontal);
  virtual void TearDown() {
    delete [] m_pMvdCostTable;
    pMa->WelsFree (m_pRefData, "RefPic");
    pMa->WelsFree (m_pSrcBlock, "SrcBlock");
    delete pMa;
  }
 public:
  uint8_t* m_pRefData;
  uint8_t* m_pSrcBlock;
  uint32_t m_uiMvdTableSize;
  uint16_t* m_pMvdCostTable;

  int32_t m_iWidth;
  int32_t m_iHeight;
  int32_t m_iMaxSearchBlock;
  CMemoryAlign* pMa;
};


TEST_F (MotionEstimateTest, TestDiamondSearch) {
#define TEST_POS (5)
  const int32_t kiPositionToCheck[TEST_POS][2] = {{0, 0}, {0, 1}, {1, 0}, {0, -1}, { -1, 0}};
  const int32_t kiMaxBlock16Sad = 72000;//a rough number
  SWelsFuncPtrList sFuncList;
  SWelsME sMe;
  SSlice sSlice;
  memset (&sSlice, 0, sizeof (sSlice));

  const uint8_t kuiQp = rand() % 52;
  InitMe (kuiQp, 648, m_uiMvdTableSize, m_pMvdCostTable, &sMe);

  SMVUnitXY sTargetMv;
  WelsInitSampleSadFunc (&sFuncList, 0); //test c functions

  uint8_t* pRefPicCenter = m_pRefData + (m_iHeight / 2) * m_iWidth + (m_iWidth / 2);
  bool bDataGeneratorSucceed = false;
  bool bFoundMatch = false;
  int32_t i, iTryTimes;
  for (i = 0; i < TEST_POS; i++) {
    sTargetMv.iMvX = kiPositionToCheck[i][0];
    sTargetMv.iMvY = kiPositionToCheck[i][1];
    iTryTimes = 100;
    bDataGeneratorSucceed = false;
    bFoundMatch = false;
    while (!bFoundMatch && (iTryTimes--) > 0) {
      if (!YUVPixelDataGenerator (m_pRefData, m_iWidth, m_iHeight, m_iWidth))
        continue;

      bDataGeneratorSucceed = true;
      CopyTargetBlock (m_pSrcBlock, 16, sTargetMv, m_iWidth, pRefPicCenter);

      //clean the sMe status
      sMe.uiBlockSize = rand() % 5;
      sMe.pEncMb = m_pSrcBlock;
      sMe.pRefMb = pRefPicCenter;
      sMe.sMv.iMvX = sMe.sMv.iMvY = 0;
      sMe.uiSadCost = sMe.uiSatdCost = kiMaxBlock16Sad;
      WelsDiamondSearch (&sFuncList, &sMe, &sSlice, m_iMaxSearchBlock, m_iWidth);

      //the last selection may be affected by MVDcost, that is when (0,0) will be better
      //when comparing (1,1) and (1,0), due to the difference between MVD cost, it is possible that (1,0) is selected while the best match is (1,1)
      bFoundMatch = ((sMe.sMv.iMvX == (sTargetMv.iMvX)) || (sMe.sMv.iMvX == 0)) && ((sMe.sMv.iMvY == (sTargetMv.iMvY))
                    || (sMe.sMv.iMvY == 0));
    }
    if (bDataGeneratorSucceed) {
      //if DataGenerator never succeed, there is no meaning to check iTryTimes
      ASSERT_TRUE (iTryTimes > 0);
      //it is possible that ref at differnt position is identical, but that should be under a low probability
    }
  }
}

class MotionEstimateRangeTest : public ::testing::Test {
 public:
  virtual void SetUp() {

    m_iWidth = 320;
    m_iHeight = 240;
    m_iWidthExt = m_iWidth + 2 * PADDING_LENGTH;
    m_iHeightExt = m_iHeight + 2 * PADDING_LENGTH;
    m_iMbWidth = m_iWidth >> 4;
    m_iMbHeight = m_iHeight >> 4;

    m_iUsageType = 0;
    m_iNumDependencyLayers = 1;
    m_iMvRange = m_iUsageType ? EXPANDED_MV_RANGE : CAMERA_STARTMV_RANGE;
    m_iMvdRange = (m_iUsageType ? EXPANDED_MVD_RANGE : ((m_iNumDependencyLayers == 1) ? CAMERA_MVD_RANGE :
                   CAMERA_HIGHLAYER_MVD_RANGE));
    m_uiMvdInterTableSize       = (m_iMvdRange << 2); //intepel*4=qpel
    m_uiMvdInterTableStride     =  1 + (m_uiMvdInterTableSize << 1);//qpel_mv_range*2=(+/-);
    m_uiMvdCacheAlignedSize     = m_uiMvdInterTableStride * sizeof (uint16_t);

    m_pMa = new CMemoryAlign (16);
    ASSERT_TRUE (NULL != m_pMa);
    m_pMvdCostTable = (uint16_t*)m_pMa->WelsMallocz (52 * m_uiMvdCacheAlignedSize, "pMvdCostTable");
    ASSERT_TRUE (NULL != m_pMvdCostTable);
    m_pRefStart = (uint8_t*)m_pMa->WelsMallocz (m_iWidthExt * m_iHeightExt, "reference frame ");
    ASSERT_TRUE (NULL != m_pRefStart);
    m_pSrc = (uint8_t*)m_pMa->WelsMallocz (m_iWidth * m_iHeight, "source frame");
    ASSERT_TRUE (NULL != m_pSrc);

  }
  virtual void TearDown() {
    if (!m_pMa)
      return;
    if (m_pRefStart) {
      m_pMa->WelsFree (m_pRefStart, "reference frame ");
      m_pRefStart = NULL;
    }
    if (m_pSrc) {
      m_pMa->WelsFree (m_pSrc, "source frame ");
      m_pSrc = NULL;
    }
    if (m_pMvdCostTable) {
      m_pMa->WelsFree (m_pMvdCostTable, "pMvdCostTable");
      m_pMvdCostTable = NULL;
    }
    if (m_pMa) {
      delete m_pMa;
      m_pMa = NULL;
    }
  }
 public:
  uint8_t* m_pRefStart;
  uint8_t* m_pSrc;
  uint16_t* m_pMvdCostTable;

  int32_t m_iWidth;
  int32_t m_iHeight;
  int32_t m_iWidthExt;
  int32_t m_iHeightExt;
  int32_t m_iMbWidth;
  int32_t m_iMbHeight;
  CMemoryAlign* m_pMa;

  int32_t m_iMvRange;
  int32_t m_iMvdRange;
  uint32_t m_uiMvdInterTableSize;
  uint32_t m_uiMvdInterTableStride;
  uint32_t m_uiMvdCacheAlignedSize;
  int32_t m_iUsageType;
  int32_t m_iNumDependencyLayers;
};

TEST_F (MotionEstimateRangeTest, TestDiamondSearch) {

  const int32_t kiMaxBlock16Sad = 72000;//a rough number
  uint8_t* pRef = m_pRefStart + PADDING_LENGTH * m_iWidthExt + PADDING_LENGTH;
  SWelsFuncPtrList sFuncList;
  SWelsME sMe;
  SSlice sSlice;
  const uint8_t kuiQp = rand() % 52;
  InitMe (kuiQp, m_uiMvdInterTableSize, m_uiMvdInterTableStride, m_pMvdCostTable, &sMe);

  WelsInitSampleSadFunc (&sFuncList, 0); //test c functions

  memset (&sSlice, 0, sizeof (sSlice));
  memset (m_pSrc, 128, m_iWidth * m_iHeight);
  memset (m_pRefStart, 0, m_iWidthExt * m_iHeightExt);

  sMe.uiBlockSize = BLOCK_16x16; //

  sMe.sMvp.iMvX = rand() % m_iMvRange;
  sMe.sMvp.iMvY = rand() % m_iMvRange;

  for (int h = 0; h < m_iHeight; h++)
    memset (pRef + h * m_iWidthExt, h, m_iWidthExt);

  sMe.pEncMb = m_pSrc;

  sMe.sMv.iMvX = sMe.sMvp.iMvX;
  sMe.sMv.iMvY = sMe.sMvp.iMvY;

  sMe.uiSadCost = sMe.uiSatdCost = kiMaxBlock16Sad;
  SetMvWithinIntegerMvRange (m_iMbWidth, m_iMbHeight,     0, 0, m_iMvRange,
                             & (sSlice.sMvStartMin), & (sSlice.sMvStartMax));


  sMe.pRefMb = pRef + sMe.sMvp.iMvY * m_iWidthExt;
  WelsDiamondSearch (&sFuncList, &sMe, &sSlice, m_iWidth, m_iWidthExt);

  if ((WELS_ABS (sMe.sMv.iMvX) > m_iMvRange))
    printf ("mvx = %d\n", sMe.sMv.iMvX);
  ASSERT_TRUE (! (WELS_ABS (sMe.sMv.iMvX) > m_iMvRange));
  if ((WELS_ABS (sMe.sMv.iMvY) > m_iMvRange))
    printf ("mvy = %d\n", sMe.sMv.iMvY);
  ASSERT_TRUE (! (WELS_ABS (sMe.sMv.iMvY) > m_iMvRange));


}

TEST_F (MotionEstimateRangeTest, TestWelsMotionCrossSearch) {

  SWelsFuncPtrList sFuncList;
  SWelsME sMe;
  SSlice sSlice;
  bool bUsageType = true;
  uint8_t* pRef = m_pRefStart + PADDING_LENGTH * m_iWidthExt + PADDING_LENGTH;
  const int32_t kiMaxBlock16Sad = 72000;//a rough number

  memset (&sSlice, 0, sizeof (sSlice));
  memset (&sMe, 0, sizeof (sMe));
  WelsInitSampleSadFunc (&sFuncList, 0); //test c functions
  WelsInitMeFunc (&sFuncList, 0, bUsageType);

  RandomPixelDataGenerator (m_pSrc, m_iWidth, m_iHeight, m_iWidth);
  RandomPixelDataGenerator (m_pRefStart, m_iWidthExt, m_iHeightExt, m_iWidthExt);

  sMe.uiBlockSize = BLOCK_16x16; //
  for (int32_t iMby = 0; iMby < m_iMbHeight; iMby++) {
    for (int32_t iMbx = 0; iMbx < m_iMbWidth; iMbx++) {

      const uint8_t kuiQp = rand() % 52;

      InitMe (kuiQp, m_uiMvdInterTableSize, m_uiMvdInterTableStride, m_pMvdCostTable, &sMe);
      SetMvWithinIntegerMvRange (m_iMbWidth, m_iMbHeight, iMbx , iMby, m_iMvRange,
                                 & (sSlice.sMvStartMin), & (sSlice.sMvStartMax));


      sMe.sMvp.iMvX = rand() % m_iMvRange;
      sMe.sMvp.iMvY = rand() % m_iMvRange;
      sMe.iCurMeBlockPixX = (iMbx << 4);
      sMe.iCurMeBlockPixY = (iMby << 4);
      sMe.pRefMb = pRef + sMe.iCurMeBlockPixX + sMe.iCurMeBlockPixY * m_iWidthExt;
      sMe.pEncMb = m_pSrc + sMe.iCurMeBlockPixX + sMe.iCurMeBlockPixY * m_iWidth;
      sMe.uiSadCost = sMe.uiSatdCost = kiMaxBlock16Sad;
      sMe.pColoRefMb = sMe.pRefMb;
      WelsMotionCrossSearch (&sFuncList, &sMe, &sSlice, m_iWidth, m_iWidthExt);
      if ((WELS_ABS (sMe.sMv.iMvX) > m_iMvRange))
        printf ("mvx = %d\n", sMe.sMv.iMvX);
      ASSERT_TRUE (! (WELS_ABS (sMe.sMv.iMvX) > m_iMvRange));
      if ((WELS_ABS (sMe.sMv.iMvY) > m_iMvRange))
        printf ("mvy = %d\n", sMe.sMv.iMvY);
      ASSERT_TRUE (! (WELS_ABS (sMe.sMv.iMvY) > m_iMvRange));
    }
  }
}
void MotionEstimateTest::DoLineTest (PLineFullSearchFunc func, bool vertical) {
  const int32_t kiMaxBlock16Sad = 72000;//a rough number
  SWelsFuncPtrList sFuncList;
  SWelsME sMe;

  const uint8_t kuiQp = rand() % 52;
  InitMe (kuiQp, 648, m_uiMvdTableSize, m_pMvdCostTable, &sMe);

  SMVUnitXY sTargetMv;
  WelsInitSampleSadFunc (&sFuncList, 0); //test c functions
  WelsInitMeFunc (&sFuncList, WelsCPUFeatureDetect (NULL), 1);

  uint8_t* pRefPicCenter = m_pRefData + (m_iHeight / 2) * m_iWidth + (m_iWidth / 2);
  sMe.iCurMeBlockPixX = (m_iWidth / 2);
  sMe.iCurMeBlockPixY = (m_iHeight / 2);

  bool bDataGeneratorSucceed = false;
  bool bFoundMatch = false;
  int32_t iTryTimes = 100;

  if (vertical) {
    sTargetMv.iMvX = 0;
    sTargetMv.iMvY = -sMe.iCurMeBlockPixY + INTPEL_NEEDED_MARGIN + rand() % (m_iHeight - 16 - 2 * INTPEL_NEEDED_MARGIN);
  } else {
    sTargetMv.iMvX = -sMe.iCurMeBlockPixX + INTPEL_NEEDED_MARGIN + rand() % (m_iWidth - 16 - 2 * INTPEL_NEEDED_MARGIN);
    sTargetMv.iMvY = 0;
  }
  bDataGeneratorSucceed = false;
  bFoundMatch = false;
  while (!bFoundMatch && (iTryTimes--) > 0) {
    if (!YUVPixelDataGenerator (m_pRefData, m_iWidth, m_iHeight, m_iWidth))
      continue;

    bDataGeneratorSucceed = true;
    CopyTargetBlock (m_pSrcBlock, 16, sTargetMv, m_iWidth, pRefPicCenter);

    //clean the sMe status
    sMe.uiBlockSize = rand() % 5;
    sMe.pEncMb = m_pSrcBlock;
    sMe.pRefMb = pRefPicCenter;
    sMe.pColoRefMb = pRefPicCenter;
    sMe.sMv.iMvX = sMe.sMv.iMvY = 0;
    sMe.uiSadCost = sMe.uiSatdCost = kiMaxBlock16Sad;
    const int32_t iCurMeBlockPixX = sMe.iCurMeBlockPixX;
    const int32_t iCurMeBlockQpelPixX = ((iCurMeBlockPixX) << 2);
    const int32_t iCurMeBlockPixY = sMe.iCurMeBlockPixY;
    const int32_t iCurMeBlockQpelPixY = ((iCurMeBlockPixY) << 2);
    uint16_t* pMvdCostX = sMe.pMvdCost - iCurMeBlockQpelPixX - sMe.sMvp.iMvX; //do the offset here
    uint16_t* pMvdCostY = sMe.pMvdCost - iCurMeBlockQpelPixY - sMe.sMvp.iMvY;
    uint16_t* pMvdCost = vertical ? pMvdCostY : pMvdCostX;
    int iSize = vertical ? m_iHeight : m_iWidth;

    //the last selection may be affected by MVDcost, that is when smaller MvY will be better
    if (vertical) {
      func (&sFuncList, &sMe,
            pMvdCost,
            m_iMaxSearchBlock, m_iWidth,
            INTPEL_NEEDED_MARGIN - sMe.iCurMeBlockPixY,
            iSize - INTPEL_NEEDED_MARGIN - 16 - sMe.iCurMeBlockPixY, vertical);
      bFoundMatch = (sMe.sMv.iMvX == 0
                     && (sMe.sMv.iMvY == sTargetMv.iMvY || abs (sMe.sMv.iMvY) < abs (sTargetMv.iMvY)));
    } else {
      func (&sFuncList, &sMe,
            pMvdCost,
            m_iMaxSearchBlock, m_iWidth,
            INTPEL_NEEDED_MARGIN - sMe.iCurMeBlockPixX,
            iSize - INTPEL_NEEDED_MARGIN - 16 - sMe.iCurMeBlockPixX, vertical);
      bFoundMatch = (sMe.sMv.iMvY == 0
                     && (sMe.sMv.iMvX == sTargetMv.iMvX || abs (sMe.sMv.iMvX) < abs (sTargetMv.iMvX)));
    }
    //printf("DoLineTest Target: %d,%d\n", sTargetMv.iMvX, sTargetMv.iMvY);
  }
  if (bDataGeneratorSucceed) {
    //if DataGenerator never succeed, there is no meaning to check iTryTimes
    ASSERT_TRUE (iTryTimes > 0);
    //it is possible that ref at differnt position is identical, but that should be under a low probability
  }
}

TEST_F (MotionEstimateTest, TestVerticalSearch) {
  DoLineTest (LineFullSearch_c, true);
}
TEST_F (MotionEstimateTest, TestHorizontalSearch) {
  DoLineTest (LineFullSearch_c, false);
}

#ifdef X86_ASM
TEST_F (MotionEstimateTest, TestVerticalSearch_SSE41) {
  int32_t iTmp = 1;
  uint32_t uiCPUFlags = WelsCPUFeatureDetect (&iTmp);
  if ((uiCPUFlags & WELS_CPU_SSE41) == 0) return ;

  DoLineTest (VerticalFullSearchUsingSSE41, true);
}

TEST_F (MotionEstimateTest, TestHorizontalSearch_SSE41) {
  int32_t iTmp = 1;
  uint32_t uiCPUFlags = WelsCPUFeatureDetect (&iTmp);
  if ((uiCPUFlags & WELS_CPU_SSE41) == 0) return ;

  DoLineTest (HorizontalFullSearchUsingSSE41, false);
}
#endif

class FeatureMotionEstimateTest : public ::testing::Test {
 public:
  virtual void SetUp() {
    m_pRefData = NULL;
    m_pSrcBlock = NULL;
    m_pMvdCostTable = NULL;

    m_iWidth = 64;//size of search window
    m_iHeight = 64;//size of search window
    m_iMaxSearchBlock = 8;
    m_uiMvdTableSize = (1 + (648 << 1));

    m_pMa = new CMemoryAlign (16);
    ASSERT_TRUE (NULL != m_pMa);
    m_pRefData = (uint8_t*)m_pMa->WelsMalloc (m_iWidth * m_iHeight * sizeof (uint8_t), "m_pRefData");
    ASSERT_TRUE (NULL != m_pRefData);
    m_pSrcBlock = (uint8_t*)m_pMa->WelsMalloc (m_iMaxSearchBlock * m_iMaxSearchBlock * sizeof (uint8_t), "m_pSrcBlock");
    ASSERT_TRUE (NULL != m_pSrcBlock);
    m_pMvdCostTable = (uint16_t*)m_pMa->WelsMalloc (52 * m_uiMvdTableSize * sizeof (uint16_t), "m_pMvdCostTable");
    ASSERT_TRUE (NULL != m_pMvdCostTable);
    m_pFeatureSearchPreparation = (SFeatureSearchPreparation*)m_pMa->WelsMalloc (sizeof (SFeatureSearchPreparation),
                                  "m_pFeatureSearchPreparation");
    ASSERT_TRUE (NULL != m_pFeatureSearchPreparation);
    m_pScreenBlockFeatureStorage = (SScreenBlockFeatureStorage*)m_pMa->WelsMalloc (sizeof (SScreenBlockFeatureStorage),
                                   "m_pScreenBlockFeatureStorage");
    ASSERT_TRUE (NULL != m_pScreenBlockFeatureStorage);
  }
  virtual void TearDown() {
    if (m_pMa) {
      if (m_pRefData) {
        m_pMa->WelsFree (m_pRefData, "m_pRefData");
        m_pRefData = NULL;
      }
      if (m_pSrcBlock) {
        m_pMa->WelsFree (m_pSrcBlock, "m_pSrcBlock");
        m_pSrcBlock = NULL;
      }
      if (m_pMvdCostTable) {
        m_pMa->WelsFree (m_pMvdCostTable, "m_pMvdCostTable");
        m_pMvdCostTable = NULL;
      }

      if (m_pFeatureSearchPreparation) {
        ReleaseFeatureSearchPreparation (m_pMa, m_pFeatureSearchPreparation->pFeatureOfBlock);
        m_pMa->WelsFree (m_pFeatureSearchPreparation, "m_pFeatureSearchPreparation");
        m_pFeatureSearchPreparation = NULL;
      }
      if (m_pScreenBlockFeatureStorage) {
        ReleaseScreenBlockFeatureStorage (m_pMa, m_pScreenBlockFeatureStorage);
        m_pMa->WelsFree (m_pScreenBlockFeatureStorage, "m_pScreenBlockFeatureStorage");
        m_pScreenBlockFeatureStorage = NULL;
      }
      delete m_pMa;
      m_pMa = NULL;
    }
  }
  void InitRefPicForMeTest (SPicture* pRefPic) {
    pRefPic->pData[0] = m_pRefData;
    pRefPic->iLineSize[0] = m_iWidth;
    pRefPic->iFrameAverageQp = rand() % 52;
    pRefPic->iWidthInPixel = m_iWidth;
    pRefPic->iHeightInPixel = m_iHeight;
  }
 public:
  CMemoryAlign* m_pMa;

  SFeatureSearchPreparation* m_pFeatureSearchPreparation;
  SScreenBlockFeatureStorage* m_pScreenBlockFeatureStorage;

  uint8_t* m_pRefData;
  uint8_t* m_pSrcBlock;
  uint16_t* m_pMvdCostTable;
  uint32_t m_uiMvdTableSize;

  int32_t m_iWidth;
  int32_t m_iHeight;
  int32_t m_iMaxSearchBlock;
};

TEST_F (FeatureMotionEstimateTest, TestFeatureSearch) {
  const int32_t kiMaxBlock16Sad = 72000;//a rough number
  SWelsFuncPtrList sFuncList;
  WelsInitSampleSadFunc (&sFuncList, 0); //test c functions
  WelsInitMeFunc (&sFuncList, 0, true);

  SWelsME sMe;
  const uint8_t kuiQp = rand() % 52;
  InitMe (kuiQp, 648, m_uiMvdTableSize, m_pMvdCostTable, &sMe);
  sMe.iCurMeBlockPixX = (m_iWidth / 2);
  sMe.iCurMeBlockPixY = (m_iHeight / 2);
  uint8_t* pRefPicCenter = m_pRefData + (m_iHeight / 2) * m_iWidth + (m_iWidth / 2);

  SPicture sRef;
  InitRefPicForMeTest (&sRef);

  SSlice sSlice;
  const int32_t kiSupposedPaddingLength = 16;
  SetMvWithinIntegerMvRange (m_iWidth / 16 - kiSupposedPaddingLength, m_iHeight / 16 - kiSupposedPaddingLength,
                             m_iWidth / 2 / 16, m_iHeight / 2 / 16, 508,
                             & (sSlice.sMvStartMin), & (sSlice.sMvStartMax));
  int32_t iReturn;
  const int32_t kiNeedFeatureStorage = ME_DIA_CROSS_FME;
  iReturn = RequestFeatureSearchPreparation (m_pMa, m_iWidth,  m_iHeight, kiNeedFeatureStorage,
            m_pFeatureSearchPreparation);
  ASSERT_TRUE (ENC_RETURN_SUCCESS == iReturn);
  iReturn = RequestScreenBlockFeatureStorage (m_pMa, m_iWidth, m_iHeight, kiNeedFeatureStorage,
            m_pScreenBlockFeatureStorage);
  ASSERT_TRUE (ENC_RETURN_SUCCESS == iReturn);

  SMVUnitXY sTargetMv;
  for (int i = sSlice.sMvStartMin.iMvX; i <= sSlice.sMvStartMax.iMvX; i++) {
    for (int j = sSlice.sMvStartMin.iMvY; j <= sSlice.sMvStartMax.iMvY; j++) {
      if (i == 0 || j == 0) continue; //exclude x=0 or y=0 since that will be skipped by FME

      bool bDataGeneratorSucceed = false;
      bool bFoundMatch = false;

      if (!YUVPixelDataGenerator (m_pRefData, m_iWidth, m_iHeight, m_iWidth))
        continue;
      bDataGeneratorSucceed = true;

      sTargetMv.iMvX = i;
      sTargetMv.iMvY = j;
      CopyTargetBlock (m_pSrcBlock, m_iMaxSearchBlock, sTargetMv, m_iWidth, pRefPicCenter);

      //clean sMe status
      sMe.uiBlockSize = BLOCK_8x8;
      sMe.pEncMb = m_pSrcBlock;
      sMe.pRefMb = pRefPicCenter;
      sMe.pColoRefMb = pRefPicCenter;
      sMe.sMv.iMvX = sMe.sMv.iMvY = 0;
      sMe.uiSadCost = sMe.uiSatdCost = kiMaxBlock16Sad;

      //begin FME process
      PerformFMEPreprocess (&sFuncList, &sRef, m_pFeatureSearchPreparation->pFeatureOfBlock,
                            m_pScreenBlockFeatureStorage);
      m_pScreenBlockFeatureStorage->uiSadCostThreshold[BLOCK_8x8] = UINT_MAX;//to avoid early skip
      uint32_t uiMaxSearchPoint = INT_MAX;
      SFeatureSearchIn sFeatureSearchIn = {0};
      if (SetFeatureSearchIn (&sFuncList, sMe, &sSlice, m_pScreenBlockFeatureStorage,
                              m_iMaxSearchBlock, m_iWidth,
                              &sFeatureSearchIn)) {
        MotionEstimateFeatureFullSearch (sFeatureSearchIn, uiMaxSearchPoint, &sMe);
      }

      bool bMvMatch  = sMe.sMv.iMvX == sTargetMv.iMvX && sMe.sMv.iMvY == sTargetMv.iMvY;
      bool bFeatureMatch =
        (* (m_pScreenBlockFeatureStorage->pFeatureOfBlockPointer + (m_iHeight / 2 + sTargetMv.iMvY) * (m_iWidth - 8) +
            (m_iWidth / 2 + sTargetMv.iMvX))
         == * (m_pScreenBlockFeatureStorage->pFeatureOfBlockPointer + (m_iHeight / 2 + sMe.sMv.iMvY) * (m_iWidth - 8) +
               (m_iWidth / 2 + sMe.sMv.iMvX)))
        && ((sMe.pMvdCost[sMe.sMv.iMvY << 2] + sMe.pMvdCost[sMe.sMv.iMvX << 2]) <= (sMe.pMvdCost[sTargetMv.iMvY << 2] +
            sMe.pMvdCost[sTargetMv.iMvX << 2]));

      //the last selection may be affected by MVDcost, that is when smaller Mv will be better
      bFoundMatch = bMvMatch || bFeatureMatch;

      if (bDataGeneratorSucceed) {
        //if DataGenerator never succeed, there is no meaning to check iTryTimes
        if (!bFoundMatch) {
          printf ("TestFeatureSearch Target: %d,%d, Result: %d,%d\n", sTargetMv.iMvX, sTargetMv.iMvY, sMe.sMv.iMvX, sMe.sMv.iMvY);
        }
        EXPECT_TRUE (bFoundMatch);
      }
    }
  }
}

