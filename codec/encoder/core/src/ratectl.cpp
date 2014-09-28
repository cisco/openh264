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
 *  ratectl.c
 *
 *  Abstract
 *      Rate Control
 *
 *  History
 *      9/8/2009 Created
 *    12/26/2011 Modified
 *
 *
 *
 *************************************************************************/
#include "rc.h"
#include "encoder_context.h"
#include "utils.h"
#include "svc_enc_golomb.h"


namespace WelsEnc {


#define VIRTUAL_BUFFER_LOW_TH   120 //*INT_MULTIPLY
#define VIRTUAL_BUFFER_HIGH_TH  180 //*INT_MULTIPLY

//#define _TEST_TEMP_RC_
#ifdef _TEST_TEMP_RC_
//#define _NOT_USE_AQ_FOR_TEST_
FILE* fp_test_rc = NULL;
FILE* fp_vgop = NULL;
#endif
#define _BITS_RANGE 0
const int32_t g_kiQpToQstepTable[52] = {   63,   71,   79,   89,  100,  112,  126,  141,  159,  178,
                                           200,  224,  252,  283,  317,  356,  400,  449,  504,  566,
                                           635,  713,  800,  898, 1008, 1131, 1270, 1425, 1600, 1796,
                                           2016, 2263, 2540, 2851, 3200, 3592, 4032, 4525, 5080, 5702,
                                           6400, 7184, 8063, 9051, 10159, 11404, 12800, 14368, 16127, 18102,
                                           20319, 22807
                                       }; //WELS_ROUND(INT_MULTIPLY*pow (2.0, (iQP - 4.0) / 6.0))

void RcInitLayerMemory (SWelsSvcRc* pWelsSvcRc, CMemoryAlign* pMA, const int32_t kiMaxTl) {
  const int32_t kiSliceNum			= pWelsSvcRc->iSliceNum;
  const int32_t kiGomSize				= pWelsSvcRc->iGomSize;
  const int32_t kiGomSizeD			= kiGomSize * sizeof (double);
  const int32_t kiGomSizeI			= kiGomSize * sizeof (int32_t);
  const int32_t kiLayerRcSize			= kiGomSizeD + (kiGomSizeI * 3) +  sizeof (SRCTemporal) * kiMaxTl;
  uint8_t* pBaseMem					= (uint8_t*)pMA->WelsMalloc (kiLayerRcSize, "pWelsSvcRc->pTemporalOverRc");

  if (NULL == pBaseMem)
    return;

  pWelsSvcRc->pTemporalOverRc			= (SRCTemporal*)pBaseMem;
  pBaseMem += sizeof (SRCTemporal) * kiMaxTl;
  pWelsSvcRc->pGomComplexity				= (double*)pBaseMem;
  pBaseMem += kiGomSizeD;
  pWelsSvcRc->pGomForegroundBlockNum	= (int32_t*)pBaseMem;
  pBaseMem += kiGomSizeI;
  pWelsSvcRc->pCurrentFrameGomSad		= (int32_t*)pBaseMem;
  pBaseMem += kiGomSizeI;
  pWelsSvcRc->pGomCost					= (int32_t*)pBaseMem;

  pWelsSvcRc->pSlicingOverRc			= (SRCSlicing*)pMA->WelsMalloc (sizeof (SRCSlicing) * kiSliceNum, "SlicingOverRC");
}

void RcFreeLayerMemory (SWelsSvcRc* pWelsSvcRc, CMemoryAlign* pMA) {
  if (pWelsSvcRc != NULL && pWelsSvcRc->pSlicingOverRc != NULL) {
    pMA->WelsFree (pWelsSvcRc->pSlicingOverRc, "SlicingOverRC");
    pWelsSvcRc->pSlicingOverRc = NULL;
  }
  if (pWelsSvcRc != NULL && pWelsSvcRc->pTemporalOverRc != NULL) {
    pMA->WelsFree (pWelsSvcRc->pTemporalOverRc, "pWelsSvcRc->pTemporalOverRc");
    pWelsSvcRc->pTemporalOverRc         = NULL;
    pWelsSvcRc->pGomComplexity			= NULL;
    pWelsSvcRc->pGomForegroundBlockNum	= NULL;
    pWelsSvcRc->pCurrentFrameGomSad	= NULL;
    pWelsSvcRc->pGomCost				= NULL;
  }
}

static inline int32_t RcConvertQp2QStep (int32_t iQP) {
  return g_kiQpToQstepTable[iQP];
}
static inline int32_t RcConvertQStep2Qp (int32_t iQpStep) {
  return WELS_ROUND ((6 * log (iQpStep * 1.0f / INT_MULTIPLY) / log (2.0) + 4.0));
}

void RcInitSequenceParameter (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc = NULL;
  SSpatialLayerConfig* pDLayerParam = NULL;

  int32_t j = 0;
  int32_t iMbWidth = 0;

  bool bMultiSliceMode = false;
  int32_t iGomRowMode0 = 1, iGomRowMode1 = 1;
#ifdef _TEST_TEMP_RC_
  fp_test_rc = fopen ("testRC.dat", "w");
  fp_vgop = fopen ("vgop.dat", "w");
#endif
  for (j = 0; j < pEncCtx->pSvcParam->iSpatialLayerNum; j++) {
    SSliceCtx* pSliceCtx = &pEncCtx->pSliceCtxList[j];
    pWelsSvcRc  = &pEncCtx->pWelsSvcRc[j];
    pDLayerParam = &pEncCtx->pSvcParam->sSpatialLayers[j];
    iMbWidth     = (pDLayerParam->iVideoWidth >> 4);
    pWelsSvcRc->iNumberMbFrame = iMbWidth * (pDLayerParam->iVideoHeight >> 4);
    pWelsSvcRc->iSliceNum = pSliceCtx->iSliceNumInFrame;

    pWelsSvcRc->iRcVaryPercentage = _BITS_RANGE;	// % -- for temp
    pWelsSvcRc->iRcVaryRatio = pWelsSvcRc->iRcVaryPercentage;

    pWelsSvcRc->iSkipBufferRatio  = SKIP_RATIO;

    pWelsSvcRc->iQpRangeUpperInFrame = (QP_RANGE_UPPER_MODE1 * MAX_BITS_VARY_PERCENTAGE - ((
                                          QP_RANGE_UPPER_MODE1 - QP_RANGE_MODE0) *
                                        pWelsSvcRc->iRcVaryRatio)) / MAX_BITS_VARY_PERCENTAGE;
    pWelsSvcRc->iQpRangeLowerInFrame = (QP_RANGE_LOWER_MODE1 * MAX_BITS_VARY_PERCENTAGE - ((
                                          QP_RANGE_LOWER_MODE1 - QP_RANGE_MODE0) *
                                        pWelsSvcRc->iRcVaryRatio)) / MAX_BITS_VARY_PERCENTAGE;

    if (iMbWidth <= MB_WIDTH_THRESHOLD_90P) {
      pWelsSvcRc->iSkipQpValue = SKIP_QP_90P;
      iGomRowMode0 = GOM_ROW_MODE0_90P;
      iGomRowMode1 = GOM_ROW_MODE1_90P;
    } else if (iMbWidth <= MB_WIDTH_THRESHOLD_180P) {
      pWelsSvcRc->iSkipQpValue = SKIP_QP_180P;
      iGomRowMode0 = GOM_ROW_MODE0_180P;
      iGomRowMode1 = GOM_ROW_MODE1_180P;
    } else if (iMbWidth <= MB_WIDTH_THRESHOLD_360P) {
      pWelsSvcRc->iSkipQpValue = SKIP_QP_360P;
      iGomRowMode0 = GOM_ROW_MODE0_360P;
      iGomRowMode1 = GOM_ROW_MODE1_360P;
    } else {
      pWelsSvcRc->iSkipQpValue = SKIP_QP_720P;
      iGomRowMode0 = GOM_ROW_MODE0_720P;
      iGomRowMode1 = GOM_ROW_MODE1_720P;
    }
    iGomRowMode0 = iGomRowMode1 + ((iGomRowMode0 - iGomRowMode1) * pWelsSvcRc->iRcVaryRatio / MAX_BITS_VARY_PERCENTAGE);

    pWelsSvcRc->iNumberMbGom   = iMbWidth * iGomRowMode0;

    pWelsSvcRc->iMinQp = GOM_MIN_QP_MODE;
    pWelsSvcRc->iMaxQp = GOM_MAX_QP_MODE;

    pWelsSvcRc->iFrameDeltaQpUpper = LAST_FRAME_QP_RANGE_UPPER_MODE1 - ((LAST_FRAME_QP_RANGE_UPPER_MODE1 -
                                     LAST_FRAME_QP_RANGE_UPPER_MODE0) * pWelsSvcRc->iRcVaryRatio / MAX_BITS_VARY_PERCENTAGE);
    pWelsSvcRc->iFrameDeltaQpLower = LAST_FRAME_QP_RANGE_LOWER_MODE1 - ((LAST_FRAME_QP_RANGE_LOWER_MODE1 -
                                     LAST_FRAME_QP_RANGE_LOWER_MODE0) * pWelsSvcRc->iRcVaryRatio / MAX_BITS_VARY_PERCENTAGE);

    pWelsSvcRc->iSkipFrameNum = 0;
    pWelsSvcRc->iGomSize = (pWelsSvcRc->iNumberMbFrame + pWelsSvcRc->iNumberMbGom - 1) / pWelsSvcRc->iNumberMbGom;


    RcInitLayerMemory (pWelsSvcRc, pEncCtx->pMemAlign, 1 + pEncCtx->pSvcParam->sDependencyLayers[j].iHighestTemporalId);

    bMultiSliceMode	= ((SM_RASTER_SLICE == pDLayerParam->sSliceCfg.uiSliceMode) ||
                       (SM_ROWMB_SLICE	 == pDLayerParam->sSliceCfg.uiSliceMode) ||
                       (SM_DYN_SLICE	 == pDLayerParam->sSliceCfg.uiSliceMode));
    if (bMultiSliceMode)
      pWelsSvcRc->iNumberMbGom = pWelsSvcRc->iNumberMbFrame;
  }
}


void RcInitTlWeight (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCTemporal* pTOverRc	= pWelsSvcRc->pTemporalOverRc;
  SSpatialLayerInternal* pDLayerParam =  &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId];
  const int32_t kiDecompositionStages = pDLayerParam->iDecompositionStages;
  const int32_t kiHighestTid = pDLayerParam->iHighestTemporalId;

