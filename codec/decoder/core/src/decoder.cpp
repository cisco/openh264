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
 * \file	decoder.c
 *
 * \brief	Interfaces implementation introduced in decoder system architecture
 *
 * \date	03/10/2009 Created
 *
 *************************************************************************************
 */
#include "codec_def.h"
#include "decoder.h"
#include "cpu.h"
#include "au_parser.h"
#include "get_intra_predictor.h"
#include "rec_mb.h"
#include "mc.h"
#include "decode_mb_aux.h"
#include "manage_dec_ref.h"
#include "decoder_core.h"
#include "deblocking.h"
#include "expand_pic.h"
#include "decode_slice.h"
#include "error_concealment.h"
#include "mem_align.h"

namespace WelsDec {

extern PPicture AllocPicture (PWelsDecoderContext pCtx, const int32_t kiPicWidth, const int32_t kiPicHeight);

extern void FreePicture (PPicture pPic);

static int32_t CreatePicBuff (PWelsDecoderContext pCtx, PPicBuff* ppPicBuf, const int32_t kiSize,
                              const int32_t kiPicWidth, const int32_t kiPicHeight) {
  PPicBuff pPicBuf = NULL;
  int32_t iPicIdx = 0;
  if (kiSize <= 0 || kiPicWidth <= 0 || kiPicHeight <= 0) {
    return 1;
  }

  pPicBuf	= (PPicBuff)WelsMalloc (sizeof (SPicBuff), "PPicBuff");

  if (NULL == pPicBuf) {
    return 1;
  }

  pPicBuf->ppPic = (PPicture*)WelsMalloc (kiSize * sizeof (PPicture), "PPicture*");

  if (NULL == pPicBuf->ppPic) {
    return 1;
  }
  for (iPicIdx = 0; iPicIdx < kiSize; ++ iPicIdx) {
    PPicture pPic = AllocPicture (pCtx, kiPicWidth, kiPicHeight);
    if (NULL == pPic) {
      return 1;
    }
    pPicBuf->ppPic[iPicIdx] = pPic;
  }

  // initialize context in queue
  pPicBuf->iCapacity	 = kiSize;
  pPicBuf->iCurrentIdx = 0;
  *ppPicBuf			 = pPicBuf;

  return 0;
}

static int32_t IncreasePicBuff (PWelsDecoderContext pCtx, PPicBuff* ppPicBuf, const int32_t kiOldSize,
                                const int32_t kiPicWidth, const int32_t kiPicHeight, const int32_t kiNewSize) {
  PPicBuff pPicOldBuf = *ppPicBuf;
  PPicBuff pPicNewBuf = NULL;
  int32_t iPicIdx = 0;
  if (kiOldSize <= 0 || kiNewSize <= 0 || kiPicWidth <= 0 || kiPicHeight <= 0) {
    return 1;
  }

  pPicNewBuf	= (PPicBuff)WelsMalloc (sizeof (SPicBuff), "PPicBuff");

  if (NULL == pPicNewBuf) {
    return 1;
  }

  pPicNewBuf->ppPic = (PPicture*)WelsMalloc (kiNewSize * sizeof (PPicture), "PPicture*");

  if (NULL == pPicNewBuf->ppPic) {
    return 1;
  }
  // copy old PicBuf to new PicBuf
  memcpy (pPicNewBuf->ppPic, pPicOldBuf->ppPic, kiOldSize * sizeof (PPicture));


  // increase new PicBuf
  for (iPicIdx = kiOldSize; iPicIdx < kiNewSize; ++ iPicIdx) {
    PPicture pPic = AllocPicture (pCtx, kiPicWidth, kiPicHeight);
    if (NULL == pPic) {
      return 1;
    }
    pPicNewBuf->ppPic[iPicIdx] = pPic;
  }
  // initialize context in queue
  pPicNewBuf->iCapacity	 = kiNewSize;
  pPicNewBuf->iCurrentIdx = pPicOldBuf->iCurrentIdx;
  *ppPicBuf			 = pPicNewBuf;
  
  for(int32_t i = 0; i < pPicNewBuf->iCapacity; i++) {
    pPicNewBuf->ppPic[i]->bUsedAsRef = false;
    pPicNewBuf->ppPic[i]->bIsLongRef = false;
    pPicNewBuf->ppPic[i]->uiRefCount = 0;
    pPicNewBuf->ppPic[i]->bAvailableFlag = true;
    pPicNewBuf->ppPic[i]->bIsComplete = false;
  }
  // remove old PicBuf
  if (pPicOldBuf->ppPic != NULL) {
    WelsFree (pPicOldBuf->ppPic, "pPicOldBuf->queue");
    pPicOldBuf->ppPic	= NULL;
  }
  pPicOldBuf->iCapacity	= 0;
  pPicOldBuf->iCurrentIdx = 0;
  WelsFree (pPicOldBuf, "pPicOldBuf");
  pPicOldBuf = NULL;
  return 0;
}

static int32_t DecreasePicBuff (PWelsDecoderContext pCtx, PPicBuff* ppPicBuf, const int32_t kiOldSize,
                                const int32_t kiPicWidth, const int32_t kiPicHeight, const int32_t kiNewSize) {
  PPicBuff pPicOldBuf = *ppPicBuf;
  PPicBuff pPicNewBuf = NULL;
  int32_t iPicIdx = 0;
  if (kiOldSize <= 0 || kiNewSize <= 0 || kiPicWidth <= 0 || kiPicHeight <= 0) {
    return 1;
  }

  pPicNewBuf	= (PPicBuff)WelsMalloc (sizeof (SPicBuff), "PPicBuff");

  if (NULL == pPicNewBuf) {
    return 1;
  }

  pPicNewBuf->ppPic = (PPicture*)WelsMalloc (kiNewSize * sizeof (PPicture), "PPicture*");

  if (NULL == pPicNewBuf->ppPic) {
    return 1;
  }

  int32_t iPrevPicIdx = -1;
  for (iPrevPicIdx = 0; iPrevPicIdx < kiOldSize; ++iPrevPicIdx) {
    if (pCtx->pPreviousDecodedPictureInDpb == pPicOldBuf->ppPic[iPrevPicIdx]) {
      break;
    }
  }
  int32_t iDelIdx;
  if (iPrevPicIdx < kiOldSize && iPrevPicIdx >= kiNewSize) {
    // found pPreviousDecodedPictureInDpb,
    pPicNewBuf->ppPic[0] = pPicOldBuf->ppPic[iPrevPicIdx];
    pPicNewBuf->iCurrentIdx = 0;
    memcpy (pPicNewBuf->ppPic + 1, pPicOldBuf->ppPic, (kiNewSize - 1) * sizeof (PPicture));
    iDelIdx = kiNewSize - 1;
  } else {
    memcpy (pPicNewBuf->ppPic, pPicOldBuf->ppPic, kiNewSize * sizeof (PPicture));
    pPicNewBuf->iCurrentIdx = iPrevPicIdx < kiNewSize ? iPrevPicIdx : 0;
    iDelIdx = kiNewSize;
  }

  for (iPicIdx = iDelIdx; iPicIdx < kiOldSize; iPicIdx++) {
    if (iPrevPicIdx != iPicIdx) {
      if (pPicOldBuf->ppPic[iPicIdx] != NULL) {
        FreePicture (pPicOldBuf->ppPic[iPicIdx]);
        pPicOldBuf->ppPic[iPicIdx] = NULL;
      }
    }
  }

  // initialize context in queue
  pPicNewBuf->iCapacity	 = kiNewSize;
  *ppPicBuf			 = pPicNewBuf;

  for(int32_t i = 0; i < pPicNewBuf->iCapacity; i++) {
    pPicNewBuf->ppPic[i]->bUsedAsRef = false;
    pPicNewBuf->ppPic[i]->bIsLongRef = false;
    pPicNewBuf->ppPic[i]->uiRefCount = 0;
    pPicNewBuf->ppPic[i]->bAvailableFlag = true;
    pPicNewBuf->ppPic[i]->bIsComplete = false;
  }
  // remove old PicBuf
  if (pPicOldBuf->ppPic != NULL) {
    WelsFree (pPicOldBuf->ppPic, "pPicOldBuf->queue");
    pPicOldBuf->ppPic	= NULL;
  }
  pPicOldBuf->iCapacity	= 0;
  pPicOldBuf->iCurrentIdx = 0;
  WelsFree (pPicOldBuf, "pPicOldBuf");
  pPicOldBuf = NULL;

  return 0;
}

static void DestroyPicBuff (PPicBuff* ppPicBuf) {
  PPicBuff pPicBuf = NULL;

  if (NULL == ppPicBuf || NULL == *ppPicBuf)
    return;

  pPicBuf = *ppPicBuf;
  while (pPicBuf->ppPic != NULL) {
    int32_t iPicIdx = 0;
    while (iPicIdx < pPicBuf->iCapacity) {
      PPicture pPic = pPicBuf->ppPic[iPicIdx];
      if (pPic != NULL) {
        FreePicture (pPic);
      }
      pPic = NULL;
      ++ iPicIdx;
    }

    WelsFree (pPicBuf->ppPic, "pPicBuf->queue");

    pPicBuf->ppPic	= NULL;
  }
  pPicBuf->iCapacity	= 0;
  pPicBuf->iCurrentIdx = 0;

  WelsFree (pPicBuf, "pPicBuf");

  pPicBuf = NULL;
  *ppPicBuf = NULL;
}
/*
 * fill data fields in default for decoder context
 */
void WelsDecoderDefaults (PWelsDecoderContext pCtx, SLogContext* pLogCtx) {
  int32_t iCpuCores               = 1;
  memset (pCtx, 0, sizeof (SWelsDecoderContext));	// fill zero first
  pCtx->sLogCtx = *pLogCtx;

  pCtx->pArgDec                   = NULL;

  pCtx->eOutputColorFormat		= videoFormatI420;	// yuv in default
  pCtx->bHaveGotMemory			= false;	// not ever request memory blocks for decoder context related
  pCtx->uiCpuFlag					= 0;

  pCtx->bAuReadyFlag				= 0; // au data is not ready
  pCtx->bCabacInited = false;

  pCtx->uiCpuFlag = WelsCPUFeatureDetect (&iCpuCores);

  pCtx->iImgWidthInPixel		= 0;
  pCtx->iImgHeightInPixel		= 0;		// alloc picture data when picture size is available
  pCtx->iLastImgWidthInPixel		= 0;
  pCtx->iLastImgHeightInPixel		= 0;
  pCtx->bFreezeOutput = true;

  pCtx->iFrameNum				= -1;
  pCtx->iPrevFrameNum			= -1;
  pCtx->iErrorCode			= ERR_NONE;

  pCtx->pDec					= NULL;

  WelsResetRefPic (pCtx);

  pCtx->iActiveFmoNum			= 0;

  pCtx->pPicBuff[LIST_0]		= NULL;
  pCtx->pPicBuff[LIST_1]		= NULL;

  pCtx->bAvcBasedFlag			= true;
  pCtx->eErrorConMethod = ERROR_CON_SLICE_COPY_CROSS_IDR_FREEZE_RES_CHANGE;
  pCtx->pPreviousDecodedPictureInDpb = NULL;
  pCtx->sDecoderStatistics.iAvgLumaQp = -1;

}

/*
 *	destory_mb_blocks
 */


/*
 *	get size of reference picture list in target layer incoming, = (iNumRefFrames
 */
static inline int32_t GetTargetRefListSize (PWelsDecoderContext pCtx) {
  int32_t iNumRefFrames	= 0;
  // +2 for EC MV Copy buffer exchange
  if ((pCtx == NULL) || (pCtx->pSps == NULL)) {
    iNumRefFrames = MAX_REF_PIC_COUNT + 2;
  } else {
    iNumRefFrames = pCtx->pSps->iNumRefFrames + 2;
  }

#ifdef LONG_TERM_REF
  //pic_queue size minimum set 2
  if (iNumRefFrames < 2) {
    iNumRefFrames = 2;
  }
#endif

  return iNumRefFrames;
}

/*
 *	request memory blocks for decoder avc part
 */
int32_t WelsRequestMem (PWelsDecoderContext pCtx, const int32_t kiMbWidth, const int32_t kiMbHeight) {
  const int32_t kiPicWidth	= kiMbWidth << 4;
  const int32_t kiPicHeight	= kiMbHeight << 4;
  int32_t iErr = ERR_NONE;

  int32_t iListIdx			= 0;	//, mb_blocks	= 0;
  int32_t	iPicQueueSize		= 0;	// adaptive size of picture queue, = (pSps->iNumRefFrames x 2)
  bool  bNeedChangePicQueue	= true;

  WELS_VERIFY_RETURN_IF (ERR_INFO_INVALID_PARAM, (NULL == pCtx || kiPicWidth <= 0 || kiPicHeight <= 0))

  // Fixed the issue about different gop size over last, 5/17/2010
  // get picture queue size currently
  iPicQueueSize	= GetTargetRefListSize (pCtx);	// adaptive size of picture queue, = (pSps->iNumRefFrames x 2)
  pCtx->iPicQueueNumber = iPicQueueSize;
  if (pCtx->pPicBuff[LIST_0] != NULL
      && pCtx->pPicBuff[LIST_0]->iCapacity ==
      iPicQueueSize)	// comparing current picture queue size requested and previous allocation picture queue
    bNeedChangePicQueue	= false;
  // HD based pic buffer need consider memory size consumed when switch from 720p to other lower size
  WELS_VERIFY_RETURN_IF (ERR_NONE, pCtx->bHaveGotMemory && (kiPicWidth == pCtx->iImgWidthInPixel
                         && kiPicHeight == pCtx->iImgHeightInPixel) && (!bNeedChangePicQueue))	// have same scaled buffer

  // sync update pRefList
  WelsResetRefPic (pCtx);	// added to sync update ref list due to pictures are free

  if (pCtx->bHaveGotMemory && (kiPicWidth == pCtx->iImgWidthInPixel && kiPicHeight == pCtx->iImgHeightInPixel)
      && pCtx->pPicBuff[LIST_0] != NULL && pCtx->pPicBuff[LIST_0]->iCapacity != iPicQueueSize) {
    // currently only active for LIST_0 due to have no B frames
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO,
             "WelsRequestMem(): memory re-alloc for no resolution change (size = %d * %d), ref list size change from %d to %d",
             kiPicWidth, kiPicHeight, pCtx->pPicBuff[LIST_0]->iCapacity, iPicQueueSize);
    if (pCtx->pPicBuff[LIST_0]->iCapacity < iPicQueueSize) {
      iErr = IncreasePicBuff (pCtx, &pCtx->pPicBuff[LIST_0], pCtx->pPicBuff[LIST_0]->iCapacity, kiPicWidth, kiPicHeight,
                              iPicQueueSize);
    } else {
      iErr = DecreasePicBuff (pCtx, &pCtx->pPicBuff[LIST_0], pCtx->pPicBuff[LIST_0]->iCapacity, kiPicWidth, kiPicHeight,
                              iPicQueueSize);
    }
  } else {
    if (pCtx->bHaveGotMemory)
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO,
               "WelsRequestMem(): memory re-alloc for resolution change, size change from %d * %d to %d * %d, ref list size change from %d to %d",
               pCtx->iImgWidthInPixel, pCtx->iImgHeightInPixel, kiPicWidth, kiPicHeight, pCtx->pPicBuff[LIST_0]->iCapacity,
               iPicQueueSize);
    else
      WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO, "WelsRequestMem(): memory alloc size = %d * %d, ref list size = %d",
               kiPicWidth, kiPicHeight, iPicQueueSize);
    // for Recycled_Pic_Queue
    for (iListIdx = LIST_0; iListIdx < LIST_A; ++ iListIdx) {
      PPicBuff* ppPic = &pCtx->pPicBuff[iListIdx];
      if (NULL != ppPic && NULL != *ppPic) {
        DestroyPicBuff (ppPic);
      }
    }

    pCtx->pPreviousDecodedPictureInDpb = NULL;

    // currently only active for LIST_0 due to have no B frames
    iErr = CreatePicBuff (pCtx, &pCtx->pPicBuff[LIST_0], iPicQueueSize, kiPicWidth, kiPicHeight);
  }

  if (iErr != ERR_NONE)
    return iErr;


  pCtx->iImgWidthInPixel	= kiPicWidth;	// target width of image to be reconstruted while decoding
  pCtx->iImgHeightInPixel	= kiPicHeight;	// target height of image to be reconstruted while decoding

  pCtx->bHaveGotMemory	= true;			// global memory for decoder context related is requested
  pCtx->pDec		        = NULL;			// need prefetch a new pic due to spatial size changed

  if (pCtx->pCabacDecEngine == NULL)
    pCtx->pCabacDecEngine = (SWelsCabacDecEngine*) WelsMalloc (sizeof (SWelsCabacDecEngine), "pCtx->pCabacDecEngine");

  return ERR_NONE;
}

