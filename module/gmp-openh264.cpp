/*!
 * \copy
 *     Copyright (c)  2009-2014, Cisco Systems
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
 *
 *************************************************************************************
 */

#include <stdint.h>
#include <time.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <memory>
#include <assert.h>
#include <limits.h>

#include "gmp-platform.h"
#include "gmp-video-host.h"
#include "gmp-video-encode.h"
#include "gmp-video-decode.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"

#include "codec_def.h"
#include "codec_app_def.h"
#include "codec_api.h"

#include "task_utils.h"

#if defined(_MSC_VER)
#define PUBLIC_FUNC __declspec(dllexport)
#else
#define PUBLIC_FUNC
#endif

// This is for supporting older versions which do not have support for nullptr.
#if defined(__clang__)
# ifndef __has_extension
# define __has_extension __has_feature
# endif

# if __has_extension(cxx_nullptr)
# define GMP_HAVE_NULLPTR
# endif

#elif defined(__GNUC__)
# if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
# if (__GNU_C__ >=4)
# if (__GNU_C_MINOR__ >= 6)
# define GMP_HAVE_NULLPTR
# endif
# endif
# endif

#elif defined(_MSC_VER)
# define GMP_HAVE_NULLPTR
#endif

#if !defined (GMP_HAVE_NULLPTR)
# define nullptr __null
#endif

static int g_log_level = 0;

#define GMPLOG(l, x) do { \
        if (l <= g_log_level) { \
        const char *log_string = "unknown"; \
        if ((l >= 0) && (l <= 3)) {               \
        log_string = kLogStrings[l];            \
        } \
        std::cerr << log_string << ": " << x << std::endl; \
        } \
    } while(0)

#define GL_CRIT 0
#define GL_ERROR 1
#define GL_INFO  2
#define GL_DEBUG 3

const char* kLogStrings[] = {
  "Critical",
  "Error",
  "Info",
  "Debug"
};


static GMPPlatformAPI* g_platform_api = nullptr;

class OpenH264VideoEncoder;

template <typename T> class SelfDestruct {
 public:
  SelfDestruct (T* t) : t_ (t) {}
  ~SelfDestruct() {
    if (t_) {
      t_->Destroy();
    }
  }

  T* forget() {
    T* t = t_;
    t_ = nullptr;

    return t;
  }

 private:
  T* t_;
};

class FrameStats {
 public:
  FrameStats (const char* type) :
    frames_in_ (0),
    frames_out_ (0),
    start_time_ (time (0)),
    last_time_ (start_time_),
    type_ (type) {}

  void FrameIn() {
    ++frames_in_;
    time_t now = time (0);

    if (now == last_time_) {
      return;
    }

    if (! (frames_in_ % 10)) {
      GMPLOG (GL_INFO, type_ << ": " << now << " Frame count "
              << frames_in_
              << "(" << (frames_in_ / (now - start_time_)) << "/"
              << (30 / (now - last_time_)) << ")"
              << " -- " << frames_out_);
      last_time_ = now;
    }
  }

  void FrameOut() {
    ++frames_out_;
  }

 private:
  uint64_t frames_in_;
  uint64_t frames_out_;
  time_t start_time_;
  time_t last_time_;
  const std::string type_;
};

class OpenH264VideoEncoder : public GMPVideoEncoder {
 public:
  OpenH264VideoEncoder (GMPVideoHost* hostAPI) :
    host_ (hostAPI),
    worker_thread_ (nullptr),
    encoder_ (nullptr),
    max_payload_size_ (0),
    callback_ (nullptr),
    stats_ ("Encoder") {}

  virtual ~OpenH264VideoEncoder() {
    worker_thread_->Join();
  }

  virtual void InitEncode (const GMPVideoCodec& codecSettings,
                           const uint8_t* aCodecSpecific,
                           uint32_t aCodecSpecificSize,
                           GMPVideoEncoderCallback* callback,
                           int32_t numberOfCores,
                           uint32_t maxPayloadSize) {
    callback_ = callback;

    GMPErr err = g_platform_api->createthread (&worker_thread_);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Couldn't create new thread");
      Error (GMPGenericErr);
      return;
    }

