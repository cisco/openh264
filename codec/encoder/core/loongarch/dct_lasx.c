/*!
 * \copy
 *     Copyright (c)  2022, Cisco Systems
 *     All rights reserved.
 *
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *        * Redistributions of source code must retain the above copyright
 *          notice, this list of conditions and the following disclaimer.
 *
 *        * Redistributions in binary form must reproduce the above copyright
 *          notice, this list of conditions and the following disclaimer in
 *          the documentation and/or other materials provided with the
 *          distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *     FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *     COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *     INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *     BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *     LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *     ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * \file    dct_lasx.c
 *
 * \brief   Loongson optimization
 *
 * \date    15/02/2022 Created
 *
 *************************************************************************************
 */
#include <stdint.h>
#include "loongson_intrinsics.h"

#define CALC_TEMPS_AND_PDCT(src0, src1, src2, src3, \
                            dst0, dst1, dst2, dst3) \
do {                                                \
  __m256i tms0, tms1, tms2, tms3;                   \
  tms0 = __lasx_xvadd_h(src0, src3);                \
  tms1 = __lasx_xvadd_h(src1, src2);                \
  tms2 = __lasx_xvsub_h(src1, src2);                \
  tms3 = __lasx_xvsub_h(src0, src3);                \
  dst0 = __lasx_xvadd_h(tms0, tms1);                \
  dst1 = __lasx_xvslli_h(tms3, 1);                  \
  dst1 = __lasx_xvadd_h(dst1, tms2);                \
  dst2 = __lasx_xvsub_h(tms0, tms1);                \
  dst3 = __lasx_xvslli_h(tms2, 1);                  \
  dst3 = __lasx_xvsub_h(tms3, dst3);                \
}while(0)

/****************************************************************************
 * DCT functions
 ****************************************************************************/
void WelsDctT4_lasx (int16_t* pDct, uint8_t* pPixel1,
                     int32_t iStride1, uint8_t* pPixel2,
                     int32_t iStride2) {
  int32_t iStride0 = 0;
  int32_t iStride1_x2 = iStride1 << 1;
  int32_t iStride1_x3 = iStride1_x2 + iStride1;
  int32_t iStride2_x2 = iStride2 << 1;
  int32_t iStride2_x3 = iStride2_x2 + iStride2;

  __m256i src0, src1, src2, src3, src4, src5, src6, src7;
  __m256i dst0, dst1, dst2, dst3;

  DUP4_ARG2(__lasx_xvldx,
            pPixel1, iStride0,
            pPixel1, iStride1,
            pPixel1, iStride1_x2,
            pPixel1, iStride1_x3,
            src0, src1, src2, src3);
  DUP4_ARG2(__lasx_xvldx,
            pPixel2, iStride0,
            pPixel2, iStride2,
            pPixel2, iStride2_x2,
            pPixel2, iStride2_x3,
            src4, src5, src6, src7);
  DUP4_ARG2(__lasx_xvilvl_b,
            src0, src4,
            src1, src5,
            src2, src6,
            src3, src7,
            src0, src1, src2, src3);
  DUP4_ARG2(__lasx_xvhsubw_hu_bu,
            src0, src0,
            src1, src1,
            src2, src2,
            src3, src3,
            src0, src1, src2, src3);
  LASX_TRANSPOSE4x4_H(src0, src1, src2, src3,
                      src0, src1, src2, src3);
  CALC_TEMPS_AND_PDCT(src0, src1, src2, src3,
                      dst0, dst1, dst2, dst3);
  LASX_TRANSPOSE4x4_H(dst0, dst1, dst2, dst3,
                      src0, src1, src2, src3);
  CALC_TEMPS_AND_PDCT(src0, src1, src2, src3,
                      dst0, dst1, dst2, dst3);
  dst0 = __lasx_xvpackev_d(dst1, dst0);
  dst2 = __lasx_xvpackev_d(dst3, dst2);
  dst0 = __lasx_xvpermi_q(dst2, dst0, 0x20);
  __lasx_xvst(dst0, pDct, 0);
}