/*
 *	free memory blocks in avc
 */
void WelsFreeMem (PWelsDecoderContext pCtx) {
  int32_t iListIdx = 0;

  /* TODO: free memory blocks introduced in avc */
  ResetFmoList (pCtx);

  WelsResetRefPic (pCtx);

  // for sPicBuff
  for (iListIdx = LIST_0; iListIdx < LIST_A; ++ iListIdx) {
    PPicBuff* pPicBuff = &pCtx->pPicBuff[iListIdx];
    if (NULL != pPicBuff && NULL != *pPicBuff) {
      DestroyPicBuff (pPicBuff);
    }
  }

  // added for safe memory
  pCtx->iImgWidthInPixel	= 0;
  pCtx->iImgHeightInPixel = 0;
  pCtx->iLastImgWidthInPixel	= 0;
  pCtx->iLastImgHeightInPixel = 0;
  pCtx->bFreezeOutput = true;
  pCtx->bHaveGotMemory	= false;
  WelsFree (pCtx->pCabacDecEngine, "pCtx->pCabacDecEngine");
}

/*!
 * \brief	Open decoder
 */
int32_t WelsOpenDecoder (PWelsDecoderContext pCtx) {
  // function pointers
  //initial MC function pointer--
  int iRet = ERR_NONE;
  InitMcFunc (& (pCtx->sMcFunc), pCtx->uiCpuFlag);
  InitErrorCon (pCtx);

  InitExpandPictureFunc (& (pCtx->sExpandPicFunc), pCtx->uiCpuFlag);
  AssignFuncPointerForRec (pCtx);

  // vlc tables
  InitVlcTable (&pCtx->sVlcTable);

  // startup memory
  iRet = WelsInitMemory (pCtx);
  if (ERR_NONE != iRet)
    return iRet;

#ifdef LONG_TERM_REF
  pCtx->bParamSetsLostFlag = true;
#else
  pCtx->bReferenceLostAtT0Flag	= true;	// should be true to waiting IDR at incoming AU bits following, 6/4/2010
#endif //LONG_TERM_REF
  pCtx->bNewSeqBegin = true;
  pCtx->bPrintFrameErrorTraceFlag = true;
  pCtx->iIgnoredErrorInfoPacketCount = 0;
  return iRet;
}