  //Index 0:Virtual GOP size, Index 1:Frame rate
  //double WeightArray[4][4] = { {1.0, 0, 0, 0}, {0.6, 0.4, 0, 0}, {0.4, 0.3, 0.15, 0}, {0.25, 0.15, 0.125, 0.0875}};
  int32_t iWeightArray[4][4] = { {2000, 0, 0, 0}, {1200, 800, 0, 0}, {800, 600, 300, 0}, {500, 300, 250, 175}}; // original*WEIGHT_MULTIPLY
  const int32_t kiGopSize = (1 << kiDecompositionStages);
  int32_t i, k, n;

  n = 0;
  while (n <= kiHighestTid) {
    pTOverRc[n].iTlayerWeight	= iWeightArray[kiDecompositionStages][n];
    ++ n;
  }
  //Calculate the frame index for the current frame and its reference frame
  for (n = 0; n < VGOP_SIZE; n += kiGopSize) {
    pWelsSvcRc->iTlOfFrames[n] = 0;
    for (i = 1; i <= kiDecompositionStages; i++) {
      for (k = 1 << (kiDecompositionStages - i); k < kiGopSize; k += (kiGopSize >> (i - 1))) {
        pWelsSvcRc->iTlOfFrames[k + n] = i;
      }
    }
  }
  pWelsSvcRc->iPreviousGopSize = kiGopSize;
  pWelsSvcRc->iGopNumberInVGop = VGOP_SIZE / kiGopSize;
}

void RcUpdateBitrateFps (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc	= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCTemporal* pTOverRc		= pWelsSvcRc->pTemporalOverRc;

  SSpatialLayerConfig* pDLayerParam     = &pEncCtx->pSvcParam->sSpatialLayers[pEncCtx->uiDependencyId];
  SSpatialLayerInternal* pDLayerParamInternal     = &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId];
  const int32_t kiGopSize	= (1 << pDLayerParamInternal->iDecompositionStages);
  const int32_t kiHighestTid = pDLayerParamInternal->iHighestTemporalId;
  int32_t input_iBitsPerFrame = WELS_ROUND (pDLayerParam->iSpatialBitrate * INT_MULTIPLY /
                                pDLayerParamInternal->fInputFrameRate);
  const int32_t kiGopBits	= WELS_DIV_ROUND (input_iBitsPerFrame * kiGopSize, INT_MULTIPLY);
  int32_t i;

  pWelsSvcRc->iBitRate   = pDLayerParam->iSpatialBitrate;
  pWelsSvcRc->fFrameRate = pDLayerParamInternal->fInputFrameRate;

  int32_t iTargetVaryRange = FRAME_iTargetBits_VARY_RANGE * (MAX_BITS_VARY_PERCENTAGE - pWelsSvcRc->iRcVaryRatio);
  int32_t iMinBitsRatio = (MAX_BITS_VARY_PERCENTAGE) * INT_MULTIPLY - iTargetVaryRange;
  int32_t iMaxBitsRatio = (MAX_BITS_VARY_PERCENTAGE) * (INT_MULTIPLY + FRAME_iTargetBits_VARY_RANGE);

  for (i = 0; i <= kiHighestTid; i++) {
    const int64_t kdConstraitBits = kiGopBits * pTOverRc[i].iTlayerWeight;
    pTOverRc[i].iMinBitsTl = WELS_DIV_ROUND (kdConstraitBits * iMinBitsRatio,
                             INT_MULTIPLY * MAX_BITS_VARY_PERCENTAGE * WEIGHT_MULTIPLY);
    pTOverRc[i].iMaxBitsTl = WELS_DIV_ROUND (kdConstraitBits * iMaxBitsRatio,
                             INT_MULTIPLY * MAX_BITS_VARY_PERCENTAGE * WEIGHT_MULTIPLY);
  }
  //When bitrate is changed, pBuffer size should be updated
  pWelsSvcRc->iBufferSizeSkip = WELS_DIV_ROUND (pWelsSvcRc->iBitRate * pWelsSvcRc->iSkipBufferRatio, INT_MULTIPLY);
  pWelsSvcRc->iBufferSizePadding = WELS_DIV_ROUND (pWelsSvcRc->iBitRate * PADDING_BUFFER_RATIO, INT_MULTIPLY);

  //change remaining bits
  if (pWelsSvcRc->iBitsPerFrame > REMAIN_BITS_TH)
    pWelsSvcRc->iRemainingBits = pWelsSvcRc->iRemainingBits * input_iBitsPerFrame / pWelsSvcRc->iBitsPerFrame;
  pWelsSvcRc->iBitsPerFrame = input_iBitsPerFrame;
}


void RcInitVGop (sWelsEncCtx* pEncCtx) {
  const int32_t kiDid		= pEncCtx->uiDependencyId;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[kiDid];
  SRCTemporal* pTOverRc		= pWelsSvcRc->pTemporalOverRc;
  const int32_t kiHighestTid = pEncCtx->pSvcParam->sDependencyLayers[kiDid].iHighestTemporalId;

  pWelsSvcRc->iRemainingBits = WELS_DIV_ROUND (VGOP_SIZE * pWelsSvcRc->iBitsPerFrame, INT_MULTIPLY);
  pWelsSvcRc->iRemainingWeights = pWelsSvcRc->iGopNumberInVGop * WEIGHT_MULTIPLY;

  pWelsSvcRc->iFrameCodedInVGop = 0;
  pWelsSvcRc->iGopIndexInVGop = 0;

  for (int32_t i = 0; i <= kiHighestTid; ++ i)
    pTOverRc[i].iGopBitsDq = 0;
  pWelsSvcRc->iSkipFrameInVGop = 0;
}

void RcInitRefreshParameter (sWelsEncCtx* pEncCtx) {
  const int32_t kiDid		  = pEncCtx->uiDependencyId;
  SWelsSvcRc* pWelsSvcRc   = &pEncCtx->pWelsSvcRc[kiDid];
  SRCTemporal* pTOverRc		  = pWelsSvcRc->pTemporalOverRc;
  SSpatialLayerConfig* pDLayerParam       = &pEncCtx->pSvcParam->sSpatialLayers[kiDid];
  SSpatialLayerInternal* pDLayerParamInternal       = &pEncCtx->pSvcParam->sDependencyLayers[kiDid];
  const int32_t kiHighestTid = pDLayerParamInternal->iHighestTemporalId;
  int32_t i;

  //I frame R-Q Model
  pWelsSvcRc->iIntraComplexity = 0;
  pWelsSvcRc->iIntraMbCount = 0;

  //P frame R-Q Model
  for (i = 0; i <= kiHighestTid; i++) {
    pTOverRc[i].iPFrameNum = 0;
    pTOverRc[i].iLinearCmplx = 0;
    pTOverRc[i].iFrameCmplxMean = 0;
  }

  pWelsSvcRc->iBufferFullnessSkip = 0;
  pWelsSvcRc->iBufferFullnessPadding = 0;

  pWelsSvcRc->iGopIndexInVGop = 0;
  pWelsSvcRc->iRemainingBits = 0;
  pWelsSvcRc->iBitsPerFrame	= 0;

  //Backup the initial bitrate and fps
  pWelsSvcRc->iPreviousBitrate  = pDLayerParam->iSpatialBitrate;
  pWelsSvcRc->dPreviousFps      = pDLayerParamInternal->fInputFrameRate;

  memset (pWelsSvcRc->pCurrentFrameGomSad, 0, pWelsSvcRc->iGomSize * sizeof (int32_t));

  RcInitTlWeight (pEncCtx);
  RcUpdateBitrateFps (pEncCtx);
  RcInitVGop (pEncCtx);
}

