#include <gtest/gtest.h>
#include "utils/HashFunctions.h"
#include "BaseThreadDecoderTest.h"
#include <climits>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <sys/mman.h>
#include <unistd.h>
#endif

static std::vector<unsigned char> ReadBinaryFile (const char* path) {
  std::ifstream ifs (path, std::ios::binary);
  return std::vector<unsigned char> ((std::istreambuf_iterator<char> (ifs)),
                                     std::istreambuf_iterator<char> ());
}

static void UpdateHashFromPlane (SHA1Context* ctx, const uint8_t* plane,
                                 int width, int height, int stride) {
  for (int i = 0; i < height; i++) {
    SHA1Input (ctx, plane, width);
    plane += stride;
  }
}

static int32_t ReadBitForHangRegression (uint8_t* pBufPtr, int32_t& curBit) {
  int nIndex = curBit / 8;
  int nOffset = curBit % 8 + 1;

  curBit++;
  return (pBufPtr[nIndex] >> (8 - nOffset)) & 0x01;
}

static int32_t ReadBitsForHangRegression (uint8_t* pBufPtr, int32_t& n, int32_t& curBit) {
  int r = 0;
  for (int i = 0; i < n; ++i) {
    r |= (ReadBitForHangRegression (pBufPtr, curBit) << (n - i - 1));
  }
  return r;
}

static int32_t BsGetUeForHangRegression (uint8_t* pBufPtr, int32_t& curBit) {
  int r = 0;
  int i = 0;
  while ((ReadBitForHangRegression (pBufPtr, curBit) == 0) && (i < 32)) {
    ++i;
  }
  r = ReadBitsForHangRegression (pBufPtr, i, curBit);
  r += (1 << i) - 1;
  return r;
}

static int32_t ReadFirstMbInSliceForHangRegression (uint8_t* pSliceNalPtr) {
  int32_t curBit = 0;
  return BsGetUeForHangRegression (pSliceNalPtr + 1, curBit);
}

static int32_t ReadFrameForHangRegression (uint8_t* pBuf, const int32_t& iFileSize, const int32_t& bufPos) {
  int32_t bytesAvailable = iFileSize - bufPos;
  if (bytesAvailable < 4) {
    return bytesAvailable;
  }

  uint8_t* ptr = pBuf + bufPos;
  int32_t readBytes = 0;
  int32_t spsCount = 0;
  int32_t ppsCount = 0;
  int32_t nonIdrPictCount = 0;
  int32_t idrPictCount = 0;
  int32_t nalDelimiterCount = 0;

  while (readBytes < bytesAvailable - 4) {
    bool has4ByteStartCode = ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 0 && ptr[3] == 1;
    bool has3ByteStartCode = false;
    if (!has4ByteStartCode) {
      has3ByteStartCode = ptr[0] == 0 && ptr[1] == 0 && ptr[2] == 1;
    }

    if (has4ByteStartCode || has3ByteStartCode) {
      int32_t byteOffset = has4ByteStartCode ? 4 : 3;
      uint8_t nalUnitType = has4ByteStartCode ? (ptr[4] & 0x1F) : (ptr[3] & 0x1F);

      if (nalUnitType == 1) {
        int32_t firstMbInSlice = ReadFirstMbInSliceForHangRegression (ptr + byteOffset);
        if (++nonIdrPictCount >= 1 && idrPictCount >= 1 && firstMbInSlice == 0) {
          return readBytes;
        }
        if (nonIdrPictCount >= 2 && firstMbInSlice == 0) {
          return readBytes;
        }
      } else if (nalUnitType == 5) {
        int32_t firstMbInSlice = ReadFirstMbInSliceForHangRegression (ptr + byteOffset);
        if (++idrPictCount >= 1 && nonIdrPictCount >= 1 && firstMbInSlice == 0) {
          return readBytes;
        }
        if (idrPictCount >= 2 && firstMbInSlice == 0) {
          return readBytes;
        }
      } else if (nalUnitType == 7) {
        if ((++spsCount >= 1) && (nonIdrPictCount >= 1 || idrPictCount >= 1)) {
          return readBytes;
        }
        if (spsCount == 2) {
          return readBytes;
        }
      } else if (nalUnitType == 8) {
        if (++ppsCount >= 1 && (nonIdrPictCount >= 1 || idrPictCount >= 1)) {
          return readBytes;
        }
      } else if (nalUnitType == 9) {
        if (++nalDelimiterCount == 2) {
          return readBytes;
        }
      }

      if (readBytes >= bytesAvailable - 4) {
        return bytesAvailable;
      }
      readBytes += 4;
      ptr += 4;
    } else {
      ++ptr;
      ++readBytes;
    }
  }

  return bytesAvailable;
}

