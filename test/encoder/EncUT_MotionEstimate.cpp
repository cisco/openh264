#include <stdlib.h>
#include "gtest/gtest.h"
#include "utils/DataGenerator.h"
#include "md.h"
#include "sample.h"
#include "svc_motion_estimate.h"
#include "wels_func_ptr_def.h"
#include "cpu.h"


using namespace WelsSVCEnc;

void CopyTargetBlock( uint8_t* pSrcBlock, const int32_t kiBlockSize, SMVUnitXY sTargetMv, const int32_t kiRefPicStride,
                     uint8_t* pRefPic) {
  uint8_t* pTargetPos = pRefPic+sTargetMv.iMvY*kiRefPicStride+sTargetMv.iMvX;
  uint8_t* pSourcePos = pSrcBlock;

  for (int i = 0; i<kiBlockSize; i++) {
    memcpy( pSourcePos, pTargetPos, kiBlockSize*sizeof(uint8_t) );
    pTargetPos += kiRefPicStride;
    pSourcePos += kiBlockSize;
  }
}


void InitMe( const uint8_t kuiQp, const uint32_t kuiMvdTableMiddle, const uint32_t kuiMvdTableStride,
            uint16_t* pMvdCostTable, SWelsME* pMe) {
  MvdCostInit( pMvdCostTable, kuiMvdTableStride );
  pMe->pMvdCost = &pMvdCostTable[kuiQp*kuiMvdTableStride + kuiMvdTableMiddle];
  pMe->sMvp.iMvX = pMe->sMvp.iMvY = 0;
  pMe->sMvBase.iMvX = pMe->sMvBase.iMvY = 0;
  pMe->sMv.iMvX = pMe->sMv.iMvY = 0;
}

class MotionEstimateTest : public ::testing::Test {
public:
  virtual void SetUp() {
    m_pRefPic = NULL;
    m_pSrcBlock = NULL;
    m_pMvdCostTable = NULL;

    m_iWidth = 64;//size of search window
    m_iHeight = 64;//size of search window
    m_iMaxSearchBlock = 16;
    m_uiMvdTableSize	=  (1 + (648 << 1));

    pMa = new CMemoryAlign(0);
    m_pRefPic = static_cast<uint8_t *>
    (pMa->WelsMalloc(m_iWidth*m_iHeight, "RefPic"));
    ASSERT_TRUE( NULL != m_pRefPic );
    m_pSrcBlock = static_cast<uint8_t *>
    (pMa->WelsMalloc(m_iMaxSearchBlock*m_iMaxSearchBlock, "SrcBlock"));
    ASSERT_TRUE( NULL != m_pSrcBlock );
    m_pMvdCostTable=new uint16_t[52*m_uiMvdTableSize];
    ASSERT_TRUE( NULL != m_pMvdCostTable );
  }
  virtual void TearDown() {
    delete [] m_pMvdCostTable;
    pMa->WelsFree( m_pRefPic, "RefPic");
    pMa->WelsFree( m_pSrcBlock, "SrcBlock");
    delete pMa;
  }
public:
  uint8_t *m_pRefPic;
  uint8_t *m_pSrcBlock;
  uint32_t m_uiMvdTableSize;
  uint16_t *m_pMvdCostTable;

  int32_t m_iWidth;
  int32_t m_iHeight;
  int32_t m_iMaxSearchBlock;
  CMemoryAlign *pMa;
};


