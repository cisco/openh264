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


namespace WelsSVCEnc {

//#define _TEST_TEMP_RC_
#ifdef _TEST_TEMP_RC_
//#define _NOT_USE_AQ_FOR_TEST_
FILE* fp_test_rc = NULL;
FILE* fp_vgop = NULL;
#endif
#define _BITS_RANGE 0

void RcInitLayerMemory (SWelsSvcRc* pWelsSvcRc, CMemoryAlign* pMA, const int32_t kiMaxTl) {
  const int32_t kiSliceNum			= pWelsSvcRc->iSliceNum;
  const int32_t kiGomSize				= pWelsSvcRc->iGomSize;
  const int32_t kiGomSizeD			= kiGomSize * sizeof (double);
  const int32_t kiGomSizeI			= kiGomSize * sizeof (int32_t);
  const int32_t kiLayerRcSize			= kiGomSizeD + (kiGomSizeI * 3) + sizeof (SRCSlicing) * kiSliceNum + sizeof (
                                      SRCTemporal) * kiMaxTl;
  uint8_t* pBaseMem					= (uint8_t*)pMA->WelsMalloc (kiLayerRcSize, "rc_layer_memory");

  if (NULL == pBaseMem)
    return;

  pWelsSvcRc->pGomComplexity				= (double*)pBaseMem;
  pBaseMem += kiGomSizeD;
  pWelsSvcRc->pGomForegroundBlockNum	= (int32_t*)pBaseMem;
  pBaseMem += kiGomSizeI;
  pWelsSvcRc->pCurrentFrameGomSad		= (int32_t*)pBaseMem;
  pBaseMem += kiGomSizeI;
  pWelsSvcRc->pGomCost					= (int32_t*)pBaseMem;
  pBaseMem += kiGomSizeI;
  pWelsSvcRc->pSlicingOverRc			= (SRCSlicing*)pBaseMem;
  pBaseMem += sizeof (SRCSlicing) * kiSliceNum;
  pWelsSvcRc->pTemporalOverRc			= (SRCTemporal*)pBaseMem;
}

void RcFreeLayerMemory (SWelsSvcRc* pWelsSvcRc, CMemoryAlign* pMA) {
  if (pWelsSvcRc != NULL && pWelsSvcRc->pGomComplexity != NULL) {
    pMA->WelsFree (pWelsSvcRc->pGomComplexity, "rc_layer_memory");
    pWelsSvcRc->pGomComplexity			= NULL;
    pWelsSvcRc->pGomForegroundBlockNum	= NULL;
    pWelsSvcRc->pCurrentFrameGomSad	= NULL;
    pWelsSvcRc->pGomCost				= NULL;
    pWelsSvcRc->pSlicingOverRc			= NULL;
    pWelsSvcRc->pTemporalOverRc		= NULL;
  }
}

static inline double RcConvertQp2QStep (double dQP) {
  return pow (2.0, (dQP - 4.0) / 6.0);
}
static inline double RcConvertQStep2Qp (double dQpStep) {
  return (6 * log (dQpStep) / log (2.0) + 4.0);
}

void RcInitSequenceParameter (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc = NULL;
  SDLayerParam* pDLayerParam = NULL;

  int32_t j = 0;
  int32_t iMbWidth = 0;

  BOOL_T bMultiSliceMode = FALSE;
  int32_t iGomRowMode0 = 1, iGomRowMode1 = 1;
#ifdef _TEST_TEMP_RC_
  fp_test_rc = fopen ("testRC.dat", "w");
  fp_vgop = fopen ("vgop.dat", "w");
#endif
  for (j = 0; j < pEncCtx->pSvcParam->iNumDependencyLayer; j++) {
    SSliceCtx* pSliceCtx = &pEncCtx->pSliceCtxList[j];
    pWelsSvcRc  = &pEncCtx->pWelsSvcRc[j];
    pDLayerParam = &pEncCtx->pSvcParam->sDependencyLayers[j];
    iMbWidth     = (pDLayerParam->iFrameWidth >> 4);
    pWelsSvcRc->iNumberMbFrame = iMbWidth * (pDLayerParam->iFrameHeight >> 4);
    pWelsSvcRc->iSliceNum = pSliceCtx->iSliceNumInFrame;

    pWelsSvcRc->iRcVaryPercentage = _BITS_RANGE;	// % -- for temp
    pWelsSvcRc->dRcVaryRatio = (double)pWelsSvcRc->iRcVaryPercentage / MAX_BITS_VARY_PERCENTAGE;

    pWelsSvcRc->dSkipBufferRatio  = SKIP_RATIO;

    pWelsSvcRc->iQpRangeUpperInFrame = QP_RANGE_UPPER_MODE1 - (int32_t) ((QP_RANGE_UPPER_MODE1 - QP_RANGE_MODE0) *
                                       pWelsSvcRc->dRcVaryRatio + 0.5);
    pWelsSvcRc->iQpRangeLowerInFrame = QP_RANGE_LOWER_MODE1 - (int32_t) ((QP_RANGE_LOWER_MODE1 - QP_RANGE_MODE0) *
                                       pWelsSvcRc->dRcVaryRatio + 0.5);

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
    iGomRowMode0 = iGomRowMode1 + (int32_t) ((iGomRowMode0 - iGomRowMode1) * pWelsSvcRc->dRcVaryRatio + 0.5);

    pWelsSvcRc->iNumberMbGom   = iMbWidth * iGomRowMode0;

    pWelsSvcRc->iMinQp = GOM_MIN_QP_MODE;
    pWelsSvcRc->iMaxQp = GOM_MAX_QP_MODE;

    pWelsSvcRc->iFrameDeltaQpUpper = LAST_FRAME_QP_RANGE_UPPER_MODE1 - (int32_t) ((LAST_FRAME_QP_RANGE_UPPER_MODE1 -
                                     LAST_FRAME_QP_RANGE_UPPER_MODE0) * pWelsSvcRc->dRcVaryRatio + 0.5);
    pWelsSvcRc->iFrameDeltaQpLower = LAST_FRAME_QP_RANGE_LOWER_MODE1 - (int32_t) ((LAST_FRAME_QP_RANGE_LOWER_MODE1 -
                                     LAST_FRAME_QP_RANGE_LOWER_MODE0) * pWelsSvcRc->dRcVaryRatio + 0.5);

    pWelsSvcRc->iSkipFrameNum = 0;
    pWelsSvcRc->iGomSize = (pWelsSvcRc->iNumberMbFrame + pWelsSvcRc->iNumberMbGom - 1) / pWelsSvcRc->iNumberMbGom;


    RcInitLayerMemory (pWelsSvcRc, pEncCtx->pMemAlign, 1 + pDLayerParam->iHighestTemporalId);

    bMultiSliceMode	= ((SM_RASTER_SLICE == pDLayerParam->sMso.uiSliceMode) ||
                       (SM_ROWMB_SLICE	 == pDLayerParam->sMso.uiSliceMode) ||
                       (SM_DYN_SLICE	 == pDLayerParam->sMso.uiSliceMode));
    if (bMultiSliceMode)
      pWelsSvcRc->iNumberMbGom = pWelsSvcRc->iNumberMbFrame;
  }
}


void RcInitTlWeight (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCTemporal* pTOverRc	= pWelsSvcRc->pTemporalOverRc;
  SDLayerParam* pDLayerParam =  &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId];
  const int32_t kiDecompositionStages = pDLayerParam->iDecompositionStages;
  const int32_t kiHighestTid = pDLayerParam->iHighestTemporalId;

