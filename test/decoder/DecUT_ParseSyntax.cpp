#include <gtest/gtest.h>
#include "codec_app_def.h"
#include "codec_api.h"
#include "decoder_context.h"
#include "decoder.h"
#include "decoder_core.h"
#include "error_concealment.h"
#include "fmo.h"
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
  int32_t Init (bool bParseOnly = false);
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
  //I_PCM CAVLC bounds handling
  void TestIPcmCavlcRegression();
  void TestIPcmCavlcTruncated();
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
int32_t DecoderParseSyntaxTest::Init (bool bParseOnly) {
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
  m_sDecParam.bParseOnly = bParseOnly;

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
  if (eDecCase == CorrectDec) {
    EXPECT_TRUE (iRet == dsErrorFree);
  } else if (eDecCase == ErrorDec) {
    EXPECT_TRUE ((iRet & (dsBitstreamError | dsRefLost | dsDataErrorConcealed)) != 0) << "iRet = " << iRet;
  }

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
    if (eDecCase == CorrectParseOnly) {
      EXPECT_TRUE (iRet == dsErrorFree || iRet == dsFramePending);
    }

    iBufPos += iSliceSize;
    ++iSliceIndex;
    if (iSliceIndex == 4)
      break;
  }
  if (eDecCase == ErrorDec) {
    EXPECT_TRUE ((iRet & (dsBitstreamError | dsRefLost | dsDataErrorConcealed)) != 0) << iRet;
  }

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

// Regression: a valid CAVLC stream that uses I_PCM macroblocks must keep
// decoding correctly after the I_PCM 384-byte bounds check was added.
// CVPCMNL1_SVA_C.264 is a CAVLC I_PCM conformance stream (exercises the
// patched WelsActualDecodeMbCavlcISlice path).
void DecoderParseSyntaxTest::TestIPcmCavlcRegression() {
  int32_t iRet = Init();
  ASSERT_EQ (iRet, ERR_NONE);
  ASSERT_TRUE (DecodeBs ("res/CVPCMNL1_SVA_C.264", CorrectDec));
  Uninit();
}

// Security regression for SPARK-814292 (CWE-125): a CAVLC I_PCM macroblock
// copies 384 bytes straight from the bitstream buffer. When the buffer is
// truncated mid I_PCM, the decoder must fail closed instead of reading past
// the allocation. Feeding a truncated all-I_PCM stream must not crash and
// must surface a decoding error rather than dsErrorFree.
void DecoderParseSyntaxTest::TestIPcmCavlcTruncated() {
  int32_t iRet = Init();
  ASSERT_EQ (iRet, ERR_NONE);

  // Disable error concealment so a truncated I_PCM macroblock surfaces as a
  // bitstream error instead of being silently concealed.
  m_pCtx->pParam->eEcActiveIdc = ERROR_CON_DISABLE;
  InitErrorCon (m_pCtx);

  FILE* pH264File = fopen ("res/CVPCMNL1_SVA_C.264", "rb");
  ASSERT_TRUE (pH264File != NULL);
  fseek (pH264File, 0L, SEEK_END);
  int32_t iFileSize = (int32_t) ftell (pH264File);
  fseek (pH264File, 0L, SEEK_SET);
  ASSERT_GT (iFileSize, 384);

  // Drop the trailing 384 bytes so the final I_PCM macroblock is short of a
  // full 384-byte PCM payload.
  int32_t iTruncatedSize = iFileSize - 384;
  uint8_t* pBuf = new uint8_t[iTruncatedSize + 4];
  ASSERT_TRUE (pBuf != NULL);
  size_t uiRead = fread (pBuf, 1, iTruncatedSize, pH264File);
  fclose (pH264File);
  ASSERT_EQ (uiRead, (size_t) iTruncatedSize);
  uint8_t uiStartCode[4] = {0, 0, 0, 1};
  memcpy (pBuf + iTruncatedSize, &uiStartCode[0], 4);

  int32_t iBufPos = 0;
  int32_t iAggregatedRet = 0;
  while (iBufPos < iTruncatedSize) {
    int32_t i = 0;
    for (i = 0; i < iTruncatedSize - iBufPos; i++) {
      if (pBuf[iBufPos + i] == 0 && pBuf[iBufPos + i + 1] == 0 && pBuf[iBufPos + i + 2] == 0
          && pBuf[iBufPos + i + 3] == 1 && i > 0) {
        break;
      }
    }
    int32_t iSliceSize = i;
    if (iSliceSize <= 0)
      break;
    // The key property is stability: no out-of-bounds read / crash while
    // decoding the truncated I_PCM payload.
    iAggregatedRet |= DecodeFrame (pBuf + iBufPos, iSliceSize, m_pData, &m_sBufferInfo, m_pCtx);
    iBufPos += iSliceSize;
  }
  // Flush the decoder so any delayed/buffered picture is emitted with its status.
  int32_t iEndOfStreamFlag = 1;
  m_pDec->SetOption (DECODER_OPTION_END_OF_STREAM, (void*)&iEndOfStreamFlag);
  iAggregatedRet |= DecodeFrame (NULL, 0, m_pData, &m_sBufferInfo, m_pCtx);

  // Truncated I_PCM must not be reported as a clean decode.
  EXPECT_NE (dsErrorFree, iAggregatedRet);

  delete[] pBuf;
  Uninit();
}

