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
 *  welsDecoderExt.cpp
 *
 *  Abstract
 *      Cisco OpenH264 decoder extension utilization
 *
 *  History
 *      3/12/2009 Created
 *
 *
 ************************************************************************/
//#include <assert.h>
#include "welsDecoderExt.h"
#include "welsCodecTrace.h"
#include "codec_def.h"
#include "typedefs.h"
#include "memory_align.h"
#include "utils.h"
#include "version.h"

//#include "macros.h"
#include "decoder.h"
#include "decoder_core.h"
#include "error_concealment.h"

#include "measure_time.h"
extern "C" {
#include "decoder_core.h"
#include "manage_dec_ref.h"
}
#include "error_code.h"
#include "crt_util_safe_x.h" // Safe CRT routines like util for cross platforms
#include <time.h>
#if defined(_WIN32) /*&& defined(_DEBUG)*/

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#else
#include <sys/time.h>
#endif

#define _PICTURE_REORDERING_ 1

static int32_t sIMinInt32 = -0x7FFFFFFF;

namespace WelsDec {

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/***************************************************************************
*   Description:
*       class CWelsDecoder constructor function, do initialization  and
*       alloc memory required
*
*   Input parameters: none
*
*   return: none
***************************************************************************/
CWelsDecoder::CWelsDecoder (void)
  : m_pDecContext (NULL),
    m_pWelsTrace (NULL),
    m_iPictInfoIndex (0),
    m_iMinPOC (sIMinInt32),
    m_iNumOfPicts (0),
    m_iLastGOPRemainPicts (0),
    m_LastWrittenPOC (sIMinInt32),
    m_iLargestBufferedPicIndex (0) {
#ifdef OUTPUT_BIT_STREAM
  char chFileName[1024] = { 0 };  //for .264
  int iBufUsed = 0;
  int iBufLeft = 1023;
  int iCurUsed;

  char chFileNameSize[1024] = { 0 }; //for .len
  int iBufUsedSize = 0;
  int iBufLeftSize = 1023;
  int iCurUsedSize;
#endif//OUTPUT_BIT_STREAM


  m_pWelsTrace = new welsCodecTrace();
  if (m_pWelsTrace != NULL) {
    m_pWelsTrace->SetCodecInstance (this);
    m_pWelsTrace->SetTraceLevel (WELS_LOG_ERROR);

    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsDecoder::CWelsDecoder() entry");
  }

  for (int32_t i = 0; i < 16; ++i) {
    m_sPictInfoList[i].bLastGOP = false;
    m_sPictInfoList[i].iPOC = sIMinInt32;
  }

#ifdef OUTPUT_BIT_STREAM
  SWelsTime sCurTime;

  WelsGetTimeOfDay (&sCurTime);

  iCurUsed = WelsSnprintf (chFileName, iBufLeft, "bs_0x%p_", (void*)this);
  iCurUsedSize = WelsSnprintf (chFileNameSize, iBufLeftSize, "size_0x%p_", (void*)this);

  iBufUsed += iCurUsed;
  iBufLeft -= iCurUsed;
  if (iBufLeft > 0) {
    iCurUsed = WelsStrftime (&chFileName[iBufUsed], iBufLeft, "%y%m%d%H%M%S", &sCurTime);
    iBufUsed += iCurUsed;
    iBufLeft -= iCurUsed;
  }

  iBufUsedSize += iCurUsedSize;
  iBufLeftSize -= iCurUsedSize;
  if (iBufLeftSize > 0) {
    iCurUsedSize = WelsStrftime (&chFileNameSize[iBufUsedSize], iBufLeftSize, "%y%m%d%H%M%S", &sCurTime);
    iBufUsedSize += iCurUsedSize;
    iBufLeftSize -= iCurUsedSize;
  }

  if (iBufLeft > 0) {
    iCurUsed = WelsSnprintf (&chFileName[iBufUsed], iBufLeft, ".%03.3u.264", WelsGetMillisecond (&sCurTime));
    iBufUsed += iCurUsed;
    iBufLeft -= iCurUsed;
  }

  if (iBufLeftSize > 0) {
    iCurUsedSize = WelsSnprintf (&chFileNameSize[iBufUsedSize], iBufLeftSize, ".%03.3u.len",
                                 WelsGetMillisecond (&sCurTime));
    iBufUsedSize += iCurUsedSize;
    iBufLeftSize -= iCurUsedSize;
  }


  m_pFBS = WelsFopen (chFileName, "wb");
  m_pFBSSize = WelsFopen (chFileNameSize, "wb");
#endif//OUTPUT_BIT_STREAM
}

/***************************************************************************
*   Description:
*       class CWelsDecoder destructor function, destroy allocced memory
*
*   Input parameters: none
*
*   return: none
***************************************************************************/
CWelsDecoder::~CWelsDecoder() {
  if (m_pWelsTrace != NULL) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsDecoder::~CWelsDecoder()");
  }

  UninitDecoder();

#ifdef OUTPUT_BIT_STREAM
  if (m_pFBS) {
    WelsFclose (m_pFBS);
    m_pFBS = NULL;
  }
  if (m_pFBSSize) {
    WelsFclose (m_pFBSSize);
    m_pFBSSize = NULL;
  }
#endif//OUTPUT_BIT_STREAM

  if (m_pWelsTrace != NULL) {
    delete m_pWelsTrace;
    m_pWelsTrace = NULL;
  }
}

long CWelsDecoder::Initialize (const SDecodingParam* pParam) {
  int iRet = ERR_NONE;
  if (m_pWelsTrace == NULL) {
    return cmMallocMemeError;
  }

  if (pParam == NULL) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "CWelsDecoder::Initialize(), invalid input argument.");
    return cmInitParaError;
  }

  // H.264 decoder initialization,including memory allocation,then open it ready to decode
  iRet = InitDecoder (pParam);
  if (iRet)
    return iRet;

  return cmResultSuccess;
}

