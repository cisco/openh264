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
#include <string.h>
#include <Carbon/Carbon.h>
#include <CoreFoundation/CFBundle.h>

#include <dlfcn.h>
#include <string>

#include "bundleloader.h"
#include "codec_api.h"

typedef long (*LPCreateWelsCSEncoder)(ISVCEncoder** ppEncoder);
typedef void (*LPDestroyWelsCSEncoder)(ISVCEncoder* pEncoder);

CFBundleRef g_at264Module = nil;

const char H264EncoderDLL[] = "welsenc.bundle";

int WelsEncGetCurrentModulePath(char* lpModulePath, const int iPathMax)
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

	int locateNumber = 1;

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

int32_t WelsEncBundleLoad()
{

	char achPath[512] = {0};

	WelsEncGetCurrentModulePath(achPath, 512);
	strlcat(achPath, H264EncoderDLL, 512);

	g_at264Module = LoadBundle(achPath);

	if (g_at264Module == NULL)
		return 1;
	else
		return 0;
}

void WelsEncBundleFree()
{
	if(g_at264Module != NULL)
	{
		CFBundleUnloadExecutable(g_at264Module);
	}
}

int32_t WelsEncBundleCreateEncoder(ISVCEncoder** ppEncoder)
{
	if(!g_at264Module)
		return 1;

	LPCreateWelsCSEncoder pfuncCreateCSEnc =
	(LPCreateWelsCSEncoder)GetProcessAddress(g_at264Module, "CreateSVCEncoder");

	if(pfuncCreateCSEnc != NULL)
	{
		return (pfuncCreateCSEnc( ppEncoder ));
	}

	return 1;
}

int32_t WelsEncBundleDestroyEncoder(ISVCEncoder* pEncoder)
{
	if(!g_at264Module)
		return 1;

	LPDestroyWelsCSEncoder pfuncDestroyCSEnc =
	(LPDestroyWelsCSEncoder)GetProcessAddress(g_at264Module, "DestroySVCEncoder");

	if(pfuncDestroyCSEnc != NULL){
		pfuncDestroyCSEnc( pEncoder );
		return 0;
	}
	else
		return 1;
}


