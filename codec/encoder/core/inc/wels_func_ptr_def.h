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

// wels_func_ptr_def.h
#ifndef WELS_ENCODER_FUNCTION_POINTERS_DEFINITION_H_
#define WELS_ENCODER_FUNCTION_POINTERS_DEFINITION_H_

#include "typedefs.h"
#include "wels_common_basis.h"
#include "svc_enc_macroblock.h"
#include "mb_cache.h"
#include "slice.h"
#include "svc_enc_slice_segment.h"
#include "svc_enc_frame.h"
#include "expand_pic.h"
#include "rc.h"

namespace WelsSVCEnc {

typedef struct TagWelsFuncPointerList SWelsFuncPtrList;

typedef void (*PSetMemoryZero) (void* pDst, int32_t iSize);
typedef void (*PDctFunc) (int16_t* pDct, uint8_t* pSample1, int32_t iStride1, uint8_t* pSample2, int32_t iStride2);

typedef void (*PCopyFunc) (uint8_t* pDst, int32_t iStrideD, uint8_t* pSrc, int32_t iStrideS);
typedef void (*PIDctFunc) (uint8_t* pRec, int32_t iStride, uint8_t* pPred, int32_t iPredStride, int16_t* pRes);
typedef void (*PDeQuantizationFunc) (int16_t* pRes, const uint16_t* kpQpTable);
typedef void (*PDeQuantizationHadamardFunc) (int16_t* pRes, const uint16_t kuiMF);
typedef int32_t (*PGetNoneZeroCountFunc) (int16_t* pLevel);

typedef void (*PScanFunc) (int16_t* pLevel, int16_t* pDct);
typedef int32_t (*PCalculateSingleCtrFunc) (int16_t* pDct);

typedef void (*PTransformHadamard4x4Func) (int16_t* pLumaDc, int16_t* pDct);
typedef void (*PQuantizationFunc) (int16_t* pDct, const int16_t* pFF, const int16_t* pMF);
typedef void (*PQuantizationMaxFunc) (int16_t* pDct, const int16_t* pFF, const int16_t* pMF, int16_t* pMax);
typedef void (*PQuantizationDcFunc) (int16_t* pDct, int16_t iFF,  int16_t iMF);
typedef int32_t (*PQuantizationSkipFunc) (int16_t* pDct, int16_t iFF,  int16_t iMF);
typedef int32_t (*PQuantizationHadamardFunc) (int16_t* pRes, const int16_t kiFF, int16_t iMF, int16_t* pDct,
    int16_t* pBlock);

typedef void (*PWelsMcFunc) (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                             SMVUnitXY mv, int32_t iWidth, int32_t iHeight);

typedef void (*PWelsLumaHalfpelMcFunc) (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                        int32_t iWidth, int32_t iHeight);
typedef void (*PWelsLumaQuarpelMcFunc) (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                                        int32_t iHeight);
typedef void (*PWelsSampleAveragingFunc) (uint8_t*, int32_t, uint8_t*, int32_t, uint8_t*, int32_t, int32_t);

typedef struct TagMcFunc {
  PWelsLumaHalfpelMcFunc      pfLumaHalfpelHor;
  PWelsLumaHalfpelMcFunc      pfLumaHalfpelVer;
  PWelsLumaHalfpelMcFunc      pfLumaHalfpelCen;
  PWelsMcFunc                         pfChromaMc;

  PWelsLumaQuarpelMcFunc*     pfLumaQuarpelMc;
  PWelsSampleAveragingFunc*   pfSampleAveraging;
} SMcFunc;

typedef void (*PLumaDeblockingLT4Func) (uint8_t* iSampleY, int32_t iStride, int32_t iAlpha, int32_t iBeta, int8_t* iTc);
typedef void (*PLumaDeblockingEQ4Func) (uint8_t* iSampleY, int32_t iStride, int32_t iAlpha, int32_t iBeta);
typedef void (*PChromaDeblockingLT4Func) (uint8_t* iSampleCb, uint8_t* iSampleCr, int32_t iStride, int32_t iAlpha,
    int32_t iBeta, int8_t* iTc);
typedef void (*PChromaDeblockingEQ4Func) (uint8_t* iSampleCb, uint8_t* iSampleCr, int32_t iStride, int32_t iAlpha,
    int32_t iBeta);

typedef struct tagDeblockingFunc {
  PLumaDeblockingLT4Func    pfLumaDeblockingLT4Ver;
  PLumaDeblockingEQ4Func    pfLumaDeblockingEQ4Ver;
  PLumaDeblockingLT4Func    pfLumaDeblockingLT4Hor;
  PLumaDeblockingEQ4Func    pfLumaDeblockingEQ4Hor;

  PChromaDeblockingLT4Func  pfChromaDeblockingLT4Ver;
  PChromaDeblockingEQ4Func  pfChromaDeblockingEQ4Ver;
  PChromaDeblockingLT4Func  pfChromaDeblockingLT4Hor;
  PChromaDeblockingEQ4Func  pfChromaDeblockinEQ4Hor;
} DeblockingFunc;

typedef  void (*PSetNoneZeroCountZeroFunc) (int8_t* pNonZeroCount);

typedef int32_t (*PIntraFineMdFunc) (void* pEncCtx, void* pWelsMd, SMB* pCurMb, SMbCache* pMbCache);
typedef void (*PInterFineMdFunc) (void* pEncCtx, void* pWelsMd, SSlice* slice, SMB* pCurMb, int32_t bestCost);
typedef BOOL_T (*PInterMdFirstIntraModeFunc) (void* pEncCtx, void* pWelsMd, SMB* pCurMb, SMbCache* pMbCache);

typedef void (*PMotionSearchFunc) (SWelsFuncPtrList* pFuncList, void* pCurDqLayer, void* pMe,
                                   void* pSlice);  // here after reset all function pointers, will set as right parameter type
typedef void (*PFillInterNeighborCacheFunc) (SMbCache* pMbCache, SMB* pCurMb, int32_t iMbWidth, int8_t* pVaaBgMbFlag);
typedef void (*PAccumulateSadFunc) (uint32_t* pSumDiff, int32_t* pGomForegroundBlockNum, int32_t* iSad8x8,
                                    int8_t* pVaaBgMbFlag);//for RC
typedef BOOL_T (*PDynamicSlicingStepBackFunc) (void* pEncCtx, void* pSlice, SSliceCtx* pSliceCtx, SMB* pCurMb,
    SDynamicSlicingStack* pDynamicSlicingStack); // 2010.8.17

typedef bool_t (*PInterMdBackgroundDecisionFunc) (void* pEncCtx, void* pWelsMd, SSlice* slice, SMB* pCurMb,
    SMbCache* pMbCache, BOOL_T* pKeepPskip);
typedef void (*PInterMdBackgroundInfoUpdateFunc) (SDqLayer* pCurLayer,  SMB* pCurMb, const bool_t bFlag,
    const int32_t kiRefPictureType);

typedef void (*PInterMdFunc) (void* pEncCtx, void* pWelsMd, SSlice* slice, SMB* pCurMb, SMbCache* pMbCache);

typedef int32_t (*PSampleSadSatdCostFunc) (uint8_t*, int32_t, uint8_t*, int32_t);
typedef void (*PSample4SadCostFunc) (uint8_t*, int32_t, uint8_t*, int32_t, int32_t*);
typedef int32_t (*PIntraPred4x4Combined3Func) (uint8_t*, int32_t, uint8_t*, int32_t, uint8_t*, int32_t*, int32_t,
    int32_t, int32_t);
typedef int32_t (*PIntraPred16x16Combined3Func) (uint8_t*, int32_t, uint8_t*, int32_t, int32_t*, int32_t, uint8_t*);
typedef int32_t (*PIntraPred8x8Combined3Func) (uint8_t*, int32_t, uint8_t*, int32_t, int32_t*, int32_t, uint8_t*,
    uint8_t*, uint8_t*);
#define     MAX_BLOCK_TYPE 5 // prev 7
typedef struct TagSampleDealingFunc {
  PSampleSadSatdCostFunc            pfSampleSad[MAX_BLOCK_TYPE];
  PSampleSadSatdCostFunc            pfSampleSatd[MAX_BLOCK_TYPE];
  PSample4SadCostFunc                 pfSample4Sad[MAX_BLOCK_TYPE];
  PIntraPred4x4Combined3Func      pfIntra4x4Combined3Satd;
  PIntraPred16x16Combined3Func  pfIntra16x16Combined3Satd;
  PIntraPred16x16Combined3Func  pfIntra16x16Combined3Sad;
  PIntraPred8x8Combined3Func      pfIntra8x8Combined3Satd;
  PIntraPred8x8Combined3Func      pfIntra8x8Combined3Sad;

  PSampleSadSatdCostFunc*            pfMdCost;
  PSampleSadSatdCostFunc*            pfMeCost;
  PIntraPred16x16Combined3Func   pfIntra16x16Combined3;
  PIntraPred8x8Combined3Func       pfIntra8x8Combined3;
  PIntraPred4x4Combined3Func       pfIntra4x4Combined3;
} SSampleDealingFunc;
typedef void (*PGetIntraPredFunc) (uint8_t* pPrediction, uint8_t* pRef, const int32_t kiStride);

typedef int32_t (*PGetVarianceFromIntraVaaFunc) (uint8_t* pSampelY, const int32_t kiStride);
typedef uint8_t (*PGetMbSignFromInterVaaFunc) (int32_t* pSad8x8);
typedef void (*PUpdateMbMvFunc) (SMVUnitXY* pMvUnit, const SMVUnitXY ksMv);

struct TagWelsFuncPointerList {
  PExpandPictureFunc			pfExpandLumaPicture;
  PExpandPictureFunc
  pfExpandChromaPicture[2];// 0: for chroma unalignment && width_uv >= 16; 1: for chroma alignment && width_uv >= 16;

