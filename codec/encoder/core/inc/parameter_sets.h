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

#ifndef WELS_PARAMETER_SETS_H__
#define WELS_PARAMETER_SETS_H__

#include "typedefs.h"
#include "wels_const.h"
#include "wels_common_basis.h"

namespace WelsSVCEnc {

/* Sequence Parameter Set, refer to Page 57 in JVT X201wcm */
typedef struct TagWelsSPS {
uint32_t	uiSpsId;
int16_t		iMbWidth;
int16_t		iMbHeight;
uint32_t	uiLog2MaxFrameNum;
//	uint32_t	uiPocType;
/* POC type 0 */
int32_t		iLog2MaxPocLsb;
/* POC type 1 */
//	int32_t		iOffsetForNonRefPic;

//	int32_t		iOffsetForTopToBottomField;
//	int32_t		iNumRefFramesInPocCycle;
//	int8_t		iOffsetForRefFrame[256];
SCropOffset	sFrameCrop;
int16_t		iNumRefFrames;
//	uint32_t	uiNumUnitsInTick;
//	uint32_t	uiTimeScale;

uint8_t		uiProfileIdc;
uint8_t		iLevelIdc;
//	uint8_t		uiChromaFormatIdc;
//	uint8_t		uiChromaArrayType;		//support =1

//	uint8_t		uiBitDepthLuma;         //=8, only used in decoder, encoder in general_***; it can be removed when removed general up_sample
//	uint8_t		uiBitDepthChroma;		//=8
/* TO BE CONTINUE: POC type 1 */
//	bool_t		bDeltaPicOrderAlwaysZeroFlag;
//	bool_t		bGapsInFrameNumValueAllowedFlag;	//=true

//	bool_t		bFrameMbsOnlyFlag;
//	bool_t		bMbaffFlag;	// MB Adapative Frame Field
//	bool_t		bDirect8x8InferenceFlag;
bool_t		bFrameCroppingFlag;

//	bool_t		bVuiParamPresentFlag;
//	bool_t		bTimingInfoPresentFlag;
//	bool_t		bFixedFrameRateFlag;

bool_t		bConstraintSet0Flag;
bool_t		bConstraintSet1Flag;
bool_t		bConstraintSet2Flag;

//	bool_t		bConstraintSet3Flag;		// reintroduce constrain_set3_flag instead of reserved filling bytes here
//	bool_t		bSeparateColorPlaneFlag;  // =false,: only used in decoder, encoder in general_***; it can be removed when removed general up_sample

} SWelsSPS, *PWelsSPS;


/* Sequence Parameter Set SVC extension syntax, refer to Page 391 in JVT X201wcm */
typedef struct TagSpsSvcExt {
//	SCropOffset	sSeqScaledRefLayer;

uint8_t		iExtendedSpatialScalability;	// ESS
//	uint8_t		uiChromaPhaseXPlus1Flag;
//	uint8_t		uiChromaPhaseYPlus1;
//	uint8_t		uiSeqRefLayerChromaPhaseXPlus1Flag;
//	uint8_t		uiSeqRefLayerChromaPhaseYPlus1;
//	bool_t		bInterLayerDeblockingFilterCtrlPresentFlag;
bool_t		bSeqTcoeffLevelPredFlag;
bool_t		bAdaptiveTcoeffLevelPredFlag;
bool_t		bSliceHeaderRestrictionFlag;
} SSpsSvcExt, *PSpsSvcExt;

/* Subset sequence parameter set syntax, refer to Page 391 in JVT X201wcm */
typedef struct TagSubsetSps {
SWelsSPS		pSps;
SSpsSvcExt	sSpsSvcExt;

//	bool_t		bSvcVuiParamPresentFlag;
//	bool_t		bAdditionalExtension2Flag;
//	bool_t		bAdditionalExtension2DataFlag;
} SSubsetSps, *PSubsetSps;

/* Picture parameter set syntax, refer to Page 59 in JVT X201wcm */
typedef struct TagWelsPPS {
uint32_t	iSpsId;
uint32_t	iPpsId;

#if !defined(DISABLE_FMO_FEATURE)
uint32_t	uiNumSliceGroups;
uint32_t	uiSliceGroupMapType;
/* uiSliceGroupMapType = 0 */
uint32_t	uiRunLength[MAX_SLICEGROUP_IDS];
/* uiSliceGroupMapType = 2 */
uint32_t	uiTopLeft[MAX_SLICEGROUP_IDS];
uint32_t	uiBottomRight[MAX_SLICEGROUP_IDS];
/* uiSliceGroupMapType = 3, 4 or 5 */
/* uiSliceGroupMapType = 3, 4 or 5 */
bool_t		bSliceGroupChangeDirectionFlag;
uint32_t	uiSliceGroupChangeRate;
/* uiSliceGroupMapType = 6 */
uint32_t	uiPicSizeInMapUnits;
uint32_t	uiSliceGroupId[MAX_SLICEGROUP_IDS];
#endif//!DISABLE_FMO_FEATURE

//	uint32_t	uiNumRefIdxL0Active;
//	uint32_t	uiNumRefIdxL1Active;

int8_t		iPicInitQp;
int8_t		iPicInitQs;
uint8_t		uiChromaQpIndexOffset;

/* potential application for High profile */
//	int32_t		iSecondChromaQpIndexOffset;
//	/* potential application for High profile */

//	bool_t		bPicOrderPresentFlag;

bool_t		bDeblockingFilterControlPresentFlag;

//	bool_t		bConstainedIntraPredFlag;
//	bool_t		bRedundantPicCntPresentFlag;
//	bool_t		bWeightedPredFlag;
//	uint8_t		uiWeightedBiPredIdc;

} SWelsPPS, *PWelsPPPS;

}

#endif //WELS_PARAMETER_SETS_H__
