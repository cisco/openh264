/*!
 **********************************************************************************
 * Copyright (c) 2022 Loongson Technology Corporation Limited
 * Contributed by Lu Wang <wanglu@loongson.cn>
 *
 * \copy
 *     Copyright (c)  2013, Cisco Systems
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
 * \file    deblock_lsx.c
 *
 * \brief   Loongson optimization
 *
 * \date    22/2/2022 Created
 *
 **********************************************************************************
 */

#include <stdint.h>
#include "loongson_intrinsics.h"

void DeblockLumaLt4V_lsx (uint8_t* pPix, int32_t iStrideX,
                          int32_t iAlpha, int32_t iBeta, int8_t* pTc) {
  __m128i p0, p1, p2, q0, q1, q2;
  __m128i p0_l, p1_l, p2_l, q0_l, q1_l, q2_l;
  __m128i p0_h, p1_h, p2_h, q0_h, q1_h, q2_h;
  __m128i t0, t1, t2, t3, t;
  __m128i t0_l, t0_h, t1_l, t1_h, t2_l, t2_h;
  __m128i iTc, iTc0, negiTc, negiTc0, f, flags;
  __m128i iTc_l, iTc_h, negiTc_l, negiTc_h;
  __m128i iTc0_l, iTc0_h, negiTc0_l, negiTc0_h;
  __m128i bDetaP0Q0, bDetaP1P0, bDetaQ1Q0, bDetaP2P0, bDetaQ2Q0;

  __m128i zero = __lsx_vreplgr2vr_b(0);
  __m128i not_255 = __lsx_vreplgr2vr_h(-256);
  __m128i alpha = __lsx_vreplgr2vr_b(iAlpha);
  __m128i beta = __lsx_vreplgr2vr_b(iBeta);
  __m128i shuf = {0x0101010100000000, 0x0303030302020202};
  int32_t iStrideX_x0 = 0;
  int32_t iStrideX_x2 = iStrideX << 1;
  int32_t iStrideX_x3 = iStrideX_x2 + iStrideX;

  iTc0 = __lsx_vldx(pTc, 0);
  iTc0 = __lsx_vshuf_b(iTc0, iTc0, shuf);
  negiTc0 = __lsx_vneg_b(iTc0);
  iTc = iTc0;

  DUP4_ARG2(__lsx_vldx, pPix, -iStrideX, pPix, -iStrideX_x2, pPix, -iStrideX_x3,
            pPix, iStrideX_x0, p0, p1, p2, q0);
  DUP2_ARG2(__lsx_vldx, pPix, iStrideX, pPix, iStrideX_x2, q1, q2);

  DUP4_ARG2(__lsx_vabsd_bu, p0, q0, p1, p0, q1, q0, p2, p0,
            bDetaP0Q0, bDetaP1P0, bDetaQ1Q0, bDetaP2P0);
  bDetaQ2Q0 = __lsx_vabsd_bu(q2, q0);
  DUP4_ARG2(__lsx_vslt_bu, bDetaP0Q0, alpha, bDetaP1P0, beta, bDetaQ1Q0, beta,
            bDetaP2P0, beta, bDetaP0Q0, bDetaP1P0, bDetaQ1Q0, bDetaP2P0);
  bDetaQ2Q0 = __lsx_vslt_bu(bDetaQ2Q0, beta);

  DUP4_ARG2(__lsx_vilvl_b, zero, p0, zero, p1, zero, p2, zero, q0,
            p0_l, p1_l, p2_l, q0_l);
  DUP2_ARG2(__lsx_vilvl_b, zero, q1, zero, q2, q1_l, q2_l);
  DUP4_ARG2(__lsx_vilvh_b, zero, p0, zero, p1, zero, p2, zero, q0,
            p0_h, p1_h, p2_h, q0_h);
  DUP2_ARG2(__lsx_vilvh_b, zero, q1, zero, q2, q1_h, q2_h);

  DUP2_ARG2(__lsx_vand_v, bDetaP0Q0, bDetaP1P0, f, bDetaQ1Q0, f, f);
  flags = __lsx_vsle_b(zero, iTc0);
  DUP2_ARG2(__lsx_vand_v, f, flags, flags, bDetaP2P0, flags, flags);
  flags = __lsx_vandi_b(flags, 1);
  iTc = __lsx_vadd_b(iTc, flags);
  flags = __lsx_vsle_b(zero, iTc0);
  DUP2_ARG2(__lsx_vand_v, f, flags, flags, bDetaQ2Q0, flags, flags);
  flags = __lsx_vandi_b(flags, 1);
  iTc = __lsx_vadd_b(iTc, flags);
  negiTc = __lsx_vneg_b(iTc);

  flags = __lsx_vslt_b(iTc0, zero);
  iTc0_l = __lsx_vilvl_b(flags, iTc0);
  iTc0_h = __lsx_vilvh_b(flags, iTc0);
  flags = __lsx_vslt_b(negiTc0, zero);
  negiTc0_l = __lsx_vilvl_b(flags, negiTc0);
  negiTc0_h = __lsx_vilvh_b(flags, negiTc0);

  flags = __lsx_vslt_b(iTc, zero);
  iTc_l = __lsx_vilvl_b(flags, iTc);
  iTc_h = __lsx_vilvh_b(flags, iTc);
  flags = __lsx_vslt_b(negiTc, zero);
  negiTc_l = __lsx_vilvl_b(flags, negiTc);
  negiTc_h = __lsx_vilvh_b(flags, negiTc);

  t0_l = __lsx_vadd_h(p0_l, q0_l);
  t0_l = __lsx_vaddi_hu(t0_l, 1);
  t0_l = __lsx_vsrai_h(t0_l, 1);
  t0_l = __lsx_vadd_h(p2_l, t0_l);
  t = __lsx_vslli_h(p1_l, 1);
  t0_l = __lsx_vsub_h(t0_l, t);
  t0_l = __lsx_vsrai_h(t0_l, 1);
  t0_l = __lsx_vmin_h(iTc0_l, t0_l);
  t0_l = __lsx_vmax_h(negiTc0_l, t0_l);
  t0_l = __lsx_vadd_h(p1_l, t0_l);

  t1_l = __lsx_vadd_h(p0_l, q0_l);
  t1_l = __lsx_vaddi_hu(t1_l, 1);
  t1_l = __lsx_vsrai_h(t1_l, 1);
  t1_l = __lsx_vadd_h(q2_l, t1_l);
  t = __lsx_vslli_h(q1_l, 1);
  t1_l = __lsx_vsub_h(t1_l, t);
  t1_l = __lsx_vsrai_h(t1_l, 1);
  t1_l = __lsx_vmin_h(iTc0_l, t1_l);
  t1_l = __lsx_vmax_h(negiTc0_l, t1_l);
  t1_l = __lsx_vadd_h(q1_l, t1_l);

  t0_h = __lsx_vadd_h(p0_h, q0_h);
  t0_h = __lsx_vaddi_hu(t0_h, 1);
  t0_h = __lsx_vsrai_h(t0_h, 1);
  t0_h = __lsx_vadd_h(p2_h, t0_h);
  t = __lsx_vslli_h(p1_h, 1);
  t0_h = __lsx_vsub_h(t0_h, t);
  t0_h = __lsx_vsrai_h(t0_h, 1);
  t0_h = __lsx_vmin_h(iTc0_h, t0_h);
  t0_h = __lsx_vmax_h(negiTc0_h, t0_h);
  t0_h = __lsx_vadd_h(p1_h, t0_h);

  t1_h = __lsx_vadd_h(p0_h, q0_h);
  t1_h = __lsx_vaddi_hu(t1_h, 1);
  t1_h = __lsx_vsrai_h(t1_h, 1);
  t1_h = __lsx_vadd_h(q2_h, t1_h);
  t = __lsx_vslli_h(q1_h, 1);
  t1_h = __lsx_vsub_h(t1_h, t);
  t1_h = __lsx_vsrai_h(t1_h, 1);
  t1_h = __lsx_vmin_h(iTc0_h, t1_h);
  t1_h = __lsx_vmax_h(negiTc0_h, t1_h);
  t1_h = __lsx_vadd_h(q1_h, t1_h);

  t2_l = __lsx_vsub_h(q0_l, p0_l);
  t2_l = __lsx_vslli_h(t2_l, 2);
  t2_l = __lsx_vadd_h(t2_l, p1_l);
  t2_l = __lsx_vsub_h(t2_l, q1_l);
  t2_l = __lsx_vaddi_hu(t2_l, 4);
  t2_l = __lsx_vsrai_h(t2_l, 3);
  t2_l = __lsx_vmin_h(iTc_l, t2_l);
  t2_l = __lsx_vmax_h(negiTc_l, t2_l);

  t2_h = __lsx_vsub_h(q0_h, p0_h);
  t2_h = __lsx_vslli_h(t2_h, 2);
  t2_h = __lsx_vadd_h(t2_h, p1_h);
  t2_h = __lsx_vsub_h(t2_h, q1_h);
  t2_h = __lsx_vaddi_hu(t2_h, 4);
  t2_h = __lsx_vsrai_h(t2_h, 3);
  t2_h = __lsx_vmin_h(iTc_h, t2_h);
  t2_h = __lsx_vmax_h(negiTc_h, t2_h);

  p0_l = __lsx_vadd_h(p0_l, t2_l);
  p1_l = __lsx_vand_v(p0_l, not_255);
  p2_l = __lsx_vsle_h(zero, p0_l);
  flags = __lsx_vseq_h(p1_l, zero);
  p0_l = __lsx_vand_v(p0_l, flags);
  flags = __lsx_vnor_v(flags,flags);
  p2_l = __lsx_vand_v(p2_l, flags);
  p0_l = __lsx_vadd_h(p0_l, p2_l);

  q0_l = __lsx_vsub_h(q0_l, t2_l);
  q1_l = __lsx_vand_v(q0_l, not_255);
  q2_l = __lsx_vsle_h(zero, q0_l);
  flags = __lsx_vseq_h(q1_l, zero);
  q0_l = __lsx_vand_v(q0_l, flags);
  flags = __lsx_vnor_v(flags, flags);
  q2_l = __lsx_vand_v(q2_l, flags);
  q0_l = __lsx_vadd_h(q0_l, q2_l);

  p0_h = __lsx_vadd_h(p0_h, t2_h);
  p1_h = __lsx_vand_v(p0_h, not_255);
  p2_h = __lsx_vsle_h(zero, p0_h);
  flags = __lsx_vseq_h(p1_h, zero);
  p0_h = __lsx_vand_v(p0_h, flags);
  flags = __lsx_vnor_v(flags, flags);
  p2_h = __lsx_vand_v(p2_h, flags);
  p0_h = __lsx_vadd_h(p0_h, p2_h);

  q0_h = __lsx_vsub_h(q0_h, t2_h);
  q1_h = __lsx_vand_v(q0_h, not_255);
  q2_h = __lsx_vsle_h(zero, q0_h);
  flags = __lsx_vseq_h(q1_h, zero);
  q0_h = __lsx_vand_v(q0_h, flags);
  flags = __lsx_vnor_v(flags, flags);
  q2_h = __lsx_vand_v(q2_h, flags);
  q0_h = __lsx_vadd_h(q0_h, q2_h);

  DUP4_ARG2(__lsx_vpickev_b, t0_h, t0_l, t1_h, t1_l,
            p0_h, p0_l, q0_h, q0_l, t0, t1, t2, t3);

  flags = __lsx_vsle_b(zero, iTc0);
  flags = __lsx_vand_v(flags, f);
  t2 = __lsx_vand_v(t2, flags);
  t = __lsx_vnor_v(flags,flags);
  p0 = __lsx_vand_v(p0, t);
  p0 = __lsx_vadd_b(t2, p0);
  t3 = __lsx_vand_v(t3, flags);
  t = __lsx_vnor_v(flags,flags);
  q0 = __lsx_vand_v(q0, t);
  q0 = __lsx_vadd_b(t3, q0);

  DUP2_ARG2(__lsx_vand_v, flags, bDetaP2P0, t0, t, t, t0);
  t = __lsx_vnor_v(t, t);
  p1 = __lsx_vand_v(p1, t);
  p1 = __lsx_vadd_b(t0, p1);
  DUP2_ARG2(__lsx_vand_v, flags, bDetaQ2Q0, t1, t, t, t1);
  t = __lsx_vnor_v(t, t);
  q1 = __lsx_vand_v(q1, t);
  q1 = __lsx_vadd_b(t1, q1);

  __lsx_vstx(p1, pPix, -iStrideX_x2);
  __lsx_vstx(p0, pPix, -iStrideX);
  __lsx_vstx(q0, pPix, iStrideX_x0);
  __lsx_vstx(q1, pPix, iStrideX);
}

