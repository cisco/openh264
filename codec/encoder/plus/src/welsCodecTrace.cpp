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

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "crt_util_safe_x.h"	// Safe CRT routines like utils for cross platforms

#include "welsCodecTrace.h"
#include "utils.h"
#if defined LINUX || defined SOLARIS || defined UNIX || defined MACOS //LINUX/SOLARIS/UNIX
#include <dlfcn.h>
#endif

#if defined(MACOS)
#include <carbon/carbon.h>
#include <CoreFoundation/CFBundle.h>
#endif//MACOS

#ifdef WIN32
extern HANDLE g_hInstDll;
#endif

#include "logging.h"

//#define CODEC_TRACE_ERROR 0
//#define CODEC_TRACE_WARNING 1
//#define CODEC_TRACE_INFO 2
//#define CODEC_TRACE_DEDBUG 3

using namespace WelsSVCEnc;

#ifdef MACOS
static CFBundleRef LoadLibrary (const str_t* lpszbundle) {
  // 1.get bundle path
  str_t cBundlePath[PATH_MAX];
  memset (cBundlePath, 0, PATH_MAX);

  Dl_info 	dlInfo;
  static int32_t  sDummy;
  dladdr ((void*)&sDummy, &dlInfo);

  strlcpy (cBundlePath, dlInfo.dli_fname, PATH_MAX);	// confirmed_safe_unsafe_usage

  str_t* pPath = NULL;
  for (int32_t i = 4; i > 0; i--) {
    pPath = strrchr (cBundlePath, '/');	// confirmed_safe_unsafe_usage
    if (pPath) {
      *pPath = 0;
    } else {
      break;
    }
  }
  if (pPath) {
    strlcat (cBundlePath, "/", PATH_MAX);	// confirmed_safe_unsafe_usage
  } else {
    return NULL;
  }

  strlcat (cBundlePath, lpszbundle, PATH_MAX);	// confirmed_safe_unsafe_usage

  FSRef bundlePath;
  OSStatus iStatus = FSPathMakeRef ((uint8_t*)cBundlePath, &bundlePath, NULL);
  if (noErr != iStatus)
    return NULL;

  CFURLRef bundleURL = CFURLCreateFromFSRef (kCFAllocatorSystemDefault, &bundlePath);
  if (NULL == bundleURL)
    return NULL;

  // 2.get bundle pRef
  CFBundleRef bundleRef = CFBundleCreate (kCFAllocatorSystemDefault, bundleURL);
  CFRelease (bundleURL);

//	Boolean bReturn = FALSE;
  if (NULL != bundleRef) {
    //	bReturn = CFBundleLoadExecutable(bundleRef);
  }

  return bundleRef;
}

static Boolean FreeLibrary (CFBundleRef bundle) {
  if (NULL != bundle) {
    //	CFBundleUnloadExecutable(bundle);
    CFRelease (bundle);
  }

  return TRUE;
}

static void* GetProcessAddress (CFBundleRef bundle, const str_t* lpszprocname) {
  if (NULL == bundle)
    return NULL;

  CFStringRef cfprocname = CFStringCreateWithCString (NULL, lpszprocname, CFStringGetSystemEncoding());
  void* processAddress = CFBundleGetFunctionPointerForName (bundle, cfprocname);
  CFRelease (cfprocname);

  return processAddress;
}
#endif

int32_t	welsCodecTrace::m_iTraceLevel			= WELS_LOG_DEFAULT;
#if defined(WIN32)
CM_WELS_TRACE welsCodecTrace::m_fpDebugTrace	= NULL;
CM_WELS_TRACE welsCodecTrace::m_fpInfoTrace	= NULL;
CM_WELS_TRACE welsCodecTrace::m_fpWarnTrace	= NULL;
CM_WELS_TRACE welsCodecTrace::m_fpErrorTrace	= NULL;
#else
CM_WELS_TRACE2 welsCodecTrace::m_fpDebugTrace   = NULL;
CM_WELS_TRACE2 welsCodecTrace::m_fpInfoTrace	= NULL;
CM_WELS_TRACE2 welsCodecTrace::m_fpWarnTrace	= NULL;
CM_WELS_TRACE2 welsCodecTrace::m_fpErrorTrace   = NULL;
#endif//WIN32

