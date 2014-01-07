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
#include <string.h>
#include "macros.h"
#include "codec_def.h"
#include "decoder.h"
#include "error_code.h"
#include "cpu.h"
#include "cpu_core.h"
#include "au_parser.h"
#include "utils.h"
#include "nal_prefix.h"
#include "dec_frame.h"
#include "pic_queue.h"
#include "vlc_decoder.h"
#include "get_intra_predictor.h"
#include "rec_mb.h"
#include "mc.h"
#include "decode_mb_aux.h"
#include "manage_dec_ref.h"
#include "codec_app_def.h"
#include "decoder_core.h"
#include "deblocking.h"
#include "expand_pic.h"
#include "decode_slice.h"
#include "crt_util_safe_x.h"	// Safe CRT routines like utils for cross platforms
#include "mem_align.h"

namespace WelsDec {

extern PPicture AllocPicture (PWelsDecoderContext pCtx, const int32_t kiPicWidth, const int32_t kiPicHeight);

extern void_t FreePicture (PPicture pPic);

#ifdef WORDS_BIGENDIAN
inline void_t GetValueOf4Bytes (uint8_t* pDstNal, int32_t iDdstIdx) {
  pDstNal[0] = (iDdstIdx & 0xff000000) >> 24;
  pDstNal[1] = (iDdstIdx & 0xff0000) >> 16;
  pDstNal[2] = (iDdstIdx & 0xff00) >> 8;
  pDstNal[3] = (iDdstIdx & 0xff);
}
#else //WORDS_BIGENDIAN
inline void_t GetValueOf4Bytes (uint8_t* pDstNal, int32_t iDdstIdx) {
  pDstNal[0] = (iDdstIdx & 0xff);
  pDstNal[1] = (iDdstIdx & 0xff00) >> 8;
  pDstNal[2] = (iDdstIdx & 0xff0000) >> 16;
  pDstNal[3] = (iDdstIdx & 0xff000000) >> 24;
}
#endif //WORDS_BIGENDIAN

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

static void_t DestroyPicBuff (PPicBuff* ppPicBuf) {
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
void_t WelsDecoderDefaults (PWelsDecoderContext pCtx) {  
  memset (pCtx, 0, sizeof (SWelsDecoderContext));	// fill zero first

  pCtx->pArgDec                   = NULL;

  pCtx->iOutputColorFormat		= videoFormatI420;	// yuv in default
  pCtx->bHaveGotMemory			= false;	// not ever request memory blocks for decoder context related
  pCtx->uiCpuFlag					= 0;

  pCtx->bAuReadyFlag				= 0; // au data is not ready


  g_uiCacheLineSize				= 16;
#if defined(X86_ASM)
  pCtx->uiCpuFlag = WelsCPUFeatureDetect ();
#ifdef HAVE_CACHE_LINE_ALIGN
  if (pCtx->uiCpuFlag & WELS_CPU_CACHELINE_64) {
    g_uiCacheLineSize	= 64;
  } else if (pCtx->uiCpuFlag & WELS_CPU_CACHELINE_32) {
    g_uiCacheLineSize	= 32;
  }
#endif//HAVE_CACHE_LINE_ALIGN
#endif//X86_ASM	

  pCtx->iImgWidthInPixel		= 0;
  pCtx->iImgHeightInPixel		= 0;		// alloc picture data when picture size is available

  pCtx->iFrameNum				= -1;
  pCtx->iPrevFrameNum			= -1;
  pCtx->iErrorCode			= ERR_NONE;

  pCtx->pDec					= NULL;

  WelsResetRefPic (pCtx);

  pCtx->iActiveFmoNum			= 0;

  pCtx->pPicBuff[LIST_0]		= NULL;
  pCtx->pPicBuff[LIST_1]		= NULL;

  pCtx->bAvcBasedFlag			= true;

}

/*
 *	destory_mb_blocks
 */


/*
 *	get size of reference picture list in target layer incoming, = (iNumRefFrames x 2)
 */
static inline int32_t GetTargetRefListSize (PWelsDecoderContext pCtx) {
  bool_t*  pSubsetSpsAvail = &pCtx->bSubspsAvailFlags[0];
  bool_t*  pSpsAvail		= &pCtx->bSpsAvailFlags[0];
  int32_t iSubsetIdx		= -1;
  int32_t iSpsIdx			= -1;
  bool_t  bExistSubsetSps = false;
  int32_t bExistSps		= false;
  int32_t iPos			= MAX_SPS_COUNT - 1;
  int32_t iNumRefFrames	= 0;

  while (iPos >= 0) {
    if (pSubsetSpsAvail[iPos]) {
      bExistSubsetSps	= true;
      iSubsetIdx		= iPos;
      break;
    }
    -- iPos;
  }

  if (!bExistSubsetSps) {
    iPos = MAX_SPS_COUNT - 1;
    while (iPos >= 0) {
      if (pSpsAvail[iPos]) {
        bExistSps	= true;
        iSpsIdx		= iPos;
        break;
      }
      -- iPos;
    }
  }

  if (! (bExistSubsetSps || bExistSps)) {
    iNumRefFrames = MAX_REF_PIC_COUNT;
  } else {
    PSps pSps = bExistSubsetSps ? (&pCtx->sSubsetSpsBuffer[iSubsetIdx].sSps) : (&pCtx->sSpsBuffer[iSpsIdx]);

    iNumRefFrames	= (pSps->iNumRefFrames) + 1;
  }

  if (0 == iNumRefFrames)
    iNumRefFrames	= (MIN_REF_PIC_COUNT);

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
  bool_t  bNeedChangePicQueue	= true;

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

  // for Recycled_Pic_Queue
  for (iListIdx = LIST_0; iListIdx < LIST_A; ++ iListIdx) {
    PPicBuff* ppPic = &pCtx->pPicBuff[iListIdx];
    if (NULL != ppPic && NULL != *ppPic) {
      DestroyPicBuff (ppPic);
    }
  }

  // currently only active for LIST_0 due to have no B frames
  iErr = CreatePicBuff (pCtx, &pCtx->pPicBuff[LIST_0], iPicQueueSize, kiPicWidth, kiPicHeight);
  if (iErr != ERR_NONE)
    return iErr;


  pCtx->iImgWidthInPixel	= kiPicWidth;	// target width of image to be reconstruted while decoding
  pCtx->iImgHeightInPixel	= kiPicHeight;	// target height of image to be reconstruted while decoding

  pCtx->bHaveGotMemory	= true;			// global memory for decoder context related is requested
  pCtx->pDec		        = NULL;			// need prefetch a new pic due to spatial size changed
  return ERR_NONE;
}

/*
 *	free memory blocks in avc
 */
void_t WelsFreeMem (PWelsDecoderContext pCtx) {
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
  pCtx->bHaveGotMemory	= false;

}

/*!
 * \brief	Open decoder
 */
void_t WelsOpenDecoder (PWelsDecoderContext pCtx) {
  // function pointers
  //initial MC function pointer--
  InitMcFunc (& (pCtx->sMcFunc), pCtx->uiCpuFlag);

  InitExpandPictureFunc (& (pCtx->sExpandPicFunc), pCtx->uiCpuFlag);
  AssignFuncPointerForRec (pCtx);

  // vlc tables
  InitVlcTable (&pCtx->sVlcTable);

  // startup memory
  if (ERR_NONE != WelsInitMemory (pCtx))
    return;

  pCtx->iMaxWidthInSps	= 0;
  pCtx->iMaxHeightInSps	= 0;
#ifdef LONG_TERM_REF
  pCtx->bParamSetsLostFlag = true;
#else
  pCtx->bReferenceLostAtT0Flag	= true;	// should be true to waiting IDR at incoming AU bits following, 6/4/2010
#endif //LONG_TERM_REF
}

/*!
 * \brief	Close decoder
 */
void_t WelsCloseDecoder (PWelsDecoderContext pCtx) {
  WelsFreeMem (pCtx);

  WelsFreeMemory (pCtx);

  UninitialDqLayersContext (pCtx);

#ifdef LONG_TERM_REF
  pCtx->bParamSetsLostFlag       = false;
#else
  pCtx->bReferenceLostAtT0Flag = false;
#endif
}

/*!
 * \brief	configure decoder parameters
 */
int32_t DecoderConfigParam (PWelsDecoderContext pCtx, const void_t* kpParam) {
  if (NULL == pCtx || NULL == kpParam)
    return 1;

  pCtx->pParam	= (SDecodingParam*)WelsMalloc (sizeof (SDecodingParam), "SDecodingParam");

  if (NULL == pCtx->pParam)
    return 1;

  memcpy (pCtx->pParam, kpParam, sizeof (SDecodingParam));
  pCtx->iOutputColorFormat	= pCtx->pParam->iOutputColorFormat;
  pCtx->bErrorResilienceFlag	= pCtx->pParam->uiEcActiveFlag ? true : false;

  if (VIDEO_BITSTREAM_SVC == pCtx->pParam->sVideoProperty.eVideoBsType ||
      VIDEO_BITSTREAM_AVC == pCtx->pParam->sVideoProperty.eVideoBsType) {
    pCtx->eVideoType = pCtx->pParam->sVideoProperty.eVideoBsType;
  } else {
    pCtx->eVideoType = VIDEO_BITSTREAM_DEFAULT;
  }

  WelsLog (pCtx, WELS_LOG_INFO, "eVideoType: %d\n", pCtx->eVideoType);

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
int32_t WelsInitDecoder (PWelsDecoderContext pCtx, void_t* pTraceHandle, PWelsLogCallbackFunc pLog) {
  if (pCtx == NULL) {
    return ERR_INFO_INVALID_PTR;
  }

  // default
  WelsDecoderDefaults (pCtx);

  pCtx->pTraceHandle = pTraceHandle;

  g_pLog = pLog;

  // open decoder
  WelsOpenDecoder (pCtx);

  // decode mode setting
  pCtx->iDecoderMode = SW_MODE;
  pCtx->iSetMode = AUTO_MODE;
  pCtx->iDecoderOutputProperty = BUFFER_HOST;
  pCtx->iModeSwitchType = 0; // 0: do not do mode switch


  return ERR_NONE;
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
void_t WelsEndDecoder (PWelsDecoderContext pCtx) {
  // close decoder
  WelsCloseDecoder (pCtx);
}

void_t GetVclNalTemporalId (PWelsDecoderContext pCtx) {
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
                      uint8_t** ppDst, SBufferInfo* pDstBufInfo) {
  if (!pCtx->bEndOfStreamFlag) {
    SDataBuffer* pRawData   = &pCtx->sRawData;

    int32_t iSrcIdx        = 0; //the index of source bit-stream till now after parsing one or more NALs
    int32_t iSrcConsumed   = 0; // consumed bit count of source bs
    int32_t iDstIdx        = 0; //the size of current NAL after 0x03 removal and 00 00 01 removal
    int32_t iSrcLength     = 0;	//the total size of current AU or NAL

    int32_t iConsumedBytes = 0;
    int32_t iOffset        = 0;

    uint8_t* pSrcNal       = NULL;
    uint8_t* pDstNal       = NULL;
    uint8_t* pNalPayload   = NULL;


    if (NULL == DetectStartCodePrefix (kpBsBuf, &iOffset,
                                       kiBsLen)) {  //CAN'T find the 00 00 01 start prefix from the source buffer
      return dsBitstreamError;
    }

    pSrcNal    = const_cast<uint8_t*> (kpBsBuf) + iOffset;
    iSrcLength = kiBsLen - iOffset;

    if ((kiBsLen + 4) > (pRawData->pEnd - pRawData->pCurPos)) {
      pRawData->pCurPos = pRawData->pHead;
    }


    //copy raw data from source buffer (application) to raw data buffer (codec inside)
    //0x03 removal and extract all of NAL Unit from current raw data
    pDstNal = pRawData->pCurPos + 4; //4-bytes used to write the length of current NAL rbsp

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
          GetValueOf4Bytes (pDstNal - 4, iDstIdx);  //pDstNal-4 (non-aligned by 4) in Solaris10(SPARC). Given value by byte.

          iConsumedBytes = 0;
          pNalPayload	= ParseNalHeader (pCtx, &pCtx->sCurNalHead, pDstNal, iDstIdx, pSrcNal - 3, iSrcIdx + 3, &iConsumedBytes);

          if (pCtx->bAuReadyFlag) {
            ConstructAccessUnit (pCtx, ppDst, pDstBufInfo);

            if ((dsOutOfMemory | dsNoParamSets) & pCtx->iErrorCode) {
#ifdef LONG_TERM_REF
              pCtx->bParamSetsLostFlag = true;
#else
              pCtx->bReferenceLostAtT0Flag = true;
#endif
              ResetParameterSetsState (pCtx);

              if (dsOutOfMemory & pCtx->iErrorCode) {
                return pCtx->iErrorCode;
              }
            }
          }

          if ((IS_PARAM_SETS_NALS (pCtx->sCurNalHead.eNalUnitType) || IS_SEI_NAL (pCtx->sCurNalHead.eNalUnitType)) &&
              pNalPayload) {
            if (ParseNonVclNal (pCtx, pNalPayload, iDstIdx - iConsumedBytes)) {
              if (dsNoParamSets & pCtx->iErrorCode) {
#ifdef LONG_TERM_REF
                pCtx->bParamSetsLostFlag = true;
#else
                pCtx->bReferenceLostAtT0Flag = true;
#endif
                ResetParameterSetsState (pCtx);
              }
              return pCtx->iErrorCode;
            }
          }

          pDstNal += iDstIdx; //update current position
          if ((iSrcLength - iSrcConsumed + 4) > (pRawData->pEnd - pDstNal)) {
            pRawData->pCurPos = pRawData->pHead;
          } else {
            pRawData->pCurPos = pDstNal;
          }
          pDstNal = pRawData->pCurPos + 4; //init, 4 bytes used to store the next NAL

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
    GetValueOf4Bytes (pDstNal - 4, iDstIdx); //pDstNal-4 (non-aligned by 4) in Solaris10(SPARC). Given value by byte.

    iConsumedBytes = 0;
    pNalPayload = ParseNalHeader (pCtx, &pCtx->sCurNalHead, pDstNal, iDstIdx, pSrcNal - 3, iSrcIdx + 3, &iConsumedBytes);

    if (pCtx->bAuReadyFlag) {
      ConstructAccessUnit (pCtx, ppDst, pDstBufInfo);

      if ((dsOutOfMemory | dsNoParamSets) & pCtx->iErrorCode) {
#ifdef LONG_TERM_REF
        pCtx->bParamSetsLostFlag = true;
#else
        pCtx->bReferenceLostAtT0Flag = true;
#endif
        ResetParameterSetsState (pCtx);
        return pCtx->iErrorCode;
      }
    }

    if ((IS_PARAM_SETS_NALS (pCtx->sCurNalHead.eNalUnitType) || IS_SEI_NAL (pCtx->sCurNalHead.eNalUnitType))
        && pNalPayload) {
      if (ParseNonVclNal (pCtx, pNalPayload, iDstIdx - iConsumedBytes)) {
        if (dsNoParamSets & pCtx->iErrorCode) {
#ifdef LONG_TERM_REF
          pCtx->bParamSetsLostFlag = true;
#else
          pCtx->bReferenceLostAtT0Flag = true;
#endif
          ResetParameterSetsState (pCtx);
        }
        return pCtx->iErrorCode;
      }
    }

    pDstNal += iDstIdx;
    pRawData->pCurPos = pDstNal; //init the pCurPos for next NAL(s) storage
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
        ResetParameterSetsState (pCtx);
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

  pCtx->iOutputColorFormat	= kiColorFormat;
  if (pCtx->pParam != NULL) {
    pCtx->pParam->iOutputColorFormat	= kiColorFormat;
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
    WelsLog (pCtx, WELS_LOG_WARNING, "SyncPictureResolutionExt()::WelsRequestMem--buffer allocated failure.\n");
    pCtx->iErrorCode = dsOutOfMemory;
    return iErr;
  }

  iErr = InitialDqLayersContext (pCtx, kiPicWidth, kiPicHeight);
  if (ERR_NONE != iErr) {
    WelsLog (pCtx, WELS_LOG_WARNING, "SyncPictureResolutionExt()::InitialDqLayersContext--buffer allocated failure.\n");
    pCtx->iErrorCode = dsOutOfMemory;
  }

  return iErr;
}

/*!
 * \brief	update maximal picture width and height if applicable when receiving a SPS NAL
 */
void_t UpdateMaxPictureResolution (PWelsDecoderContext pCtx, const int32_t kiCurWidth, const int32_t kiCurHeight) {
  //any dimension larger than that of current dimension, should modify the max-dimension
  if (kiCurWidth > pCtx->iMaxWidthInSps || kiCurHeight > pCtx->iMaxHeightInSps) {
    pCtx->iMaxWidthInSps	= kiCurWidth;
    pCtx->iMaxHeightInSps	= kiCurHeight;
  }

  return;
}

void_t AssignFuncPointerForRec (PWelsDecoderContext pCtx) {
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

  InitDctClipTable();
  pCtx->pIdctResAddPredFunc	= IdctResAddPred_c;

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
  }
#endif
  DeblockingInit (&pCtx->sDeblockingFunc, pCtx->uiCpuFlag);

  WelsBlockFuncInit (&pCtx->sBlockFunc, pCtx->uiCpuFlag);
}

} // namespace WelsDec