void DeblockLumaLt4H_lsx (uint8_t* pPix, int32_t iStrideY,
                          int32_t iAlpha, int32_t iBeta, int8_t* pTc) {
  __m128i p0, p1, p2, q0, q1, q2;
  __m128i p0_l, p1_l, p2_l, q0_l, q1_l, q2_l;
  __m128i p0_h, p1_h, p2_h, q0_h, q1_h, q2_h;
  __m128i t0, t1, t2, t3, t;
  __m128i t0_l, t0_h, t1_l, t1_h, t2_l, t2_h;
  __m128i iTc, iTc0, negiTc, negiTc0, f, flags;
  __m128i iTc_l, iTc_h, negiTc_l, negiTc_h;
  __m128i iTc0_l, iTc0_h, negiTc0_l, negiTc0_h;
  __m128i bDetaP0Q0, bDetaP1P0, bDetaQ1Q0, bDetaP2P0, bDetaQ2Q0;

  __m128i zero = __lsx_vreplgr2vr_b(0);
  __m128i not_255 = __lsx_vreplgr2vr_h(-256);
  __m128i alpha = __lsx_vreplgr2vr_b(iAlpha);
  __m128i beta = __lsx_vreplgr2vr_b(iBeta);
  __m128i shuf = {0x0101010100000000, 0x0303030302020202};
  int32_t iStrideY_x0 = 0;
  int32_t iStrideY_x2 = iStrideY << 1;
  int32_t iStrideY_x3 = iStrideY_x2 + iStrideY;
  int32_t iStrideY_x4 = iStrideY << 2;

  iTc0 = __lsx_vldx(pTc, 0);
  iTc0 = __lsx_vshuf_b(iTc0, iTc0, shuf);
  negiTc0 = __lsx_vneg_b(iTc0);
  iTc = iTc0;

  pPix -= 3;
  DUP4_ARG2(__lsx_vldx, pPix, iStrideY_x0, pPix, iStrideY, pPix, iStrideY_x2,
            pPix, iStrideY_x3, p0_l, p1_l, p2_l, q0_l);
  pPix += iStrideY_x4;
  DUP4_ARG2(__lsx_vldx, pPix, iStrideY_x0, pPix, iStrideY, pPix, iStrideY_x2,
            pPix, iStrideY_x3, p0_h, p1_h, p2_h, q0_h);
  pPix += iStrideY_x4;
  DUP4_ARG2(__lsx_vldx, pPix, iStrideY_x0, pPix, iStrideY, pPix, iStrideY_x2,
            pPix, iStrideY_x3, q1_l, q2_l, t0_l, t1_l);
  pPix += iStrideY_x4;
  DUP4_ARG2(__lsx_vldx, pPix, iStrideY_x0, pPix, iStrideY, pPix, iStrideY_x2,
            pPix, iStrideY_x3, q1_h, q2_h, t0_h, t1_h);
  LSX_TRANSPOSE16x8_B(p0_l, p1_l, p2_l, q0_l, p0_h, p1_h, p2_h, q0_h, q1_l, q2_l,
                      t0_l, t1_l, q1_h, q2_h, t0_h, t1_h, p2, p1, p0, q0, q1, q2,
                      t, f);

  DUP4_ARG2(__lsx_vabsd_bu, p0, q0, p1, p0, q1, q0, p2, p0,
            bDetaP0Q0, bDetaP1P0, bDetaQ1Q0, bDetaP2P0);
  bDetaQ2Q0 = __lsx_vabsd_bu(q2, q0);
  DUP4_ARG2(__lsx_vslt_bu, bDetaP0Q0, alpha, bDetaP1P0, beta, bDetaQ1Q0, beta,
            bDetaP2P0, beta, bDetaP0Q0, bDetaP1P0, bDetaQ1Q0, bDetaP2P0);
  bDetaQ2Q0 = __lsx_vslt_bu(bDetaQ2Q0, beta);

  DUP4_ARG2(__lsx_vilvl_b, zero, p0, zero, p1, zero, p2, zero, q0,
            p0_l, p1_l, p2_l, q0_l);
  DUP2_ARG2(__lsx_vilvl_b, zero, q1, zero, q2, q1_l, q2_l);
  DUP4_ARG2(__lsx_vilvh_b, zero, p0, zero, p1, zero, p2, zero, q0,
            p0_h, p1_h, p2_h, q0_h);
  DUP2_ARG2(__lsx_vilvh_b, zero, q1, zero, q2, q1_h, q2_h);

  DUP2_ARG2(__lsx_vand_v, bDetaP0Q0, bDetaP1P0, f, bDetaQ1Q0, f, f);
  flags = __lsx_vsle_b(zero, iTc0);
  DUP2_ARG2(__lsx_vand_v, f, flags, flags, bDetaP2P0, flags, flags);
  flags = __lsx_vandi_b(flags, 1);
  iTc = __lsx_vadd_b(iTc, flags);
  flags = __lsx_vsle_b(zero, iTc0);
  DUP2_ARG2(__lsx_vand_v, f, flags, flags, bDetaQ2Q0, flags, flags);
  flags = __lsx_vandi_b(flags, 1);
  iTc = __lsx_vadd_b(iTc, flags);
  negiTc = __lsx_vneg_b(iTc);

  flags = __lsx_vslt_b(iTc0, zero);
  iTc0_l = __lsx_vilvl_b(flags, iTc0);
  iTc0_h = __lsx_vilvh_b(flags, iTc0);
  flags = __lsx_vslt_b(negiTc0, zero);
  negiTc0_l = __lsx_vilvl_b(flags, negiTc0);
  negiTc0_h = __lsx_vilvh_b(flags, negiTc0);

  flags = __lsx_vslt_b(iTc, zero);
  iTc_l = __lsx_vilvl_b(flags, iTc);
  iTc_h = __lsx_vilvh_b(flags, iTc);
  flags = __lsx_vslt_b(negiTc, zero);
  negiTc_l = __lsx_vilvl_b(flags, negiTc);
  negiTc_h = __lsx_vilvh_b(flags, negiTc);

  t0_l = __lsx_vadd_h(p0_l, q0_l);
  t0_l = __lsx_vaddi_hu(t0_l, 1);
  t0_l = __lsx_vsrai_h(t0_l, 1);
  t0_l = __lsx_vadd_h(p2_l, t0_l);
  t = __lsx_vslli_h(p1_l, 1);
  t0_l = __lsx_vsub_h(t0_l, t);
  t0_l = __lsx_vsrai_h(t0_l, 1);
  t0_l = __lsx_vmin_h(iTc0_l, t0_l);
  t0_l = __lsx_vmax_h(negiTc0_l, t0_l);
  t0_l = __lsx_vadd_h(p1_l, t0_l);

  t1_l = __lsx_vadd_h(p0_l, q0_l);
  t1_l = __lsx_vaddi_hu(t1_l, 1);
  t1_l = __lsx_vsrai_h(t1_l, 1);
  t1_l = __lsx_vadd_h(q2_l, t1_l);
  t = __lsx_vslli_h(q1_l, 1);
  t1_l = __lsx_vsub_h(t1_l, t);
  t1_l = __lsx_vsrai_h(t1_l, 1);
  t1_l = __lsx_vmin_h(iTc0_l, t1_l);
  t1_l = __lsx_vmax_h(negiTc0_l, t1_l);
  t1_l = __lsx_vadd_h(q1_l, t1_l);

  t0_h = __lsx_vadd_h(p0_h, q0_h);
  t0_h = __lsx_vaddi_hu(t0_h, 1);
  t0_h = __lsx_vsrai_h(t0_h, 1);
  t0_h = __lsx_vadd_h(p2_h, t0_h);
  t = __lsx_vslli_h(p1_h, 1);
  t0_h = __lsx_vsub_h(t0_h, t);
  t0_h = __lsx_vsrai_h(t0_h, 1);
  t0_h = __lsx_vmin_h(iTc0_h, t0_h);
  t0_h = __lsx_vmax_h(negiTc0_h, t0_h);
  t0_h = __lsx_vadd_h(p1_h, t0_h);

  t1_h = __lsx_vadd_h(p0_h, q0_h);
  t1_h = __lsx_vaddi_hu(t1_h, 1);
  t1_h = __lsx_vsrai_h(t1_h, 1);
  t1_h = __lsx_vadd_h(q2_h, t1_h);
  t = __lsx_vslli_h(q1_h, 1);
  t1_h = __lsx_vsub_h(t1_h, t);
  t1_h = __lsx_vsrai_h(t1_h, 1);
  t1_h = __lsx_vmin_h(iTc0_h, t1_h);
  t1_h = __lsx_vmax_h(negiTc0_h, t1_h);
  t1_h = __lsx_vadd_h(q1_h, t1_h);

  t2_l = __lsx_vsub_h(q0_l, p0_l);
  t2_l = __lsx_vslli_h(t2_l, 2);
  t2_l = __lsx_vadd_h(t2_l, p1_l);
  t2_l = __lsx_vsub_h(t2_l, q1_l);
  t2_l = __lsx_vaddi_hu(t2_l, 4);
  t2_l = __lsx_vsrai_h(t2_l, 3);
  t2_l = __lsx_vmin_h(iTc_l, t2_l);
  t2_l = __lsx_vmax_h(negiTc_l, t2_l);

  t2_h = __lsx_vsub_h(q0_h, p0_h);
  t2_h = __lsx_vslli_h(t2_h, 2);
  t2_h = __lsx_vadd_h(t2_h, p1_h);
  t2_h = __lsx_vsub_h(t2_h, q1_h);
  t2_h = __lsx_vaddi_hu(t2_h, 4);
  t2_h = __lsx_vsrai_h(t2_h, 3);
  t2_h = __lsx_vmin_h(iTc_h, t2_h);
  t2_h = __lsx_vmax_h(negiTc_h, t2_h);

  p0_l = __lsx_vadd_h(p0_l, t2_l);
  p1_l = __lsx_vand_v(p0_l, not_255);
  p2_l = __lsx_vsle_h(zero, p0_l);
  flags = __lsx_vseq_h(p1_l, zero);
  p0_l = __lsx_vand_v(p0_l, flags);
  flags = __lsx_vnor_v(flags,flags);
  p2_l = __lsx_vand_v(p2_l, flags);
  p0_l = __lsx_vadd_h(p0_l, p2_l);

  q0_l = __lsx_vsub_h(q0_l, t2_l);
  q1_l = __lsx_vand_v(q0_l, not_255);
  q2_l = __lsx_vsle_h(zero, q0_l);
  flags = __lsx_vseq_h(q1_l, zero);
  q0_l = __lsx_vand_v(q0_l, flags);
  flags = __lsx_vnor_v(flags, flags);
  q2_l = __lsx_vand_v(q2_l, flags);
  q0_l = __lsx_vadd_h(q0_l, q2_l);

  p0_h = __lsx_vadd_h(p0_h, t2_h);
  p1_h = __lsx_vand_v(p0_h, not_255);
  p2_h = __lsx_vsle_h(zero, p0_h);
  flags = __lsx_vseq_h(p1_h, zero);
  p0_h = __lsx_vand_v(p0_h, flags);
  flags = __lsx_vnor_v(flags, flags);
  p2_h = __lsx_vand_v(p2_h, flags);
  p0_h = __lsx_vadd_h(p0_h, p2_h);

  q0_h = __lsx_vsub_h(q0_h, t2_h);
  q1_h = __lsx_vand_v(q0_h, not_255);
  q2_h = __lsx_vsle_h(zero, q0_h);
  flags = __lsx_vseq_h(q1_h, zero);
  q0_h = __lsx_vand_v(q0_h, flags);
  flags = __lsx_vnor_v(flags, flags);
  q2_h = __lsx_vand_v(q2_h, flags);
  q0_h = __lsx_vadd_h(q0_h, q2_h);

  DUP4_ARG2(__lsx_vpickev_b, t0_h, t0_l, t1_h, t1_l,
            p0_h, p0_l, q0_h, q0_l, t0, t1, t2, t3);

  flags = __lsx_vsle_b(zero, iTc0);
  flags = __lsx_vand_v(flags, f);
  t2 = __lsx_vand_v(t2, flags);
  t = __lsx_vnor_v(flags,flags);
  p0 = __lsx_vand_v(p0, t);
  p0 = __lsx_vadd_b(t2, p0);
  t3 = __lsx_vand_v(t3, flags);
  t = __lsx_vnor_v(flags,flags);
  q0 = __lsx_vand_v(q0, t);
  q0 = __lsx_vadd_b(t3, q0);

  DUP2_ARG2(__lsx_vand_v, flags, bDetaP2P0, t0, t, t, t0);
  t = __lsx_vnor_v(t, t);
  p1 = __lsx_vand_v(p1, t);
  p1 = __lsx_vadd_b(t0, p1);
  DUP2_ARG2(__lsx_vand_v, flags, bDetaQ2Q0, t1, t, t, t1);
  t = __lsx_vnor_v(t, t);
  q1 = __lsx_vand_v(q1, t);
  q1 = __lsx_vadd_b(t1, q1);

  DUP2_ARG2(__lsx_vilvl_b, p0, p1, q1, q0, t0, t2);
  DUP2_ARG2(__lsx_vilvh_b, p0, p1, q1, q0, t1, t3);
  DUP2_ARG2(__lsx_vilvl_h, t2, t0, t3, t1, p0, p2);
  DUP2_ARG2(__lsx_vilvh_h, t2, t0, t3, t1, p1, q0);

  pPix -= iStrideY_x4;
  pPix -= iStrideY_x4;
  pPix -= iStrideY_x4 - 1;
  __lsx_vstelm_w(p0, pPix, 0, 0);
  __lsx_vstelm_w(p0, pPix + iStrideY, 0, 1);
  __lsx_vstelm_w(p0, pPix + iStrideY_x2, 0, 2);
  __lsx_vstelm_w(p0, pPix + iStrideY_x3, 0, 3);
  pPix += iStrideY_x4;
  __lsx_vstelm_w(p1, pPix, 0, 0);
  __lsx_vstelm_w(p1, pPix + iStrideY, 0, 1);
  __lsx_vstelm_w(p1, pPix + iStrideY_x2, 0, 2);
  __lsx_vstelm_w(p1, pPix + iStrideY_x3, 0, 3);
  pPix += iStrideY_x4;
  __lsx_vstelm_w(p2, pPix, 0, 0);
  __lsx_vstelm_w(p2, pPix + iStrideY, 0, 1);
  __lsx_vstelm_w(p2, pPix + iStrideY_x2, 0, 2);
  __lsx_vstelm_w(p2, pPix + iStrideY_x3, 0, 3);
  pPix += iStrideY_x4;
  __lsx_vstelm_w(q0, pPix, 0, 0);
  __lsx_vstelm_w(q0, pPix + iStrideY, 0, 1);
  __lsx_vstelm_w(q0, pPix + iStrideY_x2, 0, 2);
  __lsx_vstelm_w(q0, pPix + iStrideY_x3, 0, 3);
}
