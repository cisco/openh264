#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"
#include "BaseDecoderTest.h"
#include "BaseEncoderTest.h"
#include <string>

struct EncodeDecodeFileParamBase {
  const char* fileName;
  int width;
  int height;
  float frameRate;
};

class EncodeDecodeTestBase : public ::testing::Test,
  public BaseEncoderTest, public BaseDecoderTest {
 public:
  virtual void SetUp() {
    BaseEncoderTest::SetUp();
    BaseDecoderTest::SetUp();
  }

  virtual void TearDown() {
    BaseEncoderTest::TearDown();
    BaseDecoderTest::TearDown();
  }

  virtual void prepareParam (int width, int height, float framerate) {
    memset (&param_, 0, sizeof (SEncParamExt));
    param_.iUsageType = CAMERA_VIDEO_REAL_TIME;
    param_.iPicWidth = width;
    param_.iPicHeight = height;
    param_.fMaxFrameRate = framerate;
    param_.iRCMode = RC_OFF_MODE; //rc off
    param_.iMultipleThreadIdc = 1; //single thread
    param_.sSpatialLayers[0].iVideoWidth = width;
    param_.sSpatialLayers[0].iVideoHeight = height;
    param_.sSpatialLayers[0].fFrameRate = framerate;
    param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
  }

  virtual void encToDecData (const SFrameBSInfo& info, int& len) {
    len = 0;
    for (int i = 0; i < info.iLayerNum; ++i) {
      const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
      for (int j = 0; j < layerInfo.iNalCount; ++j) {
        len += layerInfo.pNalLengthInByte[j];
      }
    }
  }

 protected:
  SEncParamExt param_;
  BufferedData buf_;
  SBufferInfo dstBufInfo_;
};

class EncodeDecodeTestVclNal : public EncodeDecodeTestBase {
 public:
  void SetUp() {
    EncodeDecodeTestBase::SetUp();
  }

  void TearDown() {
    EncodeDecodeTestBase::TearDown();
  }

  void prepareParam (int width, int height, float framerate) {
    EncodeDecodeTestBase::prepareParam (width, height, framerate);
  }
};

static const EncodeDecodeFileParamBase kFileParamArray =
{"res/CiscoVT2people_160x96_6fps.yuv", 160, 96, 6.0f};

TEST_F (EncodeDecodeTestVclNal, DecoderVclNal) {
  EncodeDecodeFileParamBase p = kFileParamArray;
  FileInputStream fileStream;
#if defined(ANDROID_NDK)
  std::string filename = std::string ("/sdcard/") + p.fileName;
  ASSERT_TRUE (fileStream.Open (filename.c_str()));
#else
  ASSERT_TRUE (fileStream.Open (p.fileName));
#endif

  prepareParam (p.width, p.height, p.frameRate);
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  //init for encoder
  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = p.width * p.height * 3 / 2;

  buf_.SetLength (frameSize);
  ASSERT_TRUE (buf_.Length() == (size_t)frameSize);

  SFrameBSInfo info;
  memset (&info, 0, sizeof (SFrameBSInfo));

  SSourcePicture pic;
  memset (&pic, 0, sizeof (SSourcePicture));
  pic.iPicWidth = p.width;
  pic.iPicHeight = p.height;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = pic.iPicWidth;
  pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
  pic.pData[0] = buf_.data();
  pic.pData[1] = pic.pData[0] + p.width * p.height;
  pic.pData[2] = pic.pData[1] + (p.width * p.height >> 2);
  while (fileStream.read (buf_.data(), frameSize) == frameSize) {
    rv = encoder_->EncodeFrame (&pic, &info);
    ASSERT_TRUE (rv == cmResultSuccess);
    //decoding after each encoding frame
    int vclNal, len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_TRUE (rv == cmResultSuccess);
    rv = decoder_->GetOption (DECODER_OPTION_VCL_NAL, &vclNal);
    EXPECT_EQ (vclNal, FEEDBACK_UNKNOWN_NAL); //no reconstruction, unknown return
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_TRUE (rv == cmResultSuccess);
    rv = decoder_->GetOption (DECODER_OPTION_VCL_NAL, &vclNal);
    EXPECT_EQ (vclNal, FEEDBACK_VCL_NAL);

  } //while
  //ignore last frame
}

