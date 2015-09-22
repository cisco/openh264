#include <gtest/gtest.h>
#include "codec_def.h"
#include "utils/BufferedData.h"
#include "utils/FileInputStream.h"
#include "BaseDecoderTest.h"
#include "BaseEncoderTest.h"
#include "wels_common_defs.h"
#include "utils/HashFunctions.h"
#include <string>
#include <vector>
using namespace WelsCommon;

#define TRY_TIME_RANGE           (10)
#define ENCODE_FRAME_NUM         (30)
#define LEVEL_ID_RANGE           (18)
#define MAX_WIDTH                (4096)
#define MAX_HEIGHT               (2304)
#define MAX_FRAME_RATE           (30)
#define MIN_FRAME_RATE           (1)
#define FRAME_RATE_RANGE         (2*MAX_FRAME_RATE)
#define RC_MODE_RANGE            (4)
#define BIT_RATE_RANGE           (10000)
#define MAX_QP                   (51)
#define MIN_QP                   (0)
#define QP_RANGE                 (2*MAX_QP)
#define SPATIAL_LAYER_NUM_RANGE  (2*MAX_SPATIAL_LAYER_NUM)
#define TEMPORAL_LAYER_NUM_RANGE (2*MAX_TEMPORAL_LAYER_NUM)
#define SAVED_NALUNIT_NUM        ( (MAX_SPATIAL_LAYER_NUM*MAX_QUALITY_LAYER_NUM) + 1 + MAX_SPATIAL_LAYER_NUM )
#define MAX_SLICES_NUM           ( ( MAX_NAL_UNITS_IN_LAYER - SAVED_NALUNIT_NUM ) / 3 )
#define SLICE_MODE_NUM           (6)
#define LOOP_FILTER_IDC_NUM      (3)
#define LOOF_FILTER_OFFSET_RANGE (6)
#define MAX_REF_PIC_COUNT        (16)
#define MIN_REF_PIC_COUNT        (1)
#define LONG_TERM_REF_NUM        (2)
#define LONG_TERM_REF_NUM_SCREEN (4)
#define MAX_REFERENCE_PICTURE_COUNT_NUM_CAMERA (6)
#define MAX_REFERENCE_PICTURE_COUNT_NUM_SCREEN (8)
#define VALID_SIZE(iSize) (((iSize)>16)?(iSize):16)
#define GET_MB_WIDTH(x) (((x) + 15)/16)

typedef struct SLost_Sim {
  WelsCommon::EWelsNalUnitType eNalType;
  bool isLost;
} SLostSim;


struct EncodeDecodeFileParamBase {
  int numframes;
  int width;
  int height;
  float frameRate;
  int slicenum;
  bool bLostPara;
  const char* pLossSequence;
};

static void welsStderrTraceOrigin (void* ctx, int level, const char* string) {
  fprintf (stderr, "%s\n", string);
}

typedef struct STrace_Unit {
  int iTarLevel;
} STraceUnit;

static void TestOutPutTrace (void* ctx, int level, const char* string) {
  STraceUnit* pTraceUnit = (STraceUnit*) ctx;
  EXPECT_LE (level, pTraceUnit->iTarLevel);
}

class EncodeDecodeTestBase : public BaseEncoderTest, public BaseDecoderTest {
 public:
  uint8_t iRandValue;
 public:
  virtual void SetUp() {
    BaseEncoderTest::SetUp();
    BaseDecoderTest::SetUp();
    pFunc = welsStderrTraceOrigin;
    pTraceInfo = NULL;
    encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK, &pFunc);
    encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);
    decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK, &pFunc);
    decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);
  }

  virtual void TearDown() {
    BaseEncoderTest::TearDown();
    BaseDecoderTest::TearDown();
  }

  virtual void prepareParam (int iLayers, int iSlices, int width, int height, float framerate, SEncParamExt* pParam) {
    pParam->iUsageType = CAMERA_VIDEO_REAL_TIME;
    pParam->iPicWidth = width;
    pParam->iPicHeight = height;
    pParam->fMaxFrameRate = framerate;
    pParam->iRCMode = RC_OFF_MODE; //rc off
    pParam->iMultipleThreadIdc = 1; //single thread
    pParam->iSpatialLayerNum = iLayers;
    pParam->iNumRefFrame = AUTO_REF_PIC_COUNT;
    for (int i = 0; i < iLayers; i++) {
      pParam->sSpatialLayers[i].iVideoWidth = width >> (iLayers - i - 1);
      pParam->sSpatialLayers[i].iVideoHeight = height >> (iLayers - i - 1);
      pParam->sSpatialLayers[i].fFrameRate = framerate;
      pParam->sSpatialLayers[i].sSliceCfg.uiSliceMode = SM_FIXEDSLCNUM_SLICE;
      pParam->sSpatialLayers[i].sSliceCfg.sSliceArgument.uiSliceNum = iSlices;
    }
  }

  virtual void prepareEncDecParam (const EncodeDecodeFileParamBase EncDecFileParam) {
    //for encoder
    //I420: 1(Y) + 1/4(U) + 1/4(V)
    int frameSize = EncDecFileParam.width * EncDecFileParam.height * 3 / 2;
    buf_.SetLength (frameSize);
    ASSERT_TRUE (buf_.Length() == (size_t)frameSize);
    memset (&EncPic, 0, sizeof (SSourcePicture));
    EncPic.iPicWidth = EncDecFileParam.width;
    EncPic.iPicHeight = EncDecFileParam.height;
    EncPic.iColorFormat = videoFormatI420;
    EncPic.iStride[0] = EncPic.iPicWidth;
    EncPic.iStride[1] = EncPic.iStride[2] = EncPic.iPicWidth >> 1;
    EncPic.pData[0] = buf_.data();
    EncPic.pData[1] = EncPic.pData[0] + EncDecFileParam.width * EncDecFileParam.height;
    EncPic.pData[2] = EncPic.pData[1] + (EncDecFileParam.width * EncDecFileParam.height >> 2);
    //for decoder
    memset (&info, 0, sizeof (SFrameBSInfo));
    //set a fixed random value
    iRandValue = rand() % 256;
  }

  virtual void encToDecData (const SFrameBSInfo& info, int& len) {
    len = 0;
    for (int i = 0; i < info.iLayerNum; ++i) {
      const SLayerBSInfo& layerInfo = info.sLayerInfo[i];
      for (int j = 0; j < layerInfo.iNalCount; ++j) {
        len += layerInfo.pNalLengthInByte[j];
      }
    }
  }

  virtual void encToDecSliceData (const int iLayerNum, const int iSliceNum, const SFrameBSInfo& info, int& len) {
    ASSERT_TRUE (iLayerNum < MAX_LAYER_NUM_OF_FRAME);
    len = 0;
    const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayerNum];
    if (iSliceNum < layerInfo.iNalCount)
      len = layerInfo.pNalLengthInByte[iSliceNum];
  }


  virtual int GetRandWidth() {
    return WelsClip3 ((((rand() % MAX_WIDTH) >> 1) + 1) << 1, 16, MAX_WIDTH);
  }

  virtual int GetRandHeight() {
    return WelsClip3 ((((rand() % MAX_HEIGHT) >> 1) + 1) << 1, 16, MAX_HEIGHT);
  }

 protected:
  SEncParamExt   param_;
  BufferedData   buf_;
  SSourcePicture EncPic;
  SFrameBSInfo   info;
  SBufferInfo    dstBufInfo_;
  std::vector<SLostSim> m_SLostSim;
  WelsTraceCallback pFunc;
  STraceUnit sTrace;
  STraceUnit* pTraceInfo;
};

class EncodeDecodeTestAPIBase : public EncodeDecodeTestBase {
 public:
  uint8_t iRandValue;
 public:
  void SetUp() {
    EncodeDecodeTestBase::SetUp();
  }

  void TearDown() {
    EncodeDecodeTestBase::TearDown();
  }

  void prepareParam0 (int iLayers, int iSlices, int width, int height, float framerate, SEncParamExt* pParam) {
    memset (pParam, 0, sizeof (SEncParamExt));
    EncodeDecodeTestBase::prepareParam (iLayers, iSlices, width, height, framerate, pParam);
  }

  void prepareParamDefault (int iLayers, int iSlices, int width, int height, float framerate, SEncParamExt* pParam) {
    memset (pParam, 0, sizeof (SEncParamExt));
    encoder_->GetDefaultParams (pParam);
    EncodeDecodeTestBase::prepareParam (iLayers, iSlices, width, height, framerate, pParam);
  }

  void InitialEncDec (int iWidth, int iHeight);
  void RandomParamExtCombination();
  void ValidateParamExtCombination();
  void SliceParamValidationForMode2 (int iSpatialIdx);
  void SliceParamValidationForMode3 (int iSpatialIdx);
  void SliceParamValidationForMode4();

  void EncodeOneFrame (int iCheckTypeIndex) {
    int frameSize = EncPic.iPicWidth * EncPic.iPicHeight * 3 / 2;
    memset (buf_.data(), iRandValue, (frameSize >> 2));
    memset (buf_.data() + (frameSize >> 2), rand() % 256, (frameSize - (frameSize >> 2)));
    int rv = encoder_->EncodeFrame (&EncPic, &info);
    if (0 == iCheckTypeIndex)
      ASSERT_TRUE (rv == cmResultSuccess);
    else if (1 == iCheckTypeIndex)
      ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  }

  void EncDecOneFrame (const int iWidth, const int iHeight, const int iFrame, FILE* pfEnc) {
    int iLen = 0, rv;
    InitialEncDec (iWidth, iHeight);
    EncodeOneFrame (iFrame);

    //extract target layer data
    encToDecData (info, iLen);
    //call decoder
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, iLen, pData, &dstBufInfo_);
    EXPECT_TRUE (rv == cmResultSuccess) << " rv = " << rv << " iFrameIdx = " << iFrame;

    if (NULL != pfEnc) {
      fwrite (info.sLayerInfo[0].pBsBuf, iLen, 1, pfEnc);
    }
  }

  void TestOneSimulcastAVC (SEncParamExt* pParam, ISVCDecoder** decoder, unsigned char** pBsBuf, int iSpatialLayerNum,
                            int iEncFrameNum,
                            int iCallTimes) {
//#define DEBUG_FILE_SAVE4
    int aLen[MAX_SPATIAL_LAYER_NUM] = {0, 0, 0, 0};
#ifdef DEBUG_FILE_SAVE4
    FILE* fEnc[MAX_SPATIAL_LAYER_NUM];
    if (iCallTimes == 0) {
      fEnc[0] = fopen ("enc00.264", "wb");
      fEnc[1] = fopen ("enc01.264", "wb");
      fEnc[2] = fopen ("enc02.264", "wb");
      fEnc[3] = fopen ("enc03.264", "wb");
    } else {
      fEnc[0] = fopen ("enc10.264", "wb");
      fEnc[1] = fopen ("enc11.264", "wb");
      fEnc[2] = fopen ("enc12.264", "wb");
      fEnc[3] = fopen ("enc13.264", "wb");
    }
#endif

    int rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, pParam);
    ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed pParam: rv = " << rv;

    int iIdx;
    //begin testing
    for (int iFrame = 0; iFrame < iEncFrameNum; iFrame++) {
      int iResult;
      int iLayerLen = 0;
      unsigned char* pData[3] = { NULL };

      InitialEncDec (pParam->iPicWidth, pParam->iPicHeight);
      EncodeOneFrame (0);

      // init
      for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
        aLen[iIdx] = 0;
      }
      for (int iLayer = 0; iLayer < info.iLayerNum; ++iLayer) {
        iLayerLen = 0;
        const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayer];
        for (int iNal = 0; iNal < layerInfo.iNalCount; ++iNal) {
          iLayerLen += layerInfo.pNalLengthInByte[iNal];
        }

        iIdx = layerInfo.uiSpatialId;
        EXPECT_TRUE (iIdx < iSpatialLayerNum) << "iIdx = " << iIdx << ", iSpatialLayerNum = " << iSpatialLayerNum;
        memcpy ((pBsBuf[iIdx] + aLen[iIdx]), layerInfo.pBsBuf, iLayerLen * sizeof (unsigned char));
        aLen[iIdx] += iLayerLen;
      }

      for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
        pData[0] = pData[1] = pData[2] = 0;
        memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

#ifdef DEBUG_FILE_SAVE4
        fwrite (pBsBuf[iIdx], aLen[iIdx], 1, fEnc[iIdx]);
#endif
        iResult = decoder[iIdx]->DecodeFrame2 (pBsBuf[iIdx], aLen[iIdx], pData, &dstBufInfo_);
        EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << ", LayerIdx=" << iIdx;

        iResult = decoder[iIdx]->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
        EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << ", LayerIdx=" << iIdx;
        EXPECT_EQ (dstBufInfo_.iBufferStatus, 1) << "LayerIdx=" << iIdx;
      }
    }

#ifdef DEBUG_FILE_SAVE4
    fclose (fEnc[0]);
    fclose (fEnc[1]);
    fclose (fEnc[2]);
    fclose (fEnc[3]);
#endif
  }
};

class EncodeDecodeTestAPI : public ::testing::TestWithParam<EncodeDecodeFileParamBase>, public EncodeDecodeTestAPIBase {
  void SetUp() {
    EncodeDecodeTestAPIBase::SetUp();
  }

  void TearDown() {
    EncodeDecodeTestAPIBase::TearDown();
  }
};

void EncodeDecodeTestAPIBase::InitialEncDec (int iWidth, int iHeight) {

  // for encoder
  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = iWidth * iHeight * 3 / 2;

  buf_.SetLength (frameSize);
  ASSERT_TRUE (buf_.Length() == (size_t)frameSize);

  memset (&EncPic, 0, sizeof (SSourcePicture));
  EncPic.iPicWidth    = iWidth;
  EncPic.iPicHeight   =  iHeight;
  EncPic.iColorFormat = videoFormatI420;
  EncPic.iStride[0]   = EncPic.iPicWidth;
  EncPic.iStride[1]   = EncPic.iStride[2] = EncPic.iPicWidth >> 1;
  EncPic.pData[0]     = buf_.data();
  EncPic.pData[1]     = EncPic.pData[0] + iWidth * iHeight;
  EncPic.pData[2]     = EncPic.pData[1] + (iWidth * iHeight >> 2);

  //for decoder
  memset (&info, 0, sizeof (SFrameBSInfo));

  //set a fixed random value
  iRandValue = rand() % 256;
}