long CWelsDecoder::Uninitialize() {
  UninitDecoder();

  return ERR_NONE;
}

void CWelsDecoder::UninitDecoder (void) {
  if (NULL == m_pDecContext)
    return;

  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "CWelsDecoder::UninitDecoder(), openh264 codec version = %s.",
           VERSION_NUMBER);

  WelsEndDecoder (m_pDecContext);

  if (m_pDecContext->pMemAlign != NULL) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsDecoder::UninitDecoder(), verify memory usage (%d bytes) after free..",
             m_pDecContext->pMemAlign->WelsGetMemoryUsage());
    delete m_pDecContext->pMemAlign;
    m_pDecContext->pMemAlign = NULL;
  }

  if (NULL != m_pDecContext) {
    WelsFree (m_pDecContext, "m_pDecContext");

    m_pDecContext = NULL;
  }
}

// the return value of this function is not suitable, it need report failure info to upper layer.
int32_t CWelsDecoder::InitDecoder (const SDecodingParam* pParam) {

  WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
           "CWelsDecoder::init_decoder(), openh264 codec version = %s, ParseOnly = %d",
           VERSION_NUMBER, (int32_t)pParam->bParseOnly);

  //reset decoder context
  if (m_pDecContext) //free
    UninitDecoder();
  m_pDecContext = (PWelsDecoderContext)WelsMallocz (sizeof (SWelsDecoderContext), "m_pDecContext");
  if (NULL == m_pDecContext)
    return cmMallocMemeError;
  int32_t iCacheLineSize = 16;   // on chip cache line size in byte
  m_pDecContext->pMemAlign = new CMemoryAlign (iCacheLineSize);
  WELS_VERIFY_RETURN_PROC_IF (cmMallocMemeError, (NULL == m_pDecContext->pMemAlign), UninitDecoder())

  //fill in default value into context
  WelsDecoderDefaults (m_pDecContext, &m_pWelsTrace->m_sLogCtx);

  //check param and update decoder context
  m_pDecContext->pParam = (SDecodingParam*)m_pDecContext->pMemAlign->WelsMallocz (sizeof (SDecodingParam),
                          "SDecodingParam");
  WELS_VERIFY_RETURN_PROC_IF (cmMallocMemeError, (NULL == m_pDecContext->pParam), UninitDecoder());
  int32_t iRet = DecoderConfigParam (m_pDecContext, pParam);
  WELS_VERIFY_RETURN_IFNEQ (iRet, cmResultSuccess);

  //init decoder
  WELS_VERIFY_RETURN_PROC_IF (cmMallocMemeError, WelsInitDecoder (m_pDecContext, &m_pWelsTrace->m_sLogCtx),
                              UninitDecoder())

  return cmResultSuccess;
}

int32_t CWelsDecoder::ResetDecoder() {
  // TBC: need to be modified when context and trace point are null
  if (m_pDecContext != NULL && m_pWelsTrace != NULL) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "ResetDecoder(), context error code is %d",
             m_pDecContext->iErrorCode);
    SDecodingParam sPrevParam;
    memcpy (&sPrevParam, m_pDecContext->pParam, sizeof (SDecodingParam));

    WELS_VERIFY_RETURN_PROC_IF (cmInitParaError, InitDecoder (&sPrevParam), UninitDecoder());
  } else if (m_pWelsTrace != NULL) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "ResetDecoder() failed as decoder context null");
  }
  return ERR_INFO_UNINIT;
}

/*
 * Set Option
 */
long CWelsDecoder::SetOption (DECODER_OPTION eOptID, void* pOption) {
  int iVal = 0;

  if (m_pDecContext == NULL && eOptID != DECODER_OPTION_TRACE_LEVEL &&
      eOptID != DECODER_OPTION_TRACE_CALLBACK && eOptID != DECODER_OPTION_TRACE_CALLBACK_CONTEXT)
    return dsInitialOptExpected;
  if (eOptID == DECODER_OPTION_END_OF_STREAM) { // Indicate bit-stream of the final frame to be decoded
    if (pOption == NULL)
      return cmInitParaError;

    iVal = * ((int*)pOption); // boolean value for whether enabled End Of Stream flag

    m_pDecContext->bEndOfStreamFlag = iVal ? true : false;

    return cmResultSuccess;
  } else if (eOptID == DECODER_OPTION_ERROR_CON_IDC) { // Indicate error concealment status
    if (pOption == NULL)
      return cmInitParaError;

    iVal = * ((int*)pOption); // int value for error concealment idc
    iVal = WELS_CLIP3 (iVal, (int32_t)ERROR_CON_DISABLE, (int32_t)ERROR_CON_SLICE_MV_COPY_CROSS_IDR_FREEZE_RES_CHANGE);
    if ((m_pDecContext->pParam->bParseOnly) && (iVal != (int32_t)ERROR_CON_DISABLE)) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsDecoder::SetOption for ERROR_CON_IDC = %d not allowd for parse only!.", iVal);
      return cmInitParaError;
    }

    m_pDecContext->pParam->eEcActiveIdc = (ERROR_CON_IDC)iVal;
    InitErrorCon (m_pDecContext);
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "CWelsDecoder::SetOption for ERROR_CON_IDC = %d.", iVal);

    return cmResultSuccess;
  } else if (eOptID == DECODER_OPTION_TRACE_LEVEL) {
    if (m_pWelsTrace) {
      uint32_t level = * ((uint32_t*)pOption);
      m_pWelsTrace->SetTraceLevel (level);
    }
    return cmResultSuccess;
  } else if (eOptID == DECODER_OPTION_TRACE_CALLBACK) {
    if (m_pWelsTrace) {
      WelsTraceCallback callback = * ((WelsTraceCallback*)pOption);
      m_pWelsTrace->SetTraceCallback (callback);
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
               "CWelsDecoder::SetOption():DECODER_OPTION_TRACE_CALLBACK callback = %p.",
               callback);
    }
    return cmResultSuccess;
  } else if (eOptID == DECODER_OPTION_TRACE_CALLBACK_CONTEXT) {
    if (m_pWelsTrace) {
      void* ctx = * ((void**)pOption);
      m_pWelsTrace->SetTraceCallbackContext (ctx);
    }
    return cmResultSuccess;
  } else if (eOptID == DECODER_OPTION_GET_STATISTICS) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_WARNING,
             "CWelsDecoder::SetOption():DECODER_OPTION_GET_STATISTICS: this option is get-only!");
    return cmInitParaError;
  } else if (eOptID == DECODER_OPTION_STATISTICS_LOG_INTERVAL) {
    if (pOption) {
      m_pDecContext->sDecoderStatistics.iStatisticsLogInterval = (* ((unsigned int*)pOption));
      return cmResultSuccess;
    }
  } else if (eOptID == DECODER_OPTION_GET_SAR_INFO) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_WARNING,
             "CWelsDecoder::SetOption():DECODER_OPTION_GET_SAR_INFO: this option is get-only!");
    return cmInitParaError;
  }
  return cmInitParaError;
}

