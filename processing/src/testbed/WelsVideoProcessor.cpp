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

// WelsVideoProcessor.cpp : Defines the entry point for the console application.
//

#include <tchar.h>
#include "stdafx.h"
#include "wels_process.h"

//////////////////////////////////////////////////////////////////////////
typedef struct {
  FILE*    srcfile;
  FILE*    dstfile;
  vPixMap  src;
  vPixMap  dst;
  vMethods methods[vMethods_Mask];
} VpConfigure;
//////////////////////////////////////////////////////////////////////////

void PrintHelp (TCHAR* strAppName, TCHAR* strError) {
  if (strError) {
    _tprintf (_T ("Error: %s\n"), strError);
  } else {
    _tprintf (_T ("Welsvp Sample Console\n"));
  }

  _tprintf (_T ("Usage1: %s [Options] -i InputFile -o OutputFile -w 640 -h 480\n"), strAppName);
  _tprintf (_T ("Options: \n"));

  _tprintf (_T ("   [-sx  x]       - cropX  of src video (def: 0)\n"));
  _tprintf (_T ("   [-sy  y]       - cropY  of src video (def: 0)\n"));
  _tprintf (_T ("   [-sw  width]   - cropW  of src video (def: width)\n"));
  _tprintf (_T ("   [-sh  height]  - cropH  of src video (def: height)\n"));
  _tprintf (_T ("   [-scc format]  - format (FourCC) of src video (def: support yv12|yuy2|rgb3|rgb4)\n"));

  _tprintf (_T ("   [-dx  x]       - cropX  of dst video (def: 0)\n"));
  _tprintf (_T ("   [-dy  y]       - cropY  of dst video (def: 0)\n"));
  _tprintf (_T ("   [-dw  width]   - cropW  of dst video (def: width)\n"));
  _tprintf (_T ("   [-dh  height]  - cropH  of dst video (def: height)\n"));
  _tprintf (_T ("   [-dcc format]  - format (FourCC) of dst video (def: nv12. support nv12|yuy2)\n"));

  _tprintf (_T ("   Video Processing Algorithms\n"));
  _tprintf (_T ("   [-vaa]         - enable video analysis algorithm \n"));
  _tprintf (_T ("   [-bgd]         - enable background detection algorithm \n"));
  _tprintf (_T ("   [-scd]         - enable scene change detection algorithm \n"));
  _tprintf (_T ("   [-denoise]     - enable denoise algorithm \n"));
  _tprintf (_T ("   [-downsample]  - enable downsample algorithm \n"));

  _tprintf (_T ("   [-n frames]    - number of frames to VP process\n\n"));
  _tprintf (_T ("\n"));

  _tprintf (_T ("Usage2: %s -sw 640 -sh 480 -scc rgb3 -dw 320 -dh 240 -dcc i420 -denoise -vaa -i in.rgb -o out.yuv\n"),
            strAppName);
  _tprintf (_T ("\n"));
}

vVideoFormat Str2FourCC (TCHAR* strInput) {
  vVideoFormat format = vVideoFormat_I420; // as default

  if (0 == _tcscmp (strInput, _T ("yv12"))) {
    format = vVideoFormat_YV12;
  } else if (0 == _tcscmp (strInput, _T ("i420"))) {
    format = vVideoFormat_I420;
  } else if (0 == _tcscmp (strInput, _T ("rgb24"))) {
    format = vVideoFormat_RGB24;
  } else if (0 == _tcscmp (strInput, _T ("rgb32"))) {
    format = vVideoFormat_RGB32;
  } else if (0 == _tcscmp (strInput, _T ("yuy2"))) {
    format = vVideoFormat_YUY2;
  } else if (0 == _tcscmp (strInput, _T ("nv12"))) {
    format = vVideoFormat_NV12;
  }

  return format;
}

int ReadFile (vPixMap& pixmap, FILE* fp) {
  int ret = 0;

  int size = pixmap.Rect.width * pixmap.Rect.height;
  switch (pixmap.eFormat) {
  case vVideoFormat_I420:
  case vVideoFormat_YV12: {
    if (fread (pixmap.pPixel[0], pixmap.nSizeInBits / 8, (3 * size) >> 1, fp) <= 0)
      ret = 1;
  }
  break;
  case vVideoFormat_YUY2: {
    if (fread (pixmap.pPixel[0], pixmap.nSizeInBits / 8, 2 * size, fp) <= 0)
      ret = 1;
  }
  break;
  case vVideoFormat_RGB24: {
    if (fread (pixmap.pPixel[0], pixmap.nSizeInBits / 8, 3 * size, fp) <= 0)
      ret = 1;
  }
  break;
  case vVideoFormat_RGB32: {
    if (fread (pixmap.pPixel[0], pixmap.nSizeInBits / 8, 4 * size, fp) <= 0)
      ret = 1;
  }
  break;
  default:
    ret = 1;
    break;
  }
  return ret;
}

