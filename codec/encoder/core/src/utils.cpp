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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#if defined(WIN32)
#include <windows.h>
#include <sys/types.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

#include "utils.h"
#include "macros.h"
#include "wels_const.h"
#include "property.h"
#include "cpu_core.h"
#include "encoder_context.h"
#include "as264_common.h"
#include "property.h"
#include "crt_util_safe_x.h"	// Safe CRT routines like utils for cross platforms


namespace WelsSVCEnc {

void WelsLogDefault (void* pCtx, const int32_t kiLevel, const str_t* kpFmtStr, va_list argv);
void WelsLogNil (void* pCtx, const int32_t kiLevel, const str_t* kpFmtStr, va_list argv);

real32_t WelsCalcPsnr (const void* kpTarPic,
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
static inline str_t* GetLogTag (const int32_t kiLevel, int32_t* pBit) {
  int32_t iShift	= 0;
  int32_t iVal		= 0;
  bool_t	bFound	= false;

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
    return (str_t*)g_sWelsLogTags[iShift];
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
void WelsLogDefault (void* pCtx, const int32_t kiLevel, const str_t* kpFmtStr, va_list argv) {
  sWelsEncCtx* pEncCtx	= (sWelsEncCtx*)pCtx;
  iWelsLogLevel		 iVal	= (kiLevel & g_iLevelLog);

  if (0 == iVal || NULL == pEncCtx) {	// such iLevel not enabled
    return;
  } else {
    str_t pBuf[WELS_LOG_BUF_SIZE + 1] = {0};
    const int32_t kiBufSize = sizeof (pBuf) / sizeof (pBuf[0]) - 1;
    int32_t iCurUsed = 0;
    int32_t iBufUsed = 0;
    int32_t iBufLeft = kiBufSize - iBufUsed;

    if (pEncCtx) {
      time_t l_time;
#if defined(WIN32)
#if defined(_MSC_VER)
#if _MSC_VER >= 1500
      struct tm t_now;
#else//VC6
      struct tm* t_now;
#endif//_MSC_VER >= 1500
#endif//_MSC_VER
#else//__GNUC__
      struct tm* t_now;
#endif//WIN32

#if defined( WIN32 )
      struct _timeb tb;

      time (&l_time);
#ifdef _MSC_VER
#if _MSC_VER >= 1500
      LOCALTIME (&t_now, &l_time);
#else
      t_now = LOCALTIME (&l_time);
      if (NULL == t_now) {
        return;
      }
#endif//_MSC_VER >= 1500
#endif//_MSC_VER
      FTIME (&tb);
#elif defined( __GNUC__ )
      struct timeval tv;
      time (&l_time);
      t_now = (struct tm*)LOCALTIME (&l_time);
      gettimeofday (&tv, NULL);
#endif//WIN32
      if (iBufLeft > 0) {
#ifdef _MSC_VER
#if _MSC_VER >= 1500
        iCurUsed = SNPRINTF (&pBuf[iBufUsed], iBufLeft, iBufLeft, "[0x%p @ ", pEncCtx);	// confirmed_safe_unsafe_usage
#else
        iCurUsed = SNPRINTF (&pBuf[iBufUsed], iBufLeft, "[0x%p @ ", pEncCtx);	// confirmed_safe_unsafe_usage
#endif//_MSC_VER >= 1500
#endif//_MSC_VER
        if (iCurUsed >= 0) {
          iBufUsed += iCurUsed;
          iBufLeft -= iCurUsed;
        }
      } else {
        return;
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
#if defined(WIN32) && defined(_MSC_VER) && (_MSC_VER >= 1500)
        iCurUsed = strftime (&pBuf[iBufUsed], iBufLeft, "%y-%m-%d %H:%M:%S", &t_now);
#else
        iCurUsed = strftime (&pBuf[iBufUsed], iBufLeft, "%y-%m-%d %H:%M:%S", t_now);
#endif//WIN32..
        if (iCurUsed > 0) {
          iBufUsed += iCurUsed;
          iBufLeft -= iCurUsed;
        }
      } else {
        return;
      }

      if (iBufLeft > 0) {
#if defined (WIN32)
#ifdef _MSC_VER
#if _MSC_VER >= 1500
        iCurUsed = SNPRINTF (&pBuf[iBufUsed], iBufLeft, iBufLeft, ".%03.3u]: ", tb.millitm);	// confirmed_safe_unsafe_usage
#else
        iCurUsed = SNPRINTF (&pBuf[iBufUsed], iBufLeft, ".%3.3u]: ", tb.millitm);	// confirmed_safe_unsafe_usage
#endif//_MSC_VER >= 1500
#endif//_MSC_VER
#elif defined (__GNUC__)
        iCurUsed = SNPRINTF (&pBuf[iBufUsed], iBufLeft, ".%3.3u]: ", (unsigned int) (tv.tv_usec / 1000));	// confirmed_safe_unsafe_usage
#endif//WIN32
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
      str_t* pStr = NULL;
      pStr	= GetLogTag (kiLevel, &i_shift);
      if (NULL != pCtx) {
        int32_t iLenTag = STRNLEN (pStr, 8);	// confirmed_safe_unsafe_usage
        STRCAT (&pBuf[iBufUsed], iBufLeft, pStr);	// confirmed_safe_unsafe_usage
        iBufUsed += iLenTag;
        pBuf[iBufUsed] = ' ';
        iBufUsed++;
        ++iLenTag;
        iBufLeft -= iLenTag;
      }
    }
    if (iBufLeft > 0) {
#if defined(WIN32) && defined(_MSC_VER) && (_MSC_VER >= 1500)
      int32_t len = 0;
      len = _vscprintf (kpFmtStr, argv)  // _vscprintf doesn't count
            + 1; // terminating '\0'
      iCurUsed = VSPRINTF (&pBuf[iBufUsed], len, kpFmtStr, argv);	// confirmed_safe_unsafe_usage
#else
      iCurUsed = VSPRINTF (&pBuf[iBufUsed], kpFmtStr, argv);	// confirmed_safe_unsafe_usage
#endif//WIN32..
      if (iCurUsed > 0) {
        iBufUsed += iCurUsed;
        iBufLeft -= iCurUsed;
      }
    }
#ifdef ENABLE_TRACE_FILE
    if (NULL != pEncCtx && NULL != pEncCtx->pFileLog) {
      if (pEncCtx->uiSizeLog > MAX_TRACE_LOG_SIZE) {
        if (0 == fseek (pEncCtx->pFileLog, 0L, SEEK_SET))
          pEncCtx->uiSizeLog = 0;
      }
      if (iBufUsed > 0 && iBufUsed < WELS_LOG_BUF_SIZE) {
        iCurUsed = fwrite (pBuf, 1, iBufUsed, pEncCtx->pFileLog);
        fflush (pEncCtx->pFileLog);
        if (iCurUsed == iBufUsed)
          pEncCtx->uiSizeLog += iBufUsed;
      }
    } else {
#if defined(WIN32) && defined(_DEBUG)
      OutputDebugStringA (pBuf);
#endif
    }
#endif//ENABLE_TRACE_FILE
  }
}
void WelsLogNil (void* pCtx, const int32_t kiLevel, const str_t* kpFmtStr, va_list argv) {
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
void WelsReopenTraceFile (void* pCtx, str_t* pCurPath) {
#ifdef ENABLE_TRACE_FILE
  sWelsEncCtx* pEncCtx	= (sWelsEncCtx*)pCtx;
  if (wlog == WelsLogDefault) {
    str_t strTraceFile[MAX_FNAME_LEN] = {0};
    int32_t len = 0;
    if (pEncCtx->pFileLog != NULL) {
      fclose (pEncCtx->pFileLog);
      pEncCtx->pFileLog = NULL;
    }
    pEncCtx->uiSizeLog	= 0;
    len = STRNLEN (pCurPath, MAX_FNAME_LEN - 1);	// confirmed_safe_unsafe_usage
    if (len >= MAX_FNAME_LEN)
      return;
    STRNCPY (strTraceFile, MAX_FNAME_LEN, pCurPath, len);	// confirmed_safe_unsafe_usage
#ifdef __GNUC__
    STRCAT (strTraceFile, MAX_FNAME_LEN - len, "/wels_encoder_trace.txt");	// confirmed_safe_unsafe_usage
    pEncCtx->pFileLog	= FOPEN (strTraceFile, "wt+");	// confirmed_safe_unsafe_usage
#elif WIN32
    STRCAT (strTraceFile, MAX_FNAME_LEN - len, "\\wels_encoder_trace.txt"); // confirmed_safe_unsafe_usage
#if _MSC_VER >= 1500
    FOPEN (&pEncCtx->pFileLog, strTraceFile, "wt+");	// confirmed_safe_unsafe_usage
#else
    pEncCtx->pFileLog	= FOPEN (strTraceFile, "wt+");	// confirmed_safe_unsafe_usage
#endif//_MSC_VER>=1500
#else
#endif//__GNUC__
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

void WelsLogCall (void* pCtx, int32_t iLevel, const str_t* kpFmt, va_list vl) {
  wlog (pCtx, iLevel, kpFmt, vl);
}

void WelsLog (void* pCtx, int32_t iLevel, const str_t* kpFmt, ...) {
  va_list vl;
  va_start (vl, kpFmt);
  WelsLogCall (pCtx, iLevel, kpFmt, vl);
  va_end (vl);
}

#ifndef CALC_PSNR
#define CONST_FACTOR_PSNR	(10.0 / log(10.0))	// for good computation
#define CALC_PSNR(w, h, s)	((real32_t)(CONST_FACTOR_PSNR * log( 65025.0 * w * h / iSqe )))
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
real32_t WelsCalcPsnr (const void* kpTarPic,
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