/*
 *  Get Option
 */
long CWelsDecoder::GetOption (DECODER_OPTION eOptID, void* pOption) {
  int iVal = 0;

  if (m_pDecContext == NULL)
    return cmInitExpected;

  if (pOption == NULL)
    return cmInitParaError;

  if (DECODER_OPTION_END_OF_STREAM == eOptID) {
    iVal = m_pDecContext->bEndOfStreamFlag;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  }
#ifdef LONG_TERM_REF
  else if (DECODER_OPTION_IDR_PIC_ID == eOptID) {
    iVal = m_pDecContext->uiCurIdrPicId;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  } else if (DECODER_OPTION_FRAME_NUM == eOptID) {
    iVal = m_pDecContext->iFrameNum;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  } else if (DECODER_OPTION_LTR_MARKING_FLAG == eOptID) {
    iVal = m_pDecContext->bCurAuContainLtrMarkSeFlag;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  } else if (DECODER_OPTION_LTR_MARKED_FRAME_NUM == eOptID) {
    iVal = m_pDecContext->iFrameNumOfAuMarkedLtr;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  }
#endif
  else if (DECODER_OPTION_VCL_NAL == eOptID) { //feedback whether or not have VCL NAL in current AU
    iVal = m_pDecContext->iFeedbackVclNalInAu;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  } else if (DECODER_OPTION_TEMPORAL_ID == eOptID) { //if have VCL NAL in current AU, then feedback the temporal ID
    iVal = m_pDecContext->iFeedbackTidInAu;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  } else if (DECODER_OPTION_IS_REF_PIC == eOptID) {
    iVal = m_pDecContext->iFeedbackNalRefIdc;
    if (iVal > 0)
      iVal = 1;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  } else if (DECODER_OPTION_ERROR_CON_IDC == eOptID) {
    iVal = (int)m_pDecContext->pParam->eEcActiveIdc;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  } else if (DECODER_OPTION_GET_STATISTICS == eOptID) { // get decoder statistics info for real time debugging
    SDecoderStatistics* pDecoderStatistics = (static_cast<SDecoderStatistics*> (pOption));

    memcpy (pDecoderStatistics, &m_pDecContext->sDecoderStatistics, sizeof (SDecoderStatistics));

    if (m_pDecContext->sDecoderStatistics.uiDecodedFrameCount != 0) { //not original status
      pDecoderStatistics->fAverageFrameSpeedInMs = (float) (m_pDecContext->dDecTime) /
          (m_pDecContext->sDecoderStatistics.uiDecodedFrameCount);
      pDecoderStatistics->fActualAverageFrameSpeedInMs = (float) (m_pDecContext->dDecTime) /
          (m_pDecContext->sDecoderStatistics.uiDecodedFrameCount + m_pDecContext->sDecoderStatistics.uiFreezingIDRNum +
           m_pDecContext->sDecoderStatistics.uiFreezingNonIDRNum);
    }
    return cmResultSuccess;
  } else if (eOptID == DECODER_OPTION_STATISTICS_LOG_INTERVAL) {
    if (pOption) {
      iVal = m_pDecContext->sDecoderStatistics.iStatisticsLogInterval;
      * ((unsigned int*)pOption) = iVal;
      return cmResultSuccess;
    }
  } else if (DECODER_OPTION_GET_SAR_INFO == eOptID) { //get decoder SAR info in VUI
    PVuiSarInfo pVuiSarInfo = (static_cast<PVuiSarInfo> (pOption));
    memset (pVuiSarInfo, 0, sizeof (SVuiSarInfo));
    if (!m_pDecContext->pSps) {
      return cmInitExpected;
    } else {
      pVuiSarInfo->uiSarWidth = m_pDecContext->pSps->sVui.uiSarWidth;
      pVuiSarInfo->uiSarHeight = m_pDecContext->pSps->sVui.uiSarHeight;
      pVuiSarInfo->bOverscanAppropriateFlag = m_pDecContext->pSps->sVui.bOverscanAppropriateFlag;
      return cmResultSuccess;
    }
  } else if (DECODER_OPTION_PROFILE == eOptID) {
    if (!m_pDecContext->pSps) {
      return cmInitExpected;
    }
    iVal = (int)m_pDecContext->pSps->uiProfileIdc;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  } else if (DECODER_OPTION_LEVEL == eOptID) {
    if (!m_pDecContext->pSps) {
      return cmInitExpected;
    }
    iVal = (int)m_pDecContext->pSps->uiLevelIdc;
    * ((int*)pOption) = iVal;
    return cmResultSuccess;
  } else if (DECODER_OPTION_NUM_OF_FRAMES_REMAINING_IN_BUFFER == eOptID) {
    if (m_pDecContext->pSps && m_pDecContext->pSps->uiProfileIdc != 66) {
      * ((int*)pOption) = m_iNumOfPicts > 0 ? m_iNumOfPicts : 0;
    } else {
      * ((int*)pOption) = 0;
    }
    return cmResultSuccess;
  }

  return cmInitParaError;
}