/*!
 * \brief	Close decoder
 */
void WelsCloseDecoder (PWelsDecoderContext pCtx) {
  WelsFreeMem (pCtx);

  WelsFreeMemory (pCtx);

  UninitialDqLayersContext (pCtx);

#ifdef LONG_TERM_REF
  pCtx->bParamSetsLostFlag       = false;
#else
  pCtx->bReferenceLostAtT0Flag = false;
#endif
  pCtx->bNewSeqBegin = false;
  pCtx->bPrintFrameErrorTraceFlag = false;
}

/*!
 * \brief	configure decoder parameters
 */
int32_t DecoderConfigParam (PWelsDecoderContext pCtx, const SDecodingParam* kpParam) {
  if (NULL == pCtx || NULL == kpParam)
    return 1;

  pCtx->pParam	= (SDecodingParam*)WelsMalloc (sizeof (SDecodingParam), "SDecodingParam");

  if (NULL == pCtx->pParam)
    return 1;

  memcpy (pCtx->pParam, kpParam, sizeof (SDecodingParam));
  pCtx->eOutputColorFormat	= pCtx->pParam->eOutputColorFormat;
  int32_t iRet = DecoderSetCsp (pCtx, pCtx->pParam->eOutputColorFormat);
  if (iRet)
    return iRet;
  pCtx->eErrorConMethod = pCtx->pParam->eEcActiveIdc;

  if (pCtx->bParseOnly) //parse only, disable EC method
    pCtx->eErrorConMethod = ERROR_CON_DISABLE;

  if (VIDEO_BITSTREAM_SVC == pCtx->pParam->sVideoProperty.eVideoBsType ||
      VIDEO_BITSTREAM_AVC == pCtx->pParam->sVideoProperty.eVideoBsType) {
    pCtx->eVideoType = pCtx->pParam->sVideoProperty.eVideoBsType;
  } else {
    pCtx->eVideoType = VIDEO_BITSTREAM_DEFAULT;
  }

  WelsLog (& (pCtx->sLogCtx), WELS_LOG_INFO, "eVideoType: %d", pCtx->eVideoType);

  return 0;
}