void EncodeDecodeTestAPIBase::RandomParamExtCombination() {

  param_.iPicWidth  = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 2, MAX_WIDTH);
  param_.iPicHeight = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1) + 1) << 1, 2, MAX_HEIGHT);

  param_.fMaxFrameRate      = rand() % FRAME_RATE_RANGE + 0.5f;
  param_.iUsageType         = static_cast<EUsageType> (rand() % 2);
  param_.iTemporalLayerNum  = rand() % TEMPORAL_LAYER_NUM_RANGE;
  param_.iSpatialLayerNum   = rand() % SPATIAL_LAYER_NUM_RANGE;

  param_.uiIntraPeriod      = rand() - 1;
  param_.iNumRefFrame       = AUTO_REF_PIC_COUNT;
  param_.iMultipleThreadIdc = rand();

  int iValue   = rand() % 7;
  switch (iValue) {
  case 0:
    param_.eSpsPpsIdStrategy  = CONSTANT_ID;
    break;
  case 0x01:
    param_.eSpsPpsIdStrategy  = INCREASING_ID;
    break;
  case 0x02:
    param_.eSpsPpsIdStrategy  = SPS_LISTING;
    break;
  case 0x03:
    param_.eSpsPpsIdStrategy  = SPS_LISTING_AND_PPS_INCREASING;
    break;
  case 0x06:
    param_.eSpsPpsIdStrategy  = SPS_PPS_LISTING;
    break;
  default:
    param_.eSpsPpsIdStrategy  = CONSTANT_ID;
    break;
  }
  param_.bPrefixNalAddingCtrl      = (rand() % 2 == 0) ? false : true;
  param_.bEnableSSEI               = (rand() % 2 == 0) ? false : true;
  param_.iPaddingFlag              = rand() % 2;

  //LTR
  param_.bEnableLongTermReference  = (rand() % 2 == 0) ? false : true;
  param_.bIsLosslessLink           = (rand() % 2 == 0) ? false : true;
  param_.iLTRRefNum                = rand();
  param_.iLtrMarkPeriod            = rand();

  //loop filter
  param_.iLoopFilterDisableIdc     = rand() % 7;
  param_.iLoopFilterAlphaC0Offset  = rand();
  param_.iLoopFilterBetaOffset     = rand();

  param_.bEnableDenoise             = (rand() % 2 == 0) ? false : true;
  param_.bEnableBackgroundDetection = (rand() % 2 == 0) ? false : true;
  param_.bEnableAdaptiveQuant       = (rand() % 2 == 0) ? false : true;
  param_.bEnableFrameCroppingFlag   = (rand() % 2 == 0) ? false : true;
  param_.bEnableSceneChangeDetect   = (rand() % 2 == 0) ? false : true;

  //for rc
  param_.iRCMode            = static_cast<RC_MODES> (rand() % RC_MODE_RANGE - 1);
  param_.iMaxBitrate        = rand() % BIT_RATE_RANGE;
  param_.iTargetBitrate     = rand() % BIT_RATE_RANGE;
  param_.iMaxQp             = rand() % QP_RANGE;
  param_.iMinQp             = rand() % QP_RANGE;
  param_.uiMaxNalSize       = rand();
  param_.bEnableFrameSkip   = (rand() % 2 == 0) ? false : true;

  for (int iSpatialIdx = 0; iSpatialIdx < param_.iSpatialLayerNum; iSpatialIdx++) {
    if (iSpatialIdx < MAX_SPATIAL_LAYER_NUM) {
      SSpatialLayerConfig* pSpatialLayer = &param_.sSpatialLayers[iSpatialIdx];

      //to do: profile and level id
      //pSpatialLayer->uiProfileIdc        = 0;
      //pSpatialLayer->uiLevelIdc          = 0;
      pSpatialLayer->iVideoWidth         = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 2, MAX_WIDTH);
      pSpatialLayer->iVideoHeight        = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1) + 1) << 1, 2, MAX_HEIGHT);
      pSpatialLayer->fFrameRate          = rand() % FRAME_RATE_RANGE + 0.5f;
      pSpatialLayer->iMaxSpatialBitrate  = rand() % BIT_RATE_RANGE;
      pSpatialLayer->iSpatialBitrate     = rand() % BIT_RATE_RANGE;


      pSpatialLayer->sSliceCfg.uiSliceMode = static_cast<SliceModeEnum> (rand() % SLICE_MODE_NUM);
      if (pSpatialLayer->sSliceCfg.uiSliceMode != SM_DYN_SLICE) {
        param_.uiMaxNalSize       = 0;
      }
      pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum = rand();
      pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint = rand();
    }
  }
}

void EncodeDecodeTestAPIBase::ValidateParamExtCombination() {

  bool bDynSliceModeFlag   = false;
  unsigned int uiGOPSize   = 0;
  unsigned int uiSliceNum  = 0;
  int iTotalBitRate        = 0;
  int iMinQP               = 0;

  param_.iPicWidth          = WELS_CLIP3 (param_.iPicWidth,  2, MAX_WIDTH);
  param_.iPicHeight         = WELS_CLIP3 (param_.iPicHeight, 2, MAX_HEIGHT);
  param_.fMaxFrameRate      = WELS_CLIP3 (param_.fMaxFrameRate, MIN_FRAME_RATE, MAX_FRAME_RATE);
  param_.iTemporalLayerNum  = WELS_CLIP3 (param_.iTemporalLayerNum, 1, MAX_TEMPORAL_LAYER_NUM);

  if (CAMERA_VIDEO_REAL_TIME == param_.iUsageType)
    param_.iSpatialLayerNum   = WELS_CLIP3 (param_.iSpatialLayerNum, 1, MAX_SPATIAL_LAYER_NUM);
  else
    param_.iSpatialLayerNum   = 1;

  //IntraPeriod
  uiGOPSize = 1 << (param_.iTemporalLayerNum - 1);
  param_.uiIntraPeriod -= param_.uiIntraPeriod % uiGOPSize;

  //RefNum
  int32_t iRefUpperBound    = (param_.iUsageType == CAMERA_VIDEO_REAL_TIME) ?
                              MAX_REFERENCE_PICTURE_COUNT_NUM_CAMERA : MAX_REFERENCE_PICTURE_COUNT_NUM_SCREEN;
  param_.iNumRefFrame       = WELS_CLIP3 (param_.iNumRefFrame, MIN_REF_PIC_COUNT, iRefUpperBound);

  //to do: will add more validate logic for thread number
  param_.iMultipleThreadIdc = 1;

  //LTR
  //iLTRRefNum: not supported to set it arbitrary yet
  if (true == param_.bEnableLongTermReference) {
    param_.iLTRRefNum     = (SCREEN_CONTENT_REAL_TIME == param_.iUsageType) ? LONG_TERM_REF_NUM_SCREEN : LONG_TERM_REF_NUM;
    param_.iLtrMarkPeriod = (0 == param_.iLtrMarkPeriod) ? 1 : param_.iLtrMarkPeriod;

  } else {
    param_.iLTRRefNum = 0;
  }

  //loop filter
  param_.iLoopFilterDisableIdc    = param_.iLoopFilterDisableIdc    % LOOP_FILTER_IDC_NUM;
  param_.iLoopFilterAlphaC0Offset = param_.iLoopFilterAlphaC0Offset % (2 * LOOF_FILTER_OFFSET_RANGE + 1) -
                                    LOOF_FILTER_OFFSET_RANGE;
  param_.iLoopFilterBetaOffset    = param_.iLoopFilterBetaOffset    % (2 * LOOF_FILTER_OFFSET_RANGE + 1) -
                                    LOOF_FILTER_OFFSET_RANGE;

  for (int iSpatialIdx = 0; iSpatialIdx < param_.iSpatialLayerNum; iSpatialIdx++) {
    SSpatialLayerConfig* pSpatialLayer = &param_.sSpatialLayers[iSpatialIdx];
    pSpatialLayer->iVideoWidth  = param_.iPicWidth >> (param_.iSpatialLayerNum - 1 - iSpatialIdx);
    pSpatialLayer->iVideoHeight = param_.iPicHeight >> (param_.iSpatialLayerNum - 1 - iSpatialIdx);
    pSpatialLayer->fFrameRate   = param_.fMaxFrameRate;

    pSpatialLayer->iMaxSpatialBitrate  = WELS_CLIP3 (pSpatialLayer->iMaxSpatialBitrate, 1, BIT_RATE_RANGE);
    pSpatialLayer->iSpatialBitrate     = WELS_CLIP3 (pSpatialLayer->iSpatialBitrate, 1, pSpatialLayer->iMaxSpatialBitrate);
    iTotalBitRate += pSpatialLayer->iSpatialBitrate;

    uiSliceNum  = pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum;
    pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceNum = WELS_CLIP3 (uiSliceNum, 1, MAX_SLICES_NUM);
    pSpatialLayer->sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 0;


    //for SM_FIXEDSLCNUM_SLICE
    // to do will add this when GOM bug fixed
    if (SM_FIXEDSLCNUM_SLICE == pSpatialLayer->sSliceCfg.uiSliceMode) {
      pSpatialLayer->sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
    }

    //for slice mode = SM_DYN_SLICE
    if (SM_DYN_SLICE == pSpatialLayer->sSliceCfg.uiSliceMode) {
      bDynSliceModeFlag = true;
    }

    //for slice mode = SM_RASTER_SLICE
    if (SM_RASTER_SLICE == pSpatialLayer->sSliceCfg.uiSliceMode) {
      SliceParamValidationForMode2 (iSpatialIdx);
    }
    //for slice mode = SM_ROWMB_SLICE
    if (SM_ROWMB_SLICE == pSpatialLayer->sSliceCfg.uiSliceMode) {
      SliceParamValidationForMode3 (iSpatialIdx);
    }

  }

  //for RC
  if ((RC_QUALITY_MODE == param_.iRCMode) || (RC_BITRATE_MODE == param_.iRCMode)) {
    param_.bEnableFrameSkip = true;
  }
  if (param_.iTargetBitrate < iTotalBitRate) {
    param_.iTargetBitrate = iTotalBitRate;
  }
  if (param_.iMaxBitrate < param_.iTargetBitrate) {
    param_.iMaxBitrate = param_.iTargetBitrate;
  }
  param_.iMaxQp       = WELS_CLIP3 (param_.iMaxQp, MIN_QP, MAX_QP);
  param_.iMinQp       = WELS_CLIP3 (param_.iMinQp, MIN_QP, MAX_QP);
  iMinQP              = (param_.iMaxQp < param_.iMinQp) ? param_.iMaxQp : param_.iMinQp;
  param_.iMaxQp       = (param_.iMaxQp > param_.iMinQp) ? param_.iMaxQp : param_.iMinQp;
  param_.iMinQp       = iMinQP;
  param_.uiMaxNalSize = 0;

  //for slice mode = SM_DYN_SLICE
  if (true == bDynSliceModeFlag) {
    SliceParamValidationForMode4();
  }

}


void EncodeDecodeTestAPIBase::SliceParamValidationForMode2 (int iSpatialIdx) {

  unsigned int uiMbWidth          = 0;
  unsigned int uiMbHeight         = 0;
  unsigned int uiMbNumInFrame     = 0;
  unsigned int uiCountMb          = 0;
  unsigned int uiSliceIdx         = 0;
  unsigned int uiActualSliceCount = 0;

  uiMbWidth      = (param_.iPicWidth  + 15) >> 4;
  uiMbHeight     = (param_.iPicHeight + 15) >> 4;
  uiMbNumInFrame = uiMbWidth * uiMbHeight;

  uiSliceIdx = 0;
  while (uiSliceIdx < MAX_SLICES_NUM) {
    param_.sSpatialLayers[iSpatialIdx].sSliceCfg.sSliceArgument.uiSliceMbNum[uiSliceIdx] = rand() % uiMbNumInFrame;
    uiCountMb           += param_.sSpatialLayers[iSpatialIdx].sSliceCfg.sSliceArgument.uiSliceMbNum[uiSliceIdx];
    uiActualSliceCount   =  uiSliceIdx + 1;

    if (uiCountMb >= uiMbNumInFrame) {
      break;
    }

    ++ uiSliceIdx;
  }

  if (uiCountMb >= uiMbNumInFrame) {
    param_.sSpatialLayers[iSpatialIdx].sSliceCfg.sSliceArgument.uiSliceMbNum[uiActualSliceCount - 1] -=
      (uiCountMb - uiMbNumInFrame);

  } else {
    param_.sSpatialLayers[iSpatialIdx].sSliceCfg.sSliceArgument.uiSliceMbNum[uiActualSliceCount - 1 ] +=
      (uiMbNumInFrame - uiCountMb);
  }
  param_.sSpatialLayers[iSpatialIdx].sSliceCfg.sSliceArgument.uiSliceNum = uiActualSliceCount;

}
void EncodeDecodeTestAPIBase::SliceParamValidationForMode3 (int iSpatialIdx) {

  unsigned int uiMbHeight         = 0;

  uiMbHeight = (param_.iPicHeight + 15) >> 4;

  //change slice mode to SM_SINGLE_SLICE
  if (uiMbHeight >  MAX_SLICES_NUM) {
    param_.sSpatialLayers[iSpatialIdx].sSliceCfg.uiSliceMode = SM_SINGLE_SLICE;
  }

}

void EncodeDecodeTestAPIBase::SliceParamValidationForMode4() {
  //slice mode of all spatial layer should be set as SM_DYN_SLICE
  for (int iSpatialIdx = 0; iSpatialIdx < param_.iSpatialLayerNum; iSpatialIdx++) {
    param_.sSpatialLayers[iSpatialIdx].sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 600;
    param_.sSpatialLayers[iSpatialIdx].sSliceCfg.uiSliceMode = SM_DYN_SLICE;
  }
  param_.uiMaxNalSize = 1500;
}

static const EncodeDecodeFileParamBase kFileParamArray[] = {
  {300, 160, 96, 6.0f, 2, 1, "000000000000001010101010101010101010101001101010100000010101000011"},
  {300, 140, 96, 6.0f, 4, 1, "000000000000001010101010101010101010101001101010100000010101000011"},
  {300, 140, 96, 6.0f, 4, 1, "000000000000001010111010101011101010101001101010100000010101110011010101"},
};

TEST_P (EncodeDecodeTestAPI, DecoderVclNal) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum, p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  InitialEncDec (p.width, p.height);

  int iIdx = 0;
  while (iIdx <= p.numframes) {

    EncodeOneFrame (0);

    //decoding after each encoding frame
    int vclNal, len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_TRUE (rv == cmResultSuccess);
    rv = decoder_->GetOption (DECODER_OPTION_VCL_NAL, &vclNal);
    EXPECT_EQ (vclNal, FEEDBACK_UNKNOWN_NAL); //no reconstruction, unknown return
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_TRUE (rv == cmResultSuccess);
    rv = decoder_->GetOption (DECODER_OPTION_VCL_NAL, &vclNal);
    EXPECT_EQ (vclNal, FEEDBACK_VCL_NAL);
    iIdx++;
  } //while
  //ignore last frame
}