DECODING_STATE CWelsDecoder::DecodeFrameNoDelay (const unsigned char* kpSrc,
    const int kiSrcLen,
    unsigned char** ppDst,
    SBufferInfo* pDstInfo) {
  int iRet;
  //SBufferInfo sTmpBufferInfo;
  //unsigned char* ppTmpDst[3] = {NULL, NULL, NULL};
  iRet = (int)DecodeFrame2 (kpSrc, kiSrcLen, ppDst, pDstInfo);
  //memcpy (&sTmpBufferInfo, pDstInfo, sizeof (SBufferInfo));
  //ppTmpDst[0] = ppDst[0];
  //ppTmpDst[1] = ppDst[1];
  //ppTmpDst[2] = ppDst[2];
  iRet |= DecodeFrame2 (NULL, 0, ppDst, pDstInfo);
  //if ((pDstInfo->iBufferStatus == 0) && (sTmpBufferInfo.iBufferStatus == 1)) {
  //memcpy (pDstInfo, &sTmpBufferInfo, sizeof (SBufferInfo));
  //ppDst[0] = ppTmpDst[0];
  //ppDst[1] = ppTmpDst[1];
  //ppDst[2] = ppTmpDst[2];
  //}
  return (DECODING_STATE)iRet;
}

DECODING_STATE CWelsDecoder::DecodeFrame2 (const unsigned char* kpSrc,
    const int kiSrcLen,
    unsigned char** ppDst,
    SBufferInfo* pDstInfo) {
  if (m_pDecContext == NULL || m_pDecContext->pParam == NULL) {
    if (m_pWelsTrace != NULL) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "Call DecodeFrame2 without Initialize.\n");
    }
    return dsInitialOptExpected;
  }

  if (m_pDecContext->pParam->bParseOnly) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "bParseOnly should be false for this API calling! \n");
    m_pDecContext->iErrorCode |= dsInvalidArgument;
    return dsInvalidArgument;
  }
  if (CheckBsBuffer (m_pDecContext, kiSrcLen)) {
    if (ResetDecoder())
      return dsOutOfMemory;

    return dsErrorFree;
  }
  if (kiSrcLen > 0 && kpSrc != NULL) {
#ifdef OUTPUT_BIT_STREAM
    if (m_pFBS) {
      WelsFwrite (kpSrc, sizeof (unsigned char), kiSrcLen, m_pFBS);
      WelsFflush (m_pFBS);
    }
    if (m_pFBSSize) {
      WelsFwrite (&kiSrcLen, sizeof (int), 1, m_pFBSSize);
      WelsFflush (m_pFBSSize);
    }
#endif//OUTPUT_BIT_STREAM
    m_pDecContext->bEndOfStreamFlag = false;
  } else {
    //For application MODE, the error detection should be added for safe.
    //But for CONSOLE MODE, when decoding LAST AU, kiSrcLen==0 && kpSrc==NULL.
    m_pDecContext->bEndOfStreamFlag = true;
    m_pDecContext->bInstantDecFlag = true;
  }

  int64_t iStart, iEnd;
  iStart = WelsTime();

  ppDst[0] = ppDst[1] = ppDst[2] = NULL;
  m_pDecContext->iErrorCode = dsErrorFree; //initialize at the starting of AU decoding.
  m_pDecContext->iFeedbackVclNalInAu = FEEDBACK_UNKNOWN_NAL; //initialize
  unsigned long long uiInBsTimeStamp = pDstInfo->uiInBsTimeStamp;
  memset (pDstInfo, 0, sizeof (SBufferInfo));
  pDstInfo->uiInBsTimeStamp = uiInBsTimeStamp;
#ifdef LONG_TERM_REF
  m_pDecContext->bReferenceLostAtT0Flag = false; //initialize for LTR
  m_pDecContext->bCurAuContainLtrMarkSeFlag = false;
  m_pDecContext->iFrameNumOfAuMarkedLtr = 0;
  m_pDecContext->iFrameNum = -1; //initialize
