/*!
 *@page License
 *
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

/**
  * @file codec_api.h
*/

/**
  * @page Overview
  *   * This page is for openh264 codec API usage.
  *   * For how to use the encoder,please refer to page UsageExampleForEncoder
  *   * For how to use the decoder,please refer to page UsageExampleForDecoder
  *   * For more detail about ISVEncoder,please refer to page ISVCEnoder
  *   * For more detail about ISVDecoder,please refer to page ISVCDecoder
*/

/**
  * @page DecoderUsageExample
  *
  * @brief
  *   * An example for using the decoder for Decoding only or Parsing only
  *
  * Step 1:decoder declaration
  * @code
  *
  *  //decoder declaration
  *  ISVCDecoder *pSvcDecoder;
  *  //input: encoded bitstream start position; should include start code prefix
  *	 unsigned char *pBuf =...;
  *  //input: encoded bit stream length; should include the size of start code prefix
  *	 int iSize =...;
  *  //output: [0~2] for Y,U,V buffer for Decoding only
  *	 unsigned char *pData[3] =...;
  *  //in-out: for Decoding only: declare and initialize the output buffer info, this should never co-exist with Parsing only
  *  SBufferInfo sDstBufInfo;
  *	 memset(&sDstBufInfo, 0, sizeof(SBufferInfo));
  *  //in-out: for Parsing only: declare and initialize the output bitstream buffer info for parse only, this should never co-exist with Decoding only
  *  SParserBsInfo sDstParseInfo;
  *      memset(&sDstParseInfo, 0, sizeof(SParserBsInfo));
  *      sDstParseInfo.pDstBuff = new unsigned char[PARSE_SIZE]; //In Parsing only, allocate enough buffer to save transcoded bitstream for a frame
  *
  * @endcode
  *
  *	Step 2:decoder creation
  * @code
  *  CreateDecoder(pSvcDecoder);
  * @endcode
  *
  *	Step 3:declare required parameter, used to differentiate Decoding only and Parsing only
  * @code
  *  SDecodingParam sDecParam = {0};
  *	 sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
  *  //for Parsing only, the assignment is mandatory
  *  sDecParam.bParseOnly = true;
  * @endcode
  *
  *	Step 4:initialize the parameter and decoder context, allocate memory
  * @code
  *  Initialize(&sDecParam);
  * @endcode
  *
  *	Step 5:do actual decoding process in slice level;
  *        this can be done in a loop until data ends
  * @code
  *  //for Decoding only
  *  iRet = DecodeFrame2(pBuf, iSize, pData, &sDstBufInfo);
  *  //for Parsing only
  *  iRet = DecodeParser(pBuf, iSize, &sDstParseInfo);
  *  //decode failed
  *	 If (iRet != 0){
  *	     RequestIDR or something like that.
  *  }
  *  //for Decoding only, pData can be used for render.
  *  if (sDstBufInfo.iBufferStatus==1){
  *      output pData[0], pData[1], pData[2];
  *  }
  * //for Parsing only, sDstParseInfo can be used for, e.g., HW decoding
  *  if (sDstBufInfo.iBufferStatus==1){
  *      Hardware decoding sDstParseInfo;
  *  }
  *  //no-delay decoding can be realized by directly calling decoder again with NULL input, as in the following. In this case, decoder would immediately reconstruct the input data. This can also be used similarly for Parsing only. Consequent decoding error and output indication should also be considered as above.
  *  iRet = DecodeFrame2(NULL, 0, pData, &sDstBufInfo);
  *  judge iRet, sDstBufInfo.iBufferStatus ...
  * @endcode
  *
  *	Step 6:uninitialize the decoder and memory free
  * @code
  *  Uninitialize();
  * @endcode
  *
  *	Step 7:destroy the decoder
  * @code
  *  DestroyDecoder();
  * @endcode
  *
*/