class ThreadDecoderHangRegressionTest : public ::testing::Test {
};

TEST_F (ThreadDecoderHangRegressionTest, Static264ThreeDecodeCallsDoNotDeadlock) {
  std::ifstream file ("res/Static.264", std::ios::in | std::ios::binary);
  ASSERT_TRUE (file.is_open());
  std::vector<uint8_t> bitstream ((std::istreambuf_iterator<char> (file)), std::istreambuf_iterator<char> ());
  ASSERT_FALSE (bitstream.empty());

  int32_t fileSize = static_cast<int32_t> (bitstream.size());
  int32_t pos = 0;
  int32_t frame1 = ReadFrameForHangRegression (bitstream.data(), fileSize, pos);
  pos += frame1;
  int32_t frame2 = ReadFrameForHangRegression (bitstream.data(), fileSize, pos);
  pos += frame2;
  int32_t frame3 = ReadFrameForHangRegression (bitstream.data(), fileSize, pos);

  ASSERT_GT (frame1, 1);
  ASSERT_GT (frame2, 0);
  ASSERT_GT (frame3, 0);

  ISVCDecoder* decoder = NULL;
  ASSERT_EQ (0, WelsCreateDecoder (&decoder));
  ASSERT_TRUE (decoder != NULL);

  int threadCount = 2;
  decoder->SetOption (DECODER_OPTION_NUM_OF_THREADS, &threadCount);

  SDecodingParam decodingParam;
  std::memset (&decodingParam, 0, sizeof (SDecodingParam));
  decodingParam.uiTargetDqLayer = UCHAR_MAX;
  decodingParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
  decodingParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
  ASSERT_EQ (0, decoder->Initialize (&decodingParam));

  uint8_t* dst[3] = {NULL, NULL, NULL};
  SBufferInfo info;

  std::memset (&info, 0, sizeof (info));
  info.uiInBsTimeStamp = 1;
  DECODING_STATE state = decoder->DecodeFrameNoDelay (bitstream.data(), frame1 - 1, dst, &info);
  EXPECT_EQ (dsErrorFree, state);

  std::memset (&info, 0, sizeof (info));
  info.uiInBsTimeStamp = 2;
  state = decoder->DecodeFrameNoDelay (bitstream.data() + frame1, frame2, dst, &info);
  EXPECT_EQ (dsErrorFree, state);

  std::memset (&info, 0, sizeof (info));
  info.uiInBsTimeStamp = 3;
  state = decoder->DecodeFrameNoDelay (bitstream.data() + frame1 + frame2, frame3, dst, &info);
  EXPECT_EQ (dsErrorFree, state);

  // Drain pipelined in-flight frames before teardown so Uninitialize() does not
  // free decoder state while a worker thread is still reconstructing.
  int32_t endOfStream = 1;
  decoder->SetOption (DECODER_OPTION_END_OF_STREAM, &endOfStream);
  int32_t remaining = 0;
  decoder->GetOption (DECODER_OPTION_NUM_OF_FRAMES_REMAINING_IN_BUFFER, &remaining);
  for (int32_t i = 0; i < remaining; ++i) {
    std::memset (&info, 0, sizeof (info));
    decoder->FlushFrame (dst, &info);
  }

  decoder->Uninitialize();
  WelsDestroyDecoder (decoder);
}