void WelsDctFourT4_lasx (int16_t* pDct, uint8_t* pPixel1,
                         int32_t iStride1, uint8_t* pPixel2,
                         int32_t iStride2) {
  int32_t stride_1 = iStride1 << 2;
  int32_t stride_2 = iStride2 << 2;
  int32_t iStride0 = 0;
  int32_t iStride1_x2 = iStride1 << 1;
  int32_t iStride1_x3 = iStride1_x2 + iStride1;
  int32_t iStride2_x2 = iStride2 << 1;
  int32_t iStride2_x3 = iStride2_x2 + iStride2;
  uint8_t *psrc10 = pPixel1, *psrc11 = pPixel2;
  uint8_t *psrc20 = pPixel1 + stride_1, *psrc21 = pPixel2 + stride_2;

  __m256i src0, src1, src2, src3, src4, src5, src6, src7,
          src8, src9, src10, src11, src12, src13, src14 ,src15;
  __m256i tmp0, tmp1, tmp2, tmp3, dst0, dst1, dst2, dst3, dst4,
          dst5, dst6, dst7;

  DUP4_ARG2(__lasx_xvldx,
            psrc10, iStride0,
            psrc10, iStride1,
            psrc10, iStride1_x2,
            psrc10, iStride1_x3,
            src0, src1, src2, src3);
  DUP4_ARG2(__lasx_xvldx,
            psrc11, iStride0,
            psrc11, iStride2,
            psrc11, iStride2_x2,
            psrc11, iStride2_x3,
            src4, src5, src6, src7);
  DUP4_ARG2(__lasx_xvldx,
            psrc20, iStride0,
            psrc20, iStride1,
            psrc20, iStride1_x2,
            psrc20, iStride1_x3,
            src8, src9, src10, src11);
  DUP4_ARG2(__lasx_xvldx,
            psrc21, iStride0,
            psrc21, iStride2,
            psrc21, iStride2_x2,
            psrc21, iStride2_x3,
            src12, src13, src14, src15);
  DUP4_ARG2(__lasx_xvilvl_b,
            src0, src4,
            src1, src5,
            src2, src6,
            src3, src7,
            src0, src1, src2, src3);
  DUP4_ARG2(__lasx_xvilvl_b,
            src8, src12,
            src9, src13,
            src10, src14,
            src11, src15,
            src8, src9, src10, src11);
  DUP4_ARG2(__lasx_xvhsubw_hu_bu,
            src0, src0,
            src1, src1,
            src2, src2,
            src3, src3,
            src0, src1, src2, src3);
  DUP4_ARG2(__lasx_xvhsubw_hu_bu,
            src8, src8,
            src9, src9,
            src10, src10,
            src11, src11,
            src8, src9, src10 ,src11);
  LASX_TRANSPOSE8x8_H(src0, src1, src2, src3, src8, src9, src10, src11,
                      src0, src1, src2, src3, src8, src9, src10, src11);
  DUP4_ARG3(__lasx_xvpermi_q,
            src8, src0, 0x20,
            src9, src1, 0x20,
            src10,src2, 0x20,
            src11,src3, 0x20,
            src0, src1, src2, src3);
  CALC_TEMPS_AND_PDCT(src0, src1, src2, src3,
                      dst0, dst1, dst2, dst3);
  DUP4_ARG3(__lasx_xvpermi_q,
            dst0, dst0, 0x31,
            dst1, dst1, 0x31,
            dst2, dst2, 0x31,
            dst3, dst3, 0x31,
            dst4, dst5, dst6, dst7);
  LASX_TRANSPOSE8x8_H(dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7,
                      dst0, dst1, dst2, dst3, dst4, dst5, dst6, dst7);
  DUP4_ARG3(__lasx_xvpermi_q,
            dst4, dst0, 0x20,
            dst5, dst1, 0x20,
            dst6, dst2, 0x20,
            dst7, dst3, 0x20,
            dst0, dst1, dst2, dst3);
  CALC_TEMPS_AND_PDCT(dst0, dst1, dst2, dst3,
                      dst0, dst1, dst2, dst3);
  DUP2_ARG2(__lasx_xvpackev_d,
            dst1, dst0,
            dst3, dst2,
            tmp0, tmp1);
  DUP2_ARG2(__lasx_xvpackod_d,
            dst1, dst0,
            dst3, dst2,
            tmp2, tmp3);
  DUP2_ARG3(__lasx_xvpermi_q,
            tmp1, tmp0, 0x20,
            tmp3, tmp2, 0x20,
            dst0, dst1);
  DUP2_ARG3(__lasx_xvpermi_q,
            tmp1, tmp0, 0x31,
            tmp3, tmp2, 0x31,
            dst2, dst3);
  __lasx_xvst(dst0, pDct, 0);
  __lasx_xvst(dst1, pDct, 32);
  __lasx_xvst(dst2, pDct, 64);
  __lasx_xvst(dst3, pDct, 96);
}

/****************************************************************************
 * IDCT functions, final output = prediction(CS) + IDCT(scaled_coeff)
 ****************************************************************************/
