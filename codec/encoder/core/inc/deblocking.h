/*!
 * \copy
 *     Copyright (c)  2009-2013, Cisco Systems
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
 * \file	deblocking.h
 *
 * \brief	Interfaces introduced in frame deblocking filtering
 *
 * \date	08/03/2009 Created
 *
 *************************************************************************************
 */

#ifndef WELS_DEBLOCKING_H_
#define WELS_DEBLOCKING_H_

#include "encoder_context.h"
#include "wels_func_ptr_def.h"
#include "deblocking_common.h"
namespace WelsSVCEnc {


//struct tagDeblockingFunc;

typedef struct TagDeblockingFilter {
uint8_t*		pCsData[3];	// pointer to reconstructed picture pData
int32_t		iCsStride[3];	// Cs iStride
int16_t     iMbStride;
int8_t		iSliceAlphaC0Offset;
int8_t		iSliceBetaOffset;
uint8_t     uiLumaQP;
uint8_t     uiChromaQP;
uint8_t     uiFilterIdc;
uint8_t     uiReserved;
} SDeblockingFilter;

void DeblockingInit (DeblockingFunc*   pFunc,  int32_t iCpu);

void WelsNonZeroCount_c (int8_t* pNonZeroCount);
void WelsBlockFuncInit (PSetNoneZeroCountZeroFunc* pfSetNZCZero,  int32_t iCpu);

void PerformDeblockingFilter (sWelsEncCtx* pEnc);

void DeblockingFilterFrameAvcbase (SDqLayer* pCurDq, SWelsFuncPtrList* pFunc);

void DeblockingFilterSliceAvcbase (SDqLayer* pCurDq, SWelsFuncPtrList* pFunc, const int32_t kiSliceIdx);
}

#endif