// Verify that ExpandBsBuffer retargets pNalPos for both current-AU and queued
// next-AU NALs; pre-fix, only [0, uiActualUnitsNum] was walked, leaving
// next-AU entries dangling into the freed sSavedData buffer.
TEST_F (DecoderParseSyntaxTest, ExpandBsBufferRetargetsParseOnlyNalPos) {
  ASSERT_EQ (cmResultSuccess, Init (true));

  ASSERT_TRUE (m_pCtx != NULL);
  ASSERT_TRUE (m_pCtx->pParam != NULL);
  ASSERT_TRUE (m_pCtx->pParam->bParseOnly);
  ASSERT_TRUE (m_pCtx->sSavedData.pHead != NULL);
  ASSERT_TRUE (m_pCtx->pAccessUnitList != NULL);
  ASSERT_TRUE (m_pCtx->pAccessUnitList->pNalUnitsList != NULL);
  ASSERT_TRUE (m_pCtx->pAccessUnitList->uiCountUnitsNum >= 2);

  // Slot 0: current-AU NAL; slot 1: queued next-AU NAL (uiAvailUnitsNum > uiActualUnitsNum).
  m_pCtx->pAccessUnitList->uiActualUnitsNum = 1;
  m_pCtx->pAccessUnitList->uiAvailUnitsNum  = 2;

  uint8_t* pOldSavedHead = m_pCtx->sSavedData.pHead;
  const int32_t kiOldSavedSize = m_pCtx->iMaxBsBufferSizeInByte;
  ASSERT_TRUE (kiOldSavedSize > 64);

  PNalUnit pNal0 = m_pCtx->pAccessUnitList->pNalUnitsList[0];
  PNalUnit pNal1 = m_pCtx->pAccessUnitList->pNalUnitsList[1];
  ASSERT_TRUE (pNal0 != NULL);
  ASSERT_TRUE (pNal1 != NULL);

  pNal0->sNalData.sVclNal.pNalPos = pOldSavedHead + 16;
  pNal1->sNalData.sVclNal.pNalPos = pOldSavedHead + 32;  // queued next-AU entry

  const int32_t kiSrcLen = kiOldSavedSize / MAX_BUFFERED_NUM + 1;
  ASSERT_EQ (ERR_NONE, ExpandBsBuffer (m_pCtx, kiSrcLen));

  ASSERT_TRUE (m_pCtx->sSavedData.pHead != NULL);
  EXPECT_NE (pOldSavedHead, m_pCtx->sSavedData.pHead);
  // Both current-AU and queued next-AU pNalPos must be retargeted.
  EXPECT_EQ (m_pCtx->sSavedData.pHead + 16, pNal0->sNalData.sVclNal.pNalPos);
  EXPECT_EQ (m_pCtx->sSavedData.pHead + 32, pNal1->sNalData.sVclNal.pNalPos);

  Uninit();
}

