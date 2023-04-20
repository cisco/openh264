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
#include <cmath>
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

#if defined(_MSC_VER)
#define PUBLIC_FUNC __declspec(dllexport)
#else
#define PUBLIC_FUNC
#endif

// This is for supporting older versions which do not have support for nullptr.
#if defined(nullptr)
# define GMP_HAVE_NULLPTR

#elif defined(__clang__)
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

#include "task_utils.h"

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

#define OPENH264_MAX_MB 36864

GMPPlatformAPI* g_platform_api = nullptr;

class OpenH264VideoEncoder;

static uint32_t GMPLogLevelToWelsLogLevel(GMPLogLevel aLevel) {
  switch (aLevel) {
    default:
    case kGMPLogInvalid:
    case kGMPLogDefault:
      return WELS_LOG_DEFAULT;
    case kGMPLogQuiet:
      return WELS_LOG_QUIET;
    case kGMPLogError:
      return WELS_LOG_ERROR;
    case kGMPLogWarning:
      return WELS_LOG_WARNING;
    case kGMPLogInfo:
      return WELS_LOG_INFO;
    case kGMPLogDebug:
      return WELS_LOG_DEBUG;
    case kGMPLogDetail:
      return WELS_LOG_DETAIL;
  }
}

static EUsageType GMPVideoCodecModeToWelsUsageType(GMPVideoCodecMode aMode) {
  switch (aMode) {
    default:
    case kGMPRealtimeVideo:
      return CAMERA_VIDEO_REAL_TIME;
    case kGMPScreensharing:
      return SCREEN_CONTENT_REAL_TIME;
    case kGMPStreamingVideo:
      return SCREEN_CONTENT_NON_REAL_TIME;
    case kGMPNonRealtimeVideo:
      return CAMERA_VIDEO_NON_REAL_TIME;
  }
}

static RC_MODES GMPRateControlModeToWelsRcModes(GMPRateControlMode aMode) {
  switch (aMode) {
    default:
    case kGMPRateControlUnknown:
    case kGMPRateControlBitrate:
      return RC_BITRATE_MODE;
    case kGMPRateControlQuality:
      return RC_QUALITY_MODE;
    case kGMPRateControlBufferBased:
      return RC_BUFFERBASED_MODE;
    case kGMPRateControlTimestamp:
      return RC_TIMESTAMP_MODE;
    case kGMPRateControlBitratePostskip:
      return RC_BITRATE_MODE_POST_SKIP;
    case kGMPRateControlOff:
      return RC_OFF_MODE;
  }
}

EProfileIdc GMPProfileToWelsProfile(GMPProfile aProfile) {
  switch (aProfile) {
    default:
    case kGMPH264ProfileUnknown:
      return PRO_UNKNOWN;
    case kGMPH264ProfileBaseline:
      return PRO_BASELINE;
    case kGMPH264ProfileMain:
      return PRO_MAIN;
    case kGMPH264ProfileExtended:
      return PRO_EXTENDED;
    case kGMPH264ProfileHigh:
      return PRO_HIGH;
    case kGMPH264ProfileHigh10:
      return PRO_HIGH10;
    case kGMPH264ProfileHigh422:
      return PRO_HIGH422;
    case kGMPH264ProfileHigh444:
      return PRO_HIGH444;
    case kGMPH264ProfileCavlc444:
      return PRO_CAVLC444;
    case kGMPH264ProfileScalableBaseline:
      return PRO_SCALABLE_BASELINE;
    case kGMPH264ProfileScalableHigh:
      return PRO_SCALABLE_HIGH;
  }
}

ELevelIdc GMPLevelToWelsLevel(GMPLevel aLevel) {
  switch (aLevel) {
    default:
    case kGMPH264LevelUnknown:
      return LEVEL_UNKNOWN;
    case kGMPH264Level1_0:
      return LEVEL_1_0;
    case kGMPH264Level1_B:
      return LEVEL_1_B;
    case kGMPH264Level1_1:
      return LEVEL_1_1;
    case kGMPH264Level1_2:
      return LEVEL_1_2;
    case kGMPH264Level1_3:
      return LEVEL_1_3;
    case kGMPH264Level2_0:
      return LEVEL_2_0;
    case kGMPH264Level2_1:
      return LEVEL_2_1;
    case kGMPH264Level2_2:
      return LEVEL_2_2;
    case kGMPH264Level3_0:
      return LEVEL_3_0;
    case kGMPH264Level3_1:
      return LEVEL_3_1;
    case kGMPH264Level3_2:
      return LEVEL_3_2;
    case kGMPH264Level4_0:
      return LEVEL_4_0;
    case kGMPH264Level4_1:
      return LEVEL_4_1;
    case kGMPH264Level4_2:
      return LEVEL_4_2;
    case kGMPH264Level5_0:
      return LEVEL_5_0;
    case kGMPH264Level5_1:
      return LEVEL_5_1;
    case kGMPH264Level5_2:
      return LEVEL_5_2;
  }
}

