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

#ifndef WELS_DECODE_SLICE_H__
#define WELS_DECODE_SLICE_H__

#include "decoder_context.h"

namespace WelsDec {

void WelsBlockInit (int16_t* pBlock, int32_t iWidth, int32_t iHeight, int32_t iStride, uint8_t uiVal);

int32_t WelsActualDecodeMbCavlcISlice (PWelsDecoderContext pCtx);
int32_t WelsDecodeMbCavlcISlice (PWelsDecoderContext pCtx, PNalUnit pNalCur);

int32_t WelsActualDecodeMbCavlcPSlice (PWelsDecoderContext pCtx);
int32_t WelsDecodeMbCavlcPSlice (PWelsDecoderContext pCtx, PNalUnit pNalCur);
typedef int32_t (*PWelsDecMbCavlcFunc) (PWelsDecoderContext pCtx, PNalUnit pNalCur);

int32_t WelsTargetSliceConstruction (PWelsDecoderContext pCtx); //construction based on slice

int32_t WelsDecodeSlice (PWelsDecoderContext pCtx, bool bFirstSliceInLayer, PNalUnit pNalCur);


int32_t WelsTargetMbConstruction (PWelsDecoderContext pCtx);

int32_t WelsMbIntraPredictionConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer, bool bOutput);
int32_t WelsMbInterSampleConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer,
                                       uint8_t* pDstY, uint8_t* pDstU, uint8_t* pDstV, int32_t iStrideL, int32_t iStrideC);
int32_t WelsMbInterConstruction (PWelsDecoderContext pCtx, PDqLayer pCurLayer);
void WelsLumaDcDequantIdct (int16_t* pBlock, int32_t iQp);
int32_t WelsMbInterPrediction (PWelsDecoderContext pCtx, PDqLayer pCurLayer);
void WelsChromaDcIdct (int16_t* pBlock);

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#if defined(HAVE_NEON)
void WelsResBlockZero16x16_neon(int16_t* pBlock, int32_t iStride);
void WelsResBlockZero8x8_neon(int16_t* pBlock, int32_t iStride);
void SetNonZeroCount_neon(int16_t* pBlock, int8_t* pNonZeroCount);
#endif

#ifdef  X86_ASM
void WelsResBlockZero16x16_sse2 (int16_t* pBlock, int32_t iStride);
void WelsResBlockZero8x8_sse2 (int16_t* pBlock, int32_t iStride);
#endif

#ifdef __cplusplus
}
#endif//__cplusplus

void WelsBlockZero16x16_c (int16_t* pBlock, int32_t iStride);
void WelsBlockZero8x8_c (int16_t* pBlock, int32_t iStride);
void SetNonZeroCount_c (int16_t* pBlock, int8_t* pNonZeroCount);

void WelsBlockFuncInit (SBlockFunc* pFunc,  int32_t iCpu);

} // namespace WelsDec

#endif //WELS_DECODE_SLICE_H__


