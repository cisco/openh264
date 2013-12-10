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
#include "utils.h"

#include "welsCodecTrace.h"
#include "utils.h"
#include "logging.h"
#if defined LINUX || defined SOLARIS || defined UNIX || defined MACOS //LINUX/SOLARIS/UNIX
#include <dlfcn.h>
#endif

#if defined(MACOS)
#include <carbon/carbon.h>
#include <CoreFoundation/CFBundle.h>
#endif//MACOS

//using namespace WelsDec;

namespace WelsDec {

#ifdef MACOS
static CFBundleRef LoadLibrary(const char* lpszbundle)
{
	// 1.get bundle path
	char cBundlePath[PATH_MAX];
	memset(cBundlePath, 0, PATH_MAX);
	
	Dl_info 	dlInfo;
	static int  sDummy;
	dladdr((void_t*)&sDummy, &dlInfo);
	
	strlcpy(cBundlePath, dlInfo.dli_fname, PATH_MAX);
	
	char * pPath = NULL;
	for(int i = 4; i > 0; i--)
	{
		pPath = strrchr(cBundlePath,'/');//confirmed_safe_unsafe_usage
		if(pPath)
		{
			*pPath = 0;
		}
		else
		{
			break;
		}
	}
	if(pPath)
	{
		strlcat(cBundlePath, "/", PATH_MAX);
	}
	else
	{
		return NULL;
	}
	
	strlcat(cBundlePath, lpszbundle, PATH_MAX);
	
	FSRef bundlePath;
	OSStatus iStatus = FSPathMakeRef((unsigned char*)cBundlePath, &bundlePath, NULL);
	if(noErr != iStatus)
		return NULL;
	
	CFURLRef bundleURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &bundlePath);
	if(NULL == bundleURL)
		return NULL;
	
	// 2.get bundle ref
	CFBundleRef bundleRef = CFBundleCreate(kCFAllocatorSystemDefault, bundleURL);
	CFRelease(bundleURL);
	
//	Boolean bReturn = FALSE;
	if(NULL != bundleRef)
	{
		//	bReturn = CFBundleLoadExecutable(bundleRef);
	}
	
	return bundleRef;
}

static Boolean FreeLibrary(CFBundleRef bundle)
{	
	if(NULL != bundle)
	{
		//	CFBundleUnloadExecutable(bundle);
		CFRelease(bundle);
	}
	
	return TRUE;
}

static void_t* GetProcessAddress(CFBundleRef bundle, const char* lpszprocname)
{
	if(NULL == bundle)
		return NULL;
	
	CFStringRef cfprocname = CFStringCreateWithCString(NULL,lpszprocname,CFStringGetSystemEncoding());
	void_t *processAddress = CFBundleGetFunctionPointerForName(bundle,cfprocname);
	CFRelease(cfprocname);
	
	return processAddress;
}
#endif



int32_t  CWelsTraceBase::SetTraceLevel(int iLevel)
{
	m_iLevel = iLevel;

	return 0;
}

int32_t  CWelsTraceBase::Trace(const int kLevel, const str_t *kpFormat, va_list pVl)
{
	if( kLevel & m_iLevel ){
		str_t chWStrFormat[MAX_LOG_SIZE] = {0};
		str_t chBuf[MAX_LOG_SIZE] = {0};
		str_t chResult[MAX_LOG_SIZE] = {0};
		const int32_t kLen	= WelsStrnlen((const str_t *)"[DECODER]: ", MAX_LOG_SIZE);

		WelsStrncpy(chWStrFormat, MAX_LOG_SIZE, (const str_t *)kpFormat, WelsStrnlen((const str_t *)kpFormat, MAX_LOG_SIZE));	

		WelsStrncpy(chBuf, MAX_LOG_SIZE, (const str_t *)"[DECODER]: ", kLen);

		WelsVsprintf((chBuf + kLen),  MAX_LOG_SIZE - kLen, (const str_t *)kpFormat, pVl);
		WelsStrncpy(chResult, MAX_LOG_SIZE, (const str_t *)chBuf, WelsStrnlen((const str_t *)chBuf, MAX_LOG_SIZE));

		WriteString(kLevel, chResult);
	}

	return 0;
}

CWelsTraceFile::CWelsTraceFile(const str_t * pFileName)
{
	m_pTraceFile = WelsFopen(pFileName, (const str_t *)"wt");
}

CWelsTraceFile::~CWelsTraceFile()
{
	if( m_pTraceFile ){
		WelsFclose(m_pTraceFile);
		m_pTraceFile = NULL;
	}
}

