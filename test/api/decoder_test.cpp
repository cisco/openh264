#include <gtest/gtest.h>
#include "utils/HashFunctions.h"
#include "BaseDecoderTest.h"
#include <string>

static void UpdateHashFromPlane (SHA1Context* ctx, const uint8_t* plane,
                                 int width, int height, int stride) {
  for (int i = 0; i < height; i++) {
    SHA1Input (ctx, plane, width);
    plane += stride;
  }
}

class DecoderCapabilityTest : public ::testing::Test {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST_F (DecoderCapabilityTest, JustInit) {
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


class DecoderInitTest : public ::testing::Test, public BaseDecoderTest {
 public:
  virtual void SetUp() {
    BaseDecoderTest::SetUp();
  }
  virtual void TearDown() {
    BaseDecoderTest::TearDown();
  }
};

TEST_F (DecoderInitTest, JustInit) {}

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

TEST_P (DecoderOutputTest, CompareOutput) {
  FileParam p = GetParam();
#if defined(ANDROID_NDK)
  std::string filename = std::string ("/sdcard/") + p.fileName;
  DecodeFile (filename.c_str(), this);
#else
  DecodeFile (p.fileName, this);
#endif

  unsigned char digest[SHA_DIGEST_LENGTH];
  SHA1Result (&ctx_, digest);
  if (!HasFatalFailure()) {
    CompareHash (digest, p.hashStr);
  }
}
static const FileParam kFileParamArray[] = {
  {"res/Adobe_PDF_sample_a_1024x768_50Frms.264", "9aa9a4d9598eb3e1093311826844f37c43e4c521"},
  {"res/BA1_FT_C.264", "418d152fb85709b6f172799dcb239038df437cfa"},
  {"res/BA1_Sony_D.jsv", "d94b5ceed5686a03ea682b53d415dee999d27eb6"},
  {"res/BAMQ1_JVC_C.264", "613cf662c23e5d9e1d7da7fe880a3c427411d171"},
  {"res/BAMQ2_JVC_C.264", "11bcf3713f520e606a8326d37e00e5fd6c9fd4a0"},
  {"res/BA_MW_D.264", "afd7a9765961ca241bb4bdf344b31397bec7465a"},
  {"res/BANM_MW_D.264", "92d924a857a1a7d7d9b224eaa3887830f15dee7f"},
  {"res/BASQP1_Sony_C.jsv", "3986c8c9d2876d2f0748b925101b152c6ec8b811"},
  {"res/Cisco_Men_whisper_640x320_CABAC_Bframe_9.264", "88b8864a69cee7656202bc54d2ffa8b7b6f1f6c5"},
  {"res/CI1_FT_B.264", "cbfec15e17a504678b19a1191992131c92a1ac26"},
  {"res/CI_MW_D.264", "289f29a103c8d95adf2909c646466904be8b06d7"},
  {"res/CVFC1_Sony_C.jsv", "4641abd7419a5580b97f16e83fd1d566339229d0"},
  {"res/CVPCMNL1_SVA_C.264", "c2b0d964de727c64b9fccb58f63b567c82bda95a"},
  {"res/LS_SVA_D.264", "72118f4d1674cf14e58bed7e67cb3aeed3df62b9"},
  {"res/MIDR_MW_D.264", "9467030f4786f75644bf06a7fc809c36d1959827"},
  {"res/MPS_MW_A.264", "67f1cfbef0e8025ed60dedccf8d9558d0636be5f"},
  {"res/MR1_BT_A.h264", "6e585f8359667a16b03e5f49a06f5ceae8d991e0"},
  {"res/MR1_MW_A.264", "d9e2bf34e9314dcc171ddaea2c5015d0421479f2"},
  {"res/MR2_MW_A.264", "628b1d4eff04c2d277f7144e23484957dad63cbe"},
  {"res/MR2_TANDBERG_E.264", "74d618bc7d9d41998edf4c85d51aa06111db6609"},
  {"res/NL1_Sony_D.jsv", "e401e30669938443c2f02522fd4d5aa1382931a0"},
  {"res/NLMQ1_JVC_C.264", "f3265c6ddf8db1b2bf604d8a2954f75532e28cda"},
  {"res/NLMQ2_JVC_C.264", "350ae86ef9ba09390d63a09b7f9ff54184109ca8"},
  {"res/NRF_MW_E.264", "20732198c04cd2591350a361e4510892f6eed3f0"},
  {"res/QCIF_2P_I_allIPCM.264", "8724c0866ebdba7ebb7209a0c0c3ae3ae38a0240"},
  {"res/SVA_BA1_B.264", "c4543b24823b16c424c673616c36c7f537089b2d"},
  {"res/SVA_BA2_D.264", "98ff2d67860462d8d8bcc9352097c06cc401d97e"},
  {"res/SVA_Base_B.264", "91f514d81cd33de9f6fbf5dbefdb189cc2e7ecf4"},
  {"res/SVA_CL1_E.264", "4fe09ab6cdc965ea10a20f1d6dd38aca954412bb"},
  {"res/SVA_FM1_E.264", "fad08c4ff7cf2307b6579853d0f4652fc26645d3"},
  {"res/SVA_NL1_B.264", "6d63f72a0c0d833b1db0ba438afff3b4180fb3e6"},
  {"res/SVA_NL2_E.264", "70453ef8097c94dd190d6d2d1d5cb83c67e66238"},
  {"res/SarVui.264", "98ff2d67860462d8d8bcc9352097c06cc401d97e"},
  {"res/Static.264", "91dd4a7a796805b2cd015cae8fd630d96c663f42"},
  {"res/Zhling_1280x720.264", "ad99f5eaa2d73ae3840e7da67313de8cfc866ce6"},
  {"res/sps_subsetsps_bothVUI.264", "d3a47032eb5dcc1963343a68e9bea12435bf1e4c"},
  {"res/test_cif_I_CABAC_PCM.264", "95fdf21470d3bbcf95505abb2164042063a79d98"},
  {"res/test_cif_I_CABAC_slice.264", "19121bc67f2b13fb8f030504fc0827e1ac6d0fdb"},
  {"res/test_cif_P_CABAC_slice.264", "521bbd0ba2422369b724c7054545cf107a56f959"},
  {"res/test_qcif_cabac.264", "587d1d05943f3cd416bf69469975fdee05361e69"},
  {"res/test_scalinglist_jm.264", "f690a3af2896a53360215fb5d35016bfd41499b3"},
  {"res/test_vd_1d.264", "5827d2338b79ff82cd091c707823e466197281d3"},
  {"res/test_vd_rc.264", "eea02e97bfec89d0418593a8abaaf55d02eaa1ca"},
  {"res/Cisco_Men_whisper_640x320_CABAC_Bframe_9.264", "88b8864a69cee7656202bc54d2ffa8b7b6f1f6c5"},
  {"res/Cisco_Men_whisper_640x320_CAVLC_Bframe_9.264", "270a500d2f91c9e2c8ffabc03f62e0dc0b3a24ed"},
  {"res/Cisco_Adobe_PDF_sample_a_1024x768_CAVLC_Bframe_9.264", "d3b2b986178ce3eafa806cd984543d0da830f408"},
};

INSTANTIATE_TEST_CASE_P (DecodeFile, DecoderOutputTest,
                         ::testing::ValuesIn (kFileParamArray));