TEST_P (EncodeDecodeTestAPI, GetOptionFramenum) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  InitialEncDec (p.width, p.height);

  int32_t iEncFrameNum = -1;
  int32_t iDecFrameNum;
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (0);
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_FRAME_NUM, &iDecFrameNum);
    EXPECT_EQ (iDecFrameNum, -1);
    iEncFrameNum++;
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_FRAME_NUM, &iDecFrameNum);
    EXPECT_EQ (iEncFrameNum, iDecFrameNum);
    iIdx++;
  } //while
  //ignore last frame
}

TEST_P (EncodeDecodeTestAPI, GetOptionIDR) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  //init for encoder
  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  InitialEncDec (p.width, p.height);

  int32_t iEncCurIdrPicId = 0;
  int32_t iDecCurIdrPicId;
  int32_t iIDRPeriod = 1;
  int32_t iSpsPpsIdAddition = 0;
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    iSpsPpsIdAddition = rand() %
                        2; //the current strategy supports more than 2 modes, but the switch between the modes>2 is not allowed
    iIDRPeriod = (rand() % 150) + 1;
    encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
    encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);

    EncodeOneFrame (0);

    if (info.eFrameType == videoFrameTypeIDR) {
      iEncCurIdrPicId = iEncCurIdrPicId + 1;
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_IDR_PIC_ID, &iDecCurIdrPicId);
    EXPECT_EQ (iDecCurIdrPicId, iEncCurIdrPicId);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_IDR_PIC_ID, &iDecCurIdrPicId);
    EXPECT_EQ (iDecCurIdrPicId, iEncCurIdrPicId);
    iIdx++;
  } //while
  //ignore last frame
}


long IsKeyFrameLost (ISVCDecoder* pDecoder, SLTRRecoverRequest* p_LTR_Recover_Request, long hr) {
  long bLost = NO_RECOVERY_REQUSET;
  int tempInt = -1;
  int temple_id = -1;
  bool m_P2PmodeFlag = true;
  pDecoder->GetOption (DECODER_OPTION_TEMPORAL_ID, &temple_id);
  if (hr == dsErrorFree) {
    if (m_P2PmodeFlag && temple_id == 0) {
      pDecoder->GetOption (DECODER_OPTION_IDR_PIC_ID, &tempInt);
      // idr_pic_id change ,reset last correct position
      if (p_LTR_Recover_Request->uiIDRPicId != (unsigned int) tempInt) {
        p_LTR_Recover_Request->uiIDRPicId = tempInt;
        p_LTR_Recover_Request->iLastCorrectFrameNum = -1;
      }
      pDecoder->GetOption (DECODER_OPTION_FRAME_NUM, &tempInt);
      if (tempInt >= 0) {
        p_LTR_Recover_Request->iLastCorrectFrameNum = tempInt;
      }
    }
    bLost = NO_RECOVERY_REQUSET;
  } else if (hr & dsNoParamSets) {
    bLost = IDR_RECOVERY_REQUEST;
  } else if (((hr & dsRefLost) && (1 == temple_id)) || ((dsErrorFree != hr) && (0 == temple_id))) {
    bLost = LTR_RECOVERY_REQUEST;
  } else {
    bLost = NO_RECOVERY_REQUSET;
  }
  return bLost;
}

bool IsLTRMarking (ISVCDecoder* pDecoder) {
  int bLTR_marking_flag = 0;
  pDecoder->GetOption (DECODER_OPTION_LTR_MARKING_FLAG, &bLTR_marking_flag);
  return (bLTR_marking_flag) ? (true) : (false);
}

void LTRRecoveryRequest (ISVCDecoder* pDecoder, ISVCEncoder* pEncoder, SLTRRecoverRequest* p_LTR_Recover_Request,
                         long hr, bool m_P2PmodeFlag) {
  long bKLost = IsKeyFrameLost (pDecoder, p_LTR_Recover_Request, hr);
  if (m_P2PmodeFlag) {
    if (bKLost == IDR_RECOVERY_REQUEST) {
      pEncoder->ForceIntraFrame (true);
    } else if (bKLost == LTR_RECOVERY_REQUEST) {
      p_LTR_Recover_Request->uiFeedbackType = LTR_RECOVERY_REQUEST;
      pDecoder->GetOption (DECODER_OPTION_FRAME_NUM, &p_LTR_Recover_Request->iCurrentFrameNum);
      pDecoder->GetOption (DECODER_OPTION_IDR_PIC_ID, &p_LTR_Recover_Request->uiIDRPicId);
      pEncoder->SetOption (ENCODER_LTR_RECOVERY_REQUEST, p_LTR_Recover_Request);
    }
  } else {
    if (bKLost == IDR_RECOVERY_REQUEST || bKLost == LTR_RECOVERY_REQUEST) {
      p_LTR_Recover_Request->uiFeedbackType = IDR_RECOVERY_REQUEST;
      pEncoder->ForceIntraFrame (true);
    }
  }
}

void LTRMarkFeedback (ISVCDecoder* pDecoder, ISVCEncoder* pEncoder, SLTRMarkingFeedback* p_LTR_Marking_Feedback,
                      long hr) {
  if (IsLTRMarking (pDecoder) == true) {
    p_LTR_Marking_Feedback->uiFeedbackType = (hr == dsErrorFree) ? (LTR_MARKING_SUCCESS) : (LTR_MARKING_FAILED);
    pDecoder->GetOption (DECODER_OPTION_IDR_PIC_ID, &p_LTR_Marking_Feedback->uiIDRPicId);
    pDecoder->GetOption (DECODER_OPTION_LTR_MARKED_FRAME_NUM, &p_LTR_Marking_Feedback->iLTRFrameNum);
    pEncoder->SetOption (ENCODER_LTR_MARKING_FEEDBACK, p_LTR_Marking_Feedback);
  }
}

bool ToRemainDidNal (const unsigned char* pSrc, EWelsNalUnitType eNalType, int iTarDid) {
  uint8_t uiCurByte = *pSrc;
  if (IS_NEW_INTRODUCED_SVC_NAL (eNalType)) {
    int iDid = (uiCurByte & 0x70) >> 4;
    return iDid == iTarDid;
  } else if ((IS_VCL_NAL_AVC_BASE (eNalType)) && iTarDid != 0) {
    return false;
  } else {
    return true;
  }
}

void ExtractDidNal (SFrameBSInfo* pBsInfo, int& iSrcLen, std::vector<SLostSim>* p_SLostSim, int iTarDid) {
  unsigned char* pDst = new unsigned char[iSrcLen];
  const unsigned char* pSrc = pBsInfo->sLayerInfo[0].pBsBuf;
  int iDstLen = 0;
  bool bLost;
  SLostSim tmpSLostSim;
  p_SLostSim->clear();
  int iPrefix;
  unsigned char* pSrcPtr = pBsInfo->sLayerInfo[0].pBsBuf;
  for (int j = 0; j < pBsInfo->iLayerNum; j++) {
    for (int k = 0; k < pBsInfo->sLayerInfo[j].iNalCount; k++) {
      if (pSrcPtr[0] == 0 && pSrcPtr[1] == 0 && pSrcPtr[2] == 0 && pSrcPtr[3] == 1) {
        iPrefix = 4;
      } else if (pSrcPtr[0] == 0 && pSrcPtr[1] == 0 && pSrcPtr[2] == 1) {
        iPrefix = 3;
      } else {
        iPrefix = 0;
      }
      tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrcPtr + iPrefix)) & 0x1f); // eNalUnitType
      bLost = (ToRemainDidNal ((pSrcPtr + iPrefix + 2), tmpSLostSim.eNalType, iTarDid)) ? false : true;
      tmpSLostSim.isLost = bLost;
      p_SLostSim->push_back (tmpSLostSim);
      if (!bLost) {
        memcpy (pDst + iDstLen, pSrcPtr, pBsInfo->sLayerInfo[j].pNalLengthInByte[k]);
        iDstLen += (pBsInfo->sLayerInfo[j].pNalLengthInByte[k]);
      }
      pSrcPtr += pBsInfo->sLayerInfo[j].pNalLengthInByte[k];
    }
  }
  memset ((void*)pSrc, 0, iSrcLen);
  memcpy ((void*)pSrc, pDst, iDstLen);
  iSrcLen = iDstLen;
  delete [] pDst;
}

int SimulateNALLoss (const unsigned char* pSrc,  int& iSrcLen, std::vector<SLostSim>* p_SLostSim,
                     const char* pLossChars, bool bLossPara, int& iLossIdx, bool& bVCLLoss) {
  unsigned char* pDst = new unsigned char[iSrcLen];
  int iLossCharLen = (int) strlen (pLossChars);
  int iSkipedBytes = 0;
  int iDstLen = 0;
  int iBufPos = 0;
  int ilastprefixlen = 0;
  int i = 0;
  bool bLost;
  bVCLLoss = false;
  SLostSim tmpSLostSim;
  p_SLostSim->clear();
  for (i = 0; i < iSrcLen;) {
    if (pSrc[i] == 0 && pSrc[i + 1] == 0 && pSrc[i + 2] == 0 && pSrc[i + 3] == 1) {
      if (i - iBufPos) {
        tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrc + iBufPos + ilastprefixlen)) & 0x1f); // eNalUnitType
        bLost = iLossIdx < iLossCharLen ? (pLossChars[iLossIdx] == '1') : (rand() % 2 == 1);
        bLost = (!bLossPara) && (IS_PARAM_SETS_NALS (tmpSLostSim.eNalType)) ? false : bLost;
        iLossIdx++;
        tmpSLostSim.isLost = bLost;
        p_SLostSim->push_back (tmpSLostSim);
        if (!bLost) {
          memcpy (pDst + iDstLen, pSrc + iBufPos, i - iBufPos);
          iDstLen += (i - iBufPos);
        } else {
          bVCLLoss = (IS_VCL_NAL (tmpSLostSim.eNalType, 1)) ? true : bVCLLoss;
          iSkipedBytes += (i - iBufPos);
        }
      }
      ilastprefixlen = 4;
      iBufPos = i;
      i = i + 4;
    } else if (pSrc[i] == 0 && pSrc[i + 1] == 0 && pSrc[i + 2] == 1) {
      if (i - iBufPos) {
        tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrc + iBufPos + ilastprefixlen)) & 0x1f); // eNalUnitType
        bLost = iLossIdx < iLossCharLen ? (pLossChars[iLossIdx] == '1') : (rand() % 2 == 1);
        bLost = (!bLossPara) && (IS_PARAM_SETS_NALS (tmpSLostSim.eNalType)) ? false : bLost;
        iLossIdx++;
        tmpSLostSim.isLost = bLost;
        p_SLostSim->push_back (tmpSLostSim);
        if (!bLost) {
          memcpy (pDst + iDstLen, pSrc + iBufPos, i - iBufPos);
          iDstLen += (i - iBufPos);
        } else {
          bVCLLoss = (IS_VCL_NAL (tmpSLostSim.eNalType, 1)) ? true : bVCLLoss;
          iSkipedBytes += (i - iBufPos);
        }
      }
      ilastprefixlen = 3;
      iBufPos = i;
      i = i + 3;
    } else {
      i++;
    }
  }
  if (i - iBufPos) {
    tmpSLostSim.eNalType = (EWelsNalUnitType) ((* (pSrc + iBufPos + ilastprefixlen)) & 0x1f); // eNalUnitType
    bLost = iLossIdx < iLossCharLen ? (pLossChars[iLossIdx] == '1') : (rand() % 2 == 1);
    bLost = (!bLossPara) && (IS_PARAM_SETS_NALS (tmpSLostSim.eNalType)) ? false : bLost;
    iLossIdx++;
    tmpSLostSim.isLost = bLost;
    p_SLostSim->push_back (tmpSLostSim);
    if (!bLost) {
      memcpy (pDst + iDstLen, pSrc + iBufPos, i - iBufPos);
      iDstLen += (i - iBufPos);
    } else {
      bVCLLoss = (IS_VCL_NAL (tmpSLostSim.eNalType, 1)) ? true : bVCLLoss;
      iSkipedBytes += (i - iBufPos);
    }
  }
  memset ((void*)pSrc, 0, iSrcLen);
  memcpy ((void*)pSrc, pDst, iDstLen);
  iSrcLen = iDstLen;
  delete [] pDst;
  return iSkipedBytes;
}

TEST_P (EncodeDecodeTestAPI, GetOptionLTR_ALLIDR) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  InitialEncDec (p.width, p.height);

  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = (int32_t) pow (2.0f, (param_.iTemporalLayerNum - 1)) * ((rand() % 5) + 1);
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (0);
    ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    encoder_->ForceIntraFrame (true);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionLTR_ALLLTR) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
  int frameSize = p.width * p.height * 3 / 2;
  buf_.SetLength (frameSize);
  ASSERT_TRUE (buf_.Length() == (size_t)frameSize);
  SFrameBSInfo info;
  memset (&info, 0, sizeof (SFrameBSInfo));

  SSourcePicture pic;
  memset (&pic, 0, sizeof (SSourcePicture));
  pic.iPicWidth = p.width;
  pic.iPicHeight = p.height;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = pic.iPicWidth;
  pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;
  pic.pData[0] = buf_.data();
  pic.pData[1] = pic.pData[0] + p.width * p.height;
  pic.pData[2] = pic.pData[1] + (p.width * p.height >> 2);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    memset (buf_.data(), rand() % 256, frameSize);
    rv = encoder_->EncodeFrame (&pic, &info);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
    m_LTR_Recover_Request.uiFeedbackType = LTR_RECOVERY_REQUEST;
    m_LTR_Recover_Request.iCurrentFrameNum = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    m_LTR_Recover_Request.uiIDRPicId = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    m_LTR_Recover_Request.iLastCorrectFrameNum = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    encoder_->SetOption (ENCODER_LTR_RECOVERY_REQUEST, &m_LTR_Recover_Request);
    m_LTR_Marking_Feedback.uiFeedbackType = rand() % 2 == 1 ? LTR_MARKING_SUCCESS : LTR_MARKING_FAILED;
    m_LTR_Marking_Feedback.uiIDRPicId = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    m_LTR_Marking_Feedback.iLTRFrameNum = rand() % 2 == 1 ? -rand() % 10000 : rand() % 10000;
    encoder_->SetOption (ENCODER_LTR_MARKING_FEEDBACK, &m_LTR_Marking_Feedback);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionLTR_Engine) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);

  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  int iLossIdx = 0;
  bool bVCLLoss = false;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx, bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, SetOptionECFlag_ERROR_CON_DISABLE) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.bEnableLongTermReference = true;
  param_.iLTRRefNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  if (decoder_ != NULL) {
    decoder_->Uninitialize();
  }
  SDecodingParam decParam;
  memset (&decParam, 0, sizeof (SDecodingParam));
  decParam.eOutputColorFormat  = videoFormatI420;
  decParam.uiTargetDqLayer = UCHAR_MAX;
  decParam.eEcActiveIdc = ERROR_CON_DISABLE;
  decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;
  rv = decoder_->Initialize (&decParam);
  ASSERT_EQ (0, rv);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  int iLossIdx = 0;
  int iSkipedBytes;
  bool bVCLLoss = false;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    iSkipedBytes = SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx,
                                    bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    if (iSkipedBytes && bVCLLoss) {
      ASSERT_TRUE (dstBufInfo_.iBufferStatus == 0);
    }
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, SetOptionECFlag_ERROR_CON_SLICE_COPY) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  int iLossIdx = 0;
  int iSkipedBytes;
  bool bVCLLoss = false;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    iSkipedBytes = SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx,
                                    bVCLLoss);
    uint32_t uiEcIdc = rand() % 2 == 1 ? ERROR_CON_DISABLE : ERROR_CON_SLICE_COPY;
    decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
  (void) iSkipedBytes;
}

