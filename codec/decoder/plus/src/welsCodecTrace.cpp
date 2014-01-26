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

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "crt_util_safe_x.h"	// Safe CRT routines like utils for cross platforms
#include "utils.h"

#include "welsCodecTrace.h"
#include "utils.h"
#include "logging.h"

//using namespace WelsDec;

namespace WelsDec {


int32_t  CWelsTraceBase::SetTraceLevel (int iLevel) {
  m_iLevel = iLevel;

  return 0;
}

int32_t  CWelsTraceBase::Trace (const int kLevel, const str_t* kpFormat, va_list pVl) {
  if (kLevel & m_iLevel) {
    str_t chWStrFormat[MAX_LOG_SIZE] = {0};
    str_t chBuf[MAX_LOG_SIZE] = {0};
    str_t chResult[MAX_LOG_SIZE] = {0};
    const int32_t kLen	= WelsStrnlen ((const str_t*)"[DECODER]: ", MAX_LOG_SIZE);

    WelsStrncpy (chWStrFormat, MAX_LOG_SIZE, (const str_t*)kpFormat, WelsStrnlen ((const str_t*)kpFormat, MAX_LOG_SIZE));

    WelsStrncpy (chBuf, MAX_LOG_SIZE, (const str_t*)"[DECODER]: ", kLen);

    WelsVsnprintf ((chBuf + kLen),  MAX_LOG_SIZE - kLen, (const str_t*)kpFormat, pVl);
    WelsStrncpy (chResult, MAX_LOG_SIZE, (const str_t*)chBuf, WelsStrnlen ((const str_t*)chBuf, MAX_LOG_SIZE));

    WriteString (kLevel, chResult);
  }

  return 0;
}

CWelsTraceFile::CWelsTraceFile (const str_t* pFileName) {
  m_pTraceFile = WelsFopen (pFileName, (const str_t*)"wt");
}

CWelsTraceFile::~CWelsTraceFile() {
  if (m_pTraceFile) {
    WelsFclose (m_pTraceFile);
    m_pTraceFile = NULL;
  }
}

int32_t CWelsTraceFile::WriteString (int32_t iLevel, const str_t* pStr) {
  int  iRC = 0;
  const static str_t chEnter[16] = "\n";
  if (m_pTraceFile) {
    iRC += WelsFwrite (pStr, 1, WelsStrnlen (pStr, MAX_LOG_SIZE), m_pTraceFile);
    iRC += WelsFwrite (chEnter, 1, WelsStrnlen (chEnter,  16), m_pTraceFile);
    WelsFflush (m_pTraceFile);
  }
  return iRC;
}


#ifdef _WIN32

int32_t CWelsTraceWinDgb::WriteString (int32_t iLevel, const str_t* pStr) {
  OutputDebugStringA (pStr);

  return WelsStrnlen (pStr, MAX_LOG_SIZE); //strnlen(pStr, MAX_LOG_SIZE);
}

#endif

CWelsCodecTrace::CWelsCodecTrace() {
  m_fpDebugTrace = NULL;
  m_fpInfoTrace = NULL;
  m_fpWarnTrace = NULL;
  m_fpErrorTrace = NULL;

  LoadWelsTraceModule();
}

CWelsCodecTrace::~CWelsCodecTrace() {
  UnloadWelsTraceModule();
}

int32_t  CWelsCodecTrace::LoadWelsTraceModule() {
  m_fpDebugTrace = welsStderrTrace<WELS_LOG_DEBUG>;
  m_fpInfoTrace = welsStderrTrace<WELS_LOG_INFO>;
  m_fpWarnTrace = welsStderrTrace<WELS_LOG_WARNING>;
  m_fpErrorTrace = welsStderrTrace<WELS_LOG_ERROR>;
  return 0;
}

int32_t  CWelsCodecTrace::UnloadWelsTraceModule() {
  m_fpDebugTrace = NULL;
  m_fpInfoTrace = NULL;
  m_fpWarnTrace = NULL;
  m_fpErrorTrace = NULL;
  return 0;
}

int32_t  CWelsCodecTrace::WriteString (int32_t iLevel, const str_t* pStr) {
  {
    switch (iLevel) {
    case WELS_LOG_ERROR:
      if (m_fpErrorTrace)
        m_fpErrorTrace ("%s", pStr);
      break;
    case WELS_LOG_WARNING:
      if (m_fpWarnTrace)
        m_fpWarnTrace ("%s", pStr);
      break;
    case WELS_LOG_INFO:
      if (m_fpInfoTrace)
        m_fpInfoTrace ("%s", pStr);
      break;
    case WELS_LOG_DEBUG:
      if (m_fpDebugTrace)
        m_fpDebugTrace ("%s", pStr);
      break;
    default:
      if (m_fpDebugTrace)
        m_fpDebugTrace ("%s", pStr);
      break;
    }
  }

  return 0;
}


IWelsTrace*   CreateWelsTrace (EWelsTraceType  eType,  void_t* pParam) {
  IWelsTrace*   pTrace = NULL;
  switch (eType) {
  case Wels_Trace_Type:
    pTrace = new CWelsCodecTrace();
    break;
  case Wels_Trace_Type_File:
    pTrace = new CWelsTraceFile();
    break;
#ifdef _WIN32
  case Wels_Trace_Type_WinDgb:
    pTrace = new CWelsTraceWinDgb();
    break;
#endif
  default:
    break;
  }

  return pTrace;
}

} // namespace WelsDec
