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
    std::string p_hashStr(p.hashStr);
    std::stringstream ss(p_hashStr);
    std::string buf[4];
    const char * hashStr[4];
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
  {"res/BA1_FT_C.264", "48d65bf8c731f29efc72f6222dd85b8ef7f636a7 463284ff3f5a0b1a0c829d47e9b73dcfe617c926"},
  {"res/BA1_Sony_D.jsv", "37c9a951a0348d6abe1880b59e2b5a4d7d18c94c"},
  {"res/BAMQ1_JVC_C.264", "6720462624f632f5475716ef32a7bbd12b3b428a 477b1e45e30661a138ff0b43c1ed3e00ded13d9c"},
  {"res/BAMQ2_JVC_C.264", "5f0fbb0dab7961e782224f6887c83d4866fc1af8 e3dfdc770fa5fee8b92f896a92214886c109a688"},
  {"res/BA_MW_D.264", "ace02cdce720bdb0698b40dc749a0e61fe0f590b"},
  {"res/BANM_MW_D.264", "c51f1d2fa63dba4f5787f1b726c056d1c01d6ab9"},
  {"res/BASQP1_Sony_C.jsv", "68e604b77e3f57f8ef1c2e450fcef03f5d2aee90 d5e1f122e8bf8d58bc6775d69b837db0d9ea3454"},
  {"res/CI1_FT_B.264", "96042b70e212c8b253a786e865131ac89c133ca1 53d2a2f276a81b6ef869791373f7bc3928cc9ca3"},
  {"res/CI_MW_D.264", "49a8916edd3e571efad328f2784fbe6aec5570d7"},
  {"res/CVFC1_Sony_C.jsv", "109dfc8357a98b16aa74469a5506e362e563aa85 f29da8955c6f01d972dfe10b22c4f879aab05412"},
  {"res/CVPCMNL1_SVA_C.264", "c2b0d964de727c64b9fccb58f63b567c82bda95a"},
  //{"res/LS_SVA_D.264", "72118f4d1674cf14e58bed7e67cb3aeed3df62b9"}, //DPB buffer is too small
  {"res/MIDR_MW_D.264", "aeded2be7b97484cbf25f367ec34208f2220a8ab"},
  {"res/MPS_MW_A.264", "b0fce28218e678d89f464810f88b143ada49dd06"},
  //{"res/MR1_BT_A.h264", "eebd1d7cdb67df5b8688b1ce18f6acae129b32e6 d20e96f9ecc2e24c13eb25b1c786da53eb716327"}, three hash values temp disabled
  {"res/MR1_MW_A.264", "14d8ddb12ed711444039329db29c496b079680ba"},
  {"res/MR2_MW_A.264", "6d332a653fe3b923eb3af8f3695d46ce2a1d4b2c 14a38e41f4dbf924b8eff6e96aad77394c8aabcd"},
  //{"res/MR2_TANDBERG_E.264", "74d618bc7d9d41998edf4c85d51aa06111db6609"}, //DPB buffer is too small
  {"res/NL1_Sony_D.jsv", "e401e30669938443c2f02522fd4d5aa1382931a0"},
  {"res/NLMQ1_JVC_C.264", "f3265c6ddf8db1b2bf604d8a2954f75532e28cda a86ec7a843e93f44aaee2619a7932c6c5c8d233f"},
  {"res/NLMQ2_JVC_C.264", "350ae86ef9ba09390d63a09b7f9ff54184109ca8 95e6e4426b75f38a6744f3d04cfc62a2c0489354"},
  {"res/NRF_MW_E.264", "866f267afd2ed1595bcb90de0f539e929c169aa4 db2d135cef07db8247ef858daf870d07955b912a"},
  {"res/QCIF_2P_I_allIPCM.264", "9879ce127d3263cfbaf5211ab6657dbf0ccabea8"},
  { "res/SVA_BA1_B.264", "4cb45a99ae44a0a98b174efd66245daa1fbaeb47 e9127875b268f9e7da4c495799b9972b8e72cf7b"},
  {"res/SVA_BA2_D.264", "ac9e960015b96f83279840802f6637c61ee1c5b8 719fe839fa68b915b614fbbbae15edf492cc2133"},
  {"res/SVA_Base_B.264", "6015682a8f957bd499242150315cfd2fdf23e728 cd8c8ad018fe2fd7b3893f8175265b0eb7bcd342"},
  {"res/SVA_CL1_E.264", "4fe09ab6cdc965ea10a20f1d6dd38aca954412bb"},
  {"res/SVA_FM1_E.264", "164a9a2db58e7200ae60db8079d7201ac431887a fb33868ae38b3642edd309ad3005de4214fcc63f"},
  {"res/SVA_NL1_B.264", "6d63f72a0c0d833b1db0ba438afff3b4180fb3e6"},
  {"res/SVA_NL2_E.264", "70453ef8097c94dd190d6d2d1d5cb83c67e66238"},
  //{"res/SarVui.264", "1843d19d8e13588ef5de2d647804ae141e55cf72 719fe839fa68b915b614fbbbae15edf492cc2133"}, //same as "res/SVA_BA1_B.264"
  {"res/Static.264", "d865faee7df56a8f532b7baeacb814483b8be148 52af285a888b8c9e04dc9f38fd61105e805ada3a"},
  {"res/Zhling_1280x720.264", "10f9c803e80b51786f7833255afc3ef75c5c1339"},
  {"res/sps_subsetsps_bothVUI.264", "d65a34075c452196401340c554e83225c9454397"},
  //{"res/test_cif_I_CABAC_PCM.264", "dfe2f87ac76bdb58e227267907a2eeccf04715ad 02ac993be06b5d88118beb96ee5dfd0995b7cb00 95fdf21470d3bbcf95505abb2164042063a79d98 c2b42f489ca9c2ebc43c0ab2238551a0c958a692"},
  {"res/test_cif_I_CABAC_slice.264", "4260cc7a211895341092b0361bcfc3f13721ab44 106da52c2c6d30255b6ac0aa0b4a881a06ebb762"},
  //{"res/test_cif_P_CABAC_slice.264", "ac2d1e9ca0e097ab44a4b592a93e06e5c0c3d761 276a5ccef4bbe20ad9c769824aea5553acc7b54a 8ba773ccf5f682a4a90b0d070aa4198a5cfa0220 b09e066f797235fed8f59c408b5914d143f71c9e"},
  {"res/test_qcif_cabac.264", "c79e9a32e4d9e38a1bd12079da19dcb0d2efe539"},
  {"res/test_scalinglist_jm.264", "b36efd05c8b17faa23f1c071b92aa5d55a5a826f"},
  {"res/test_vd_1d.264", "15d8beaf991f9e5d56a854cdafc0a7abdd5bec69"},
  {"res/test_vd_rc.264", "cd6ef57fc884e5ecd9867591b01e35e3f091b8d0"},
  {"res/Cisco_Men_whisper_640x320_CABAC_Bframe_9.264", "7df59855104a319b44a7611dd6c37b1670bf74c9"},
  {"res/Cisco_Men_whisper_640x320_CAVLC_Bframe_9.264", "0d77e3c53f46d8962cd95b975e76d0f32613da0f"},
  {"res/Cisco_Adobe_PDF_sample_a_1024x768_CAVLC_Bframe_9.264", "6cac61a6b58bba59b8e9944b18aba2df20efeca2"},
  {"res/VID_1280x544_cabac_temporal_direct.264", "e8ee8dd56ec5df1338f3c21ed8690d074c7ec03f"},
  {"res/VID_1280x720_cabac_temporal_direct.264", "1efa6aec8c5f953c53d713c31999420fdbd10b22"},
  {"res/VID_1920x1080_cabac_temporal_direct.264", "90b3f1cf0c85b490108a2db40d2b2151ee346dfb aafd2606e8fe8be2a956deed48218c9f5176b3d0"},
  {"res/VID_1280x544_cavlc_temporal_direct.264", "fe779025f3b42d6fc3590476cb3594540950d716"},
  {"res/VID_1280x720_cavlc_temporal_direct.264", "1c5afab7cfeb082b087821d4220d57238c1c161f"},
  {"res/VID_1920x1080_cavlc_temporal_direct.264", "5c47d30fed9d2988c653b2c3bc83f6d19dfa5ab1 eecd84b68f416270eb21c6c90a4cef8603d37e25"},
};

INSTANTIATE_TEST_CASE_P (ThreadDecodeFile, ThreadDecoderOutputTest,
                         ::testing::ValuesIn (kFileParamArray));