TEST_P (EncodeDecodeTestAPI, InOutTimeStamp) {
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  int iSkipedBytes;
  unsigned long long uiEncTimeStamp = 100;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    uint32_t uiEcIdc = ERROR_CON_SLICE_COPY_CROSS_IDR_FREEZE_RES_CHANGE;
    decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
    dstBufInfo_.uiInBsTimeStamp = uiEncTimeStamp;
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    dstBufInfo_.uiInBsTimeStamp = uiEncTimeStamp;
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    if (dstBufInfo_.iBufferStatus == 1) {
      EXPECT_EQ (uiEncTimeStamp, dstBufInfo_.uiOutYuvTimeStamp);
    }
    iIdx++;
    uiEncTimeStamp++;
  }
  (void) iSkipedBytes;
}

TEST_P (EncodeDecodeTestAPI, GetOptionTid_AVC_NOPREFIX) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.bPrefixNalAddingCtrl = false;
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  int iLossIdx = 0;
  bool bVCLLoss = false;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx, bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    int iTid = -1;
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    if (iTid != -1) {
      ASSERT_EQ (iTid, 0);
    }
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    std::vector<SLostSim>::iterator iter = m_SLostSim.begin();
    bool bHasVCL = false;
    for (unsigned int k = 0; k < m_SLostSim.size(); k++) {
      if (IS_VCL_NAL (iter->eNalType, 0) && iter->isLost == false) {
        bHasVCL = true;
        break;
      }
      iter++;
    }
    (void) bHasVCL;
    if (iTid != -1) {
      ASSERT_EQ (iTid, 0);
    }
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionTid_AVC_WITH_PREFIX_NOLOSS) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.bPrefixNalAddingCtrl = true;
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    ExtractDidNal (&info, len, &m_SLostSim, 0);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    int iTid = -1;
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, -1);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, info.sLayerInfo[0].uiTemporalId);
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, GetOptionTid_SVC_L1_NOLOSS) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (2, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 2;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    ExtractDidNal (&info, len, &m_SLostSim, 1);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    int iTid = -1;
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, -1);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, info.sLayerInfo[0].uiTemporalId);
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}



TEST_P (EncodeDecodeTestAPI, SetOption_Trace) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = 1;

  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  pFunc = TestOutPutTrace;
  pTraceInfo = &sTrace;
  sTrace.iTarLevel = iTraceLevel;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK, &pFunc);
  encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);
  decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK, &pFunc);
  decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);


  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  int iLossIdx = 0;
  bool bVCLLoss = false;
  while (iIdx <= p.numframes) {
    iTraceLevel = rand() % 33;
    sTrace.iTarLevel = iTraceLevel;
    encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    ExtractDidNal (&info, len, &m_SLostSim, 0);
    SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx, bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_P (EncodeDecodeTestAPI, SetOption_Trace_NULL) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = 1;
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);

  int32_t iTraceLevel = WELS_LOG_QUIET;
  pFunc = NULL;
  pTraceInfo = NULL;
  encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK, &pFunc);
  encoder_->SetOption (ENCODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);
  decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK, &pFunc);
  decoder_->SetOption (DECODER_OPTION_TRACE_CALLBACK_CONTEXT, &pTraceInfo);
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  int iLossIdx = 0;
  bool bVCLLoss = false;
  while (iIdx <= p.numframes) {
    iTraceLevel = rand() % 33;
    encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    ExtractDidNal (&info, len, &m_SLostSim, 0);
    SimulateNALLoss (info.sLayerInfo[0].pBsBuf, len, &m_SLostSim, p.pLossSequence, p.bLostPara, iLossIdx, bVCLLoss);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

INSTANTIATE_TEST_CASE_P (EncodeDecodeTestAPIBase, EncodeDecodeTestAPI,
                         ::testing::ValuesIn (kFileParamArray));

TEST_P (EncodeDecodeTestAPI, SetOptionECIDC_GeneralSliceChange) {
  uint32_t uiEcIdc;
  uint32_t uiGet;
  EncodeDecodeFileParamBase p = GetParam();
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY

  uiEcIdc = 0;
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);

  //Start for enc/dec

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int iIdx = 0;
  bool bVCLLoss = false;
  int iPacketNum;
  int len;
  int iTotalSliceSize;

  //enc/dec pictures
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    //decoding after each encoding frame
    len = 0;
    iPacketNum = 0;
    iTotalSliceSize = 0;
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    while (iPacketNum < info.sLayerInfo[0].iNalCount) {
      encToDecSliceData (0, iPacketNum, info, len);
      uiEcIdc = (ERROR_CON_IDC) (rand() % 2);
      decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
      decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
      EXPECT_EQ (uiGet, uiEcIdc);

      bVCLLoss = rand() & 1; //loss or not
      if (!bVCLLoss) { //not loss
        rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf + iTotalSliceSize,
                                     info.sLayerInfo[0].pNalLengthInByte[iPacketNum], pData, &dstBufInfo_);
        if (uiEcIdc == ERROR_CON_DISABLE)
          EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
      }
      //EC_IDC should not change till now
      decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
      EXPECT_EQ (uiGet, uiEcIdc);
      //Set again inside
      uiEcIdc = (ERROR_CON_IDC) (rand() % 2);
      decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
      decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
      EXPECT_EQ (uiGet, uiEcIdc);

      rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
      //EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
      if (uiEcIdc == ERROR_CON_DISABLE && rv != 0)
        EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);

      //deal with next slice
      iTotalSliceSize += len;
      iPacketNum++;
    } //while slice
    iIdx++;
  } //while frame
}

//This case contain 1 slice per picture
//coding order:                 0   1   2   3   4   5   6   7
//frame type:                   IDR P   P   P   P   P   P   P
//EC_IDC:                       0   0   0   2   0   0   1   1
//loss:                         N   Y   N   N   N   Y   Y   N

TEST_F (EncodeDecodeTestAPI, SetOptionECIDC_SpecificFrameChange) {
  uint32_t uiEcIdc;
  uint32_t uiGet;
  EncodeDecodeFileParamBase p = kFileParamArray[0];
  prepareParamDefault (1, p.slicenum,  p.width, p.height, p.frameRate, &param_);

  param_.iSpatialLayerNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (ERROR_CON_IDC) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);

  //Start for enc/dec
  int iIdx = 0;
  int len = 0;
  unsigned char* pData[3] = { NULL };
  InitialEncDec (p.width, p.height);
  //Frame 0: IDR, EC_IDC=DISABLE, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (ERROR_CON_IDC) ERROR_CON_DISABLE);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_EQ (rv, 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1);
  iIdx++;

  //Frame 1: P, EC_IDC=DISABLE, loss = 1
  EncodeOneFrame (1);
  iIdx++;

  //Frame 2: P, EC_IDC=DISABLE, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv != 0); //construction error due to data loss
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output due to EC DISABLE
  iIdx++;

  //set EC=SLICE_COPY
  uiEcIdc = (uint32_t) (ERROR_CON_SLICE_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 3: P, EC_IDC=SLICE_COPY, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv != 0); //construction error due to data loss
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1);
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 4: P, EC_IDC=DISABLE, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  //Ref picture is ECed, so current status is ECed, when EC disable, NO output
  EXPECT_TRUE (rv != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  iIdx++;

  //Frame 5: P, EC_IDC=DISABLE, loss = 1
  EncodeOneFrame (1);
  iIdx++;

  //set EC=FRAME_COPY
  uiEcIdc = (uint32_t) (ERROR_CON_FRAME_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 6: P, EC_IDC=FRAME_COPY, loss = 1
  EncodeOneFrame (1);
  EXPECT_EQ (uiGet, uiEcIdc);
  iIdx++;

  //Frame 7: P, EC_IDC=FRAME_COPY, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_FRAME_COPY);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv == 0); // Now the parse process is Error_None, and the reconstruction will has error return
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv != 0); //not sure if previous data drop would be detected in construction
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1);
  iIdx++;
}

//This case contain 2 slices per picture for IDR loss
//coding order:                 0   1   2   3   4
//frame type                    IDR P   P   P   P
//EC_IDC                        2   2   0   1   0
//loss (2 slice: 1,2):          2   0   0   1   0

TEST_F (EncodeDecodeTestAPI, SetOptionECIDC_SpecificSliceChange_IDRLoss) {
  uint32_t uiEcIdc = 2; //default set as SLICE_COPY
  uint32_t uiGet;
  EncodeDecodeFileParamBase p = kFileParamArray[0];
  prepareParamDefault (1, 2,  p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  //Start for enc/dec
  int iIdx = 0;
  int len = 0;
  unsigned char* pData[3] = { NULL };
  int iTotalSliceSize = 0;

  InitialEncDec (p.width, p.height);

  //Frame 0: IDR, EC_IDC=2, loss = 2
  EncodeOneFrame (1);
  iTotalSliceSize = 0;
  encToDecSliceData (0, 0, info, len); //SPS
  iTotalSliceSize = len;
  encToDecSliceData (0, 1, info, len); //PPS
  iTotalSliceSize += len;
  encToDecSliceData (1, 0, info, len); //first slice
  iTotalSliceSize += len;
  //second slice loss
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, iTotalSliceSize, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_EQ (rv, 0); // Reconstruct first slice OK
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //slice incomplete, no output
  iIdx++;

  //Frame 1: P, EC_IDC=2, loss = 0
  //will clean SPS/PPS status
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice together
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE ((rv & 32) != 0); //parse correct, but reconstruct ECed
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //ECed output for frame 0
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //ECed status, reconstruction current frame 1
  EXPECT_TRUE ((rv & 32) != 0); //decoder ECed status
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //ECed output for frame 1
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 2: P, EC_IDC=0, loss = 0
  /////will clean SPS/PPS status
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice together
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  // Ref picture is ECed, so reconstructed picture is ECed
  EXPECT_TRUE ((rv & 32) != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  iIdx++;

  //set EC=SLICE_COPY
  uiEcIdc = (uint32_t) (ERROR_CON_FRAME_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 3: P, EC_IDC=2, loss = 1
  EncodeOneFrame (1);
  encToDecSliceData (0, 0, info, iTotalSliceSize); //slice 1 lost
  encToDecSliceData (0, 1, info, len); //slice 2
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_FRAME_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf + iTotalSliceSize, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE ((rv & 32) != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //slice loss
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 4: P, EC_IDC=0, loss = 0
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); // EC_IDC=0, previous picture slice lost, no output
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE (rv != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); // No ref picture, no output
  iIdx++;

}



//This case contain 2 slices per picture for no IDR loss
//coding order:                 0   1   2   3   4   5
//frame type                    IDR P   P   P   P   IDR
//EC_IDC                        0   2   0   2   0   ^2^
//loss (2 slice: 1,2):          0   0   1   2   0   0

TEST_F (EncodeDecodeTestAPI, SetOptionECIDC_SpecificSliceChange_IDRNoLoss) {
  uint32_t uiEcIdc;
  uint32_t uiGet;
  EncodeDecodeFileParamBase p = kFileParamArray[0];
  prepareParamDefault (1, 2,  p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = 1;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  //Start for enc/dec
  int iIdx = 0;
  int len = 0;
  unsigned char* pData[3] = { NULL };
  int iTotalSliceSize = 0;

  InitialEncDec (p.width, p.height);

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 0: IDR, EC_IDC=0, loss = 0
  //Expected result: all OK, 2nd Output
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //output frame 0
  iIdx++;

  //Frame 1: P, EC_IDC=0, loss = 0
  //Expected result: all OK, 2nd Output
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice together
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //ECed output for frame 0
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction current frame 1
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //ECed output for frame 1
  iIdx++;

  //Frame 2: P, EC_IDC=0, loss = 1
  //Expected result: all OK, no Output
  EncodeOneFrame (1);
  encToDecSliceData (0, 0, info, iTotalSliceSize); // slice 1 lost
  encToDecSliceData (0, 1, info, len); // slice 2 only
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf + iTotalSliceSize, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_EQ (rv, 0); //parse correct
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0);
  iIdx++;

  //set EC=SLICE_COPY
  uiEcIdc = (uint32_t) (ERROR_CON_SLICE_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 3: P, EC_IDC=2, loss = 2
  //Expected result: neither OK, 1st Output
  EncodeOneFrame (1);
  encToDecSliceData (0, 0, info, len); //slice 1 only
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE ((rv & 32) != 0); //parse OK but frame 2 ECed
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //slice loss but ECed output Frame 2
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
  EXPECT_TRUE ((rv & 32) != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //slice loss
  iIdx++;

  //set EC=DISABLE
  uiEcIdc = (uint32_t) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  //Frame 4: P, EC_IDC=0, loss = 0
  //Expected result: depends on DecodeFrame2 result. If OK, output; else ,no output
  EncodeOneFrame (1);
  encToDecData (info, len); //all slice
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
  EXPECT_TRUE (rv != 0); //previous slice not outputted, will return error due to incomplete frame
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //output previous pic
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction,
  // previous frame NOT output, no ref
  EXPECT_TRUE (rv != 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //output previous pic
  iIdx++;

  //Frame 5: IDR, EC_IDC=2->0, loss = 0
  //Expected result: depends on DecodeFrame2 result. If OK, output; else ,no output
  int32_t iIDRPeriod = 1;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  EncodeOneFrame (1);
  EXPECT_TRUE (info.eFrameType == videoFrameTypeIDR);
  encToDecSliceData (0, 0, info, len); //SPS
  iTotalSliceSize = len;
  encToDecSliceData (0, 1, info, len); //PPS
  iTotalSliceSize += len;
  encToDecSliceData (1, 0, info, len); //slice 1
  iTotalSliceSize += len;
  //set EC=SLICE_COPY for slice 1
  uiEcIdc = (uint32_t) (ERROR_CON_SLICE_COPY);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, iTotalSliceSize, pData, &dstBufInfo_);
  EXPECT_TRUE (rv == 0); // IDR status return error_free
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //frame incomplete
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction,
  EXPECT_TRUE (rv == 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //output previous pic
  //set EC=DISABLE for slice 2
  encToDecSliceData (1, 1, info, len); //slice 1
  uiEcIdc = (int) (ERROR_CON_DISABLE);
  decoder_->SetOption (DECODER_OPTION_ERROR_CON_IDC, &uiEcIdc);
  decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
  EXPECT_EQ (uiGet, uiEcIdc);
  pData[0] = pData[1] = pData[2] = 0;
  memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
  rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf + iTotalSliceSize, len, pData, &dstBufInfo_);
  EXPECT_EQ (rv, 0); //Parse correct under no EC
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 0); //frame incomplete
  rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction,
  EXPECT_EQ (rv, 0);
  EXPECT_EQ (dstBufInfo_.iBufferStatus, 1); //output previous pic
  iIdx++;

}

static const EncodeDecodeFileParamBase kSVCSwitch[] = {
  {300, 160, 96, 6.0f, 2, 1, "120012130101012311201221323"},
};

TEST_F (EncodeDecodeTestAPI, Engine_SVC_Switch_I) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = kSVCSwitch[0];
  p.width = p.width << 2;
  p.height = p.height << 2;
  prepareParamDefault (4, p.slicenum, p.width, p.height, p.frameRate, &param_);
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 4;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = (int32_t) pow (2.0f, (param_.iTemporalLayerNum - 1)) * ((rand() % 5) + 1);
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;
  int iTarDid = 0;
  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    if (info.eFrameType == videoFrameTypeIDR) {
      iTarDid = rand() % 4;
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

    ExtractDidNal (&info, len, &m_SLostSim, iTarDid);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    ASSERT_EQ (rv, 0);
    int iTid = -1;
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, -1);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    ASSERT_EQ (rv, 0);
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, info.sLayerInfo[0].uiTemporalId);
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, true);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
  }
}

