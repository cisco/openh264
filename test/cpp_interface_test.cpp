#include <gtest/gtest.h>
#include "codec_api.h"

static void CheckFunctionOrder(int expect, int actual, const char* name) {
  EXPECT_EQ(expect, actual) << "Wrong function order: " << name;
}

typedef void(*CheckFunc)(int, int, const char*);
extern "C" void CheckEncoderInterface(ISVCEncoder* p, CheckFunc);
extern "C" void CheckDecoderInterface(ISVCDecoder* p, CheckFunc);

// Store the 'this' pointer to verify 'this' is received as expected from C code.
static void* gThis;

/**
 * Return a unique number for each virtual function so that we are able to
 * check if the order of functions in the virtual table is as expected.
 */
struct SVCEncoderImpl : public ISVCEncoder {
  virtual ~SVCEncoderImpl() {}
  virtual int EXTAPI Initialize(SVCEncodingParam* pParam,
      const INIT_TYPE kiInitType) {
    EXPECT_TRUE(gThis == this);
    return 1;
  }
  virtual int EXTAPI Initialize2(void* pParam, const INIT_TYPE kiInitType) {
    EXPECT_TRUE(gThis == this);
    return 2;
  }
  virtual int EXTAPI Uninitialize() {
    EXPECT_TRUE(gThis == this);
    return 3;
  }
  virtual int EXTAPI EncodeFrame(const unsigned char* kpSrc,
      SFrameBSInfo* pBsInfo) {
    EXPECT_TRUE(gThis == this);
    return 4;
  }
  virtual int EXTAPI EncodeFrame2(const SSourcePicture** kppSrcPicList,
      int nSrcPicNum, SFrameBSInfo* pBsInfo) {
    EXPECT_TRUE(gThis == this);
    return 5;
  }
  virtual int EXTAPI EncodeParameterSets(SFrameBSInfo* pBsInfo) {
    EXPECT_TRUE(gThis == this);
    return 6;
  }
  virtual int EXTAPI PauseFrame(const unsigned char* kpSrc,
      SFrameBSInfo* pBsInfo) {
    EXPECT_TRUE(gThis == this);
    return 7;
  }
  virtual int EXTAPI ForceIntraFrame(bool bIDR) {
    EXPECT_TRUE(gThis == this);
    return 8;
  }
  virtual int EXTAPI SetOption(ENCODER_OPTION eOptionId, void* pOption) {
    EXPECT_TRUE(gThis == this);
    return 9;
  }
  virtual int EXTAPI GetOption(ENCODER_OPTION eOptionId, void* pOption) {
    EXPECT_TRUE(gThis == this);
    return 10;
  }
};

struct SVCDecoderImpl : public ISVCDecoder {
  virtual ~SVCDecoderImpl() {}
  virtual long EXTAPI Initialize(void* pParam, const INIT_TYPE iInitType) {
    EXPECT_TRUE(gThis == this);
    return 1;
  }
  virtual long EXTAPI Uninitialize() {
    EXPECT_TRUE(gThis == this);
    return 2;
  }
  virtual DECODING_STATE EXTAPI DecodeFrame(const unsigned char* pSrc,
      const int iSrcLen, unsigned char** ppDst, int* pStride,
      int& iWidth, int& iHeight) {
    EXPECT_TRUE(gThis == this);
    return static_cast<DECODING_STATE>(3);
  }
  virtual DECODING_STATE EXTAPI DecodeFrame2(const unsigned char* pSrc,
      const int iSrcLen, void** ppDst, SBufferInfo* pDstInfo) {
    EXPECT_TRUE(gThis == this);
    return static_cast<DECODING_STATE>(4);
  }
  virtual DECODING_STATE EXTAPI DecodeFrameEx(const unsigned char* pSrc,
      const int iSrcLen, unsigned char* pDst, int iDstStride,
      int& iDstLen, int& iWidth, int& iHeight, int& iColorFormat) {
    EXPECT_TRUE(gThis == this);
    return static_cast<DECODING_STATE>(5);
  }
  virtual long EXTAPI SetOption (DECODER_OPTION eOptionId, void* pOption) {
    EXPECT_TRUE(gThis == this);
    return 6;
  }
  virtual long EXTAPI GetOption (DECODER_OPTION eOptionId, void* pOption) {
    EXPECT_TRUE(gThis == this);
    return 7;
  }
};

TEST(ISVCEncoderTest, CheckFunctionOrder) {
  SVCEncoderImpl* p = new SVCEncoderImpl;
  gThis = p;
  CheckEncoderInterface(p, CheckFunctionOrder);
  delete p;
}

TEST(ISVCDecoderTest, CheckFunctionOrder) {
  SVCDecoderImpl* p = new SVCDecoderImpl;
  gThis = p;
  CheckDecoderInterface(p, CheckFunctionOrder);
  delete p;
}
