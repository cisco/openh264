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
 * \file	get_intra_predictor.h
 *
 * \brief	interfaces for get intra predictor about 16x16, 4x4, chroma.
 *
 * \date	4/2/2009 Created
 *
 *************************************************************************************
 */

#ifndef GET_INTRA_PREDICTOR_H
#define GET_INTRA_PREDICTOR_H

#include "typedefs.h"
#include "wels_func_ptr_def.h"

namespace WelsSVCEnc {
void WelsI4x4LumaPredV_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredH_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredDc_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredDcLeft_c (uint8_t* pPred, uint8_t* pRef,  const int32_t kiStride);
void WelsI4x4LumaPredDcTop_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredDcNA_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);

void WelsI4x4LumaPredDDL_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredDDLTop_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredDDR_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);

void WelsI4x4LumaPredVR_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredHD_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredVL_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredVLTop_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredHU_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);


void WelsIChormaPredV_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsIChormaPredH_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsIChormaPredPlane_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsIChormaPredDc_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsIChormaPredDcLeft_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsIChormaPredDcTop_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsIChormaPredDcNA_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);

void WelsI16x16ChormaPredVer (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16ChormaPredHor (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);

void WelsI16x16LumaPredV_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredH_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredPlane_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredDc_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredDcLeft_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredDcTop_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredDcNA_c (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);

#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

#if defined(X86_ASM)
void WelsFillingPred8to16_mmx (uint8_t* pPred, uint8_t* pValue);
void WelsFillingPred8x2to16_mmx (uint8_t* pPred, uint8_t* pValue);
void WelsFillingPred1to16_mmx (uint8_t* pPred, const uint8_t kuiValue);
void WelsFillingPred8x2to16_sse2 (uint8_t* pPred, uint8_t* pValue);
void WelsFillingPred1to16_sse2 (uint8_t* pPred, const uint8_t kuiValue);

//for intra-prediction ASM functions
void WelsI16x16LumaPredV_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredH_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredDc_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI16x16LumaPredPlane_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);

void WelsIChromaPredH_mmx (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsIChromaPredV_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsIChromaPredDc_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsIChromaPredPlane_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);

void WelsI4x4LumaPredV_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredH_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredDc_sse2 (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredDDL_mmx (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredDDR_mmx (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredVR_mmx (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredHD_mmx (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredVL_mmx (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
void WelsI4x4LumaPredHU_mmx (uint8_t* pPred, uint8_t* pRef, const int32_t kiStride);
#endif//X86_ASM

#if defined(__cplusplus)
}
#endif//__cplusplus

void WelsInitFillingPredFuncs (const uint32_t kuiCpuFlag);
void WelsInitIntraPredFuncs (SWelsFuncPtrList* pFuncList, const uint32_t kuiCpuFlag);

//#pragma pack()
}
#endif //GET_INTRA_PREDICTOR_H