bool RcJudgeBitrateFpsUpdate (sWelsEncCtx* pEncCtx) {
  int32_t iCurDid = pEncCtx->uiDependencyId;
  SWelsSvcRc* pWelsSvcRc       = &pEncCtx->pWelsSvcRc[iCurDid];
  SSpatialLayerInternal* pDLayerParamInternal    = &pEncCtx->pSvcParam->sDependencyLayers[iCurDid];
  SSpatialLayerConfig* pDLayerParam    = &pEncCtx->pSvcParam->sSpatialLayers[iCurDid];

  if ((pWelsSvcRc->iPreviousBitrate != pDLayerParam->iSpatialBitrate) ||
      (pWelsSvcRc->dPreviousFps - pDLayerParamInternal->fInputFrameRate) > EPSN ||
      (pWelsSvcRc->dPreviousFps - pDLayerParamInternal->fInputFrameRate) < -EPSN) {
    pWelsSvcRc->iPreviousBitrate = pDLayerParam->iSpatialBitrate;
    pWelsSvcRc->dPreviousFps = pDLayerParamInternal->fInputFrameRate;
    return true;
  } else
    return false;
}

#if GOM_TRACE_FLAG
void RcTraceVGopBitrate (sWelsEncCtx* pEncCtx) {
  const int32_t kiDid				= pEncCtx->uiDependencyId;
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[kiDid];

  if (pWelsSvcRc->iFrameCodedInVGop) {
    const int32_t kiHighestTid	= pEncCtx->pSvcParam->sDependencyLayers[kiDid].iHighestTemporalId;
    SRCTemporal* pTOverRc			= pWelsSvcRc->pTemporalOverRc;
    int32_t iVGopBitrate = 0;
    int32_t	iTotalBits = pWelsSvcRc->iPaddingBitrateStat;
    int32_t iTid = 0;
    while (iTid <= kiHighestTid) {
      iTotalBits += pTOverRc[iTid].iGopBitsDq;
      ++ iTid;
    }
    int32_t iFrameInVGop = pWelsSvcRc->iFrameCodedInVGop + pWelsSvcRc->iSkipFrameInVGop;
    if (0 != iFrameInVGop)
      iVGopBitrate = WELS_ROUND (iTotalBits / iFrameInVGop * pWelsSvcRc->fFrameRate);
#ifdef _TEST_TEMP_Rc_
    fprintf (fp_vgop, "%d\n", WELS_ROUND ((double)iTotalBits / iFrameInVGop));
#endif
    WelsLog (& (pEncCtx->sLogCtx), WELS_LOG_INFO, "[Rc] VGOPbitrate%d: %d ", kiDid, iVGopBitrate);
    if (iTotalBits > 0) {
      iTid = 0;
      while (iTid <= kiHighestTid) {
        WelsLog (& (pEncCtx->sLogCtx), WELS_LOG_INFO, "T%d=%8.3f ", iTid, (double) (pTOverRc[iTid].iGopBitsDq / iTotalBits));
        ++ iTid;
      }
    }
  }
}
#endif

void RcUpdateTemporalZero (sWelsEncCtx* pEncCtx) {
  const int32_t kiDid		= pEncCtx->uiDependencyId;
  SWelsSvcRc* pWelsSvcRc	= &pEncCtx->pWelsSvcRc[kiDid];
  SSpatialLayerInternal* pDLayerParam		= &pEncCtx->pSvcParam->sDependencyLayers[kiDid];
  const int32_t kiGopSize	= (1 << pDLayerParam->iDecompositionStages);

  if (pWelsSvcRc->iPreviousGopSize  != kiGopSize) {
#if GOM_TRACE_FLAG
    RcTraceVGopBitrate (pEncCtx);
#endif
    RcInitTlWeight (pEncCtx);
    RcInitVGop (pEncCtx);
  } else if (pWelsSvcRc->iGopIndexInVGop == pWelsSvcRc->iGopNumberInVGop || pEncCtx->eSliceType == I_SLICE) {
#if GOM_TRACE_FLAG
    RcTraceVGopBitrate (pEncCtx);
#endif
    RcInitVGop (pEncCtx);
  }
  pWelsSvcRc->iGopIndexInVGop++;
}


void RcInitIdrQp (sWelsEncCtx* pEncCtx) {
  double dBpp = 0;
  int32_t i;

  //64k@6fps for 90p:     bpp 0.74    QP:24
  //192k@12fps for 180p:  bpp 0.28    QP:26
  //512k@24fps for 360p:  bpp 0.09    QP:30
  //1500k@30fps for 720p: bpp 0.05    QP:32
  double dBppArray[4][3] = {{0.5, 0.75, 1.0}, {0.2, 0.3, 0.4}, {0.05, 0.09, 0.13}, {0.03, 0.06, 0.1}};
  int32_t dInitialQPArray[4][4] = {{28, 26, 24, 22}, {30, 28, 26, 24}, {32, 30, 28, 26}, {34, 32, 30, 28}};
  int32_t iBppIndex = 0;

  SWelsSvcRc* pWelsSvcRc		= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SSpatialLayerConfig* pDLayerParam			= &pEncCtx->pSvcParam->sSpatialLayers[pEncCtx->uiDependencyId];

  if (pDLayerParam->fFrameRate > EPSN && pDLayerParam->iVideoWidth && pDLayerParam->iVideoHeight)
    dBpp = (double) (pDLayerParam->iSpatialBitrate) / (double) (pDLayerParam->fFrameRate * pDLayerParam->iVideoWidth *
           pDLayerParam->iVideoHeight);
  else
    dBpp = 0.1;

  //Area*2
  if (pDLayerParam->iVideoWidth * pDLayerParam->iVideoHeight <= 28800) // 90p video:160*90
    iBppIndex = 0;
  else if (pDLayerParam->iVideoWidth * pDLayerParam->iVideoHeight <= 115200) // 180p video:320*180
    iBppIndex = 1;
  else if (pDLayerParam->iVideoWidth * pDLayerParam->iVideoHeight <= 460800) // 360p video:640*360
    iBppIndex = 2;
  else
    iBppIndex = 3;

  //Search
  for (i = 0; i < 3; i++) {
    if (dBpp <= dBppArray[iBppIndex][i])
      break;
  }
  pWelsSvcRc->iInitialQp = dInitialQPArray[iBppIndex][i];
  pWelsSvcRc->iInitialQp = WELS_CLIP3 (pWelsSvcRc->iInitialQp, MIN_IDR_QP, MAX_IDR_QP);
  pEncCtx->iGlobalQp = pWelsSvcRc->iInitialQp;
  pWelsSvcRc->iQStep = RcConvertQp2QStep (pEncCtx->iGlobalQp);
  pWelsSvcRc->iLastCalculatedQScale = pEncCtx->iGlobalQp;
}

void RcCalculateIdrQp (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  //obtain the idr qp using previous idr complexity
  if (pWelsSvcRc->iNumberMbFrame != pWelsSvcRc->iIntraMbCount) {
    pWelsSvcRc->iIntraComplexity = pWelsSvcRc->iIntraComplexity * pWelsSvcRc->iNumberMbFrame /
                                   pWelsSvcRc->iIntraMbCount;
  }
  pWelsSvcRc->iInitialQp = RcConvertQStep2Qp (pWelsSvcRc->iIntraComplexity /
                           pWelsSvcRc->iTargetBits);
  pWelsSvcRc->iInitialQp = WELS_CLIP3 (pWelsSvcRc->iInitialQp, MIN_IDR_QP, MAX_IDR_QP);
  pEncCtx->iGlobalQp = pWelsSvcRc->iInitialQp;
  pWelsSvcRc->iQStep = RcConvertQp2QStep (pEncCtx->iGlobalQp);
  pWelsSvcRc->iLastCalculatedQScale = pEncCtx->iGlobalQp;
}