  //Index 0:Virtual GOP size, Index 1:Frame rate
  double WeightArray[4][4] = { {1.0, 0, 0, 0}, {0.6, 0.4, 0, 0}, {0.4, 0.3, 0.15, 0}, {0.25, 0.15, 0.125, 0.0875}};
  const int32_t kiGopSize = (1 << kiDecompositionStages);
  int32_t i, k, n;

  n = 0;
  while (n <= kiHighestTid) {
    pTOverRc[n].dTlayerWeight	= WeightArray[kiDecompositionStages][n];
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
  SDLayerParam* pDLayerParam     = &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId];
  const int32_t kiGopSize	= (1 << pDLayerParam->iDecompositionStages);
  const int32_t kiHighestTid = pDLayerParam->iHighestTemporalId;
  double input_dBitsPerFrame = pDLayerParam->iSpatialBitrate / pDLayerParam->fInputFrameRate;
  const int32_t kiGopBits	= (int32_t) (input_dBitsPerFrame * kiGopSize);
  int32_t i;

  pWelsSvcRc->iBitRate   = pDLayerParam->iSpatialBitrate;
  pWelsSvcRc->fFrameRate = pDLayerParam->fInputFrameRate;

  double dTargetVaryRange = FRAME_iTargetBits_VARY_RANGE * (1.0 - pWelsSvcRc->dRcVaryRatio);
  double dMinBitsRatio = 1.0 - dTargetVaryRange;
  double dMaxBitsRatio = 1.0 + FRAME_iTargetBits_VARY_RANGE;//dTargetVaryRange;