void WelsIDctT4Rec_lasx (uint8_t* pRec, int32_t iStride,
                         uint8_t* pPred, int32_t iPredStride,
                         int16_t* pDct) {
  int32_t iDstStride_x2 = iStride << 1;
  int32_t iDstStride_x3 = iStride + iDstStride_x2;
  int32_t iPredStride_x2 = iPredStride << 1;
  int32_t iPredStride_x3 = iPredStride + iPredStride_x2;

  __m256i src0, src1, src2, src3, src4, src5, src6, src7,
          tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7,
          dst0, dst1, dst2, dst3;

  __m256i zero = __lasx_xvldi(0);
  src0 = __lasx_xvld(pDct, 0);
  src4 = __lasx_xvld(pPred, 0);
  src5 = __lasx_xvldx(pPred, iPredStride);
  src6 = __lasx_xvldx(pPred, iPredStride_x2);
  src7 = __lasx_xvldx(pPred, iPredStride_x3);

  src1 = __lasx_xvpickve_d(src0, 1);
  src2 = __lasx_xvpickve_d(src0, 2);
  src3 = __lasx_xvpickve_d(src0, 3);

  LASX_TRANSPOSE4x4_H(src0, src1, src2, src3,
                      tmp0, tmp1, tmp2, tmp3);
  DUP4_ARG1(__lasx_vext2xv_w_h,
            tmp0, tmp1, tmp2, tmp3,
            src0, src1, src2, src3);
  //horizon
  tmp0 = __lasx_xvadd_w(src0, src2); //0+2 sumu
  tmp1 = __lasx_xvsrai_w(src3, 1);
  tmp1 = __lasx_xvadd_w(src1, tmp1); //1+3 sumd
  tmp2 = __lasx_xvsub_w(src0, src2); //0-2 delu
  tmp3 = __lasx_xvsrai_w(src1, 1);
  tmp3 = __lasx_xvsub_w(tmp3, src3); //1-3 deld

  src0 = __lasx_xvadd_w(tmp0 ,tmp1); //0 4 8  12
  src1 = __lasx_xvadd_w(tmp2, tmp3); //1 5 9  13
  src2 = __lasx_xvsub_w(tmp2, tmp3); //2 6 10 14
  src3 = __lasx_xvsub_w(tmp0, tmp1); //3 7 11 15
  DUP4_ARG2(__lasx_xvpickev_h,
            zero, src0,
            zero, src1,
            zero, src2,
            zero, src3,
            src0, src1, src2, src3);
  //vertical
  LASX_TRANSPOSE4x4_H(src0, src1, src2, src3,
                      tmp0, tmp1, tmp2, tmp3);
  DUP4_ARG1(__lasx_vext2xv_w_h,
            tmp0, tmp1, tmp2, tmp3,
            src0, src1, src2, src3);
  tmp0 = __lasx_xvadd_w(src0, src2); //suml
  tmp1 = __lasx_xvsrai_w(src3, 1);
  tmp1 = __lasx_xvadd_w(src1, tmp1); //sumr
  tmp2 = __lasx_xvsub_w(src0, src2); //dell
  tmp3 = __lasx_xvsrai_w(src1, 1);
  tmp3 = __lasx_xvsub_w(tmp3, src3); //delr

  dst0 = __lasx_xvadd_w(tmp0, tmp1);
  dst1 = __lasx_xvadd_w(tmp2, tmp3);
  dst2 = __lasx_xvsub_w(tmp2, tmp3);
  dst3 = __lasx_xvsub_w(tmp0, tmp1);
  DUP4_ARG2(__lasx_xvsrari_w,
            dst0, 6,
            dst1, 6,
            dst2, 6,
            dst3, 6,
            dst0, dst1, dst2, dst3);
  DUP4_ARG1(__lasx_vext2xv_wu_bu,
            src4, src5, src6, src7,
            tmp4, tmp5, tmp6, tmp7);
  DUP4_ARG2(__lasx_xvadd_w,
            tmp4, dst0,
            tmp5, dst1,
            tmp6, dst2,
            tmp7, dst3,
            dst0, dst1, dst2, dst3);
  DUP4_ARG1(__lasx_xvclip255_w,
            dst0, dst1, dst2, dst3,
            dst0, dst1, dst2, dst3);
  DUP2_ARG2(__lasx_xvpickev_h,
            dst1, dst0,
            dst3, dst2,
            dst0, dst2);
  dst0 = __lasx_xvpickev_b(dst2, dst0);
  __lasx_xvstelm_w(dst0, pRec, 0, 0);
  __lasx_xvstelm_w(dst0, pRec + iStride, 0, 1);
  __lasx_xvstelm_w(dst0, pRec + iDstStride_x2, 0, 2);
  __lasx_xvstelm_w(dst0, pRec + iDstStride_x3, 0, 3);
}

void WelsIDctFourT4Rec_lasx (uint8_t* pRec, int32_t iStride,
                             uint8_t* pPred, int32_t iPredStride,
                             int16_t* pDct) {
  int32_t iDstStridex4  = iStride << 2;
  int32_t iPredStridex4 = iPredStride << 2;
  WelsIDctT4Rec_lasx (pRec,                    iStride, pPred,                     iPredStride, pDct);
  WelsIDctT4Rec_lasx (&pRec[4],                iStride, &pPred[4],                 iPredStride, pDct + 16);
  WelsIDctT4Rec_lasx (&pRec[iDstStridex4    ], iStride, &pPred[iPredStridex4  ],   iPredStride, pDct + 32);
  WelsIDctT4Rec_lasx (&pRec[iDstStridex4 + 4], iStride, &pPred[iPredStridex4 + 4], iPredStride, pDct + 48);
}
