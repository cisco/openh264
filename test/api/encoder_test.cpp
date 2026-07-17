#include <gtest/gtest.h>
#include "utils/HashFunctions.h"
#include "BaseEncoderTest.h"
#include "utils/BufferedData.h"
#include <string>
#include <cstdlib>
#include <cmath>

static void GeneratePattern(uint8_t* yBuf, int yStride, uint8_t* uBuf, int uStride, uint8_t* vBuf, int vStride, int width, int height) {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (x < yStride) {
        yBuf[y * yStride + x] = (x * 4 + y * 4) % 256;
      }
    }
  }
  for (int y = 0; y < (height >> 1); ++y) {
    for (int x = 0; x < (width >> 1); ++x) {
      if (x < uStride) {
        uBuf[y * uStride + x] = (x * 8) % 256;
      }
      if (x < vStride) {
        vBuf[y * vStride + x] = (y * 8) % 256;
      }
    }
  }
}

static double CalculatePlanePsnr(const uint8_t* ref, int refStride, const uint8_t* test, int testStride, int width, int height) {
  double mse = 0;
  int compareWidth = std::min(width, std::min(refStride, testStride));
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < compareWidth; ++x) {
      double diff = ref[y * refStride + x] - test[y * testStride + x];
      mse += diff * diff;
    }
  }
  mse /= (compareWidth * height);
  if (mse == 0) return 99.0;
  return 10.0 * log10((255.0 * 255.0) / mse);
}

static void UpdateHashFromFrame (const SFrameBSInfo& info, SHA1Context* ctx) {
  for (int i = 0; i < info.iLayerNum; ++i) {
    const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
    int layerSize = 0;
    for (int j = 0; j < layerInfo.iNalCount; ++j) {
      layerSize += layerInfo.pNalLengthInByte[j];
    }
    SHA1Input (ctx, layerInfo.pBsBuf, layerSize);
  }
}

class EncoderInitTest : public ::testing::Test, public BaseEncoderTest {
 public:
  virtual void SetUp() {
    BaseEncoderTest::SetUp();
  }
  virtual void TearDown() {
    BaseEncoderTest::TearDown();
  }
};

TEST_F (EncoderInitTest, JustInit) {}

struct EncodeFileParam {
  const char* pkcFileName;
  const char* pkcHashStr[2];
  EUsageType eUsageType;
  int iWidth;
  int iHeight;
  float fFrameRate;
  SliceModeEnum eSliceMode;
  bool bDenoise;
  int  iLayerNum;
  bool bLossless;
  bool bEnableLtr;
  bool bCabac;
// unsigned short iMultipleThreadIdc;
};

void EncFileParamToParamExt (EncodeFileParam* pEncFileParam, SEncParamExt* pEnxParamExt) {
  ASSERT_TRUE (NULL != pEncFileParam && NULL != pEnxParamExt);
  pEnxParamExt->iUsageType       = pEncFileParam->eUsageType;
  pEnxParamExt->iPicWidth        = pEncFileParam->iWidth;
  pEnxParamExt->iPicHeight       = pEncFileParam->iHeight;
  pEnxParamExt->fMaxFrameRate    = pEncFileParam->fFrameRate;
  pEnxParamExt->iSpatialLayerNum = pEncFileParam->iLayerNum;

  pEnxParamExt->bEnableDenoise   = pEncFileParam->bDenoise;
  pEnxParamExt->bIsLosslessLink  = pEncFileParam->bLossless;
  pEnxParamExt->bEnableLongTermReference = pEncFileParam->bEnableLtr;
  pEnxParamExt->iEntropyCodingModeFlag   = pEncFileParam->bCabac ? 1 : 0;

  for (int i = 0; i < pEnxParamExt->iSpatialLayerNum; i++) {
    pEnxParamExt->sSpatialLayers[i].sSliceArgument.uiSliceMode = pEncFileParam->eSliceMode;
  }

}

class EncoderOutputTest : public ::testing::WithParamInterface<EncodeFileParam>,
  public EncoderInitTest , public BaseEncoderTest::Callback {
 public:
  virtual void SetUp() {
    EncoderInitTest::SetUp();
    if (HasFatalFailure()) {
      return;
    }
    SHA1Reset (&ctx_);
  }
  virtual void onEncodeFrame (const SFrameBSInfo& frameInfo) {
    UpdateHashFromFrame (frameInfo, &ctx_);
  }

 protected:
  SHA1Context ctx_;
};


