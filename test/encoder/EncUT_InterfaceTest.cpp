#include <gtest/gtest.h>
#include "codec_def.h"
#include "codec_api.h"
#include "BaseEncoderTest.h"

class EncInterfaceCallTest : public ::testing::Test, public BaseEncoderTest {
 public:
  virtual void SetUp() {
    BaseEncoderTest::SetUp();
  };
  virtual void TearDown() {
    BaseEncoderTest::TearDown();
  };

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