void RcCalculatePictureQp (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc		= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  int32_t iTl					= pEncCtx->uiTemporalId;
  SRCTemporal* pTOverRc			= &pWelsSvcRc->pTemporalOverRc[iTl];
  int32_t iLumaQp = 0;

  if (0 == pTOverRc->iPFrameNum) {
    iLumaQp = pWelsSvcRc->iInitialQp;
  } else if (pWelsSvcRc->iCurrentBitsLevel == BITS_EXCEEDED) {
    iLumaQp = MAX_LOW_BR_QP;
    //limit QP
    int32_t iLastIdxCodecInVGop = pWelsSvcRc->iFrameCodedInVGop - 1;
    if (iLastIdxCodecInVGop < 0)
      iLastIdxCodecInVGop += VGOP_SIZE;
    int32_t iTlLast = pWelsSvcRc->iTlOfFrames[iLastIdxCodecInVGop];
    int32_t iDeltaQpTemporal = iTl - iTlLast;
    if (0 == iTlLast && iTl > 0)
      iDeltaQpTemporal += 3;
    else if (0 == iTl && iTlLast > 0)
      iDeltaQpTemporal -= 3;

    iLumaQp = WELS_CLIP3 (iLumaQp,
                          pWelsSvcRc->iLastCalculatedQScale - pWelsSvcRc->iFrameDeltaQpLower + iDeltaQpTemporal,
                          pWelsSvcRc->iLastCalculatedQScale + pWelsSvcRc->iFrameDeltaQpUpper + iDeltaQpTemporal);
    iLumaQp = WELS_CLIP3 (iLumaQp,  GOM_MIN_QP_MODE, MAX_LOW_BR_QP);

    pWelsSvcRc->iQStep = RcConvertQp2QStep (iLumaQp);
    pWelsSvcRc->iLastCalculatedQScale = iLumaQp;

    if (pEncCtx->pSvcParam->bEnableAdaptiveQuant) {
      iLumaQp = WELS_CLIP3 ((iLumaQp * INT_MULTIPLY - pEncCtx->pVaa->sAdaptiveQuantParam.iAverMotionTextureIndexToDeltaQp) /
                            INT_MULTIPLY, GOM_MIN_QP_MODE, MAX_LOW_BR_QP);
    }

    pEncCtx->iGlobalQp = iLumaQp;

    return;
  } else {
    int64_t iCmplxRatio = WELS_DIV_ROUND64 (pEncCtx->pVaa->sComplexityAnalysisParam.iFrameComplexity * INT_MULTIPLY,
                                            pTOverRc->iFrameCmplxMean);
    iCmplxRatio = WELS_CLIP3 (iCmplxRatio, INT_MULTIPLY - FRAME_CMPLX_RATIO_RANGE, INT_MULTIPLY + FRAME_CMPLX_RATIO_RANGE);

    pWelsSvcRc->iQStep = WELS_DIV_ROUND ((pTOverRc->iLinearCmplx * iCmplxRatio), (pWelsSvcRc->iTargetBits * INT_MULTIPLY));
    iLumaQp = RcConvertQStep2Qp (pWelsSvcRc->iQStep);

    //limit QP
    int32_t iLastIdxCodecInVGop = pWelsSvcRc->iFrameCodedInVGop - 1;
    if (iLastIdxCodecInVGop < 0)
      iLastIdxCodecInVGop += VGOP_SIZE;
    int32_t iTlLast = pWelsSvcRc->iTlOfFrames[iLastIdxCodecInVGop];
    int32_t iDeltaQpTemporal = iTl - iTlLast;
    if (0 == iTlLast && iTl > 0)
      iDeltaQpTemporal += 3;
    else if (0 == iTl && iTlLast > 0)
      iDeltaQpTemporal -= 3;

    iLumaQp = WELS_CLIP3 (iLumaQp,
                          pWelsSvcRc->iLastCalculatedQScale - pWelsSvcRc->iFrameDeltaQpLower + iDeltaQpTemporal,
                          pWelsSvcRc->iLastCalculatedQScale + pWelsSvcRc->iFrameDeltaQpUpper + iDeltaQpTemporal);
  }

  iLumaQp = WELS_CLIP3 (iLumaQp,  GOM_MIN_QP_MODE, GOM_MAX_QP_MODE);

  pWelsSvcRc->iQStep = RcConvertQp2QStep (iLumaQp);
  pWelsSvcRc->iLastCalculatedQScale = iLumaQp;
#ifndef _NOT_USE_AQ_FOR_TEST_
  if (pEncCtx->pSvcParam->bEnableAdaptiveQuant) {

    iLumaQp =  WELS_DIV_ROUND (iLumaQp * INT_MULTIPLY - pEncCtx->pVaa->sAdaptiveQuantParam.iAverMotionTextureIndexToDeltaQp,
                               INT_MULTIPLY);

    if (! ((pEncCtx->pSvcParam->iRCMode == RC_BITRATE_MODE) && (pEncCtx->pSvcParam->bEnableFrameSkip == false)))
      iLumaQp = WELS_CLIP3 (iLumaQp, pWelsSvcRc->iMinQp, pWelsSvcRc->iMaxQp);

  }
#endif
  pEncCtx->iGlobalQp = iLumaQp;
}

void RcInitSliceInformation (sWelsEncCtx* pEncCtx) {
  SSliceCtx* pCurSliceCtx	= pEncCtx->pCurDqLayer->pSliceEncCtx;
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCSlicing* pSOverRc				= &pWelsSvcRc->pSlicingOverRc[0];
  const int32_t kiSliceNum			= pWelsSvcRc->iSliceNum;
  const int32_t kiBitsPerMb		= WELS_DIV_ROUND (pWelsSvcRc->iTargetBits * INT_MULTIPLY, pWelsSvcRc->iNumberMbFrame);

  for (int32_t i = 0; i < kiSliceNum; i++) {
    pSOverRc->iStartMbSlice	=
      pSOverRc->iEndMbSlice		= pCurSliceCtx->pFirstMbInSlice[i];
    pSOverRc->iEndMbSlice		+= (pCurSliceCtx->pCountMbNumInSlice[i] - 1);
    pSOverRc->iTotalQpSlice	= 0;
    pSOverRc->iTotalMbSlice	= 0;
    pSOverRc->iTargetBitsSlice = WELS_DIV_ROUND (kiBitsPerMb * pCurSliceCtx->pCountMbNumInSlice[i], INT_MULTIPLY);
    pSOverRc->iFrameBitsSlice	= 0;
    pSOverRc->iGomBitsSlice	= 0;
    ++ pSOverRc;
  }
}

void RcDecideTargetBits (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc	= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCTemporal* pTOverRc		= &pWelsSvcRc->pTemporalOverRc[pEncCtx->uiTemporalId];

  pWelsSvcRc->iCurrentBitsLevel = BITS_NORMAL;
  //allocate bits
  if (pEncCtx->eSliceType == I_SLICE) {
    pWelsSvcRc->iTargetBits = WELS_DIV_ROUND (pWelsSvcRc->iBitsPerFrame * IDR_BITRATE_RATIO, INT_MULTIPLY);
  } else {
    if (pWelsSvcRc->iRemainingWeights > pTOverRc->iTlayerWeight)
      pWelsSvcRc->iTargetBits = (int32_t) ((int64_t)pWelsSvcRc->iRemainingBits * pTOverRc->iTlayerWeight /
                                           pWelsSvcRc->iRemainingWeights);
    else //this case should be not hit. needs to more test case to verify this
      pWelsSvcRc->iTargetBits = pWelsSvcRc->iRemainingBits;
    if ((pWelsSvcRc->iTargetBits <= 0) && ((pEncCtx->pSvcParam->iRCMode == RC_BITRATE_MODE)
                                           && (pEncCtx->pSvcParam->bEnableFrameSkip == false))) {
      pWelsSvcRc->iCurrentBitsLevel = BITS_EXCEEDED;
    }
    pWelsSvcRc->iTargetBits = WELS_CLIP3 (pWelsSvcRc->iTargetBits, pTOverRc->iMinBitsTl,	pTOverRc->iMaxBitsTl);
  }
  pWelsSvcRc->iRemainingWeights -= pTOverRc->iTlayerWeight;
}


void RcInitGomParameters (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCSlicing* pSOverRc				= &pWelsSvcRc->pSlicingOverRc[0];
  const int32_t kiSliceNum			= pWelsSvcRc->iSliceNum;
  const int32_t kiGlobalQp			= pEncCtx->iGlobalQp;

  pWelsSvcRc->iAverageFrameQp = 0;
  pWelsSvcRc->iMinFrameQp = 51;;
  pWelsSvcRc->iMaxFrameQp = 0;
  for (int32_t i = 0; i < kiSliceNum; ++i) {
    pSOverRc->iComplexityIndexSlice	= 0;
    pSOverRc->iCalculatedQpSlice		= kiGlobalQp;
    ++ pSOverRc;
  }
  memset (pWelsSvcRc->pGomComplexity, 0, pWelsSvcRc->iGomSize * sizeof (double));
  memset (pWelsSvcRc->pGomCost, 0, pWelsSvcRc->iGomSize * sizeof (int32_t));
}

void RcCalculateMbQp (sWelsEncCtx* pEncCtx, SMB* pCurMb, const int32_t kiSliceId) {
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCSlicing* pSOverRc		= &pWelsSvcRc->pSlicingOverRc[kiSliceId];
  int32_t iLumaQp			= pSOverRc->iCalculatedQpSlice;
  SDqLayer* pCurLayer				= pEncCtx->pCurDqLayer;
  const uint8_t kuiChromaQpIndexOffset = pCurLayer->sLayerInfo.pPpsP->uiChromaQpIndexOffset;
#ifndef _NOT_USE_AQ_FOR_TEST_
  if (pEncCtx->pSvcParam->bEnableAdaptiveQuant) {
    iLumaQp   = (int8_t)WELS_CLIP3 (iLumaQp +
                                    pEncCtx->pVaa->sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp[pCurMb->iMbXY], pWelsSvcRc->iMinQp, 51);
  }
#endif
  pCurMb->uiChromaQp	= g_kuiChromaQpTable[CLIP3_QP_0_51 (iLumaQp + kuiChromaQpIndexOffset)];
  pCurMb->uiLumaQp		= iLumaQp;
}