TEST_P (EncoderOutputTest, CompareOutput) {
  EncodeFileParam p = GetParam();
  SEncParamExt EnxParamExt;

  EncFileParamToParamExt (&p, &EnxParamExt);

#if defined(ANDROID_NDK)
  std::string filename = std::string ("/sdcard/") + p.pkcFileName;
  EncodeFile (p.pkcFileName, &EnxParamExt, this);
#else
  EncodeFile (p.pkcFileName, &EnxParamExt, this);
#endif
  //will remove this after screen content algorithms are ready,
  //because the bitstream output will vary when the different algorithms are added.
  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1Result (&ctx_, digest);
  if (!HasFatalFailure()) {
    CompareHashAnyOf (digest, p.pkcHashStr, sizeof p.pkcHashStr / sizeof *p.pkcHashStr);
  }
}
static const EncodeFileParam kFileParamArray[] = {
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"672a52fb6b6e6d52b5b3f3480d13d44e88481fb9"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    {"08ade1853e4e49d50be675393780e75519586143"}, CAMERA_VIDEO_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    {"e60f12e3c24500d4306d812b0811d3c21855dd1c"}, CAMERA_VIDEO_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"266de2d059a00ad2f28304e7eb378543ea7d85ab"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_RASTER_SLICE, false, 1, false, false, false // One slice per MB row
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"913e49c787a0abdb378e9bc55bcffc27da89b965"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, true, 1, false, false, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    // Allow for different output depending on whether averaging is done
    // vertically or horizontally first when downsampling.
    { "e626f7efa3d54da15794407c15b7c694f7ddd383", "eb4adc831563ce4f02f2942f52c992da760b4113" },
    CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 2, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"53f5681a0c2b7068f4edc94538d6133a657df25d"}, CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    // Allow for different output depending on whether averaging is done
    // vertically or horizontally first when downsampling.
    { "0d4bf6a3b6f09d6de7bbce6daf8002c614ee6241", "a32db3cfa66568e231d1f580d239d6468d26ce9a" },
    CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SINGLE_SLICE, false, 4, false, false, false
  },

  // the following values may be adjusted for times since we start tuning the strategy
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"fd57470eebb9b334e8edcb8b47f7fb5b5868f111"}, SCREEN_CONTENT_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/CiscoVT2people_160x96_6fps.yuv",
    {"5f63e723c3ec82fad186b48fcbcfb54730ce3b26"}, SCREEN_CONTENT_REAL_TIME, 160, 96, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Static_152_100.yuv",
    {"e77a5b0ffb48753556e617544616fb06a049e9be"}, SCREEN_CONTENT_REAL_TIME, 152, 100, 6.0f, SM_SINGLE_SLICE, false, 1, false, false, false
  },
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"f01e41426ca49932a8f1f67ad59a1700a3fa7fee"}, SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, false
  },
  //for different strategy
  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"4684962979bc306e35de93bed58cc84938abcdee"}, SCREEN_CONTENT_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, true, true, false
  },
  {
    "res/CiscoVT2people_320x192_12fps.yuv",
    {"d31a72395a4ca760c5b86a06901a2557e0373e76"}, CAMERA_VIDEO_REAL_TIME, 320, 192, 12.0f, SM_SINGLE_SLICE, false, 1, false, false, true //turn on cabac
  },

  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"8bef37fa5965d5e650c1170d938423269f7406ac"}, CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_SIZELIMITED_SLICE, false, 1, false, false, true
  },

  {
    "res/Cisco_Absolute_Power_1280x720_30fps.yuv",
    {"c5cb4a6f55c10485aa90f8b237fcb8697ba70d43"}, CAMERA_VIDEO_REAL_TIME, 1280, 720, 30.0f, SM_FIXEDSLCNUM_SLICE, false, 1, false, false, true
  },

};

INSTANTIATE_TEST_SUITE_P (EncodeFile, EncoderOutputTest,
                          ::testing::ValuesIn (kFileParamArray));

class RandomInputStream : public InputStream {
 public:
  RandomInputStream(int max_frames) : max_frames_(max_frames), count_(0) {
    srand(12345);
  }
  virtual int read (void* ptr, size_t len) {
    if (count_ >= max_frames_) {
      return 0;
    }
    unsigned char* p = (unsigned char*)ptr;
    for (size_t i = 0; i < len; ++i) {
      p[i] = rand() % 256;
    }
    count_++;
    return static_cast<int>(len);
  }
 private:
  int max_frames_;
  int count_;
};

