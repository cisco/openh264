// Codec_UT_RTComponent.cpp
#include <windows.h>
#include <iostream>
#include "Codec_UT_RTComponent.h"

using namespace Codec_UT_RTComponent;
using namespace Platform;
using namespace Windows;
using namespace Windows::Storage;

typedef int (*pfTestAllCases) (int argc, char** argv);

CodecUTTest::CodecUTTest() {
}

int CodecUTTest::TestAllCases() {
  int   argc = 2;
  int   iRet = 0;
  char* argv[6];

  HMODULE         phTestCasesDllHandler = NULL;
  pfTestAllCases  pUTHandler            = NULL;
  LPCWSTR         cTestCasesDllDLLName  = L"ut.dll";

  // output xml file location
  char OutputPath[256] = { 0 };
  Windows::Storage::StorageFolder^ OutputLocation;
  Platform::String^ OutputLocationPath;

  OutputLocation = ApplicationData::Current->LocalFolder;
  OutputLocationPath = Platform::String::Concat (OutputLocation->Path, "\\Shared\\");
  const wchar_t* pWcharOutputFile = OutputLocationPath->Data();

  int size = wcslen (pWcharOutputFile);
  OutputPath[size] = 0;
  for (int y = 0; y < size; y++) {
    OutputPath[y] = (char)pWcharOutputFile[y];
  }

  // load dynamic library
  phTestCasesDllHandler = LoadPackagedLibrary (cTestCasesDllDLLName, 0);
  DWORD dw = GetLastError();
  if (NULL == phTestCasesDllHandler) {
    std::cout << "failed to load dll,error code is : " << dw << std::endl;
    return 1;
  }

  pUTHandler = (pfTestAllCases)GetProcAddress (phTestCasesDllHandler, "CodecUtMain");

  if (NULL == pUTHandler) {
    std::cout << "failed to load function" << std::endl;
    return 2;
  }

  // test all cases
  argv[0] = "CodecUTAPP";
  argv[1] = OutputPath;
  argc    = 2;

  iRet = pUTHandler(argc, argv);

  return iRet;
}
