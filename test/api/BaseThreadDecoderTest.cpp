#include <fstream>
#include <gtest/gtest.h>
#include "codec_def.h"
#include "codec_app_def.h"
#include "utils/BufferedData.h"
#include "BaseThreadDecoderTest.h"

static int32_t readBit (uint8_t* pBufPtr, int32_t& curBit) {
  int nIndex = curBit / 8;
  int nOffset = curBit % 8 + 1;

  curBit++;
  return (pBufPtr[nIndex] >> (8 - nOffset)) & 0x01;
}

static int32_t readBits (uint8_t* pBufPtr, int32_t& n, int32_t& curBit) {
  int r = 0;
  int i;
  for (i = 0; i < n; i++) {
    r |= (readBit (pBufPtr, curBit) << (n - i - 1));
  }
  return r;
}

static int32_t bsGetUe (uint8_t* pBufPtr, int32_t& curBit) {
  int r = 0;
  int i = 0;
  while ((readBit (pBufPtr, curBit) == 0) && (i < 32)) {
    i++;
  }
  r = readBits (pBufPtr, i, curBit);
  r += (1 << i) - 1;
  return r;
}

static int32_t readFirstMbInSlice (uint8_t* pSliceNalPtr) {
  int32_t curBit = 0;
  int32_t firstMBInSlice = bsGetUe (pSliceNalPtr + 1, curBit);
  return firstMBInSlice;
}

static int32_t ReadFrame (uint8_t* pBuf, const int32_t& iFileSize, const int32_t& bufPos) {
  int32_t bytes_available = iFileSize - bufPos;
  if (bytes_available < 4) {
    return bytes_available;
  }
  uint8_t* ptr = pBuf + bufPos;
  int32_t read_bytes = 0;
  int32_t sps_count = 0;
  int32_t pps_count = 0;
  int32_t non_idr_pict_count = 0;
  int32_t idr_pict_count = 0;
  int32_t nal_deliminator = 0;
  while (read_bytes < bytes_available - 4) {
    bool has4ByteStartCode = ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 0 && ptr[3] == 1;
    bool has3ByteStartCode = false;
    if (!has4ByteStartCode) {
      has3ByteStartCode = ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 1;
    }
    if (has4ByteStartCode || has3ByteStartCode) {
      int32_t byteOffset = has4ByteStartCode ? 4 : 3;
      uint8_t nal_unit_type = has4ByteStartCode ? (ptr[4] & 0x1F) : (ptr[3] & 0x1F);
      if (nal_unit_type == 1) {
        int32_t firstMBInSlice = readFirstMbInSlice (ptr + byteOffset);
        if (++non_idr_pict_count >= 1 && idr_pict_count >= 1 && firstMBInSlice == 0) {
          return read_bytes;
        }
        if (non_idr_pict_count >= 2 && firstMBInSlice == 0) {
          return read_bytes;
        }
      } else if (nal_unit_type == 5) {
        int32_t firstMBInSlice = readFirstMbInSlice (ptr + byteOffset);
        if (++idr_pict_count >= 1 && non_idr_pict_count >= 1 && firstMBInSlice == 0) {
          return read_bytes;
        }
        if (idr_pict_count >= 2 && firstMBInSlice == 0) {
          return read_bytes;
        }
      } else if (nal_unit_type == 7) {
        if ((++sps_count >= 1) && (non_idr_pict_count >= 1 || idr_pict_count >= 1)) {
          return read_bytes;
        }
        if (sps_count == 2) return read_bytes;
      } else if (nal_unit_type == 8) {
        if (++pps_count >= 1 && (non_idr_pict_count >= 1 || idr_pict_count >= 1)) return read_bytes;
      } else if (nal_unit_type == 9) {
        if (++nal_deliminator == 2) {
          return read_bytes;
        }
      }
      if (read_bytes >= bytes_available - 4) {
        return bytes_available;
      }
      read_bytes += 4;
      ptr += 4;
    } else {
      ++ptr;
      ++read_bytes;
    }
  }
  return bytes_available;
}

