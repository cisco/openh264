/*!
 * \copy
 *     Copyright (c)  2011-2013, Cisco Systems
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
 * \file	load_bundle_functions.cpp
 *
 * \brief	load bundle and function on Mac platform
 *
 * \date	Created on 03/15/2011
 *
 * \description : 1. Load bundle: welsdec.bundle
 *                2. Load address of function
 *                3. Create or destroy decoder
 *
 *************************************************************************************
 */




#include <string.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CFBundle.h>

#include <dlfcn.h>
#include <string>

#include "dec_console.h"
#include "codec_api.h"

typedef long (*LPCreateWelsCSDecoder)(ISVCDecoder** ppDecoder);
typedef void (*LPDestroyWelsCSDecoder)(ISVCDecoder* pDecoder);


typedef long (*LPCreateVHDController)();
typedef void (*LPDestroyVHDController)();

CFBundleRef g_at264Module = nil;

const char H264DecoderDLL[] = "welsdec.bundle";

CFBundleRef g_at264ModuleHWD = nil;


////////////////////////////////////////////////////////////////////////////////////////
int GetCurrentModulePath(char* lpModulePath, const int iPathMax)
{
	if(lpModulePath == NULL || iPathMax <= 0)
	{
		return -1;
	}

	memset(lpModulePath, 0, iPathMax);

	char cCurrentPath[PATH_MAX];
	memset(cCurrentPath, 0, PATH_MAX);

	Dl_info 	dlInfo;
	static int  sDummy;
	dladdr((void*)&sDummy, &dlInfo);

	strlcpy(cCurrentPath, dlInfo.dli_fname, PATH_MAX);

#if defined(__apple__)
	// whether is self a framework ?
	int locateNumber = 1;
	struct FSRef currentPath;
	OSStatus iStatus = FSPathMakeRef((unsigned char*)cCurrentPath, &currentPath, NULL);
	if(noErr == iStatus)
	{
		LSItemInfoRecord  info;
		iStatus = LSCopyItemInfoForRef(&currentPath, kLSRequestExtension, &info);
		if(noErr == iStatus && NULL == info.extension)
		{
			locateNumber = 4;
		}
	}
#else
	int locateNumber = 1;
#endif

	std::string strPath(cCurrentPath);
	int pos = std::string::npos;
	for(int i = 0; i < locateNumber; i++)
	{
		pos = strPath.rfind('/');
		if(std::string::npos == pos)
		{
			break;
		}
		strPath.erase(pos);
	}
	if(std::string::npos == pos)
	{
		return -2;
	}
	cCurrentPath[pos] = 0;

	strlcpy(lpModulePath, cCurrentPath, iPathMax);
	strlcat(lpModulePath, "/", iPathMax);

	return 0;
}

CFBundleRef LoadBundle(const char* lpBundlePath)
{
	if(lpBundlePath == NULL)
	{
		return NULL;
	}

	CFStringRef bundlePath = CFStringCreateWithCString(kCFAllocatorSystemDefault, lpBundlePath, CFStringGetSystemEncoding());
	if(NULL == bundlePath)
	{
		return NULL;
	}

	CFURLRef bundleURL = CFURLCreateWithString(kCFAllocatorSystemDefault, bundlePath, NULL);
	if(NULL == bundleURL)
	{
		return NULL;
	}
#endif

	// 2.get bundle ref
	CFBundleRef bundleRef = CFBundleCreate(kCFAllocatorSystemDefault, bundleURL);
	CFRelease(bundleURL);

	if(NULL != bundleRef)
	{
	}

	return bundleRef;
}

void* GetProcessAddress(CFBundleRef bundleRef, const char* lpProcName)
{
	void *processAddress = NULL;
	if(NULL != bundleRef)
	{
		CFStringRef cfProcName = CFStringCreateWithCString(kCFAllocatorSystemDefault, lpProcName, CFStringGetSystemEncoding());
		processAddress = CFBundleGetFunctionPointerForName(bundleRef, cfProcName);
		CFRelease(cfProcName);
	}
	return processAddress;
}


////////////////////////

bool load_bundle_welsdec()
{

	char achPath[512] = {0};

	GetCurrentModulePath(achPath, 512);
	strlcat(achPath, H264DecoderDLL, 512);

	g_at264Module = LoadBundle(achPath);

	if (g_at264Module == NULL)
		return false;

	return true;

}

void free_bundle_welsdec()
{
	if(g_at264Module != NULL)
	{
		CFBundleUnloadExecutable(g_at264Module);
	}
}

bool get_functions_address_create_decoder(ISVCDecoder** ppDecoder)
{
	if(!g_at264Module)
		return false;

	LPCreateWelsCSDecoder pfuncCreateSWDec =
	(LPCreateWelsCSDecoder)GetProcessAddress(g_at264Module, "CreateSVCDecoder");

	LPCreateVHDController pfuncCreateHWDec =
	(LPCreateVHDController)GetProcessAddress(g_at264Module, "CreateSVCVHDController");


	if(pfuncCreateSWDec != NULL)
	{
		pfuncCreateSWDec( ppDecoder );
	}
	else
	{
		return false;
	}

	if(pfuncCreateHWDec != NULL)
	{
		pfuncCreateHWDec();
	}
	else
	{
		return false;
	}

	return true;

}

bool get_functions_address_free_decoder(ISVCDecoder* pDecoder)
{
	if(!g_at264Module)
		return false;

	LPDestroyWelsCSDecoder pfuncDestroySWDec =
	(LPDestroyWelsCSDecoder)GetProcessAddress(g_at264Module, "DestroySVCDecoder");

	LPDestroyVHDController pfuncDestroyHWDec =
	(LPDestroyVHDController)GetProcessAddress(g_at264Module, "DestroySVCVHDController");

	if(pfuncDestroySWDec != NULL)
	{
		pfuncDestroySWDec( pDecoder );
	}
	else
	{
		return false;
	}

	if(pfuncDestroyHWDec != NULL)
	{
		pfuncDestroyHWDec();
	}
	else
	{
		return false;
	}

	return true;
}