/*!
 *************************************************************************************
 * \brief	Initialize Wels decoder parameters and memory
 *
 * \param 	pCtx input context to be initialized at first stage
 *
 * \return	0 - successed
 * \return	1 - failed
 *
 * \note	N/A
 *************************************************************************************
 */
int32_t WelsInitDecoder (PWelsDecoderContext pCtx, const bool bParseOnly, SLogContext* pLogCtx) {
  if (pCtx == NULL) {
    return ERR_INFO_INVALID_PTR;
  }

  // default
  WelsDecoderDefaults (pCtx, pLogCtx);

  pCtx->bParseOnly = bParseOnly;
  // open decoder
  return WelsOpenDecoder (pCtx);
}

/*!
 *************************************************************************************
 * \brief	Uninitialize Wels decoder parameters and memory
 *
 * \param 	pCtx input context to be uninitialized at release stage
 *
 * \return	NONE
 *
 * \note	N/A
 *************************************************************************************
 */
void WelsEndDecoder (PWelsDecoderContext pCtx) {
  // close decoder
  WelsCloseDecoder (pCtx);
}

void GetVclNalTemporalId (PWelsDecoderContext pCtx) {
  PAccessUnit pAccessUnit = pCtx->pAccessUnitList;
  int32_t idx = pAccessUnit->uiStartPos;

  pCtx->iFeedbackVclNalInAu = FEEDBACK_VCL_NAL;
  pCtx->iFeedbackTidInAu    = pAccessUnit->pNalUnitsList[idx]->sNalHeaderExt.uiTemporalId;
}

