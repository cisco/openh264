#include <gtest/gtest.h>

#include "../../codec/decoder/core/inc/deblocking.h"
#include "../../codec/common/inc/deblocking_common.h"

using namespace WelsDec;

/* extern pure C functions */
extern void DeblockLumaLt4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta,
                              int8_t* pTc);
extern void DeblockLumaEq4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta);
extern void DeblockChromaLt4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha,
                                int32_t iBeta, int8_t* pTc);
extern void DeblockChromaEq4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha,
                                int32_t iBeta);
namespace WelsDec {
extern void FilteringEdgeChromaHV (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag);
extern void FilteringEdgeLumaHV (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag);
}

/* Macros body */
#define GENERATE_DATA_DEBLOCKING(pBase, pRef, iWidth) \
if (iNum==0) { \
  iAlpha = 255; \
  iBeta = 18; \
  iTc[0] = iTc[1] = iTc[2] = iTc[3] = 25; \
  pBase[0] = pRef[0] = 128; \
  for (int i = 1; i < iWidth*iWidth; i++) { \
  pBase[i] = pRef[i] = WelsClip3( pBase[i-1] -16 + rand()%32, 0, 255 ); \
  } \
} else if (iNum==1) { \
  iAlpha = 4; \
  iBeta = 2; \
  iTc[0] = iTc[1] = iTc[2] = iTc[3] = 9; \
  pBase[0] = pRef[0] = 128; \
  for (int i = 1; i < iWidth*iWidth; i++) { \
  pBase[i] = pRef[i] = WelsClip3( pBase[i-1] -4 + rand()%8, 0, 255 ); \
  } \
} else { \
  iAlpha = rand() % 256; \
  iBeta = rand() % 19; \
  for (int i=0; i<4; i++) { \
  iTc[i] = rand() % 26; \
  } \
  for (int i = 0; i < iWidth*iWidth; i++) { \
  pBase[i] = pRef[i] = rand() % 256; \
  } \
}

/* NULL functions, for null call */
void UT_DeblockingFuncInterface (PDqLayer pCurDqLayer, PDeblockingFilter  filter, int32_t boundry_flag) {
  return;
}

/* Set deblocking functions to NULL */
void UT_DeblockingFuncLumaLT4Func (uint8_t* iSampleY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* iTc) {
  if (iAlpha > 0 || iBeta > 0) {
    iSampleY[0]++;
  }
  return;
}

void UT_DeblockingFuncLumaEQ4Func (uint8_t* iSampleY, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
  if (iAlpha > 0 || iBeta > 0) {
    iSampleY[0]++;
  }
  return;
}

void UT_DeblockingFuncChromaLT4Func (uint8_t* iSampleCb, uint8_t* iSampleCr, int32_t iStride, int32_t iAlpha,
                                     int32_t iBeta, int8_t* iTc) {
  if (iAlpha > 0 || iBeta > 0) {
    iSampleCb[0]++;
    iSampleCr[0]++;
  }
  return;
}

void UT_DeblockingFuncChromaEQ4Func (uint8_t* iSampleCb, uint8_t* iSampleCr, int32_t iStride, int32_t iAlpha,
                                     int32_t iBeta) {
  if (iAlpha > 0 || iBeta > 0) {
    iSampleCb[0]++;
    iSampleCr[0]++;
  }
  return;
}

/* Public function for local test */

/* Anchor functions body, some directly from the current code */
void anchor_DeblockingLumaNormal (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta,
                                  int8_t* pTc) {
  // void DeblockLumaLt4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta, int8_t* pTc)
  // bS<4, Section 8.7.2.3

  int32_t p[3];
  int32_t q[3];
  int32_t iTc;
  int32_t iDelta;
  int32_t iIndexTc;
  for (int iLine = 0; iLine < 16; iLine++) {
    iIndexTc = iLine >> 2;

    iTc = pTc[iIndexTc];
    for (int m = 0; m < 3; m++) {
      p[m] = pPix[iStrideX * -1 * (m + 1)];
      q[m] = pPix[iStrideX * m];
    }// for

    // filterSampleFlag, 8-460
    if (abs (p[0] - q[0]) < iAlpha && abs (p[1] - p[0]) < iBeta && abs (q[1] - q[0]) < iBeta) {
      // 8-470
      if (abs (p[2] - p[0]) < iBeta) {
        pPix[iStrideX * -2] = WELS_CLIP3 (p[1] + WELS_CLIP3 (((p[2] + ((p[0] + q[0] + 1) >> 1) - (p[1] << 1)) >> 1),
                                          -1 * pTc[iIndexTc], pTc[iIndexTc]), 0, 255);
        iTc++;
      }
      // 8-472
      if (abs (q[2] - q[0]) < iBeta) {
        pPix[iStrideX * 1] = WELS_CLIP3 (q[1] + WELS_CLIP3 (((q[2] + ((p[0] + q[0] + 1) >> 1) - (q[1] << 1)) >> 1),
                                         -1 * pTc[iIndexTc],  pTc[iIndexTc]), 0, 255);
        iTc++;
      }
      // 8-467,468,469
      iDelta = WELS_CLIP3 (((((q[0] - p[0]) * (1 << 2)) + (p[1] - q[1]) + 4) >> 3), -1 * iTc, iTc);
      pPix[iStrideX * -1] = WELS_CLIP3 ((p[0] + iDelta), 0, 255);
      pPix[0] = WELS_CLIP3 ((q[0] - iDelta), 0, 255);
    }

    // Next line
    pPix += iStrideY;
  }
}

void anchor_DeblockingLumaIntra (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta) {
  // void DeblockLumaEq4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta)
  // bS==4, Section 8.7.2.4

  int32_t p[4], q[4];
  for (int iLine = 0; iLine < 16; iLine++) {

    for (int m = 0; m < 4; m++) {
      p[m] = pPix[iStrideX * -1 * (m + 1)];
      q[m] = pPix[iStrideX * m];
    }

    // filterSampleFlag, 8-460
    if (abs (p[0] - q[0]) < iAlpha && abs (p[1] - p[0]) < iBeta && abs (q[1] - q[0]) < iBeta) {

      // 8-476
      if (abs (p[2] - p[0]) < iBeta && abs (p[0] - q[0]) < ((iAlpha >> 2) + 2)) {
        // 8-477,478, 479
        pPix[iStrideX * -1] = (p[2] + 2 * p[1] + 2 * p[0] + 2 * q[0] + q[1] + 4) >> 3;
        pPix[iStrideX * -2] = (p[2] + p[1] + p[0] + q[0] + 2) >> 2;
        pPix[iStrideX * -3] = (2 * p[3] + 3 * p[2] + p[1] + p[0] + q[0] + 4) >> 3;
      } else {
        // 8-480
        pPix[iStrideX * -1] = (2 * p[1] + p[0] + q[1] + 2) >> 2;
      }

      // 8-483
      if (abs (q[2] - q[0]) < iBeta && abs (p[0] - q[0]) < ((iAlpha >> 2) + 2)) {
        // 8-484,485,486
        pPix[ 0           ] = (p[1] + 2 * p[0] + 2 * q[0] + 2 * q[1] + q[2] + 4) >> 3;
        pPix[1 * iStrideX ] = (p[0] + q[0] + q[1] + q[2] + 2) >> 2;
        pPix[2 * iStrideX ] = (2 * q[3] + 3 * q[2] + q[1] + q[0] + p[0] + 4) >> 3;
      } else {
        // 8-487
        pPix[0 * iStrideX ] = (2 * q[1] + q[0] + p[1] + 2) >> 2;
      }
    }
    // Next line
    pPix += iStrideY;
  }
}