// This test uses random noise input and a very low QP to intentionally create
// poor compression (less than 1:1 ratio). This produces very large slices
// and tests the encoder's ability to handle inflated buffers correctly without
// insufficient memory errors.
TEST_F(EncoderInitTest, VeryLargeSlices) {
  SEncParamExt param;
  encoder_->GetDefaultParams(&param);
  
  param.iUsageType = CAMERA_VIDEO_REAL_TIME;
  param.iPicWidth = 1280;
  param.iPicHeight = 720;
  param.fMaxFrameRate = 30.0f;
  param.iSpatialLayerNum = 1;
  param.iMultipleThreadIdc = 4;
  param.iRCMode = RC_OFF_MODE;
  
  param.sSpatialLayers[0].iVideoWidth = param.iPicWidth;
  param.sSpatialLayers[0].iVideoHeight = param.iPicHeight;
  param.sSpatialLayers[0].fFrameRate = param.fMaxFrameRate;
  param.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
  param.sSpatialLayers[0].sSliceArgument.uiSliceNum = 4;
  param.sSpatialLayers[0].iDLayerQp = 12;
  param.iMinQp = 0;
  param.iMaxQp = 51;

  int rv = encoder_->InitializeExt(&param);
  ASSERT_EQ(0, rv);

  RandomInputStream stream(1);
  
  int frameSize = param.iPicWidth * param.iPicHeight * 3 / 2;
  BufferedData buf;
  buf.SetLength(frameSize);
  ASSERT_EQ(buf.Length(), (size_t)frameSize);

  SFrameBSInfo info;
  memset(&info, 0, sizeof(SFrameBSInfo));

  SSourcePicture pic;
  memset(&pic, 0, sizeof(SSourcePicture));
  pic.iPicWidth = param.iPicWidth;
  pic.iPicHeight = param.iPicHeight;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = pic.iPicWidth;
  pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
  pic.pData[0] = buf.data();
  pic.pData[1] = pic.pData[0] + param.iPicWidth * param.iPicHeight;
  pic.pData[2] = pic.pData[1] + (param.iPicWidth * param.iPicHeight >> 2);

  while (stream.read(buf.data(), frameSize) == frameSize) {
    rv = encoder_->EncodeFrame(&pic, &info);
    ASSERT_EQ(0, rv);
  }
}

// This test verifies that the encoder correctly handles screen content
// sequences with large vertical scrolling motion vectors. It ensures that
// motion vector difference table indexing remains within allocated bounds when
// scroll detection predicts large vertical displacements at high quantization
// parameters.
TEST_F(EncoderInitTest, ScreenContentScrollMotionVectorBounds) {
  SEncParamExt param;
  encoder_->GetDefaultParams(&param);

  param.iUsageType = SCREEN_CONTENT_REAL_TIME;
  param.iPicWidth = 640;
  param.iPicHeight = 1800;
  param.fMaxFrameRate = 30.0f;
  param.iSpatialLayerNum = 1;
  param.iRCMode = RC_OFF_MODE;

  param.sSpatialLayers[0].iVideoWidth = param.iPicWidth;
  param.sSpatialLayers[0].iVideoHeight = param.iPicHeight;
  param.sSpatialLayers[0].fFrameRate = param.fMaxFrameRate;
  param.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
  param.sSpatialLayers[0].iDLayerQp = 51;
  param.iMinQp = 51;
  param.iMaxQp = 51;

  int rv = encoder_->InitializeExt(&param);
  ASSERT_EQ(0, rv);

  SFrameBSInfo info;
  memset(&info, 0, sizeof(SFrameBSInfo));

  int width = param.iPicWidth;
  int height = param.iPicHeight;
  int frameSize = width * height * 3 / 2;
  std::vector<uint8_t> frame0(frameSize, 128);
  std::vector<uint8_t> frame1(frameSize, 128);

  // Fill frame0 luma with pseudo-random patterns to ensure CheckLine qualifies
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      frame0[y * width + x] =
          static_cast<uint8_t>((x * 29 + y * 43 + 17) % 251);
    }
  }

  // Frame 1: shift vertically by -512 pixels (content moving downward by 512)
  int scroll_mv = -512;
  for (int y = 0; y < height; ++y) {
    if (y + scroll_mv >= 0 && y + scroll_mv < height) {
      memcpy(&frame1[y * width], &frame0[(y + scroll_mv) * width], width);
    } else {
      for (int x = 0; x < width; ++x) {
        frame1[y * width + x] =
            static_cast<uint8_t>((x * 53 + y * 71 + 101) & 0xFF);
      }
    }
  }

  // Modify macroblock (1, 32) at pixel rows 512..527 and cols 16..31 in frame1.
  // MB(0, 32) will be skipped by scroll detection with MV -2048.
  // MB(1, 32) cannot be skipped due to this modification, forcing it into
  // motion estimation where it uses MB(0, 32)'s scrolled MV as a predictor
  // during vertical full search.
  for (int y = 512; y < 528; ++y) {
    for (int x = 16; x < 32; ++x) {
      frame1[y * width + x] ^= 0xFF;
    }
  }

  SSourcePicture pic;
  memset(&pic, 0, sizeof(SSourcePicture));
  pic.iPicWidth = width;
  pic.iPicHeight = height;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = width;
  pic.iStride[1] = pic.iStride[2] = width >> 1;
  pic.pData[0] = frame0.data();
  pic.pData[1] = frame0.data() + width * height;
  pic.pData[2] = pic.pData[1] + (width * height >> 2);

  // Encode Frame 0 (Base pattern)
  rv = encoder_->EncodeFrame(&pic, &info);
  ASSERT_EQ(0, rv);
  pic.uiTimeStamp += 33;

  // Encode Frame 1 (Scrolled pattern with modified MB)
  pic.pData[0] = frame1.data();
  pic.pData[1] = frame1.data() + width * height;
  pic.pData[2] = pic.pData[1] + (width * height >> 2);
  rv = encoder_->EncodeFrame(&pic, &info);
  ASSERT_EQ(0, rv);
}

