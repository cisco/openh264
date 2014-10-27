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

#include "wels_const.h"

#include "wels_common_defs.h"

using namespace WelsCommon;

namespace WelsEnc {


struct SMVUnitXY {			// each 4 Bytes
  int16_t		iMvX;
  int16_t		iMvY;
 public:
  SMVUnitXY& sDeltaMv (const SMVUnitXY& _v0, const SMVUnitXY& _v1) {
    iMvX = _v0.iMvX - _v1.iMvX;
    iMvY = _v0.iMvY - _v1.iMvY;
    return (*this);
  };
  SMVUnitXY& sAssginMv (const SMVUnitXY& _v0) {
    iMvX = _v0.iMvX;
    iMvY = _v0.iMvY;
    return (*this);
  };
};

typedef struct TagMVComponentUnit {		// each 	LIST_0/LIST_1
  SMVUnitXY	sMotionVectorCache[5 * 6 - 1];			// Luma only: 5 x 6 - 1 = 29 D-Words
  int8_t		iRefIndexCache[5 * 6];			// Luma only: 5 x 6 = 30 bytes
} SMVComponentUnit, *PMVComponentUnit;


typedef struct TagParaSetOffsetVariable {
  int32_t 	iParaSetIdDelta[MAX_DQ_LAYER_NUM/*+1*/];	//mark delta between SPS_ID_in_bs and sps_id_in_encoder, can be minus, for each dq-layer
//need not extra +1 due no MGS and FMO case so far
  bool		bUsedParaSetIdInBs[MAX_PPS_COUNT];	//mark the used SPS_ID with 1
  uint32_t	uiNextParaSetIdToUseInBs;					//mark the next SPS_ID_in_bs, for all layers
} SParaSetOffsetVariable;

typedef struct TagParaSetOffset {
//in PS0 design, "sParaSetOffsetVariable" record the previous paras before current IDR, AND NEED to be stacked and recover across IDR
  SParaSetOffsetVariable
  sParaSetOffsetVariable[PARA_SET_TYPE]; //PARA_SET_TYPE=3; paraset_type = 0: AVC_SPS; =1: Subset_SPS; =2: PPS
//in PSO design, "bPpsIdMappingIntoSubsetsps" uses the current para of current IDR period
  bool
  bPpsIdMappingIntoSubsetsps[MAX_DQ_LAYER_NUM/*+1*/];	// need not extra +1 due no MGS and FMO case so far
  uint16_t
  uiIdrPicId;		// IDR picture id: [0, 65535], this one is used for LTR!! Can we just NOT put this into the SParaSetOffset structure?!!
#if _DEBUG
  bool                  bEnableSpsPpsIdAddition;
#endif
} SParaSetOffset;



/* Position Offset structure */
typedef struct TagCropOffset {
  int16_t	iCropLeft;
  int16_t	iCropRight;
  int16_t	iCropTop;
  int16_t	iCropBottom;
} SCropOffset;


/* Transform Type */

enum ETransType {
  T_4x4	= 0,
  T_8x8	= 1,
  T_16x16	= 2,
  T_PCM	= 3
};

enum EMbPosition {
  LEFT_MB_POS     = 0x01,	// A
  TOP_MB_POS      = 0x02,	// B
  TOPRIGHT_MB_POS = 0x04,	// C
  TOPLEFT_MB_POS	= 0x08,	// D,
  RIGHT_MB_POS	= 0x10,	//  add followed four case to reuse when intra up-sample
  BOTTOM_MB_POS	= 0x20,	//
  BOTTOMRIGHT_MB_POS = 0x40,	//
  BOTTOMLEFT_MB_POS	= 0x80,	//
  MB_POS_A  = 0x100
};
#define MB_ON_PIC_BOUNDRY			(RIGHT_MB_POS|BOTTOM_MB_POS|LEFT_MB_POS|TOP_MB_POS)

/* MB Type & Sub-MB Type */
typedef uint32_t Mb_Type;

#define	MB_LEFT_BIT			0// add to use in intra up-sample
#define	MB_TOP_BIT			1
#define	MB_TOPRIGHT_BIT		2
#define	MB_TOPLEFT_BIT		3
#define	MB_RIGHT_BIT		4
#define	MB_BOTTOM_BIT		5
#define	MB_BTMRIGHT_BIT		6
#define	MB_BTMLEFT_BIT		7


/* AVC types*/
#define MB_TYPE_INTRA4x4		0x00000001
#define MB_TYPE_INTRA16x16		0x00000002
#define MB_TYPE_INTRA_PCM		0x00000004
#define MB_TYPE_16x16			0x00000008
#define MB_TYPE_16x8			0x00000010
#define MB_TYPE_8x16			0x00000020
#define MB_TYPE_8x8				0x00000040
#define MB_TYPE_8x8_REF0		0x00000080

#define MB_TYPE_SKIP			0x00000100
#define MB_TYPE_P0L0			0x00000200
#define MB_TYPE_P1L0			0x00000400
#define MB_TYPE_P0L1			0x00000800
#define MB_TYPE_P1L1			0x00001000
#define MB_TYPE_L0				(MB_TYPE_P0L0 | MB_TYPE_P1L0)
#define MB_TYPE_L1				(MB_TYPE_P0L1 | MB_TYPE_P1L1)
#define MB_TYPE_L0L1			(MB_TYPE_L0   | MB_TYPE_L1)
#define MB_TYPE_QUANT			0x00002000
#define MB_TYPE_CBP				0x00004000
/* SVC extension types */
#define MB_TYPE_INTRA_BL		0x00008000// I_BL new MB type derived H.264 SVC specific

#define MB_TYPE_BACKGROUND		0x00010000  // conditional BG skip_mb


#define MB_TYPE_INTRA			(MB_TYPE_INTRA4x4 | MB_TYPE_INTRA16x16 | MB_TYPE_INTRA_PCM)
#define MB_TYPE_INTER			(MB_TYPE_16x16 | MB_TYPE_16x8 | MB_TYPE_8x16 | MB_TYPE_8x8 | MB_TYPE_8x8_REF0)
#define SUB_TYPE_8x8			(MB_TYPE_8x8 | MB_TYPE_8x8_REF0)

#define MB_TYPE_UNAVAILABLE		0xFF000000
#define REF_NOT_AVAIL    -2
#define REF_NOT_IN_LIST -1    //intra
#define	REF_PIC_REORDER_DEFAULT	true

#define IS_INTRA4x4(type) ( MB_TYPE_INTRA4x4 == (type) )
#define IS_INTRA16x16(type) ( MB_TYPE_INTRA16x16 == (type) )
#define IS_INTRA(type) ((type)&MB_TYPE_INTRA)
#define IS_INTER(type) ((type)&MB_TYPE_INTER)

#define IS_SKIP(type) ( (type) == MB_TYPE_SKIP )
#define IS_SVC_INTER(type) ( IS_INTER(type) || IS_SKIP(type) )
#define IS_I_BL(type) ( (type) == MB_TYPE_INTRA_BL )
#define IS_SVC_INTRA(type) ( IS_I_BL(type) || IS_INTRA(type) )
#define IS_SUB8x8(type) ((type)&SUB_TYPE_8x8)
#define IS_Inter_8x8(type) ( (type) == MB_TYPE_8x8)



enum {
  Intra4x4			= 0,
  Intra16x16			= 1,
  Inter16x16			= 2,
  Inter16x8			= 3,
  Inter8x16			= 4,
  Inter8x8			= 5,
  PSkip				= 6
};


}
#endif//WELS_COMMON_BASIS_H__
