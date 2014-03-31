/*!
 * \copy
 *     Copyright (c)  2004-2013, Cisco Systems
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
 * h264dec.cpp:		Wels Decoder Console Implementation file
 */

#if defined (_WIN32)
#include <windows.h>
#include <tchar.h>
#else
#include <string.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#if defined (ANDROID_NDK)
#include <android/log.h>
#endif
#include "codec_def.h"
#include "codec_app_def.h"
#include "codec_api.h"
#include "read_config.h"
#include "typedefs.h"
#include "measure_time.h"
#include "d3d9_utils.h"
#include "logging.h"


using namespace std;
#if defined(ANDROID_NDK)
#define LOG_TAG "welsdec"
#define LOGI(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define printf LOGI
#define fprintf(a, ...) LOGI(__VA_ARGS__)
#endif
//using namespace WelsDec;

//#define STICK_STREAM_SIZE	// For Demo interfaces test with track file of integrated frames

void H264DecodeInstance (ISVCDecoder* pDecoder, const char* kpH264FileName, const char* kpOuputFileName,
                         int32_t& iWidth, int32_t& iHeight, const char* pOptionFileName) {
  FILE* pH264File	  = NULL;
  FILE* pYuvFile	  = NULL;
  FILE* pOptionFile = NULL;
  int64_t iStart = 0, iEnd = 0, iTotal = 0;
  int32_t iSliceSize;
  int32_t iSliceIndex = 0;
  uint8_t* pBuf = NULL;
  uint8_t uiStartCode[4] = {0, 0, 0, 1};

  void* pData[3] = {NULL};
  uint8_t* pDst[3] = {NULL};
  SBufferInfo sDstBufInfo;

  int32_t iBufPos = 0;
  int32_t iFileSize;
  int32_t i = 0;
  int32_t iLastWidth = 0, iLastHeight = 0;
  int32_t iFrameCount = 0;
  int32_t iEndOfStreamFlag = 0;
  int32_t iColorFormat = videoFormatInternal;

  CUtils cOutputModule;
  double dElapsed = 0;

  if (pDecoder == NULL) return;
  if (kpH264FileName) {
    pH264File = fopen (kpH264FileName, "rb");
    if (pH264File == NULL) {
      fprintf (stderr, "Can not open h264 source file, check its legal path related please..\n");
      return;
    }
    fprintf (stderr, "H264 source file name: %s..\n", kpH264FileName);
  } else {
    fprintf (stderr, "Can not find any h264 bitstream file to read..\n");
    fprintf (stderr, "----------------decoder return------------------------\n");
    return;
  }

  if (kpOuputFileName) {
    pYuvFile = fopen (kpOuputFileName, "wb");
    if (pYuvFile == NULL) {
      fprintf (stderr, "Can not open yuv file to output result of decoding..\n");
      // any options
      //return;	// can let decoder work in quiet mode, no writing any output
    } else
      fprintf (stderr, "Sequence output file name: %s..\n", kpOuputFileName);
  } else {
    fprintf (stderr, "Can not find any output file to write..\n");
    // any options
  }

  if (pOptionFileName) {
    pOptionFile = fopen (pOptionFileName, "wb");
    if (pOptionFile == NULL) {
      fprintf (stderr, "Can not open optional file for write..\n");
    } else
      fprintf (stderr, "Extra optional file: %s..\n", pOptionFileName);
  }

  printf ("------------------------------------------------------\n");

  fseek (pH264File, 0L, SEEK_END);
  iFileSize = ftell (pH264File);
  if (iFileSize <= 0) {
    fprintf (stderr, "Current Bit Stream File is too small, read error!!!!\n");
    goto label_exit;
  }
  fseek (pH264File, 0L, SEEK_SET);

  pBuf = new uint8_t[iFileSize + 4];
  if (pBuf == NULL) {
    fprintf (stderr, "new buffer failed!\n");
    goto label_exit;
  }

  if (fread (pBuf, 1, iFileSize, pH264File) != (uint32_t)iFileSize) {
    fprintf (stderr, "Unable to read whole file\n");
    goto label_exit;
  }

  memcpy (pBuf + iFileSize, &uiStartCode[0], 4); //confirmed_safe_unsafe_usage

  if (pDecoder->SetOption (DECODER_OPTION_DATAFORMAT,  &iColorFormat)) {
    fprintf (stderr, "SetOption() failed, opt_id : %d  ..\n", DECODER_OPTION_DATAFORMAT);
    goto label_exit;
  }

#if defined ( STICK_STREAM_SIZE )
  FILE* fpTrack = fopen ("3.len", "rb");

#endif// STICK_STREAM_SIZE


  while (true) {

    if (iBufPos >= iFileSize) {
      iEndOfStreamFlag = true;
      if (iEndOfStreamFlag)
        pDecoder->SetOption (DECODER_OPTION_END_OF_STREAM, (void*)&iEndOfStreamFlag);
      break;
    }

#if defined ( STICK_STREAM_SIZE )
    if (fpTrack)
      fread (&iSliceSize, 1, sizeof (int32_t), fpTrack);
#else
    for (i = 0; i < iFileSize; i++) {
      if (pBuf[iBufPos + i] == 0 && pBuf[iBufPos + i + 1] == 0 && pBuf[iBufPos + i + 2] == 0 &&
          pBuf[iBufPos + i + 3] == 1 && i > 0) {
        break;
      }
    }
    iSliceSize = i;
#endif

//for coverage test purpose
    int32_t iOutputColorFormat;
    pDecoder->GetOption (DECODER_OPTION_DATAFORMAT, &iOutputColorFormat);
    int32_t iEndOfStreamFlag;
    pDecoder->GetOption (DECODER_OPTION_END_OF_STREAM, &iEndOfStreamFlag);
    int32_t iCurIdrPicId;
    pDecoder->GetOption (DECODER_OPTION_IDR_PIC_ID, &iCurIdrPicId);
    int32_t iFrameNum;
    pDecoder->GetOption (DECODER_OPTION_FRAME_NUM, &iFrameNum);
    int32_t bCurAuContainLtrMarkSeFlag;
    pDecoder->GetOption (DECODER_OPTION_LTR_MARKING_FLAG, &bCurAuContainLtrMarkSeFlag);
    int32_t iFrameNumOfAuMarkedLtr;
    pDecoder->GetOption (DECODER_OPTION_LTR_MARKED_FRAME_NUM, &iFrameNumOfAuMarkedLtr);
    int32_t iFeedbackVclNalInAu;
    pDecoder->GetOption (DECODER_OPTION_VCL_NAL, &iFeedbackVclNalInAu);
    int32_t iFeedbackTidInAu;
    pDecoder->GetOption (DECODER_OPTION_TEMPORAL_ID, &iFeedbackTidInAu);
//~end for

    iStart = WelsTime();
    pData[0] = NULL;
    pData[1] = NULL;
    pData[2] = NULL;
    memset (&sDstBufInfo, 0, sizeof (SBufferInfo));

    pDecoder->DecodeFrame2 (pBuf + iBufPos, iSliceSize, pData, &sDstBufInfo);

    if (sDstBufInfo.iBufferStatus == 1) {
      pDst[0] = (uint8_t*)pData[0];
      pDst[1] = (uint8_t*)pData[1];
      pDst[2] = (uint8_t*)pData[2];
    }
    iEnd	= WelsTime();
    iTotal	+= iEnd - iStart;
    if (sDstBufInfo.iBufferStatus == 1) {
      cOutputModule.Process ((void**)pDst, &sDstBufInfo, pYuvFile);
      iWidth  = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
      iHeight = sDstBufInfo.UsrData.sSystemBuffer.iHeight;

      if (pOptionFile != NULL) {
        if (iWidth != iLastWidth && iHeight != iLastHeight) {
          fwrite (&iFrameCount, sizeof (iFrameCount), 1, pOptionFile);
          fwrite (&iWidth , sizeof (iWidth) , 1, pOptionFile);
          fwrite (&iHeight, sizeof (iHeight), 1, pOptionFile);
          iLastWidth  = iWidth;
          iLastHeight = iHeight;
        }
      }
      ++ iFrameCount;
    }

    iBufPos += iSliceSize;
    ++ iSliceIndex;
  }

  // Get pending last frame
  pData[0] = NULL;
  pData[1] = NULL;
  pData[2] = NULL;
  memset (&sDstBufInfo, 0, sizeof (SBufferInfo));

  pDecoder->DecodeFrame2 (NULL, 0, pData, &sDstBufInfo);
  if (sDstBufInfo.iBufferStatus == 1) {
    pDst[0] = (uint8_t*)pData[0];
    pDst[1] = (uint8_t*)pData[1];
    pDst[2] = (uint8_t*)pData[2];
  }

  if (sDstBufInfo.iBufferStatus == 1) {
    cOutputModule.Process ((void**)pDst, &sDstBufInfo, pYuvFile);
    iWidth  = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
    iHeight = sDstBufInfo.UsrData.sSystemBuffer.iHeight;

    if (pOptionFile != NULL) {
      /* Anyway, we need write in case of final frame decoding */
      fwrite (&iFrameCount, sizeof (iFrameCount), 1, pOptionFile);
      fwrite (&iWidth , sizeof (iWidth) , 1, pOptionFile);
      fwrite (&iHeight, sizeof (iHeight), 1, pOptionFile);
      iLastWidth	= iWidth;
      iLastHeight	= iHeight;
    }
    ++ iFrameCount;
  }


#if defined ( STICK_STREAM_SIZE )
  if (fpTrack) {
    fclose (fpTrack);
    fpTrack = NULL;
  }
#endif// STICK_STREAM_SIZE

  dElapsed = iTotal / 1e6;
  fprintf (stderr, "-------------------------------------------------------\n");
  fprintf (stderr, "iWidth:		%d\nheight:		%d\nFrames:		%d\ndecode time:	%f sec\nFPS:		%f fps\n",
           iWidth, iHeight, iFrameCount, dElapsed, (iFrameCount * 1.0) / dElapsed);
  fprintf (stderr, "-------------------------------------------------------\n");

  // coverity scan uninitial
label_exit:
  if (pBuf) {
    delete[] pBuf;
    pBuf = NULL;
  }
  if (pH264File) {
    fclose (pH264File);
    pH264File = NULL;
  }
  if (pYuvFile) {
    fclose (pYuvFile);
    pYuvFile = NULL;
  }
  if (pOptionFile) {
    fclose (pOptionFile);
    pOptionFile = NULL;
  }
}

