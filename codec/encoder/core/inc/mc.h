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

//macroblock.h
#ifndef WELS_MC_H__
#define WELS_MC_H__

#include <string.h>
#include "typedefs.h"
#include "wels_const.h"
#include "macros.h"
#include "wels_func_ptr_def.h"

/////////////////////luma MC//////////////////////////
//x y means dx(mv[0] & 3) and dy(mv[1] & 3)

namespace WelsSVCEnc {
void WelsInitMcFuncs (SWelsFuncPtrList* pFuncList, uint32_t uiCpuFlag);


#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

//***************************************************************************//
//                       MMXEXT and SSE2 definition                          //
//***************************************************************************//
#if defined(X86_ASM)
void McChromaWidthEq4_mmx (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, const uint8_t* kpABCD,
                           int32_t iHeigh);
void McCopyWidthEq4_mmx (uint8_t*, int32_t, uint8_t*, int32_t, int32_t);
void McCopyWidthEq8_mmx (uint8_t*, int32_t, uint8_t*, int32_t, int32_t);
void PixelAvgWidthEq8_mmx (uint8_t*,  int32_t, uint8_t*, int32_t, uint8_t*, int32_t, int32_t);

void McHorVer20Width9Or17_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                      int32_t iHeight);
void McHorVer02Height9Or17_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                      int32_t iHeight);
void McHorVer22HorFirst_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pTap, int32_t iTapStride, int32_t iWidth,
                              int32_t iHeight);
void McHorVer22Width8VerLastAlign_sse2 (uint8_t* pTap, int32_t iTapStride, uint8_t* pDst, int32_t iDstStride, int32_t iWidth,
                                  int32_t iHeight);
void McHorVer22Width8VerLastUnAlign_sse2 (uint8_t* pTap, int32_t iTapStride, uint8_t* pDst, int32_t iDstStride,
                                    int32_t iWidth, int32_t iHeight);
void McChromaWidthEq8_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, const uint8_t* kpABCD,
                            int32_t iHeigh);
void McCopyWidthEq16_sse2 (uint8_t*, int32_t, uint8_t*, int32_t, int32_t);
void McHorVer20WidthEq16_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iHeight);
void McHorVer02WidthEq8_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride, int32_t iHeight);
void McHorVer22Width8HorFirst_sse2 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pTap,	int32_t iTapStride,
                                    int32_t iHeight);
void PixelAvgWidthEq16_sse2 (uint8_t*,  int32_t, uint8_t*, int32_t, uint8_t*, int32_t, int32_t);

void McChromaWidthEq8_ssse3 (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                             const uint8_t* kpABCD, int32_t iHeigh);


#endif //X86_ASM

#if defined(__cplusplus)
}
#endif//__cplusplus
}
#endif//WELS_MC_H__
