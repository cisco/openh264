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
 * \file	utils.c
 *
 * \brief	common tool/function utilization
 *
 * \date	03/10/2009 Created
 *
 *************************************************************************************
 */
#if defined(_WIN32)
#include <windows.h>
#include <sys/types.h>
#include <sys/timeb.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#else
#include <sys/time.h>
#endif

#include "utils.h"
#include "property.h"
#include "encoder_context.h"
#include "property.h"
#include "crt_util_safe_x.h"	// Safe CRT routines like utils for cross platforms


namespace WelsSVCEnc {

void WelsLogDefault (void* pCtx, const int32_t kiLevel, const char* kpFmtStr, va_list argv);
void WelsLogNil (void* pCtx, const int32_t kiLevel, const char* kpFmtStr, va_list argv);

float WelsCalcPsnr (const void* kpTarPic,
                    const int32_t kiTarStride,
                    const void* kpRefPic,
                    const int32_t kiRefStride,
                    const int32_t kiWidth,
                    const int32_t kiHeight);

// to fill default routines
#ifdef ENABLE_TRACE_FILE
PWelsLogCallbackFunc wlog	= WelsLogDefault;
#else
PWelsLogCallbackFunc wlog	= WelsLogNil;
#endif

iWelsLogLevel		g_iLevelLog	= WELS_LOG_DEFAULT;	// default log iLevel
int32_t			g_iSizeLogBuf	= 1024;			// pBuffer size for each log output

/*
 *	Log output routines
 */

/*!
 * \brief	get log tag
 * \param	kiLevel		log iLevel
 * \return  tag of log iLevel
 */
static inline char* GetLogTag (const int32_t kiLevel, int32_t* pBit) {
  int32_t iShift	= 0;
  int32_t iVal		= 0;
  bool	bFound	= false;

  if (kiLevel <= 0 || kiLevel > (1 << (WELS_LOG_LEVEL_COUNT - 1)) || NULL == pBit)
    return NULL;

  for (;;) {
    if (iShift >= WELS_LOG_LEVEL_COUNT)
      break;
    iVal	= (1 << iShift);
    if (iVal == kiLevel) {
      bFound	= true;
      break;
    }
    ++ iShift;
  }

  if (bFound) {
    *pBit	= iShift;
    return (char*)g_sWelsLogTags[iShift];
  }
  return NULL;
}

/*!
 *************************************************************************************
 * \brief	System trace log output in Wels
 *
 * \param	pCtx	instance pointer
 * \param	kiLevel	log iLevel ( WELS_LOG_QUIET, ERROR, WARNING, INFO, DEBUG )
 * \param	kpFmtStr	formated string to mount
 * \param 	argv	pData string argument
 *
 * \return	NONE
 *
 * \note	N/A
 *************************************************************************************
 */
void WelsLogDefault (void* pCtx, const int32_t kiLevel, const char* kpFmtStr, va_list argv) {
  sWelsEncCtx* pEncCtx	= (sWelsEncCtx*)pCtx;
  iWelsLogLevel		 iVal	= (kiLevel & g_iLevelLog);

  if (0 == iVal || NULL == pEncCtx) {	// such iLevel not enabled
    return;
  } else {
    char pBuf[WELS_LOG_BUF_SIZE + 1] = {0};
    const int32_t kiBufSize = sizeof (pBuf) / sizeof (pBuf[0]) - 1;
    int32_t iCurUsed = 0;
    int32_t iBufUsed = 0;
    int32_t iBufLeft = kiBufSize - iBufUsed;

    if (pEncCtx) {
      SWelsTime tTime;

      WelsGetTimeOfDay(&tTime);
      iCurUsed = WelsSnprintf (&pBuf[iBufUsed], iBufLeft, "[0x%p @ ", pEncCtx);	// confirmed_safe_unsafe_usage
      if (iCurUsed >= 0) {
        iBufUsed += iCurUsed;
        iBufLeft -= iCurUsed;
      }

      if (iBufLeft > 0) {
        iCurUsed = GetCodeName (&pBuf[iBufUsed], iBufLeft);
        if (iCurUsed > 0) {
          iBufUsed += iCurUsed;
          iBufLeft -= iCurUsed;
        }
        pBuf[iBufUsed] = ' ';
        ++ iBufUsed;
        -- iBufLeft;

        iCurUsed = GetLibName (&pBuf[iBufUsed], iBufLeft);
        if (iCurUsed > 0) {
          iBufUsed += iCurUsed;
          iBufLeft -= iCurUsed;
        }
        pBuf[iBufUsed] = ' ';
        ++ iBufUsed;
        -- iBufLeft;

        pBuf[iBufUsed] = 'v';
        ++ iBufUsed;
        -- iBufLeft;
        iCurUsed = GetVerNum (&pBuf[iBufUsed], iBufLeft);
        if (iCurUsed > 0) {
          iBufUsed += iCurUsed;
          iBufLeft -= iCurUsed;
        }
        pBuf[iBufUsed] = ' ';
        ++ iBufUsed;
        -- iBufLeft;
      }

      if (iBufLeft > 0) {
        iCurUsed = WelsStrftime (&pBuf[iBufUsed], iBufLeft, "%y-%m-%d %H:%M:%S", &tTime);
        if (iCurUsed > 0) {
          iBufUsed += iCurUsed;
          iBufLeft -= iCurUsed;
        }
      } else {
        return;
      }

      if (iBufLeft > 0) {
        iCurUsed = WelsSnprintf (&pBuf[iBufUsed], iBufLeft, ".%3.3u]: ", tTime.millitm);	// confirmed_safe_unsafe_usage
        if (iCurUsed >= 0) {
          iBufUsed += iCurUsed;
          iBufLeft -= iCurUsed;
        }
      } else {
        return;
      }
    }

    // fixed stack corruption issue on vs2008
    if (iBufLeft > 0) {
      int32_t i_shift = 0;
      char* pStr = NULL;
      pStr	= GetLogTag (kiLevel, &i_shift);
      if (NULL != pStr) {
        iCurUsed = WelsSnprintf (&pBuf[iBufUsed], iBufLeft, "%s ", pStr);
        if (iCurUsed >= 0) {
          iBufUsed += iCurUsed;
          iBufLeft -= iCurUsed;
        }
      }
    }
    if (iBufLeft > 0) {
      iCurUsed = WelsVsnprintf (&pBuf[iBufUsed], iBufLeft, kpFmtStr, argv);	// confirmed_safe_unsafe_usage
      if (iCurUsed > 0) {
        iBufUsed += iCurUsed;
        iBufLeft -= iCurUsed;
      }
    }
#ifdef ENABLE_TRACE_FILE
    if (NULL != pEncCtx && NULL != pEncCtx->pFileLog) {
      if (pEncCtx->uiSizeLog > MAX_TRACE_LOG_SIZE) {
        if (0 == WelsFseek (pEncCtx->pFileLog, 0L, SEEK_SET))
          pEncCtx->uiSizeLog = 0;
      }
      if (iBufUsed > 0 && iBufUsed < WELS_LOG_BUF_SIZE) {
        iCurUsed = WelsFwrite (pBuf, 1, iBufUsed, pEncCtx->pFileLog);
        WelsFflush (pEncCtx->pFileLog);
        if (iCurUsed == iBufUsed)
          pEncCtx->uiSizeLog += iBufUsed;
      }
    } else {
#if defined(_WIN32) && defined(_DEBUG)
      OutputDebugStringA (pBuf);
#endif
    }
#endif//ENABLE_TRACE_FILE
  }
}
void WelsLogNil (void* pCtx, const int32_t kiLevel, const char* kpFmtStr, va_list argv) {
  // NULL implementation
}

/*!
*************************************************************************************
* \brief	reopen log file when finish setting current path
*
* \param	pCtx		context pCtx
* \param	pCurPath	current path string
*
* \return	NONE
*
* \note	N/A
*************************************************************************************
*/
void WelsReopenTraceFile (void* pCtx, char* pCurPath) {
#ifdef ENABLE_TRACE_FILE
  sWelsEncCtx* pEncCtx	= (sWelsEncCtx*)pCtx;
  if (wlog == WelsLogDefault) {
    if (pEncCtx->pFileLog != NULL) {
      WelsFclose (pEncCtx->pFileLog);
      pEncCtx->pFileLog = NULL;
    }
    pEncCtx->uiSizeLog	= 0;
    pEncCtx->pFileLog	= WelsFopen ("wels_encoder_trace.txt", "wt+");	// confirmed_safe_unsafe_usage
  }
#endif//ENABLE_TRACE_FILE
}

/*!
 *************************************************************************************
 * \brief	set log iLevel from external call
 *
 * \param	iLevel	iLevel of log
 *
 * \return	NONE
 *
 * \note	can be able to control log iLevel dynamically
 *************************************************************************************
 */
void WelsSetLogLevel (const int32_t kiLevel) {
  iWelsLogLevel iVal = 0;
  if (kiLevel & WELS_LOG_ERROR) {
    iVal |= WELS_LOG_ERROR;
  }
  if (kiLevel & WELS_LOG_WARNING) {
    iVal |= WELS_LOG_WARNING;
  }
  if (kiLevel & WELS_LOG_INFO) {
    iVal |= WELS_LOG_INFO;
  }
  if (kiLevel & WELS_LOG_DEBUG) {
    iVal |= WELS_LOG_DEBUG;
  }
  g_iLevelLog	= iVal;
}

/*!
 *************************************************************************************
 * \brief	get log iLevel from external call
 *
 * \param	N/A
 *
 * \return	current iLevel of log used in codec internal
 *
 * \note	can be able to get log iLevel of internal codec applicable
 *************************************************************************************
 */
int32_t WelsGetLogLevel (void) {
  return g_iLevelLog;
}

/*!
 *************************************************************************************
 * \brief	set log callback from external call
 *
 * \param	_log	log function routine
 *
 * \return	NONE
 *
 * \note	N/A
 *************************************************************************************
 */
void WelsSetLogCallback (PWelsLogCallbackFunc _log) {
  wlog	= _log;
}

void WelsLogCall (void* pCtx, int32_t iLevel, const char* kpFmt, va_list vl) {
  wlog (pCtx, iLevel, kpFmt, vl);
}

void WelsLog (void* pCtx, int32_t iLevel, const char* kpFmt, ...) {
  va_list vl;
  va_start (vl, kpFmt);
  WelsLogCall (pCtx, iLevel, kpFmt, vl);
  va_end (vl);
}

#ifndef CALC_PSNR
#define CONST_FACTOR_PSNR	(10.0 / log(10.0))	// for good computation
#define CALC_PSNR(w, h, s)	((float)(CONST_FACTOR_PSNR * log( 65025.0 * w * h / iSqe )))
#endif//CALC_PSNR

/*
 *	PSNR calculation routines
 */
/*!
 *************************************************************************************
 * \brief	PSNR calculation utilization in Wels
 *
 * \param	pTarPic		target picture to be calculated in Picture pData format
 * \param	iTarStride	stride of target picture pData pBuffer
 * \param 	pRefPic		base referencing picture samples
 * \param	iRefStride	stride of reference picture pData pBuffer
 * \param	iWidth		picture iWidth in pixel
 * \param	iHeight		picture iHeight in pixel
 *
 * \return	actual PSNR result;
 *
 * \note	N/A
 *************************************************************************************
 */
float WelsCalcPsnr (const void* kpTarPic,
                    const int32_t kiTarStride,
                    const void* kpRefPic,
                    const int32_t kiRefStride,
                    const int32_t kiWidth,
                    const int32_t kiHeight) {
  int64_t	iSqe = 0;
  int32_t x, y;
  uint8_t* pTar = (uint8_t*)kpTarPic;
  uint8_t* pRef = (uint8_t*)kpRefPic;

  if (NULL == pTar || NULL == pRef)
    return (-1.0f);

  for (y = 0; y < kiHeight; ++ y) {	// OPTable !!
    for (x = 0; x < kiWidth; ++ x) {
      const int32_t kiT = pTar[y * kiTarStride + x] - pRef[y * kiRefStride + x];
      iSqe	+= kiT * kiT;
    }
  }
  if (0 == iSqe) {
    return (99.99f);
  }
  return CALC_PSNR (kiWidth, kiHeight, iSqe);
}


}
