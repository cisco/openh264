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

//wels_common_basis.h
#ifndef WELS_COMMON_BASIS_H__
#define WELS_COMMON_BASIS_H__

#include "typedefs.h"
#include "macros.h"

#include "wels_common_defs.h"

using namespace WelsCommon;

namespace WelsDec {

/*common use table*/
extern const uint8_t g_kuiScan8[24];
extern const uint8_t g_kuiLumaDcZigzagScan[16];
extern const uint8_t g_kuiChromaDcScan[4];
extern const uint8_t g_kMbNonZeroCountIdx[24];
extern const uint8_t g_kCacheNzcScanIdx[4*4+4+4+3];
extern const uint8_t g_kCache26ScanIdx[16];
extern const uint8_t g_kCache30ScanIdx[16];
extern const uint8_t g_kNonZeroScanIdxC[4];
/* Profile IDC */
typedef uint8_t		ProfileIdc;

/* Position Offset structure */
typedef struct TagPosOffset {
int32_t	iLeftOffset;
int32_t	iTopOffset;
int32_t	iRightOffset;
int32_t	iBottomOffset;
} SPosOffset;

/* MB Type & Sub-MB Type */
typedef int32_t MbType;
typedef int32_t SubMbType;

#define I16_LUMA_DC  1
#define I16_LUMA_AC  2
#define LUMA_DC_AC   3
#define CHROMA_DC    4
#define CHROMA_AC    5
#define CHROMA_DC_U  6
#define CHROMA_DC_V  7
#define CHROMA_AC_U  8
#define CHROMA_AC_V  9
#define LUMA_DC_AC_INTRA 10
#define LUMA_DC_AC_INTER 11
#define CHROMA_DC_U_INTER  12
#define CHROMA_DC_V_INTER  13
#define CHROMA_AC_U_INTER  14
#define CHROMA_AC_V_INTER  15

#define SHIFT_BUFFER(pBitsCache)	{	pBitsCache->pBuf+=2; pBitsCache->uiRemainBits += 16; pBitsCache->uiCache32Bit |= (((pBitsCache->pBuf[2] << 8) | pBitsCache->pBuf[3]) << (32 - pBitsCache->uiRemainBits));	}
#define POP_BUFFER(pBitsCache, iCount)	{ pBitsCache->uiCache32Bit <<= iCount;	pBitsCache->uiRemainBits -= iCount;	}

static const uint8_t g_kuiZigzagScan[16] = { //4*4block residual zig-zag scan order
    0,  1,  4,  8,
    5,  2,  3,  6,
    9, 12, 13, 10,
    7, 11, 14, 15,
};


static inline void GetMbResProperty(int32_t * pMBproperty,int32_t* pResidualProperty,bool bCavlc)
{
 switch(*pResidualProperty)
  {
  case CHROMA_AC_U:
	  *pMBproperty = 1;
	  *pResidualProperty = bCavlc ? CHROMA_AC : CHROMA_AC_U;
	  break;
  case CHROMA_AC_V:
	  *pMBproperty = 2;
	  *pResidualProperty = bCavlc ? CHROMA_AC : CHROMA_AC_V;
	  break;
  case LUMA_DC_AC_INTRA:
	  *pMBproperty = 0;
	  *pResidualProperty = LUMA_DC_AC;
	  break;
  case CHROMA_DC_U:
      *pMBproperty = 1;
	  *pResidualProperty =  bCavlc ? CHROMA_DC : CHROMA_DC_U;
      break;
 case CHROMA_DC_V:
	  *pMBproperty = 2;
	  *pResidualProperty =  bCavlc ? CHROMA_DC : CHROMA_DC_V;
	  break;
  case I16_LUMA_AC:
	  *pMBproperty = 0;
	  break;
  case I16_LUMA_DC:
	  *pMBproperty = 0;
	  break;
  case LUMA_DC_AC_INTER:
	  *pMBproperty = 3;
      *pResidualProperty = LUMA_DC_AC;
	  break;
  case CHROMA_DC_U_INTER:
      *pMBproperty = 4;
	  *pResidualProperty =  bCavlc ? CHROMA_DC : CHROMA_DC_U;
      break;
  case CHROMA_DC_V_INTER:
	  *pMBproperty = 5;
	  *pResidualProperty =  bCavlc ? CHROMA_DC : CHROMA_DC_V;
	  break;
 case CHROMA_AC_U_INTER:
	  *pMBproperty = 4;
	  *pResidualProperty =  bCavlc ? CHROMA_AC : CHROMA_AC_U;
	  break;
 case CHROMA_AC_V_INTER:
	  *pMBproperty = 5;
	  *pResidualProperty =  bCavlc ?CHROMA_AC:CHROMA_AC_V;
	  break;
 }
  }

typedef struct TagI16PredInfo {
    int8_t iPredMode;
    int8_t iLeftAvail;
    int8_t iTopAvail;
    int8_t iLeftTopAvail;
} SI16PredInfo;
static const SI16PredInfo g_ksI16PredInfo[4] = {
    {I16_PRED_V, 0, 1, 0},
    {I16_PRED_H, 1, 0, 0},
    {         0, 0, 0, 0},
    {I16_PRED_P, 1, 1, 1},
};

static const SI16PredInfo g_ksChromaPredInfo[4] = {
    {       0, 0, 0, 0},
    {C_PRED_H, 1, 0, 0},
    {C_PRED_V, 0, 1, 0},
    {C_PRED_P, 1, 1, 1},
};


typedef struct TagI4PredInfo {
    int8_t iPredMode;
    int8_t iLeftAvail;
    int8_t iTopAvail;
    int8_t iLeftTopAvail;
    //	int8_t right_top_avail; //when right_top unavailable but top avail, we can pad the right-top with the rightmost pixel of top
} SI4PredInfo;
static const SI4PredInfo g_ksI4PredInfo[9] = {
    {  I4_PRED_V, 0, 1, 0},
    {  I4_PRED_H, 1, 0, 0},
    {          0, 0, 0, 0},
    {I4_PRED_DDL, 0, 1, 0},
    {I4_PRED_DDR, 1, 1, 1},
    { I4_PRED_VR, 1, 1, 1},
    { I4_PRED_HD, 1, 1, 1},
    { I4_PRED_VL, 0, 1, 0},
    { I4_PRED_HU, 1, 0, 0},
};

static const uint8_t g_kuiI16CbpTable[6] = {0, 16, 32, 15, 31, 47}; 


typedef struct TagPartMbInfo {
    MbType iType;
    int8_t iPartCount; //P_16*16, P_16*8, P_8*16, P_8*8 based on 8*8 block; P_8*4, P_4*8, P_4*4 based on 4*4 block
    int8_t iPartWidth; //based on 4*4 block
} SPartMbInfo;
static const SPartMbInfo g_ksInterMbTypeInfo[5] = {
    {MB_TYPE_16x16,    1, 4},
    {MB_TYPE_16x8,     2, 4},
    {MB_TYPE_8x16,     2, 2},
    {MB_TYPE_8x8,      4, 4},
    {MB_TYPE_8x8_REF0, 4, 4}, //ref0--ref_idx not present in bit-stream and default as 0
};
static const SPartMbInfo g_ksInterSubMbTypeInfo[4] = {
    {SUB_MB_TYPE_8x8, 1, 2},
    {SUB_MB_TYPE_8x4, 2, 2},
    {SUB_MB_TYPE_4x8, 2, 1},
    {SUB_MB_TYPE_4x4, 4, 1},
};

} // namespace WelsDec

#endif//WELS_COMMON_BASIS_H__