  for (i = 0; i <= kiHighestTid; i++) {
    const double kdConstraitBits = kiGopBits * pTOverRc[i].dTlayerWeight;
    pTOverRc[i].iMinBitsTl = (int32_t) (kdConstraitBits * dMinBitsRatio);
    pTOverRc[i].iMaxBitsTl = (int32_t) (kdConstraitBits * dMaxBitsRatio);
  }
  //When bitrate is changed, pBuffer size should be updated
  pWelsSvcRc->iBufferSizeSkip = (int32_t) (pWelsSvcRc->iBitRate * pWelsSvcRc->dSkipBufferRatio);
  pWelsSvcRc->iBufferSizePadding = (int32_t) (pWelsSvcRc->iBitRate * PADDING_BUFFER_RATIO);

  //change remaining bits
  if (pWelsSvcRc->dBitsPerFrame > 0.1)
    pWelsSvcRc->iRemainingBits = (int32_t) (pWelsSvcRc->iRemainingBits * input_dBitsPerFrame / pWelsSvcRc->dBitsPerFrame);
  pWelsSvcRc->dBitsPerFrame = input_dBitsPerFrame;
}


void RcInitVGop (sWelsEncCtx* pEncCtx) {
  const int32_t kiDid		= pEncCtx->uiDependencyId;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[kiDid];
  SRCTemporal* pTOverRc		= pWelsSvcRc->pTemporalOverRc;
  const int32_t kiHighestTid = pEncCtx->pSvcParam->sDependencyLayers[kiDid].iHighestTemporalId;

  pWelsSvcRc->iRemainingBits = (int32_t) (VGOP_SIZE * pWelsSvcRc->dBitsPerFrame);
  pWelsSvcRc->dRemainingWeights = pWelsSvcRc->iGopNumberInVGop;

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
  SDLayerParam* pDLayerParam       = &pEncCtx->pSvcParam->sDependencyLayers[kiDid];
  const int32_t kiHighestTid = pDLayerParam->iHighestTemporalId;
  int32_t i;

  //I frame R-Q Model
  pWelsSvcRc->iIntraComplexity = 0;
  pWelsSvcRc->iIntraMbCount = 0;

  //P frame R-Q Model
  for (i = 0; i <= kiHighestTid; i++) {
    pTOverRc[i].iPFrameNum = 0;
    pTOverRc[i].dLinearCmplx = 0.0;
    pTOverRc[i].iFrameCmplxMean = 0;
  }

  pWelsSvcRc->iBufferFullnessSkip = 0;
  pWelsSvcRc->iBufferFullnessPadding = 0;

  pWelsSvcRc->iGopIndexInVGop = 0;
  pWelsSvcRc->iRemainingBits = 0;
  pWelsSvcRc->dBitsPerFrame	= 0.0;

  //Backup the initial bitrate and fps
  pWelsSvcRc->iPreviousBitrate  = pDLayerParam->iSpatialBitrate;
  pWelsSvcRc->dPreviousFps      = pDLayerParam->fInputFrameRate;

  memset (pWelsSvcRc->pCurrentFrameGomSad, 0, pWelsSvcRc->iGomSize * sizeof (int32_t));

  RcInitTlWeight (pEncCtx);
  RcUpdateBitrateFps (pEncCtx);
  RcInitVGop (pEncCtx);
}

bool_t RcJudgeBitrateFpsUpdate (sWelsEncCtx* pEncCtx) {
  int32_t iCurDid = pEncCtx->uiDependencyId;
  SWelsSvcRc* pWelsSvcRc       = &pEncCtx->pWelsSvcRc[iCurDid];
  SDLayerParam* pDLayerParam    = &pEncCtx->pSvcParam->sDependencyLayers[iCurDid];

  if ((pWelsSvcRc->iPreviousBitrate != pDLayerParam->iSpatialBitrate) ||
      (pWelsSvcRc->dPreviousFps - pDLayerParam->fInputFrameRate) > EPSN ||
      (pWelsSvcRc->dPreviousFps - pDLayerParam->fInputFrameRate) < -EPSN) {
    pWelsSvcRc->iPreviousBitrate = pDLayerParam->iSpatialBitrate;
    pWelsSvcRc->dPreviousFps = pDLayerParam->fInputFrameRate;
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
      iVGopBitrate = (int32_t) (iTotalBits / iFrameInVGop * pWelsSvcRc->fFrameRate);
#ifdef _TEST_TEMP_Rc_
    fprintf (fp_vgop, "%d\n", (int32_t) ((double)iTotalBits / iFrameInVGop));
#endif
    WelsLog (pEncCtx, WELS_LOG_INFO, "[Rc] VGOPbitrate%d: %d \n", kiDid, iVGopBitrate);
    if (iTotalBits > 0) {
      iTid = 0;
      while (iTid <= kiHighestTid) {
        WelsLog (pEncCtx, WELS_LOG_INFO, "T%d=%8.3f \n", iTid, (double) (pTOverRc[iTid].iGopBitsDq / iTotalBits));
        ++ iTid;
      }
    }
  }
}
#endif

