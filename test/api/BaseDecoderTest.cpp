#include <fstream>
#include <gtest/gtest.h>
#include "codec_def.h"
#include "codec_app_def.h"
#include "utils/BufferedData.h"
#include "BaseDecoderTest.h"

static bool ReadFrame (std::ifstream* file, BufferedData* buf) {
  // start code of a frame is {0, 0, 0, 1}
  int zeroCount = 0;
  char b;

  buf->Clear();
  for (;;) {
    file->read (&b, 1);
    if (file->gcount() != 1) { // end of file
      return true;
    }
    if (!buf->PushBack (b)) {
      std::cout << "unable to allocate memory" << std::endl;
      return false;
    }

    if (buf->Length() <= 4) {
      continue;
    }

    if (zeroCount < 3) {
      zeroCount = b != 0 ? 0 : zeroCount + 1;
    } else {
      if (b == 1) {
        if (file->seekg (-4, file->cur).good()) {
          if (-1 == buf->SetLength(buf->Length() - 4))
            return false;
          return true;
        } else {
          std::cout << "unable to seek file" << std::endl;
          return false;
        }
      } else if (b == 0) {
        zeroCount = 3;
      } else {
        zeroCount = 0;
      }
    }
  }
}

BaseDecoderTest::BaseDecoderTest()
  : decoder_ (NULL), decodeStatus_ (OpenFile) {}

int32_t BaseDecoderTest::SetUp() {
  long rv = WelsCreateDecoder (&decoder_);
  EXPECT_EQ (0, rv);
  EXPECT_TRUE (decoder_ != NULL);
  if (decoder_ == NULL) {
    return rv;
  }

  SDecodingParam decParam;
  memset (&decParam, 0, sizeof (SDecodingParam));
  decParam.uiTargetDqLayer = UCHAR_MAX;
  decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
  decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

  rv = decoder_->Initialize (&decParam);
  EXPECT_EQ (0, rv);
  return (int32_t)rv;
}

void BaseDecoderTest::TearDown() {
  if (decoder_ != NULL) {
    decoder_->Uninitialize();
    WelsDestroyDecoder (decoder_);
  }
}


void BaseDecoderTest::DecodeFrame (const uint8_t* src, size_t sliceSize, Callback* cbk) {
  uint8_t* data[3];
  SBufferInfo bufInfo;
  memset (data, 0, sizeof (data));
  memset (&bufInfo, 0, sizeof (SBufferInfo));

  DECODING_STATE rv = decoder_->DecodeFrame2 (src, (int) sliceSize, data, &bufInfo);
  ASSERT_TRUE (rv == dsErrorFree);

  if (bufInfo.iBufferStatus == 1 && cbk != NULL) {
    const Frame frame = {
      {
        // y plane
        data[0],
        bufInfo.UsrData.sSystemBuffer.iWidth,
        bufInfo.UsrData.sSystemBuffer.iHeight,
        bufInfo.UsrData.sSystemBuffer.iStride[0]
      },
      {
        // u plane
        data[1],
        bufInfo.UsrData.sSystemBuffer.iWidth / 2,
        bufInfo.UsrData.sSystemBuffer.iHeight / 2,
        bufInfo.UsrData.sSystemBuffer.iStride[1]
      },
      {
        // v plane
        data[2],
        bufInfo.UsrData.sSystemBuffer.iWidth / 2,
        bufInfo.UsrData.sSystemBuffer.iHeight / 2,
        bufInfo.UsrData.sSystemBuffer.iStride[1]
      },
    };
    cbk->onDecodeFrame (frame);
  }
}
void BaseDecoderTest::FlushFrame (Callback* cbk) {
  uint8_t* data[3];
  SBufferInfo bufInfo;
  memset (data, 0, sizeof (data));
  memset (&bufInfo, 0, sizeof (SBufferInfo));

  DECODING_STATE rv = decoder_->FlushFrame (data, &bufInfo);
  ASSERT_TRUE (rv == dsErrorFree);

  if (bufInfo.iBufferStatus == 1 && cbk != NULL) {
    const Frame frame = {
      {
        // y plane
        data[0],
        bufInfo.UsrData.sSystemBuffer.iWidth,
        bufInfo.UsrData.sSystemBuffer.iHeight,
        bufInfo.UsrData.sSystemBuffer.iStride[0]
      },
      {
        // u plane
        data[1],
        bufInfo.UsrData.sSystemBuffer.iWidth / 2,
        bufInfo.UsrData.sSystemBuffer.iHeight / 2,
        bufInfo.UsrData.sSystemBuffer.iStride[1]
      },
      {
        // v plane
        data[2],
        bufInfo.UsrData.sSystemBuffer.iWidth / 2,
        bufInfo.UsrData.sSystemBuffer.iHeight / 2,
        bufInfo.UsrData.sSystemBuffer.iStride[1]
      },
    };
    cbk->onDecodeFrame (frame);
  }
}
bool BaseDecoderTest::DecodeFile (const char* fileName, Callback* cbk) {
  std::ifstream file (fileName, std::ios::in | std::ios::binary);
  if (!file.is_open())
    return false;

  BufferedData buf;
  while (true) {
    if (false == ReadFrame(&file, &buf))
      return false;
    if (::testing::Test::HasFatalFailure()) {
      return false;
    }
    if (buf.Length() == 0) {
      break;
    }
    DecodeFrame (buf.data(), buf.Length(), cbk);
    if (::testing::Test::HasFatalFailure()) {
      return false;
    }
  }

  int32_t iEndOfStreamFlag = 1;
  decoder_->SetOption (DECODER_OPTION_END_OF_STREAM, &iEndOfStreamFlag);

  // Get pending last frame
  DecodeFrame (NULL, 0, cbk);
  // Flush out last frames in decoder buffer
  int32_t num_of_frames_in_buffer = 0;
  decoder_->GetOption (DECODER_OPTION_NUM_OF_FRAMES_REMAINING_IN_BUFFER, &num_of_frames_in_buffer);
  for (int32_t i = 0; i < num_of_frames_in_buffer; ++i) {
    FlushFrame (cbk);
  }
  return true;
}

bool BaseDecoderTest::Open (const char* fileName) {
  if (decodeStatus_ == OpenFile) {
    file_.open (fileName, std::ios_base::out | std::ios_base::binary);
    if (file_.is_open()) {
      decodeStatus_ = Decoding;
      return true;
    }
  }
  return false;
}

bool BaseDecoderTest::DecodeNextFrame (Callback* cbk) {
  switch (decodeStatus_) {
  case Decoding:
    if (false == ReadFrame(&file_, &buf_))
      return false;
    if (::testing::Test::HasFatalFailure()) {
      return false;
    }
    if (buf_.Length() == 0) {
      decodeStatus_ = EndOfStream;
      return true;
    }
    DecodeFrame (buf_.data(), buf_.Length(), cbk);
    if (::testing::Test::HasFatalFailure()) {
      return false;
    }
    return true;
  case EndOfStream: {
    int32_t iEndOfStreamFlag = 1;
    decoder_->SetOption (DECODER_OPTION_END_OF_STREAM, &iEndOfStreamFlag);
    DecodeFrame (NULL, 0, cbk);
    decodeStatus_ = End;
    break;
  }
  case OpenFile:
  case End:
    break;
  }
  return false;
}