/*!
 *************************************************************************************
 * \brief	First entrance to decoding core interface.
 *
 * \param 	pCtx	        decoder context
 * \param	pBufBs	        bit streaming buffer
 * \param	kBsLen	        size in bytes length of bit streaming buffer input
 * \param	ppDst	        picture payload data to be output
 * \param	pDstBufInfo	    buf information of ouput data
 *
 * \return	0 - successed
 * \return	1 - failed
 *
 * \note	N/A
 *************************************************************************************
 */
int32_t WelsDecodeBs (PWelsDecoderContext pCtx, const uint8_t* kpBsBuf, const int32_t kiBsLen,
                      uint8_t** ppDst, SBufferInfo* pDstBufInfo, SParserBsInfo* pDstBsInfo) {
  if (!pCtx->bEndOfStreamFlag) {
    SDataBuffer* pRawData   = &pCtx->sRawData;
    SDataBuffer* pSavedData = NULL;

    int32_t iSrcIdx        = 0; //the index of source bit-stream till now after parsing one or more NALs
    int32_t iSrcConsumed   = 0; // consumed bit count of source bs
    int32_t iDstIdx        = 0; //the size of current NAL after 0x03 removal and 00 00 01 removal
    int32_t iSrcLength     = 0;	//the total size of current AU or NAL
    int32_t iRet = 0;
    int32_t iConsumedBytes = 0;
    int32_t iOffset        = 0;

    uint8_t* pSrcNal       = NULL;
    uint8_t* pDstNal       = NULL;
    uint8_t* pNalPayload   = NULL;


    if (NULL == DetectStartCodePrefix (kpBsBuf, &iOffset,
                                       kiBsLen)) {  //CAN'T find the 00 00 01 start prefix from the source buffer
      pCtx->iErrorCode |= dsBitstreamError;
      return dsBitstreamError;
    }

    pSrcNal    = const_cast<uint8_t*> (kpBsBuf) + iOffset;
    iSrcLength = kiBsLen - iOffset;

    if ((kiBsLen + 4) > (pRawData->pEnd - pRawData->pCurPos)) {
      pRawData->pCurPos = pRawData->pHead;
    }

    if (pCtx->bParseOnly) {
      pSavedData = &pCtx->sSavedData;
      if ((kiBsLen + 4) > (pSavedData->pEnd - pSavedData->pCurPos)) {
        pSavedData->pCurPos = pSavedData->pHead;
      }
    }
    //copy raw data from source buffer (application) to raw data buffer (codec inside)
    //0x03 removal and extract all of NAL Unit from current raw data
    pDstNal = pRawData->pCurPos;

    while (iSrcConsumed < iSrcLength) {
      if ((2 + iSrcConsumed < iSrcLength) &&
          (0 == LD16 (pSrcNal + iSrcIdx)) &&
          ((pSrcNal[2 + iSrcIdx] == 0x03) || (pSrcNal[2 + iSrcIdx] == 0x01))) {
        if (pSrcNal[2 + iSrcIdx] == 0x03) {
          ST16 (pDstNal + iDstIdx, 0);
          iDstIdx	+= 2;
          iSrcIdx	+= 3;
          iSrcConsumed += 3;
        } else {

          iConsumedBytes = 0;
          pDstNal[iDstIdx] = pDstNal[iDstIdx + 1] = pDstNal[iDstIdx + 2] = pDstNal[iDstIdx + 3] =
                               0; // set 4 reserved bytes to zero
          pNalPayload	= ParseNalHeader (pCtx, &pCtx->sCurNalHead, pDstNal, iDstIdx, pSrcNal - 3, iSrcIdx + 3, &iConsumedBytes);
          if (IS_VCL_NAL (pCtx->sCurNalHead.eNalUnitType, 1)) {
            CheckAndFinishLastPic (pCtx, ppDst, pDstBufInfo);
          }
          if (IS_PARAM_SETS_NALS (pCtx->sCurNalHead.eNalUnitType) && pNalPayload) {
            iRet = ParseNonVclNal (pCtx, pNalPayload, iDstIdx - iConsumedBytes, pSrcNal - 3, iSrcIdx + 3);
          }
          if (pCtx->bAuReadyFlag) {
            ConstructAccessUnit (pCtx, ppDst, pDstBufInfo);

            if ((dsOutOfMemory | dsNoParamSets) & pCtx->iErrorCode) {
#ifdef LONG_TERM_REF
              pCtx->bParamSetsLostFlag = true;
#else
              pCtx->bReferenceLostAtT0Flag = true;
#endif
              if (dsOutOfMemory & pCtx->iErrorCode) {
                return pCtx->iErrorCode;
              }
            }
          }
          if (iRet) {
            iRet = 0;
            if (dsNoParamSets & pCtx->iErrorCode) {
#ifdef LONG_TERM_REF
              pCtx->bParamSetsLostFlag = true;
#else
              pCtx->bReferenceLostAtT0Flag = true;
#endif
            }
            return pCtx->iErrorCode;
          }

          pDstNal += (iDstIdx + 4); //init, increase 4 reserved zero bytes, used to store the next NAL
          if ((iSrcLength - iSrcConsumed + 4) > (pRawData->pEnd - pDstNal)) {
            pDstNal = pRawData->pCurPos = pRawData->pHead;
          } else {
            pRawData->pCurPos = pDstNal;
          }

          pSrcNal += iSrcIdx + 3;
          iSrcConsumed += 3;
          iSrcIdx = 0;
          iDstIdx  = 0; //reset 0, used to statistic the length of next NAL
        }
        continue;
      }
      pDstNal[iDstIdx++] = pSrcNal[iSrcIdx++];
      iSrcConsumed++;
    }

    //last NAL decoding

    iConsumedBytes = 0;
    pDstNal[iDstIdx] = pDstNal[iDstIdx + 1] = pDstNal[iDstIdx + 2] = pDstNal[iDstIdx + 3] =
                         0; // set 4 reserved bytes to zero
    pNalPayload = ParseNalHeader (pCtx, &pCtx->sCurNalHead, pDstNal, iDstIdx, pSrcNal - 3, iSrcIdx + 3, &iConsumedBytes);
    if (IS_VCL_NAL (pCtx->sCurNalHead.eNalUnitType, 1)) {
      CheckAndFinishLastPic (pCtx, ppDst, pDstBufInfo);
    }
    if (IS_PARAM_SETS_NALS (pCtx->sCurNalHead.eNalUnitType) && pNalPayload) {
      iRet = ParseNonVclNal (pCtx, pNalPayload, iDstIdx - iConsumedBytes, pSrcNal - 3, iSrcIdx + 3);
    }
    if (pCtx->bAuReadyFlag) {
      ConstructAccessUnit (pCtx, ppDst, pDstBufInfo);

      if ((dsOutOfMemory | dsNoParamSets) & pCtx->iErrorCode) {
#ifdef LONG_TERM_REF
        pCtx->bParamSetsLostFlag = true;
#else
        pCtx->bReferenceLostAtT0Flag = true;
#endif
        return pCtx->iErrorCode;
      }
    }
    if (iRet) {
      iRet = 0;
      if (dsNoParamSets & pCtx->iErrorCode) {
#ifdef LONG_TERM_REF
        pCtx->bParamSetsLostFlag = true;
#else
        pCtx->bReferenceLostAtT0Flag = true;
#endif
      }
      return pCtx->iErrorCode;
    }
    pRawData->pCurPos = pDstNal + iDstIdx + 4; //init, increase 4 reserved zero bytes, used to store the next NAL
  } else { /* no supplementary picture payload input, but stored a picture */
    PAccessUnit pCurAu	=
      pCtx->pAccessUnitList;	// current access unit, it will never point to NULL after decode's successful initialization

    if (pCurAu->uiAvailUnitsNum == 0) {
      return pCtx->iErrorCode;
    } else {
      pCtx->pAccessUnitList->uiEndPos = pCtx->pAccessUnitList->uiAvailUnitsNum - 1;

      ConstructAccessUnit (pCtx, ppDst, pDstBufInfo);

      if ((dsOutOfMemory | dsNoParamSets) & pCtx->iErrorCode) {
#ifdef LONG_TERM_REF
        pCtx->bParamSetsLostFlag = true;
#else
        pCtx->bReferenceLostAtT0Flag = true;
#endif
        return pCtx->iErrorCode;
      }
    }
  }

  return pCtx->iErrorCode;
}