TEST_F (EncodeDecodeTestAPI, Engine_SVC_Switch_P) {
  SLTRMarkingFeedback m_LTR_Marking_Feedback;
  SLTRRecoverRequest m_LTR_Recover_Request;
  m_LTR_Recover_Request.uiIDRPicId = 0;
  EncodeDecodeFileParamBase p = kSVCSwitch[0];
  int iTarDid = 0;
  int iLastDid = 0;
  p.width = p.width << 2;
  p.height = p.height << 2;
  prepareParamDefault (4, p.slicenum, p.width, p.height, p.frameRate, &param_);
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 4;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;

  InitialEncDec (p.width, p.height);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = 60;
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  SLTRConfig sLtrConfigVal;
  sLtrConfigVal.bEnableLongTermReference = 1;
  sLtrConfigVal.iLTRRefNum = 1;
  encoder_->SetOption (ENCODER_OPTION_LTR, &sLtrConfigVal);
  int32_t iLtrPeriod = 2;
  encoder_->SetOption (ENCODER_LTR_MARKING_PERIOD, &iLtrPeriod);
  int iIdx = 0;

  while (iIdx <= p.numframes) {
    EncodeOneFrame (1);
    if (m_LTR_Recover_Request.uiFeedbackType == IDR_RECOVERY_REQUEST) {
      ASSERT_TRUE (info.eFrameType == videoFrameTypeIDR);
    }
    //decoding after each encoding frame
    int len = 0;
    encToDecData (info, len);
    unsigned char* pData[3] = { NULL };
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    if (iIdx < (int) strlen (p.pLossSequence)) {
      switch (p.pLossSequence[iIdx]) {
      case '0':
        iTarDid = 0;
        break;
      case '1':
        iTarDid = 1;
        break;
      case '2':
        iTarDid = 2;
        break;
      case '3':
        iTarDid = 3;
        break;
      default :
        iTarDid = rand() % 4;
        break;
      }
    } else {
      iTarDid = rand() % 4;
    }

    ExtractDidNal (&info, len, &m_SLostSim, iTarDid);
    rv = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
    int iTid = -1;
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, -1);
    m_LTR_Recover_Request.uiFeedbackType = NO_RECOVERY_REQUSET;
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, false);
    rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
    if (info.eFrameType == videoFrameTypeP && iIdx > 0 && iLastDid != iTarDid) {
      ASSERT_NE (rv, 0);
    } else if (info.eFrameType == videoFrameTypeIDR) {
      ASSERT_EQ (rv, 0);
    }
    decoder_->GetOption (DECODER_OPTION_TEMPORAL_ID, &iTid);
    ASSERT_EQ (iTid, info.sLayerInfo[0].uiTemporalId);
    LTRRecoveryRequest (decoder_, encoder_, &m_LTR_Recover_Request, rv, false);
    LTRMarkFeedback (decoder_, encoder_, &m_LTR_Marking_Feedback, rv);
    iIdx++;
    iLastDid = iTarDid;
  }
}

TEST_F (EncodeDecodeTestAPI, SetOptionEncParamExt) {
  int iSpatialLayerNum = 4;
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_HEIGHT);
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);
  int iSliceNum        = 1;
  encoder_->GetDefaultParams (&param_);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &param_);

  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  int iTraceLevel = WELS_LOG_QUIET;
  rv = encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  ASSERT_TRUE (rv == cmResultSuccess);

  for (int i = 0; i < iEncFrameNum; i++) {
    int iResult;
    int len = 0;
    unsigned char* pData[3] = { NULL };

    RandomParamExtCombination();
    iResult = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &param_);
    //ValidateParamExtCombination();
    //ASSERT_TRUE (iResult == cmResultSuccess);
    //to do
    // currently, there are still some error cases even though under condition cmResultSuccess == iResult
    // so need to enhance the validation check for any random value of each variable in ParamExt

    if (cmResultSuccess == iResult) {
      InitialEncDec (param_.iPicWidth, param_.iPicHeight);
      EncodeOneFrame (0);
      encToDecData (info, len);
      pData[0] = pData[1] = pData[2] = 0;
      memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

      iResult = decoder_->DecodeFrame2 (info.sLayerInfo[0].pBsBuf, len, pData, &dstBufInfo_);
      ASSERT_TRUE (iResult == cmResultSuccess);
      iResult = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
      ASSERT_TRUE (iResult == cmResultSuccess);
      EXPECT_EQ (dstBufInfo_.iBufferStatus, 1);
    }
  }

  iTraceLevel = WELS_LOG_ERROR;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
}

class DecodeCrashTestAPI : public ::testing::TestWithParam<EncodeDecodeFileParamBase>, public EncodeDecodeTestBase {
 public:
  uint8_t iRandValue;
 public:
  void SetUp() {
    EncodeDecodeTestBase::SetUp();
    ucBuf_ = NULL;
    ucBuf_ = new unsigned char [1000000];
    ASSERT_TRUE (ucBuf_ != NULL);
  }

  void TearDown() {
    EncodeDecodeTestBase::TearDown();
    if (NULL != ucBuf_) {
      delete[] ucBuf_;
      ucBuf_ = NULL;
    }
    ASSERT_TRUE (ucBuf_ == NULL);
  }

  void prepareParam (int iLayerNum, int iSliceNum, int width, int height, float framerate, SEncParamExt* pParam) {
    memset (pParam, 0, sizeof (SEncParamExt));
    EncodeDecodeTestBase::prepareParam (iLayerNum, iSliceNum,  width, height, framerate, pParam);
  }

  void prepareEncDecParam (const EncodeDecodeFileParamBase EncDecFileParam);
  void EncodeOneFrame() {
    int frameSize = EncPic.iPicWidth * EncPic.iPicHeight * 3 / 2;
    memset (buf_.data(), iRandValue, (frameSize >> 2));
    memset (buf_.data() + (frameSize >> 2), rand() % 256, (frameSize - (frameSize >> 2)));
    int rv = encoder_->EncodeFrame (&EncPic, &info);
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  }
 protected:
  unsigned char* ucBuf_;
};

void DecodeCrashTestAPI::prepareEncDecParam (const EncodeDecodeFileParamBase EncDecFileParam) {
  // for encoder
  // I420: 1(Y) + 1/4(U) + 1/4(V)
  int frameSize = EncDecFileParam.width * EncDecFileParam.height * 3 / 2;

  buf_.SetLength (frameSize);
  ASSERT_TRUE (buf_.Length() == (size_t)frameSize);

  memset (&EncPic, 0, sizeof (SSourcePicture));
  EncPic.iPicWidth = EncDecFileParam.width;
  EncPic.iPicHeight = EncDecFileParam.height;
  EncPic.iColorFormat = videoFormatI420;
  EncPic.iStride[0] = EncPic.iPicWidth;
  EncPic.iStride[1] = EncPic.iStride[2] = EncPic.iPicWidth >> 1;
  EncPic.pData[0] = buf_.data();
  EncPic.pData[1] = EncPic.pData[0] + EncDecFileParam.width * EncDecFileParam.height;
  EncPic.pData[2] = EncPic.pData[1] + (EncDecFileParam.width * EncDecFileParam.height >> 2);

  //for decoder
  memset (&info, 0, sizeof (SFrameBSInfo));

  //set a fixed random value
  iRandValue = rand() % 256;
}

struct EncodeDecodeParamBase {
  int width;
  int height;
  float frameRate;
  int iTarBitrate;
};

#define NUM_OF_POSSIBLE_RESOLUTION (9)
static const EncodeDecodeParamBase kParamArray[] = {
  {160, 90, 6.0f, 250000},
  {90, 160, 6.0f, 250000},
  {320, 180, 12.0f, 500000},
  {180, 320, 12.0f, 500000},
  {480, 270, 12.0f, 600000},
  {270, 480, 12.0f, 600000},
  {640, 360, 24.0f, 800000},
  {360, 640, 24.0f, 800000},
  {1280, 720, 24.0f, 1000000},
};

//#define DEBUG_FILE_SAVE_CRA
TEST_F (DecodeCrashTestAPI, DecoderCrashTest) {
  uint32_t uiGet;
  encoder_->Uninitialize();

  //do tests until crash
  unsigned int uiLoopRound = 0;
  unsigned char* pucBuf = ucBuf_;
  int iDecAuSize;
#ifdef DEBUG_FILE_SAVE_CRA
  //open file to save tested BS
  FILE* fDataFile = fopen ("test_crash.264", "wb");
  FILE* fLenFile = fopen ("test_crash_len.log", "w");
  int iFileSize = 0;
#endif

  //set eCurStrategy for one test
  EParameterSetStrategy eCurStrategy = CONSTANT_ID;
  switch (rand() % 7) {
  case 1:
    eCurStrategy = INCREASING_ID;
    break;
  case 2:
    eCurStrategy = SPS_LISTING;
    break;
  case 3:
    eCurStrategy = SPS_LISTING_AND_PPS_INCREASING;
    break;
  case 6:
    eCurStrategy = SPS_PPS_LISTING;
    break;
  default:
    //using the initial value
    break;
  }

  do {
    int iTotalFrameNum = (rand() % 100) + 1;
    int iSeed = rand() % NUM_OF_POSSIBLE_RESOLUTION;
    EncodeDecodeParamBase p = kParamArray[iSeed];
#ifdef DEBUG_FILE_SAVE_CRA
    printf ("using param set %d in loop %d\n", iSeed, uiLoopRound);
#endif
    //Initialize Encoder
    prepareParam (1, 1, p.width, p.height, p.frameRate, &param_);
    param_.iRCMode = RC_TIMESTAMP_MODE;
    param_.iTargetBitrate = p.iTarBitrate;
    param_.uiIntraPeriod = 0;
    param_.eSpsPpsIdStrategy = eCurStrategy;
    param_.bEnableBackgroundDetection = true;
    param_.bEnableSceneChangeDetect = (rand() % 3) ? true : false;
    param_.bPrefixNalAddingCtrl = (rand() % 2) ? true : false;
    param_.iEntropyCodingModeFlag = 0;
    param_.bEnableFrameSkip = true;
    param_.iMultipleThreadIdc = 0;
    param_.sSpatialLayers[0].iSpatialBitrate = p.iTarBitrate;
    param_.sSpatialLayers[0].iMaxSpatialBitrate = p.iTarBitrate << 1;
    param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = (rand() % 2) ? SM_DYN_SLICE : SM_SINGLE_SLICE;
    if (param_.sSpatialLayers[0].sSliceCfg.uiSliceMode == SM_DYN_SLICE) {
      param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 1400;
      param_.uiMaxNalSize = 1400;
    } else {
      param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceSizeConstraint = 0;
      param_.uiMaxNalSize = 0;
    }

    int rv = encoder_->InitializeExt (&param_);
    ASSERT_TRUE (rv == cmResultSuccess);
    decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
    EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY); //default value should be ERROR_CON_SLICE_COPY
    int32_t iTraceLevel = WELS_LOG_QUIET;
    encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
    decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

    //Start for enc/dec
    int iIdx = 0;
    unsigned char* pData[3] = { NULL };

    EncodeDecodeFileParamBase pInput; //to conform with old functions
    pInput.width =  p.width;
    pInput.height = p.height;
    pInput.frameRate = p.frameRate;
    prepareEncDecParam (pInput);
    while (iIdx++ < iTotalFrameNum) { // loop in frame
      EncodeOneFrame();
#ifdef DEBUG_FILE_SAVE_CRA
      //reset file if file size large
      if ((info.eFrameType == videoFrameTypeIDR) && (iFileSize >= (1 << 25))) {
        fclose (fDataFile);
        fDataFile = fopen ("test_crash.264", "wb");
        iFileSize = 0;
        decoder_->Uninitialize();

        SDecodingParam decParam;
        memset (&decParam, 0, sizeof (SDecodingParam));
        decParam.eOutputColorFormat = videoFormatI420;
        decParam.uiTargetDqLayer = UCHAR_MAX;
        decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
        decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

        rv = decoder_->Initialize (&decParam);
        ASSERT_EQ (0, rv);
      }
#endif
      if (info.eFrameType == videoFrameTypeSkip)
        continue;
      //deal with packets
      unsigned char* pBsBuf;
      iDecAuSize = 0;
      pucBuf = ucBuf_; //init buf start pos for decoder usage
      for (int iLayerNum = 0; iLayerNum < info.iLayerNum; iLayerNum++) {
        SLayerBSInfo* pLayerBsInfo = &info.sLayerInfo[iLayerNum];
        pBsBuf = info.sLayerInfo[iLayerNum].pBsBuf;
        int iTotalNalCnt = pLayerBsInfo->iNalCount;
        for (int iNalCnt = 0; iNalCnt < iTotalNalCnt; iNalCnt++) {  //loop in NAL
          int iPacketSize = pLayerBsInfo->pNalLengthInByte[iNalCnt];
          //packet loss
          int iLossRateRange = (uiLoopRound % 100) + 1; //1-100
          int iLossRate = (rand() % iLossRateRange);
          bool bPacketLost = (rand() % 101) > (100 -
                                               iLossRate);   // [0, (100-iLossRate)] indicates NO LOSS, (100-iLossRate, 100] indicates LOSS
          if (!bPacketLost) { //no loss
            memcpy (pucBuf, pBsBuf, iPacketSize);
            pucBuf += iPacketSize;
            iDecAuSize += iPacketSize;
          }
#ifdef DEBUG_FILE_SAVE_CRA
          else {
            printf ("lost packet size=%d at frame-type=%d at loss rate %d (%d)\n", iPacketSize, info.eFrameType, iLossRate,
                    iLossRateRange);
          }
#endif
          //update bs info
          pBsBuf += iPacketSize;
        } //nal
      } //layer

#ifdef DEBUG_FILE_SAVE_CRA
      //save to file
      fwrite (ucBuf_, 1, iDecAuSize, fDataFile);
      fflush (fDataFile);
      iFileSize += iDecAuSize;

      //save to len file
      unsigned long ulTmp[4];
      ulTmp[0] = ulTmp[1] = ulTmp[2] = iIdx;
      ulTmp[3] = iDecAuSize;
      fwrite (ulTmp, sizeof (unsigned long), 4, fLenFile); // index, timeStamp, data size
      fflush (fLenFile);
#endif

      //decode
      pData[0] = pData[1] = pData[2] = 0;
      memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

      rv = decoder_->DecodeFrame2 (ucBuf_, iDecAuSize, pData, &dstBufInfo_);
      rv = decoder_->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_); //reconstruction
      //guarantee decoder EC status
      decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
      EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_SLICE_COPY);
    } //frame
    uiLoopRound ++;
    if (uiLoopRound >= (1 << 30))
      uiLoopRound = 0;