/**
  * @page EncoderUsageExample1
  *
  * @brief
  *  * An example for using encoder with basic parameter
  *
  * Step1:setup encoder
  * @code
  *  int rv = WelsCreateSVCEncoder (&encoder_);
  *  ASSERT_EQ (0, rv);
  *  ASSERT_TRUE (encoder_ != NULL);
  * @endcode
  *
  * Step2:initilize with basic parameter
  * @code
  *  SEncParamBase param;
  *  memset (&param, 0, sizeof (SEncParamBase));
  *  param.iUsageType = usageType;
  *  param.fMaxFrameRate = frameRate;
  *  param.iPicWidth = width;
  *  param.iPicHeight = height;
  *  param.iTargetBitrate = 5000000;
  *  encoder_->Initialize (&param);
  * @endcode
  *
  * Step3:set option, set option during encoding process
  * @code
  *  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &g_LevelSetting);
  *  int videoFormat = videoFormatI420;
  *  encoder_->SetOption (ENCODER_OPTION_DATAFORMAT, &videoFormat);
  * @endcode
  *
  * Step4: encode and  store ouput bistream
  * @code
  *  int frameSize = width * height * 3 / 2;
  *  BufferedData buf;
  *  buf.SetLength (frameSize);
  *  ASSERT_TRUE (buf.Length() == (size_t)frameSize);
  *  SFrameBSInfo info;
  *  memset (&info, 0, sizeof (SFrameBSInfo));
  *  SSourcePicture pic;
  *  memset (&pic, 0, sizeof (SsourcePicture));
  *  pic.iPicWidth = width;
  *  pic.iPicHeight = height;
  *  pic.iColorFormat = videoFormatI420;
  *  pic.iStride[0] = pic.iPicWidth;
  *  pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
  *  pic.pData[0] = buf.data();
  *  pic.pData[1] = pic.pData[0] + width * height;
  *  pic.pData[2] = pic.pData[1] + (width * height >> 2);
  *  for(int num = 0;num<total_num;num++) {
  *     //prepare input data
  *     rv = encoder_->EncodeFrame (&pic, &info);
  *     ASSERT_TRUE (rv == cmResultSuccess);
  *     if (info.eFrameType != videoFrameTypeSkip && cbk != NULL) {
  *      //output bitstream
  *     }
  *  }
  * @endcode
  *
  * Step5:teardown encoder
  * @code
  *  if (encoder_) {
  *      encoder_->Uninitialize();
  *      WelsDestroySVCEncoder (encoder_);
  *  }
  * @endcode
  *
  */

/**
  * @page EncoderUsageExample2
  *
  * @brief
  *     * An example for using the encoder with extension parameter.
  *     * The same operation on Step 1,3,4,5 with Example-1
  *
  * Step 2:initialize with extension parameter
  * @code
  *  SEncParamExt param;
  *  encoder->GetDefaultParams (&param);
  *  param.iUsageType = usageType;
  *  param.fMaxFrameRate = frameRate;
  *  param.iPicWidth = width;
  *  param.iPicHeight = height;
  *  param.iTargetBitrate = 5000000;
  *  param.bEnableDenoise = denoise;
  *  param.iSpatialLayerNum = layers;
  *  //SM_DYN_SLICE don't support multi-thread now
  *  if (sliceMode != SM_SINGLE_SLICE && sliceMode != SM_DYN_SLICE)
  *      param.iMultipleThreadIdc = 2;
  *
  *  for (int i = 0; i < param.iSpatialLayerNum; i++) {
  *      param.sSpatialLayers[i].iVideoWidth = width >> (param.iSpatialLayerNum - 1 - i);
  *      param.sSpatialLayers[i].iVideoHeight = height >> (param.iSpatialLayerNum - 1 - i);
  *      param.sSpatialLayers[i].fFrameRate = frameRate;
  *      param.sSpatialLayers[i].iSpatialBitrate = param.iTargetBitrate;
  *
  *      param.sSpatialLayers[i].sSliceCfg.uiSliceMode = sliceMode;
  *      if (sliceMode == SM_DYN_SLICE) {
  *          param.sSpatialLayers[i].sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 600;
  *          param.uiMaxNalSize = 1500;
  *      }
  *  }
  *  param.iTargetBitrate *= param.iSpatialLayerNum;
  *  encoder_->InitializeExt (&param);
  *  int videoFormat = videoFormatI420;
  *  encoder_->SetOption (ENCODER_OPTION_DATAFORMAT, &videoFormat);
  *
  * @endcode
  */




