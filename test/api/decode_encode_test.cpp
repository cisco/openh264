#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/HashFunctions.h"
#include "utils/BufferedData.h"
#include "utils/InputStream.h"
#include "BaseDecoderTest.h"
#include "BaseEncoderTest.h"

static void UpdateHashFromFrame(const SFrameBSInfo& info, SHA1Context* ctx) {
  for (int i = 0; i < info.iLayerNum; ++i) {
    const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
    int layerSize = 0;
    for (int j = 0; j < layerInfo.iNalCount; ++j) {
      layerSize += layerInfo.iNalLengthInByte[j];
    }
    SHA1Input(ctx, layerInfo.pBsBuf, layerSize);
  }
}

static void WritePlaneBuffer(BufferedData* buf, const uint8_t* plane,
    int width, int height, int stride) {
  for (int i = 0; i < height; i++) {
    if (!buf->PushBack(plane, width)) {
      FAIL() << "unable to allocate memory";
    }
    plane += stride;
  }
}

struct DecodeEncodeFileParam {
  const char* fileName;
  const char* hashStr;
  int width;
  int height;
  float frameRate;
};

class DecodeEncodeTest : public ::testing::TestWithParam<DecodeEncodeFileParam>,
    public BaseDecoderTest, public BaseDecoderTest::Callback,
    public BaseEncoderTest , public BaseEncoderTest::Callback,
    public InputStream {
 public:
  virtual void SetUp() {
    BaseDecoderTest::SetUp();
    if (HasFatalFailure()) {
      return;
    }
    BaseEncoderTest::SetUp();
    if (HasFatalFailure()) {
      return;
    }
    SHA1Reset(&ctx_);
  }

  virtual void TearDown() {
    BaseDecoderTest::TearDown();
    BaseEncoderTest::TearDown();
  }

  virtual void onDecodeFrame(const Frame& frame) {
    const Plane& y = frame.y;
    const Plane& u = frame.u;
    const Plane& v = frame.v;
    WritePlaneBuffer(&buf_, y.data, y.width, y.height, y.stride);
    WritePlaneBuffer(&buf_, u.data, u.width, u.height, u.stride);
    WritePlaneBuffer(&buf_, v.data, v.width, v.height, v.stride);
  }

  virtual void onEncodeFrame(const SFrameBSInfo& frameInfo) {
    UpdateHashFromFrame(frameInfo, &ctx_);
  }

  virtual int read(void* ptr, size_t len) {
    while (buf_.Length() < len) {
      bool hasNext = DecodeNextFrame(this);
      if (HasFatalFailure()) {
        return -1;
      }
      if (!hasNext) {
        if (buf_.Length() == 0) {
          return -1;
        }
        break;
      }
    }
    return buf_.PopFront(static_cast<uint8_t*>(ptr), len);
  }

 protected:
  SHA1Context ctx_;
  BufferedData buf_;
};

TEST_P(DecodeEncodeTest, CompareOutput) {
  DecodeEncodeFileParam p = GetParam();

  ASSERT_TRUE(Open(p.fileName));
  EncodeStream(this, CAMERA_VIDEO_REAL_TIME,p.width, p.height, p.frameRate, SM_SINGLE_SLICE, false, 1, this);
  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1Result(&ctx_, digest);
  if (!HasFatalFailure()) {
    CompareHash(digest, p.hashStr);
  }
}

static const DecodeEncodeFileParam kFileParamArray[] = {
  {"res/test_vd_1d.264", "c73c1cc9b7dbab51f48cf41453073bb11337a445", 320, 192, 12.0f},
  {"res/test_vd_rc.264", "593ab31fdc67cbad7373abbf7d08daf2771fb229", 320, 192, 12.0f},
};

INSTANTIATE_TEST_CASE_P(DecodeEncodeFile, DecodeEncodeTest,
    ::testing::ValuesIn(kFileParamArray));