void anchor_DeblockingChromaNormal (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY,
                                    int32_t iAlpha, int32_t iBeta, int8_t* pTc) {
  // void DeblockChromaLt4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta, int8_t* pTc)
  // Section 8.7.2.3
  int32_t p[2], q[2];
  int32_t iIndexTc;
  int32_t iDelta;
  int32_t iTc;
  for (int iLine = 0; iLine < 8; iLine++) {
    iIndexTc = iLine >> 1;
    iTc = pTc[iIndexTc];
    /* for Cb */
    for (int m = 0; m < 2; m++) {
      p[m] = pPixCb[iStrideX * -1 * (m + 1)];
      q[m] = pPixCb[iStrideX * m];
    }

    // filterSampleFlag, 8-460
    if (abs (p[0] - q[0]) < iAlpha && abs (p[1] - p[0]) < iBeta && abs (q[1] - q[0]) < iBeta) {
      // 8-467, 468, 469
      iDelta = WELS_CLIP3 (((((q[0] - p[0]) * (1 << 2)) + (p[1] - q[1]) + 4) >> 3), -1 * iTc, iTc);
      pPixCb[iStrideX * -1] = WELS_CLIP3 ((p[0] + iDelta), 0, 255);
      pPixCb[iStrideX * 0 ] = WELS_CLIP3 ((q[0] - iDelta), 0, 255);
    }
    pPixCb += iStrideY;

    /* for Cr */
    for (int m = 0; m < 2; m++) {
      p[m] = pPixCr[iStrideX * -1 * (m + 1)];
      q[m] = pPixCr[iStrideX * m];
    }

    // filterSampleFlag, 8-460
    if (abs (p[0] - q[0]) < iAlpha && abs (p[1] - p[0]) < iBeta && abs (q[1] - q[0]) < iBeta) {
      // 8-467, 468, 469
      iDelta = WELS_CLIP3 (((((q[0] - p[0]) * (1 << 2)) + (p[1] - q[1]) + 4) >> 3), -1 * iTc, iTc);
      pPixCr[iStrideX * -1] = WELS_CLIP3 ((p[0] + iDelta), 0, 255);
      pPixCr[iStrideX * 0 ] = WELS_CLIP3 ((q[0] - iDelta), 0, 255);
    }
    pPixCr += iStrideY;
  }
}

void anchor_DeblockingChromaIntra (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha,
                                   int32_t iBeta) {
  //void DeblockChromaEq4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta)
  // Section 8.7.2.4
  int32_t p[2], q[2];

  for (int iLine = 0; iLine < 8; iLine++) {
    /* for Cb */
    for (int m = 0; m < 2; m++) {
      p[m] = pPixCb[iStrideX * -1 * (m + 1)];
      q[m] = pPixCb[iStrideX * m];
    }

    // filterSampleFlag, 8-460
    if (abs (p[0] - q[0]) < iAlpha && abs (p[1] - p[0]) < iBeta && abs (q[1] - q[0]) < iBeta) {
      // 8-480, 487
      pPixCb[iStrideX * -1] = WELS_CLIP3 ((2 * p[1] + p[0] + q[1] + 2) >> 2, 0, 255);
      pPixCb[iStrideX * 0 ] = WELS_CLIP3 ((2 * q[1] + q[0] + p[1] + 2) >> 2, 0, 255);
    }
    pPixCb += iStrideY;

    /* for Cr */
    for (int m = 0; m < 2; m++) {
      p[m] = pPixCr[iStrideX * -1 * (m + 1)];
      q[m] = pPixCr[iStrideX * m];
    }

    // filterSampleFlag, 8-460
    if (abs (p[0] - q[0]) < iAlpha && abs (p[1] - p[0]) < iBeta && abs (q[1] - q[0]) < iBeta) {
      // 8-480, 487
      pPixCr[iStrideX * -1] = WELS_CLIP3 ((2 * p[1] + p[0] + q[1] + 2) >> 2, 0, 255);
      pPixCr[iStrideX * 0 ] = WELS_CLIP3 ((2 * q[1] + q[0] + p[1] + 2) >> 2, 0, 255);
    }
    pPixCr += iStrideY;
  }
}