void RcUpdateTemporalZero (sWelsEncCtx* pEncCtx) {
  const int32_t kiDid		= pEncCtx->uiDependencyId;
  SWelsSvcRc* pWelsSvcRc	= &pEncCtx->pWelsSvcRc[kiDid];
  SDLayerParam* pDLayerParam		= &pEncCtx->pSvcParam->sDependencyLayers[kiDid];
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
  SDLayerParam* pDLayerParam			= &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId];

  if (pDLayerParam->fOutputFrameRate > EPSN && pDLayerParam->iFrameWidth && pDLayerParam->iFrameHeight)
    dBpp = (double) (pDLayerParam->iSpatialBitrate) / (double) (pDLayerParam->fOutputFrameRate * pDLayerParam->iFrameWidth *
           pDLayerParam->iFrameHeight);
  else
    dBpp = 0.1;

  //Area*2
  if (pDLayerParam->iFrameWidth * pDLayerParam->iFrameHeight <= 28800) // 90p video:160*90
    iBppIndex = 0;
  else if (pDLayerParam->iFrameWidth * pDLayerParam->iFrameHeight <= 115200) // 180p video:320*180
    iBppIndex = 1;
  else if (pDLayerParam->iFrameWidth * pDLayerParam->iFrameHeight <= 460800) // 360p video:640*360
    iBppIndex = 2;
  else
    iBppIndex = 3;

  //Search
  for (i = 0; i < 3; i++) {
    if (dBpp <= dBppArray[iBppIndex][i])
      break;
  }
  pWelsSvcRc->iInitialQp = dInitialQPArray[iBppIndex][i];
  pWelsSvcRc->iInitialQp = (int32_t)WELS_CLIP3 (pWelsSvcRc->iInitialQp, MIN_IDR_QP, MAX_IDR_QP);
  pEncCtx->iGlobalQp = pWelsSvcRc->iInitialQp;
  pWelsSvcRc->dQStep = RcConvertQp2QStep (pEncCtx->iGlobalQp);
  pWelsSvcRc->iLastCalculatedQScale = pEncCtx->iGlobalQp;
}

void RcCalculateIdrQp (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  //obtain the idr qp using previous idr complexity
  if (pWelsSvcRc->iNumberMbFrame != pWelsSvcRc->iIntraMbCount) {
    pWelsSvcRc->iIntraComplexity = (int32_t) ((double)pWelsSvcRc->iIntraComplexity * pWelsSvcRc->iNumberMbFrame /
                                   pWelsSvcRc->iIntraMbCount + 0.5);
  }
  pWelsSvcRc->iInitialQp = (int32_t)RcConvertQStep2Qp ((double)pWelsSvcRc->iIntraComplexity / pWelsSvcRc->iTargetBits);
  pWelsSvcRc->iInitialQp = (int32_t)WELS_CLIP3 (pWelsSvcRc->iInitialQp, MIN_IDR_QP, MAX_IDR_QP);
  pEncCtx->iGlobalQp = pWelsSvcRc->iInitialQp;
  pWelsSvcRc->dQStep = RcConvertQp2QStep (pEncCtx->iGlobalQp);
  pWelsSvcRc->iLastCalculatedQScale = pEncCtx->iGlobalQp;
}


