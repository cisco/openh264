#include "deblocking_common.h"
#include "macros.h"
//  C code only
void_t DeblockLumaLt4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta,
                         int8_t* pTc) {
  for (int32_t i = 0; i < 16; i++) {
    int32_t iTc0 = pTc[i >> 2];
    if (iTc0 >= 0) {
      int32_t p0 = pPix[-iStrideX];
      int32_t p1 = pPix[-2 * iStrideX];
      int32_t p2 = pPix[-3 * iStrideX];
      int32_t q0 = pPix[0];
      int32_t q1 = pPix[iStrideX];
      int32_t q2 = pPix[2 * iStrideX];
      bool_t bDetaP0Q0 = WELS_ABS (p0 - q0) < iAlpha;
      bool_t bDetaP1P0 = WELS_ABS (p1 - p0) < iBeta;
      bool_t bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;
      int32_t iTc = iTc0;
      if (bDetaP0Q0 && bDetaP1P0 && bDetaQ1Q0) {
        bool_t bDetaP2P0 =  WELS_ABS (p2 - p0) < iBeta;
        bool_t bDetaQ2Q0 =  WELS_ABS (q2 - q0) < iBeta;
        if (bDetaP2P0) {
          pPix[-2 * iStrideX] = p1 + WELS_CLIP3 ((p2 + ((p0 + q0 + 1) >> 1) - (p1 << 1)) >> 1, -iTc0, iTc0);
          iTc++;
        }
        if (bDetaQ2Q0) {
          pPix[iStrideX] = q1 + WELS_CLIP3 ((q2 + ((p0 + q0 + 1) >> 1) - (q1 << 1)) >> 1, -iTc0, iTc0);
          iTc++;
        }
        int32_t iDeta = WELS_CLIP3 ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3, -iTc, iTc);
        pPix[-iStrideX] = WELS_CLIP1 (p0 + iDeta);     /* p0' */
        pPix[0]  = WELS_CLIP1 (q0 - iDeta);     /* q0' */
      }
    }
    pPix += iStrideY;
  }
}
void_t DeblockLumaEq4_c (uint8_t* pPix, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha, int32_t iBeta) {
  int32_t p0, p1, p2, q0, q1, q2;
  int32_t iDetaP0Q0;
  bool_t bDetaP1P0, bDetaQ1Q0;
  for (int32_t i = 0; i < 16; i++) {
    p0 = pPix[-iStrideX];
    p1 = pPix[-2 * iStrideX];
    p2 = pPix[-3 * iStrideX];
    q0 = pPix[0];
    q1 = pPix[iStrideX];
    q2 = pPix[2 * iStrideX];
    iDetaP0Q0 = WELS_ABS (p0 - q0);
    bDetaP1P0 = WELS_ABS (p1 - p0) < iBeta;
    bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;
    if ((iDetaP0Q0 < iAlpha) && bDetaP1P0 && bDetaQ1Q0) {
      if (iDetaP0Q0 < ((iAlpha >> 2) + 2)) {
        bool_t bDetaP2P0 = WELS_ABS (p2 - p0) < iBeta;
        bool_t bDetaQ2Q0 =  WELS_ABS (q2 - q0) < iBeta;
        if (bDetaP2P0) {
          const int32_t p3 = pPix[-4 * iStrideX];
          pPix[-iStrideX] = (p2 + (p1 << 1) + (p0 << 1) + (q0 << 1) + q1 + 4) >> 3;	   //p0
          pPix[-2 * iStrideX] = (p2 + p1 + p0 + q0 + 2) >> 2;	 //p1
          pPix[-3 * iStrideX] = ((p3 << 1) + p2 + (p2 << 1) + p1 + p0 + q0 + 4) >> 3;//p2
        } else {
          pPix[-1 * iStrideX] = ((p1 << 1) + p0 + q1 + 2) >> 2;	//p0
        }
        if (bDetaQ2Q0) {
          const int32_t q3 = pPix[3 * iStrideX];
          pPix[0] = (p1 + (p0 << 1) + (q0 << 1) + (q1 << 1) + q2 + 4) >> 3;   //q0
          pPix[iStrideX] = (p0 + q0 + q1 + q2 + 2) >> 2;   //q1
          pPix[2 * iStrideX] = ((q3 << 1) + q2 + (q2 << 1) + q1 + q0 + p0 + 4) >> 3;//q2
        } else {
          pPix[0] = ((q1 << 1) + q0 + p1 + 2) >> 2;   //q0
        }
      } else {
        pPix[-iStrideX] = ((p1 << 1) + p0 + q1 + 2) >> 2;   //p0
        pPix[ 0] = ((q1 << 1) + q0 + p1 + 2) >> 2;   //q0
      }
    }
    pPix += iStrideY;
  }
}
void_t DeblockLumaLt4V_c (uint8_t* pPix, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* tc) {
  DeblockLumaLt4_c (pPix, iStride, 1, iAlpha, iBeta, tc);
}
void_t DeblockLumaLt4H_c (uint8_t* pPix, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* tc) {
  DeblockLumaLt4_c (pPix, 1, iStride, iAlpha, iBeta, tc);
}
void_t DeblockLumaEq4V_c (uint8_t* pPix, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
  DeblockLumaEq4_c (pPix, iStride, 1, iAlpha, iBeta);
}
void_t DeblockLumaEq4H_c (uint8_t* pPix, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
  DeblockLumaEq4_c (pPix, 1, iStride, iAlpha, iBeta);
}
void_t DeblockChromaLt4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha,
                           int32_t iBeta, int8_t* pTc) {
  int32_t p0, p1, q0, q1, iDeta;
  bool_t bDetaP0Q0, bDetaP1P0, bDetaQ1Q0;

  for (int32_t i = 0; i < 8; i++) {
    int32_t iTc0 = pTc[i >> 1];
    if (iTc0 > 0) {
      p0 = pPixCb[-iStrideX];
      p1 = pPixCb[-2 * iStrideX];
      q0 = pPixCb[0];
      q1 = pPixCb[iStrideX];

      bDetaP0Q0 =  WELS_ABS (p0 - q0) < iAlpha;
      bDetaP1P0 =  WELS_ABS (p1 - p0) < iBeta;
      bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;
      if (bDetaP0Q0 && bDetaP1P0 &&	bDetaQ1Q0) {
        iDeta = WELS_CLIP3 ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3, -iTc0, iTc0);
        pPixCb[-iStrideX] = WELS_CLIP1 (p0 + iDeta);     /* p0' */
        pPixCb[0]  = WELS_CLIP1 (q0 - iDeta);     /* q0' */
      }


      p0 = pPixCr[-iStrideX];
      p1 = pPixCr[-2 * iStrideX];
      q0 = pPixCr[0];
      q1 = pPixCr[iStrideX];

      bDetaP0Q0 =  WELS_ABS (p0 - q0) < iAlpha;
      bDetaP1P0 =  WELS_ABS (p1 - p0) < iBeta;
      bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;

      if (bDetaP0Q0 && bDetaP1P0 &&	bDetaQ1Q0) {
        iDeta = WELS_CLIP3 ((((q0 - p0) << 2) + (p1 - q1) + 4) >> 3, -iTc0, iTc0);
        pPixCr[-iStrideX] = WELS_CLIP1 (p0 + iDeta);     /* p0' */
        pPixCr[0]  = WELS_CLIP1 (q0 - iDeta);     /* q0' */
      }
    }
    pPixCb += iStrideY;
    pPixCr += iStrideY;
  }
}
void_t DeblockChromaEq4_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStrideX, int32_t iStrideY, int32_t iAlpha,
                           int32_t iBeta) {
  int32_t d = 0;
  int32_t p0, p1, q0, q1;
  bool_t bDetaP0Q0, bDetaP1P0, bDetaQ1Q0;
  for (int32_t i = 0; i < 8; i++) {
    //cb
    p0 = pPixCb[-iStrideX];
    p1 = pPixCb[-2 * iStrideX];
    q0 = pPixCb[0];
    q1 = pPixCb[iStrideX];
    bDetaP0Q0 = WELS_ABS (p0 - q0) < iAlpha;
    bDetaP1P0 = WELS_ABS (p1 - p0) < iBeta;
    bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;
    if (bDetaP0Q0 && bDetaP1P0 && bDetaQ1Q0) {
      pPixCb[-iStrideX] = ((p1 << 1) + p0 + q1 + 2) >> 2;     /* p0' */
      pPixCb[0]  = ((q1 << 1) + q0 + p1 + 2) >> 2;     /* q0' */
    }

    //cr
    p0 = pPixCr[-iStrideX];
    p1 = pPixCr[-2 * iStrideX];
    q0 = pPixCr[0];
    q1 = pPixCr[iStrideX];
    bDetaP0Q0 = WELS_ABS (p0 - q0) < iAlpha;
    bDetaP1P0 = WELS_ABS (p1 - p0) < iBeta;
    bDetaQ1Q0 = WELS_ABS (q1 - q0) < iBeta;
    if (bDetaP0Q0 && bDetaP1P0 && bDetaQ1Q0) {
      pPixCr[-iStrideX] = ((p1 << 1) + p0 + q1 + 2) >> 2;     /* p0' */
      pPixCr[0]  = ((q1 << 1) + q0 + p1 + 2) >> 2;     /* q0' */
    }
    pPixCr += iStrideY;
    pPixCb += iStrideY;
  }
}
void_t DeblockChromaLt4V_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                            int8_t* tc) {
  DeblockChromaLt4_c (pPixCb, pPixCr, iStride, 1, iAlpha, iBeta, tc);
}
void_t DeblockChromaLt4H_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                            int8_t* tc) {
  DeblockChromaLt4_c (pPixCb, pPixCr, 1, iStride, iAlpha, iBeta, tc);
}
void_t DeblockChromaEq4V_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
  DeblockChromaEq4_c (pPixCb, pPixCr, iStride, 1, iAlpha, iBeta);
}
void_t DeblockChromaEq4H_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
  DeblockChromaEq4_c (pPixCb, pPixCr, 1, iStride, iAlpha, iBeta);
}

#ifdef X86_ASM
extern "C" {
  void DeblockLumaLt4H_sse2 (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc) {
    FORCE_STACK_ALIGN_1D (uint8_t,  uiBuf,   16 * 8, 16);

    DeblockLumaTransposeH2V_sse2 (pPixY - 4, iStride, &uiBuf[0]);
    DeblockLumaLt4V_sse2 (&uiBuf[4 * 16], 16, iAlpha, iBeta, pTc);
    DeblockLumaTransposeV2H_sse2 (pPixY - 4, iStride, &uiBuf[0]);
  }

  void DeblockLumaEq4H_sse2 (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta) {
    FORCE_STACK_ALIGN_1D (uint8_t,  uiBuf,   16 * 8, 16);

    DeblockLumaTransposeH2V_sse2 (pPixY - 4, iStride, &uiBuf[0]);
    DeblockLumaEq4V_sse2 (&uiBuf[4 * 16], 16, iAlpha, iBeta);
    DeblockLumaTransposeV2H_sse2 (pPixY - 4, iStride, &uiBuf[0]);
  }

}

#endif

