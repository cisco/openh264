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

namespace WelsDec {

// for data sharing cross modules and try to reduce size of binary generated

extern const uint8_t g_kuiChromaQp[52];

/*common use table*/
extern const uint8_t g_kuiScan8[24];
extern const uint8_t g_kuiLumaDcZigzagScan[16];
extern const uint8_t g_kuiChromaDcScan[4];
extern __align16 (const uint16_t, g_kuiDequantCoeff[52][8]);
/* Profile IDC */
typedef uint8_t		ProfileIdc;
enum {
PRO_BASELINE	= 66,
PRO_MAIN		= 77,
PRO_EXTENDED	= 88,
PRO_HIGH		= 100,
PRO_HIGH10		= 110,
PRO_HIGH422		= 122,
PRO_HIGH444		= 144,
PRO_CAVLC444	= 244,

PRO_SCALABLE_BASELINE	= 83,
PRO_SCALABLE_HIGH		= 86,
};

/*
 *	NAL Unit Type (5 Bits)
 */
typedef enum TagNalUnitType {
NAL_UNIT_UNSPEC_0			= 0,
NAL_UNIT_CODED_SLICE		= 1,
NAL_UNIT_CODED_SLICE_DPA	= 2,
NAL_UNIT_CODED_SLICE_DPB	= 3,
NAL_UNIT_CODED_SLICE_DPC	= 4,
NAL_UNIT_CODED_SLICE_IDR	= 5,
NAL_UNIT_SEI				= 6,
NAL_UNIT_SPS				= 7,
NAL_UNIT_PPS				= 8,
NAL_UNIT_AU_DELIMITER		= 9,
NAL_UNIT_END_OF_SEQ			= 10,
NAL_UNIT_END_OF_STR			= 11,
NAL_UNIT_FILLER_DATA		= 12,
NAL_UNIT_SPS_EXT			= 13,
NAL_UNIT_PREFIX				= 14,
NAL_UNIT_SUBSET_SPS			= 15,
NAL_UNIT_RESV_16			= 16,
NAL_UNIT_RESV_17			= 17,
NAL_UNIT_RESV_18			= 18,
NAL_UNIT_AUX_CODED_SLICE	= 19,
NAL_UNIT_CODED_SLICE_EXT	= 20,
NAL_UNIT_RESV_21			= 21,
NAL_UNIT_RESV_22			= 22,
NAL_UNIT_RESV_23			= 23,
NAL_UNIT_UNSPEC_24			= 24,
NAL_UNIT_UNSPEC_25			= 25,
NAL_UNIT_UNSPEC_26			= 26,
NAL_UNIT_UNSPEC_27			= 27,
NAL_UNIT_UNSPEC_28			= 28,
NAL_UNIT_UNSPEC_29			= 29,
NAL_UNIT_UNSPEC_30			= 30,
NAL_UNIT_UNSPEC_31			= 31
} ENalUnitType;

static const uint8_t g_kuiEmulationPreventionThreeByte	= 0x03;

/*
 *	NAL Reference IDC (2 Bits)
 */
typedef uint8_t		NalRefIdc;
enum {
NRI_PRI_LOWEST	= 0,
NRI_PRI_LOW		= 1,
NRI_PRI_HIGH	= 2,
NRI_PRI_HIGHEST	= 3
};

/*
 * VCL TYPE
 */
typedef uint8_t		VclType;
enum {
NON_VCL			= 0,
VCL				= 1,
NOT_APP			= 2
};

/*
 *	vcl type map for given NAL unit type and corresponding H264 type
 */
extern const VclType g_kuiVclTypeMap[32][2];

#define IS_VCL_NAL(t, ext_idx)			(g_kuiVclTypeMap[t][ext_idx] == VCL)
#define IS_PARAM_SETS_NALS(t)			( (t) == NAL_UNIT_SPS || (t) == NAL_UNIT_PPS || (t) == NAL_UNIT_SUBSET_SPS )
#define IS_SPS_NAL(t)					( (t) == NAL_UNIT_SPS )
#define IS_SUBSET_SPS_NAL(t)			( (t) == NAL_UNIT_SUBSET_SPS )
#define IS_PPS_NAL(t)					( (t) == NAL_UNIT_PPS )
#define IS_SEI_NAL(t)					( (t) == NAL_UNIT_SEI )
#define IS_PREFIX_NAL(t)				( (t) == NAL_UNIT_PREFIX )
#define IS_SUBSET_SPS_USED(t)			( (t) == NAL_UNIT_SUBSET_SPS || (t) == NAL_UNIT_CODED_SLICE_EXT )
#define IS_VCL_NAL_AVC_BASE(t)			( (t) == NAL_UNIT_CODED_SLICE || (t) == NAL_UNIT_CODED_SLICE_IDR )
#define IS_NEW_INTRODUCED_NAL(t)	( (t) == NAL_UNIT_PREFIX || (t) == NAL_UNIT_CODED_SLICE_EXT )

/* Base Slice Types
 * Invalid in case of eSliceType exceeds 9,
 * Need trim when eSliceType > 4 as fixed SliceType(eSliceType-4),
 * meaning mapped version after eSliceType minus 4.
 */
typedef enum TagSliceType {
P_SLICE	= 0,
B_SLICE	= 1,
I_SLICE	= 2,
SP_SLICE = 3,
SI_SLICE = 4,
UNKNOWN_SLICE = 5
} ESliceType;

/* List Index */
typedef uint8_t		ListIndex;
enum {
LIST_0	= 0,
LIST_1	= 1,
LIST_A	= 2
};

/* Picture Size */
typedef struct TagPictureSize {
int32_t	iWidth;
int32_t iHeight;
} SPictureSize;

/* Motion Vector components */
typedef uint8_t		MvComp;
enum {
MV_X	= 0,
MV_Y	= 1,
MV_A	= 2
};

/* Chroma Components */
typedef uint8_t		ChromaComp;
enum {
CHROMA_CB	= 0,
CHROMA_CR	= 1,
CHROMA_A	= 2
};

/* Position Offset structure */
typedef struct TagPosOffset {
int32_t	iLeftOffset;
int32_t	iTopOffset;
int32_t	iRightOffset;
int32_t	iBottomOffset;
} SPosOffset;

enum EMbPosition { //
MB_LEFT     = 0x01,	// A
MB_TOP      = 0x02,	// B
MB_TOPRIGHT = 0x04,	// C
MB_TOPLEFT	= 0x08,	// D,
MB_PRIVATE  = 0x10,
};
/* MB Type & Sub-MB Type */
typedef int32_t MbType;
typedef int32_t SubMbType;

#define MB_TYPE_INTRA4x4       0x01
#define MB_TYPE_INTRA16x16     0x02
#define MB_TYPE_INTRA8x8       0x03
#define MB_TYPE_INTRA_PCM      0x04

#define MB_TYPE_INTRA_BL       0x05// I_BL new MB type

#define MB_TYPE_16x16          0x06
#define MB_TYPE_16x8           0x07
#define MB_TYPE_8x16           0x08
#define MB_TYPE_8x8            0x09
#define MB_TYPE_8x8_REF0       0x0a

#define SUB_MB_TYPE_8x8        0x0b
#define SUB_MB_TYPE_8x4        0x0c
#define SUB_MB_TYPE_4x8        0x0d
#define SUB_MB_TYPE_4x4        0x0e
#define MB_TYPE_SKIP           0x0f
#define MB_TYPE_DIRECT2        0x10
#define not_available		   0x20

#define IS_INTRA4x4(type) ( MB_TYPE_INTRA4x4 == (type) )
#define IS_INTRA16x16(type) ( MB_TYPE_INTRA16x16 == (type) )
#define IS_INTRA(type) ( (type) > 0 && (type) < 5 )
#define IS_INTER(type) ( (type) > 5 && (type) < 16 )

#define IS_I_BL(type) ( (type) == MB_TYPE_INTRA_BL )
#define IS_SUB8x8(type) (MB_TYPE_8x8 == (type) || MB_TYPE_8x8_REF0 == (type))

/*
 *	Memory Management Control Operation (MMCO) code
 */
enum {
MMCO_END			= 0,
MMCO_SHORT2UNUSED	= 1,
MMCO_LONG2UNUSED	= 2,
MMCO_SHORT2LONG		= 3,
MMCO_SET_MAX_LONG	= 4,
MMCO_RESET			= 5,
MMCO_LONG			= 6
};

/////////intra16x16  Luma
#define I16_PRED_V       0
#define I16_PRED_H       1
#define I16_PRED_DC      2
#define I16_PRED_P       3

#define I16_PRED_DC_L    4
#define I16_PRED_DC_T    5
#define I16_PRED_DC_128  6
//////////intra4x4   Luma
#define I4_PRED_V        0
#define I4_PRED_H        1
#define I4_PRED_DC       2
#define I4_PRED_DDL      3 //diagonal_down_left
#define I4_PRED_DDR      4 //diagonal_down_right
#define I4_PRED_VR       5 //vertical_right
#define I4_PRED_HD       6 //horizon_down
#define I4_PRED_VL       7 //vertical_left
#define I4_PRED_HU       8 //horizon_up

#define I4_PRED_DC_L     9
#define I4_PRED_DC_T     10
#define I4_PRED_DC_128   11

#define I4_PRED_DDL_TOP  12 //right-top replacing by padding rightmost pixel of top
#define I4_PRED_VL_TOP   13 //right-top replacing by padding rightmost pixel of top

//////////intra Chroma
#define C_PRED_DC        0
#define C_PRED_H         1
#define C_PRED_V         2
#define C_PRED_P         3

#define C_PRED_DC_L      4
#define C_PRED_DC_T      5
#define C_PRED_DC_128    6

} // namespace WelsDec

#endif//WELS_COMMON_BASIS_H__
