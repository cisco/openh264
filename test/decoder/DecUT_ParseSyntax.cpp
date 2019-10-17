#include <gtest/gtest.h>
#include "codec_app_def.h"
#include "codec_api.h"
#include "decoder_context.h"
#include "decoder.h"
#include "decoder_core.h"
#include "welsCodecTrace.h"
#include "../../common/src/welsCodecTrace.cpp"

using namespace WelsDec;

#define BUF_SIZE 100
typedef enum DecCase {
  CorrectDec = 0,
  ErrorDec = 1,
  CorrectParseOnly = 2,
  ErrorParseOnly = 3,
  RESERVED,
} EDecCase;

DECODING_STATE DecodeFrame (const unsigned char* kpSrc,
                            const int kiSrcLen,
                            unsigned char** ppDst,
                            SBufferInfo* pDstInfo,
                            PWelsDecoderContext pCtx) {
  PWelsDecoderContext m_pDecContext = pCtx;
  if (CheckBsBuffer (m_pDecContext, kiSrcLen)) {
    return dsOutOfMemory;
  }
  if (kiSrcLen > 0 && kpSrc != NULL) {
    m_pDecContext->bEndOfStreamFlag = false;
  } else {
    //For application MODE, the error detection should be added for safe.
    //But for CONSOLE MODE, when decoding LAST AU, kiSrcLen==0 && kpSrc==NULL.
    m_pDecContext->bEndOfStreamFlag = true;
    m_pDecContext->bInstantDecFlag = true;
  }


  ppDst[0] = ppDst[1] = ppDst[2] = NULL;
  m_pDecContext->iErrorCode             = dsErrorFree; //initialize at the starting of AU decoding.
  m_pDecContext->iFeedbackVclNalInAu = FEEDBACK_UNKNOWN_NAL; //initialize
  unsigned long long uiInBsTimeStamp = pDstInfo->uiInBsTimeStamp;
  memset (pDstInfo, 0, sizeof (SBufferInfo));
  pDstInfo->uiInBsTimeStamp = uiInBsTimeStamp;

  m_pDecContext->bReferenceLostAtT0Flag       = false; //initialize for LTR
  m_pDecContext->bCurAuContainLtrMarkSeFlag = false;
  m_pDecContext->iFrameNumOfAuMarkedLtr      = 0;
  m_pDecContext->iFrameNum                       = -1; //initialize


  m_pDecContext->iFeedbackTidInAu             = -1; //initialize
  if (pDstInfo) {
    pDstInfo->uiOutYuvTimeStamp = 0;
    m_pDecContext->uiTimeStamp = pDstInfo->uiInBsTimeStamp;
  } else {
    m_pDecContext->uiTimeStamp = 0;
  }
  WelsDecodeBs (m_pDecContext, kpSrc, kiSrcLen, ppDst,
                pDstInfo, NULL); //iErrorCode has been modified in this function
  m_pDecContext->bInstantDecFlag = false; //reset no-delay flag

  return (DECODING_STATE) m_pDecContext->iErrorCode;
}

void UninitDecoder (PWelsDecoderContext& pCtx) {
  if (NULL == pCtx)
    return;

  WelsEndDecoder (pCtx);
  if (NULL != pCtx->pMemAlign) {
    delete pCtx->pMemAlign;
    pCtx->pMemAlign = NULL;
  }
  if (NULL != pCtx) {
    free (pCtx);
    pCtx = NULL;
  }

}

int32_t InitDecoder (const SDecodingParam* pParam, PWelsDecoderContext pCtx, SLogContext* pLogCtx) {


  if (NULL == pCtx)
    return cmMallocMemeError;

  if (NULL == pCtx->pMemAlign) {
    pCtx->pMemAlign = new CMemoryAlign (16);
    if (NULL == pCtx->pMemAlign)
      return cmMallocMemeError;
  }

  pCtx->sLogCtx = *pLogCtx;

  //check param and update decoder context
  pCtx->pParam = (SDecodingParam*) pCtx->pMemAlign->WelsMallocz (sizeof (SDecodingParam), "SDecodingParam");
  WELS_VERIFY_RETURN_PROC_IF (cmMallocMemeError, (NULL == pCtx->pParam), UninitDecoder (pCtx));
  int32_t iRet = DecoderConfigParam (pCtx, pParam);
  WELS_VERIFY_RETURN_IFNEQ (iRet, cmResultSuccess);

  WELS_VERIFY_RETURN_PROC_IF (cmInitParaError, WelsInitDecoder (pCtx, pLogCtx), UninitDecoder (pCtx));

  return cmResultSuccess;
}

