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

#include "welsCodecTrace.h"
#include "utils.h"

#include "logging.h"

int32_t	welsCodecTrace::m_iTraceLevel			= WELS_LOG_DEFAULT;
CM_WELS_TRACE welsCodecTrace::m_fpDebugTrace	= NULL;
CM_WELS_TRACE welsCodecTrace::m_fpInfoTrace	= NULL;
CM_WELS_TRACE welsCodecTrace::m_fpWarnTrace	= NULL;
CM_WELS_TRACE welsCodecTrace::m_fpErrorTrace	= NULL;

welsCodecTrace::welsCodecTrace() {

  m_fpDebugTrace = welsStderrTrace<WELS_LOG_DEBUG>;
  m_fpInfoTrace = welsStderrTrace<WELS_LOG_INFO>;
  m_fpWarnTrace = welsStderrTrace<WELS_LOG_WARNING>;
  m_fpErrorTrace = welsStderrTrace<WELS_LOG_ERROR>;
}

welsCodecTrace::~welsCodecTrace() {
  m_fpDebugTrace = NULL;
  m_fpInfoTrace = NULL;
  m_fpWarnTrace = NULL;
  m_fpErrorTrace = NULL;
}

void welsCodecTrace::TraceString (int32_t iLevel, const char* str) {
  switch (iLevel) {
  case WELS_LOG_ERROR:
    if (m_fpErrorTrace)
      m_fpErrorTrace ("%s", str);
    break;
  case WELS_LOG_WARNING:
    if (m_fpWarnTrace)
      m_fpWarnTrace ("%s", str);
    break;
  case WELS_LOG_INFO:
    if (m_fpInfoTrace)
      m_fpInfoTrace ("%s", str);
    break;
  case WELS_LOG_DEBUG:
    if (m_fpDebugTrace)
      m_fpDebugTrace ("%s", str);
    break;
  default:
    if (m_fpDebugTrace)
      m_fpInfoTrace ("%s", str);
    break;
  }
}

#define MAX_LOG_SIZE	1024

void welsCodecTrace::CODEC_TRACE (void* ignore, const int32_t iLevel, const char* Str_Format, va_list vl) {
  if (m_iTraceLevel < iLevel) {
    return;
  }

  char pBuf[MAX_LOG_SIZE] = {0};
  WelsVsnprintf (pBuf, MAX_LOG_SIZE, Str_Format, vl);	// confirmed_safe_unsafe_usage

  welsCodecTrace::TraceString (iLevel, pBuf);
}

void welsCodecTrace::SetTraceLevel (const int32_t iLevel) {
  if (iLevel >= 0)
    m_iTraceLevel	= iLevel;
  WelsStderrSetTraceLevel (iLevel);
}