SWelsSvcRc* RcJudgeBaseUsability (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc  = NULL, *pWelsSvcRc_Base = NULL;
  SSpatialLayerConfig* pDlpBase = NULL, *pDLayerParam = NULL;
  SSpatialLayerInternal* pDlpBaseInternal = NULL;
  if (pEncCtx->uiDependencyId <= 0)
    return NULL;
  pDlpBaseInternal = &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId - 1];
  pDlpBase = &pEncCtx->pSvcParam->sSpatialLayers[pEncCtx->uiDependencyId - 1];
  pWelsSvcRc_Base = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId - 1];
  if (pEncCtx->uiTemporalId <= pDlpBaseInternal->iDecompositionStages) {
    pWelsSvcRc      = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
    pWelsSvcRc_Base = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId - 1];
    pDLayerParam             = &pEncCtx->pSvcParam->sSpatialLayers[pEncCtx->uiDependencyId];
    pDlpBase        = &pEncCtx->pSvcParam->sSpatialLayers[pEncCtx->uiDependencyId - 1];
    if ((pDLayerParam->iVideoWidth * pDLayerParam->iVideoHeight / pWelsSvcRc->iNumberMbGom) ==
        (pDlpBase->iVideoWidth * pDlpBase->iVideoHeight / pWelsSvcRc_Base->iNumberMbGom))
      return pWelsSvcRc_Base;
    else
      return NULL;
  } else
    return NULL;
}

void RcGomTargetBits (sWelsEncCtx* pEncCtx, const int32_t kiSliceId) {
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SWelsSvcRc* pWelsSvcRc_Base	= NULL;
  SRCSlicing* pSOverRc				= &pWelsSvcRc->pSlicingOverRc[kiSliceId];

  int32_t iAllocateBits = 0;
  int32_t iSumSad = 0;
  int32_t iLastGomIndex = 0;
  int32_t iLeftBits = 0;
  const int32_t kiComplexityIndex	= pSOverRc->iComplexityIndexSlice;
  int32_t i;

  iLastGomIndex  = pSOverRc->iEndMbSlice / pWelsSvcRc->iNumberMbGom;
  iLeftBits = pSOverRc->iTargetBitsSlice - pSOverRc->iFrameBitsSlice;

  if (iLeftBits <= 0) {
    pSOverRc->iGomTargetBits = 0;
    return;
  } else if (kiComplexityIndex >= iLastGomIndex) {
    iAllocateBits = iLeftBits;
  } else {
    pWelsSvcRc_Base = RcJudgeBaseUsability (pEncCtx);
    pWelsSvcRc_Base = (pWelsSvcRc_Base) ? pWelsSvcRc_Base : pWelsSvcRc;
    for (i = kiComplexityIndex; i <= iLastGomIndex; i++) {
      iSumSad += pWelsSvcRc_Base->pCurrentFrameGomSad[i];
    }
    if (0 == iSumSad)
      iAllocateBits = WELS_DIV_ROUND (iLeftBits, (iLastGomIndex - kiComplexityIndex));
    else
      iAllocateBits = WELS_DIV_ROUND ((int64_t)iLeftBits * pWelsSvcRc_Base->pCurrentFrameGomSad[kiComplexityIndex + 1],
                                      iSumSad);

  }
  pSOverRc->iGomTargetBits = iAllocateBits;
}



void RcCalculateGomQp (sWelsEncCtx* pEncCtx, SMB* pCurMb, int32_t iSliceId) {
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCSlicing* pSOverRc				= &pWelsSvcRc->pSlicingOverRc[iSliceId];
  int64_t iBitsRatio = 1;

  int64_t iLeftBits = pSOverRc->iTargetBitsSlice - pSOverRc->iFrameBitsSlice;
  int64_t iTargetLeftBits = iLeftBits + pSOverRc->iGomBitsSlice - pSOverRc->iGomTargetBits;

  if (iLeftBits <= 0) {
    pSOverRc->iCalculatedQpSlice += 2;
  } else {
    //globe decision
    iBitsRatio = 10000 * iLeftBits / (iTargetLeftBits + 1);
    if (iBitsRatio < 8409)		//2^(-1.5/6)*10000
      pSOverRc->iCalculatedQpSlice += 2;
    else if (iBitsRatio < 9439)	//2^(-0.5/6)*10000
      pSOverRc->iCalculatedQpSlice += 1;
    else if (iBitsRatio > 10600)		//2^(0.5/6)*10000
      pSOverRc->iCalculatedQpSlice -= 1;
    else if (iBitsRatio > 11900)		//2^(1.5/6)*10000
      pSOverRc->iCalculatedQpSlice -= 2;
  }

  pSOverRc->iCalculatedQpSlice = WELS_CLIP3 (pSOverRc->iCalculatedQpSlice,
                                 pEncCtx->iGlobalQp - pWelsSvcRc->iQpRangeLowerInFrame, pEncCtx->iGlobalQp + pWelsSvcRc->iQpRangeUpperInFrame);
  if (! ((pEncCtx->pSvcParam->iRCMode == RC_BITRATE_MODE) && (pEncCtx->pSvcParam->bEnableFrameSkip == false)))
    pSOverRc->iCalculatedQpSlice = WELS_CLIP3 (pSOverRc->iCalculatedQpSlice, pWelsSvcRc->iMinQp, pWelsSvcRc->iMaxQp);

  pSOverRc->iGomBitsSlice = 0;

}

void   RcVBufferCalculationSkip (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCTemporal* pTOverRc		= pWelsSvcRc->pTemporalOverRc;
  const int32_t kiOutputBits = WELS_DIV_ROUND (pWelsSvcRc->iBitsPerFrame, INT_MULTIPLY);
  //condition 1: whole pBuffer fullness
  pWelsSvcRc->iBufferFullnessSkip += (pWelsSvcRc->iFrameDqBits - kiOutputBits);
  WelsLog (& (pEncCtx->sLogCtx), WELS_LOG_DEBUG,"[Rc] bits in buffer = %3d",pWelsSvcRc->iBufferFullnessSkip);
  //condition 2: VGOP bits constraint
  int32_t iVGopBitsPred = 0;
  for (int32_t i = pWelsSvcRc->iFrameCodedInVGop + 1; i < VGOP_SIZE; i++)
    iVGopBitsPred += pTOverRc[pWelsSvcRc->iTlOfFrames[i]].iMinBitsTl;
  iVGopBitsPred -= pWelsSvcRc->iRemainingBits;
  double dIncPercent = iVGopBitsPred * 100.0 * INT_MULTIPLY / (pWelsSvcRc->iBitsPerFrame * VGOP_SIZE) -
                       (double)VGOP_BITS_PERCENTAGE_DIFF;

  if ((pWelsSvcRc->iBufferFullnessSkip > pWelsSvcRc->iBufferSizeSkip
       &&	pWelsSvcRc->iAverageFrameQp > pWelsSvcRc->iSkipQpValue)
      || (dIncPercent > pWelsSvcRc->iRcVaryPercentage)) {
    pEncCtx->iSkipFrameFlag = 1;
    pWelsSvcRc->iBufferFullnessSkip = pWelsSvcRc->iBufferFullnessSkip - kiOutputBits;
    pWelsSvcRc->iRemainingBits +=  kiOutputBits;
    pWelsSvcRc->iSkipFrameNum++;
    pWelsSvcRc->iSkipFrameInVGop++;
    WelsLog (& (pEncCtx->sLogCtx), WELS_LOG_DEBUG,"[Rc] bits in buffer = %3d",pWelsSvcRc->iBufferFullnessSkip);
  }
    pWelsSvcRc->iBufferFullnessSkip = WELS_MAX (pWelsSvcRc->iBufferFullnessSkip, 0);

}

void WelsRcFrameDelayJudge (void* pCtx, EVideoFrameType eFrameType, long long uiTimeStamp) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SSpatialLayerConfig* pDLayerParam     = &pEncCtx->pSvcParam->sSpatialLayers[pEncCtx->uiDependencyId];
  SSpatialLayerInternal* pDLayerParamInternal     = &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId];

  int32_t iSentBits = WELS_ROUND (pDLayerParam->iSpatialBitrate / pDLayerParamInternal->fOutputFrameRate);

  pWelsSvcRc->bSkipFlag = false;
  if (pWelsSvcRc->iBufferFullnessSkip > pWelsSvcRc->iBufferSizeSkip) {
    pWelsSvcRc->bSkipFlag = true;
    pWelsSvcRc->iSkipFrameNum++;
    pWelsSvcRc->iSkipFrameInVGop++;
    pWelsSvcRc->iBufferFullnessSkip -= iSentBits;
    WelsLog (& (pEncCtx->sLogCtx), WELS_LOG_DEBUG,"[Rc] bits in buffer = %3d",pWelsSvcRc->iBufferFullnessSkip);
    pWelsSvcRc->iBufferFullnessSkip = WELS_MAX (pWelsSvcRc->iBufferFullnessSkip, 0);
  }
}