#ifdef DEBUG_FILE_SAVE_CRA
    if (uiLoopRound % 100 == 0)
      printf ("run %d times.\n", uiLoopRound);
  } while (1); //while (iLoopRound<100);
  fclose (fDataFile);
  fclose (fLenFile);
#else
  }
  while (uiLoopRound < 10);
#endif

}

const uint32_t kiTotalLayer = 3; //DO NOT CHANGE!
const uint32_t kiSliceNum = 2; //DO NOT CHANGE!
const uint32_t kiWidth = 160; //DO NOT CHANGE!
const uint32_t kiHeight = 96; //DO NOT CHANGE!
const uint32_t kiFrameRate = 12; //DO NOT CHANGE!
const uint32_t kiFrameNum = 100; //DO NOT CHANGE!
const char* pHashStr[] = { //DO NOT CHANGE!
  "585663f78cadb70d9c9f179b9b53b90ffddf3178",
  "f350001c333902029800bd291fbed915a4bdf19a",
  "eb9d853b7daec03052c4850027ac94adc84c3a7e"
};

class DecodeParseAPI : public ::testing::TestWithParam<EncodeDecodeFileParamBase>, public EncodeDecodeTestBase {
 public:
  void SetUp() {
    SHA1Reset (&ctx_);
    EncodeDecodeTestBase::SetUp();

    if (decoder_)
      decoder_->Uninitialize();
    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.eOutputColorFormat = videoFormatRGB;
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.bParseOnly = true;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    int rv = decoder_->Initialize (&decParam);
    ASSERT_EQ (0, rv);
    memset (&BsInfo_, 0, sizeof (SParserBsInfo));
    fYuv_ = fopen ("./res/CiscoVT2people_160x96_6fps.yuv", "rb");
    ASSERT_TRUE (fYuv_ != NULL);
    iWidth_ = kiWidth;
    iHeight_ = kiHeight;
  }
  void TearDown() {
    EncodeDecodeTestBase::TearDown();
    fclose (fYuv_);
  }

  void prepareEncDecParam (const EncodeDecodeFileParamBase p) {
    EncodeDecodeTestBase::prepareEncDecParam (p);
    unsigned char* pTmpPtr = BsInfo_.pDstBuff; //store for restore
    memset (&BsInfo_, 0, sizeof (SParserBsInfo));
    BsInfo_.pDstBuff = pTmpPtr;
  }

  void EncodeOneFrame (int iIdx) {
    int iFrameSize = iWidth_ * iHeight_ * 3 / 2;
    int iSize = (int) fread (buf_.data(), sizeof (char), iFrameSize, fYuv_);
    if (feof (fYuv_) || iSize != iFrameSize) {
      rewind (fYuv_);
      iSize = (int) fread (buf_.data(), sizeof (char), iFrameSize, fYuv_);
      ASSERT_TRUE (iSize == iFrameSize);
    }
    int rv = encoder_->EncodeFrame (&EncPic, &info);
    ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason);
  }

  void prepareParam (int iLayerNum, int iSliceNum, int width, int height, float framerate, SEncParamExt* pParam) {
    memset (pParam, 0, sizeof (SEncParamExt));
    EncodeDecodeTestBase::prepareParam (iLayerNum, iSliceNum,  width, height, framerate, pParam);
  }

 protected:
  SParserBsInfo BsInfo_;
  FILE* fYuv_;
  int iWidth_;
  int iHeight_;
  SHA1Context ctx_;
};

//#define DEBUG_FILE_SAVE
TEST_F (DecodeParseAPI, ParseOnly_General) {
  EncodeDecodeFileParamBase p;
  p.width = iWidth_;
  p.height = iHeight_;
  p.frameRate = kiFrameRate;
  p.numframes = kiFrameNum;
  prepareParam (kiTotalLayer, kiSliceNum, p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = kiTotalLayer;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == 0);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  uint32_t uiTargetLayerId = rand() % kiTotalLayer; //run only once
#ifdef DEBUG_FILE_SAVE
  FILE* fDec = fopen ("output.264", "wb");
  FILE* fEnc = fopen ("enc.264", "wb");
  FILE* fExtract = fopen ("extract.264", "wb");
#endif
  if (uiTargetLayerId < kiTotalLayer) { //should always be true
    //Start for enc
    int iLen = 0;
    prepareEncDecParam (p);
    int iFrame = 0;

    while (iFrame < p.numframes) {
      //encode
      EncodeOneFrame (iFrame);
      //extract target layer data
      encToDecData (info, iLen);
#ifdef DEBUG_FILE_SAVE
      fwrite (info.sLayerInfo[0].pBsBuf, iLen, 1, fEnc);
#endif
      ExtractDidNal (&info, iLen, &m_SLostSim, uiTargetLayerId);
#ifdef DEBUG_FILE_SAVE
      fwrite (info.sLayerInfo[0].pBsBuf, iLen, 1, fExtract);
#endif
      //parseonly
      //BsInfo_.pDstBuff = new unsigned char [1000000];
      rv = decoder_->DecodeParser (info.sLayerInfo[0].pBsBuf, iLen, &BsInfo_);
      EXPECT_TRUE (rv == 0);
      EXPECT_TRUE (BsInfo_.iNalNum == 0);
      rv = decoder_->DecodeParser (NULL, 0, &BsInfo_);
      EXPECT_TRUE (rv == 0);
      EXPECT_TRUE (BsInfo_.iNalNum != 0);
      //get final output bs
      iLen = 0;
      int i = 0;
      while (i < BsInfo_.iNalNum) {
        iLen += BsInfo_.iNalLenInByte[i];
        i++;
      }
#ifdef DEBUG_FILE_SAVE
      fwrite (BsInfo_.pDstBuff, iLen, 1, fDec);
#endif
      SHA1Input (&ctx_, BsInfo_.pDstBuff, iLen);
      iFrame++;
    }
    //calculate final SHA1 value
    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1Result (&ctx_, digest);
    if (!HasFatalFailure()) {
      CompareHash (digest, pHashStr[uiTargetLayerId]);
    }
  } //while
#ifdef DEBUG_FILE_SAVE
  fclose (fEnc);
  fclose (fExtract);
  fclose (fDec);
#endif
}

//This case is for one layer only, for incomplete frame input
//First slice is loss for random one picture with 2 slices per pic
TEST_F (DecodeParseAPI, ParseOnly_SpecSliceLoss) {
  int32_t iLayerNum = 1;
  int32_t iSliceNum = 2;
  EncodeDecodeFileParamBase p;
  p.width = iWidth_;
  p.height = iHeight_;
  p.frameRate = kiFrameRate;
  p.numframes = 5;
  prepareParam (iLayerNum, iSliceNum, p.width, p.height, p.frameRate, &param_);
  param_.iSpatialLayerNum = iLayerNum;
  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == 0);
  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);

  int32_t iMissedPicNum = rand() % (p.numframes - 1) + 1; //IDR no loss
  //Start for enc
  int iLen = 0;
  uint32_t uiGet;
  prepareEncDecParam (p);
  int iFrame = 0;

  while (iFrame < p.numframes) {
    //encode
    EncodeOneFrame (iFrame);
    //parseonly
    if (iFrame == iMissedPicNum) { //make current frame partly missing
      //Frame: P, first slice loss
      int32_t iTotalSliceSize = 0;
      encToDecSliceData (0, 0, info, iTotalSliceSize); //slice 1 lost
      encToDecSliceData (0, 1, info, iLen); //slice 2
      decoder_->GetOption (DECODER_OPTION_ERROR_CON_IDC, &uiGet);
      EXPECT_EQ (uiGet, (uint32_t) ERROR_CON_DISABLE);
      rv = decoder_->DecodeParser (info.sLayerInfo[0].pBsBuf + iTotalSliceSize, iLen, &BsInfo_);
      EXPECT_TRUE (rv == 0);
      EXPECT_TRUE (BsInfo_.iNalNum == 0);
      rv = decoder_->DecodeParser (NULL, 0, &BsInfo_);
      EXPECT_TRUE (rv != 0);
    } else { //normal frame, complete
      encToDecData (info, iLen);
      rv = decoder_->DecodeParser (info.sLayerInfo[0].pBsBuf, iLen, &BsInfo_);
      EXPECT_TRUE (rv == 0); //parse correct
      EXPECT_TRUE (BsInfo_.iNalNum == 0);
      rv = decoder_->DecodeParser (NULL, 0, &BsInfo_);
      if (iFrame < iMissedPicNum) { //correct frames, all OK with output
        EXPECT_TRUE (rv == 0);
        EXPECT_TRUE (BsInfo_.iNalNum != 0);
      } else { //(iFrame > iMissedPicNum), should output nothing as error
        EXPECT_TRUE (rv != 0);
        EXPECT_TRUE (BsInfo_.iNalNum == 0);
      }
    }
    iFrame++;
  } //while
}

//#define DEBUG_FILE_SAVE2
TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_LISTING_AND_PPS_INCREASING1) {

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  SEncParamExt   sParam3;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam2.iPicWidth) == GET_MB_WIDTH (sParam1.iPicWidth)) {
    sParam2.iPicWidth = GetRandWidth();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
  sParam2.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;
  //prepare param3
  memcpy (&sParam3, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam3.iPicHeight) == GET_MB_WIDTH (sParam1.iPicHeight)) {
    sParam3.iPicHeight = GetRandHeight();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam3.iPicWidth, sParam3.iPicHeight, fFrameRate, &sParam3);
  sParam3.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE2
  fEnc = fopen ("enc2.264", "wb");
#endif

  // Test part#1
  // step#1: pParam1
  //int TraceLevel = WELS_LOG_INFO;
  //encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &TraceLevel);
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // step#2: pParam2
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption: rv = " << rv << " at " << sParam2.iPicWidth << "x" <<
                                      sParam2.iPicHeight;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // step#3: back to pParam1
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // step#4: back to pParam2
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv << sParam2.iPicWidth << sParam2.iPicHeight;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

#ifdef DEBUG_FILE_SAVE2
  fclose (fEnc);
#endif
  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

  // Test part#2
  // step#1: pParam1
  rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt Failed: rv = " << rv;
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam3);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam3: rv = " << rv;

#ifdef DEBUG_FILE_SAVE2
  fEnc = fopen ("enc3.264", "wb");
#endif
  iEncFrameNum = 0;
  EncDecOneFrame (sParam3.iPicWidth, sParam3.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

#ifdef DEBUG_FILE_SAVE2
  fclose (fEnc);
#endif
  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
}

//#define DEBUG_FILE_SAVE5
TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_LISTING_AND_PPS_INCREASING2) {
  //usage 3: 2 Params with different num_ref, encode IDR0, P1, IDR2;
  //the bs will show two SPS and different PPS

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;
  sParam1.iTemporalLayerNum = 1;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
  sParam2.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;
  sParam2.iTemporalLayerNum = 3;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE5
  fEnc = fopen ("encID2.264", "wb");
#endif

  // step#1: pParam1
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;

  // step#2: pParam2
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;

  // step#3: set back to pParam1, with a smaller num_ref, it still uses the previous SPS
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam1: rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR, PPS increases
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

#ifdef DEBUG_FILE_SAVE5
  fclose (fEnc);
#endif
}

TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_LISTING_AND_PPS_INCREASING3) {

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE2
  fEnc = fopen ("enc4.264", "wb");
#endif

  // step#1: pParam1
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt Failed: rv = " << rv;

  int max_count = 65; // make it more then twice as MAX_SPS_COUNT
  std::vector<int> vWidthTable;
  vWidthTable.push_back (GET_MB_WIDTH (sParam1.iPicWidth));

  std::vector<int>::iterator vWidthTableIt;
  for (int times = 0; times < max_count; times++) {
    //prepare param2
    memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
    do {
      sParam2.iPicWidth = GetRandWidth();
      vWidthTableIt = std::find (vWidthTable.begin(), vWidthTable.end(), GET_MB_WIDTH (sParam2.iPicWidth));
    } while (vWidthTableIt != vWidthTable.end());
    vWidthTable.push_back (GET_MB_WIDTH (sParam2.iPicWidth));
    prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
    sParam2.eSpsPpsIdStrategy = SPS_LISTING_AND_PPS_INCREASING;

    rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
    ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv << ", sParam2.iPicWidth=" <<
                                        sParam2.iPicWidth;
  } // end of setting loop

  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

#ifdef DEBUG_FILE_SAVE2
  fclose (fEnc);
#endif
  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
}

//#define DEBUG_FILE_SAVE6
TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_PPS_LISTING1) {
  //usage 1: 1 resolution Params, encode IDR0, P1, IDR2;
  //the bs will show same SPS and different PPS
  // PPS: pic_parameter_set_id                                     1 (  0)
  // PPS: seq_parameter_set_id                                     1 (  0)
  // PPS: pic_parameter_set_id                                   010 (  1)
  // PPS: seq_parameter_set_id                                     1 (  0)
  // SH: slice_type                                              011 (  2)
  // SH: pic_parameter_set_id                                      1 (  0)
  // SH: slice_type                                                1 (  0)
  // SH: pic_parameter_set_id                                      1 (  0)
  // SH: slice_type                                              011 (  2)
  // SH: pic_parameter_set_id                                    010 (  1)
  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_PPS_LISTING;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE6
  fEnc = fopen ("encLIST1.264", "wb");
#endif

  // step#1: pParam1
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

#ifdef DEBUG_FILE_SAVE6
  fclose (fEnc);
#endif
}

TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_PPS_LISTING2) {
  //usage 2: 2 resolution Params, encode IDR0, IDR1, IDR2;
  //the bs will show two SPS and different PPS
  // === SPS LIST ===
  //SPS: seq_parameter_set_id                                     1 (  0) -- PARAM1
  //SPS: seq_parameter_set_id                                   010 (  1) -- PARAM2
  // === PPS LIST ===
  //PPS: pic_parameter_set_id                                     1 (  0)
  //PPS: seq_parameter_set_id                                     1 (  0)
  //PPS: pic_parameter_set_id                                   010 (  1)
  //PPS: seq_parameter_set_id                                   010 (  1)
  //PPS: pic_parameter_set_id                                   011 (  2) -- PPS2 - SPS0
  //PPS: seq_parameter_set_id                                     1 (  0)
  //PPS: pic_parameter_set_id                                 00100 (  3) -- PPS3 - SPS1
  //PPS: seq_parameter_set_id                                   010 (  1)
  //PPS: pic_parameter_set_id                                 00101 (  4) -- PPS4 - SPS0
  //PPS: seq_parameter_set_id                                     1 (  0)
  // === VCL LAYER ===
  //SH: slice_type                                              011 (  2) -- PARAM2
  //SH: pic_parameter_set_id                                    010 (  1) -- PPS1 - SPS1 - PARAM2
  //SH: slice_type                                              011 (  2) -- PARAM1
  //SH: pic_parameter_set_id                                    011 (  2) -- PPS2 - SPS0 - PARAM1
  //SH: slice_type                                             011 (  2) -- PARAM1
  //SH: pic_parameter_set_id                                 00101 (  4) -- PPS4 - SPS0 - PARAM1

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_PPS_LISTING;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam2.iPicWidth) == GET_MB_WIDTH (sParam1.iPicWidth)) {
    sParam2.iPicWidth = GetRandWidth();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
  sParam2.eSpsPpsIdStrategy = SPS_PPS_LISTING;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE5
  fEnc = fopen ("encLIST2.264", "wb");
#endif

  // step#1: pParam1
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;

  // step#2: pParam2
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // step#3: back to pParam1, SHOULD NOT encounter ERROR
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  // new IDR
  rv = encoder_->ForceIntraFrame (true);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;
  EncDecOneFrame (sParam1.iPicWidth, sParam1.iPicHeight, iEncFrameNum++, fEnc);

  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

#ifdef DEBUG_FILE_SAVE5
  fclose (fEnc);
#endif
}

TEST_F (EncodeDecodeTestAPI, ParameterSetStrategy_SPS_PPS_LISTING3) {

  int iWidth       = GetRandWidth();
  int iHeight      = GetRandHeight();
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = 0;
  int iSpatialLayerNum = 1;
  int iSliceNum        = 1;

  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  SEncParamExt   sParam3;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  sParam1.eSpsPpsIdStrategy = SPS_PPS_LISTING;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam2.iPicWidth) == GET_MB_WIDTH (sParam1.iPicWidth)) {
    sParam2.iPicWidth = GetRandWidth();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam2.iPicWidth, sParam2.iPicHeight, fFrameRate, &sParam2);
  sParam2.eSpsPpsIdStrategy = SPS_PPS_LISTING;
  //prepare param3
  memcpy (&sParam3, &sParam1, sizeof (SEncParamExt));
  while (GET_MB_WIDTH (sParam3.iPicWidth) == GET_MB_WIDTH (sParam1.iPicWidth) ||
         GET_MB_WIDTH (sParam3.iPicWidth) == GET_MB_WIDTH (sParam2.iPicWidth)) {
    sParam3.iPicWidth = GetRandWidth();
  }
  prepareParamDefault (iSpatialLayerNum, iSliceNum, sParam3.iPicWidth, sParam3.iPicHeight, fFrameRate, &sParam3);
  sParam3.eSpsPpsIdStrategy = SPS_PPS_LISTING;

  //prepare output if needed
  FILE* fEnc =  NULL;
#ifdef DEBUG_FILE_SAVE5
  fEnc = fopen ("enc4.264", "wb");
#endif

  // step#1: ordinary encoding
  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "InitializeExt: rv = " << rv << " at " << sParam1.iPicWidth << "x" <<
                                      sParam1.iPicHeight;
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;
  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // step#2: set strategy for success
  int32_t iNewStra = SPS_PPS_LISTING;
  rv = encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iNewStra);
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv << " iNewStra=" << iNewStra;

  // step#3: setting new strategy, SHOULD encounter ERROR
  unsigned int TraceLevel = WELS_LOG_QUIET;
  rv = encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &TraceLevel);
  ASSERT_TRUE (rv == cmResultSuccess);
  iNewStra = CONSTANT_ID;
  rv = encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iNewStra);
  ASSERT_TRUE (rv != cmResultSuccess);

  EncDecOneFrame (sParam2.iPicWidth, sParam2.iPicHeight, iEncFrameNum++, fEnc);

  // step#4: pParam3, SHOULD encounter ERROR
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam3);
  ASSERT_TRUE (rv != cmResultSuccess) << "SetOption: rv = " << rv << " at " << sParam3.iPicWidth << "x" <<
                                      sParam3.iPicHeight;

  rv = encoder_->Uninitialize();
  ASSERT_TRUE (rv == cmResultSuccess) << "rv = " << rv;

#ifdef DEBUG_FILE_SAVE5
  fclose (fEnc);
#endif
}


TEST_F (EncodeDecodeTestAPI, SimulcastSVC) {
  int iSpatialLayerNum = WelsClip3 ((rand() % MAX_SPATIAL_LAYER_NUM), 2, MAX_SPATIAL_LAYER_NUM);
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 16, MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, 16, MAX_HEIGHT);
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);
  int iSliceNum        = 1;
  encoder_->GetDefaultParams (&param_);
  prepareParam (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &param_);

  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  int aLen[MAX_SPATIAL_LAYER_NUM] = {0};
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

#ifdef DEBUG_FILE_SAVE2
  FILE* fEnc[MAX_SPATIAL_LAYER_NUM] = { NULL };
  fEnc[0] = fopen ("enc0.264", "wb");
  fEnc[1] = fopen ("enc1.264", "wb");
  fEnc[2] = fopen ("enc2.264", "wb");
  fEnc[3] = fopen ("enc3.264", "wb");
#endif

  // init decoders
  int iIdx = 0;
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);
    aLen[iIdx] = 0;

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.eOutputColorFormat  = videoFormatI420;
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

  for (int iFrame = 0; iFrame < iEncFrameNum; iFrame++) {
    int iResult;
    int iLayerLen = 0;
    unsigned char* pData[3] = { NULL };

    InitialEncDec (param_.iPicWidth, param_.iPicHeight);
    EncodeOneFrame (0);

    iLayerLen = 0;
    for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
      aLen[iIdx] = 0;
    }
    for (int iLayer = 0; iLayer < info.iLayerNum; ++iLayer) {
      iLayerLen = 0;
      const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayer];
      for (int iNal = 0; iNal < layerInfo.iNalCount; ++iNal) {
        iLayerLen += layerInfo.pNalLengthInByte[iNal];
      }

      if (layerInfo.uiLayerType == NON_VIDEO_CODING_LAYER) {
        // under SimulcastSVC, need to copy non-VCL to all layers
        for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
          memcpy ((pBsBuf[iIdx] + aLen[iIdx]), layerInfo.pBsBuf, iLayerLen * sizeof (unsigned char));
          aLen[iIdx] += iLayerLen;
        }
      } else {
        iIdx = layerInfo.uiSpatialId;
        EXPECT_TRUE (iIdx < iSpatialLayerNum);
        memcpy ((pBsBuf[iIdx] + aLen[iIdx]), layerInfo.pBsBuf, iLayerLen * sizeof (unsigned char));
        aLen[iIdx] += iLayerLen;
      }
    }

    for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
      pData[0] = pData[1] = pData[2] = 0;
      memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

#ifdef DEBUG_FILE_SAVE2
      fwrite (pBsBuf[iIdx], aLen[iIdx], 1, fEnc[iIdx]);
#endif
      iResult = decoder[iIdx]->DecodeFrame2 (pBsBuf[iIdx], aLen[iIdx], pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;
      iResult = decoder[iIdx]->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;
      EXPECT_EQ (dstBufInfo_.iBufferStatus, 1) << "LayerIdx=" << iIdx;
    }
  }

  // free all
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    if (pBsBuf[iIdx]) {
      free (pBsBuf[iIdx]);
    }

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }

#ifdef DEBUG_FILE_SAVE2
    fclose (fEnc[iIdx]);
#endif
  }

}

TEST_F (EncodeDecodeTestAPI, SimulcastAVC) {
//#define DEBUG_FILE_SAVE3
  int iSpatialLayerNum = WelsClip3 ((rand() % MAX_SPATIAL_LAYER_NUM), 2, MAX_SPATIAL_LAYER_NUM);
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 16, MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, 16, MAX_HEIGHT);
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);
  int iSliceNum        = 1;
  encoder_->GetDefaultParams (&param_);
  prepareParam (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &param_);

  //set flag of bSimulcastAVC
  param_.bSimulcastAVC = true;

  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  int aLen[MAX_SPATIAL_LAYER_NUM] = {0};
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

#ifdef DEBUG_FILE_SAVE3
  FILE* fEnc[MAX_SPATIAL_LAYER_NUM];
  fEnc[0] = fopen ("enc0.264", "wb");
  fEnc[1] = fopen ("enc1.264", "wb");
  fEnc[2] = fopen ("enc2.264", "wb");
  fEnc[3] = fopen ("enc3.264", "wb");
#endif

  int iIdx = 0;

  //create decoder
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);
    aLen[iIdx] = 0;

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.eOutputColorFormat  = videoFormatI420;
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

  iEncFrameNum = 10;
  for (int iFrame = 0; iFrame < iEncFrameNum; iFrame++) {
    int iResult;
    int iLayerLen = 0;
    unsigned char* pData[3] = { NULL };

    InitialEncDec (param_.iPicWidth, param_.iPicHeight);
    EncodeOneFrame (0);

    // init
    for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
      aLen[iIdx] = 0;
    }
    for (int iLayer = 0; iLayer < info.iLayerNum; ++iLayer) {
      iLayerLen = 0;
      const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayer];
      const int kiFirstNalType = ((* (layerInfo.pBsBuf + 4)) & 0x1f);
      ASSERT_TRUE ((kiFirstNalType == NAL_SPS) || (kiFirstNalType == NAL_PPS) || (kiFirstNalType == NAL_SLICE)
                   || (kiFirstNalType == NAL_SLICE_IDR) || (kiFirstNalType == NAL_SEI));
      for (int iNal = 0; iNal < layerInfo.iNalCount; ++iNal) {
        iLayerLen += layerInfo.pNalLengthInByte[iNal];
      }

      iIdx = layerInfo.uiSpatialId;
      EXPECT_TRUE (iIdx < iSpatialLayerNum);
      memcpy ((pBsBuf[iIdx] + aLen[iIdx]), layerInfo.pBsBuf, iLayerLen * sizeof (unsigned char));
      aLen[iIdx] += iLayerLen;
    }

    for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
      pData[0] = pData[1] = pData[2] = 0;
      memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

#ifdef DEBUG_FILE_SAVE3
      fwrite (pBsBuf[iIdx], aLen[iIdx], 1, fEnc[iIdx]);
#endif
      iResult = decoder[iIdx]->DecodeFrame2 (pBsBuf[iIdx], aLen[iIdx], pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;

      iResult = decoder[iIdx]->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
      EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;
      EXPECT_EQ (dstBufInfo_.iBufferStatus, 1) << "LayerIdx=" << iIdx;
    }
  }

  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    free (pBsBuf[iIdx]);

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }
  }
#ifdef DEBUG_FILE_SAVE_SimulcastAVCDiffFps
  for (int i = 0; i <)MAX_SPATIAL_LAYER_NUM;
  i++) {
    fclose (fEnc[i]);
  }
#endif
}

TEST_F (EncodeDecodeTestAPI, SimulcastAVC_SPS_PPS_LISTING) {
  int iSpatialLayerNum = WelsClip3 ((rand() % MAX_SPATIAL_LAYER_NUM), 2, MAX_SPATIAL_LAYER_NUM);;
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_HEIGHT);
  float fFrameRate = rand() + 0.5f;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);
  int iSliceNum        = 1;
  iWidth = VALID_SIZE(iWidth);
  iHeight = VALID_SIZE(iHeight);
  // prepare params
  SEncParamExt   sParam1;
  SEncParamExt   sParam2;
  encoder_->GetDefaultParams (&sParam1);
  prepareParamDefault (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &sParam1);
  //set flag of SPS_PPS_LISTING
  sParam1.eSpsPpsIdStrategy = SPS_PPS_LISTING;//SPS_LISTING;//
  //set flag of bSimulcastAVC
  sParam1.bSimulcastAVC = true;
  //prepare param2
  memcpy (&sParam2, &sParam1, sizeof (SEncParamExt));
  sParam2.sSpatialLayers[0].iVideoWidth = (sParam1.sSpatialLayers[0].iVideoWidth / 2);
  sParam2.sSpatialLayers[0].iVideoHeight = (sParam1.sSpatialLayers[0].iVideoHeight / 2);

  int rv = encoder_->InitializeExt (&sParam1);
  ASSERT_TRUE (rv == cmResultSuccess) << "Init Failed sParam1: rv = " << rv;;
  rv = encoder_->SetOption (ENCODER_OPTION_SVC_ENCODE_PARAM_EXT, &sParam2);
  ASSERT_TRUE (rv == cmResultSuccess) << "SetOption Failed sParam2: rv = " << rv;

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

  int iIdx = 0;

  //create decoder
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.eOutputColorFormat  = videoFormatI420;
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

  TestOneSimulcastAVC (&sParam1, decoder, pBsBuf, iSpatialLayerNum, iEncFrameNum, 0);
  TestOneSimulcastAVC (&sParam2, decoder, pBsBuf, iSpatialLayerNum, iEncFrameNum, 1);

  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    free (pBsBuf[iIdx]);

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }

  }
}

struct EncodeOptionParam {
  bool bTestNalSize;
  bool bAllRandom;
  bool bTestDecoder;
  int iNumframes;
  int iWidth;
  int iHeight;
  int iQp;
  SliceModeEnum eSliceMode;
  int uiMaxNalLen;
  float fFramerate;
  int iThreads;
  const char* sFileSave;
};

