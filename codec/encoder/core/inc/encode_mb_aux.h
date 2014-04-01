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

#ifndef ENCODE_MB_AUX_H
#define ENCODE_MB_AUX_H

#include "typedefs.h"
#include "wels_func_ptr_def.h"

namespace WelsSVCEnc {
void WelsInitEncodingFuncs (SWelsFuncPtrList* pFuncList, uint32_t  uiCpuFlag);
int32_t WelsGetNoneZeroCount_c (int16_t* pLevel);

/****************************************************************************
 * Scan and Score functions
 ****************************************************************************/
void	WelsScan4x4Ac_c (int16_t* pZigValue, int16_t* pDct);
void	WelsScan4x4Dc (int16_t* pLevel, int16_t* pDct);
void	WelsScan4x4DcAc_c (int16_t* pLevel, int16_t* pDct);
int32_t		WelsCalculateSingleCtr4x4_c (int16_t* pDct);

/****************************************************************************
 * HDM and Quant functions
 ****************************************************************************/
void WelsHadamardT4Dc_c (int16_t* pLumaDc, int16_t* pDct);
int32_t WelsHadamardQuant2x2_c (int16_t* pRes, const int16_t kiFF, int16_t iMF, int16_t* pDct, int16_t* pBlock);
int32_t WelsHadamardQuant2x2Skip_c (int16_t* pRes, int16_t iFF,  int16_t iMF);

void WelsQuant4x4_c (int16_t* pDct, const int16_t* pFF, const int16_t* pMF);
void WelsQuant4x4Dc_c (int16_t* pDct, int16_t iFF,  int16_t iMF);
void WelsQuantFour4x4_c (int16_t* pDct, const int16_t* pFF, const int16_t* pQpTable);
void WelsQuantFour4x4Max_c (int16_t* pDct, const int16_t* pF, const int16_t* pQpTable, int16_t* pMax);


/****************************************************************************
 * DCT functions
 ****************************************************************************/
void WelsDctT4_c (int16_t* pDct, uint8_t* pPixel1, int32_t iStride1, uint8_t* pPixel2, int32_t iStride2);
// dct_data is no-use here, just for the same interface with dct_save functions
void WelsDctFourT4_c (int16_t* pDct, uint8_t* pPixel1, int32_t iStride1, uint8_t* pPixel2, int32_t iStride2);

/****************************************************************************
 * Copy functions
 ****************************************************************************/
void WelsCopy4x4 (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x8_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x16_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);	//
void WelsCopy16x8_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);	//
void WelsCopy16x16_c (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);

#if defined(__cplusplus)
extern "C" {
#endif//__cplusplus

#ifdef X86_ASM

int32_t WelsGetNoneZeroCount_sse2 (int16_t* pLevel);

/****************************************************************************
 * Scan and Score functions
 ****************************************************************************/
void WelsScan4x4Ac_sse2 (int16_t* zig_value, int16_t* pDct);
void WelsScan4x4DcAc_ssse3 (int16_t* pLevel, int16_t* pDct);
void WelsScan4x4DcAc_sse2 (int16_t* pLevel, int16_t* pDct);
int32_t WelsCalculateSingleCtr4x4_sse2 (int16_t* pDct);

/****************************************************************************
 * DCT functions
 ****************************************************************************/
void WelsDctT4_mmx (int16_t* pDct,  uint8_t* pPixel1, int32_t iStride1, uint8_t* pPixel2, int32_t iStride2);
void WelsDctFourT4_sse2 (int16_t* pDct, uint8_t* pPixel1, int32_t iStride1, uint8_t* pPixel2, int32_t iStride2);

/****************************************************************************
 * HDM and Quant functions
 ****************************************************************************/
int32_t WelsHadamardQuant2x2_mmx (int16_t* pRes, const int16_t kiFF, int16_t iMF, int16_t* pDct, int16_t* pBlock);
void WelsHadamardT4Dc_sse2 (int16_t* pLumaDc, int16_t* pDct);
int32_t WelsHadamardQuant2x2Skip_mmx (int16_t* pRes, int16_t iFF,  int16_t iMF);

void WelsQuant4x4_sse2 (int16_t* pDct, const int16_t* pFF, const int16_t* pMF);
void WelsQuant4x4Dc_sse2 (int16_t* pDct, int16_t iFF, int16_t iMF);
void WelsQuantFour4x4_sse2 (int16_t* pDct, const int16_t* pFF, const int16_t* pMF);
void WelsQuantFour4x4Max_sse2 (int16_t* pDct, const int16_t* pFF, const int16_t* pMF, int16_t* pMax);


/****************************************************************************
 * Copy functions for rec
 ****************************************************************************/
void WelsCopy8x8_mmx (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy8x16_mmx (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
void WelsCopy16x8NotAligned_sse2 (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
void WelsCopy16x16_sse2 (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
void WelsCopy16x16NotAligned_sse2 (uint8_t* Dst, int32_t  iStrideD, uint8_t* Src, int32_t  iStrideS);
#endif

#ifdef	HAVE_NEON
void WelsCopy8x8_neon( uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS );
void WelsCopy16x16_neon( uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS );
void WelsCopy16x16NotAligned_neon( uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS );
void WelsCopy16x8NotAligned_neon( uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS );
void WelsCopy8x16_neon( uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS );

void WelsHadamardT4Dc_neon(int16_t* pLumaDc, int16_t* pDct);
int32_t WelsHadamardQuant2x2_neon(int16_t* pRes, const int16_t kiFF, int16_t iMF, int16_t* pDct, int16_t* pBlock);
int32_t WelsHadamardQuant2x2Skip_neon(int16_t* pRes, int16_t iFF,  int16_t iMF);
int32_t WelsHadamardQuant2x2SkipKernel_neon(int16_t *pRes, int16_t iThreshold);// avoid divide operator

void WelsDctT4_neon(int16_t* pDct,  uint8_t* pPixel1, int32_t iStride1, uint8_t* pPixel2, int32_t iStride2);
void WelsDctFourT4_neon(int16_t* pDct,  uint8_t* pPixel1, int32_t iStride1, uint8_t* pPixel2, int32_t iStride2);

int32_t WelsGetNoneZeroCount_neon(int16_t* pLevel);

void WelsQuant4x4_neon(int16_t* pDct, const int16_t* pFF, const int16_t* pMF);
void WelsQuant4x4Dc_neon(int16_t* pDct, int16_t iFF, int16_t iMF);
void WelsQuantFour4x4_neon(int16_t* pDct, const int16_t* pFF, const int16_t* pMF);
void WelsQuantFour4x4Max_neon(int16_t* pDct, const int16_t* pFF, const int16_t* pMF, int16_t* pMax);
#endif

#if defined(__cplusplus)
}
#endif//__cplusplus

__align16 (extern const int16_t, g_kiQuantInterFF[58][8]);
#define g_iQuantIntraFF (g_kiQuantInterFF +6 )
__align16 (extern const int16_t, g_kiQuantMF[52][8]) ;
}
#endif//ENCODE_MB_AUX_H