TEST_F(MotionEstimateTest, TestDiamondSearch) {
#define TEST_POS (5)
  const int32_t kiPositionToCheck[TEST_POS][2] = {{0,0}, {0,1}, {1,0}, {0,-1}, {-1,0}};
  const int32_t kiMaxBlock16Sad = 72000;//a rough number
  SWelsFuncPtrList sFuncList;
  SWelsME sMe;
  SSlice sSlice;

  srand((uint32_t)time(NULL));
  const uint8_t kuiQp = rand()%52;
  InitMe(kuiQp, 648, m_uiMvdTableSize, m_pMvdCostTable, &sMe);

  SMVUnitXY sTargetMv;
  WelsInitSampleSadFunc( &sFuncList, 0 );//test c functions

  uint8_t *pRefPicCenter = m_pRefPic+(m_iHeight/2)*m_iWidth+(m_iWidth/2);
  bool bDataGeneratorSucceed = false;
  bool bFoundMatch = false;
  int32_t i, iTryTimes;
  for (i=0;i<TEST_POS;i++) {
    sTargetMv.iMvX = kiPositionToCheck[i][0];
    sTargetMv.iMvY = kiPositionToCheck[i][1];
    iTryTimes = 100;
    bDataGeneratorSucceed = false;
    bFoundMatch = false;
    while (!bFoundMatch && (iTryTimes--)>0) {
      if (!YUVPixelDataGenerator( m_pRefPic, m_iWidth, m_iHeight, m_iWidth ))
        continue;

      bDataGeneratorSucceed = true;
      CopyTargetBlock( m_pSrcBlock, 16, sTargetMv, m_iWidth, pRefPicCenter);

      //clean the sMe status
      sMe.uiBlockSize = rand()%5;
      sMe.pEncMb = m_pSrcBlock;
      sMe.pRefMb = pRefPicCenter;
      sMe.sMv.iMvX = sMe.sMv.iMvY = 0;
      sMe.uiSadCost = sMe.uiSatdCost = kiMaxBlock16Sad;
      WelsDiamondSearch (&sFuncList, &sMe, &sSlice, m_iMaxSearchBlock, m_iWidth);

      //the last selection may be affected by MVDcost, that is when (0,0) will be better
      //when comparing (1,1) and (1,0), due to the difference between MVD cost, it is possible that (1,0) is selected while the best match is (1,1)
      bFoundMatch = ((sMe.sMv.iMvX==(sTargetMv.iMvX))||(sMe.sMv.iMvX==0)) && ((sMe.sMv.iMvY==(sTargetMv.iMvY))||(sMe.sMv.iMvY==0));
    }
    if (bDataGeneratorSucceed) {
      //if DataGenerator never succeed, there is no meaning to check iTryTimes
      ASSERT_TRUE(iTryTimes > 0);
      //it is possible that ref at differnt position is identical, but that should be under a low probability
    }
  }
}