long Initialize (const SDecodingParam* pParam, PWelsDecoderContext pCtx, SLogContext* pLogCtx) {
  int iRet = ERR_NONE;
  if (pParam == NULL) {
    return cmInitParaError;
  }

  // H.264 decoder initialization,including memory allocation,then open it ready to decode
  iRet = InitDecoder (pParam, pCtx, pLogCtx);
  if (iRet)
    return iRet;

  return cmResultSuccess;
}

class DecoderParseSyntaxTest : public ::testing::Test {
 public:
  virtual void SetUp() {

    int rv = WelsCreateDecoder (&m_pDec);
    ASSERT_EQ (0, rv);
    ASSERT_TRUE (m_pDec != NULL);
  }

  virtual void TearDown() {
    if (m_pDec) {
      WelsDestroyDecoder (m_pDec);
    }
  }
  //Init members
  int32_t Init();
  //Uninit members
  void Uninit();
  //Decoder real bitstream
  bool DecodeBs (const char* sFileName, EDecCase eDecCase);
  //Parse real bitstream
  bool ParseBs (const char* sFileName, EDecCase eDecCase);
  //Scalinglist
  void TestScalingList();
  //specific bitstream test
  void TestSpecificBs();
  void TestSpecificBsError();
  //Do whole tests here
  void DecoderParseSyntaxTestAll();


 public:
  ISVCDecoder* m_pDec;
  SDecodingParam m_sDecParam;
  SBufferInfo m_sBufferInfo;
  SParserBsInfo m_sParserBsInfo;
  SWelsDecoderSpsPpsCTX   m_sDecoderSpsPpsCTX;
  SWelsLastDecPicInfo     m_sLastDecPicInfo;
  SDecoderStatistics      m_sDecoderStatistics;
  SVlcTable               m_sVlcTable;

  uint8_t* m_pData[3];
  unsigned char m_szBuffer[BUF_SIZE]; //for mocking packet
  int m_iBufLength; //record the valid data in m_szBuffer
  PWelsDecoderContext m_pCtx;
  welsCodecTrace* m_pWelsTrace;

};

//Init members
int32_t DecoderParseSyntaxTest::Init() {
  memset (&m_sBufferInfo, 0, sizeof (SBufferInfo));
  memset (&m_sDecParam, 0, sizeof (SDecodingParam));
  memset (&m_sParserBsInfo, 0, sizeof (SParserBsInfo));
  memset (&m_sDecoderSpsPpsCTX, 0, sizeof (SWelsDecoderSpsPpsCTX));
  memset (&m_sLastDecPicInfo, 0, sizeof (SWelsLastDecPicInfo));
  memset (&m_sDecoderStatistics, 0, sizeof (SDecoderStatistics));
  memset (&m_sVlcTable, 0, sizeof (SVlcTable));

  m_sDecParam.pFileNameRestructed = NULL;
  m_sDecParam.uiCpuLoad = rand() % 100;
  m_sDecParam.uiTargetDqLayer = rand() % 100;
  m_sDecParam.eEcActiveIdc = (ERROR_CON_IDC)7;
  m_sDecParam.sVideoProperty.size = sizeof (SVideoProperty);
  m_sDecParam.sVideoProperty.eVideoBsType = (VIDEO_BITSTREAM_TYPE) (rand() % 2);
  m_sDecParam.bParseOnly = false;

  m_pData[0] = m_pData[1] = m_pData[2] = NULL;
  m_szBuffer[0] = m_szBuffer[1] = m_szBuffer[2] = 0;
  m_szBuffer[3] = 1;
  m_iBufLength = 4;
  //
  m_pCtx = (PWelsDecoderContext)malloc (sizeof (SWelsDecoderContext));
  if (m_pCtx == NULL)
    return ERR_MALLOC_FAILED;
  memset (m_pCtx, 0, sizeof (SWelsDecoderContext));
  m_pWelsTrace = new welsCodecTrace();
  if (m_pWelsTrace != NULL) {
    m_pWelsTrace->SetTraceLevel (WELS_LOG_ERROR);
  } else {
    free (m_pCtx);
    m_pCtx = NULL;
    return ERR_MALLOC_FAILED;
  }
  m_pCtx->pLastDecPicInfo = &m_sLastDecPicInfo;
  m_pCtx->pDecoderStatistics = &m_sDecoderStatistics;
  m_pCtx->pVlcTable = &m_sVlcTable;
  WelsDecoderSpsPpsDefaults (m_pCtx->sSpsPpsCtx);
  CM_RETURN eRet = (CM_RETURN)Initialize (&m_sDecParam, m_pCtx, &m_pWelsTrace->m_sLogCtx);
  return (int32_t)eRet;
}