void RcCalculatePictureQp (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc		= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  int32_t iTl					= pEncCtx->uiTemporalId;
  SRCTemporal* pTOverRc			= &pWelsSvcRc->pTemporalOverRc[iTl];
  int32_t iLumaQp = 0;

  if (0 == pTOverRc->iPFrameNum) {
    iLumaQp = pWelsSvcRc->iInitialQp;
  } else {
    double dCmplxRatio = (double)pEncCtx->pVaa->sComplexityAnalysisParam.iFrameComplexity / pTOverRc->iFrameCmplxMean;
    dCmplxRatio = WELS_CLIP3 (dCmplxRatio, 1.0 - FRAME_CMPLX_RATIO_RANGE, 1.0 + FRAME_CMPLX_RATIO_RANGE);

    pWelsSvcRc->dQStep = pTOverRc->dLinearCmplx * dCmplxRatio / pWelsSvcRc->iTargetBits;
    iLumaQp = (int32_t) (RcConvertQStep2Qp (pWelsSvcRc->dQStep) + 0.5);

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

  pWelsSvcRc->dQStep = RcConvertQp2QStep (iLumaQp);
  pWelsSvcRc->iLastCalculatedQScale = iLumaQp;
#ifndef _NOT_USE_AQ_FOR_TEST_
  if (pEncCtx->pSvcParam->bEnableAdaptiveQuant) {

    iLumaQp = (int32_t)WELS_CLIP3 (iLumaQp - pEncCtx->pVaa->sAdaptiveQuantParam.dAverMotionTextureIndexToDeltaQp,
                                   pWelsSvcRc->iMinQp, pWelsSvcRc->iMaxQp);
  }
#endif
  pEncCtx->iGlobalQp = iLumaQp;
}

void RcInitSliceInformation (sWelsEncCtx* pEncCtx) {
  SSliceCtx* pCurSliceCtx	= pEncCtx->pCurDqLayer->pSliceEncCtx;
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCSlicing* pSOverRc				= &pWelsSvcRc->pSlicingOverRc[0];
  const int32_t kiSliceNum			= pCurSliceCtx->iSliceNumInFrame;
  const double kdBitsPerMb		= (double)pWelsSvcRc->iTargetBits / pWelsSvcRc->iNumberMbFrame;

  for (int32_t i = 0; i < kiSliceNum; i++) {
    pSOverRc->iStartMbSlice	=
      pSOverRc->iEndMbSlice		= pCurSliceCtx->pFirstMbInSlice[i];
    pSOverRc->iEndMbSlice		+= (pCurSliceCtx->pCountMbNumInSlice[i] - 1);
    pSOverRc->iTotalQpSlice	= 0;
    pSOverRc->iTotalMbSlice	= 0;
    pSOverRc->iTargetBitsSlice = (int32_t) (kdBitsPerMb * pCurSliceCtx->pCountMbNumInSlice[i]);
    pSOverRc->iFrameBitsSlice	= 0;
    pSOverRc->iGomBitsSlice	= 0;
    ++ pSOverRc;
  }
}

void RcDecideTargetBits (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc	= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCTemporal* pTOverRc		= &pWelsSvcRc->pTemporalOverRc[pEncCtx->uiTemporalId];
  //allocate bits
  if (pEncCtx->eSliceType == I_SLICE) {
    pWelsSvcRc->iTargetBits = (int32_t) (pWelsSvcRc->dBitsPerFrame * IDR_BITRATE_RATIO);
  } else {
    pWelsSvcRc->iTargetBits = (int32_t) (pWelsSvcRc->iRemainingBits * pTOverRc->dTlayerWeight /
                                         pWelsSvcRc->dRemainingWeights);
    pWelsSvcRc->iTargetBits = WELS_CLIP3 (pWelsSvcRc->iTargetBits, pTOverRc->iMinBitsTl,	pTOverRc->iMaxBitsTl);
  }
  pWelsSvcRc->dRemainingWeights -= pTOverRc->dTlayerWeight;
}


void RcInitGomParameters (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCSlicing* pSOverRc				= &pWelsSvcRc->pSlicingOverRc[0];
  const int32_t kiSliceNum			= pWelsSvcRc->iSliceNum;
  const int32_t kiGlobalQp			= pEncCtx->iGlobalQp;

  pWelsSvcRc->iAverageFrameQp = 0;
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

#ifndef _NOT_USE_AQ_FOR_TEST_
  if (pEncCtx->pSvcParam->bEnableAdaptiveQuant) {
    iLumaQp   = (int8_t)WELS_CLIP3 (iLumaQp +
                                    pEncCtx->pVaa->sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp[pCurMb->iMbXY], pWelsSvcRc->iMinQp, 51);
  }
#endif
  pCurMb->uiChromaQp	= g_kuiChromaQpTable[iLumaQp];
  pCurMb->uiLumaQp		= iLumaQp;
}

SWelsSvcRc* RcJudgeBaseUsability (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc  = NULL, *pWelsSvcRc_Base = NULL;
  SDLayerParam* pDlpBase = NULL, *pDLayerParam = NULL;

  if (pEncCtx->uiDependencyId <= 0)
    return NULL;

  pDlpBase = &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId - 1];
  pWelsSvcRc_Base = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId - 1];
  if (pEncCtx->uiTemporalId <= pDlpBase->iDecompositionStages) {
    pWelsSvcRc      = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
    pWelsSvcRc_Base = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId - 1];
    pDLayerParam             = &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId];
    pDlpBase        = &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId - 1];
    if ((pDLayerParam->iFrameWidth * pDLayerParam->iFrameHeight / pWelsSvcRc->iNumberMbGom) ==
        (pDlpBase->iFrameWidth * pDlpBase->iFrameHeight / pWelsSvcRc_Base->iNumberMbGom))
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

  double dAllocateBits = 0;
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
    dAllocateBits = iLeftBits;
  } else {
    pWelsSvcRc_Base = RcJudgeBaseUsability (pEncCtx);
    pWelsSvcRc_Base = (pWelsSvcRc_Base) ? pWelsSvcRc_Base : pWelsSvcRc;
    for (i = kiComplexityIndex; i <= iLastGomIndex; i++) {
      iSumSad += pWelsSvcRc_Base->pCurrentFrameGomSad[i];
    }
    if (0 == iSumSad)
      dAllocateBits = (double)iLeftBits / (iLastGomIndex - kiComplexityIndex);
    else
      dAllocateBits = (double)iLeftBits * pWelsSvcRc_Base->pCurrentFrameGomSad[kiComplexityIndex + 1] / iSumSad;

  }
  pSOverRc->iGomTargetBits = int32_t (dAllocateBits + 0.5);
}



