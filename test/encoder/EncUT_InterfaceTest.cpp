#include <gtest/gtest.h>
#include "codec_def.h"
#include "codec_api.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"
#include "BaseEncoderTest.h"

class EncInterfaceCallTest : public ::testing::Test, public BaseEncoderTest {
 public:
  virtual void SetUp() {
    BaseEncoderTest::SetUp();
  };
  virtual void TearDown() {
    BaseEncoderTest::TearDown();
  };

  virtual void onEncodeFrame (const SFrameBSInfo& frameInfo) {
    //nothing
  }
  //testing case

};


TEST_F (EncInterfaceCallTest, BaseParameterVerify) {
  int uiTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &uiTraceLevel);

  int ret = cmResultSuccess;
  SEncParamBase baseparam;
  memset (&baseparam, 0, sizeof (SEncParamBase));

  baseparam.iPicWidth = 0;
  baseparam.iPicHeight = 7896;

  ret =  encoder_->Initialize (&baseparam);
  EXPECT_EQ (ret, static_cast<int> (cmInitParaError));

  uiTraceLevel = WELS_LOG_ERROR;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &uiTraceLevel);
}

void outputData() {

}

TEST_F (EncInterfaceCallTest, SetOptionLTR) {

  int iTotalFrameNum = 100;
  int iFrameNum = 0;
  int frameSize = 0;
  int ret = cmResultSuccess;
  int width = 320;
  int height = 192;


  SEncParamBase baseparam;
  memset (&baseparam, 0, sizeof (SEncParamBase));

  baseparam.iUsageType = CAMERA_VIDEO_REAL_TIME;
  baseparam.fMaxFrameRate = 12;
  baseparam.iPicWidth = width;
  baseparam.iPicHeight = height;
  baseparam.iTargetBitrate = 5000000;
  encoder_->Initialize (&baseparam);

  frameSize = width * height * 3 / 2;

  BufferedData buf;
  buf.SetLength (frameSize);
  ASSERT_TRUE (buf.Length() == (size_t)frameSize);

  SFrameBSInfo info;
  memset (&info, 0, sizeof (SFrameBSInfo));

  SSourcePicture pic;
  memset (&pic, 0, sizeof (SSourcePicture));
  pic.iPicWidth = width;
  pic.iPicHeight = height;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = pic.iPicWidth;
  pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
  pic.pData[0] = buf.data();
  pic.pData[1] = pic.pData[0] + width * height;
  pic.pData[2] = pic.pData[1] + (width * height >> 2);

  SLTRConfig config;
  config.bEnableLongTermReference = true;
  config.iLTRRefNum = rand() % 4;
  encoder_->SetOption (ENCODER_OPTION_LTR, &config);
  do {
    FileInputStream fileStream;
    ASSERT_TRUE (fileStream.Open ("res/CiscoVT2people_320x192_12fps.yuv"));

    while (fileStream.read (buf.data(), frameSize) == frameSize) {
      ret = encoder_->EncodeFrame (&pic, &info);
      ASSERT_TRUE (ret == cmResultSuccess);
      if (info.eFrameType != videoFrameTypeSkip) {
        this->onEncodeFrame (info);
        iFrameNum++;
      }
    }
  } while (iFrameNum < iTotalFrameNum);

}