/* Unit test functions body */
TEST (DeblockingCommon, DeblockLumaLt4_c) {
  // void DeblockLumaLt4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta, int8_t* pTc)

#define TEST_CYCLE 1000
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixBase, 16 * 16, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixRef, 16 * 16, 16);

  int32_t iAlpha, iBeta;

  ENFORCE_STACK_ALIGN_1D (int8_t,  iTc,   4, 16);

  for (int iNum = 0; iNum < TEST_CYCLE; iNum++) {
    /* Horizontal */
    GENERATE_DATA_DEBLOCKING (iPixBase, iPixRef, 16)

    anchor_DeblockingLumaNormal (&iPixBase[8 * 1], 1, 16, iAlpha, iBeta, iTc);
    DeblockLumaLt4_c (&iPixRef[8 * 1], 1, 16, iAlpha, iBeta, iTc);

    for (int i = 0; i < 16 * 16; i++) {
      ASSERT_FALSE (iPixBase[i] != iPixRef[i]) << "Horizontal Error, (Pos, Base, Ref)-(" << i << "," <<
          (uint32_t)iPixBase[i] << "," << (uint32_t)iPixRef[i] << ")";
    }

    /* Vertical */
    GENERATE_DATA_DEBLOCKING (iPixBase, iPixRef, 16)

    anchor_DeblockingLumaNormal (&iPixBase[8 * 16], 16, 1, iAlpha, iBeta, iTc);
    DeblockLumaLt4_c (&iPixRef[8 * 16], 16, 1, iAlpha, iBeta, iTc);

    for (int i = 0; i < 16 * 16; i++) {
      ASSERT_FALSE (iPixBase[i] != iPixRef[i]) << "Vertical Error, (Pos, Base, Ref)-(" << i << "," <<
          (uint32_t)iPixBase[i] << "," << (uint32_t)iPixRef[i] << ")";
    }
  }
}
TEST (DeblockingCommon, DeblockLumaEq4_c) {
  //void DeblockLumaEq4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta)
#define TEST_CYCLE 1000
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixBase, 16 * 16, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixRef, 16 * 16, 16);

  int32_t iAlpha, iBeta;

  /* NOT used here */
  ENFORCE_STACK_ALIGN_1D (int8_t,  iTc,   4, 16);

  for (int iNum = 0; iNum < TEST_CYCLE; iNum++) {
    /* Horizontal */
    GENERATE_DATA_DEBLOCKING (iPixBase, iPixRef, 16)

    anchor_DeblockingLumaIntra (&iPixBase[8 * 1], 1, 16, iAlpha, iBeta);
    DeblockLumaEq4_c (&iPixRef[8 * 1], 1, 16, iAlpha, iBeta);

    for (int i = 0; i < 16 * 16; i++) {
      ASSERT_FALSE (iPixBase[i] != iPixRef[i]) << "Horizontal Error, (Pos, Base, Ref)-(" << i << "," <<
          (uint32_t)iPixBase[i] << "," << (uint32_t)iPixRef[i] << ")";
    }

    /* Vertical */
    GENERATE_DATA_DEBLOCKING (iPixBase, iPixRef, 16)

    anchor_DeblockingLumaIntra (&iPixBase[8 * 16], 16, 1, iAlpha, iBeta);
    DeblockLumaEq4_c (&iPixRef[8 * 16], 16, 1, iAlpha, iBeta);

    for (int i = 0; i < 16 * 16; i++) {
      ASSERT_FALSE (iPixBase[i] != iPixRef[i]) << "Vertical Error, (Pos, Base, Ref)-(" << i << "," <<
          (uint32_t)iPixBase[i] << "," << (uint32_t)iPixRef[i] << ")";
    }
  }
}

TEST (DeblockingCommon, DeblockChromaLt4_c) {
  // void DeblockChromaLt4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta, int8_t* pTc)
#define TEST_CYCLE 1000
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixCbBase, 8 * 8, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixCrBase, 8 * 8, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixCbRef, 8 * 8, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixCrRef, 8 * 8, 16);

  int32_t iAlpha, iBeta;

  ENFORCE_STACK_ALIGN_1D (int8_t,  iTc,   4, 16);

  for (int iNum = 0; iNum < TEST_CYCLE; iNum++) {
    /* Horizontal */
    GENERATE_DATA_DEBLOCKING (iPixCbBase, iPixCbRef, 8)
    GENERATE_DATA_DEBLOCKING (iPixCrBase, iPixCrRef, 8)

    anchor_DeblockingChromaNormal (&iPixCbBase[4 * 1], &iPixCrBase[4 * 1], 1, 8, iAlpha, iBeta, iTc);
    DeblockChromaLt4_c (&iPixCbRef[4 * 1], &iPixCrRef[4 * 1], 1, 8, iAlpha, iBeta, iTc);

    for (int i = 0; i < 8 * 8; i++) {
      ASSERT_FALSE (iPixCbBase[i] != iPixCbRef[i]
                    ||  iPixCrBase[i] != iPixCrRef[i]) << "Horizontal Error, (pos, CbBase, CbRef, CrBase, CrRef)-(" << i << "," <<
                        (uint32_t)iPixCbBase[i] << "," << (uint32_t)iPixCbRef[i] << "," << (uint32_t)iPixCrBase[i] << "," <<
                        (uint32_t)iPixCrRef[i] << ")";
    }

    /* Vertical */
    GENERATE_DATA_DEBLOCKING (iPixCbBase, iPixCbRef, 8)
    GENERATE_DATA_DEBLOCKING (iPixCrBase, iPixCrRef, 8)

    anchor_DeblockingChromaNormal (&iPixCbBase[4 * 8], &iPixCrBase[4 * 8], 8, 1, iAlpha, iBeta, iTc);
    DeblockChromaLt4_c (&iPixCbRef[4 * 8], &iPixCrRef[4 * 8], 8, 1, iAlpha, iBeta, iTc);

    for (int i = 0; i < 8 * 8; i++) {
      ASSERT_FALSE (iPixCbBase[i] != iPixCbRef[i]
                    ||  iPixCrBase[i] != iPixCrRef[i]) << "Vertical Error, (pos, CbBase, CbRef, CrBase, CrRef)-(" << i << "," <<
                        (uint32_t)iPixCbBase[i] << "," << (uint32_t)iPixCbRef[i] << "," << (uint32_t)iPixCrBase[i] << "," <<
                        (uint32_t)iPixCrRef[i] << ")";
    }
  }
}

TEST (DeblockingCommon, DeblockChromaEq4_c) {
  // void DeblockChromaEq4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta)
#define TEST_CYCLE 1000
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixCbBase, 8 * 8, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixCrBase, 8 * 8, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixCbRef, 8 * 8, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixCrRef, 8 * 8, 16);

  int32_t iAlpha, iBeta;

  /* NOT used here*/
  ENFORCE_STACK_ALIGN_1D (int8_t,  iTc,   4, 16);

  for (int iNum = 0; iNum < TEST_CYCLE; iNum++) {
    /* Horizontal */
    GENERATE_DATA_DEBLOCKING (iPixCbBase, iPixCbRef, 8)
    GENERATE_DATA_DEBLOCKING (iPixCrBase, iPixCrRef, 8)

    anchor_DeblockingChromaIntra (&iPixCbBase[4 * 1], &iPixCrBase[4 * 1], 1, 8, iAlpha, iBeta);
    DeblockChromaEq4_c (&iPixCbRef[4 * 1], &iPixCrRef[4 * 1], 1, 8, iAlpha, iBeta);

    for (int i = 0; i < 8 * 8; i++) {
      ASSERT_FALSE (iPixCbBase[i] != iPixCbRef[i]
                    ||  iPixCrBase[i] != iPixCrRef[i]) << "Horizontal Error, (pos, CbBase, CbRef, CrBase, CrRef)-(" << i << "," <<
                        (uint32_t)iPixCbBase[i] << "," << (uint32_t)iPixCbRef[i] << "," << (uint32_t)iPixCrBase[i] << "," <<
                        (uint32_t)iPixCrRef[i] << ")";
    }

    /* Vertical */
    GENERATE_DATA_DEBLOCKING (iPixCbBase, iPixCbRef, 8)
    GENERATE_DATA_DEBLOCKING (iPixCrBase, iPixCrRef, 8)

    anchor_DeblockingChromaIntra (&iPixCbBase[4 * 8], &iPixCrBase[4 * 8], 8, 1, iAlpha, iBeta);
    DeblockChromaEq4_c (&iPixCbRef[4 * 8], &iPixCrRef[4 * 8], 8, 1, iAlpha, iBeta);

    for (int i = 0; i < 8 * 8; i++) {
      ASSERT_FALSE (iPixCbBase[i] != iPixCbRef[i]
                    ||  iPixCrBase[i] != iPixCrRef[i]) << "Vertical Error, (pos, CbBase, CbRef, CrBase, CrRef)-(" << i << "," <<
                        (uint32_t)iPixCbBase[i] << "," << (uint32_t)iPixCbRef[i] << "," << (uint32_t)iPixCrBase[i] << "," <<
                        (uint32_t)iPixCrRef[i] << ")";
    }
  }
}

