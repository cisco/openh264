#include<gtest/gtest.h>
#include<math.h>
#include<stdlib.h>
#include<time.h>

#include "cpu_core.h"
#include "cpu.h"
#include "sample.h"
#include "sad_common.h"

using namespace WelsSVCEnc;

#ifndef ASSERT_MEMORY_FAIL
#define ASSERT_MEMORY_FAIL                     \
  if (NULL == pSrcB) {                         \
    cMemoryAlign.WelsFree(pSrcA, "Sad_pSrcA"); \
    ASSERT_TRUE(0);                            \
  }
#endif

TEST(SampleTest, TestPixel_sad_4x4) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+4;
  const int32_t kiStridePixB = rand()%256+4;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<2,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<2,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<2); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<2); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 4; i++ ) {
    for(int j=0; j<4; j++)
      iSumSad+=abs(pPixA[j]-pPixB[j]);
    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  EXPECT_EQ(WelsSampleSad4x4_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB ),iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST(SampleTest, TestPixel_sad_8x8) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+8;
  const int32_t kiStridePixB = rand()%256+8;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<3,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<3,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<3); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<3); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i < 8; i++ ) {
    for(int j=0; j<8; j++)
      iSumSad+=abs(pPixA[j]-pPixB[j]);

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  EXPECT_EQ(WelsSampleSad8x8_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB ),iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}


TEST(SampleTest, TestPixel_sad_16x8) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<3,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<3,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<3); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<3); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i <8; i++ ) {
    for(int j=0; j<16; j++)
      iSumSad+=abs(pPixA[j]-pPixB[j]);

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  EXPECT_EQ(WelsSampleSad16x8_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB ),iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST(SampleTest, TestPixel_sad_8x16) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+8;
  const int32_t kiStridePixB = rand()%256+8;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i <16; i++ ) {
    for(int j=0; j<8; j++)
      iSumSad+=abs(pPixA[j]-pPixB[j]);

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  EXPECT_EQ(WelsSampleSad8x16_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB ),iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST(SampleTest, TestPixel_sad_16x16) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB;

  int32_t iSumSad = 0;
  for (int i = 0; i <16; i++ ) {
    for(int j=0; j<16; j++)
      iSumSad+=abs(pPixA[j]-pPixB[j]);

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  EXPECT_EQ(WelsSampleSad16x16_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB ),iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST(SampleTest, TestPixel_satd_4x4) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+4;
  const int32_t kiStridePixB = rand()%256+4;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<2,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<2,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<2); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<2); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB;

  int32_t W[16],T[16],Y[16],k=0;
  for(int i=0; i<4; i++) {
    for(int j=0; j<4; j++)
      W[k++]=pPixA[j]-pPixB[j];
    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  T[0]=W[0]+W[4]+W[8]+W[12];
  T[1]=W[1]+W[5]+W[9]+W[13];
  T[2]=W[2]+W[6]+W[10]+W[14];
  T[3]=W[3]+W[7]+W[11]+W[15];

  T[4]=W[0]+W[4]-W[8]-W[12];
  T[5]=W[1]+W[5]-W[9]-W[13];
  T[6]=W[2]+W[6]-W[10]-W[14];
  T[7]=W[3]+W[7]-W[11]-W[15];

  T[8]=W[0]-W[4]-W[8]+W[12];
  T[9]=W[1]-W[5]-W[9]+W[13];
  T[10]=W[2]-W[6]-W[10]+W[14];
  T[11]=W[3]-W[7]-W[11]+W[15];

  T[12]=W[0]-W[4]+W[8]-W[12];
  T[13]=W[1]-W[5]+W[9]-W[13];
  T[14]=W[2]-W[6]+W[10]-W[14];
  T[15]=W[3]-W[7]+W[11]-W[15];

  Y[0]=T[0]+T[1]+T[2]+T[3];
  Y[1]=T[0]+T[1]-T[2]-T[3];
  Y[2]=T[0]-T[1]-T[2]+T[3];
  Y[3]=T[0]-T[1]+T[2]-T[3];

  Y[4]=T[4]+T[5]+T[6]+T[7];
  Y[5]=T[4]+T[5]-T[6]-T[7];
  Y[6]=T[4]-T[5]-T[6]+T[7];
  Y[7]=T[4]-T[5]+T[6]-T[7];

  Y[8]=T[8]+T[9]+T[10]+T[11];
  Y[9]=T[8]+T[9]-T[10]-T[11];
  Y[10]=T[8]-T[9]-T[10]+T[11];
  Y[11]=T[8]-T[9]+T[10]-T[11];

  Y[12]=T[12]+T[13]+T[14]+T[15];
  Y[13]=T[12]+T[13]-T[14]-T[15];
  Y[14]=T[12]-T[13]-T[14]+T[15];
  Y[15]=T[12]-T[13]+T[14]-T[15];

  int32_t iSumSatd = 0;
  for(int i=0; i<16; i++)
    iSumSatd+=abs(Y[i]);

  EXPECT_EQ(WelsSampleSatd4x4_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB ),(iSumSatd+1)>>1 );
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

