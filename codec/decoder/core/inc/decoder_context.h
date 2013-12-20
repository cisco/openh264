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
 * \file	decoder_context.h
 *
 * \brief	mainly interface introduced in Wels decoder side
 *
 * \date	3/4/2009 Created
 *
 *************************************************************************************
 */
#ifndef WELS_DECODER_FRAMEWORK_H__
#define WELS_DECODER_FRAMEWORK_H__
#include "typedefs.h"
#include "utils.h"
#include "wels_const.h"
#include "wels_common_basis.h"
#include "codec_app_def.h"
#include "parameter_sets.h"
#include "nalu.h"
#include "dec_frame.h"
#include "pic_queue.h"
#include "vlc_decoder.h"
#include "fmo.h"
#include "as264_common.h" // for LONG_TERM_REF macro,can be delete if not need this macro
#include "crt_util_safe_x.h"
#include "mb_cache.h"

namespace WelsDec {

#ifndef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
//#define MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
#endif //MOSAIC_AVOID_BASED_ON_SPS_PPS_ID

typedef struct TagDataBuffer {
  uint8_t* pHead;
  uint8_t* pEnd;

  uint8_t* pStartPos;
  uint8_t* pCurPos;
} SDataBuffer;

//#ifdef __cplusplus
//extern "C" {
//#endif//__cplusplus

//#pragma pack(1)

/*
 *	Need move below structures to function pointer to seperate module/file later
 */

//typedef int32_t (*rec_mb) (Mb *cur_mb, PWelsDecoderContext pCtx);

/*typedef for get intra predictor func pointer*/
typedef void_t (*PGetIntraPredFunc) (uint8_t* pPred, const int32_t kiLumaStride);
typedef void_t (*PIdctResAddPredFunc) (uint8_t* pPred, const int32_t kiStride, int16_t* pRs);
typedef void_t (*PExpandPictureFunc) (uint8_t* pDst, const int32_t kiStride, const int32_t kiPicWidth,
                                      const int32_t kiPicHeight);

/**/
typedef struct TagRefPic {
  PPicture			pRefList[LIST_A][MAX_REF_PIC_COUNT];	// reference picture marking plus FIFO scheme
  PPicture			pShortRefList[LIST_A][MAX_SHORT_REF_COUNT];
  PPicture			pLongRefList[LIST_A][MAX_LONG_REF_COUNT];
  uint8_t				uiRefCount[LIST_A];
  uint8_t				uiShortRefCount[LIST_A];
  uint8_t				uiLongRefCount[LIST_A];	// dependend on ref pic module
  int32_t				iMaxLongTermFrameIdx;
} SRefPic, *PRefPic;

typedef void_t (*PWelsMcFunc) (uint8_t* pSrc, int32_t iSrcStride, uint8_t* pDst, int32_t iDstStride,
                               int16_t iMvX, int16_t iMvY, int32_t iWidth, int32_t iHeight);
typedef struct TagMcFunc {
  PWelsMcFunc pMcLumaFunc;
  PWelsMcFunc pMcChromaFunc;
} SMcFunc;

//deblock module defination
struct TagDeblockingFunc;

typedef struct tagDeblockingFilter {
  uint8_t*	pCsData[3];	// pointer to reconstructed picture data
  int32_t	iCsStride[2];	// Cs stride
  ESliceType  eSliceType;
  int8_t	iSliceAlphaC0Offset;
  int8_t	iSliceBetaOffset;
  int8_t  iChromaQP;
  int8_t  iLumaQP;
  struct TagDeblockingFunc*  pLoopf;
} SDeblockingFilter, *PDeblockingFilter;

typedef void_t (*PDeblockingFilterMbFunc) (PDqLayer pCurDqLayer, PDeblockingFilter  filter, int32_t boundry_flag);
typedef void_t (*PLumaDeblockingLT4Func) (uint8_t* iSampleY, int32_t iStride, int32_t iAlpha, int32_t iBeta,
    int8_t* iTc);
typedef void_t (*PLumaDeblockingEQ4Func) (uint8_t* iSampleY, int32_t iStride, int32_t iAlpha, int32_t iBeta);
typedef void_t (*PChromaDeblockingLT4Func) (uint8_t* iSampleCb, uint8_t* iSampleCr, int32_t iStride, int32_t iAlpha,
    int32_t iBeta, int8_t* iTc);
typedef void_t (*PChromaDeblockingEQ4Func) (uint8_t* iSampleCb, uint8_t* iSampleCr, int32_t iStride, int32_t iAlpha,
    int32_t iBeta);

typedef struct TagDeblockingFunc {
  PLumaDeblockingLT4Func    pfLumaDeblockingLT4Ver;
  PLumaDeblockingEQ4Func    pfLumaDeblockingEQ4Ver;
  PLumaDeblockingLT4Func    pfLumaDeblockingLT4Hor;
  PLumaDeblockingEQ4Func    pfLumaDeblockingEQ4Hor;

  PChromaDeblockingLT4Func  pfChromaDeblockingLT4Ver;
  PChromaDeblockingEQ4Func  pfChromaDeblockingEQ4Ver;
  PChromaDeblockingLT4Func  pfChromaDeblockingLT4Hor;
  PChromaDeblockingEQ4Func  pfChromaDeblockinEQ4Hor;
} SDeblockingFunc, *PDeblockingFunc;

typedef void_t (*PWelsBlockAddStrideFunc) (uint8_t* pDest, uint8_t* pPred, int16_t* pRes, int32_t iPredStride,
    int32_t iResStride);
typedef void_t (*PWelsBlockZeroFunc) (int16_t* pBlock, int32_t iStride);
typedef void_t (*PWelsNonZeroCountFunc) (int16_t* pBlock, int8_t* pNonZeroCount);
typedef void_t (*PWelsSimpleIdct4x4AddFunc) (int16_t* pDest, int16_t* pSrc, int32_t iStride);

typedef  struct  TagBlockFunc {
  PWelsBlockZeroFunc			pWelsBlockZero16x16Func;
  PWelsBlockZeroFunc			pWelsBlockZero8x8Func;
  PWelsNonZeroCountFunc		pWelsSetNonZeroCountFunc;
} SBlockFunc;

typedef void_t (*PWelsFillNeighborMbInfoIntra4x4Func) (PNeighAvail pNeighAvail, uint8_t* pNonZeroCount,
    int8_t* pIntraPredMode, PDqLayer pCurLayer);
typedef int32_t (*PWelsParseIntra4x4ModeFunc) (PNeighAvail pNeighAvail, int8_t* pIntraPredMode, PBitStringAux pBs,
    PDqLayer pCurDqLayer);
typedef int32_t (*PWelsParseIntra16x16ModeFunc) (PNeighAvail pNeighAvail, PBitStringAux pBs, PDqLayer pCurDqLayer);

typedef struct TagExpandPicFunc {
  PExpandPictureFunc pExpandLumaPicture;
  PExpandPictureFunc pExpandChromaPicture[2];
} SExpandPicFunc;

/*
 *	SWelsDecoderContext: to maintail all modules data over decoder@framework
 */

typedef struct TagWelsDecoderContext {
  // Input
  void_t*				pArgDec;			// structured arguments for decoder, reserved here for extension in the future

  SDataBuffer       	sRawData;

  // Configuration
  SDecodingParam*    	pParam;
  uint32_t			uiCpuFlag;			// CPU compatibility detected
  int32_t 	   		iDecoderMode;		// indicate decoder running mode
  int32_t				iSetMode;			// indicate decoder mode set from upper layer, this is read-only for decoder internal
  int32_t 			iDecoderOutputProperty; // indicate the output buffer property
  int32_t				iModeSwitchType;	// 1: optimal decision; 2: forced switch to the other mode; 0: no switch

  int32_t				iOutputColorFormat;		// color space format to be outputed
  VIDEO_BITSTREAM_TYPE eVideoType; //indicate the type of video to decide whether or not to do qp_delta error detection.
  bool_t				bErrorResilienceFlag;		// error resilience flag
  bool_t				bHaveGotMemory;	// global memory for decoder context related ever requested?

  int32_t				iImgWidthInPixel;	// width of image in pixel reconstruction picture to be output
  int32_t				iImgHeightInPixel;// height of image in pixel reconstruction picture to be output
  int32_t				iMaxWidthInSps;	// maximal width of pixel in SPS sets
  int32_t				iMaxHeightInSps;	// maximal height of pixel in SPS sets

  // Derived common elements
  SNalUnitHeader		sCurNalHead;
  ESliceType			eSliceType;			// Slice type
  int32_t				iFrameNum;
  int32_t				iPrevFrameNum;		// frame number of previous frame well decoded for non-truncated mode yet
  bool_t              bLastHasMmco5;      //
  int32_t				iErrorCode;			// error code return while decoding in case packets lost
  SFmo				sFmoList[MAX_PPS_COUNT];	// list for FMO storage
  PFmo				pFmo;				// current fmo context after parsed slice_header
  int32_t				iActiveFmoNum;		// active count number of fmo context in list

  /*needed info by decode slice level and mb level*/
  int32_t
  iDecBlockOffsetArray[24];	// address talbe for sub 4x4 block in intra4x4_mb, so no need to caculta the address every time.

  struct {
    int8_t*  pMbType[LAYER_NUM_EXCHANGEABLE];                      /* mb type */
    int16_t	(*pMv[LAYER_NUM_EXCHANGEABLE][LIST_A])[MB_BLOCK4x4_NUM][MV_A]; //[LAYER_NUM_EXCHANGEABLE   MB_BLOCK4x4_NUM*]
    int8_t	(*pRefIndex[LAYER_NUM_EXCHANGEABLE][LIST_A])[MB_BLOCK4x4_NUM];
    int8_t*	pLumaQp[LAYER_NUM_EXCHANGEABLE];	/*mb luma_qp*/
    int8_t*	pChromaQp[LAYER_NUM_EXCHANGEABLE];					/*mb chroma_qp*/
    int8_t	(*pNzc[LAYER_NUM_EXCHANGEABLE])[24];
    int8_t	(*pNzcRs[LAYER_NUM_EXCHANGEABLE])[24];
    int16_t (*pScaledTCoeff[LAYER_NUM_EXCHANGEABLE])[MB_COEFF_LIST_SIZE]; /*need be aligned*/
    int8_t	(*pIntraPredMode[LAYER_NUM_EXCHANGEABLE])[8]; //0~3 top4x4 ; 4~6 left 4x4; 7 intra16x16
    int8_t (*pIntra4x4FinalMode[LAYER_NUM_EXCHANGEABLE])[MB_BLOCK4x4_NUM];
    int8_t*  pChromaPredMode[LAYER_NUM_EXCHANGEABLE];
    int8_t*  pCbp[LAYER_NUM_EXCHANGEABLE];
    uint8_t (*pMotionPredFlag[LAYER_NUM_EXCHANGEABLE][LIST_A])[MB_PARTITION_SIZE]; // 8x8
    int8_t (*pSubMbType[LAYER_NUM_EXCHANGEABLE])[MB_SUB_PARTITION_SIZE];
    int32_t* pSliceIdc[LAYER_NUM_EXCHANGEABLE];		// using int32_t for slice_idc
    int8_t*  pResidualPredFlag[LAYER_NUM_EXCHANGEABLE];
    int8_t*  pInterPredictionDoneFlag[LAYER_NUM_EXCHANGEABLE];
    int16_t iMbWidth;
    int16_t iMbHeight;
  } sMb;


  // reconstruction picture
  PPicture			pDec;			//pointer to current picture being reconstructed

  // reference pictures
  SRefPic				sRefPic;

  SVlcTable			sVlcTable;		 // vlc table

  SBitStringAux		sBs;

  /* Global memory external */

  SPosOffset	sFrameCrop;

#ifdef MOSAIC_AVOID_BASED_ON_SPS_PPS_ID
  int32_t             iSpsTotalNum;  //the number of SPS in current IDR interval
  int32_t             iSubspsTotalNum; //the number of subsps in current IDR interval
  int32_t             iPpsTotalNum; //the number of PPS in current IDR interval.
#endif //MOSAIC_AVOID_BASED_ON_SPS_PPS_ID	

  SSps				sSpsBuffer[MAX_SPS_COUNT];
  SPps				sPpsBuffer[MAX_PPS_COUNT];
  PSliceHeader		pSliceHeader;

  PPicBuff	        pPicBuff[LIST_A];	// Initially allocated memory for pictures which are used in decoding.
  int32_t				iPicQueueNumber;

  SSubsetSps			sSubsetSpsBuffer[MAX_SPS_COUNT];
  SNalUnit            sPrefixNal;

  PAccessUnit			pAccessUnitList;	// current access unit list to be performed
  PSps				pSps;	// used by current AU
  PPps				pPps;	// used by current AU
  // Memory for pAccessUnitList is dynamically held till decoder destruction.
  PDqLayer			pCurDqLayer;		// current DQ layer representation, also carry reference base layer if applicable
  PDqLayer			pDqLayersList[LAYER_NUM_EXCHANGEABLE];	// DQ layers list with memory allocated
  uint8_t*				pCsListXchg[LAYER_NUM_EXCHANGEABLE][3];	// Constructed picture buffer: 0- cur layer, 1- ref layer;
  int16_t*				pRsListXchg[LAYER_NUM_EXCHANGEABLE][3];// Residual picture buffer: 0- cur layer, 1- ref layer;

  int32_t				iCsStride[3];		// strides for Cs
  int32_t				iRsStride[3];		// strides for Rs

  int32_t             iPicWidthReq;		// picture width have requested the memory
  int32_t             iPicHeightReq;		// picture height have requested the memory

  uint8_t				uiTargetDqId;		// maximal DQ ID in current access unit, meaning target layer ID
  bool_t				bAvcBasedFlag;		// For decoding bitstream:
  bool_t				bEndOfStreamFlag;	// Flag on end of stream requested by external application layer
  bool_t				bInitialDqLayersMem;	// dq layers related memory is available?

  bool_t              bOnlyOneLayerInCurAuFlag; //only one layer in current AU: 1

  // for EC parameter sets
  bool_t				bSpsExistAheadFlag;	// whether does SPS NAL exist ahead of sequence?
  bool_t				bSubspsExistAheadFlag;// whether does Subset SPS NAL exist ahead of sequence?
  bool_t				bPpsExistAheadFlag;	// whether does PPS NAL exist ahead of sequence?

  bool_t				bSpsAvailFlags[MAX_SPS_COUNT];
  bool_t				bSubspsAvailFlags[MAX_SPS_COUNT];
  bool_t				bPpsAvailFlags[MAX_PPS_COUNT];
  bool_t				bReferenceLostAtT0Flag;
#ifdef LONG_TERM_REF
  bool_t				bParamSetsLostFlag;	//sps or pps do not exist or not correct

  bool_t
  bCurAuContainLtrMarkSeFlag; //current AU has the LTR marking syntax element, mark the previous frame or self
  int32_t             iFrameNumOfAuMarkedLtr; //if bCurAuContainLtrMarkSeFlag==true, SHOULD set this variable

  uint16_t            uiCurIdrPicId;
#endif

  PGetIntraPredFunc 	pGetI16x16LumaPredFunc[7];		//h264_predict_copy_16x16;
  PGetIntraPredFunc 	pGetI4x4LumaPredFunc[14];		// h264_predict_4x4_t
  PGetIntraPredFunc 	pGetIChromaPredFunc[7];		// h264_predict_8x8_t
  PIdctResAddPredFunc	pIdctResAddPredFunc;
  SMcFunc				sMcFunc;
  /* For Deblocking */
  SDeblockingFunc     sDeblockingFunc;
  SExpandPicFunc	    sExpandPicFunc;

  /* For Block */
  SBlockFunc          sBlockFunc;
  /* For EC */
  int32_t iCurSeqIntervalTargetDependId;
  int32_t iCurSeqIntervalMaxPicWidth;
  int32_t iCurSeqIntervalMaxPicHeight;

  PWelsFillNeighborMbInfoIntra4x4Func  pFillInfoCacheIntra4x4Func;
  PWelsParseIntra4x4ModeFunc           pParseIntra4x4ModeFunc;
  PWelsParseIntra16x16ModeFunc         pParseIntra16x16ModeFunc;

  //feedback whether or not have VCL in current AU, and the temporal ID
  int32_t iFeedbackVclNalInAu;
  int32_t iFeedbackTidInAu;

  bool_t bAuReadyFlag;   // TRUE: one au is ready for decoding; FALSE: default value

  //trace handle
  void_t*      pTraceHandle;

#ifdef NO_WAITING_AU
  //Save the last nal header info
  SNalUnitHeaderExt sLastNalHdrExt;
  SSliceHeader      sLastSliceHeader;
#endif

} SWelsDecoderContext, *PWelsDecoderContext;

//#pragma pack()

//#ifdef __cplusplus
//}
//#endif//__cplusplus

} // namespace WelsDec

#endif//WELS_DECODER_FRAMEWORK_H__