/*
 * set colorspace format in decoder
 */
int32_t DecoderSetCsp (PWelsDecoderContext pCtx, const int32_t kiColorFormat) {
  WELS_VERIFY_RETURN_IF (1, (NULL == pCtx));

  pCtx->eOutputColorFormat	= (EVideoFormatType) kiColorFormat;
  if (pCtx->pParam != NULL) {
    pCtx->pParam->eOutputColorFormat	= (EVideoFormatType) kiColorFormat;
  }

  //For now, support only videoFormatI420!
  if (kiColorFormat == (int32_t) videoFormatInternal) {
    pCtx->pParam->eOutputColorFormat = pCtx->eOutputColorFormat = videoFormatI420;
  } else if (kiColorFormat != (int32_t) videoFormatI420) {
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING, "Support I420 output only for now! Change to I420...");
    pCtx->pParam->eOutputColorFormat = pCtx->eOutputColorFormat = videoFormatI420;
    return cmUnsupportedData;
  }

  return 0;
}

/*!
 * \brief	make sure synchonozization picture resolution (get from slice header) among different parts (i.e, memory related and so on)
 *			over decoder internal
 * ( MB coordinate and parts of data within decoder context structure )
 * \param	pCtx		Wels decoder context
 * \param	iMbWidth	MB width
 * \pram	iMbHeight	MB height
 * \return	0 - successful; none 0 - something wrong
 */
int32_t SyncPictureResolutionExt (PWelsDecoderContext pCtx, const int32_t kiMbWidth, const int32_t kiMbHeight) {
  int32_t iErr = ERR_NONE;
  const int32_t kiPicWidth	= kiMbWidth << 4;
  const int32_t kiPicHeight   = kiMbHeight << 4;

  iErr = WelsRequestMem (pCtx, kiMbWidth, kiMbHeight);	// common memory used
  if (ERR_NONE != iErr) {
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING,
             "SyncPictureResolutionExt()::WelsRequestMem--buffer allocated failure.");
    pCtx->iErrorCode = dsOutOfMemory;
    return iErr;
  }

  iErr = InitialDqLayersContext (pCtx, kiPicWidth, kiPicHeight);
  if (ERR_NONE != iErr) {
    WelsLog (& (pCtx->sLogCtx), WELS_LOG_WARNING,
             "SyncPictureResolutionExt()::InitialDqLayersContext--buffer allocated failure.");
    pCtx->iErrorCode = dsOutOfMemory;
  }

  return iErr;
}