// Regression: a second SPS that reuses an existing id with a larger
// resolution is an in-place overwrite, which CheckNewSeqBeginAndUpdateActiveLayerSps
// does not see as a new sequence (the active-sps pointer is unchanged and the
// slice referencing it is non-IDR). Before the fix, the larger picture was
// decoded into Dq-layer buffers still sized for the previous, smaller
// resolution, so the picture-start memsets in DecodeCurrentAccessUnit wrote
// past pMbCorrectlyDecodedFlag / pSliceIdc (ASan heap-buffer-overflow). The
// fix forces the new-sequence reallocation path when the active SPS exceeds
// the allocated Dq-layer capacity.
//
// Captured, reduced parse-only bitstream: SPS id 0 @ 176x144 (99 MBs), a
// coded picture at that resolution, then SPS id 0 re-sent @ 320x192 (240 MBs)
// followed by a non-IDR slice that uses it.
TEST_F (DecoderParseSyntaxTest, ParseOnlySpsResolutionOverwriteNoOverflow) {
  static const uint8_t kBs[] = {
    0x00, 0x00, 0x01, 0x67, 0x42, 0xe0, 0x15, 0x8d, 0x94, 0xc1, 0x62, 0x72, 0x00, 0x00, 0x01, 0x68,
    0xce, 0x38, 0x00, 0x00, 0x01, 0x65, 0x88, 0x80, 0x00, 0x40, 0x01, 0x98, 0xd2, 0x2f, 0x01, 0x07,
    0xdd, 0xf8, 0x03, 0x30, 0x20, 0xfe, 0xc3, 0xa8, 0x8b, 0xef, 0x0b, 0xfc, 0xcf, 0x9c, 0x7a, 0x07,
    0xdf, 0x9f, 0x15, 0x6e, 0x18, 0xfa, 0x14, 0x43, 0x03, 0x9f, 0xa8, 0xfc, 0x18, 0xb0, 0x0c, 0x74,
    0xbc, 0x0b, 0xf8, 0x90, 0x10, 0xb4, 0xe6, 0x60, 0x34, 0x52, 0xa8, 0xa5, 0x80, 0x20, 0x2a, 0x93,
    0x36, 0xe0, 0xa3, 0x50, 0xc5, 0xcb, 0xff, 0xfb, 0xbb, 0xbf, 0x77, 0xc6, 0xe2, 0x91, 0x4d, 0x81,
    0xaf, 0xab, 0x7c, 0xdf, 0x2f, 0x78, 0x0d, 0x92, 0xa2, 0xc8, 0xc0, 0x7c, 0x7a, 0xd9, 0x4a, 0x0e,
    0xa8, 0xea, 0xc1, 0x02, 0x04, 0xa6, 0xbe, 0x48, 0xcb, 0xfe, 0xef, 0x7f, 0xfb, 0xbd, 0x40, 0x62,
    0x28, 0xf0, 0x09, 0x03, 0xd8, 0xc9, 0x2b, 0xbd, 0x8d, 0x7f, 0x78, 0x23, 0xde, 0x28, 0x22, 0xbe,
    0x52, 0x26, 0x9b, 0x2f, 0xfb, 0xde, 0xf7, 0x7c, 0x65, 0x60, 0x2a, 0x8a, 0x3c, 0x0e, 0x1e, 0x04,
    0xe0, 0x63, 0xe1, 0x4b, 0xe2, 0xde, 0x7f, 0xf9, 0xfb, 0xd9, 0xd6, 0xbe, 0xe6, 0x60, 0x69, 0x5c,
    0xde, 0xfe, 0xf7, 0xff, 0x50, 0x21, 0x15, 0x1c, 0xcd, 0x27, 0x49, 0x38, 0x01, 0x5a, 0x41, 0xdc,
    0xe0, 0x90, 0x48, 0x40, 0x5e, 0x46, 0xef, 0x1d, 0xff, 0x9e, 0x54, 0xd9, 0x17, 0x58, 0xda, 0x6f,
    0xf1, 0x51, 0xff, 0x0b, 0x73, 0xec, 0xbf, 0x13, 0xcb, 0xd9, 0xd7, 0xa9, 0xe2, 0x7e, 0x79, 0xcd,
    0x81, 0x13, 0x91, 0xd1, 0xaf, 0x96, 0xc1, 0xed, 0x0b, 0xb1, 0x29, 0xb6, 0x04, 0x67, 0x4e, 0xdb,
    0x81, 0x76, 0xc0, 0xee, 0x5b, 0xcf, 0xf3, 0xcf, 0x3a, 0x9e, 0x1e, 0xae, 0x7c, 0xca, 0x7d, 0x5c,
    0x64, 0xa3, 0x1b, 0xb3, 0xb4, 0xe4, 0xa9, 0xea, 0x79, 0xe7, 0xcc, 0xf7, 0x03, 0x71, 0xd0, 0x3b,
    0x73, 0x69, 0xd3, 0x04, 0x31, 0xef, 0x30, 0xcc, 0x0a, 0x2b, 0xfe, 0xe3, 0xc3, 0x36, 0xbf, 0x09,
    0x96, 0x5f, 0x9e, 0x9c, 0xe6, 0x67, 0xa8, 0x03, 0x8a, 0x8c, 0x81, 0xef, 0x25, 0x04, 0xc0, 0x9d,
    0xbc, 0xc7, 0x85, 0x27, 0x80, 0xad, 0x93, 0x76, 0x6c, 0x7b, 0xcd, 0x29, 0x3a, 0x32, 0x78, 0x35,
    0xc6, 0x46, 0xf3, 0x3e, 0x64, 0x66, 0x47, 0x34, 0xa9, 0x35, 0xa3, 0x4b, 0x38, 0xf9, 0x55, 0xb3,
    0xa0, 0xce, 0x99, 0x00, 0x45, 0x98, 0x8f, 0xb4, 0xd0, 0xa4, 0x43, 0xad, 0xc6, 0xe1, 0x31, 0x86,
    0xfb, 0xda, 0x57, 0x30, 0x3c, 0x2e, 0x59, 0xce, 0x76, 0x0a, 0x02, 0xfc, 0x20, 0x63, 0x7d, 0x69,
    0x80, 0xa7, 0x21, 0x35, 0x59, 0xa3, 0x1f, 0x7f, 0x51, 0xf3, 0xe0, 0x97, 0x1b, 0xb0, 0x1d, 0xf1,
    0x67, 0xda, 0x80, 0x85, 0xc5, 0x1f, 0xe0, 0x66, 0xe4, 0x8e, 0x48, 0xf7, 0xe7, 0xf7, 0x3d, 0x8d,
    0xdd, 0xee, 0xef, 0x2d, 0xf8, 0x17, 0x31, 0xac, 0x08, 0x55, 0x39, 0x8e, 0x18, 0xb9, 0x4b, 0x0e,
    0x52, 0x11, 0xd6, 0xb3, 0xb9, 0xb1, 0x91, 0xf1, 0x30, 0x2a, 0x54, 0xfb, 0x9d, 0xd5, 0xdf, 0xff,
    0x0b, 0x75, 0xc9, 0x2e, 0x6f, 0xb0, 0x83, 0xa4, 0xf8, 0xcd, 0x18, 0x4f, 0xee, 0xf1, 0xd3, 0xe7,
    0x5f, 0xff, 0xe5, 0xa9, 0xe7, 0xfc, 0xdf, 0x80, 0xaa, 0xa7, 0x45, 0x79, 0xac, 0xbf, 0xff, 0xc2,
    0x37, 0xbe, 0x57, 0xd1, 0x4c, 0xb5, 0xd3, 0xdb, 0x09, 0x6a, 0x71, 0xb5, 0xe3, 0x25, 0x0e, 0x86,
    0x3e, 0x32, 0x4f, 0x39, 0xa2, 0x6b, 0xfa, 0xeb, 0xc6, 0x4f, 0xa5, 0xbc, 0x41, 0x1f, 0x19, 0x2b,
    0xc6, 0x4f, 0xaf, 0xb9, 0xfe, 0x91, 0x0c, 0xf1, 0x71, 0xf1, 0x92, 0x4f, 0xb0, 0x6a, 0xee, 0xed,
    0xde, 0x12, 0xbb, 0xbb, 0x7c, 0x3a, 0x89, 0xf7, 0x4f, 0xfe, 0x53, 0xb8, 0x25, 0x79, 0x9d, 0x3f,
    0x05, 0x9c, 0x4f, 0xb8, 0x17, 0x66, 0xe2, 0xb5, 0xb8, 0xae, 0x38, 0xce, 0xae, 0x75, 0xcc, 0xc2,
    0x21, 0xdf, 0xe4, 0x0c, 0x05, 0xf8, 0x43, 0xf7, 0xac, 0x7f, 0x0d, 0xd2, 0x0e, 0xcf, 0x82, 0xc1,
    0x37, 0xe7, 0xa6, 0x79, 0xf5, 0xfc, 0x6a, 0x38, 0x46, 0x8a, 0x3b, 0x8f, 0xfe, 0xee, 0xef, 0xc6,
    0x7b, 0xc0, 0x85, 0xa7, 0x33, 0x61, 0x8e, 0xf3, 0xd8, 0xd2, 0x01, 0x9d, 0x28, 0xd0, 0xda, 0x3b,
    0xa0, 0xf7, 0xbb, 0xdf, 0xfa, 0x7c, 0x08, 0xc4, 0x51, 0xdc, 0x64, 0x22, 0x01, 0x0b, 0x5c, 0xc5,
    0xfe, 0xa5, 0x2f, 0x70, 0x7f, 0x60, 0x5e, 0x9e, 0xa6, 0x75, 0x71, 0x38, 0x76, 0x4c, 0xb2, 0xfb,
    0xb1, 0xb7, 0xbf, 0xf9, 0xea, 0x1f, 0x15, 0xe9, 0xd2, 0x0b, 0xde, 0x9d, 0x98, 0xdb, 0xfd, 0xf9,
    0xa5, 0x7f, 0xff, 0x17, 0xc5, 0xe1, 0xde, 0x14, 0x56, 0x2c, 0x30, 0x32, 0x1f, 0x91, 0x51, 0x96,
    0x9a, 0x7f, 0xff, 0x88, 0xdc, 0x74, 0x1d, 0xa5, 0xb1, 0x0d, 0x19, 0xda, 0x41, 0x3e, 0x45, 0xfd,
    0xb6, 0xff, 0xff, 0xe8, 0x2b, 0x8a, 0x96, 0xf3, 0x5f, 0xe9, 0x0f, 0xaa, 0x68, 0xf7, 0xf8, 0x99,
    0x04, 0x23, 0x52, 0xbd, 0x57, 0xa4, 0xa0, 0xff, 0x13, 0xaa, 0xfe, 0xfe, 0x35, 0x98, 0x26, 0x79,
    0x57, 0x96, 0x51, 0xfb, 0xcd, 0x2e, 0x8e, 0xf3, 0xcc, 0xf2, 0x3c, 0xf3, 0x3c, 0xef, 0xc3, 0xcb,
    0xb9, 0x6b, 0x1a, 0x3a, 0xa5, 0xdc, 0xdc, 0x7e, 0x58, 0x84, 0xac, 0x66, 0xce, 0xee, 0x27, 0xaf,
    0x20, 0x9e, 0x73, 0x3b, 0x84, 0x4f, 0xb1, 0x96, 0x0c, 0x34, 0xdc, 0xed, 0xfe, 0x95, 0xcb, 0xed,
    0x3e, 0x66, 0x55, 0xd7, 0x00, 0x00, 0x01, 0x65, 0x04, 0x42, 0x20, 0x00, 0x10, 0x00, 0x66, 0x20,
    0xef, 0xb8, 0x38, 0x4b, 0xd4, 0x06, 0x83, 0xd0, 0x29, 0xee, 0xaf, 0x93, 0x88, 0x16, 0x9f, 0x51,
    0x8c, 0x96, 0x93, 0x56, 0xc3, 0x4a, 0xda, 0x80, 0x44, 0xe3, 0x13, 0xa3, 0x4c, 0xeb, 0x9d, 0x71,
    0xdd, 0xdf, 0x7f, 0xff, 0x09, 0x50, 0x8e, 0xe5, 0x7d, 0xb3, 0xfe, 0xf8, 0x43, 0x0a, 0x2b, 0xff,
    0xfc, 0xdb, 0xfb, 0xbc, 0x89, 0x93, 0xfd, 0x52, 0x01, 0x89, 0x10, 0x78, 0x80, 0xdc, 0xe8, 0x4e,
    0x66, 0xb8, 0xf3, 0xe2, 0xbe, 0x41, 0x73, 0x97, 0x97, 0x8a, 0x35, 0x00, 0x7a, 0x64, 0xf7, 0xfb,
    0x89, 0x93, 0xbf, 0x82, 0xbb, 0xbe, 0xbc, 0x88, 0x78, 0xae, 0xe2, 0x0a, 0xbe, 0xce, 0x94, 0xb4,
    0x5c, 0x57, 0xee, 0x52, 0x28, 0x8a, 0xc4, 0x77, 0x1f, 0x2d, 0x73, 0xd8, 0xb5, 0x55, 0x49, 0x7d,
    0xea, 0xbc, 0x58, 0x88, 0x28, 0xfb, 0x79, 0xe2, 0xb1, 0x0f, 0xcf, 0xf4, 0xaf, 0xc9, 0x62, 0x1e,
    0xff, 0x3f, 0x45, 0xdd, 0x9a, 0xf5, 0x2f, 0xe9, 0x2d, 0x75, 0x9b, 0x2b, 0x12, 0x96, 0x2e, 0xdf,
    0xb5, 0x3d, 0xde, 0x10, 0x8d, 0xd1, 0x7f, 0x2f, 0xcf, 0x3f, 0x99, 0x9a, 0xcf, 0xf3, 0x78, 0x09,
    0x18, 0xa0, 0x79, 0x04, 0x69, 0x64, 0x3a, 0x31, 0x93, 0x73, 0x2c, 0xdc, 0x7d, 0x27, 0x62, 0xd9,
    0xd1, 0xf1, 0xdf, 0xf9, 0xe7, 0xfa, 0xd7, 0x00, 0xb2, 0x86, 0xc2, 0x9e, 0xf1, 0x10, 0x64, 0xed,
    0xfe, 0x71, 0x0e, 0xfd, 0x92, 0x7c, 0xe4, 0x4f, 0x33, 0xe8, 0x79, 0x9a, 0x75, 0x8b, 0xb0, 0xad,
    0xb9, 0x74, 0xc8, 0xe6, 0x09, 0x24, 0x3e, 0x98, 0xd1, 0x5d, 0x63, 0x8f, 0xe8, 0x0e, 0x5c, 0xe0,
    0x82, 0xf6, 0x0e, 0x98, 0x43, 0xe1, 0x07, 0x77, 0x77, 0x86, 0x69, 0x80, 0xaa, 0x28, 0xb4, 0x02,
    0x18, 0x8f, 0x96, 0x77, 0xc9, 0x3d, 0x8a, 0x0c, 0xb0, 0x78, 0xbe, 0xe8, 0xce, 0x97, 0x7d, 0xf7,
    0x72, 0x21, 0x57, 0xe9, 0xf0, 0x46, 0x67, 0x42, 0x73, 0x57, 0x74, 0x31, 0x90, 0x77, 0x70, 0xec,
    0x74, 0x3c, 0xaa, 0xfa, 0xc6, 0x0d, 0x64, 0xd9, 0xbc, 0xe7, 0x3b, 0xb8, 0xbb, 0xbf, 0xc3, 0xa0,
    0x94, 0x06, 0xa2, 0x8a, 0xab, 0x98, 0xfe, 0xa1, 0xa8, 0x63, 0xad, 0x3f, 0xe9, 0x56, 0x06, 0xfa,
    0xe7, 0x22, 0xbe, 0x98, 0x8e, 0xe2, 0x45, 0x0f, 0x5d, 0x0b, 0x7e, 0x8f, 0x15, 0x78, 0x79, 0xcf,
    0xad, 0x5a, 0xcb, 0x2f, 0x7f, 0x35, 0x00, 0x5b, 0x5e, 0x20, 0x78, 0x87, 0x3e, 0x9d, 0x58, 0xcf,
    0x3f, 0x37, 0x97, 0x59, 0xa8, 0x60, 0xf7, 0x6b, 0x95, 0xdd, 0x20, 0xa2, 0xb7, 0x9b, 0x1b, 0x7d,
    0x3a, 0xb5, 0xf8, 0x5f, 0xd2, 0x86, 0x0b, 0xbc, 0xb9, 0x10, 0xf2, 0xe4, 0xe7, 0x03, 0xac, 0xae,
    0xb7, 0xda, 0xf3, 0xaa, 0x07, 0x83, 0x96, 0xa7, 0x54, 0x97, 0x8f, 0x17, 0xc9, 0xe1, 0xb2, 0x37,
    0x67, 0xe0, 0x59, 0xb6, 0xa9, 0x4e, 0x54, 0xbf, 0x81, 0xc3, 0xfd, 0x84, 0xab, 0xe1, 0x5f, 0x3a,
    0x97, 0xcc, 0xf9, 0xc5, 0x79, 0x9e, 0xfe, 0x32, 0x30, 0x83, 0x36, 0x57, 0x02, 0x98, 0xcc, 0xa9,
    0xc3, 0x4f, 0x6f, 0x81, 0x0e, 0xe4, 0x53, 0x34, 0x0f, 0x1c, 0xf3, 0x3c, 0xfe, 0x32, 0x18, 0xb2,
    0x2a, 0x1d, 0x9a, 0x81, 0x93, 0xbf, 0x1c, 0xcc, 0xc1, 0xc2, 0x2d, 0x84, 0x3b, 0x1c, 0xd6, 0xcb,
    0x61, 0x98, 0x39, 0xb6, 0x16, 0xa9, 0xf3, 0x2f, 0xf5, 0xf9, 0xcf, 0x3e, 0x87, 0x62, 0x65, 0xbb,
    0xf4, 0xc6, 0x7a, 0x89, 0xb9, 0xad, 0x75, 0x55, 0xa8, 0x56, 0x89, 0xd5, 0x62, 0xfb, 0xda, 0x55,
    0x5b, 0x4f, 0xd7, 0x40, 0xfa, 0xc6, 0x87, 0x11, 0x83, 0xee, 0x0f, 0xfe, 0xf7, 0x70, 0xaf, 0x2d,
    0xd9, 0xe4, 0x01, 0xee, 0x16, 0xba, 0x7e, 0xdf, 0xff, 0x8c, 0x2c, 0x52, 0xcd, 0x56, 0xf5, 0x7b,
    0xf8, 0x9f, 0xb5, 0x85, 0x15, 0x6e, 0x76, 0xe6, 0x2c, 0xd5, 0x35, 0x3d, 0xee, 0xb5, 0x03, 0xac,
    0x25, 0xdb, 0xab, 0xbe, 0x32, 0xe2, 0x4f, 0xd6, 0xc5, 0xe5, 0x52, 0x29, 0x08, 0x41, 0x67, 0x43,
    0xed, 0x6f, 0x5d, 0x12, 0xb5, 0xb1, 0xbe, 0xfd, 0xb5, 0xe7, 0x53, 0xce, 0x07, 0x15, 0x35, 0x58,
    0x27, 0x97, 0x1d, 0x10, 0x05, 0x9a, 0x9e, 0xc0, 0x4b, 0xe7, 0xd3, 0x61, 0xb0, 0xa1, 0xeb, 0xe7,
    0x59, 0x14, 0x2b, 0xb8, 0x68, 0x6a, 0x5d, 0xdf, 0xc0, 0xcc, 0x14, 0xe1, 0x9d, 0x71, 0x53, 0x8a,
    0x72, 0x31, 0xaf, 0x36, 0x57, 0x78, 0x53, 0x19, 0x00, 0x00, 0x01, 0x65, 0x02, 0x18, 0x88, 0x00,
    0x04, 0x00, 0x19, 0xc3, 0x90, 0x84, 0x25, 0xe5, 0x81, 0xa0, 0x82, 0xe2, 0x67, 0x3f, 0x10, 0xfd,
    0xbc, 0x43, 0xed, 0x8e, 0xd3, 0xe0, 0xa4, 0x68, 0xbd, 0xe8, 0x82, 0x28, 0x75, 0xf4, 0xc4, 0xc3,
    0x43, 0xce, 0x15, 0xfb, 0x14, 0x7f, 0x78, 0x89, 0xf4, 0x1b, 0x74, 0xea, 0xef, 0x80, 0xc6, 0x72,
    0x58, 0x17, 0x57, 0x47, 0xef, 0xe9, 0xbc, 0x38, 0x7a, 0xe0, 0xd5, 0x4a, 0x15, 0x9c, 0xf1, 0xdf,
    0xe2, 0x1c, 0xff, 0x78, 0x8f, 0xc1, 0xb1, 0x55, 0xae, 0x1c, 0x07, 0x96, 0xf9, 0xc2, 0xca, 0x8f,
    0xec, 0xb5, 0x06, 0x5f, 0xe2, 0xe4, 0xbe, 0x42, 0xe5, 0x73, 0x99, 0x27, 0xbd, 0xb8, 0x8d, 0xa8,
    0xbf, 0xd7, 0xbb, 0xfb, 0xea, 0xe9, 0x0b, 0xb8, 0x34, 0xfe, 0x7e, 0x1e, 0xd0, 0x55, 0xbe, 0xf2,
    0x34, 0xb1, 0xbb, 0x9f, 0xce, 0x9c, 0x21, 0x36, 0x1f, 0x5d, 0x4c, 0xc8, 0xa0, 0x79, 0x7d, 0xf7,
    0xd0, 0x40, 0xc7, 0xe2, 0x13, 0x1f, 0xf2, 0x6a, 0x7a, 0x69, 0xb6, 0xcf, 0xcf, 0x33, 0xdd, 0x4d,
    0x63, 0x13, 0x0c, 0x6a, 0x4d, 0x09, 0x4d, 0xb2, 0xfa, 0x4b, 0xde, 0x16, 0x3e, 0xad, 0xed, 0x84,
    0xaa, 0xaf, 0xff, 0x03, 0xcf, 0x3c, 0x46, 0x34, 0x99, 0xe3, 0x32, 0xb5, 0x12, 0x83, 0x5a, 0x6c,
    0xde, 0xef, 0x09, 0x62, 0x56, 0x3f, 0x35, 0x3f, 0xed, 0x7f, 0xf4, 0x9f, 0x84, 0x70, 0xfe, 0xbf,
    0x4d, 0xf4, 0x9a, 0x7c, 0x16, 0x6d, 0xd6, 0xff, 0x96, 0x1c, 0xf4, 0xb1, 0xd2, 0x07, 0xfd, 0x4e,
    0x2d, 0x69, 0xb6, 0xad, 0xf6, 0x04, 0x4d, 0xc2, 0xfa, 0xb7, 0x5e, 0xde, 0x13, 0x17, 0x67, 0x64,
    0x81, 0x04, 0xb4, 0x73, 0x79, 0xa6, 0xdf, 0x43, 0x67, 0x5f, 0x54, 0xa8, 0xf1, 0x74, 0xf7, 0xb6,
    0x8d, 0x9b, 0xfa, 0x8e, 0xe1, 0xf6, 0xe9, 0xfb, 0xb0, 0x3c, 0x79, 0x48, 0x9a, 0xed, 0x6b, 0xe2,
    0x07, 0x6a, 0xf1, 0xcc, 0x36, 0xdf, 0xfa, 0xea, 0xec, 0x27, 0xb8, 0xca, 0xef, 0x51, 0x55, 0x95,
    0x8f, 0xcc, 0x4f, 0xff, 0x4f, 0xff, 0x36, 0x9f, 0xfc, 0x11, 0xfb, 0xf7, 0x35, 0xa9, 0xf9, 0x7e,
    0xf7, 0xe7, 0x05, 0x37, 0xb9, 0x23, 0x86, 0xfa, 0xef, 0x1d, 0xd7, 0xfe, 0xbf, 0x6e, 0x9e, 0xdf,
    0x1e, 0x3b, 0xbc, 0x2b, 0xdc, 0xb9, 0xbf, 0x3d, 0x29, 0x27, 0x91, 0xfa, 0xab, 0xf7, 0x8d, 0x66,
    0x3b, 0xe7, 0xcd, 0xb9, 0xae, 0x16, 0xc5, 0xd1, 0x4e, 0x60, 0x06, 0x08, 0x79, 0xeb, 0x0f, 0xf9,
    0x61, 0x2d, 0x37, 0xe2, 0x3d, 0x11, 0xbf, 0xc5, 0xbe, 0x5b, 0xbe, 0xc7, 0xe2, 0xdb, 0xe8, 0x78,
    0xef, 0x6b, 0x1d, 0x46, 0x9d, 0x17, 0xc2, 0xfe, 0xd0, 0x51, 0xfc, 0x78, 0x7f, 0x67, 0x50, 0xfe,
    0x68, 0x45, 0xab, 0x3f, 0x97, 0xda, 0x48, 0x5f, 0x11, 0xc5, 0xd6, 0x7a, 0x27, 0x25, 0x35, 0xda,
    0xb7, 0x15, 0xf2, 0x56, 0x35, 0x56, 0xeb, 0x31, 0x1a, 0x1b, 0x7e, 0xa0, 0xb0, 0xdc, 0xee, 0x44,
    0xb8, 0xfb, 0xaf, 0x5a, 0x24, 0x46, 0x7a, 0x84, 0xf1, 0xd7, 0x88, 0xf8, 0x4b, 0xfa, 0x73, 0xfe,
    0x24, 0x29, 0x2d, 0x2b, 0x1d, 0x00, 0x00, 0x01, 0x41, 0x9a, 0x00, 0x02, 0x05, 0x83, 0x4d, 0xe5,
    0x99, 0xff, 0x98, 0xd8, 0xfc, 0x79, 0x7e, 0xda, 0xf5, 0xd2, 0xbe, 0x9f, 0xed, 0x49, 0x27, 0xdc,
    0x95, 0xa4, 0xf1, 0x5b, 0x2b, 0x8d, 0xe3, 0x79, 0x6d, 0x5e, 0xab, 0xbf, 0x77, 0xf2, 0x13, 0x00,
    0x00, 0x01, 0x41, 0x04, 0x46, 0x80, 0x00, 0x81, 0x60, 0x5e, 0xc8, 0x57, 0x97, 0x62, 0xbc, 0x22,
    0x23, 0x8b, 0xf9, 0xd5, 0x68, 0x43, 0x97, 0xe2, 0x27, 0x5d, 0xd2, 0xc2, 0xf1, 0x0d, 0xdb, 0x7e,
    0x51, 0x32, 0x31, 0x87, 0x95, 0xe9, 0xe1, 0xf7, 0x59, 0xb8, 0x92, 0x6c, 0x7b, 0x00, 0x00, 0x01,
    0x41, 0x9a, 0x00, 0x04, 0x09, 0x41, 0xa5, 0xdf, 0xf2, 0xa2, 0xaa, 0xde, 0xde, 0xf2, 0x65, 0x57,
    0xb9, 0x2b, 0x9a, 0x0d, 0x18, 0x8f, 0x7a, 0x35, 0xaf, 0xfa, 0xa5, 0xa8, 0xaf, 0x06, 0x7e, 0xdc,
    0x72, 0x7b, 0xda, 0xac, 0xdf, 0x54, 0x8b, 0x27, 0xb3, 0xb2, 0x99, 0xf9, 0xf9, 0x98, 0xdb, 0x57,
    0xbd, 0xc8, 0xe4, 0x47, 0x67, 0x18, 0xa1, 0xeb, 0x5d, 0x65, 0xea, 0x5d, 0x72, 0x30, 0xa5, 0xdf,
    0x7b, 0xde, 0x4a, 0xd8, 0x34, 0xeb, 0x17, 0xa8, 0x2c, 0xf1, 0xa9, 0xef, 0x7d, 0x63, 0xa0, 0x94,
    0xc5, 0x54, 0xa2, 0xd5, 0x70, 0xe8, 0xd2, 0x02, 0x8c, 0x8a, 0xbf, 0xc7, 0xe8, 0x33, 0xf1, 0x32,
    0x09, 0xa8, 0x85, 0xce, 0xe5, 0xa2, 0xec, 0xc5, 0x79, 0x5e, 0xb4, 0x94, 0x5f, 0x0d, 0x00, 0x01,
    0x41, 0x04, 0x22, 0x46, 0xca, 0x92, 0x07, 0xf8, 0x19, 0xf3, 0x3b, 0x82, 0xde, 0x7f, 0x02, 0xf8,
    0xbe, 0x6b, 0x1b, 0x68, 0x61, 0x26, 0xe3, 0x4d, 0xc8, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0d,
    0x8c, 0x8d, 0x3c, 0x28, 0x32, 0x20, 0x00, 0x00, 0x01, 0x41, 0x02, 0x19, 0xa0, 0x02, 0x04, 0x12,
    0x81,
  };
  const int32_t kiLen = (int32_t) sizeof (kBs);

  SDecodingParam sParam;
  memset (&sParam, 0, sizeof (sParam));
  sParam.uiTargetDqLayer = 0xff;
  sParam.eEcActiveIdc = ERROR_CON_DISABLE;
  sParam.bParseOnly = true;
  sParam.sVideoProperty.size = sizeof (SVideoProperty);
  sParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
  ASSERT_EQ (0, m_pDec->Initialize (&sParam));

  int32_t iStarts[64];
  int32_t iNum = 0;
  for (int32_t i = 0; i + 3 <= kiLen && iNum < 63; ++i) {
    if (kBs[i] == 0 && kBs[i + 1] == 0 && kBs[i + 2] == 1) {
      iStarts[iNum++] = (i > 0 && kBs[i - 1] == 0) ? i - 1 : i;
      i += 2;
    }
  }
  iStarts[iNum] = kiLen;

  SParserBsInfo sInfo;
  for (int32_t k = 0; k < iNum; ++k) {
    const int32_t iNalLen = iStarts[k + 1] - iStarts[k];
    memset (&sInfo, 0, sizeof (sInfo));
    m_pDec->DecodeParser (kBs + iStarts[k], iNalLen, &sInfo);
    memset (&sInfo, 0, sizeof (sInfo));
    m_pDec->DecodeParser (NULL, 0, &sInfo);
  }
  // Reaching here without an ASan/UBSan abort is the regression check.
  m_pDec->Uninitialize();
}

