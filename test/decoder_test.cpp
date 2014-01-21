#include <gtest/gtest.h>
#include <stdint.h>
#include <limits.h>
#include <fstream>

#include "codec_api.h"

#include "utils/BufferedData.h"
#include "utils/HashFunctions.h"

static void UpdateHashFromPlane(SHA_CTX* ctx, const uint8_t* plane,
    int width, int height, int stride) {
  for (int i = 0; i < height; i++) {
    SHA1_Update(ctx, plane, width);
    plane += stride;
  }
}

/**
 * @return frame size (>= 0), or -1 for memory allocation error.
 */
static int ReadFrame(std::ifstream* file, BufferedData* buf) {
  // start code of a frame is {0, 0, 0, 1}
  int zeroCount = 0;
  char b;

  for (;;) {
    file->read(&b, 1);
    if (file->gcount() != 1) {
      break;
    }
    if (!buf->Push(b)) {
      return -1;
    }

    if (buf->Length() <= 4) {
      continue;
    }

    if (zeroCount < 3) {
      zeroCount = b != 0 ? 0 : zeroCount + 1;
    } else {
      if (b == 1) {
        if (file->seekg(-4, file->cur).good()) {
          return buf->Length() - 4;
        } else {
          // seeking fails
          return -1;
        }
      } else if (b == 0) {
        zeroCount = 3;
      } else {
        zeroCount = 0;
      }
    }
  }
  return buf->Length();
}

/**
 * @return true if a frame is decoded successfully, otherwise false.
 */
static bool DecodeAndProcess(ISVCDecoder* decoder, const uint8_t* src,
    int sliceSize, SHA_CTX* ctx) {
  void* data[3];
  SBufferInfo bufInfo;
  memset(data, 0, sizeof(data));
  memset(&bufInfo, 0, sizeof(SBufferInfo));

  DECODING_STATE rv = decoder->DecodeFrame(src, sliceSize, data, &bufInfo);
  if (rv != dsErrorFree) {
    return false;
  }

  if (bufInfo.iBufferStatus == 1) {
    // y plane
    UpdateHashFromPlane(ctx, static_cast<uint8_t*>(data[0]),
        bufInfo.UsrData.sSystemBuffer.iWidth,
        bufInfo.UsrData.sSystemBuffer.iHeight,
        bufInfo.UsrData.sSystemBuffer.iStride[0]);
    // u plane
    UpdateHashFromPlane(ctx, static_cast<uint8_t*>(data[1]),
        bufInfo.UsrData.sSystemBuffer.iWidth / 2,
        bufInfo.UsrData.sSystemBuffer.iHeight / 2,
        bufInfo.UsrData.sSystemBuffer.iStride[1]);
    // v plane
    UpdateHashFromPlane(ctx, static_cast<uint8_t*>(data[2]),
        bufInfo.UsrData.sSystemBuffer.iWidth / 2,
        bufInfo.UsrData.sSystemBuffer.iHeight / 2,
        bufInfo.UsrData.sSystemBuffer.iStride[1]);
  }
  return true;
}

static void CompareFileToHash(ISVCDecoder* decoder,
    const char* fileName, const char* hashStr) {
  std::ifstream file(fileName, std::ios::in | std::ios::binary);
  ASSERT_TRUE(file.is_open());

  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA_CTX ctx;
  SHA1_Init(&ctx);

  BufferedData buf;
  int sliceSize;

  while ((sliceSize = ReadFrame(&file, &buf)) > 0) {
    if (DecodeAndProcess(decoder, buf.data(), sliceSize, &ctx)) {
      buf.Clear();
    } else {
      SHA1_Final(digest, &ctx);
      FAIL() << "unable to decode frame";
    }
  }

  if (sliceSize < 0) {
    SHA1_Final(digest, &ctx);
    FAIL() << "unable to allocate memory";
  }

  int32_t iEndOfStreamFlag = 1;
  decoder->SetOption(DECODER_OPTION_END_OF_STREAM, &iEndOfStreamFlag);

  // Get pending last frame
  if (!DecodeAndProcess(decoder, NULL, 0, &ctx)) {
    SHA1_Final(digest, &ctx);
    FAIL() << "unable to decode last frame";
  }

  SHA1_Final(digest, &ctx);
  ASSERT_TRUE(CompareHash(digest, hashStr));
}

class DecoderInitTest : public ::testing::Test {
 public:
  DecoderInitTest() : decoder_(NULL) {}

  virtual void SetUp() {
    long rv = CreateDecoder(&decoder_);
    ASSERT_EQ(0, rv);
    ASSERT_TRUE(decoder_ != NULL);

    SDecodingParam decParam;
    memset(&decParam, 0, sizeof(SDecodingParam));
    decParam.iOutputColorFormat  = videoFormatI420;
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.uiEcActiveFlag  = 1;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder_->Initialize(&decParam, INIT_TYPE_PARAMETER_BASED);
    ASSERT_EQ(0, rv);
  }

  virtual void TearDown() {
    if (decoder_ != NULL) {
      decoder_->Uninitialize();
      DestroyDecoder(decoder_);
    }
  }

 protected:
  ISVCDecoder* decoder_;
};


TEST_F(DecoderInitTest, JustInit) {
}

struct FileParam {
  const char* fileName;
  const char* hashStr;
};

class DecoderOutputTest : public DecoderInitTest,
    public ::testing::WithParamInterface<FileParam> {
};

TEST_P(DecoderOutputTest, CompareOutput) {
  FileParam p = GetParam();
  CompareFileToHash(decoder_, p.fileName, p.hashStr);
}

static const FileParam kFileParamArray[] = {
  {"res/test_vd_1d.264", "5827d2338b79ff82cd091c707823e466197281d3"},
  {"res/test_vd_rc.264", "eea02e97bfec89d0418593a8abaaf55d02eaa1ca"},
  {"res/Static.264", "91dd4a7a796805b2cd015cae8fd630d96c663f42"}
};

INSTANTIATE_TEST_CASE_P(DecodeFile, DecoderOutputTest,
    ::testing::ValuesIn(kFileParamArray));