TEST_F(MotionEstimateTest, TestVerticalSearch) {
  const int32_t kiMaxBlock16Sad = 72000;//a rough number
  SWelsFuncPtrList sFuncList;
  SWelsME sMe;

  srand((uint32_t)time(NULL));
  const uint8_t kuiQp = rand()%52;
  InitMe(kuiQp, 648, m_uiMvdTableSize, m_pMvdCostTable, &sMe);

  SMVUnitXY sTargetMv;
  WelsInitSampleSadFunc( &sFuncList, 0 );//test c functions

  uint8_t *pRefPicCenter = m_pRefPic+(m_iHeight/2)*m_iWidth+(m_iWidth/2);
  sMe.iCurMeBlockPixX = (m_iWidth/2);
  sMe.iCurMeBlockPixY = (m_iHeight/2);

  bool bDataGeneratorSucceed = false;
  bool bFoundMatch = false;
  int32_t iTryTimes=100;

  sTargetMv.iMvX = 0;
  sTargetMv.iMvY = WELS_MAX(INTPEL_NEEDED_MARGIN, rand()%m_iHeight-INTPEL_NEEDED_MARGIN);
  bDataGeneratorSucceed = false;
  bFoundMatch = false;
  while (!bFoundMatch && (iTryTimes--)>0) {
    if (!YUVPixelDataGenerator( m_pRefPic, m_iWidth, m_iHeight, m_iWidth ))
      continue;

    bDataGeneratorSucceed = true;
    CopyTargetBlock( m_pSrcBlock, 16, sTargetMv, m_iWidth, pRefPicCenter);

    //clean the sMe status
    sMe.uiBlockSize = rand()%5;
    sMe.pEncMb = m_pSrcBlock;
    sMe.pRefMb = pRefPicCenter;
    sMe.pColoRefMb = pRefPicCenter;
    sMe.sMv.iMvX = sMe.sMv.iMvY = 0;
    sMe.uiSadCost = sMe.uiSatdCost = kiMaxBlock16Sad;
    const int32_t iCurMeBlockPixX = sMe.iCurMeBlockPixX;
    const int32_t iCurMeBlockQpelPixX = ((iCurMeBlockPixX)<<2);
    const int32_t iCurMeBlockPixY = sMe.iCurMeBlockPixY;
    const int32_t iCurMeBlockQpelPixY = ((iCurMeBlockPixY)<<2);
    uint16_t* pMvdCostX = sMe.pMvdCost - iCurMeBlockQpelPixX - sMe.sMvp.iMvX;	//do the offset here
    uint16_t* pMvdCostY = sMe.pMvdCost - iCurMeBlockQpelPixY - sMe.sMvp.iMvY;
    LineFullSearch_c ( &sFuncList, &sMe,
                      pMvdCostY, pMvdCostX[ iCurMeBlockQpelPixX ],
                      m_iMaxSearchBlock, m_iWidth,
                      INTPEL_NEEDED_MARGIN,
                      m_iHeight-INTPEL_NEEDED_MARGIN, true );

    //the last selection may be affected by MVDcost, that is when smaller MvY will be better
    bFoundMatch = (sMe.sMv.iMvX==0
                   &&(sMe.sMv.iMvY==sTargetMv.iMvY||abs(sMe.sMv.iMvY)<abs(sTargetMv.iMvY)));
    //printf("TestVerticalSearch Target: %d,%d\n", sTargetMv.iMvX, sTargetMv.iMvY);
  }
  if (bDataGeneratorSucceed) {
    //if DataGenerator never succeed, there is no meaning to check iTryTimes
    ASSERT_TRUE(iTryTimes > 0);
    //it is possible that ref at differnt position is identical, but that should be under a low probability
  }
}
TEST_F(MotionEstimateTest, TestHorizontalSearch) {
  const int32_t kiMaxBlock16Sad = 72000;//a rough number
  SWelsFuncPtrList sFuncList;
  SWelsME sMe;

  srand((uint32_t)time(NULL));
  const uint8_t kuiQp = rand()%52;
  InitMe(kuiQp, 648, m_uiMvdTableSize, m_pMvdCostTable, &sMe);

  SMVUnitXY sTargetMv;
  WelsInitSampleSadFunc( &sFuncList, 0 );//test c functions

  uint8_t *pRefPicCenter = m_pRefPic+(m_iHeight/2)*m_iWidth+(m_iWidth/2);
  sMe.iCurMeBlockPixX = (m_iWidth/2);
  sMe.iCurMeBlockPixY = (m_iHeight/2);

  bool bDataGeneratorSucceed = false;
  bool bFoundMatch = false;
  int32_t iTryTimes=100;

  sTargetMv.iMvX = WELS_MAX(INTPEL_NEEDED_MARGIN, rand()%m_iWidth-INTPEL_NEEDED_MARGIN);
  sTargetMv.iMvY = 0;
  bDataGeneratorSucceed = false;
  bFoundMatch = false;
  while (!bFoundMatch && (iTryTimes--)>0) {
    if (!YUVPixelDataGenerator( m_pRefPic, m_iWidth, m_iHeight, m_iWidth ))
      continue;

    bDataGeneratorSucceed = true;
    CopyTargetBlock( m_pSrcBlock, 16, sTargetMv, m_iWidth, pRefPicCenter);

    //clean the sMe status
    sMe.uiBlockSize = rand()%5;
    sMe.pEncMb = m_pSrcBlock;
    sMe.pRefMb = pRefPicCenter;
    sMe.pColoRefMb = pRefPicCenter;
    sMe.sMv.iMvX = sMe.sMv.iMvY = 0;
    sMe.uiSadCost = sMe.uiSatdCost = kiMaxBlock16Sad;
    const int32_t iCurMeBlockPixX = sMe.iCurMeBlockPixX;
    const int32_t iCurMeBlockQpelPixX = ((iCurMeBlockPixX)<<2);
    const int32_t iCurMeBlockPixY = sMe.iCurMeBlockPixY;
    const int32_t iCurMeBlockQpelPixY = ((iCurMeBlockPixY)<<2);
    uint16_t* pMvdCostX = sMe.pMvdCost - iCurMeBlockQpelPixX - sMe.sMvp.iMvX;	//do the offset here
    uint16_t* pMvdCostY = sMe.pMvdCost - iCurMeBlockQpelPixY - sMe.sMvp.iMvY;
    LineFullSearch_c ( &sFuncList, &sMe,
                      pMvdCostX, pMvdCostY[ iCurMeBlockQpelPixY ],
                      m_iMaxSearchBlock, m_iWidth,
                      INTPEL_NEEDED_MARGIN,
                      m_iWidth-INTPEL_NEEDED_MARGIN, false );

    //the last selection may be affected by MVDcost, that is when smaller MvY will be better
    bFoundMatch = (sMe.sMv.iMvY==0
                   &&(sMe.sMv.iMvX==sTargetMv.iMvX||abs(sMe.sMv.iMvX)<abs(sTargetMv.iMvX)));
    //printf("TestHorizontalSearch Target: %d,%d\n", sTargetMv.iMvX, sTargetMv.iMvY);
  }
  if (bDataGeneratorSucceed) {
    //if DataGenerator never succeed, there is no meaning to check iTryTimes
    ASSERT_TRUE(iTryTimes > 0);
    //it is possible that ref at differnt position is identical, but that should be under a low probability
  }
}