//TEST here for whole tests
TEST_F (DecoderParseSyntaxTest, DecoderParseSyntaxTestAll) {

  TestScalingList();
  TestSpecificBs();
  TestSpecificBsError();
  TestIPcmCavlcRegression();
  TestIPcmCavlcTruncated();
}

TEST (DecoderFmoSecurityTest, RejectsOversizedRunLengthBeforeIndexWrap) {
  SFmo sFmo;
  SPps sPps;
  memset (&sFmo, 0, sizeof (sFmo));
  memset (&sPps, 0, sizeof (sPps));

  sPps.uiNumSliceGroups = 2;
  sPps.uiSliceGroupMapType = 0;
  sPps.uiRunLength[0] = 0xffffffffu;
  sPps.uiRunLength[1] = 1;

  CMemoryAlign cMa (16);
  const int32_t iRet = InitFmo (&sFmo, &sPps, 120, 68, &cMa);
  EXPECT_NE (ERR_NONE, iRet);

  // The rejection path must also free the allocation map it allocated, otherwise
  // the FMO is left allocated-but-inactive and leaks (CMemoryAlign asserts on
  // teardown). A non-NULL map here means the leak regressed.
  EXPECT_TRUE (NULL == sFmo.pMbAllocMap);

  if (NULL != sFmo.pMbAllocMap) {
    cMa.WelsFree (sFmo.pMbAllocMap, "_fmo->pMbAllocMap");
    sFmo.pMbAllocMap = NULL;
  }
}