/////////// Logic call functions
TEST (DecoderDeblocking, DeblockingAvailableNoInterlayer) {
  // DeblockingAvailableNoInterlayer (PDqLayer pCurDqLayer, int32_t iFilterIdc)
  SDqLayer sLayer;
  int32_t iFilterIdc;
  int32_t iSliceIdc[9];

  sLayer.pSliceIdc = iSliceIdc;

  /* iFilterIdc only support 0 and 2, which is related with the encode configuration */
  /* Using 3x3 grids to simulate the different situations */

#define UT_DBAvailable_idc_0(iX, iY, iExpect) \
  iFilterIdc = 0; \
  sLayer.iMbX = iX; \
  sLayer.iMbY = iY; \
  sLayer.iMbXyIndex =  sLayer.iMbX + sLayer.iMbY*3; \
  sLayer.iMbWidth = 3; \
  EXPECT_TRUE(DeblockingAvailableNoInterlayer (&sLayer, iFilterIdc)==iExpect);

#define UT_DBAvailable_idc_2_same_slice(iX, iY, iExpect) \
  iFilterIdc = 2; \
  sLayer.iMbX = iX; \
  sLayer.iMbY = iY; \
  sLayer.iMbXyIndex =  sLayer.iMbX + sLayer.iMbY*3; \
  sLayer.iMbWidth = 3; \
  iSliceIdc[0] = rand()%10; \
  for (int i=1; i<9; i++) { \
    iSliceIdc[i] = iSliceIdc[0]; \
  } \
  EXPECT_TRUE(DeblockingAvailableNoInterlayer (&sLayer, iFilterIdc)==iExpect)<<"Same Slice";

#define UT_DBAvailable_idc_2_diff_slice(iX, iY, iExpect) \
  iFilterIdc = 2; \
  sLayer.iMbX = iX; \
  sLayer.iMbY = iY; \
  sLayer.iMbXyIndex =  sLayer.iMbX + sLayer.iMbY*3; \
  sLayer.iMbWidth = 3; \
  for (int i=0; i<9; i++) { \
    iSliceIdc[i] = i; \
  } \
  EXPECT_TRUE(DeblockingAvailableNoInterlayer (&sLayer, iFilterIdc)==iExpect)<<"Different Slice";

  // (1) idc==0
  UT_DBAvailable_idc_0 (0, 0, 0x00)
  UT_DBAvailable_idc_0 (0, 1, 0x02)
  UT_DBAvailable_idc_0 (0, 2, 0x02)
  UT_DBAvailable_idc_0 (1, 0, 0x01)
  UT_DBAvailable_idc_0 (1, 1, 0x03)
  UT_DBAvailable_idc_0 (1, 2, 0x03)
  UT_DBAvailable_idc_0 (2, 0, 0x01)
  UT_DBAvailable_idc_0 (2, 1, 0x03)
  UT_DBAvailable_idc_0 (2, 2, 0x03)

  // (2) idc==2, same slice
  UT_DBAvailable_idc_2_same_slice (0, 0, 0x00)
  UT_DBAvailable_idc_2_same_slice (0, 1, 0x02)
  UT_DBAvailable_idc_2_same_slice (0, 2, 0x02)
  UT_DBAvailable_idc_2_same_slice (1, 0, 0x01)
  UT_DBAvailable_idc_2_same_slice (1, 1, 0x03)
  UT_DBAvailable_idc_2_same_slice (1, 2, 0x03)
  UT_DBAvailable_idc_2_same_slice (2, 0, 0x01)
  UT_DBAvailable_idc_2_same_slice (2, 1, 0x03)
  UT_DBAvailable_idc_2_same_slice (2, 2, 0x03)

  // (3) idc==3, diff slice
  UT_DBAvailable_idc_2_diff_slice (0, 0, 0x00)
  UT_DBAvailable_idc_2_diff_slice (0, 1, 0x00)
  UT_DBAvailable_idc_2_diff_slice (0, 2, 0x00)
  UT_DBAvailable_idc_2_diff_slice (1, 0, 0x00)
  UT_DBAvailable_idc_2_diff_slice (1, 1, 0x00)
  UT_DBAvailable_idc_2_diff_slice (1, 2, 0x00)
  UT_DBAvailable_idc_2_diff_slice (2, 0, 0x00)
  UT_DBAvailable_idc_2_diff_slice (2, 1, 0x00)
  UT_DBAvailable_idc_2_diff_slice (2, 2, 0x00)
}