  PFillInterNeighborCacheFunc       pfFillInterNeighborCache;

  PGetVarianceFromIntraVaaFunc	pfGetVarianceFromIntraVaa;
  PGetMbSignFromInterVaaFunc	pfGetMbSignFromInterVaa;
  PUpdateMbMvFunc					    pfUpdateMbMv;
  PInterMdFirstIntraModeFunc      pfFirstIntraMode; //svc_encode_slice.c svc_mode_decision.c svc_base_layer_md.c
  PIntraFineMdFunc
  pfIntraFineMd;          //svc_encode_slice.c svc_mode_decision.c svc_base_layer_md.c
  PInterFineMdFunc                     pfInterFineMd;          //svc_encode_slice.c svc_base_layer_md.c
  PInterMdFunc                           pfInterMd;

  PInterMdBackgroundDecisionFunc          pfInterMdBackgroundDecision;
  PInterMdBackgroundInfoUpdateFunc      pfInterMdBackgroundInfoUpdate;

  SMcFunc				        sMcFuncs;
  SSampleDealingFunc     sSampleDealingFuncs;
  PGetIntraPredFunc 		pfGetLumaI16x16Pred[I16_PRED_DC_A];
  PGetIntraPredFunc 		pfGetLumaI4x4Pred[I4_PRED_A];
  PGetIntraPredFunc 		pfGetChromaPred[C_PRED_A];
  PMotionSearchFunc
  pfMotionSearch; //svc_encode_slice.c svc_mode_decision.c svc_enhance_layer_md.c svc_base_layer_md.c