class CFuncSetSad_4_X_Test : public testing::Test {
public:
  virtual void SetUp() {
    WelsInitSampleSadFunc(&m_FuncListC, 0);
  }
  virtual void TearDown() { }
public:
  SWelsFuncPtrList m_FuncListC;
};

TEST_F(CFuncSetSad_4_X_Test, TestPixel_sad_4_16x16) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+32;
  const int32_t kiStridePixB = rand()%256+32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<5,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<5); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<5); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <16; i++ ) {
    for(int j=0; j<16; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  m_FuncListC.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x16](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(CFuncSetSad_4_X_Test, TestPixel_sad_4_16x8) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+32;
  const int32_t kiStridePixB = rand()%256+32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<5,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<5); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<5); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <8; i++ ) {
    for(int j=0; j<16; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  m_FuncListC.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x8](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(CFuncSetSad_4_X_Test, TestPixel_sad_4_8x16) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+32;
  const int32_t kiStridePixB = rand()%256+32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<5,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<5); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<5); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <16; i++ ) {
    for(int j=0; j<8; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  m_FuncListC.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x16](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(CFuncSetSad_4_X_Test, TestPixel_sad_4_8x8) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <8; i++ ) {
    for(int j=0; j<8; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  m_FuncListC.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x8](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(CFuncSetSad_4_X_Test, TestPixel_sad_4_4x4) {
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+8;
  const int32_t kiStridePixB = rand()%256+8;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<3,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<3,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<3); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<3); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <4; i++ ) {
    for(int j=0; j<4; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  m_FuncListC.sSampleDealingFuncs.pfSample4Sad[BLOCK_4x4](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

#ifdef X86_ASM
class SadSatdAssemblyFuncTest : public testing::Test {
public:
  virtual void SetUp() {
    int32_t iCpuCores = 0;
    m_uiCpuFeatureFlag = WelsCPUFeatureDetect(&iCpuCores);
  }
  virtual void TearDown() {
  }
public:
  uint32_t m_uiCpuFeatureFlag;
};

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSad4x4_mmx) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_MMXEXT))
    return;

  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+4;
  const int32_t kiStridePixB = rand()%256+4;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<2,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<2,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<2); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<2); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSad4x4_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSad4x4_mmx(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}


TEST_F(SadSatdAssemblyFuncTest, WelsSampleSad8x8_sse21) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;

  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+8;
  const int32_t kiStridePixB = rand()%256+8;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<3); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<3); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSad8x8_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSad8x8_sse21(pSrcA, kiStridePixA, pSrcB, kiStridePixB ));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSad8x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = 32;
  const int32_t kiStridePixB = 32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSad8x16_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSad8x16_sse2(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSad16x8_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = 32;
  const int32_t kiStridePixB = 32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSad16x8_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSad16x8_sse2(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSad16x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = 32;
  const int32_t kiStridePixB = 32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSad16x16_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSad16x16_sse2(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSatd4x4_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+4;
  const int32_t kiStridePixB = rand()%256+4;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<2,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<2,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<2); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<2); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSatd4x4_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSatd4x4_sse2(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}


TEST_F(SadSatdAssemblyFuncTest, WelsSampleSatd8x8_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+8;
  const int32_t kiStridePixB = rand()%256+8;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<3,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<3,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<3); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<3); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSatd8x8_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSatd8x8_sse2(pSrcA, kiStridePixA, pSrcB, kiStridePixB ));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSatd8x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSatd8x16_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSatd8x16_sse2(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSatd16x8_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSatd16x8_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSatd16x8_sse2(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSatd16x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSatd16x16_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSatd16x16_sse2(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSatd4x4_sse41) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+4;
  const int32_t kiStridePixB = rand()%256+4;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<2,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<2,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<2); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<2); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSatd4x4_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSatd4x4_sse41(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}