#ifdef __cplusplus
/**
* @brief Endocder definition
*/
class ISVCEncoder {
 public:
  /**
  * @brief  Initialize the encoder
  * @param  pParam  basic encoder parameter
  * @return CM_RETURN: 0 - success; otherwise - failed;
  */
  virtual int EXTAPI Initialize (const SEncParamBase* pParam) = 0;

  /**
  * @brief  Initilaize encoder by using extension parameters.
  * @param  pParam  extension parameter for encoder
  * @return CM_RETURN: 0 - success; otherwise - failed;
  */
  virtual int EXTAPI InitializeExt (const SEncParamExt* pParam) = 0;

  /**
  * @brief   Get the default extension parameters.
  *          If you want to change some parameters of encoder, firstly you need to get the default encoding parameters,
  *          after that you can change part of parameters you want to.
  * @param   pParam  extension parameter for encoder
  * @return  CM_RETURN: 0 - success; otherwise - failed;
  * */
  virtual int EXTAPI GetDefaultParams (SEncParamExt* pParam) = 0;
  /// uninitialize the encoder
  virtual int EXTAPI Uninitialize() = 0;

  /**
  * @brief Encode one frame
  * @param kpSrcPic the pointer to the source luminance plane
  *        chrominance data:
  *        CbData = kpSrc  +  m_iMaxPicWidth * m_iMaxPicHeight;
  *        CrData = CbData + (m_iMaxPicWidth * m_iMaxPicHeight)/4;
  *        the application calling this interface needs to ensure the data validation between the location
  * @param pBsInfo output bit stream
  * @return  0 - success; otherwise -failed;
  */
  virtual int EXTAPI EncodeFrame (const SSourcePicture* kpSrcPic, SFrameBSInfo* pBsInfo) = 0;

  /**
  * @brief  Encode the parameters from output bit stream
  * @param  pBsInfo output bit stream
  * @return 0 - success; otherwise - failed;
  */
  virtual int EXTAPI EncodeParameterSets (SFrameBSInfo* pBsInfo) = 0;

  /**
  * @brief  Force encoder to encoder frame as IDR if bIDR set as true
  * @param  bIDR true: force encoder to encode frame as IDR frame;false, return 1 and nothing to do
  * @return 0 - success; otherwise - failed;
  */
  virtual int EXTAPI ForceIntraFrame (bool bIDR) = 0;

  /**
  * @brief   Set option for encoder, detail option type, please refer to enumurate ENCODER_OPTION.
  * @param   pOption option for encoder such as InDataFormat, IDRInterval, SVC Encode Param, Frame Rate, Bitrate,...
  * @return  CM_RETURN: 0 - success; otherwise - failed;
  */
  virtual int EXTAPI SetOption (ENCODER_OPTION eOptionId, void* pOption) = 0;

  /**
  * @brief   Set option for encoder, detail option type, please refer to enumurate ENCODER_OPTION.
  * @param   pOption option for encoder such as InDataFormat, IDRInterval, SVC Encode Param, Frame Rate, Bitrate,...
  * @return  CM_RETURN: 0 - success; otherwise - failed;
  */
  virtual int EXTAPI GetOption (ENCODER_OPTION eOptionId, void* pOption) = 0;
  virtual ~ISVCEncoder() {}
};



/**
* @brief Decoder definition
*/
class ISVCDecoder {
 public:

  /**
  * @brief  Initilaize decoder
  * @param  pParam  parameter for decoder
  * @return 0 - success; otherwise - failed;
  */
  virtual long EXTAPI Initialize (const SDecodingParam* pParam) = 0;

  /// Uninitialize the decoder
  virtual long EXTAPI Uninitialize() = 0;

  /**
  * @brief   Decode one frame
  * @param   pSrc the h264 stream to be decoded
  * @param   iSrcLen the length of h264 stream
  * @param   ppDst buffer pointer of decoded data (YUV)
  * @param   pStride output stride
  * @param   iWidth output width
  * @param   iHeight output height
  * @return  0 - success; otherwise -failed;
  */
  virtual DECODING_STATE EXTAPI DecodeFrame (const unsigned char* pSrc,
      const int iSrcLen,
      unsigned char** ppDst,
      int* pStride,
      int& iWidth,
      int& iHeight) = 0;

