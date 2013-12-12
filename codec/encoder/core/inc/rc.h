/*!
 * \copy
 *     Copyright (c)  2004-2013, Cisco Systems
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
 *  ratectl.c
 *
 *  Abstract
 *      Include file for ratectl.c
 *
 *  History
 *      9/8/2004 Created
 *    12/26/2011 Modified
 *
 *
 *************************************************************************/
#ifndef _RC_H
#define _RC_H


#include "codec_app_def.h"
#include "svc_enc_macroblock.h"
#include "slice.h"

namespace WelsSVCEnc {
//trace
#define GOM_TRACE_FLAG 1
//skip frame
#define SKIP_FRAME_FLAG      1

#define    WELS_RC_DISABLE        0
#define    WELS_RC_GOM            1

typedef enum
{
	RC_MODE0,	//Quality mode
	RC_MODE1,   //Bitrate mode
}RC_MODES;

enum {
	//virtual gop size
	VGOP_SIZE             = 8,

	//qp information
	GOM_MIN_QP_MODE       = 12,
	GOM_MAX_QP_MODE       = 36,
    MIN_IDR_QP            = 26,
    MAX_IDR_QP            = 32,
    DELTA_QP              = 2,
    DELTA_QP_BGD_THD      = 3,

	//frame skip constants
    SKIP_QP_90P           = 24,
    SKIP_QP_180P          = 24,
    SKIP_QP_360P          = 31,
    SKIP_QP_720P          = 31,
    LAST_FRAME_QP_RANGE_UPPER_MODE0  = 3,
	LAST_FRAME_QP_RANGE_LOWER_MODE0  = 2,
    LAST_FRAME_QP_RANGE_UPPER_MODE1  = 5,
	LAST_FRAME_QP_RANGE_LOWER_MODE1  = 3,

	MB_WIDTH_THRESHOLD_90P   = 15,
	MB_WIDTH_THRESHOLD_180P  = 30,
	MB_WIDTH_THRESHOLD_360P  = 60,

	//Mode 0 parameter
	GOM_ROW_MODE0_90P     = 2,
	GOM_ROW_MODE0_180P    = 2,
	GOM_ROW_MODE0_360P    = 4,
	GOM_ROW_MODE0_720P    = 4,
    QP_RANGE_MODE0        = 3,

	//Mode 1 parameter
	GOM_ROW_MODE1_90P     = 1,
	GOM_ROW_MODE1_180P    = 1,
	GOM_ROW_MODE1_360P    = 2,
	GOM_ROW_MODE1_720P    = 2,
    QP_RANGE_UPPER_MODE1  = 9,
	QP_RANGE_LOWER_MODE1  = 4,
    QP_RANGE_INTRA_MODE1  = 3,
};

//bits allocation
#define MAX_BITS_VARY_PERCENTAGE 100 //bits vary range in percentage
#define VGOP_BITS_PERCENTAGE_DIFF 5
#define IDR_BITRATE_RATIO  4.0
#define FRAME_iTargetBits_VARY_RANGE 0.5
//R-Q Model
#define LINEAR_MODEL_DECAY_FACTOR 0.8
#define FRAME_CMPLX_RATIO_RANGE 0.1
#define SMOOTH_FACTOR_MIN_VALUE 0.02
//#define VGOP_BITS_MIN_RATIO 0.8
//skip and padding
#define SKIP_RATIO  0.5
#define PADDING_BUFFER_RATIO 0.5
#define PADDING_THRESHOLD    0.05

typedef struct TagRCSlicing
{
	int32_t   iComplexityIndexSlice;
	int32_t   iCalculatedQpSlice;
	int32_t   iStartMbSlice;
	int32_t   iEndMbSlice;
	int32_t   iTotalQpSlice;
	int32_t   iTotalMbSlice;
	int32_t   iTargetBitsSlice;
	int32_t   iBsPosSlice;
	int32_t   iFrameBitsSlice;
	int32_t   iGomBitsSlice;
	int32_t   iGomTargetBits;
	//int32_t   gom_coded_mb;
} SRCSlicing;

typedef struct TagRCTemporal
{
	int32_t   iMinBitsTl;
	int32_t   iMaxBitsTl;
	double    dTlayerWeight;
	int32_t   iGopBitsDq;
	//P frame level R-Q Model
	double    dLinearCmplx;
	int32_t   iPFrameNum;
	int32_t   iFrameCmplxMean;

} SRCTemporal;

typedef struct TagWelsRc{
	int32_t   iRcVaryPercentage;
	double    dRcVaryRatio;

	int32_t   iInitialQp; //initial qp
	int32_t   iBitRate;
	int32_t   iPreviousBitrate;
	int32_t   iPreviousGopSize;
	double    fFrameRate;
	double    dBitsPerFrame;
	double    dPreviousFps;

	// bits allocation and status
	int32_t   iRemainingBits;
	int32_t   iTargetBits;

	int32_t   iIdrNum;
	int32_t   iIntraComplexity;
	int32_t   iIntraMbCount;

	int8_t    iTlOfFrames[VGOP_SIZE];
	double    dRemainingWeights;
	int32_t   iFrameDqBits;

	double    *pGomComplexity;
	int32_t	  *pGomForegroundBlockNum;
	int32_t   *pCurrentFrameGomSad;
	int32_t   *pGomCost;

	int32_t   iAverageFrameQp;
	int32_t   iNumberMbFrame;
	int32_t   iNumberMbGom;
	int32_t	  iSliceNum;
	int32_t   iGomSize;

	int32_t   iSkipFrameNum;
	int32_t   iFrameCodedInVGop;
	int32_t   iSkipFrameInVGop;
	int32_t   iGopNumberInVGop;
	int32_t   iGopIndexInVGop;

	int32_t   iSkipQpValue;
	int32_t   iQpRangeUpperInFrame;
	int32_t   iQpRangeLowerInFrame;
	int32_t   iMinQp;
	int32_t   iMaxQp;
	//int32_t   delta_adaptive_qp;
	double    dSkipBufferRatio;

	double    dQStep;
	int32_t   iFrameDeltaQpUpper;
	int32_t   iFrameDeltaQpLower;
	int32_t   iLastCalculatedQScale;

	//for skip frame and padding
	int32_t   iBufferSizeSkip;
	int32_t   iBufferFullnessSkip;
	int32_t   iBufferSizePadding;
	int32_t   iBufferFullnessPadding;
	int32_t   iPaddingSize;
	int32_t   iPaddingBitrateStat;

	SRCSlicing	*pSlicingOverRc;
	SRCTemporal *pTemporalOverRc;
}SWelsSvcRc;

typedef  void (*PWelsRCPictureInitFunc) (void *pCtx);
typedef  void (*PWelsRCPictureInfoUpdateFunc) (void *pCtx, int32_t iLayerSize);
typedef  void (*PWelsRCMBInfoUpdateFunc)(void *pCtx, SMB * pCurMb, int32_t iCostLuma, SSlice *pSlice);
typedef  void (*PWelsRCMBInitFunc)(void *pCtx, SMB * pCurMb, SSlice *pSlice);

typedef  struct  WelsRcFunc_s
{
    PWelsRCPictureInitFunc			pfWelsRcPictureInit;
	PWelsRCPictureInfoUpdateFunc	pfWelsRcPictureInfoUpdate;
	PWelsRCMBInitFunc				pfWelsRcMbInit;
	PWelsRCMBInfoUpdateFunc			pfWelsRcMbInfoUpdate;
} SWelsRcFunc;

void WelsRcInitModule(void *pCtx,  int32_t iModule);
void WelsRcFreeMemory(void *pCtx);

}
#endif //_RC_H