#if (defined(ANDROID_NDK)||defined(APPLE_IOS))
int32_t DecMain(int32_t iArgC, char* pArgV[]) {
#else
int32_t main (int32_t iArgC, char* pArgV[]) {
#endif
  ISVCDecoder* pDecoder = NULL;

  SDecodingParam sDecParam = {0};
  string strInputFile (""), strOutputFile (""), strOptionFile ("");

  sDecParam.sVideoProperty.size = sizeof (sDecParam.sVideoProperty);

  if (iArgC < 2) {
    printf ("usage 1: h264dec.exe welsdec.cfg\n");
    printf ("usage 2: h264dec.exe welsdec.264 out.yuv\n");
    printf ("usage 3: h264dec.exe welsdec.264\n");
    return 1;
  } else if (iArgC == 2) {
    if (strstr (pArgV[1], ".cfg")) { // read config file //confirmed_safe_unsafe_usage
      CReadConfig cReadCfg (pArgV[1]);
      string strTag[4];
      string strReconFile ("");

      if (!cReadCfg.ExistFile()) {
        printf ("Specified file: %s not exist, maybe invalid path or parameter settting.\n", cReadCfg.GetFileName().c_str());
        return 1;
      }

      while (!cReadCfg.EndOfFile()) {
        long nRd = cReadCfg.ReadLine (&strTag[0]);
        if (nRd > 0) {
          if (strTag[0].compare ("InputFile") == 0) {
            strInputFile	= strTag[1];
          } else if (strTag[0].compare ("OutputFile") == 0) {
            strOutputFile	= strTag[1];
          } else if (strTag[0].compare ("RestructionFile") == 0) {
            strReconFile	= strTag[1];
            int32_t iLen = strReconFile.length();
            sDecParam.pFileNameRestructed	= new char[iLen + 1];
            if (sDecParam.pFileNameRestructed != NULL) {
              sDecParam.pFileNameRestructed[iLen] = 0;
            }

            strncpy (sDecParam.pFileNameRestructed, strReconFile.c_str(), iLen); //confirmed_safe_unsafe_usage
          } else if (strTag[0].compare ("TargetDQID") == 0) {
            sDecParam.uiTargetDqLayer	= (uint8_t)atol (strTag[1].c_str());
          } else if (strTag[0].compare ("OutColorFormat") == 0) {
            sDecParam.iOutputColorFormat = atol (strTag[1].c_str());
          } else if (strTag[0].compare ("ErrorConcealmentFlag") == 0) {
            sDecParam.uiEcActiveFlag	= (uint8_t)atol (strTag[1].c_str());
          } else if (strTag[0].compare ("CPULoad") == 0) {
            sDecParam.uiCpuLoad	= (uint32_t)atol (strTag[1].c_str());
          } else if (strTag[0].compare ("VideoBitstreamType") == 0) {
            sDecParam.sVideoProperty.eVideoBsType = (VIDEO_BITSTREAM_TYPE)atol (strTag[1].c_str());
          }
        }
      }
      if (strOutputFile.empty()) {
        printf ("No output file specified in configuration file.\n");
        return 1;
      }
    } else if (strstr (pArgV[1],
                       ".264")) { // no output dump yuv file, just try to render the decoded pictures //confirmed_safe_unsafe_usage
      strInputFile	= pArgV[1];
      sDecParam.iOutputColorFormat          = videoFormatI420;
      sDecParam.uiTargetDqLayer	          = (uint8_t) - 1;
      sDecParam.uiEcActiveFlag	          = 1;
      sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
    }
  } else { //iArgC > 2
    strInputFile	= pArgV[1];
    strOutputFile	= pArgV[2];
    sDecParam.iOutputColorFormat	= videoFormatI420;
    sDecParam.uiTargetDqLayer	= (uint8_t) - 1;
    sDecParam.uiEcActiveFlag	= 1;
    sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
    if (iArgC > 3) {
      for (int i = 3; i < iArgC; i++) {
        char* cmd = pArgV[i];

        if (!strcmp (cmd, "-options")) {
          if (i + 1 < iArgC)
            strOptionFile = pArgV[i++];
          else {
            printf ("options file not specified.\n");
            return 1;
          }
        } else if (!strcmp (cmd, "-trace")) {
          if (i + 1 < iArgC)
            WelsStderrSetTraceLevel (atoi (pArgV[i++]));
          else {
            printf ("trace level not specified.\n");
            return 1;
          }
        }
      }
    }

    if (strOutputFile.empty()) {
      printf ("No output file specified in configuration file.\n");
      return 1;
    }
  }

  if (strInputFile.empty()) {
    printf ("No input file specified in configuration file.\n");
    return 1;
  }




  if (WelsCreateDecoder (&pDecoder)  || (NULL == pDecoder)) {
    printf ("Create Decoder failed.\n");
    return 1;
  }

  if (pDecoder->Initialize (&sDecParam)) {
    printf ("Decoder initialization failed.\n");
    return 1;
  }


  int32_t iWidth = 0;
  int32_t iHeight = 0;


  H264DecodeInstance (pDecoder, strInputFile.c_str(), !strOutputFile.empty() ? strOutputFile.c_str() : NULL, iWidth, iHeight,
                      (!strOptionFile.empty() ? strOptionFile.c_str() : NULL));

  if (sDecParam.pFileNameRestructed != NULL) {
    delete []sDecParam.pFileNameRestructed;
    sDecParam.pFileNameRestructed = NULL;
  }

  if (pDecoder) {
    pDecoder->Uninitialize();

    WelsDestroyDecoder (pDecoder);
  }

  return 0;
}
