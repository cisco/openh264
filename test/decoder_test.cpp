#include <gtest/gtest.h>
#if defined (WIN32)
#include <windows.h>
#include <tchar.h>
#else
#include <string.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sha.h" // for cryptopp sha1 functions
#include "codec_def.h"
#include "codec_app_def.h"
#include "codec_api.h"
class DecoderTest : public ::testing::Test {
 public:
  DecoderTest() : decoder_ (NULL) {}

  ~DecoderTest() {
    if (decoder_) decoder_->Uninitialize();
    if (decoder_) DestroyDecoder (decoder_);
  }

  void SetUp() {
    long rv = CreateDecoder (&decoder_);
    SDecodingParam sDecParam;
    memset(&sDecParam, 0, sizeof(sDecParam));
    sDecParam.iOutputColorFormat = videoFormatI420;
    sDecParam.uiTargetDqLayer = (uint8_t) - 1;
    sDecParam.uiEcActiveFlag = 1;
    sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
    ASSERT_EQ (0, rv);
    ASSERT_TRUE (decoder_);
    rv = decoder_->Initialize (&sDecParam, INIT_TYPE_PARAMETER_BASED);
    ASSERT_EQ (0, rv);
  }
 // decode a bitstream @bitstream and compute 
 // the SHA1 hash of the reconstruction YUV file
 void updateHash(CryptoPP::SHA1 & hash, uint8_t *buf, int width, int height, int stride) {
    for(int i = 0; i < height; i++)
      hash.Update(buf + i * stride, width);
 }
 void DecodeHash(const char* bitstream, unsigned char* md) {
  FILE* pH264File	  = NULL;
  int64_t iStart = 0, iEnd = 0, iTotal = 0;
  int32_t iSliceSize;
  int32_t iSliceIndex = 0;
  uint8_t* pBuf = NULL;
  uint8_t uiStartCode[4] = {0, 0, 0, 1};
  int iWidth, iHeight;
  void* pData[3] = {NULL};
  uint8_t* pDst[3] = {NULL};
  SBufferInfo sDstBufInfo;
  CryptoPP::SHA1 hash;
  int32_t iBufPos = 0;
  int32_t iFileSize;
  int32_t i = 0;
  int32_t iLastWidth = 0, iLastHeight = 0;
  int32_t iFrameCount = 0;
  int32_t iEndOfStreamFlag = 0;
  int iYStride, iUVStride;
  static int32_t iFrameNum = 0;

  EDecodeMode     eDecoderMode    = AUTO_MODE;
  EBufferProperty	eOutputProperty = BUFFER_DEVICE;
  double dElapsed = 0;
  pH264File = fopen(bitstream, "rb");
  ASSERT_TRUE(pH264File);
  fseek (pH264File, 0L, SEEK_END);
  iFileSize = ftell (pH264File);
  if (iFileSize <= 0) {
    fprintf (stderr, "Current Bit Stream File is too small, read error!!!!\n");
    goto label_exit;
  }
  fseek (pH264File, 0L, SEEK_SET);

  pBuf = new uint8_t[iFileSize + 4];
  if (pBuf == NULL) {
    fprintf (stderr, "new buffer failed!\n");
    goto label_exit;
  }

  fread (pBuf, 1, iFileSize, pH264File);
  memcpy (pBuf + iFileSize, &uiStartCode[0], 4); //confirmed_safe_unsafe_usage

  while (true) {

    if (iBufPos >= iFileSize) {
      iEndOfStreamFlag = true;
      break;
    }

    for (i = 0; i < iFileSize; i++) {
      if (pBuf[iBufPos + i] == 0 && pBuf[iBufPos + i + 1] == 0 && pBuf[iBufPos + i + 2] == 0 &&
          pBuf[iBufPos + i + 3] == 1 && i > 0) {
        break;
      }
    }
    iSliceSize = i;

    pData[0] = NULL;
    pData[1] = NULL;
    pData[2] = NULL;
    memset (&sDstBufInfo, 0, sizeof (SBufferInfo));

    decoder_->DecodeFrame (pBuf + iBufPos, iSliceSize, pData, &sDstBufInfo);

    if (sDstBufInfo.iBufferStatus == 1) {
      pDst[0] = (uint8_t*)pData[0];
      pDst[1] = (uint8_t*)pData[1];
      pDst[2] = (uint8_t*)pData[2];
    }
    iTotal	+= iEnd - iStart;
    if (sDstBufInfo.iBufferStatus == 1) {
      iFrameNum++;
      if (sDstBufInfo.eBufferProperty == BUFFER_HOST) {
        iWidth  = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
        iHeight = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
        iYStride = sDstBufInfo.UsrData.sSystemBuffer.iStride[0];
        iUVStride = sDstBufInfo.UsrData.sSystemBuffer.iStride[1];
      } else {
        iWidth  = sDstBufInfo.UsrData.sVideoBuffer.iSurfaceWidth;
        iHeight = sDstBufInfo.UsrData.sVideoBuffer.iSurfaceHeight;
      }
      updateHash(hash, pDst[0], iWidth, iHeight, iYStride);
      updateHash(hash, pDst[1], iWidth >> 1, iHeight >> 1, iUVStride);
      updateHash(hash, pDst[2], iWidth >> 1, iHeight >> 1, iUVStride);
      ++ iFrameCount;
    }

    iBufPos += iSliceSize;
    ++ iSliceIndex;
  }

  // Get pending last frame
  pData[0] = NULL;
  pData[1] = NULL;
  pData[2] = NULL;
  memset (&sDstBufInfo, 0, sizeof (SBufferInfo));

  decoder_->DecodeFrame (NULL, 0, pData, &sDstBufInfo);
  if (sDstBufInfo.iBufferStatus == 1) {
    pDst[0] = (uint8_t*)pData[0];
    pDst[1] = (uint8_t*)pData[1];
    pDst[2] = (uint8_t*)pData[2];
  }

  if (sDstBufInfo.iBufferStatus == 1) {
    if (sDstBufInfo.eBufferProperty == BUFFER_HOST) {
      iWidth  = sDstBufInfo.UsrData.sSystemBuffer.iWidth;
      iHeight = sDstBufInfo.UsrData.sSystemBuffer.iHeight;
      iYStride = sDstBufInfo.UsrData.sSystemBuffer.iStride[0];
      iUVStride = sDstBufInfo.UsrData.sSystemBuffer.iStride[1];
    } else {
      iWidth  = sDstBufInfo.UsrData.sVideoBuffer.iSurfaceWidth;
      iHeight = sDstBufInfo.UsrData.sVideoBuffer.iSurfaceHeight;
    }
    updateHash(hash, pDst[0], iWidth, iHeight, iYStride);
    updateHash(hash, pDst[1], iWidth >> 1, iHeight >> 1, iUVStride);
    updateHash(hash, pDst[2], iWidth >> 1, iHeight >> 1, iUVStride);
    ++ iFrameCount;
  }
  hash.Final(md);
label_exit:
  if (pBuf) {
    delete[] pBuf;
    pBuf = NULL;
  }
  if (pH264File) {
    fclose (pH264File);
    pH264File = NULL;
  }
 }
 protected:
  ISVCDecoder* decoder_;
};

TEST_F (DecoderTest, SGETest) {
  const char *hashfile = "./test/sha1sum.yuv.txt";
  FILE *fp = fopen(hashfile, "r");
  ASSERT_TRUE(fp);
  char buf[1024];
  char *ret = fgets(buf, 1024, fp);
  char ref_hash[256], bitstream[1024];
  unsigned char hash[128];
  while(ret != NULL) {
    sscanf(ret, "%s%s", ref_hash, bitstream);
    DecodeHash(bitstream, hash);
    for(int i = 0; i < 20; i++) {
      sprintf(buf+2*i, "%02x", hash[i]);
    }
    int rv = memcmp(ref_hash, buf, 40); // for SHA1, there are 20 byte hash, the string is 40bytes
    ASSERT_EQ(rv, 0);
    ret = fgets(buf, 1024, fp);
  }
}