static void Write2File (FILE* pFp, unsigned char* pData[3], int iStride[2], int iWidth, int iHeight) {
  int   i;
  unsigned char*  pPtr = NULL;

  pPtr = pData[0];
  for (i = 0; i < iHeight; i++) {
    fwrite (pPtr, 1, iWidth, pFp);
    pPtr += iStride[0];
  }

  iHeight = iHeight / 2;
  iWidth = iWidth / 2;
  pPtr = pData[1];
  for (i = 0; i < iHeight; i++) {
    fwrite (pPtr, 1, iWidth, pFp);
    pPtr += iStride[1];
  }

  pPtr = pData[2];
  for (i = 0; i < iHeight; i++) {
    fwrite (pPtr, 1, iWidth, pFp);
    pPtr += iStride[1];
  }
}

static void Process (SBufferInfo* pInfo, FILE* pFp) {
  if (pFp && pInfo->pDst[0] && pInfo->pDst[1] && pInfo->pDst[2] && pInfo) {
    int iStride[2];
    int iWidth = pInfo->UsrData.sSystemBuffer.iWidth;
    int iHeight = pInfo->UsrData.sSystemBuffer.iHeight;
    iStride[0] = pInfo->UsrData.sSystemBuffer.iStride[0];
    iStride[1] = pInfo->UsrData.sSystemBuffer.iStride[1];

    Write2File (pFp, (unsigned char**)pInfo->pDst, iStride, iWidth, iHeight);
  }
}

BaseThreadDecoderTest::BaseThreadDecoderTest()
  : decoder_ (NULL), uiTimeStamp (0), pYuvFile (NULL), bEnableYuvDumpTest (false), decodeStatus_ (OpenFile) {
}

int32_t BaseThreadDecoderTest::SetUp() {
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
  int iThreadCount = (rand() % 2) + 2;
  decoder_->SetOption (DECODER_OPTION_NUM_OF_THREADS, &iThreadCount);

  rv = decoder_->Initialize (&decParam);
  EXPECT_EQ (0, rv);
  return (int32_t)rv;
}

void BaseThreadDecoderTest::TearDown() {
  if (decoder_ != NULL) {
    decoder_->Uninitialize();
    WelsDestroyDecoder (decoder_);
  }
}