#endif

  m_pDecContext->iFeedbackTidInAu = -1; //initialize
  m_pDecContext->iFeedbackNalRefIdc = -1; //initialize
  if (pDstInfo) {
    pDstInfo->uiOutYuvTimeStamp = 0;
    m_pDecContext->uiTimeStamp = pDstInfo->uiInBsTimeStamp;
  } else {
    m_pDecContext->uiTimeStamp = 0;
  }
  WelsDecodeBs (m_pDecContext, kpSrc, kiSrcLen, ppDst,
                pDstInfo, NULL); //iErrorCode has been modified in this function
  m_pDecContext->bInstantDecFlag = false; //reset no-delay flag
  if (m_pDecContext->iErrorCode) {
    EWelsNalUnitType eNalType =
      NAL_UNIT_UNSPEC_0; //for NBR, IDR frames are expected to decode as followed if error decoding an IDR currently

    eNalType = m_pDecContext->sCurNalHead.eNalUnitType;

    if (m_pDecContext->iErrorCode & dsOutOfMemory) {
      if (ResetDecoder())
        return dsOutOfMemory;

      return dsErrorFree;
    }
    //for AVC bitstream (excluding AVC with temporal scalability, including TP), as long as error occur, SHOULD notify upper layer key frame loss.
    if ((IS_PARAM_SETS_NALS (eNalType) || NAL_UNIT_CODED_SLICE_IDR == eNalType) ||
        (VIDEO_BITSTREAM_AVC == m_pDecContext->eVideoType)) {
      if (m_pDecContext->pParam->eEcActiveIdc == ERROR_CON_DISABLE) {
#ifdef LONG_TERM_REF
        m_pDecContext->bParamSetsLostFlag = true;
#else
        m_pDecContext->bReferenceLostAtT0Flag = true;
#endif
      }
    }

    if (m_pDecContext->bPrintFrameErrorTraceFlag) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "decode failed, failure type:%d \n",
               m_pDecContext->iErrorCode);
      m_pDecContext->bPrintFrameErrorTraceFlag = false;
    } else {
      m_pDecContext->iIgnoredErrorInfoPacketCount++;
      if (m_pDecContext->iIgnoredErrorInfoPacketCount == INT_MAX) {
        WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_WARNING, "continuous error reached INT_MAX! Restart as 0.");
        m_pDecContext->iIgnoredErrorInfoPacketCount = 0;
      }
    }
    if ((m_pDecContext->pParam->eEcActiveIdc != ERROR_CON_DISABLE) && (pDstInfo->iBufferStatus == 1)) {
      //TODO after dec status updated
      m_pDecContext->iErrorCode |= dsDataErrorConcealed;

      m_pDecContext->sDecoderStatistics.uiDecodedFrameCount++;
      if (m_pDecContext->sDecoderStatistics.uiDecodedFrameCount == 0) { //exceed max value of uint32_t
        ResetDecStatNums (&m_pDecContext->sDecoderStatistics);
        m_pDecContext->sDecoderStatistics.uiDecodedFrameCount++;
      }
      int32_t iMbConcealedNum = m_pDecContext->iMbEcedNum + m_pDecContext->iMbEcedPropNum;
      m_pDecContext->sDecoderStatistics.uiAvgEcRatio = m_pDecContext->iMbNum == 0 ?
          (m_pDecContext->sDecoderStatistics.uiAvgEcRatio * m_pDecContext->sDecoderStatistics.uiEcFrameNum) : ((
                m_pDecContext->sDecoderStatistics.uiAvgEcRatio * m_pDecContext->sDecoderStatistics.uiEcFrameNum) + ((
                      iMbConcealedNum * 100) / m_pDecContext->iMbNum));
      m_pDecContext->sDecoderStatistics.uiAvgEcPropRatio = m_pDecContext->iMbNum == 0 ?
          (m_pDecContext->sDecoderStatistics.uiAvgEcPropRatio * m_pDecContext->sDecoderStatistics.uiEcFrameNum) : ((
                m_pDecContext->sDecoderStatistics.uiAvgEcPropRatio * m_pDecContext->sDecoderStatistics.uiEcFrameNum) + ((
                      m_pDecContext->iMbEcedPropNum * 100) / m_pDecContext->iMbNum));
      m_pDecContext->sDecoderStatistics.uiEcFrameNum += (iMbConcealedNum == 0 ? 0 : 1);
      m_pDecContext->sDecoderStatistics.uiAvgEcRatio = m_pDecContext->sDecoderStatistics.uiEcFrameNum == 0 ? 0 :
          m_pDecContext->sDecoderStatistics.uiAvgEcRatio / m_pDecContext->sDecoderStatistics.uiEcFrameNum;
      m_pDecContext->sDecoderStatistics.uiAvgEcPropRatio = m_pDecContext->sDecoderStatistics.uiEcFrameNum == 0 ? 0 :
          m_pDecContext->sDecoderStatistics.uiAvgEcPropRatio / m_pDecContext->sDecoderStatistics.uiEcFrameNum;
    }
    iEnd = WelsTime();
    m_pDecContext->dDecTime += (iEnd - iStart) / 1e3;

    OutputStatisticsLog (m_pDecContext->sDecoderStatistics);

#ifdef  _PICTURE_REORDERING_
    ReorderPicturesInDisplay (ppDst, pDstInfo);
#endif

    return (DECODING_STATE)m_pDecContext->iErrorCode;
  }
  // else Error free, the current codec works well

  if (pDstInfo->iBufferStatus == 1) {

    m_pDecContext->sDecoderStatistics.uiDecodedFrameCount++;
    if (m_pDecContext->sDecoderStatistics.uiDecodedFrameCount == 0) { //exceed max value of uint32_t
      ResetDecStatNums (&m_pDecContext->sDecoderStatistics);
      m_pDecContext->sDecoderStatistics.uiDecodedFrameCount++;
    }

    OutputStatisticsLog (m_pDecContext->sDecoderStatistics);
  }
  iEnd = WelsTime();
  m_pDecContext->dDecTime += (iEnd - iStart) / 1e3;

#ifdef  _PICTURE_REORDERING_
  ReorderPicturesInDisplay (ppDst, pDstInfo);
#endif
  return dsErrorFree;
}