    int rv = WelsCreateSVCEncoder (&encoder_);
    if (rv) {
      Error (GMPGenericErr);
      return;
    }

    SEncParamBase param;
    memset (&param, 0, sizeof (param));

    GMPLOG (GL_INFO, "Initializing encoder at "
            << codecSettings.mWidth
            << "x"
            << codecSettings.mHeight
            << "@"
            << static_cast<int> (codecSettings.mMaxFramerate)
            << "max payload size="
            << maxPayloadSize);

    // Translate parameters.
    param.iUsageType = CAMERA_VIDEO_REAL_TIME;
    param.iPicWidth = codecSettings.mWidth;
    param.iPicHeight = codecSettings.mHeight;
    param.iTargetBitrate = codecSettings.mStartBitrate * 1000;
    GMPLOG (GL_INFO, "Initializing Bit Rate at: Start: "
            << codecSettings.mStartBitrate
            << "; Min: "
            << codecSettings.mMinBitrate
            << "; Max: "
            << codecSettings.mMaxBitrate);
    param.iRCMode = RC_BITRATE_MODE;

    // TODO(ekr@rtfm.com). Scary conversion from unsigned char to float below.
    param.fMaxFrameRate = static_cast<float> (codecSettings.mMaxFramerate);

    rv = encoder_->Initialize (&param);
    if (rv) {
      GMPLOG (GL_ERROR, "Couldn't initialize encoder");
      Error (GMPGenericErr);
      return;
    }

    max_payload_size_ = maxPayloadSize;