SliceModeEnum GMPSliceToWelsSliceMode(GMPSliceMode aMode) {
  switch (aMode) {
    default:
    case kGMPSliceUnknown:
    case kGMPSliceSingle:
      return SM_SINGLE_SLICE;
    case kGMPSliceSizeLimited:
      return SM_SIZELIMITED_SLICE;
    case kGMPSliceFixedSlcNum:
      return SM_FIXEDSLCNUM_SLICE;
    case kGMPSliceRaster:
      return SM_RASTER_SLICE;
  }
}

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

class OpenH264VideoEncoder : public GMPVideoEncoder, public RefCounted {
 public:
  OpenH264VideoEncoder (GMPVideoHost* hostAPI) :
    host_ (hostAPI),
    worker_thread_ (nullptr),
    encoder_ (nullptr),
    max_payload_size_ (0),
    callback_ (nullptr),
    stats_ ("Encoder"),
    gmp_api_version_ (kGMPVersion33),
    shutting_down(false) {
      AddRef();
    }

  virtual void InitEncode (const GMPVideoCodec& codecSettings,
                           const uint8_t* aCodecSpecific,
                           uint32_t aCodecSpecificSize,
                           GMPVideoEncoderCallback* callback,
                           int32_t numberOfCores,
                           uint32_t maxPayloadSize) {
    gmp_api_version_ = codecSettings.mGMPApiVersion;
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

    if (gmp_api_version_ >= kGMPVersion34) {
      uint32_t logLevel = GMPLogLevelToWelsLogLevel(codecSettings.mLogLevel);
      long rv = encoder_->SetOption(ENCODER_OPTION_TRACE_LEVEL, &logLevel);
      if (rv != cmResultSuccess) {
        GMPLOG (GL_ERROR, "Encoder SetOption OPTION_TRACE_LEVEL failed " << rv);
      }
    }

    SEncParamExt param;
    memset (&param, 0, sizeof (param));
    encoder_->GetDefaultParams (&param);

    GMPLOG (GL_INFO, "Initializing encoder at "
            << codecSettings.mWidth
            << "x"
            << codecSettings.mHeight
            << "@"
            << static_cast<int> (codecSettings.mMaxFramerate));

    // Translate parameters.
    if (gmp_api_version_ >= kGMPVersion35) {
      param.iUsageType = GMPVideoCodecModeToWelsUsageType(codecSettings.mMode);
    } else if (codecSettings.mMode == kGMPScreensharing) {
      param.iUsageType = SCREEN_CONTENT_REAL_TIME;
    } else {
      param.iUsageType = CAMERA_VIDEO_REAL_TIME;
    }
    param.iPicWidth = codecSettings.mWidth;
    param.iPicHeight = codecSettings.mHeight;
    if (gmp_api_version_ >= kGMPVersion35) {
      param.iRCMode = GMPRateControlModeToWelsRcModes(codecSettings.mRateControlMode);
    } else {
      param.iRCMode = RC_BITRATE_MODE;
    }
    param.iTargetBitrate = codecSettings.mStartBitrate * 1000;
    param.iMaxBitrate = codecSettings.mMaxBitrate * 1000;
    GMPLOG (GL_INFO, "Initializing Bit Rate at: Start: "
            << codecSettings.mStartBitrate
            << "; Min: "
            << codecSettings.mMinBitrate
            << "; Max: "
            << codecSettings.mMaxBitrate
            << "; Max payload size:"
            << maxPayloadSize);

    param.uiMaxNalSize = maxPayloadSize;

    // TODO(ekr@rtfm.com). Scary conversion from unsigned char to float below.
    param.fMaxFrameRate = static_cast<float> (codecSettings.mMaxFramerate);

    // Set up layers. Currently we have one layer.
    SSpatialLayerConfig* layer = &param.sSpatialLayers[0];

    // Make sure the output resolution doesn't exceed the Openh264 capability
    double width_mb = std::ceil(codecSettings.mWidth/16.0);
    double height_mb = std::ceil(codecSettings.mHeight/16.0);
    double input_mb = width_mb * height_mb;
    if (static_cast<uint32_t>(input_mb) > OPENH264_MAX_MB) {
      double scale = std::sqrt(OPENH264_MAX_MB / input_mb);
      layer->iVideoWidth = static_cast<uint32_t>(width_mb * 16 * scale);
      layer->iVideoHeight = static_cast<uint32_t>(height_mb * 16 * scale);
      GMPLOG (GL_INFO, "InitEncode: the output resolution overflows, w x h = " << codecSettings.mWidth << " x " << codecSettings.mHeight
              << ", turned to be " << layer->iVideoWidth << " x " << layer->iVideoHeight);
    } else {
      layer->iVideoWidth = codecSettings.mWidth;
      layer->iVideoHeight = codecSettings.mHeight;
    }
    if (layer->iVideoWidth < 16) {
      layer->iVideoWidth = 16;
    }
    if (layer->iVideoHeight < 16) {
      layer->iVideoHeight = 16;
    }

    layer->fFrameRate = param.fMaxFrameRate;
    layer->iSpatialBitrate = param.iTargetBitrate;
    layer->iMaxSpatialBitrate = param.iMaxBitrate;

    if (gmp_api_version_ >= kGMPVersion35) {
      layer->uiProfileIdc = GMPProfileToWelsProfile(codecSettings.mProfile);
      layer->uiLevelIdc = GMPLevelToWelsLevel(codecSettings.mLevel);
      layer->sSliceArgument.uiSliceMode = GMPSliceToWelsSliceMode(codecSettings.mSliceMode);
      if (codecSettings.mUseThreadedEncode) {
        param.iMultipleThreadIdc = numberOfCores;
      } else {
        param.iMultipleThreadIdc = 1; // disabled
      }
    }

    //for controlling the NAL size (normally for packetization-mode=0)
    if (maxPayloadSize != 0) {
      if (gmp_api_version_ < kGMPVersion35) {
        layer->sSliceArgument.uiSliceMode = SM_SIZELIMITED_SLICE;
      }
      layer->sSliceArgument.uiSliceSizeConstraint = maxPayloadSize;
    }
    rv = encoder_->InitializeExt (&param);
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

    worker_thread_->Post (WrapTaskRefCounted (
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
    shutting_down = true;

    // Release the reference to the external objects, because it is no longer safe to call them
    host_     = nullptr;
    callback_ = nullptr;
    TearDownEncoder();

    Release();
  }

 private:
  virtual ~OpenH264VideoEncoder() {
    // Tear down the internal encoder in case of EncodingComplete() not being called
    TearDownEncoder();
  }

   void TearDownEncoder() {
     // Stop the worker thread first
     if (worker_thread_) {
       worker_thread_->Join();
       worker_thread_ = nullptr;
     }

     // Destroy OpenH264 encoder
     if (encoder_) {
       WelsDestroySVCEncoder(encoder_);
       encoder_ = nullptr;
     }
   }

  void TrySyncRunOnMainThread(GMPTask* aTask) {
    if (!shutting_down && g_platform_api) {
      g_platform_api->syncrunonmainthread (aTask);
    }
  }

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
    src.uiTimeStamp = inputImage->Timestamp() / 1000; //encoder needs millisecond
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
      TrySyncRunOnMainThread (WrapTask (
                                   this,
                                   &OpenH264VideoEncoder::DestroyInputFrame_m,
                                   inputImage));
      return;
    }

    // Synchronously send this back to the main thread for delivery.
    TrySyncRunOnMainThread (WrapTask (
                                   this,
                                   &OpenH264VideoEncoder::Encode_m,
                                   inputImage,
                                   &encoded,
                                   encoded_type));
  }