void RcCalculateGomQp (sWelsEncCtx* pEncCtx, SMB* pCurMb, int32_t iSliceId) {
  SWelsSvcRc* pWelsSvcRc			= &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCSlicing* pSOverRc				= &pWelsSvcRc->pSlicingOverRc[iSliceId];
  double dBitsRatio = 1.0;

  int32_t iLeftBits = pSOverRc->iTargetBitsSlice - pSOverRc->iFrameBitsSlice;
  int32_t iTargetLeftBits = iLeftBits + pSOverRc->iGomBitsSlice - pSOverRc->iGomTargetBits;

  if (iLeftBits <= 0) {
    pSOverRc->iCalculatedQpSlice += 2;
  } else {
    //globe decision
    dBitsRatio = iLeftBits / (iTargetLeftBits + 0.1);
    if (dBitsRatio < 0.8409)		//2^(-1.5/6)
      pSOverRc->iCalculatedQpSlice += 2;
    else if (dBitsRatio < 0.9439)	//2^(-0.5/6)
      pSOverRc->iCalculatedQpSlice += 1;
    else if (dBitsRatio > 1.06)		//2^(0.5/6)
      pSOverRc->iCalculatedQpSlice -= 1;
    else if (dBitsRatio > 1.19)		//2^(1.5/6)
      pSOverRc->iCalculatedQpSlice -= 2;
  }

  pSOverRc->iCalculatedQpSlice = WELS_CLIP3 (pSOverRc->iCalculatedQpSlice,
                                 pEncCtx->iGlobalQp - pWelsSvcRc->iQpRangeLowerInFrame, pEncCtx->iGlobalQp + pWelsSvcRc->iQpRangeUpperInFrame);
  pSOverRc->iCalculatedQpSlice = WELS_CLIP3 (pSOverRc->iCalculatedQpSlice, pWelsSvcRc->iMinQp, pWelsSvcRc->iMaxQp);

  pSOverRc->iGomBitsSlice = 0;

}