void DecoderParseSyntaxTest::Uninit() {
  if (m_pCtx) {
    UninitDecoder (m_pCtx);
  }
  if (m_pWelsTrace) {
    delete m_pWelsTrace;
    m_pWelsTrace = NULL;
  }
  memset (&m_sDecParam, 0, sizeof (SDecodingParam));
  memset (&m_sBufferInfo, 0, sizeof (SBufferInfo));
  m_pData[0] = m_pData[1] = m_pData[2] = NULL;
  m_iBufLength = 0;
}

bool DecoderParseSyntaxTest::DecodeBs (const char* sFileName, EDecCase eDecCase) {

  uint8_t* pBuf = NULL;
  int32_t iBufPos = 0;
  int32_t iFileSize;
  int32_t i = 0;
  int32_t iSliceSize;
  int32_t iEndOfStreamFlag = 0;
  FILE* pH264File;
  uint8_t uiStartCode[4] = {0, 0, 0, 1};
  int iRet = 0;

#if defined(ANDROID_NDK)
  std::string filename = std::string ("/sdcard/") + sFileName;
  if ((pH264File = fopen (filename.c_str(), "rb")) == NULL)
    return false;
#else
  if ((pH264File = fopen (sFileName, "rb")) == NULL)
    return false;
#endif
  fseek (pH264File, 0L, SEEK_END);
  iFileSize = (int32_t) ftell (pH264File);
  fseek (pH264File, 0L, SEEK_SET);
  pBuf = new uint8_t[iFileSize + 4];
  if (pBuf == NULL) {
    fclose (pH264File);
    return false;
  }
  if ((fread (pBuf, 1, iFileSize, pH264File) != (unsigned int) iFileSize)) {
    fclose (pH264File);
    if (pBuf) {
      delete[] pBuf;
      pBuf = NULL;
    }
    return false;
  }
  memcpy (pBuf + iFileSize, &uiStartCode[0], 4); //confirmed_safe_unsafe_usage
  while (true) {
    if (iBufPos >= iFileSize) {
      iEndOfStreamFlag = true;
      if (iEndOfStreamFlag)
        m_pDec->SetOption (DECODER_OPTION_END_OF_STREAM, (void*)&iEndOfStreamFlag);
      break;
    }
    for (i = 0; i < iFileSize; i++) {
      if ((pBuf[iBufPos + i] == 0 && pBuf[iBufPos + i + 1] == 0 && pBuf[iBufPos + i + 2] == 0 && pBuf[iBufPos + i + 3] == 1
           && i > 0)) {
        break;
      }
    }
    iSliceSize = i;
    iRet |= DecodeFrame (pBuf + iBufPos, iSliceSize, m_pData, &m_sBufferInfo, m_pCtx);
    iBufPos += iSliceSize;
  }
  if (eDecCase == CorrectDec)
    EXPECT_TRUE (iRet == dsErrorFree);
  else if (eDecCase == ErrorDec)
    EXPECT_TRUE ((iRet & (dsBitstreamError | dsRefLost | dsDataErrorConcealed)) != 0) << "iRet = " << iRet;

  fclose (pH264File);
  if (pBuf) {
    delete[] pBuf;
    pBuf = NULL;
  }

  return true;
}
bool DecoderParseSyntaxTest::ParseBs (const char* sFileName, EDecCase eDecCase) {

  uint8_t* pBuf = NULL;
  int32_t iBufPos = 0;
  int32_t iFileSize;
  int32_t i = 0;
  int32_t iSliceSize;
  int32_t iSliceIndex = 0;
  int32_t iEndOfStreamFlag = 0;
  FILE* pH264File;
  uint8_t uiStartCode[4] = { 0, 0, 0, 1 };
  int iRet = 0;

#if defined(ANDROID_NDK)
  std::string filename = std::string ("/sdcard/") + sFileName;
  if ((pH264File = fopen (filename.c_str(), "rb")) == NULL)
    return false;
#else
  if ((pH264File = fopen (sFileName, "rb")) == NULL)
    return false;
#endif
  fseek (pH264File, 0L, SEEK_END);
  iFileSize = (int32_t)ftell (pH264File);
  fseek (pH264File, 0L, SEEK_SET);
  pBuf = new uint8_t[iFileSize + 4];
  if (pBuf == NULL) {
    fclose (pH264File);
    return false;
  }
  if (fread (pBuf, 1, iFileSize, pH264File) != (unsigned int)iFileSize) {
    fclose (pH264File);
    if (pBuf) {
      delete[] pBuf;
      pBuf = NULL;
    }
    return false;
  }
  memcpy (pBuf + iFileSize, &uiStartCode[0], 4); //confirmed_safe_unsafe_usage
  while (true) {
    if (iBufPos >= iFileSize) {
      iEndOfStreamFlag = true;
      if (iEndOfStreamFlag)
        m_pDec->SetOption (DECODER_OPTION_END_OF_STREAM, (void*)&iEndOfStreamFlag);
      break;
    }
    for (i = 0; i < iFileSize; i++) {
      if ((pBuf[iBufPos + i] == 0 && pBuf[iBufPos + i + 1] == 0 && pBuf[iBufPos + i + 2] == 0 && pBuf[iBufPos + i + 3] == 1
           && i > 0)) {
        break;
      }
    }
    iSliceSize = i;
    memset (&m_sParserBsInfo, 0, sizeof (SParserBsInfo));
    iRet |= m_pDec->DecodeParser (pBuf + iBufPos, iSliceSize, &m_sParserBsInfo);
    iRet |= m_pDec->DecodeParser (NULL, 0, &m_sParserBsInfo);
    if (eDecCase == CorrectParseOnly)
      EXPECT_TRUE (iRet == dsErrorFree || iRet == dsFramePending);
    iBufPos += iSliceSize;
    ++iSliceIndex;
    if (iSliceIndex == 4)
      break;
  }
  if (eDecCase == ErrorDec)
    EXPECT_TRUE ((iRet & (dsBitstreamError | dsRefLost | dsDataErrorConcealed)) != 0) << iRet;

  fclose (pH264File);
  if (pBuf) {
    delete[] pBuf;
    pBuf = NULL;
  }

  return true;
}