welsCodecTrace::welsCodecTrace() {
  m_hTraceHandle = NULL;
  m_fpDebugTrace = NULL;
  m_fpInfoTrace = NULL;
  m_fpWarnTrace = NULL;
  m_fpErrorTrace = NULL;
  m_WelsTraceExistFlag	= false;
#ifdef NO_DYNAMIC_VP
  m_fpDebugTrace = welsStderrTrace<WELS_LOG_DEBUG>;
  m_fpInfoTrace = welsStderrTrace<WELS_LOG_INFO>;
  m_fpWarnTrace = welsStderrTrace<WELS_LOG_WARNING>;
  m_fpErrorTrace = welsStderrTrace<WELS_LOG_ERROR>;

  m_WelsTraceExistFlag = true;
#else
#if defined WIN32
  HMODULE handle = ::GetModuleHandle ("welstrace.dll");
//	HMODULE handle = ::GetModuleHandle("contrace.dll"); // for c7
  if (NULL == handle)
    return;

  CHAR achPath[ _MAX_PATH] = {0};
  GetModuleFileName ((HMODULE)handle, achPath, _MAX_PATH);

  m_hTraceHandle = ::LoadLibrary (achPath);

  OutputDebugStringA (achPath);
  if (m_hTraceHandle) {
    m_fpDebugTrace = (CM_WELS_TRACE)::GetProcAddress ((HMODULE)m_hTraceHandle, "WELSDEBUGA");
    m_fpInfoTrace = (CM_WELS_TRACE)::GetProcAddress ((HMODULE)m_hTraceHandle, "WELSINFOA");
    m_fpWarnTrace = (CM_WELS_TRACE)::GetProcAddress ((HMODULE)m_hTraceHandle, "WELSWARNA");
    m_fpErrorTrace = (CM_WELS_TRACE)::GetProcAddress ((HMODULE)m_hTraceHandle, "WELSERRORA");
  }
#elif defined MACOS
  m_hTraceHandle = LoadLibrary ("welstrace.bundle");
  if (m_hTraceHandle) {
    m_fpDebugTrace = (CM_WELS_TRACE2)GetProcessAddress ((CFBundleRef)m_hTraceHandle, "WELSDEBUG2");
    m_fpInfoTrace = (CM_WELS_TRACE2)GetProcessAddress ((CFBundleRef)m_hTraceHandle, "WELSINFO2");
    m_fpWarnTrace = (CM_WELS_TRACE2)GetProcessAddress ((CFBundleRef)m_hTraceHandle, "WELSWARN2");
    m_fpErrorTrace = (CM_WELS_TRACE2)GetProcessAddress ((CFBundleRef)m_hTraceHandle, "WELSERROR2");
  }
#elif defined LINUX || defined SOLARIS || defined UNIX
//#else
//	CCmString	cmPath;
  str_t achPath[255] = {0};
  Dl_info		DlInfo;
  static int32_t	nMmTPAddress;
  dladdr (&nMmTPAddress, &DlInfo);

  if (NULL == DlInfo.dli_fname)
    return;
  STRNCPY (achPath, 255, DlInfo.dli_fname, STRNLEN (DlInfo.dli_fname, 255));	// confirmed_safe_unsafe_usage
  str_t* p = strrchr (achPath, '/');	// confirmed_safe_unsafe_usage
  if (NULL == p)
    return;
  const int32_t kiLenTraceName = STRNLEN ("/libwelstrace.so", 15);	// confirmed_safe_unsafe_usage
  const int32_t kiCurPos = p - achPath;
  if (kiCurPos + kiLenTraceName < 255)
    STRNCPY (p, 254 - kiCurPos, "/libwelstrace.so", kiLenTraceName);	// confirmed_safe_unsafe_usage
  else
    return;

  m_hTraceHandle = dlopen (achPath, RTLD_LAZY);
  if (m_hTraceHandle == NULL) {
    FILE* fp = fopen ("/tmp/trace.txt", "a");
    if (fp) {
      fprintf (fp, "welsCodecTrace::welsCodecTrace ===> dlopen %s fail, %s\n", achPath, dlerror());
      fclose (fp);
    }
    return;
  }
  if (m_hTraceHandle) {
    m_fpDebugTrace = (CM_WELS_TRACE2)dlsym (m_hTraceHandle, "WELSDEBUG2");
    m_fpInfoTrace = (CM_WELS_TRACE2)dlsym (m_hTraceHandle, "WELSINFO2");
    m_fpWarnTrace = (CM_WELS_TRACE2)dlsym (m_hTraceHandle, "WELSWARN2");
    m_fpErrorTrace = (CM_WELS_TRACE2)dlsym (m_hTraceHandle, "WELSERROR2");
    if (m_fpDebugTrace == NULL) {
      FILE* fp = fopen ("/tmp/trace.txt", "a");
      if (fp) {
        printf ("welsCodecTrace::welsCodecTrace ===> dlsym failed (WELSDEBUG2) , dlerror = %s\n", dlerror());
        fclose (fp);
      }
      return;
    }
  }
#endif
  if (m_hTraceHandle != NULL) {
    m_WelsTraceExistFlag	= true;
  }
#endif
}

