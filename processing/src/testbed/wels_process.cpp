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

#include <windows.h>
#include "wels_process.h"
#include "bundleloader.h"

// entry API declaration
typedef vResult (WELSAPI* pfnCreateVpInterface) (void**, int);
typedef vResult (WELSAPI* pfnDestroyVpInterface) (void*, int);

////////////////////////////////////////////////////////
void* loadlib() {
#if defined(WIN32)
  HMODULE shModule = LoadLibraryA ("WelsVP.dll");
  if (shModule == NULL)
    shModule = LoadLibraryA ("../WelsVP.dll");

#elif defined(MACOS)
  const char WelsVPLib[] = "WelsVP.bundle";
  CFBundleRef shModule = LoadBundle (WelsVPLib);

#elif defined(UNIX)
  const char WelsVPLib[] = "WelsVP.so";
  void* shModule = dlopen (WelsVPLib, RTLD_LAZY);
#endif

  return (void*)shModule;
}

void freelib (void* lib) {
  if (lib) {
#ifdef WIN32
    HMODULE shModule = (HMODULE)lib;
    FreeLibrary (shModule);

#elif defined(MACOS)
    CFBundleRef shModule = (CFBundleRef)lib;
    FreeBundle (shModule);

#elif defined(UNIX)
    void* shModule = lib;
    dlclose (shModule);
#endif
  }
}

void* queryfunc (void* lib, const char* name) {
  void* pFunc = NULL;
#ifdef WIN32
  HMODULE shModule = (HMODULE)lib;
  pFunc = (void*)GetProcAddress (shModule, name);
#elif defined(MACOS)
  CFBundleRef shModule = (CFBundleRef)lib;
  pFunc = (void*)GetProcessAddress (shModule, name);
#elif defined(UNIX)
  void* shModule = lib;
  pFunc = (void*)dlsym (shModule, name);
#endif

  return pFunc;
}

IWelsVpPlugin::IWelsVpPlugin (int& ret)
  : flag (0)
  , ivp (NULL)
  , hlib (NULL) {
  pfnCreateVpInterface  pCreateVpInterface  = NULL;
  pfnDestroyVpInterface pDestroyVpInterface = NULL;
  iface[0] = iface[1] = NULL;

  hlib  = loadlib();
  if (!hlib)
    goto exit;

  pCreateVpInterface  = (pfnCreateVpInterface)  queryfunc (hlib, ("CreateVpInterface"));
  pDestroyVpInterface = (pfnDestroyVpInterface) queryfunc (hlib, ("DestroyVpInterface"));
  if (!pCreateVpInterface || !pDestroyVpInterface)
    goto exit;

  iface[0] = (void*) pCreateVpInterface;
  iface[1] = (void*) pDestroyVpInterface;
  pCreateVpInterface ((void**)&ivp, WELSVP_INTERFACE_VERION);
  if (!iface)
    goto exit;

  ret = 0;
  return;

exit:
  ret = 1;
}

IWelsVpPlugin::~IWelsVpPlugin() {
  if (hlib) {
    pfnDestroyVpInterface pDestroyVpInterface = (pfnDestroyVpInterface) iface[1];
    if (pDestroyVpInterface)
      pDestroyVpInterface ((void*)ivp, WELSVP_INTERFACE_VERION);

    freelib (hlib);
    hlib = NULL;
  }
}

vResult IWelsVpPlugin::Init (int nType, void* pCfg) {
  vResult ret = vRet_NotSupport;
  if (hlib && nType > 0)
    ret = ivp->Init (nType, pCfg);
  return ret;
}

vResult IWelsVpPlugin::Uninit (int nType) {
  vResult ret = vRet_NotSupport;
  if (hlib && nType > 0)
    ret = ivp->Uninit (nType);
  return ret;
}

vResult IWelsVpPlugin::Flush (int nType) {
  vResult ret = vRet_NotSupport;
  if (hlib && nType > 0)
    ret = ivp->Flush (nType);
  return ret;
}

vResult IWelsVpPlugin::Process (int nType, vPixMap* src, vPixMap* dst) {
  vResult ret = vRet_NotSupport;
  if (hlib && nType > 0)
    ret = ivp->Process (nType, src, dst);
  return ret;
}

vResult IWelsVpPlugin::Get (int nType, void* pParam) {
  vResult ret = vRet_NotSupport;
  if (hlib && nType > 0)
    ret = ivp->Get (nType, pParam);
  return ret;
}

vResult IWelsVpPlugin::Set (int nType, void* pParam) {
  vResult ret = vRet_NotSupport;
  if (hlib && nType > 0)
    ret = ivp->Set (nType, pParam);
  return ret;
}

vResult IWelsVpPlugin::SpecialFeature (int nType, void* pIn, void* pOut) {
  vResult ret = vRet_NotSupport;
  if (hlib && nType > 0)
    ret = ivp->SpecialFeature (nType, pIn, pOut);
  return ret;
}