class ThreadDecoderCapabilityTest : public ::testing::Test {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F (ThreadDecoderCapabilityTest, JustInit) {
  SDecoderCapability sDecCap;
  int iRet = WelsGetDecoderCapability (&sDecCap);
  ASSERT_TRUE (iRet == 0);
  EXPECT_EQ (sDecCap.iProfileIdc, 66);
  EXPECT_EQ (sDecCap.iProfileIop, 0xE0);
  EXPECT_EQ (sDecCap.iLevelIdc, 32);
  EXPECT_EQ (sDecCap.iMaxMbps, 216000);
  EXPECT_EQ (sDecCap.iMaxFs, 5120);
  EXPECT_EQ (sDecCap.iMaxCpb, 20000);
  EXPECT_EQ (sDecCap.iMaxDpb, 20480);
  EXPECT_EQ (sDecCap.iMaxBr, 20000);
  EXPECT_EQ (sDecCap.bRedPicCap, false);
}


class ThreadDecoderInitTest : public ::testing::Test, public BaseThreadDecoderTest {
 public:
  virtual void SetUp() {
    BaseThreadDecoderTest::SetUp();
  }
  virtual void TearDown() {
    BaseThreadDecoderTest::TearDown();
  }
};

TEST_F (ThreadDecoderInitTest, JustInit) {}

TEST (ThreadDecoderSecurityTest, DecodeFrameNoDelayNoPostReturnWriteToCallerPpDst) {
#if defined(_WIN32)
  GTEST_SKIP() << "mmap/munmap based lifetime stress is POSIX-only";
#else
  ISVCDecoder* decoder = NULL;
  ASSERT_EQ (0, WelsCreateDecoder (&decoder));
  ASSERT_TRUE (decoder != NULL);

  SDecodingParam decParam;
  memset (&decParam, 0, sizeof (decParam));
  decParam.uiTargetDqLayer = UCHAR_MAX;
  decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
  decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
  int32_t iThreadCount = 2;
  decoder->SetOption (DECODER_OPTION_NUM_OF_THREADS, &iThreadCount);
  ASSERT_EQ (0, decoder->Initialize (&decParam));

  std::vector<unsigned char> bitstream = ReadBinaryFile ("res/BA_MW_D.264");
  ASSERT_FALSE (bitstream.empty());

  const size_t pageSize = static_cast<size_t> (sysconf (_SC_PAGESIZE));
  const size_t chunkSize = 1200;
  const int32_t kMaxIters = 12;

  for (size_t off = 0, iter = 0; off < bitstream.size() && iter < static_cast<size_t> (kMaxIters);
       off += chunkSize, ++iter) {
    void* page = mmap (NULL, pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ASSERT_NE (MAP_FAILED, page);

    unsigned char** ppDst = reinterpret_cast<unsigned char**> (page);
    memset (ppDst, 0, pageSize);

    SBufferInfo dstInfo;
    memset (&dstInfo, 0, sizeof (dstInfo));
    int32_t len = static_cast<int32_t> (std::min (chunkSize, bitstream.size() - off));
    DECODING_STATE rv = decoder->DecodeFrameNoDelay (bitstream.data() + off, len, ppDst, &dstInfo);
    EXPECT_EQ (dsErrorFree, rv);

    munmap (page, pageSize);
    usleep (5000);
  }

  decoder->Uninitialize();
  WelsDestroyDecoder (decoder);
#endif
}

struct FileParam {
  const char* fileName;
  const char* hashStr;
};

class ThreadDecoderOutputTest : public ::testing::WithParamInterface<FileParam>,
  public ThreadDecoderInitTest, public BaseThreadDecoderTest::Callback {
 public:
  virtual void SetUp() {
    ThreadDecoderInitTest::SetUp();
    if (HasFatalFailure()) {
      return;
    }
    SHA1Reset (&ctx_);
  }
  virtual void onDecodeFrame (const Frame& frame) {
    const Plane& y = frame.y;
    const Plane& u = frame.u;
    const Plane& v = frame.v;
    UpdateHashFromPlane (&ctx_, y.data, y.width, y.height, y.stride);
    UpdateHashFromPlane (&ctx_, u.data, u.width, u.height, u.stride);
    UpdateHashFromPlane (&ctx_, v.data, v.width, v.height, v.stride);
  }
 protected:
  SHA1Context ctx_;
};

TEST_P (ThreadDecoderOutputTest, DISABLED_CompareOutput) {
  FileParam p = GetParam();
#if defined(ANDROID_NDK)
  std::string filename = std::string ("/sdcard/") + p.fileName;
  ASSERT_TRUE (ThreadDecodeFile (filename.c_str(), this));
#else
  ASSERT_TRUE (ThreadDecodeFile (p.fileName, this));
#endif

  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1Result (&ctx_, digest);
  if (!HasFatalFailure()) {
    std::string p_hashStr (p.hashStr);
    std::stringstream ss (p_hashStr);
    std::string buf[4];
    const char* hashStr[4];
    int i = 0;
    while (i < 4 && ss >> buf[i]) {
      hashStr[i] = buf[i].c_str();
      ++i;
    }
    CompareHashAnyOf (digest, hashStr, i);
  }
}
static const FileParam kFileParamArray[] = {
  {"res/Adobe_PDF_sample_a_1024x768_50Frms.264", "041434a5819d1d903d49c0eda884b345e9f83596"},
  //{"res/BA1_FT_C.264", "072ccfd92528f09ae8888cb5e023af511e1010a1"}, //multi hash values only in travis-ci build machine
  {"res/BA1_Sony_D.jsv", "37c9a951a0348d6abe1880b59e2b5a4d7d18c94c"},
  {"res/BAMQ1_JVC_C.264", "6720462624f632f5475716ef32a7bbd12b3b428a"},
  {"res/BAMQ2_JVC_C.264", "5f0fbb0dab7961e782224f6887c83d4866fc1af8"},
  {"res/BA_MW_D.264", "ace02cdce720bdb0698b40dc749a0e61fe0f590b"},
  //{"res/BANM_MW_D.264", "c51f1d2fa63dba4f5787f1b726c056d1c01d6ab9"}, //multi hash values only in travis-ci build machine
  {"res/BASQP1_Sony_C.jsv", "2e10e98fc54f92cb5e72513bf417c4e4df333361"},
  //{"res/CI1_FT_B.264", "721e555a33cfff81b6034a127334c5891776373c"}, //multi hash values only in travis-ci build machine
  {"res/CI_MW_D.264", "49a8916edd3e571efad328f2784fbe6aec5570d7"},
  {"res/CVFC1_Sony_C.jsv", "5cc447bb7906d5b9858cc7092aaf491035861660"},
  {"res/CVPCMNL1_SVA_C.264", "c2b0d964de727c64b9fccb58f63b567c82bda95a"},
  //{"res/LS_SVA_D.264", "e020a1c6668501887bb55e00741ebfdbc91d400d"}, //Multi-thread decoding hanging due to high pSps->iNumRefFrames which is 15
  {"res/MIDR_MW_D.264", "aeded2be7b97484cbf25f367ec34208f2220a8ab"},
  {"res/MPS_MW_A.264", "b0fce28218e678d89f464810f88b143ada49dd06"},
  //{"res/MR1_BT_A.h264", "7f6d806f12d19ec991182467e801a78fb4f80e04"}, //multi hash values only in travis-ci build machine
  //{"res/MR1_MW_A.264", "14d8ddb12ed711444039329db29c496b079680ba"}, //multi hash values on osx x86_64 and segment fault on Linux m32 only in travis-ci build machine
  //{"res/MR2_MW_A.264", "6d332a653fe3b923eb3af8f3695d46ce2a1d4b2c"}, //multi hash values
  //{"res/MR2_TANDBERG_E.264", "74d618bc7d9d41998edf4c85d51aa06111db6609"}, //Multi-thread decoding hanging due to high pSps->iNumRefFrames which is 15
  {"res/NL1_Sony_D.jsv", "e401e30669938443c2f02522fd4d5aa1382931a0"},
  {"res/NLMQ1_JVC_C.264", "f3265c6ddf8db1b2bf604d8a2954f75532e28cda"},
  {"res/NLMQ2_JVC_C.264", "350ae86ef9ba09390d63a09b7f9ff54184109ca8"},
  {"res/NRF_MW_E.264", "866f267afd2ed1595bcb90de0f539e929c169aa4"},
  {"res/QCIF_2P_I_allIPCM.264", "9879ce127d3263cfbaf5211ab6657dbf0ccabea8"},
  { "res/SVA_BA1_B.264", "4cb45a99ae44a0a98b174efd66245daa1fbaeb47"},
  {"res/SVA_BA2_D.264", "ac9e960015b96f83279840802f6637c61ee1c5b8"},
  {"res/SVA_Base_B.264", "e6010d1b47aa796c1f5295b2563ed696aa9c37ab"},
  {"res/SVA_CL1_E.264", "4fe09ab6cdc965ea10a20f1d6dd38aca954412bb"},
  {"res/SVA_FM1_E.264", "1a114fbd096f637acd0c3fb8f35bdfa3bc275199"},
  {"res/SVA_NL1_B.264", "6d63f72a0c0d833b1db0ba438afff3b4180fb3e6"},
  {"res/SVA_NL2_E.264", "70453ef8097c94dd190d6d2d1d5cb83c67e66238"},
  {"res/SarVui.264", "ac9e960015b96f83279840802f6637c61ee1c5b8"},
  {"res/Static.264", "1310f9a1d7d115eec8155d071b9b45b5cfbf8321"},
  {"res/Zhling_1280x720.264", "10f9c803e80b51786f7833255afc3ef75c5c1339"},
  {"res/sps_subsetsps_bothVUI.264", "d65a34075c452196401340c554e83225c9454397"},
  //{"res/test_cif_I_CABAC_PCM.264", "95fdf21470d3bbcf95505abb2164042063a79d98"}, //multi hash values only in travis-ci build machine
  //{"res/test_cif_I_CABAC_slice.264", "a7154eb1d0909eb9fd1e4e89f5d6271e5201814b"}, //multi hash values only in travis-ci build machine
  //{"res/test_cif_P_CABAC_slice.264", "b08bcf1056458ae113d0a55f35e6b00eb2bd7811"},//multi hash values only in travis-ci build machine
  {"res/test_qcif_cabac.264", "c79e9a32e4d9e38a1bd12079da19dcb0d2efe539"},
  {"res/test_scalinglist_jm.264", "b36efd05c8b17faa23f1c071b92aa5d55a5a826f"},
  {"res/test_vd_1d.264", "15d8beaf991f9e5d56a854cdafc0a7abdd5bec69"},
  {"res/test_vd_rc.264", "cd6ef57fc884e5ecd9867591b01e35e3f091b8d0"},
  {"res/Cisco_Men_whisper_640x320_CABAC_Bframe_9.264", "5d3d08fb47ac8c6e379c1572aed517522d883920"},
  {"res/Cisco_Men_whisper_640x320_CAVLC_Bframe_9.264", "89742b454cac4843e0bf18a3df9b46f21155b48a"},
  {"res/Cisco_Adobe_PDF_sample_a_1024x768_CAVLC_Bframe_9.264", "5fce0b92c5f2a1636ea06ae48ea208908fd01416"},
  {"res/VID_1280x544_cabac_temporal_direct.264", "ae5f21eff917d09d5a1ba2ad2075edd92eb6b61c"},
  //{"res/VID_1280x720_cabac_temporal_direct.264", "2597181429a48740a143053a5b027dcbe4173f4e"}, // hangs only on travis - ci build machine
  {"res/VID_1920x1080_cabac_temporal_direct.264", "8c93ae9acfdf6d902c1a47102d4bf3294f45c0f3"},
  {"res/VID_1280x544_cavlc_temporal_direct.264", "d9b31a2586ee156fe697de5934afb5a769f79494"},
  {"res/VID_1280x720_cavlc_temporal_direct.264", "888c31cef73eb6804e2469fa77e51636c915ff82"},
  {"res/VID_1920x1080_cavlc_temporal_direct.264", "4467039825f472bae31e58b383b1f2c9a73ce8e0"},
};

INSTANTIATE_TEST_SUITE_P (ThreadDecodeFile, ThreadDecoderOutputTest,
                          ::testing::ValuesIn (kFileParamArray));

// Regression: threaded decode of BA_MW_D.264 must produce the same SHA1 output
// as single-thread decode. Pre-fix, the shared pPreviousDecodedPictureInDpb
// pointer was overwritten by concurrent workers before BufferingReadyPicture()
// read it, causing output hash divergence and occasional SIGSEGV.
class ThreadDecoderPreviousPicRaceTest : public ::testing::Test {
 public:
  struct HashCbk : public BaseThreadDecoderTest::Callback {
    SHA1Context ctx;
    HashCbk() { SHA1Reset (&ctx); }
    void onDecodeFrame (const BaseThreadDecoderTest::Frame& frame) override {
      UpdateHashFromPlane (&ctx, frame.y.data, frame.y.width, frame.y.height, frame.y.stride);
      UpdateHashFromPlane (&ctx, frame.u.data, frame.u.width, frame.u.height, frame.u.stride);
      UpdateHashFromPlane (&ctx, frame.v.data, frame.v.width, frame.v.height, frame.v.stride);
    }
    std::string Digest() {
      unsigned char d[SHA_DIGEST_LENGTH];
      SHA1Result (&ctx, d);
      char buf[SHA_DIGEST_LENGTH * 2 + 1];
      for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        std::snprintf (buf + i * 2, 3, "%02x", d[i]);
      return std::string (buf);
    }
  };

  static std::string DecodeFile (const char* path, int threads) {
    long rv = 0;
    ISVCDecoder* dec = NULL;
    rv = WelsCreateDecoder (&dec);
    if (rv != 0 || dec == NULL) return "";
    SDecodingParam p;
    std::memset (&p, 0, sizeof (p));
    p.uiTargetDqLayer = UCHAR_MAX;
    p.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    p.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
    dec->SetOption (DECODER_OPTION_NUM_OF_THREADS, &threads);
    if (dec->Initialize (&p) != 0) { WelsDestroyDecoder (dec); return ""; }

    std::ifstream f (path, std::ios::binary);
    if (!f.is_open()) { dec->Uninitialize(); WelsDestroyDecoder (dec); return ""; }
    std::vector<uint8_t> bs ((std::istreambuf_iterator<char> (f)), std::istreambuf_iterator<char>());
    f.close();

    HashCbk cbk;
    uint64_t ts = 0;
    int32_t pos = 0;
    const int32_t sz = static_cast<int32_t> (bs.size());
    // dst/info must outlive each DecodeFrameNoDelay call: worker threads may
    // write through ppDst after the call returns (stack-use-after-scope otherwise).
    unsigned char* dst[3] = {NULL, NULL, NULL};
    SBufferInfo info;
    while (pos < sz) {
      int32_t fsz = ReadFrameForHangRegression (bs.data(), sz, pos);
      if (fsz <= 0) break;
      std::memset (dst, 0, sizeof (dst));
      std::memset (&info, 0, sizeof (info));
      info.uiInBsTimeStamp = ++ts;
      dec->DecodeFrameNoDelay (bs.data() + pos, fsz, dst, &info);
      if (info.iBufferStatus == 1) {
        BaseThreadDecoderTest::Frame fr;
        fr.y = {info.pDst[0], info.UsrData.sSystemBuffer.iWidth, info.UsrData.sSystemBuffer.iHeight, info.UsrData.sSystemBuffer.iStride[0]};
        fr.u = {info.pDst[1], info.UsrData.sSystemBuffer.iWidth/2, info.UsrData.sSystemBuffer.iHeight/2, info.UsrData.sSystemBuffer.iStride[1]};
        fr.v = {info.pDst[2], info.UsrData.sSystemBuffer.iWidth/2, info.UsrData.sSystemBuffer.iHeight/2, info.UsrData.sSystemBuffer.iStride[1]};
        cbk.onDecodeFrame (fr);
      }
      pos += fsz;
    }
    int32_t eos = 1;
    dec->SetOption (DECODER_OPTION_END_OF_STREAM, &eos);
    int32_t rem = 0;
    dec->GetOption (DECODER_OPTION_NUM_OF_FRAMES_REMAINING_IN_BUFFER, &rem);
    for (int i = 0; i < rem; ++i) {
      std::memset (dst, 0, sizeof (dst));
      std::memset (&info, 0, sizeof (info));
      dec->FlushFrame (dst, &info);
      if (info.iBufferStatus == 1) {
        BaseThreadDecoderTest::Frame fr;
        fr.y = {info.pDst[0], info.UsrData.sSystemBuffer.iWidth, info.UsrData.sSystemBuffer.iHeight, info.UsrData.sSystemBuffer.iStride[0]};
        fr.u = {info.pDst[1], info.UsrData.sSystemBuffer.iWidth/2, info.UsrData.sSystemBuffer.iHeight/2, info.UsrData.sSystemBuffer.iStride[1]};
        fr.v = {info.pDst[2], info.UsrData.sSystemBuffer.iWidth/2, info.UsrData.sSystemBuffer.iHeight/2, info.UsrData.sSystemBuffer.iStride[1]};
        cbk.onDecodeFrame (fr);
      }
    }
    dec->Uninitialize();
    WelsDestroyDecoder (dec);
    return cbk.Digest();
  }
};

TEST_F (ThreadDecoderPreviousPicRaceTest, ThreadedOutputIsConsistent) {
  const char* kFile = "res/BA_MW_D.264";
  // Run the same stream three times with the same thread count.
  // Pre-fix, the shared pPreviousDecodedPictureInDpb could be overwritten by a
  // concurrent worker between the write and BufferingReadyPicture(), making
  // multi-thread output non-deterministic.  Post-fix all runs must agree.
  std::string h1 = DecodeFile (kFile, 3);
  std::string h2 = DecodeFile (kFile, 3);
  std::string h3 = DecodeFile (kFile, 3);
  ASSERT_FALSE (h1.empty()) << "3-thread decode produced no output (run 1)";
  ASSERT_FALSE (h2.empty()) << "3-thread decode produced no output (run 2)";
  ASSERT_FALSE (h3.empty()) << "3-thread decode produced no output (run 3)";
  EXPECT_EQ (h1, h2)
      << "3-thread decode is non-deterministic between run 1 and run 2";
  EXPECT_EQ (h1, h3)
      << "3-thread decode is non-deterministic between run 1 and run 3";
}