int WriteFile (vPixMap& pixmap, FILE* fp) {
  int ret = 0;
  int size = pixmap.Rect.width * pixmap.Rect.height;
  switch (pixmap.eFormat) {
  case vVideoFormat_I420:
  case vVideoFormat_YV12: {
    if (fwrite (pixmap.pPixel[0], pixmap.nSizeInBits / 8, (3 * size) >> 1, fp) <= 0)
      ret = 1;
  }
  break;
  case vVideoFormat_YUY2: {
    if (fwrite (pixmap.pPixel[0], pixmap.nSizeInBits / 8, 2 * size, fp) <= 0)
      ret = 1;
  }
  break;
  case vVideoFormat_RGB24: {
    if (fwrite (pixmap.pPixel[0], pixmap.nSizeInBits / 8, 3 * size, fp) <= 0)
      ret = 1;
  }
  break;
  case vVideoFormat_RGB32: {
    if (fwrite (pixmap.pPixel[0], pixmap.nSizeInBits / 8, 4 * size, fp) <= 0)
      ret = 1;
  }
  break;
  default:
    ret = 1;
    break;
  }
  return ret;
}


int AllocPixMap (vPixMap& pixmap) {
  pixmap.nSizeInBits = sizeof (unsigned char) * 8;

  switch (pixmap.eFormat) {
  case vVideoFormat_I420:
  case vVideoFormat_YV12: {
    pixmap.nStride[0]  = pixmap.Rect.width;
    pixmap.nStride[1]  = pixmap.nStride[2]  = pixmap.Rect.width / 2;
    pixmap.pPixel[0]   = new void* [pixmap.nStride[0] * pixmap.Rect.height * pixmap.nSizeInBits / 8 * 3 / 2];
    pixmap.pPixel[1]   = (unsigned char*)pixmap.pPixel[0] + pixmap.nStride[0] * pixmap.Rect.height * pixmap.nSizeInBits / 8;
    pixmap.pPixel[2]   = (unsigned char*)pixmap.pPixel[0] + pixmap.nStride[0] * pixmap.Rect.height * pixmap.nSizeInBits /
                         8 * 5 / 4;
  }
  break;

  case vVideoFormat_YUY2: {
    pixmap.nStride[0]  = pixmap.nStride[1]  = pixmap.nStride[2]  = pixmap.Rect.width * 2;
    pixmap.pPixel[0]   = new void* [pixmap.nStride[0] * pixmap.Rect.height * pixmap.nSizeInBits / 8 * 2];
    pixmap.pPixel[1]   = pixmap.pPixel[2] = NULL;
  }
  break;

  case vVideoFormat_RGB24: {
    pixmap.nStride[0]  = pixmap.nStride[1]  = pixmap.nStride[2]  = pixmap.Rect.width * 3;
    pixmap.pPixel[0]   = new void* [pixmap.nStride[0] * pixmap.Rect.height * pixmap.nSizeInBits / 8 * 3];
    pixmap.pPixel[1]   = pixmap.pPixel[2] = NULL;
  }
  break;

  case vVideoFormat_RGB32: {
    pixmap.nStride[0]  = pixmap.nStride[1]  = pixmap.nStride[2]  = pixmap.Rect.width * 4;
    pixmap.pPixel[0]   = new void* [pixmap.nStride[0] * pixmap.Rect.height * pixmap.nSizeInBits / 8 * 4];
    pixmap.pPixel[1]   = pixmap.pPixel[2] = NULL;
  }
  break;

  default:
    return 1;
  }

  return (pixmap.pPixel[0]) ? 0 : 1;
}

void FreePixMap (vPixMap& pixmap) {
  if (pixmap.pPixel[0]) {
    free (pixmap.pPixel[0]);
    pixmap.pPixel[0] = pixmap.pPixel[1] = pixmap.pPixel[2] = NULL;
  }
}

int InitResource (TCHAR* strAppName, VpConfigure& cfg) {
  if (0 == cfg.srcfile) {
    PrintHelp (strAppName, _T ("Source file can not found!\n"));
    goto exit;
  };

  if (0 == cfg.dstfile) {
    PrintHelp (strAppName, _T ("Destination file name not found"));
    goto exit;
  };

  if (cfg.dst.Rect.width == 0)
    cfg.dst.Rect.width = cfg.src.Rect.width;
  if (cfg.dst.Rect.height == 0)
    cfg.dst.Rect.height = cfg.src.Rect.height;

  cfg.methods[vMethods_ColorSpaceConvert] = vMethods_ColorSpaceConvert;

  if (AllocPixMap (cfg.src))
    goto exit;

  if (AllocPixMap (cfg.dst))
    goto exit;

  return 0;

exit:
  FreePixMap (cfg.src);
  FreePixMap (cfg.dst);
  return 1;
}

