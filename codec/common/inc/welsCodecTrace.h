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

#ifndef WELS_CODEC_TRACE
#define WELS_CODEC_TRACE

#include <stdarg.h>
#include "typedefs.h"

typedef int32_t (*CM_WELS_TRACE) (const char* string);

class welsCodecTrace {
 public:
  welsCodecTrace();
  ~welsCodecTrace();

  static void TraceString (int32_t iLevel, const char* kpStrFormat);
  static void CODEC_TRACE (void* pIgnore, const int32_t kiLevel, const char* kpStrFormat, va_list vl);

  void SetTraceLevel (const int32_t kiLevel);

 public:
  static int32_t	m_iTraceLevel;
  static CM_WELS_TRACE m_fpDebugTrace;
  static CM_WELS_TRACE m_fpInfoTrace;
  static CM_WELS_TRACE m_fpWarnTrace;
  static CM_WELS_TRACE m_fpErrorTrace;

};

#endif //WELS_CODEC_TRACE
