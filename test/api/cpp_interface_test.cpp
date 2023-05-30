#include <gtest/gtest.h>
#include "codec_api.h"
#include <stddef.h>

static void CheckFunctionOrder (int expect, int actual, const char* name) {
  EXPECT_EQ (expect, actual) << "Wrong function order: " << name;
}

typedef void (*CheckFunc) (int, int, const char*);
extern "C" void CheckEncoderInterface (ISVCEncoder* p, CheckFunc);
extern "C" void CheckDecoderInterface (ISVCDecoder* p, CheckFunc);
extern "C" size_t GetBoolSize (void);
extern "C" size_t GetBoolOffset (void);
extern "C" size_t GetBoolStructSize (void);

// Store the 'this' pointer to verify 'this' is received as expected from C code.
static void* gThis;

/**
 * Return a unique number for each virtual function so that we are able to
 * check if the order of functions in the virtual table is as expected.
 */
struct SVCEncoderImpl : public ISVCEncoder {
  virtual ~SVCEncoderImpl() {}
  virtual int EXTAPI Initialize (const SEncParamBase* pParam) {
    EXPECT_TRUE (gThis == this);
    return 1;
  }
  virtual int EXTAPI InitializeExt (const SEncParamExt* pParam) {
    EXPECT_TRUE (gThis == this);
    return 2;
  }
  virtual int EXTAPI GetDefaultParams (SEncParamExt* pParam) {
    EXPECT_TRUE (gThis == this);
    return 3;
  }
  virtual int EXTAPI Uninitialize() {
    EXPECT_TRUE (gThis == this);
    return 4;
  }
  virtual int EXTAPI EncodeFrame (const SSourcePicture* kpSrcPic,
                                  SFrameBSInfo* pBsInfo) {
    EXPECT_TRUE (gThis == this);
    return 5;
  }
  virtual int EXTAPI EncodeParameterSets (SFrameBSInfo* pBsInfo) {
    EXPECT_TRUE (gThis == this);
    return 6;
  }
  virtual int EXTAPI ForceIntraFrame (bool bIDR, int iLayerId = -1) {
    EXPECT_TRUE (gThis == this);
    return 7;
  }
  virtual int EXTAPI SetOption (ENCODER_OPTION eOptionId, void* pOption) {
    EXPECT_TRUE (gThis == this);
    return 8;
  }
  virtual int EXTAPI GetOption (ENCODER_OPTION eOptionId, void* pOption) {
    EXPECT_TRUE (gThis == this);
    return 9;
  }
};

struct SVCDecoderImpl : public ISVCDecoder {
  virtual ~SVCDecoderImpl() {}
  virtual long EXTAPI Initialize (const SDecodingParam* pParam) {
    EXPECT_TRUE (gThis == this);
    return 1;
  }
  virtual long EXTAPI Uninitialize() {
    EXPECT_TRUE (gThis == this);
    return 2;
  }
  virtual DECODING_STATE EXTAPI DecodeFrame (const unsigned char* pSrc,
      const int iSrcLen, unsigned char** ppDst, int* pStride,
      int& iWidth, int& iHeight) {
    EXPECT_TRUE (gThis == this);
    return static_cast<DECODING_STATE> (3);
  }
  virtual DECODING_STATE EXTAPI DecodeFrameNoDelay (const unsigned char* pSrc,
      const int iSrcLen, unsigned char** ppDst, SBufferInfo* pDstInfo) {
    EXPECT_TRUE (gThis == this);
    return static_cast<DECODING_STATE> (4);
  }
  virtual DECODING_STATE EXTAPI ParseBitstreamGetMotionVectors (const unsigned char* kpSrc,
    const int kiSrcLen, unsigned char** ppDst, SParserBsInfo* pDstInfo, SBufferInfo* ppDecodeInfo, int32_t* motionVectorSize,
    int16_t** motionVectorData) {
    EXPECT_TRUE (gThis == this);
    return static_cast<DECODING_STATE> (11);
  }
  virtual DECODING_STATE EXTAPI DecodeFrameGetMotionVectorsNoDelay (const unsigned char* pSrc,
      const int iSrcLen, unsigned char** ppDst, SBufferInfo* pDstInfo,int32_t* motionVectorSize,
      int16_t** motionVectorData) {
    EXPECT_TRUE (gThis == this);
    return static_cast<DECODING_STATE> (12);
  }
  virtual DECODING_STATE EXTAPI DecodeFrame2 (const unsigned char* pSrc,
      const int iSrcLen, unsigned char** ppDst, SBufferInfo* pDstInfo) {
    EXPECT_TRUE (gThis == this);
    return static_cast<DECODING_STATE> (5);
  }
  virtual DECODING_STATE EXTAPI FlushFrame (unsigned char** ppDst, SBufferInfo* pDstInfo) {
    EXPECT_TRUE (gThis == this);
    return static_cast<DECODING_STATE> (10);
  }
  virtual DECODING_STATE EXTAPI DecodeFrameEx (const unsigned char* pSrc,
      const int iSrcLen, unsigned char* pDst, int iDstStride,
      int& iDstLen, int& iWidth, int& iHeight, int& iColorFormat) {
    EXPECT_TRUE (gThis == this);
    return static_cast<DECODING_STATE> (6);
  }
  virtual DECODING_STATE EXTAPI DecodeParser (const unsigned char* pSrc,
      const int iSrcLen, SParserBsInfo* pDstInfo) {
    EXPECT_TRUE (gThis == this);
    return static_cast<DECODING_STATE> (7);
  }
  virtual long EXTAPI SetOption (DECODER_OPTION eOptionId, void* pOption) {
    EXPECT_TRUE (gThis == this);
    return static_cast<DECODING_STATE> (8);
  }
  virtual long EXTAPI GetOption (DECODER_OPTION eOptionId, void* pOption) {
    EXPECT_TRUE (gThis == this);
    return static_cast<DECODING_STATE> (9);
  }
};

TEST (ISVCEncoderTest, CheckFunctionOrder) {
  SVCEncoderImpl* p = new SVCEncoderImpl;
  gThis = p;
  CheckEncoderInterface (p, CheckFunctionOrder);
  delete p;
}

TEST (ISVCDecoderTest, CheckFunctionOrder) {
  SVCDecoderImpl* p = new SVCDecoderImpl;
  gThis = p;
  CheckDecoderInterface (p, CheckFunctionOrder);
  delete p;
}

struct bool_test_struct {
  char c;
  bool b;
};

TEST (ISVCDecoderEncoderTest, CheckCAbi) {
  EXPECT_EQ (sizeof (bool), GetBoolSize()) << "Wrong size of bool type";
  EXPECT_EQ (offsetof (bool_test_struct, b), GetBoolOffset()) << "Wrong alignment of bool in a struct";
  EXPECT_EQ (sizeof (bool_test_struct), GetBoolStructSize()) << "Wrong size of struct with a bool";
}
