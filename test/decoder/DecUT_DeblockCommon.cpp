#include <gtest/gtest.h>

#include "../../codec/decoder/core/inc/deblocking.h"
#include "../../codec/common/inc/deblocking_common.h"

#define CLIP3(VALUE, MIN, MAX) ((VALUE) < (MIN) ? (MIN) : ((VALUE) > (MAX) ? (MAX) : (VALUE)))

using namespace WelsDec;

/* extern pure C functions */
extern void DeblockLumaLt4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta,
                              int8_t* pTc);
extern void DeblockLumaEq4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta);
extern void DeblockChromaLt4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha,
                                int32_t iBeta, int8_t* pTc);
extern void DeblockChromaEq4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha,
                                int32_t iBeta);

/* Macros body */
#define GENERATE_DATA_DEBLOCKING(pBase, pRef, iWidth) \
if (iNum==0) { \
  iAlpha = 255; \
  iBeta = 18; \
  iTc[0] = iTc[1] = iTc[2] = iTc[3] = 25; \
  pBase[0] = pRef[0] = 128; \
  for (int i = 1; i < iWidth*iWidth; i++) { \
  pBase[i] = pRef[i] = CLIP3(0, 255, pBase[i-1] -16 + rand()%32); \
  } \
} else if (iNum==1) { \
  iAlpha = 4; \
  iBeta = 2; \
  iTc[0] = iTc[1] = iTc[2] = iTc[3] = 9; \
  pBase[0] = pRef[0] = 128; \
  for (int i = 1; i < iWidth*iWidth; i++) { \
  pBase[i] = pRef[i] = CLIP3(0, 255, pBase[i-1] -4 + rand()%8); \
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
        pPix[iStrideX * -2] = p[1] + CLIP3 (((p[2] + ((p[0] + q[0] + 1) >> 1) - (p[1] << 1)) >> 1), -1 * pTc[iIndexTc],
                                            pTc[iIndexTc]);
        iTc++;
      }
      // 8-472
      if (abs (q[2] - q[0]) < iBeta) {
        pPix[iStrideX * 1] = q[1] + CLIP3 (((q[2] + ((p[0] + q[0] + 1) >> 1) - (q[1] << 1)) >> 1), -1 * pTc[iIndexTc],
                                           pTc[iIndexTc]);
        iTc++;
      }
      // 8-467,468,469
      iDelta = CLIP3 (((((q[0] - p[0]) << 2) + (p[1] - q[1]) + 4) >> 3), -1 * iTc, iTc);
      pPix[iStrideX * -1] = CLIP3 ((p[0] + iDelta), 0, 255);
      pPix[0] = CLIP3 ((q[0] - iDelta), 0, 255);
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
      iDelta = CLIP3 (((((q[0] - p[0]) << 2) + (p[1] - q[1]) + 4) >> 3), -1 * iTc, iTc);
      pPixCb[iStrideX * -1] = CLIP3 ((p[0] + iDelta), 0, 255);
      pPixCb[iStrideX * 0 ] = CLIP3 ((q[0] - iDelta), 0, 255);
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
      iDelta = CLIP3 (((((q[0] - p[0]) << 2) + (p[1] - q[1]) + 4) >> 3), -1 * iTc, iTc);
      pPixCr[iStrideX * -1] = CLIP3 ((p[0] + iDelta), 0, 255);
      pPixCr[iStrideX * 0 ] = CLIP3 ((q[0] - iDelta), 0, 255);
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
      pPixCb[iStrideX * -1] = CLIP3 ((2 * p[1] + p[0] + q[1] + 2) >> 2, 0, 255);
      pPixCb[iStrideX * 0 ] = CLIP3 ((2 * q[1] + q[0] + p[1] + 2) >> 2, 0, 255);
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
      pPixCr[iStrideX * -1] = CLIP3 ((2 * p[1] + p[0] + q[1] + 2) >> 2, 0, 255);
      pPixCr[iStrideX * 0 ] = CLIP3 ((2 * q[1] + q[0] + p[1] + 2) >> 2, 0, 255);
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

  bool bEqual = true;

  for (int iNum = 0; bEqual && iNum < TEST_CYCLE; iNum++) {
    /* Horizontal */
    GENERATE_DATA_DEBLOCKING (iPixBase, iPixRef, 16)

    anchor_DeblockingLumaNormal (&iPixBase[8 * 1], 1, 16, iAlpha, iBeta, iTc);
    DeblockLumaLt4_c (&iPixRef[8 * 1], 1, 16, iAlpha, iBeta, iTc);

    for (int i = 0; i < 16 * 16; i++) {
      if (iPixBase[i] != iPixRef[i]) {
        bEqual = false;
        break;
      }
    }

    /* Vertical */
    GENERATE_DATA_DEBLOCKING (iPixBase, iPixRef, 16)

    anchor_DeblockingLumaNormal (&iPixBase[8 * 16], 16, 1, iAlpha, iBeta, iTc);
    DeblockLumaLt4_c (&iPixRef[8 * 16], 16, 1, iAlpha, iBeta, iTc);

    for (int i = 0; i < 16 * 16; i++) {
      if (iPixBase[i] != iPixRef[i]) {
        bEqual = false;
        break;
      }
    }
  }

  EXPECT_TRUE (bEqual);
}
TEST (DeblockingCommon, DeblockLumaEq4_c) {
  //void DeblockLumaEq4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta)
#define TEST_CYCLE 1000
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixBase, 16 * 16, 16);
  ENFORCE_STACK_ALIGN_1D (uint8_t, iPixRef, 16 * 16, 16);

  int32_t iAlpha, iBeta;

  /* NOT used here */
  ENFORCE_STACK_ALIGN_1D (int8_t,  iTc,   4, 16);

  bool bEqual = true;

  for (int iNum = 0; bEqual && iNum < TEST_CYCLE; iNum++) {
    /* Horizontal */
    GENERATE_DATA_DEBLOCKING (iPixBase, iPixRef, 16)

    anchor_DeblockingLumaIntra (&iPixBase[8 * 1], 1, 16, iAlpha, iBeta);
    DeblockLumaEq4_c (&iPixRef[8 * 1], 1, 16, iAlpha, iBeta);

    for (int i = 0; i < 16 * 16; i++) {
      if (iPixBase[i] != iPixRef[i]) {
        bEqual = false;
        break;
      }
    }

    /* Vertical */
    GENERATE_DATA_DEBLOCKING (iPixBase, iPixRef, 16)

    anchor_DeblockingLumaIntra (&iPixBase[8 * 16], 16, 1, iAlpha, iBeta);
    DeblockLumaEq4_c (&iPixRef[8 * 16], 16, 1, iAlpha, iBeta);

    for (int i = 0; i < 16 * 16; i++) {
      if (iPixBase[i] != iPixRef[i]) {
        bEqual = false;
        break;
      }
    }
  }

  EXPECT_TRUE (bEqual);
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

  bool bEqual = true;

  for (int iNum = 0; bEqual && iNum < TEST_CYCLE; iNum++) {
    /* Horizontal */
    GENERATE_DATA_DEBLOCKING (iPixCbBase, iPixCbRef, 8)
    GENERATE_DATA_DEBLOCKING (iPixCrBase, iPixCrRef, 8)

    anchor_DeblockingChromaNormal (&iPixCbBase[4 * 1], &iPixCrBase[4 * 1], 1, 8, iAlpha, iBeta, iTc);
    DeblockChromaLt4_c (&iPixCbRef[4 * 1], &iPixCrRef[4 * 1], 1, 8, iAlpha, iBeta, iTc);

    for (int i = 0; i < 8 * 8; i++) {
      if (iPixCbBase[i] != iPixCbRef[i] ||  iPixCrBase[i] != iPixCrRef[i]) {
        bEqual = false;
        break;
      }
    }

    /* Vertical */
    GENERATE_DATA_DEBLOCKING (iPixCbBase, iPixCbRef, 8)
    GENERATE_DATA_DEBLOCKING (iPixCrBase, iPixCrRef, 8)

    anchor_DeblockingChromaNormal (&iPixCbBase[4 * 8], &iPixCrBase[4 * 8], 8, 1, iAlpha, iBeta, iTc);
    DeblockChromaLt4_c (&iPixCbRef[4 * 8], &iPixCrRef[4 * 8], 8, 1, iAlpha, iBeta, iTc);

    for (int i = 0; i < 8 * 8; i++) {
      if (iPixCbBase[i] != iPixCbRef[i] ||  iPixCrBase[i] != iPixCrRef[i]) {
        bEqual = false;
        break;
      }
    }
  }

  EXPECT_TRUE (bEqual);
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

  bool bEqual = true;

  for (int iNum = 0; bEqual && iNum < TEST_CYCLE; iNum++) {
    /* Horizontal */
    GENERATE_DATA_DEBLOCKING (iPixCbBase, iPixCbRef, 8)
    GENERATE_DATA_DEBLOCKING (iPixCrBase, iPixCrRef, 8)

    anchor_DeblockingChromaIntra (&iPixCbBase[4 * 1], &iPixCrBase[4 * 1], 1, 8, iAlpha, iBeta);
    DeblockChromaEq4_c (&iPixCbRef[4 * 1], &iPixCrRef[4 * 1], 1, 8, iAlpha, iBeta);

    for (int i = 0; i < 8 * 8; i++) {
      if (iPixCbBase[i] != iPixCbRef[i] ||  iPixCrBase[i] != iPixCrRef[i]) {
        bEqual = false;
        break;
      }
    }

    /* Vertical */
    GENERATE_DATA_DEBLOCKING (iPixCbBase, iPixCbRef, 8)
    GENERATE_DATA_DEBLOCKING (iPixCrBase, iPixCrRef, 8)

    anchor_DeblockingChromaIntra (&iPixCbBase[4 * 8], &iPixCrBase[4 * 8], 8, 1, iAlpha, iBeta);
    DeblockChromaEq4_c (&iPixCbRef[4 * 8], &iPixCrRef[4 * 8], 8, 1, iAlpha, iBeta);

    for (int i = 0; i < 8 * 8; i++) {
      if (iPixCbBase[i] != iPixCbRef[i] ||  iPixCrBase[i] != iPixCrRef[i]) {
        bEqual = false;
        break;
      }
    }
  }

  EXPECT_TRUE (bEqual);
}