  void Encode_m (GMPVideoi420Frame* frame, SFrameBSInfo* encoded,
                 GMPVideoFrameType frame_type) {
    // Attach a self-destructor so that this dies on return.
    SelfDestruct<GMPVideoi420Frame> ifd (frame);
    
    if (!host_) {
      return;
    }
    
    // Now return the encoded data back to the parent.
    GMPVideoFrame* ftmp;
    GMPErr err = host_->CreateFrame (kGMPEncodedVideoFrame, &ftmp);
    if (err != GMPNoErr) {
      GMPLOG (GL_ERROR, "Error creating encoded frame");
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

    // Return the encoded frame.
    GMPCodecSpecificInfo info;
    memset (&info, 0, sizeof (info)); // shouldn't be needed, we init everything
    info.mCodecType = kGMPVideoCodecH264;
    info.mBufferType = GMP_BufferLength32;
    info.mCodecSpecific.mH264.mSimulcastIdx = 0;

    if (callback_) {
      callback_->Encoded (f, reinterpret_cast<uint8_t*> (&info), sizeof (info));
    }

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
  uint32_t gmp_api_version_;
  bool shutting_down;
};

uint16_t readU16BE(const uint8_t* in) {
  return in[0] << 8 | in[1];
}

void copyWithStartCode(std::vector<uint8_t>& out, const uint8_t* in, size_t size) {
  static const uint8_t code[] = { 0x00, 0x00, 0x00, 0x01 };
  out.insert(out.end(), code, code + sizeof(code));
  out.insert(out.end(), in, in + size);
}

class OpenH264VideoDecoder : public GMPVideoDecoder, public RefCounted {
 public:
  OpenH264VideoDecoder (GMPVideoHost* hostAPI) :
    host_ (hostAPI),
    worker_thread_ (nullptr),
    callback_ (nullptr),
    decoder_ (nullptr),
    stats_ ("Decoder"),
    gmp_api_version_ (kGMPVersion33),
    shutting_down(false) {
      AddRef();
    }

  virtual void InitDecode (const GMPVideoCodec& codecSettings,
                           const uint8_t* aCodecSpecific,
                           uint32_t aCodecSpecificSize,
                           GMPVideoDecoderCallback* callback,
                           int32_t coreCount) {
    gmp_api_version_ = codecSettings.mGMPApiVersion;
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

    if (gmp_api_version_ >= kGMPVersion34) {
      if (codecSettings.mUseThreadedDecode) {
        long rv = decoder_->SetOption(DECODER_OPTION_NUM_OF_THREADS, &coreCount);
        if (rv != cmResultSuccess) {
          GMPLOG (GL_ERROR, "Decoder SetOption NUM_OF_THREADS failed " << rv);
        }
      }

      uint32_t logLevel = GMPLogLevelToWelsLogLevel(codecSettings.mLogLevel);
      long rv = decoder_->SetOption(DECODER_OPTION_TRACE_LEVEL, &logLevel);
      if (rv != cmResultSuccess) {
        GMPLOG (GL_ERROR, "Decoder SetOption OPTION_TRACE_LEVEL failed " << rv);
      }
    }

    SDecodingParam param;
    memset (&param, 0, sizeof (param));
    param.uiTargetDqLayer = UCHAR_MAX;  // Default value
    param.eEcActiveIdc = ERROR_CON_SLICE_MV_COPY_CROSS_IDR_FREEZE_RES_CHANGE; // Error concealment on.
    param.sVideoProperty.size = sizeof(param.sVideoProperty);
    param.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    if (decoder_->Initialize (&param)) {
      GMPLOG (GL_ERROR, "Couldn't initialize decoder");
      Error (GMPGenericErr);
      return;
    }

    if (aCodecSpecific && aCodecSpecificSize >= sizeof(GMPVideoCodecH264)) {
      std::vector<uint8_t> annexb;

      // Convert the AVCC data, starting at the byte containing
      // numOfSequenceParameterSets, to Annex B format.
      const uint8_t* avcc = aCodecSpecific + offsetof(GMPVideoCodecH264, mAVCC.mNumSPS);

      static const int kSPSMask = (1 << 5) - 1;
      uint8_t spsCount = *avcc++ & kSPSMask;
      for (int i = 0; i < spsCount; ++i) {
        size_t size = readU16BE(avcc);
        avcc += 2;
        copyWithStartCode(annexb, avcc, size);
        avcc += size;
      }

      uint8_t ppsCount = *avcc++;
      for (int i = 0; i < ppsCount; ++i) {
        size_t size = readU16BE(avcc);
        avcc += 2;
        copyWithStartCode(annexb, avcc, size);
        avcc += size;
      }

      SBufferInfo decoded;
      memset (&decoded, 0, sizeof (decoded));
      unsigned char* data[3] = {nullptr, nullptr, nullptr};
      DECODING_STATE dState = decoder_->DecodeFrame2 (&*annexb.begin(),
                                                      annexb.size(),
                                                      data,
                                                      &decoded);
      if (dState) {
        GMPLOG (GL_ERROR, "Decoding error dState=" << dState);
      }
      GMPLOG (GL_ERROR, "InitDecode iBufferStatus=" << decoded.iBufferStatus);
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
      // start code should be at least four bytes from the end or we risk
      // reading/writing outside the buffer.
      while (start_code < inputFrame->Buffer() + inputFrame->Size() - 4) {
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
    worker_thread_->Post (WrapTaskRefCounted (
                            this, &OpenH264VideoDecoder::Decode_w,
                            inputFrame,
                            missingFrames,
                            dState,
                            renderTimeMs));
    if (dState) {
      Error (GMPGenericErr);
    }
  }

  virtual void Reset() {
    if (gmp_api_version_ >= kGMPVersion34) {
      worker_thread_->Post (WrapTaskRefCounted (
                              this, &OpenH264VideoDecoder::Reset_w));
    } else if (callback_) {
      callback_->ResetComplete ();
    }
  }

  virtual void Drain() {
    if (callback_) {
      callback_->DrainComplete ();
    }
  }

  virtual void DecodingComplete() {
    shutting_down = true;

    // Release the reference to the external objects, because it is no longer safe to call them
    host_     = nullptr;
    callback_ = nullptr;
    TearDownDecoder();

    Release();
  }

 private:
  virtual ~OpenH264VideoDecoder() {
    // Tear down the internal decoder in case of DecodingComplete() not being called
    TearDownDecoder();
  }

  void TearDownDecoder() {
    // Stop the worker thread first
    if (worker_thread_) {
      worker_thread_->Join();
      worker_thread_ = nullptr;
    }

    // Destroy OpenH264 decoder
    if (decoder_) {
      WelsDestroyDecoder(decoder_);
      decoder_ = nullptr;
    }
  }

  void TrySyncRunOnMainThread(GMPTask* aTask) {
    if (!shutting_down && g_platform_api) {
      g_platform_api->syncrunonmainthread (aTask);
    }
  }

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
    if (gmp_api_version_ >= kGMPVersion34) {
      decoded.uiInBsTimeStamp = inputFrame->TimeStamp();
    }
    unsigned char* data[3] = {nullptr, nullptr, nullptr};

    dState = decoder_->DecodeFrameNoDelay (inputFrame->Buffer(),
                                     inputFrame->Size(),
                                     data,
                                     &decoded);

    if (dState) {
      GMPLOG (GL_ERROR, "Decoding error dState=" << dState);
    } else {
      valid = true;
    }

    TrySyncRunOnMainThread (WrapTask (
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
      GMPLOG (GL_ERROR, "No valid data decoded");
      Error (GMPDecodeErr);
      return;
    }

    if (decoded->iBufferStatus != 1) {
      GMPLOG (GL_ERROR, "iBufferStatus=" << decoded->iBufferStatus);
      if (callback_) {
        callback_->InputDataExhausted();
      }
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

    if (!host_) {
      return;
    }
    
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
    if (gmp_api_version_ >= kGMPVersion34) {
      frame->SetUpdatedTimestamp (decoded->uiOutYuvTimeStamp);
    }
    frame->SetDuration (inputFrame->Duration());
    if (callback_) {
      callback_->Decoded (frame);
    }

    stats_.FrameOut();
  }

  void Reset_w () {
    int eos = 1;
    long rv = decoder_->SetOption(DECODER_OPTION_END_OF_STREAM, &eos);
    if (rv != cmResultSuccess) {
      GMPLOG (GL_ERROR, "Decoder SetOption END_OF_STREAM failed " << rv);
    }

    DECODING_STATE dState;
    SBufferInfo decoded;
    unsigned char* data[3];
    do {
      memset (&decoded, 0, sizeof (decoded));
      memset (data, 0, sizeof (data));
      dState = decoder_->FlushFrame(data, &decoded);
      if (dState) {
        GMPLOG (GL_ERROR, "Flush error dState=" << dState);
        break;
      }
    } while (decoded.iBufferStatus == 1);

    TrySyncRunOnMainThread (WrapTask (
                                 this,
                                 &OpenH264VideoDecoder::Reset_m));
  }

  void Reset_m () {
    if (callback_) {
      callback_->ResetComplete ();
    }
  }

  GMPVideoHost* host_;
  GMPThread* worker_thread_;
  GMPVideoDecoderCallback* callback_;
  ISVCDecoder* decoder_;
  FrameStats stats_;
  uint32_t gmp_api_version_;
  bool shutting_down;
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