int ParseCommond (TCHAR* strInput[], int nArgNum, VpConfigure& cfg) {
  if (nArgNum < 9) {
    PrintHelp (strInput[0], _T ("please specify all necessary parameters!"));
    return 1;
  }

  int width = 0, height = 0;
  for (int i = 1; i < nArgNum; i++) {
    if (strInput[i]) {
      if (0 == _tcscmp (strInput[i], _T ("-i"))) {
        i++;
        _tfopen_s (&cfg.srcfile, strInput[i], _T ("rb"));
      } else if (0 == _tcscmp (strInput[i], _T ("-o"))) {
        i++;
        _tfopen_s (&cfg.dstfile, strInput[i], _T ("wb"));
      } else if (0 == _tcscmp (strInput[i], _T ("-w"))) {
        i++;
        _stscanf_s (strInput[i], _T ("%d"), &width);
      } else if (0 == _tcscmp (strInput[i], _T ("-h"))) {
        i++;
        _stscanf_s (strInput[i], _T ("%d"), &height);
      }
      //-----------------------------------------------------------------------------------
      else if (0 == _tcscmp (strInput[i], _T ("-sx"))) {
        i++;
        _stscanf_s (strInput[i], _T ("%hd"), &cfg.src.Rect.top);
      } else if (0 == _tcscmp (strInput[i], _T ("-sy"))) {
        i++;
        _stscanf_s (strInput[i], _T ("%hd"), &cfg.src.Rect.left);
      } else if (0 == _tcscmp (strInput[i], _T ("-sw"))) {
        i++;
        TCHAR* a = strInput[i];
        _stscanf_s (strInput[i], _T ("%hd"), &cfg.src.Rect.width);
      } else if (0 == _tcscmp (strInput[i], _T ("-sh"))) {
        i++;
        _stscanf_s (strInput[i], _T ("%hd"), &cfg.src.Rect.height);
      } else if (0 == _tcscmp (strInput[i], _T ("-scc"))) {
        i++;
        cfg.src.eFormat = Str2FourCC (strInput[i]);
      }
      //-----------------------------------------------------------------------------------
      else if (0 == _tcscmp (strInput[i], _T ("-dx"))) {
        i++;
        _stscanf_s (strInput[i], _T ("%hd"), &cfg.dst.Rect.top);
      } else if (0 == _tcscmp (strInput[i], _T ("-dy"))) {
        i++;
        _stscanf_s (strInput[i], _T ("%hd"), &cfg.dst.Rect.left);
      } else if (0 == _tcscmp (strInput[i], _T ("-dw"))) {
        i++;
        _stscanf_s (strInput[i], _T ("%hd"), &cfg.dst.Rect.width);
      } else if (0 == _tcscmp (strInput[i], _T ("-dh"))) {
        i++;
        _stscanf_s (strInput[i], _T ("%hd"), &cfg.dst.Rect.height);
      } else if (0 == _tcscmp (strInput[i], _T ("-dcc"))) {
        i++;
        cfg.dst.eFormat = Str2FourCC (strInput[i]);
      }
      //-----------------------------------------------------------------------------------
      else if (0 == _tcscmp (strInput[i], _T ("-denoise"))) {
        cfg.methods[vMethods_Denoise] = vMethods_Denoise;
      } else if (0 == _tcscmp (strInput[i], _T ("-scd"))) {
        cfg.methods[vMethods_SceneChangeDetection] = vMethods_SceneChangeDetection;
      } else if (0 == _tcscmp (strInput[i], _T ("-downsample"))) {
      } else if (0 == _tcscmp (strInput[i], _T ("-vaa"))) {
      } else if (0 == _tcscmp (strInput[i], _T ("-bgd"))) {
      } else if (0 == _tcscmp (strInput[i], _T ("-aq"))) {
      }
    }
  }

  if (cfg.src.Rect.width == 0)  cfg.src.Rect.width  = width;
  if (cfg.src.Rect.height == 0) cfg.src.Rect.height = height;
  if (cfg.dst.Rect.width == 0)  cfg.dst.Rect.width  = width;
  if (cfg.dst.Rect.height == 0) cfg.dst.Rect.height = height;

  return InitResource (strInput[0], cfg);
}

int _tmain (int argc, _TCHAR* argv[]) {
  int   ret           = 0;
  VpConfigure cfg     = {0};
  IWelsVpPlugin* pVpp = NULL;

  ret = ParseCommond (argv, argc, cfg);
  if (ret)
    goto exit;

  pVpp = new IWelsVpPlugin (ret);
  if (pVpp && ret == 0) {
    vResult vret = vRet_Success;
    while (1) {
      if (feof (cfg.srcfile))
        break;

      if (ReadFile (cfg.src, cfg.srcfile))
        break;

      vret = pVpp->Process (cfg.methods[vMethods_ColorSpaceConvert], &cfg.src, &cfg.dst);
      if (vret)
        break;

      vret = pVpp->Process (cfg.methods[vMethods_Denoise], &cfg.dst, NULL);
      if (vret)
        break;

      if (WriteFile (cfg.dst, cfg.dstfile))
        break;
    }
  }

exit:

  if (pVpp) {
    delete pVpp;
    pVpp = NULL;
  }

  if (cfg.srcfile)
    fclose (cfg.srcfile);
  if (cfg.dstfile)
    fclose (cfg.dstfile);

  FreePixMap (cfg.src);
  FreePixMap (cfg.dst);

  return 0;
}

