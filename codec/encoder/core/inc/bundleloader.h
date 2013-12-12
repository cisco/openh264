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

#ifndef WELS_BOUNDLELOAD_H
#define WELS_BOUNDLELOAD_H

#if defined(MACOS)

#include <dlfcn.h>
#include <carbon/carbon.h>
#include <coreFoundation/CFBundle.h>
#include <string>

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

	struct FSRef bundlePath;
	OSStatus iStatus = FSPathMakeRef((unsigned char*)lpBundlePath, &bundlePath, NULL);
	if(noErr != iStatus)
	{
		return NULL;
	}

	CFURLRef bundleURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &bundlePath);
	if(NULL == bundleURL)
	{
		return NULL;
	}

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

Boolean FreeBundle(CFBundleRef bundleRef)
{
	if(NULL != bundleRef)
	{
		//	CFBundleUnloadExecutable(bundleRef);
		CFRelease(bundleRef);
	}
	return TRUE;
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
#endif

#endif