void AssignFuncPointerForRec (PWelsDecoderContext pCtx) {
  pCtx->pGetI16x16LumaPredFunc[I16_PRED_V     ] = WelsI16x16LumaPredV_c;
  pCtx->pGetI16x16LumaPredFunc[I16_PRED_H     ] = WelsI16x16LumaPredH_c;
  pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC    ] = WelsI16x16LumaPredDc_c;
  pCtx->pGetI16x16LumaPredFunc[I16_PRED_P     ] = WelsI16x16LumaPredPlane_c;
  pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC_L  ] = WelsI16x16LumaPredDcLeft_c;
  pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC_T  ] = WelsI16x16LumaPredDcTop_c;
  pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC_128] = WelsI16x16LumaPredDcNA_c;

  pCtx->pGetI4x4LumaPredFunc[I4_PRED_V     ] = WelsI4x4LumaPredV_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_H     ] = WelsI4x4LumaPredH_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_DC    ] = WelsI4x4LumaPredDc_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_DC_L  ] = WelsI4x4LumaPredDcLeft_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_DC_T  ] = WelsI4x4LumaPredDcTop_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_DC_128] = WelsI4x4LumaPredDcNA_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_DDL    ] = WelsI4x4LumaPredDDL_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_DDL_TOP] = WelsI4x4LumaPredDDLTop_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_DDR    ] = WelsI4x4LumaPredDDR_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_VL    ] = WelsI4x4LumaPredVL_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_VL_TOP] = WelsI4x4LumaPredVLTop_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_VR    ] = WelsI4x4LumaPredVR_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_HU    ] = WelsI4x4LumaPredHU_c;
  pCtx->pGetI4x4LumaPredFunc[I4_PRED_HD    ] = WelsI4x4LumaPredHD_c;

  pCtx->pGetIChromaPredFunc[C_PRED_DC    ] = WelsIChromaPredDc_c;
  pCtx->pGetIChromaPredFunc[C_PRED_H     ] = WelsIChromaPredH_c;
  pCtx->pGetIChromaPredFunc[C_PRED_V     ] = WelsIChromaPredV_c;
  pCtx->pGetIChromaPredFunc[C_PRED_P     ] = WelsIChromaPredPlane_c;
  pCtx->pGetIChromaPredFunc[C_PRED_DC_L  ] = WelsIChromaPredDcLeft_c;
  pCtx->pGetIChromaPredFunc[C_PRED_DC_T  ] = WelsIChromaPredDcTop_c;
  pCtx->pGetIChromaPredFunc[C_PRED_DC_128] = WelsIChromaPredDcNA_c;

  pCtx->pIdctResAddPredFunc	= IdctResAddPred_c;

#if defined(HAVE_NEON)
  if (pCtx->uiCpuFlag & WELS_CPU_NEON) {
    pCtx->pIdctResAddPredFunc	= IdctResAddPred_neon;

    pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC] = WelsDecoderI16x16LumaPredDc_neon;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_P]  = WelsDecoderI16x16LumaPredPlane_neon;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_H]  = WelsDecoderI16x16LumaPredH_neon;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_V]  = WelsDecoderI16x16LumaPredV_neon;

    pCtx->pGetI4x4LumaPredFunc[I4_PRED_V    ] = WelsDecoderI4x4LumaPredV_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_H    ] = WelsDecoderI4x4LumaPredH_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_DDL  ] = WelsDecoderI4x4LumaPredDDL_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_DDR  ] = WelsDecoderI4x4LumaPredDDR_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_VL   ] = WelsDecoderI4x4LumaPredVL_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_VR   ] = WelsDecoderI4x4LumaPredVR_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_HU   ] = WelsDecoderI4x4LumaPredHU_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_HD   ] = WelsDecoderI4x4LumaPredHD_neon;

    pCtx->pGetIChromaPredFunc[C_PRED_H]       = WelsDecoderIChromaPredH_neon;
    pCtx->pGetIChromaPredFunc[C_PRED_V]       = WelsDecoderIChromaPredV_neon;
    pCtx->pGetIChromaPredFunc[C_PRED_P ]      = WelsDecoderIChromaPredPlane_neon;
    pCtx->pGetIChromaPredFunc[C_PRED_DC]      = WelsDecoderIChromaPredDc_neon;
  }
#endif//HAVE_NEON

#if defined(HAVE_NEON_AARCH64)
  if (pCtx->uiCpuFlag & WELS_CPU_NEON) {
    pCtx->pIdctResAddPredFunc	= IdctResAddPred_AArch64_neon;

    pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC] = WelsDecoderI16x16LumaPredDc_AArch64_neon;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_P]  = WelsDecoderI16x16LumaPredPlane_AArch64_neon;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_H]  = WelsDecoderI16x16LumaPredH_AArch64_neon;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_V]  = WelsDecoderI16x16LumaPredV_AArch64_neon;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC_L]  = WelsDecoderI16x16LumaPredDcLeft_AArch64_neon;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC_T]  = WelsDecoderI16x16LumaPredDcTop_AArch64_neon;

    pCtx->pGetI4x4LumaPredFunc[I4_PRED_H    ] = WelsDecoderI4x4LumaPredH_AArch64_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_DDL  ] = WelsDecoderI4x4LumaPredDDL_AArch64_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_DDL_TOP] = WelsDecoderI4x4LumaPredDDLTop_AArch64_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_VL   ] = WelsDecoderI4x4LumaPredVL_AArch64_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_VL_TOP ] = WelsDecoderI4x4LumaPredVLTop_AArch64_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_VR   ] = WelsDecoderI4x4LumaPredVR_AArch64_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_HU   ] = WelsDecoderI4x4LumaPredHU_AArch64_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_HD   ] = WelsDecoderI4x4LumaPredHD_AArch64_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_DC   ] = WelsDecoderI4x4LumaPredDc_AArch64_neon;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_DC_T   ] = WelsDecoderI4x4LumaPredDcTop_AArch64_neon;

    pCtx->pGetIChromaPredFunc[C_PRED_H]       = WelsDecoderIChromaPredH_AArch64_neon;
    pCtx->pGetIChromaPredFunc[C_PRED_V]       = WelsDecoderIChromaPredV_AArch64_neon;
    pCtx->pGetIChromaPredFunc[C_PRED_P ]      = WelsDecoderIChromaPredPlane_AArch64_neon;
    pCtx->pGetIChromaPredFunc[C_PRED_DC]      = WelsDecoderIChromaPredDc_AArch64_neon;
    pCtx->pGetIChromaPredFunc[C_PRED_DC_T]      = WelsDecoderIChromaPredDcTop_AArch64_neon;
  }
