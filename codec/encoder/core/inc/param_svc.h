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
 * \file	param_svc.h
 *
 * \brief	Configurable parameters in H.264/SVC Encoder
 *
 * \date	4/20/2009 Created
 *
 *************************************************************************************
 */
#if !defined(WELS_ENCODER_PARAMETER_SVC_H__)
#define WELS_ENCODER_PARAMETER_SVC_H__

#include <string.h>
#include <math.h>
#include "typedefs.h"
#include "codec_def.h"
#include "macros.h"
#include "wels_const.h"
#include "wels_common_basis.h"
#include "rc.h"
#include "svc_enc_slice_segment.h"
#include "as264_common.h"

namespace WelsSVCEnc {

#define   INVALID_TEMPORAL_ID   ((uint8_t)0xff)

extern const uint8_t   g_kuiTemporalIdListTable[MAX_TEMPORAL_LEVEL][MAX_GOP_SIZE + 1];

/*!
* \brief	get Logarithms base 2 of (upper/base)
* \param	base	based scaler
* \param	upper	input upper value
* \return	2 based scaling factor
*/
static inline uint32_t GetLogFactor (float base, float upper) {
const double dLog2factor	= log10 (1.0 * upper / base) / log10 (2.0);
const double dEpsilon		= 0.0001;
const double dRound		= floor (dLog2factor + 0.5);

if (dLog2factor < dRound + dEpsilon && dRound < dLog2factor + dEpsilon) {
  return (uint32_t) (dRound);
}
return UINT_MAX;
}

/*
 *	Dependency Layer Parameter
 */
typedef struct TagDLayerParam {
int32_t		iActualWidth;			// input source picture actual width
int32_t		iActualHeight;			// input source picture actual height
int32_t		iFrameWidth;			// frame width
int32_t		iFrameHeight;			// frame height

int32_t		iSpatialBitrate;

/* temporal settings related */
int32_t		iTemporalResolution;
int32_t		iDecompositionStages;
uint8_t     uiCodingIdx2TemporalId[ (1 << MAX_TEMPORAL_LEVEL) + 1];

uint8_t		uiProfileIdc;			// value of profile IDC (0 for auto-detection)

int8_t		iHighestTemporalId;
//	uint8_t		uiDependencyId;
int8_t      iDLayerQp;

SMulSliceOption sMso;	// multiple slice options

float		fInputFrameRate;		// input frame rate
float		fOutputFrameRate;		// output frame rate

#ifdef ENABLE_FRAME_DUMP
char		sRecFileName[MAX_FNAME_LEN];	// file to be constructed
#endif//ENABLE_FRAME_DUMP
} SDLayerParam;

/*
 *	Cisco OpenH264 Encoder Parameter Configuration
 */
typedef struct TagWelsSvcCodingParam {
SDLayerParam	sDependencyLayers[MAX_DEPENDENCY_LAYER];

/* General */
uint32_t	uiGopSize;			// GOP size (at maximal frame rate: 16)
uint32_t	uiIntraPeriod;		// intra period (multiple of GOP size as desired)
int32_t		iNumRefFrame;		// number of reference frame used

int32_t     iActualPicWidth;    //   actual input picture width
int32_t     iActualPicHeight;   //   actual input picture height

struct {
  int32_t iLeft;
  int32_t iTop;
  int32_t iWidth;
  int32_t iHeight;
} SUsedPicRect;	// the rect in input picture that encoder actually used

char*       pCurPath; // record current lib path such as:/pData/pData/com.wels.enc/lib/

float		fMaxFrameRate;		// maximal frame rate [Hz / fps]
int32_t		iInputCsp;			// color space of input sequence
uint32_t	uiFrameToBeCoded;	// frame to be encoded (at input frame rate)

int32_t		iTargetBitrate;			// overall target bitrate introduced in RC module
int16_t		iMultipleThreadIdc;		// 1	# 0: auto(dynamic imp. internal encoder); 1: multiple threads imp. disabled; > 1: count number of threads;
int16_t		iCountThreadsNum;			//		# derived from disable_multiple_slice_idc (=0 or >1) means;

int32_t		iLTRRefNum;
uint32_t    uiLtrMarkPeriod;	//the min distance of two int32_t references

bool		bDeblockingParallelFlag;	// deblocking filter parallelization control flag
bool		bMgsT0OnlyStrategy; //MGS_T0_only_strategy
bool		bEnableSSEI;
bool		bEnableFrameCroppingFlag;	// enable frame cropping flag: true alwayse in application

bool		bEnableCropPic;			// enable cropping source picture. , 8/25/2010
// false: Streaming Video Sharing; true: Video Conferencing Meeting;
int8_t		iDecompStages;		// GOP size dependency

/* Deblocking loop filter */
int8_t		iLoopFilterDisableIdc;	// 0: on, 1: off, 2: on except for slice boundaries
int8_t		iLoopFilterAlphaC0Offset;// AlphaOffset: valid range [-6, 6], default 0

int8_t		iLoopFilterBetaOffset;	// BetaOffset:	valid range [-6, 6], default 0
int8_t		iInterLayerLoopFilterDisableIdc; // Employed based upon inter-layer, same comment as above
int8_t		iInterLayerLoopFilterAlphaC0Offset;	// InterLayerLoopFilterAlphaC0Offset
int8_t		iInterLayerLoopFilterBetaOffset;	// InterLayerLoopFilterBetaOffset

/* Rate Control */
bool		bEnableRc;
int8_t		iRCMode;
int8_t		iPaddingFlag;
/* denoise control */
bool      bEnableDenoise;

/* scene change detection control */
bool      bEnableSceneChangeDetect;
// background detection control
bool		bEnableBackgroundDetection;
/* adaptive quantization control */
bool		bEnableAdaptiveQuant;
/* frame skipping */
bool		bEnableFrameSkip;
/* long term reference control */
bool      bEnableLongTermReference;

/* pSps pPps id addition control */
bool      bEnableSpsPpsIdAddition;
/* Layer definition */
bool		bPrefixNalAddingCtrl;
int8_t		iNumDependencyLayer;	// number of dependency(Spatial/CGS) layers used to be encoded
int8_t		iNumTemporalLayer;		// number of temporal layer specified



 public:
TagWelsSvcCodingParam (const bool kbEnableRc = true) {
  FillDefault (kbEnableRc);
}
~TagWelsSvcCodingParam()	{}

void FillDefault (const bool kbEnableRc) {
  uiGopSize			= 1;			// GOP size (at maximal frame rate: 16)
  uiIntraPeriod		= 0;			// intra period (multiple of GOP size as desired)
  iNumRefFrame		= MIN_REF_PIC_COUNT;	// number of reference frame used

  iActualPicWidth	= 0;    //   actual input picture width
  iActualPicHeight	= 0;	//   actual input picture height
  SUsedPicRect.iLeft	=
    SUsedPicRect.iTop	=
      SUsedPicRect.iWidth	=
        SUsedPicRect.iHeight = 0;	// the rect in input picture that encoder actually used

  pCurPath			= NULL; // record current lib path such as:/pData/pData/com.wels.enc/lib/

  fMaxFrameRate		= MAX_FRAME_RATE;	// maximal frame rate [Hz / fps]
  iInputCsp			= videoFormatI420;	// input sequence color space in default
  uiFrameToBeCoded	= (uint32_t) - 1;		// frame to be encoded (at input frame rate)

  iTargetBitrate			= 0;	// overall target bitrate introduced in RC module
  bDeblockingParallelFlag = false;	// deblocking filter parallelization control flag
#ifdef MT_ENABLED
  iMultipleThreadIdc		= 0;	// auto to detect cpu cores inside
#else
  iMultipleThreadIdc		=
    1;	// 1 # 0: auto(dynamic imp. internal encoder); 1: multiple threads imp. disabled; > 1: count number of threads;
#endif//MT_ENABLED
  iCountThreadsNum		= 1;	//		# derived from disable_multiple_slice_idc (=0 or >1) means;

  iLTRRefNum				= 0;
  uiLtrMarkPeriod			= 30;	//the min distance of two int32_t references

  bMgsT0OnlyStrategy			=
    true;	// Strategy of have MGS only at T0 frames (0: do not use this strategy; 1: use this strategy)
  bEnableSSEI					= true;
  bEnableFrameCroppingFlag	= true;	// enable frame cropping flag: true alwayse in application
  bEnableCropPic				= true;	// enable cropping source picture. , 8/25/2010
  // false: Streaming Video Sharing; true: Video Conferencing Meeting;
  iDecompStages				= 0;	// GOP size dependency, unknown here and be revised later

  /* Deblocking loop filter */
  iLoopFilterDisableIdc		= 1;	// 0: on, 1: off, 2: on except for slice boundaries
  iLoopFilterAlphaC0Offset	= 0;	// AlphaOffset: valid range [-6, 6], default 0
  iLoopFilterBetaOffset		= 0;	// BetaOffset:	valid range [-6, 6], default 0
  iInterLayerLoopFilterDisableIdc		= 1;	// Employed based upon inter-layer, same comment as above
  iInterLayerLoopFilterAlphaC0Offset	= 0;	// InterLayerLoopFilterAlphaC0Offset
  iInterLayerLoopFilterBetaOffset		= 0;	// InterLayerLoopFilterBetaOffset

  /* Rate Control */
  bEnableRc		= kbEnableRc;
  iRCMode			= 0;
  iPaddingFlag	= 0;

  bEnableDenoise				= false;	// denoise control
  bEnableSceneChangeDetect	= true;		// scene change detection control
  bEnableBackgroundDetection	= true;		// background detection control
  bEnableAdaptiveQuant		= true;		// adaptive quantization control
  bEnableFrameSkip		= true;		// frame skipping
  bEnableLongTermReference	= false;	// long term reference control
  bEnableSpsPpsIdAddition	= true;		// pSps pPps id addition control
  bPrefixNalAddingCtrl		= true;		// prefix NAL adding control
  iNumDependencyLayer		= 0;		// number of dependency(Spatial/CGS) layers used to be encoded
  iNumTemporalLayer			= 0;		// number of temporal layer specified

  memset(sDependencyLayers,0,sizeof(SDLayerParam)*MAX_DEPENDENCY_LAYER);
}

int32_t ParamTranscode (SVCEncodingParam& pCodingParam, const bool kbEnableRc = true) {
  pCodingParam.fFrameRate		= WELS_CLIP3 (pCodingParam.fFrameRate, MIN_FRAME_RATE, MAX_FRAME_RATE);
  iInputCsp		= pCodingParam.iInputCsp;		// color space of input sequence
  uiFrameToBeCoded	= (uint32_t) -
                      1;		// frame to be encoded (at input frame rate), -1 dependents on length of input sequence

  iActualPicWidth   = pCodingParam.iPicWidth;
  iActualPicHeight  = pCodingParam.iPicHeight;

  SUsedPicRect.iLeft = 0;
  SUsedPicRect.iTop  = 0;
  SUsedPicRect.iWidth = ((iActualPicWidth >> 1) << 1);
  SUsedPicRect.iHeight = ((iActualPicHeight >> 1) << 1);

  /* Deblocking loop filter */
#ifdef MT_ENABLED
  iLoopFilterDisableIdc	= 2;//pCodingParam.iLoopFilterDisableIdc;	// 0: on, 1: off, 2: on except for slice boundaries,
#else
  iLoopFilterDisableIdc	= 0;	// 0: on, 1: off, 2: on except for slice boundaries
#endif
  iLoopFilterAlphaC0Offset = 0;	// AlphaOffset: valid range [-6, 6], default 0
  iLoopFilterBetaOffset	= 0;	// BetaOffset:	valid range [-6, 6], default 0
  iInterLayerLoopFilterDisableIdc	= iLoopFilterDisableIdc;	// Employed based upon inter-layer, same comment as above
  iInterLayerLoopFilterAlphaC0Offset = 0;
  iInterLayerLoopFilterBetaOffset	= 0;

  bEnableFrameCroppingFlag	= true;

  /* Rate Control */
  bEnableRc			= kbEnableRc;
  if (pCodingParam.iRCMode != RC_MODE0 && pCodingParam.iRCMode != RC_MODE1)
    iRCMode = RC_MODE1;
  else
    iRCMode = pCodingParam.iRCMode;    // rc mode
  iPaddingFlag = pCodingParam.iPaddingFlag;

  iTargetBitrate		= pCodingParam.iTargetBitrate;	// target bitrate

  /* Denoise Control */
  bEnableDenoise = pCodingParam.bEnableDenoise ? true : false;    // Denoise Control  // only support 0 or 1 now

  /* Scene change detection control */
  bEnableSceneChangeDetect	= true;

  /* Background detection Control */
  bEnableBackgroundDetection = pCodingParam.bEnableBackgroundDetection ? true : false;

  /* Adaptive quantization control */
  bEnableAdaptiveQuant	= pCodingParam.bEnableAdaptiveQuant ? true : false;

  /* Frame skipping */
  bEnableFrameSkip	= pCodingParam.bEnableFrameSkip ? true : false;

  /* Enable cropping source picture */
  bEnableCropPic	= pCodingParam.bEnableCropPic ? true : false;

  /* Enable int32_t term reference */
  bEnableLongTermReference	= pCodingParam.bEnableLongTermReference ? true : false;
  uiLtrMarkPeriod = pCodingParam.iLtrMarkPeriod;

  /* For ssei information */
  bEnableSSEI		= true;

  /* Layer definition */
  iNumDependencyLayer	= (int8_t)WELS_CLIP3 (pCodingParam.iSpatialLayerNum, 1,
                        MAX_DEPENDENCY_LAYER); // number of dependency(Spatial/CGS) layers used to be encoded
  pCodingParam.iTemporalLayerNum = (int8_t)WELS_CLIP3 (pCodingParam.iTemporalLayerNum, 1,
                                   MAX_TEMPORAL_LEVEL);	// safe valid iTemporalLayerNum
  iNumTemporalLayer		= (int8_t)
                        pCodingParam.iTemporalLayerNum;//(int8_t)WELS_CLIP3(pCodingParam.iTemporalLayerNum, 1, MAX_TEMPORAL_LEVEL);// number of temporal layer specified

  uiGopSize			= 1 << (iNumTemporalLayer - 1);	// Override GOP size based temporal layer
  iDecompStages		= iNumTemporalLayer - 1;	// WELS_LOG2( uiGopSize );// GOP size dependency
  uiIntraPeriod		= pCodingParam.iIntraPeriod;// intra period (multiple of GOP size as desired)
  if (uiIntraPeriod == (uint32_t) (-1))
    uiIntraPeriod = 0;
  else if (uiIntraPeriod & uiGopSize)	// none multiple of GOP size
    uiIntraPeriod = ((uiIntraPeriod + uiGopSize - 1) / uiGopSize) * uiGopSize;

  iLTRRefNum = bEnableLongTermReference ? LONG_TERM_REF_NUM : 0;
  iNumRefFrame		= ((uiGopSize >> 1) > 1) ? ((uiGopSize >> 1) + iLTRRefNum) : (MIN_REF_PIC_COUNT + iLTRRefNum);
  iNumRefFrame		= WELS_CLIP3 (iNumRefFrame, MIN_REF_PIC_COUNT, MAX_REFERENCE_PICTURE_COUNT_NUM);

  uiLtrMarkPeriod  = pCodingParam.iLtrMarkPeriod;

  bPrefixNalAddingCtrl	= pCodingParam.bPrefixNalAddingCtrl;

  bEnableSpsPpsIdAddition =
    pCodingParam.bEnableSpsPpsIdAddition;//For SVC meeting application, to avoid mosaic issue caused by cross-IDR reference.
  //SHOULD enable this feature.

  SDLayerParam* pDlp		= &sDependencyLayers[0];
  float fMaxFr			= .0f;
  uint8_t uiProfileIdc		= PRO_BASELINE;
  int8_t iIdxSpatial	= 0;
  while (iIdxSpatial < iNumDependencyLayer) {
    pDlp->uiProfileIdc		= uiProfileIdc;

    pCodingParam.sSpatialLayers[iIdxSpatial].fFrameRate	= WELS_CLIP3 (pCodingParam.sSpatialLayers[iIdxSpatial].fFrameRate,
        MIN_FRAME_RATE, pCodingParam.fFrameRate);
    pDlp->fInputFrameRate	=
      pDlp->fOutputFrameRate	= WELS_CLIP3 (pCodingParam.sSpatialLayers[iIdxSpatial].fFrameRate, MIN_FRAME_RATE,
                                            MAX_FRAME_RATE);
    if (pDlp->fInputFrameRate > fMaxFr + EPSN)
      fMaxFr = pDlp->fInputFrameRate;

#ifdef ENABLE_FRAME_DUMP
    pDlp->sRecFileName[0]	= '\0';	// file to be constructed
#endif//ENABLE_FRAME_DUMP
    pDlp->iFrameWidth		= pCodingParam.sSpatialLayers[iIdxSpatial].iVideoWidth;	// frame width
    pDlp->iFrameHeight		= pCodingParam.sSpatialLayers[iIdxSpatial].iVideoHeight;// frame height
    pDlp->iSpatialBitrate	=
      pCodingParam.sSpatialLayers[iIdxSpatial].iSpatialBitrate;	// target bitrate for current spatial layer


    //multi slice
    pDlp->sMso.uiSliceMode = (SliceMode)pCodingParam.sSpatialLayers[iIdxSpatial].sSliceCfg.uiSliceMode;
    pDlp->sMso.sSliceArgument.uiSliceSizeConstraint
      = (uint32_t) (pCodingParam.sSpatialLayers[iIdxSpatial].sSliceCfg.sSliceArgument.uiSliceSizeConstraint);
    pDlp->sMso.sSliceArgument.iSliceNum
      = pCodingParam.sSpatialLayers[iIdxSpatial].sSliceCfg.sSliceArgument.uiSliceNum;
    const int32_t kiLesserSliceNum = ((MAX_SLICES_NUM < MAX_SLICES_NUM_TMP) ? MAX_SLICES_NUM : MAX_SLICES_NUM_TMP);
    memcpy (pDlp->sMso.sSliceArgument.uiSliceMbNum,
            pCodingParam.sSpatialLayers[iIdxSpatial].sSliceCfg.sSliceArgument.uiSliceMbNum,	// confirmed_safe_unsafe_usage
            kiLesserSliceNum * sizeof (uint32_t)) ;

    pDlp->iDLayerQp = SVC_QUALITY_BASE_QP;

    uiProfileIdc	= PRO_SCALABLE_BASELINE;
    ++ pDlp;
    ++ iIdxSpatial;
  }

  fMaxFrameRate	= fMaxFr;

  SetActualPicResolution();

  return 0;
}

// assuming that the width/height ratio of all spatial layers are the same

void SetActualPicResolution() {
  int32_t iSpatialIdx			= iNumDependencyLayer - 1;
  SDLayerParam* pDlayer		= &sDependencyLayers[iSpatialIdx];

  for (; iSpatialIdx >= 0; iSpatialIdx --) {
    pDlayer	= &sDependencyLayers[iSpatialIdx];

    pDlayer->iActualWidth = pDlayer->iFrameWidth;
    pDlayer->iActualHeight = pDlayer->iFrameHeight;
    pDlayer->iFrameWidth = WELS_ALIGN (pDlayer->iActualWidth, MB_WIDTH_LUMA);
    pDlayer->iFrameHeight = WELS_ALIGN (pDlayer->iActualHeight, MB_HEIGHT_LUMA);
  }
}

/*!
* \brief	determined key coding tables for temporal scalability, uiProfileIdc etc for each spatial layer settings
* \param	SWelsSvcCodingParam, and carried with known GOP size, max, input and output frame rate of each spatial
* \return	NONE (should ensure valid parameter before this procedure)
*/
void DetermineTemporalSettings() {
  const int32_t iDecStages		= WELS_LOG2 (
                                  uiGopSize);	// (int8_t)GetLogFactor(1.0f, 1.0f * pcfg->uiGopSize);	//log2(uiGopSize)
  const uint8_t* pTemporalIdList	= &g_kuiTemporalIdListTable[iDecStages][0];
  SDLayerParam* pDlp				= &sDependencyLayers[0];
  uint8_t uiProfileIdc				= PRO_BASELINE;
  int8_t i						= 0;

  while (i < iNumDependencyLayer) {
    const uint32_t kuiLogFactorInOutRate	= GetLogFactor (pDlp->fOutputFrameRate, pDlp->fInputFrameRate);
    const uint32_t kuiLogFactorMaxInRate	= GetLogFactor (pDlp->fInputFrameRate, fMaxFrameRate);
    int32_t iNotCodedMask = 0;
    int8_t iMaxTemporalId = 0;

    memset (pDlp->uiCodingIdx2TemporalId, INVALID_TEMPORAL_ID, sizeof (pDlp->uiCodingIdx2TemporalId));
    pDlp->uiProfileIdc = uiProfileIdc;	// PRO_BASELINE, PRO_SCALABLE_BASELINE;

    iNotCodedMask	= (1 << (kuiLogFactorInOutRate + kuiLogFactorMaxInRate)) - 1;
    for (uint32_t uiFrameIdx = 0; uiFrameIdx <= uiGopSize; ++ uiFrameIdx) {
      if (0 == (uiFrameIdx & iNotCodedMask)) {
        const int8_t kiTemporalId = pTemporalIdList[uiFrameIdx];
        pDlp->uiCodingIdx2TemporalId[uiFrameIdx] = kiTemporalId;
        if (kiTemporalId > iMaxTemporalId) {
          iMaxTemporalId = kiTemporalId;
        }
      }
    }

    pDlp->iHighestTemporalId	= iMaxTemporalId;
    pDlp->iTemporalResolution	= kuiLogFactorMaxInRate + kuiLogFactorInOutRate;
    pDlp->iDecompositionStages	= iDecStages - kuiLogFactorMaxInRate - kuiLogFactorInOutRate;

    uiProfileIdc	= PRO_SCALABLE_BASELINE;
    ++ pDlp;
    ++ i;
  }
  iDecompStages = (int8_t)iDecStages;
}

} SWelsSvcCodingParam;

static inline int32_t FreeCodingParam (SWelsSvcCodingParam** pParam, CMemoryAlign* pMa) {
if (pParam == NULL || *pParam == NULL || pMa == NULL)
  return 1;
pMa->WelsFree (*pParam, "SWelsSvcCodingParam");
*pParam = NULL;
return 0;
}

static inline int32_t AllocCodingParam (SWelsSvcCodingParam** pParam, CMemoryAlign* pMa,
                                        const int32_t kiRequestNumSpatial) {
if (pParam == NULL || pMa == NULL || kiRequestNumSpatial < 1 || kiRequestNumSpatial > MAX_SPATIAL_LAYER_NUM)
  return 1;
if (*pParam != NULL) {
  FreeCodingParam (pParam, pMa);
}
SWelsSvcCodingParam* pCodingParam = (SWelsSvcCodingParam*)pMa->WelsMalloc (sizeof (SWelsSvcCodingParam),
                                    "SWelsSvcCodingParam");
if (NULL == pCodingParam)
  return 1;
*pParam = pCodingParam;
return 0;
}

}//end of namespace WelsSVCEnc

#endif//WELS_ENCODER_PARAMETER_SVC_H__