DECODING_STATE CWelsDecoder::FlushFrame (unsigned char** ppDst,
    SBufferInfo* pDstInfo) {
  if (m_pDecContext->bEndOfStreamFlag && m_iNumOfPicts > 0) {
    m_iMinPOC = sIMinInt32;
    for (int32_t i = 0; i <= m_iLargestBufferedPicIndex; ++i) {
      if (m_iMinPOC == sIMinInt32 && m_sPictInfoList[i].iPOC > sIMinInt32) {
        m_iMinPOC = m_sPictInfoList[i].iPOC;
        m_iPictInfoIndex = i;
      }
      if (m_sPictInfoList[i].iPOC > sIMinInt32 && m_sPictInfoList[i].iPOC < m_iMinPOC) {
        m_iMinPOC = m_sPictInfoList[i].iPOC;
        m_iPictInfoIndex = i;
      }
    }
  }
  if (m_iMinPOC > sIMinInt32) {
    m_LastWrittenPOC = m_iMinPOC;
#if defined (_DEBUG)
#ifdef _MOTION_VECTOR_DUMP_
    fprintf (stderr, "Output POC: #%d\n", m_LastWrittenPOC);
#endif
#endif
    memcpy (pDstInfo, &m_sPictInfoList[m_iPictInfoIndex].sBufferInfo, sizeof (SBufferInfo));
    ppDst[0] = m_sPictInfoList[m_iPictInfoIndex].pData[0];
    ppDst[1] = m_sPictInfoList[m_iPictInfoIndex].pData[1];
    ppDst[2] = m_sPictInfoList[m_iPictInfoIndex].pData[2];
    m_sPictInfoList[m_iPictInfoIndex].iPOC = sIMinInt32;
    m_sPictInfoList[m_iPictInfoIndex].bLastGOP = false;
    m_iMinPOC = sIMinInt32;
    --m_iNumOfPicts;
  }
  return dsErrorFree;
}

void CWelsDecoder::OutputStatisticsLog (SDecoderStatistics& sDecoderStatistics) {
  if ((sDecoderStatistics.uiDecodedFrameCount > 0) && (sDecoderStatistics.iStatisticsLogInterval > 0)
      && ((sDecoderStatistics.uiDecodedFrameCount % sDecoderStatistics.iStatisticsLogInterval) == 0)) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO,
             "DecoderStatistics: uiWidth=%d, uiHeight=%d, fAverageFrameSpeedInMs=%.1f, fActualAverageFrameSpeedInMs=%.1f, \
              uiDecodedFrameCount=%d, uiResolutionChangeTimes=%d, uiIDRCorrectNum=%d, \
              uiAvgEcRatio=%d, uiAvgEcPropRatio=%d, uiEcIDRNum=%d, uiEcFrameNum=%d, \
              uiIDRLostNum=%d, uiFreezingIDRNum=%d, uiFreezingNonIDRNum=%d, iAvgLumaQp=%d, \
              iSpsReportErrorNum=%d, iSubSpsReportErrorNum=%d, iPpsReportErrorNum=%d, iSpsNoExistNalNum=%d, iSubSpsNoExistNalNum=%d, iPpsNoExistNalNum=%d, \
              uiProfile=%d, uiLevel=%d, \
              iCurrentActiveSpsId=%d, iCurrentActivePpsId=%d,",
             sDecoderStatistics.uiWidth,
             sDecoderStatistics.uiHeight,
             sDecoderStatistics.fAverageFrameSpeedInMs,
             sDecoderStatistics.fActualAverageFrameSpeedInMs,

             sDecoderStatistics.uiDecodedFrameCount,
             sDecoderStatistics.uiResolutionChangeTimes,
             sDecoderStatistics.uiIDRCorrectNum,

             sDecoderStatistics.uiAvgEcRatio,
             sDecoderStatistics.uiAvgEcPropRatio,
             sDecoderStatistics.uiEcIDRNum,
             sDecoderStatistics.uiEcFrameNum,

             sDecoderStatistics.uiIDRLostNum,
             sDecoderStatistics.uiFreezingIDRNum,
             sDecoderStatistics.uiFreezingNonIDRNum,
             sDecoderStatistics.iAvgLumaQp,

             sDecoderStatistics.iSpsReportErrorNum,
             sDecoderStatistics.iSubSpsReportErrorNum,
             sDecoderStatistics.iPpsReportErrorNum,
             sDecoderStatistics.iSpsNoExistNalNum,
             sDecoderStatistics.iSubSpsNoExistNalNum,
             sDecoderStatistics.iPpsNoExistNalNum,

             sDecoderStatistics.uiProfile,
             sDecoderStatistics.uiLevel,

             sDecoderStatistics.iCurrentActiveSpsId,
             sDecoderStatistics.iCurrentActivePpsId);
  }
}