#endif//HAVE_NEON_AARCH64

#if defined(X86_ASM)
  if (pCtx->uiCpuFlag & WELS_CPU_MMXEXT) {
    pCtx->pIdctResAddPredFunc	= IdctResAddPred_mmx;

    /////////mmx code opt---
    pCtx->pGetIChromaPredFunc[C_PRED_H]      = WelsDecoderIChromaPredH_mmx;
    pCtx->pGetIChromaPredFunc[C_PRED_V]      = WelsDecoderIChromaPredV_mmx;
    pCtx->pGetIChromaPredFunc[C_PRED_DC_L  ] = WelsDecoderIChromaPredDcLeft_mmx;
    pCtx->pGetIChromaPredFunc[C_PRED_DC_128] = WelsDecoderIChromaPredDcNA_mmx;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_DDR]  = WelsDecoderI4x4LumaPredDDR_mmx;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_HD ]  = WelsDecoderI4x4LumaPredHD_mmx;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_HU ]  = WelsDecoderI4x4LumaPredHU_mmx;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_VR ]  = WelsDecoderI4x4LumaPredVR_mmx;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_DDL]  = WelsDecoderI4x4LumaPredDDL_mmx;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_VL ]  = WelsDecoderI4x4LumaPredVL_mmx;
  }
  if (pCtx->uiCpuFlag & WELS_CPU_SSE2) {
    /////////sse2 code opt---
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC] = WelsDecoderI16x16LumaPredDc_sse2;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_P]  = WelsDecoderI16x16LumaPredPlane_sse2;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_H]  = WelsDecoderI16x16LumaPredH_sse2;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_V]  = WelsDecoderI16x16LumaPredV_sse2;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC_T  ] = WelsDecoderI16x16LumaPredDcTop_sse2;
    pCtx->pGetI16x16LumaPredFunc[I16_PRED_DC_128] = WelsDecoderI16x16LumaPredDcNA_sse2;
    pCtx->pGetIChromaPredFunc[C_PRED_P ]      = WelsDecoderIChromaPredPlane_sse2;
    pCtx->pGetIChromaPredFunc[C_PRED_DC]      = WelsDecoderIChromaPredDc_sse2;
    pCtx->pGetIChromaPredFunc[C_PRED_DC_T]    = WelsDecoderIChromaPredDcTop_sse2;
    pCtx->pGetI4x4LumaPredFunc[I4_PRED_H]     = WelsDecoderI4x4LumaPredH_sse2;
  }
#endif
  DeblockingInit (&pCtx->sDeblockingFunc, pCtx->uiCpuFlag);

  WelsBlockFuncInit (&pCtx->sBlockFunc, pCtx->uiCpuFlag);
}

//reset decoder number related statistics info
void ResetDecStatNums (SDecoderStatistics* pDecStat) {
  uint32_t uiWidth = pDecStat->uiWidth;
  uint32_t uiHeight = pDecStat->uiHeight;
  int32_t iAvgLumaQp = pDecStat->iAvgLumaQp;
  memset (pDecStat, 0, sizeof (SDecoderStatistics));
  pDecStat->uiWidth = uiWidth;
  pDecStat->uiHeight = uiHeight;
  pDecStat->iAvgLumaQp = iAvgLumaQp;
}

//update information when freezing occurs, including IDR/non-IDR number
void UpdateDecStatFreezingInfo (const bool kbIdrFlag, SDecoderStatistics* pDecStat) {
  if (kbIdrFlag)
    pDecStat->uiFreezingIDRNum++;
  else
    pDecStat->uiFreezingNonIDRNum++;
}

//update information when no freezing occurs, including QP, correct IDR number, ECed IDR number
void UpdateDecStatNoFreezingInfo (PWelsDecoderContext pCtx) {
  PDqLayer pCurDq = pCtx->pCurDqLayer;
  PPicture pPic = pCtx->pDec;
  SDecoderStatistics* pDecStat = &pCtx->sDecoderStatistics;

  if (pDecStat->iAvgLumaQp == -1) //first correct frame received
    pDecStat->iAvgLumaQp = 0;

  //update QP info
  int32_t iTotalQp = 0;
  const int32_t kiMbNum = pCurDq->iMbWidth * pCurDq->iMbHeight;
  for (int32_t iMb = 0; iMb < kiMbNum; ++iMb) {
    iTotalQp += pCurDq->pLumaQp[iMb] * pCurDq->pMbCorrectlyDecodedFlag[iMb];
  }
  iTotalQp /= kiMbNum;
  if (pDecStat->uiDecodedFrameCount + 1 == 0) { //maximum uint32_t reached
    ResetDecStatNums (pDecStat);
    pDecStat->iAvgLumaQp = iTotalQp;
  } else
    pDecStat->iAvgLumaQp = (uint64_t) (pDecStat->iAvgLumaQp * pDecStat->uiDecodedFrameCount + iTotalQp) /
                           (pDecStat->uiDecodedFrameCount + 1);

  //update IDR number
  if (pCurDq->sLayerInfo.sNalHeaderExt.bIdrFlag) {
    pDecStat->uiIDRCorrectNum += (pPic->bIsComplete);
    pDecStat->uiEcIDRNum += (!pPic->bIsComplete);
  }
}

//update decoder statistics information
void UpdateDecStat (PWelsDecoderContext pCtx, const bool kbOutput) {
  if (pCtx->bFreezeOutput)
    UpdateDecStatFreezingInfo (pCtx->pCurDqLayer->sLayerInfo.sNalHeaderExt.bIdrFlag, &pCtx->sDecoderStatistics);
  else if (kbOutput)
    UpdateDecStatNoFreezingInfo (pCtx);
}


} // namespace WelsDec
