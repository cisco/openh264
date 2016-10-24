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
#include "welsencUtil.cpp"

/****************************************************************************
 * main:
 ****************************************************************************/
#if defined(ANDROID_NDK) || defined(APPLE_IOS) || defined (WINDOWS_PHONE)
extern "C" int EncMain (int argc, char** argv)
#else
int main (int argc, char** argv)
#endif
{
  ISVCEncoder* pSVCEncoder = NULL;
  int iRet = 0;

#ifdef _MSC_VER
  _setmode (_fileno (stdin), _O_BINARY);  /* thanks to Marcoss Morais <morais at dee.ufcg.edu.br> */
  _setmode (_fileno (stdout), _O_BINARY);

  // remove the LOCK_TO_SINGLE_CORE micro, user need to enable it with manual
  // LockToSingleCore();
#endif

  /* Control-C handler */
  signal (SIGINT, SigIntHandler);

  iRet = CreateSVCEncHandle (&pSVCEncoder);
  if (iRet) {
    cout << "WelsCreateSVCEncoder() failed!!" << endl;
    goto exit;
  }

  if (argc < 2) {
    goto exit;
  } else {
    if (!strstr (argv[1], ".cfg")) { // check configuration type (like .cfg?)
      if (argc > 2) {
        iRet = ProcessEncoding (pSVCEncoder, argc, argv, false);
        if (iRet != 0)
          goto exit;
      } else if (argc == 2 && ! strcmp (argv[1], "-h"))
        PrintHelp();
      else {
        cout << "You specified pCommand is invalid!!" << endl;
        goto exit;
      }
    } else {
      iRet = ProcessEncoding (pSVCEncoder, argc, argv, true);
      if (iRet > 0)
        goto exit;
    }
  }

  DestroySVCEncHandle (pSVCEncoder);
  return 0;

exit:
  DestroySVCEncHandle (pSVCEncoder);
  PrintHelp();
  return 1;
}
