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

#include "codec_app_def.h"
#include "codec_def.h"

class ISVCEncoder {
 public:
  /*
   * return: CM_RETURN: 0 - success; otherwise - failed;
   */
  virtual int Initialize (SVCEncodingParam* pParam, const INIT_TYPE kiInitType = INIT_TYPE_PARAMETER_BASED) = 0;
  virtual int Initialize2 (void* pParam, const INIT_TYPE kiInitType = INIT_TYPE_CONFIG_BASED) = 0;

  virtual int Uninitialize() = 0;

  /*
   * return: EVideoFrameType [IDR: videoFrameTypeIDR; P: videoFrameTypeP; ERROR: videoFrameTypeInvalid]
   */
  virtual int EncodeFrame (const unsigned char* kpSrc, SFrameBSInfo* pBsInfo) = 0;
  virtual int EncodeFrame2 (const SSourcePicture**   kppSrcPicList, int nSrcPicNum, SFrameBSInfo* pBsInfo) = 0;

  /*
   * return: 0 - success; otherwise - failed;
   */
  virtual int EncodeParameterSets (SFrameBSInfo* pBsInfo) = 0;

  /*
   * return: 0 - success; otherwise - failed;
   */
  virtual int PauseFrame (const unsigned char* kpSrc, SFrameBSInfo* pBsInfo) = 0;

  /*
   * return: 0 - success; otherwise - failed;
   */
  virtual int ForceIntraFrame (bool bIDR) = 0;

  /************************************************************************
   * InDataFormat, IDRInterval, SVC Encode Param, Frame Rate, Bitrate,..
   ************************************************************************/
  /*
   * return: CM_RETURN: 0 - success; otherwise - failed;
   */
  virtual int SetOption (ENCODER_OPTION eOptionId, void* pOption) = 0;
  virtual int GetOption (ENCODER_OPTION eOptionId, void* pOption) = 0;
};

class ISVCDecoder {
 public:
  virtual long Initialize (void* pParam, const INIT_TYPE iInitType) = 0;
  virtual long Uninitialize() = 0;

  virtual DECODING_STATE DecodeFrame (const unsigned char* pSrc,
                                      const int iSrcLen,
                                      unsigned char** ppDst,
                                      int* pStride,
                                      int& iWidth,
                                      int& iHeight) = 0;

  /*
   *  src must be 4 byte aligned,   recommend 16 byte aligned.    the available src size must be multiple of 4.
   */
  virtual DECODING_STATE DecodeFrame2 (const unsigned char* pSrc,
                                      const int iSrcLen,
                                      void** ppDst,
                                      SBufferInfo* pDstInfo) = 0;

  /*
   *  src must be 4 byte aligned,   recommend 16 byte aligned.    the available src size must be multiple of 4.
   *  this API does not work for now!! This is for future use to support non-I420 color format output.
   */
  virtual DECODING_STATE DecodeFrameEx (const unsigned char* pSrc,
                                        const int iSrcLen,
                                        unsigned char* pDst,
                                        int iDstStride,
                                        int& iDstLen,
                                        int& iWidth,
                                        int& iHeight,
                                        int& iColorFormat) = 0;

  /*************************************************************************
   * OutDataFormat
   *************************************************************************/
  virtual long SetOption (DECODER_OPTION eOptionId, void* pOption) = 0;
  virtual long GetOption (DECODER_OPTION eOptionId, void* pOption) = 0;
};


extern "C"
{
  int  CreateSVCEncoder (ISVCEncoder** ppEncoder);
  void DestroySVCEncoder (ISVCEncoder* pEncoder);

  long CreateDecoder (ISVCDecoder** ppDecoder);
  void DestroyDecoder (ISVCDecoder* pDecoder);
}

#endif//WELS_VIDEO_CODEC_SVC_API_H__