TEST (DecoderReorderingBufferTest, PartialResetInitializesPicBuffIdx) {
  SPictReoderingStatus sStatus;
  SPictInfo sPictInfo[16];

  memset (&sStatus, 0, sizeof (sStatus));
  memset (&sPictInfo, 0, sizeof (sPictInfo));

  for (int32_t i = 0; i < 16; ++i) {
    sPictInfo[i].iPOC = i + 100;
    sPictInfo[i].iPicBuffIdx = i + 200;
  }
  sPictInfo[0].sBufferInfo.iBufferStatus = 1;
  sStatus.iLargestBufferedPicIndex = 3;

  ResetReorderingPictureBuffers (&sStatus, sPictInfo, false);

  for (int32_t i = 0; i <= 3; ++i) {
    EXPECT_EQ (IMinInt32, sPictInfo[i].iPOC);
    EXPECT_EQ (-1, sPictInfo[i].iPicBuffIdx);
  }

  // Partial reset should not touch entries beyond iLargestBufferedPicIndex.
  EXPECT_EQ (104, sPictInfo[4].iPOC);
  EXPECT_EQ (204, sPictInfo[4].iPicBuffIdx);
  EXPECT_EQ (0, sPictInfo[0].sBufferInfo.iBufferStatus);
  EXPECT_FALSE (sStatus.bHasBSlice);
  EXPECT_EQ (0, sStatus.iNumOfPicts);
  EXPECT_EQ (0, sStatus.iLargestBufferedPicIndex);
}