void   RcVBufferCalculationSkip (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SRCTemporal* pTOverRc		= pWelsSvcRc->pTemporalOverRc;
  const int32_t kiOutputBits = (int32_t) (pWelsSvcRc->dBitsPerFrame + 0.5);
  //condition 1: whole pBuffer fullness
  pWelsSvcRc->iBufferFullnessSkip += (pWelsSvcRc->iFrameDqBits - kiOutputBits);
  //condition 2: VGOP bits constraint
  const int32_t kiVGopBits = (int32_t) (pWelsSvcRc->dBitsPerFrame * VGOP_SIZE);
  int32_t iVGopBitsPred = 0;
  for (int32_t i = pWelsSvcRc->iFrameCodedInVGop + 1; i < VGOP_SIZE; i++)
    iVGopBitsPred += pTOverRc[pWelsSvcRc->iTlOfFrames[i]].iMinBitsTl;
  iVGopBitsPred -= pWelsSvcRc->iRemainingBits;
  double dIncPercent = iVGopBitsPred * 100.0 / kiVGopBits - (double)VGOP_BITS_PERCENTAGE_DIFF;

  if ((pWelsSvcRc->iBufferFullnessSkip > pWelsSvcRc->iBufferSizeSkip
       &&	pWelsSvcRc->iAverageFrameQp > pWelsSvcRc->iSkipQpValue)
      || (dIncPercent > pWelsSvcRc->iRcVaryPercentage)) {
    pEncCtx->iSkipFrameFlag = 1;
    pWelsSvcRc->iBufferFullnessSkip = pWelsSvcRc->iBufferFullnessSkip - kiOutputBits;
#ifdef FRAME_INFO_OUTPUT
    fprintf (stderr, "skip one frame\n");
#endif
  }

  if (pWelsSvcRc->iBufferFullnessSkip < 0)
    pWelsSvcRc->iBufferFullnessSkip = 0;

  if (pEncCtx->iSkipFrameFlag == 1) {
    pWelsSvcRc->iRemainingBits += (int32_t) (pWelsSvcRc->dBitsPerFrame + 0.5);
    pWelsSvcRc->iSkipFrameNum++;
    pWelsSvcRc->iSkipFrameInVGop++;
  }
}

void RcVBufferCalculationPadding (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  const int32_t kiOutputBits = (int32_t) (pWelsSvcRc->dBitsPerFrame + 0.5);
  const int32_t kiBufferThreshold = (int32_t) (PADDING_THRESHOLD * (-pWelsSvcRc->iBufferSizePadding));

  pWelsSvcRc->iBufferFullnessPadding += (pWelsSvcRc->iFrameDqBits - kiOutputBits);

  if (pWelsSvcRc->iBufferFullnessPadding < kiBufferThreshold) {
    pWelsSvcRc->iPaddingSize = -pWelsSvcRc->iBufferFullnessPadding;
    pWelsSvcRc->iPaddingSize >>= 3;	// /8
    pWelsSvcRc->iBufferFullnessPadding = 0;
  } else
    pWelsSvcRc->iPaddingSize = 0;
}


void RcTraceFrameBits (sWelsEncCtx* pEncCtx) {
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];

  WelsLog (pEncCtx, WELS_LOG_INFO,
           "[Rc] encoding_qp%d, qp = %3d, index = %8d, iTid = %1d, used = %8d, target = %8d, remaingbits = %8d\n",
           pEncCtx->uiDependencyId, pWelsSvcRc->iAverageFrameQp, pEncCtx->uiFrameIdxRc, pEncCtx->uiTemporalId,
           pWelsSvcRc->iFrameDqBits,
           pWelsSvcRc->iTargetBits, pWelsSvcRc->iRemainingBits);
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
      pWelsSvcRc->iAverageFrameQp = (int32_t) (1.0 * iTotalQp / iTotalMb + 0.5);
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
  double iAlpha = 1.0 / (1 + pWelsSvcRc->iIdrNum);
  if (iAlpha < 0.25) iAlpha = 0.25;

  double dIntraCmplx = pWelsSvcRc->dQStep * pWelsSvcRc->iFrameDqBits;
  dIntraCmplx = (1.0 - iAlpha) * pWelsSvcRc->iIntraComplexity + iAlpha * dIntraCmplx;
  pWelsSvcRc->iIntraComplexity = (int32_t) (dIntraCmplx + 0.5);
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
    pTOverRc->dLinearCmplx = pWelsSvcRc->iFrameDqBits * pWelsSvcRc->dQStep;
  } else {
    pTOverRc->dLinearCmplx = LINEAR_MODEL_DECAY_FACTOR * pTOverRc->dLinearCmplx
                             + (1.0 - LINEAR_MODEL_DECAY_FACTOR) * (pWelsSvcRc->iFrameDqBits * pWelsSvcRc->dQStep);
  }
  double iAlpha = 1.0 / (1 + pTOverRc->iPFrameNum);
  if (iAlpha < SMOOTH_FACTOR_MIN_VALUE)
    iAlpha = SMOOTH_FACTOR_MIN_VALUE;
  pTOverRc->iFrameCmplxMean = (int32_t) ((1.0 - iAlpha) * pTOverRc->iFrameCmplxMean + iAlpha *
                                         pEncCtx->pVaa->sComplexityAnalysisParam.iFrameComplexity + 0.5);

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