DECODING_STATE CWelsDecoder::ReorderPicturesInDisplay (unsigned char** ppDst, SBufferInfo* pDstInfo) {
  if (pDstInfo->iBufferStatus == 1 && m_pDecContext->pSps->uiProfileIdc != 66) {
    if (m_pDecContext->pSliceHeader->iPicOrderCntLsb == 0) {
      if (m_iNumOfPicts > 0) {
        m_iLastGOPRemainPicts = m_iNumOfPicts;
        for (int32_t i = 0; i <= m_iLargestBufferedPicIndex; ++i) {
          if (m_sPictInfoList[i].iPOC > sIMinInt32) {
            m_sPictInfoList[i].bLastGOP = true;
          }
        }
      }
    }
    for (int32_t i = 0; i < 16; ++i) {
      if (m_sPictInfoList[i].iPOC == sIMinInt32) {
        memcpy (&m_sPictInfoList[i].sBufferInfo, pDstInfo, sizeof (SBufferInfo));
        m_sPictInfoList[i].pData[0] = ppDst[0];
        m_sPictInfoList[i].pData[1] = ppDst[1];
        m_sPictInfoList[i].pData[2] = ppDst[2];
        m_sPictInfoList[i].iPOC = m_pDecContext->pSliceHeader->iPicOrderCntLsb;
        m_sPictInfoList[i].iFrameNum = m_pDecContext->pSliceHeader->iFrameNum;
        m_sPictInfoList[i].bLastGOP = false;
        pDstInfo->iBufferStatus = 0;
        ++m_iNumOfPicts;
        if (i > m_iLargestBufferedPicIndex) {
          m_iLargestBufferedPicIndex = i;
        }
        break;
      }
    }
    if (m_iLastGOPRemainPicts > 0) {
      m_iMinPOC = sIMinInt32;
      for (int32_t i = 0; i <= m_iLargestBufferedPicIndex; ++i) {
        if (m_iMinPOC == sIMinInt32 && m_sPictInfoList[i].iPOC > sIMinInt32 && m_sPictInfoList[i].bLastGOP) {
          m_iMinPOC = m_sPictInfoList[i].iPOC;
          m_iPictInfoIndex = i;
        }
        if (m_sPictInfoList[i].iPOC > sIMinInt32 && m_sPictInfoList[i].iPOC < m_iMinPOC && m_sPictInfoList[i].bLastGOP) {
          m_iMinPOC = m_sPictInfoList[i].iPOC;
          m_iPictInfoIndex = i;
        }
      }
      m_LastWrittenPOC = m_iMinPOC;
#if defined (_DEBUG)
#ifdef _MOTION_VECTOR_DUMP_
      fprintf (stderr, "Output POC: #%d\n", m_LastWrittenPOC);
#endif
#endif
      memcpy (pDstInfo, &m_sPictInfoList[m_iPictInfoIndex].sBufferInfo, sizeof (SBufferInfo));
      ppDst[0] = m_sPictInfoList[m_iPictInfoIndex].pData[0];
      ppDst[1] = m_sPictInfoList[m_iPictInfoIndex].pData[1];
      ppDst[2] = m_sPictInfoList[m_iPictInfoIndex].pData[2];
      m_sPictInfoList[m_iPictInfoIndex].iPOC = sIMinInt32;
      m_sPictInfoList[m_iPictInfoIndex].bLastGOP = false;
      m_iMinPOC = sIMinInt32;
      --m_iNumOfPicts;
      --m_iLastGOPRemainPicts;
      if (m_iLastGOPRemainPicts == 0) {
        m_LastWrittenPOC = sIMinInt32;
      }
      return dsErrorFree;
    }
    if (m_iNumOfPicts > 0) {
      m_iMinPOC = sIMinInt32;
      for (int32_t i = 0; i <= m_iLargestBufferedPicIndex; ++i) {
        if (m_iMinPOC == sIMinInt32 && m_sPictInfoList[i].iPOC > sIMinInt32) {
          m_iMinPOC = m_sPictInfoList[i].iPOC;
          m_iPictInfoIndex = i;
        }
        if (m_sPictInfoList[i].iPOC > sIMinInt32 && m_sPictInfoList[i].iPOC < m_iMinPOC) {
          m_iMinPOC = m_sPictInfoList[i].iPOC;
          m_iPictInfoIndex = i;
        }
      }
    }
    if (m_iMinPOC > sIMinInt32) {
      if ((m_LastWrittenPOC > sIMinInt32 && m_iMinPOC - m_LastWrittenPOC <= 1)
          || m_iMinPOC < m_pDecContext->pSliceHeader->iPicOrderCntLsb) {
        m_LastWrittenPOC = m_iMinPOC;
#if defined (_DEBUG)
#ifdef _MOTION_VECTOR_DUMP_
        fprintf (stderr, "Output POC: #%d\n", m_LastWrittenPOC);
#endif
#endif
        memcpy (pDstInfo, &m_sPictInfoList[m_iPictInfoIndex].sBufferInfo, sizeof (SBufferInfo));
        ppDst[0] = m_sPictInfoList[m_iPictInfoIndex].pData[0];
        ppDst[1] = m_sPictInfoList[m_iPictInfoIndex].pData[1];
        ppDst[2] = m_sPictInfoList[m_iPictInfoIndex].pData[2];
        m_sPictInfoList[m_iPictInfoIndex].iPOC = sIMinInt32;
        m_sPictInfoList[m_iPictInfoIndex].bLastGOP = false;
        m_iMinPOC = sIMinInt32;
        --m_iNumOfPicts;
        return dsErrorFree;
      }
    }
  }

  return dsErrorFree;
}