    GMPLOG (GL_INFO, "Initialized encoder");
  }

  virtual void Encode (GMPVideoi420Frame* inputImage,
                       const uint8_t* aCodecSpecificInfo,
                       uint32_t aCodecSpecificInfoLength,
                       const GMPVideoFrameType* aFrameTypes,
                       uint32_t aFrameTypesLength) {
    GMPLOG (GL_DEBUG,
            __FUNCTION__
            << " size="
            << inputImage->Width() << "x" << inputImage->Height());

    stats_.FrameIn();

    assert (aFrameTypesLength != 0);

    worker_thread_->Post (WrapTask (
                            this, &OpenH264VideoEncoder::Encode_w,
                            inputImage,
                            (aFrameTypes)[0]));
  }

  virtual void SetChannelParameters (uint32_t aPacketLoss, uint32_t aRTT) {
  }

  virtual void SetRates (uint32_t aNewBitRate, uint32_t aFrameRate) {
    GMPLOG (GL_INFO, "[SetRates] Begin with: "
            << aNewBitRate << " , " << aFrameRate);
    //update bitrate if needed
    const int32_t newBitRate = aNewBitRate * 1000; //kbps->bps
    SBitrateInfo existEncoderBitRate;
    existEncoderBitRate.iLayer = SPATIAL_LAYER_ALL;
    int rv = encoder_->GetOption (ENCODER_OPTION_BITRATE, &existEncoderBitRate);
    if (rv != cmResultSuccess) {
      GMPLOG (GL_ERROR, "[SetRates] Error in Getting Bit Rate at Layer:"
              << rv
              << " ; Layer = "
              << existEncoderBitRate.iLayer
              << " ; BR = "
              << existEncoderBitRate.iBitrate);
      Error (GMPGenericErr);
      return;
    }
    if (rv == cmResultSuccess && existEncoderBitRate.iBitrate != newBitRate) {
      SBitrateInfo newEncoderBitRate;
      newEncoderBitRate.iLayer = SPATIAL_LAYER_ALL;
      newEncoderBitRate.iBitrate = newBitRate;
      rv = encoder_->SetOption (ENCODER_OPTION_BITRATE, &newEncoderBitRate);
      if (rv == cmResultSuccess) {
        GMPLOG (GL_INFO, "[SetRates] Update Encoder Bandwidth (AllLayers): ReturnValue: "
                << rv
                << "BitRate(kbps): "
                << aNewBitRate);
      } else {
        GMPLOG (GL_ERROR, "[SetRates] Error in Setting Bit Rate at Layer:"
                << rv
                << " ; Layer = "
                << newEncoderBitRate.iLayer
                << " ; BR = "
                << newEncoderBitRate.iBitrate);
        Error (GMPGenericErr);
        return;
      }
    }
    //update framerate if needed
    float existFrameRate = 0;
    rv = encoder_->GetOption (ENCODER_OPTION_FRAME_RATE, &existFrameRate);
    if (rv != cmResultSuccess) {
      GMPLOG (GL_ERROR, "[SetRates] Error in Getting Frame Rate:"
              << rv << " FrameRate: " << existFrameRate);
      Error (GMPGenericErr);
      return;
    }
    if (rv == cmResultSuccess &&
        (aFrameRate - existFrameRate > 0.001f ||
         existFrameRate - aFrameRate > 0.001f)) {
      float newFrameRate = static_cast<float> (aFrameRate);
      rv = encoder_->SetOption (ENCODER_OPTION_FRAME_RATE, &newFrameRate);
      if (rv == cmResultSuccess) {
        GMPLOG (GL_INFO, "[SetRates] Update Encoder Frame Rate: ReturnValue: "
                << rv << " FrameRate: " << aFrameRate);
      } else {
        GMPLOG (GL_ERROR, "[SetRates] Error in Setting Frame Rate: ReturnValue: "
                << rv << " FrameRate: " << aFrameRate);
        Error (GMPGenericErr);
        return;
      }
    }
  }

  virtual void SetPeriodicKeyFrames (bool aEnable) {
  }

  virtual void EncodingComplete() {
    delete this;
  }

 private:
  void Error (GMPErr error) {
    if (callback_) {
      callback_->Error (error);
    }
  }

  void Encode_w (GMPVideoi420Frame* inputImage,
                 GMPVideoFrameType frame_type) {
    SFrameBSInfo encoded;

    if (frame_type  == kGMPKeyFrame) {
      encoder_->ForceIntraFrame (true);
      if (!inputImage)
        return;
    }
    if (!inputImage) {
      GMPLOG (GL_ERROR, "no input image");
      return;
    }
    SSourcePicture src;

    src.iColorFormat = videoFormatI420;
    src.iStride[0] = inputImage->Stride (kGMPYPlane);
    src.pData[0] = reinterpret_cast<unsigned char*> (
                     const_cast<uint8_t*> (inputImage->Buffer (kGMPYPlane)));
    src.iStride[1] = inputImage->Stride (kGMPUPlane);
    src.pData[1] = reinterpret_cast<unsigned char*> (
                     const_cast<uint8_t*> (inputImage->Buffer (kGMPUPlane)));
    src.iStride[2] = inputImage->Stride (kGMPVPlane);
    src.pData[2] = reinterpret_cast<unsigned char*> (
                     const_cast<uint8_t*> (inputImage->Buffer (kGMPVPlane)));
    src.iStride[3] = 0;
    src.pData[3] = nullptr;
    src.iPicWidth = inputImage->Width();
    src.iPicHeight = inputImage->Height();

    const SSourcePicture* pics = &src;

    int result = encoder_->EncodeFrame (pics, &encoded);
    if (result != cmResultSuccess) {
      GMPLOG (GL_ERROR, "Couldn't encode frame. Error = " << result);
    }


    // Translate int to enum
    GMPVideoFrameType encoded_type;
    bool has_frame = false;

    switch (encoded.eFrameType) {
    case videoFrameTypeIDR:
      encoded_type = kGMPKeyFrame;
      has_frame = true;
      break;
    case videoFrameTypeI:
      encoded_type = kGMPKeyFrame;
      has_frame = true;
      break;
    case videoFrameTypeP:
      encoded_type = kGMPDeltaFrame;
      has_frame = true;
      break;
    case videoFrameTypeSkip:
      // Can skip the call back since no actual bitstream will be generated
      break;
    case videoFrameTypeIPMixed://this type is currently not suppported
    case videoFrameTypeInvalid:
      GMPLOG (GL_ERROR, "Couldn't encode frame. Type = "
              << encoded.eFrameType);
      break;
    default:
      // The API is defined as returning a type.
      assert (false);
      break;
    }

    if (!has_frame) {
      // This frame must be destroyed on the main thread.
      g_platform_api->syncrunonmainthread (WrapTask (
                                             this,
                                             &OpenH264VideoEncoder::DestroyInputFrame_m,
                                             inputImage));
      return;
    }

    // Synchronously send this back to the main thread for delivery.
    g_platform_api->syncrunonmainthread (WrapTask (
                                           this,
                                           &OpenH264VideoEncoder::Encode_m,
                                           inputImage,
                                           &encoded,
                                           encoded_type));
  }

  void Encode_m (GMPVideoi420Frame* frame, SFrameBSInfo* encoded,
                 GMPVideoFrameType frame_type) {
    // Now return the encoded data back to the parent.
    GMPVideoFrame* ftmp;
    GMPErr err = host_->CreateFrame (kGMPEncodedVideoFrame, &ftmp);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Error creating encoded frame");
      frame->Destroy();
      return;
    }

    GMPVideoEncodedFrame* f = static_cast<GMPVideoEncodedFrame*> (ftmp);
    // Buffer up the data.
    uint32_t length = 0;
    std::vector<uint32_t> lengths;

    for (int i = 0; i < encoded->iLayerNum; ++i) {
      lengths.push_back (0);
      uint8_t* tmp = encoded->sLayerInfo[i].pBsBuf;
      for (int j = 0; j < encoded->sLayerInfo[i].iNalCount; ++j) {
        lengths[i] += encoded->sLayerInfo[i].pNalLengthInByte[j];
        // Convert from 4-byte start codes to GMP_BufferLength32 (NAL lengths)
        assert (* (reinterpret_cast<uint32_t*> (tmp)) == 0x01000000);
        // BufferType32 doesn't include the length of the length itself!
        * (reinterpret_cast<uint32_t*> (tmp)) = encoded->sLayerInfo[i].pNalLengthInByte[j] - sizeof (uint32_t);
        length += encoded->sLayerInfo[i].pNalLengthInByte[j];
        tmp += encoded->sLayerInfo[i].pNalLengthInByte[j];
      }
    }

    err = f->CreateEmptyFrame (length);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Error allocating frame data");
      f->Destroy();
      frame->Destroy();
      return;
    }

    // Copy the data.
    // Here we concatenate into one big buffer
    uint8_t* tmp = f->Buffer();
    for (int i = 0; i < encoded->iLayerNum; ++i) {
      memcpy (tmp, encoded->sLayerInfo[i].pBsBuf, lengths[i]);
      tmp += lengths[i];
    }

    f->SetEncodedWidth (frame->Width());
    f->SetEncodedHeight (frame->Height());
    f->SetTimeStamp (frame->Timestamp());
    f->SetFrameType (frame_type);
    f->SetCompleteFrame (true);
    f->SetBufferType (GMP_BufferLength32);

    GMPLOG (GL_DEBUG, "Encoding complete. type= "
            << f->FrameType()
            << " length="
            << f->Size()
            << " timestamp="
            << f->TimeStamp());

    // Destroy the frame.
    frame->Destroy();

    // Return the encoded frame.
    GMPCodecSpecificInfo info;
    memset (&info, 0, sizeof (info)); // shouldn't be needed, we init everything
    info.mCodecType = kGMPVideoCodecH264;
    info.mBufferType = GMP_BufferLength32;
    info.mCodecSpecific.mH264.mSimulcastIdx = 0;

    callback_->Encoded (f, reinterpret_cast<uint8_t*> (&info), sizeof (info));

    stats_.FrameOut();
  }

  // These frames must be destroyed on the main thread.
  void DestroyInputFrame_m (GMPVideoi420Frame* frame) {
    frame->Destroy();
  }


 private:
  GMPVideoHost* host_;
  GMPThread* worker_thread_;
  ISVCEncoder* encoder_;
  uint32_t max_payload_size_;
  GMPVideoEncoderCallback* callback_;
  FrameStats stats_;
};