#ifdef X86_ASM
TEST_F(MotionEstimateTest, TestVerticalSearch_SSE41)
{
  const int32_t kiMaxBlock16Sad = 72000;//a rough number
  SWelsFuncPtrList sFuncList;
  SWelsME sMe;

  srand((uint32_t)time(NULL));
  const uint8_t kuiQp = rand()%52;
  InitMe(kuiQp, 648, m_uiMvdTableSize, m_pMvdCostTable, &sMe);

  SMVUnitXY sTargetMv;
  WelsInitSampleSadFunc( &sFuncList, 0 );//test c functions
  WelsInitMeFunc(&sFuncList, WELS_CPU_SSE41, 1);

  uint8_t *pRefPicCenter = m_pRefPic+(m_iHeight/2)*m_iWidth+(m_iWidth/2);
  sMe.iCurMeBlockPixX = (m_iWidth/2);
  sMe.iCurMeBlockPixY = (m_iHeight/2);

  bool bDataGeneratorSucceed = false;
  bool bFoundMatch = false;
  int32_t iTryTimes=100;

  sTargetMv.iMvX = 0;
  sTargetMv.iMvY = WELS_MAX(INTPEL_NEEDED_MARGIN, rand()%m_iHeight-INTPEL_NEEDED_MARGIN);
  bDataGeneratorSucceed = false;
  bFoundMatch = false;
  while (!bFoundMatch && (iTryTimes--)>0) {
    if (!YUVPixelDataGenerator( m_pRefPic, m_iWidth, m_iHeight, m_iWidth ))
      continue;

    bDataGeneratorSucceed = true;
    CopyTargetBlock( m_pSrcBlock, 16, sTargetMv, m_iWidth, pRefPicCenter);

    //clean the sMe status
    sMe.uiBlockSize = rand()%5;
    sMe.pEncMb = m_pSrcBlock;
    sMe.pRefMb = pRefPicCenter;
    sMe.pColoRefMb = pRefPicCenter;
    sMe.sMv.iMvX = sMe.sMv.iMvY = 0;
    sMe.uiSadCost = sMe.uiSatdCost = kiMaxBlock16Sad;
    const int32_t iCurMeBlockPixX = sMe.iCurMeBlockPixX;
    const int32_t iCurMeBlockQpelPixX = ((iCurMeBlockPixX)<<2);
    const int32_t iCurMeBlockPixY = sMe.iCurMeBlockPixY;
    const int32_t iCurMeBlockQpelPixY = ((iCurMeBlockPixY)<<2);
    uint16_t* pMvdCostX = sMe.pMvdCost - iCurMeBlockQpelPixX - sMe.sMvp.iMvX;	//do the offset here
    uint16_t* pMvdCostY = sMe.pMvdCost - iCurMeBlockQpelPixY - sMe.sMvp.iMvY;
    VerticalFullSearchUsingSSE41 ( &sFuncList, &sMe,
                      pMvdCostY, pMvdCostX[ iCurMeBlockQpelPixX ],
                      m_iMaxSearchBlock, m_iWidth,
                      INTPEL_NEEDED_MARGIN,
                      m_iHeight-INTPEL_NEEDED_MARGIN, true );

    //the last selection may be affected by MVDcost, that is when smaller MvY will be better
    bFoundMatch = (sMe.sMv.iMvX==0
                   &&(sMe.sMv.iMvY==sTargetMv.iMvY||abs(sMe.sMv.iMvY)<abs(sTargetMv.iMvY)));
    //printf("TestVerticalSearch Target: %d,%d\n", sTargetMv.iMvX, sTargetMv.iMvY);
  }
  if (bDataGeneratorSucceed) {
    //if DataGenerator never succeed, there is no meaning to check iTryTimes
    ASSERT_TRUE(iTryTimes > 0);
    //it is possible that ref at differnt position is identical, but that should be under a low probability
  }
}

