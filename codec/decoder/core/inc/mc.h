/*!
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
 */

#ifndef WELS_MC_H__
#define WELS_MC_H__

#include "wels_const.h"
#include "macros.h"
#include "decoder_context.h"

namespace WelsDec {

void_t InitMcFunc (SMcFunc* pMcFunc, int32_t iCpu);

#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

//***************************************************************************//
//                       MMXEXT definition                          //
//***************************************************************************//
#if defined(X86_ASM)
typedef void_t (*PMcChromaWidthExtFunc) (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    const uint8_t* kpABCD, int32_t iHeight);
extern void_t McHorVer20WidthEq4_mmx (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iHeight);
extern void_t McChromaWidthEq4_mmx (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    const uint8_t* kpABCD, int32_t iHeight);
extern void_t McCopyWidthEq4_mmx (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                  int32_t iHeight);
extern void_t McCopyWidthEq8_mmx (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                  int32_t iHeight);
extern void_t PixelAvgWidthEq4_mmx (uint8_t* pDst, int32_t iDstStride, uint8_t* pSrcA, int32_t iSrcAStride,
                                    uint8_t* pSrcB, int32_t iSrcBStride, int32_t iHeight);
extern void_t PixelAvgWidthEq8_mmx (uint8_t* pDst, int32_t iDstStride, uint8_t* pSrcA, int32_t iSrcAStride,
                                    uint8_t* pSrcB, int32_t iSrcBStride, int32_t iHeight);
//***************************************************************************//
//                       SSE2 definition                          //
//***************************************************************************//
extern void_t McChromaWidthEq8_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                     const uint8_t* kpABCD, int32_t iHeight);
extern void_t McCopyWidthEq16_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iHeight);
extern void_t McHorVer20WidthEq8_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                       int32_t iHeight);
extern void_t McHorVer20WidthEq16_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                        int32_t iHeight);
extern void_t McHorVer02WidthEq8_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                       int32_t iHeight);
extern void_t McHorVer22Width8HorFirst_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
    int32_t iHeight);
extern void_t McHorVer22Width8VerLastAlign_sse2 (uint8_t* pTap, int32_t iTapStride, uint8_t* pDst, int32_t iDstStride,
                                      int32_t iWidth, int32_t iHeight);
extern void_t PixelAvgWidthEq16_sse2 (uint8_t* pDst, int32_t iDstStride, uint8_t* pSrcA, int32_t iSrcAStride,
                                      uint8_t* pSrcB, int32_t iSrcBStride, int32_t iHeight);

#endif //X86_ASM

#if defined(__cplusplus)
}
#endif//__cplusplus

} // namespace WelsDec

#endif//WELS_MC_H__