  PCopyFunc      pfCopy16x16Aligned;		//svc_encode_slice.c svc_mode_decision.c svc_base_layer_md.c
  PCopyFunc      pfCopy16x16NotAligned;	//md.c
  PCopyFunc      pfCopy8x8Aligned;		//svc_encode_slice.c svc_mode_decision.c svc_base_layer_md.c md.c
  PCopyFunc	  pfCopy16x8NotAligned;	//for MeRefineFracPixel 16x8 based
  PCopyFunc	  pfCopy8x16Aligned;		//for MeRefineFracPixel 8x16 based

  //svc_encode_mb.c encode_mb_aux.c
  PDctFunc					pfDctT4;
  PDctFunc    		        pfDctFourT4;

  PCalculateSingleCtrFunc				pfCalculateSingleCtr4x4;
  PScanFunc				pfScan4x4;		//DC/AC
  PScanFunc				pfScan4x4Ac;

  PQuantizationFunc				        pfQuantization4x4;
  PQuantizationFunc				        pfQuantizationFour4x4;
  PQuantizationDcFunc			        pfQuantizationDc4x4;
  PQuantizationMaxFunc		        pfQuantizationFour4x4Max;
  PQuantizationHadamardFunc		pfQuantizationHadamard2x2;
  PQuantizationSkipFunc		        pfQuantizationHadamard2x2Skip;

  PTransformHadamard4x4Func	 pfTransformHadamard4x4Dc;

  PGetNoneZeroCountFunc		      pfGetNoneZeroCount;

  PDeQuantizationFunc				      pfDequantization4x4;
  PDeQuantizationFunc			          pfDequantizationFour4x4;
  PDeQuantizationHadamardFunc	  pfDequantizationIHadamard4x4;
  PIDctFunc				                      pfIDctFourT4;
  PIDctFunc				                      pfIDctT4;
  PIDctFunc				                      pfIDctI16x16Dc;



  // OPTI: if MT under diff uiSliceMode, need change here
  //PDynamicSlicingStepBackFunc	dynslc_funcpointer_stepback;//svc_encode_slice.c
  //DYNSLC_LNGTH_CRTL		dynslc_funcpointer_slcsize_ctrl;

  /* For Deblocking */
  DeblockingFunc                         pfDeblocking;
  PSetNoneZeroCountZeroFunc     pfSetNZCZero;

  SWelsRcFunc					    pfRc;
  PAccumulateSadFunc         pfAccumulateSadForRc;

  PSetMemoryZero				pfSetMemZeroSize8;			// for size is times to 8
  PSetMemoryZero				pfSetMemZeroSize64Aligned16;			// for size is times of 64, and address is align to 16
  PSetMemoryZero				pfSetMemZeroSize64;			// for size is times of 64, and don't know address is align to 16 or not
};

}	//end of namespace WelsSVCEnc {

#endif//WELS_ENCODER_FUNCTION_POINTERS_DEFINITION_H_