// This test verifies that the encoder correctly handles frames with distinct,
// asymmetric strides for the U and V chroma planes (e.g., when U stride is much
// larger than V stride, or vice versa). It ensures that memory copy routines
// advance each chroma plane pointer by its own stride without invalid memory
// access.
TEST_F(EncoderInitTest, CustomChromaPlaneStrides) {
  SEncParamExt param;
  encoder_->GetDefaultParams(&param);

  param.iUsageType = CAMERA_VIDEO_REAL_TIME;
  param.iPicWidth = 64;
  param.iPicHeight = 64;
  param.fMaxFrameRate = 30.0f;
  param.iSpatialLayerNum = 1;
  param.iRCMode = RC_OFF_MODE;

  param.sSpatialLayers[0].iVideoWidth = param.iPicWidth;
  param.sSpatialLayers[0].iVideoHeight = param.iPicHeight;
  param.sSpatialLayers[0].fFrameRate = param.fMaxFrameRate;
  param.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
  param.sSpatialLayers[0].iDLayerQp = 0;

  int rv = encoder_->InitializeExt(&param);
  ASSERT_EQ(0, rv);

  // Initialize a decoder to verify correctness of the output
  ISVCDecoder* decoder = nullptr;
  rv = WelsCreateDecoder(&decoder);
  ASSERT_EQ(0, rv);
  ASSERT_TRUE(decoder != nullptr);

  SDecodingParam decParam;
  memset(&decParam, 0, sizeof(SDecodingParam));
  decParam.uiTargetDqLayer = UCHAR_MAX;
  decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
  decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

  rv = decoder->Initialize(&decParam);
  ASSERT_EQ(0, rv);

  SFrameBSInfo info;
  memset(&info, 0, sizeof(SFrameBSInfo));

  // Configure distinct strides: U stride is much larger than V stride.
  int strideY = 64;
  int strideU = 4096;
  int strideV = 32;

  // Generate a distinct pattern
  std::vector<uint8_t> bufY(strideY * param.iPicHeight);
  std::vector<uint8_t> bufU(strideU * (param.iPicHeight >> 1));
  std::vector<uint8_t> bufV(strideV * (param.iPicHeight >> 1));
  GeneratePattern(bufY.data(), strideY, bufU.data(), strideU, bufV.data(), strideV, param.iPicWidth, param.iPicHeight);

  SSourcePicture pic;
  memset(&pic, 0, sizeof(SSourcePicture));
  pic.iPicWidth = param.iPicWidth;
  pic.iPicHeight = param.iPicHeight;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = strideY;
  pic.iStride[1] = strideU;
  pic.iStride[2] = strideV;
  pic.pData[0] = bufY.data();
  pic.pData[1] = bufU.data();
  pic.pData[2] = bufV.data();

  rv = encoder_->EncodeFrame(&pic, &info);
  ASSERT_EQ(0, rv);

  // Calculate total bitstream size
  int len = 0;
  for (int i = 0; i < info.iLayerNum; ++i) {
    const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
    for (int j = 0; j < layerInfo.iNalCount; ++j) {
      len += layerInfo.pNalLengthInByte[j];
    }
  }
  ASSERT_GT(len, 0);

  // Decode the encoded frame
  unsigned char* pData[3] = {nullptr};
  SBufferInfo dstBufInfo;
  memset(&dstBufInfo, 0, sizeof(SBufferInfo));

  rv = decoder->DecodeFrame2(info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo);
  ASSERT_EQ(0, rv);

  if (dstBufInfo.iBufferStatus == 0) {
    rv = decoder->DecodeFrame2(nullptr, 0, pData, &dstBufInfo);
    ASSERT_EQ(0, rv);
  }

  ASSERT_EQ(1, dstBufInfo.iBufferStatus);

  // Verify that the decoded YUV content matches our original pattern (high PSNR)
  int decodedWidthU = dstBufInfo.UsrData.sSystemBuffer.iWidth >> 1;
  int decodedHeightU = dstBufInfo.UsrData.sSystemBuffer.iHeight >> 1;
  int strideDecY = dstBufInfo.UsrData.sSystemBuffer.iStride[0];
  int strideDecChroma = dstBufInfo.UsrData.sSystemBuffer.iStride[1];

  uint8_t* decY = pData[0];
  uint8_t* decU = pData[1];
  uint8_t* decV = pData[2];

  ASSERT_TRUE(decY != nullptr);
  ASSERT_TRUE(decU != nullptr);
  ASSERT_TRUE(decV != nullptr);

  double psnrY = CalculatePlanePsnr(bufY.data(), strideY, decY, strideDecY, param.iPicWidth, param.iPicHeight);
  double psnrU = CalculatePlanePsnr(bufU.data(), strideU, decU, strideDecChroma, decodedWidthU, decodedHeightU);
  double psnrV = CalculatePlanePsnr(bufV.data(), strideV, decV, strideDecChroma, decodedWidthU, decodedHeightU);

  // With lossless QP=0, PSNR should be extremely high (effectively identical)
  EXPECT_GT(psnrY, 40.0);
  EXPECT_GT(psnrU, 40.0);
  EXPECT_GT(psnrV, 40.0);

  WelsDestroyDecoder(decoder);
}