void DecoderParseSyntaxTest::TestScalingList() {
  uint8_t iScalingList[6][16] = {
    {17, 17, 16, 16, 17, 16, 15, 15, 16, 15, 15, 15, 16, 15, 15, 15 },
    { 6, 12, 19, 26, 12, 19, 26, 31, 19, 26, 31, 35, 26, 31, 35, 39 },
    { 6, 12, 19, 26, 12, 19, 26, 31, 19, 26, 31, 35, 26, 31, 35, 40 },
    {17, 17, 16, 16, 17, 16, 15, 15, 16, 15, 15, 15, 16, 15, 15, 14 },
    {10, 14, 20, 24, 14, 20, 24, 27, 20, 24, 27, 30, 24, 27, 30, 34 },
    { 9, 13, 18, 21, 13, 18, 21, 24, 18, 21, 24, 27, 21, 24, 27, 27 }
  };
  uint8_t iScalingListPPS[6][16] = {
    { 17, 17, 16, 16, 17, 16, 15, 15, 16, 15, 15, 15, 16, 15, 15, 15 },
    { 6, 12, 19, 26, 12, 19, 26, 31, 19, 26, 31, 35, 26, 31, 35, 39 },
    { 6, 12, 19, 26, 12, 19, 26, 31, 19, 26, 31, 35, 26, 31, 35, 40 },
    { 17, 17, 16, 16, 17, 16, 15, 15, 16, 15, 15, 15, 16, 15, 15, 14 },
    { 10, 14, 20, 24, 14, 20, 24, 27, 20, 24, 27, 30, 24, 27, 30, 34 },
    { 9, 13, 18, 21, 13, 18, 21, 24, 18, 21, 24, 27, 21, 24, 27, 27 }
  };
  uint8_t iScalingListZero[6][16];
  memset (iScalingListZero, 0, 6 * 16 * sizeof (uint8_t));
  //Scalinglist matrix not written into sps or pps
  int32_t iRet = ERR_NONE;
  iRet = Init();
  ASSERT_EQ (iRet, ERR_NONE);
  ASSERT_TRUE (DecodeBs ("res/BA_MW_D.264", CorrectDec));
  ASSERT_TRUE (m_pCtx->sSpsPpsCtx.sSpsBuffer[0].bSeqScalingMatrixPresentFlag == false);
  EXPECT_EQ (0, memcmp (iScalingListZero, m_pCtx->sSpsPpsCtx.sSpsBuffer[0].iScalingList4x4, 6 * 16 * sizeof (uint8_t)));
  ASSERT_TRUE (m_pCtx->sSpsPpsCtx.sPpsBuffer[0].bPicScalingMatrixPresentFlag == false);
  EXPECT_EQ (0, memcmp (iScalingListZero, m_pCtx->sSpsPpsCtx.sPpsBuffer[0].iScalingList4x4, 6 * 16 * sizeof (uint8_t)));
  Uninit();
  //Scalinglist value just written into sps and pps
  iRet = Init();
  ASSERT_EQ (iRet, ERR_NONE);
  ASSERT_TRUE (DecodeBs ("res/test_scalinglist_jm.264", CorrectDec));
  ASSERT_TRUE (m_pCtx->sSpsPpsCtx.sSpsBuffer[0].bSeqScalingMatrixPresentFlag);
  for (int i = 0; i < 6; i++) {
    EXPECT_EQ (0, memcmp (iScalingList[i], m_pCtx->sSpsPpsCtx.sSpsBuffer[0].iScalingList4x4[i], 16 * sizeof (uint8_t)));
  }

  ASSERT_TRUE (m_pCtx->sSpsPpsCtx.sPpsBuffer[0].bPicScalingMatrixPresentFlag == true);
  for (int i = 0; i < 6; i++) {
    EXPECT_EQ (0, memcmp (iScalingListPPS[i], m_pCtx->sSpsPpsCtx.sPpsBuffer[0].iScalingList4x4[i], 16 * sizeof (uint8_t)));
  }
  Uninit();
}

void DecoderParseSyntaxTest::TestSpecificBs() {
  int32_t iRet = ERR_NONE;
  m_sDecParam.bParseOnly = true;
  m_sDecParam.eEcActiveIdc = ERROR_CON_DISABLE;
  iRet = m_pDec->Initialize (&m_sDecParam);
  ASSERT_EQ (iRet, ERR_NONE);
  ASSERT_TRUE (ParseBs ("res/jm_1080p_allslice.264", CorrectParseOnly));
  m_pDec->Uninitialize();
}

void DecoderParseSyntaxTest::TestSpecificBsError() {
  int32_t iRet = ERR_NONE;
  Init();
  ASSERT_EQ (iRet, ERR_NONE);
  ASSERT_TRUE (DecodeBs ("res/Cisco_Men_whisper_640x320_CAVLC_Bframe_9.264", CorrectDec));
  Uninit();
}

//TEST here for whole tests
TEST_F (DecoderParseSyntaxTest, DecoderParseSyntaxTestAll) {

  TestScalingList();
  TestSpecificBs();
  TestSpecificBsError();
}


