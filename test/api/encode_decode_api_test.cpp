#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"
#include "BaseDecoderTest.h"
#include "BaseEncoderTest.h"
#include <string>

struct EncodeDecodeFileParamBase {
  int numframes;
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
{300, 160, 96, 6.0f};

TEST_F (EncodeDecodeTestVclNal, DecoderVclNal) {
  EncodeDecodeFileParamBase p = kFileParamArray;

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
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
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
    iIdx++;
  } //while
  //ignore last frame
}

TEST_F (EncodeDecodeTestVclNal, GetOptionFramenum) {
  EncodeDecodeFileParamBase p = kFileParamArray;
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
  int32_t iEncFrameNum = -1;
  int32_t iDecFrameNum;
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    ASSERT_TRUE (rv == cmResultSuccess);
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_FRAME_NUM, &iDecFrameNum);
    EXPECT_EQ (iDecFrameNum, -1);
    iEncFrameNum++;
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_FRAME_NUM, &iDecFrameNum);
    EXPECT_EQ (iEncFrameNum, iDecFrameNum);
    iIdx++;
  } //while
  //ignore last frame
}

TEST_F (EncodeDecodeTestVclNal, GetOptionIDR) {
  EncodeDecodeFileParamBase p = kFileParamArray;
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
  int32_t iEncCurIdrPicId = 0;
  int32_t iDecCurIdrPicId;
  int32_t iIDRPeriod = 1;
  int32_t iSpsPpsIdAddition = 0;
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
    iSpsPpsIdAddition = rand() % 2;
    iIDRPeriod = (rand() % 150) + 1;
    encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
    encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
    rv = encoder_->EncodeFrame (&pic, &info);
    ASSERT_TRUE (rv == cmResultSuccess);
    if (info.eFrameType == videoFrameTypeIDR) {
      iEncCurIdrPicId = (iSpsPpsIdAddition == 0) ? 0 : (iEncCurIdrPicId + 1);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_IDR_PIC_ID, &iDecCurIdrPicId);
    EXPECT_EQ (iDecCurIdrPicId, iEncCurIdrPicId);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_IDR_PIC_ID, &iDecCurIdrPicId);
    EXPECT_EQ (iDecCurIdrPicId, iEncCurIdrPicId);
    iIdx++;
  } //while
  //ignore last frame
}