int32_t CWelsTraceFile::WriteString(int32_t iLevel, const str_t * pStr)
{
	int  iRC = 0;
	const static str_t chEnter[16] = "\n";
	if( m_pTraceFile ){
		iRC += WelsFwrite(pStr, 1, WelsStrnlen(pStr, MAX_LOG_SIZE), m_pTraceFile);
		iRC += WelsFwrite(chEnter, 1, WelsStrnlen(chEnter,  16), m_pTraceFile);
		WelsFflush(m_pTraceFile);
	}
	return iRC;
}


#ifdef WIN32

int32_t CWelsTraceWinDgb::WriteString(int32_t iLevel, const str_t * pStr)
{
	OutputDebugStringA(pStr);

	return WelsStrnlen(pStr, MAX_LOG_SIZE);//strnlen(pStr, MAX_LOG_SIZE);
}

#endif

CWelsCodecTrace::CWelsCodecTrace()
{
	m_hTraceHandle = NULL;
        m_fpDebugTrace = NULL;
	m_fpInfoTrace = NULL;
	m_fpWarnTrace = NULL;
	m_fpErrorTrace = NULL;

	LoadWelsTraceModule();
}

CWelsCodecTrace::~CWelsCodecTrace()
{
	UnloadWelsTraceModule();
}

int32_t  CWelsCodecTrace::LoadWelsTraceModule()
{
#ifdef NO_DYNAMIC_VP
        m_fpDebugTrace = welsStderrTrace<WELS_LOG_DEBUG>;
        m_fpInfoTrace = welsStderrTrace<WELS_LOG_INFO>;
        m_fpWarnTrace = welsStderrTrace<WELS_LOG_WARNING>;
        m_fpErrorTrace = welsStderrTrace<WELS_LOG_ERROR>;
#else
#if defined WIN32	
	HMODULE hHandle = ::LoadLibrary("welstrace.dll");
//	HMODULE handle = ::LoadLibrary("contrace.dll");  // for c7 trace
	if ( NULL == hHandle )
		return -1;

	CHAR chPath[ _MAX_PATH]= {0};
	GetModuleFileName( (HMODULE)hHandle, chPath, _MAX_PATH);

	m_hTraceHandle = ::LoadLibrary(chPath);
	
	OutputDebugStringA(chPath);
	if( m_hTraceHandle) {
		m_fpDebugTrace = ( CM_WELS_TRACE)::GetProcAddress( ( HMODULE)m_hTraceHandle, "WELSDEBUGA");
		m_fpInfoTrace = ( CM_WELS_TRACE)::GetProcAddress( ( HMODULE)m_hTraceHandle, "WELSINFOA");
		m_fpWarnTrace = ( CM_WELS_TRACE)::GetProcAddress( ( HMODULE)m_hTraceHandle, "WELSWARNA");
		m_fpErrorTrace = ( CM_WELS_TRACE)::GetProcAddress( ( HMODULE)m_hTraceHandle, "WELSERRORA");
	}

	// coverity scan uninitial
	if (hHandle != NULL)
	{
		::FreeLibrary(hHandle);
		hHandle = NULL;
	}
#elif defined MACOS
	m_hTraceHandle = LoadLibrary("welstrace.bundle");
	if(m_hTraceHandle) {
		m_fpDebugTrace = ( CM_WELS_TRACE)GetProcessAddress( (CFBundleRef)m_hTraceHandle, "WELSDEBUG2");
		m_fpInfoTrace = ( CM_WELS_TRACE)GetProcessAddress( (CFBundleRef)m_hTraceHandle, "WELSINFO2");
		m_fpWarnTrace = ( CM_WELS_TRACE)GetProcessAddress( (CFBundleRef)m_hTraceHandle, "WELSWARN2");
		m_fpErrorTrace = ( CM_WELS_TRACE)GetProcessAddress( (CFBundleRef)m_hTraceHandle, "WELSERROR2");
	}
#elif defined LINUX || defined SOLARIS || defined UNIX
//#else
//	CCmString	cmPath;
	str_t chPath[255]= {0};
	Dl_info		sDlInfo;
	static int	iMmTPAddress;
    dladdr( &iMmTPAddress, &sDlInfo);

	if (NULL == sDlInfo.dli_fname)
		return -1;
	WelsStrncpy(chPath, 255, (const str_t*)sDlInfo.dli_fname, WelsStrnlen((const str_t*)sDlInfo.dli_fname, 255));
	str_t* p = strrchr(chPath, '/');//confirmed_safe_unsafe_usage
	if ( NULL == p )
		return -1;
	const int iLenTraceName = WelsStrnlen((const str_t*)"/libwelstrace.so", 15);
	const int iCurPos = p - chPath;
	if ( iCurPos + iLenTraceName < 255 )
		WelsStrncpy(p, 254-iCurPos, (const str_t*)"/libwelstrace.so", iLenTraceName );
	else
		return -1;

	m_hTraceHandle = dlopen( chPath, RTLD_LAZY);
	if (m_hTraceHandle == NULL)
	{
		WelsFileHandle* fp = WelsFopen((const str_t*)"/tmp/trace.txt", (const str_t*)"a");
		if(fp)
		{
			fprintf(fp, "welsCodecTrace::welsCodecTrace ===> dlopen %s fail, %s\n", chPath, dlerror());
			WelsFclose(fp);
		}
		return -1;
	}
	if (m_hTraceHandle) {
		m_fpDebugTrace = ( CM_WELS_TRACE)dlsym( m_hTraceHandle, "WELSDEBUG2");
		m_fpInfoTrace = ( CM_WELS_TRACE)dlsym( m_hTraceHandle, "WELSINFO2");
		m_fpWarnTrace = ( CM_WELS_TRACE)dlsym( m_hTraceHandle, "WELSWARN2");
		m_fpErrorTrace = ( CM_WELS_TRACE)dlsym( m_hTraceHandle, "WELSERROR2");
		if(m_fpDebugTrace == NULL)
		{
			WelsFileHandle* fp = WelsFopen((const str_t*)"/tmp/trace.txt", (const str_t*)"a");
			if(fp)
			{
				printf("welsCodecTrace::welsCodecTrace ===> dlsym failed (WELSDEBUG2) , dlerror = %s\n", dlerror());
				WelsFclose(fp);
			}
			return -1;
		}
	}
#endif
#endif  // NO_DYNAMIC_VP
	return 0;
}

