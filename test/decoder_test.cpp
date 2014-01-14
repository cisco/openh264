#include <gtest/gtest.h>
#include "utils/HashFunctions.h"
#include "BaseDecoderTest.h"

static void UpdateHashFromPlane(SHA_CTX* ctx, const uint8_t* plane,
    int width, int height, int stride) {
  for (int i = 0; i < height; i++) {
    SHA1_Update(ctx, plane, width);
    plane += stride;
  }
}


class DecoderInitTest : public ::testing::Test, public BaseDecoderTest {
 public:
  virtual void SetUp() {
    BaseDecoderTest::SetUp();
  }
  virtual void TearDown() {
    BaseDecoderTest::TearDown();
  }
};

TEST_F(DecoderInitTest, JustInit) {}

struct FileParam {
  const char* fileName;
  const char* hashStr;
};

class DecoderOutputTest : public ::testing::WithParamInterface<FileParam>,
    public DecoderInitTest, public BaseDecoderTest::Callback {
 public:
  virtual void SetUp() {
    DecoderInitTest::SetUp();
    if (HasFatalFailure()) {
      return;
    }
    SHA1_Init(&ctx_);
  }
  virtual void onDecodeFrame(const Frame& frame) {
    const Plane& y = frame.y;
    const Plane& u = frame.u;
    const Plane& v = frame.v;
    UpdateHashFromPlane(&ctx_, y.data, y.width, y.height, y.stride);
    UpdateHashFromPlane(&ctx_, u.data, u.width, u.height, u.stride);
    UpdateHashFromPlane(&ctx_, v.data, v.width, v.height, v.stride);
  }
 protected:
  SHA_CTX ctx_;
};

TEST_P(DecoderOutputTest, CompareOutput) {
  FileParam p = GetParam();
  DecodeFile(p.fileName, this);

  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1_Final(digest, &ctx_);
  if (!HasFatalFailure()) {
    ASSERT_TRUE(CompareHash(digest, p.hashStr));
  }
}

static const FileParam kFileParamArray[] = {
  {"res/test_vd_1d.264", "5827d2338b79ff82cd091c707823e466197281d3"},
  {"res/test_vd_rc.264", "eea02e97bfec89d0418593a8abaaf55d02eaa1ca"},
  {"res/Static.264", "91dd4a7a796805b2cd015cae8fd630d96c663f42"}
};

INSTANTIATE_TEST_CASE_P(DecodeFile, DecoderOutputTest,
    ::testing::ValuesIn(kFileParamArray));