// This test verifies that the encoder safely returns early and avoids invalid
// memory access (no crash / ASan error) when the input source picture has U or
// V strides that are smaller than the required width/2.
TEST_F(EncoderInitTest, CustomChromaPlaneStridesInvalidSrc) {
  SEncParamExt param;
  encoder_->GetDefaultParams(&param);

  param.iUsageType = CAMERA_VIDEO_REAL_TIME;
  param.iPicWidth = 64;
  param.iPicHeight = 64;
  param.fMaxFrameRate = 30.0f;
  param.iSpatialLayerNum = 1;
  param.iRCMode = RC_OFF_MODE;

  param.sSpatialLayers[0].iVideoWidth = param.iPicWidth;
  param.sSpatialLayers[0].iVideoHeight = param.iPicHeight;
  param.sSpatialLayers[0].fFrameRate = param.fMaxFrameRate;
  param.sSpatialLayers[0].sSliceArgument.uiSliceMode = SM_SINGLE_SLICE;
  param.sSpatialLayers[0].iDLayerQp = 0;

  int rv = encoder_->InitializeExt(&param);
  ASSERT_EQ(0, rv);

  SFrameBSInfo info;
  memset(&info, 0, sizeof(SFrameBSInfo));

  // Configure invalid strides: U stride is 16, which is smaller than width/2 (32).
  int strideY = 64;
  int strideU = 16; // Invalid: must be >= 32
  int strideV = 32;

  // Generate pattern (alternative pattern is not strictly needed but we'll use a simple fill)
  std::vector<uint8_t> bufY(strideY * param.iPicHeight, 128);
  std::vector<uint8_t> bufU(strideU * (param.iPicHeight >> 1), 100);
  std::vector<uint8_t> bufV(strideV * (param.iPicHeight >> 1), 200);

  SSourcePicture pic;
  memset(&pic, 0, sizeof(SSourcePicture));
  pic.iPicWidth = param.iPicWidth;
  pic.iPicHeight = param.iPicHeight;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = strideY;
  pic.iStride[1] = strideU;
  pic.iStride[2] = strideV;
  pic.pData[0] = bufY.data();
  pic.pData[1] = bufU.data();
  pic.pData[2] = bufV.data();

  // This should reject the frame and return cmUnsupportedData
  rv = encoder_->EncodeFrame(&pic, &info);
  ASSERT_EQ(cmUnsupportedData, rv);
}
