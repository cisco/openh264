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

//wels_common_defs.h
#ifndef WELS_COMMON_DEFS_H__
#define WELS_COMMON_DEFS_H__

#include "typedefs.h"
#include "macros.h"



namespace WelsCommon {
/*common use table*/

#define LEVEL_NUMBER 17
typedef struct TagLevelLimits {
  uint8_t uiLevelIdc;  // level idc
  uint32_t uiMaxMBPS; // Max macroblock processing rate(MB/s)
  uint32_t uiMaxFS;   // Max frame sizea(MBs)
  uint32_t uiMaxDPBMbs;// Max decoded picture buffer size(MBs)
  uint32_t uiMaxBR; // Max video bit rate
  uint32_t uiMaxCPB; // Max CPB size
  int16_t iMinVmv; // Vertical MV component range upper bound
  int16_t iMaxVmv; // Vertical MV component range lower bound
  uint16_t uiMinCR;  // Min compression ration
  int16_t iMaxMvsPer2Mb; // Max number of motion vectors per two consecutive MBs
} SLevelLimits;

extern const SLevelLimits g_ksLevelLimits[LEVEL_NUMBER];
extern const uint8_t g_kuiMbCountScan4Idx[24];
extern const uint8_t g_kuiCache30ScanIdx[16];
extern const uint8_t g_kuiCache48CountScan4Idx[24];

extern const  ALIGNED_DECLARE (uint16_t, g_kuiDequantCoeff[52][8], 16);
extern const uint8_t g_kuiChromaQpTable[52];

/*
 *	NAL Unit Type (5 Bits)
 */
enum EWelsNalUnitType {
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
};

/*
 *	NAL Reference IDC (2 Bits)
 */

enum EWelsNalRefIdc {
  NRI_PRI_LOWEST	= 0,
  NRI_PRI_LOW		= 1,
  NRI_PRI_HIGH	= 2,
  NRI_PRI_HIGHEST	= 3
};

/*
 * VCL TYPE
 */

enum EVclType {
  NON_VCL			= 0,
  VCL				= 1,
  NOT_APP			= 2
};

/*
 *	vcl type map for given NAL unit type and corresponding H264 type (0: AVC; 1: SVC).
 */
extern const EVclType g_keTypeMap[32][2];

#define IS_VCL_NAL(t, ext_idx)			(g_keTypeMap[t][ext_idx] == VCL)
#define IS_PARAM_SETS_NALS(t)			( (t) == NAL_UNIT_SPS || (t) == NAL_UNIT_PPS || (t) == NAL_UNIT_SUBSET_SPS )
#define IS_SPS_NAL(t)					( (t) == NAL_UNIT_SPS )
#define IS_SUBSET_SPS_NAL(t)			( (t) == NAL_UNIT_SUBSET_SPS )
#define IS_PPS_NAL(t)					( (t) == NAL_UNIT_PPS )
#define IS_SEI_NAL(t)					( (t) == NAL_UNIT_SEI )
#define IS_PREFIX_NAL(t)				( (t) == NAL_UNIT_PREFIX )
#define IS_SUBSET_SPS_USED(t)			( (t) == NAL_UNIT_SUBSET_SPS || (t) == NAL_UNIT_CODED_SLICE_EXT )
#define IS_VCL_NAL_AVC_BASE(t)			( (t) == NAL_UNIT_CODED_SLICE || (t) == NAL_UNIT_CODED_SLICE_IDR )
#define IS_NEW_INTRODUCED_SVC_NAL(t)	( (t) == NAL_UNIT_PREFIX || (t) == NAL_UNIT_CODED_SLICE_EXT )


/* Base SSlice Types
 * Invalid in case of eSliceType exceeds 9,
 * Need trim when eSliceType > 4 as fixed SliceType(eSliceType-4),
 * meaning mapped version after eSliceType minus 4.
 */

enum EWelsSliceType {
  P_SLICE	= 0,
  B_SLICE	= 1,
  I_SLICE	= 2,
  SP_SLICE = 3,
  SI_SLICE = 4,
  UNKNOWN_SLICE = 5
};

/* SSlice Types in scalable extension */		;
enum ESliceTypeExt {
  EP_SLICE = 0,	// EP_SLICE: 0, 5
  EB_SLICE = 1,	// EB_SLICE: 1, 6
  EI_SLICE = 2	// EI_SLICE: 2, 7
};

/* List Index */
enum EListIndex {
  LIST_0	= 0,
  LIST_1	= 1,
  LIST_A	= 2
};



/* Motion Vector components */
enum EMvComp {
  MV_X	= 0,
  MV_Y	= 1,
  MV_A	= 2
};

/* Chroma Components */

enum EChromaComp {
  CHROMA_CB	= 0,
  CHROMA_CR	= 1,
  CHROMA_A	= 2
};



/*
 *	Memory Management Control Operation (MMCO) code
 */
enum EMmcoCode {
  MMCO_END			= 0,
  MMCO_SHORT2UNUSED	= 1,
  MMCO_LONG2UNUSED	= 2,
  MMCO_SHORT2LONG		= 3,
  MMCO_SET_MAX_LONG	= 4,
  MMCO_RESET			= 5,
  MMCO_LONG			= 6
};

/////////intra16x16  Luma
#define I16_PRED_INVALID   -1
#define I16_PRED_V       0
#define I16_PRED_H       1
#define I16_PRED_DC      2
#define I16_PRED_P       3

#define I16_PRED_DC_L    4
#define I16_PRED_DC_T    5
#define I16_PRED_DC_128  6
#define I16_PRED_DC_A  7
//////////intra4x4   Luma
#define I4_PRED_INVALID    0
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
#define I4_PRED_A   14

//////////intra Chroma
#define C_PRED_INVALID   -1
#define C_PRED_DC        0
#define C_PRED_H         1
#define C_PRED_V         2
#define C_PRED_P         3

#define C_PRED_DC_L      4
#define C_PRED_DC_T      5
#define C_PRED_DC_128    6
#define C_PRED_A    7
}
#endif//WELS_COMMON_DEFS_H__