TEST (DecoderDeblocking, DeblockingInit) {
  // void  DeblockingInit (PDeblockingFunc pDeblockingFunc,  int32_t iCpu)
  SDeblockingFunc sDBFunc;
  memset (&sDBFunc, 0, sizeof (SDeblockingFunc));

#define DB_FUNC_CPUFLAG(idx) \
  EXPECT_TRUE(sDBFunc.pfLumaDeblockingLT4Ver == &DeblockLumaLt4V_##idx); \
  EXPECT_TRUE(sDBFunc.pfLumaDeblockingEQ4Ver == &DeblockLumaEq4V_##idx); \
  EXPECT_TRUE(sDBFunc.pfLumaDeblockingLT4Hor == &DeblockLumaLt4H_##idx); \
  EXPECT_TRUE(sDBFunc.pfLumaDeblockingEQ4Hor == &DeblockLumaEq4H_##idx); \
  EXPECT_TRUE(sDBFunc.pfChromaDeblockingLT4Ver == &DeblockChromaLt4V_##idx); \
  EXPECT_TRUE(sDBFunc.pfChromaDeblockingEQ4Ver == &DeblockChromaEq4V_##idx); \
  EXPECT_TRUE(sDBFunc.pfChromaDeblockingLT4Hor == &DeblockChromaLt4H_##idx); \
  EXPECT_TRUE(sDBFunc.pfChromaDeblockingEQ4Hor == &DeblockChromaEq4H_##idx);

#ifndef X86_ASM
  // pure C
  DeblockingInit (&sDBFunc, 0x00000000);
  DB_FUNC_CPUFLAG (c)
#endif

#ifdef X86_ASM
  // pure C
  DeblockingInit (&sDBFunc, 0x00000000);
  DB_FUNC_CPUFLAG (c)

  // SSE3
  DeblockingInit (&sDBFunc, 0x00000200);
  DB_FUNC_CPUFLAG (ssse3)
#endif

#ifdef HAVE_NEON
  // pure C
  DeblockingInit (&sDBFunc, 0x00000000);
  DB_FUNC_CPUFLAG (c)

  // NEON
  DeblockingInit (&sDBFunc, 0x000004);
  DB_FUNC_CPUFLAG (neon)
#endif

#ifdef HAVE_NEON_AARCH64
  // pure C
  DeblockingInit (&sDBFunc, 0x00000000);
  DB_FUNC_CPUFLAG (c)

  // NEON_AARCH64
  DeblockingInit (&sDBFunc, 0x000004);
  DB_FUNC_CPUFLAG (AArch64_neon)
#endif

#ifdef HAVE_MMI
  // pure C
  DeblockingInit (&sDBFunc, 0x00000000);
  DB_FUNC_CPUFLAG (c)

  // mmi
  DeblockingInit (&sDBFunc, 0x00000001);
  DB_FUNC_CPUFLAG (mmi)
#endif

}

TEST (DecoderDeblocking, WelsDeblockingFilterSlice) {
  // void WelsDeblockingFilterSlice (PWelsDecoderContext pCtx, PDeblockingFilterMbFunc pDeblockMb)

  /* NOT support FMO now */
  SWelsDecoderContext sCtx;
  SDqLayer sDqLayer;
  SSps sSPS;
  SPps sPPS;
  SPicture sDec;
  PDeblockingFilterMbFunc pDeblockMb = &UT_DeblockingFuncInterface;

  /* NOT do actual deblocking process, set related parameters to null */
  sCtx.pDec = &sDec;
  sCtx.pDec->iLinesize[0] = sCtx.pDec->iLinesize[1] = sCtx.pDec->iLinesize[2] = 0;
  sCtx.pDec->pData[0] = sCtx.pDec->pData[1] = sCtx.pDec->pData[2] = NULL;

  /* As no FMO in encoder now, the multi slicegroups has not been set */
  sCtx.pFmo = NULL;

  sCtx.pCurDqLayer = &sDqLayer;
  /* As void return, using iMbXyIndex to reflect whether the all MBs have been passed. */
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.iFirstMbInSlice = 0;
  sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice = 0;

  // whether disable Deblocking Filter Idc
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.uiDisableDeblockingFilterIdc = 0;
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.iSliceAlphaC0Offset = 0;
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.iSliceBetaOffset = 0;

  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pSps = &sSPS;
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pSps->uiTotalMbCount = 0;

  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pPps = &sPPS;
  /* Only test one slicegroup, not reflect the FMO func */
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pPps->uiNumSliceGroups = 1;

  // (1) Normal case, the iTotalMbInCurSlice == pSps->uiTotalMbCount
  sDqLayer.iMbX = sDqLayer.iMbY = 0;
  sDqLayer.iMbXyIndex = 0;
  sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice = 1 + rand() % 256; // at least one MB
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pSps->uiTotalMbCount =
    sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice;
  sDqLayer.iMbWidth = 1 + rand() % 128;
  WelsDeblockingFilterSlice (&sCtx, pDeblockMb);
  EXPECT_TRUE ((sDqLayer.iMbXyIndex + 1) == sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice) << sDqLayer.iMbXyIndex
      << " " << sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice;

  // (2) Normal case, multi slices, iTotalMbInCurSlice <= pSps->uiTotalMbCount
  sDqLayer.iMbX = sDqLayer.iMbY = 0;
  sDqLayer.iMbXyIndex = 0;
  sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice = 1 + rand() % 256;
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pSps->uiTotalMbCount =
    sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice + rand() % 256;
  sDqLayer.iMbWidth = 1 + rand() % 128;
  WelsDeblockingFilterSlice (&sCtx, pDeblockMb);
  EXPECT_TRUE ((sDqLayer.iMbXyIndex + 1) == sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice);

  // (3) Special case, iTotalMbInCurSlice >= pSps->uiTotalMbCount, JUST FOR TEST
  sDqLayer.iMbX = sDqLayer.iMbY = 0;
  sDqLayer.iMbXyIndex = 0;
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pSps->uiTotalMbCount = 1 + rand() % 256;
  sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice =
    sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pSps->uiTotalMbCount + rand() % 256;
  sDqLayer.iMbWidth = 1 + rand() % 128;
  WelsDeblockingFilterSlice (&sCtx, pDeblockMb);
  EXPECT_TRUE ((uint32_t) (sDqLayer.iMbXyIndex + 1) ==
               sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pSps->uiTotalMbCount);

  // (4) Special case, uiDisableDeblockingFilterIdc==1, disable deblocking
  sDqLayer.iMbX = sDqLayer.iMbY = 0;
  sDqLayer.iMbXyIndex = 0;
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.uiDisableDeblockingFilterIdc = 1;
  sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice = 1 + rand() % 256;
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.pSps->uiTotalMbCount =
    sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice;
  sDqLayer.iMbWidth = 1 + rand() % 128;
  WelsDeblockingFilterSlice (&sCtx, pDeblockMb);
  EXPECT_TRUE (sDqLayer.iMbXyIndex == 0) << sDqLayer.iMbXyIndex << " " <<
                                         sDqLayer.sLayerInfo.sSliceInLayer.iTotalMbInCurSlice;

}

TEST (DecoderDeblocking, FilteringEdgeChromaHV) {
  // void FilteringEdgeChromaHV (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag)
  SDqLayer sDqLayer;
  SDeblockingFilter sFilter;
  int32_t iBoundryFlag = 0x01;
  int32_t iQP;

  memset (&sDqLayer, 0, sizeof (SDqLayer));
  memset (&sFilter, 0, sizeof (SDeblockingFilter));

  SDeblockingFunc sDBFunc;
  sFilter.pLoopf = &sDBFunc;
  sFilter.pLoopf->pfChromaDeblockingLT4Hor = &UT_DeblockingFuncChromaLT4Func;
  sFilter.pLoopf->pfChromaDeblockingLT4Ver = &UT_DeblockingFuncChromaLT4Func;
  sFilter.pLoopf->pfChromaDeblockingEQ4Hor = &UT_DeblockingFuncChromaEQ4Func;
  sFilter.pLoopf->pfChromaDeblockingEQ4Ver = &UT_DeblockingFuncChromaEQ4Func;

  int8_t iChromaQP[9][2];
  sDqLayer.pChromaQp = iChromaQP;

  uint8_t iCb[9] = {0};
  uint8_t iCr[9] = {0};
  sFilter.pCsData[1] = iCb;
  sFilter.pCsData[2] = iCr;
  sFilter.iCsStride[0] = sFilter.iCsStride[1] = 2;

  sDqLayer.iMbX = 0;
  sDqLayer.iMbY = 0; //Only for test easy
  sDqLayer.iMbXyIndex = 1;  // this function has NO iMbXyIndex validation

#define UT_DB_CHROMA_TEST(iFlag, iQP, iV0, iV1, iV2) \
  iBoundryFlag = iFlag; \
  memset(iChromaQP, iQP, sizeof(int8_t)*9*2); \
  memset(iCb, 0, sizeof(uint8_t)*9); \
  memset(iCr, 0, sizeof(uint8_t)*9); \
  FilteringEdgeChromaHV(&sDqLayer, &sFilter, iBoundryFlag); \
  EXPECT_TRUE(iCb[0]==iV0 && iCr[0]==iV0); \
  EXPECT_TRUE(iCb[2<<1]==iV1 && iCr[2<<1]==iV1); \
  EXPECT_TRUE(iCb[(2<<1)*sFilter.iCsStride[1]]==iV2 && iCr[(2<<1)*sFilter.iCsStride[1]]==iV2);

  // QP<=15, iAlpha == iBeta == 0, TOP & LEFT
  iQP = rand() % 16;
  UT_DB_CHROMA_TEST (0x03, iQP, 0, 0, 0)

  // QP>=16, iAlpha>0 && iBeta>0, TOP & LEFT
  iQP = 16 + rand() % 35;
  UT_DB_CHROMA_TEST (0x03, iQP, 2, 1, 1)

  // QP<=15, iAlpha == iBeta == 0, TOP | LEFT
  iQP = rand() % 16;
  UT_DB_CHROMA_TEST (0x01, iQP, 0, 0, 0)
  iQP = rand() % 16;
  UT_DB_CHROMA_TEST (0x02, iQP, 0, 0, 0)

  // QP>=16, iAlpha>0 && iBeta>0, TOP | LEFT
  iQP = 16 + rand() % 35;
  UT_DB_CHROMA_TEST (0x01, iQP, 1, 1, 1)
  iQP = 16 + rand() % 35;
  UT_DB_CHROMA_TEST (0x02, iQP, 1, 1, 1)

  // QP<=15, iAlpha == iBeta == 0, !TOP & !LEFT
  iQP = rand() % 16;
  UT_DB_CHROMA_TEST (0x00, iQP, 0, 0, 0)

  // QP>=16, iAlpha>0 && iBeta>0, !TOP & !LEFT
  iQP = 16 + rand() % 35;
  UT_DB_CHROMA_TEST (0x00, iQP, 0, 1, 1)
}

TEST (DecoderDeblocking, FilteringEdgeLumaHV) {
  // void FilteringEdgeLumaHV (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag)
  SDqLayer sDqLayer;
  SDeblockingFilter sFilter;
  int32_t iBoundryFlag = 0x03;
  int32_t iQP;

  memset (&sDqLayer, 0, sizeof (SDqLayer));
  memset (&sFilter, 0, sizeof (SDeblockingFilter));

  SDeblockingFunc sDBFunc;
  sFilter.pLoopf = &sDBFunc;
  sFilter.pLoopf->pfLumaDeblockingLT4Hor = &UT_DeblockingFuncLumaLT4Func;
  sFilter.pLoopf->pfLumaDeblockingEQ4Hor = &UT_DeblockingFuncLumaEQ4Func;
  sFilter.pLoopf->pfLumaDeblockingLT4Ver = &UT_DeblockingFuncLumaLT4Func;
  sFilter.pLoopf->pfLumaDeblockingEQ4Ver = &UT_DeblockingFuncLumaEQ4Func;

  int8_t iLumaQP[50];
  sDqLayer.pLumaQp = iLumaQP;

  uint8_t iY[50] = {0};
  sFilter.pCsData[0] = iY;
  sFilter.iCsStride[0] = sFilter.iCsStride[1] = 4;

  sDqLayer.iMbX = 0;
  sDqLayer.iMbY = 0; //Only for test easy
  sDqLayer.iMbXyIndex = 1;  // this function has NO iMbXyIndex validation

  bool bTSize8x8Flag[50] = {false};
  sDqLayer.pTransformSize8x8Flag = bTSize8x8Flag;
  sDqLayer.pTransformSize8x8Flag[sDqLayer.iMbXyIndex] = false;

#define UT_DB_LUMA_TEST(iFlag, iQP, iV0, iV1, iV2) \
  iBoundryFlag = iFlag; \
  memset(iLumaQP, iQP, sizeof(int8_t)*50); \
  memset(iY, 0, sizeof(uint8_t)*50); \
  FilteringEdgeLumaHV(&sDqLayer, &sFilter, iBoundryFlag); \
  EXPECT_TRUE(iY[0]==iV0); \
  EXPECT_TRUE(iY[1<<2]==iV1 && iY[2<<2]==iV1 && iY[3<<2]==iV1); \
  EXPECT_TRUE(iY[(1 << 2)*sFilter.iCsStride[0]]==iV2 && iY[(2 << 2)*sFilter.iCsStride[0]]==iV2 && iY[(3 << 2)*sFilter.iCsStride[0]]==iV2);

  // QP<=15, iAlpha == iBeta == 0, TOP & LEFT
  iQP = rand() % 16;
  UT_DB_LUMA_TEST (0x03, iQP, 0, 0, 0)

  // QP>=16, iAlpha>0 && iBeta>0, TOP & LEFT
  iQP = 16 + rand() % 35;
  UT_DB_LUMA_TEST (0x03, iQP, 2, 1, 1)

  // QP<=15, iAlpha == iBeta == 0, TOP | LEFT
  iQP = rand() % 16;
  UT_DB_LUMA_TEST (0x01, iQP, 0, 0, 0)
  iQP = rand() % 16;
  UT_DB_LUMA_TEST (0x02, iQP, 0, 0, 0)

  // QP>=16, iAlpha>0 && iBeta>0, TOP | LEFT
  iQP = 16 + rand() % 35;
  UT_DB_LUMA_TEST (0x01, iQP, 1, 1, 1)
  iQP = 16 + rand() % 35;
  UT_DB_LUMA_TEST (0x02, iQP, 1, 1, 1)

  // QP<=15, iAlpha == iBeta == 0, !TOP & !LEFT
  iQP = rand() % 16;
  UT_DB_LUMA_TEST (0x00, iQP, 0, 0, 0)

  // QP>=16, iAlpha>0 && iBeta>0, !TOP & !LEFT
  iQP = 16 + rand() % 35;
  UT_DB_LUMA_TEST (0x00, iQP, 0, 1, 1)
}

/////////// Bs calculation functions
TEST (DecoderDeblocking, DeblockingBsMarginalMBAvcbase) {
  // uint32_t DeblockingBsMarginalMBAvcbase (PDqLayer pCurDqLayer, int32_t iEdge, int32_t iNeighMb, int32_t iMbXy)
  /* Calculate the Bs equal to 2 or 1 */
  SDqLayer sDqLayer;

  // Only define 2 MBs here
  int8_t iNoZeroCount[24 * 2]; // (*pNzc)[24]
  int8_t iLayerRefIndex[2][16 * 2]; // (*pRefIndex[LIST_A])[MB_BLOCK4x4_NUM];
  int16_t iLayerMv[2][16 * 2][2]; //(*pMv[LIST_A])[MB_BLOCK4x4_NUM][MV_A];
  uint32_t uiBSx4;
  uint8_t* pBS = (uint8_t*) (&uiBSx4);

  sDqLayer.pNzc = (int8_t (*)[24])iNoZeroCount;
  sDqLayer.pRefIndex[0] = (int8_t (*)[16])&iLayerRefIndex[0];
  sDqLayer.pRefIndex[1] = (int8_t (*)[16])&iLayerRefIndex[1];

  sDqLayer.pMv[0] = (int16_t (*) [16][2])&iLayerMv[0];
  sDqLayer.pMv[1] = (int16_t (*) [16][2])&iLayerMv[1];

  bool bTSize8x8Flag[50] = {false};
  sDqLayer.pTransformSize8x8Flag = bTSize8x8Flag;
  memset (bTSize8x8Flag, 0, sizeof (bool) * 50);

#define UT_DB_CLEAN_STATUS \
  memset(iNoZeroCount, 0, sizeof(int8_t)*24*2); \
  memset(iLayerRefIndex, 0, sizeof(int8_t)*2*16*2); \
  memset(iLayerMv, 0, sizeof(int16_t)*2*16*2*2);

#define SET_REF_VALUE(value, pos) \
  uiBSx4 = 0; \
  pBS[pos] = value;

  int32_t iCurrBlock, iNeighborBlock;

  /* Cycle for each block and its neighboring block */
  for (int iEdge = 0; iEdge < 2; iEdge++) { // Vertical and Horizontal
    for (int iPos = 0; iPos < 4; iPos++) { // Four different blocks on the edge
      iCurrBlock = (iEdge == 0 ? 4 * iPos : iPos);
      iNeighborBlock = (iEdge == 0 ? (3 + iPos * 4) : (12 + iPos));

      // (1) iEdge == 0, current block NoZeroCount != 0
      UT_DB_CLEAN_STATUS
      iNoZeroCount[0 * 24 + iCurrBlock] = 1; // Current MB_block position
      SET_REF_VALUE(2, iPos);
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1,
                   0) == uiBSx4) << iEdge << " " << iPos << " NoZeroCount!=0";

      // (2) iEdge == 0, neighbor block NoZeroCount != 0
      UT_DB_CLEAN_STATUS
      iNoZeroCount[1 * 24 + iNeighborBlock ] = 1; // Neighbor MB_block position
      SET_REF_VALUE(2, iPos);
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1,
                   0) == uiBSx4) << iEdge << " " << iPos << " NoZeroCount!=0";

      // (3) iEdge == 0, reference idx diff
      UT_DB_CLEAN_STATUS
      iLayerRefIndex[0][0 * 16 + iCurrBlock] = 0;
      iLayerRefIndex[0][1 * 16 + iNeighborBlock] = 1;
      SET_REF_VALUE(1, iPos);
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1,
                   0) == uiBSx4) << iEdge << " " << iPos << " Ref idx diff";

      // (4) iEdge == 0, abs(mv diff) < 4
      UT_DB_CLEAN_STATUS
      iLayerMv[0][0 * 16 + iCurrBlock][0] = rand() % 4;
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1, 0) == 0) << iEdge << " " << iPos << " diff_mv < 4";

      UT_DB_CLEAN_STATUS
      iLayerMv[0][0 * 16 + iCurrBlock][1] = rand() % 4;
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1, 0) == 0) << iEdge << " " << iPos << " diff_mv < 4";

      UT_DB_CLEAN_STATUS
      iLayerMv[0][1 * 16 + iNeighborBlock][0] = rand() % 4;
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1, 0) == 0) << iEdge << " " << iPos << " diff_mv < 4";

      UT_DB_CLEAN_STATUS
      iLayerMv[0][1 * 16 + iNeighborBlock][1] = rand() % 4;
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1, 0) == 0) << iEdge << " " << iPos << " diff_mv < 4";

      // (5) iEdge == 0, abs(mv diff) > 4
      UT_DB_CLEAN_STATUS
      iLayerMv[0][0 * 16 + iCurrBlock][0] = 4;
      SET_REF_VALUE(1, iPos);
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1,
                   0) == uiBSx4) << iEdge << " " << iPos << " diff_mv == 4";

      UT_DB_CLEAN_STATUS
      iLayerMv[0][0 * 16 + iCurrBlock][1] = 4;
      SET_REF_VALUE(1, iPos);
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1,
                   0) == uiBSx4) << iEdge << " " << iPos << " diff_mv == 4";

      UT_DB_CLEAN_STATUS
      iLayerMv[0][1 * 16 + iNeighborBlock][0] = 4;
      SET_REF_VALUE(1, iPos);
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1,
                   0) == uiBSx4) << iEdge << " " << iPos << " diff_mv == 4";

      UT_DB_CLEAN_STATUS
      iLayerMv[0][1 * 16 + iNeighborBlock][1] = 4;
      SET_REF_VALUE(1, iPos);
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1,
                   0) == uiBSx4) << iEdge << " " << iPos << " diff_mv == 4";

      UT_DB_CLEAN_STATUS
      iLayerMv[0][0 * 16 + iCurrBlock][0] = -2048;
      iLayerMv[0][1 * 16 + iNeighborBlock][0] = 2047;
      SET_REF_VALUE(1, iPos);
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1,
                   0) == uiBSx4) << iEdge << " " << iPos << " diff_mv == maximum";

      UT_DB_CLEAN_STATUS
      iLayerMv[0][0 * 16 + iCurrBlock][1] = -2048;
      iLayerMv[0][1 * 16 + iNeighborBlock][1] = 2047;
      SET_REF_VALUE(1, iPos);
      EXPECT_TRUE (DeblockingBsMarginalMBAvcbase (&sDqLayer, iEdge, 1,
                   0) == uiBSx4) << iEdge << " " << iPos << " diff_mv == maximum";
    }
  }
}