DECODING_STATE CWelsDecoder::DecodeParser (const unsigned char* kpSrc,
    const int kiSrcLen,
    SParserBsInfo* pDstInfo) {
  if (m_pDecContext == NULL || m_pDecContext->pParam == NULL) {
    if (m_pWelsTrace != NULL) {
      WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "Call DecodeParser without Initialize.\n");
    }
    return dsInitialOptExpected;
  }

  if (!m_pDecContext->pParam->bParseOnly) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_ERROR, "bParseOnly should be true for this API calling! \n");
    m_pDecContext->iErrorCode |= dsInvalidArgument;
    return dsInvalidArgument;
  }
  int64_t iEnd, iStart = WelsTime();
  if (CheckBsBuffer (m_pDecContext, kiSrcLen)) {
    if (ResetDecoder())
      return dsOutOfMemory;

    return dsErrorFree;
  }
  if (kiSrcLen > 0 && kpSrc != NULL) {
#ifdef OUTPUT_BITSTREAM
    if (m_pFBS) {
      WelsFwrite (kpSrc, sizeof (unsigned char), kiSrcLen, m_pFBS);
      WelsFflush (m_pFBS);
    }
#endif//OUTPUT_BIT_STREAM
    m_pDecContext->bEndOfStreamFlag = false;
  } else {
    //For application MODE, the error detection should be added for safe.
    //But for CONSOLE MODE, when decoding LAST AU, kiSrcLen==0 && kpSrc==NULL.
    m_pDecContext->bEndOfStreamFlag = true;
    m_pDecContext->bInstantDecFlag = true;
  }

  m_pDecContext->iErrorCode = dsErrorFree; //initialize at the starting of AU decoding.
  m_pDecContext->pParam->eEcActiveIdc = ERROR_CON_DISABLE; //add protection to disable EC here.
  m_pDecContext->iFeedbackNalRefIdc = -1; //initialize
  if (!m_pDecContext->bFramePending) { //frame complete
    m_pDecContext->pParserBsInfo->iNalNum = 0;
    memset (m_pDecContext->pParserBsInfo->pNalLenInByte, 0, MAX_NAL_UNITS_IN_LAYER);
  }
  pDstInfo->iNalNum = 0;
  pDstInfo->iSpsWidthInPixel = pDstInfo->iSpsHeightInPixel = 0;
  if (pDstInfo) {
    m_pDecContext->uiTimeStamp = pDstInfo->uiInBsTimeStamp;
    pDstInfo->uiOutBsTimeStamp = 0;
  } else {
    m_pDecContext->uiTimeStamp = 0;
  }
  WelsDecodeBs (m_pDecContext, kpSrc, kiSrcLen, NULL, NULL, pDstInfo);
  if (m_pDecContext->iErrorCode & dsOutOfMemory) {
    if (ResetDecoder())
      return dsOutOfMemory;
    return dsErrorFree;
  }

  if (!m_pDecContext->bFramePending && m_pDecContext->pParserBsInfo->iNalNum) {
    memcpy (pDstInfo, m_pDecContext->pParserBsInfo, sizeof (SParserBsInfo));

    if (m_pDecContext->iErrorCode == ERR_NONE) { //update statistics: decoding frame count
      m_pDecContext->sDecoderStatistics.uiDecodedFrameCount++;
      if (m_pDecContext->sDecoderStatistics.uiDecodedFrameCount == 0) { //exceed max value of uint32_t
        ResetDecStatNums (&m_pDecContext->sDecoderStatistics);
        m_pDecContext->sDecoderStatistics.uiDecodedFrameCount++;
      }
    }
  }

  m_pDecContext->bInstantDecFlag = false; //reset no-delay flag

  if (m_pDecContext->iErrorCode && m_pDecContext->bPrintFrameErrorTraceFlag) {
    WelsLog (&m_pWelsTrace->m_sLogCtx, WELS_LOG_INFO, "decode failed, failure type:%d \n", m_pDecContext->iErrorCode);
    m_pDecContext->bPrintFrameErrorTraceFlag = false;
  }
  iEnd = WelsTime();
  m_pDecContext->dDecTime += (iEnd - iStart) / 1e3;

  return (DECODING_STATE) m_pDecContext->iErrorCode;
}

DECODING_STATE CWelsDecoder::DecodeFrame (const unsigned char* kpSrc,
    const int kiSrcLen,
    unsigned char** ppDst,
    int* pStride,
    int& iWidth,
    int& iHeight) {
  DECODING_STATE eDecState = dsErrorFree;
  SBufferInfo    DstInfo;

  memset (&DstInfo, 0, sizeof (SBufferInfo));
  DstInfo.UsrData.sSystemBuffer.iStride[0] = pStride[0];
  DstInfo.UsrData.sSystemBuffer.iStride[1] = pStride[1];
  DstInfo.UsrData.sSystemBuffer.iWidth = iWidth;
  DstInfo.UsrData.sSystemBuffer.iHeight = iHeight;

  eDecState = DecodeFrame2 (kpSrc, kiSrcLen, ppDst, &DstInfo);
  if (eDecState == dsErrorFree) {
    pStride[0] = DstInfo.UsrData.sSystemBuffer.iStride[0];
    pStride[1] = DstInfo.UsrData.sSystemBuffer.iStride[1];
    iWidth     = DstInfo.UsrData.sSystemBuffer.iWidth;
    iHeight    = DstInfo.UsrData.sSystemBuffer.iHeight;
  }

  return eDecState;
}

DECODING_STATE CWelsDecoder::DecodeFrameEx (const unsigned char* kpSrc,
    const int kiSrcLen,
    unsigned char* pDst,
    int iDstStride,
    int& iDstLen,
    int& iWidth,
    int& iHeight,
    int& iColorFormat) {
  DECODING_STATE state = dsErrorFree;

  return state;
}


} // namespace WelsDec


using namespace WelsDec;
/*
*       WelsGetDecoderCapability
*       @return: DecCapability information
*/
int WelsGetDecoderCapability (SDecoderCapability* pDecCapability) {
  memset (pDecCapability, 0, sizeof (SDecoderCapability));
  pDecCapability->iProfileIdc = 66; //Baseline
  pDecCapability->iProfileIop = 0xE0; //11100000b
  pDecCapability->iLevelIdc = 32; //level_idc = 3.2
  pDecCapability->iMaxMbps = 216000; //from level_idc = 3.2
  pDecCapability->iMaxFs = 5120; //from level_idc = 3.2
  pDecCapability->iMaxCpb = 20000; //from level_idc = 3.2
  pDecCapability->iMaxDpb = 20480; //from level_idc = 3.2
  pDecCapability->iMaxBr = 20000; //from level_idc = 3.2
  pDecCapability->bRedPicCap = 0; //not support redundant pic

  return ERR_NONE;
}
/* WINAPI is indeed in prefix due to sync to application layer callings!! */

/*
*   WelsCreateDecoder
*   @return:    success in return 0, otherwise failed.
*/
long WelsCreateDecoder (ISVCDecoder** ppDecoder) {

  if (NULL == ppDecoder) {
    return ERR_INVALID_PARAMETERS;
  }

  *ppDecoder = new CWelsDecoder();

  if (NULL == *ppDecoder) {
    return ERR_MALLOC_FAILED;
  }

  return ERR_NONE;
}

/*
*   WelsDestroyDecoder
*/
void WelsDestroyDecoder (ISVCDecoder* pDecoder) {
  if (NULL != pDecoder) {
    delete (CWelsDecoder*)pDecoder;
  }
}
