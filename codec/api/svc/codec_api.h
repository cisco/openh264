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

#ifndef WELS_VIDEO_CODEC_SVC_API_H__
#define WELS_VIDEO_CODEC_SVC_API_H__

#ifndef __cplusplus
#ifdef _MSC_VER
typedef unsigned char bool;
#else
#include <stdbool.h>
#endif
#endif

#include "codec_app_def.h"
#include "codec_def.h"

#if defined(_WIN32) || defined(__cdecl)
#define EXTAPI __cdecl
#else
#define EXTAPI
#endif

#ifdef __cplusplus
class ISVCEncoder {
 public:
  /*
   * return: CM_RETURN: 0 - success; otherwise - failed;
   */
  virtual int EXTAPI Initialize (const SEncParamBase* pParam) = 0;
  virtual int EXTAPI InitializeExt (const SEncParamExt* pParam) = 0;

  virtual int EXTAPI GetDefaultParams (SEncParamExt* pParam) = 0;

  virtual int EXTAPI Uninitialize() = 0;

  /*
   * return: 0 - success; otherwise -failed;
   */
  virtual int EXTAPI EncodeFrame (const SSourcePicture* kpSrcPic, SFrameBSInfo* pBsInfo) = 0;
  /*
   * return: 0 - success; otherwise - failed;
   */
  virtual int EXTAPI EncodeParameterSets (SFrameBSInfo* pBsInfo) = 0;
  /*
   * return: 0 - success; otherwise - failed;
   */
  virtual int EXTAPI ForceIntraFrame (bool bIDR) = 0;

  /************************************************************************
   * InDataFormat, IDRInterval, SVC Encode Param, Frame Rate, Bitrate,..
   ************************************************************************/
  /*
   * return: CM_RETURN: 0 - success; otherwise - failed;
   */
  virtual int EXTAPI SetOption (ENCODER_OPTION eOptionId, void* pOption) = 0;
  virtual int EXTAPI GetOption (ENCODER_OPTION eOptionId, void* pOption) = 0;
  virtual ~ISVCEncoder() {}
};

class ISVCDecoder {
 public:
  virtual long EXTAPI Initialize (const SDecodingParam* pParam) = 0;
  virtual long EXTAPI Uninitialize() = 0;

  virtual DECODING_STATE EXTAPI DecodeFrame (const unsigned char* pSrc,
      const int iSrcLen,
      unsigned char** ppDst,
      int* pStride,
      int& iWidth,
      int& iHeight) = 0;
  /*
   * return: 0 - success; otherwise -failed;
   */
  virtual DECODING_STATE EXTAPI DecodeFrame2 (const unsigned char* pSrc,
      const int iSrcLen,
      unsigned char** ppDst,
      SBufferInfo* pDstInfo) = 0;

  /*
   * This function parse input bitstream only, and rewrite possible SVC syntax to AVC syntax
   * return: 0 - success; otherwise -failed;
   */
  virtual DECODING_STATE EXTAPI DecodeParser (const unsigned char* pSrc,
      const int iSrcLen,
      SParserBsInfo* pDstInfo) = 0;

  /*
   *  this API does not work for now!! This is for future use to support non-I420 color format output.
   */
  virtual DECODING_STATE EXTAPI DecodeFrameEx (const unsigned char* pSrc,
      const int iSrcLen,
      unsigned char* pDst,
      int iDstStride,
      int& iDstLen,
      int& iWidth,
      int& iHeight,
      int& iColorFormat) = 0;

  /*************************************************************************
   * OutDataFormat, Eos Flag, EC method, ...
   *************************************************************************/
  virtual long EXTAPI SetOption (DECODER_OPTION eOptionId, void* pOption) = 0;
  virtual long EXTAPI GetOption (DECODER_OPTION eOptionId, void* pOption) = 0;
  virtual ~ISVCDecoder() {}
};


extern "C"
{
#else

typedef struct ISVCEncoderVtbl ISVCEncoderVtbl;
typedef const ISVCEncoderVtbl* ISVCEncoder;
struct ISVCEncoderVtbl {

int (*Initialize) (ISVCEncoder*, const SEncParamBase* pParam);
int (*InitializeExt) (ISVCEncoder*, const SEncParamExt* pParam);

int (*GetDefaultParams) (ISVCEncoder*, SEncParamExt* pParam);

int (*Uninitialize) (ISVCEncoder*);

int (*EncodeFrame) (ISVCEncoder*, const SSourcePicture* kpSrcPic, SFrameBSInfo* pBsInfo);
int (*EncodeParameterSets) (ISVCEncoder*, SFrameBSInfo* pBsInfo);

int (*ForceIntraFrame) (ISVCEncoder*, bool bIDR);

int (*SetOption) (ISVCEncoder*, ENCODER_OPTION eOptionId, void* pOption);
int (*GetOption) (ISVCEncoder*, ENCODER_OPTION eOptionId, void* pOption);
};

typedef struct ISVCDecoderVtbl ISVCDecoderVtbl;
typedef const ISVCDecoderVtbl* ISVCDecoder;
struct ISVCDecoderVtbl {
long (*Initialize) (ISVCDecoder*, const SDecodingParam* pParam);
long (*Uninitialize) (ISVCDecoder*);

DECODING_STATE (*DecodeFrame) (ISVCDecoder*, const unsigned char* pSrc,
                               const int iSrcLen,
                               unsigned char** ppDst,
                               int* pStride,
                               int* iWidth,
                               int* iHeight);

DECODING_STATE (*DecodeFrame2) (ISVCDecoder*, const unsigned char* pSrc,
                                const int iSrcLen,
                                unsigned char** ppDst,
                                SBufferInfo* pDstInfo);

DECODING_STATE (*DecodeParser) (ISVCDecoder*, const unsigned char* pSrc,
                                const int iSrcLen,
                                SParserBsInfo* pDstInfo);

DECODING_STATE (*DecodeFrameEx) (ISVCDecoder*, const unsigned char* pSrc,
                                 const int iSrcLen,
                                 unsigned char* pDst,
                                 int iDstStride,
                                 int* iDstLen,
                                 int* iWidth,
                                 int* iHeight,
                                 int* iColorFormat);

long (*SetOption) (ISVCDecoder*, DECODER_OPTION eOptionId, void* pOption);
long (*GetOption) (ISVCDecoder*, DECODER_OPTION eOptionId, void* pOption);
};
#endif

typedef void (*WelsTraceCallback) (void* ctx, int level, const char* string);

int  WelsCreateSVCEncoder (ISVCEncoder** ppEncoder);
void WelsDestroySVCEncoder (ISVCEncoder* pEncoder);

int WelsGetDecoderCapability (SDecoderCapability* pDecCapability);
long WelsCreateDecoder (ISVCDecoder** ppDecoder);
void WelsDestroyDecoder (ISVCDecoder* pDecoder);

#ifdef __cplusplus
}
#endif

#endif//WELS_VIDEO_CODEC_SVC_API_H__