static const EncodeOptionParam kOptionParamArray[] = {
  {true, true, false, 30, 600, 460, 1, SM_DYN_SLICE, 450, 15.0, 1, ""},
  {true, true, false, 30, 340, 96, 24, SM_DYN_SLICE, 1000, 30.0, 1, ""},
  {true, true, false, 30, 140, 196, 51, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, true, false, 30, 110, 296, 50, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, true, false, 30, 104, 416, 44, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, true, false, 30, 16, 16, 2, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, false, true, 30, 600, 460, 1, SM_DYN_SLICE, 450, 15.0, 1, ""},
  {true, false, true, 30, 340, 96, 24, SM_DYN_SLICE, 1000, 30.0, 1, ""},
  {true, false, true, 30, 140, 196, 51, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, false, true, 30, 110, 296, 50, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, false, true, 30, 104, 416, 44, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, false, true, 30, 16, 16, 2, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, true, true, 30, 600, 460, 1, SM_DYN_SLICE, 450, 15.0, 1, ""},
  {true, true, true, 30, 340, 96, 24, SM_DYN_SLICE, 1000, 30.0, 1, ""},
  {true, true, true, 30, 140, 196, 51, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, true, true, 30, 110, 296, 50, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, true, true, 30, 104, 416, 44, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {true, true, true, 30, 16, 16, 2, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {false, false, true, 3, 4096, 2304, 2, SM_SINGLE_SLICE, 0, 7.5, 1, ""}, // large picture size
  {false, true, false, 30, 32, 16, 2, SM_DYN_SLICE, 500, 7.5, 1, ""},
  {false, true, false, 30, 600, 460, 1, SM_DYN_SLICE, 450, 15.0, 4, ""},
  {false, true, false, 30, 340, 96, 24, SM_DYN_SLICE, 1000, 30.0, 2, ""},
  {false, true, false, 30, 140, 196, 51, SM_DYN_SLICE, 500, 7.5, 3, ""},
  {false, true, false, 30, 110, 296, 50, SM_DYN_SLICE, 500, 7.5, 2, ""},
  {false, true, false, 30, 104, 416, 44, SM_DYN_SLICE, 500, 7.5, 2, ""},
  {false, true, false, 30, 16, 16, 2, SM_DYN_SLICE, 500, 7.5, 3, ""},
  {false, true, false, 30, 32, 16, 2, SM_DYN_SLICE, 500, 7.5, 3, ""},
  {false, false, true, 30, 600, 460, 1, SM_FIXEDSLCNUM_SLICE, 0, 15.0, 4, ""},
  {false, false, true, 30, 600, 460, 1, SM_AUTO_SLICE, 0, 15.0, 4, ""},
};

class EncodeTestAPI : public ::testing::TestWithParam<EncodeOptionParam>, public ::EncodeDecodeTestAPIBase {
 public:
  void SetUp() {
    EncodeDecodeTestAPIBase::SetUp();
  }

  void TearDown() {
    EncodeDecodeTestAPIBase::TearDown();
  }
  void EncodeOneFrameRandom (int iCheckTypeIndex, bool bAllRandom) {
    int frameSize = EncPic.iPicWidth * EncPic.iPicHeight * 3 / 2;
    uint8_t* ptr = buf_.data();
    uint8_t uiVal = rand() % 256;
    for (int i = 0; i < frameSize; i++) {
      ptr[i] = bAllRandom ? (rand() % 256) : uiVal;
    }
    int rv = encoder_->EncodeFrame (&EncPic, &info);
    if (0 == iCheckTypeIndex)
      ASSERT_TRUE (rv == cmResultSuccess) << "rv=" << rv;
    else if (1 == iCheckTypeIndex)
      ASSERT_TRUE (rv == cmResultSuccess || rv == cmUnkonwReason) << "rv=" << rv;
  }
};

INSTANTIATE_TEST_CASE_P (EncodeDecodeTestAPIBase, EncodeTestAPI,
                         ::testing::ValuesIn (kOptionParamArray));

TEST_P (EncodeTestAPI, SetEncOptionSize) {
  EncodeOptionParam p = GetParam();
  FILE* pFile = NULL;
  if (p.sFileSave != NULL && strlen (p.sFileSave) > 0) {
    pFile = fopen (p.sFileSave, "wb");
  }
  memset (&param_, 0, sizeof (SEncParamExt));
  encoder_->GetDefaultParams (&param_);
  param_.uiMaxNalSize = p.uiMaxNalLen;
  param_.iTemporalLayerNum = (rand() % 4) + 1;
  param_.iSpatialLayerNum = 1;
  param_.iUsageType = CAMERA_VIDEO_REAL_TIME;
  param_.iPicWidth = p.iWidth;
  param_.iPicHeight = p.iHeight;
  param_.fMaxFrameRate = p.fFramerate;
  param_.iRCMode = RC_OFF_MODE; //rc off
  param_.iMultipleThreadIdc = p.iThreads;
  param_.iNumRefFrame = AUTO_REF_PIC_COUNT;
  param_.sSpatialLayers[0].iVideoWidth = p.iWidth ;
  param_.sSpatialLayers[0].iVideoHeight = p.iHeight;
  param_.sSpatialLayers[0].fFrameRate = p.fFramerate;
  param_.sSpatialLayers[0].sSliceCfg.uiSliceMode = p.eSliceMode;
  if (SM_AUTO_SLICE == p.eSliceMode || SM_FIXEDSLCNUM_SLICE == p.eSliceMode ) {
    param_.sSpatialLayers[0].sSliceCfg.sSliceArgument.uiSliceNum = 8;
  }

  encoder_->Uninitialize();
  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);
  InitialEncDec (p.iWidth, p.iHeight);

  int32_t iTraceLevel = WELS_LOG_QUIET;
  encoder_->SetOption (ENCODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  decoder_->SetOption (DECODER_OPTION_TRACE_LEVEL, &iTraceLevel);
  int32_t iSpsPpsIdAddition = 1;
  encoder_->SetOption (ENCODER_OPTION_ENABLE_SPS_PPS_ID_ADDITION, &iSpsPpsIdAddition);
  int32_t iIDRPeriod = (int32_t) pow (2.0f, (param_.iTemporalLayerNum - 1)) * ((rand() % 5) + 1);
  encoder_->SetOption (ENCODER_OPTION_IDR_INTERVAL, &iIDRPeriod);
  int iIdx = 0;
  int iLen;
  unsigned char* pData[3] = { NULL };

  //FIXME: remove this after the multi-thread case is correctly handled in encoder
  if (p.iThreads>1 && SM_DYN_SLICE == p.eSliceMode) {
    p.bAllRandom = false;
  }

  while (iIdx <= p.iNumframes) {
    EncodeOneFrameRandom (0, p.bAllRandom);
    encToDecData (info, iLen);
    if (pFile) {
      fwrite (info.sLayerInfo[0].pBsBuf, iLen, 1, pFile);
      fflush (pFile);
    }
    memset (&dstBufInfo_, 0, sizeof (SBufferInfo));
    if (iLen && p.bTestDecoder) {
      rv = decoder_->DecodeFrameNoDelay (info.sLayerInfo[0].pBsBuf, iLen, pData, &dstBufInfo_);
      ASSERT_EQ (rv, 0);
      ASSERT_EQ (dstBufInfo_.iBufferStatus, 1);
    }
    int iLayer = 0;
    while (iLayer < info.iLayerNum) {
      SLayerBSInfo* pLayerBsInfo = &info.sLayerInfo[iLayer];
      if (pLayerBsInfo != NULL) {
        int iNalIdx = WELS_MAX (pLayerBsInfo->iNalCount - 2, 0); // ignore last slice under single slice mode
        do {
          if (p.bTestNalSize) { // ignore the case that 2 MBs in one picture, and the multithreads case, enable them when code is ready
            ASSERT_GE (((int)param_.uiMaxNalSize), pLayerBsInfo->pNalLengthInByte[iNalIdx]);
          }
          -- iNalIdx;
        } while (iNalIdx >= 0);
      }
      ++ iLayer;
    }
    iIdx++;
  }
  if (pFile) {
    fclose (pFile);
  }
}



TEST_F (EncodeDecodeTestAPI, SimulcastAVCDiffFps) {
//#define DEBUG_FILE_SAVE3
  int iSpatialLayerNum = WelsClip3 ((rand() % MAX_SPATIAL_LAYER_NUM), 2, MAX_SPATIAL_LAYER_NUM);
  int iWidth       = WelsClip3 ((((rand() % MAX_WIDTH) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_WIDTH);
  int iHeight      = WelsClip3 ((((rand() % MAX_HEIGHT) >> 1)  + 1) << 1, 1 << iSpatialLayerNum, MAX_HEIGHT);
  float fFrameRate = 30;
  int iEncFrameNum = WelsClip3 ((rand() % ENCODE_FRAME_NUM) + 1, 1, ENCODE_FRAME_NUM);
  int iSliceNum        = 1;
  encoder_->GetDefaultParams (&param_);
  prepareParam (iSpatialLayerNum, iSliceNum, iWidth, iHeight, fFrameRate, &param_);

  //set flag of bSimulcastAVC
  param_.bSimulcastAVC = true;
  param_.iTemporalLayerNum = 3;

  int rv = encoder_->InitializeExt (&param_);
  ASSERT_TRUE (rv == cmResultSuccess);

  unsigned char*  pBsBuf[MAX_SPATIAL_LAYER_NUM];
  int aLen[MAX_SPATIAL_LAYER_NUM] = {0};
  ISVCDecoder* decoder[MAX_SPATIAL_LAYER_NUM];

#ifdef DEBUG_FILE_SAVE_SimulcastAVCDiffFps
  FILE* fEnc[MAX_SPATIAL_LAYER_NUM];
  fEnc[0] = fopen ("enc0.264", "wb");
  fEnc[1] = fopen ("enc1.264", "wb");
  fEnc[2] = fopen ("enc2.264", "wb");
  fEnc[3] = fopen ("enc3.264", "wb");
#endif

  int iIdx = 0;

  //create decoder
  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    pBsBuf[iIdx] = static_cast<unsigned char*> (malloc (iWidth * iHeight * 3 * sizeof (unsigned char) / 2));
    EXPECT_TRUE (pBsBuf[iIdx] != NULL);
    aLen[iIdx] = 0;

    long rv = WelsCreateDecoder (&decoder[iIdx]);
    ASSERT_EQ (0, rv);
    EXPECT_TRUE (decoder[iIdx] != NULL);

    SDecodingParam decParam;
    memset (&decParam, 0, sizeof (SDecodingParam));
    decParam.eOutputColorFormat  = videoFormatI420;
    decParam.uiTargetDqLayer = UCHAR_MAX;
    decParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
    decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

    rv = decoder[iIdx]->Initialize (&decParam);
    ASSERT_EQ (0, rv);
  }

#define PATTERN_NUM (16)
  const int32_t iTemporalPattern[PATTERN_NUM][MAX_SPATIAL_LAYER_NUM] = { {2, 1, 1, 1}, {2, 2, 2, 1}, {4, 1, 1, 1}, {4, 2, 1, 1},
    {1, 2, 1, 1}, {1, 1, 2, 1}, {1, 4, 1, 1}, {2, 4, 2, 1}, {1, 4, 2, 1},
    {1, 2, 2, 1}, {1, 2, 4, 1},
    {1, 1, 1, 2}, {1, 2, 2, 2}, {1, 2, 2, 4}, {1, 2, 4, 2}, {1, 4, 4, 4},
  };
  for (int iPatternIdx = 0; iPatternIdx < PATTERN_NUM; iPatternIdx++) {
    for (int i = 0; i < iSpatialLayerNum; i++) {
      param_.sSpatialLayers[i].fFrameRate = (fFrameRate / iTemporalPattern[iPatternIdx][i]);
    }

    iEncFrameNum = 10;
    for (int iFrame = 0; iFrame < iEncFrameNum; iFrame++) {
      int iResult;
      int iLayerLen = 0;
      unsigned char* pData[3] = { NULL };

      InitialEncDec (param_.iPicWidth, param_.iPicHeight);
      EncodeOneFrame (0);

      // init
      for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
        aLen[iIdx] = 0;
      }
      for (int iLayer = 0; iLayer < info.iLayerNum; ++iLayer) {
        iLayerLen = 0;
        const SLayerBSInfo& layerInfo = info.sLayerInfo[iLayer];
        const int kiFirstNalType = ((* (layerInfo.pBsBuf + 4)) & 0x1f);
        ASSERT_TRUE ((kiFirstNalType == NAL_SPS) || (kiFirstNalType == NAL_PPS) || (kiFirstNalType == NAL_SLICE)
                     || (kiFirstNalType == NAL_SLICE_IDR) || (kiFirstNalType == NAL_SEI));
        for (int iNal = 0; iNal < layerInfo.iNalCount; ++iNal) {
          iLayerLen += layerInfo.pNalLengthInByte[iNal];
        }

        iIdx = layerInfo.uiSpatialId;
        EXPECT_TRUE (iIdx < iSpatialLayerNum);
        memcpy ((pBsBuf[iIdx] + aLen[iIdx]), layerInfo.pBsBuf, iLayerLen * sizeof (unsigned char));
        aLen[iIdx] += iLayerLen;
      }

      for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
        pData[0] = pData[1] = pData[2] = 0;
        memset (&dstBufInfo_, 0, sizeof (SBufferInfo));

        if (aLen[iIdx] > 0) {
#ifdef DEBUG_FILE_SAVE_SimulcastAVCDiffFps
          fwrite (pBsBuf[iIdx], aLen[iIdx], 1, fEnc[iIdx]);
#endif
          iResult = decoder[iIdx]->DecodeFrame2 (pBsBuf[iIdx], aLen[iIdx], pData, &dstBufInfo_);
          EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;

          iResult = decoder[iIdx]->DecodeFrame2 (NULL, 0, pData, &dstBufInfo_);
          EXPECT_TRUE (iResult == cmResultSuccess) << "iResult=" << iResult << "LayerIdx=" << iIdx;
          EXPECT_EQ (dstBufInfo_.iBufferStatus, 1) << "LayerIdx=" << iIdx;
        }
      }
    }
  }

  for (iIdx = 0; iIdx < iSpatialLayerNum; iIdx++) {
    free (pBsBuf[iIdx]);

    if (decoder[iIdx] != NULL) {
      decoder[iIdx]->Uninitialize();
      WelsDestroyDecoder (decoder[iIdx]);
    }
  }
#ifdef DEBUG_FILE_SAVE_SimulcastAVCDiffFps
  for (int i = 0; i <)MAX_SPATIAL_LAYER_NUM;
  i++) {
    fclose (fEnc[i]);
  }
#endif
}