class OpenH264VideoDecoder : public GMPVideoDecoder {
 public:
  OpenH264VideoDecoder (GMPVideoHost* hostAPI) :
    host_ (hostAPI),
    worker_thread_ (nullptr),
    callback_ (nullptr),
    decoder_ (nullptr),
    stats_ ("Decoder") {}

  virtual ~OpenH264VideoDecoder() {
  }

  virtual void InitDecode (const GMPVideoCodec& codecSettings,
                           const uint8_t* aCodecSpecific,
                           uint32_t aCodecSpecificSize,
                           GMPVideoDecoderCallback* callback,
                           int32_t coreCount) {
    callback_ = callback;

    GMPLOG (GL_INFO, "InitDecode");

    GMPErr err = g_platform_api->createthread (&worker_thread_);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Couldn't create new thread");
      Error (GMPGenericErr);
      return;
    }

    if (WelsCreateDecoder (&decoder_)) {
      GMPLOG (GL_ERROR, "Couldn't create decoder");
      Error (GMPGenericErr);
      return;
    }

    if (!decoder_) {
      GMPLOG (GL_ERROR, "Couldn't create decoder");
      Error (GMPGenericErr);
      return;
    }

    SDecodingParam param;
    memset (&param, 0, sizeof (param));
    param.iOutputColorFormat = videoFormatI420;
    param.uiTargetDqLayer = UCHAR_MAX;  // Default value
    param.uiEcActiveFlag = 1; // Error concealment on.
    param.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    if (decoder_->Initialize (&param)) {
      GMPLOG (GL_ERROR, "Couldn't initialize decoder");
      Error (GMPGenericErr);
      return;
    }
  }

  virtual void Decode (GMPVideoEncodedFrame* inputFrame,
                       bool missingFrames,
                       const uint8_t* aCodecSpecificInfo,
                       uint32_t aCodecSpecificInfoLength,
                       int64_t renderTimeMs = -1) {
    GMPLOG (GL_DEBUG, __FUNCTION__
            << "Decoding frame size=" << inputFrame->Size()
            << " timestamp=" << inputFrame->TimeStamp());
    stats_.FrameIn();
    //const GMPCodecSpecificInfo *codecSpecificInfo = (GMPCodecSpecificInfo) aCodecSpecificInfo;

    // Convert to H.264 start codes
    switch (inputFrame->BufferType()) {
    case GMP_BufferSingle:
    case GMP_BufferLength8:
    case GMP_BufferLength16:
    case GMP_BufferLength24:
      // We should look to support these, especially GMP_BufferSingle
      assert (false);
      break;

    case GMP_BufferLength32: {
      uint8_t* start_code = inputFrame->Buffer();
      while (start_code < inputFrame->Buffer() + inputFrame->Size()) {
        static const uint8_t code[] = { 0x00, 0x00, 0x00, 0x01 };
        uint8_t* lenp = start_code;
        start_code += * (reinterpret_cast<int32_t*> (lenp));
        memcpy (lenp, code, 4);
      }
    }
    break;

    default:
      assert (false);
      break;
    }
    DECODING_STATE dState = dsErrorFree;
    worker_thread_->Post (WrapTask (
                            this, &OpenH264VideoDecoder::Decode_w,
                            inputFrame,
                            missingFrames,
                            dState,
                            renderTimeMs));
    if (dState) {
      Error(GMPGenericErr);
    }
  }

  virtual void Reset() {
  }

  virtual void Drain() {
  }

  virtual void DecodingComplete() {
    delete this;
  }

 private:
  void Error (GMPErr error) {
    if (callback_) {
      callback_->Error (error);
    }
  }

  void Decode_w (GMPVideoEncodedFrame* inputFrame,
                 bool missingFrames,
                 DECODING_STATE& dState,
                 int64_t renderTimeMs = -1) {
    GMPLOG (GL_DEBUG, "Frame decode on worker thread length = "
            << inputFrame->Size());

    SBufferInfo decoded;
    bool valid = false;
    memset (&decoded, 0, sizeof (decoded));
    unsigned char* data[3] = {nullptr, nullptr, nullptr};

    dState = decoder_->DecodeFrame2 (inputFrame->Buffer(),
                                     inputFrame->Size(),
                                     data,
                                     &decoded);

    if (dState) {
      GMPLOG (GL_ERROR, "Decoding error dState=" << dState);
    } else {
      valid = true;
    }

    g_platform_api->syncrunonmainthread (WrapTask (
                                           this,
                                           &OpenH264VideoDecoder::Decode_m,
                                           inputFrame,
                                           &decoded,
                                           data,
                                           renderTimeMs,
                                           valid));
  }

  // Return the decoded data back to the parent.
  void Decode_m (GMPVideoEncodedFrame* inputFrame,
                 SBufferInfo* decoded,
                 unsigned char* data[3],
                 int64_t renderTimeMs,
                 bool valid) {
    // Attach a self-destructor so that this dies on return.
    SelfDestruct<GMPVideoEncodedFrame> ifd (inputFrame);

    // If we don't actually have data, just abort.
    if (!valid) {
      return;
    }

    if (decoded->iBufferStatus != 1) {
      return;
    }

    int width = decoded->UsrData.sSystemBuffer.iWidth;
    int height = decoded->UsrData.sSystemBuffer.iHeight;
    int ystride = decoded->UsrData.sSystemBuffer.iStride[0];
    int uvstride = decoded->UsrData.sSystemBuffer.iStride[1];

    GMPLOG (GL_DEBUG, "Video frame ready for display "
            << width
            << "x"
            << height
            << " timestamp="
            << inputFrame->TimeStamp());

    GMPVideoFrame* ftmp = nullptr;

    // Translate the image.
    GMPErr err = host_->CreateFrame (kGMPI420VideoFrame, &ftmp);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Couldn't allocate empty I420 frame");
      return;
    }


    GMPVideoi420Frame* frame = static_cast<GMPVideoi420Frame*> (ftmp);
    err = frame->CreateFrame (
            ystride * height, static_cast<uint8_t*> (data[0]),
            uvstride * height / 2, static_cast<uint8_t*> (data[1]),
            uvstride * height / 2, static_cast<uint8_t*> (data[2]),
            width, height,
            ystride, uvstride, uvstride);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Couldn't make decoded frame");
      return;
    }

    GMPLOG (GL_DEBUG, "Allocated size = "
            << frame->AllocatedSize (kGMPYPlane));
    frame->SetTimestamp (inputFrame->TimeStamp());
    frame->SetDuration (inputFrame->Duration());
    callback_->Decoded (frame);

    stats_.FrameOut();
  }

  GMPVideoHost* host_;
  GMPThread* worker_thread_;
  GMPVideoDecoderCallback* callback_;
  ISVCDecoder* decoder_;
  FrameStats stats_;
};

extern "C" {

  PUBLIC_FUNC GMPErr
  GMPInit (GMPPlatformAPI* aPlatformAPI) {
    g_platform_api = aPlatformAPI;
    return GMPNoErr;
  }

  PUBLIC_FUNC GMPErr
  GMPGetAPI (const char* aApiName, void* aHostAPI, void** aPluginApi) {
    if (!strcmp (aApiName, "decode-video")) {
      *aPluginApi = new OpenH264VideoDecoder (static_cast<GMPVideoHost*> (aHostAPI));
      return GMPNoErr;
    } else if (!strcmp (aApiName, "encode-video")) {
      *aPluginApi = new OpenH264VideoEncoder (static_cast<GMPVideoHost*> (aHostAPI));
      return GMPNoErr;
    }
    return GMPGenericErr;
  }

  PUBLIC_FUNC void
  GMPShutdown (void) {
    g_platform_api = nullptr;
  }

} // extern "C"
