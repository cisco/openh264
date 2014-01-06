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
 * \date	05/14/2009 Created
 *
 *************************************************************************************
 */

#ifndef WELS_DEBLOCKING_H__
#define WELS_DEBLOCKING_H__

#include "decoder_context.h"

//#pragma pack(1)

namespace WelsDec {

/*!
 * \brief	deblocking module initialize
 *
 * \param	pf
 *          cpu
 *
 * \return	NONE
 */

void_t  DeblockingInit (PDeblockingFunc pDeblockingFunc,  int32_t iCpu);


/*!
 * \brief	deblocking filtering target slice
 *
 * \param	dec			Wels decoder context
 *
 * \return	NONE
 */
void_t WelsDeblockingFilterSlice (PWelsDecoderContext pCtx, PDeblockingFilterMbFunc pDeblockMb);

/*!
 * \brief	pixel deblocking filtering
 *
 * \param	filter			      deblocking filter
 * \param	pix	                  pixel value
 * \param	stride	              frame stride
 * \param	bs	                  boundary strength
 *
 * \return	NONE
 */

uint32_t DeblockingBsMarginalMBAvcbase (PDqLayer pCurDqLayer, int32_t iEdge, int32_t iNeighMb, int32_t iMbXy);

int32_t DeblockingAvailableNoInterlayer (PDqLayer pCurDqLayer, int32_t iFilterIdc);

void_t DeblockingIntraMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag);
void_t DeblockingInterMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, uint8_t nBS[2][4][4], int32_t iBoundryFlag);

void_t WelsDeblockingMb (PDqLayer pCurDqLayer, PDeblockingFilter  pFilter, int32_t iBoundryFlag);

void_t DeblockLumaLt4V_c (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc);
void_t DeblockLumaEq4V_c (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta);

void_t DeblockLumaLt4H_c (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc);
void_t DeblockLumaEq4H_c (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta);

void_t DeblockChromaLt4V_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                            int8_t* pTc);
void_t DeblockChromaEq4V_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta);

void_t DeblockChromaLt4H_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                            int8_t* pTc);
void_t DeblockChromaEq4H_c (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta);

#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

#ifdef  X86_ASM
void DeblockLumaLt4V_sse2 (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* pTc);
void DeblockLumaEq4V_sse2 (uint8_t* pPixY, int32_t iStride, int32_t iAlpha, int32_t iBeta);
void DeblockLumaTransposeH2V_sse2 (uint8_t* pPixY, int32_t iStride, uint8_t* pDst);
void DeblockLumaTransposeV2H_sse2 (uint8_t* pPixY, int32_t iStride, uint8_t* pSrc);
void DeblockChromaEq4V_sse2 (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta);
void DeblockChromaLt4V_sse2 (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                             int8_t* pTC);
void DeblockChromaEq4H_sse2 (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta);
void DeblockChromaLt4H_sse2 (uint8_t* pPixCb, uint8_t* pPixCr, int32_t iStride, int32_t iAlpha, int32_t iBeta,
                             int8_t* pTC);
#endif
#if defined(__cplusplus)
}
#endif//__cplusplus

} // namespace WelsDec

//#pragma pack()

#endif //WELS_DEBLOCKING_H__