void BaseThreadDecoderTest::DecodeFrame (const uint8_t* src, size_t sliceSize, Callback* cbk) {
  SBufferInfo bufInfo;
  memset (pData, 0, sizeof (pData));
  memset (&bufInfo, 0, sizeof (SBufferInfo));
  bufInfo.uiInBsTimeStamp = ++uiTimeStamp;

  DECODING_STATE rv = decoder_->DecodeFrameNoDelay (src, (int) sliceSize, pData, &bufInfo);
  ASSERT_TRUE (rv == dsErrorFree);
  sBufInfo = bufInfo;
  if (sBufInfo.iBufferStatus == 1 && cbk != NULL) {
    if (bEnableYuvDumpTest) {
      Process (&sBufInfo, pYuvFile);
    }
    const Frame frame = {
      {
        // y plane
        sBufInfo.pDst[0],
        bufInfo.UsrData.sSystemBuffer.iWidth,
        bufInfo.UsrData.sSystemBuffer.iHeight,
        bufInfo.UsrData.sSystemBuffer.iStride[0]
      },
      {
        // u plane
        sBufInfo.pDst[1],
        sBufInfo.UsrData.sSystemBuffer.iWidth / 2,
        sBufInfo.UsrData.sSystemBuffer.iHeight / 2,
        sBufInfo.UsrData.sSystemBuffer.iStride[1]
      },
      {
        // v plane
        sBufInfo.pDst[2],
        sBufInfo.UsrData.sSystemBuffer.iWidth / 2,
        sBufInfo.UsrData.sSystemBuffer.iHeight / 2,
        sBufInfo.UsrData.sSystemBuffer.iStride[1]
      },
    };
    cbk->onDecodeFrame (frame);
  }
}
void BaseThreadDecoderTest::FlushFrame (Callback* cbk) {
  SBufferInfo bufInfo;
  memset (pData, 0, sizeof (pData));
  memset (&bufInfo, 0, sizeof (SBufferInfo));

  DECODING_STATE rv = decoder_->FlushFrame (pData, &bufInfo);
  ASSERT_TRUE (rv == dsErrorFree);
  sBufInfo = bufInfo;
  if (sBufInfo.iBufferStatus == 1 && cbk != NULL) {
    if (bEnableYuvDumpTest) {
      Process (&sBufInfo, pYuvFile);
    }
    const Frame frame = {
      {
        // y plane
        sBufInfo.pDst[0],
        sBufInfo.UsrData.sSystemBuffer.iWidth,
        sBufInfo.UsrData.sSystemBuffer.iHeight,
        sBufInfo.UsrData.sSystemBuffer.iStride[0]
      },
      {
        // u plane
        sBufInfo.pDst[1],
        sBufInfo.UsrData.sSystemBuffer.iWidth / 2,
        sBufInfo.UsrData.sSystemBuffer.iHeight / 2,
        sBufInfo.UsrData.sSystemBuffer.iStride[1]
      },
      {
        // v plane
        sBufInfo.pDst[2],
        sBufInfo.UsrData.sSystemBuffer.iWidth / 2,
        sBufInfo.UsrData.sSystemBuffer.iHeight / 2,
        sBufInfo.UsrData.sSystemBuffer.iStride[1]
      },
    };
    cbk->onDecodeFrame (frame);
  }
}
bool BaseThreadDecoderTest::ThreadDecodeFile (const char* fileName, Callback* cbk) {
  std::ifstream file (fileName, std::ios::in | std::ios::binary);
  if (!file.is_open())
    return false;

  BufferedData buf;
  char b;
  for (;;) {
    file.read (&b, 1);
    if (file.gcount() != 1) { // end of file
      break;
    }
    if (!buf.PushBack (b)) {
      std::cout << "unable to allocate memory" << std::endl;
      return false;
    }
  }

  std::string outFileName = std::string (fileName);
  size_t pos = outFileName.find_last_of (".");
  if (bEnableYuvDumpTest) {
    outFileName = outFileName.substr (0, pos) + std::string (".yuv");
    pYuvFile = fopen (outFileName.c_str(), "wb");
  }

  uiTimeStamp = 0;
  memset (&sBufInfo, 0, sizeof (SBufferInfo));
  int32_t bufPos = 0;
  int32_t bytesConsumed = 0;
  int32_t fileSize = buf.Length();
  while (bytesConsumed < fileSize) {
    int32_t frameSize = ReadFrame (buf.data(), fileSize, bufPos);
    if (::testing::Test::HasFatalFailure()) {
      return false;
    }
    uint8_t* frame_ptr = buf.data() + bufPos;
    DecodeFrame (frame_ptr, frameSize, cbk);
    if (::testing::Test::HasFatalFailure()) {
      return false;
    }
    bufPos += frameSize;
    bytesConsumed += frameSize;
  }

  int32_t iEndOfStreamFlag = 1;
  decoder_->SetOption (DECODER_OPTION_END_OF_STREAM, &iEndOfStreamFlag);

  // Flush out last frames in decoder buffer
  int32_t num_of_frames_in_buffer = 0;
  decoder_->GetOption (DECODER_OPTION_NUM_OF_FRAMES_REMAINING_IN_BUFFER, &num_of_frames_in_buffer);
  for (int32_t i = 0; i < num_of_frames_in_buffer; ++i) {
    FlushFrame (cbk);
  }
  if (bEnableYuvDumpTest) {
    fclose (pYuvFile);
  }
  return true;
}

bool BaseThreadDecoderTest::Open (const char* fileName) {
  if (decodeStatus_ == OpenFile) {
    file_.open (fileName, std::ios_base::out | std::ios_base::binary);
    if (file_.is_open()) {
      decodeStatus_ = Decoding;
      return true;
    }
  }
  return false;
}