TEST_F(SadSatdAssemblyFuncTest, WelsSampleSatd8x8_sse41) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+8;
  const int32_t kiStridePixB = rand()%256+8;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<3,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<3,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<3); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<3); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSatd8x8_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSatd8x8_sse41(pSrcA, kiStridePixA, pSrcB, kiStridePixB ));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSatd8x16_sse41) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSatd8x16_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSatd8x16_sse41(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSatd16x8_sse41) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSatd16x8_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSatd16x8_sse41(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSatd16x16_sse41) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE41))
    return;
  srand((uint32_t)time(NULL));
  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;

  EXPECT_EQ(WelsSampleSatd16x16_c(pSrcA, kiStridePixA, pSrcB, kiStridePixB), WelsSampleSatd16x16_sse41(pSrcA, kiStridePixA, pSrcB, kiStridePixB));
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, TestPixel_sad_4_16x16_mmxext) {
  if (0 == (m_uiCpuFeatureFlag & (WELS_CPU_MMXEXT | WELS_CPU_MMX)))
    return;
  srand((uint32_t)time(NULL));
  SWelsFuncPtrList pListMMX;
  WelsInitSampleSadFunc( &pListMMX, WELS_CPU_MMXEXT | WELS_CPU_MMX );

  const int32_t kiStridePixA = rand()%256+32;
  const int32_t kiStridePixB = rand()%256+32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<5,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<5); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<5); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <16; i++ ) {
    for(int j=0; j<16; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  pListMMX.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x16](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, TestPixel_sad_4_16x8_mmxext) {
  if (0 == (m_uiCpuFeatureFlag & (WELS_CPU_MMXEXT | WELS_CPU_MMX)))
    return;
  srand((uint32_t)time(NULL));
  SWelsFuncPtrList pListMMX;
  WelsInitSampleSadFunc( &pListMMX, WELS_CPU_MMXEXT | WELS_CPU_MMX );

  const int32_t kiStridePixA = rand()%256+32;
  const int32_t kiStridePixB = rand()%256+32;
;
  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<5,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<5); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<5); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <8; i++ ) {
    for(int j=0; j<16; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  pListMMX.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x8](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, TestPixel_sad_4_8x16_mmxext) {
  if (0 == (m_uiCpuFeatureFlag & (WELS_CPU_MMXEXT | WELS_CPU_MMX)))
    return;
  srand((uint32_t)time(NULL));
  SWelsFuncPtrList pListMMX;
  WelsInitSampleSadFunc( &pListMMX, WELS_CPU_MMXEXT | WELS_CPU_MMX );

  const int32_t kiStridePixA = rand()%256+32;
  const int32_t kiStridePixB = rand()%256+32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<5,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<5); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<5); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <16; i++ ) {
    for(int j=0; j<8; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  pListMMX.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x16](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, TestPixel_sad_4_8x8_mmxext) {
  if (0 == (m_uiCpuFeatureFlag & (WELS_CPU_MMXEXT | WELS_CPU_MMX)))
    return;
  srand((uint32_t)time(NULL));
  SWelsFuncPtrList pListMMX;
  WelsInitSampleSadFunc( &pListMMX, WELS_CPU_MMXEXT | WELS_CPU_MMX );

  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <8; i++ ) {
    for(int j=0; j<8; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  pListMMX.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x8](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}


TEST_F(SadSatdAssemblyFuncTest, TestPixel_sad_4_4x4_mmxext) {
  if (0 == (m_uiCpuFeatureFlag & (WELS_CPU_MMXEXT | WELS_CPU_MMX)))
    return;
  srand((uint32_t)time(NULL));
  SWelsFuncPtrList pListMMX;
  WelsInitSampleSadFunc( &pListMMX, WELS_CPU_MMXEXT | WELS_CPU_MMX );

  const int32_t kiStridePixA = rand()%256+8;
  const int32_t kiStridePixB = rand()%256+8;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<3,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixB<<3,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<3); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<3); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <4; i++ ) {
    for(int j=0; j<4; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  pListMMX.sSampleDealingFuncs.pfSample4Sad[BLOCK_4x4](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSadFour16x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  SWelsFuncPtrList pListSSE2;
  WelsInitSampleSadFunc( &pListSSE2, WELS_CPU_SSE2 );

  const int32_t kiStridePixA = 32;
  const int32_t kiStridePixB = 32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<5); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<5); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <16; i++ ) {
    for(int j=0; j<16; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  pListSSE2.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x16](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);

  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSadFour16x8_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  SWelsFuncPtrList pListSSE2;
  WelsInitSampleSadFunc( &pListSSE2, WELS_CPU_SSE2 );

  const int32_t kiStridePixA = 32;
  const int32_t kiStridePixB = 32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<5); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<5); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <8; i++ ) {
    for(int j=0; j<16; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  pListSSE2.sSampleDealingFuncs.pfSample4Sad[BLOCK_16x8](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSadFour8x16_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  SWelsFuncPtrList pListSSE2;
  WelsInitSampleSadFunc( &pListSSE2, WELS_CPU_SSE2 );

  const int32_t kiStridePixA = rand()%256+32;
  const int32_t kiStridePixB = rand()%256+32;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<5,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<5); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<5); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <16; i++ ) {
    for(int j=0; j<8; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  pListSSE2.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x16](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}

TEST_F(SadSatdAssemblyFuncTest, WelsSampleSadFour8x8_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  SWelsFuncPtrList pListSSE2;
  WelsInitSampleSadFunc( &pListSSE2, WELS_CPU_SSE2 );

  const int32_t kiStridePixA = rand()%256+16;
  const int32_t kiStridePixB = rand()%256+16;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<4,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<4); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<4); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <8; i++ ) {
    for(int j=0; j<8; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  pListSSE2.sSampleDealingFuncs.pfSample4Sad[BLOCK_8x8](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}


TEST_F(SadSatdAssemblyFuncTest, WelsSampleSadFour4x4_sse2) {
  if (0 == (m_uiCpuFeatureFlag & WELS_CPU_SSE2))
    return;
  srand((uint32_t)time(NULL));
  SWelsFuncPtrList pListSSE2;
  WelsInitSampleSadFunc( &pListSSE2, WELS_CPU_SSE2 );

  const int32_t kiStridePixA = rand()%256+8;
  const int32_t kiStridePixB = rand()%256+8;

  CMemoryAlign cMemoryAlign(0);
  uint8_t* pSrcA = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<3,"Sad_pSrcA");
  ASSERT_TRUE(NULL != pSrcA);
  uint8_t* pSrcB = (uint8_t *)cMemoryAlign.WelsMalloc(kiStridePixA<<3,"Sad_pSrcB");
  ASSERT_MEMORY_FAIL

  for(int i=0; i<(kiStridePixA<<3); i++)
    pSrcA[i]=rand()%256;
  for(int i=0; i<(kiStridePixB<<3); i++)
    pSrcB[i]=rand()%256;
  uint8_t *pPixA=pSrcA;
  uint8_t *pPixB=pSrcB+kiStridePixB;

  int32_t iSumSad = 0;
  for (int i = 0; i <4; i++ ) {
    for(int j=0; j<4; j++) {
      iSumSad+=abs(pPixA[j]-pPixB[j-1]);
      iSumSad+=abs(pPixA[j]-pPixB[j+1]);
      iSumSad+=abs(pPixA[j]-pPixB[j-kiStridePixB]);
      iSumSad+=abs(pPixA[j]-pPixB[j+kiStridePixB]);
    }

    pPixA += kiStridePixA;
    pPixB += kiStridePixB;
  }

  int32_t iSad[4];
  pListSSE2.sSampleDealingFuncs.pfSample4Sad[BLOCK_4x4](pSrcA, kiStridePixA, pSrcB+kiStridePixB, kiStridePixB, iSad);
  EXPECT_EQ(iSad[0]+iSad[1]+iSad[2]+iSad[3],iSumSad);
  cMemoryAlign.WelsFree(pSrcA,"Sad_pSrcA");
  cMemoryAlign.WelsFree(pSrcB,"Sad_pSrcB");
}
#endif