TEST (DecoderReorderingBufferTest, FullResetInitializesPicBuffIdx) {
  SPictReoderingStatus sStatus;
  SPictInfo sPictInfo[16];

  memset (&sStatus, 0, sizeof (sStatus));
  memset (&sPictInfo, 0, sizeof (sPictInfo));

  for (int32_t i = 0; i < 16; ++i) {
    sPictInfo[i].iPOC = i + 300;
    sPictInfo[i].iPicBuffIdx = i + 400;
  }
  sStatus.iLargestBufferedPicIndex = 1;

  ResetReorderingPictureBuffers (&sStatus, sPictInfo, true);

  for (int32_t i = 0; i < 16; ++i) {
    EXPECT_EQ (IMinInt32, sPictInfo[i].iPOC);
    EXPECT_EQ (-1, sPictInfo[i].iPicBuffIdx);
  }
  EXPECT_FALSE (sStatus.bHasBSlice);
  EXPECT_EQ (0, sStatus.iLargestBufferedPicIndex);
}

TEST_F (DecoderParseSyntaxTest, ExpandBsBufferRetargetsQueuedNalUnitsOnly) {
  ASSERT_EQ (ERR_NONE, Init());
  ASSERT_TRUE (m_pCtx != NULL);
  ASSERT_TRUE (m_pCtx->pAccessUnitList != NULL);

  PAccessUnit pAu = m_pCtx->pAccessUnitList;
  ASSERT_TRUE (pAu->uiCountUnitsNum >= 3);
  ASSERT_TRUE (m_pCtx->sRawData.pHead != NULL);

  uint8_t* pOldHead = m_pCtx->sRawData.pHead;
  PBitStringAux pActual = &pAu->pNalUnitsList[0]->sNalData.sVclNal.sSliceBitsRead;
  pActual->pStartBuf = pOldHead + 8;
  pActual->pCurBuf = pOldHead + 12;
  pActual->pEndBuf = pOldHead + 16;

  PBitStringAux pQueued = &pAu->pNalUnitsList[1]->sNalData.sVclNal.sSliceBitsRead;
  pQueued->pStartBuf = pOldHead + 24;
  pQueued->pCurBuf = pOldHead + 28;
  pQueued->pEndBuf = pOldHead + 32;

  PBitStringAux pOnePastAvail = &pAu->pNalUnitsList[2]->sNalData.sVclNal.sSliceBitsRead;
  pOnePastAvail->pStartBuf = pOldHead + 40;
  pOnePastAvail->pCurBuf = pOldHead + 44;
  pOnePastAvail->pEndBuf = pOldHead + 48;
  uint8_t* pOnePastStartBefore = pOnePastAvail->pStartBuf;
  uint8_t* pOnePastCurBefore = pOnePastAvail->pCurBuf;
  uint8_t* pOnePastEndBefore = pOnePastAvail->pEndBuf;

  pAu->uiAvailUnitsNum = 2;
  pAu->uiActualUnitsNum = 1; // queued count is still 2, so index 1 must be retargeted

  ASSERT_EQ (ERR_NONE, ExpandBsBuffer (m_pCtx, m_pCtx->iMaxBsBufferSizeInByte));
  ASSERT_TRUE (m_pCtx->sRawData.pHead != pOldHead);

  EXPECT_EQ (m_pCtx->sRawData.pHead + 8, pActual->pStartBuf);
  EXPECT_EQ (m_pCtx->sRawData.pHead + 12, pActual->pCurBuf);
  EXPECT_EQ (m_pCtx->sRawData.pHead + 16, pActual->pEndBuf);

  EXPECT_EQ (m_pCtx->sRawData.pHead + 24, pQueued->pStartBuf);
  EXPECT_EQ (m_pCtx->sRawData.pHead + 28, pQueued->pCurBuf);
  EXPECT_EQ (m_pCtx->sRawData.pHead + 32, pQueued->pEndBuf);

  // Slot at index uiAvailUnitsNum (one-past queued range) must not be touched.
  EXPECT_EQ (pOnePastStartBefore, pOnePastAvail->pStartBuf);
  EXPECT_EQ (pOnePastCurBefore, pOnePastAvail->pCurBuf);
  EXPECT_EQ (pOnePastEndBefore, pOnePastAvail->pEndBuf);

  Uninit();
}

