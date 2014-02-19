#include <fstream>
#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"
#include "BaseEncoderTest.h"

static int InitWithParam(ISVCEncoder* encoder, int width,
    int height, float frameRate) {
  SEncParamBase param;
  memset (&param, 0, sizeof(SEncParamBase));

  param.fMaxFrameRate = frameRate;
  param.iPicWidth = width;
  param.iPicHeight = height;
  param.iTargetBitrate = 5000000;
  param.iInputCsp = videoFormatI420;
  return encoder->Initialize(&param);
}

BaseEncoderTest::BaseEncoderTest() : encoder_(NULL) {}

void BaseEncoderTest::SetUp() {
  int rv = CreateSVCEncoder(&encoder_);
  ASSERT_EQ(0, rv);
  ASSERT_TRUE(encoder_ != NULL);
}

void BaseEncoderTest::TearDown() {
  if (encoder_) {
    encoder_->Uninitialize();
    DestroySVCEncoder(encoder_);
  }
}

void BaseEncoderTest::EncodeStream(InputStream* in, int width, int height,
    float frameRate, Callback* cbk) {
  int rv = InitWithParam(encoder_, width, height, frameRate);
  ASSERT_TRUE(rv == cmResultSuccess);

  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = width * height * 3 / 2;

  BufferedData buf;
  buf.SetLength(frameSize);
  ASSERT_TRUE(buf.Length() == frameSize);

  SFrameBSInfo info;
  memset(&info, 0, sizeof(SFrameBSInfo));

  SSourcePicture pic;
  memset(&pic,0,sizeof(SSourcePicture));
  pic.iPicWidth = width;
  pic.iPicHeight = height;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = pic.iPicWidth;
  pic.iStride[1] = pic.iStride[2] = pic.iPicWidth>>1;
  pic.pData[0] = buf.data();
  pic.pData[1] = pic.pData[0] + width *height;
  pic.pData[2] = pic.pData[1] + (width*height>>2);
  while (in->read(buf.data(), frameSize) == frameSize) {
    rv = encoder_->EncodeFrame(&pic, &info);
    ASSERT_TRUE(rv != videoFrameTypeInvalid);
    if (rv != videoFrameTypeSkip && cbk != NULL) {
      cbk->onEncodeFrame(info);
    }
  }
}

void BaseEncoderTest::EncodeFile(const char* fileName, int width, int height,
    float frameRate, Callback* cbk) {
  FileInputStream fileStream;
  ASSERT_TRUE(fileStream.Open(fileName));
  EncodeStream(&fileStream, width, height, frameRate, cbk);
}
