/*!
 * \copy
 *     Copyright (c)  2011-2013, Cisco Systems
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
 * \file	wels_preprocess.h
 *
 * \brief	interface of video pre-process plugins
 *
 * \date	03/15/2011
 *
 * \description : this class is designed as an interface to unify video pre-processing
 *                class implement sets such as denoise,colorspace conversion etc...
 *
 *************************************************************************************
 */

#ifndef WELS_PREPROCESS_H
#define WELS_PREPROCESS_H

#include "typedefs.h"
#include "picture.h"
#include "wels_const.h"
#include "IWelsVP.h"
#include "param_svc.h"

namespace WelsSVCEnc {

typedef struct TagWelsEncCtx sWelsEncCtx;

typedef  struct {
  SPicture*	pScaledInputPicture;
  int32_t		iScaledWidth[MAX_DEPENDENCY_LAYER];
  int32_t     iScaledHeight[MAX_DEPENDENCY_LAYER];
} Scaled_Picture;

typedef struct {
  SVAACalcResult		sVaaCalcInfo;
  SAdaptiveQuantizationParam sAdaptiveQuantParam;
  SComplexityAnalysisParam sComplexityAnalysisParam;

  int32_t			iPicWidth;			// maximal iWidth of picture in samples for svc coding
  int32_t			iPicHeight;			// maximal iHeight of picture in samples for svc coding
  int32_t         iPicStride;         //luma
  int32_t			iPicStrideUV;

  uint8_t*         pRefY; //pRef
  uint8_t*         pCurY; //cur
  uint8_t*         pRefU; //pRef
  uint8_t*         pCurU; //cur
  uint8_t*         pRefV; //pRef
  uint8_t*         pCurV; //cur

  int8_t*			pVaaBackgroundMbFlag;
  uint8_t         uiValidLongTermPicIdx;
  uint8_t         uiMarkLongTermPicIdx;

  bool          bSceneChangeFlag;
  bool          bIdrPeriodFlag;
} SVAAFrameInfo;

class CWelsLib {
 public:
  CWelsLib (sWelsEncCtx* pEncCtx);
  virtual  ~CWelsLib();

  int32_t CreateIface (IWelsVP** ppInterfaceVp);
  int32_t DestroyIface (IWelsVP* pInterfaceVp);

 protected:
  void* QueryFunction (const char* pName);

 private:
  void* m_pVpLib;
  void* m_pInterface[2];
};

class CWelsPreProcess {
 public:
  CWelsPreProcess (sWelsEncCtx* pEncCtx);
  virtual  ~CWelsPreProcess();

 public:
  int32_t WelsPreprocessReset (sWelsEncCtx* pEncCtx);
  int32_t AllocSpatialPictures (sWelsEncCtx* pCtx, SWelsSvcCodingParam* pParam);
  void    FreeSpatialPictures (sWelsEncCtx* pCtx);
  int32_t BuildSpatialPicList (sWelsEncCtx* pEncCtx, const SSourcePicture* kpSrcPic);
  int32_t AnalyzeSpatialPic (sWelsEncCtx* pEncCtx, const int32_t kiDIdx);
  int32_t UpdateSpatialPictures(sWelsEncCtx* pEncCtx, SWelsSvcCodingParam* pParam, const int8_t iCurTid, const int32_t d_idx);

 private:
  int32_t WelsPreprocessCreate();
  int32_t WelsPreprocessDestroy();
  int32_t InitLastSpatialPictures (sWelsEncCtx* pEncCtx);

 private:
  int32_t SingleLayerPreprocess (sWelsEncCtx* pEncCtx, const SSourcePicture* kpSrc, Scaled_Picture* m_sScaledPicture);
  int32_t MultiLayerPreprocess (sWelsEncCtx* pEncCtx, const SSourcePicture** kppSrcPicList, const int32_t kiSpatialNum);

  void	BilateralDenoising (SPicture* pSrc, const int32_t iWidth, const int32_t iHeight);
  bool  DetectSceneChange (SPicture* pCurPicture, SPicture* pRefPicture);
  int32_t DownsamplePadding (SPicture* pSrc, SPicture* pDstPic,  int32_t iSrcWidth, int32_t iSrcHeight,
                             int32_t iShrinkWidth, int32_t iShrinkHeight, int32_t iTargetWidth, int32_t iTargetHeight);

  void    VaaCalculation (SVAAFrameInfo* pVaaInfo, SPicture* pCurPicture, SPicture* pRefPicture, bool bCalculateSQDiff,
                          bool bCalculateVar, bool bCalculateBGD);
  void    BackgroundDetection (SVAAFrameInfo* pVaaInfo, SPicture* pCurPicture, SPicture* pRefPicture, bool bDetectFlag);
  void    AdaptiveQuantCalculation (SVAAFrameInfo* pVaaInfo, SPicture* pCurPicture, SPicture* pRefPicture);
  void    AnalyzePictureComplexity (sWelsEncCtx* pCtx, SPicture* pCurPicture, SPicture* pRefPicture,
                                    const int32_t kiDependencyId, const bool kbCalculateBGD);
  void    Padding (uint8_t* pSrcY, uint8_t* pSrcU, uint8_t* pSrcV, int32_t iStrideY, int32_t iStrideUV,
                   int32_t iActualWidth, int32_t iPaddingWidth, int32_t iActualHeight, int32_t iPaddingHeight);
  void    SetRefMbType (sWelsEncCtx* pCtx, uint32_t** pRefMbTypeArray, int32_t iRefPicType);

  int32_t ColorspaceConvert (SWelsSvcCodingParam* pSvcParam, SPicture* pDstPic, const SSourcePicture* kpSrc,
                             const int32_t kiWidth, const int32_t kiHeight);
  void WelsMoveMemoryWrapper (SWelsSvcCodingParam* pSvcParam, SPicture* pDstPic, const SSourcePicture* kpSrc,
                              const int32_t kiWidth, const int32_t kiHeight);

 private:
  Scaled_Picture   m_sScaledPicture;
  SPicture*	   m_pLastSpatialPicture[MAX_DEPENDENCY_LAYER][2];
  IWelsVP*         m_pInterfaceVp;
  CWelsLib*        m_pEncLib;
  sWelsEncCtx*     m_pEncCtx;
  bool             m_bInitDone;
  bool             m_bOfficialBranch;
  /* For Downsampling & VAA I420 based source pictures */
  SPicture*        m_pSpatialPic[MAX_DEPENDENCY_LAYER][MAX_TEMPORAL_LEVEL + 1 +
      LONG_TERM_REF_NUM];	// need memory requirement with total number of (log2(uiGopSize)+1+1+long_term_ref_num)
  uint8_t          m_uiSpatialLayersInTemporal[MAX_DEPENDENCY_LAYER];
  uint8_t          m_uiSpatialPicNum[MAX_DEPENDENCY_LAYER];
};

}

#endif