void RcVBufferCalculationPadding (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  const int32_t kiOutputBits = WELS_DIV_ROUND (pWelsSvcRc->iBitsPerFrame, INT_MULTIPLY);
  const int32_t kiBufferThreshold = WELS_DIV_ROUND (PADDING_THRESHOLD * (-pWelsSvcRc->iBufferSizePadding), INT_MULTIPLY);

  pWelsSvcRc->iBufferFullnessPadding += (pWelsSvcRc->iFrameDqBits - kiOutputBits);

  if (pWelsSvcRc->iBufferFullnessPadding < kiBufferThreshold) {
    pWelsSvcRc->iPaddingSize = -pWelsSvcRc->iBufferFullnessPadding;
    pWelsSvcRc->iPaddingSize >>= 3;	// /8
    pWelsSvcRc->iBufferFullnessPadding = 0;
  } else
    pWelsSvcRc->iPaddingSize = 0;
}


void RcTraceFrameBits (void* pCtx, long long uiTimeStamp) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];

  WelsLog (& (pEncCtx->sLogCtx), WELS_LOG_DEBUG,
    "[Rc] Frame timestamp = %8d, Frame type =%d, encoding_qp%d, average qp = %3d, max qp = %3d, min qp = %3d, index = %8d,\
    iTid = %1d, used = %8d, bitsperframe = %8d, target = %8d, remaingbits = %8d, skipbuffersize = %8d",
    (uint32_t)uiTimeStamp,pEncCtx->eSliceType, pEncCtx->uiDependencyId, pWelsSvcRc->iAverageFrameQp,pWelsSvcRc->iMaxFrameQp,pWelsSvcRc->iMinFrameQp,
    pEncCtx->iFrameIndex, pEncCtx->uiTemporalId, pWelsSvcRc->iFrameDqBits,WELS_DIV_ROUND (pWelsSvcRc->iBitsPerFrame, INT_MULTIPLY),
    pWelsSvcRc->iTargetBits, pWelsSvcRc->iRemainingBits, pWelsSvcRc->iBufferSizeSkip);

}

void RcUpdatePictureQpBits (sWelsEncCtx* pEncCtx, int32_t iCodedBits) {
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCSlicing* pSOverRc		= &pWelsSvcRc->pSlicingOverRc[0];
  SSliceCtx* pCurSliceCtx = pEncCtx->pCurDqLayer->pSliceEncCtx;
  int32_t iTotalQp = 0, iTotalMb = 0;
  int32_t i;

  if (pEncCtx->eSliceType == P_SLICE) {
    for (i = 0; i < pCurSliceCtx->iSliceNumInFrame; i++) {
      iTotalQp += pSOverRc->iTotalQpSlice;
      iTotalMb += pSOverRc->iTotalMbSlice;
      ++ pSOverRc;
    }
    if (iTotalMb > 0)
      pWelsSvcRc->iAverageFrameQp = WELS_DIV_ROUND (INT_MULTIPLY * iTotalQp, iTotalMb * INT_MULTIPLY);
    else
      pWelsSvcRc->iAverageFrameQp = pEncCtx->iGlobalQp;
  } else {
    pWelsSvcRc->iAverageFrameQp = pEncCtx->iGlobalQp;
  }
  pWelsSvcRc->iFrameDqBits = iCodedBits;
  pWelsSvcRc->pTemporalOverRc[pEncCtx->uiTemporalId].iGopBitsDq += pWelsSvcRc->iFrameDqBits;
}

void RcUpdateIntraComplexity (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  int32_t iAlpha = WELS_DIV_ROUND (INT_MULTIPLY, (1 + pWelsSvcRc->iIdrNum));
  if (iAlpha < (INT_MULTIPLY / 4)) iAlpha = INT_MULTIPLY / 4;

  int64_t iIntraCmplx = pWelsSvcRc->iQStep * pWelsSvcRc->iFrameDqBits;
  pWelsSvcRc->iIntraComplexity = WELS_DIV_ROUND (((INT_MULTIPLY - iAlpha) * pWelsSvcRc->iIntraComplexity + iAlpha *
                                 iIntraCmplx), INT_MULTIPLY);
  pWelsSvcRc->iIntraMbCount = pWelsSvcRc->iNumberMbFrame;

  pWelsSvcRc->iIdrNum++;
  if (pWelsSvcRc->iIdrNum > 255)
    pWelsSvcRc->iIdrNum = 255;
}

void RcUpdateFrameComplexity (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc		= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  const int32_t kiTl			= pEncCtx->uiTemporalId;
  SRCTemporal* pTOverRc			= &pWelsSvcRc->pTemporalOverRc[kiTl];

  if (0 == pTOverRc->iPFrameNum) {
    pTOverRc->iLinearCmplx = ((int64_t)pWelsSvcRc->iFrameDqBits) * pWelsSvcRc->iQStep;
  } else {
    pTOverRc->iLinearCmplx = WELS_DIV_ROUND64 ((LINEAR_MODEL_DECAY_FACTOR * (int64_t)pTOverRc->iLinearCmplx
                             + (INT_MULTIPLY - LINEAR_MODEL_DECAY_FACTOR) * (int64_t) (pWelsSvcRc->iFrameDqBits * pWelsSvcRc->iQStep)),
                             INT_MULTIPLY);
  }
  int32_t iAlpha = WELS_DIV_ROUND (INT_MULTIPLY, (1 + pTOverRc->iPFrameNum));
  if (iAlpha < SMOOTH_FACTOR_MIN_VALUE)
    iAlpha = SMOOTH_FACTOR_MIN_VALUE;
  pTOverRc->iFrameCmplxMean = WELS_DIV_ROUND (((INT_MULTIPLY - iAlpha) * pTOverRc->iFrameCmplxMean + iAlpha *
                              pEncCtx->pVaa->sComplexityAnalysisParam.iFrameComplexity), INT_MULTIPLY);

  pTOverRc->iPFrameNum++;
  if (pTOverRc->iPFrameNum > 255)
    pTOverRc->iPFrameNum = 255;
}

int32_t RcCalculateCascadingQp (struct TagWelsEncCtx* pEncCtx, int32_t iQp) {
  int32_t iTemporalQp = 0;
  if (pEncCtx->pSvcParam->iDecompStages) {
    if (pEncCtx->uiTemporalId == 0)
      iTemporalQp = iQp - 3 - (pEncCtx->pSvcParam->iDecompStages - 1);
    else
      iTemporalQp = iQp - (pEncCtx->pSvcParam->iDecompStages - pEncCtx->uiTemporalId);
    iTemporalQp = WELS_CLIP3 (iTemporalQp, 1, 51);
  } else
    iTemporalQp = iQp;
  return iTemporalQp;
}

void  WelsRcPictureInitGom (void* pCtx) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];

  if (pEncCtx->eSliceType == I_SLICE) {
    if (0 == pWelsSvcRc->iIdrNum) {	//iIdrNum == 0 means encoder has been initialed
      RcInitRefreshParameter (pEncCtx);
    }
  }
  if (RcJudgeBitrateFpsUpdate (pEncCtx)) {
    RcUpdateBitrateFps (pEncCtx);
  }
  if (pEncCtx->uiTemporalId == 0) {
    RcUpdateTemporalZero (pEncCtx);
  }
  RcDecideTargetBits (pEncCtx);
  //decide globe_qp
  if (pEncCtx->eSliceType == I_SLICE) {
    if (0 == pWelsSvcRc->iIdrNum)
      RcInitIdrQp (pEncCtx);
    else {
      RcCalculateIdrQp (pEncCtx);
    }
  } else {
    RcCalculatePictureQp (pEncCtx);
  }
  RcInitSliceInformation (pEncCtx);
  RcInitGomParameters (pEncCtx);

}



void  WelsRcPictureInfoUpdateGom (void* pCtx, int32_t iLayerSize) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  int32_t iCodedBits = (iLayerSize << 3);

  RcUpdatePictureQpBits (pEncCtx, iCodedBits);

  if (pEncCtx->eSliceType == P_SLICE) {
    RcUpdateFrameComplexity (pEncCtx);
  } else {
    RcUpdateIntraComplexity (pEncCtx);
  }
  pWelsSvcRc->iRemainingBits -= pWelsSvcRc->iFrameDqBits;

  if (pEncCtx->pSvcParam->bEnableFrameSkip /*&&
      pEncCtx->uiDependencyId == pEncCtx->pSvcParam->iSpatialLayerNum - 1*/) {
    RcVBufferCalculationSkip (pEncCtx);
  }

  if (pEncCtx->pSvcParam->iPaddingFlag)
    RcVBufferCalculationPadding (pEncCtx);
  pWelsSvcRc->iFrameCodedInVGop++;
#ifdef _TEST_TEMP_Rc_
  fprintf (fp_test_rc, "%d\n", pWelsSvcRc->iFrameDqBits);
  if (pEncCtx->iSkipFrameFlag)
    fprintf (fp_test_rc, "0\n");
  fflush (fp_test_rc);
#endif
}