TEST (Deblocking, WelsDeblockingMb) {
  // void WelsDeblockingMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag)
  /* Deblock one MB, calculate the Bs inside the function, only consider the intra / intra block */
  SDqLayer sDqLayer;
  sDqLayer.sLayerInfo.sSliceInLayer.sSliceHeaderExt.sSliceHeader.eSliceType = P_SLICE;

  SDeblockingFilter sFilter;
  SDeblockingFunc sDBFunc;
  sFilter.pLoopf = &sDBFunc;
  sFilter.pLoopf->pfChromaDeblockingLT4Hor = &UT_DeblockingFuncChromaLT4Func;
  sFilter.pLoopf->pfChromaDeblockingLT4Ver = &UT_DeblockingFuncChromaLT4Func;
  sFilter.pLoopf->pfChromaDeblockingEQ4Hor = &UT_DeblockingFuncChromaEQ4Func;
  sFilter.pLoopf->pfChromaDeblockingEQ4Ver = &UT_DeblockingFuncChromaEQ4Func;
  sFilter.pLoopf->pfLumaDeblockingLT4Hor = &UT_DeblockingFuncLumaLT4Func;
  sFilter.pLoopf->pfLumaDeblockingEQ4Hor = &UT_DeblockingFuncLumaEQ4Func;
  sFilter.pLoopf->pfLumaDeblockingLT4Ver = &UT_DeblockingFuncLumaLT4Func;
  sFilter.pLoopf->pfLumaDeblockingEQ4Ver = &UT_DeblockingFuncLumaEQ4Func;

  sDqLayer.iMbX = sDqLayer.iMbY = 0;
  sDqLayer.iMbXyIndex = 1;
  sDqLayer.iMbWidth = 1;

  bool bTSize8x8Flag[50] = {false};
  sDqLayer.pTransformSize8x8Flag = bTSize8x8Flag;
  memset (bTSize8x8Flag, 0, sizeof (bool) * 50);

  uint8_t iY[50] = {0};
  sFilter.pCsData[0] = iY;
  sFilter.iCsStride[0] = 4;

  uint8_t iCb[9] = {0};
  uint8_t iCr[9] = {0};
  sFilter.pCsData[1] = iCb;
  sFilter.pCsData[2] = iCr;
  sFilter.iCsStride[1] = 2;

  int8_t iLumaQP[50] = {0};
  int8_t iChromaQP[9][2] = {{0, 0}};
  sDqLayer.pLumaQp = iLumaQP;
  sDqLayer.pChromaQp = iChromaQP;

  uint32_t iMbType[2];
  sDqLayer.pMbType = iMbType;
  sDqLayer.pMbType[0] = MB_TYPE_INTRA4x4;
  sDqLayer.pMbType[1] = MB_TYPE_INTRA4x4;

  sFilter.iSliceAlphaC0Offset = 0;
  sFilter.iSliceBetaOffset = 0;

  int32_t iQP;

#define UT_DB_MACROBLOCK_TEST( iBoundFlag, iQP, iLumaV0, iLumaV1, iLumaV2, iChromaV0, iChromaV1, iChromaV2 ) \
  memset(sDqLayer.pLumaQp, iQP, sizeof(int8_t)*50); \
  memset(sDqLayer.pChromaQp, iQP, sizeof(int8_t)*9*2); \
  memset(sFilter.pCsData[0], 0, sizeof(int8_t)*50); \
  memset(sFilter.pCsData[1], 0, sizeof(int8_t)*9); \
  memset(sFilter.pCsData[2], 0, sizeof(int8_t)*9); \
  WelsDeblockingMb(&sDqLayer, &sFilter, iBoundFlag ); \
  EXPECT_TRUE(iY[0]==iLumaV0)<<iQP<<" "<<sDqLayer.pMbType[1]; \
  EXPECT_TRUE(iY[1<<2]==iLumaV1 && iY[2<<2]==iLumaV1 && iY[3<<2]==iLumaV1)<<iQP<<" "<<sDqLayer.pMbType[1]; \
  EXPECT_TRUE(iY[(1 << 2)*sFilter.iCsStride[0]]==iLumaV2 && iY[(2 << 2)*sFilter.iCsStride[0]]==iLumaV2 && iY[(3 << 2)*sFilter.iCsStride[0]]==iLumaV2)<<iQP<<" "<<sDqLayer.pMbType[1]; \
  EXPECT_TRUE(iCb[0]==iChromaV0 && iCr[0]==iChromaV0)<<iQP<<" "<<sDqLayer.pMbType[1]; \
  EXPECT_TRUE(iCb[2<<1]==iChromaV1 && iCr[2<<1]==iChromaV1)<<iQP<<" "<<sDqLayer.pMbType[1]; \
  EXPECT_TRUE(iCb[(2<<1)*sFilter.iCsStride[1]]==iChromaV2 && iCr[(2<<1)*sFilter.iCsStride[1]]==iChromaV2)<<iQP<<" "<<sDqLayer.pMbType[1];

  // QP>16, LEFT & TOP, Intra mode MB_TYPE_INTRA4x4
  iQP = 16 + rand() % 35;
  sDqLayer.pMbType[1] = MB_TYPE_INTRA4x4;
  UT_DB_MACROBLOCK_TEST (0x03, iQP, 2, 1, 1, 2, 1, 1)

  // QP>16, LEFT & TOP, Intra mode MB_TYPE_INTRA16x16
  iQP = 16 + rand() % 35;
  sDqLayer.pMbType[1] = MB_TYPE_INTRA16x16;
  UT_DB_MACROBLOCK_TEST (0x03, iQP, 2, 1, 1, 2, 1, 1)

  // MbType==0x03, Intra8x8 has not been supported now.

  // QP>16, LEFT & TOP, Intra mode MB_TYPE_INTRA_PCM
  iQP = 16 + rand() % 35;
  sDqLayer.pMbType[1] = MB_TYPE_INTRA_PCM;
  UT_DB_MACROBLOCK_TEST (0x03, iQP, 2, 1, 1, 2, 1, 1)

  // QP>16, LEFT & TOP, neighbor is Intra
  iQP = 16 + rand() % 35;
  sDqLayer.pMbType[0] = MB_TYPE_INTRA16x16;
  sDqLayer.pMbType[1] = MB_TYPE_SKIP; // Internal SKIP, Bs==0
  UT_DB_MACROBLOCK_TEST (0x03, iQP, 2, 0, 0, 2, 0, 0)

  // QP<15, no output
  iQP = rand() % 16;
  sDqLayer.pMbType[1] = MB_TYPE_INTRA_PCM;
  UT_DB_MACROBLOCK_TEST (0x03, iQP, 0, 0, 0, 0, 0, 0)
}
