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
 * \file	parse_mb_syn_cavlc.h
 *
 * \brief	Parsing all syntax elements of mb and decoding residual with cavlc
 *
 * \date	03/17/2009 Created
 *
 *************************************************************************************
 */


#ifndef WELS_PARSE_MB_SYN_CAVLC_H__
#define WELS_PARSE_MB_SYN_CAVLC_H__

#include "wels_common_basis.h"
#include "decoder_context.h"
#include "dec_frame.h"
#include "slice.h"

namespace WelsDec {

#define I16_LUMA_DC  1
#define I16_LUMA_AC  2
#define LUMA_DC_AC   3
#define CHROMA_DC    4
#define CHROMA_AC    5

typedef struct TagReadBitsCache {
uint32_t uiCache32Bit;
uint8_t  uiRemainBits;
uint8_t*  pBuf;
} SReadBitsCache;

#define SHIFT_BUFFER(pBitsCache)	{	pBitsCache->pBuf+=2; pBitsCache->uiRemainBits += 16; pBitsCache->uiCache32Bit |= (((pBitsCache->pBuf[2] << 8) | pBitsCache->pBuf[3]) << (32 - pBitsCache->uiRemainBits));	}
#define POP_BUFFER(pBitsCache, iCount)	{ pBitsCache->uiCache32Bit <<= iCount;	pBitsCache->uiRemainBits -= iCount;	}

static const uint8_t g_kuiZigzagScan[16] = { //4*4block residual zig-zag scan order
0,  1,  4,  8,
5,  2,  3,  6,
9, 12, 13, 10,
7, 11, 14, 15,
};


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

static const uint8_t g_kuiI16CbpTable[6] = {0, 16, 32, 15, 31, 47}; //reference to JM


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

void_t GetNeighborAvailMbType (PNeighAvail pNeighAvail, PDqLayer pCurLayer);
void_t WelsFillCacheNonZeroCount (PNeighAvail pNeighAvail, uint8_t* pNonZeroCount, PDqLayer pCurLayer);
void_t WelsFillCacheConstrain0Intra4x4 (PNeighAvail pNeighAvail, uint8_t* pNonZeroCount, int8_t* pIntraPredMode,
                                        PDqLayer pCurLayer);
void_t WelsFillCacheConstrain1Intra4x4 (PNeighAvail pNeighAvail, uint8_t* pNonZeroCount, int8_t* pIntraPredMode,
                                        PDqLayer pCurLayer);
void_t WelsFillCacheInter (PNeighAvail pNeighAvail, uint8_t* pNonZeroCount,
                           int16_t iMvArray[LIST_A][30][MV_A], int8_t iRefIdxArray[LIST_A][30], PDqLayer pCurLayer);

void_t PredPSkipMvFromNeighbor (PDqLayer pCurLayer, int16_t iMvp[2]);

/*!
 * \brief   check iPredMode for intra16x16 eligible or not
 * \param 	input : current iPredMode
 * \param 	output: 0 indicating decoding correctly; -1 means error occurence
 */
int32_t CheckIntra16x16PredMode (uint8_t uiSampleAvail, int8_t* pMode);

/*!
 * \brief   check iPredMode for intra4x4 eligible or not
 * \param 	input : current iPredMode
 * \param 	output: 0 indicating decoding correctly; -1 means error occurence
 */
int32_t CheckIntra4x4PredMode (int32_t* pSampleAvail, int8_t* pMode, int32_t iIndex);

/*!
 * \brief   check iPredMode for chroma eligible or not
 * \param 	input : current iPredMode
 * \param 	output: 0 indicating decoding correctly; -1 means error occurence
 */
int32_t CheckIntraChromaPredMode (uint8_t uiSampleAvail, int8_t* pMode);

/*!
 * \brief   predict the mode of intra4x4
 * \param 	input : current intra4x4 block index
 * \param 	output: mode index
 */
int32_t PredIntra4x4Mode (int8_t* pIntraPredMode, int32_t iIdx4);


void_t BsStartCavlc (PBitStringAux pBs);
void_t BsEndCavlc (PBitStringAux pBs);

int32_t WelsResidualBlockCavlc (SVlcTable* pVlcTable,
                                uint8_t* pNonZeroCountCache,
                                PBitStringAux pBs,
                                /*int16_t* coeff_level,*/
                                int32_t iIndex,
                                int32_t iMaxNumCoeff,
                                const uint8_t* kpZigzagTable,
                                int32_t iResidualProperty,
                                /*short *tCoeffLevel,*/
                                int16_t* pTCoeff,
                                int32_t iMbMode,
                                uint8_t uiQp,
                                PWelsDecoderContext pCtx);

/*!
 * \brief   parsing intra mode
 * \param 	input : current mb, bit-stream
 * \param 	output: 0 indicating decoding correctly; -1 means error
 */
int32_t ParseIntra4x4ModeConstrain0 (PNeighAvail pNeighAvail, int8_t* pIntraPredMode, PBitStringAux pBs,
                                     PDqLayer pCurDqLayer);
int32_t ParseIntra4x4ModeConstrain1 (PNeighAvail pNeighAvail, int8_t* pIntraPredMode, PBitStringAux pBs,
                                     PDqLayer pCurDqLayer);
int32_t ParseIntra16x16ModeConstrain0 (PNeighAvail pNeighAvail, PBitStringAux pBs, PDqLayer pCurDqLayer);
int32_t ParseIntra16x16ModeConstrain1 (PNeighAvail pNeighAvail, PBitStringAux pBs, PDqLayer pCurDqLayer);

/*!
 * \brief   parsing inter info (including ref_index and mvd)
 * \param 	input : decoding context, current mb, bit-stream
 * \param 	output: 0 indicating decoding correctly; -1 means error
 */
int32_t ParseInterInfo (PWelsDecoderContext pCtx, int16_t iMvArray[LIST_A][30][MV_A], int8_t iRefIdxArray[LIST_A][30],
                        PBitStringAux pBs);

} // namespace WelsDec
#endif//WELS_PARSE_MB_SYN_CAVLC_H__