void WelsRcMbInitGom (void* pCtx, SMB* pCurMb, SSlice* pSlice) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  const int32_t kiSliceId			= pSlice->uiSliceIdx;
  SRCSlicing* pSOverRc				= &pWelsSvcRc->pSlicingOverRc[kiSliceId];
  SBitStringAux* bs				= pSlice->pSliceBsa;
  SDqLayer* pCurLayer				= pEncCtx->pCurDqLayer;
  const uint8_t kuiChromaQpIndexOffset = pCurLayer->sLayerInfo.pPpsP->uiChromaQpIndexOffset;

  pSOverRc->iBsPosSlice = BsGetBitsPos (bs);

  if (pEncCtx->eSliceType == I_SLICE) {
    pCurMb->uiLumaQp   = pEncCtx->iGlobalQp;
    pCurMb->uiChromaQp = g_kuiChromaQpTable[CLIP3_QP_0_51 (pCurMb->uiLumaQp + kuiChromaQpIndexOffset)];
    return;
  }
  //calculate gom qp and target bits at the beginning of gom
  if (0 == (pCurMb->iMbXY % pWelsSvcRc->iNumberMbGom)) {
    if (pCurMb->iMbXY != pSOverRc->iStartMbSlice) {
      pSOverRc->iComplexityIndexSlice++;
      RcCalculateGomQp (pEncCtx, pCurMb, kiSliceId);
    }
    RcGomTargetBits (pEncCtx, kiSliceId);
  }

  RcCalculateMbQp (pEncCtx, pCurMb, kiSliceId);
}

void WelsRcMbInfoUpdateGom (void* pCtx, SMB* pCurMb, int32_t iCostLuma, SSlice* pSlice) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SBitStringAux* bs				= pSlice->pSliceBsa;
  int32_t iSliceId				= pSlice->uiSliceIdx;
  SRCSlicing* pSOverRc				= &pWelsSvcRc->pSlicingOverRc[iSliceId];
  const int32_t kiComplexityIndex	= pSOverRc->iComplexityIndexSlice;

  int32_t iCurMbBits = BsGetBitsPos (bs) - pSOverRc->iBsPosSlice;
  pSOverRc->iFrameBitsSlice += iCurMbBits;
  pSOverRc->iGomBitsSlice += iCurMbBits;

  pWelsSvcRc->pGomCost[kiComplexityIndex] += iCostLuma;

  pWelsSvcRc->iMinFrameQp = WELS_MIN(pWelsSvcRc->iMinFrameQp,pCurMb->uiLumaQp);
  pWelsSvcRc->iMaxFrameQp = WELS_MAX(pWelsSvcRc->iMaxFrameQp,pCurMb->uiLumaQp);
  if (iCurMbBits > 0) {
    pSOverRc->iTotalQpSlice += pCurMb->uiLumaQp;
    pSOverRc->iTotalMbSlice++;
  }
}

void  WelsRcPictureInitDisable (void* pCtx) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SSpatialLayerConfig* pDLayerParam		= &pEncCtx->pSvcParam->sSpatialLayers[pEncCtx->uiDependencyId];
  const int32_t kiQp = pDLayerParam->iDLayerQp;

  pEncCtx->iGlobalQp	= RcCalculateCascadingQp (pEncCtx, kiQp);

  if (pEncCtx->pSvcParam->bEnableAdaptiveQuant && (pEncCtx->eSliceType == P_SLICE)) {
    pEncCtx->iGlobalQp = WELS_CLIP3 ((pEncCtx->iGlobalQp * INT_MULTIPLY -
                                      pEncCtx->pVaa->sAdaptiveQuantParam.iAverMotionTextureIndexToDeltaQp) / INT_MULTIPLY, GOM_MIN_QP_MODE, GOM_MAX_QP_MODE);
  } else {
    pEncCtx->iGlobalQp = WELS_CLIP3 (pEncCtx->iGlobalQp, 0, 51);
  }

  pWelsSvcRc->iAverageFrameQp = pEncCtx->iGlobalQp;
}

void  WelsRcPictureInfoUpdateDisable (void* pCtx, int32_t iLayerSize) {
}

void  WelsRcMbInitDisable (void* pCtx, SMB* pCurMb, SSlice* pSlice) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  int32_t iLumaQp					= pEncCtx->iGlobalQp;

  SDqLayer* pCurLayer				= pEncCtx->pCurDqLayer;
  const uint8_t kuiChromaQpIndexOffset = pCurLayer->sLayerInfo.pPpsP->uiChromaQpIndexOffset;


  if (pEncCtx->pSvcParam->bEnableAdaptiveQuant && (pEncCtx->eSliceType == P_SLICE)) {
    iLumaQp   = (int8_t)WELS_CLIP3 (iLumaQp +
                                    pEncCtx->pVaa->sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp[pCurMb->iMbXY], GOM_MIN_QP_MODE, 51);
  } else {
    iLumaQp = WELS_CLIP3 (iLumaQp, 0, 51);
  }
  pCurMb->uiChromaQp = g_kuiChromaQpTable[CLIP3_QP_0_51 (iLumaQp + kuiChromaQpIndexOffset)];
  pCurMb->uiLumaQp = iLumaQp;
}

void  WelsRcMbInfoUpdateDisable (void* pCtx, SMB* pCurMb, int32_t iCostLuma, SSlice* pSlice) {
}

void WelRcPictureInitBufferBasedQp (void* pCtx) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SVAAFrameInfo* pVaa			= static_cast<SVAAFrameInfo*> (pEncCtx->pVaa);

  int32_t iMinQp = MIN_SCREEN_QP;
  if (pVaa->eSceneChangeIdc == LARGE_CHANGED_SCENE)
    iMinQp = MIN_SCREEN_QP + 2;
  else if (pVaa->eSceneChangeIdc == MEDIUM_CHANGED_SCENE)
    iMinQp = MIN_SCREEN_QP + 1;
  else
    iMinQp = MIN_SCREEN_QP;
  if (pEncCtx->bDeliveryFlag)
    pEncCtx->iGlobalQp -= 1;
  else
    pEncCtx->iGlobalQp += 2;
  pEncCtx->iGlobalQp = WELS_CLIP3 (pEncCtx->iGlobalQp, iMinQp, MAX_SCREEN_QP);
}

void InitRcModuleScc (void* pCtx) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc =  &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  pWelsSvcRc->iBaseQp = 30;

  pWelsSvcRc->iBufferFullnessSkip = 0;
  pWelsSvcRc->uiLastTimeStamp = 0;

  pWelsSvcRc->iCost2BitsIntra = INT_MULTIPLY;
  pWelsSvcRc->iAvgCost2Bits = INT_MULTIPLY;
}
void WelRcPictureInitScc (void* pCtx) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc =  &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SVAAFrameInfoExt* pVaa = static_cast<SVAAFrameInfoExt*> (pEncCtx->pVaa);
  SSpatialLayerConfig* pDLayerConfig   = &pEncCtx->pSvcParam->sSpatialLayers[pEncCtx->uiDependencyId];
  int32_t iFrameCplx = pVaa->sComplexityScreenParam.iFrameComplexity;
  int32_t iBitRate = pDLayerConfig->iSpatialBitrate;// pEncCtx->pSvcParam->target_bitrate;

  int32_t iBaseQp = pWelsSvcRc->iBaseQp;
  pEncCtx->iGlobalQp = iBaseQp;
  int32_t iDeltaQp = 0;
  if (pEncCtx->eSliceType == I_SLICE) {
    int32_t iTargetBits = iBitRate * 2 - pWelsSvcRc->iBufferFullnessSkip;
    iTargetBits = WELS_MAX (1, iTargetBits);
    int32_t iQstep = WELS_DIV_ROUND ((((int64_t)iFrameCplx) * pWelsSvcRc->iCost2BitsIntra), iTargetBits);
    int32_t iQp = RcConvertQStep2Qp (iQstep);

    pEncCtx->iGlobalQp = WELS_CLIP3 (iQp, MIN_IDR_QP, MAX_IDR_QP);
  } else {
    int32_t iTargetBits = WELS_ROUND (((float)iBitRate / pDLayerConfig->fFrameRate)); //iBitRate / 10;
    int32_t iQstep = (int32_t)(WELS_DIV_ROUND64 (iFrameCplx * pWelsSvcRc->iAvgCost2Bits, iTargetBits));
    int32_t iQp = RcConvertQStep2Qp (iQstep);
    iDeltaQp = iQp - iBaseQp;
    if (pWelsSvcRc->iBufferFullnessSkip > iBitRate) {
      if (iDeltaQp > 0) {
        ++iBaseQp;
      }
    } else if (pWelsSvcRc->iBufferFullnessSkip == 0) {
      if (iDeltaQp < 0) {
        --iBaseQp;
      }
    }
    if (iDeltaQp >= 6) {
      iBaseQp += 3;
    } else if ((iDeltaQp <= -6)) {
      --iBaseQp;
    }
    iBaseQp = WELS_CLIP3 (iBaseQp, MIN_IDR_QP, MAX_IDR_QP);

    pEncCtx->iGlobalQp = iBaseQp;


    if (iDeltaQp < -6) {
      pEncCtx->iGlobalQp = WELS_CLIP3 (pWelsSvcRc->iBaseQp - 6, MIN_SCREEN_QP, MAX_SCREEN_QP);
    }

    if (iDeltaQp > 5) {
      if (LARGE_CHANGED_SCENE == pEncCtx->pVaa->eSceneChangeIdc || pWelsSvcRc->iBufferFullnessSkip > 2 * iBitRate
          || iDeltaQp > 10) {
        pEncCtx->iGlobalQp = WELS_CLIP3 (pWelsSvcRc->iBaseQp + iDeltaQp, MIN_SCREEN_QP, MAX_SCREEN_QP);
      } else if (MEDIUM_CHANGED_SCENE == pEncCtx->pVaa->eSceneChangeIdc || pWelsSvcRc->iBufferFullnessSkip > iBitRate) {
        pEncCtx->iGlobalQp = WELS_CLIP3 (pWelsSvcRc->iBaseQp + 5, MIN_SCREEN_QP, MAX_SCREEN_QP);
      }
    }
    pWelsSvcRc->iBaseQp = iBaseQp;
  }
  pWelsSvcRc->iAverageFrameQp = pEncCtx->iGlobalQp;
  WelsLog (& (pEncCtx->sLogCtx), WELS_LOG_DEBUG, "WelRcPictureInitScc iLumaQp = %d\n", pEncCtx->iGlobalQp);

}
void WelsRcFrameDelayJudgeScc (void* pCtx, EVideoFrameType eFrameType, long long uiTimeStamp) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[0];
  SSpatialLayerConfig* pDLayerConfig   = &pEncCtx->pSvcParam->sSpatialLayers[pEncCtx->uiDependencyId];

  if (pDLayerConfig->iSpatialBitrate > pDLayerConfig->iMaxSpatialBitrate)
    pDLayerConfig->iSpatialBitrate = pDLayerConfig->iMaxSpatialBitrate;

  int32_t iBitRate = pDLayerConfig->iSpatialBitrate;
  int32_t iEncTimeInv = (pWelsSvcRc->uiLastTimeStamp == 0) ? 0 : (int32_t) (uiTimeStamp - pWelsSvcRc->uiLastTimeStamp);
  int32_t iSentBits  = (int32_t) ((double)iBitRate * iEncTimeInv * (1.0E-3) + 0.5);
  iSentBits = WELS_MAX (iSentBits, 0);

  const int32_t iVbufferThRatio = (eFrameType == videoFrameTypeI
                                  || eFrameType == videoFrameTypeIDR) ? VIRTUAL_BUFFER_HIGH_TH : VIRTUAL_BUFFER_LOW_TH;
  const int32_t iVbufferTh = WELS_DIV_ROUND ((((int64_t)iBitRate) * iVbufferThRatio), INT_MULTIPLY);

  pWelsSvcRc->iBufferFullnessSkip -= iSentBits;
  pWelsSvcRc->iBufferFullnessSkip = WELS_MAX (0, pWelsSvcRc->iBufferFullnessSkip);

  pWelsSvcRc->bSkipFlag = true;
  if (pWelsSvcRc->iBufferFullnessSkip < iVbufferTh) {
    pWelsSvcRc->bSkipFlag = false;
  }
  if (pWelsSvcRc->bSkipFlag) {
    pWelsSvcRc->iSkipFrameNum++;
    WelsLog (& (pEncCtx->sLogCtx), WELS_LOG_INFO, "SCC iSkipFrameNum = %d,buffer = %d,threadhold = %d,bitrate = %d,timestamp=%lld\n", pWelsSvcRc->iSkipFrameNum,pWelsSvcRc->iBufferFullnessSkip,iVbufferTh,iBitRate,uiTimeStamp);
  }
  pWelsSvcRc->uiLastTimeStamp = uiTimeStamp;
}