  /**
  * @brief    For slice level DecodeFrame2() (4 parameters input),
  *           whatever the function return value is, the output data
  *           of I420 format will only be available when pDstInfo->iBufferStatus == 1,.
  *           (e.g., in multi-slice cases, only when the whole picture
  *           is completely reconstructed, this variable would be set equal to 1.)
  * @param   pSrc the h264 stream to be decoded
  * @param   iSrcLen the length of h264 stream
  * @param   ppDst buffer pointer of decoded data (YUV)
  * @param   pDstInfo information provided to API(width, height, etc.)
  * @return  0 - success; otherwise -failed;
  */
  virtual DECODING_STATE EXTAPI DecodeFrame2 (const unsigned char* pSrc,
      const int iSrcLen,
      unsigned char** ppDst,
      SBufferInfo* pDstInfo) = 0;

  /**
  * @brief   This function parse input bitstream only, and rewrite possible SVC syntax to AVC syntax
  * @param   pSrc the h264 stream to be decoded
  * @param   iSrcLen the length of h264 stream
  * @param   pDstInfo bit stream info
  * @return  0 - success; otherwise -failed;
  */
  virtual DECODING_STATE EXTAPI DecodeParser (const unsigned char* pSrc,
      const int iSrcLen,
      SParserBsInfo* pDstInfo) = 0;

  /**
  * @brief   This API does not work for now!! This is for future use to support non-I420 color format output.
  * @param   pSrc the h264 stream to be decoded
  * @param   iSrcLen the length of h264 stream
  * @param   pDst buffer pointer of decoded data (YUV)
  * @param   iDstStride output stride
  * @param   iDstLen bit stream info
  * @param   iWidth output width
  * @param   iHeight output height
  * @param   iColorFormat output color format
  * @return  to do ...
  */
  virtual DECODING_STATE EXTAPI DecodeFrameEx (const unsigned char* pSrc,
      const int iSrcLen,
      unsigned char* pDst,
      int iDstStride,
      int& iDstLen,
      int& iWidth,
      int& iHeight,
      int& iColorFormat) = 0;

  /**
  * @brief   Set option for decoder, detail option type, please refer to enumurate DECODER_OPTION.
  * @param   pOption  option for decoder such as OutDataFormat, Eos Flag, EC method, ...
  * @return  CM_RETURN: 0 - success; otherwise - failed;
  */
  virtual long EXTAPI SetOption (DECODER_OPTION eOptionId, void* pOption) = 0;

  /**
  * @brief   Get option for decoder, detail option type, please refer to enumurate DECODER_OPTION.
  * @param   pOption  option for decoder such as OutDataFormat, Eos Flag, EC method, ...
  * @return  CM_RETURN: 0 - success; otherwise - failed;
  */
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

/** @brief   Create encoder
 *  @param   ppEncoder encoder
 *  @return  0 - success; otherwise - failed;
*/
int  WelsCreateSVCEncoder (ISVCEncoder** ppEncoder);


/** @brief   Destroy encoder
*   @param   pEncoder encoder
 *  @return  void
*/
void WelsDestroySVCEncoder (ISVCEncoder* pEncoder);


/** @brief   Get the capability of decoder
 *  @param   pDecCapability  decoder capability
 *  @return  0 - success; otherwise - failed;
*/
int WelsGetDecoderCapability (SDecoderCapability* pDecCapability);


/** @brief   Create decoder
 *  @param   ppDecoder decoder
 *  @return  0 - success; otherwise - failed;
*/
long WelsCreateDecoder (ISVCDecoder** ppDecoder);


/** @brief   Destroy decoder
 *  @param   pDecoder  decoder
 *  @return  void
*/
void WelsDestroyDecoder (ISVCDecoder* pDecoder);

#ifdef __cplusplus
}
#endif

#endif//WELS_VIDEO_CODEC_SVC_API_H__
