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
 * \file    svc_encode_slice.h
 *
 * \brief   svc encoding slice
 *
 * \date    2009.07.27 Created
 *
 *************************************************************************************
 */
#ifndef SVC_ENCODE_SLICE_H__
#define SVC_ENCODE_SLICE_H__

#include "encoder_context.h"
#include "as264_common.h"
#include "svc_enc_macroblock.h"
#include "mb_cache.h"

namespace WelsEnc {
#if defined(MB_TYPES_CHECK)
void WelsCountMbType (int32_t (*iMbCount)[18], const EWelsSliceType eSt, const SMB* pMb);
#endif


void UpdateNonZeroCountCache (SMB* pMb, SMbCache* pMbCache);

//for P SSlice (intra part + inter part, MB level)
void OutputPMbWithoutConstructCsRsNoCopy (sWelsEncCtx* pEncCtx, SDqLayer* pDq, SSlice* pSlice, SMB* pMb);

void WelsSliceHeaderScalExtInit (SDqLayer* pCurLayer, SSlice* pSlice);
void WelsSliceHeaderExtInit (sWelsEncCtx* pEncCtx, SDqLayer* pCurLayer, SSlice* pSlice);

void WelsSliceHeaderWrite (SBitStringAux* pBs, SDqLayer* pCurLayer, SSlice* pSlice, uint32_t uiPpsIdBasis);
void WelsSliceHeaderExtWrite (SBitStringAux* pBs, SDqLayer* pCurLayer, SSlice* pSlice, uint32_t uiPpsIdBasis);

//===================MB-leve encode====================//
void WelsInterMbEncode (sWelsEncCtx* pEncCtx, SSlice* pSlice, SMB* pCurMb); //only for inter part
//for I SSlice (only intra part, MB level)
void WelsIMbChromaEncode (sWelsEncCtx* pEncCtx, SMB* pCurMb, SMbCache* pMbCache);
//for P SSlice (intra part + inter part, MB level)
void WelsPMbChromaEncode (sWelsEncCtx* pEncCtx, SSlice* pSlice, SMB* pCurMb);


//===================MB-level encode====================//
//encapsulation func: store base rec, highest Dependency Layer(only one quality) rec, single layer rec
int32_t WelsPSliceMdEnc (sWelsEncCtx* pEncCtx, SSlice* pSlice,  const bool kbIsHighestDlayerFlag);
int32_t WelsPSliceMdEncDynamic (sWelsEncCtx* pEncCtx, SSlice* pSlice,  const bool kbIsHighestDlayerFlag);

//encapsulation func: store base rec, highest Dependency Layer(only one quality) rec, single layer rec
int32_t WelsISliceMdEnc (sWelsEncCtx* pEncCtx, SSlice* pSlice);         // for intra non-dynamic slice
int32_t WelsISliceMdEncDynamic (sWelsEncCtx* pEncCtx, SSlice* pSlice);  // for intra dynamic slice

int32_t WelsCodePSlice (sWelsEncCtx* pEncCtx, SSlice* pSlice);
int32_t WelsCodePOverDynamicSlice (sWelsEncCtx* pEncCtx, SSlice* pSlice);

int32_t WelsCodeOneSlice (sWelsEncCtx* pEncCtx, const int32_t kiSliceIdx,
                          const int32_t keNalType);

void WelsInitSliceEncodingFuncs (uint32_t uiCpuFlag);

void UpdateMbNeighbourInfoForNextSlice (SSliceCtx* pSliceCtx,
                                        SMB* pMbList,
                                        const int32_t kiNextSliceFirstMbIdx,
                                        const int32_t kiLastMbIdxInPartition);
void AddSliceBoundary (sWelsEncCtx* pEncCtx, SSlice* pCurSlice, SSliceCtx* pSliceCtx, SMB* pCurMb,
                       int32_t iNextSliceFirstMbIdx, const int32_t kiLastMbIdxInPartition);
int32_t WelsMdInterMbLoop (sWelsEncCtx* pEncCtx, SSlice* pSlice, void* pMd,
                           const int32_t kiSliceFirstMbXY); // for inter non-dynamic slice
int32_t WelsMdInterMbLoopOverDynamicSlice (sWelsEncCtx* pEncCtx, SSlice* pSlice, void* pMd,
    const int32_t kiSliceFirstMbXY); // for inter dynamic slice


bool DynSlcJudgeSliceBoundaryStepBack (void* pEncCtx, void* pSlice, SSliceCtx* pSliceCtx, SMB* pCurMb,
                                       SDynamicSlicingStack* pDss);
}
#endif //SVC_ENCODE_SLICE_H__