welsCodecTrace::~welsCodecTrace() {
#if defined WIN32
  if (m_hTraceHandle) {
    ::FreeLibrary ((HMODULE)m_hTraceHandle);
  }
#elif defined MACOS
  if (m_hTraceHandle) {
    FreeLibrary ((CFBundleRef)m_hTraceHandle);
  }
#elif defined LINUX || defined SOLARIS || defined UNIX
  if (m_hTraceHandle) {
    ::dlclose (m_hTraceHandle);
  }
#endif

  m_hTraceHandle = NULL;
  m_fpDebugTrace = NULL;
  m_fpInfoTrace = NULL;
  m_fpWarnTrace = NULL;
  m_fpErrorTrace = NULL;
//	g_bWelsLibLoaded = false;
  m_WelsTraceExistFlag = false;
}

int32_t welsCodecTrace::WelsTraceModuleIsExist() {
  return m_WelsTraceExistFlag;
}

void welsCodecTrace::TraceString (int32_t iLevel, const str_t* str) {
#ifdef WIN32
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
#else
  switch (iLevel) {
  case WELS_LOG_ERROR:
    if (m_fpErrorTrace)
      m_fpErrorTrace ("CODEC", "%s", str);
    break;
  case WELS_LOG_WARNING:
    if (m_fpWarnTrace)
      m_fpWarnTrace ("CODEC", "%s",  str);
    break;
  case WELS_LOG_INFO:
    if (m_fpInfoTrace)
      m_fpInfoTrace ("CODEC", "%s",  str);
    break;
  case WELS_LOG_DEBUG:
    if (m_fpInfoTrace)
      m_fpInfoTrace ("CODEC", "%s",  str);
    break;
  default:
    if (m_fpInfoTrace)
      m_fpInfoTrace ("CODEC", "%s",  str);
    break;
  }
#endif
}

#define MAX_LOG_SIZE	1024

void welsCodecTrace::CODEC_TRACE (void* ignore, const int32_t iLevel, const str_t* Str_Format, va_list vl) {
//		if(g_traceLevel < iLevel)
  if (m_iTraceLevel < iLevel) {
    return;
  }

  str_t WStr_Format[MAX_LOG_SIZE] = {0};
  str_t pBuf[MAX_LOG_SIZE] = {0};
  str_t cResult[MAX_LOG_SIZE] = {0};
  const int32_t len	= STRNLEN ("[ENCODER]: ", MAX_LOG_SIZE);	// confirmed_safe_unsafe_usage

  STRNCPY (WStr_Format, MAX_LOG_SIZE, Str_Format, STRNLEN (Str_Format, MAX_LOG_SIZE));	// confirmed_safe_unsafe_usage

  STRNCPY (pBuf, MAX_LOG_SIZE, "[ENCODER]: ", len);	// confirmed_safe_unsafe_usage
#if defined(WIN32)
#if defined(_MSC_VER)
#if _MSC_VER>=1500
  VSPRINTF (pBuf + len, MAX_LOG_SIZE - len, WStr_Format, vl);	// confirmed_safe_unsafe_usage
#else
  VSPRINTF (pBuf + len, WStr_Format, vl);	// confirmed_safe_unsafe_usage
#endif//_MSC_VER>=1500
#endif//_MSC_VER
#else//__GNUC__
  VSPRINTF (pBuf + len, WStr_Format, vl);	// confirmed_safe_unsafe_usage
#endif//WIN32
  STRNCPY (cResult, MAX_LOG_SIZE, pBuf, STRNLEN (pBuf, MAX_LOG_SIZE));	// confirmed_safe_unsafe_usage

//		g_WelsCodecTrace.TraceString(iLevel, cResult);
  welsCodecTrace::TraceString (iLevel, cResult);
}

void welsCodecTrace::SetTraceLevel (const int32_t iLevel) {
//	g_traceLevel	= iLevel;
  if (iLevel >= 0)
    m_iTraceLevel	= iLevel;
}