int32_t  CWelsCodecTrace::UnloadWelsTraceModule()
{
#if defined WIN32
	if( m_hTraceHandle) {
		::FreeLibrary( ( HMODULE)m_hTraceHandle);
	}
#elif defined MACOS
	if (m_hTraceHandle) {
		FreeLibrary( (CFBundleRef)m_hTraceHandle);
	}
#elif defined LINUX || defined SOLARIS || defined UNIX
	if (m_hTraceHandle) {
		::dlclose( m_hTraceHandle);
	}
#endif

	m_hTraceHandle = NULL;
	m_fpDebugTrace = NULL;
	m_fpInfoTrace = NULL;
	m_fpWarnTrace = NULL;
	m_fpErrorTrace = NULL;
	return 0;
}

int32_t  CWelsCodecTrace::WriteString(int32_t iLevel, const str_t * pStr)
{
#ifndef NO_DYNAMIC_VP
	if( m_hTraceHandle )
#endif
	{
#ifdef WIN32
		switch(iLevel)
		{
		case WELS_LOG_ERROR:
			if(m_fpErrorTrace)
				m_fpErrorTrace("%s", pStr);
			break;
		case WELS_LOG_WARNING:
			if(m_fpWarnTrace)
				m_fpWarnTrace("%s", pStr);
			break;
		case WELS_LOG_INFO:
			if(m_fpInfoTrace)
				m_fpInfoTrace("%s", pStr);
			break;
		case WELS_LOG_DEBUG:
			if(m_fpDebugTrace)
				m_fpDebugTrace("%s", pStr);
			break;
		default:
			if(m_fpDebugTrace)
				m_fpInfoTrace("%s", pStr);
			break;
		}
#else
		switch(iLevel)
		{
		case WELS_LOG_ERROR:
			if(m_fpErrorTrace)
				m_fpErrorTrace("CODEC", "%s", pStr);
			break;
		case WELS_LOG_WARNING:
			if(m_fpWarnTrace)
				m_fpWarnTrace("CODEC", "%s",  pStr);
			break;
		case WELS_LOG_INFO:
			if(m_fpInfoTrace)
				m_fpInfoTrace("CODEC", "%s",  pStr);
			break;
		case WELS_LOG_DEBUG:
			if(m_fpInfoTrace)
				m_fpInfoTrace("CODEC", "%s",  pStr);
			break;
		default:
			if(m_fpInfoTrace)
				m_fpInfoTrace("CODEC", "%s",  pStr);
			break;
		}
#endif
	}

	return 0;
}


IWelsTrace  * CreateWelsTrace(EWelsTraceType  eType,  void_t * pParam)
{
	IWelsTrace  * pTrace = NULL;
	switch(eType)
	{
	case Wels_Trace_Type:
		pTrace = new CWelsCodecTrace();
		break;
	case Wels_Trace_Type_File:
		pTrace = new CWelsTraceFile();
		break;
#ifdef WIN32
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