TEST_F(MotionEstimateTest, TestHorizontalSearch_SSE41)
{
  const int32_t kiMaxBlock16Sad = 72000;//a rough number
  SWelsFuncPtrList sFuncList;
  SWelsME sMe;

  srand((uint32_t)time(NULL));
  const uint8_t kuiQp = rand()%52;
  InitMe(kuiQp, 648, m_uiMvdTableSize, m_pMvdCostTable, &sMe);

  SMVUnitXY sTargetMv;
  WelsInitSampleSadFunc( &sFuncList, 0 );//test c functions
  WelsInitMeFunc(&sFuncList, WELS_CPU_SSE41, 1);

  uint8_t *pRefPicCenter = m_pRefPic+(m_iHeight/2)*m_iWidth+(m_iWidth/2);
  sMe.iCurMeBlockPixX = (m_iWidth/2);
  sMe.iCurMeBlockPixY = (m_iHeight/2);

  bool bDataGeneratorSucceed = false;
  bool bFoundMatch = false;
  int32_t iTryTimes=100;

  sTargetMv.iMvX = WELS_MAX(INTPEL_NEEDED_MARGIN, rand()%m_iWidth-INTPEL_NEEDED_MARGIN);
  sTargetMv.iMvY = 0;
  bDataGeneratorSucceed = false;
  bFoundMatch = false;
  while (!bFoundMatch && (iTryTimes--)>0) {
    if (!YUVPixelDataGenerator( m_pRefPic, m_iWidth, m_iHeight, m_iWidth ))
      continue;

    bDataGeneratorSucceed = true;
    CopyTargetBlock( m_pSrcBlock, 16, sTargetMv, m_iWidth, pRefPicCenter);

    //clean the sMe status
    sMe.uiBlockSize = rand()%5;
    sMe.pEncMb = m_pSrcBlock;
    sMe.pRefMb = pRefPicCenter;
    sMe.pColoRefMb = pRefPicCenter;
    sMe.sMv.iMvX = sMe.sMv.iMvY = 0;
    sMe.uiSadCost = sMe.uiSatdCost = kiMaxBlock16Sad;
    const int32_t iCurMeBlockPixX = sMe.iCurMeBlockPixX;
    const int32_t iCurMeBlockQpelPixX = ((iCurMeBlockPixX)<<2);
    const int32_t iCurMeBlockPixY = sMe.iCurMeBlockPixY;
    const int32_t iCurMeBlockQpelPixY = ((iCurMeBlockPixY)<<2);
    uint16_t* pMvdCostX = sMe.pMvdCost - iCurMeBlockQpelPixX - sMe.sMvp.iMvX;	//do the offset here
    uint16_t* pMvdCostY = sMe.pMvdCost - iCurMeBlockQpelPixY - sMe.sMvp.iMvY;
    HorizontalFullSearchUsingSSE41 ( &sFuncList, &sMe,
                      pMvdCostX, pMvdCostY[ iCurMeBlockQpelPixY ],
                      m_iMaxSearchBlock, m_iWidth,
                      INTPEL_NEEDED_MARGIN,
                      m_iWidth-INTPEL_NEEDED_MARGIN, false );

    //the last selection may be affected by MVDcost, that is when smaller MvY will be better
    bFoundMatch = (sMe.sMv.iMvY==0
                   &&(sMe.sMv.iMvX==sTargetMv.iMvX||abs(sMe.sMv.iMvX)<abs(sTargetMv.iMvX)));
    //printf("TestHorizontalSearch Target: %d,%d\n", sTargetMv.iMvX, sTargetMv.iMvY);
  }
  if (bDataGeneratorSucceed) {
    //if DataGenerator never succeed, there is no meaning to check iTryTimes
    ASSERT_TRUE(iTryTimes > 0);
    //it is possible that ref at differnt position is identical, but that should be under a low probability
  }
}
#endif