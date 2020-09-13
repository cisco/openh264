#include <gtest/gtest.h>
#include "utils/HashFunctions.h"
#include "BaseThreadDecoderTest.h"
#include <string>

static void UpdateHashFromPlane (SHA1Context* ctx, const uint8_t* plane,
                                 int width, int height, int stride) {
  for (int i = 0; i < height; i++) {
    SHA1Input (ctx, plane, width);
    plane += stride;
  }
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

TEST_P (ThreadDecoderOutputTest, CompareOutput) {
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
  {"res/MR1_MW_A.264", "14d8ddb12ed711444039329db29c496b079680ba"},
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

INSTANTIATE_TEST_CASE_P (ThreadDecodeFile, ThreadDecoderOutputTest,
                         ::testing::ValuesIn (kFileParamArray));