void WelsRcDropFrameUpdate (void* pCtx, uint32_t iDropSize) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[0];

  pWelsSvcRc->iBufferFullnessSkip -= (int32_t)iDropSize;
  pWelsSvcRc->iBufferFullnessSkip = WELS_MAX (0, pWelsSvcRc->iBufferFullnessSkip);
  WelsLog (& (pEncCtx->sLogCtx), WELS_LOG_INFO, "[WelsRcDropFrameUpdate:\tdrop:%d\t%d\n", iDropSize,
           pWelsSvcRc->iBufferFullnessSkip);
}

void WelsRcPictureInfoUpdateScc (void* pCtx, int32_t iNalSize) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;

  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[0];
  int32_t iFrameBits = (iNalSize << 3);
  pWelsSvcRc->iBufferFullnessSkip += iFrameBits;

  SVAAFrameInfoExt* pVaa = static_cast<SVAAFrameInfoExt*> (pEncCtx->pVaa);

  int32_t iQstep = RcConvertQp2QStep (pEncCtx->iGlobalQp);
  int64_t iCost2Bits = WELS_DIV_ROUND64 ((((int64_t)iFrameBits * iQstep)), pVaa->sComplexityScreenParam.iFrameComplexity);

  if (pEncCtx->eSliceType == P_SLICE) {
    pWelsSvcRc->iAvgCost2Bits = WELS_DIV_ROUND64 ((95 * pWelsSvcRc->iAvgCost2Bits + 5 * iCost2Bits), INT_MULTIPLY);
  } else {
    pWelsSvcRc->iCost2BitsIntra = WELS_DIV_ROUND64 ((90 * pWelsSvcRc->iCost2BitsIntra + 10 * iCost2Bits), INT_MULTIPLY);
  }
}


void WelsRcMbInitScc (void* pCtx, SMB* pCurMb, SSlice* pSlice) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;

  /* Get delta iQp of this MB */
  pCurMb->uiLumaQp = pEncCtx->iGlobalQp;
  pCurMb->uiChromaQp = g_kuiChromaQpTable[WELS_CLIP3 (pCurMb->uiLumaQp + pEncCtx->pPps->uiChromaQpIndexOffset, 0, 51)];
}

void  WelsRcInitModule (void* pCtx, RC_MODES iRcMode) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsRcFunc*   pRcf = &pEncCtx->pFuncList->pfRc;

  switch (iRcMode) {
  case RC_OFF_MODE:
    pRcf->pfWelsRcPictureInit = WelsRcPictureInitDisable;
    pRcf->pfWelsRcPicDelayJudge = NULL;
    pRcf->pfWelsRcPictureInfoUpdate = WelsRcPictureInfoUpdateDisable;
    pRcf->pfWelsRcMbInit = WelsRcMbInitDisable;
    pRcf->pfWelsRcMbInfoUpdate = WelsRcMbInfoUpdateDisable;
    break;
  case RC_BUFFERBASED_MODE:
    pRcf->pfWelsRcPictureInit = WelRcPictureInitBufferBasedQp;
    pRcf->pfWelsRcPicDelayJudge = NULL;
    pRcf->pfWelsRcPictureInfoUpdate = WelsRcPictureInfoUpdateDisable;
    pRcf->pfWelsRcMbInit = WelsRcMbInitDisable;
    pRcf->pfWelsRcMbInfoUpdate = WelsRcMbInfoUpdateDisable;
    break;
  case RC_BITRATE_MODE:
    if (pEncCtx->pSvcParam->iUsageType == SCREEN_CONTENT_REAL_TIME) {
      pRcf->pfWelsRcPictureInit = WelRcPictureInitScc;
      pRcf->pfWelsRcPicDelayJudge = WelsRcFrameDelayJudgeScc;
      pRcf->pfWelsRcPictureInfoUpdate = WelsRcPictureInfoUpdateScc;
      pRcf->pfWelsRcMbInit = WelsRcMbInitScc;
      pRcf->pfWelsRcMbInfoUpdate = WelsRcMbInfoUpdateDisable;
      InitRcModuleScc (pEncCtx);

    } else {
      pRcf->pfWelsRcPictureInit = WelsRcPictureInitGom;
      pRcf->pfWelsRcPicDelayJudge = WelsRcFrameDelayJudge;
      pRcf->pfWelsRcPictureInfoUpdate = WelsRcPictureInfoUpdateGom;
      pRcf->pfWelsRcMbInit = WelsRcMbInitGom;
      pRcf->pfWelsRcMbInfoUpdate = WelsRcMbInfoUpdateGom;
    }
    break;
  case RC_QUALITY_MODE:
  default:
    pRcf->pfWelsRcPictureInit = WelsRcPictureInitGom;
    pRcf->pfWelsRcPicDelayJudge = WelsRcFrameDelayJudge;
    pRcf->pfWelsRcPictureInfoUpdate = WelsRcPictureInfoUpdateGom;
    pRcf->pfWelsRcMbInit = WelsRcMbInitGom;
    pRcf->pfWelsRcMbInfoUpdate = WelsRcMbInfoUpdateGom;
    break;
  }

  RcInitSequenceParameter (pEncCtx);
}

void  WelsRcFreeMemory (void* pCtx) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc = NULL;
  int32_t i = 0;
#ifdef _TEST_TEMP_Rc_
  if (fp_test_rc)
    fclose (fp_test_rc);
  fp_test_rc = NULL;
  if (fp_vgop)
    fclose (fp_vgop);
  fp_vgop = NULL;
#endif
  for (i = 0; i < pEncCtx->pSvcParam->iSpatialLayerNum; i++) {
    pWelsSvcRc  = &pEncCtx->pWelsSvcRc[i];
    RcFreeLayerMemory (pWelsSvcRc, pEncCtx->pMemAlign);
  }
}

}//end of namespace
