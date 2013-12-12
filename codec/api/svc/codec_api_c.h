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

#ifndef WELS_VIDEO_CODEC_API_C_H__
#define WELS_VIDEO_CODEC_API_C_H__

#include "codec_app_def.h"
#include "codec_def.h"

#ifdef __cplusplus
extern "C" {
class ISVCEncoder;
class ISVCDecoder;
#else
typedef struct ISVCEncoder ISVCEncoder;
typedef struct ISVCDecoder ISVCDecoder;
#endif


int  CreateSVCEncoder (ISVCEncoder** ppEncoder);
void DestroySVCEncoder (ISVCEncoder* pEncoder);

long CreateDecoder (ISVCDecoder** ppDecoder);
void DestroyDecoder (ISVCDecoder* pDecoder);


int SVCEncoder_Initialize(ISVCEncoder* pEncoder, SVCEncodingParam* pParam, const INIT_TYPE kiInitType);
int SVCEncoder_Initialize2(ISVCEncoder* pEncoder, void* pParam, const INIT_TYPE kiInitType);

int SVCEncoder_Uninitialize(ISVCEncoder* pEncoder);

int SVCEncoder_EncodeFrame(ISVCEncoder* pEncoder, const unsigned char* kpSrc, SFrameBSInfo* pBsInfo);
int SVCEncoder_EncodeFrame2(ISVCEncoder* pEncoder, const SSourcePicture** kppSrcPicList, int nSrcPicNum, SFrameBSInfo* pBsInfo);
int SVCEncoder_EncodeParameterSets(ISVCEncoder* pEncoder, SFrameBSInfo* pBsInfo);

int SVCEncoder_PauseFrame(ISVCEncoder* pEncoder, const unsigned char* kpSrc, SFrameBSInfo* pBsInfo);
int SVCEncoder_ForceIntraFrame(ISVCEncoder* pEncoder, bool bIDR);

int SVCEncoder_SetOption(ISVCEncoder* pEncoder, ENCODER_OPTION eOptionId, void* pOption);
int SVCEncoder_GetOption(ISVCEncoder* pEncoder, ENCODER_OPTION eOptionId, void* pOption);


long SVCDecoder_Initialize(ISVCDecoder* pDecoder, void* pParam, const INIT_TYPE iInitType);
long SVCDecoder_Uninitialize(ISVCDecoder* pDecoder);

DECODING_STATE SVCDecoder_DecodeFrame(ISVCDecoder* pDecoder, const unsigned char* pSrc, const int iSrcLen, unsigned char** ppDst, int* pStride, int* iWidth, int* iHeight);

DECODING_STATE SVCDecoder_DecodeFrame2(ISVCDecoder* pDecoder, const unsigned char* pSrc, const int iSrcLen, void** ppDst, SBufferInfo* pDstInfo);

DECODING_STATE SVCDecoder_DecodeFrameEx(ISVCDecoder* pDecoder, const unsigned char* pSrc, const int iSrcLen, unsigned char* pDst, int iDstStride, int* iDstLen, int* iWidth, int* iHeight, int* iColorFormat);

long SVCDecoder_SetOption(ISVCDecoder* pDecoder, DECODER_OPTION eOptionId, void* pOption);
long SVCDecoder_GetOption(ISVCDecoder* pDecoder, DECODER_OPTION eOptionId, void* pOption);

#ifdef __cplusplus
}
#endif

#endif//WELS_VIDEO_CODEC_API_C_H__