TEST (DecoderBitStreamBoundsTest, DecInitBitsHandlesShortSeedBytesSafely) {
  uint8_t uiBuf[3] = {0xff, 0xff, 0xff};
  SBitStringAux sBs;
  memset (&sBs, 0, sizeof (sBs));

  ASSERT_EQ (ERR_NONE, DecInitBits (&sBs, uiBuf, 24));

  uint32_t uiCode = 0;
  EXPECT_EQ (ERR_NONE, BsGetBits (&sBs, 16, &uiCode));
  EXPECT_EQ (ERR_NONE, BsGetBits (&sBs, 16, &uiCode));
  EXPECT_EQ (ERR_INFO_READ_OVERFLOW, BsGetBits (&sBs, 16, &uiCode));
}

TEST (DecoderBitStreamBoundsTest, BsGetBitsStopsOnTwoByteOverread) {
  uint8_t uiBuf[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  SBitStringAux sBs;
  memset (&sBs, 0, sizeof (sBs));

  ASSERT_EQ (ERR_NONE, DecInitBits (&sBs, uiBuf, 48));

  uint32_t uiCode = 0;
  EXPECT_EQ (ERR_NONE, BsGetBits (&sBs, 16, &uiCode));
  EXPECT_EQ (ERR_NONE, BsGetBits (&sBs, 16, &uiCode));
  EXPECT_EQ (ERR_NONE, BsGetBits (&sBs, 16, &uiCode));
  EXPECT_EQ (ERR_INFO_READ_OVERFLOW, BsGetBits (&sBs, 16, &uiCode));
}

// Regression: WelsDecodeBs must not wrap sRawData to pHead when queued VCL
// NALs have sSliceBitsRead.pStartBuf pointing into the region that would be
// overwritten by the incoming write.  Pre-fix, the wrap was unconditional and
// silently replaced queued slice bytes with the new input bytes while leaving
// the saved bit-reader pointer unchanged.
TEST_F (DecoderParseSyntaxTest, WelsDecodeBsRejectsWrapIntoQueuedSlice) {
  ASSERT_EQ (ERR_NONE, Init());
  ASSERT_TRUE (m_pCtx != NULL);
  ASSERT_TRUE (m_pCtx->pAccessUnitList != NULL);
  ASSERT_TRUE (m_pCtx->sRawData.pHead != NULL);

  PAccessUnit pAu = m_pCtx->pAccessUnitList;
  const int32_t kiBufSize = m_pCtx->iMaxBsBufferSizeInByte;

  // Place a synthetic queued VCL slice bit-reader at offset 8 from pHead,
  // within the region that a full-buffer wrap would overwrite.
  PBitStringAux pBs = &pAu->pNalUnitsList[0]->sNalData.sVclNal.sSliceBitsRead;
  pBs->pStartBuf = m_pCtx->sRawData.pHead + 8;
  pBs->pCurBuf   = m_pCtx->sRawData.pHead + 8;
  pBs->pEndBuf   = m_pCtx->sRawData.pHead + kiBufSize / 4;
  pAu->uiAvailUnitsNum = 1;

  // Position write cursor near pEnd so any new input forces a wrap.
  m_pCtx->sRawData.pCurPos = m_pCtx->sRawData.pEnd - 3;

  // Remember the bytes the queued slice points to before the wrap attempt.
  uint8_t snapshot[8];
  memcpy (snapshot, pBs->pStartBuf, sizeof (snapshot));
  const uint8_t* pStartBefore = pBs->pStartBuf;

  // Input large enough to trigger wrap check (kiBsLen + 4 > pEnd - pCurPos).
  static const uint8_t kFiller[16] = {0, 0, 0, 1, 0x0c, 0x80, 0, 0, 0, 1, 0x0c, 0x80, 0, 0, 0, 1};
  uint8_t* dst[3] = {NULL, NULL, NULL};
  SBufferInfo dstInfo;
  memset (&dstInfo, 0, sizeof (dstInfo));
  m_pCtx->bEndOfStreamFlag = false;
  WelsDecodeBs (m_pCtx, kFiller, static_cast<int32_t> (sizeof (kFiller)), dst, &dstInfo, NULL);

  // The pointer must not have moved (no retarget happened).
  EXPECT_EQ (pStartBefore, pBs->pStartBuf);
  // The bytes at the queued slice start must be unchanged.
  EXPECT_EQ (0, memcmp (snapshot, pBs->pStartBuf, sizeof (snapshot)));

  Uninit();
}

