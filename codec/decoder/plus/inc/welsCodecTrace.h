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


#include "typedefs.h"

//using namespace WelsDec;
namespace WelsDec {

#ifdef _WIN32
typedef int (*CM_WELS_TRACE) (const char* kpFormat, ...);
#else
typedef int (*CM_WELS_TRACE) (const char* kpDllName, const char* kpFormat, ...);
#endif


typedef  enum {
Wels_Trace_Type     = 0,
Wels_Trace_Type_File    = 1,
Wels_Trace_Type_WinDgb  = 2,
} EWelsTraceType;

class  IWelsTrace {
 public:
enum {
  WELS_LOG_QUIET     = 0,
  WELS_LOG_ERROR     = 1 << 0,
  WELS_LOG_WARNING   = 1 << 1,
  WELS_LOG_INFO      = 1 << 2,
  WELS_LOG_DEBUG     = 1 << 3,
  WELS_LOG_RESV      = 1 << 4,
  WELS_LOG_DEFAULT   = WELS_LOG_ERROR | WELS_LOG_WARNING | WELS_LOG_INFO | WELS_LOG_DEBUG,


  MAX_LOG_SIZE       = 1024,
};

virtual ~IWelsTrace() {};

virtual int32_t  SetTraceLevel (int32_t iLevel) = 0;
virtual int32_t  Trace (const int32_t kLevel, const str_t* kpFormat,  va_list pVl) = 0;

static void_t  WelsTrace (void_t* pObject, const int32_t kLevel, const str_t* kpFormat, va_list pVl) {
  IWelsTrace*   pThis = (IWelsTrace*) (pObject);

  if (pThis) {
    pThis->Trace (kLevel, kpFormat, pVl);
  }
}

static void_t WelsVTrace (void_t* pObject, const int32_t kLevel, const str_t* kpFormat, ...) {
  IWelsTrace* pThis = (IWelsTrace*) (pObject);

  va_list  argptr;

  va_start (argptr, kpFormat);

  if (pThis) {
    pThis->Trace (kLevel, kpFormat, argptr);
  }

  va_end (argptr);
}


};

class CWelsTraceBase : public IWelsTrace {
 public:
virtual int32_t  SetTraceLevel (int32_t iLevel);
virtual int32_t  Trace (const int32_t kLevel, const str_t* kpFormat,  va_list pVl);

virtual int32_t  WriteString (int32_t iLevel, const str_t* pStr) = 0;
 protected:
CWelsTraceBase() {
  m_iLevel = WELS_LOG_DEFAULT;
};

 private:
int32_t   m_iLevel;
};

class CWelsTraceFile : public CWelsTraceBase {
 public:
CWelsTraceFile (const str_t*   filename = (const str_t*)"wels_decoder_trace.txt");
virtual ~CWelsTraceFile();

 public:
virtual int32_t  WriteString (int32_t iLevel, const str_t* pStr);

 private:
WelsFileHandle* m_pTraceFile;
};

#ifdef  _WIN32
class CWelsTraceWinDgb : public CWelsTraceBase {
 public:
CWelsTraceWinDgb() {};
virtual ~CWelsTraceWinDgb() {};

 public:
virtual int32_t  WriteString (int32_t iLevel, const str_t* pStr);
};
#endif

class CWelsCodecTrace : public CWelsTraceBase {
 public:
CWelsCodecTrace() ;
virtual ~CWelsCodecTrace();

 public:
virtual int32_t  WriteString (int32_t iLevel, const str_t* pStr);

 protected:
int32_t  LoadWelsTraceModule();
int32_t  UnloadWelsTraceModule();

 private:

CM_WELS_TRACE m_fpDebugTrace;
CM_WELS_TRACE m_fpInfoTrace;
CM_WELS_TRACE m_fpWarnTrace;
CM_WELS_TRACE m_fpErrorTrace;
};


IWelsTrace*   CreateWelsTrace (EWelsTraceType  eType,  void_t* pParam = NULL);

} // namespace WelsDec

#endif //WELS_CODEC_TRACE