void  WelsRcPictureInfoUpdateGom (void* pCtx, int32_t layer_size) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  int32_t iCodedBits = (layer_size << 3);

  RcUpdatePictureQpBits (pEncCtx, iCodedBits);

  if (pEncCtx->eSliceType == P_SLICE) {
    RcUpdateFrameComplexity (pEncCtx);
  } else {
    RcUpdateIntraComplexity (pEncCtx);
  }
  pWelsSvcRc->iRemainingBits -= pWelsSvcRc->iFrameDqBits;

#if GOM_TRACE_FLAG
  RcTraceFrameBits (pEncCtx);
#endif


  if (pEncCtx->pSvcParam->bEnableFrameSkip &&
      pEncCtx->uiDependencyId == pEncCtx->pSvcParam->iNumDependencyLayer - 1) {
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


  pSOverRc->iBsPosSlice = BsGetBitsPos (bs);

  if (pEncCtx->eSliceType == I_SLICE)
    return;
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

  int32_t cur_mb_bits = BsGetBitsPos (bs) - pSOverRc->iBsPosSlice;
  pSOverRc->iFrameBitsSlice += cur_mb_bits;
  pSOverRc->iGomBitsSlice += cur_mb_bits;

  pWelsSvcRc->pGomCost[kiComplexityIndex] += iCostLuma;

  if (cur_mb_bits > 0) {
    pSOverRc->iTotalQpSlice += pCurMb->uiLumaQp;
    pSOverRc->iTotalMbSlice++;
  }
}

void  WelsRcPictureInitDisable (void* pCtx) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsSvcRc* pWelsSvcRc = &pEncCtx->pWelsSvcRc[pEncCtx->uiDependencyId];
  SDLayerParam* pDLayerParam		= &pEncCtx->pSvcParam->sDependencyLayers[pEncCtx->uiDependencyId];

  const int32_t kiQp = pDLayerParam->iDLayerQp;

  pEncCtx->iGlobalQp	= RcCalculateCascadingQp (pEncCtx, kiQp);

  if (pEncCtx->pSvcParam->bEnableAdaptiveQuant && (pEncCtx->eSliceType == P_SLICE)) {
    pEncCtx->iGlobalQp = (int32_t)WELS_CLIP3 (pEncCtx->iGlobalQp -
                         pEncCtx->pVaa->sAdaptiveQuantParam.dAverMotionTextureIndexToDeltaQp, GOM_MIN_QP_MODE, GOM_MAX_QP_MODE);
  }
  pWelsSvcRc->iAverageFrameQp = pEncCtx->iGlobalQp;
}

void  WelsRcPictureInfoUpdateDisable (void* pCtx, int32_t layer_size) {
}

void  WelsRcMbInitDisable (void* pCtx, SMB* pCurMb, SSlice* pSlice) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  int32_t iLumaQp					= pEncCtx->iGlobalQp;

  if (pEncCtx->pSvcParam->bEnableAdaptiveQuant && (pEncCtx->eSliceType == P_SLICE)) {
    iLumaQp   = (int8_t)WELS_CLIP3 (iLumaQp +
                                    pEncCtx->pVaa->sAdaptiveQuantParam.pMotionTextureIndexToDeltaQp[pCurMb->iMbXY], GOM_MIN_QP_MODE, 51);
  }
  pCurMb->uiChromaQp = g_kuiChromaQpTable[iLumaQp];
  pCurMb->uiLumaQp = iLumaQp;
}

void  WelsRcMbInfoUpdateDisable (void* pCtx, SMB* pCurMb, int32_t iCostLuma, SSlice* pSlice) {
}


void  WelsRcInitModule (void* pCtx,  int32_t iModule) {
  sWelsEncCtx* pEncCtx = (sWelsEncCtx*)pCtx;
  SWelsRcFunc*   pRcf = &pEncCtx->pFuncList->pfRc;

  switch (iModule) {
  case WELS_RC_DISABLE:
    pRcf->pfWelsRcPictureInit = WelsRcPictureInitDisable;
    pRcf->pfWelsRcPictureInfoUpdate = WelsRcPictureInfoUpdateDisable;
    pRcf->pfWelsRcMbInit = WelsRcMbInitDisable;
    pRcf->pfWelsRcMbInfoUpdate = WelsRcMbInfoUpdateDisable;
    break;
  case WELS_RC_GOM:
  default:
    pRcf->pfWelsRcPictureInit = WelsRcPictureInitGom;
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
  for (i = 0; i < pEncCtx->pSvcParam->iNumDependencyLayer; i++) {
    pWelsSvcRc  = &pEncCtx->pWelsSvcRc[i];
    RcFreeLayerMemory (pWelsSvcRc, pEncCtx->pMemAlign);
  }
}

}//end of namespace
